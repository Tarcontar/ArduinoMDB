#include <SoftwareSerial.h>

#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define COIN_CHANGER 0x08
#define BILL_VALIDATOR 0x30

#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

#define RESET 0x00
#define SETUP 0x01
#define COIN_TUBE_STATUS 0x02
#define BILL_SECURITY 0x02
#define POLL 0x03
#define COIN_TYPE 0x04
#define BILL_TYPE 0x04
#define COIN_DISPENSE 0x05
#define BILL_ESCROW 0x05
#define EXPANSION_CMD 0x07

unsigned char data[64];
unsigned long _commandSentTime;

void setup()
{
  Serial.begin(9600);
  Serial.println("started VMC");

  //hard Reset
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  delay(200);
  digitalWrite(18, HIGH);
  delay(200);

  mdb_uart_init();

  SendCommand(COIN_CHANGER, RESET);
  delay(20);

   while(available())
  {
     GetResponse(&data[0], 0);
     Serial.println(data[0]);
  }

  SendCommand(COIN_CHANGER, SETUP);
  delay(20);

   while(available())
  {
     GetResponse(&data[0], 0);
     Serial.println(data[0]);
  }

  SendCommand(COIN_CHANGER, POLL);
  delay(200);

  while(available())
  {
     GetResponse(&data[0], 0);
     Serial.println(data[0]);
  }
}

void loop()
{
  send(0x00,1);
  delay(500);
}

void SendCommand(unsigned char address, unsigned char command)
{
  unsigned char dataBytes[1];
  SendCommand(address, command, dataBytes, 0);
}

void SendCommand(unsigned char address, unsigned char command, unsigned char *dataBytes, unsigned int dataByteCount)
{
  unsigned char sum = 0;
  send(address | command, 1);
  sum += address | command;

  for (unsigned int i = 0; i < dataByteCount; i++)
  {
    send(dataBytes[i], 0);
    sum += dataBytes[i];
  }
  //send checksum
  send(sum, 0);
}

void SendACK(unsigned char address)
{
  send(address | ACK, 1);
}

void send(unsigned char cmd, int mode)
{
  while(!(UCSR1A & (1<<UDRE)));
  UCSR1B &= ~(1<<TXB8);
  if (mode)
    UCSR1B |= (1<<TXB8);
  UDR1 = cmd;

  _commandSentTime = millis();
}

int GetResponse(unsigned char *response, unsigned int *numBytes)
{
  int index = 0;
  int mode = 0;
  //while(available() > 0) 
  //{
    receive(&response[index], &mode);
    index++;
  //}
  *numBytes = index;
  return 0;
}

unsigned char receive(unsigned char *data, int *mode)
{
  unsigned char status, resh, resl;
  if(!(UCSR1A & (1<<RXC))) return;
  status = UCSR1A;
  resh = UCSR1B;
  resl = UDR1;
  Serial.println(resl);

  /*
  if (status & ((1<<FE) | (1<<DOR) | (1<<UPE)))
  {
    Serial.println("failed");
    return -1;
  }
  */
  resh = (resh >> 1) & 0x01;
  *mode = resh;
  (*data) = resl; 
  return 0;
}

int available()
{
  return (UCSR1A & (1<<RXC));
}

void mdb_uart_init()
{
  UCSR1B = (1<<RXEN1)|(1<<TXEN1);
  UBRR1H = 0;
  UBRR1L = 103; // 9600baud
  UCSR1C |= (1<<UCSZ11)|(1<<UCSZ10); //9bit mode
  UCSR1B |= (1<<UCSZ12); //also for 9 bit mode
  UCSR1C = UCSR1C | B00000100; //one stop bit
  UCSR1C = UCSR1C | B00000000; //no parity bit
}

