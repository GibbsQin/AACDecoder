LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
	
FAAD2_TOP := $(LOCAL_PATH)/faad2-2.8.8
include $(FAAD2_TOP)/libfaad/Android.mk

LOCAL_LDLIBS := -llog

