#pragma once

#include "MDBSerial.h"
#include "SoftwareSerial.h"

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

	inline void SetSerial(SoftwareSerial &serial) { m_serial = &serial; }

protected:
	MDBSerial *m_mdb;
	SoftwareSerial *m_serial;

	int m_count;
	char m_buffer[64];

	char m_feature_level;
	unsigned int m_country;
};