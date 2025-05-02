#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Firebase_ESP_Client.h>
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include <ESP32Servo.h>

// Wi-Fi & Firebase
#define API_KEY         "AIzaSyAdD1Th2M7EX9F6waL4N0JY3naGUR2IDPg"
#define DATABASE_URL    "https://homesecurity-dfb93-default-rtdb.firebaseio.com/"
#define WIFI_SSID       "tarek_EXT"
#define WIFI_PASSWORD   "Ahmed1488"

// Hardware pins
#define LED             2
#define RXD2            16
#define TXD2            17

Servo MyServo;

static const int servoPin = 18;

// Firebase setup
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// HardwareSerial
HardwareSerial tivacSerial(2);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Shared buffer & mutex for lcd and uart recieve and firebase upload
char sharedMessage[32] = "";
SemaphoreHandle_t msgMutex;

char Activation[4] = "";
SemaphoreHandle_t actMutex;

char door[10] = "";
SemaphoreHandle_t doorMutex;

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

  // ✅ Add these lines before Firebase.begin()
  //config.keepAlive = true;
  //config.timeout.serverResponse = 10 * 1000; // 10 seconds
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(100);
}

// === Task: UART Reader ===
void TaskReadUART(void *pvParameters) {
  char buffer[32];
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
  char localCopy[32];
  char lastSent[32] = "";
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
  char localCopy[32];
  char lastDisplayed[32] = "";
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
// === Task: Read door status from Firebase
void TaskFirebaseReadDoor(void *pvParameters){
  char door_buffer[10] = "";
  while(1){
    if (Firebase.RTDB.getString(&fbdo, "/Door")) {
      String value = fbdo.stringData();
      value.trim();
      value.toCharArray(door_buffer, sizeof(door_buffer));
      if (xSemaphoreTake(doorMutex, portMAX_DELAY)) {
        strncpy(door, door_buffer , sizeof(door));
        xSemaphoreGive(doorMutex);
      }
      Serial.print("[Firebase Read Door]: ");
      Serial.println(value);
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
// === Task: Read Activation status from Firebase
void TaskFirebaseReadActivation(void *pvParameters){
  while(1){
    if (Firebase.RTDB.getString(&fbdo, "/Activation")) {
      String value = fbdo.stringData();
      // Determine the character to send
      if (value == "ON") {
        if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
          strncpy(Activation, "A\n", sizeof(Activation));
          xSemaphoreGive(actMutex);
        }
      } else if (value == "OFF") {
        if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
          strncpy(Activation, "B\n", sizeof(Activation));
          xSemaphoreGive(actMutex);
        }
      }
      Serial.print("[Firebase Read Act]: ");
      Serial.println(value);
    }
      
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// === Task: Send to Activation to Tiva ===
void TaskSendActivation(void *pvParameters) {
  char localCopy[4];  // To store the activation value
  while (1) {
    // Take the mutex to safely copy the shared value
    if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
      strncpy(localCopy, Activation, sizeof(localCopy));
      xSemaphoreGive(actMutex);
    }

    
    
    // Always send the activation value
    // Only send if the value is "A\n" or "B\n"
    if (strcmp(localCopy, "A\n") == 0 || strcmp(localCopy, "B\n") == 0) {
      tivacSerial.print(localCopy);  // Send via UART
      Serial.print("[UART] Sent Activation: ");
      Serial.println(localCopy);    // For debugging
    }
    
     
    
    vTaskDelay(500 / portTICK_PERIOD_MS);  // Delay before sending again
  }
}

// === Task: Rotate motor based on door status ===
void TaskControlServo(void *pvParameters) {
  char localCopy[10];  // To store the door status
  char lastStatus[10] = "";  // To check if the door status has changed

  while (1) {
    // Take the mutex to safely copy the shared door value
    if (xSemaphoreTake(doorMutex, portMAX_DELAY)) {
      strncpy(localCopy, door, sizeof(localCopy));
      xSemaphoreGive(doorMutex);
    }

    // If door status changes, blink LED
    if (strcmp(localCopy, lastStatus) != 0) {
      if (strcmp(localCopy, "OPEN") == 0) {
        MyServo.write(90);
        Serial.println("[LED] Door is OPEN - LED ON");
      } else if (strcmp(localCopy, "CLOSED") == 0) {
        MyServo.write(0);
        Serial.println("[LED] Door is CLOSED - LED OFF");
      }

      strncpy(lastStatus, localCopy, sizeof(lastStatus));  // Update last status
    }
    
    vTaskDelay(400 / portTICK_PERIOD_MS);  // Delay before checking again
  }
}






// === Setup ===
void setup() {
  Serial.begin(115200);
  MyServo.attach(servoPin);
  tivacSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(LED, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System is ready");

  ConnectToWiFi();
  FirebaseInit();

  msgMutex = xSemaphoreCreateMutex();
  actMutex = xSemaphoreCreateMutex();
  doorMutex = xSemaphoreCreateMutex();


  
  // Core 1 for tasks
  xTaskCreatePinnedToCore(TaskReadUART, "UART", 4096, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebase, "Firebase", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(TaskLCD, "LCD",   4096, NULL, 2, NULL, 1);

  xTaskCreatePinnedToCore(TaskFirebaseReadDoor, "FirebaseRead door", 8192, NULL, 2, NULL, 1);  // Read Firebase task
  xTaskCreatePinnedToCore(TaskFirebaseReadActivation, "FirebaseRead Activation", 8192, NULL, 2, NULL, 1);  // Read Firebase task
  
  xTaskCreatePinnedToCore(TaskSendActivation, "SendActivation", 4096, NULL, 1, NULL, 1);  // Send Activation task
  xTaskCreatePinnedToCore(TaskControlServo, "ControlServo", 4096, NULL, 1, NULL, 1);  // Control LED task
  // Print the remaining heap memory
  Serial.print("Remaining heap memory: ");
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  // Idle
}
