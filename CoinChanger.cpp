#include "CoinChanger.h"

CoinChanger::CoinChanger(MDBSerial &mdb, HardwareSerial &print)
{
	m_serial = &mdb;

	serial = &print;
	serial->begin(9600);
}

void CoinChanger::Reset()
{
	m_serial->SendCommand(ADDRESS, RESET);
	if ((*m_serial->GetResponse(&m_count) == ACK) && (m_count == 1))
	{
		if (Poll() != JUST_RESET)
			serial->println("no just_reset");
		setup();
		//Expansion(0x00); //ID
		//Expansion(0x01); //Feature
		//Expansion(0x05); //Status
		return;
	}
	//error
}

void CoinChanger::Enable()
{
	status();
	type();
}

int CoinChanger::Poll()
{
	m_serial->SendCommand(ADDRESS, POLL);
	int *result = m_serial->GetResponse(&m_count);
	if (m_count == 1)
	{
		if (*result == ACK)
		{
			m_serial.Ack();
			return 1;
		}
		//error
	}
	else
	{
		//coins dispensed manually
		if (result[0] & 0b10000000)
		{
			int number = result[0] & 0b01110000;
			int type = result[0] & 0b00001111;
			int coins_in_tube = result[1];
		}
		//coins deposited
		else if (result[0] & 0b01000000)
		{
			int routing = result[0] & 0b00110000;
			int type = result[0] & 0b00001111;
			int coins_in_tube = result[1];
			if (routing < 2)
			{
				m_credit += m_type_values[type] * m_scaling_factor;
			}
			//else coin rejected
		}
		//slug
		else if (result[0] & 0b00100000)
		{
			int slug_count = result[0] & 0b00011111;
		}
		//status
		else
		{
			switch (result[0])
			{
			case 1:
				//escrow request
				break;
			case 2:
				//changer payout busy
				break;
			case 3:
				//no credit
				break;
			case 4:
				//defective tube sensor
				break;
			case 5:
				//double arrival
				break;
			case 6:
				//acceptor unplugged
				break;
			case 7:
				//tube jam
				break;
			case 8:
				//ROM checksum error
				break;
			case 9:
				//coin routing error
				break;
			case 10:
				//changer busy
				break;
			case 11:
				//changer was reset
				return JUST_RESET;
			case 12:
				//coin jam
				break;
			case 13:
				//possible credited coin removal
				break;
			}
		}
		m_serial->Ack();
	}
	return 1;
}

void CoinChanger::Dispense(int coin, int count)
{
	int out = (count << 4) | coin;
	m_serial->SendCommand(ADDRESS, DISPENSE, &out, 1);
	if (*m_serial->GetResponse(&m_count) == ACK)
	{
		return;
	}
	//error
}


void CoinChanger::setup()
{
	m_serial->SendCommand(ADDRESS, SETUP);
	if (*m_serial->GetResponse(&m_count) && m_count == 23)
	{
		m_serial->Ack();
		return;
	}
	else //we only get 3 bytes here
	{
		m_serial->Ack();
	}
}

void CoinChanger::status()
{
	m_serial->SendCommand(ADDRESS, STATUS);
	if (m_serial->GetResponse(&m_count) && m_count == 18)
	{
		m_serial->Ack();
		return;
	}
	else //we only get 3 bytes here
	{
		m_serial->Ack();
	}
}

void CoinChanger::type()
{
	int out[4] = {0xff, 0xff, 0xff, 0xff};
	m_serial->SendCommand(ADDRESS, TYPE, out, 4);
	if (m_serial->GetResponse(&m_count) == ACK)
	{
		return;
	}
	//error
}

void CoinChanger::expansion()
{

}