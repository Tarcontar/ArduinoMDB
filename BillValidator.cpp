#include "BillValidator.hpp"
#include <Arduino.h>


BillValidator::BillValidator(MDBSerial &mdb) : MDBDevice(mdb)
{
	ADDRESS = 0x30;
	SECURITY = 0x02;
	ESCROW = 0x05;
	STACKER = 0x06;
	
	m_resetCount = 0;
	
	m_credit = 0;
	m_full = false;
	m_bill_in_escrow = false;
	m_bill_scaling_factor = 0;
	m_decimal_places = 0;
	m_stacker_capacity = 0;
	m_security_levels = 0;
	m_can_escrow = 0;
	for (int i = 0; i < 16; i++)
		m_bill_type_credit[i] = 0;
}

bool BillValidator::Update(unsigned long cc_change)
{
	poll();
	stacker();

	int b = 0;

	if (cc_change > 2500) //more than 25€
		bitSet(b, 2); //enable 20€ bill
	if (cc_change > 1500) //more than 15€
		bitSet(b, 1); //enable 10€ bill
	if (cc_change > 500) //more than 5€
		bitSet(b, 0); //enable 5€ bill
	
	int bills[] = { 0x00, b, 0x00, 0x00 };
	if (m_bill_in_escrow)
	{
		escrow(true);
	}
	else
		type(bills);
	return true;
}

bool BillValidator::Reset()
{
	m_mdb->SendCommand(ADDRESS, RESET);
	if (m_mdb->GetResponse() == ACK)
	{
		int count = 0;
		while (poll() != JUST_RESET)
		{
			if (count > MAX_RESET_POLL) return false;
			count++;
		}
		if (!setup())
			return false;
		security();
		//Expansion(0x00); //ID
		//Expansion(0x01); //Feature
		//Expansion(0x05); //Status
		Print();
		debug << F("BV: RESET COMPLETED") << endl;
		return true;
	}
	debug << F("BV: RESET FAILED") << endl;
	
	if (m_resetCount < MAX_RESET)
	{
		m_resetCount++;
		return Reset();
	}
	else
	{
		m_resetCount = 0;
		status << F("BV: NOT RESPONDING") << endl;
		return false;
	}
	return true;
}

void BillValidator::Print()
{
	debug << F("## BILL VALIDATOR ##") << endl;
	debug << F("credit: ") << m_credit << endl;
	debug << F("full: ") << (bool)m_full << endl;
	debug << F("bills in stacker: ") << m_bills_in_stacker << endl;
	debug << F("feature level: ") << (int)m_feature_level << endl;
	debug << F("bill scaling factor: ") << (int)m_bill_scaling_factor << endl;
	debug << F("decimal places: ") << (int)m_decimal_places << endl;
	debug << F("capacity: ") << m_stacker_capacity << endl;
	debug << F("security levels: ") << m_security_levels << endl;
	debug << F("###") << endl;;
}

int BillValidator::poll()
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
		//bill status
		if (m_buffer[i] & 0b10000000)
		{
			int routing = (m_buffer[i] & 0b01110000) >> 4;
			int type = m_buffer[i] & 0b00001111;

			if (routing == 0)
			{
				m_credit += (m_bill_type_credit[type] * m_bill_scaling_factor);
			}
			else if (routing == 1)
			{
				debug << F("escrow position") << endl;
				m_bill_in_escrow = true;
			}
			else if (routing == 2)
			{
				debug << F("bill returned") << endl;
			}
			else if (routing == 3)
			{
				debug << F("bill to recycler") << endl;
			}
			else if (routing == 4)
			{
				debug << F("disabled bill rejected") << endl;
			}
			else if (routing == 5)
			{
				debug << F("bill to recycler - manual fill") << endl;
			}
			else if (routing == 6)
			{
				debug << F("manual dispense") << endl;
			}
			else if (routing == 7)
			{
				debug << F("transferred from recycler to cashbox") << endl;
			}
			else
				debug << F("routing error") << endl;
		}
		// number of input attempts while validator is disabled
		else if (m_buffer[i] & 0b01000000)
		{
			int number = m_buffer[i] & 0b00011111;
		}
		// bill recycler only
		else if (m_buffer[i] & 0b00100000)
		{
			int val = m_buffer[i] & 0b00011111;
			switch (val)
			{
			case 1:
				//m_uart->println("escrow request");
				break;
			case 2:
				//m_uart->println("payout busy");
				break;
			case 3:
				//m_uart->println("dispenser busy");
				break;
			case 4:
				//m_uart->println("defective dispenser sensor");
				break;
			case 5:
				//m_uart->println("not used");
				break;
			case 6:
				//m_uart->println("dispenser did not start / motor problem");
				break;
			case 7:
				//m_uart->println("dispenser jam");
				break;
			case 8:
				//m_uart->println("ROM checksum error");
				break;
			case 9:
				//m_uart->println("dispenser disabled");
				break;
			case 10:
				//m_uart->println("bill waiting");
				break;
			case 11: //unused
			case 12: //unused
			case 13: //unused
			case 14: //unused
			case 15:
				//m_uart->println("filled key pressed");
				break;
			/*
			default:
				m_uart->println("default bill rec");
			*/
			}
		}
		//status
		else
		{
			switch (m_buffer[i])
			{
			case 1:
				//defective motor
				warning << F("BV: defective motor") << endl;
				break;
			case 2:
				//sensor problem
				warning << F("BV: sensor problem") << endl;
				break;
			case 3:
				//validator busy
				break;
			case 4:
				//ROM Checksum Error
				warning << F("BV: ROM Checksum error") << endl;
				break;
			case 5:
				//Validator jammed
				//m_uart->println("validator jammed");
				break;
			case 6:
				//validator was reset
				//m_uart->println("just reset");
				reset = true;
				break;
			case 7:
				//bill removed
				//m_uart->println("bill removed");
				break;
			case 8:
				//cash box out of position
				warning << F("BV: cash box out of position") << endl;
				break;
			case 9:
				//validator disabled
				//m_uart->println(F("validator disabled"));
				break;
			case 10:
				//invalid escrow request
				warning << F("BV: invalid escrow request") << endl;
				break;
			case 11:
				//bill rejected
				//m_uart->println("bill rejected");
				break;
			case 12:
				//possible credited bill removal
				warning << F("BV: possible credited bill removal") << endl;
				break;
			}
		}
	}
	if (reset)
		return JUST_RESET;
	return 1;
}

bool BillValidator::setup(int it)
{
	for (int i = 0; i < 64; i++)
		m_buffer[i] = 0;
	m_mdb->SendCommand(ADDRESS, SETUP);
	m_mdb->Ack();
	delay(50);
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 27);
	if (answer > 0 && m_count == 27)
	{	
		m_feature_level = m_buffer[0];
		m_country = m_buffer[1] << 8 | m_buffer[2];
		m_bill_scaling_factor = m_buffer[3] << 8 | m_buffer[4];
		m_decimal_places = m_buffer[5];
		m_stacker_capacity = m_buffer[6] << 8 | m_buffer[7];
		m_security_levels = m_buffer[8] << 8 | m_buffer[9];
		m_can_escrow = m_buffer[10];
		for (int i = 0; i < 16; i++)
		{
			m_bill_type_credit[i] = m_buffer[11 + i];
		}
		return true;
	}
	if (it < 5)
	{
		delay(50);
		return setup(++it);
	}
	error << F("BV: SETUP ERROR") << endl;
	return false;
}

void BillValidator::security(int it)
{
	int out[] = { 0xff, 0xff };
	m_mdb->SendCommand(ADDRESS, SECURITY, out, 2);
	if (m_mdb->GetResponse() != ACK)
	{
		if (it < 5)
		{
			delay(50);
			return security(++it);
		}
		warning << F("BV: SECURITY FAILED") << endl;
	}
}

void BillValidator::type(int bills[], int it)
{
	m_mdb->SendCommand(ADDRESS, TYPE, bills, 4);
	if (m_mdb->GetResponse() != ACK)
	{
		if (it < 5)
		{
			delay(50);
			return type(bills, ++it);
		}
		warning << F("BV: TYPE ERROR") << endl;
	}
}

void BillValidator::stacker(int it)
{
	m_mdb->SendCommand(ADDRESS, STACKER);
	m_mdb->Ack();
	int answer = m_mdb->GetResponse(m_buffer, &m_count, 2);
	if (answer > 0 && m_count == 2)
	{
		if (m_buffer[0] & 0b10000000)
		{
			m_full = true;
		}
		else
			m_full = false;
		m_bills_in_stacker = m_buffer[0] & 0b01111111;
		return;
	}
	if (it < 5)
	{
		delay(50);
		return stacker(++it);
	}
	warning << F("BV: STACKER ERROR") << endl;
}

void BillValidator::escrow(bool accept, int it)
{
	int data[] = { 0x00 };
	if (accept)
		data[0] = 0x01;
	m_mdb->SendCommand(ADDRESS, ESCROW, data, 1);
	if (m_mdb->GetResponse() == ACK)
	{
		m_bill_in_escrow = false;
	}
	if (it < 5)
	{
		delay(50);
		return escrow(accept, ++it);
	}
	error << F("BV: ESCROW ERROR") << endl;
}