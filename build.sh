PROJECT_PATH=$(pwd)
INCLUDE_PATH="$PROJECT_PATH/include"
LIBS_PATH="$PROJECT_PATH/lib"
JAVA_SRC_PATH="$PROJECT_PATH/java"

mkdir -p $INCLUDE_PATH
mkdir -p $LIBS_PATH

# Check for SDK
if [[ -z "${SDK_PATH}" ]]; then
    echo "[-] No SDK path defined."
    exit 1
fi

if [ ! -e "${SDK_PATH}/android.jar" ]; then
    echo "[-] Android jar file not found."
    exit 1
fi

# Stage 1: Compile JNI java source code
echo "[*] Compiling java code ..."
javac -source 1.8 -target 1.8 -classpath $SDK_PATH/android.jar -d $LIBS_PATH $JAVA_SRC_PATH/com/android/blueping/BlueManager.java
if [ $? -ne 0 ]; then
    echo "[-] Java compilation failed."
    exit 1
fi

# Stage 2: Generate JNI header
echo "[*] Generating JNI header ..."
javah -d $INCLUDE_PATH -classpath $LIBS_PATH com.android.blueping.BlueManager
if [ $? -ne 0 ]; then
    echo "[-] JNI header generation failed."
    exit 1
fi

# Step 3: Build native code using ndk-build
echo "[*] Building native code..."

ndk-build clean NDK_PROJECT_PATH=$PROJECT_PATH \
    APP_BUILD_SCRIPT=$PROJECT_PATH/Android.mk \
    NDK_APPLICATION_MK=$PROJECT_PATH/Application.mk
ndk-build NDK_PROJECT_PATH=$PROJECT_PATH \
    APP_BUILD_SCRIPT=$PROJECT_PATH/Android.mk \
    NDK_APPLICATION_MK=$PROJECT_PATH/Application.mk

if [ $? -ne 0 ]; then
    echo "[-] NDK build failed."
    exit 1
fi

echo "[+] Build complete!"