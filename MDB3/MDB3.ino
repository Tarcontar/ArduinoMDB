#include <SoftwareSerial.h>

#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define ADDRESS 1
#define DATA 0
#define DATA_MAX 36

//TIMINGS
#define TIME_OUT 5
#define CHANGER_NO_RESPONSE 2000


//ADDRESSES
#define COIN_CHANGER 0x08
#define BILL_VALIDATOR 0x30

//COMMANDS
#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

#define RESET 0x00
#define SETUP 0x01
#define STATUS 0x02
#define BILL_SECURITY 0x02
#define POLL 0x03
#define TYPE 0x04
#define COIN_DISPENSE 0x05
#define BILL_ESCROW 0x05
#define EXPANSION_CMD 0x07

int data_count = 0;
unsigned char in_buffer[40];
int out_buffer[40];


unsigned long _commandSentTime;

void setup()
{
  Serial.begin(9600);
  Serial.println("started VMC");

  mdb_uart_init();

  HardReset(); 

  Reset(COIN_CHANGER);
  delay(500);
 //Enable(COIN_CHANGER);
}

void loop()
{

}

void Enable(unsigned char address)
{
  Serial.println("ENABLE");
  Status(address);
  Type(address);
  Serial.println("ENABLE_SUCCEEDED");
}

void Error(unsigned char address)
{
  Poll(address);
}

void Reset(unsigned char address)
{
  Serial.println("RESET");
  SendCommand(address, RESET);
  if ((GetResponse(&data_count) == ACK) && (data_count == 1))
  {
    Poll(address);
    Setup(address);
    //Expansion(address, 0x00); //ID
   // Expansion(address, 0x01); //Feature
   // Expansion(address, 0x05); //Status
    Serial.println("RESET_SUCCEEDED");
    return;
  }
  println("RESET_FAILED: ", data_count);
  delay(1000);
  Reset(address);
}

void Poll(unsigned char address)
{
  SendCommand(address, POLL);
  if (((GetResponse(&data_count) == ACK) && (data_count == 1)) || (data_count == 16))
  {
    SendACK();
    println("POLL_SUCCEEDED: ", data_count);
    return;
  }
  Serial.println("POLL_FAILED");
  delay(1000);
  Poll(address);
}

void Setup(unsigned char address)
{
  SendCommand(address, STATUS);
  if (GetResponse(&data_count) && data_count == 23)
  {
    SendACK();
    Serial.println("SETUP_SUCCEEDED");
    Serial.println(data_count);
    return;
  }
  println("SETUP_FAILED: ", data_count);
  delay(1000);
  Setup(address);
}

void Status(unsigned char address)
{
  SendCommand(address, STATUS);
  if (GetResponse(&data_count) && data_count == 18)
  {
    SendACK();
    Serial.println("STATUS_SUCCEEDED");
    Serial.println(data_count);
  }
  println("STATUS_FAILED: ", data_count);
  delay(1000);
  Status(address);
}

void Type(unsigned char address)
{
  out_buffer[0] = 0xff;
  out_buffer[1] = 0xff;
  out_buffer[2] = 0xff;
  out_buffer[3] = 0xff;
  SendCommand(address, TYPE, 4);
  if (GetResponse(&data_count) != ACK)
  {
    Serial.println("TYPE_FAILED");
    delay(1000);
    Type(address);
  }
  Serial.println("TYPE_SUCCEEDED");
}

void Expansion(unsigned char address, unsigned char sub_cmd)
{
  out_buffer[0] = sub_cmd;
  SendCommand(address, EXPANSION_CMD, 1);
  if (GetResponse(&data_count) && data_count == 33)
  {
    switch(sub_cmd)
    {
      case 0x00:
        if (data_count == 33)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      case 0x01:
        if (data_count == 4)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
     case 0x02:
        if (data_count == 0)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      case 0x03:
        if (data_count == 16)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      case 0x04:
        if (data_count == 1)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      case 0x05:
        if (data_count == 2)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      case 0x06:
        if (data_count == 16)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      case 0x07:
        if (data_count == 16)
          Serial.println("EXPANSION_SUCCEEDED");
          return;
        break;
      default:
        break;
    }
  }
  println("EXPANSION_FAILED", data_count);
  delay(1000);
  Expansion(address, sub_cmd);
}




///BASE FUNCTIONS

void SendCommand(unsigned char address, unsigned char command)
{
  SendCommand(address, command, 0);
}

void SendCommand(unsigned char address, unsigned char command, unsigned int dataByteCount)
{
  unsigned char sum = 0;
  send(address | command, ADDRESS);
  sum += address | command;

  for (unsigned int i = 0; i < dataByteCount; i++)
  {
    send(out_buffer[i], DATA);
    sum += out_buffer[i];
  }
  //send checksum
  send(sum, DATA);
}

void SendACK()
{
  send(ACK, DATA);
}

void SendRET()
{
  send(RET, DATA);
}

void SendNAK()
{
  send(NAK, DATA);
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

int GetResponse(int *count)
{
  int mode = 0;
  (*count) = 0;
  //wait for response
  while(!available())
  {
    if ((millis() - _commandSentTime) > TIME_OUT) 
    {
      Serial.println("TIMED_OUT");
      return -1;
    }
  }
  while(available() && !mode && *count < DATA_MAX)
  {
    if (receive(&in_buffer[*count], &mode))
    {
      if ((in_buffer[*count] == ACK) && (mode == 1))
        return ACK;
      (*count)++;
    }
  }
  return 1;
}

int receive(unsigned char *data, int *mode)
{
  unsigned char status, resh, resl;
  if(!(UCSR1A & (1<<RXC))) 
  {
    Serial.println("NO DATA TO READ");
    return 0;
  }
  status = UCSR1A;
  resh = UCSR1B;
  resl = UDR1;

  if (status & ((1<<FE) | (1<<DOR) | (1<<UPE)))
  {
    Serial.println("STATUS ERROR");
    return 0;
  }
  
  (*mode) = (resh >> 1) & 0x01;
  (*data) = resl; 
  Serial.println(*mode);
  Serial.println(*data);
  return 1;
}

int available()
{
  return (UCSR1A & (1<<RXC));
}

void HardReset()
{
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  delay(200);
  digitalWrite(18, HIGH);
  delay(200);
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

void println(String msg, int val)
{
  Serial.print(msg);
  Serial.print(val);
  Serial.print("\n");
}



