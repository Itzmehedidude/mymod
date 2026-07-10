#include <jni.h>
#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <vector>

// Zygisk API headers
#include <zygisk.h>
#include <logging.h>

using namespace zygisk;

#define MODULE_TAG "MyModule"
#define CONFIG_PATH "/data/local/tmp/mymod/app.txt"
#define LOG_PATH "/data/local/tmp/mymod/detected.txt"

class MyModule : public ModuleBase {
public:
    void onLoad(api::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        
        LOGD("%s: onLoad called - module loaded successfully!", MODULE_TAG);
        LOGD("%s: Zygisk API version: %d", MODULE_TAG, api->getApiVersion());
    }

    void preAppSpecialize(api::AppSpecializeArgs *args) override {
        const char *packageName = env->GetStringUTFChars(args->package_name, nullptr);
        
        LOGD("%s: preAppSpecialize for package: %s", MODULE_TAG, packageName);
        LOGD("%s: Process UID: %d, PID: %d", MODULE_TAG, args->uid, getpid());
        
        std::string targetPackage = readTargetPackage();
        
        if (!targetPackage.empty()) {
            LOGD("%s: Target package from file: %s", MODULE_TAG, targetPackage.c_str());
            
            if (packageName == targetPackage) {
                LOGD("🟢 %s: TARGET APP DETECTED: %s", MODULE_TAG, packageName);
                LOGD("🟢 %s: UID: %d, PID: %d", MODULE_TAG, args->uid, getpid());
                
                writeDetectionLog(packageName, args->uid);
                
                // Store detection state for postAppSpecialize
                targetDetected = true;
            } else {
                LOGD("❌ %s: Not target: %s (expected: %s)", 
                     MODULE_TAG, packageName, targetPackage.c_str());
                targetDetected = false;
            }
        } else {
            LOGD("⚠️ %s: No target package configured - watching all apps", MODULE_TAG);
            targetDetected = false;
        }
        
        env->ReleaseStringUTFChars(args->package_name, packageName);
    }

    void postAppSpecialize(const api::AppSpecializeArgs *args) override {
        const char *packageName = env->GetStringUTFChars(args->package_name, nullptr);
        
        LOGD("%s: postAppSpecialize for: %s", MODULE_TAG, packageName);
        LOGD("%s: Now inside app process - PID: %d", MODULE_TAG, getpid());
        
        if (targetDetected) {
            LOGD("✅ %s: Target app %s is running!", MODULE_TAG, packageName);
            // Here we can add more functionality in later steps
        }
        
        env->ReleaseStringUTFChars(args->package_name, packageName);
    }

private:
    api::Api *api;
    JNIEnv *env;
    bool targetDetected = false;

    std::string readTargetPackage() {
        std::ifstream file(CONFIG_PATH);
        
        if (!file.is_open()) {
            LOGD("⚠️ %s: Could not open config file: %s", MODULE_TAG, CONFIG_PATH);
            return "";
        }
        
        std::string line;
        if (std::getline(file, line)) {
            size_t start = line.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return "";
            
            size_t end = line.find_last_not_of(" \t\n\r");
            if (end == std::string::npos) return "";
            
            return line.substr(start, end - start + 1);
        }
        
        return "";
    }

    void writeDetectionLog(const char *packageName, uid_t uid) {
        // Ensure directory exists
        std::string dirPath = "/data/local/tmp/mymod";
        mkdir(dirPath.c_str(), 0755);
        
        std::ofstream file(LOG_PATH);
        
        if (file.is_open()) {
            time_t now = time(nullptr);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            file << "=== DETECTED TARGET APP ===" << std::endl;
            file << "Package: " << packageName << std::endl;
            file << "PID: " << getpid() << std::endl;
            file << "UID: " << uid << std::endl;
            file << "Timestamp: " << timeStr << std::endl;
            file << "===========================" << std::endl;
            file.close();
            
            LOGD("✅ %s: Wrote detection log to: %s", MODULE_TAG, LOG_PATH);
            LOGD("✅ %s: Detection log contents:", MODULE_TAG);
            LOGD("✅ %s: Package: %s", MODULE_TAG, packageName);
            LOGD("✅ %s: PID: %d", MODULE_TAG, getpid());
        } else {
            LOGD("⚠️ %s: Could not write detection log to: %s", MODULE_TAG, LOG_PATH);
        }
    }
};

// Zygisk entry point
REGISTER_ZYGISK_MODULE(MyModule)