#pragma once

#include "MDBDevice.h"

#define ADDRESS 0x08
#define STATUS 0x02
#define DISPENSE 0x05

class CoinChanger : public MDBDevice
{
public:
	CoinChanger(MDBSerial &mdb);

	void Reset();
	int Poll();
	void Enable();
	void Dispense(int coin, int count);
	void Print();

private:
	void setup();
	void status();
	void type();
	void expansion();


	float m_credit;

	char m_coin_scaling_factor;
	char m_decimal_places;
	unsigned int m_coin_type_routing;
	char m_coin_type_credit[16];

	unsigned int m_tube_full_status;
	char m_tube_status[16];
};