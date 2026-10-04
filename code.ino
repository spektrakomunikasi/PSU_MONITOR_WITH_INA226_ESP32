#include <Wire.h>
#include <TFT_eSPI.h>
#include <INA226_WE.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <math.h>

// ===================== DEVICE =====================
TFT_eSPI tft = TFT_eSPI();
INA226_WE ina226(0x44);   // alamat INA226 dari scanner kamu

// DS18B20
#define ONE_WIRE_BUS 27
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// FAN output
#define FAN_PIN 26
bool fanState = false;

// ===================== SETTING (ubah via serial) =====================
float fanOnTemp = 40.0f;   // default ON
float fanHys    = 2.0f;    // default hysteresis -> OFF = ON - HYS

// ===================== EEPROM =====================
#define EEPROM_SIZE   64
#define EE_MAGIC_ADDR 0
#define EE_ON_ADDR    4
#define EE_HYS_ADDR   8
const uint32_t EE_MAGIC = 0xA55A3CC3;

// ===================== RANGE DISPLAY =====================
const float V_RANGE = 25.0f;  // 0-25V
const float I_RANGE = 20.0f;  // 0-20A

// ===================== LAYOUT 240x240 =====================
const int E_Y = 8;
const int I_Y = 36;

const int V_LABEL_Y = 72;
const int V_BAR_Y   = 92;

const int A_LABEL_Y = 130;
const int A_BAR_Y   = 150;

const int BAR_X = 12;
const int BAR_W = 216;
const int BAR_H = 20;

// ===================== FILTER =====================
float vFilt = 0.0f, iFilt = 0.0f;
bool filterInit = false;
const float ALPHA_V = 0.18f;
const float ALPHA_I = 0.15f;

// suhu
float tempC = NAN;
float prevTempShown = -9999.0f;

// cache tampilan
float prevV = -999.0f, prevI = -999.0f;
int prevVW = -1, prevIW = -1;

// warna
const uint16_t C_BG    = TFT_BLACK;
const uint16_t C_MAIN  = TFT_CYAN;
const uint16_t C_LABEL = TFT_RED;
const uint16_t C_FRAME = TFT_SKYBLUE;
const uint16_t C_BARV  = TFT_GREEN;
const uint16_t C_BARI  = TFT_RED;
const uint16_t C_TEMP  = TFT_MAGENTA;

// ===================== UTIL =====================
float clampf(float v, float lo, float hi){
  if(v < lo) return lo;
  if(v > hi) return hi;
  return v;
}

bool isTempValid(float t){
  return (!isnan(t) && t > -55.0f && t < 125.0f && t != 85.0f && t != -127.0f);
}

void printHelp() {
  Serial.println("\n=== COMMAND LIST ===");
  Serial.println("SET <tempC>   -> set FAN ON temp. Example: SET 45");
  Serial.println("HYS <tempC>   -> set hysteresis. Example: HYS 3");
  Serial.println("SHOW          -> show current setting");
  Serial.println("SAVE          -> save setting to EEPROM");
  Serial.println("HELP          -> command list");
}

void showSetting() {
  Serial.println("\n=== CURRENT SETTING ===");
  Serial.printf("FAN ON  : %.2f C\n", fanOnTemp);
  Serial.printf("HYS     : %.2f C\n", fanHys);
  Serial.printf("FAN OFF : %.2f C\n", fanOnTemp - fanHys);
}

void saveSettingEEPROM() {
  EEPROM.put(EE_MAGIC_ADDR, EE_MAGIC);
  EEPROM.put(EE_ON_ADDR, fanOnTemp);
  EEPROM.put(EE_HYS_ADDR, fanHys);
  EEPROM.commit();
  Serial.println("Setting saved.");
}

void loadSettingEEPROM() {
  uint32_t mg;
  EEPROM.get(EE_MAGIC_ADDR, mg);
  if (mg == EE_MAGIC) {
    EEPROM.get(EE_ON_ADDR, fanOnTemp);
    EEPROM.get(EE_HYS_ADDR, fanHys);

    // sanity check
    if (isnan(fanOnTemp) || fanOnTemp < 20.0f || fanOnTemp > 90.0f) fanOnTemp = 40.0f;
    if (isnan(fanHys)    || fanHys < 0.5f     || fanHys > 20.0f)    fanHys = 2.0f;

    Serial.println("Setting loaded from EEPROM.");
  } else {
    Serial.println("EEPROM empty -> use default.");
  }
  showSetting();
}

void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.startsWith("SET ")) {
    float v = cmd.substring(4).toFloat();
    if (v >= 20.0f && v <= 90.0f) {
      fanOnTemp = v;
      Serial.printf("OK: FAN ON = %.2f C, FAN OFF = %.2f C\n", fanOnTemp, fanOnTemp - fanHys);
    } else {
      Serial.println("Invalid SET (20..90)");
    }
  }
  else if (cmd.startsWith("HYS ")) {
    float h = cmd.substring(4).toFloat();
    if (h >= 0.5f && h <= 20.0f) {
      fanHys = h;
      Serial.printf("OK: HYS = %.2f C, FAN OFF = %.2f C\n", fanHys, fanOnTemp - fanHys);
    } else {
      Serial.println("Invalid HYS (0.5..20)");
    }
  }
  else if (cmd == "SHOW") showSetting();
  else if (cmd == "SAVE") saveSettingEEPROM();
  else if (cmd == "HELP") printHelp();
  else if (cmd.length() > 0) Serial.println("Unknown command. Type HELP");
}

void bootScreen(){
  tft.fillScreen(C_BG);
  tft.setTextColor(TFT_CYAN, C_BG);
  tft.setTextSize(2);
  tft.drawString("SPEKTRA KOMUNIKASI", 12, 95);

  tft.setTextColor(TFT_YELLOW, C_BG);
  tft.setTextSize(3);
  tft.drawString("PSU MONITOR", 28, 125);
  delay(900);
}

void drawStaticUI(){
  tft.fillScreen(C_BG);

  tft.setTextColor(C_MAIN, C_BG);
  tft.setTextSize(3);
  tft.drawString("E=", 8, E_Y);
  tft.drawString("I=", 8, I_Y);

  tft.setTextSize(2);
  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("VOLT", 12, V_LABEL_Y);
  tft.setTextColor(TFT_WHITE, C_BG);
  tft.drawString("0-25V", 88, V_LABEL_Y);

  tft.setTextColor(C_LABEL, C_BG);
  tft.drawString("AMPER", 12, A_LABEL_Y);
  tft.setTextColor(TFT_WHITE, C_BG);
  tft.drawString("0-20A", 108, A_LABEL_Y);

  tft.drawRoundRect(BAR_X, V_BAR_Y, BAR_W, BAR_H, 5, C_FRAME);
  tft.drawRoundRect(BAR_X, A_BAR_Y, BAR_W, BAR_H, 5, C_FRAME);

  // Bottom suhu (BERSIH)
  tft.setTextColor(C_TEMP, C_BG);
  tft.setTextSize(2);
  tft.drawString("SUHU:", 12, 214);
}

void drawEandI(float v, float a){
  tft.setTextColor(C_MAIN, C_BG);
  tft.setTextSize(3);

  if(fabs(v - prevV) > 0.01f){
    tft.fillRect(52, E_Y, 184, 24, C_BG);
    tft.drawString(String(v, 2) + " V", 52, E_Y);
    prevV = v;
  }

  if(fabs(a - prevI) > 0.01f){
    tft.fillRect(52, I_Y, 184, 24, C_BG);
    tft.drawString(String(a, 2) + " A", 52, I_Y);
    prevI = a;
  }
}

void drawBars(float v, float a){
  int vw = (int)(clampf(v, 0.0f, V_RANGE) / V_RANGE * (BAR_W - 2));
  int iw = (int)(clampf(a, 0.0f, I_RANGE) / I_RANGE * (BAR_W - 2));

  if(vw != prevVW){
    tft.fillRect(BAR_X + 1, V_BAR_Y + 1, BAR_W - 2, BAR_H - 2, C_BG);
    if(vw > 0) tft.fillRoundRect(BAR_X + 1, V_BAR_Y + 1, vw, BAR_H - 2, 4, C_BARV);
    prevVW = vw;
  }

  if(iw != prevIW){
    tft.fillRect(BAR_X + 1, A_BAR_Y + 1, BAR_W - 2, BAR_H - 2, C_BG);
    if(iw > 0) tft.fillRoundRect(BAR_X + 1, A_BAR_Y + 1, iw, BAR_H - 2, 4, C_BARI);
    prevIW = iw;
  }
}

void updateFanByTemp(float tC){
  if (!isTempValid(tC)) return;

  float fanOffTemp = fanOnTemp - fanHys;
  if (!fanState && tC >= fanOnTemp) {
    fanState = true;
    digitalWrite(FAN_PIN, HIGH);
  } else if (fanState && tC <= fanOffTemp) {
    fanState = false;
    digitalWrite(FAN_PIN, LOW);
  }
}

void drawTemp(float tC){
  bool valid = isTempValid(tC);

  // update kalau berubah 0.2C atau status valid berubah
  if (valid && isTempValid(prevTempShown) && fabs(tC - prevTempShown) < 0.2f) return;
  if (!valid && !isnan(prevTempShown) && prevTempShown < -9000) return; // sudah "--.-"

  tft.fillRect(82, 214, 154, 22, C_BG);
  tft.setTextColor(C_TEMP, C_BG);
  tft.setTextSize(2);

  if (valid) {
    tft.drawString(String(tC, 1) + " C", 82, 214);
    prevTempShown = tC;
  } else {
    tft.drawString("--.- C", 82, 214);
    prevTempShown = -99999.0f;
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadSettingEEPROM();
  printHelp();

  Wire.begin(21, 22);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  tft.init();
  tft.setRotation(1);

  bootScreen();
  drawStaticUI();

  if(!ina226.init()){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("INA226 NOT FOUND", 20, 110);
    while(true) { handleSerial(); delay(10); }
  }

  // shunt 10mR, max 20A
  ina226.setResistorRange(0.01, 20.0);
  ina226.setAverage(INA226_AVERAGE_16);
  ina226.setConversionTime(INA226_CONV_TIME_1100);
  ina226.waitUntilConversionCompleted();

  ds18b20.begin();
  ds18b20.setResolution(12);
}

void loop() {
  handleSerial();

  // baca INA226
  float vRaw = ina226.getBusVoltage_V();
  float iRaw = ina226.getCurrent_mA() / 1000.0f;

  if(iRaw > -0.005f && iRaw < 0.005f) iRaw = 0.0f;
  if(iRaw < 0.0f) iRaw = 0.0f;

  if(!filterInit){
    vFilt = vRaw;
    iFilt = iRaw;
    filterInit = true;
  } else {
    vFilt += ALPHA_V * (vRaw - vFilt);
    iFilt += ALPHA_I * (iRaw - iFilt);
  }

  // baca DS18B20 periodik
  static unsigned long lastTempMs = 0;
  unsigned long now = millis();
  if(now - lastTempMs > 700){
    lastTempMs = now;
    ds18b20.requestTemperatures();
    tempC = ds18b20.getTempCByIndex(0);
    updateFanByTemp(tempC);
  }

  drawEandI(vFilt, iFilt);
  drawBars(vFilt, iFilt);
  drawTemp(tempC);

  delay(120);
}
