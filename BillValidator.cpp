#include "BillValidator.h"
#include <Arduino.h>

BillValidator::BillValidator(MDBSerial &mdb) : MDBDevice(mdb)
{
	//Reset();
}

void BillValidator::Reset()
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
		//Stacker();
		//Type();
		//serial->println("RESET Completed");
		return;
	}
	Reset();
}

int BillValidator::Poll()
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
		//bill status
		if (m_buffer[i] & 0b10000000)
		{
			int routing = (m_buffer[i] & 0b01110000) >> 4;
			int type = m_buffer[i] & 0b00001111;

			if (type == 0)
			{
				m_credit += (m_bill_type_credit[type] * m_bill_scaling_factor) / 100.0f;
				m_serial->println("stacked");
			}
			else if (type == 1)
			{
				m_serial->println("escrow position");
			}
			else if (type == 2)
			{
				m_serial->println("bill returned");
			}
			else if (type == 3)
			{
				m_serial->println("bill to recycler");
			}
			else if (type == 4)
			{
				m_serial->println("disabled bill rejected");
			}
			else if (type == 5)
			{
				m_serial->println("bill to recycler - manual fill");
			}
			else if (type == 6)
			{
				m_serial->println("manual dispense");
			}
			else
			{
				m_serial->println("transferred from recycler to cashbox");
			}
		}
		// number of input attempts while validator is disabled
		if (m_buffer[i] & 0b01000000)
		{
			int number = m_buffer[i] & 0b00011111;
			m_serial->println("number of attempts: ");
			m_serial->println(number);
		}
		// bill recycler only
		if (m_buffer[i] & 0b00100000)
		{
			int val = m_buffer[i] & 0b00011111;
			switch (val)
			{
			case 1:
				m_serial->println("escrow request");
				break;
			case 2:
				m_serial->println("payout busy");
				break;
			case 3:
				m_serial->println("dispenser busy");
				break;
			case 4:
				m_serial->println("defective dispenser sensor");
				break;
			case 5:
				m_serial->println("not used");
				break;
			case 6:
				m_serial->println("dispenser did not start / motor problem");
				break;
			case 7:
				m_serial->println("dispenser jam");
				break;
			case 8:
				m_serial->println("ROM checksum error");
				break;
			case 9:
				m_serial->println("dispenser disabled");
				break;
			case 10:
				m_serial->println("bill waiting");
				break;
			case 11: //unused
				break; 
			case 12: //unused
				break;
			case 13: //unused
				break;
			case 14: //unused
				break;
			case 15:
				m_serial->println("filled key pressed");
				break;
			}
		}
		//status
		else
		{
			switch (m_buffer[i])
			{
			case 1:
				//defective motor
				m_serial->println("defective motor");
				break;
			case 2:
				//sensor problem
				m_serial->println("sensor problem");
				break;
			case 3:
				//validator busy
				m_serial->println("validator busy");
				break;
			case 4:
				//ROM Checksum Error
				m_serial->println("ROM Checksum error");
				break;
			case 5:
				//Validator jammed
				m_serial->println("validator jammed");
				break;
			case 6:
				//validator was reset
				m_serial->println("JUST RESET");
				return JUST_RESET;
			case 7:
				//bill removed
				m_serial->println("bill removed");
				break;
			case 8:
				//cash box out of position
				m_serial->println("cash box out of position");
				break;
			case 9:
				//validator disabled
				m_serial->println("validator disabled");
				break;
			case 10:
				//invalid escrow request
				m_serial->println("invalid escrow request");
				break;
			case 11:
				//bill rejected
				m_serial->println("bill rejected");
				break;
			case 12:
				//possible credited bill removal
				m_serial->println("possible credited bill removal");
				break;
				//default:
				//m_serial->println("default");
				//for (int i = 0; i < m_count; i++)
				//	serial->println(result[i]);
			}
		}
	}

	m_mdb->Ack();
	//get the additional stacker info
	stacker();
	return 1;
}

void BillValidator::Security()
{
	int out[] = { 0xff, 0xff, 0xff, 0xff };
	m_mdb->SendCommand(ADDRESS, SECURITY, out, 4);
}

void BillValidator::Print()
{

}

void BillValidator::setup()
{
	m_mdb->SendCommand(ADDRESS, SETUP);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);

	if (answer != -1 && m_count == 23)
	{
		m_mdb->Ack();
		m_feature_level = m_buffer[0];
		m_country = m_buffer[1] << 8 | m_buffer[2];
		m_bill_scaling_factor = m_buffer[3];
		m_decimal_places = m_buffer[4];
		m_stacker_capacity = m_buffer[5] << 8 | m_buffer[6];
		m_security_levels = m_buffer[7] << 8 | m_buffer[8];
		m_can_escrow = m_buffer[9];
		for (int i = 0; i < 16; i++)
		{
			m_bill_type_credit[i] = m_buffer[10 + i];
		}

		m_serial->println("setup complete");
		return;
	}
	delay(50);
	m_serial->println("setup error");
	setup();
}

void BillValidator::status()
{

}

void BillValidator::type()
{
	int out[] = { 0xff, 0xff, 0xff, 0xff };
	m_mdb->SendCommand(ADDRESS, TYPE, out, 4);
}

void BillValidator::stacker()
{
	m_mdb->SendCommand(ADDRESS, STACKER);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);
	if (answer != -1 && m_count == 2)
	{
		if (m_buffer[0] & 0b10000000)
		{
			m_full = true;
		}
		m_bills_in_stacker = m_buffer[0] & 0b01111111;
	}
}

void BillValidator::expansion()
{

}