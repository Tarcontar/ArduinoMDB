#include "BillValidator.h"
#include "CoinChanger.h"
#include "MDBSerial.h"

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator validator(mdb);

UART uart;

void setup()
{
  mdb.begin();
  
  uart = UART(0);
  uart.begin();
  Logger::SetUART(&uart);
  
  changer.Reset();
  validator.Reset();
  uart.println("###############");
}

void loop()
{
  unsigned long change;
  changer.Update(change);
  validator.Update(change);
  delay(200);
}
