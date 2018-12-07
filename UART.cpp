#include "UART.h"
#include <Arduino.h>
#include <avr/interrupt.h>

bool uarts_in_use[4];

volatile uint16_t v_buffer[4][UART_BUFFER_SIZE];
volatile uint16_t v_start[4];
volatile uint16_t v_end[4];
volatile bool v_error[4];
volatile bool v_ninthBitSet[4];

volatile uint8_t *v_UDRn[4];
volatile uint8_t *v_UCSRnA[4];
volatile uint8_t *v_UCSRnB[4];


UART::UART(uint8_t uart)
{
	m_uart = uart % 4;
}

UART::~UART()
{
	end();
}

void UART::clear()
{
	for (int i = 0; i < 4; i++)
		uarts_in_use[i] = false;
}

bool UART::begin(uint32_t baud, bool nine_bit)
{
	if (uarts_in_use[m_uart])
		return false;
	uarts_in_use[m_uart] = true;
	
	v_start[m_uart] = 0;
	v_end[m_uart] = 0;
	v_error[m_uart] = false;
	v_ninthBitSet[m_uart] = false;
	if (m_uart == 0)
	{
		m_TXn = 1;
		RXENn = RXEN0;
		TXENn = TXEN0;
		UCSZn0 = UCSZ00;
		UCSZn1 = UCSZ01;
		UCSZn2 = UCSZ02;
		USBSn = USBS0;
		U2Xn = U2X0;
		RXCIEn = RXCIE0;
		
		v_UDRn[m_uart] = &UDR0;
		v_UBRRnH = &UBRR0H;
		v_UBRRnL = &UBRR0L;
		v_UCSRnA[m_uart] = &UCSR0A;
		v_UCSRnB[m_uart] = &UCSR0B;
		v_UCSRnC = &UCSR0C;
	}
	else if (m_uart == 2)
	{
		m_TXn = 16;
		RXENn = RXEN2;
		TXENn = TXEN2;
		UCSZn0 = UCSZ20;
		UCSZn1 = UCSZ21;
		UCSZn2 = UCSZ22;
		USBSn = USBS2;
		U2Xn = U2X2;
		RXCIEn = RXCIE2;
		
		v_UDRn[m_uart] = &UDR2;
		v_UBRRnH = &UBRR2H;
		v_UBRRnL = &UBRR2L;
		v_UCSRnA[m_uart] = &UCSR2A;
		v_UCSRnB[m_uart] = &UCSR2B;
		v_UCSRnC = &UCSR2C;
	}
	else if (m_uart == 3)
	{
		m_TXn = 14;
		RXENn = RXEN3;
		TXENn = TXEN3;
		UCSZn0 = UCSZ30;
		UCSZn1 = UCSZ31;
		UCSZn2 = UCSZ32;
		USBSn = USBS3;
		U2Xn = U2X3;
		RXCIEn = RXCIE3;
		
		v_UDRn[m_uart] = &UDR3;
		v_UBRRnH = &UBRR3H;
		v_UBRRnL = &UBRR3L;
		v_UCSRnA[m_uart] = &UCSR3A;
		v_UCSRnB[m_uart] = &UCSR3B;
		v_UCSRnC = &UCSR3C;
	}
	else
	{
		m_TXn = 18;
		RXENn = RXEN1;
		TXENn = TXEN1;
		UCSZn0 = UCSZ10;
		UCSZn1 = UCSZ11;
		UCSZn2 = UCSZ12;
		USBSn = USBS1;
		U2Xn = U2X1;
		RXCIEn = RXCIE1;
		
		v_UDRn[m_uart] = &UDR1;
		v_UBRRnH = &UBRR1H;
		v_UBRRnL = &UBRR1L;
		v_UCSRnA[m_uart] = &UCSR1A;
		v_UCSRnB[m_uart] = &UCSR1B;
		v_UCSRnC = &UCSR1C;
	}
	//set also UDRIE0 ?? 
	*v_UCSRnB[m_uart] |= (1 << RXENn) | (1 << TXENn) | (1 << RXCIEn);
	*v_UCSRnC |= (1 << UCSZn1) | (1 << UCSZn0); //8 bit mode
	if (nine_bit)
		*v_UCSRnB[m_uart] |= (1 << UCSZn2); 
	*v_UCSRnC |= (0 << USBSn); //one stop bit else 1
	*v_UCSRnC |= 0b00000000; //no parity bit
	
	uint16_t baud_setting = (F_CPU / 8 / baud - 1) / 2;
	*v_UBRRnH = baud_setting >> 8;
	*v_UBRRnL = baud_setting;
	*v_UCSRnA[m_uart] &= ~(1 << U2Xn); //disable rate doubler
	return true;
}

void UART::end()
{
	flush();
	*v_UCSRnB[m_uart] |= (0 << RXENn) | (0 << TXENn) | (0 << RXCIEn);
	uarts_in_use[m_uart] = false;
}

int UART::available()
{
	return ((int)(UART_BUFFER_SIZE + v_end[m_uart] - v_start[m_uart])) % UART_BUFFER_SIZE;
}

int UART::peek()
{
	if (v_start[m_uart] == v_end[m_uart]) {
		return -1;
	} else {
		v_buffer[m_uart][v_start[m_uart]];
	}
}

void UART::flush()
{
	v_start[m_uart] = 0;
	v_end[m_uart] = 0;
}

void UART::print(const char* c)
{
	while (*c)
	{
		*this << *c;
		++c;
	}
}

void UART::print(String s)
{
	print(const_cast<char*>(s.c_str()));
}

void UART::print(const __FlashStringHelper* fsh)
{
	PGM_P p = reinterpret_cast<PGM_P>(fsh);
	while (1) 
	{
		unsigned char c = pgm_read_byte(p++);
		if (c == 0) break;
		if (!write(c))
			break;
	}
}

void UART::print(const int i)
{
	char str[30];
	sprintf(str, "%d", i);
	*this << str;
}

void UART::print(const long l)
{
	*this << (int)l;
}

void UART::print(const unsigned long lu)
{
	char str[30];
	sprintf(str, "%lu", lu);
	*this << str;
}

void UART::print(const float f)
{
	char str[30];
	int num = f;
	int komma = (f-num) * 100;
	sprintf(str, "%d.%d", num, komma);
	*this << str;
}

void UART::print(const double d)
{
	*this << (float)d;
}


size_t UART::write(uint8_t data)
{
	v_ninthBitSet[m_uart] = false;
	while (!(*v_UCSRnA[m_uart] & (1 << UDRE))) {}
	*v_UDRn[m_uart] = data;
}

size_t UART::write9bit(uint16_t data)
{
	while (!(*v_UCSRnA[m_uart] & (1 << UDRE))) {}
	if (data & 0x100)
		*v_UCSRnB[m_uart] |= (1 << TXB8);
	else
		*v_UCSRnB[m_uart] &= ~(1 << TXB8);
	write((uint8_t)data);
}

int UART::read()
{
	if (v_start[m_uart] == v_end[m_uart]) {
		return -1;
	} else {
		int c = v_buffer[m_uart][v_start[m_uart]];
		v_start[m_uart] = (v_start[m_uart] + 1) % UART_BUFFER_SIZE;
		return c;
	}
}

bool UART::readUL(unsigned long *val)
{
	if (available() >= 4)
	{	
		union u_tag {
			byte b[4];
			unsigned long ulval;
		} u;
		u.b[0] = read();
		u.b[1] = read();
		u.b[2] = read();
		u.b[3] = read();
		*val = u.ulval;
		return true;
	}
	return false;
}

bool UART::error()
{
	if (v_error[m_uart])
	{
		v_error[m_uart] = false;
		return true;
	}
	return false;
}

bool UART::ninthBitSet()
{
	if (v_ninthBitSet[m_uart])
	{
		v_ninthBitSet[m_uart] = false;
		return true;
	}
	return false;
}

void receive(int id)
{
	char status = *v_UCSRnA[id];
	if (status & ((1 << FE) | (1 << DOR) | (1 << UPE)))
	{
		v_error[id] = true;
	}
	uint16_t result = ((*v_UCSRnB[id] >> 1) & 0x01) << 8;
	if (result & 0x100)
		v_ninthBitSet[id] = true;
	result |= *v_UDRn[id];
	v_buffer[id][v_end[id]] = result;
	v_end[id] = (v_end[id] + 1) % UART_BUFFER_SIZE;
	if (v_end[id] == v_start[id])
	{
		v_start[id] = (v_start[id] + 1) % UART_BUFFER_SIZE;
	}
}

ISR(USART0_RX_vect)
{
	receive(0);
}

ISR(USART1_RX_vect)
{
	receive(1);
}

ISR(USART2_RX_vect)
{
	receive(2);
}

ISR(USART3_RX_vect)
{
	receive(3);
}