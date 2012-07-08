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

#define LOG_TAG "Ov965xCamera"
#include <utils/Log.h>


#include <string.h>
#include <stdlib.h>
#include <utils/String8.h>

#include <fcntl.h>

#include <linux/videodev2.h>
#include "Ov965xCamera.h"
#define MAX_BUFFERS 4

#define DEV_NAME "/dev/video0"
struct fimc_buffer {
    void    *start;
    size_t  length;
};
static    struct fimc_buffer m_buffers_c[MAX_BUFFERS];
static    struct pollfd   m_events_c;
namespace android {

// TODO: All this rgb to yuv should probably be in a util class.

// TODO: I think something is wrong in this class because the shadow is kBlue
// and the square color should alternate between kRed and kGreen. However on the
// emulator screen these are all shades of gray. Y seems ok but the U and V are
// probably not.
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
static int tables_initialized = 0;
uint8_t *gYTable, *gCbTable, *gCrTable;

static int fimc_v4l2_enum_fmt(int fp, unsigned int fmt);
static int fimc_v4l2_querybuf(int fp, struct fimc_buffer *buffers, enum v4l2_buf_type type, int nr_frames);
static int fimc_v4l2_querycap(int fp);
static int fimc_poll(struct pollfd *events);
static int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture);
static int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs);
static int fimc_v4l2_streamon(int fp);
static int fimc_v4l2_qbuf(int fp, int index);
static int fimc_v4l2_dqbuf(int fp);

static int get_pixel_depth(unsigned int fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_NV12:
        depth = 12;
        break;
   // case V4L2_PIX_FMT_NV12T:
    //    depth = 12;
   //     break;
    case V4L2_PIX_FMT_NV21:
        depth = 12;
        break;
    case V4L2_PIX_FMT_YUV420:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    }

    return depth;
}
static int init_device(int fd,int width, int height)
{
	int ret;
	int nr;
	unsigned int fmt;
	ret = fimc_v4l2_querycap(fd);
	CHECK(ret);

	//fmt = V4L2_PIX_FMT_YUYV;
	fmt = V4L2_PIX_FMT_RGB565; 
	ret = fimc_v4l2_enum_fmt(fd,fmt);
	CHECK(ret);

	
	ret = fimc_v4l2_s_fmt(fd,width,height,fmt,0);
	CHECK(ret);

	ret = fimc_v4l2_reqbufs(fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS);//
	CHECK(ret);

	nr = ret;
	if(nr > 0)
	{

		ret = fimc_v4l2_querybuf(fd,m_buffers_c,V4L2_BUF_TYPE_VIDEO_CAPTURE,nr);
		CHECK(ret);

		int i;
		for(i=0; i<nr; i++)
		{
			
		}
	}

    	ret = fimc_v4l2_streamon(fd);
    	CHECK(ret);
	return 0;
	


}

static int fimc_poll(struct pollfd *events)
{
    int ret;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
    ret = poll(events, 1, 10000);
    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return ret;
    }

    if (ret == 0) {
        LOGE("ERR(%s):No data in 10 secs..\n", __func__);
        return ret;
    }

    return ret;
}



static int fimc_v4l2_querycap(int fp)
{
    struct v4l2_capability cap;
    int ret = 0;

    ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("ERR(%s):no capture devices\n", __func__);
        return -1;
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("ERR(%s):no steaming devices\n", __func__);
        return -1;
    }
    return ret;
}

static const __u8* fimc_v4l2_enuminput(int fp, int index)
{
    static struct v4l2_input input;

    input.index = index;
    if (ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0) {
        LOGE("ERR(%s):No matching index found\n", __func__);
        return NULL;
    }
    LOGI("Name of input channel[%d] is %s\n", input.index, input.name);

    return input.name;
}


static int fimc_v4l2_s_input(int fp, int index)
{
    struct v4l2_input input;
    int ret;

    input.index = index;

    ret = ioctl(fp, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture)
{
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    int ret;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = width;
    pixfmt.height = height;
    pixfmt.pixelformat = fmt;

    pixfmt.sizeimage = (width * height * get_pixel_depth(fmt)) / 8;

    pixfmt.field = V4L2_FIELD_NONE;

    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    ret = ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return -1;
    }

    return 0;
}

static int fimc_v4l2_s_fmt_cap(int fp, int width, int height, unsigned int fmt)
{
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    int ret;

    memset(&pixfmt, 0, sizeof(pixfmt));

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    pixfmt.width = width;
    pixfmt.height = height;
    pixfmt.pixelformat = fmt;
    if (fmt == V4L2_PIX_FMT_JPEG) {
        pixfmt.colorspace = V4L2_COLORSPACE_JPEG;
    }

    pixfmt.sizeimage = (width * height * get_pixel_depth(fmt)) / 8;

    v4l2_fmt.fmt.pix = pixfmt;

    //LOGE("ori_w %d, ori_h %d, w %d, h %d\n", width, height, v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height);

    /* Set up for capture */
    ret = ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_enum_fmt(int fp, unsigned int fmt)
{
    struct v4l2_fmtdesc fmtdesc;
    int found = 0;

    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
	LOGD("%s: fmt %x, RGB %x",__func__,fmt,V4L2_PIX_FMT_RGB565);
    while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        if (fmtdesc.pixelformat == fmt) {
            LOGV("passed fmt = %#x found pixel format[%d]: %s\n", fmt, fmtdesc.index, fmtdesc.description);
            found = 1;
            break;
        }
 LOGE("passed fmt = %#x pix %x,found pixel format[%d]: %s\n", fmt,fmtdesc.pixelformat, fmtdesc.index, fmtdesc.description);//hnmsky

        fmtdesc.index++;
    }

    if (!found) {
        LOGE("unsupported pixel format\n");
        return -1;
    }

    return 0;
}

static int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fp, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
        return -1;
    }

    return req.count;
}

static int fimc_v4l2_querybuf(int fp, struct fimc_buffer *buffers, enum v4l2_buf_type type, int nr_frames)
{
    struct v4l2_buffer v4l2_buf;
    int i, ret;

    for (i = 0; i < nr_frames; i++) {
        v4l2_buf.type = type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = i;

          //  LOGE("VIDIOC_QUERYBUF    type %d  index %d, mem %d\n", v4l2_buf.type,v4l2_buf.index,v4l2_buf.memory );//hnmsky
        ret = ioctl(fp , VIDIOC_QUERYBUF, &v4l2_buf);
        if (ret < 0) {
            LOGE("ERR(%s):VIDIOC_QUERYBUF failed\n", __func__);
            return -1;
        }

       // if (nr_frames == 1) //hnmsky
    //   if(1)
	{
            buffers[i].length = v4l2_buf.length;
            if ((buffers[i].start = (char *)mmap(0, v4l2_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fp, v4l2_buf.m.offset)) < 0) {
                LOGE("%s %d] mmap() failed\n",__func__, __LINE__);
                return -1;
            }

            LOGD("buffers[%d].start = %p v4l2_buf.length = %d", i, buffers[i].start, v4l2_buf.length);
        } 
    }

    return 0;
}

static int fimc_v4l2_streamon(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_STREAMON failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_streamoff(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

#ifdef SWP1_CAMERA_ADD_ADVANCED_FUNCTION
    LOGV("%s :", __func__);
#endif
    ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_STREAMOFF failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_qbuf(int fp, int index)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = ioctl(fp, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QBUF failed\n", __func__);
        return ret;
    }

    return 0;
}

static int fimc_v4l2_dqbuf(int fp)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fp, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_DQBUF failed\n", __func__);
        return ret;
    }

    return v4l2_buf.index;
}

static int fimc_v4l2_g_ctrl(int fp, unsigned int id)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;

    ret = ioctl(fp, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s): VIDIOC_G_CTRL(id = 0x%x (%d)) failed, ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, ret);
        return ret;
    }

    return ctrl.value;
}

static int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;

    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);

        return ret;
    }

    return ctrl.value;
}


static int fimc_v4l2_g_parm(int fp, struct v4l2_streamparm *streamparm)
{
    int ret;

    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fp, VIDIOC_G_PARM, streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_G_PARM failed\n", __func__);
        return -1;
    }

    LOGV("%s : timeperframe: numerator %d, denominator %d\n", __func__,
            streamparm->parm.capture.timeperframe.numerator,
            streamparm->parm.capture.timeperframe.denominator);

    return 0;
}

static int fimc_v4l2_s_parm(int fp, struct v4l2_streamparm *streamparm)
{
    int ret;

    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fp, VIDIOC_S_PARM, streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_PARM failed\n", __func__);
        return ret;
    }

    return 0;
}

static int
clamp(int  x)
{
    if (x > 255) return 255;
    if (x < 0)   return 0;
    return x;
}

/* the equation used by the video code to translate YUV to RGB looks like this
 *
 *    Y  = (Y0 - 16)*k0
 *    Cb = Cb0 - 128
 *    Cr = Cr0 - 128
 *
 *    G = ( Y - k1*Cr - k2*Cb )
 *    R = ( Y + k3*Cr )
 *    B = ( Y + k4*Cb )
 *
 */

static const double  k0 = 1.164;
static const double  k1 = 0.813;
static const double  k2 = 0.391;
static const double  k3 = 1.596;
static const double  k4 = 2.018;

/* let's try to extract the value of Y
 *
 *   G + k1/k3*R + k2/k4*B = Y*( 1 + k1/k3 + k2/k4 )
 *
 *   Y  = ( G + k1/k3*R + k2/k4*B ) / (1 + k1/k3 + k2/k4)
 *   Y0 = ( G0 + k1/k3*R0 + k2/k4*B0 ) / ((1 + k1/k3 + k2/k4)*k0) + 16
 *
 * let define:
 *   kYr = k1/k3
 *   kYb = k2/k4
 *   kYy = k0 * ( 1 + kYr + kYb )
 *
 * we have:
 *    Y  = ( G + kYr*R + kYb*B )
 *    Y0 = clamp[ Y/kYy + 16 ]
 */

static const double kYr = k1/k3;
static const double kYb = k2/k4;
static const double kYy = k0*( 1. + kYr + kYb );

static void
initYtab( void )
{
    const  int imax = (int)( (kYr + kYb)*(31 << 2) + (61 << 3) + 0.1 );
    int    i;

    gYTable = (uint8_t *)malloc(imax);

    for(i=0; i<imax; i++) {
        int  x = (int)(i/kYy + 16.5);
        if (x < 16) x = 16;
        else if (x > 235) x = 235;
        gYTable[i] = (uint8_t) x;
    }
}

/*
 *   the source is RGB565, so adjust for 8-bit range of input values:
 *
 *   G = (pixels >> 3) & 0xFC;
 *   R = (pixels >> 8) & 0xF8;
 *   B = (pixels & 0x1f) << 3;
 *
 *   R2 = (pixels >> 11)      R = R2*8
 *   B2 = (pixels & 0x1f)     B = B2*8
 *
 *   kYr*R = kYr2*R2 =>  kYr2 = kYr*8
 *   kYb*B = kYb2*B2 =>  kYb2 = kYb*8
 *
 *   we want to use integer multiplications:
 *
 *   SHIFT1 = 9
 *
 *   (ALPHA*R2) >> SHIFT1 == R*kYr  =>  ALPHA = kYr*8*(1 << SHIFT1)
 *
 *   ALPHA = kYr*(1 << (SHIFT1+3))
 *   BETA  = kYb*(1 << (SHIFT1+3))
 */

static const int  SHIFT1  = 9;
static const int  ALPHA   = (int)( kYr*(1 << (SHIFT1+3)) + 0.5 );
static const int  BETA    = (int)( kYb*(1 << (SHIFT1+3)) + 0.5 );

/*
 *  now let's try to get the values of Cb and Cr
 *
 *  R-B = (k3*Cr - k4*Cb)
 *
 *    k3*Cr = k4*Cb + (R-B)
 *    k4*Cb = k3*Cr - (R-B)
 *
 *  R-G = (k1+k3)*Cr + k2*Cb
 *      = (k1+k3)*Cr + k2/k4*(k3*Cr - (R-B)/k0)
 *      = (k1 + k3 + k2*k3/k4)*Cr - k2/k4*(R-B)
 *
 *  kRr*Cr = (R-G) + kYb*(R-B)
 *
 *  Cr  = ((R-G) + kYb*(R-B))/kRr
 *  Cr0 = clamp(Cr + 128)
 */

static const double  kRr = (k1 + k3 + k2*k3/k4);

static void
initCrtab( void )
{
    uint8_t *pTable;
    int i;

    gCrTable = (uint8_t *)malloc(768*2);

    pTable = gCrTable + 384;
    for(i=-384; i<384; i++)
        pTable[i] = (uint8_t) clamp( i/kRr + 128.5 );
}

/*
 *  B-G = (k2 + k4)*Cb + k1*Cr
 *      = (k2 + k4)*Cb + k1/k3*(k4*Cb + (R-B))
 *      = (k2 + k4 + k1*k4/k3)*Cb + k1/k3*(R-B)
 *
 *  kBb*Cb = (B-G) - kYr*(R-B)
 *
 *  Cb   = ((B-G) - kYr*(R-B))/kBb
 *  Cb0  = clamp(Cb + 128)
 *
 */

static const double  kBb = (k2 + k4 + k1*k4/k3);

static void
initCbtab( void )
{
    uint8_t *pTable;
    int i;

    gCbTable = (uint8_t *)malloc(768*2);

    pTable = gCbTable + 384;
    for(i=-384; i<384; i++)
        pTable[i] = (uint8_t) clamp( i/kBb + 128.5 );
}

/*
 *   SHIFT2 = 16
 *
 *   DELTA = kYb*(1 << SHIFT2)
 *   GAMMA = kYr*(1 << SHIFT2)
 */

static const int  SHIFT2 = 16;
static const int  DELTA  = kYb*(1 << SHIFT2);
static const int  GAMMA  = kYr*(1 << SHIFT2);

int32_t ccrgb16toyuv_wo_colorkey(uint8_t *rgb16, uint8_t *yuv420,
        uint32_t *param, uint8_t *table[])
{
    uint16_t *inputRGB = (uint16_t*)rgb16;
    uint8_t *outYUV = yuv420;
    int32_t width_dst = param[0];
    int32_t height_dst = param[1];
    int32_t pitch_dst = param[2];
    int32_t mheight_dst = param[3];
    int32_t pitch_src = param[4];
    uint8_t *y_tab = table[0];
    uint8_t *cb_tab = table[1];
    uint8_t *cr_tab = table[2];

    int32_t size16 = pitch_dst*mheight_dst;
    int32_t i,j,count;
    int32_t ilimit,jlimit;
    uint8_t *tempY,*tempU,*tempV;
    uint16_t pixels;
    int   tmp;
uint32_t temp;

    tempY = outYUV;
    tempU = outYUV + (height_dst * pitch_dst);
    tempV = tempU + 1;

    jlimit = height_dst;
    ilimit = width_dst;

    for(j=0; j<jlimit; j+=1)
    {
        for (i=0; i<ilimit; i+=2)
        {
            int32_t   G_ds = 0, B_ds = 0, R_ds = 0;
            uint8_t   y0, y1, u, v;

            pixels =  inputRGB[i];
            temp = (BETA*(pixels & 0x001F) + ALPHA*(pixels>>11) );
            y0   = y_tab[(temp>>SHIFT1) + ((pixels>>3) & 0x00FC)];

            G_ds    += (pixels>>1) & 0x03E0;
            B_ds    += (pixels<<5) & 0x03E0;
            R_ds    += (pixels>>6) & 0x03E0;

            pixels =  inputRGB[i+1];
            temp = (BETA*(pixels & 0x001F) + ALPHA*(pixels>>11) );
            y1   = y_tab[(temp>>SHIFT1) + ((pixels>>3) & 0x00FC)];

            G_ds    += (pixels>>1) & 0x03E0;
            B_ds    += (pixels<<5) & 0x03E0;
            R_ds    += (pixels>>6) & 0x03E0;

            R_ds >>= 1;
            B_ds >>= 1;
            G_ds >>= 1;

            tmp = R_ds - B_ds;

            u = cb_tab[(((B_ds-G_ds)<<SHIFT2) - GAMMA*tmp)>>(SHIFT2+2)];
            v = cr_tab[(((R_ds-G_ds)<<SHIFT2) + DELTA*tmp)>>(SHIFT2+2)];

            tempY[0] = y0;
            tempY[1] = y1;
            tempY += 2;

            if ((j&1) == 0) {
                tempU[0] = u;
                tempV[0] = v;
                tempU += 2;
                tempV += 2;
            }
        }

        inputRGB += pitch_src;
    }

    return 1;
}

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

static void convert_rgb16_to_yuv420(uint8_t *rgb, uint8_t *yuv, int width, int height)
{
    if (!tables_initialized) {
        initYtab();
        initCrtab();
        initCbtab();
        tables_initialized = 1;
    }

    uint32_t param[6];
    param[0] = (uint32_t) width;
    param[1] = (uint32_t) height;
    param[2] = (uint32_t) width;
    param[3] = (uint32_t) height;
    param[4] = (uint32_t) width;
    param[5] = (uint32_t) 0;

    uint8_t *table[3];
    table[0] = gYTable;
    table[1] = gCbTable + 384;
    table[2] = gCrTable + 384;

    ccrgb16toyuv_wo_colorkey(rgb, yuv, param, table);
}

const int Ov965xCamera::kRed;
const int Ov965xCamera::kGreen;
const int Ov965xCamera::kBlue;

Ov965xCamera::Ov965xCamera(int width, int height)
          : mTmpRgb16Buffer(0)
{
	struct v4l2_format fmt;
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	m_flag_camera_start =0;
	mFmt = V4L2_PIX_FMT_RGB565;
	v4l2_fd = open(DEV_NAME, O_RDWR | O_SYNC);


	LOGE(" open uvc return %d\n",v4l2_fd);
	LOGE("OV9650xCamera: open /dev/video0 return %x  width %x height %x errno=%x\n",v4l2_fd,width,height,errno);
#if 0
	if(v4l2_fd != -1)
	{
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.width = width;
		fmt.fmt.pix.height = height;
		//fmt.fmt.pix.depth = 16;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

	
		ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt);
		ioctl(v4l2_fd, VIDIOC_STREAMON, &type);
	}
#endif
	//init_device(v4l2_fd,width,height);
    	setSize(width, height,mFmt);
}

Ov965xCamera::~Ov965xCamera()
{
	if(v4l2_fd != -1)
		close(v4l2_fd);
	
    delete[] mTmpRgb16Buffer;
}
int Ov965xCamera::startPreview(void)
{
	int ret;
	int nr;
	unsigned int fmt;
    // aleady started
    if (m_flag_camera_start > 0) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    if (v4l2_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }	
	ret = fimc_v4l2_querycap(v4l2_fd);
	CHECK(ret);

	//fmt = V4L2_PIX_FMT_YUYV;
	//fmt = V4L2_PIX_FMT_RGB565; 
	fmt = mFmt;
	ret = fimc_v4l2_enum_fmt(v4l2_fd,fmt);
	CHECK(ret);

	
	ret = fimc_v4l2_s_fmt(v4l2_fd,mWidth,mHeight,fmt,0);
	CHECK(ret);

	ret = fimc_v4l2_reqbufs(v4l2_fd,V4L2_BUF_TYPE_VIDEO_CAPTURE,MAX_BUFFERS);//
	CHECK(ret);

	nr = ret;
	if(nr > 0)
	{

		ret = fimc_v4l2_querybuf(v4l2_fd,m_buffers_c,V4L2_BUF_TYPE_VIDEO_CAPTURE,nr);
		CHECK(ret);

		int i;
		for(i = 0;i < nr; i++)
		{

			fimc_v4l2_qbuf(v4l2_fd, i);
			CHECK(ret);
		}
	}

    	ret = fimc_v4l2_streamon(v4l2_fd);
    	CHECK(ret);

	return ret;
}
int Ov965xCamera::stopPreview(void)
{
    int ret = fimc_v4l2_streamoff(v4l2_fd);

    m_flag_camera_start = 0;
    CHECK(ret);

    return ret;
}
void Ov965xCamera::setSize(int width, int height,unsigned int fmt)
{
    mWidth = width;
    mHeight = height;
    mFmt = fmt;
    mCounter = 0;
    mCheckX = 0;
    mCheckY = 0;

    // This will cause it to be reallocated on the next call
    // to getNextFrameAsYuv420().
    delete[] mTmpRgb16Buffer;
    mTmpRgb16Buffer = 0;

}


void Ov965xCamera::getNextFrameAsRgb565(uint16_t *buffer)
{
        int ret;
	unsigned long len;
	int index;
	if(v4l2_fd != -1)
	{
	#if 0
		len = mWidth*mHeight*2;
                ret = read(v4l2_fd, buffer, len);
	#endif
		len = mWidth*mHeight*2;
		memset(&m_event,0,sizeof(m_event));
		m_event.fd = v4l2_fd;
		m_event.events = POLLIN | POLLERR;

		ret = fimc_poll(&m_event);
		if(ret > 0)
		{
			index = fimc_v4l2_dqbuf(v4l2_fd);

			memcpy(buffer,m_buffers_c[index].start ,len);

			//convert_yuyv_to_rgb565((uint8_t *)(m_buffers_c[index].start) , buffer, mWidth,mHeight);  
			fimc_v4l2_qbuf(v4l2_fd, index);
		}
	}
	else
	{
	    int size = mWidth / 10;

	    drawCheckerboard(buffer, size);

	    int x = ((mCounter*3)&255);
	    if(x>128) x = 255 - x;
	    int y = ((mCounter*5)&255);
	    if(y>128) y = 255 - y;

	    drawSquare(buffer, x*size/32, y*size/32, (size*5)>>1, (mCounter&0x100)?kRed:kGreen, kBlue);

	    mCounter++;
	}
}

void Ov965xCamera::getNextFrameAsYuv420(uint8_t *buffer)
{
    if (mTmpRgb16Buffer == 0)
        mTmpRgb16Buffer = new uint16_t[mWidth * mHeight];

    getNextFrameAsRgb565(mTmpRgb16Buffer);
	
    convert_rgb16_to_yuv420((uint8_t*)mTmpRgb16Buffer, buffer, mWidth, mHeight);

	unsigned long i;
	for(i=0;i<mWidth*mHeight;i++)
	{
		if(i< (mWidth*mHeight/2) )
			buffer[i] = 0;
		else
			buffer[i] = 0xFF;	
	}


}

void Ov965xCamera::drawSquare(uint16_t *dst, int x, int y, int size, int color, int shadow)
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

void Ov965xCamera::drawCheckerboard(uint16_t *dst, int size)
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


void Ov965xCamera::dump(int fd) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, 255, " width x height (%d x %d), counter (%d), check x-y coordinate(%d, %d)\n", mWidth, mHeight, mCounter, mCheckX, mCheckY);
    result.append(buffer);
    ::write(fd, result.string(), result.size());
}
}; // namespace android
