/* linux/drivers/input/touchscreen/ts-if.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>

#include <linux/string.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
//#include <linux/stdlib.h>

#define CONFIG_FB_S3C_EXT_TFT480272
#if defined(CONFIG_FB_S3C_EXT_TFT480272)
#define S3CFB_HRES		480	/* horizon pixel  x resolition */
#define S3CFB_VRES		272	/* line cnt       y resolution */
#elif defined(CONFIG_FB_S3C_EXT_TFT800480)
#define S3CFB_HRES		800	/* horizon pixel  x resolition */
#define S3CFB_VRES		480	/* line cnt       y resolution */
#elif defined(CONFIG_FB_S3C_EXT_X240320)
#define S3CFB_HRES		240	/* horizon pixel  x resolition */
#define S3CFB_VRES		320	/* line cnt       y resolution */
#else
#error mini6410 frame buffer driver not configed
#endif

#define S3C_TSVERSION	0x0101
#define DEBUG_LVL    KERN_DEBUG

static struct input_dev *input_dev;
static char phys[] = "input(ts)";

#define DEVICE_NAME     "ts-if"

static struct proc_dir_entry * ts_cal_file;//hnmsky
extern struct proc_dir_entry  proc_root; 
#define PROCTS_MAX_SIZE		128
#define PROCTS_NAME 		"ts_cal"

static char ts_proc[PROCTS_MAX_SIZE];
static int  cal[7]={1,0,0,1,0,0,1};

static long _ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned is_down;
	unsigned x,y;
	unsigned cal_x,cal_y;

	is_down = (((unsigned)(arg)) >> 31);
	if (is_down) {
		x = (arg >> 16) & 0x7FFF;
		y = arg &0x7FFF;
		cal_x = (cal[0] * x + cal[1] * y + cal[2]) / cal[6];
		cal_y = (cal[3] * x + cal[4] * y + cal[5]) / cal[6];

	//	printk("cal x %d  cal y %d \n", cal_x, cal_y);
		input_report_abs(input_dev, ABS_X, cal_x);
		input_report_abs(input_dev, ABS_Y, cal_y);

		input_report_key(input_dev, BTN_TOUCH, 1);
		input_report_abs(input_dev, ABS_PRESSURE, 1);
		input_sync(input_dev);
	} else {
		input_report_key(input_dev, BTN_TOUCH, 0);
		input_report_abs(input_dev, ABS_PRESSURE, 0);
		input_sync(input_dev);
	}

//	printk("ts_if ioctl (%x)  x(%d),y(%d), is_down(%d)\n",arg,x,y,is_down);
	return 0;
}


static struct file_operations dev_fops = {
    .owner   =   THIS_MODULE,
    .unlocked_ioctl   =   _ioctl,
};

static struct miscdevice misc = {
	.minor = 185,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};
static int procfile_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	int len=0;
	int i;
	 if (off > 0)
	 {
   		 *eof = 1;
   		 return 0;
 	}
	for(i = 0;i < 7; i++)
	{
		printk("%d ",cal[i]);
	}
	printk("\n");


	return len;

}
static int procfile_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{

	int i;
	char * head;
	char* tail;
	int len = 0;


	if(count > PROCTS_MAX_SIZE)
	{
		printk(KERN_INFO "ts procfile  is full \n");
		return -EFAULT;
	}

	if(copy_from_user(ts_proc,buffer,count))
	{
		return -EFAULT;
	}


	head = ts_proc;
	for(i = 0 ; i < 7; i++)
	{
		tail = strchr(head,' ');
		if(tail == NULL)
		{
			return -EFAULT;
		}
		*tail = 0;
		cal[i] = simple_strtol(head,NULL,10);
		printk("%d ",cal[i]);
		len += strlen(head);
		if(len >  PROCTS_MAX_SIZE -1)
		{
		
			return -EFAULT;
		}

		head = tail + 1;


	}
	printk("\n");

	return count;

}
static int __init dev_init(void)
{
	int ret;

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		return ret;
	}
	
	input_dev->evbit[0] = input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input_set_abs_params(input_dev, ABS_X, 0, S3CFB_HRES, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, S3CFB_VRES, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 1, 0, 0);

	input_dev->name = "TouchScreen Pipe";
	input_dev->phys = phys;
	input_dev->id.bustype = BUS_RS232;
	input_dev->id.vendor = 0xDEAD;
	input_dev->id.product = 0xBEEF;
	input_dev->id.version = S3C_TSVERSION;

	/* All went ok, so register to the input system */
	ret = input_register_device(input_dev);
	
	if(ret) {
		printk("s3c_ts.c: Could not register input device(touchscreen)!\n");
		input_free_device(input_dev);
		return ret;
	}

	ret = misc_register(&misc);
	if (ret) {
		input_unregister_device(input_dev);
		input_free_device(input_dev);
		return ret;
	}

	ts_cal_file =  create_proc_entry(PROCTS_NAME, 0644, NULL);//hnmsky
	if (ts_cal_file == NULL) {
		remove_proc_entry(PROCTS_NAME, &proc_root);
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
			PROCTS_NAME);
		return -ENOMEM;
	}

	ts_cal_file->read_proc  = procfile_read;
	ts_cal_file->write_proc = procfile_write;
	//ts_cal_file->owner 	  = THIS_MODULE;
	//ts_cal_file->mode 	  = S_IFREG | S_IRUGO;
	//ts_cal_file->uid 	  = 0;
	//ts_cal_file->gid 	  = 0;
	//ts_cal_file->size 	  = 37;

	printk(KERN_INFO "/proc/%s created\n", PROCTS_NAME);	
	printk (DEVICE_NAME"\tinitialized\n");
    	return ret;
}

static void __exit dev_exit(void)
{
	input_unregister_device(input_dev);
	misc_deregister(&misc);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");
MODULE_DESCRIPTION("S3C6410 Touch Screen Interface Driver");
