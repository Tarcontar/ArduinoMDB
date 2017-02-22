#include <SoftwareSerial.h>

//REGISTERS
#define TXB8 0
#define UPE 2
#define DOR 3
#define FE 4
#define UDRE 5
#define RXC 7

//DATA_MODES
#define ADDRESS 1
#define DATA 0
#define DATA_MAX 36

//TIMINGS
#define TIME_OUT 5

//COMMANDS
#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

#define VALIDATOR_JUST_RESET 0x06

#define RESET 0x00
#define SETUP 0x01
#define STATUS 0x02
#define POLL 0x03
#define TYPE 0x04
#define EXPANSION_CMD 0x07

//COIN_CHANGER
bool coin_changer_available = false;
#define COIN_CHANGER 0x08
#define CHANGER_NO_RESPONSE 2000
#define CHANGER_JUST_RESET 0x0B
#define COIN_DISPENSE 0x05

int coin_scaling_factor = 0;
int coin_type_values[16];


//BILL_VALIDATOR
bool bill_validator_available = false;
#define BILL_VALIDATOR 0x30
#define BILL_SECURITY 0x02
#define BILL_ESCROW 0x05



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
  Enable(COIN_CHANGER);
}

void loop()
{
  //HardReset();
  //delay(5000);
  //SendCommand(BILL_VALIDATOR, POLL);
  //delay(500);
  //SendCommand(BILL_VALIDATOR, STATUS);
  //delay(500);
  Enable(COIN_CHANGER);
  delay(500);
  Dispense_Coin(1, 0);
  delay(500);
}

void Enable(unsigned char address)
{
  Serial.println("ENABLE");
  Status(address);
  Type(address);
  Serial.println("ENABLE_SUCCEEDED");
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
  //println("RESET_FAILED: ", data_count);
  delay(1000);
  Reset(address);
}

void Poll(unsigned char address)
{
  SendCommand(address, POLL);
  int result = GetResponse(&data_count);
  if (((result == ACK) && (data_count == 1)) || (data_count == 16))
  {
    SendACK();
    println("POLL_SUCCEEDED: ", data_count);
    return;
  }
  //got a just reset response
  else if (data_count == 1)
  {
    if (address == COIN_CHANGER)
    {
      int result = processChangerPoll();
      if (result == CHANGER_JUST_RESET)
      {
        SendACK();
        println("POLL_SUCCEEDED: ", data_count);
        return;
      }
    }
  }
  Serial.println("POLL_FAILED");
  delay(1000);
  Poll(address);
}

float credit = 0.0f;
int processChangerPoll()
{
  //coins dispensed manually
  if (in_buffer[0] & 0b10000000)
  {
    int number = in_buffer[0] & 0b01110000;
    int coin_type = in_buffer[0] & 0b00001111;
    int coins_in_tube = in_buffer[1];
    Serial.println("coins dispensed manually");
  }
  //coins deposited
  else if (in_buffer[0] & 0b01000000)
  {
    int routing = in_buffer[0] & 0b00110000;
    int coin_type = in_buffer[0] & 0b00001111;
    int coins_in_tube = in_buffer[1];
    if (routing < 2) 
    {
      credit += coin_type_values[coin_type] * coin_scaling_factor;
      Serial.println("coins added to credit");
    }
    else
    {
      Serial.println("coin rejected");
    }
  }
  //slug
  else if (in_buffer[0] & 0b00100000)
  {
    int slug_count = in_buffer[0] & 0b00011111;
  }
  //status
  else
  {
    Serial.println("changer status message....");
    int state = in_buffer[0]; 
    switch (state)
    {
      case 1:
        //escrow request
        Serial.println("NOT_IMPLEMENTED: escrow request");
        break;
      case 2:
        //changer payout busy
        Serial.println("NOT_IMPLEMENTED: payout busy");
        break;
      case 3:
        //no credit
        Serial.println("NOT_IMPLEMENTED: no credit");
        break;
      case 4:
        //defective tube sensor
        Serial.println("NOT_IMPLEMENTED: tube sensor defect");
        break;
      case 5:
        //double arrival
        Serial.println("NOT_IMPLEMENTED: double arrival");
        break;
      case 6:
        //acceptor unplugged
        Serial.println("NOT_IMPLEMENTED: acceptor unplugged");
        break;
      case 7:
        //tube jam
        Serial.println("NOT_IMPLEMENTED: tube jam");
        break;
      case 8:
        //ROM checksum error
        Serial.println("NOT_IMPLEMENTED: checksum error");
        break;
      case 9:
        //coin routing error
        Serial.println("NOT_IMPLEMENTED: coin routing error");
        break;
      case 10:
        //changer busy
        Serial.println("NOT_IMPLEMENTED: changer busy");
        break;
      case 11:
        //changer was reset
        Serial.println("changer was just reset");
        return CHANGER_JUST_RESET;
        break;
      case 12:
        //coin jam 
        Serial.println("NOT_IMPLEMENTED: coin jam");
        break;
      case 13:
        //possible credited coin removal
        Serial.println("NOT_IMPLEMENTED: possible credited coi removal");
        break;
    }
  }
  return 1;
}

//make this unique for every device??
void Setup(unsigned char address)
{
  SendCommand(address, SETUP);
  if (GetResponse(&data_count) && data_count == 23)
  {
    SendACK();
    Serial.println("SETUP_SUCCEEDED");
    Serial.println(data_count);
    return;
  }
  //we get only 3 bytes here why?
  if (data_count <= 3)
  {
    SendACK();
    Serial.println("SETUP_SUCCEEDED");
    Serial.println(data_count);
    return;
  }
  println("SETUP_FAILED: ", data_count);
  SendACK();
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
   //we get only 3 bytes here why?
  if (data_count <= 3)
  {
    SendACK();
    Serial.println("STATUS_SUCCEEDED");
    return;
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

//only valid for coin changer??
void Dispense_Coin(int count, int coin)
{
  // 0 = 5c
  // 1 = 10c
  // 2 = 20c
  // 3 = 50c
  // 4 = 1E
  // 5 = 2E
  out_buffer[0] = (count << 4) | coin;
  out_buffer[0] = 0x10;
  SendCommand(COIN_CHANGER, COIN_DISPENSE, 1);
  if (GetResponse(&data_count) != ACK)
  {
    Serial.println("DISPENSE_FAILED");
    delay(500);
    Dispense_Coin(count, coin);
  }
  Serial.println("DISPENSED");
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
  if (mode)
    UCSR1B |= (1<<TXB8);
  else
    UCSR1B &= ~(1<<TXB8);
  //UDR1 = reverse(cmd);
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
  int sum = 0;
  while(available() && !mode && *count < DATA_MAX)
  {
    if (receive(&in_buffer[*count], &mode))
    {
      sum += in_buffer[*count];
      (*count)++;
      if ((*count == 1) && mode == 1)
      {
        if (in_buffer[(*count)-1] == ACK)
        {
          return ACK;
        }
        else if (in_buffer[(*count)-1] == NAK)
        {
          return NAK;
        }
        else if (in_buffer[(*count)-1] == RET)
        {
          return RET;
        }
      }
    }
  }
  /*
  if (mode)
  {
    if (sum != in_buffer[*count - 1])
    {
      Serial.println("CHECKSUM WRONG");
      return -1;
    }
  }
  */
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

  Serial.println(resl);

  /*
  if (status & ((1<<FE) | (1<<DOR) | (1<<UPE)))
  {
    Serial.println("RECEIVE STATUS ERROR");
    return 0;
  }
  */
  
  (*mode) = (resh >> 1) & 0x01;
  Serial.println(*mode);
  (*data) = resl; 
  return 1;
}

int available()
{
  return (UCSR1A & (1<<RXC));
}

void HardReset()
{
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH);
  delay(210);
  digitalWrite(18, LOW);
  delay(100);
  digitalWrite(18, HIGH);
  mdb_uart_init();
}

void mdb_uart_init()
{
  UCSR1B = (1<<RXEN1)|(1<<TXEN1);
  UBRR1H = 0;
  UBRR1L = 103; // 9600baud
  UCSR1A &= ~(1 << U2X1); //disable rate doubler
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

