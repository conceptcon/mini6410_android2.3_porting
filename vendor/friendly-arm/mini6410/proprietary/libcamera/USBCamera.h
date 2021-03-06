/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_USBCamera_H
#define ANDROID_HARDWARE_USBCamera_H

#include <sys/types.h>
#include <stdint.h>
#include <sys/poll.h>
namespace android {

/*
 * USBCamera is used in the CameraHardwareStub to provide a fake video feed
 * when the system does not have a camera in hardware.
 * The fake video is a moving black and white checkerboard background with a
 * bouncing gray square in the foreground.
 * This class is not thread-safe.
 *
 * TODO: Since the major methods provides a raw/uncompressed video feed, rename
 * this class to RawVideoSource.
 */

class USBCamera {
public:
    USBCamera(int width, int height);
    ~USBCamera();

    void setSize(int width, int height,int fmt);
    // Write to the fd a string representing the current state.
    void dump(int fd) const;
    void getNextFrameAsRgb565(uint16_t *buffer);
    int startPreview(void);
    int stopPreview(void);

    int initDevice(void);
    int deinitDevice(void);
    int init_mmap(void);   
	int readFrame(uint8_t* rgb, int width, int height);   
private:
    // TODO: remove the uint16_t buffer param everywhere since it is a field of
    // this class.


    void drawSquare(uint16_t *buffer, int x, int y, int size, int color, int shadow);
    void drawCheckerboard(uint16_t *buffer, int size);

    static const int kRed = 0xf800;
    static const int kGreen = 0x07c0;
    static const int kBlue = 0x003e;

    int         mWidth, mHeight;
	int			mFrameLen;
    int         mCounter;
    int         mCheckX, mCheckY;
    uint16_t    *mTmpRgb16Buffer;
    int		mFmt;
	int mPreviewing;
    int mFd;
    pollfd mEvent;
};

}; // namespace android

#endif // ANDROID_HARDWARE_USBCamera_H
