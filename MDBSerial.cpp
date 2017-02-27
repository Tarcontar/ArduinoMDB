#include "MDBSerial.h"

MDBSerial::MDBSerial()
{
	UDRn = UDR1;
	RXENn = RXEN1;
	TXENn = TXEN1;
	UBRRnH = UBRR1H;
	UBRRnL = UBRR1L;
	UCSRnA = UCSR1A;
	UCSRnB = UCSR1B;
	UCSRnC = UCSR1C;
	UCSZn0 = UCSZ10;
	UCSZn1 = UCSZ11;
	UCSZn2 = UCSZ12;
	init();
}

void MDBSerial::ACK()
{
	write(ACK, DATA);
}

void MDBSerial::NAK()
{
	write(NAK, DATA);
}

void MDBSerial::RET()
{
	write(RET, DATA);
}

void SendCommand(unsigned char address, unsigned char cmd, unsigned char *data = 0, unsigned int dataCount = 0)
{
	unsigned char sum = 0;
	write(address | cmd, ADDRESS);
	sum += address | cmd;

	for (unsigned int i = 0; i < dataCount; i++)
	{
		write(data[i], DATA);
		sum += data[i];
	}

	//send the checksum
	write(sum, DATA);
}

void MDBSerial::init()
{
	UCSRnB = (1 << RXENn) | (1 << TXENn);
	UBRRnH = 0;
	UBRRnL = 103; // 9600baud
	UCSRnA &= ~(1 << U2X1); //disable rate doubler
	UCSRnC |= (1 << UCSZn1) | (1 << UCSZn0); //9bit mode
	UCSRnB |= (1 << UCSZn2); //also for 9 bit mode
	UCSRnC = UCSRnC | B00000100; //one stop bit
	UCSRnC = UCSRnC | B00000000; //no parity bit
}

void MDBSerial::write(char cmd, int mode)
{
	while (!(UCSRnA & (1 << UDRE)));
	if (mode)
		UCSRnB |= (1 << TXB8);
	else
		UCSRnB &= ~(1 << TXB8);
	UDRn = cmd;
	m_commandSentTime = millis();
}

int receive(unsigned char *data, int *mode)
{
	unsigned char status, resh, resl;
	if (!available())
		return 0;
	status = UCSRnA;
	resh = UCSRnB;
	resl = UDRn;

	/*
	if (status & ((1 << FE) | (1 << DOR) | (1 << UPE)))
	{
		return 0;
	}
	*/
	*mode = (resh >> 1) & 0x01;
	*data = resl;
	return 1;
}

bool available()
{
	return (UCSRnA & (1 << RXC));
}