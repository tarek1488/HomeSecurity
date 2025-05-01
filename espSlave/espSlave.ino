#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"

// Wi-Fi & Firebase
#define API_KEY         "AIzaSyAdD1Th2M7EX9F6waL4N0JY3naGUR2IDPg"
#define DATABASE_URL    "https://homesecurity-dfb93-default-rtdb.firebaseio.com/"
#define WIFI_SSID       "tarek_EXT"
#define WIFI_PASSWORD   "Ahmed1488"

// Hardware pins
#define LED             2
#define RXD2            16
#define TXD2            17

// Firebase setup
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// HardwareSerial
HardwareSerial tivacSerial(2);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Shared buffer & mutex for lcd and uart recieve and firebase upload
char sharedMessage[64] = "";
SemaphoreHandle_t msgMutex;

// // shared buffer & mutex for uart send to tiva c task and read from firebase
// char activationValue[32] = "";
// SemaphoreHandle_t activationMutex;


// === Wi-Fi Connect ===
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

// === Firebase Init ===
void FirebaseInit() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Sign Up: Ok");
    signupOK = true;
  } else {
    Serial.println("Firebase SignUp: Error");
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(100);
}

// === Task: UART Reader ===
void TaskReadUART(void *pvParameters) {
  char buffer[64];
  while (1) {
    if (tivacSerial.available()) {
      String input = tivacSerial.readStringUntil('\n');
      input.trim();
      input.toCharArray(buffer, sizeof(buffer));

      if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
        strncpy(sharedMessage, buffer, sizeof(sharedMessage));
        xSemaphoreGive(msgMutex);
      }

      Serial.print("[UART] New message: ");
      Serial.println(buffer);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// === Task: Firebase Upload ===
void TaskFirebase(void *pvParameters) {
  char localCopy[64];
  char lastSent[64] = "";
  while (1) {
    if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
      strncpy(localCopy, sharedMessage, sizeof(localCopy));
      xSemaphoreGive(msgMutex);
    }

    if (strcmp(localCopy, lastSent) != 0 && WiFi.status() == WL_CONNECTED) {
      if (Firebase.RTDB.setString(&fbdo, "/HomeStatus", localCopy)) {
        Serial.print("[Firebase] Uploaded: ");
        Serial.println(localCopy);
        strncpy(lastSent, localCopy, sizeof(lastSent));
      } else {
        Serial.println("[Firebase] Upload FAILED");
      }
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

// === Task: LCD Display ===
void TaskLCD(void *pvParameters) {
  char localCopy[64];
  char lastDisplayed[64] = "";
  while (1) {
    if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
      strncpy(localCopy, sharedMessage, sizeof(localCopy));
      xSemaphoreGive(msgMutex);
    }

    if (strcmp(localCopy, lastDisplayed) != 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String(localCopy).substring(0, 16)); // truncate
      strncpy(lastDisplayed, localCopy, sizeof(lastDisplayed));

      digitalWrite(LED, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      digitalWrite(LED, LOW);
    }

    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

// === Task: Read activation from Firebase and send to Tiva ===
void TaskFirebaseReadAndSend(void *pvParameters) {
  char activationChar;

  while (1) {
    if (Firebase.RTDB.getString(&fbdo, "/Activation")) {
      String value = fbdo.stringData();

      // Determine the character to send
      if (value == "ON") {
        //activationChar = "A\n";
        tivacSerial.print("A\n");
      } else if (value == "OFF") {
        //activationChar = "B\n";
        tivacSerial.print("B\n");
      }
      

      // Send to Tiva with \r\n
      //tivacSerial.print(activationChar);
      //tivacSerial.print("\r\n");

      Serial.print("[Firebase] Activation: ");
      Serial.println(value);
      //Serial.print("[UART -> Tiva] Sent: ");
      //Serial.println(activationChar);
    } else {
      Serial.println("[Firebase] Read failed");
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);  // Every 1 seconds
  }
}




// === Setup ===
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

  msgMutex = xSemaphoreCreateMutex();
  //activationMutex = xSemaphoreCreateMutex();

  


  // Core 1 for tasks
  xTaskCreatePinnedToCore(TaskReadUART, "UART", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebase, "Firebase", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskLCD, "LCD", 4096, NULL, 1, NULL, 1);

  xTaskCreatePinnedToCore(TaskFirebaseReadAndSend, "FirebaseReadSend", 8192, NULL, 1, NULL, 1);

}

void loop() {
  // Idle
}
