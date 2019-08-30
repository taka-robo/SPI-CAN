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
	can.reset();
	can.LED();
	can.config(0,0);
	while(1){
	}
	return 0;
}
