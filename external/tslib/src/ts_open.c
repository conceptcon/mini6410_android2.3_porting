/*
 *  tslib/src/ts_open.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_open.c,v 1.4 2004/07/21 19:12:59 dlowder Exp $
 *
 * Open a touchscreen device.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <linux/fcntl.h>

#include "tslib-private.h"

extern struct tslib_module_info __ts_raw;

struct tsdev *ts_open(const char *name, int nonblock)
{
	struct tsdev *ts;
	int flags = O_RDONLY;

	if (nonblock)
		flags |= O_NONBLOCK;

	ts = malloc(sizeof(struct tsdev));
	if (ts) {
		memset(ts, 0, sizeof(struct tsdev));

		ts->fd = open(name, flags);
		if (ts->fd == -1)
			goto free;
	}
	/* hnmsky */
	/*
	{
		ts->dev_fd = open("/dev/touchscreen-1wire",O_RDONLY);
		if(ts->dev_fd < 0)
			perror("ts_open error  def_fd\n");

		ts->if_fd = open("/dev/ts-if",O_RDONLY);
		if(ts->if_fd < 0)
			perror("ts_open error  if_fd\n");


	}*/
	return ts;

free:
	free(ts);
	return NULL;
}
