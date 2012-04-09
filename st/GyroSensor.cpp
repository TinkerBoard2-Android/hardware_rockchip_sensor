/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <utils/BitSet.h>
#include <cutils/properties.h>
#include <linux/l3g4200d.h>

#include "GyroSensor.h"
#include "MEMSAlgLib_Fusion.h"

#define FETCH_FULL_EVENT_BEFORE_RETURN 1
#define IGNORE_EVENT_TIME 350000000

#define INPUT_SYSFS_PATH_GYRO "/sys/class/i2c-adapter/i2c-0/0-0068/"

/*****************************************************************************/

GyroSensor::GyroSensor()
    : SensorBase(GY_DEVICE_NAME, "gyro"),
      mEnabled(0),
      mInputReader(32)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_GY;
    mPendingEvent.type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvent.gyro.status = SENSOR_STATUS_ACCURACY_HIGH;
    memset(mPendingEvent.data, 0x00, sizeof(mPendingEvent.data));
	int err = 0;
    err = open_device();
	err = err<0 ? -errno : 0;
	if(err)
	{
		LOGD("%s:%s\n",__func__,strerror(-err));
		return;
	}
	
    int flags = 0;
    if (!ioctl(dev_fd, L3G4200D_IOCTL_GET_ENABLE, &flags)) {
        if (flags)  {
            mEnabled = 1;
        }
    }

    if (!mEnabled) {
        close_device();
    }
}

GyroSensor::~GyroSensor() {
	if (mEnabled) {
        enable(0, 0);
    }
}

int GyroSensor::setInitialState() {
    struct input_absinfo absinfo_x;
    struct input_absinfo absinfo_y;
    struct input_absinfo absinfo_z;
    float value;
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_GYRO_X), &absinfo_x) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_GYRO_X), &absinfo_y) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_GYRO_X), &absinfo_z)) {
        value = absinfo_x.value;
        mPendingEvent.data[0] = value * CONVERT_GYRO_X;
        value = absinfo_x.value;
        mPendingEvent.data[1] = value * CONVERT_GYRO_Y;
        value = absinfo_x.value;
        mPendingEvent.data[2] = value * CONVERT_GYRO_Z;
        mHasPendingEvent = true;
    }
    return 0;
}


int GyroSensor::enable(int32_t, int en)
{
	int flags = en ? 1 : 0;
	int err = 0;
	if (flags != mEnabled) {
		if (!mEnabled) {
			open_device();
		}
		err = ioctl(dev_fd, L3G4200D_IOCTL_SET_ENABLE, &flags);
		err = err<0 ? -errno : 0;
		LOGE_IF(err, "LIGHTSENSOR_IOCTL_ENABLE failed (%s)", strerror(-err));
		if (!err) {
			mEnabled = en ? 1 : 0;
			if (en) {
				setInitialState();
			}
		}
		if (!mEnabled) {
			close_device();
		}
	}
	return err;
}


bool GyroSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int GyroSensor::setDelay(int32_t handle, int64_t ns)
{
    if (ns < 0)
        return -EINVAL;

    int delay = ns / 1000000;
    if (ioctl(dev_fd, L3G4200D_IOCTL_SET_DELAY, &delay)) {
        return -errno;
    }
    return 0;
}

int GyroSensor::readEvents(sensors_event_t* data, int count)
{
    //LOGD("*******************Gyro readEvents");
    //LOGD("count: %d, mHasPendingEvent: %d", count, mHasPendingEvent);
    static int64_t prev_time;
    int64_t time;

    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;
    float gyrox = 0, gyroy = 0, gyroz = 0;

#if FETCH_FULL_EVENT_BEFORE_RETURN
again:
#endif
    while (count && mInputReader.readEvent(&event)) {

        //LOGD("GyroSensor::readEvents() coutn = %d, event->value = %f", count, event->value);
        int type = event->type;
        if (type == EV_REL) {
            float value = event->value;
            if (event->code == EVENT_TYPE_GYRO_X) {
                gyrox = value;
		mPendingEvent.data[0] = value * CONVERT_GYRO_X;
            } else if (event->code == EVENT_TYPE_GYRO_Y) {
                gyroy = value;
		mPendingEvent.data[1] = value * CONVERT_GYRO_Y;
            } else if (event->code == EVENT_TYPE_GYRO_Z) {
                gyroz = value;
		mPendingEvent.data[2] = value * CONVERT_GYRO_Z;
            }

	    //LOGD("mPendingEvent: %f, %f, %f", mPendingEvent.data[0], mPendingEvent.data[1], mPendingEvent.data[2]);
        }else if (type == EV_SYN) {
           
            time = timevalToNano(event->time);
            if(mEnabled) {
                mPendingEvent.timestamp = time;

                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }

        }else {
            LOGE("GyroSensor: unknown event (type=%d, code=%d)", type, event->code);
        }
        mInputReader.next();
    }

#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnabled == 1) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif

    return numEventReceived;
}
