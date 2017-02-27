#pragma once

//define registers
#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define UDRn UDR0
#define RXENn RXEN0
#define TXENn TXEN0
#define UBRRnH UBRR0H
#define UBRRnL UBRR0L
#define UCSRnA UCSR0A
#define UCSRnB UCSR0B
#define UCSRnC UCSR0C
#define UCSZn0 UCSZ00
#define UCSZn1 UCSZ01
#define UCSZn2 UCSZ02


#define ADDRESS 1
#define DATA 0
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
	MDBSerial();

	void ACK();
	void NAK();
	void RET();

	void SendCommand(unsigned char address, unsigned char cmd, unsigned char *data = 0, unsigned int dataCount = 0);

private:
	void init();
	void write(char cmd, int mode);
	int receive(unsigned char *data, int *mode);
	bool available();

	unsigned long m_commandSentTime;
};




