#include "Logger.h"

static UART *s_uart;
static bool s_uartSet = false;

//static String s_fileString;
//static File s_file;
//static bool s_fileSet = false;

//static DS3231 *s_rtc;
//static bool s_rtcSet = false;

static bool s_debug = false;

//static ERROR_NUMBER *s_errorNumbers;
//static uint8_t s_errorNumbersCount = 0;

Logger debug(0);
Logger console(1);
Logger status(2);
Logger warning(3);
Logger error(4);
Logger severe(5);


Logger::Logger(int level) : m_level(level), m_lineStart(true)
{
}

void Logger::SetUART(UART *uart)
{
	s_uart = uart;
	s_uartSet = true;
}

/*
void Logger::SetLogFile(String file)
{
	s_fileString = file;
	s_fileSet = true;
}

void Logger::SetRTC(DS3231 *rtc)
{
	s_rtc = rtc;
	s_rtcSet = true;
}

void Logger::SetGSM()
{
}
*/

/*
void Logger::SetErrorNumbers(String* names, String* numbers, int count)
{
	s_errorNumbersCount = count;
	s_errorNumbers = new ERROR_NUMBER[count];
	for (int i = 0; i < count; i++)
	{
		names[i].toCharArray(s_errorNumbers[i].name, SMALL_STRING);
		numbers[i].toCharArray(s_errorNumbers[i].number, SMALL_STRING);
	}
}

int Logger::GetErrorNumbersCount()
{
	return s_errorNumbersCount;
}

ERROR_NUMBER* Logger::GetErrorNumber(int id)
{
	return &s_errorNumbers[id];
}
*/

void Logger::SetDebug(bool val)
{
	s_debug = val;
}

Logger& Logger::operator<<(const char c) 
{ 
	print(c);
	if (c == '\n' || c == '\r') 
		endLine();
	return *this; 
}

Logger& Logger::operator<<(const char* c)
{
	print(c);
	if (c[0] == '\r' && c[1] == '\n')
		endLine();
	return *this; 
}

Logger& Logger::operator<<(const String s)
{
	print(s);
	return *this; 
}

Logger& Logger::operator<<(const __FlashStringHelper* fsh)
{
	print(fsh);
	return *this; 
}

Logger& Logger::operator<<(const int i)
{
	print(i);
	return *this; 
}

Logger& Logger::operator<<(const unsigned int i)
{
	print((long)i);
	return *this; 
}

Logger& Logger::operator<<(const long l)
{
	print(l);
	return *this; 
}

Logger& Logger::operator<<(const unsigned long lu)
{
	print(lu);
	return *this; 
}

Logger& Logger::operator<<(const float f)
{
	print(f);
	return *this; 
}

Logger& Logger::operator<<(const double d)
{
	print(d);
	return *this; 
}

template <class T>
void Logger::print(T t)
{
	startLine();
	if (s_uartSet)
	{
		if (m_level > 0)
			*s_uart << t;
		else
			if (s_debug)
				*s_uart << t;
	}
	/*
	if (m_level > 1)
	{	
		if (s_fileSet)
			s_file.print(t);
	}
	*/
}

void Logger::startLine()
{
	if (m_lineStart)
	{
		if (m_level > 1)
		{
			/*
			s_file = SD.open(s_fileString, FILE_WRITE);
			if (s_rtcSet)
			{
				s_file.print(s_rtc->getDateStr());
				s_file.print(" / ");
				s_file.print(s_rtc->getTimeStr());
				s_file.print(" / ");
			}
			*/
			if (m_level == 3)
			{
				if (s_uartSet)
					*s_uart << F("WARNING: ");
				//if (s_fileSet)
				//	s_file.print(F("WARNING: "));
			}
			else if (m_level > 3)
			{
				if (s_uartSet)
					*s_uart << F("ERROR: ");
				//if (s_fileSet)
				//	s_file.print(F("ERROR: "));
			}
		}
	}
	m_lineStart = false;
}

void Logger::endLine()
{
	//if (m_level > 1)
	//	s_file.close();
	m_lineStart = true;
}