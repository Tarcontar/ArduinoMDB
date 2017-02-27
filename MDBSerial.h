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

#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

#define RESET 0x00
#define SETUP 0x01
#define STATUS 0x02
#define POLL 0x03
#define TYPE 0x04
#define EXPANSION 0x07

class MDBSerial
{
public:
	MDBSerial(int uart = 0);

	void Ack();
	void Nak();
	void Ret();

	void SendCommand(int address, int cmd, int *data = 0, int dataCount = 0);
	int* GetResponse(int *count);

private:
	enum MODE {
		DATA = 0,
		ADDRESS = 1
	};

	void init();
	void write(char cmd, int mode);
	int read(int *data, int *mode);
	bool available();

	unsigned long m_commandSentTime;
	int input_buffer[DATA_MAX];

	int m_UDRn;
	int m_RXENn;
	int m_TXENn;
	int m_UBRRnH;
	int m_UBRRnL;
	int m_UCSRnA;
	int m_UCSRnB;
	int m_UCSRnC;
	int m_UCSZn0;
	int m_UCSZn1;
	int m_UCSZn2;
};




