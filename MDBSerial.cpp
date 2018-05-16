#include "MDBSerial.hpp"


MDBSerial::MDBSerial(uint8_t uart) 
{
	m_uart = new UART(uart);
}

bool MDBSerial::begin()
{
	//hardReset(); //does not work at the moment
	return m_uart->begin(9600, true);
}

void MDBSerial::hardReset()
{
	/*
	pinMode(m_TXn, OUTPUT);
	digitalWrite(m_TXn, HIGH);
	delay(TIME_SETUP);
	digitalWrite(m_TXn, LOW);
	delay(TIME_BREAK);
	digitalWrite(m_TXn, HIGH);
	*/
}

void MDBSerial::Ack()
{
	m_uart->write9bit(0x00); //need 0 since ACK is 0x100 and doesnt work
}

void MDBSerial::Nak()
{
	m_uart->write9bit(NAK);
}

void MDBSerial::Ret()
{
	m_uart->write9bit(RET);
}

void MDBSerial::SendCommand(int address, int cmd, int *data, int dataCount)
{
	SendCommand(address, cmd, -1, data, dataCount);
}

void MDBSerial::SendCommand(int address, int cmd,  int subCmd, int *data, int dataCount)
{
	char sum = 0;
	m_uart->flush();
	m_uart->write9bit(0x100 | address | cmd);
	sum += address | cmd;
	
	if (subCmd >= 0)
	{
		m_uart->write9bit(subCmd);
		sum += subCmd;
	}

	for (int i = 0; i < dataCount; i++)
	{
		m_uart->write9bit(data[i]);
		sum += data[i];
	}
	m_uart->write9bit(sum); //send the checksum
	delay(RESPONSE_TIME * 2);
}

int MDBSerial::GetResponse(char data[], int *count, int num_bytes)
{	
	char sum = 0;
	*count = 0;
	
	//wait for the response to arrive
	delay(RESPONSE_TIME);
	for (int i = 0; i < num_bytes; i++)
	{
		if (m_uart->ninthBitSet())
		{
			break;
		}
		else if (m_uart->error())
		{
			m_uart->flush();
			return -1;
		}
		else
		{
			delay(BYTE_SEND_TIME + INTER_BYTE_TIME);
		}
	}
	
	//nothing received
	if (!m_uart->available())
		return -2;
	
	int c = 0;
	while (m_uart->available())
	{
		int resp = m_uart->read();
		char val = resp;
		if (resp & 0x100)
		{
			if (c == 0) //we got an ACK //or NAK or RET??
			{
				m_uart->flush();
				if (resp == ACK)
					return ACK;
				else if (resp == NAK)
					return -4;
				else
					return -5;
			}
			else //checksum of data
			{
				if (sum != val)
				{
					m_uart->flush();
					return -3;
				}
			}
		}
		else
		{
			data[c++] = val;
			sum += val;
		}
	}
	*count = c;
	m_uart->flush();
	return 1;
}