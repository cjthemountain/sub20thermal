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
int op_mode,motor_dir,i,j,adc_buff[8],adc_mux[8],rc,rca,config,found=0,serial_number=0,stat;
char sn_buff[20], pid_buff[20];
double volts[8], celcius[4];

void print_binary(int n){
	while (n) {
		if (n & 1)
			printf("1");
		else
			printf("0");
		n >>= 1;
	}
	printf("\n");
}

int get_temps(){
	//printf("read analog values\n");
	adc_mux[0] = ADC_S0;
	adc_mux[1] = ADC_S1;
	adc_mux[2] = ADC_S2;
	adc_mux[3] = ADC_S3;
	adc_mux[4] = ADC_S4;
	adc_mux[5] = ADC_S5;
	adc_mux[6] = ADC_S6;
	adc_mux[7] = ADC_S7;
	sub_adc_read(handle, adc_buff, adc_mux, 8);
	for(i=0;i<8;i++){volts[i] = (double)adc_buff[i]*3.3/1023;}
	i=0;j=0;
	while (i<8){
		celcius[j] = ((volts[i]-volts[i+1])-1.245)/.005;
		i+=2;
		j++;
	}
}

int init_sub(){	
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

	//activate ADC
	config = ADC_ENABLE;
	config|=ADC_REF_VCC;
	//config|=ADC_REF_2_56;
	if ( (rc = sub_adc_config(handle, config)) !=0){
		printf("ERROR: couldn't set ADC flag\n");
		return -4;
	}

	//configure pwm
	if ( (rc=sub_pwm_config(handle,pwm_res,pwm_limit))!=0 ){
		printf("ERROR: can't configure PWM\n");
		return -6;
	}


	//configure gpio pins for dir=gpio12, pwm=gpio27
	if ( (rc = sub_gpio_config(handle,0x08001000,&config,0xFFFFFFFF))!=0){
		printf("ERROR: can't configure GPIO\n");
		return -5;
	}
	
	printf("device configured\n");
	return 0;
}

int main(){
	if ((stat=init_sub())!=0)
		printf("ERROR:%d, Failed to configure device\n", stat);

	sub_pwm_set(handle,3,255); //set pwm signal on hp=12,gpio=27
	sub_gpio_write(handle,0x08000000,&config,0x08000000); //set dir to high

	while (1) {
		while (scanf("%d", &op_mode)==1){
			switch(op_mode){
				case 1:
					rca = sub_pwm_set(handle,3,0); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x08000000,&config,0x08000000); //set dir to high
					printf("pwm @ 0  \tdir high\tsuccess:%d%d\n",rca,rc);
					break;
				case 2:
					rca = sub_pwm_set(handle,3,63); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x08000000,&config,0x08000000); //set dir to high
					printf("pwm @ 63 \tdir high\tsuccess:%d%d\n",rca,rc);
					break;
				case 3:
					rca = sub_pwm_set(handle,3,127); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x08000000,&config,0x08000000); //set dir to high
					printf("pwm @ 127\tdir high\tsuccess:%d%d\n",rca,rc);
					break;
				case 4:
					rca = sub_pwm_set(handle,3,190); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x08000000,&config,0x08000000); //set dir to high
					printf("pwm @ 190\tdir high\tsuccess:%d%d\n",rca,rc);
					break;
				case 5:
					rca = sub_pwm_set(handle,3,255); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x08000000,&config,0x08000000); //set dir to high
					printf("pwm @ 255\tdir high\tsuccess:%d%d\n",rca,rc);
					break;
				case 6:
					rca = sub_pwm_set(handle,3,0); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x00000000,&config,0x08000000); //set dir to low
					printf("pwm @ 0  \tdir low\t\tsuccess:%d%d\n",rca,rc);
					break;
				case 7:
					rca = sub_pwm_set(handle,3,63); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x00000000,&config,0x08000000); //set dir to low
					printf("pwm @ 63 \tdir low\t\tsucess:%d%d\n",rca,rc);
					break;
				case 8:
					rca = sub_pwm_set(handle,3,127); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x00000000,&config,0x08000000); //set dir to low
					printf("pwm @ 127\tdir low\t\tsuccess:%d%d\n",rca,rc);
					break;
				case 9:
					rca = sub_pwm_set(handle,3,190); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x00000000,&config,0x08000000); //set dir to low
					printf("pwm @ 190\tdir low\t\tsuccess:%d%d\n",rca,rc);
					break;
				case 0:
					rca = sub_pwm_set(handle,3,255); //set pwm signal on hp=12,gpio=27
					rc = sub_gpio_write(handle,0x00000000,&config,0x08000000); //set dir to low
					printf("pwm @ 255\tdir low\t\tsuccess:%d%d\n",rca,rc);
					break;
			}
		}
		
	
		get_temps();
		if (celcius[0]>70 || celcius[1]>70){ 
			//sub_gpio_write(handle,0x00000000,&config,0xFFFFFFFF);//set everything low
			sub_pwm_set(handle,3,0);
		}
		printf("tc0:%f\ttc1:%f\ttc2:%f\ttc3:%f\n", celcius[0],celcius[1],celcius[2],celcius[3]);
		sleep(1);
	}

	sub_close(handle);
	printf("done\n");
}
