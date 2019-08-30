#ifndef _MCP2515_H_
#define _MCP2515_H_

#define SPI_CHANNEL0 0
#define SPI_SPEED 5000000
#define SPI_SS_PORT 22
//command
#define RESET 0xC0
#define WRITE 0x02
#define READ 0x03
#define BITMODIFY 0x05

#define	BFPCTRL 0x0C
#define CANCTRL 0xF
#define CANSTAT 0xE
#define CNF1 0x2A
#define CNF2 0x29

#define B1BFE 3
#define B1BFS 5
#define B0BFE 2
#define B0BFS 4

class MCP2515 {
public:
	MCP2515(
		int theSPIChannel = SPI_CHANNEL0,
		int theSPISpeed = SPI_SPEED,
		int theSSpoet = SPI_SS_PORT
	);
	virtual ~MCP2515();
	int GPIOSetUp();
	int SPISetUp();
	void write(unsigned char addr, unsigned char *buf);
	void BitModWrite(unsigned char addr, unsigned char *buf); 
	void LED();
	void config(int mode,int rate);
	void reset();

private:
	struct MCP2515Parameters{
		int Speed;
		int SSport;
	}MCP2515Param;
	unsigned char buff[10];
	int channelSPI;
};

#endif
