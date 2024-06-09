//Khai báo các thư viện cần thiết
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_ADS1X15.h>
#include <BlynkSimpleEsp8266.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DHT.h>   
#include <NTPClient.h>

const char *ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // Offset cho múi giờ GMT+7 (phụ thuộc vào múi giờ của bạn)
const int   daylightOffset_sec = 0;

//Đầu vào đọc DHT11 (pin D4) 
#define DHTPIN 2  
DHT dht(DHTPIN, DHT11);

//Khai báo wifi
#define WIFI_SSID "14DS61"
#define WIFI_PASSWORD "khongbiet"

//Khai báo cho thiết bị ads1115
Adafruit_ADS1115 ads; 

//Khai báo cho firebase
#define API_KEY "AIzaSyD3MrS7YiEXffnsQqOKa2qh2E4epmmTc30"
#define USER_EMAIL "lengocnguyenminh.it@gmail.com"
#define USER_PASSWORD "123456789"
#define DATABASE_URL "https://qtmt-c9731-default-rtdb.firebaseio.com/"


//Đầu vào A0, A1 ads1115 mq2, mq135
int16_t adc0, adc1;

//Khai báo firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String mq2Path = "/Flammable_Gas:";
String mq135Path = "/Air_Quality";
String tempPath = "/Temperature";
String huPath = "/Humidity";
String timePath = "/timestamp";
String datePath = "/datestamp";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);


// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000;

//Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

//Hàm setup
void setup(){
  Serial.begin(115200);
  ads.begin();
  dht.begin();

  // Initialize BME280 sensor
  initWiFi();
  timeClient.begin();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/Tu_01";
}
String timestamp;
String epoTime;
char formattedDate[40];
//Hàm loop
void loop(){
  adc0 = ads.readADC_SingleEnded(0);  // Đọc giá trị từ kênh 0
  adc1 = ads.readADC_SingleEnded(1); // Đọc giá trị từ kênh 1
  float h = dht.readHumidity(); // Đọc giá trị độ ẩm
  float t = dht.readTemperature(); // Đọc giá trị nhiệt độ
  // Send new readings to database
  if (Firebase.ready()){
    sendDataPrevMillis = millis();
    
    epoTime = getTime();
    parentPath= databasePath + "/" + String(epoTime);

    //Get current timestamp
      time_t now = timeClient.getEpochTime();
  
  // Chuyển đổi thời gian hiện tại thành cấu trúc tm
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    timestamp = timeClient.getFormattedTime();
    sprintf(formattedDate, "%02d/%02d/%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);

    json.set(mq2Path.c_str(), String(adc0/10)); //Đưa giá trị mq2 lên firebase
    json.set(mq135Path.c_str(), String(adc1)); //Đưa giá trị mq135 lên firebase
    json.set(tempPath.c_str(), String(t)); //Đưa giá trị nhiệt độ lên firebase
    json.set(huPath.c_str(), String(h)); //Đưa giá trị độ ẩm lên firebase
    json.set(timePath, String(timestamp)); //Đưa giá trị epoch time lên firebase 
    json.set(datePath, String(formattedDate)); //Đưa giá trị epoch time lên firebase 
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str()); 
    delay(10000);
  }
}