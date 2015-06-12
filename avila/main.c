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
int cont,op_mode,old_op_mode,motor_dir,i,j,adc_buff[8],
	adc_mux[8],rc,rca,rcp,config,found=0,serial_number=0,stat,z,
	seconds=5,microseconds=0;
double volta, voltb, prop, integ, deriv, t_des, t_set;
char sn_buff[20], pid_buff[20];
double celcius[4], old_celcius[4];
struct timeval timeout;
char filepath[1024];
FILE *fptr;
fd_set set;
sub_device dev = NULL;
sub_handle handle = NULL;

int bang_bang(){ //bang bang control
    if (t_set > celcius[2])
       op_mode = 254;
    else if (t_set < celcius[2])
        op_mode = -254;
	else
		op_mode = 0;
    return op_mode;
}

int pid(){//just integral for now
	//integ becomes nan when t_des-celcius[2] is negative
	if ((t_set - celcius[2]) > 0)
		integ += pow( abs(t_set - celcius[2]), 2);

	if ((t_set - celcius[2]) < 0)	
		integ -= pow( abs(t_set - celcius[2]), 2);

	if ((t_set-celcius[2])==0.0)
		integ = 0;

	if (integ >= 254.0)
		integ = 254.0;
	if (integ <= -254.0)
		integ = -254.0;
	op_mode = ( (int) (round(integ)) );

	return op_mode;
}

int analog_2_celcius(int *adc_buff){
    for (i=0;i<8;i++){
        old_celcius[i] = celcius[i];
    }
    i=0; j=0;
    while (i<8){
        volta =   (((double)adc_buff[i]))*3.3/1023;
        voltb = (((double)adc_buff[i+1]))*3.3/1023;
        celcius[j] =   ((volta-voltb)-1.245)/.005;
        i+=2; j+=1;
    }
	celcius[2] = celcius[2]+0.7; //calibration
    return 0;
}

int user_input(){
    scanf("%lf",&t_des);
    t_set = 1.2924*t_des-3.5177; //best fit line from calibration study
    integ = 0.0;
    prop=0.0;
	deriv = 0.0;
	fprintf(fptr,"\nset t_des to %f\n\n",t_des);
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
	op_mode=0;

    //initialize celcius values    
    rca = sub_adc_read(handle,adc_buff,adc_mux,8);
    analog_2_celcius(adc_buff);
    rca = sub_adc_read(handle,adc_buff,adc_mux,8);
    analog_2_celcius(adc_buff);
    t_des = celcius[2];
	t_set = 1.2924*celcius[2]-3.5177; 
	printf("inital values: t_des:%f\tt_set:%f\tc2:%f\n",t_des,t_set,celcius[2]);
	
	//open file for writing
	printf("enter filename\n-->");
	fgets(filepath,sizeof(filepath),stdin);
	if ( strlen(filepath) > 0 && filepath[strlen(filepath)-1] == '\n')
		filepath[strlen(filepath)-1] = '\0'; 
	fptr=fopen(filepath,"w");	
	if (!fptr){
		printf("ERROR: could not open file [%s] for writing\n", filepath);
		exit(-1);
	}else{
		printf("init success\nstarting program in 5 seconds\n");
		printf("set t_des to -1000 to save data and exit\n");
		fprintf(fptr,"PWM\tTdesired(C)\tcu plate(C)\tTECbottom(C)\tTECtop(C)\tTEMPamb(C)\ttime(sec)\n");
		sleep(5);
	}
    return 0;
}

int main(){
    if (init_sub()!=0)
        printf("ERROR: could not configure device\n");
    cont=0;
    z = 0;
    while(cont!=1){
        z+=1;
        timeout.tv_sec = seconds;
        timeout.tv_usec= microseconds;
        FD_SET(STDIN_FILENO,&set);

		//wait then poll
        if (select(STDIN_FILENO+1,&set,NULL,NULL,&timeout)==1)
            user_input();

		//read temps
		rca = sub_adc_read(handle,adc_buff,adc_mux,8);
		analog_2_celcius(adc_buff);


		//check for overheat condition
        if (celcius[0] > 80.0 || celcius[1] > 80.0 || celcius[2] > 60.0){
				op_mode = 0;
                printf("above saftey cutoff temperature. pwm set to 0\n");
                fprintf(fptr,"above saftey cutoff temperature. pwm set to 0\n");
        }
		
		//set pwm signal and direction
		old_op_mode = op_mode;		
		//pid();
		bang_bang();
		if (rcp = sub_pwm_set(handle,3,abs(op_mode)) != 0){
			printf("could not set pwm\n");
			fprintf(fptr,"could not set pwm\n");
			cont=1;
		}
		if ( (op_mode > 0) && (op_mode != old_op_mode) ) 
			rc = sub_gpio_write(handle,0x00001000,&config,0x00001000);

		if (rc!=0){
			printf("failed setting direction\n");
			cont=1;
		}
		if ( (op_mode < 0) && (op_mode != old_op_mode) )
			rc = sub_gpio_write(handle,0x00000000,&config,0x00001000);

		if (rc!=0){
			printf("failed setting direction\n");
			cont=1;
		}

		printf("pwm:%d\ttdes:%.1f\tplate:%.1f\ti:%.1f\tbot:%.1f\ttop:%.1f\tamb%.1f\ttime:%d\n",
		op_mode,t_des,celcius[2],integ,celcius[0],celcius[1],celcius[3],z*(seconds+microseconds));
		fprintf(fptr,"%d\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%d\n",
				op_mode,t_des,celcius[2],celcius[0],celcius[1],celcius[3],z*(seconds+microseconds));
		//user exit condition, set desired temperature to -1000 to exit cleanly
		if (t_des==-1000){
			cont=1;
			rcp = sub_pwm_set(handle,3,0);
		}
		if (z>10000)
			z=0;//prevent integer rollover
    }
	fclose(fptr);
    return 0;
}
