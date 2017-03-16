#include "CoinChanger.h"
#include <Arduino.h>

CoinChanger::CoinChanger(MDBSerial &mdb)
{
	m_mdb = &mdb;

	//Reset();
}

void CoinChanger::SetSerial(HardwareSerial &print)
{
	serial = &print;
	//serial->begin(9600);
}

void CoinChanger::Reset()
{
	m_mdb->SendCommand(ADDRESS, RESET);
	if ((m_mdb->GetResponse() == ACK))
	{
		Poll();
		setup();
		//status();
		//Expansion(0x00); //ID
		//Expansion(0x01); //Feature
		//Expansion(0x05); //Status
		//serial->println("RESET Completed");
		return;
	}
	else
	{
		serial->println("RESET FAILED");
	}
	Reset();
}

void CoinChanger::Enable()
{
	status();
	type();
}

int CoinChanger::Poll()
{
	//serial->print("POLL: ");
	m_mdb->SendCommand(ADDRESS, POLL);
	int *result;
	int answer = m_mdb->GetResponse(&result, &m_count);

	if (answer == ACK)
	{
		//serial->println("ACK");
		m_mdb->Ack();
		return 1;
	}

	serial->println(m_count);

	//coins dispensed manually
	if (result[0] & 0b10000000)
	{
		int number = result[0] & 0b01110000;
		int type = result[0] & 0b00001111;
		int coins_in_tube = result[1];
		serial->println("dispensed");
	}
	//coins deposited
	else if (result[0] & 0b01000000)
	{
		serial->println("deposited");
		int routing = result[0] & 0b00110000;
		int type = result[0] & 0b00001111;
		int coins_in_tube = result[1];
		if (routing < 2)
		{
			m_credit += m_type_values[type] * m_scaling_factor;
			serial->println(m_credit);
			serial->println("coin credited");
		}
		//else coin rejected
		else
		{
			serial->println("coin rejected");
		}
	}
	//slug
	else if (result[0] & 0b00100000)
	{
		int slug_count = result[0] & 0b00011111;
		serial->println("slug");
	}
	//status
	else
	{
		switch (result[0])
		{
		case 1:
			//escrow request
			serial->println("escrow request");
			break;
		case 2:
			//changer payout busy
			serial->println("changer payout busy");
			break;
		case 3:
			//no credit
			serial->println("no credit");
			break;
		case 4:
			//defective tube sensor
			serial->println("defective tube sensor");
			break;
		case 5:
			//double arrival
			serial->println("double arrival");
			break;
		case 6:
			//acceptor unplugged
			serial->println("acceptor unplugged");
			break;
		case 7:
			//tube jam
			serial->println("tube jam");
			break;
		case 8:
			//ROM checksum error
			serial->println("ROM checksum error");
			break;
		case 9:
			//coin routing error
			serial->println("coin routing error");
			break;
		case 10:
			//changer busy
			serial->println("changer busy");
			break;
		case 11:
			//changer was reset
			serial->println("JUST RESET");
			return JUST_RESET;
		case 12:
			//coin jam
			serial->println("coin jam");
			break;
		case 13:
			//possible credited coin removal
			serial->println("credited coin removal");
			break;
		//default:
			//serial->println("default");
			//for (int i = 0; i < m_count; i++)
			//	serial->println(result[i]);
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
	serial->println("DISPENSE FAILED");
}


void CoinChanger::setup()
{
	m_mdb->SendCommand(ADDRESS, SETUP);
	int *result;
	int answer = m_mdb->GetResponse(&result, &m_count);

	if (answer == ACK)
	{
		m_mdb->Ack();
		return;
	}

	if (answer != -1 && m_count == 23)
	{
		m_mdb->Ack();
		char feature_level = result[0];
		int country = result[1] << 8 | result[2];
		char coin_scaling_factor = result[3];
		char decimal_places = result[4];
		int coin_type_routing = result[5] << 8 | result[6];
		char coin_type_credit[] = new char[16];
		for (int i = 0; i < 16; i++)
		{
			coin_type_credit[i] = result[7 + i];
		}

		serial->println("setup complete");
		return;
	}
	serial->println("setup error");
}

void CoinChanger::status()
{
	m_mdb->SendCommand(ADDRESS, STATUS);

	int *result;
	int answer = m_mdb->GetResponse(&result, &m_count);

	if (answer == ACK)
	{
		m_mdb->Ack();
	}

	if (answer != -1 && m_count == 18)
	{
		int tube_full_status = result[0] << 8 | result[1];
		char tube_status[] = new char[16];
		for (int i = 0; i < 16; i++)
		{
			tube_status[i] = result[2 + i];
		}

		serial->println("status complete");
		m_mdb->Ack();
		return;
	}
	serial->println("status error");
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
	serial->println("TYPE FAILED");*/
}

void CoinChanger::expansion()
{

}