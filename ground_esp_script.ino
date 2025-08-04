#include <esp_now.h>
#include <WiFi.h>

#define MAX_BUFF_LEN 255

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
char c;
char str[MAX_BUFF_LEN];
uint8_t idx = 0;


void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));

  Serial.print("[");
  Serial.print(myData.currenttime);
  Serial.print(", ");
  Serial.print(myData.peakalitutude);
  Serial.print(", ");
  Serial.print(myData.currentalitutude);
  Serial.print(", ");
  Serial.print(myData.groundaltitude);
  Serial.print(", ");
  Serial.print(myData.parachutedeployed);
  Serial.print(", ");
  Serial.print(myData.apogeedetected);
  Serial.print(", ");
  Serial.print(myData.isparachutearmed);
  Serial.println("]");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if(Serial.available() > 0) {
    c = Serial.read();

    if(c != '\n') {
      str[idx++] = c;
    }
    else {
      str[idx] = '\0';
      idx = 0;
      Serial.print("Received: ");
      Serial.println(str);
    }
  }
}