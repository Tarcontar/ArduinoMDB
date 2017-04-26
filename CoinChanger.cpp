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

void CoinChanger::Enable()
{
	//status();
	type();
}

int CoinChanger::Poll()
{
	for (int i = 0; i < 64; i++)
		m_buffer[i] = 0;
	m_mdb->SendCommand(ADDRESS, POLL);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);

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

void CoinChanger::expansion()
{

}