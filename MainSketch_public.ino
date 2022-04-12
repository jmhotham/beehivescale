#include <HX711_ADC.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>

#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define Y_POS_0 0
#define X_POS_0 0
#define Y_POS_10 10
#define X_POS_10 10
#define TEXT_SIZE_SMALL 1
#define TEXT_SIZE_MED 2
#define TEXT_SIZE_LARGE 3
#define LONG_DELAY 4000
#define MED_DELAY 2000
#define SHORT_DELAY 1000

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//pins:
//OLED USES I2C -> ON ARDUINO UNO: SCL @ A5, SDA @ A4
const int HX711_dout = 6; //mcu > HX711 dout pin
const int HX711_sck = 7; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
const int tareOffsetVal_eepromAdress = 4;
unsigned long t = 0;

void setup() {
  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

//SETUP LOAD CELLS
  LoadCell.begin();
  //LoadCell.setReverseOutput();
  float calibrationValue; // calibration value (see example file "Calibration.ino")

#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512);
#endif

  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  //restore the zero offset value from eeprom:
  long tare_offset = 0;
  EEPROM.get(tareOffsetVal_eepromAdress, tare_offset);
  LoadCell.setTareOffset(tare_offset);

    unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  LoadCell.start(stabilizingtime, false);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }

  //SETUP OLED
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.clearDisplay();
  display.setTextSize(TEXT_SIZE_SMALL);
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(Y_POS_0,X_POS_0);             // Start at top-left corner
  //display.println("Hotham's Apiary 2022");  //customize your own start screen message
  display.println("\nInitializing");
  display.display();
  delay(LONG_DELAY);

}

void setDisplayParameters(int cursorPosY, int cursorPosX, int textSize)
{
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(cursorPosY,cursorPosX);             // Starting point of display
}

void displayWeightInKGs(float weightInGrams, int cursorPosY, int cursorPosX, int size, int delayTime)
{
  setDisplayParameters(cursorPosY, cursorPosX, size);
  float weight = weightInGrams / 1000;
  if(weight < 0)
  {
    weight = 0.00;
  }
  
  String weightInKGs = String(weight);
  display.println(weightInKGs);
  display.display();
  delay(delayTime);
}


void loop() {
  //GET VALUES FROM LOAD CELLS
  float value = 0.0;
  static boolean newDataReady = 0;
  const int serialPrintInterval = 250; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update())
  {
    newDataReady = true;
  }

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float value = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(value);
      //DISPLAY VALUE ON THE OLED
      displayWeightInKGs(value, Y_POS_10, X_POS_10, TEXT_SIZE_LARGE, SHORT_DELAY);
      newDataReady = 0;
      t = millis();
    }
  }

}
