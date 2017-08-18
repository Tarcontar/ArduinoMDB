#include <CoinChanger.h>
#include <MDBSerial.h>
#include <SoftwareSerial.h>
#include <BillValidator.h>

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator validator(mdb);

SoftwareSerial serial(0, 1);

void setup()
{
  serial.begin(9600);
  serial.println("test");
  changer.SetSerial(serial);
  changer.Reset();
  validator.SetSerial(serial);
  validator.Reset();
  serial.println("VMC###############");
}

void loop()
{
  unsigned long change = changer.Update();
  validator.Update(change);
  delay(200);
}

