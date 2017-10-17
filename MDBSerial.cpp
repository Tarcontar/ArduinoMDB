#include "MDBSerial.hpp"
#include <Arduino.h>
#include <avr/interrupt.h>

struct data
{
	char value;
	char mode;
};

volatile data v_buffer[100];
volatile int v_pos = 0;
volatile bool v_error = false;
volatile bool v_msg_complete = false;
volatile int v_uart = 0;

volatile unsigned char *m_UDRn;
volatile unsigned char *m_UBRRnH;
volatile unsigned char *m_UBRRnL;
volatile unsigned char *m_UCSRnA;
volatile unsigned char *m_UCSRnB;
volatile unsigned char *m_UCSRnC;

MDBSerial::MDBSerial(int uart) 
{
	v_uart = uart;
	if (uart == 1)
	{
		m_TXn = 18;
		m_UDRn = &UDR1;
		m_RXENn = RXEN1;
		m_TXENn = TXEN1;
		m_UBRRnH = &UBRR1H;
		m_UBRRnL = &UBRR1L;
		m_UCSRnA = &UCSR1A;
		m_UCSRnB = &UCSR1B;
		m_UCSRnC = &UCSR1C;
		m_UCSZn0 = UCSZ10;
		m_UCSZn1 = UCSZ11;
		m_UCSZn2 = UCSZ12;
	}
	else if (uart == 2)
	{
		m_TXn = 16;
		m_UDRn = &UDR2;
		m_RXENn = RXEN2;
		m_TXENn = TXEN2;
		m_UBRRnH = &UBRR2H;
		m_UBRRnL = &UBRR2L;
		m_UCSRnA = &UCSR2A;
		m_UCSRnB = &UCSR2B;
		m_UCSRnC = &UCSR2C;
		m_UCSZn0 = UCSZ20;
		m_UCSZn1 = UCSZ21;
		m_UCSZn2 = UCSZ22;
	}
	else if (uart == 3)
	{
		m_TXn = 14;
		m_UDRn = &UDR3;
		m_RXENn = RXEN3;
		m_TXENn = TXEN3;
		m_UBRRnH = &UBRR3H;
		m_UBRRnL = &UBRR3L;
		m_UCSRnA = &UCSR3A;
		m_UCSRnB = &UCSR3B;
		m_UCSRnC = &UCSR3C;
		m_UCSZn0 = UCSZ30;
		m_UCSZn1 = UCSZ31;
		m_UCSZn2 = UCSZ32;
	}
	else
	{
		m_TXn = 1;
		m_UDRn = &UDR0;
		m_RXENn = RXEN0;
		m_TXENn = TXEN0;
		m_UBRRnH = &UBRR0H;
		m_UBRRnL = &UBRR0L;
		m_UCSRnA = &UCSR0A;
		m_UCSRnB = &UCSR0B;
		m_UCSRnC = &UCSR0C;
		m_UCSZn0 = UCSZ00;
		m_UCSZn1 = UCSZ01;
		m_UCSZn2 = UCSZ02;
	}
	//hardReset(); //does not work at the moment
	init();
}

ISR(USART1_RX_vect)
{
	if (v_uart == 1)
	{
		char status = *m_UCSRnA;

		if (status & ((1 << FE) | (1 << DOR) | (1 << UPE)))
		{
			v_error = true;
		}
		v_buffer[v_pos].mode = (*m_UCSRnB >> 1) & 0x01;
		if (v_buffer[v_pos].mode == 1)
		{
			//we got an ack or an checksum
			v_msg_complete = true;
		}
		v_buffer[v_pos++].value = *m_UDRn;
	}
}

ISR(USART2_RX_vect)
{
	if (v_uart == 2)
	{
		char status = *m_UCSRnA;

		if (status & ((1 << FE) | (1 << DOR) | (1 << UPE)))
		{
			v_error = true;
		}
		v_buffer[v_pos].mode = (*m_UCSRnB >> 1) & 0x01;
		if (v_buffer[v_pos].mode == 1)
		{
			//we got an ack or an checksum
			v_msg_complete = true;
		}
		v_buffer[v_pos++].value = *m_UDRn;
	}
}

ISR(USART3_RX_vect)
{
	if (v_uart == 3)
	{
		char status = *m_UCSRnA;

		if (status & ((1 << FE) | (1 << DOR) | (1 << UPE)))
		{
			v_error = true;
		}
		v_buffer[v_pos].mode = (*m_UCSRnB >> 1) & 0x01;
		if (v_buffer[v_pos].mode == 1)
		{
			//we got an ack or an checksum
			v_msg_complete = true;
		}
		v_buffer[v_pos++].value = *m_UDRn;
	}
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

void MDBSerial::SendCommand(int address, int cmd, int *data, int dataCount, int subCmd)
{
	char sum = 0;
	write(address | cmd, ADDRESS);
	
	//subcommand followed by checksum? subcommand with address?
	if (subCmd >= 0)
	{
		write(subCmd, ADDRESS);
		//write(subCmd, DATA);
		//write(address | subCmd, ADDRESS);
		sum += subCmd;
		//sum += address | subCmd;
	}
	
	sum += address | cmd;

	for (int i = 0; i < dataCount; i++)
	{
		write(data[i], DATA);
		sum += data[i];
	}
	write(sum, DATA); //send the checksum
	delay(RESPONSE_TIME * 2);
}

int MDBSerial::GetResponse(char data[], int *count, int num_bytes)
{	
	char sum = 0;
	*count = 0;
	
	//wait for the response to arrive
	delay(RESPONSE_TIME);
	for (int i = 0; i < num_bytes; i++)
	{
		if (v_msg_complete)
		{
			break;
		}
		else if (v_error)
		{
			v_error = false;
			v_pos = 0;
			return -1;
		}
		else
		{
			delay(BYTE_SEND_TIME + INTER_BYTE_TIME);
		}
	}
	
	if (v_pos == 0)
		return -2;
 
    *count = v_pos - 1; //do not send checksum
	for (int i = 0; i < v_pos; i++)
	{
		char val = v_buffer[i].value;
		if (v_buffer[i].mode != 1)
		{
			data[i] = val;
			sum += val;
		}
		else
		{
			if (v_pos == 1) //we got an ACK
			{
				v_pos = 0;
				return ACK;
			}
			else //checksum of data
			{
				if (sum != val)
				{
					v_pos = 0;
					return -3;
				}
			}
		}
	}
	v_pos = 0;
	return 1;
}

void MDBSerial::init()
{
	*m_UCSRnB = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
	*m_UBRRnH = 0;
	*m_UBRRnL = 103; // 9600baud
	*m_UCSRnA &= ~(1 << U2X1); //disable rate doubler //should be U2Xn ??
	*m_UCSRnB |= (1 << UCSZ12); //9 bit mode
	*m_UCSRnC |= (1 << UCSZ11) | (1 << UCSZ10); //also 9bit mode
	*m_UCSRnC |= 0b00000100; //one stop bit
	*m_UCSRnC |= 0b00000000; //no parity bit
}

void MDBSerial::write(int cmd, int mode)
{
	while (!(*m_UCSRnA & (1 << UDRE))) {}
	if (mode)
		*m_UCSRnB |= (1 << TXB8);
	else
		*m_UCSRnB &= ~(1 << TXB8);
	*m_UDRn = cmd;
	m_commandSentTime = millis();
}