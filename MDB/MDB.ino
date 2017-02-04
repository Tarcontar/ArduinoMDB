#include <MdbSerial.h>
#include <SoftwareSerial.h>

#define COIN_ADDR 0x08

#define COIN_RESET 0x00
#define COIN_SETUP 0x01
#define COIN_TUBE_STATUS 0x02
#define COIN_POLL 0x03
#define COIN_TYPE 0x04
#define COIN_DISPENSE 0x05
#define COIN_EXPANSION 0x07

#define ACK 0x00
#define RET 0xAA
#define NAK 0xFF

// MDB state machine states.
enum MdbState
{
  hardReset,
  waitHardReset,
  softReset,
  waitSoftReset,
  initState,
  waitJustReset,
  ackJustReset,
  getStatus,
  waitStatusResponse,
  ackStatusResponse,
  sendSecurity,
  waitSecurityResponse,
  getLevel1Id,
  waitLevel1Id,
  enableCoinTypes,
  poll,
  readPollResponse,
  ackPollResponse,
  acceptCoin,

  displayResponse
};


SoftwareSerial pcSerial(0, 1);
MdbState state;

void setup()
{
  pcSerial.begin(9600);
  pcSerial.println("vmc started");
  
  //Hard Reset
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  delay(200);
  digitalWrite(18, HIGH);
  delay(200);

  MdbPort1.begin();
  state = initState;
}

unsigned char extraCmdBytes[33];
unsigned char responseBytes[36];

unsigned long currentTime = 0;
unsigned long debugTime = 0;

void loop()
{
 switch(state)
 {   
  case softReset:
      MdbSendCommand(COIN_RESET, extraCmdBytes, 0);
      currentTime = millis();

      pcSerial.println("Soft Reset");
      state = waitSoftReset;
      break;
      
  case waitSoftReset:
      if (millis() - currentTime > (10*1000))
      {
        state = initState;
      }
      break;

  case initState:
      pcSerial.println(MdbPort1.available());
      while (MdbPort1.available())
        MdbGetResponse(responseBytes, 1);
      pcSerial.println(MdbPort1.available());
      MdbSendCommand(COIN_SETUP, extraCmdBytes, 0);
      currentTime = millis();

      pcSerial.println("initState");
      state = waitJustReset;
      break;

  case waitJustReset:
      pcSerial.println(MdbPort1.available());
      if (MdbPort1.available() > 0)
      {
        MdbGetResponse(responseBytes, MdbPort1.available());
        pcSerial.println("got Reset response");
        state = ackJustReset;
      }
      else if ((millis() - currentTime) > 150)
      {
        //try again
        state = initState;
      }
      pcSerial.println("waitJustReset");
      break;

  case ackJustReset:
      MdbSendAck();
      delay(10);

      state = getStatus;
      break;

  case getStatus:
      MdbSendCommand(COIN_TUBE_STATUS, extraCmdBytes, 0);
      currentTime = millis();

      pcSerial.println("getStatus");
      state = waitStatusResponse;
      break;

  case waitStatusResponse:
      pcSerial.println(MdbPort1.available());
      if(MdbPort1.available() > 0)
      {
        MdbGetResponse(responseBytes, 23);
        pcSerial.print("scaling: ");
        pcSerial.print(responseBytes[3]);
        pcSerial.print("routing: ");
        pcSerial.print(responseBytes[5]);
        pcSerial.print(responseBytes[6]);
        for (int i = 7; i < 23; i++)
        {
          //pcSerial.println("credit: ");
          //pcSerial.print(responseBytes[i]);
        } 
        pcSerial.println("gotStatusResponse");
        delay(10);
        state = ackStatusResponse;
      }
      else if ((millis() - currentTime) > 150)
      {
        //try again
        state = getStatus;
      }
      break;

  case ackStatusResponse:
      MdbSendAck();
      delay(10);
      state = enableCoinTypes;
      break;

  case enableCoinTypes:
      extraCmdBytes[0] = 0x00;
      extraCmdBytes[1] = 0x0F;
      extraCmdBytes[2] = 0xFF;
      extraCmdBytes[2] = 0xFF;


      pcSerial.println("enable coins");
      MdbSendCommand(COIN_TYPE, extraCmdBytes, 4);
      delay(10);
      state = poll;
      break;

  case poll:
      MdbSendCommand(COIN_POLL, extraCmdBytes, 0);
      state = readPollResponse;
      delay(10);
      break;

  case readPollResponse:
      if (MdbPort1.available() > 1)
      {
        MdbGetResponse(responseBytes, 1); //nothing to report, only ack
        state = ackPollResponse;
      }
      else if (MdbPort1.available() > 1)
      {
        MdbGetResponse(responseBytes, 16); 
        pcSerial.println("number and coin: " + responseBytes[0]);
        //pcSerial.println("eingezahlt: " + responseBytes[]);
        pcSerial.println("got poll response");
        state = ackPollResponse;
      }
      else if ((millis() - currentTime) > 150)
      {
        //try again
        state = poll;
      }
      delay(1000);
      break;

  case ackPollResponse:
      MdbSendAck();
      delay(10);

      if (responseBytes[0] == 0x94)
      {
        state = acceptCoin;
      }
      else
      {
        //poll repeatedly
        state = poll;
      }
      break;

  case acceptCoin:
      extraCmdBytes[0] = 0x01;
      //MdbSendCommand(??, extraCmdBytes, 1);

      state = poll;
      break;
  
  default:
      break;
      
 }
}

void MdbSendCommand(unsigned char command, unsigned char *cmdBytes, unsigned int numCmdBytes)
{
  unsigned char sum = 0;
  //write Address
  MdbPort1.write(COIN_ADDR | command, 1);
  sum += COIN_ADDR | command;

  //avoid buffer overflows on cmdBytes
  if (numCmdBytes > 33)
  {
    numCmdBytes = 33;
  }

  //send each additional byte for the command
  for (int i = 0; i < numCmdBytes; i++)
  {
    MdbPort1.write(cmdBytes[i], 0);
    sum += cmdBytes[i];
  }

  MdbPort1.write(sum, 0);
}

void MdbSendAck()
{
  MdbPort1.write(COIN_ADDR, 0);
}

void MdbGetResponse(unsigned char* response, unsigned int numBytes) //should be max 36 bytes
{
  int index = 0;
  int lastMode = 0;
  // wait for enough bytes to be ready
  while (MdbPort1.available() < numBytes);
  while ((MdbPort1.available() > 0) && (index < 37) && (lastMode == 0))
  {
    response[index] = MdbPort1.read() ; //& 0xFF;
    lastMode = (response[index] >> 8) & 0x01;
    index++;
  }
}

