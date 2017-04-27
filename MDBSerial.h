#pragma once

//define registers
#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

//MDB specific stuff
#define DATA_MAX 36

#define TIME_OUT 5
#define TIME_BREAK 100
#define TIME_SETUP 200

#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

class MDBSerial
{
public:
	MDBSerial(int uart = 0);

	void Ack();
	void Nak();
	void Ret();

	void SendCommand(int address, int cmd, int *data = 0, int dataCount = 0);
	int GetResponse(char data[] = 0, int *count = nullptr);

private:

	enum MODE {
		DATA = 0,
		ADDRESS = 1
	};

	void hardReset();
	void init();
	void write(int cmd, int mode);

	unsigned long m_commandSentTime;

	int m_TXn;

	volatile unsigned char  m_RXENn;
	volatile unsigned char  m_TXENn;
	volatile unsigned char  m_UCSZn0;
	volatile unsigned char  m_UCSZn1;
	volatile unsigned char  m_UCSZn2;
};




