PROJECT_PATH=$(pwd)

echo "[*] Building native code ..."

$NDK_PATH/build/ndk-build clean NDK_PROJECT_PATH=$PROJECT_PATH \
    APP_BUILD_SCRIPT=$PROJECT_PATH/Android.mk \
    NDK_APPLICATION_MK=$PROJECT_PATH/Application.mk
$NDK_PATH/build/ndk-build NDK_PROJECT_PATH=$PROJECT_PATH \
    APP_BUILD_SCRIPT=$PROJECT_PATH/Android.mk \
    NDK_APPLICATION_MK=$PROJECT_PATH/Application.mk

if [ $? -ne 0 ]; then
    echo "[-] NDK build failed."
    exit 1
fi

echo "[+] Build complete!"

if [[ " $* " == *" push "* ]]; then
    echo "[*] Pushing project into the phone ..."

    adb push $PROJECT_PATH/obj/local/armeabi-v7a/blueping /data/local/tmp
    if [ $? -ne 0 ]; then
        echo "[-] Failed to push project into the phone."
        exit 1
    fi

    echo "[+] Project pushed!"
fi