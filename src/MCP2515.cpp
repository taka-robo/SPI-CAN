#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <iostream>
#include "MCP2515.hpp"

MCP2515::MCP2515(int theSPIChannel,int theSPISpeed,int theSSPort)
{
	channelSPI = theSPIChannel;
	MCP2515Param.Speed = theSPISpeed;
	MCP2515Param.SSport = theSSPort;
}
int MCP2515::GPIOSetUp()
{
	return wiringPiSetupGpio();
}
int MCP2515::SPISetUp()
{
	pinMode(MCP2515Param.SSport,OUTPUT);
	digitalWrite(MCP2515Param.SSport,1);
	return wiringPiSPISetup(channelSPI,MCP2515Param.Speed);
}
void MCP2515::write(unsigned char addr,unsigned char *buf)
{
	digitalWrite(MCP2515Param.SSport,0);
	buff[0] = WRITE;
	buff[1] = addr;
	buff[2] = buf[0];
	wiringPiSPIDataRW(channelSPI,buff,3);
	digitalWrite(MCP2515Param.SSport,1);	
}
unsigned char MCP2515::read(unsigned char addr)
{
	digitalWrite(MCP2515Param.SSport,0);
	buff[0] = READ;
	buff[1] = addr;
	buff[2] = 0;//dont care
	wiringPiSPIDataRW(channelSPI,buff,3);
	digitalWrite(MCP2515Param.SSport,1);	
	return buff[2];
	
}
void MCP2515::BitModWrite(unsigned char addr,unsigned char *buf)
{
	digitalWrite(MCP2515Param.SSport,0);
	buff[0] = BITMODIFY;
	buff[1] = addr;
	buff[2] = buf[0];
	buff[3] = buf[1];	
	wiringPiSPIDataRW(channelSPI,buff,4);
	digitalWrite(MCP2515Param.SSport,1);	
}	
void MCP2515::LED()
{
	unsigned char buf[2];
	std::cout<<"LED ON"<<std::endl;
	buf[0] = (1<<B1BFE)|(1<<B1BFS);
	buf[1] = (1<<B1BFE)|(1<<B1BFS);
	BitModWrite(BFPCTRL,buf);
	delay(1000);
	std::cout<<"LED OFF"<<std::endl;
	buf[0] = (1<<B1BFE)|(1<<B1BFS);
	buf[1] = (1<<B1BFE);
	BitModWrite(BFPCTRL,buf);
	delay(1000);
       		
}
void MCP2515::config(int mode,int rate)
{
	//buff[0]=0;
	//write(CNF1,buff);
	//delay(1);
	printf("%d\n",read(CANSTAT));

	//set normal operation mode
	buff[0]=0;
	write(CANCTRL,buff);		

	printf("%d\n",read(CANSTAT));
	
}
void MCP2515::reset()
{
	std::cout<<"MCP2515 reset"<<std::endl;
	digitalWrite(MCP2515Param.SSport,0);
	buff[0] = RESET;
	wiringPiSPIDataRW(channelSPI,buff,1);
	digitalWrite(MCP2515Param.SSport,1);
}
MCP2515::~MCP2515()
{

}
