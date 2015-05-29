#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include "../lib/libsub.h"

int pwm_res = 20; //microseconds
int pwm_limit = 255; //8bit, 100% duty cycle
int foo,cont,op_mode,motor_dir,i,j,adc_buff[8],adc_mux[8],rc,rca,rcp,config,found=0,serial_number=0,stat;
char sn_buff[20], pid_buff[20];
double volts[8], celcius[4];
struct timeval timeout;
fd_set set;
sub_device dev = NULL;
sub_handle handle = NULL;

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
    //configure analog circuits
    config = ADC_ENABLE;
    config|=ADC_REF_VCC;
    if ( (rc = sub_adc_config(handle, config)) !=0){
            printf("ERROR: couldn't set ADC flag\n");
            return -4;
    }else{
        adc_mux[0] = ADC_S0;
        adc_mux[1] = ADC_S1;
        adc_mux[2] = ADC_S2;
        adc_mux[3] = ADC_S3;
        adc_mux[4] = ADC_S4;
        adc_mux[5] = ADC_S5;
        adc_mux[6] = ADC_S6;
        adc_mux[7] = ADC_S7;
    }
    //configure pwm
    if ( (rc=sub_pwm_config(handle,pwm_res,pwm_limit))!=0 ){
            printf("ERROR: can't configure PWM\n");
            return -6;
    } else {//configure output gpio (dir and pwm signal pins)
    sub_gpio_config(handle,0x08001000,&config,0xFFFFFFFF);
    printf("config: %08x\n",config);
    }
    printf("device configured\n");

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO,&set);
    timeout.tv_sec = 1;
    timeout.tv_usec= 0;
    printf("setup done, sec:%ld, usec:%ld\n",timeout.tv_sec,timeout.tv_usec);
        
    return 0;
}

int user_input(){
    scanf("%d",&op_mode);
    if (-255<=op_mode && op_mode<=255){
        return op_mode;
    }
    
    printf("invalid value for pwm signal. no chance made\n");
    return -1000;
}

double convert2celcius(){
    //printf("error: could not convert to celcius\n");
    return -9999.0;
}

int main(){
    if (init_sub()!=0)
        printf("ERROR: could not configure device\n");
    //printf("stdin #=%d\tFD_SETSIZE:%d\n", STDIN_FILENO,FD_SETSIZE); 

    cont=0;
    while(cont!=1){
        timeout.tv_sec = 0;
        timeout.tv_usec= 100000;
        FD_SET(STDIN_FILENO,&set);
        if (select(STDIN_FILENO+1,&set,NULL,NULL,&timeout)==1){//if user input waiting, deal with it
            printf("\n\ncaught user trigger, foo: %d\n\n",foo);//be loud about it
            user_input();
            //set pwm
            if (-255<=op_mode && op_mode <=255)
                rcp = sub_pwm_set(handle,3,abs(op_mode));
            else
                printf("error: you entered a value out of range [-255,255]\n");           
            //set dir
            if (op_mode<0){
                printf("negative op_mode\n");
                rc = sub_gpio_write(handle,0x00001000,&config,0x00001000);
            }
            if (op_mode>0){
                printf("positive op_mode\n");
                rc = sub_gpio_write(handle,0x00000000,&config,0x00001000);
            }
        }//otherwise, read temps and loop
        rca = sub_adc_read(handle,adc_buff,adc_mux,8);
        convert2celcius();
        //printf("op_mode:%d\tconfig:0x%08x\trc:%d\trcp:%d\trca:%d\n",op_mode,config,rc,rcp,rca);

        for (i=0;i<8;i++){
            printf("%d\t",adc_buff[i]);
        }
        printf("\n\n");
	}
    return 0;
}
