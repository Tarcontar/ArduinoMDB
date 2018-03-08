#pragma once
#include <Arduino.h>

#define MDB_DEBUG TRUE//undefine to minimize code

//MDB specific stuff
#define DATA_MAX 36

#define TIME_OUT 	5
#define TIME_BREAK 	100
#define TIME_SETUP 	200

#define ACK 	0x100
#define RET 	0x1AA
#define NAK 	0x1FF

#define INTER_BYTE_TIME 	1
#define RESPONSE_TIME 		5
#define BYTE_SEND_TIME 		1.2

class MDBSerial
{
public:
	MDBSerial(uint8_t uart = 0);
	
	void Ack();
	void Nak();
	void Ret();

	void SendCommand(int address, int cmd, int *data, int dataCount);
	void SendCommand(int address, int cmd, int subCmd = -1, int *data = 0, int dataCount = 0);
	int GetResponse(char data[] = 0, int *count = 0, int num_bytes = 0);

private:
	void hardReset();
};




