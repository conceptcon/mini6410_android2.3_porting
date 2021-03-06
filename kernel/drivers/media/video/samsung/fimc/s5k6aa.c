/* linux/drivers/media/video/samsung/s5k3ba.c
 *
 * Samsung S5K3BA CMOS Image Sensor driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "s3c_fimc.h"
#include "s5k6aa.h"

#define s5k6aa_I2C_ADDR		0x78

const static u16 ignore[] = { I2C_CLIENT_END };
const static u16 normal_addr[] = { (s5k6aa_I2C_ADDR >> 1), I2C_CLIENT_END };
const static u16 *forces[] = { NULL };
static struct i2c_driver s5k6aa_i2c_driver;

static struct s3c_fimc_camera s5k6aa_data = {
	.id 		= CONFIG_VIDEO_FIMC_CAM_CH,
	.type		= CAM_TYPE_MIPI,
	.mode		= MIPI_CSI_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.clockrate	= 24000000,
	.width		= 640,
	.height		= 480,
	.offset		= {
		.h1 = 0,
		.h2 = 0,
		.v1 = 0,
		.v2 = 0,
	},

	.polarity	= {
		.pclk	= 0,
		.vsync	= 1,
		.href	= 0,
		.hsync	= 0,
	},

	.initialized	= 0,
};

static struct i2c_client_address_data addr_data = {
	.normal_i2c	= normal_addr,
	.probe		= ignore,
	.ignore		= ignore,
	.forces		= forces,
};

static void s5k6aa_start(struct i2c_client *client)
{
	int i;

	for (i = 0; i < S5K6AA_INIT_REGS; i++) {
		if(i==4)
			mdelay(100);
		s3c_fimc_i2c_write(client, s5k6aa_init_reg[i], sizeof(s5k6aa_init_reg[i]));
	}
}

static int s5k6aa_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, "s5k6aa");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &s5k6aa_i2c_driver;

	s5k6aa_data.client = c;

	info("s5k6aa attached successfully\n");

	return i2c_attach_client(c);
}

static int s5k6aa_attach_adapter(struct i2c_adapter *adap)
{
	int ret = 0;

	s3c_fimc_register_camera(&s5k6aa_data);

	ret = i2c_probe(adap, &addr_data, s5k6aa_attach);
	if (ret) {
		err("failed to attach s5k6aa driver\n");
		ret = -ENODEV;
	}

	return ret;
}

static int s5k6aa_detach(struct i2c_client *client)
{
	i2c_detach_client(client);

	return 0;
}

static int s5k6aa_change_resolution(struct i2c_client *client, int res)
{
	switch (res) {
	case CAM_RES_DEFAULT:	/* fall through */
	case CAM_RES_MAX:	/* fall through */
		break;

	default:
		err("unexpect value\n");
	}

	return 0;
}

static int s5k6aa_change_whitebalance(struct i2c_client *client, enum s3c_fimc_wb_t type)
{
//	s3c_fimc_i2c_write(client, 0xfc, 0x0);
//	s3c_fimc_i2c_write(client, 0x30, type);

	return 0;
}

static int s5k6aa_command(struct i2c_client *client, u32 cmd, void *arg)
{
	switch (cmd) {
	case I2C_CAM_INIT:
		s5k6aa_start(client);
		info("external camera initialized\n");
		break;

	case I2C_CAM_RESOLUTION:
		s5k6aa_change_resolution(client, (int) arg);
		break;

	case I2C_CAM_WB:
		s5k6aa_change_whitebalance(client, (enum s3c_fimc_wb_t) arg);
        	break;

	default:
		err("unexpect command\n");
		break;
	}

	return 0;
}

static struct i2c_driver s5k6aa_i2c_driver = {
	.driver = {
		.name = "s5k6aa",
	},
	.id = I2C_DRIVERID_S5K6AA,
	.attach_adapter = s5k6aa_attach_adapter,
	.detach_client = s5k6aa_detach,
	.command = s5k6aa_command,
};

static __init int s5k6aa_init(void)
{
	return i2c_add_driver(&s5k6aa_i2c_driver);
}

static __init void s5k6aa_exit(void)
{
	i2c_del_driver(&s5k6aa_i2c_driver);
}

module_init(s5k6aa_init)
module_exit(s5k6aa_exit)

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION(" I2C based CMOS Image Sensor driver");
MODULE_LICENSE("GPL");
