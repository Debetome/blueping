#include <iostream>
#include <jni.h>
#include "ThreadPool.h"
#include "BluePing.h"

// JNI_OnLoad function: This is automatically called when the native library is loaded.
JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void* reserved) {
    JNIEnv* env = nullptr;

    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        std::cerr << "Failed to get the environment using GetEnv()" << std::endl;
        return JNI_ERR;
    }

    // Initialize any required classes or JNI objects here.
    return JNI_VERSION_1_6;
}

void sendPing(void* data) {
    BluePing* bluePing = static_cast<BluePing*>(data);
    uint8_t dataToSend[600] = {0x01}; // Fixed 600-byte payload

    try {
        bluePing->sendPing(std::vector<uint8_t>(dataToSend, dataToSend + sizeof(dataToSend)));
        std::cout << "Ping sent successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending ping: " << e.what() << std::endl;
    }
}

void receiveResponse(void* data) {
    BluePing* bluePing = static_cast<BluePing*>(data);
    try {
        std::cout << "Receiving response..." << std::endl;
        std::vector<uint8_t> responseVec = bluePing->receiveResponse();
        if (!responseVec.empty()) {
            std::cout << "Received response: ";
            for (size_t i = 0; i < 10 && i < responseVec.size(); ++i) {
                std::cout << std::hex << static_cast<int>(responseVec[i]) << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "No response received." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error receiving response: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <Bluetooth Address> <Number of Threads>" << std::endl;
        return 1;
    }

    const char* deviceAddress = argv[1];
    int threadCount = std::stoi(argv[2]);

    // Retrieve the existing JVM (Android will manage this for us)
    JavaVM* jvm;
    JNIEnv* env;

    if (jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr) != JNI_OK) {
        std::cerr << "Failed to attach current thread to JVM." << std::endl;
        return 1;
    }

    // Locate the BlueManager Java class
    jclass blueManagerClass = env->FindClass("com/android/blueping/BlueManager");
    if (!blueManagerClass) {
        std::cerr << "Could not find BlueManager class." << std::endl;
        return 1;
    }

    // Create an instance of BlueManager
    jmethodID constructor = env->GetMethodID(blueManagerClass, "<init>", "()V");
    if (!constructor) {
        std::cerr << "Could not find BlueManager constructor." << std::endl;
        return 1;
    }

    jobject blueManagerObj = env->NewObject(blueManagerClass, constructor);
    if (!blueManagerObj) {
        std::cerr << "Failed to create BlueManager instance." << std::endl;
        return 1;
    }

    // Use BluePing to interact with BlueManager
    try {
        BluePing bluePing(env, blueManagerObj);
        if (!bluePing.connect(deviceAddress)) {
            std::cerr << "Failed to connect to device: " << deviceAddress << std::endl;
            return 1;
        }

        // Launch the ThreadPool for sending/receiving
        ThreadPool pool(threadCount);
        for (int i = 0; i < threadCount; i++) {
            pool.addJob({sendPing, &bluePing});
            pool.addJob({receiveResponse, &bluePing});
        }

        std::cout << "Press Enter to exit and disconnect ..." << std::endl;
        std::cin.get();

        bluePing.disconnect();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Detach the thread from the JVM
    jvm->DetachCurrentThread();
    return 0;
}
