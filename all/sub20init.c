#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include "./libsub.h"

sub_device dev;
sub_handle *handle = NULL;
int mode, bootmode;
char sn_buff[20];
int found = 0;
int serial_number = 0;

int main() 
{
	while( (dev = sub_find_devices(dev)) )
	{
		handle = sub_open(dev);
		if (!handle) 
		{
			printf("ERROR: cannot open handle to device\n");
		}
		serial_number = sub_get_serial_number(handle,sn_buff,sizeof(sn_buff));
		sub_close(handle);
		if (serial_number<0)
		{
			printf("ERROR: can't get serial number\n");
		} 
		else 
		{
			found = 1;
			printf("found device %d\n", serial_number);
		}
	}
	return 0;
}
