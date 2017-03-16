#pragma once

#include "MDBSerial.h"
#include "HardwareSerial.h"

#define ADDRESS 0x08
#define JUST_RESET 0x0B
#define DISPENSE 0x05

#define NO_RESPONSE 2000


class CoinChanger
{
public:
	CoinChanger(MDBSerial &mdb);

	void SetSerial(HardwareSerial &print);

	void Reset();
	void Enable();
	int Poll();
	void Dispense(int coin, int count);

private:
	HardwareSerial *serial;

	void setup();
	void status();
	void type();
	void expansion();

	MDBSerial *m_mdb;
	int m_count;
	float m_credit;
	int m_scaling_factor;
	int m_type_values[16];
};