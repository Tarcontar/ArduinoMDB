#include "MDBSerial.h"
#include <Arduino.h>
#include <avr/interrupt.h>

struct data
{
	uint8_t value;
	uint8_t mode;
};

volatile data buffer[100];
volatile int pos = 0;
volatile bool error = false;

ISR(USART1_RX_vect) 
{
	int status = (uint8_t)UCSR1A;
	
	/*if (status & ((1 << FE) | (1 << DOR) | (1 << UPE)))
	{
		error = true;
	}*/
	buffer[pos].mode = (uint8_t)((UCSR1B >> 1) & 0x01);
	buffer[pos].value = (uint8_t)UDR1;
	pos++;
}

MDBSerial::MDBSerial(int uart)
{
	switch (uart)
	{
	case 1:
		m_TXn = 18;
		m_UDRn = &(UDR1);
		m_RXENn = RXEN1;
		m_TXENn = TXEN1;
		m_UBRRnH = &(UBRR1H);
		m_UBRRnL = &(UBRR1L);
		m_UCSRnA = &(UCSR1A);
		m_UCSRnB = &(UCSR1B);
		m_UCSRnC = &(UCSR1C);
		m_UCSZn0 = UCSZ10;
		m_UCSZn1 = UCSZ11;
		m_UCSZn2 = UCSZ12;
		break;

	case 2:
		m_TXn = 16;
		m_UDRn = &(UDR2);
		m_RXENn = RXEN2;
		m_TXENn = TXEN2;
		m_UBRRnH = &(UBRR2H);
		m_UBRRnL = &(UBRR2L);
		m_UCSRnA = &(UCSR2A);
		m_UCSRnB = &(UCSR2B);
		m_UCSRnC = &(UCSR2C);
		m_UCSZn0 = UCSZ20;
		m_UCSZn1 = UCSZ21;
		m_UCSZn2 = UCSZ22;
		break;

	case 3:
		m_TXn = 14;
		m_UDRn = &(UDR3);
		m_RXENn = RXEN3;
		m_TXENn = TXEN3;
		m_UBRRnH = &(UBRR3H);
		m_UBRRnL = &(UBRR3L);
		m_UCSRnA = &(UCSR3A);
		m_UCSRnB = &(UCSR3B);
		m_UCSRnC = &(UCSR3C);
		m_UCSZn0 = UCSZ30;
		m_UCSZn1 = UCSZ31;
		m_UCSZn2 = UCSZ32;
		break;

	default:
		m_TXn = 1;
		m_UDRn = &(UDR0);
		m_RXENn = RXEN0;
		m_TXENn = TXEN0;
		m_UBRRnH = &(UBRR0H);
		m_UBRRnL = &(UBRR0L);
		m_UCSRnA = &(UCSR0A);
		m_UCSRnB = &(UCSR0B);
		m_UCSRnC = &(UCSR0C);
		m_UCSZn0 = UCSZ00;
		m_UCSZn1 = UCSZ01;
		m_UCSZn2 = UCSZ02;
		break;
	}
	
	//hardReset(); //does not work at the moment
	init();
}

void MDBSerial::SetSerial(HardwareSerial &print)
{
	serial = &print;
	serial->begin(9600);
}

void MDBSerial::hardReset()
{
	pinMode(m_TXn, OUTPUT);
	digitalWrite(m_TXn, HIGH);
	delay(TIME_SETUP);
	digitalWrite(m_TXn, LOW);
	delay(TIME_BREAK);
	digitalWrite(m_TXn, HIGH);
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
	write(sum, DATA); //send the checksum
}

int MDBSerial::GetResponse(int **data, int *count)
{
	uint8_t sum = 0;
	if (count != nullptr)
		*count = 0;
	if (error)
	{
		serial->println("receive error");
		error = false;
		pos = 0;
		return -1;
	}
	if (count != nullptr)
		*count = pos;
	for (int i = 0; i < pos; i++)
	{
		int val = buffer[i].value;
		if (buffer[i].mode != 1)
		{
			/*if (data != nullptr)
				*(data)[i] = val;*/
			sum += val;
		}
		else
		{
			if (pos == 1) //we got an ACK
			{
				serial->println("got ACK");
				pos = 0;
				return ACK;
			}
			else //checksum of data
			{
				if (sum != val)
				{
					serial->println("wrong checksum");
					pos = 0;
					return -1;
				}
			}
		}
	}
	pos = 0;
	return 1;
}

void MDBSerial::init()
{
	/*
	m_UCSRnB = (1 << m_RXENn) | (1 << m_TXENn);
	m_UBRRnH = 0;
	m_UBRRnL = 103; // 9600baud
	m_UCSRnA &= ~(1 << U2X1); //disable rate doubler //should be U2Xn ??
	m_UCSRnC |= (1 << m_UCSZn1) | (1 << m_UCSZn0); //9bit mode
	m_UCSRnB |= (1 << m_UCSZn2); //also for 9 bit mode
	m_UCSRnC = m_UCSRnC | 0b00000100; //one stop bit
	m_UCSRnC = m_UCSRnC | 0b00000000; //no parity bit
	*/

	UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
	UBRR1H = 0;
	UBRR1L = 103; // 9600baud
	UCSR1A &= ~(1 << U2X1); //disable rate doubler //should be U2Xn ??
	UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10); //9bit mode
	UCSR1B |= (1 << UCSZ12); //also for 9 bit mode
	UCSR1C = UCSR1C | 0b00000100; //one stop bit
	UCSR1C = UCSR1C | 0b00000000; //no parity bit
}

void MDBSerial::write(int cmd, int mode)
{
	/*
	while (!(m_UCSRnA & (1 << UDRE)));
	if (mode)
		m_UCSRnB |= (1 << TXB8);
	else
		m_UCSRnB &= ~(1 << TXB8);
	m_UDRn = cmd;
	m_commandSentTime = millis();
	*/

	while (!(UCSR1A & (1 << UDRE)));
	if (mode)
		UCSR1B |= (1 << TXB8);
	else
		UCSR1B &= ~(1 << TXB8);
	UDR1 = cmd;
	m_commandSentTime = millis();
}