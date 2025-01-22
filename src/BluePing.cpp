#include "BluePing.h"
#include "com_android_blueping_BlueManager.h"
#include <iostream>
#include <vector>
#include <string>

BluePing::BluePing(JNIEnv* env, jobject blueManagerObj)
    : env(env), blueManagerObj(blueManagerObj) {

    jclass blueManagerClass = env->GetObjectClass(blueManagerObj);

    // Retrieve the method IDs from the Java class
    connectMethodID = env->GetMethodID(blueManagerClass, "connect", "(Ljava/lang/String;)Z");
    sendPingMethodID = env->GetMethodID(blueManagerClass, "sendPing", "([B)V");
    receiveResponseMethodID = env->GetMethodID(blueManagerClass, "receiveResponse", "()[B");
    disconnectMethodID = env->GetMethodID(blueManagerClass, "disconnect", "()V");
}

BluePing::~BluePing() {
    disconnect();
}

bool BluePing::connect(const std::string& deviceAddress) {
    jstring jDeviceAddress = env->NewStringUTF(deviceAddress.c_str());
    jboolean result = env->CallBooleanMethod(blueManagerObj, connectMethodID, jDeviceAddress);
    env->DeleteLocalRef(jDeviceAddress);
    return result == JNI_TRUE;
}

void BluePing::sendPing(const std::vector<uint8_t>& data) {
    jbyteArray jData = env->NewByteArray(data.size());
    env->SetByteArrayRegion(jData, 0, data.size(), reinterpret_cast<const jbyte*>(data.data()));
    env->CallVoidMethod(blueManagerObj, sendPingMethodID, jData);
    env->DeleteLocalRef(jData);
}

std::vector<uint8_t> BluePing::receiveResponse() {
    jbyteArray jResponse = (jbyteArray)env->CallObjectMethod(blueManagerObj, receiveResponseMethodID);
    jsize responseLength = env->GetArrayLength(jResponse);
    std::vector<uint8_t> response(responseLength);
    env->GetByteArrayRegion(jResponse, 0, responseLength, reinterpret_cast<jbyte*>(response.data()));
    env->DeleteLocalRef(jResponse);
    return response;
}

void BluePing::disconnect() {
    env->CallVoidMethod(blueManagerObj, disconnectMethodID);
}
