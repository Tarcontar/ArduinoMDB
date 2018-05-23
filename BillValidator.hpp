#pragma once

#include "MDBDevice.hpp"

class BillValidator : public MDBDevice
{
public:
	BillValidator(MDBSerial &mdb);

	bool Update(unsigned long cc_change);

	bool Reset();
	void Print();

	inline unsigned long GetCredit() { return m_credit; }
	inline void ClearCredit() { m_credit = 0; }

private:
	bool init();
	int poll();
	bool setup(int it = 0);
	void security(int it = 0);
	void type(int bills[], int it = 0);
	void stacker(int it = 0);
	void escrow(bool accept, int it = 0);

	int ADDRESS;
	int SECURITY;
	int ESCROW;
	int STACKER;

	unsigned long m_change;
	unsigned long m_credit;

	bool m_full;
	int m_bills_in_stacker;
	
	bool m_bill_in_escrow;

	char m_bill_scaling_factor;
	char m_decimal_places;
	unsigned int m_stacker_capacity;
	unsigned int m_security_levels;
	char m_can_escrow;
	char m_bill_type_credit[16];
};