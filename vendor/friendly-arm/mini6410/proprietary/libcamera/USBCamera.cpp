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

#define LOG_TAG "USBCamera"
#include <utils/Log.h>


#include <string.h>
#include <stdlib.h>
#include <utils/String8.h>

#include <getopt.h>    
#include <fcntl.h>    
#include <unistd.h>  
#include <errno.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>  
#include <asm/types.h>  
#include <linux/videodev2.h>  
#include <linux/fb.h>  
#include <fcntl.h>
#include "USBCamera.h"
#define MAX_BUFF 4
#define CLEAR(x) memset (&(x), 0, sizeof (x)) 
#define dev_name  "/dev/video0"//hnmsky
#define CHECK(return_value)                                          \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail. errno: %s, \n",           \
             __func__, __LINE__, strerror(errno));      \
        return -1;                                                   \
    }


#define CHECK_PTR(return_value)                                      \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail, errno: %s, \n",           \
             __func__,__LINE__, strerror(errno));       \
        return NULL;                                                 \
    }
struct V4L_Buffer {   
    void * start;   
    size_t length;   
};   

typedef struct V4L_Buffer V4BUFFER;
    
//static char * dev_name = NULL;   
//static int fd = -1;   
V4BUFFER *buffers = NULL;   
static unsigned int n_buffers = 0;   
static int time_in_sec_capture=5;      
static struct fb_var_screeninfo vinfo;   
static struct fb_fix_screeninfo finfo;   
static char *fbp=NULL;   
static long screensize=0;   
    
static void errno_exit (const char * s)   
{   
    fprintf (stderr, "%s error %d, %s\n",s, errno, strerror (errno));   
    exit (EXIT_FAILURE);   
}   
    
static int xioctl (int fd,int request,void * arg)   
{   
    int r;   
    do r = ioctl (fd, request, arg);   
    while (-1 == r && EINTR == errno);   
    return r;   
}   
    
inline int clip(int value, int min, int max) {   
    return (value > max ? max : value < min ? min : value);   
  }   
  

static void convert_yuyv_to_rgb565(uint8_t *yuv, uint8_t *rgb, int width, int height)
{
//	if(yuv == NULL || rgb == NULL)
//		return;
//    if(width <= 0 || height <= 0)
//		return;

 	uint8_t *in=yuv;   
    int istride=width*2;   
    int x,y,j;   
    int y0,u,y1,v,r,g,b;   
    long location=0;   
    
    for ( y = 0; y < height ; ++y) {   
        for (j = 0, x=0; j < width * 2 ; j += 4,x +=2) {   
     
          y0 = in[j];   
          u = in[j + 1] - 128;                   
          y1 = in[j + 2];           
          v = in[j + 3] - 128;           
    
          r = (298 * y0 + 409 * v + 128) >> 8;   
          g = (298 * y0 - 100 * u - 208 * v + 128) >> 8;   
          b = (298 * y0 + 516 * u + 128) >> 8;   
           
          rgb[ location + 0] = ((clip(g, 0, 255)&0x1c)<< 3 ) | (clip(b, 0, 255) >> 3 );   
          rgb[ location + 1] = ((clip(r, 0, 255) & 0xf8) ) | (clip(g, 0, 255)>> 5);   
    
          r = (298 * y1 + 409 * v + 128) >> 8;   
          g = (298 * y1 - 100 * u - 208 * v + 128) >> 8;   
          b = (298 * y1 + 516 * u + 128) >> 8;   
    
          rgb[ location + 2] = ((clip(g, 0, 255)&0x1c)<< 3 ) | ( clip(b, 0, 255)  >> 3 );   
          rgb[ location + 3] = ((clip(r, 0, 255) & 0xf8) ) | ( clip(g, 0, 255)>> 5);   
          
		  location +=4;
          }   
        in +=istride;   
       }   
}

   
  

 
namespace android {

// TODO: All this rgb to yuv should probably be in a util class.

// TODO: I think something is wrong in this class because the shadow is kBlue
// and the square color should alternate between kRed and kGreen. However on the
// emulator screen these are all shades of gray. Y seems ok but the U and V are
// probably not.


int USBCamera::readFrame(uint8_t* rgb, int width, int height)   
{   
    struct v4l2_buffer buf;   
    unsigned int i;   
    int ret;
    CLEAR (buf);   
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    buf.memory = V4L2_MEMORY_MMAP;   
    
    if (-1 == ioctl (mFd, VIDIOC_DQBUF, &buf)) {   
        switch (errno) {   
        case EAGAIN:   
        return 0;   
        case EIO:       
    
           
        default:   
            LOGE("%s VIDIOC_DQBUF fail",__func__);   
        }   
    }   
    
    //assert (buf.index < n_buffers);   
    LOGD("v4l2_pix_format->field(%d)\n", buf.field);   
    //assert (buf.field ==V4L2_FIELD_NONE);   
   // process_image (buffers[buf.index].start); 

	convert_yuyv_to_rgb565(((uint8_t *)buffers[buf.index].start), rgb, width, height);  
    if (-1 == ioctl (mFd, VIDIOC_QBUF, &buf))   
        LOGE("%s VIDIOC_QBUF fail",__func__);   
    
    return 1;   
} 
static void run (uint8_t* rgb, int width, int height)   
{   
    unsigned int count;   
    int frames;   
    frames = 1; 
#if 0
    while (frames-- > 0) {   
        for (;;) {   
            fd_set fds;   
            struct timeval tv;   
            int r;   
            FD_ZERO (&fds);   
            FD_SET (fd, &fds);   
    
               
            tv.tv_sec = 2;   
            tv.tv_usec = 0;   
    
            r = select (fd + 1, &fds, NULL, NULL, &tv);   
    
            if (-1 == r) {   
                if (EINTR == errno)   
                    continue;   
                errno_exit ("select");   
            }   
    
            if (0 == r) {   
                fprintf (stderr, "select timeout\n");   
                exit (EXIT_FAILURE);   
            }   
    
            if (read_frame (rgb, width, height))   
                break;   
               
            }   
    }   
   #endif
}   
    
static void stop_capturing (void)   
{   
#if 0
       	enum v4l2_buf_type type;   
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))   
        errno_exit ("VIDIOC_STREAMOFF");
#endif   
}   
    
static void start_capturing (void)   
{   
#if 0
    unsigned int i;   
    enum v4l2_buf_type type;   
    
    for (i = 0; i < n_buffers; ++i) {   
        struct v4l2_buffer buf;   
        CLEAR (buf);   
    
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
        buf.memory = V4L2_MEMORY_MMAP;   
        buf.index = i;   
    
        if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))   
            errno_exit ("VIDIOC_QBUF");   
        }   
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    
    if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))   
        errno_exit ("VIDIOC_STREAMON");   
#endif  
}   


int USBCamera::init_mmap (void)   
{   
    struct v4l2_requestbuffers req;   
    int ret,i;
    struct v4l2_buffer buf;   
    
    memset(&req, 0, sizeof(struct v4l2_requestbuffers ));
    req.count = MAX_BUFF;   
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
    req.memory = V4L2_MEMORY_MMAP;   
    
    ret = ioctl (mFd, VIDIOC_REQBUFS, &req) ;   
    CHECK(ret);

    if (req.count < 4) {    //if (req.count < 2)   
        LOGE("%s Insufficient buffer memory on %s\n",__func__,dev_name);   
    }   
    
    buffers = new V4BUFFER[req.count];

    if (!buffers) {   
        LOGE("%s Out of memory\n",__func__);   
        //exit (EXIT_FAILURE);   
    }   
    
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {   
    
    
    memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   
        buf.memory = V4L2_MEMORY_MMAP;   
        buf.index = n_buffers;   
    
        ret = ioctl (mFd, VIDIOC_QUERYBUF, &buf);   
        CHECK(ret);
    
        buffers[n_buffers].length = buf.length;   
        buffers[n_buffers].start =mmap (NULL,buf.length,PROT_READ | PROT_WRITE ,MAP_SHARED,mFd, buf.m.offset);   
    
        if (MAP_FAILED == buffers[n_buffers].start)   
           LOGE("%s  mmap fail",__func__);   
    } 

    /*
   * Queue the buffers.
   */
  for (i = 0; i < MAX_BUFF; ++i) {
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.index = i;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(mFd, VIDIOC_QBUF, &buf);
    CHECK(ret);
    }
    return 1;
    
}   

  
int USBCamera::initDevice (void)   
{   
    struct v4l2_capability cap;   
    struct v4l2_cropcap cropcap;   
    struct v4l2_crop crop;   
    struct v4l2_format fmt; 
	struct v4l2_control ctrl;  
    unsigned int min;   
    int ret;
    
    memset(&cap, 0, sizeof(struct v4l2_capability));
    ret = ioctl(mFd, VIDIOC_QUERYCAP, &cap); 
  	CHECK(ret);
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {   
        LOGE("it is no video capture device\n");   
        return -1;   
    }   
    
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {   
        LOGE("it does not support streaming i/o\n",dev_name);   
    	return -1;
    }   
    
      /*
   * set format in
   */
  memset(&fmt, 0, sizeof(struct v4l2_format));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = mWidth;
  fmt.fmt.pix.height = mHeight;
  fmt.fmt.pix.pixelformat = mFmt;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  ret = ioctl(mFd, VIDIOC_S_FMT, &fmt);
    CHECK(ret);
	LOGD("USBCamea  width %d  height %d",mWidth,mHeight);

  if ((fmt.fmt.pix.width != mWidth) ||
      (fmt.fmt.pix.height != mHeight)) {
    LOGE( " format asked unavailable get width %d height %d \n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    /*
     * look the format is not part of the deal ???
     */
  }
#if 0
  /*
   * set framerate
   */
  struct v4l2_streamparm setfps;
  memset(&setfps, 0, sizeof(struct v4l2_streamparm));
  setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  setfps.parm.capture.timeperframe.numerator = 1;
  setfps.parm.capture.timeperframe.denominator = 15;//?
  ret = ioctl(mFd, VIDIOC_S_PARM, setfps);
   // CHECK(ret);
LOGE("%s setfps ret %d",__func__,ret);
#endif
 


    
 
	
	  
    
    init_mmap ();   
    return ret;
    
}   

int USBCamera::deinitDevice (void)   
{   
    unsigned int i;   
    
    for (i = 0; i < n_buffers; ++i) 
	{  
        if (-1 == munmap (buffers[i].start, buffers[i].length))   
            LOGE("%s unmap fail",__func__);
	//	return -1;
	}	
       
    
    delete [] buffers;   
    return 1;
}   
  
    
static void close_device (void)   
{   
    //if (-1 == close (fd))   
    //errno_exit ("close");   
    //fd = -1;   
   
}   
    
static void open_device (void)   
{  
#if 0	
    struct stat st;     
    
    if (-1 == stat (dev_name, &st)) {   
    fprintf (stderr, "Cannot identify '%s': %d, %s\n",dev_name, errno, strerror (errno));   
    exit (EXIT_FAILURE);   
    }   
    
    if (!S_ISCHR (st.st_mode)) {   
    fprintf (stderr, "%s is no device\n", dev_name);   
    exit (EXIT_FAILURE);   
    }   
    
    //open camera   
    mFd = open (dev_name, O_RDWR| O_NONBLOCK, 0);   
    
    if (-1 == mFd) {   
    LOGE("Cannot open '%s': %d, %s\n",dev_name, errno, strerror (errno));   
    //exit (EXIT_FAILURE);   
    }   
#endif
}

const int USBCamera::kRed;
const int USBCamera::kGreen;
const int USBCamera::kBlue;

USBCamera::USBCamera(int width, int height)
          : mTmpRgb16Buffer(0),mPreviewing(0)
{	
	 
	//open_device ();   
       //open camera   
    mFd = open (dev_name, O_RDWR| O_NONBLOCK, 0);   
    
    if (-1 == mFd) {   
    LOGE("Cannot open '%s'\n",dev_name); 
    } 
    //init_device (width, height);   
    
   // start_capturing (); 

//	setSize(width, height);

  
}

USBCamera::~USBCamera()
{
//	stop_capturing ();   
    
//    uninit_device ();   
    
//    close_device ();  
	if(mPreviewing){
		stopPreview();
	}
	deinitDevice();
	if(mFd != -1)
		close(mFd);

	     
}
int USBCamera::startPreview(void)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  	int ret;
	if(mPreviewing)
	{
		LOGW("Already previewing");
		return 1;
	}
	mPreviewing = 1;	
	initDevice();
  	ret = ioctl(mFd, VIDIOC_STREAMON, &type);
	CHECK(ret);
	return ret;
}
int USBCamera::stopPreview(void)
{
	  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  	int ret;
	if(!mPreviewing)
	{
		LOGE("No previewing");
		return -1;
	}
	mPreviewing = 0;
  	ret = ioctl(mFd, VIDIOC_STREAMOFF, &type);
	CHECK(ret);

	ret = deinitDevice();

	CHECK(ret);
	return ret;
}
void USBCamera::setSize(int width, int height, int fmt)
{
    mWidth = width;
    mHeight = height;
    mCounter = 0;
    mCheckX = 0;
    mCheckY = 0;
    mFmt = fmt;

    // This will cause it to be reallocated on the next call
    // to getNextFrameAsYuv420().
	//if (mTmpRgb16Buffer != 0)
	//{
    //   delete[] mTmpRgb16Buffer;// = new uint16_t[mWidth * mHeight];
	//	mTmpRgb16Buffer = NULL;
	//}
	
}

void USBCamera::getNextFrameAsRgb565(uint16_t *buffer)
{
    
   //	run((uint8_t*)buffer, mWidth, mHeight);
   //
#if 0
           for (;;) 
	   {   
            fd_set fds;   
            struct timeval tv;   
            int r;   
            FD_ZERO (&fds);   
            FD_SET (mFd, &fds);   
    
               
            tv.tv_sec = 2;   
            tv.tv_usec = 0;   
    
            r = select (mFd + 1, &fds, NULL, NULL, &tv);   
    
            if (-1 == r) {   
                if (EINTR == errno)   
                    continue;   
                LOGE("%s select fail",__func__);
	     	break;	
            }   
    
            if (0 == r) {   
                LOGE("select timeout\n");
	     	break;	
                //exit (EXIT_FAILURE);   
            }   
    		
            if (readFrame ((uint8_t*)buffer, mWidth, mHeight))   
                break;   
               
            }
#endif
	      int ret;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
	memset(&mEvent,0,sizeof(mEvent));
	mEvent.fd = mFd;
	mEvent.events = POLLIN | POLLERR;
    ret = poll(&mEvent, 1, 10000);
    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return;
    }

    if (ret == 0) {
        LOGE("ERR(%s):No data in 10 secs..\n", __func__);
        return;
    } 
    readFrame ((uint8_t*)buffer, mWidth, mHeight) ;  
 	
}
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

void USBCamera::drawSquare(uint16_t *dst, int x, int y, int size, int color, int shadow)
{
    int square_xstop, square_ystop, shadow_xstop, shadow_ystop;

    square_xstop = min(mWidth, x+size);
    square_ystop = min(mHeight, y+size);
    shadow_xstop = min(mWidth, x+size+(size/4));
    shadow_ystop = min(mHeight, y+size+(size/4));

    // Do the shadow.
    uint16_t *sh = &dst[(y+(size/4))*mWidth];
    for (int j = y + (size/4); j < shadow_ystop; j++) {
        for (int i = x + (size/4); i < shadow_xstop; i++) {
            sh[i] &= shadow;
        }
        sh += mWidth;
    }

    // Draw the square.
    uint16_t *sq = &dst[y*mWidth];
    for (int j = y; j < square_ystop; j++) {
        for (int i = x; i < square_xstop; i++) {
            sq[i] = color;
        }
        sq += mWidth;
    }
}

void USBCamera::drawCheckerboard(uint16_t *dst, int size)
{
    bool black = true;

    if((mCheckX/size)&1)
        black = false;
    if((mCheckY/size)&1)
        black = !black;

    int county = mCheckY%size;
    int checkxremainder = mCheckX%size;

    for(int y=0;y<mHeight;y++) {
        int countx = checkxremainder;
        bool current = black;
        for(int x=0;x<mWidth;x++) {
            dst[y*mWidth+x] = current?0:0xffff;
            if(countx++ >= size) {
                countx=0;
                current = !current;
            }
        }
        if(county++ >= size) {
            county=0;
            black = !black;
        }
    }
    mCheckX += 3;
    mCheckY++;
}


void USBCamera::dump(int fd) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, 255, " width x height (%d x %d), counter (%d), check x-y coordinate(%d, %d)\n", mWidth, mHeight, mCounter, mCheckX, mCheckY);
    result.append(buffer);
    ::write(fd, result.string(), result.size());
}


}; // namespace android
