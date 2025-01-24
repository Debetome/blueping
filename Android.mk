LOCAL_PATH := $(call my-dir)
JAVA_HOME := $(value JAVA_HOME)

include $(CLEAR_VARS)

LOCAL_MODULE	:=	blueping
LOCAL_C_INCLUDES :=	$(LOCAL_PATH)/include \
					$(JAVA_HOME)/include \
					$(JAVA_HOME)/include/linux \
					$(NDK)/sources/cxx-stl/llvm-libc++/include \
					$(NDK)/sysroot/usr/include
					
LOCAL_LDFLAGS	:=	-L$(JAVA_HOME)/lib \
					-L$(NDK)/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a \
					-L$(NDK)/sysroot/usr/lib/arm-linux-androideabi

LOCAL_SRC_FILES	:=	src/main.cpp \
					src/ThreadPool.cpp \
					src/BluePing.cpp

LOCAL_LDLIBS := -llog -landroid -lstdc++
LOCAL_CPPFLAGS := -std=c++11 -fexceptions -D__ANDROID_API__=19

include $(BUILD_EXECUTABLE)
