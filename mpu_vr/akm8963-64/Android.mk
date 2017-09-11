ifeq (${TARGET_ARCH},arm64)
ifneq ($(TARGET_SIMULATOR),true)

BOARD_SENSOR_COMPASS_AK09911 := true

LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(BOARD_SENSOR_COMPASS_AK09911)), true)
AKMD_DEVICE_TYPE := 9911
else
AKMD_DEVICE_TYPE := 8963
endif
AKMD_ACC_AOT := yes

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)

LOCAL_SRC_FILES:= \
	AKMD_Driver.c \
	DispMessage.c \
	FileIO.c \
	Measure.c \
	main.c \
	misc.c

LOCAL_MODULE  := akmd

ifeq ($(AKMD_DEVICE_TYPE), 8963)
LOCAL_SRC_FILES += FST_AK8963.c
LOCAL_CFLAGS  += -DAKMD_FOR_AK8963
#LOCAL_LDFLAGS += $(LOCAL_PATH)/libAK8963wPGplus.a
LOCAL_STATIC_LIBRARIES := libAK8963wPGplus

else ifeq ($(AKMD_DEVICE_TYPE), 8975)
LOCAL_SRC_FILES += FST_AK8975.c
LOCAL_CFLAGS  += -DAKMD_FOR_AK8975
#LOCAL_LDFLAGS += -L$(LOCAL_PATH)/$(SMARTCOMPASS_LIB) -lAK8975wPGplus
LOCAL_STATIC_LIBRARIES := libAK8975wPGplus

else ifeq ($(AKMD_DEVICE_TYPE), 9911)
LOCAL_SRC_FILES += FST_AK09911.c
LOCAL_CFLAGS  += -DAKMD_FOR_AK09911
LOCAL_CFLAGS  += -DAKMD_AK099XX
#LOCAL_LDFLAGS += -L$(LOCAL_PATH)/$(SMARTCOMPASS_LIB) -lAK09911wPGplus
LOCAL_STATIC_LIBRARIES := libAK09911wPGplus

else
$(error AKMD_DEVICE_TYPE is not defined)
endif


### Acceleration Sensors 
ifeq ($(AKMD_ACC_AOT),yes)
#LOCAL_CFLAGS += -DAKMD_ACC_EXTERNAL
#LOCAL_SRC_FILES += Acc_aot.c
else

ifeq ($(AKMD_SENSOR_ACC),adxl346)
LOCAL_CFLAGS += -DAKMD_ACC_ADXL346
LOCAL_SRC_FILES += Acc_adxl34x.c

else ifeq ($(AKMD_SENSOR_ACC),kxtf9)
LOCAL_CFLAGS += -DAKMD_ACC_KXTF9
LOCAL_SRC_FILES += Acc_kxtf9.c

else ifeq ($(AKMD_SENSOR_ACC),dummy)
LOCAL_CFLAGS += -DAKMD_ACC_DUMMY
LOCAL_SRC_FILES += Acc_dummy.c

else
$(error AKMD_SENSOR_ACC is not defined)
endif

endif

LOCAL_CFLAGS += -Wall -Wextra
#LOCAL_CFLAGS += -DENABLE_AKMDEBUG=1
#LOCAL_CFLAGS += -DAKM_LOG_ENABLE

LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := false 
LOCAL_SHARED_LIBRARIES := libc libm libutils libcutils liblog
include $(BUILD_EXECUTABLE)

ifeq ($(AKMD_DEVICE_TYPE), 8963)
include $(CLEAR_VARS)
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE := libAK8963wPGplus
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES_arm64 := libAK8963wPGplus.a
include $(BUILD_PREBUILT)

else ifeq ($(AKMD_DEVICE_TYPE), 8975)
include $(CLEAR_VARS)
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE := libAK8975wPGplus
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES_arm64 := libAK8975wPGplus.a
include $(BUILD_PREBUILT)

else ifeq ($(AKMD_DEVICE_TYPE), 9911)
include $(CLEAR_VARS)
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE := libAK09911wPGplus
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES_arm64 := libAK09911wPGplus.a
include $(BUILD_PREBUILT)

else
$(error AKMD_SENSOR_ACC is not defined)
endif

endif  # TARGET_SIMULATOR != true
endif  # TARGET_ARCH := arm64 