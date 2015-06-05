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
int cont,op_mode,motor_dir,i,j,adc_buff[8],adc_mux[8],rc,rca,rcp,config,found=0,serial_number=0,stat,z;
double volta, voltb, prop, integ, deriv, t_des;
char sn_buff[20], pid_buff[20];
double celcius[4], old_celcius[4];
struct timeval timeout;
fd_set set;
sub_device dev = NULL;
sub_handle handle = NULL;

int pid(){ //bang bang control
    if (t_des > celcius[2])
       op_mode = 255;
    else if (t_des < celcius[2])
        op_mode = -255;
    return op_mode;
}

int analog_2_celcius(int *adc_buff){
    for (i=0;i<8;i++){
        old_celcius[i] = celcius[i];
    }
    i=0; j=0;
    while (i<6){
        volta =   (((double)adc_buff[i]))*3.3/1023;
        voltb = (((double)adc_buff[i+1]))*3.3/1023;
        celcius[j] =   ((volta-voltb)-1.245)/.005;
        i+=2; j+=1;
    }
    return 0;
}

int analog_2_celcius2(){
    celcius[0] = ((double)adc_buff[0])*.58496;
    celcius[1] = ((double)adc_buff[2])*.58496;
    celcius[2] = ((double)adc_buff[4])*.58496;
}

int user_input(){
    scanf("%lf",&t_des);
    t_des = 1.2924*t_des-3.5177; //best fit line from uncalibrated thermometer study
    integ = 0;
    prop=0;
    return 0;
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
    //configure analog circuits
    config = ADC_ENABLE;
    config|=ADC_REF_VCC;
    //config|=ADC_REF_2_56;
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

    //setup for select (polling)
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO,&set);
    timeout.tv_sec = 1;
    timeout.tv_usec= 0;
    printf("setup done, sec:%ld, usec:%ld\n",timeout.tv_sec,timeout.tv_usec);

    //setup for PID
    prop = 0.0;
    deriv = 0.0;
    integ = 0.0;

    //initialize celcius values    
    rca = sub_adc_read(handle,adc_buff,adc_mux,8);
    analog_2_celcius(adc_buff);
    t_des = 24.0; //loosely room temperature
    return 0;
}

int main(){
    if (init_sub()!=0)
        printf("ERROR: could not configure device\n");
    cont=0;
    z = 0;
    while(cont!=1){
        z+=1;
        timeout.tv_sec = 0;
        timeout.tv_usec= 10000;
        FD_SET(STDIN_FILENO,&set);
        if (select(STDIN_FILENO+1,&set,NULL,NULL,&timeout)==1)
            user_input();
        pid(); 
        rcp = sub_pwm_set(handle,3,abs(op_mode));
        if (op_mode>0) 
            rc = sub_gpio_write(handle,0x00001000,&config,0x00001000);
        else if (op_mode < 0)
            rc = sub_gpio_write(handle,0x00000000,&config,0x00001000);
        rca = sub_adc_read(handle,adc_buff,adc_mux,8);
        analog_2_celcius(adc_buff);
        //analog_2_celcius2(adc_buff);
        if (z%10==0){
            system("clear");
            printf("p: %f\t i:%f\t d:%f\te:%f\n",prop,integ,deriv,(t_des-celcius[0]));
            printf("t_des: %.1f\tpower: %d\tt0:%.1fC\tt1:%.1fC\tt2:%.1fC\n\n",t_des, op_mode, celcius[0], celcius[1],celcius[2]);
            for (i=0;i<8;i++){
                printf("%d\t",adc_buff[i]);
            }
            printf("\n");
        }
        
        if (celcius[0] > 80 || celcius[1] > 80 || celcius[2] > 40){
                rcp = sub_pwm_set(handle,3,abs(op_mode));
                printf("over 30 degrees celcius. setting pwm to 0...");
                if (rcp!=0)
                    printf("CUT POWER IMMEDIATELY!!!!!!!!!!!! failure to change pwm cycle\n");
                else 
                    printf("done. PWM=0\n");
        }
        
	}
    return 0;
}
