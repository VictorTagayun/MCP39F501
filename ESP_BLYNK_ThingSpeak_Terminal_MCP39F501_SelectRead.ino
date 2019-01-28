//#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
char auth[] = "215df2de13a54c86b54cbb0fcbef1730";
char ssid[] = "SingaporeMilitia";
char pass[] = "123Qweasd";
WidgetTerminal terminal(V0);
WidgetLED led1(V10);
WidgetLED led2(V11);

bool autorun = false;
#define numdata  0x76
int add_cntr = 0;
char address;
char add_0;
char add_1;
bool rts = false;
int rts_pin = 14;

char mcp_regs[numdata];
char data_received[numdata + 3];
int data_cntr = 0;
bool msg_ok = false;
char system_version1;
char system_version2;
float current_rms;
float voltage_rms;
float active_power;
float reactive_power;
float apparent_power;
float power_factor;
float line_freq;
float thermstor_v;
char event_flag1;
char event_flag2;
char system_status;

#include <ThingSpeak.h>
unsigned long myChannelNumber = 167131;
const char * myWriteAPIKey = "K4R1W09M7E4D5ZF2";
WiFiClient  client;

#include <SoftwareSerial.h>
// SoftwareSerial(int receivePin, int transmitPin, bool inverse_logic, unsigned int buffSize)
SoftwareSerial swSer(0, 14, false, 256); // Pin2 and 13 are high and only those pins

#include <SimpleTimer.h>
SimpleTimer timer1, timer2, timer3;

void setup()
{
  Serial1.begin(4800); // Pin2 of ESP8266
  Serial.begin(4800);
  Blynk.begin(auth, ssid, pass);
  while (Blynk.connect() == false)   // Wait until connected
  {
  }

  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("  Software v" BLYNK_VERSION ": Device ok...  "));
  terminal.println("=================================");
  terminal.println("========     Victor T.    =======");
  terminal.println("=================================");
  terminal.flush();
  ThingSpeak.begin(client);
  blynk_init();
  timer1.setInterval(1000L, blinkLedWidget);
  timer2.setInterval(2000L, auto_measure);
  timer3.setInterval(20000L, display_thingspeak);

  autorun = false;
}

// V1 LED Widget is blinking
void blinkLedWidget()
{
  if (led1.getValue())
  {
    led1.off();
  }
  else
  {
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
  Blynk.virtualWrite(V6, 0); //
  Blynk.virtualWrite(V7, 0); //
  Blynk.virtualWrite(V8, 0); //
  Blynk.virtualWrite(V9, 0); //
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

  if (rts)
  {
    digitalWrite(rts_pin, HIGH);
    Blynk_Delay(1);
  }

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
  }
  else   // deselect
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

  if (rts)
  {
    Blynk_Delay(10);
    digitalWrite(rts_pin, LOW);
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
  }
  else if (ack == 0x06)
  {
    terminal.println("0x06 = ACK");
    terminal.flush();
  }

}

BLYNK_WRITE(V4) // read vaues
{
  char read_packet[8] = {0xA5, 0x08, 0x41, 0x00, 0x0, 0x4e, numdata, 0xff};
  char checksum = 0x00;
  int send_cntr = 8;

  if (rts)
  {
    digitalWrite(rts_pin, HIGH);
    Blynk_Delay(1);
  }
  read_packet[1] = send_cntr;
  for (int cntr = 0 ; cntr < (send_cntr - 1); cntr++)
  {
    checksum = checksum + read_packet[cntr];
  }
  read_packet[7] = checksum;
  for (int cntr = 0 ; cntr < send_cntr; cntr++)
  {
    Serial.write(read_packet[cntr]);
    //Serial1.write(read_packet[cntr]);
  }

  if (rts)
  {
    Blynk_Delay(10);
    digitalWrite(rts_pin, LOW);
  }

  Blynk_Delay(500);

  read_and_chk_packet();

}

BLYNK_WRITE(V5) // autorun
{
  autorun = param.asInt();
  if (autorun)
  {
    timer2.restartTimer(0);
    timer3.restartTimer(0);
    led2.on();
  }
}

BLYNK_WRITE(V6) // RTS
{
  rts = param.asInt();
}

BLYNK_WRITE(V7) // select and read data
{
  // select device = 0x4c

  // read data = 0x41, 0x00, 0x0, 0x4e, numdata,

  // combine select and read
  char send_checksum = 0x00;
  int send_cntr = 10;
  char select_and_read [] = {0xa5, 0x0a, 0x4c, 0x4c, 0x41, 0x00, 0x0, 0x4e, numdata, 0xff}; // {0xa5, 0x0a, 0x4c, 0x4c, 0x41, 0x00, 0x0, 0x4e, 0x5e, 0xff};

  if (rts)
  {
    digitalWrite(rts_pin, HIGH);
    Blynk_Delay(1);
  }
  select_and_read[1] = send_cntr;
  select_and_read[3] = address + 0x4c;

  for (int cntr = 0 ; cntr < (send_cntr - 1); cntr++)
  {
    send_checksum = send_checksum + select_and_read[cntr];
  }
  select_and_read[9] = send_checksum;

  for (int cntr = 0 ; cntr < send_cntr; cntr++)
  {
    Serial.write(select_and_read[cntr]);
  }

  if (rts)
  {
    Blynk_Delay(10);
    digitalWrite(rts_pin, LOW);
  }

  Blynk_Delay(500);

  read_and_chk_packet();
  //read_and_chk_packet_temp();

}

BLYNK_WRITE(V8) // select and read data deselect
{
  // select device = 0x4c, address

  // read data = 0x41, 0x00, 0x0, 0x4e, numdata,

  // deselect device 0x4b, address

  // combine select and read
  char send_checksum = 0x00;
  int send_cntr = 12 ;
  char select_read_deselect [] = {0xa5, 0x0a, 0x4c, 0x4f, 0x41, 0x00, 0x0, 0x4e, numdata, 0x4b, 0x4f, 0xff}; // char select_and_read [] = {0xa5, 0x0a, 0x4c, 0x4c, 0x41, 0x00, 0x0, 0x4e, numdata, 0xff};

  if (rts)
  {
    digitalWrite(rts_pin, HIGH);
    Blynk_Delay(1);
  }

  select_read_deselect[3] = address + 0x4c;
  select_read_deselect[10] = address + 0x4c;
  select_read_deselect[1] = send_cntr;
  for (int cntr = 0 ; cntr < (send_cntr - 1); cntr++)
  {
    send_checksum = send_checksum + select_read_deselect[cntr];
  }
  select_read_deselect[send_cntr - 1] = send_checksum;

  for (int cntr = 0 ; cntr < send_cntr; cntr++)
  {
    Serial.write(select_read_deselect[cntr]);
  }

  if (rts)
  {
    Blynk_Delay(10);
    digitalWrite(rts_pin, LOW);
  }

  Blynk_Delay(500);

  read_and_chk_packet();

}

BLYNK_WRITE(V9) //
{

}

void read_and_chk_packet()
{
  // reset all
  char checksum = 0x0;
  data_cntr = 0;

  while (Serial.available() > 0)
  {
    data_received[data_cntr] = Serial.read();
    //Serial1.write(data_received[data_cntr]);
    data_cntr++;
  }

  if (data_received[0] == 0x15) // NACK
  {
    msg_ok = false;
    terminal.println("NACK");
    //Serial1.println("NACK");
    //Serial1.write(data_received[0]);
  }
  else if (data_received[0] == 0x51) // checksum error
  {
    msg_ok = false;
    terminal.println("Checksum Error!");
    //Serial1.println("Checksum Error!");
    //Serial1.write(data_received[0]);
  }
  else if (data_cntr < (numdata + 3) ) // wrong packet number
  {
    msg_ok = false;
    terminal.println("ERROR in DATA Rceived!");
    //Serial1.println("ERROR in DATA Rceived!");
    //Serial1.write(data_cntr);
  }
  else if (data_cntr > (numdata + 3) ) // wrong packet number
  {
    msg_ok = false;
    terminal.println("ERROR in DATA Rceived!");
    //Serial1.println("ERROR in DATA Rceived!");
    //Serial1.write(data_cntr);
  }
  else if ((data_cntr  ==  (numdata + 3)) && (data_received[0] == 0x06)) // correct packet number & ACK
  {
    // check checksum
    for (int cntr = 0; cntr < (data_cntr - 1); cntr++)
    {
      checksum = checksum + data_received[cntr];
    }

    if (checksum == data_received[data_cntr - 1])
    {
      msg_ok = true;
      // transfer to mcp reg mapping
      for (int cntr = 0; cntr < numdata; cntr++)
      {
        mcp_regs[cntr] = data_received[cntr + 2];
        //Serial1.write(mcp_regs[cntr]);
      }
      display_values();
    }
  }

}

void display_values()
{
  system_version1 = mcp_regs[2];
  system_version1 = mcp_regs[3];
  current_rms = (mcp_regs[4] | (mcp_regs[5] << 8) | (mcp_regs[6] << 16) | (mcp_regs[7] << 24)) / 10000;
  voltage_rms = (mcp_regs[8] | (mcp_regs[9] << 8)) / 10;
  active_power = (mcp_regs[10] | (mcp_regs[11] << 8) | (mcp_regs[12] << 16) | (mcp_regs[13] << 24)) / 100;
  reactive_power = (mcp_regs[14] | (mcp_regs[15] << 8) | (mcp_regs[16] << 16) | (mcp_regs[17] << 24)) / 100;
  apparent_power = (mcp_regs[18] | (mcp_regs[19] << 8) | (mcp_regs[20] << 16) | (mcp_regs[21] << 24)) / 100;
  power_factor = (mcp_regs[22] | (mcp_regs[23] << 8)) * 0.000030517578125;
  line_freq = (mcp_regs[24] | (mcp_regs[25] << 8)) / 1000;
  thermstor_v =  (mcp_regs[26] | (mcp_regs[27] << 8)) / 10.0 * 1.8 + 32;
  event_flag1 = mcp_regs[28];
  event_flag2 = mcp_regs[29];
  system_status = mcp_regs[30];

  // display to terminal
  terminal.print("Voltage (RMS)  : "); terminal.println(voltage_rms);
  terminal.print("Current (RMS)  : "); terminal.println(current_rms);
  terminal.print("Active Power   : "); terminal.println(active_power);
  terminal.print("Reactive Power : "); terminal.println(reactive_power);
  //terminal.print("Apparent Power : "); terminal.println(apparent_power);
  terminal.print("Power factor   : "); terminal.println(power_factor);
  terminal.print("Line Freq.     : "); terminal.println(line_freq);
  //terminal.println("============================");
  terminal.flush();

  // display to serial
  Serial1.print("Voltage (RMS)  : "); Serial1.println(voltage_rms);
  Serial1.print("Current (RMS)  : "); Serial1.println(current_rms);
  Serial1.print("Active Power   : "); Serial1.println(active_power);
  Serial1.print("Reactive Power : "); Serial1.println(reactive_power);
  Serial1.print("Apparent Power : "); Serial1.println(apparent_power);
  Serial1.print("Power factor   : "); Serial1.println(power_factor);
  Serial1.print("Line Freq.     : "); Serial1.println(line_freq);
  Serial1.println("============================");

}

void display_thingspeak()
{

  // ThkngSpeak
  ThingSpeak.setField(1, voltage_rms);
  ThingSpeak.setField(2, current_rms);
  ThingSpeak.setField(3, active_power);
  ThingSpeak.setField(4, reactive_power);
  ThingSpeak.setField(5, apparent_power);
  ThingSpeak.setField(6, power_factor);
  ThingSpeak.setField(7, line_freq);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

}

void auto_measure()
{
  
  char read_packet[8] = {0xA5, 0x08, 0x41, 0x00, 0x0, 0x4e, numdata, 0xff};
  char checksum = 0x00;
  int send_cntr = 8;

  if (rts)
  {
    digitalWrite(rts_pin, HIGH);
    Blynk_Delay(1);
  }
  read_packet[1] = send_cntr;
  for (int cntr = 0 ; cntr < (send_cntr - 1); cntr++)
  {
    checksum = checksum + read_packet[cntr];
  }
  read_packet[7] = checksum;
  for (int cntr = 0 ; cntr < send_cntr; cntr++)
  {
    Serial.write(read_packet[cntr]);
    //Serial1.write(read_packet[cntr]);
  }

  if (rts)
  {
    Blynk_Delay(10);
    digitalWrite(rts_pin, LOW);
  }

  Blynk_Delay(500);

  read_and_chk_packet();


  if (msg_ok)
  {

    if (led2.getValue())
    {
      led2.off();
    }
    else
    {
      led2.on();
    }
  }

}

void loop()
{
  Blynk.run();
  timer1.run();
  if (autorun)
  {
    timer2.run();
    timer3.run();
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
