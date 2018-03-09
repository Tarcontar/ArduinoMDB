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

CoinChanger::CoinChanger(MDBSerial &mdb) : MDBDevice(mdb)
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
	
	m_update_count = 0;
	
	m_value_to_dispense = 0;
	m_dispensed_value = 0;
}

bool CoinChanger::Update(unsigned long &change)
{
	poll();
	status();
	
	if (m_feature_level >= 3 && (m_update_count % 50) == 0)
	{
		expansion_send_diagnostic_status();
	}
	
	//TODO: check if last dispense worked
	for (int i = 0; i < 15; i++)
	{
		if (m_dispensed_value >= m_value_to_dispense)
		{
			m_dispensed_value = 0;
			m_value_to_dispense = 0;
			break;
		}
		poll();
		if(m_feature_level >= 3)
			expansion_payout_status();
		delay(1000);
		if (i == 14)
			m_uart->println("Dispense failed");
	}
	
	change = 0;
	for (int i = 0; i < 16; i++)
	{
		change += m_coin_type_credit[i] * m_tube_status[i] * m_coin_scaling_factor;
	}
	
	type(); // TODO: disable some coins when change is low
	
	m_update_count++;
	if (change == 0)
		return false;
	return true; 
}

bool CoinChanger::Reset()
{
	//m_logging(m_logger, "test", 0);
	
	m_mdb->SendCommand(ADDRESS, RESET);
	if ((m_mdb->GetResponse() == ACK))
	{
		int count = 0;
		while (poll() != JUST_RESET) 
		{
			if (count > MAX_RESET_POLL) return false;
			count++;
		}
		setup();
		status();
		Print();
		m_uart->println(CC_RESET_COMPLETED);
		return true;
	}
	//m_uart->println(CC_RESET_FAILED);
	//m_warning(CC_RESET_FAILED);
	if (m_resetCount < MAX_RESET)
	{
		m_resetCount++;
		return Reset();
	}
	else
	{
		m_resetCount = 0;
		//m_uart->println(CC_NOT_RESPONDING);
		//m_error(CC_NOT_RESPONDING);
		return false;
	}
	return true;
}

int CoinChanger::poll()
{
	bool reset = false;
	bool payout_busy = false;
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
			
#ifdef MDB_DEBUG
			//else coin rejected
			else
			{
				m_uart->println("coin rejected");
			}
#endif
			i++; //cause we used 2 bytes
		}
#ifdef MDB_DEBUG
		//slug
		else if (m_buffer[i] & 0b00100000)
		{
			int slug_count = m_buffer[i] & 0b00011111;
			m_uart->println("slug");
		}
#endif
		//status
		else
		{
			switch (m_buffer[i])
			{
#ifdef MDB_DEBUG
			case 1:
				//escrow request
				m_uart->println("escrow request");
				//TODO: handle dispense here?
				break;
#endif				
			case 2:
				//changer payout busy
				m_uart->println("changer payout busy");
				payout_busy = true;
				break;
#ifdef MDB_DEBUG
			case 3:
				//no credit
				m_uart->println("no credit");
				break;
#endif
			case 4:
				//defective tube sensor
				m_uart->println("defective tube sensor");
				//m_warning("defective tube sensor");
				break;
#ifdef MDB_DEBUG
			case 5:
				//double arrival
				m_uart->println("double arrival");
				break;
			case 6:
				//acceptor unplugged
				m_uart->println("acceptor unplugged");
				break;
			case 7:
				//tube jam
				m_uart->println("tube jam");
				break;
			case 8:
				//ROM checksum error
				m_uart->println("ROM checksum error");
				break;
			case 9:
				//coin routing error
				m_uart->println("coin routing error");
				break;
			case 10:
				//changer busy
				m_uart->println("changer busy");
				break;
#endif
			case 11:
				//changer was reset
				m_uart->println("JUST RESET");
				reset = true;
				break;
#ifdef MDB_DEBUG
			case 12:
				//coin jam
				m_uart->println("coin jam");
				break;
			case 13:
				//possible credited coin removal
				m_uart->println("credited coin removal");
				break;
			default:
				m_uart->println("default");
				for ( ; i < m_count; i++)
					m_uart->println(m_buffer[i]);
#endif
			}
		}
	}
	if (reset)
		return JUST_RESET;
	if (payout_busy)
		poll();
	return 1;
}

bool CoinChanger::Dispense(unsigned long value)
{
	if (m_alternative_payout_supported)
	{
		m_value_to_dispense += value;
		char val = value / m_coin_scaling_factor;
		expansion_payout(val);
		return true;
	}
	else
	{
		for (int i = 15; i >= 0; i--)
		{
			int count = value / (m_coin_type_credit[i] * m_coin_scaling_factor);
			count = min(count, m_tube_status[i]); //check if sufficient coins in tube
			if (Dispense(i, count))
			{
				unsigned long val = count * m_coin_type_credit[i] * m_coin_scaling_factor;
				m_value_to_dispense += val;
				value -= val;
			}
		}
		
		if (value == 0)
			return true;
		
		if (value < 5)
			value = 5;
		else 
			value++;
		
		//possible deadloop
		return Dispense(value);
	}
}

bool CoinChanger::Dispense(int coin, int count)
{
	int out = (count << 4) | coin;
	m_mdb->SendCommand(ADDRESS, DISPENSE, &out, 1);
	if (m_mdb->GetResponse() != ACK)
	{
		m_uart->println(CC_DISPENSE_FAILED);
		//m_warning(CC_DISPENSE_FAILED);
		return false;
	}
	return true;
}

void CoinChanger::Print()
{
#ifdef MDB_DEBUG
	m_uart->println("## CoinChanger ##");
	m_uart->print("credit: ");
	m_uart->println(m_credit);
	
	m_uart->print("country: ");
	m_uart->println(m_country);

	m_uart->print("feature level: ");
	m_uart->println((int)m_feature_level);
	
	m_uart->print("coin scaling factor: ");
	m_uart->println((int)m_coin_scaling_factor);
	
	m_uart->print("decimal places: ");
	m_uart->println((int)m_decimal_places);
	
	m_uart->print("coin type routing: ");
	m_uart->println(m_coin_type_routing);
	
	m_uart->print("coin type credits: ");
	for (int i = 0; i < 16; i++)
	{
		m_uart->print((int)m_coin_type_credit[i]);
		m_uart->print(" ");
    }
	m_uart->println();
	
	m_uart->print("tube full status: ");
	m_uart->println((int)m_tube_full_status);
	
	m_uart->print("tube status: ");
	for (int i = 0; i < 16; i++)
	{
		m_uart->print((int)m_tube_status[i]);
		m_uart->print(" ");
    }
	m_uart->println();
	
	m_uart->print("software version: ");
	m_uart->println(m_software_version);
	
	m_uart->print("optional features: ");
	m_uart->println(m_optional_features);
	
	m_uart->print("alternative payout supported: ");
	m_uart->println(m_alternative_payout_supported);
	
	m_uart->print("extended diagnostic supported: ");
	m_uart->println(m_extended_diagnostic_supported);
	
	m_uart->print("mauall fill and payout supported: ");
	m_uart->println(m_manual_fill_and_payout_supported);
	
	m_uart->print("file transport layer supported: ");
	m_uart->println(m_file_transport_layer_supported);
	
	m_uart->println("\n######");
	m_uart->println();
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
#ifdef MDB_DEBUG
		m_uart->println("CC: setup complete");
#endif
		return;
	}
	delay(50);
	m_uart->println(CC_SETUP_ERROR);
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
		m_uart->println("CC: status complete");
#endif
		return;
	}
	m_uart->println(CC_STATUS_ERROR);
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
		m_uart->println(CC_TYPE_ERROR);
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
	m_uart->println(CC_EXP_ID_ERROR);
	expansion_identification();
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

//number of each coin dispensed since last alternative payout (expansion_payout)
//-> response to alternative payout (expansion_payout)
//changer clears output data after an ack from controller
void CoinChanger::expansion_payout_status()
{
	m_mdb->SendCommand(ADDRESS, EXPANSION, PAYOUT_STATUS);
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 16);
	m_mdb->Ack();
	if (answer == ACK) //payout busy
	{
		//return 1;
	}
	if (answer > 0 && m_count == 16)
	{
		for (int i = 0; i < 16; i++)
		{
			int count = m_buffer[i];
			m_dispensed_value += m_coin_type_credit[i] * m_coin_scaling_factor * count;
		}
	}
}

//scaled value that indicates the amount of change payd out since last poll or
//after the initial alternative_payout
long CoinChanger::expansion_payout_value_poll()
{
	m_mdb->SendCommand(ADDRESS, EXPANSION, PAYOUT_VALUE_POLL);
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 1);
	if (answer == ACK) //payout finished 
		expansion_payout_status();
	else if (answer > 0 && m_count == 1)
	{
		return m_buffer[0];
	}
	return 0;
}

//should be send by the vmc every 1-10 seconds
void CoinChanger::expansion_send_diagnostic_status()
{
	m_mdb->SendCommand(ADDRESS, EXPANSION, SEND_DIAGNOSTIC_STATUS);
	m_mdb->Ack();
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 2);
	if (answer > 0 && m_count == 2)
	{
#ifdef MDB_DEBUG
		String first = String(m_buffer[0]);
		String msg = String(first + m_buffer[1]);
		//m_warning(msg);
		
#elif
		switch (m_buffer[0])
		{
		case 1:
			m_warning("powering up");
			break;
		case 2:
			m_warning("powering down");
			break;
		case 3:
			m_warning("OK");
			break;
		case 4:
			m_warning("Keypad shifted");
			break;
			
		case 5:
			switch (m_buffer[1])
			{
			case 10:
				m_warning("manual fill / payout active");
				break;
			case 20:
				m_warning("new inventory information available");
				break;
			}
			break;
			
		case 6:
			m_warning("inhibited by VMC");
			break;
			
		case 10:
			switch(m_buffer[1])
			{
			case 0:
				m_warning("non specific error");
				break;
			case 1:
				m_warning("check sum error #1");
				break;
			case 2:
				m_warning("check sum error #2");
				break;
			case 3:
				m_warning("low line voltage detected");
				break;
			}
			break;
			
		case 11:
			switch(m_buffer[1])
			{
				case 0:
					m_warning("non specific discriminator error");
					break;
				case 10:
					m_warning("flight deck open");
					break;
				case 11:
					m_warning("escrow return stuck open");
					break;
				case 30:
					m_warning("coin jam in sensor");
					break;
				case 41:
					m_warning("discrimination below specified standard");
					break;
				case 50:
					m_warning("validation sensor A out of range");
					break;
				case 51:
					m_warning("validation sensor B out of range");
					break;
				case 52:
					m_warning("validation sensor C out of range");
					break;
				case 53:
					m_warning("operation temperature exceeded");
					break;
				case 54:
					m_warning("sizing optics failure");
					break;
			}
			break;
		
		case 12:
			switch(m_buffer[1])
			{
				case 0:
					m_warning("non specific accept gate error");
					break;
				case 30:
					m_warning("coins entered gate, but did not exit");
					break;
				case 31:
					m_warning("accept gate alarm active");
					break;
				case 40:
					m_warning("accept gate open, but no coin detected");
					break;
				case 50:
					m_warning("post gate sensor covered before gate openend");
					break;
			}
			break;
		
		case 13:
			switch(m_buffer[1])
			{
				case 0:
					m_warning("non specific separator error");
					break;
				case 10:
					m_warning("sort sensor error");
					break;
			}
			break;

		case 14:
			switch(m_buffer[1])
			{
				case 0:
					m_warning("non specific dispenser error");
					break;
			}
			break;
			
		case 15:
			switch(m_buffer[1])
			{
				case 0:
					m_warning("non specific cassette error");
					break;
				case 2:
					m_warning("cassette removed");
					break;
				case 3:
					m_warning("cash box sensor error");
					break;
				case 4:
					m_warning("sunlight on tube sensors");
					break;
			}
			break;
		}
#endif
	}
}