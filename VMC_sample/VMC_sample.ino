#include <CoinChanger.h>
#include <MDBSerial.h>
#include <SoftwareSerial.h>

MDBSerial mdb(1);
CoinChanger changer(mdb);

SoftwareSerial serial(0, 1);

void setup()
{
  serial.begin(9600);
  serial.println("test");
  changer.SetSerial(serial);
  changer.Reset();
  serial.println("VMC###############");
}

void loop()
{
  changer.Poll();
  changer.Enable();
  delay(200);
}

