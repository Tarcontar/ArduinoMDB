#include "CoinChanger.hpp"
#include <Arduino.h>

#define CC_RESET_COMPLETED "CC: RESET COMPLETED"
#define CC_RESET_FAILED "CC: RESET FAILED"
#define CC_NOT_RESPONDING "CC: NOT RESPONDING"
#define CC_DISPENSE_FAILED "CC: DISPENSE FAILED"
#define CC_SETUP_ERROR "CC: SETUP ERROR"
#define CC_STATUS_ERROR "CC: STATUS ERROR"
#define CC_TYPE_ERROR "CC: TYPE ERROR"
#define CC_EXP_ID_ERROR "CC: EXP ID ERROR"

CoinChanger::CoinChanger(MDBSerial &mdb /*, void (*error)(String), void (*warning)(String)*/) : MDBDevice(mdb /*, error, warning*/)
{
	ADDRESS = 0x08;
	STATUS = 0x02;
	DISPENSE = 0x05;
	
	m_acceptedCoins = 0xFFFF; //all coins enabled by default
	m_dispenseableCoins = 0xFFFF;
	
	m_resetCount = 0;
	
	m_credit = 0;
	m_coin_scaling_factor = 0;
	m_decimal_places = 0;
	m_coin_type_routing = 0;
	
	for (int i = 0; i < 16; i++)
	{
		m_coin_type_credit[i] = 0;
		m_tube_status[i] = 0;
	}
	m_tube_full_status = 0;
	
	m_software_version = 0;
	m_optional_features = 0;
	
	m_alternative_payout_supported = false;
	m_extended_diagnostic_supported = false;
	m_manual_fill_and_payout_supported = false;
	m_file_transport_layer_supported = false;
}

bool CoinChanger::Update(unsigned long &change)
{
	poll();
	status();
	change = 0;

	for (int i = 0; i < 16; i++)
	{
		change += m_coin_type_credit[i] * m_tube_status[i] * m_coin_scaling_factor;
	}
	
	type(); // TODO: disable some coins when change is low
	if (change == 0)
		return false;
	return true; 
}

bool CoinChanger::Reset()
{
	m_mdb->SendCommand(ADDRESS, RESET);
	if ((m_mdb->GetResponse() == ACK))
	{
		int count = 0;
		while (poll() != JUST_RESET) 
		{
			if (count > 10) return false;
			count++;
		}
		setup();
		status();
		Print();
		m_serial->println(CC_RESET_COMPLETED);
		return true;
	}
	m_serial->println(CC_RESET_FAILED);
	//m_warning(CC_RESET_FAILED);
	if (m_resetCount < MAX_RESET)
	{
		m_resetCount++;
		Reset();
	}
	else
	{
		m_resetCount = 0;
		m_serial->println(CC_NOT_RESPONDING);
		//m_error(CC_NOT_RESPONDING);
		return false;
	}
	return true;
}

int CoinChanger::poll()
{
	bool reset = false;
	for (int i = 0; i < 64; i++)
		m_buffer[i] = 0;
	m_mdb->SendCommand(ADDRESS, POLL);
	m_mdb->Ack();
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 16);
	if (answer == ACK)
	{
		return 1;
	}

	//max of 16 bytes as response
	for (int i = 0; i < m_count && i < 16; i++)
	{
		//coins dispensed manually
		if (m_buffer[i] & 0b10000000)
		{
			
			int count = (m_buffer[i] & 0b01110000) >> 4;
			int type = m_buffer[i] & 0b00001111;
			int coins_in_tube = m_buffer[i + 1];
			
			m_dispensed_value += m_coin_type_credit[type] * m_coin_scaling_factor * count;
			i++; //cause we used 2 bytes
		}
		//coins deposited
		else if (m_buffer[i] & 0b01000000)
		{
			int routing = (m_buffer[i] & 0b00110000) >> 4;
			int type = m_buffer[i] & 0b00001111;
			int those_coins_in_tube = m_buffer[i + 1];
			if (routing < 2)
			{
				m_credit += (m_coin_type_credit[type] * m_coin_scaling_factor);
			}
			
			//else coin rejected
			else
			{
				m_serial->println("coin rejected");
			}
			i++; //cause we used 2 bytes
		}
		//slug
		else if (m_buffer[i] & 0b00100000)
		{
			int slug_count = m_buffer[i] & 0b00011111;
			m_serial->println("slug");
		}
		//status
		else
		{
			switch (m_buffer[i])
			{
			case 1:
				//escrow request
				m_serial->println("escrow request");
				break;
			case 2:
				//changer payout busy
				m_serial->println("changer payout busy");
				break;
			case 3:
				//no credit
				m_serial->println("no credit");
				break;
				
			case 4:
				//defective tube sensor
				m_serial->println("defective tube sensor");
				//m_warning("defective tube sensor");
				break;

			case 5:
				//double arrival
				m_serial->println("double arrival");
				break;
			case 6:
				//acceptor unplugged
				m_serial->println("acceptor unplugged");
				break;
			case 7:
				//tube jam
				m_serial->println("tube jam");
				break;
			case 8:
				//ROM checksum error
				m_serial->println("ROM checksum error");
				break;
			case 9:
				//coin routing error
				m_serial->println("coin routing error");
				break;
			case 10:
				//changer busy
				m_serial->println("changer busy");
				break;
			
			case 11:
				//changer was reset
				m_serial->println("JUST RESET");
				reset = true;
				break;
				
			case 12:
				//coin jam
				m_serial->println("coin jam");
				break;
			case 13:
				//possible credited coin removal
				m_serial->println("credited coin removal");
				break;
			default:
				m_serial->println("default");
				for ( ; i < m_count; i++)
					m_serial->println(m_buffer[i]);
			}
		}
	}
	if (reset)
		return JUST_RESET;
	return 1;
}

bool CoinChanger::Dispense(unsigned long value)
{
	if (m_alternative_payout_supported)
	{
		char val = value / m_coin_scaling_factor;
		expansion_payout(val);
	}
	else
	{
		//TODO: do this in for loop with  m_coin_type_credit[i]
		int num_2e = value / 200;
		num_2e = min(num_2e, m_tube_status[TUBE_2E]); 
		if (Dispense(TUBE_2E, num_2e))
			value -= num_2e * 200;
		
		int num_1e = value / 100;
		num_1e = min(num_1e, m_tube_status[TUBE_1E]);
		if (Dispense(TUBE_1E, num_1e))
			value -= num_1e * 100;
		
		int num_50c = value / 50;
		num_50c = min(num_50c, m_tube_status[TUBE_50c]);
		if (Dispense(TUBE_50c, num_50c))
			value -= num_50c * 50;
		
		int num_20c = value / 20;
		num_20c = min(num_20c, m_tube_status[TUBE_20c]);
		if (Dispense(TUBE_20c, num_20c))
			value -= num_20c * 20;
		
		int num_10c = value / 10;
		num_10c = min(num_10c, m_tube_status[TUBE_10c]);
		if (Dispense(TUBE_10c, num_10c))
			value -= num_10c * 10;
		
		int num_5c = value / 5;
		num_5c = min(num_5c, m_tube_status[TUBE_5c]);
		if (Dispense(TUBE_5c, num_5c))
			value -= num_5c * 5;
		
		//check if we can dispense some other coins, eg no 5c available
		if (value > 0) 
		{
			if (value < 5) //no 1c and 2c available in CC
			{
				if (m_tube_status[TUBE_5c] > 0)
					if (Dispense(TUBE_5c, 1))
						value = 0;
			}
			else if (value < 10)
			{
				if (m_tube_status[TUBE_10c] > 0)
					if (Dispense(TUBE_10c, 1))
						value = 0;
			}
			else if (value < 20)
			{
				if (m_tube_status[TUBE_20c] > 0)
					if (Dispense(TUBE_20c, 1))
						value = 0;
			}
			else if (value < 50)
			{
				if (m_tube_status[TUBE_50c] > 0)
					if (Dispense(TUBE_50c, 1))
						value = 0;
			}
			else if (value < 100)
			{
				if (m_tube_status[TUBE_1E] > 0)
					if (Dispense(TUBE_1E, 1))
						value = 0;
			}
			else if (value < 200)
			{
				if (m_tube_status[TUBE_2E] > 0)
					if (Dispense(TUBE_2E, 1))
						value = 0;
			}
		}
					
		if (value > 0)  //too less coins in CC to dispense change
			return false; //maybe return bill from bill validator?
		return true;
	}
}

bool CoinChanger::Dispense(int coin, int count)
{
	int out = (count << 4) | coin;
	m_mdb->SendCommand(ADDRESS, DISPENSE, &out, 1);
	if (m_mdb->GetResponse() != ACK)
	{
		m_serial->println(CC_DISPENSE_FAILED);
		//m_warning(CC_DISPENSE_FAILED);
		return false;
	}
	return true;
}

void CoinChanger::Print()
{
#ifdef MDB_DEBUG
	m_serial->println("## CoinChanger ##");
	m_serial->print("credit: ");
	m_serial->println(m_credit);
	
	m_serial->print("country: ");
	m_serial->println(m_country);

	m_serial->print("feature level: ");
	m_serial->println((int)m_feature_level);
	
	m_serial->print("coin scaling factor: ");
	m_serial->println((int)m_coin_scaling_factor);
	
	m_serial->print("decimal places: ");
	m_serial->println((int)m_decimal_places);
	
	m_serial->print("coin type routing: ");
	m_serial->println(m_coin_type_routing);
	
	m_serial->print("coin type credits: ");
	for (int i = 0; i < 16; i++)
	{
		m_serial->print((int)m_coin_type_credit[i]);
		m_serial->print(" ");
    }
	m_serial->println();
	
	m_serial->print("tube full status: ");
	m_serial->println((int)m_tube_full_status);
	
	m_serial->print("tube status: ");
	for (int i = 0; i < 16; i++)
	{
		m_serial->print((int)m_tube_status[i]);
		m_serial->print(" ");
    }
	m_serial->println();
	
	m_serial->print("software version: ");
	m_serial->println(m_software_version);
	
	m_serial->print("optional features: ");
	m_serial->println(m_optional_features);
	
	m_serial->print("alternative payout supported: ");
	m_serial->println(m_alternative_payout_supported);
	
	m_serial->print("extended diagnostic supported: ");
	m_serial->println(m_extended_diagnostic_supported);
	
	m_serial->print("mauall fill and payout supported: ");
	m_serial->println(m_manual_fill_and_payout_supported);
	
	m_serial->print("file transport layer supported: ");
	m_serial->println(m_file_transport_layer_supported);
	
	m_serial->println("\n######");
	m_serial->println();
#endif	
}

void CoinChanger::setup()
{
	m_mdb->SendCommand(ADDRESS, SETUP);
	m_mdb->Ack();
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 23);
	if (answer >= 0 && m_count == 23)
	{
		m_feature_level = m_buffer[0];
		m_country = m_buffer[1] << 8 | m_buffer[2];
		m_coin_scaling_factor = m_buffer[3];
		m_decimal_places = m_buffer[4];
		m_coin_type_routing = m_buffer[5] << 8 | m_buffer[6];
		for (int i = 0; i < 16; i++)
		{
			m_coin_type_credit[i] = m_buffer[7 + i];
		}

		if (m_feature_level >= 3)
		{
			expansion_identification();
			expansion_feature_enable(m_optional_features);
		}
		m_serial->println("CC: setup complete");
		return;
	}
	delay(50);
	m_serial->println(CC_SETUP_ERROR);
	//m_warning(CC_SETUP_ERROR);
	setup();
}

void CoinChanger::status()
{
	m_mdb->SendCommand(ADDRESS, STATUS);
	m_mdb->Ack();
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 18);
	if (answer != -1 && m_count == 18)
	{
		//if bit is set, the tube is full
		m_tube_full_status = m_buffer[0] << 8 | m_buffer[1];
		for (int i = 0; i < 16; i++)
		{
			//number of coins in the tube
			m_tube_status[i] = m_buffer[2 + i];
		}
#ifdef MDB_DEBUG
		m_serial->println("CC: status complete");
#endif
		return;
	}
	m_serial->println(CC_STATUS_ERROR);
	//m_warning(CC_STATUS_ERROR);
	delay(50);
	status();
}

void CoinChanger::type()
{
	int out[] = { m_acceptedCoins >> 8, m_acceptedCoins & 0b11111111, m_dispenseableCoins >> 8, m_dispenseableCoins & 0b11111111 };
	m_mdb->SendCommand(ADDRESS, TYPE, out, 4);
	if (m_mdb->GetResponse() != ACK)
	{
		delay(50);
		m_serial->println(CC_TYPE_ERROR);
		//m_warning(CC_TYPE_ERROR);
		type();
	}
}

void CoinChanger::expansion_identification()
{
	m_mdb->SendCommand(ADDRESS, EXPANSION, IDENTIFICATION);
	m_mdb->Ack();
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 33);
	if (answer > 0 && m_count == 33)
	{
		// * 1L to overcome 16bit integer error
		m_manufacturer_code = (m_buffer[0] * 1L) << 16 | m_buffer[1] << 8 | m_buffer[2];
		for (int i = 0; i < 12; i++)
		{
			m_serial_number[i] = m_buffer[3 + i];
			m_model_number[i] = m_buffer[15 + i];
		}

		m_software_version = m_buffer[27] << 8 | m_buffer[28];
		m_optional_features = (m_buffer[29] * 1L) << 24 | (m_buffer[30] * 1L) << 16 | m_buffer[31] << 8 | m_buffer[32];

		if (m_optional_features & 0b1)
		{
			m_alternative_payout_supported = true;
		}
		if (m_optional_features & 0b10)
		{
			m_extended_diagnostic_supported = true;
		}
		if (m_optional_features & 0b100)
		{
			m_manual_fill_and_payout_supported = true;
		}
		if (m_optional_features & 0b1000)
		{ 
			m_file_transport_layer_supported = true;
		}
		return;
	}

	expansion_identification();
	m_serial->println(CC_EXP_ID_ERROR);
	//m_warning(CC_EXP_ID_ERROR);
}

void CoinChanger::expansion_feature_enable(int features)
{
	int out[] = { 0x00, 0x00, 0x00, features };
	m_mdb->SendCommand(ADDRESS, EXPANSION, FEATURE_ENABLE, out, 4);
}

void CoinChanger::expansion_payout(int value)
{
	m_mdb->SendCommand(ADDRESS, EXPANSION, PAYOUT, &value, 1);
}