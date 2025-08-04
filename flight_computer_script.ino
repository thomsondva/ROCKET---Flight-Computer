#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <ESP32Servo.h>
#include <esp_now.h>
#include <WiFi.h>

#define SEALEVELPRESSURE_HPA 1013.25
#define SERVO_PIN 27
#define LED_GREEN 25
#define LED_RED 26
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_BMP280 bmp;
Servo servo;

float groundAltitude = 0;
float relativeAltitude = 0;
float peakAltitude = 0;
bool apogeeDetected = false;
bool parachuteDeployed = false;
bool referenceSet = false;

unsigned long setupTime;
const unsigned long INIT_DELAY = 2 * 60 * 1000;

uint8_t broadcastAddress[] = {0xD0, 0xEF, 0x76, 0x33, 0x27, 0xE0};

typedef struct struct_message {
  char a[32];
  float currenttime;
  float peakalitutude;
  float currentalitutude;
  float groundaltitude;
  bool parachutedeployed;
  bool apogeedetected;
  bool isparachutearmed;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(1000);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN);
  servo.write(0);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    signalError();
  } else {
    Serial.println("ESP-NOW zapnuto");
    signalOK();
  }

  if (!bmp.begin(0x76)) {
    Serial.println("ERROR: BMP280 nenalezeno");
    signalError();
  } else {
    Serial.println("BMP280 zapnuto");
    signalOK();
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  setupTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  float currentAltitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  

  if (!referenceSet && currentTime - setupTime > INIT_DELAY) {
    groundAltitude = currentAltitude;
    referenceSet = true;
    Serial.print("MnM nastaveno na: ");
    Serial.print(groundAltitude);
    Serial.println(" m");
  }

  if (referenceSet) {
    relativeAltitude = currentAltitude - groundAltitude;
    Serial.print("Výška: ");
    Serial.print(relativeAltitude);
    Serial.println(" m");

    if (!apogeeDetected) {
      if (relativeAltitude > peakAltitude) {
        peakAltitude = relativeAltitude;
      } else if ((peakAltitude - relativeAltitude) > 4.0) {
        apogeeDetected = true;
        Serial.println("Apogee detected!");
        deployParachute();
      }
    }
  } else {
    Serial.println("WAITING-4minuty na přípravu...");
  }
  strcpy(myData.a, "NEW DATA PACKET: ");
  myData.currenttime = currentTime;
  myData.peakalitutude = peakAltitude;
  myData.currentalitutude = currentAltitude;
  myData.groundaltitude = groundAltitude;
  myData.parachutedeployed = parachuteDeployed;
  myData.apogeedetected = apogeeDetected;
  myData.isparachutearmed = referenceSet;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(100);
}

void deployParachute() {
  if (!parachuteDeployed) {
    servo.write(90);
    parachuteDeployed = true;
    Serial.println("Padák vypuštěn");
  }
}

void signalError() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  while (true);
}

void signalOK() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
}