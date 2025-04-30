#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"

#define API_KEY         "AIzaSyAdD1Th2M7EX9F6waL4N0JY3naGUR2IDPg"
#define DATABASE_URL    "https://homesecurity-dfb93-default-rtdb.firebaseio.com/"
#define WIFI_SSID       "tarek_EXT"
#define WIFI_PASSWORD   "Ahmed1488"

#define LED             2
#define RXD2            16
#define TXD2            17

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

HardwareSerial tivacSerial(2);
LiquidCrystal_I2C lcd(0x27, 16, 2);

char prevMsg[64] = "";
QueueHandle_t uartQueue;

void ConnectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void FirebaseInit() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Sign Up: Ok");
    signupOK = true;
  } else {
    Serial.println("Firebase SignUp: Error");
  }
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(100);
  Serial.println("FirebaseInit: Passed");

}

// === Task: UART Reading ===
void TaskReadUART(void *pvParameters) {
  char buffer[64];
  while (1) {
    if (tivacSerial.available()) {
      String msg = tivacSerial.readStringUntil('\n');
      msg.trim();
      msg.toCharArray(buffer, sizeof(buffer));
      xQueueSend(uartQueue, &buffer, portMAX_DELAY);
      Serial.println("Recieved: ");
      Serial.println(msg);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// === Task: Firebase Upload ===
void TaskFirebase(void *pvParameters) {
  char message[64];
  while (1) {
    if (xQueueReceive(uartQueue, &message, portMAX_DELAY)) {
      if (strcmp(message, prevMsg) != 0) {
        if (Firebase.RTDB.setString(&fbdo, "/HomeStatus", message)) {
          Serial.println("Firebase upload successful!");
        }
        strncpy(prevMsg, message, sizeof(prevMsg));
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Yield CPU
  }
}

// === Task: LCD Display ===
void TaskLCD(void *pvParameters) {
  char message[64];
  while (1) {
    if (xQueueReceive(uartQueue, &message, portMAX_DELAY)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String(message).substring(0, 16));
      digitalWrite(LED, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      digitalWrite(LED, LOW);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Yield CPU
  }
}

void setup() {
  Serial.begin(115200);
  tivacSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(LED, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System is ready");

  ConnectToWiFi();
  FirebaseInit();

  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

  uartQueue = xQueueCreate(5, sizeof(char[64]));  // Shared queue with char arrays

  // Assign tasks with logical priorities on core 1
  xTaskCreatePinnedToCore(TaskReadUART, "ReadUART", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebase, "Firebase", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskLCD, "LCD", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // Nothing here, everything runs in tasks
}
