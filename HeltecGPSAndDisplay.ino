/*
  This is a simple example show the GPS recived data in OLED.
  
  The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connection:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16

  The diplayed pages are switched using the GPIO 2 

  The GPS connections are like this:
  GPS VCC -- 3V3
  GPS GND -- GND
  GPS RX  -- GPIO17 
  GPS TX  -- GPIO18
  GPS PPS -- NC
  
*/

#include "heltec.h" 
#include <TinyGPS++.h>

/////////
// for the TinyGPS++ object
////////////
TinyGPSPlus gps;

/////////
// for the displayed pages managment
////////////
int pages = 0;
const int maxpages = 4;
const int timePerPages = 5;


/////////
// for the sweeping button
////////////
int digitalSweep = 2;


/////////
// For the battery managment
/////////
int analogPin = 13;              // Wifi Kit 32 shows analog value of voltage in A4
int batteryVal = 0;                     // variable to store the value read

//******************************** needed for running average (smoothening the analog value)

//float XS = 2.25;      //The returned reading is multiplied by this XS to get the battery voltage in mV.
float XS = 2.0295;      //The returned reading is multiplied by this XS to get the battery voltage in mV.
#define Fbattery    3700  //The default battery is 3700mv when the battery is fully charged.
#define maxBattery    4200  //The default battery is 3700mv when the battery is fully charged.
#define minBattery    Fbattery/1.15  //The default battery is 3700mv when the battery is fully charged.
const int numReadings = 32;     // the higher the value, the smoother the average
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
//char buf[128];

/*///////////////////////////*/

void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, false /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, false /*PABOOST Enable*/, 868E6 /*long BAND*/);
 
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 0, "Don't push me");
  Heltec.display->drawString(0, 16, "I am thinking ...");
  Heltec.display->display();
  
  
  // The GPS is connected to this serial interface
  // Serial2.begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert)
  // The txPin & rxPin can set to any output pin
  Serial2.begin(9600, SERIAL_8N1, 18, 17);
  

  Serial.begin(115200);
  Serial.print(F("TinyGPS++ library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();
  delay(1500);
  Heltec.display->clear();

  ///  Smoothening init the battery reading
  for (int thisReading = 0; thisReading < numReadings; thisReading++) //used for smoothening
  {
    readings[thisReading] = 0;
  }
  adcAttachPin(analogPin);
  analogSetClockDiv(255); // 1338mS
}

void batteryReading ();

void loop()
{
  // red the battery value
  batteryReading ();

  // This sketch displays information every time a new sentence is correctly encoded.
  while (Serial2.available() > 0)
    if (gps.encode(Serial2.read()))
      displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 0, "No GPS detected:");
    Heltec.display->drawString(0, 24, "check wiring.");
    Heltec.display->display();
    delay(10000);
  }

  //test if the tuch button is pushed during more than 100ms
  if (touchRead(digitalSweep)<45)
  {
    delay(100);
    if (touchRead(digitalSweep)<45)//test si bouton appuyé
    {
      if (++pages>=maxpages) 
        pages = 0;
    }
  }
  displayIt();
  delay(150);
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

void batteryReading ()
{
  total = total - readings[readIndex];          // subtract the last reading
  readings[readIndex] = analogRead(analogPin);  // read from the sensor
  total = total + readings[readIndex];          // add the reading to the total
  readIndex = readIndex + 1;                    // advance to the next position in the array
  if (readIndex >= numReadings)                 // if we're at the end of the array...
  {  
    readIndex = 0;                              // ...wrap around to the beginning
  }
  batteryVal = total / numReadings;                    // calculate the average

//  if (batteryVal>=320){batteryVal=320;}                       // 3.7v 500 mAh Batt - estimated max at 320, cut at 320

//  batteryVal = (batteryVal-100)/2.2;                          // calculate percentage  
}


// this function display an horizontal battery symbol filled as the battery power
void filledBatteryH(int16_t vBat, int16_t x=0, int16_t y=10) {
  int16_t h=15;int16_t l=30;
  if (vBat>=100){vBat=100;}
  Heltec.display->setColor(WHITE);
  Heltec.display->fillRect(x, y, l-2, h);
  Heltec.display->fillRect(x, y+h/3, l, h/3);
  Heltec.display->setColor(BLACK);
  Heltec.display->fillRect(x+1, y+1, l-4, h-2);
  Heltec.display->setColor(WHITE);
  int16_t baragraph= (l-6)*vBat/100;
  Heltec.display->fillRect(x+2, y+2, baragraph, h-4);
}

// this function display a vertical battery symbol filled as the battery power
void filledBatteryV(int16_t vBatPercentage, int16_t x=0) {
  int16_t y=0;
  int16_t h=64;int16_t l=15;
  if (vBatPercentage>=100){vBatPercentage=100;}
  Heltec.display->setColor(WHITE);
  Heltec.display->fillRect(x, y+2, l, h-2);
  Heltec.display->fillRect(x+l/3, y, l/3, h);
  Heltec.display->setColor(BLACK);
  Heltec.display->fillRect(x+1, y+3, l-2, h-4);
  Heltec.display->setColor(WHITE);
  int16_t baragraph= (h-6)*vBatPercentage/100;
  Heltec.display->fillRect(x+2, y+4+(h-6-baragraph),  l-4, baragraph);
  Heltec.display->display();
}

// weird function to get standardized min, sec, month, etc. display
String format2Digit (int8_t toConvert)
{
  return (toConvert > 9 ? String(toConvert) : ("0" + String(toConvert)));
}

// display managment as state machine with several pages
void displayIt(){
  uint8_t batPercentage;
  Heltec.display->clear();
  delay(100);
  switch (pages)
  {
    case 0:
      int x,y;
      Heltec.display->setFont(ArialMT_Plain_16);
      Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
      Heltec.display->drawString(128, 0,  String(gps.satellites.value()));
      Heltec.display->setFont(ArialMT_Plain_10);
      Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
      y = 0;
      if (gps.date.isValid())
      {
        Heltec.display->drawString(64, y,  String(gps.date.year()) + "/" + 
        (gps.date.month() > 9 ? String(gps.date.month()) : ("0" + String(gps.date.month()))) + "/" + 
        (gps.date.day() > 9 ? String(gps.date.day()) : ("0" + String(gps.date.day())))); 
      }
      else
      {
        Heltec.display->drawString(64, y, "INVALID DATE");  
      }
      y += 10;
      if (gps.time.isValid())
      {
        Heltec.display->drawString(64, y,  format2Digit(gps.time.hour()) + ":" + format2Digit(gps.time.minute()) + ":" + format2Digit(gps.time.second()) + "." + format2Digit(gps.time.centisecond()));
      }
      else
      {
        Heltec.display->drawString(64, y, "INVALID TIME");  
      }
      y += 10;
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->setFont(ArialMT_Plain_10);
      if (gps.location.isValid())
      {
        Heltec.display->drawString(30, y, "LAT: " + String(gps.location.lat(),6));
        Heltec.display->drawString(30, y + 10, "LNG: " + String(gps.location.lng(),6));
        Serial.print(F(","));
        Serial.print(gps.location.lng(), 6);
      }
      else
      {
        Heltec.display->drawString(30, y, "LAT: INVALID");
        Heltec.display->drawString(30, y + 10, "LNG: INVALID");
      }
      y += 20;
      Heltec.display->drawString(30, y, "Alt: " + String(gps.altitude.meters()) + "m");
      y += 10;
      batPercentage = (batteryVal*XS-minBattery)*100/(maxBattery-minBattery);
      if (batPercentage>100){batPercentage=100;}
      Heltec.display->drawString(30, y,"Bat: " + String(batteryVal*XS,0) + "mV");  //Display the battery value in mV
      filledBatteryV (batPercentage);
      
      break;
      
    case 1:
      Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
      Heltec.display->setFont(ArialMT_Plain_16);
      Heltec.display->drawString(64, 0, "Altitude:");  
      Heltec.display->setFont(ArialMT_Plain_24);
      Heltec.display->drawString(64, 33, String(gps.altitude.meters()) + "m");
      break;
      
    case 2:
      Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
      Heltec.display->setFont(ArialMT_Plain_16);
      Heltec.display->drawString(64, 0, "Vitesse:");  
      Heltec.display->drawString(64, 16, "(km/h)");  
      Heltec.display->setFont(ArialMT_Plain_24);
      Heltec.display->drawString(64, 36, String(gps.speed.kmph()));
      break;
      
    case 3:
      batPercentage = (batteryVal*XS-minBattery)*100/(maxBattery-minBattery);
      if (batPercentage>100){batPercentage=100;}
      Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
      Heltec.display->setFont(ArialMT_Plain_10);
      Heltec.display->drawString(64, 0, "Battery: " + String(batPercentage) + "%");  //Display the battery value in %
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->setFont(ArialMT_Plain_24);
      Heltec.display->drawString(35, 35, String(batteryVal*XS,0) + "mV");  //Display the battery value in mV
      //filledBatteryV (100);
      filledBatteryV (batPercentage);
      break;

      
    default:
      break;
  }
  Heltec.display->display();
}
