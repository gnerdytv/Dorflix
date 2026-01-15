package com.dorflix.app.video

/**
 * Memory manager for video processing using C++ backend
 * Provides efficient memory pool management for video operations
 */
class MemoryManager {
    
    // Load the native library
    companion object {
        init {
            try {
                System.loadLibrary("dorflix-native")
            } catch (e: UnsatisfiedLinkError) {
                // Handle native library loading failure
                throw RuntimeException("Failed to load native library", e)
            }
        }
    }
    
    // Native methods
    private external fun nativeCreateMemoryPool(poolSize: Int): Long
    private external fun nativeDestroyMemoryPool(poolHandle: Long)
    private external fun nativeAllocate(poolHandle: Long, size: Int): Long
    private external fun nativeFree(poolHandle: Long, ptr: Long)
    private external fun nativeGetPoolSize(poolHandle: Long): Int
    private external fun nativeGetUsedSize(poolHandle: Long): Int
    
    private var poolHandle: Long = 0
    
    /**
     * Create a memory pool with the specified size
     * @param poolSize Size of the memory pool in bytes
     */
    fun createMemoryPool(poolSize: Int) {
        poolHandle = nativeCreateMemoryPool(poolSize)
    }
    
    /**
     * Allocate a block of memory from the pool
     * @param size Size of memory block to allocate in bytes
     * @return Pointer to allocated memory block, or 0 if allocation failed
     */
    fun allocate(size: Int): Long {
        return nativeAllocate(poolHandle, size)
    }
    
    /**
     * Free a previously allocated memory block
     * @param ptr Pointer to the memory block to free
     */
    fun free(ptr: Long) {
        if (ptr != 0L) {
            nativeFree(poolHandle, ptr)
        }
    }
    
    /**
     * Get the total size of the memory pool
     * @return Total pool size in bytes
     */
    fun getPoolSize(): Int {
        return nativeGetPoolSize(poolHandle)
    }
    
    /**
     * Get the currently used size of the memory pool
     * @return Used memory size in bytes
     */
    fun getUsedSize(): Int {
        return nativeGetUsedSize(poolHandle)
    }
    
    /**
     * Release the memory pool resources
     */
    fun release() {
        if (poolHandle != 0L) {
            nativeDestroyMemoryPool(poolHandle)
            poolHandle = 0
        }
    }
    
    /**
     * Clean up when the object is garbage collected
     */
    protected fun finalize() {
        if (poolHandle != 0L) {
            release()
        }
    }
}
