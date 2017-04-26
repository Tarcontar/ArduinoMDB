#pragma once

#include "MDBSerial.h"
#include "HardwareSerial.h"

#define JUST_RESET 0x0B
#define NO_RESPONSE 2000

class MDBDevice
{
public:
	explicit
	MDBDevice(MDBSerial &mdb) : m_mdb(&mdb){}

	virtual void Reset() = 0;
	virtual int Poll() = 0;

	virtual void Print() = 0;

	inline void SetSerial(HardwareSerial &print) { m_serial = &print; }

protected:
	MDBSerial *m_mdb;
	HardwareSerial *m_serial;

	int m_count;
	char m_buffer[64];

	char m_feature_level;
	unsigned int m_country;
};