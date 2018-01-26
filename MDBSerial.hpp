#pragma once

#define MDB_DEBUG TRUE//undefine to minimize code

//define registers
#define TXB8 	0
#define UPE 	2
#define DOR 	3
#define FE 		4
#define UDRE 	5
#define RXC 	7

//MDB specific stuff
#define DATA_MAX 36

#define TIME_OUT 		5
#define TIME_BREAK 	100
#define TIME_SETUP 	200

#define ACK 	0x00
#define RET 	0xAA
#define NAK 	0xFF

#define INTER_BYTE_TIME 	1
#define RESPONSE_TIME 		5
#define BYTE_SEND_TIME 	1.2

class MDBSerial
{
public:
	MDBSerial(int uart = 0);
	
	void Ack();
	void Nak();
	void Ret();

	void SendCommand(int address, int cmd, int *data, int dataCount);
	void SendCommand(int address, int cmd, int subCmd = -1, int *data = 0, int dataCount = 0);
	int GetResponse(char data[] = 0, int *count = 0, int num_bytes = 0);

private:

	enum MODE {
		DATA = 0,
		ADDRESS = 1
	};

	void hardReset();
	void init();
	void write(int cmd, int mode);

private:
	unsigned long m_commandSentTime;

	int m_TXn;

	volatile unsigned char  m_RXENn;
	volatile unsigned char  m_TXENn;
	volatile unsigned char  m_UCSZn0;
	volatile unsigned char  m_UCSZn1;
	volatile unsigned char  m_UCSZn2;
};




