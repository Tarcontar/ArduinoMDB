#include "MDBSerial.h"
#include <Arduino.h>
#include <avr/interrupt.h>

volatile int input_buffer[999];
volatile int modes[999];
volatile int pos = 0;

ISR(USART1_RX_vect) 
{
	/*if (UCSR3B & (1 << 2)) 
	{
		data = ((UCSR3B >> 1) & 0x01);
		write_buffer(3, RX, data);
	}*/

	int resh = UCSR1B;
	modes[pos] = (resh >> 1) & 0x01;

	input_buffer[pos] = UDR1;
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

	//send the checksum
	write(sum, DATA);
}

int MDBSerial::GetResponse(int *count, int **data)
{
	int mode = 0;
	int sum = 0;
	*count = 0;
	/*while (!available())
	{
		if ((millis() - m_commandSentTime) > TIME_OUT)
		{
			return -1;
		}
	}*/

	read(&input_buffer[*count], &mode);

	//while (available() && !mode /*&& *count < DATA_MAX*/)
	//{
	//	if (read(&input_buffer[*count], &mode))
	//	{
	//		sum += input_buffer[*count];
	//		(*count)++;
	//		if (((*count) == 1) && mode == 1)
	//		{
	//			//just for testing
	//			return ACK;
	//		}
	//	}
	//}

	//TODO: check if checksum is correct
	return ACK;
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

int MDBSerial::read(volatile int *data, int *mode)
{
	unsigned char status, resh, resl;
	int count = 0;

	/*status = *m_UCSRnA;

	if (status & (1 << FE))
	{
		serial->println("NO STOP BIT");
		return 0;
	}
	else if (status & (1 << DOR))
	{
		serial->println("BUFFER FULL");
		return 0;
	}
	else if (status & (1 << UPE))
	{
		serial->println("INVALID PARITY");
		return 0;
	}*/

	//pos = 0;
	//int modes[32];
	//for (int i = 0; i < 32; i++)
	//{
	//	modes[pos] = (*m_UCSRnB >> 1) & 0x01;
	//	data[pos++] = *m_UDRn;
	//	/*if (modes[pos])
	//		break;*/
	//	//delay(1);
	//	delayMicroseconds(200);
	//}
	////serial->println(pos);

	//serial->println(pos);

	for (int i = 0; i < pos; i++)
	{
		serial->println(input_buffer[i]);
		//serial->println(modes[i]);
	}
	pos = 0;

	serial->println("######################");
	/*resh = *m_UCSRnB;
	resl = *m_UDRn;

	*mode = (resh >> 1) & 0x01;
	*data = resl;*/
	
	return 1;
}

bool MDBSerial::available()
{
	//return (*m_UCSRnA & (1 << RXC));
	return (UCSR1A & (1 << RXC));
}