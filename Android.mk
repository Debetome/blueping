LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := blueping
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
					$(NDK)/sources/cxx-stl/llvm-libc++/include \
					$(NDK)/sysroot/usr/include
					
LOCAL_LDFLAGS 	:= 	-L$(JAVA_HOME)/lib/server \
					-L$(NDK)/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a \
					-L$(NDK)/sysroot/usr/lib/arm-linux-androideabi

LOCAL_SRC_FILES := src/main.cpp \
                   src/ThreadPool.cpp \
				   src/BluePing.cpp

LOCAL_LDLIBS := -llog -landroid -lstdc++ -ljvm -ljavacore
LOCAL_CPPFLAGS := -std=c++11 -fexceptions -D__ANDROID_API__=19

include $(BUILD_EXECUTABLE)
