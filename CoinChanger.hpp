#pragma once

#include "MDBDevice.hpp"

#define PAYOUT 								0x02
#define PAYOUT_STATUS 	 				0x03
#define PAYOUT_VALUE_POLL			0x04
#define SEND_DIAGNOSTIC_STATUS 	0x05

class CoinChanger : public MDBDevice
{
public:
	CoinChanger(MDBSerial &mdb);

	//return false if we encounter sth so we cant go on
	bool Update(unsigned long &change);
	bool Reset();
	bool Dispense(unsigned long value);
	bool Dispense(int coin, int count);
	void Print();
	
	inline unsigned long GetDispensedValue() { unsigned long val = m_dispensed_value; m_dispensed_value = 0; return val; }
	inline unsigned long GetCredit() { return m_credit; }
	inline void ClearCredit() { m_credit = 0; }
	
private:
	int poll();
	void setup();
	void status();
	void type();
	void type_new();
	void expansion_identification();
	void expansion_feature_enable(int features);
	
	//untested below
	void expansion_payout(int value);
	void expansion_payout_status(); 
	long expansion_payout_value_poll();
	void expansion_send_diagnostic_status();

	int ADDRESS;
	int STATUS;
	int DISPENSE;
	
	int m_acceptedCoins;
	int m_dispenseableCoins;

	unsigned long m_credit;
	
	unsigned long m_value_to_dispense;
	unsigned long m_dispensed_value;

	char m_coin_scaling_factor;
	char m_decimal_places;
	unsigned int m_coin_type_routing;
	char m_coin_type_credit[16]; //coin value divided by coin scaling factor

	unsigned int m_tube_full_status;
	char m_tube_status[16];

	unsigned long m_software_version;
	unsigned long m_optional_features;

	bool m_alternative_payout_supported;
	bool m_extended_diagnostic_supported;
	bool m_manual_fill_and_payout_supported;
	bool m_file_transport_layer_supported;
	
	int m_update_count;
};