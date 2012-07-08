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

#define LOG_TAG "CameraHardware"
#include <utils/Log.h>

#include "S3C6410CameraHardware.h"
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include "CannedJpeg.h"
//-------------------------------------------------------------------------------
// function to check camera interface, if found return its type, else return none
//-------------------------------------------------------------------------------
static int probe_camera()
{
	int fd ;
#if 0
	fd = open("/dev/video2", O_RDWR | O_SYNC);
	
	LOGD("%s :open /dev/video2 return %d  errno=%x\n",LOG_TAG,fd,errno);
	
	if(fd <= 0)
	{
		fd = open("/dev/video0", O_RDWR | O_SYNC);
		LOGD("%s :open /dev/video0 return %d  errno=%x\n",LOG_TAG,fd,errno);
		if (fd <=0)
		{
			return CAMTYPE_NONE;
		}
		else
		{	close(fd);
			return CAMTYPE_CMOS;
		}
	}
	else
	{
		close(fd);
		return CAMTYPE_USB;
	}
#endif
	//		return CAMTYPE_CMOS;
	struct stat st;     
    
    	if (-1 == stat ("/dev/video2", &st)) {   
		if(-1 == stat("/dev/video0",&st))
			return CAMTYPE_NONE;
		else
			return CAMTYPE_CMOS;

   	 }
	else
		return CAMTYPE_USB;	
    
      
   

}



namespace android {

CameraHardware::CameraHardware()
                  : mParameters(),
                    mPreviewHeap(0),
                    mRawHeap(0),
                    mOv965xCamera(0),
                    mPreviewFrameSize(0),
                    mNotifyCb(0),
                    mDataCb(0),
                    mDataCbTimestamp(0),
                    mCallbackCookie(0),
                    mMsgEnabled(0),
                    mCurrentPreviewFrame(0)
{
	mOv965xCamera = 0;
	mUSBCamera = 0;
	LOGE("%s, starting probe camera!", LOG_TAG);
	mCamType = probe_camera();

    LOGE("%s, end probe camera! cametype:%d", LOG_TAG, mCamType);
	//mCamType =CAMTYPE_USB ;
    initDefaultParameters();
}

void CameraHardware::initDefaultParameters()
{
    CameraParameters p;

	if(mCamType == CAMTYPE_CMOS){
	
    		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "640x480");//hnmsky
    		p.setPreviewSize(640, 480);

    		p.setPreviewFrameRate(15);
		p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_RGB565);
 	 	p.set(CameraParameters::KEY_ROTATION, 90);
	}
	else if(mCamType == CAMTYPE_USB){

    		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, "320x240");
    		p.setPreviewSize(320, 240);
    		p.setPreviewFrameRate(5);//hnmsky
    		//p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);
		p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_RGB565);
 	 	//  p.set(CameraParameters::KEY_ROTATION, 90);
    }
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, "320x240");
    p.setPictureSize(320, 240);
    p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

    if (setParameters(p) != NO_ERROR) {
        LOGE("Failed to set default parameters?!");
    }


    int preview_width, preview_height;
    mParameters.getPreviewSize(&preview_width, &preview_height);
	if(mCamType == CAMTYPE_CMOS)
	{
		LOGD("mOv965xCamera");
		//mOv965xCamera = new Ov965xCamera(preview_width, preview_height);V4L2_PIX_FMT_YUYV
		mOv965xCamera->setSize(preview_width, preview_height,V4L2_PIX_FMT_RGB565);//V4L2_PIX_FMT_RGB565//hnmsky
	}
	else if(mCamType == CAMTYPE_USB)
	{

		LOGD("mUSBCamera");
		mUSBCamera->setSize(preview_width, preview_height,V4L2_PIX_FMT_YUYV);
	}

	
}

void CameraHardware::initHeapLocked()
{
    // Create raw heap.
    int picture_width, picture_height;
    mParameters.getPictureSize(&picture_width, &picture_height);
    mRawHeap = new MemoryHeapBase(picture_width * picture_height * 2);

    int preview_width, preview_height;
    mParameters.getPreviewSize(&preview_width, &preview_height);
    LOGD("initHeapLocked: preview size=%dx%d, mPreiviewFrameSize:%d", preview_width, preview_height,mPreviewFrameSize);

    // Note that we enforce yuv420sp in setParameters().
    //int how_big = preview_width * preview_height * 3 / 2;
	int how_big = preview_width * preview_height*2;

    // If we are being reinitialized to the same size as before, no
    // work needs to be done.
    if (how_big == mPreviewFrameSize)
        return;

    mPreviewFrameSize = how_big;

    // Make a new mmap'ed heap that can be shared across processes.
    // use code below to test with pmem
    mPreviewHeap = new MemoryHeapBase(mPreviewFrameSize * kBufferCount);
    // Make an IMemory for each frame so that we can reuse them in callbacks.
    for (int i = 0; i < kBufferCount; i++) {
        mBuffers[i] = new MemoryBase(mPreviewHeap, i * mPreviewFrameSize, mPreviewFrameSize);
    }

	if(mUSBCamera != 0)
	{
		LOGD("delete mUSBCamera");
		delete mUSBCamera;
	 	mUSBCamera = 0;
	}
	else if(mCamType == CAMTYPE_USB)
	{
		LOGD("new mUSBCamera");
		mUSBCamera = new USBCamera(preview_width, preview_height);
	}
    // Recreate the fake camera to reflect the current size.
    if(mOv965xCamera != 0)
	{
		LOGD("delete mOv965xCamera");
    	delete mOv965xCamera;
    	mOv965xCamera = 0; // paranoia
	}
	else if(mCamType == CAMTYPE_CMOS)
	{
		LOGD("new mOv965xCamera");
		mOv965xCamera = new Ov965xCamera(preview_width, preview_height);
		//mOv965xCamera->setSize(preview_width, preview_height,V4L2_PIX_FMT_RGB565);
	}

}

CameraHardware::~CameraHardware()
{
	if(mOv965xCamera != 0)
	{
    	delete mOv965xCamera;
    	mOv965xCamera = 0; // paranoia
	}
	if(mUSBCamera != 0)
	{
		delete mUSBCamera;
	 	mUSBCamera = 0;
	}
}

sp<IMemoryHeap> CameraHardware::getPreviewHeap() const
{
    return mPreviewHeap;
}

sp<IMemoryHeap> CameraHardware::getRawHeap() const
{
    return mRawHeap;
}

void CameraHardware::setCallbacks(notify_callback notify_cb,
                                      data_callback data_cb,
                                      data_callback_timestamp data_cb_timestamp,
                                      void* user)
{
    Mutex::Autolock lock(mLock);
    mNotifyCb = notify_cb;
    mDataCb = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mCallbackCookie = user;
}

void CameraHardware::enableMsgType(int32_t msgType)
{
    Mutex::Autolock lock(mLock);
    mMsgEnabled |= msgType;
}

void CameraHardware::disableMsgType(int32_t msgType)
{
    Mutex::Autolock lock(mLock);
    mMsgEnabled &= ~msgType;
}

bool CameraHardware::msgTypeEnabled(int32_t msgType)
{
    Mutex::Autolock lock(mLock);
    return (mMsgEnabled & msgType);
}

// ---------------------------------------------------------------------------
int CameraHardware::previewThread()
{
    mLock.lock();
        // the attributes below can change under our feet...

        int previewFrameRate = mParameters.getPreviewFrameRate();

        // Find the offset within the heap of the current buffer.
        ssize_t offset = mCurrentPreviewFrame * mPreviewFrameSize;

        sp<MemoryHeapBase> heap = mPreviewHeap;

        // this assumes the internal state of fake camera doesn't change
        // (or is thread safe)
        Ov965xCamera* Ov965xCamera = mOv965xCamera;
	USBCamera* USBCamera = mUSBCamera;
        sp<MemoryBase> buffer = mBuffers[mCurrentPreviewFrame];

    mLock.unlock();

    // TODO: here check all the conditions that could go wrong
    if (buffer != 0) {
        // Calculate how long to wait between frames.
        int delay = (int)(1000000.0f / float(previewFrameRate));

        // This is always valid, even if the client died -- the memory
        // is still mapped in our process.
        void *base = heap->base();

        // Fill the current frame with the fake camera.
        uint8_t *frame = ((uint8_t *)base) + offset;
        //Ov965xCamera->getNextFrameAsYuv420(frame);

		if(mCamType == CAMTYPE_CMOS) 
       			Ov965xCamera->getNextFrameAsRgb565((uint16_t*) frame);
		else if (mCamType == CAMTYPE_USB)
			USBCamera->getNextFrameAsRgb565((uint16_t *)frame);

        //LOGV("previewThread: generated frame to buffer %d", mCurrentPreviewFrame);

        // Notify the client of a new frame.
        if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
            mDataCb(CAMERA_MSG_PREVIEW_FRAME, buffer, mCallbackCookie);

        // Advance the buffer pointer.
        mCurrentPreviewFrame = (mCurrentPreviewFrame + 1) % kBufferCount;

        // Wait for it...
        //usleep(delay);
    }

    return NO_ERROR;
}

status_t CameraHardware::startPreview()
{
    Mutex::Autolock lock(mLock);
    if (mPreviewThread != 0) {
        // already running
        return INVALID_OPERATION;
    }
   	if(mCamType == CAMTYPE_CMOS) 
      			mOv965xCamera->startPreview();
	else if (mCamType == CAMTYPE_USB)
			mUSBCamera->startPreview();
    
    mPreviewThread = new PreviewThread(this);
    return NO_ERROR;
}

void CameraHardware::stopPreview()
{
    sp<PreviewThread> previewThread;

    { // scope for the lock
        Mutex::Autolock lock(mLock);
        previewThread = mPreviewThread;
    }

    // don't hold the lock while waiting for the thread to quit
    if (previewThread != 0) {
        previewThread->requestExitAndWait();
    }

    Mutex::Autolock lock(mLock);
    mPreviewThread.clear();
	if(mCamType == CAMTYPE_CMOS) 
      		mOv965xCamera->stopPreview();
	else if (mCamType == CAMTYPE_USB)
		mUSBCamera->stopPreview();

}

bool CameraHardware::previewEnabled() {
    return mPreviewThread != 0;
}

status_t CameraHardware::startRecording()
{
    return UNKNOWN_ERROR;
}

void CameraHardware::stopRecording()
{
}

bool CameraHardware::recordingEnabled()
{
    return false;
}

void CameraHardware::releaseRecordingFrame(const sp<IMemory>& mem)
{
}

// ---------------------------------------------------------------------------

int CameraHardware::beginAutoFocusThread(void *cookie)
{
    CameraHardware *c = (CameraHardware *)cookie;
    return c->autoFocusThread();
}

int CameraHardware::autoFocusThread()
{
    if (mMsgEnabled & CAMERA_MSG_FOCUS)
        mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
    return NO_ERROR;
}

status_t CameraHardware::autoFocus()
{
    Mutex::Autolock lock(mLock);
    if (createThread(beginAutoFocusThread, this) == false)
        return UNKNOWN_ERROR;
    return NO_ERROR;
}

status_t CameraHardware::cancelAutoFocus()
{
    return NO_ERROR;
}

/*static*/ int CameraHardware::beginPictureThread(void *cookie)
{
    CameraHardware *c = (CameraHardware *)cookie;
    return c->pictureThread();
}

int CameraHardware::pictureThread()
{
    if (mMsgEnabled & CAMERA_MSG_SHUTTER)
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);

    if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
        //FIXME: use a canned YUV image!
        // In the meantime just make another fake camera picture.
        int w, h;
        mParameters.getPictureSize(&w, &h);
        sp<MemoryBase> mem = new MemoryBase(mRawHeap, 0, w * h * 2);
		
        //Ov965xCamera cam(w, h);
        //cam.getNextFrameAsYuv420((uint8_t *)mRawHeap->base());

		if(mCamType == CAMTYPE_CMOS)
		{
			Ov965xCamera cam(w, h);
        	cam.getNextFrameAsYuv420((uint8_t *)mRawHeap->base());
		}		
		else if (mCamType == CAMTYPE_USB)
		{
			//USBCamera cam1(w, h);
        	//cam1.getNextFrameAsYuv420((uint16_t *)mRawHeap->base());		
		}
        mDataCb(CAMERA_MSG_RAW_IMAGE, mem, mCallbackCookie);
    }

    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
        sp<MemoryHeapBase> heap = new MemoryHeapBase(kCannedJpegSize);
        sp<MemoryBase> mem = new MemoryBase(heap, 0, kCannedJpegSize);
        memcpy(heap->base(), kCannedJpeg, kCannedJpegSize);
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mem, mCallbackCookie);
    }
    return NO_ERROR;
}

status_t CameraHardware::takePicture()
{
    stopPreview();
    if (createThread(beginPictureThread, this) == false)
        return UNKNOWN_ERROR;
    return NO_ERROR;
}

status_t CameraHardware::cancelPicture()
{
    return NO_ERROR;
}

status_t CameraHardware::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    AutoMutex lock(&mLock);
    if (mOv965xCamera != 0) {
        mOv965xCamera->dump(fd);
        mParameters.dump(fd, args);
        snprintf(buffer, 255, " preview frame(%d), size (%d), running(%s)\n", mCurrentPreviewFrame, mPreviewFrameSize, mPreviewRunning?"true": "false");
        result.append(buffer);
		 write(fd, result.string(), result.size());
    } else {
        result.append("No camera client yet.\n");
		 write(fd, result.string(), result.size());
    }
   

	if (mUSBCamera!= 0) {
        mUSBCamera->dump(fd);
        mParameters.dump(fd, args);
        snprintf(buffer, 255, " preview frame(%d), size (%d), running(%s)\n", mCurrentPreviewFrame, mPreviewFrameSize, mPreviewRunning?"true": "false");
        result.append(buffer);
		 write(fd, result.string(), result.size());
    } else {
			
        result.append("No camera client yet.\n");
	 	write(fd, result.string(), result.size());    
	}
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t CameraHardware::setParameters(const CameraParameters& params)
{
    Mutex::Autolock lock(mLock);
    // XXX verify params

    if (strcmp(params.getPreviewFormat(),
//        CameraParameters::PIXEL_FORMAT_YUV420SP) != 0) {
	CameraParameters::PIXEL_FORMAT_RGB565) != 0) {
        LOGE("Only yuv420sp preview is supported");
    //    return -1;//hnmsky
    }

    if (strcmp(params.getPictureFormat(),
        CameraParameters::PIXEL_FORMAT_JPEG) != 0) {
        LOGE("Only jpeg still pictures are supported");
        return -1;
    }

    int w, h;
    params.getPictureSize(&w, &h);
    if (w != kCannedJpegWidth && h != kCannedJpegHeight) {
        LOGE("Still picture size must be size of canned JPEG (%dx%d)",
             kCannedJpegWidth, kCannedJpegHeight);
        return -1;
    }

    mParameters = params;
    initHeapLocked();

    return NO_ERROR;
}

CameraParameters CameraHardware::getParameters() const
{
    Mutex::Autolock lock(mLock);
    return mParameters;
}

status_t CameraHardware::sendCommand(int32_t command, int32_t arg1,
                                         int32_t arg2)
{
    return BAD_VALUE;
}

void CameraHardware::release()
{
}

sp<CameraHardwareInterface> CameraHardware::createInstance()
{
    return new CameraHardware();
}

static CameraInfo sCameraInfo[] = {
    {
        CAMERA_FACING_BACK,
        90,  /* orientation */
    }
};

extern "C" int HAL_getNumberOfCameras()
{
    return sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
}

extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo)
{
    memcpy(cameraInfo, &sCameraInfo[cameraId], sizeof(CameraInfo));
}

extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId)
{
    return CameraHardware::createInstance();
}

}; // namespace android
