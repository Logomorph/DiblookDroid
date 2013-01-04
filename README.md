DiblookDroid
============

Instructions:

1. Download and install Android SDK and NDK

1.1. Using SDK Manager, install the following:
1.1.1. Tools
1.1.2. Android 4.2
1.1.3. Android 4.1.2
1.1.4. Android 4.0.3
1.1.5. Android 4.0
1.1.6. Extras (Emphasis on USB driver, if you're using a physical device)

1.2. extract the NDK somewhere

1.3. Notes:
1.3.1. A good way to check if the USB driver for debugging works is to go to <SDK_PATH>/platform-tools and run
"adb devices". If your device is connected in USB Debugging mode and the driver is installed properly, the device
will be listed

2. Install Eclipse
2.1. Install CDT
2.2. Install Android plugin with NDK support
2.3. Set the NDK path in the plugin

3.Set up an emulator using SDK version 14 (Android 4.0) and up (on windows, <SDK_PATH>/AVD Manager.exe),
or connect an android device that meets these requirements, with USB debug enabled

3.1. Notes:
3.1.1. The app should work on devices running versions older than 4.0, but I don't like the ui on the older versions that
much, so I locked it to 4.0+
3.1.2 If you're running an emulator, it would be a good idea to run it on ubuntu, because it seems to run much better

4. Load the project in Eclipse

5. Build and run the project as an android application and select the device/AVD to run on