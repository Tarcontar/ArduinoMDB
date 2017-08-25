#pragma once

#include "MDBDevice.h"

#define TUBE_2E  5
#define TUBE_1E  4
#define TUBE_50c 3
#define TUBE_20c 2
#define TUBE_10c 1

class CoinChanger : public MDBDevice
{
public:
	CoinChanger(MDBSerial &mdb);

	unsigned long Update();
	bool Reset();
	void Dispense(unsigned long value);
	void Dispense(int coin, int count);
	void Print();

	inline unsigned long GetCredit() { return m_credit; }
	inline void ClearCredit() { m_credit = 0; }

private:
	int poll();
	void setup();
	void status();
	void type();
	void type_new();
	void expansion_identification();
	void expansion_feature_enable();
	void expansion_payout(int value);

	int ADDRESS = 0x08;
	int STATUS = 0x02;
	int DISPENSE = 0x05;
	
	int m_acceptedCoins = 0xFFFF; //all coins enabled by default
	int m_dispenseableCoins = 0xFF;

	unsigned long m_credit;

	char m_coin_scaling_factor;
	char m_decimal_places;
	unsigned int m_coin_type_routing;
	char m_coin_type_credit[16];

	unsigned int m_tube_full_status;
	char m_tube_status[16];

	unsigned long m_software_version;
	unsigned long m_optional_features;

	bool m_alternative_payout_supported = false;
	bool m_extended_diagnostic_supported = false;
	bool m_manual_fill_and_payout_supported = false;
	bool m_file_transport_layer_supported = false;
};