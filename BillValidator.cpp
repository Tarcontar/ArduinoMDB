#include "BillValidator.h"
#include <Arduino.h>

BillValidator::BillValidator(MDBSerial &mdb) : MDBDevice(mdb)
{
	ADDRESS = 0x30;
	SECURITY = 0x02;
	ESCROW = 0x05;
	STACKER = 0x06;
	
	m_full = false;
}

void BillValidator::Update(unsigned long cc_change)
{
	poll();
	stacker();

	int bills[] = { 0xff, 0xff, 0xff, 0xff };
	//also disable if credit is higher than 20 euros
	/*
	if (m_credit > 2000)
		//bills[1] = 0x00;
	if (!m_full)
	{
		if (cc_change < 2500)
		{
			//disable 20 bill
			//bills[1] ^= 0b00000100;
		}
		if (cc_change < 1500) 
		{
			//disable 10 bill
			//bills[1] ^= 0b00000010;
		}
		if (cc_change < 1000)
		{
			//disable 5 bill
			//bills[1] ^= 0b00000001;
		}
	}
	*/
	type(bills);
}

bool BillValidator::Reset()
{
	m_mdb->SendCommand(ADDRESS, RESET);
	delay(10);
	if ((m_mdb->GetResponse() == ACK))
	{
		while (poll() != JUST_RESET);
		setup();
		security();
		//Expansion(0x00); //ID
		//Expansion(0x01); //Feature
		//Expansion(0x05); //Status
		Print();
		m_serial->println(F("BV: RESET Completed"));
		return true;
	}
	m_serial->println(F("BV: RESET failed"));
	if (m_resetCount < MAX_RESET)
	{
		m_resetCount++;
		Reset();
	}
	else
	{
		m_resetCount = 0;
		m_serial->println(F("BV: NOT RESPONDING"));
		return false;
	}
}

int BillValidator::poll()
{
	//m_mdb->GetResponse(m_buffer, &m_count);
	for (int i = 0; i < 64; i++)
		m_buffer[i] = 0;
	m_mdb->SendCommand(ADDRESS, POLL);
	delay(45);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);
	m_mdb->Ack();
	if (answer == ACK)
	{
		return 1;
	}

	//max of 16 bytes as response
	for (int i = 0; i < m_count; i++)
	{
		//bill status
		if (m_buffer[i] & 0b10000000)
		{
			int routing = (m_buffer[i] & 0b01110000) >> 4;
			int type = m_buffer[i] & 0b00001111;

			if (routing == 0)
			{
				m_credit += (m_bill_type_credit[type] * m_bill_scaling_factor);
				return 1;
			}
			else if (routing == 1)
			{
				m_serial->println("escrow position");
				escrow(true);
				poll();
				return 1;
			}
			else if (routing == 2)
			{
				//m_serial->println("bill returned");
			}
			else if (routing == 3)
			{
				//m_serial->println("bill to recycler");
			}
			else if (routing == 4)
			{
				//m_serial->println("disabled bill rejected");
			}
			else if (routing == 5)
			{
				//m_serial->println("bill to recycler - manual fill");
			}
			else if (routing == 6)
			{
				//m_serial->println("manual dispense");
			}
			else if (routing == 7)
			{
				//m_serial->println("transferred from recycler to cashbox");
			}
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
				//m_serial->println("escrow request");
				break;
			case 2:
				//m_serial->println("payout busy");
				break;
			case 3:
				//m_serial->println("dispenser busy");
				break;
			case 4:
				//m_serial->println("defective dispenser sensor");
				break;
			case 5:
				//m_serial->println("not used");
				break;
			case 6:
				//m_serial->println("dispenser did not start / motor problem");
				break;
			case 7:
				//m_serial->println("dispenser jam");
				break;
			case 8:
				//m_serial->println("ROM checksum error");
				break;
			case 9:
				//m_serial->println("dispenser disabled");
				break;
			case 10:
				//m_serial->println("bill waiting");
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
				//m_serial->println("filled key pressed");
				break;
			//default:
				//m_serial->println("default bill rec");
			}
		}
		//status
		else
		{
			switch (m_buffer[i])
			{
			case 1:
				//defective motor
				//m_serial->println("defective motor");
				break;
			case 2:
				//sensor problem
				//m_serial->println("sensor problem");
				break;
			case 3:
				//validator busy
				m_serial->println(F("validator busy"));
				break;
			case 4:
				//ROM Checksum Error
				//m_serial->println("ROM Checksum error");
				break;
			case 5:
				//Validator jammed
				//m_serial->println("validator jammed");
				break;
			case 6:
				//validator was reset
				return JUST_RESET;
			case 7:
				//bill removed
				//m_serial->println("bill removed");
				break;
			case 8:
				//cash box out of position
				//m_serial->println("cash box out of position");
				break;
			case 9:
				//validator disabled
				m_serial->println(F("validator disabled"));
				break;
			case 10:
				//invalid escrow request
				//m_serial->println("invalid escrow request");
				break;
			case 11:
				//bill rejected
				//m_serial->println("bill rejected");
				break;
			case 12:
				//possible credited bill removal
				//m_serial->println("possible credited bill removal");
				break;
			}
		}
	}
	return 1;
}

void BillValidator::security()
{
	int out[] = { 0xff, 0xff };
	m_mdb->SendCommand(ADDRESS, SECURITY, out, 2);
	delay(10);
	if (m_mdb->GetResponse(m_buffer, &m_count) == ACK)
	{
		return;
	}
	delay(50);
	//m_serial->println("security failed");
	security();
}

void BillValidator::Print()
{
	/*
	m_serial->println("BILL VALIDATOR: ");
	m_serial->print(" credit: ");
	m_serial->print(m_credit);
	m_serial->print("\n full: ");
	m_serial->print((bool)m_full);
	m_serial->print("\n bills in stacker: ");
	m_serial->print(m_bills_in_stacker);
	m_serial->print("\n feature level: ");
	m_serial->print((int)m_feature_level);
	m_serial->print("\n bill scaling factor: ");
	m_serial->print((int)m_bill_scaling_factor);
	m_serial->print("\n decimal places: ");
	m_serial->print((int)m_decimal_places);
	m_serial->print("\n capacity: ");
	m_serial->print(m_stacker_capacity);
	m_serial->print("\n security levels: ");
	m_serial->print(m_security_levels);
	
	m_serial->println("\n###");
	*/
}

void BillValidator::setup()
{
	for (int i = 0; i < 64; i++)
		m_buffer[i] = 0;
	m_mdb->SendCommand(ADDRESS, SETUP);
	delay(60);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);
	if (answer > 0 && m_count == 27)
	{	
		m_mdb->Ack();
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
		return;
	}
	
	delay(50);
	m_serial->println(F("BV: setup error"));
	setup();
}

void BillValidator::type(int bills[])
{
	m_mdb->SendCommand(ADDRESS, TYPE, bills, 4);
	delay(10);
	if (m_mdb->GetResponse() == ACK)
	{
		return;
	}
	delay(50);
	m_serial->println(F("type error"));
	type(bills);
}

void BillValidator::escrow(bool accept)
{
	int data[] = { 0x00 };
	if (accept)
		data[0] = 0x01;
	m_mdb->SendCommand(ADDRESS, ESCROW, data, 1);
	delay(10);
	if (m_mdb->GetResponse() == ACK)
	{
		return;
	}
	m_serial->println(F("escrow error"));
	delay(50);
	escrow(accept);
}

void BillValidator::stacker()
{
	m_mdb->SendCommand(ADDRESS, STACKER);
	delay(20);
	int answer = m_mdb->GetResponse(m_buffer, &m_count);
	if (answer > 0 && m_count == 2)
	{
		mdb->Ack();
		if (m_buffer[0] & 0b10000000)
		{
			m_full = true;
		}
		else
			m_full = false;
		m_bills_in_stacker = m_buffer[0] & 0b01111111;
		return;
	}
	m_serial->println(F("stacker error"));
	delay(50);
	stacker();
}

void BillValidator::expansion()
{
	
}