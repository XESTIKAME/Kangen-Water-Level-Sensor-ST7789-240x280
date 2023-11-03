#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include "Orbitron_Medium_20.h"

#include "freefonts.h"
#include "fonts.h"
#include "colors.h"
#include "images.h"
#include "bg.h"
#include "emptyfull.h"

TFT_eSPI tft = TFT_eSPI();

int wsnsPin1 = 13;                                      // PINS FOR WATER SENSOR 1 TO 6
int wsnsPin2 = 12;
int wsnsPin3 = 14;
int wsnsPin4 = 27;
int wsnsPin5 = 26;
int wsnsPin6 = 25;
int BUZPIN = 21;                                        // BUZZER PIN

bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 1500;              // BUZZZER DURATION
const unsigned long buzzerInterval = 3000;

enum BuzzerState {
  BUZZER_OFF,
  BUZZER_ON
};

BuzzerState currentBuzzerState = BUZZER_OFF;

const char* ssid = "Your SSID here";                      // WIFI SSID
const char* password = "wifi password";                   // WIFI PASSWORD
String town = "Your city here";                           // CITY
String Country = "Your Country Here";                     // COUNTRY
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=" + town + "," + Country + "&units=imperial&APPID=";
const String apiKey = "Your OpenWeather API here";         // WEATHER API KEY

const char* ntpServer = "us.pool.ntp.org";                    // TIME SERVER                   
const long gmtOffset_sec = 0;                                 
const int daylightOffset_sec = -25200;                       

unsigned long lastTextChangeTime = 0;                         // SETTINGS FOR HOW OFTEN TO UPDATE THE TIME
const unsigned long textChangeInterval = 1000;
unsigned long lastTimeUpdate = 0;
const unsigned long updateTimeInterval = 1000;                // 1 SECOND INTERVAL
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 60000;            // 1 MINUTE INTERVAL SETTINGS FOR HOW OFTEN TO UPDATE THE WEATHER

void updateTimeCallback(void* arg) {                          // FUNCTION TO UPDATE TIME
  printLocalTime();
}

// Timer handle
esp_timer_handle_t updateTimeTimer;

bool screenNeedsUpdate = true;

void setup() {
  pinMode(BUZPIN, OUTPUT);                                    // BUZPIN PIN SETTINGS
  pinMode(wsnsPin1, INPUT_PULLUP);                            // WATER SENSORS 1 TO 6 PIN SETTINGS
  pinMode(wsnsPin2, INPUT_PULLUP);
  pinMode(wsnsPin3, INPUT_PULLUP);
  pinMode(wsnsPin4, INPUT_PULLUP);
  pinMode(wsnsPin5, INPUT_PULLUP);
  pinMode(wsnsPin6, INPUT_PULLUP);

  Serial.begin(9600);
  WiFi.begin(ssid, password);                                 // BEGIN WIFI

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  tft.init();                                                 // STARTS THE DISPLAY
  tft.setRotation(0);                                         // SETS SPECIFIC MODE, 2 PORTRAIT AND 2 LANDSCAPE
  tft.fillScreen(BLACK);                                      // FILLS THE SCREEN WITH BLACK COLOR
  tft.setSwapBytes(true);                                     // SETTING OFTEN NEEDED FOR IMAGES TO DISPLAY PROPERLY FOR ESP32
  tft.drawRoundRect(0, 0, 240, 280, 40, GREENYELLOW);         // DRAWS THE BORDER
  tft.setTextColor(GOLD, BLACK);                              // SETS THE TEXT COLOR
  tft.setTextSize(2);                                         // SETS THE TEXT SIZE
  tft.setFreeFont(&Orbitron_Medium_20);                       // SETS THE SPECIFIC FONT
  tft.setCursor(13, 50);                                      // SETS THE CURSOR TO A SPECIFIC X, Y LOCATION
  tft.println("STATION");                                     // PRINTS "STATION" TO THE DISPLAY
  tft.setCursor(85, 90);                                      // SETS THE CURSOR TO A SPECIFIC X, Y LOCATION
  tft.setTextColor(GREENYELLOW);                              // SETS THE TEXT COLOR
  tft.println("3D");                                          // PRINTS "3D" TO THE DISPLAY

  tft.setTextSize(1);                                         // SETS THE TEXT SIZE
  tft.setTextColor(WHITE, BLACK);                             // SETS THE TEXT COLOR
  tft.setCursor(10, 170);                                     // SETS THE CURSOR TO DESIRED X, Y POSITION
  tft.println("Connecting to ");                              // PRINTS "CONNECTING TO " ON THE DISPLAY
  tft.setTextColor(CYAN, BLACK);                              // SETS THE TEXT COLOR
  tft.setCursor(10, 190);                                     // SETS THE TEXT CURSOR TO DESIRED X, Y POSITION
  tft.println(ssid);                                          // PRINTS YOUR NETWORK NAME (SSID) ON THE DISPLAY

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    tft.print(".");
  }

  tft.setCursor(10, 190);                                     // SETS THE CURSOR TO DESIRED X, Y POSITION
  tft.println("");                                            
  tft.setTextColor(WHITE, BLACK);                             // SETS THE TEXT COLOR
  tft.setCursor(10, 220);                                     // SETS THE CURSOR TO DESIRED X, Y POSITION
  tft.println("WiFi connected!");                             // PRINTS "WiFi connected!" ON THE DISPLAY
  tft.setTextColor(CYAN, BLACK);                              // SETS THE TEXT COLOR
  tft.setCursor(10, 240);                                     // SETS THE CURSOR TO DESIRED X, Y POSITION
  tft.println("IP address: ");                                // PRINTS "IP address:" ON THE DISPLAY
  tft.setTextColor(GOLD, BLACK);                              // SETS THE TEXT COLOR
  tft.setCursor(32, 260);                                     // SETS THE CURSOR TO DESIRED X, Y POSITOIN
  tft.println(WiFi.localIP());                                // PRINTS YOUR IP ADDRESS ON THE DISPLAY
  delay(3000);                                                // DELAY 3 SECONDS
  tft.setTextColor(GOLD, BLACK);                              // SETS THE TEXT COLOR
  tft.setTextSize(1);                                         // SETS THE TEXT SIZE
  tft.fillScreen(BLACK);                                      // SETS THE TEXT COLOR

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);           // CONFIGURES NTP TIME CONFIGURATION
  lastTimeUpdate = millis();                                          // TIME COUNTER UNTIL UPDATE
  lastWeatherUpdate = millis();                                       // WEATHER COUNTER UNTIL UPDATE

  tft.pushImage(0, 0, 240, 280, bg);                                  // PRINTS THE BACKGROUND IMAGE ON THE DISPLAY

  tft.fillRectHGradient(0, 250, 48, 30, GREEN, GREENYELLOW);              // FILLS THE FIRST WATER METER COLOR
  tft.fillRectHGradient(48, 250, 48, 30, GREENYELLOW, YELLOW);            // FILLS THE SECOND WATER METER
  tft.fillRectHGradient(96, 250, 48, 30, YELLOW, ORANGE1);                // FILLS THE THIRD WATER METER
  tft.fillRectHGradient(144, 250, 48, 30, ORANGE1, RED);                  // FILLS THE FOURTH WATER METER
  tft.fillRect(192, 250, 48, 30, RED);                                    // FILLS THE FIFTH WATER METER

  printLocalTime();                                             
  printWeatherValues();

  }

  void centerText(String text, int y, int leftBound, int rightBound) {      // CALCULATES THE X POSITION FOR CENTERING TEXT
  int x = leftBound + (rightBound - leftBound - text.length() * 12) / 2;
  tft.setCursor(x, y);
  tft.println(text);
  }

String formatTime(struct tm* timeinfo) {                                    // DEFINES formatTime(); FUNCTION BEFORE printLocalTime();
  char timeString[9];
  strftime(timeString, sizeof(timeString), "%H:%M", timeinfo);
  return String(timeString);
}

String formatDate(struct tm* timeinfo) {                                    // DEFINES formatDate(); FUNCTION BEFORE printLocalTime();
  char dateString[12];                                                      // ADJUSTS THE BUFFER SIZE IF NEEDED
  strftime(dateString, sizeof(dateString), "%b-%d", timeinfo);              // FORMAT DATE AS  "MON DD"
  return String(dateString);
}

String formatDay(struct tm* timeinfo) {                                     // DEFINES formatDay(); FUNCTION BEFORE printLocalTime();
  char dayString[12];                                                       // ADJUSTS THE BUFFER SIZE IF NEEDED
  strftime(dayString, sizeof(dayString), "%A", timeinfo);                   // FORMAT DAY OF THE WEEK (e.g., "MONDAY")
  return String(dayString);
}

void printLocalTime() {                                                     // DEFINES THE TIME SETTINGS 
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  tft.fillRoundRect(40, 10, 160, 43, 5, DARKERBLUE);                        // PRINTS TIME BACKGROUND FOR REFRESHING THE DIGITS
  tft.fillRoundRect(0, 61, 240, 27, 5, DARKERBLUE);                         // PRINTS DAY AND DATE BACKGROUND FOR REFRESHING THE LETTERS AND DIGITS

  tft.setTextColor(GREENYELLOW, DARKERBLUE);                                // SETS THE TEXT COLOR FOR THE CLOCK    
  tft.setTextSize(2);                                                       // SETS THE TEXT SIZE
  tft.setTextDatum(MC_DATUM);                                               // CENTERS THE CLOCK
  String timeString = formatTime(&timeinfo);
  int textWidth = tft.textWidth(timeString);
  int x = (240 - textWidth) / 2;                                            // CENTERS THE TEXT HORIZONTALY
  int y = 45;                                                               // Y POSITION OF CENTERED TEXT
  tft.setCursor(x, y);
  tft.print(timeString);                                                    // PRINTS THE TIME

  tft.setTextColor(ORANGE1, DARKERBLUE);                                    // SETS THE TEXT COLOR FOR THE DATE
  tft.setTextSize(1);                                                       // SETS THE TEXT SIZE
  String dateString = formatDate(&timeinfo);
  textWidth = tft.textWidth(dateString);
  x = (239 - textWidth);                                                    // MAKES THE DATE STRING END AT X 239
  y = 82;                                                                   // Y POSITION FOR TEXT BOTH X AND Y
  tft.setCursor(x, y);                                                      // SETS THE CURSOR TO THE X, Y POSITIONS SHOWN IN THE 2 LINES ABOVE
  tft.print(dateString);                                                    // PRINTS THE DATE

  tft.setTextColor(ORANGE1, GREY6);                                         // SETS THE TEXT COLOR
  tft.setTextSize(1);                                                       // SETS THE TEXT SIZE
  String dayString = formatDay(&timeinfo);
  x = 1;                                                                    // MAKES THE DAY STRING BEGIN AT X 1
  tft.setCursor(x, y);                                                      // SETS THE X, Y POSITION ACCORDING THE DATE SETTINGS
  tft.print(dayString);                                                     // PRINTS THE DAY
}


  void loop() {

  int waterSensorState1 = digitalRead(wsnsPin1);                            // SETS WATER SENSORS 1 TO 6 PINS AS DIGITAL READ
  int waterSensorState2 = digitalRead(wsnsPin2);
  int waterSensorState3 = digitalRead(wsnsPin3);
  int waterSensorState4 = digitalRead(wsnsPin4);
  int waterSensorState5 = digitalRead(wsnsPin5);
  int waterSensorState6 = digitalRead(wsnsPin6);

  unsigned long currentTime = millis();
  if (currentTime - lastTimeUpdate >= updateTimeInterval) {
    lastTimeUpdate = currentTime;                                            
    printLocalTime();                                                       // UPDATES THE TIME AND DATE ON THE DISPLAY
  }

  if (millis() - lastWeatherUpdate >= weatherUpdateInterval) {
    // It's time to update
    lastWeatherUpdate = millis();                                           // CHECKS TIME FOR WEATHER UPDATE
    printWeatherValues();                                                   // UPDATES THE WEATHER VALUES ON THE DISPLAY
  }

  if (waterSensorState1 == LOW) {                                           // IF WATER SENSOR 1 IS LOW
    screenNeedsUpdate = true;                                               // UPDATES THE DISPLAY
    tft.fillRectHGradient(0, 250, 48, 30, GREEN, GREENYELLOW);              // FILLS THE FIRST WATER METER COLOR
    tft.pushImage(132, 220, 105, 21, whale1);                               // PRINTS THE FIRST (FROM THE BOTTOM) WHALE IMAGE
  }
    else if (waterSensorState1 == HIGH) {                                   
          screenNeedsUpdate = true;
  }
  if (screenNeedsUpdate) {
    if (waterSensorState1 == HIGH) {                                        // IF WATER SENSOR 1 IS HIGH
      tft.fillRectHGradient(132, 220, 105, 21, DARKBLUE, DARKERBLUE);       // HIDES THE FIRST (FROM THE BOTTOM) WHALE IMAGE
      tft.fillRect(2, 252, 44, 26, BLACK);                                  // CLEARS THE FIRST WATER METER
    }
  if (waterSensorState2 == LOW) {                                           // IF WATER SENSOR 2 IS LOW
    tft.fillRectHGradient(48, 250, 48, 30, GREENYELLOW, YELLOW);            // FILLS THE SECOND WATER METER
    tft.pushImage(132, 199, 105, 21, whale2);                               // PRINTS THE SECOND (FROM THE BOTTOM) WHALE IMAGE
  }
  else if (waterSensorState2 == HIGH) {                                     // IF WATER SENSOR 2 IS HIGH
    tft.fillRectHGradient(132, 199, 105, 21, DARKERBLUE, DARKBLUE);         // HIDES THE SECOND (FROM THE BOTTOM) WHALE IMAGE
    tft.fillRect(50, 252, 44, 26, BLACK);                                   // CLEARS THE SECOND WATER METER
  }
  if (waterSensorState3 == LOW) {                                           // IF WATER SENSOR 3 IS LOW
    tft.fillRectHGradient(96, 250, 48, 30, YELLOW, ORANGE1);                // FILLS THE THIRD WATER METER
    tft.pushImage(132, 178, 105, 21, whale3);                               // PRINTS THE THIRD (FROM THE BOTTOM) WHALE IMAGE
  }
  else if (waterSensorState3 == HIGH) {                                     // IF WATER SENSOR 3 IS HIGH
    tft.fillRectHGradient(132, 178, 105, 21, DARKBLUE, DARKERBLUE);         // HIDES THE THIRD (FROM THE BOTTOM) WHALE IMAGE
    tft.fillRect(98, 252, 44, 26, BLACK);                                   // CLEARS THE THIRD WATER METER
  }
  if (waterSensorState4 == LOW) {                                           // IF WATER SENSOR 4 IS LOW
    tft.fillRectHGradient(144, 250, 48, 30, ORANGE1, RED);                  // FILLS THE FOURTH WATER METER
    tft.pushImage(132, 157, 105, 21, whale4);                               // PRINTS THE FOURTH (FROM THE BOTTOM) WHALE IMAGE
  }
  else if (waterSensorState4 == HIGH) {                                     // IF WATER SENSOR 4 IS HIGH
    tft.fillRectHGradient(132, 157, 105, 21, DARKERBLUE, DARKBLUE);         // HIDES THE FOURTH (FROM THE BOTTOM) WHALE IMAGE
    tft.fillRect(146, 252, 44, 26, BLACK);                                  // CLEARS THE FOURTH WATER METER
  }
  if (waterSensorState5 == LOW) {                                           // IF WATER SENSOR 5 IS LOW
    tft.fillRect(192, 250, 48, 30, RED);                                    // FILLS THE FIFTH WATER METER
    tft.pushImage(132, 136, 105, 21, whale5);                               // PRINTS THE FIFTF (FROM THE BOTTOM) WHALE IMAGE
  }
  else if (waterSensorState5 == HIGH) {                                     // IF WATER SENSOR STAE 5 IS HIGH
    tft.fillRectHGradient(132, 136, 105, 21, DARKBLUE, DARKERBLUE);         // HIDES THE FIFTH (FROM THE BOTTOM) WHALE IMAGE
    tft.fillRect(194, 252, 44, 26, BLACK);                                  // CLEARS THE FIFTH WATER METER
  }
  if (waterSensorState6 == LOW) {                                           // IF WATER SENSOR 6 IS LOW
    startBuzzer();                                                          // STARTS THE BUZZER
    tft.pushImage(136, 98, 95, 32, full);                                   // DISPLAYS "FULL!" IMAGE ON THE DISPLAY
    tft.drawRoundRect(135, 97, 97, 34, 2, RED);                             // DRAWS BORDER AROUND "FULL" IMAGE
  }
  else if (waterSensorState6 == HIGH) {                                     // IF WATER SENSOR 6 IS HIGH
    stopBuzzer();                                                           // STOPS THE BUZZER
    tft.pushImage(136, 98, 95, 32, empty);                                  // DISPLAYS "EMPTY" IMAGE ON THE DISPLAY 
    tft.drawRoundRect(135, 97, 97, 34, 2, CYAN);                            // DRAWS BORDER AROUND "EMPTY" IMAGE
    
}
    
    screenNeedsUpdate = false;
  }
  if (buzzerActive) {                                                       // CHECK AND CONTROL THE BUZZER/SPEAKER PATTERN
    switch (currentBuzzerState) {
      case BUZZER_OFF:
        if (currentTime - buzzerStartTime >= buzzerInterval) {
          currentBuzzerState = BUZZER_ON;
          buzzerStartTime = currentTime;
          digitalWrite(BUZPIN, HIGH);                                       // TURN ON THE BUZZER/SPEAKER
        }
        break;
      case BUZZER_ON:
        if (currentTime - buzzerStartTime >= buzzerDuration) {
          currentBuzzerState = BUZZER_OFF;
          buzzerStartTime = currentTime;
          digitalWrite(BUZPIN, LOW);                                        // TURN OFF THE BUZZER/SPEAKER
        }
        break;
    }
  }
}


  void printWeatherValues() {
    tft.setTextSize(0);                                   // SETS THE TEXT SIZE
    tft.setFreeFont(&FreeSans9pt7b);                      // SETS THE FONT
    tft.setTextColor(GREENYELLOW, BLACK);                 // SETS THE TEXT COLOR
    tft.drawRoundRect(5, 103, 44, 28, 5, GREEN);          // DRAWS BORDER AROUND "TMP" TEXT
    tft.drawLine(50, 117, 54, 117, GREEN);                // DRAWS JOINING LINE BETWEEN "TMP" TEXT AND TEMP VALUE BORDER
    tft.setCursor(8, 123);                                // SETS THE CURSOR TO DESIRED X, Y POSITION
    tft.print("TMP");                                     // PRINTS "TMP" ON THE DISPLAY
    tft.drawRoundRect(5, 138, 48, 28, 5, GREEN);          // DRAWS BORDER AOUND "HUM" TEXT
    tft.drawLine(54, 152, 59, 152, GREEN);                // DRAWS JOINING LINE BETWEEN "HUM" TEXT AND HUMIDITY VALUE BORDER
    tft.setCursor(8, 158);                                // SETS THE CURSOR TO DESIRED X, Y POSITION
    tft.print("HUM");                                     // PRINTS "HUM" ON THE DISPLAY
    tft.drawRoundRect(5, 173, 33, 28, 5, GREEN);          // DRAWS BORDER AROUND "RN" TEXT
    tft.drawLine(39, 187, 64, 187, GREEN);                // DRAWS JOINING LINE BETWEEN "RN" TEXT AND RAIN VALUE BORDER
    tft.setCursor(8, 193);                                // SETS THE CURSOR TO DESIRED X, Y POSITION
    tft.print("RN");                                      // PRINT "RN" ON THE DISPLAY
    tft.drawRoundRect(5, 208, 49, 28, 5, GREEN);          // DRAWS BORDER AROUND "WND" TEXT
    tft.drawLine(55, 222, 64, 222, GREEN);                // DRAWS JOINING LINE BETWEEN "WND" TEXT AND WIND VALUE BORDER
    tft.setCursor(8, 228);                                // SETS THE CURSOR TO DESIRED X, Y POSITION
    tft.print("WND");                                     // PRINTS "WND" ON THE DISPLAY
    tft.setTextColor(GOLD, BLACK);                        // SETS THE TEXT COLOR
    tft.setTextSize(2);                                   // SETS THE TEXT SIZE
    tft.setFreeFont(&Orbitron_Medium_20);                 // SETS THE FONT

    tft.fillRoundRect(55, 103, 70, 28, 5, DARKERBLUE);    // DRAWS REFRESH/UPDATE BACKGROUND BEHIND "TMP" VALUE
    tft.fillRoundRect(60, 138, 65, 28, 5, DARKERBLUE);    // DRAWS REFRESH/UPDATE BACKGROUND BEHIND "HUM" VALUE
    tft.fillRoundRect(65, 173, 55, 28, 5, DARKERBLUE);    // DRAWS REFRESH/UPDATE BACKGROUND BEHIND "RN" VALUE
    tft.fillRoundRect(65, 208, 55, 28, 5, DARKERBLUE);    // DRAWS REFRESH/UPDATE BACKGROUND BEHIND "WND" VALUE
    tft.drawRoundRect(55, 103, 70, 28, 5, GREEN);         // DRAWS THE BORDER AROUND THE "TMP" VALUE
    tft.drawRoundRect(60, 138, 65, 28, 5, GREEN);         // DRAWS THE BORDER AROUND THE "HUM" VALUE
    tft.drawRoundRect(65, 173, 55, 28, 5, GREEN);         // DRAWS THE BORDER AROUND THE "RN" VALUE
    tft.drawRoundRect(65, 208, 55, 28, 5, GREEN);         // DRAWS THE BORDER AROUND THE "WND" VALUE
    

    if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(endpoint + apiKey);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      http.end();

      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        int temperature = int(doc["main"]["temp"]);
        int humidity = int(doc["main"]["humidity"]); // Humidity
        int rainProbability = int(doc["pop"]); // Probability of precipitation
        float windSpeed = doc["wind"]["speed"]; // Wind speed

        tft.setTextColor(ORANGE1, GREY6);
        tft.setTextSize(1);
        centerText(String(temperature) + "F", 123, 60, 120); // Temperature
        centerText(String(humidity) + "%", 155, 57, 119);   // Humidity
        centerText(String(rainProbability), 193, 60, 120); // Rain Probability
        centerText(String(int(windSpeed)), 228, 60, 120); // Wind Speed

        Serial.println("Temperature: " + String(temperature) + "°F");
        Serial.println("Humidity: " + String(humidity) + "%");
        Serial.println("Rain Probability: " + String(rainProbability) + "%");
        Serial.println("Wind Speed: " + String(windSpeed));
      } else {
        Serial.println("Failed to parse JSON");
      }
    } else {
      Serial.println("Error on HTTP request");
    }
  }
}

  void startBuzzer() {
    buzzerActive = true;
    currentBuzzerState = BUZZER_OFF;
    buzzerStartTime = millis();
  }

  void stopBuzzer() {
    buzzerActive = false;
    digitalWrite(BUZPIN, LOW);
  }
  
