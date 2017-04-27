#include "CoinChanger.h"
#include <Arduino.h>

CoinChanger::CoinChanger(MDBSerial &mdb) : MDBDevice(mdb)
{

}

void CoinChanger::Reset()
{
	m_mdb->SendCommand(ADDRESS, RESET);
	if ((m_mdb->GetResponse() == ACK))
	{
		Poll();
		setup();
		status();
		//Expansion(0x00); //ID
		//Expansion(0x01); //Feature
		//Expansion(0x05); //Status
		m_serial->println("RESET Completed");
		return;
	}
	m_serial->println("RESET FAILED");
	Reset();
}

int CoinChanger::Poll()
{
	for (int i = 0; i < 64; i++)
		m_buffer[i] = 0;
	m_mdb->SendCommand(ADDRESS, POLL);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);

	//status();
	type();

	if (answer == ACK)
	{
		m_mdb->Ack();
		return 1;
	}

	//max of 16 bytes as response
	for (int i = 0; i < 16; i++)
	{
		//coins dispensed manually
		if (m_buffer[i] & 0b10000000)
		{
			int number = (m_buffer[i] & 0b01110000) >> 4;
			int type = m_buffer[i] & 0b00001111;
			int coins_in_tube = m_buffer[i + 1];
			m_serial->println("dispensed");
			i++; //cause we used 2 bytes
		}
		//coins deposited
		else if (m_buffer[i] & 0b01000000)
		{
			m_serial->println("deposited");
			int routing = (m_buffer[i] & 0b00110000) >> 4;
			int type = m_buffer[i] & 0b00001111;
			int coins_in_tube = m_buffer[i + 1];
			if (routing < 2)
			{
				m_credit += (m_coin_type_credit[type] * m_coin_scaling_factor) / 100.0f;
				m_serial->println(m_credit);
				m_serial->println("coin credited");
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
				return JUST_RESET;
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
				//for (int i = 0; i < m_count; i++)
				//	serial->println(result[i]);
			}
		}
	}
	
	m_mdb->Ack();
	return 1;
}

void CoinChanger::Dispense(float value)
{
	//4.30 -> 430
	value *= 100;
	if (m_alternative_payout_supported)
	{
		char val = value / m_coin_scaling_factor;
		expansion_payout(val);
	}
	else
	{
		int num_2e = value / 200;
		value -= num_2e * 200;
		int num_1e = value / 100;
		value -= num_1e * 100;
		int num_50c = value / 50;
		value -= num_50c * 50;
		int num_20c = value / 20;
		value -= num_20c * 20;
		int num_10c = value / 10;
		value -= num_10c * 10;

		//5 cent currently not supported !!!

		m_serial->println("num_2e: " + num_2e);
		m_serial->println("num_1e: " + num_1e);
		m_serial->println("num_50c: " + num_50c);
		m_serial->println("num_20c: " + num_20c);
		m_serial->println("num_10c: " + num_10c);
		Dispense(0, num_2e);
		Dispense(0, num_1e);
		Dispense(0, num_50c);
		Dispense(0, num_20c);
		Dispense(0, num_10c);
	}
}

void CoinChanger::Dispense(int coin, int count)
{
	int out = (count << 4) | coin;
	m_mdb->SendCommand(ADDRESS, DISPENSE, &out, 1);
	if (m_mdb->GetResponse() == ACK)
	{
		return;
	}
	m_serial->println("DISPENSE FAILED");
}

void CoinChanger::DisableCoin(int coin)
{
	m_acceptedCoins |= (1 << coin);
}

void CoinChanger::EnableCoin(int coin)
{
	m_acceptedCoins ^= (1 << coin);
}

void CoinChanger::Print()
{
	m_serial->println("## CoinChanger ##");
	m_serial->print("credit: ");
	m_serial->println(m_credit);

	m_serial->print("feature level: ");
	m_serial->println(m_feature_level);

	/*unsigned int m_country;
	char m_coin_scaling_factor;
	char m_decimal_places;
	unsigned int m_coin_type_routing;
	char m_coin_type_credit[16];

	unsigned int m_tube_full_status;
	char m_tube_status[16];*/
}

void CoinChanger::setup()
{
	m_mdb->SendCommand(ADDRESS, SETUP);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);

	if (answer != -1 && m_count == 23)
	{
		m_mdb->Ack();
		m_feature_level = m_buffer[0];
		m_country = m_buffer[1] << 8 | m_buffer[2];
		m_coin_scaling_factor = m_buffer[3];
		m_decimal_places = m_buffer[4];
		m_coin_type_routing = m_buffer[5] << 8 | m_buffer[6];
		for (int i = 0; i < 16; i++)
		{
			m_coin_type_credit[i] = m_buffer[7 + i];
		}

		//test for expansion support
		if (m_feature_level >= 3)
		{

		}

		m_serial->println("setup complete");
		return;
	}
	delay(50);
	m_serial->println("setup error");
	setup();
}

void CoinChanger::status()
{
	m_mdb->SendCommand(ADDRESS, STATUS);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);
	if (answer != -1 && m_count == 18)
	{
		m_tube_full_status = m_buffer[0] << 8 | m_buffer[1];
		for (int i = 0; i < 16; i++)
		{
			m_tube_status[i] = m_buffer[2 + i];
		}

		m_serial->println("status complete");
		m_mdb->Ack();
		return;
	}
	m_serial->println("status error");
	delay(50);
	status();
}

void CoinChanger::type()
{
	int out[] = {0xff, 0xff, 0xff, 0xff};
	m_mdb->SendCommand(ADDRESS, TYPE, out, 4);
	//does not always return an ack
	/*if (m_mdb->GetResponse() == ACK)
	{
		return;
	}
	m_serial->println("TYPE FAILED");*/
}

void CoinChanger::type_new()
{
	int out[] = { m_acceptedCoins >> 8, m_acceptedCoins & 0b11111111, m_dispenseableCoins >> 8, m_dispenseableCoins & 0b11111111 };
	m_mdb->SendCommand(ADDRESS, TYPE, out, 4);
	//does not always return an ack
	/*if (m_mdb->GetResponse() == ACK)
	{
	return;
	}
	m_serial->println("TYPE FAILED");*/
}

void CoinChanger::expansion_identification()
{
	m_mdb->SendCommand(ADDRESS, EXPANSION + IDENTIFICATION);
	m_mdb->GetResponse(m_buffer, &m_count);

	if (m_count != 33)
	{
		m_serial->println("expansion identification failed");
		return;
	}

	m_manufacturer_code = m_buffer[0] << 16 | m_buffer[1] << 8 | m_buffer[2];
	for (int i = 0; i < 12; i++)
	{
		m_serial_number[i] = m_buffer[3 + i];
		m_model_number[i] = m_buffer[15 + i];
	}

	m_software_version = m_buffer[27] << 8 | m_buffer[28];
	m_optional_features = m_buffer[29] << 24 | m_buffer[30] << 16 | m_buffer[31] << 8 | m_buffer[32];

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
	expansion_feature_enable();
}

void CoinChanger::expansion_feature_enable()
{
	//enable all features at the moment
	int out[] = { 0x00, 0x00, 0x00, 0x04 };
	m_mdb->SendCommand(ADDRESS, EXPANSION + FEATURE_ENABLE, out, 4);
}

void CoinChanger::expansion_payout(char value)
{
	m_mdb->SendCommand(ADDRESS, EXPANSION + PAYOUT, value, 1);
}