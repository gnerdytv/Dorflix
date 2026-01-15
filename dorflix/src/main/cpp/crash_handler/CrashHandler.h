#pragma once

#include <jni.h>
#include <android/log.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <memory>

#define CRASH_HANDLER_TAG "DorflixCrashHandler"

/**
 * Native crash handler for C++ code in Dorflix app
 * Handles SIGSEGV, SIGABRT, and other fatal signals
 */
class CrashHandler {
public:
    static void install();
    static void uninstall();
    static void logMessage(const char* level, const char* message, ...);
    static void logException(const char* function, const char* file, int line, const char* message);
    static void logMemoryInfo();
    static void logStackTrace();
    static void setJavaVM(JavaVM* vm);
    static void setCrashCallback(jmethodID callback);
    
private:
    static void signalHandler(int sig, siginfo_t* info, void* context);
    static std::string getStackTraceString();
    static void writeCrashReport(const std::string& crashInfo);
    static void cleanupResources();
    
    static JavaVM* s_javaVM;
    static jmethodID s_crashCallback;
    static bool s_installed;
    static pid_t s_pid;
};
