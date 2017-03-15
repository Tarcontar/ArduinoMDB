#pragma once

//define registers
#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define DATA_MAX 36

#define TIME_OUT 5
#define TIME_BREAK 100
#define TIME_SETUP 200

#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

#define RESET 0x00
#define SETUP 0x01
#define STATUS 0x02
#define POLL 0x03
#define TYPE 0x04
#define EXPANSION 0x07

#include "HardwareSerial.h"

class MDBSerial
{
public:
	MDBSerial(int uart = 0);

	void SetSerial(HardwareSerial &print);

	void Ack();
	void Nak();
	void Ret();

	void SendCommand(int address, int cmd, int *data = 0, int dataCount = 0);
	int GetResponse(int *count, int **data = 0);

private:
	HardwareSerial *serial;

	enum MODE {
		DATA = 0,
		ADDRESS = 1
	};

	void hardReset();
	void init();
	void write(int cmd, int mode);
	int read(volatile int *data, int *mode);
	bool available();

	unsigned long m_commandSentTime;

	int m_TXn;

	volatile unsigned char *m_UDRn;
	volatile unsigned char  m_RXENn;
	volatile unsigned char  m_TXENn;
	volatile unsigned char *m_UBRRnH;
	volatile unsigned char *m_UBRRnL;
	volatile unsigned char *m_UCSRnA;
	volatile unsigned char *m_UCSRnB;
	volatile unsigned char *m_UCSRnC;
	volatile unsigned char  m_UCSZn0;
	volatile unsigned char  m_UCSZn1;
	volatile unsigned char  m_UCSZn2;
};




