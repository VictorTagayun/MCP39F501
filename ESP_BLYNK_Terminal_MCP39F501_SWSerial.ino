//#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
char auth[] = "215df2de13a54c86b54cbb0fcbef1730";
char ssid[] = "SingaporeMilitia";
char pass[] = "123Qweasd";
WidgetTerminal terminal(V0);
WidgetLED led1(V10);
WidgetLED led2(V11);
int add_cntr = 0;
int data_cntr = 0;
char mcpdata[] = "";
char address;
char add_0;
char add_1;
bool autorun = false;

#include <SoftwareSerial.h>
// SoftwareSerial(int receivePin, int transmitPin, bool inverse_logic, unsigned int buffSize)
SoftwareSerial swSer(0, 14, false, 256); // Pin2 and 13 are high and only those pins

#include <SimpleTimer.h>
SimpleTimer timer1, timer2;

void setup()
{
  Serial1.begin(4800); // Pin2 of ESP8266
  Serial.begin(4800);
  Blynk.begin(auth, ssid, pass);
  while (Blynk.connect() == false) { // Wait until connected
  }

  //pinMode(2, OUTPUT);
  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("  Software v" BLYNK_VERSION ": Device ok...  "));
  terminal.println("=================================");
  terminal.println("========     Victor T.    =======");
  terminal.println("=================================");
  terminal.flush();
  blynk_init();
  timer1.setInterval(1000L, blinkLedWidget);
  timer2.setInterval(1000L, auto_measure);
  autorun = false;
}

// V1 LED Widget is blinking
void blinkLedWidget()
{
  if (led1.getValue()) {
    led1.off();
  } else {
    led1.on();
  }
}

void blynk_init()
{
  //   Init all the wigets
  Blynk.virtualWrite(V1, 0); // address 0
  Blynk.virtualWrite(V2, 0); // address 1
  Blynk.virtualWrite(V3, 0); // select device
  Blynk.virtualWrite(V4, 0); // read values
  Blynk.virtualWrite(V5, 0); // Autorun every 1sec
  Blynk.virtualWrite(V10, 0); // Internet connected
  Blynk.virtualWrite(V11, 0); // Autorun every 1sec
}

BLYNK_WRITE(V1) // address 0
{
  add_0 = param.asInt();
  address = ((add_1 << 1) | (0x01 && add_0));
}

BLYNK_WRITE(V2) // address 1
{
  add_1 = param.asInt();
  address = ((add_1 << 1) | (0x01 && add_0));
}

BLYNK_WRITE(V3) // Select/Deselect Device
{

  char select_packet[5] = {0xa5, 0x05, 0x4c, 0x4c, 0x42};
  char checksum = 0x00;
  char ack = 0x00;

  if (param.asInt()) // select
  {
    select_packet [2] = 0x4c;
    select_packet [3] = address + 0x4c;
    for (int cntr = 0 ; cntr < 4; cntr++)
    {
      checksum = checksum + select_packet[cntr];
    }
    select_packet[4] = checksum;
    for (int cntr = 0 ; cntr < 5; cntr++)
    {
      Serial.write(select_packet[cntr]);
      //Serial1.write(select_packet[cntr]);
    }
  } else // deselect
  {
    select_packet [2] = 0x4b;
    select_packet [3] = address + 0x4c;
    for (int cntr = 0 ; cntr < 4; cntr++)
    {
      checksum = checksum + select_packet[cntr];
    }
    select_packet[4] = checksum;
    for (int cntr = 0 ; cntr < 5; cntr++)
    {
      Serial.write(select_packet[cntr]);
      //Serial1.write(select_packet[cntr]);
    }
  }

  Blynk_Delay(50);

  while (Serial.available() > 0)
  {
    ack = Serial.read();
    //Serial1.write(ack);
  }

  if (ack == 0x15)
  {
    terminal.println("0x15 = NACK");
    terminal.flush();
  } else if (ack == 0x06)
  {
    terminal.println("0x06 = ACK");
    terminal.flush();
  }

}

BLYNK_WRITE(V4) // read vaues
{
  char read_packet[8] = {0xA5, 0x08, 0x41, 0x00, 0x0, 0x4e, 0x98, 0xff};
  char checksum = 0x00;
  char values_packet[9];
  char ack;

  for (int cntr = 0 ; cntr < 7; cntr++)
  {
    checksum = checksum + read_packet[cntr];
  }
  read_packet[7] = checksum;
  for (int cntr = 0 ; cntr < 8; cntr++)
  {
    Serial.write(read_packet[cntr]);
    //Serial1.write(read_packet[cntr]);
  }

  Blynk_Delay(50);

  while (Serial.available() > 0)
  {
    ack = Serial.read();
    Serial1.write(ack);
  }

  terminal.println("Read Values");
  terminal.flush();

}

BLYNK_WRITE(V5) // autorun
{
  autorun = param.asInt();
  if (autorun)
  {
    timer2.restartTimer(0);
    led2.on();
  }
}

void auto_measure()
{
  char read_packet[8] = {0xA5, 0x08, 0x41, 0x00, 0x0, 0x4e, 0x98, 0xff};
  char checksum = 0x00;

  for (int cntr = 0 ; cntr < 7; cntr++)
  {
    checksum = checksum + read_packet[cntr];
  }
  read_packet[7] = checksum;
  for (int cntr = 0 ; cntr < 8; cntr++)
  {
    Serial.write(read_packet[cntr]);
    //Serial1.write(read_packet[cntr]);
  }

  Blynk_Delay(50);

  while (Serial.available() > 0)
  {
    char ack = Serial.read();
    Serial1.write(ack);
  }

  terminal.println("Aut run");
  terminal.flush();

  if (led2.getValue()) {
    led2.off();
  } else {
    led2.on();
  }

}

void loop()
{
  Blynk.run();
  timer1.run();
  if (autorun)
  {
    timer2.run();
  }
}

void Blynk_Delay(int milli)
{
  int end_time = millis() + milli;
  while (millis() < end_time)
  {
    if (Blynk.connected())
    {
      Blynk.run();
    }
    yield();
  }
}

// Keep this flag not to re-sync on every reconnection
bool isFirstConnect = true; // http://community.blynk.cc/t/solved-blynk-syncall-not-found-in-esp8266-arduino/4401

//BLYNK_CONNECTED() {
//  if (isFirstConnect) {
//    Blynk.syncAll();
//  }
//  isFirstConnect = false;
//}
