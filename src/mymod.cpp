/*
 * My Zygisk Module - Step 1
 * Basic app detection with config file
 */

#include "../include/zygisk.h"
#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <cstring>

#define MODULE_TAG "MyModule"
#define CONFIG_DIR "/data/local/tmp/mymod"
#define CONFIG_FILE "/data/local/tmp/mymod/app.txt"
#define LOG_FILE "/data/local/tmp/mymod/detected.txt"

using namespace zygisk;

class MyModule : public ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        
        LOGD("========================================");
        LOGD("MyModule: v1.0.0 loaded successfully!");
        LOGD("MyModule: Zygisk API version: %d", api->getApiVersion());
        LOGD("MyModule: Process: %d", getpid());
        LOGD("========================================");
        
        // Ensure config directory exists
        mkdir(CONFIG_DIR, 0755);
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args || !args->package_name) {
            LOGD("MyModule: preAppSpecialize called with null args");
            return;
        }
        
        // Get package name
        const char *packageName = env->GetStringUTFChars(args->package_name, nullptr);
        if (!packageName) {
            LOGD("MyModule: Could not get package name");
            return;
        }
        
        LOGD("========================================");
        LOGD("MyModule: preAppSpecialize called");
        LOGD("MyModule: Package: %s", packageName);
        LOGD("MyModule: UID: %d", args->uid);
        LOGD("MyModule: PID: %d", getpid());
        
        // Read target package from config
        std::string targetPackage = readConfigFile();
        
        if (!targetPackage.empty()) {
            LOGD("MyModule: Target package from config: %s", targetPackage.c_str());
            
            // Check if this is our target
            if (strcmp(packageName, targetPackage.c_str()) == 0) {
                LOGD("🟢🟢🟢 TARGET APP DETECTED! 🟢🟢🟢");
                LOGD("🟢 Package: %s", packageName);
                LOGD("🟢 UID: %d", args->uid);
                LOGD("🟢 PID: %d", getpid());
                
                // Write detection log
                writeDetectionLog(packageName, args->uid);
                targetDetected = true;
            } else {
                LOGD("❌ Not target: %s (expected: %s)", 
                     packageName, targetPackage.c_str());
                targetDetected = false;
            }
        } else {
            LOGD("⚠️ No target package configured in %s", CONFIG_FILE);
            LOGD("⚠️ Watching all apps (no filtering)");
            targetDetected = false;
        }
        
        env->ReleaseStringUTFChars(args->package_name, packageName);
        LOGD("========================================");
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!args || !args->package_name) {
            LOGD("MyModule: postAppSpecialize called with null args");
            return;
        }
        
        const char *packageName = env->GetStringUTFChars(args->package_name, nullptr);
        if (!packageName) return;
        
        LOGD("MyModule: postAppSpecialize for: %s", packageName);
        LOGD("MyModule: Running in app process - PID: %d", getpid());
        
        if (targetDetected) {
            LOGD("✅✅✅ Target app %s is running!", packageName);
            LOGD("✅✅✅ Module is active in the app process");
        }
        
        env->ReleaseStringUTFChars(args->package_name, packageName);
    }

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool targetDetected = false;

    std::string readConfigFile() {
        std::ifstream file(CONFIG_FILE);
        
        if (!file.is_open()) {
            LOGD("⚠️ Could not open config file: %s", CONFIG_FILE);
            LOGD("⚠️ Create file with: echo 'com.example.app' > %s", CONFIG_FILE);
            return "";
        }
        
        std::string line;
        if (std::getline(file, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return "";
            
            size_t end = line.find_last_not_of(" \t\n\r");
            if (end == std::string::npos) return "";
            
            std::string result = line.substr(start, end - start + 1);
            
            // Remove comments
            size_t commentPos = result.find('#');
            if (commentPos != std::string::npos) {
                result = result.substr(0, commentPos);
                // Trim again
                start = result.find_first_not_of(" \t\n\r");
                if (start == std::string::npos) return "";
                end = result.find_last_not_of(" \t\n\r");
                if (end == std::string::npos) return "";
                result = result.substr(start, end - start + 1);
            }
            
            return result;
        }
        
        return "";
    }

    void writeDetectionLog(const char *packageName, uid_t uid) {
        // Ensure directory exists
        mkdir(CONFIG_DIR, 0755);
        
        std::ofstream file(LOG_FILE);
        
        if (!file.is_open()) {
            LOGD("⚠️ Could not write detection log to: %s", LOG_FILE);
            return;
        }
        
        time_t now = time(nullptr);
        char timeStr[64];
        struct tm *tm_info = localtime(&now);
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
        
        file << "========================================" << std::endl;
        file << "         TARGET APP DETECTED           " << std::endl;
        file << "========================================" << std::endl;
        file << "Package:  " << packageName << std::endl;
        file << "PID:      " << getpid() << std::endl;
        file << "UID:      " << uid << std::endl;
        file << "Timestamp: " << timeStr << std::endl;
        file << "========================================" << std::endl;
        file.close();
        
        LOGD("✅ Detection log written to: %s", LOG_FILE);
        
        // Read back to verify
        std::ifstream checkFile(LOG_FILE);
        if (checkFile.is_open()) {
            std::string line;
            LOGD("✅ Detection log contents:");
            while (std::getline(checkFile, line)) {
                LOGD("   %s", line.c_str());
            }
            checkFile.close();
        }
    }
};

// Zygisk entry point - required for all modules
REGISTER_ZYGISK_MODULE(MyModule)