#include "CrashHandler.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <android/log.h>

// Static member definitions
JavaVM* CrashHandler::s_javaVM = nullptr;
jmethodID CrashHandler::s_crashCallback = nullptr;
bool CrashHandler::s_installed = false;
pid_t CrashHandler::s_pid = 0;

void CrashHandler::install() {
    if (s_installed) return;
    
    s_pid = getpid();
    s_installed = true;
    
    // Set up signal handlers for common crash signals
    struct sigaction sa;
    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    
    // Install handlers for critical signals
    sigaction(SIGSEGV, &sa, nullptr);  // Segmentation fault
    sigaction(SIGABRT, &sa, nullptr);  // Abort signal
    sigaction(SIGBUS, &sa, nullptr);   // Bus error
    sigaction(SIGFPE, &sa, nullptr);   // Floating point exception
    sigaction(SIGILL, &sa, nullptr);   // Illegal instruction
    
    logMessage("INFO", "Crash handler installed for PID %d", s_pid);
}

void CrashHandler::uninstall() {
    if (!s_installed) return;
    
    // Reset signal handlers to default
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    
    s_installed = false;
    logMessage("INFO", "Crash handler uninstalled");
}

void CrashHandler::logMessage(const char* level, const char* message, ...) {
    va_list args;
    va_start(args, message);
    __android_log_vprint(ANDROID_LOG_INFO, CRASH_HANDLER_TAG, message, args);
    va_end(args);
}

void CrashHandler::logException(const char* function, const char* file, int line, const char* message) {
    logMessage("ERROR", "Exception in %s at %s:%d - %s", function, file, line, message);
    logStackTrace();
}

void CrashHandler::logMemoryInfo() {
    // Log basic memory information
    logMessage("INFO", "Memory info logging not implemented for Android");
}

void CrashHandler::logStackTrace() {
    std::string stackTrace = getStackTraceString();
    logMessage("INFO", "Stack trace:\n%s", stackTrace.c_str());
}

void CrashHandler::setJavaVM(JavaVM* vm) {
    s_javaVM = vm;
}

void CrashHandler::setCrashCallback(jmethodID callback) {
    s_crashCallback = callback;
}

void CrashHandler::signalHandler(int sig, siginfo_t* info, void* context) {
    char signalName[32];
    switch (sig) {
        case SIGSEGV: strcpy(signalName, "SIGSEGV"); break;
        case SIGABRT: strcpy(signalName, "SIGABRT"); break;
        case SIGBUS:  strcpy(signalName, "SIGBUS");  break;
        case SIGFPE:  strcpy(signalName, "SIGFPE");  break;
        case SIGILL:  strcpy(signalName, "SIGILL");  break;
        default:      strcpy(signalName, "UNKNOWN"); break;
    }
    
    logMessage("FATAL", "Signal %s received (code: %d, addr: %p)", 
               signalName, info->si_code, info->si_addr);
    
    // Log memory information
    logMemoryInfo();
    
    // Log stack trace
    logStackTrace();
    
    // Write crash report to file
    std::string crashInfo = getStackTraceString();
    writeCrashReport(crashInfo);
    
    // Call Java crash callback if available
    if (s_javaVM && s_crashCallback) {
        JNIEnv* env;
        if (s_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
            jstring crashInfoStr = env->NewStringUTF(crashInfo.c_str());
            env->CallStaticVoidMethod(env->FindClass("com/dorflix/app/util/CrashLogger"), 
                                   s_crashCallback, crashInfoStr);
            env->DeleteLocalRef(crashInfoStr);
        }
    }
    
    // Cleanup resources before terminating
    cleanupResources();
    
    // Restore default signal handler and re-raise signal
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, nullptr);
    raise(sig);
}

std::string CrashHandler::getStackTraceString() {
    // Android NDK doesn't have backtrace functions, so we'll return a simple message
    // In a real implementation, you might use Android's libcorkscrew or other alternatives
    return "Stack trace not available on Android NDK (backtrace functions not supported)";
}

void CrashHandler::writeCrashReport(const std::string& crashInfo) {
    // Note: File operations are simplified for Android NDK compatibility
    // In a production app, you might use Android's file APIs or JNI to write files
    logMessage("INFO", "Crash report would be written to file (file operations simplified for Android NDK)");
    logMessage("INFO", "Crash details: %s", crashInfo.c_str());
}

void CrashHandler::cleanupResources() {
    // Clean up any allocated resources
    // This is called during crash handling, so be careful not to allocate memory
    
    // Close file descriptors, release locks, etc.
    // Avoid complex operations that might cause additional crashes
}
