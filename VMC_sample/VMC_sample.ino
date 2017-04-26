#include <CoinChanger.h>
#include <MDBSerial.h>

MDBSerial mdb(1);
CoinChanger changer(mdb);

void setup()
{
  Serial.begin(9600);
  Serial.println("test");
  changer.Reset();
  Serial.println("VMC###############");
}

void loop()
{
  changer.Poll();
  changer.Enable();
  delay(200);
}

