/*
   IAQ_Monitor
   Autor: Sebastian Balzer
   Date: 2021-10-17

   Hardware: Teensy 4.0, ILI9341 320x240 TFT with XPT2046 Touch, BME680
*/

#include <XPT2046_Touchscreen.h>
#include "SPI.h"
#include "ILI9341_t3n.h"
#include <ili9341_t3n_font_Arial.h>
#include <ili9341_t3n_font_ArialBold.h>
#include "BUI.h" // [important] include BUI after TFT and touch libraries
#include "bsec.h"
#include "images.c"

//Touchscreen config
#define CS_PIN  8
XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  2

//TFT config
#define TFT_DC       9
#define TFT_CS      10
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

Bsec iaqSensor;
int IaqSensorStatus = 0;
String output;
float tempOffset = 0.9f; // Subtracts the value from the displayed temperature

// Graphical user interface object
BUI ui = BUI(&tft, &ts);

/*
   MainScreenControls
*/
// Value labels
Label lblIAQvalue = Label(180, 1, (char *)"NA", Arial_48);
Label lblIAQlevel = Label(220, 29, (char *)"NA", Arial_20);
Label lblIAQtext = Label(0, 64, (char *)"NA", Arial_8);
Label lblCO2value = Label(90, 110, (char *)"NA", Arial_32);
Label lblVOCvalue = Label(260, 110, (char *)"NA", Arial_32);
Label lblTEMPvalue = Label(90, 165, (char *)"NA", Arial_32);
Label lblRHvalue = Label(260, 165, (char *)"NA", Arial_32);
// Value type labels
Label lblIAQ = Label(190, 1, (char *)"IAQ", Arial_20);
Label lblCO2 = Label(100, 110, (char *)"CO2", Arial_14);
Label lblVOC = Label(270, 110, (char *)"VOC", Arial_14);
Label lblTEMP = Label(100, 165, (char *)"TEMP", Arial_14);
Label lblRH = Label(270, 165, (char *)"RH", Arial_14);
// Value unit labels
Label lblIAQlvl = Label(190, 29, (char *)"lvl", Arial_20);
Label lblCO2ppm = Label(100, 128, (char *)"ppm", Arial_14);
Label lblVOCppm = Label(270, 128, (char *)"ppm", Arial_14);
Label lblTEMPc = Label(100, 183, (char *)"*C", Arial_14);
Label lblRHp = Label(270, 183, (char *)"%", Arial_14);
// Buttons
Button btnMore = Button(0, 210, 100, 30, (char *)"More", &btnMore_onClickHandler);
Button btnStatus = Button(110, 210, 100, 30, (char *)"Status", &btnStatus_onClickHandler);
Button btnSettings = Button(220, 210, 100, 30, (char *)"Settings", &btnSettings_onClickHandler);
//Images
Image imgWarn = Image(264, 1, 48, 48, warn);

/*
   MoreScreenControls
*/
// Value type labels
Label lblPressure = Label(10, 10, (char *)"Pressure [hPa]:", Arial_14);
Label lblAltitude = Label(10, 35, (char *)"Altitude [m]:", Arial_14);
Label lblAH = Label(10, 60, (char *)"Absolute humidity [g/cbm]:", Arial_14);
Label lblDewpoint = Label(10, 85, (char *)"Dewpoint [*C]:", Arial_14);
Label lblAirDensity = Label(10, 110, (char *)"Air density [kg/cbm]:", Arial_14);
// Value labels
Label lblPressureVal = Label(319, 10, (char *)"0", Arial_14); //143
Label lblAltitudeVal = Label(319, 35, (char *)"0", Arial_14); //112
Label lblAHVal = Label(319, 60, (char *)"0", Arial_14); //235
Label lblDewpointVal = Label(319, 85, (char *)"0", Arial_14); //133
Label lblAirDensityVal = Label(319, 110, (char *)"0", Arial_14); //185

/*
   StatusScreenControls
*/
Label lblIaqAccuracy = Label(0, 10, (char *)"Iaq Accuracy = ", Arial_14);
Label lblIaqAccuracyValue = Label(130, 10, (char *)"0", Arial_14);
Label lblIaqAccuracyText = Label(0, 35, (char *)"", Arial_14);
Label lblStatusText = Label(0, 160, (char *)"", Arial_14);

/*
   SettingsScreenControls
*/
NumericUpDown numTempOffset = NumericUpDown(10, 10, 100, 40, -10, 10, &numTempOffset_onClickHandler);
Label lblTempOffset = Label(115, 23, (char *)"Temp. offset", Arial_14);
CheckBox cbDarkMode = CheckBox(10, 70, (char *)"Enable darkmode", &cbDarkMode_onClickHandler);
CheckBox cbFahrenheit = CheckBox(10, 100, (char *)"Temperature in Fahrenheit", &cbFahrenheit_onClickHandler);
Button btnCalTouch = Button(10, 130, 140, 30, (char *)"Calibrate touch", &btnCalTouch_onClickHandler);

/*
   OmniControls
*/
Button btnMain = Button(0, 210, 100, 30, (char *)"Main", &btnMain_onClickHandler);

void setup() {
  // Init serial and wire
  Serial.begin(9600);
  // Uncomment these to use alternate wire pins (default = 19, 18)
  // Wire.setSCL(16);
  // Wire.setSDA(17);
  Wire.begin();

  // Init BME680
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  checkIaqSensorStatus();

  if (iaqSensor.status == BSEC_OK) {
    bsec_virtual_sensor_t sensorList[10] = {
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    };
    //set a temperature offset to BME680
    iaqSensor.setTemperatureOffset(tempOffset); //can be changed in SettingsScreen

    iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
    checkIaqSensorStatus();
  }

  // Init TFT (tft.begin(); is executed in the initialization of ui)
  tft.setRotation(1); // Landscape format, TFT pins on the right
  ts.setRotation(1);

  // Set touchscreen calibration in ui for my ILI9341
  ui.TS_MINX = 285;
  ui.TS_MINY = 280;
  ui.TS_MAXX = 3700;
  ui.TS_MAXY = 3780;

  // Enable darkmode at startup
  ui.enableDarkmode(true);

  // Setup some controls
  cbDarkMode.checked = true;
  numTempOffset.decimalPlaces = 1;
  numTempOffset.setValue(tempOffset);
  btnSettings.setImage(26, 26, sett);
  btnStatus.setImage(26, 26, info);
  btnMore.setImage(26, 26, more);
  lblIAQvalue.textAlign = ALIGNRIGHT;
  lblCO2value.textAlign = ALIGNRIGHT;
  lblVOCvalue.textAlign = ALIGNRIGHT;
  lblTEMPvalue.textAlign = ALIGNRIGHT;
  lblRHvalue.textAlign = ALIGNRIGHT;
  lblPressureVal.textAlign = ALIGNRIGHT;
  lblAltitudeVal.textAlign = ALIGNRIGHT;
  lblAHVal.textAlign = ALIGNRIGHT;
  lblDewpointVal.textAlign = ALIGNRIGHT;
  lblAirDensityVal.textAlign = ALIGNRIGHT;

  //start program
  getMainScreen();
}

void loop(void) {
  if (iaqSensor.run()) { // If new data is available
    lblIAQvalue.setText(iaqSensor.iaq);
    lblIAQlevel.setText(getIAQlevel(iaqSensor.iaq));
    lblIAQtext.setText(getIAQtext(iaqSensor.iaq));
    lblCO2value.setText(iaqSensor.co2Equivalent);
    lblVOCvalue.setText(iaqSensor.breathVocEquivalent, 4, 1);
    if (cbFahrenheit.checked) {
      lblTEMPvalue.setText(calcF(iaqSensor.temperature), 4, 1);
      lblDewpointVal.setText(calcF(getDp100(iaqSensor.temperature, iaqSensor.humidity)), 5, 2);
    } else {
      lblTEMPvalue.setText(iaqSensor.temperature, 4, 1);
      lblDewpointVal.setText(getDp100(iaqSensor.temperature, iaqSensor.humidity), 5, 2);
    }
    lblRHvalue.setText(iaqSensor.humidity, 5, 1);

    lblPressureVal.setText(iaqSensor.pressure / 100.0f, 7, 2);
    lblAltitudeVal.setText(getAltitude(iaqSensor.pressure), 7, 2);
    lblAHVal.setText(getAH(iaqSensor.temperature, iaqSensor.humidity), 5, 2);
    lblAirDensityVal.setText(getAirDensity(iaqSensor.temperature, iaqSensor.humidity, iaqSensor.pressure), 4, 2);
    
    lblIaqAccuracyValue.setText(iaqSensor.iaqAccuracy);
    lblIaqAccuracyText.setText(getIaqAccuracyText(iaqSensor.iaqAccuracy));
    lblStatusText.setText(output);

    lblIAQvalue.foreColor = getIAQcolor(iaqSensor.iaq);
    lblCO2value.foreColor = getCO2color(iaqSensor.co2Equivalent);
    lblVOCvalue.foreColor = getVOCcolor(iaqSensor.breathVocEquivalent);
    lblTEMPvalue.foreColor = getTEMPcolor(iaqSensor.temperature);
    lblRHvalue.foreColor = getRHcolor(iaqSensor.humidity);

    if(iaqSensor.iaq > 150) imgWarn.visible = true;
    else imgWarn.visible = false;
  } else {
    checkIaqSensorStatus();
  }

  ui.update(); //needed for ui (events and draws)
}

/*
   Helper functions
*/
void checkIaqSensorStatus(void) {
  IaqSensorStatus = iaqSensor.status;

  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  } else if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME68x error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    } else {
      output = "BME68x warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  } else {
    output = "BSEC and BME68x OK";
    Serial.println(output);
  }
}

uint16_t getIAQcolor(float iaq) {
  if (iaq <= 50) return 0x07E0; //GREEN
  if (iaq <= 100) return 0xFFE0; //YELLOW
  if (iaq <= 150) return 0xFC00; //ORANGE
  if (iaq <= 200) return 0xF800; //RED
  if (iaq <= 300) return 0xF81F; //PURPLE
  if (iaq > 300) return 0x8000; //MAROON
  else return 0xFFFF; //WHITE
}

uint8_t getIAQlevel(float iaq) {
  if (iaq <= 50) return 1; //GREEN
  if (iaq <= 100) return 2; //YELLOW
  if (iaq <= 150) return 3; //ORANGE
  if (iaq <= 200) return 4; //RED
  if (iaq <= 300) return 5; //PURPLE
  if (iaq > 300) return 6; //MAROON
  else return 0; //WHITE
}

uint16_t getCO2color(float co2) {
  if (co2 <= 1000) return 0x07E0; //GREEN
  if (co2 <= 1500) return 0xFFE0; //YELLOW
  if (co2 <= 1900) return 0xFC00; //ORANGE
  else return 0xF800; //RED
}

uint16_t getVOCcolor(float voc) {
  if (voc < 15) return 0x07E0; //GREEN
  if (voc < 25) return 0xFFE0; //YELLOW
  else return 0xF800; //RED
}

uint16_t getTEMPcolor(float temp) {
  if (temp >= 19 && temp <= 23) return 0x07E0; //GREEN
  if (temp >= 17 && temp <= 25) return 0xFFE0; //YELLOW
  else return 0xF800; //RED
}

uint16_t getRHcolor(float rh) {
  if (rh >= 37 && rh <= 70) return 0x07E0; //GREEN
  if (rh >= 20 && rh <= 80) return 0xFFE0; //YELLOW
  else return 0xF800; //RED
}

char* getIAQtext(float iaq) {
  if (iaq <= 50) return (char*)"Air quality is satisfactory, and air pollution poses little or no risk.";
  if (iaq <= 100) return (char*)"Air quality is acceptable. However, there may be a risk for some people, particularly those who are unusually sensitive to air pollution.";
  if (iaq <= 150) return (char*)"Members of sensitive groups may experience health effects. The general public is less likely to be affected.";
  if (iaq <= 200) return (char*)"Some members of the general public may experience health effects; members of sensitive groups may experience more serious health effects.";
  if (iaq <= 300) return (char*)"Health alert: The risk of health effects is increased for everyone.";
  if (iaq > 300) return (char*)"Health warning of emergency conditions: everyone is more likely to be affected.";
  else return (char*)"Invalid IAQ";
}

char* getIaqAccuracyText(int iav) {
  if (iav == 0) return (char*)"Stabilization / run-in ongoing";
  if (iav == 1) return (char*)"Low accuracy,to reach high accuracy(3),please expose sensor once to good air (e.g. outdoor air) and bad air (e.g. box with exhaled breath) for auto-trimming";
  if (iav == 2) return (char*)"Medium accuracy: auto-trimming ongoing";
  if (iav == 3) return (char*)"High accuracy";
  else return (char*)"Invalid value";
}

float calcF(float C) {
  return C * 1.8f + 32.0f;
}

float getAltitude(float p) {
  return 44330.0f * (1.0f - pow((p / 100.0f) / 1013.25f, 0.1903f));
}

float getDp100(float t, float r) {
  float a = 17.269f;
  float b = 237.3f;
  float z1 = (a * t) / (b + t);

  return b * log(6.105f * exp(z1) / 100.0f * r / 6.105f) / (a - log(6.105f * exp(z1) / 100.0f * r / 6.105f));
}

float getAH(float t, float r) {
  float tK = t + 273.15f; //Temperatur (K)
  float psT = exp((-6094.4642f * pow(tK, -1)) + 21.1249952f - (2.7245552f * pow(10, -2) * tK) + (1.6853396f * pow(10, -5) * pow(tK, 2)) + (2.4575506f * log(tK))) / 100.0f; //Sättigungsdampfdruck von Wasserdampf (Pa)
  float pD = ((r / 100.0f) * psT); //Partialdruck Wasserdampf (Pa)

  return (((pD * 100.0f) / (461.51f * tK))) * 1000.0f; //Absolute Luftfeuchtigkeit (g Wasserdampf / m³ Luftvolumen)
}

float getAirDensity(float t, float r, float p) {
  //https://www.schweizer-fn.de/lueftung/feuchte/feuchte.php
  float eSat = 611.2f * exp((17.62f * t) / (243.12f + t)); //Sättigungsdampfdruck über Wasser (Pa)
  float Rs = 287.058f; //Gaskonstante trockene Luft (J/(kg*K)
  float Rd = 461.51f; //Gaskonstante Wasserdampf (J/(kg*K)
  float T = t + 273.15f; //Temperatur (K)
  float Rf = Rs / (1 - ((r / 100.0f) * eSat / p) * (1 - Rs / Rd)); // Gaskonstante feuchte Luft (J/(kg*K)

  return p / (Rf * T); // Dichte feuchter Luft (kg/m³)
}

/*
   ui screens
*/
void getMainScreen() {
  //init before adding controls to a new window
  ui.init();

  //Add labels to ui
  ui.addControl(&lblIAQvalue);
  ui.addControl(&lblIAQlvl);
  ui.addControl(&lblIAQlevel);
  ui.addControl(&lblIAQtext);
  ui.addControl(&lblCO2value);
  ui.addControl(&lblVOCvalue);
  ui.addControl(&lblTEMPvalue);
  ui.addControl(&lblRHvalue);
  ui.addControl(&lblIAQ);
  ui.addControl(&lblCO2);
  ui.addControl(&lblVOC);
  ui.addControl(&lblTEMP);
  ui.addControl(&lblRH);
  ui.addControl(&lblCO2ppm);
  ui.addControl(&lblVOCppm);
  ui.addControl(&lblTEMPc);
  ui.addControl(&lblRHp);

  //Add buttons to ui
  ui.addControl(&btnMore);
  ui.addControl(&btnStatus);
  ui.addControl(&btnSettings);

  ui.addControl(&imgWarn);
  imgWarn.visible = false;
}

void getMoreScreen() {
  //init before adding controls to a new window
  ui.init();

  ui.addControl(&lblPressure);
  ui.addControl(&lblAltitude);
  ui.addControl(&lblAH);
  ui.addControl(&lblDewpoint);
  ui.addControl(&lblAirDensity);
  ui.addControl(&lblPressureVal);
  ui.addControl(&lblAltitudeVal);
  ui.addControl(&lblAHVal);
  ui.addControl(&lblDewpointVal);
  ui.addControl(&lblAirDensityVal);

  ui.addControl(&btnMain);

  //Colorize labels after adding them to ui
  lblPressureVal.foreColor = 0x07E0; //GREEN
  lblAltitudeVal.foreColor = 0x07E0; //GREEN
  lblAHVal.foreColor = 0x07E0; //GREEN
  lblDewpointVal.foreColor = 0x07E0; //GREEN
  lblAirDensityVal.foreColor = 0x07E0; //GREEN
}

void getStatusScreen() {
  //init before adding controls to a new window
  ui.init();

  //Add controls to ui
  ui.addControl(&lblIaqAccuracy);
  ui.addControl(&lblIaqAccuracyValue);
  ui.addControl(&lblIaqAccuracyText);
  ui.addControl(&lblStatusText);
  ui.addControl(&btnMain);

  //Colorize labels after adding them to ui
  lblIaqAccuracyValue.foreColor = 0xFFE0; //YELLOW
  lblIaqAccuracyText.foreColor = 0xFFE0; //YELLOW
  lblStatusText.foreColor = 0xFFE0; //YELLOW
}

void getSettingsScreen() {
  //init before adding controls to a new window
  ui.init();

  //Add controls to ui
  ui.addControl(&numTempOffset);
  ui.addControl(&lblTempOffset);
  ui.addControl(&cbDarkMode);
  ui.addControl(&cbFahrenheit);
  ui.addControl(&btnCalTouch);
  ui.addControl(&btnMain);
}

/*
   click handlers
*/
void btnMore_onClickHandler() {
  getMoreScreen();
}

void btnStatus_onClickHandler() {
  getStatusScreen();
}

void btnSettings_onClickHandler() {
  getSettingsScreen();
}

void btnMain_onClickHandler() {
  getMainScreen();
}

void numTempOffset_onClickHandler() {
  iaqSensor.setTemperatureOffset(numTempOffset.getValue());
}

void cbDarkMode_onClickHandler() {
  ui.enableDarkmode(cbDarkMode.checked);
}

void cbFahrenheit_onClickHandler() {
  if (cbFahrenheit.checked) {
    lblTEMPc.setText((char*)"*F");
    lblDewpoint.setText((char *)"Dewpoint [*F]:");
  } else {
    lblTEMPc.setText((char*)"*C");
    lblDewpoint.setText((char *)"Dewpoint [*C]:");
  }
}

void btnCalTouch_onClickHandler() {
  ui.calibrateTouch();
  getSettingsScreen();
}
