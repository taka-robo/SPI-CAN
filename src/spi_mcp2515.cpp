#include <wiringPiSPI.h>
#include <wiringPi.h>
#include <stdio.h>
#include "MCP2515.hpp"


int main(void)
{
	MCP2515 can;
	unsigned char MsgBuf[29];
	if(can.GPIOSetUp()<0){
		printf("wiringPiGPIOSetup error \n");
		return -1;
	}
	if(can.SPISetUp()<0){
		printf("wiringPiSPISetup error \n");
		return -1;
	}
	//MC
	MsgBuf[0]=1;
	//RC
	MsgBuf[1]=0;
	MsgBuf[2]=0;
	//Time Stamp
	MsgBuf[3]=0;
	MsgBuf[4]=0;
	MsgBuf[5]=0;
	MsgBuf[6]=1;
	//Category
	MsgBuf[7]=0;
	MsgBuf[8]=0;
	MsgBuf[9]=0;
	//ID
	MsgBuf[10]=0;
	MsgBuf[11]=0;
	MsgBuf[12]=0;
	MsgBuf[13]=0;
	//Category
	MsgBuf[14]=0;
	MsgBuf[15]=0;
	MsgBuf[16]=0;
	//ID
	MsgBuf[17]=0;
	MsgBuf[18]=0;
	MsgBuf[19]=0;
	MsgBuf[20]=0;
	//Command Code
	MsgBuf[21]=0;
	MsgBuf[22]=0;
	MsgBuf[23]=0;
	MsgBuf[24]=0;
	MsgBuf[25]=0;
	MsgBuf[26]=0;
	MsgBuf[27]=0;
	MsgBuf[28]=0;
	
	

	while(1){
		can.LED();
	}
	return 0;
}
