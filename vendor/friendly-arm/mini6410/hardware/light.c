#define LOG_TAG "light"

#include <hardware/hardware.h>
#include <hardware/lights.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <linux/input.h>
#include <utils/Log.h>
#include <cutils/atomic.h>
static int lights_device_close(struct hw_device_t* device);
static int lights_device_open(const struct hw_module_t *module,const char *id, struct hw_device_t **device);
static int other_set_light(struct light_device_t* dev,struct light_state_t const* state);
static int lcd_set_light(struct light_device_t* dev,struct light_state_t const* state);
struct hw_module_methods_t lights_module_methods = {
    .open = lights_device_open,
};



const struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "lights Module",
	.author = "Google, Inc.",
	.methods = &lights_module_methods,
};

static int lights_device_open(const struct hw_module_t *module,const char *id, struct hw_device_t **device)
{
    struct light_device_t *dev = NULL;
    int resvalue = -1;
    dev = (struct light_device_t *)malloc(sizeof(struct light_device_t));
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = lights_device_close;
    if(!strcmp(id, LIGHT_ID_BACKLIGHT))
    {
        dev->set_light = lcd_set_light;
        resvalue = 0;
    }
    else
    {
        dev->set_light = other_set_light;
        resvalue = 0;
    }
    *device = &dev->common;
    return resvalue;
}
static int lights_device_close(struct hw_device_t* device)
{
    struct light_device_t *m_device = (struct light_device_t *)device;
    if(m_device)
        free(m_device);
    return 0;
}
static int lcd_set_light(struct light_device_t* dev,struct light_state_t const* state)
{
    int fd = -1;
    int bytes = 0;
    int rlt = -1;
    unsigned char brightness = ((77*((state->color>>16)&0x00ff))
                               + (150*((state->color>>8)&0x00ff)) 
                              + (29*(state->color&0x00ff))) >> 8;
     LOGE("brightness  %x  color %x \n",brightness,state->color); 
    fd = open("/dev/backlight-1wire", O_RDWR);
    if(fd>0)
    {
        char buffer[20];
     memset(buffer, 0, 20);
    bytes = sprintf(buffer, "%d", brightness);
    rlt = write(fd, buffer, bytes);
        if(rlt>0)
        {
           close(fd);
           return 0;
        }
    }
    close(fd);
    return -1;
}

static int other_set_light(struct light_device_t* dev,struct light_state_t const* state)
{
	LOGE("other_set_light \n");
    return 0;
}
