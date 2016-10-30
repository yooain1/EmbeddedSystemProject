#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <linux/fb.h>
#include "bmp_header.h"

#define LED_DEV "/dev/iom_led"
#define FND_DEV "/dev/iom_fnd"
#define PUSH_DEV "/dev/iom_push"
#define KTIMER_NAME "/dev/kernel_timer"
#define FB_DEVFILE "/dev/fb0"

/* for LED */
unsigned char led_hex_data[] = {0x00, 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f, 0xff}; /* all, 1 ~ 8, OFF */

/* for FND */
unsigned char fnd_hex_val[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x90}; /* 0 ~ 9 */

/* for PUSH */
int key_lookup_table[] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f}; /* 1 ~ 8 */

/* for clock running status */
typedef enum {FALSE, TRUE} BOOLEAN;
BOOLEAN clock_running = TRUE;	//clock running status variable

/* functions */
void stop_clock_running(int sig);
int read_push(int push_fd);
void read_bmp(int file_num, char **pDib, char **data, int *cols, int *rows);
void close_bmp(char **pDib);
void draw(int screen_width, unsigned char *pfbmap, unsigned long *bmpdata,int rows, int cols, int mp_offset, int y_offset);

/* thread functions */
void* clock_func(void *arg);
void* interface_func(void *arg);
void* alarm_func(void *arg);

/* time variables */
typedef struct time_t{
	unsigned int hour;
	unsigned int min;
	unsigned int sec;
}TIME;
TIME alarm_time;
TIME current_time;

int main(int argc, char** argv){

	/* thread variables */
	pthread_t clock_th, interface_th, alarm_th;
	int clock_thread_param, interface_thread_param, alarm_thread_param=0;

	/* time sync variables */
	int ret;
	FILE* time_fd = NULL;
	char time_buf[30];
	char time_buf2[5];

	ret = system("rdate -s 203.248.240.140");
	if(ret <0){
		printf("time sync error...\n");
	}
	else{
		printf("Now, time is synchronized...\n");
	}
	ret = system("date > time.txt");
	if(ret <0){
		printf("time save error...\n");
	}
	time_fd = fopen("time.txt", "r");
	if(time_fd == NULL){
		printf("FIle open error...\n");
		exit(0);
	}
	fgets(time_buf, 30, time_fd);
	
	printf("\n--- Current time ---\n\n");
	puts(time_buf);
	printf("\n");

	/* save current time value to current_time */
	strncpy(time_buf2, time_buf+11, 2);
	time_buf2[3] = '\0';
	current_time.hour = atoi(time_buf2);
	
	strncpy(time_buf2, time_buf+14, 2);
	time_buf2[3] = '\0';
	current_time.min = atoi(time_buf2);

	strncpy(time_buf2, time_buf+17, 2);
	time_buf2[3] = '\0';
	current_time.sec = atoi(time_buf2);

	/* setup clock running status signal */
	(void) signal (SIGINT, stop_clock_running); 
	printf("Press <ctrl + c> to quit...\n\n"); 


/* thread */
	/* create thread */
	if(pthread_create(&clock_th, NULL, clock_func, (void *)&clock_thread_param)!=0){
		printf("clock_thread create() error\n");
		return -1;
	} 
	if(pthread_create(&interface_th, NULL, interface_func, (void *)&interface_thread_param)!=0){
		printf("interface_thread create() error\n");
		return -1;
	} 
	if(pthread_create(&alarm_th, NULL, alarm_func, (void *)&alarm_thread_param)!=0){
		printf("alarm_thread create() error\n");
		return -1;
	} 

	/* join thread */
	pthread_join(clock_th, NULL);
	pthread_join(interface_th, NULL);
	pthread_join(alarm_th, NULL);

	fclose(time_fd);
	return 0;
}


/***************************Thread Functions******************************************************/
/* Clock Thread Main Function */
void* clock_func(void *arg){
	
	/* kernel timer variables */
	int ktimer_fd;
	unsigned char tick = 1;
	unsigned char tock = ~tick;
	
	/* fb variables */
	int i, j, k, t, h;
	int fb_fd; 
	int screen_width;
	int screen_height;
	int bits_per_pixel;
	int line_length;
	int cor_x, cor_y;
	int cols = 0, rows = 0;
	int cols2 = 0, rows2 = 0; //hour
	int cols3 = 0, rows3 = 0; //min
	int mem_size;
	int hour, minute, am_pm;
	int mp_offset =0 , y_offset=0;
	int letter_num;
	char time_info[4]= { 0 };
	char offset;
	int change_screen = 0;

	char fnd_time[4];
	char alam_info;
	char fnd_alert;

	char *pData, *data;
	char *pData2, *data2; //hour
	char *pData3, *data3; //min
	char r, g, b;
	unsigned long bmpdata[800*480];
	unsigned long bmpdata2[87*83]; //hour
	unsigned long bmpdata3[87*83]; //min
	unsigned long pixel;
	unsigned char *pfbmap;
	
	struct fb_var_screeninfo fbvar;
	struct fb_fix_screeninfo fbfix;
	
	/* open devices */
	ktimer_fd = open(KTIMER_NAME, O_RDWR);
	if(ktimer_fd<0){
		printf("Kernel timer device open error: %s\n", KTIMER_NAME);
		exit(1);
	}
	
	if((fb_fd= open(FB_DEVFILE, O_RDWR))<0)
	{
		printf("%s: open error\n", FB_DEVFILE);
		exit(1);
	}
	
	/* read FB info */
	printf("Frame buffer Application - Bitmap\n");
	if(ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbvar) <0)
	{
		printf("%s: ioctl error -FBIOGET VSCREENINFO \n", FB_DEVFILE);
		exit(1);
	}

	if(ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbfix)<0)
	{
		printf("%s: ioctl error - FBIOGET FSCREENINFO \n", FB_DEVFILE);
		exit(1);
	}
	
	if(fbvar.bits_per_pixel != 32)
	{
		fprintf(stderr, "bpp is not 32\n");
		exit(1);
	}

	screen_width = fbvar.xres;
	screen_height = fbvar.yres;
	bits_per_pixel = fbvar.bits_per_pixel;
	line_length = fbfix.line_length;
	mem_size = line_length * screen_height;

	
	read_bmp(0,&pData, &data, &cols, &rows);
	printf("Bitmap : cols = %d, rows = %d\n", cols, rows);
	
	/* save BMP info to array */
	for (j=0; j<rows; j++){
		k = j*cols*3;
		t = (rows - 1 - j) * cols;
		for(i=0; i<cols; i++)
		{
			b=*(data+(k+i*3));
			g=*(data+(k+i*3+1));
			r=*(data+(k+i*3+2));
			//pixel = ((r<<24) | (g<<16) |(b<<8));
			pixel = ((0x77<<24) |(r<<16) |(g<<8)|b);
			bmpdata[t+i] = pixel;
		}
	}
	close_bmp(&pData);
	
	/* print initial screen */
	pfbmap = (unsigned char *) mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);

	if((unsigned) pfbmap == (unsigned)-1)
	{
		perror("fbdev mmap");
		exit(1);
	}
	
	draw(screen_width, pfbmap, bmpdata, rows, cols, 0, 0);

	/* start timer */
	write(ktimer_fd, NULL, 0);	

	while(clock_running){
		
		/* timeout occur (1 sec passed) */
		while (tock==tick){	/* until time out */
			read(ktimer_fd, &tick, sizeof(tick));
		}
		tock = ~tock;

		/* update time */
		current_time.sec = (current_time.sec + 1)%60;
		if(current_time.sec == 0){
			current_time.min = (current_time.min + 1)%60;
			if(current_time.min == 0) current_time.hour = (current_time.hour + 1)%24;
		}
		
		/* print initial screen again if the minute is a multiple of 5 */
		if(current_time.min%5==0 && change_screen == 0){
			draw(screen_width, pfbmap, bmpdata, rows, cols, 0, 0);
			change_screen = 1;
		}
		else if(current_time.min%5!=0)
			change_screen = 0;
		
		/* get time info to print time on screen */
		hour = (unsigned char) current_time.hour;
		minute = (unsigned char) current_time.min;
		
		printf("current time is %d:%d:%d(clock th)\n", current_time.hour, current_time.min, current_time.sec);	//print global time variable
		printf("current time is %d:%d(bmp clock)\n", hour, minute);												//print screen time variable
		
		/*print am or pm */
		if(hour>12)
			am_pm = 1;
		else
			am_pm = 0;

		switch(am_pm){
			case 0: letter_num = 2; time_info[0] = 16; time_info[1] = 17; break;
			case 1: letter_num = 2; time_info[0] = 17; time_info[1] = 22; break;
		}
	
		for(h=0; h<letter_num; h++){
	
			read_bmp(time_info[h], &pData2, &data2, &cols2, &rows2);

			for (j=0; j<rows2; j++){
				k = j*cols2*3;
				t = (rows2 - 1 - j) * cols2;
				for(i=0; i<cols2; i++)
				{
					b=*(data2+(k+i*3+1));
					g=*(data2+(k+i*3+1));
					r=*(data2+(k+i*3));
					//pixel = ((r<<24) | (g<<16) |(b<<8));
					pixel = ((0x77<<24) |(r<<16) |(g<<8)|b);
					bmpdata2[t+i] = pixel;
				}
			}
		
			offset = time_info[h];

			switch(offset){
				printf("time_info[i] = %d\n", offset);
				case 16: case 17: mp_offset = 17+93*(offset-16); y_offset = 27+88*3; break;
				case 22: mp_offset = 17+93*(offset-21); y_offset = 27+88*4; break;
			}
			draw(screen_width, pfbmap, bmpdata2, rows2, cols2, mp_offset, y_offset);

		}
		/* print hour */
		switch(hour){
			case 1: case 13: letter_num = 2; time_info[0] = 2; time_info[1] = 15; break;
			case 2: case 14: letter_num = 2; time_info[0] = 6; time_info[1] = 15; break;
			case 3: case 15: letter_num = 2; time_info[0] = 4; time_info[1] = 15; break;
			case 4: case 16: letter_num = 2; time_info[0] = 5; time_info[1] = 15; break;
			case 5: case 17: letter_num = 3; time_info[0] = 3; time_info[1]= 8; time_info[2] = 15; break;
			case 6: case 18: letter_num = 3; time_info[0] = 7; time_info[1]= 8; time_info[2] = 15; break;
			case 7: case 19: letter_num = 3; time_info[0] = 9; time_info[1]= 10;time_info[2] = 15; break;
			case 8: case 20: letter_num = 3; time_info[0] = 11;time_info[1]= 12; time_info[2] = 15; break;
			case 9: case 21: letter_num = 3; time_info[0] = 13; time_info[1]= 14; time_info[2] = 15; break;
			case 10: case 22: letter_num = 2; time_info[0] = 1; time_info[1] = 15; break;
			case 11: case 23: letter_num = 3; time_info[0] = 1; time_info[1]= 2; time_info[2] = 15; break;
			case 0: case 12: letter_num = 3; time_info[0] = 1; time_info[1]= 6; time_info[2] = 15; break;
		}

		for(h=0; h<letter_num; h++){
	
			read_bmp(time_info[h], &pData2, &data2, &cols2, &rows2);

			for (j=0; j<rows2; j++){
				k = j*cols2*3;
				t = (rows2 - 1 - j) * cols2;
				for(i=0; i<cols2; i++)
				{
					b=*(data2+(k+i*3+2));
					g=*(data2+(k+i*3+3));
					r=*(data2+(k+i*3+4));
					//pixel = ((r<<24) | (g<<16) |(b<<8));
					pixel = ((0x77<<24) |(r<<16) |(g<<8)|b);
					bmpdata2[t+i] = pixel;
				}
			}
			offset = time_info[h];

			switch(offset){
				case 1: case 2: case 3: case 4: case 5: mp_offset = 17+93*(offset-1); y_offset = 27; break;
				case 6: case 7: case 8: case 9: case 10: mp_offset = 17+93*(offset-6); y_offset = 27+88*1;  break; 
				case 11: case 12: case 13: case 14: case 15: mp_offset = 14+93*(offset-11); y_offset = 27+88*2; break;
				case 16: case 17: case 18: case 19: case 20: mp_offset = 17+93*(offset-16); y_offset = 27+88*3; break;
				case 21: case 22: case 23: case 24: case 25: mp_offset = 17+93*(offset-21); y_offset = 27+88*4; break;
			}
			draw(screen_width, pfbmap, bmpdata2, rows2, cols2, mp_offset, y_offset);

		}

		/* print min */
		if(minute>0 && minute <5) minute = 0;
		else if(minute>5 && minute <10) minute = 5;
		else if(minute>10 && minute <15) minute = 10;
		else if(minute>15 && minute <20) minute = 15;
		else if(minute>20 && minute <25) minute = 20;
		else if(minute>25 && minute <30) minute = 25;
		else if(minute>30 && minute <35) minute = 30;
		else if(minute>35 && minute <40) minute = 35;
		else if(minute>40 && minute <45) minute = 40;
		else if(minute>45 && minute <50) minute = 45;
		else if(minute>50 && minute <55) minute = 50;
		else if(minute>55) minute = 55;
		switch(minute){
			case 5: letter_num = 2; time_info[0] = 24; time_info[1] = 25; break;
			case 10: letter_num = 2; time_info[0] = 20; time_info[1] = 25; break;
			case 15: letter_num = 3; time_info[0] = 23; time_info[1] = 24; time_info[2] = 25; break;
			case 20: letter_num = 3; time_info[0] = 18; time_info[1] = 23; time_info[2] = 25; break;
			case 25: letter_num = 4; time_info[0] = 18; time_info[1]= 23; time_info[2] = 24; time_info[3] = 25; break;
			case 30: letter_num = 3; time_info[0] = 19; time_info[1]= 20; time_info[2] = 25; break;
			case 35: letter_num = 4; time_info[0] = 19; time_info[1]= 20; time_info[2] = 24; time_info[3] = 25; break;
			case 40: letter_num = 3; time_info[0] = 21; time_info[1]= 23; time_info[2] = 25; break;
			case 45: letter_num = 4; time_info[0] = 21; time_info[1]= 23; time_info[2] = 24; time_info[3] = 25; break;
			case 50: letter_num = 3; time_info[0] = 22; time_info[1] = 23; time_info[2] = 25; break;
			case 55: letter_num = 4; time_info[0] = 22; time_info[1]= 23; time_info[2] = 24; time_info[3] = 25; break;
		}

		for(h=0; h<letter_num; h++){
	
			read_bmp(time_info[h], &pData3, &data3, &cols3, &rows3);

			for (j=0; j<rows3; j++){
				k = j*cols3*3;
				t = (rows3 - 1 - j) * cols3;
				for(i=0; i<cols3; i++)
				{
					b=*(data3+(k+i*3+1));
					g=*(data3+(k+i*3+2));
					r=*(data3+(k+i*3+3));
					//pixel = ((r<<24) | (g<<16) |(b<<8));
					pixel = ((0x77<<24) |(r<<16) |(g<<8)|b);
					bmpdata3[t+i] = pixel;
				}
			}
	
		
			offset = time_info[h];


			switch(offset){
				case 1: case 2: case 3: case 4: case 5: mp_offset = 17+93*(offset-1); y_offset = 27; break;
				case 6: case 7: case 8: case 9: case 10: mp_offset = 17+93*(offset-6); y_offset = 27+88*1;  break; 
				case 11: case 12: case 13: case 14: case 15: mp_offset = 14+93*(offset-11); y_offset = 27+88*2; break;
				case 16: case 17: case 18: case 19: case 20: mp_offset = 14+93*(offset-16); y_offset = 25+88*3; break;
				case 21: case 22: case 23: case 24: case 25: mp_offset = 14+93*(offset-21); y_offset = 22+88*4; break;
			}
			if(minute != 0)
				draw(screen_width, pfbmap, bmpdata3, rows3, cols3, mp_offset, y_offset);

		}
		

		/* get alarm time info */
		fnd_time[0] = (unsigned char) alarm_time.hour/10;	//10 hour
		fnd_time[1] = (unsigned char) alarm_time.hour%10;	// 1 hour
		fnd_time[2] = (unsigned char) alarm_time.min/10;	// 10 min
		fnd_time[3] = (unsigned char) alarm_time.min%10;	// 1 min

		/* print alarm info */
		for(h=0; h<4; h++){
		
			alam_info = fnd_time[h]+30;	
	
			read_bmp(alam_info, &pData2, &data2, &cols2, &rows2);

			for (j=0; j<rows2; j++){
				k = j*cols2*3;
				t = (rows2 - 1 - j) * cols2;
				for(i=0; i<cols2; i++)
				{
					b=*(data2+(k+i*3+1));
					g=*(data2+(k+i*3+2));
					r=*(data2+(k+i*3+3));
					//pixel = ((r<<24) | (g<<16) |(b<<8));
					pixel = ((0x77<<24) |(r<<16) |(g<<8)|b);
					bmpdata2[t+i] = pixel;
				}
			}

			switch(h){
				case 0: mp_offset = 505; y_offset = 203; break;
				case 1: mp_offset = 555; y_offset = 203; break;
				case 2: mp_offset = 645; y_offset = 203; break;
				case 3: mp_offset = 700; y_offset = 203; break;
			}

			draw(screen_width, pfbmap, bmpdata2, rows2, cols2, mp_offset, y_offset);

		}	

	}
	munmap(pfbmap, mem_size);
	close(fb_fd);
	close(ktimer_fd);
}
	
/* Alarm Setup Interface Thread Main Function */
void* interface_func(void *arg){

	/* variables for LED */
	int led_fd;
	int led_index;

	/* variables for FND */
	int fnd_fd;
	int fnd_index;
	unsigned char fnd_seg_data;

	/* variables for PUSH */
	int push_fd;
	int push_index;
	int pushed_int = 0;
	unsigned char pushed_value = 0;
	int state = 0;

	/* open devices */
	led_fd = open(LED_DEV, O_WRONLY);
	if(led_fd<0){
		printf("Device open error(%s)...\n", LED_DEV);
		exit(1);
	}
	fnd_fd = open(FND_DEV, O_WRONLY);
	if(fnd_fd<0){
		printf("Device open error(%s)...\n", FND_DEV);
		exit(1);
	}
	push_fd = open(PUSH_DEV, O_RDONLY);
	if(push_fd<0){
		printf("Device open error(%s)...\n", PUSH_DEV);
		exit(1);
	}

	/* initialize led, fnd */
	led_index = 0;	//initialize led to ON
	write(led_fd, &led_hex_data[led_index], sizeof(led_hex_data[led_index]));	

	fnd_index = 0;	//initialize fnd to 0
	fnd_seg_data = fnd_hex_val[fnd_index];
	write(fnd_fd, &fnd_seg_data, sizeof(fnd_seg_data));	

	printf("current state is %d\n\n", state);		

	while(clock_running){

		usleep(200000);

		pushed_int = read_push(push_fd); //read pushed button value

		if(pushed_int == 8){ //change mode

			state = (state+1)%5;

			switch(state){
				case 0:
					printf("FND value of state %d = %d\n(0: init 1: 10 hour 2: 1 hour 3: 10 min 4: 1 min)\n\n", state-1, fnd_index);
					
					/* 1 min set based on FND value */
					alarm_time.min += fnd_index;
					printf("alarm time is %d:%d\n\n", alarm_time.hour, alarm_time.min);

					led_index = 0;	//initial mode
					write(led_fd, &led_hex_data[led_index], sizeof(led_hex_data[led_index]));	//led 0 on
					printf("current state is %d\n\n", state);		
					break;
				case 1:
					led_index = 1;	//10 hour mode
					write(led_fd, &led_hex_data[led_index], sizeof(led_hex_data[led_index]));	//led 1 on
					printf("current state is %d\n\n", state);		

					break;
				case 2:
					if(fnd_index > 2){
						printf("Invalid time value\n");
						state--;
						break;
					}
					printf("FND value of state %d = %d\n(0: init 1: 10 hour 2: 1 hour 3: 10 min 4: 1 min)\n\n", state-1, fnd_index);

					/* 10 hour set based on FND value */
					alarm_time.hour = fnd_index * 10;

					led_index = 2;	//1 hour mode
					write(led_fd, &led_hex_data[led_index], sizeof(led_hex_data[led_index]));	//led 2 on
					printf("current state is %d\n\n", state);		

					break;
				case 3:
					printf("FND value of state %d = %d\n(0: init 1: 10 hour 2: 1 hour 3: 10 min 4: 1 min)\n\n", state-1, fnd_index);

					/* 1 hour set based on FND value sec*/
					alarm_time.hour += fnd_index;

					led_index = 3;	//10 min mode
					write(led_fd, &led_hex_data[led_index], sizeof(led_hex_data[led_index]));	//led 3 on
					printf("current state is %d\n\n", state);		

					break;
				case 4:
					if(fnd_index > 6){
						printf("Invalid time value\n");
						state--;
						break;
					}
					printf("FND value of state %d = %d\n(0: init 1: 10 hour 2: 1 hour 3: 10 min 4: 1 min)\n\n", state-1, fnd_index);

					/* 10 min set based on FND value */
					alarm_time.min = fnd_index * 10;

					led_index = 4;	//1 min mode
					write(led_fd, &led_hex_data[led_index], sizeof(led_hex_data[led_index]));	//led 4 on
					printf("current state is %d\n\n", state);		

					break;
			}
		}

		else if(pushed_int == 1){ //increment fnd value
			fnd_index = (fnd_index +1);
			if(fnd_index == 10) fnd_index = 0;
			fnd_seg_data = fnd_hex_val[fnd_index];
			write(fnd_fd, &fnd_seg_data, sizeof(fnd_seg_data));
		}
		else if(pushed_int == 5){ //decrement fnd value
			fnd_index = (fnd_index -1);
			if(fnd_index == -1) fnd_index = 9;
			fnd_seg_data = fnd_hex_val[fnd_index];
			write(fnd_fd, &fnd_seg_data, sizeof(fnd_seg_data));
		}

	}


	/* close devices */
	close(led_fd);
	close(fnd_fd);
	close(push_fd);		

}

/* Alarm Checker Thread Main Function */
void* alarm_func(void *arg){
	while(clock_running){
		sleep(1);
		printf("alarm time is %d:%d(alarm th)\n", alarm_time.hour, alarm_time.min);
		if( (alarm_time.hour == current_time.hour) && (alarm_time.min == current_time.min) ){
			//speaker
			printf("\n\n\n\n\n\nalarm\n\n\n\n\n\n\n\n\n");
		}
	}
}

/*************************************Other Functions***************************************************/
/* Clock status function */
void stop_clock_running(int sig){
        clock_running = FALSE;
}

/* Push button function */
int read_push(int push_fd){
	int push_index = 0;
	unsigned char pushed_value = 0;

	read(push_fd, &pushed_value, 1);
	if(pushed_value != 0xFF){	//error check
		for(push_index = 0; push_index < 8; push_index++){	//find pushed value index
			if(pushed_value == key_lookup_table[push_index]){
				return push_index + 1;
			}
		}
	}
	return 0;
}


/* FB functions */
void read_bmp(int file_num, char **pDib, char **data, int *cols, int *rows)
{
	BITMAPFILEHEADER bmpHeader;
	BITMAPINFOHEADER *bmpInfoHeader;
	unsigned int size;
	int nread;
	char filename[30] = "./images/";
	char num[3];
	int len;
	FILE *fp;

	len = sprintf(filename+9, "%d", file_num);
	strcat(filename+len, ".bmp");

	fp= fopen (filename, "rb");
	if(fp==NULL){
		printf("Screen Iamges File open error..\n");
		return;
	}
	
	nread = fread(&bmpHeader, sizeof(BITMAPFILEHEADER),1, fp);

	size = bmpHeader.bfSize - sizeof(BITMAPFILEHEADER);

	*pDib = (unsigned char *) malloc (size);
	fread(*pDib, size, 1, fp);
	
	bmpInfoHeader = (BITMAPINFOHEADER *) *pDib;

	if(24!=bmpInfoHeader->biBitCount)
	{
		printf("It supports only 24bit bmp!\n");
		fclose(fp);
		return ;
	}

	*cols = bmpInfoHeader->biWidth;
	*rows = bmpInfoHeader->biHeight;
	*data = (char*) (*pDib + bmpHeader.bfOffBits - sizeof(bmpHeader) - 2);
	fclose(fp);
}

void close_bmp(char **pDib)
{
	free(*pDib);
}

void draw(int screen_width, unsigned char *pfbmap, unsigned long *bmpdata,int rows, int cols, int mp_offset, int y_offset){
	int cor_x, cor_y;	
	unsigned long *ptr;

	for(cor_y=0; cor_y<rows; cor_y++){
		ptr = (unsigned long*) pfbmap+(screen_width*(cor_y+y_offset))+mp_offset;
		for(cor_x=0; cor_x<cols; cor_x++)
			*ptr++ = bmpdata[cor_x+cor_y*cols];
	}
}
/*********************************************************************************************************/
