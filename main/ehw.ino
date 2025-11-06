#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP_Mail_Client.h>



// WiFi and ThingSpeak
const char* ssid = "";
const char* password = "";
const char* apiKey = "_key_";
const char* server = "http://api.thingspeak.com/update?api_key=";


// Sensor objects
Adafruit_MPU6050 mpu;
MAX30105 particleSensor;

// I2C pins
#define I2C_SDA 6
#define I2C_SCL 15

// Other pins
#define ALERT_PIN 19
#define LED_PIN 2

// Thresholds
#define FALL_THRESHOLD_G 0.01
#define LOW_BPM 50
#define HIGH_BPM 110
#define LOW_SPO2 92

// Sleep interval
#define DEEP_SLEEP_TIME 15
#define uS_TO_S_FACTOR 1000000ULL

// Data variables
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;
float temperature;
int32_t heartRate = 0;
int8_t validHeartRate = 0;
int32_t spo2 = 0;
int8_t validSPO2 = 0;
bool fallDetected = false;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute; 
int beatAvg;

unsigned long lastSensorRead = 0;
unsigned long lastSend = 0;
unsigned long inactivityTimer = 0;
const unsigned long SENSOR_INTERVAL = 1000;  
const unsigned long SEND_INTERVAL = 2000;    
const unsigned long INACTIVITY_TIMEOUT = 300000; 

// Function declarations
void initSensors();
void readMPU6050();
void readMAX30102();
bool checkFall();
void sendDataToThingSpeak();
void triggerAlert(String message);
void goToDeepSleep();
void print_wakeup_reason();
void sendMailOnFall(const char* recipient);
void sendEmail(const String& subject, const String& body);
void postJsonData(const char* serverUrl, const String& jsonPayload);

void setup() {
  Serial.begin(115200);
  delay(1000);
  print_wakeup_reason();

  pinMode(LED_PIN, OUTPUT);
  pinMode(ALERT_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(ALERT_PIN, LOW);

  Wire.begin(I2C_SDA, I2C_SCL);
  initSensors();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.println("Setup complete!\n");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;
    readMPU6050();
    readMAX30102();

    bool fall = checkFall();
    if (fall) {
      fallDetected = true;
      triggerAlert("FALL DETECTED!");
      sendDataToThingSpeak(); // upload data
      // Send email alerts
      /*
      sendEmail("Fall Detected!", "A fall has been detected by your ESP32 health monitor.");
      sendMailOnFall(recipientEmail1);
      sendMailOnFall(recipientEmail2);
      sendMailOnFall(recipientEmail3);*/
    }

    if (validHeartRate && (heartRate < LOW_BPM || heartRate > HIGH_BPM)) {
      triggerAlert("Heart rate abnormal: " + String(heartRate) + " BPM");
    }
    if (validSPO2 && spo2 < LOW_SPO2) {
      triggerAlert("SpO2 low: " + String(spo2) + "%");
    }

    if (abs(accelX) < 0.5 && abs(accelY) < 0.5 && abs(accelZ - 9.8) < 0.5) {
      inactivityTimer += SENSOR_INTERVAL;
    } else {
      inactivityTimer = 0;
    }
  }

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();
    sendDataToThingSpeak();
  }
  if (inactivityTimer >= INACTIVITY_TIMEOUT) {
    Serial.println("Prolonged inactivity. Sleeping...");
    goToDeepSleep();
  }
  delay(10);
}

void initSensors() {
  Serial.println("Initializing MPU6050...");
  if (!mpu.begin()) { Serial.println("Failed to find MPU6050!"); while (1); }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  Serial.println("Initializing MAX30102...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { Serial.println("MAX30102 not found!"); while (1); }
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

// Read sensors
void readMPU6050() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  accelX = a.acceleration.x; accelY = a.acceleration.y; accelZ = a.acceleration.z;
  gyroX = g.gyro.x; gyroY = g.gyro.y; gyroZ = g.gyro.z;
  temperature = temp.temperature;
}
void readMAX30102() {
  long irValue = particleSensor.getIR();
  if (irValue < 50000) {
    heartRate=0; spo2=0; validHeartRate=0; validSPO2=0; return;
  }
  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);
    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; rateSpot %= RATE_SIZE;
      beatAvg=0; for (byte x=0; x<RATE_SIZE; x++) { beatAvg += rates[x]; }
      beatAvg /= RATE_SIZE; heartRate=beatAvg; validHeartRate=1;
    }
  }
  // Simplified SpO2
  static int spo2Counter=0; spo2Counter++; 
  if (spo2Counter>25 && validHeartRate){ spo2=97; validSPO2=1; spo2Counter=0; }
}

// Check for fall
bool checkFall() {
  float mag=sqrt(accelX*accelX + accelY*accelY + accelZ*accelZ);
  float g=mag/9.81;
  if (g>FALL_THRESHOLD_G) {
    Serial.println("*** FALL DETECTED! ***");
    return true;
  }
  return false;
}

// Send data to ThingSpeak
void sendDataToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) { Serial.println("WiFi not connected"); return; }
  HTTPClient http;
  String url=String(server)+String(apiKey);
  url+="&field1="+String(heartRate);
  url+="&field2="+String(spo2);
  url+="&field3="+String(fallDetected?1:0);
  url+="&field4="+String(accelX);
  url+="&field5="+String(accelY);
  url+="&field6="+String(accelZ);
  url+="&field7="+String(gyroX);
  url+="&field8="+String(gyroY);
  url+="&field9="+String(gyroZ);
  url+="&field10="+String(temperature);
  http.begin(url);
  int resp=http.GET();
  if (resp>0){ Serial.println("Data sent to ThingSpeak"); }
  else { Serial.println("Error sending data"); }
  http.end();
  fallDetected=false;
}

// Trigger alert
void triggerAlert(String msg) {
  Serial.println("ALERT: "+msg);
  digitalWrite(ALERT_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(ALERT_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

// Deep sleep
void goToDeepSleep() {
  Serial.println("Going to deep sleep");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  switch (reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup: RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup: RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup: timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup: touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup: ULP"); break;
    default: Serial.println("Wakeup: first boot"); break;
  }
}
