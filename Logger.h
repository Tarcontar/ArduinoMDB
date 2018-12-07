#pragma once

#include "UART.h"
//#include "SD.h"
//#include "GPRS.hpp"
//#include "DS3231.h"

static const uint8_t SMALL_STRING = 16;

struct ERROR_NUMBER
{
	char name[SMALL_STRING];
	char number[SMALL_STRING];
};

class Logger
{
public:
	Logger(int level);

	static void SetUART(UART *uart);
	//static void SetLogFile(String file);
	//static void SetRTC(DS3231 *rtc);
	//static void SetGSM();
	//static void SetErrorNumbers(String* names, String* numbers, int count);
	
	//static int GetErrorNumbersCount();
	//static ERROR_NUMBER* GetErrorNumber(int id);
	
	static void SetDebug(bool val);
	
	Logger& operator<<(const char c);
	Logger& operator<<(const char* c);
	Logger& operator<<(const String s);
	Logger& operator<<(const __FlashStringHelper* fsh);
	Logger& operator<<(const int i);
	Logger& operator<<(const unsigned int i);
	Logger& operator<<(const long l);
	Logger& operator<<(const unsigned long lu);
	Logger& operator<<(const float f);
	Logger& operator<<(const double d);
	
private:
	void startLine();
	void endLine();
	
	template <class T>
	void print(T t);
	
private:
	int m_level;
	bool m_lineStart;
};

extern Logger debug;
extern Logger console;
extern Logger status;
extern Logger warning;
extern Logger error;
extern Logger severe;
