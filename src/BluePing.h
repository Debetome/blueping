#ifndef BLUEPING_H
#define BLUEPING_H

#include <jni.h>
#include <vector>
#include <string>

class BluePing {
public:
    BluePing(JNIEnv* env, jobject blueManagerObj);
    
    bool connect(const std::string& deviceAddress);
    void sendPing(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receiveResponse();
    void disconnect();
    
    ~BluePing();

private:
    JNIEnv* env;
    jobject blueManagerObj;
    jmethodID connectMethodID;
    jmethodID sendPingMethodID;
    jmethodID receiveResponseMethodID;
    jmethodID disconnectMethodID;
};

#endif // BLUEPING_H
