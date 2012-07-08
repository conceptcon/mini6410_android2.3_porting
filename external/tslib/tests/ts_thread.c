#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <linux/fcntl.h>
#include <stdlib.h>

#include <sys/time.h>

//hnmsky
int main()
{
	int dev_fd, if_fd;
       unsigned int ts_data = 0;
	int cal_fd, proc_fd;
	char buff[256];
	int cal_size;
	int bl_fd;

	char bl[]="127";

	cal_fd = open("/etc/pointercal",O_RDONLY);
	proc_fd = open("/proc/ts_cal",O_RDWR);
	if(cal_fd > 0 && proc_fd > 0)
	{
		cal_size = read(cal_fd, buff, 256);

		if(cal_size > 0)
			write(proc_fd,buff,cal_size);

		close(cal_fd);
		close(proc_fd);
	}
	else
	{
		perror("cal file open error \n");
	}



	dev_fd = open("/dev/touchscreen-1wire",O_RDONLY);
	if(dev_fd < 0)
	{
		perror("ts_open error  def_fd\n");
		return -1;
	}

	if_fd = open("/dev/ts-if",O_RDONLY);
	if(if_fd < 0)
	{
		perror("ts_open error  if_fd\n");
		return -1;
	}
	
	bl_fd = open("/dev/backlight-1wire",O_RDWR);
	if(bl_fd<0)
	{
		perror("dev backlight file open fail\n");
		return -1;
	}
	perror("======bl_fd \n");
	fprintf(stdout,"open blacklight\n");
	
	write(bl_fd,&bl,4);

	usleep(10*1000);
	

	close(bl_fd);

	while(1)
	{
		int sz=0;
		ts_data = 0;
		sz=read(dev_fd,&ts_data,sizeof(ts_data));//hnmsky
		fprintf(stderr,"read dev data  (%x)\n",ts_data);

		if(sz < sizeof(ts_data))
		{
			fprintf(stderr,"===============================read dev erro  (%d)\n",sz);
		 	break;
		}
		else
		{
			if(ioctl(if_fd,0,ts_data) < 0)
			{
				fprintf(stderr,"==============================read if erro  (%d)\n",sz);
				break;
			}
			fprintf(stderr,"ioctl\n");
			

		}

	}
	return 0;


	}
