#pragma once

#include "MDBDevice.h"

#define ADDRESS 0x30
#define SECURITY 0x02
#define ESCROW 0x05
#define STACKER 0x06

class BillValidator : public MDBDevice
{
public:
	BillValidator(MDBSerial &mdb);

	void Reset();
	int Poll();
	void Security();
	void Print();

private:
	void setup();
	void status();
	void type();
	void stacker();
	void expansion();

	float m_credit;

	bool m_full = false;
	int m_bills_in_stacker;

	char m_bill_scaling_factor;
	char m_decimal_places;
	unsigned int m_stacker_capacity;
	unsigned int m_security_levels;
	char m_can_escrow;
	char m_bill_type_credit[16];
};