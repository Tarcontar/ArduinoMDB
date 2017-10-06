#pragma once

#include "MDBDevice.hpp"

#define TUBE_2E  5
#define TUBE_1E  4
#define TUBE_50c 3
#define TUBE_20c 2
#define TUBE_10c 1
#define TUBE_5c  0

class CoinChanger : public MDBDevice
{
public:
	CoinChanger(MDBSerial &mdb, void (*error)(String) = NULL, void (*warning)(String) = NULL);

	//return false if we encounter sth so we cant go on
	bool Update(unsigned long &change);
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

	int ADDRESS;
	int STATUS;
	int DISPENSE;
	
	int m_acceptedCoins;
	int m_dispenseableCoins;

	unsigned long m_credit;

	char m_coin_scaling_factor;
	char m_decimal_places;
	unsigned int m_coin_type_routing;
	char m_coin_type_credit[16];

	unsigned int m_tube_full_status;
	char m_tube_status[16];

	unsigned long m_software_version;
	unsigned long m_optional_features;

	bool m_alternative_payout_supported;
	bool m_extended_diagnostic_supported;
	bool m_manual_fill_and_payout_supported;
	bool m_file_transport_layer_supported;
};