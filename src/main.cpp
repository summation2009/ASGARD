#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "ModbusMaster.h"

// ================= TFT =================
TFT_eSPI tft = TFT_eSPI();

// ================= Modbus =================
ModbusMaster myModbus;
float Temp = 0.0;
float Humi = 0.0;

// ================= IO =================
#define Input1  34
#define Input2  35
#define Button36 36
#define Button39 39

int in1 = 0, in2 = 0, btn36 = 0, btn39 = 0;

// ================= Relay Touch =================
#define Relay13 13
bool relayState = false;
bool wasTouched = false;

#define BTN_X 120
#define BTN_Y 180
#define BTN_W 100
#define BTN_H 60

// ================= Calibration =================
#define CALIBRATION_FILE "/calibrationData"
#define CAL_DATA_SIZE 10
uint16_t calibrationData[5];
bool calibrationDone = false;

// ================= FUNCTIONS =================

void read_sensor_RS485()
{
  uint8_t result = myModbus.readHoldingRegisters(0, 4);
  if (result == myModbus.ku8MBSuccess)
  {
    Humi = myModbus.getResponseBuffer(0) / 10.0;
    Temp = myModbus.getResponseBuffer(1) / 10.0;
  }
}

void drawButton()
{
  if (relayState)
  {
    tft.fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.setCursor(BTN_X + 20, BTN_Y + 20, 2);
    tft.print("ON");
  }
  else
  {
    tft.fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setCursor(BTN_X + 20, BTN_Y + 20, 2);
    tft.print("OFF");
  }
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);
  Serial.println("Start System");

  pinMode(Input1, INPUT);
  pinMode(Input2, INPUT);
  pinMode(Button36, INPUT);
  pinMode(Button39, INPUT);

  pinMode(Relay13, OUTPUT);
  digitalWrite(Relay13, HIGH); // relay off (active LOW)

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  // ===== SPIFFS =====
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Failed");
  }

  uint8_t calDataOK = 0;

  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f)
    {
      if (f.readBytes((char *)calibrationData, CAL_DATA_SIZE) == CAL_DATA_SIZE)
        calDataOK = 1;
      f.close();
    }
  }

  if (calDataOK)
  {
    tft.setTouch(calibrationData);
    calibrationDone = true;
  }
  else
  {
    tft.fillScreen(TFT_WHITE);
    tft.calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);

    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calibrationData, CAL_DATA_SIZE);
      f.close();
    }
    calibrationDone = true;
  }

  tft.fillScreen(TFT_WHITE);
  drawButton();

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  myModbus.begin(1, Serial2);
}

// ================= LOOP =================
void loop()
{
  if (!calibrationDone) return;

  // ===== Touch Relay =====
  uint16_t x, y;
  bool touched = tft.getTouch(&x, &y);

  if (touched && !wasTouched)
  {
    Serial.printf("Touch: %d,%d\n", x, y);

    if (x > BTN_X && x < BTN_X + BTN_W &&
        y > BTN_Y && y < BTN_Y + BTN_H)
    {
      relayState = !relayState;
      digitalWrite(Relay13, relayState ? LOW : HIGH);
      drawButton();
    }
  }
  wasTouched = touched;

  // ===== Modbus =====
  read_sensor_RS485();

  // ===== IO =====
  in1  = digitalRead(Input1);
  in2  = digitalRead(Input2);
  btn36 = digitalRead(Button36);
  btn39 = digitalRead(Button39);

  // ===== Display =====
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  tft.setCursor(10, 10, 2);
  tft.printf("Temp: %.1f C   ", Temp);

  tft.setCursor(10, 30, 2);
  tft.printf("Humi: %.1f %%   ", Humi);

  tft.setCursor(10, 110, 2);
  tft.printf("IN1:  %d  IN2:  %d   ", in1, in2);

  tft.setCursor(10, 130, 2);
  tft.printf("B36:  %d  B39:  %d   ", btn36, btn39);

  delay(50);
}