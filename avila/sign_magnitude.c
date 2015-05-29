#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <unistd.h>
#include "../lib/libsub.h"

sub_device dev = NULL;
sub_handle handle = NULL;
int pwm_res = 20; //microseconds
int pwm_limit = 255; //8bit, 100% duty cycle
int cont,op_mode,motor_dir,i,j,adc_buff[8],adc_mux[8],rc,rca,config,found=0,serial_number=0,stat;
char sn_buff[20], pid_buff[20];
double volts[8], celcius[4];

int main(){
        memset(sn_buff, 0, 20);
        if (!(dev = sub_find_devices(dev)))
                printf("no device found\n");
        if (!(handle = sub_open(dev))){
                printf("ERROR: cannot open handle to device\n");
                return -1;
        }
        serial_number = sub_get_serial_number(handle,sn_buff,sizeof(sn_buff));
        if (serial_number<0) {
                printf("ERROR: can't get serial number\n");
                return -2;
        }
        else {
                found = 1;
                printf("found device %d\n", sn_buff);
        }
        if ( (rc = sub_get_product_id(handle, pid_buff, sizeof(pid_buff))) == 0){
                printf("ERROR: couldn't retrieve product id\n");
                return -3;
        }
        printf("product id:\t%s\n", pid_buff);
 
        config = ADC_ENABLE;
        config|=ADC_REF_VCC;
	if ( (rc = sub_adc_config(handle, config)) !=0){
                printf("ERROR: couldn't set ADC flag\n");
                return -4;
        }
	
	//configure pwm
        if ( (rc=sub_pwm_config(handle,pwm_res,pwm_limit))!=0 ){
                printf("ERROR: can't configure PWM\n");
                return -6;
        } else {
		sub_gpio_config(handle,0x08001000,&config,0xFFFFFFFF);
		printf("config: %08x\n",config);
	}
/*
	while (1){
		rc = sub_gpio_write(handle,0x00001000,&config,0x00001000); 
		rc = sub_pwm_set(handle,3,0);
		printf("congig: 0x%08x\n",config);
		usleep(2000000);
		rc = sub_gpio_write(handle,0x00000000,&config,0x00001000); 
		rc = sub_pwm_set(handle,3,127);
		printf("config: %08x\n",config);
		usleep(2000000);
	}
*/
        printf("device configured\n");
	
	while(cont!=1){
		op_mode = getchar();
		getchar();
		switch(op_mode){
			case '1':
				rca = sub_pwm_set(handle,3,0); //set pwm signal on hp=12,gpio=27
				break;
			case '2':
				rca = sub_pwm_set(handle,3,63); //set pwm signal on hp=12,gpio=27
				break;
			case '3':
				rca = sub_pwm_set(handle,3,127); //set pwm signal on hp=12,gpio=27
				break;
			case '4':
				rca = sub_pwm_set(handle,3,190); //set pwm signal on hp=12,gpio=27
				break;
			case '5':
				rca = sub_pwm_set(handle,3,255); //set pwm signal on hp=12,gpio=27
				break;
			case 'a':
				rc = sub_gpio_write(handle,0x00001000,&config,0x00001000);//set dir on gpio12,hp5 high
				break;
			case 's':
				rc = sub_gpio_write(handle,0x00000000,&config,0x00001000);//set dir on gpio12,hp5 low
				break;
			case 'q':
				cont=1;
				break;
			default:
				cont=0;
		}
		printf("case:%d\tsuccess:%d%d config:0x%08x\n",op_mode,rca,rc,config);
	}

        return 0;
}
