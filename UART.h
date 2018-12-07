#pragma once
#include <Arduino.h>

//registers
#define TXB8	0
#define UPE		2
#define DOR		3
#define FE		4
#define UDRE	5
#define RXC		7

#define UART_BUFFER_SIZE 128

static const char* endl = "\r\n";

class UART
{
public:
	UART(uint8_t uart = 1);
	
	~UART();
	
	static void clear();
	
	bool begin(uint32_t baud = 9600, bool nine_bit = false);
	void end();
	int available();
	int peek();
	void flush();
	
	inline void print(const char c) { write((uint8_t)c); }
	void print(const char* c);
	void print(const String s);
	void print(const __FlashStringHelper* fsh);
	void print(const int i);
	void print(const long l);
	void print(const unsigned long l);
	void print(const float f);
	void print(const double d);
	
	inline UART& operator<<(const char c) { print(c); return *this; }
	inline UART& operator<<(const char* c) { print(c); return *this; }
	inline UART& operator<<(const String s) { print(s); return *this; }
	inline UART& operator<<(const __FlashStringHelper* fsh) { print(fsh); return *this; }
	inline UART& operator<<(const int i) { print(i); return *this; }
	inline UART& operator<<(const long l) { print(l); return *this; }
	inline UART& operator<<(const unsigned long lu) { print(lu); return *this; }
	inline UART& operator<<(const float f) { print(f); return *this; }
	inline UART& operator<<(const double d) { print(d); return *this; }
	
	inline void println(const char c) { print(c); println(); }
	inline void println(const char* c) { print(c); println(); }
	inline void println(const String s) { print(s); println(); }
	inline void println(const __FlashStringHelper* fsh) { print(fsh); println(); }
	inline void println(const int i) { print(i); println(); }
	inline void println(const long l) { print(l); println(); }
	inline void println(const float f) { print(f); println(); }
	inline void println(const double d) { print(d); println(); }
	
	inline void println() { print(endl); }
	
	size_t write(uint8_t data);
	inline size_t write(int data) { return write((uint8_t)data); }
	size_t write9bit(uint16_t data);
	
	int read();
	bool readUL(unsigned long *val);

	bool error();
	bool ninthBitSet();
	inline uint8_t getTXPin() { return m_TXn; }
	
private:
	uint8_t m_TXn;
	uint8_t m_uart;
	
	uint8_t RXENn;
	uint8_t TXENn;
	uint8_t RXCIEn;
	uint8_t UCSZn0;
	uint8_t UCSZn1;
	uint8_t UCSZn2;
	uint8_t USBSn;
	uint8_t U2Xn;
	
	volatile uint8_t *v_UBRRnH;
	volatile uint8_t *v_UBRRnL;
	volatile uint8_t *v_UCSRnC;
};
