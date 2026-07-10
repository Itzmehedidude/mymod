# My Zygisk Module

Step 1: Basic app detection with ZygiskNext and ZN Linker.

## Features
- ✅ Reads target app package from config file
- ✅ Detects when target app starts
- ✅ Logs detailed information about the app
- ✅ Writes detection confirmation to file
- ✅ Works with ZygiskNext's ZN Linker

## Installation

1. Download the latest release from [Releases](https://github.com/itzmehedidude/mymod/releases)
2. Flash the module in KernelSU/Magisk
3. Reboot device
4. Configure target package:
   ```bash
   adb shell "echo 'com.android.settings' > /data/local/tmp/mymod/app.txt"
   adb shell chmod 644 /data/local/tmp/mymod/app.txt