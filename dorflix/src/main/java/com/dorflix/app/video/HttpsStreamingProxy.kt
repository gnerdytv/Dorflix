package com.dorflix.app.video

import android.content.Context
import android.util.Log
import kotlinx.coroutines.*
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import okhttp3.*
import okhttp3.logging.HttpLoggingInterceptor
import okio.Buffer
import okio.BufferedSource
import okio.ForwardingSource
import okio.Source
import java.io.IOException
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/**
 * HTTPS Streaming Proxy for Dorflix
 *
 * Streams HTTPS video content to FFmpeg using Android's robust SSL/TLS
 * Creates a local HTTP server that proxies HTTPS requests through OkHttp
 */
class HttpsStreamingProxy private constructor(private val context: Context) {

    companion object {
        private const val TAG = "HttpsStreamingProxy"
        private const val BUFFER_SIZE = 8192
        private const val CONNECT_TIMEOUT = 30L
        private const val READ_TIMEOUT = 30L
        private const val WRITE_TIMEOUT = 30L

        @Volatile
        private var instance: HttpsStreamingProxy? = null

        fun getInstance(context: Context): HttpsStreamingProxy {
            return instance ?: synchronized(this) {
                instance ?: HttpsStreamingProxy(context.applicationContext).also { instance = it }
            }
        }
    }

    private val okHttpClient: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .connectTimeout(CONNECT_TIMEOUT, TimeUnit.SECONDS)
            .readTimeout(READ_TIMEOUT, TimeUnit.SECONDS)
            .writeTimeout(WRITE_TIMEOUT, TimeUnit.SECONDS)
            .addInterceptor(HttpLoggingInterceptor().apply {
                level = HttpLoggingInterceptor.Level.BASIC
            })
            .build()
    }

    private val activeStreams = ConcurrentHashMap<String, StreamSession>()
    private val serverJob: Job? = null
    private var serverSocket: ServerSocket? = null
    private var serverPort: Int = 0
    private val isServerRunning = AtomicBoolean(false)

    data class StreamSession(
        val httpsUrl: String,
        val localUrl: String,
        val response: Response? = null,
        val createdAt: Long = System.currentTimeMillis()
    )

    /**
     * Start the proxy server if not already running
     */
    private fun ensureServerRunning(): Int {
        if (isServerRunning.get()) {
            val port = serverPort
            Log.i(TAG, "Server already running on port $port")
            return port
        }

        synchronized(this) {
            if (isServerRunning.get()) {
                val port = serverPort
                Log.i(TAG, "Server already running on port $port (after sync)")
                return port
            }

            try {
                Log.i(TAG, "Creating new server socket...")
                serverSocket = ServerSocket(0) // Auto-assign port
                serverPort = serverSocket!!.localPort
                isServerRunning.set(true)

                Log.i(TAG, "✅ HTTPS Streaming Proxy created on port $serverPort")
                Log.i(TAG, "Starting connection acceptance coroutine...")

                // Start server coroutine
                CoroutineScope(Dispatchers.IO).launch {
                    Log.i(TAG, "Connection acceptance coroutine launched")
                    acceptConnections()
                }

                Log.i(TAG, "✅ Server startup complete, returning port $serverPort")
                return serverPort
            } catch (e: IOException) {
                Log.e(TAG, "❌ Failed to start proxy server", e)
                throw RuntimeException("Failed to start HTTPS proxy server", e)
            }
        }
    }

    /**
     * Accept incoming connections from FFmpeg - IMPROVED
     */
    private suspend fun acceptConnections() {
        val socket = serverSocket ?: run {
            Log.e(TAG, "Server socket is null, cannot accept connections")
            return
        }

        Log.i(TAG, "Starting connection acceptance loop on port $serverPort")

        while (isServerRunning.get()) {
            try {
                Log.d(TAG, "Waiting for client connection...")
                val clientSocket = withContext(Dispatchers.IO) {
                    socket.accept()
                }

                Log.i(TAG, "Accepted connection from: ${clientSocket.inetAddress.hostAddress}:${clientSocket.port}")

                // Handle each client connection in parallel
                CoroutineScope(Dispatchers.IO).launch {
                    handleClientConnection(clientSocket)
                }
            } catch (e: Exception) {
                if (isServerRunning.get()) {
                    Log.e(TAG, "Error accepting connection", e)
                } else {
                    Log.i(TAG, "Server stopped, exiting accept loop")
                }
                break
            }
        }
        Log.i(TAG, "Connection acceptance loop ended")
    }

    /**
     * Handle individual client connections (FFmpeg requests) - IMPROVED
     */
    private suspend fun handleClientConnection(clientSocket: Socket) {
        try {
            clientSocket.use { socket ->
                Log.d(TAG, "Processing connection from ${socket.inetAddress}")

                val input = socket.getInputStream()
                val output = socket.getOutputStream()

                // Read the full HTTP request with better parsing
                val requestBuffer = StringBuilder()
                val byteBuffer = ByteArray(4096)
                var totalRead = 0
                var headersComplete = false

                // Read request headers
                while (!headersComplete && totalRead < 16384) { // 16KB limit
                    val read = withContext(Dispatchers.IO) {
                        input.read(byteBuffer)
                    }

                    if (read == -1) {
                        Log.w(TAG, "Connection closed by client before headers complete")
                        return
                    }

                    val chunk = String(byteBuffer, 0, read, Charsets.UTF_8)
                    requestBuffer.append(chunk)
                    totalRead += read

                    // Check for end of headers (double CRLF)
                    if (requestBuffer.contains("\r\n\r\n")) {
                        headersComplete = true
                    }
                }

                if (!headersComplete) {
                    Log.w(TAG, "Request headers too large or malformed")
                    sendErrorResponse(output, 413, "Request Entity Too Large")
                    return
                }

                val requestText = requestBuffer.toString()
                Log.d(TAG, "Received request (${requestText.length} chars)")

                // Parse request line
                val lines = requestText.lines()
                if (lines.isEmpty()) {
                    Log.w(TAG, "Empty request")
                    sendErrorResponse(output, 400, "Bad Request")
                    return
                }

                val requestLine = lines[0].trim()
                Log.d(TAG, "Request line: '$requestLine'")

                // Parse: GET /path HTTP/1.1
                val parts = requestLine.split("\\s+".toRegex())
                if (parts.size < 3) {
                    Log.w(TAG, "Malformed request line: $requestLine")
                    sendErrorResponse(output, 400, "Bad Request")
                    return
                }

                val method = parts[0].uppercase()
                val path = parts[1].removePrefix("/")
                val version = parts[2]

                Log.d(TAG, "Parsed request: method=$method, path='$path', version=$version")

                if (method != "GET") {
                    Log.w(TAG, "Unsupported method: $method")
                    sendErrorResponse(output, 405, "Method Not Allowed")
                    return
                }

                // Find stream session
                val streamSession = activeStreams[path]
                if (streamSession == null) {
                    Log.w(TAG, "No stream session found for path: '$path'")
                    Log.d(TAG, "Available sessions: ${activeStreams.keys.joinToString(", ")}")
                    sendErrorResponse(output, 404, "Stream not found")
                    return
                }

                Log.i(TAG, "Found stream session for: ${streamSession.httpsUrl}")

                // Stream content with headers sent after getting Content-Length from HEAD request
                streamHttpsContent(output, streamSession.httpsUrl)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error handling client connection", e)
        }
    }

    /**
     * Send HTTP response headers to FFmpeg - FFmpeg 8.0 compatible
     */
    private fun sendHttpResponseHeaders(output: java.io.OutputStream, httpsUrl: String, contentLength: Long? = null) {
        val headers = buildString {
            append("HTTP/1.1 200 OK\r\n")
            append("Content-Type: video/mp4\r\n")
            if (contentLength != null && contentLength > 0) {
                append("Content-Length: $contentLength\r\n")
            }
            append("Accept-Ranges: bytes\r\n")
            append("Connection: keep-alive\r\n")
            append("Cache-Control: no-cache\r\n")
            append("Access-Control-Allow-Origin: *\r\n")
            append("\r\n")
        }

        Log.d(TAG, "Sending FFmpeg-compatible headers: ${headers.replace("\r\n", " | ")}")
        output.write(headers.toByteArray())
        output.flush()
    }

    /**
     * Send error response to FFmpeg
     */
    private fun sendErrorResponse(output: java.io.OutputStream, code: Int, message: String) {
        val response = """
            HTTP/1.1 $code $message
            Content-Type: text/plain
            Connection: close

            $message
        """.trimIndent()

        output.write(response.toByteArray())
        output.flush()
    }

    /**
     * Stream HTTPS content using OkHttp - FINAL VERSION WITH CONTENT-LENGTH
     */
    private suspend fun streamHttpsContent(output: java.io.OutputStream, httpsUrl: String) {
        try {
            Log.d(TAG, "Starting HTTPS streaming for: $httpsUrl")

            // First, get content length with HEAD request
            val headRequest = Request.Builder()
                .url(httpsUrl)
                .head() // HEAD request to get headers only
                .addHeader("User-Agent", "Dorflix/1.0")
                .build()

            val headResponse = withContext(Dispatchers.IO) {
                okHttpClient.newCall(headRequest).execute()
            }

            val contentLength = headResponse.use { resp ->
                if (!resp.isSuccessful) {
                    Log.e(TAG, "HEAD request failed: ${resp.code} ${resp.message}")
                    return
                }

                resp.header("Content-Length")?.toLongOrNull().also { length ->
                    Log.d(TAG, "HEAD request successful, Content-Length: $length")
                }
            }

            headResponse.close()

            if (contentLength == null || contentLength <= 0) {
                Log.e(TAG, "Could not determine content length")
                return
            }

            // Now send complete headers with content length
            Log.i(TAG, "📏 Sending complete headers with Content-Length: $contentLength")
            sendHttpResponseHeaders(output, httpsUrl, contentLength)

            // Now make GET request to stream the actual content
            val getRequest = Request.Builder()
                .url(httpsUrl)
                .addHeader("User-Agent", "Dorflix/1.0")
                .addHeader("Range", "bytes=0-") // Support partial content
                .build()

            val response = withContext(Dispatchers.IO) {
                okHttpClient.newCall(getRequest).execute()
            }

            response.use { resp ->
                Log.d(TAG, "GET response: ${resp.code} ${resp.message}")

                if (!resp.isSuccessful) {
                    Log.e(TAG, "GET request failed: ${resp.code} ${resp.message}")
                    val errorBody = resp.body?.string()
                    Log.e(TAG, "Error response body: $errorBody")
                    return
                }

                resp.body?.byteStream()?.use { input ->
                    val buffer = ByteArray(BUFFER_SIZE)
                    var totalBytesRead = 0L
                    var bytesRead: Int

                    Log.d(TAG, "Starting data streaming...")

                    while (input.read(buffer).also { bytesRead = it } != -1) {
                        if (bytesRead > 0) {
                            withContext(Dispatchers.IO) {
                                output.write(buffer, 0, bytesRead)
                                output.flush()
                            }

                            totalBytesRead += bytesRead

                            // Log progress every 100KB
                            if (totalBytesRead % 102400 == 0L) {
                                Log.d(TAG, "Streamed ${totalBytesRead / 1024} KB so far")
                            }
                        }

                        // Check if streaming should continue
                        if (!isServerRunning.get()) {
                            Log.i(TAG, "Server stopped, ending stream")
                            break
                        }
                    }

                    Log.i(TAG, "✅ Completed streaming ${totalBytesRead / 1024} KB (${totalBytesRead} bytes) for: $httpsUrl")
                } ?: run {
                    Log.e(TAG, "Response body is null")
                }
            }

        } catch (e: Exception) {
            Log.e(TAG, "Error streaming HTTPS content: ${e.javaClass.simpleName}: ${e.message}", e)
        }
    }

    /**
     * Create a streaming URL for HTTPS content
     */
    fun createStreamingUrl(httpsUrl: String): String {
        val port = ensureServerRunning()

        // Generate unique stream ID
        val streamId = generateStreamId(httpsUrl)

        // Create stream session
        val localUrl = "http://127.0.0.1:$port/$streamId"
        val session = StreamSession(httpsUrl, localUrl)
        activeStreams[streamId] = session

        Log.i(TAG, "Created streaming URL: $localUrl for HTTPS: $httpsUrl")

        // Clean up old sessions periodically
        cleanupOldSessions()

        return localUrl
    }

    /**
     * Generate unique stream ID from URL
     */
    private fun generateStreamId(url: String): String {
        return url.hashCode().toString().replace("-", "") + "_" + System.currentTimeMillis()
    }

    /**
     * Clean up old stream sessions
     */
    private fun cleanupOldSessions() {
        val cutoffTime = System.currentTimeMillis() - (5 * 60 * 1000) // 5 minutes ago
        activeStreams.entries.removeIf { (_, session) ->
            session.createdAt < cutoffTime
        }
    }

    /**
     * Read line from input stream
     */
    private fun readLine(input: java.io.InputStream): String? {
        val buffer = StringBuilder()
        var c: Int

        while (input.read().also { c = it } != -1) {
            if (c == '\n'.code) break
            if (c != '\r'.code) buffer.append(c.toChar())
        }

        return if (buffer.isEmpty()) null else buffer.toString()
    }

    /**
     * Shutdown the proxy server
     */
    fun shutdown() {
        Log.i(TAG, "Shutting down HTTPS Streaming Proxy")

        isServerRunning.set(false)

        try {
            serverSocket?.close()
        } catch (e: Exception) {
            Log.e(TAG, "Error closing server socket", e)
        }

        activeStreams.clear()
        serverSocket = null
        serverPort = 0
    }

    /**
     * Get streaming statistics
     */
    fun getStats(): Map<String, Any> {
        return mapOf(
            "server_running" to isServerRunning.get(),
            "server_port" to serverPort,
            "active_streams" to activeStreams.size,
            "streams" to activeStreams.values.map { it.httpsUrl }
        )
    }
}