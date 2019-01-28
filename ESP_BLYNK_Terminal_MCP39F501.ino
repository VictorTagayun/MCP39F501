#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
char auth[] = "215df2de13a54c86b54cbb0fcbef1730";
char ssid[] = "SingaporeMilitia";
char pass[] = "123Qweasd";
WidgetTerminal terminal(V0);
int add_cntr = 0;
int data_cntr = 0;
char mcpdata[] = "";

void setup()
{
  Serial.begin(4800);
  Blynk.begin(auth, ssid, pass);
  while (Blynk.connect() == false) { // Wait until connected
  }
  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println("=================================");
  terminal.println("========     Victor T.    =======");
  terminal.println("=================================");
  terminal.flush();
}

// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
BLYNK_WRITE(V9) // cycle device address
{
  add_cntr++;
  switch (add_cntr)
  {
    case 1:    // your hand is on the sensor
      terminal.println("a5+05+4c+4c+42");
      terminal.flush();
      Serial.write(0xa5);
      Serial.write(0x05);
      Serial.write(0x4c);
      Serial.write(0x4c);
      Serial.write(0x42);
      break;
    case 2:    // your hand is close to the sensor
      terminal.println("a5+05+4c+4d+43");
      terminal.flush();
      Serial.write(0xa5);
      Serial.write(0x05);
      Serial.write(0x4c);
      Serial.write(0x4d);
      Serial.write(0x43);
      break;
    case 3:    // your hand is a few inches from the sensor
      terminal.println("a5+05+4c+4e+44");
      terminal.flush();
      Serial.write(0xa5);
      Serial.write(0x05);
      Serial.write(0x4c);
      Serial.write(0x4e);
      Serial.write(0x44);
      break;
    case 4:    // your hand is nowhere near the sensor
      terminal.println("a5+05+4c+4f+45");
      terminal.flush();
      Serial.write(0xa5);
      Serial.write(0x05);
      Serial.write(0x4c);
      Serial.write(0x4f);
      Serial.write(0x45);
      break;
  }
  if (add_cntr == 4) add_cntr = 0;
}

BLYNK_WRITE(V5) // read values
{
  terminal.println("0xa5+0x08+0x41+0x00+0x02+0x4e+0x1e+0x5c");
  terminal.flush();
  Serial.write(0xa5);
  Serial.write(0x08);
  Serial.write(0x41);
  Serial.write(0x00);
  Serial.write(0x02);
  Serial.write(0x4e);
  Serial.write(0x1e);
  Serial.write(0x5c);
}

BLYNK_WRITE(V2)
{

}

void loop()
{
  Blynk.run();
}

