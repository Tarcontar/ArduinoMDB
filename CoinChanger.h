#pragma once

#include "MDBDevice.h"

class CoinChanger : public MDBDevice
{
public:
	CoinChanger(MDBSerial &mdb);
	CoinChanger();

	void Reset();
	int Poll();
	void Enable();
	void Dispense(int coin, int count);
	void Print();

	inline float GetCredit() { return m_credit; }
	inline void ClearCredit() { m_credit = 0.0f; }

private:
	void setup();
	void status();
	void type();
	void expansion();

	int ADDRESS = 0x08;
	int STATUS = 0x02;
	int DISPENSE = 0x05;

	float m_credit;

	char m_coin_scaling_factor;
	char m_decimal_places;
	unsigned int m_coin_type_routing;
	char m_coin_type_credit[16];

	unsigned int m_tube_full_status;
	char m_tube_status[16];
};