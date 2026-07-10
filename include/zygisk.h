/*
 * Zygisk API - Public Header
 * Compatible with Magisk Zygisk and ZygiskNext
 * Based on official Zygisk API documentation
 */

#ifndef ZYGISK_API_H
#define ZYGISK_API_H

#include <jni.h>
#include <android/log.h>

#define LOG_TAG "ZygiskModule"

// Logging macros
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace zygisk {

// Forward declarations
class Api;
class ModuleBase;

// API version
constexpr int ZYGISK_API_VERSION = 1;

// App specialize arguments (standard Zygisk API)
struct AppSpecializeArgs {
    jstring package_name;      // Package name of the app
    jint uid;                  // UID of the app
    jint runtime_flags;        // Runtime flags
    jstring process_name;      // Process name (optional)
};

// System server specialize arguments
struct SystemServerSpecializeArgs {
    // Currently unused for modules
};

// Module base class - implement this in your module
class ModuleBase {
public:
    virtual ~ModuleBase() = default;
    
    /**
     * Called when the module is loaded into Zygote
     * @param api API instance for communication with Zygisk
     * @param env JNI environment
     */
    virtual void onLoad(Api *api, JNIEnv *env) {}
    
    /**
     * Called before app process is specialized
     * @param args App specialize arguments
     */
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    
    /**
     * Called after app process is specialized
     * @param args App specialize arguments
     */
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    
    /**
     * Called before system server is specialized
     */
    virtual void preSystemServerSpecialize(SystemServerSpecializeArgs *args) {}
    
    /**
     * Called after system server is specialized
     */
    virtual void postSystemServerSpecialize(const SystemServerSpecializeArgs *args) {}
};

// API class for interacting with Zygisk
class Api {
public:
    virtual ~Api() = default;
    
    /**
     * Get the Zygisk API version
     */
    virtual int getApiVersion() = 0;
    
    /**
     * Hook JNI native methods
     * @param env JNI environment
     * @param clazz Java class to hook
     * @param method Method name to hook
     * @param signature Method signature
     * @param hook Hook function pointer
     * @param backup Backup function pointer for original
     */
    virtual void hookJniNativeMethods(
        JNIEnv *env,
        jclass clazz,
        const char *method,
        const char *signature,
        void *hook,
        void **backup
    ) = 0;
    
    /**
     * Hook PLT/GOT entries
     * @param lib_name Library name
     * @param symbol Symbol name
     * @param hook Hook function pointer
     * @param backup Backup function pointer for original
     */
    virtual void hookPLT(
        const char *lib_name,
        const char *symbol,
        void *hook,
        void **backup
    ) = 0;
};

} // namespace zygisk

// Registration macro - entry point for Zygisk
#define REGISTER_ZYGISK_MODULE(cls) \
    extern "C" __attribute__((visibility("default"))) \
    zygisk::ModuleBase* ZygiskModule_Init() { \
        return new cls(); \
    }

#endif // ZYGISK_API_H