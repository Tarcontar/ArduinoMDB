#include <CoinChanger.h>
#include <MDBSerial.h>
#include <SoftwareSerial.h>
#include <BillValidator.h>

MDBSerial mdb(1);
CoinChanger changer(mdb);
BillValidator vali(mdb);

SoftwareSerial serial(0, 1);

void setup()
{
  serial.begin(9600);
  serial.println("test");
  changer.SetSerial(serial);
  changer.Reset();
  vali.SetSerial(serial);
  vali.Reset();
  serial.println("VMC###############");
}

void loop()
{
  //changer.Update();
  //vali.Update(100);
  delay(200);
}

