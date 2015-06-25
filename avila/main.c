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

int pwm_res = 20,pwm_limit=255,cont,op_mode=0,old_op_mode,motor_dir,i,j,adc_buff[8],
	adc_mux[8],rc,rca,rcp,config,found=0,serial_number=0,stat,z,
	seconds=1,microseconds=0, bang_high=254, bang_low=-254;;
double t_des, celcius[4], old_celcius[4];
char sn_buff[20], pid_buff[20], filepath[1024];
struct timeval timeout;
FILE *fptr;
fd_set set;
sub_device dev = NULL;
sub_handle handle = NULL;

int bang_bang(){ //bang bang control
    if (t_des > celcius[0])
       op_mode = bang_high;
    else if (t_des < celcius[0])
        op_mode = bang_low;
	else
		op_mode = 0;
    return op_mode;
}

int analog_2_celcius(int *adc_buff){
    for (i=0;i<4;i++){
        old_celcius[i] = celcius[i];
    }
    j=0;
    for (i=0;i<8;i+=2){
        celcius[j] = 0.7369*(double)adc_buff[i]-289.77;
        j++;
    }
    return 0;
}

int user_input(){
    scanf("%lf",&t_des);
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
	rc=sub_pwm_config(handle,pwm_res,pwm_limit);
	if (rc!=0){
            printf("ERROR: can't configure PWM\n");
            return -6;
    } 
	//configure output gpio (dir and pwm signal pins)
	printf("pre-config: %08x\n",config);
    rc = sub_gpio_config(handle,0x08001000,&config,0xFFFFFFFF);
	if (rc!=0){
		printf("failed to configure gpio pins\npost_config:%08x\n",config);
	} 
    
    printf("device configured\n");

    //setup for select (polling)
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO,&set);
    timeout.tv_sec = 1;
    timeout.tv_usec= 0;
    printf("setup done, sec:%ld, usec:%ld\n",timeout.tv_sec,timeout.tv_usec);

    //initialize celcius values    
    rca = sub_adc_read(handle,adc_buff,adc_mux,8);
    analog_2_celcius(adc_buff);
    t_des = celcius[0];
	printf("inital values: t_des:%f\tc2:%f\n",t_des,celcius[2]);
	
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
		printf("init success\nstarting program in 2 seconds\n");
		printf("set t_des to -1000 to save data and exit\n");
		fprintf(fptr,"t0\tt1\tt2\tt3\tt_des\top_mode\ttime\n");
		printf("t0\tt1\tt2\tt3\tt_des\top_mode\ttime\n");
		sleep(2);
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


		//if overheat condition failure, wait for "safe" temps before continuing to drive
        if (celcius[0] > 60.0 || celcius[1] > 60.0 || celcius[2] > 60.0){
				op_mode = 0;
				if (rcp = sub_pwm_set(handle,3,abs(op_mode)) != 0){
					printf("could not set pwm during overheat crisis. Quitting...\n");
					fprintf(fptr,"could not set pwm during overheat crisis. Quitting...\n");
					cont=1;
					exit(-1);
				}
                printf("above saftey cutoff temperature. pwm set to 0\n");
                fprintf(fptr,"above saftey cutoff temperature. pwm set to 0\n");
        } 
		else {//set pwm signal and direction
			old_op_mode = op_mode;		
			bang_bang();
            
            //change pwm and dir only if different from old setting
            if (op_mode != old_op_mode){
		    	//set pwm
                rcp = sub_pwm_set(handle,3,abs(op_mode));
                if (rcp!=0){
                    printf("could not set pwm\t\t\t\t\t\t\t\t%d\n",z*(seconds+microseconds));
                    fprintf(fptr,"could not set pwm\t\t\t\t\t\t\t\t%d\n",z*(seconds+microseconds));
                    cont=1;
                }
                
                //set direction
                if (op_mode > 0) 
                    rc = sub_gpio_write(handle,0x00001000,&config,0x00001000);
                else 
                    rc = sub_gpio_write(handle,0x00000000,&config,0x00001000);

                if (rc != 0) {
                    printf("could not set dir\t\t\t\t\t\t\t\t%d\n",z*(seconds+microseconds));
                    fprintf(fptr,"could not set dir\t\t\t\t\t\t\t\t%d\n",z*(seconds+microseconds));
                    cont=1;
                }
		    }

            printf("%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%d\t%d\n",
                    celcius[0],celcius[1],celcius[2],celcius[3],t_des,op_mode,z*(seconds+microseconds));
            fprintf(fptr,"%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%d\t%d\n",
                    celcius[0],celcius[1],celcius[2],celcius[3],t_des,op_mode,z*(seconds+microseconds));
            //user exit condition, set desired temperature to -1000 to exit cleanly
            if (t_des==-1000){
                cont=1;
                rcp = sub_pwm_set(handle,3,0);
            }
            if (z>10000)
                z=0;//prevent integer rollover
        }
    }
    fclose(fptr);
    return 0;
}
