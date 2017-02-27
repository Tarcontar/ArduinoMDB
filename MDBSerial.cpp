#include "MDBSerial.h"
#include <Arduino.h>

MDBSerial::MDBSerial(int uart)
{
	switch (uart)
	{
	case 1:
		m_UDRn = UDR1;
		m_RXENn = RXEN1;
		m_TXENn = TXEN1;
		m_UBRRnH = UBRR1H;
		m_UBRRnL = UBRR1L;
		m_UCSRnA = UCSR1A;
		m_UCSRnB = UCSR1B;
		m_UCSRnC = UCSR1C;
		m_UCSZn0 = UCSZ10;
		m_UCSZn1 = UCSZ11;
		m_UCSZn2 = UCSZ12;
		break;
	case 2:
		m_UDRn = UDR2;
		m_RXENn = RXEN2;
		m_TXENn = TXEN2;
		m_UBRRnH = UBRR2H;
		m_UBRRnL = UBRR2L;
		m_UCSRnA = UCSR2A;
		m_UCSRnB = UCSR2B;
		m_UCSRnC = UCSR2C;
		m_UCSZn0 = UCSZ20;
		m_UCSZn1 = UCSZ21;
		m_UCSZn2 = UCSZ22;
		break;
	case 3:
		m_UDRn = UDR3;
		m_RXENn = RXEN3;
		m_TXENn = TXEN3;
		m_UBRRnH = UBRR3H;
		m_UBRRnL = UBRR3L;
		m_UCSRnA = UCSR3A;
		m_UCSRnB = UCSR3B;
		m_UCSRnC = UCSR3C;
		m_UCSZn0 = UCSZ30;
		m_UCSZn1 = UCSZ31;
		m_UCSZn2 = UCSZ32;
		break;
	default:
		m_UDRn = UDR0;
		m_RXENn = RXEN0;
		m_TXENn = TXEN0;
		m_UBRRnH = UBRR0H;
		m_UBRRnL = UBRR0L;
		m_UCSRnA = UCSR0A;
		m_UCSRnB = UCSR0B;
		m_UCSRnC = UCSR0C;
		m_UCSZn0 = UCSZ00;
		m_UCSZn1 = UCSZ01;
		m_UCSZn2 = UCSZ02;
		break;
	}
	
	init();
}

void MDBSerial::Ack()
{
	write(ACK, DATA);
}

void MDBSerial::Nak()
{
	write(NAK, DATA);
}

void MDBSerial::Ret()
{
	write(RET, DATA);
}

void MDBSerial::SendCommand(int address, int cmd, int *data, int dataCount)
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

int *MDBSerial::GetResponse(int *count)
{
	int mode = 0;
	int sum = 0;
	*count = 0;
	while (!available())
	{
		if ((millis() - m_commandSentTime) > TIME_OUT)
		{
			return nullptr;
		}
	}
	while (available() && !mode && *count < DATA_MAX)
	{
		if (read(&input_buffer[*count], &mode))
		{
			sum += input_buffer[*count];
			*count++;
			if ((*count == 1) && mode == 1)
			{
				return &input_buffer[(*count) - 1];
			}
		}
	}
	//TODO: check if checksum is correct
	return input_buffer;
}

void MDBSerial::init()
{
	m_UCSRnB = (1 << m_RXENn) | (1 << m_TXENn);
	m_UBRRnH = 0;
	m_UBRRnL = 103; // 9600baud
	m_UCSRnA &= ~(1 << U2X1); //disable rate doubler
	m_UCSRnC |= (1 << m_UCSZn1) | (1 << m_UCSZn0); //9bit mode
	m_UCSRnB |= (1 << m_UCSZn2); //also for 9 bit mode
	m_UCSRnC = m_UCSRnC | 0b00000100; //one stop bit
	m_UCSRnC = m_UCSRnC | 0b00000000; //no parity bit
}

void MDBSerial::write(char cmd, int mode)
{
	while (!(m_UCSRnA & (1 << UDRE)));
	if (mode)
		m_UCSRnB |= (1 << TXB8);
	else
		m_UCSRnB &= ~(1 << TXB8);
	m_UDRn = cmd;
	m_commandSentTime = millis();
}

int MDBSerial::read(int *data, int *mode)
{
	unsigned char status, resh, resl;
	if (!available())
		return 0;
	status = m_UCSRnA;
	resh = m_UCSRnB;
	resl = m_UDRn;

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

bool MDBSerial::available()
{
	return (m_UCSRnA & (1 << RXC));
}