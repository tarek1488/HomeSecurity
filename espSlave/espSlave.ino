#include <WiFi.h>
//#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

// Wi-Fi & Firebase
#define API_KEY         "AIzaSyAdD1Th2M7EX9F6waL4N0JY3naGUR2IDPg"
#define DATABASE_URL    "https://homesecurity-dfb93-default-rtdb.firebaseio.com/"
#define USER_EMAIL "tarekshalaby2015@gmail.com"
#define USER_PASS "t123456789"

#define WIFI_SSID       "tarek_EXT"
#define WIFI_PASSWORD   "Ahmed1488"

// Hardware pins
#define LED             2
#define RXD2            16
#define TXD2            17

// Define TX and RX pins for UART (change if needed)
#define TXD1 33
#define RXD1 32

Servo MyServo;
Servo MyServo2;

static const int servoPin = 13;
int angle = 0;

void processData(AsyncResult &aResult);
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// HardwareSerial
HardwareSerial tivacSerial(2);
HardwareSerial mp3Serial(1);
//LiquidCrystal_I2C lcd(0x27, 16, 2);

DFRobotDFPlayerMini mp3player;
#define BUSY_PIN 25

void playHadras() {
  Serial.println("Playing Hadras (Track 2)");
  mp3player.play(2);
}

// Shared buffer & mutex for lcd and uart recieve and firebase upload
char sharedMessage[32] = "";
SemaphoreHandle_t msgMutex;

char Activation[10] = "";
SemaphoreHandle_t actMutex;

char door[10] = "";
SemaphoreHandle_t doorMutex;


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
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  Serial.println("----> FireBase Signup ok");
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
      if((strcmp(buffer, "Home Is Safe") != 0) && (strcmp(buffer, "System off") != 0)){// there is an alert
        if(digitalRead(BUSY_PIN) == HIGH){
          playHadras();
        }
        MyServo.write(180);
        Serial.println("[SERVO] Door is CLOSED");
        if(app.ready()){
          Database.set<String>(aClient, "/Door", "CLOSED");
        }
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
    if(app.ready()){
      if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
      strncpy(localCopy, sharedMessage, sizeof(localCopy));
      xSemaphoreGive(msgMutex);
      } 
    if (strcmp(localCopy, lastSent) != 0) {
      Database.set<String>(aClient, "/HomeStatus", localCopy);
      if (aClient.lastError().code() == 0){
        Serial.print("[Firebase] Uploaded: ");
        Serial.println(localCopy);
        strncpy(lastSent, localCopy, sizeof(lastSent));
      }
      else {
        Serial.println("[Firebase] Upload FAILED\n");
        Firebase.printf("Error, msg: %s, code: %d\n", aClient.lastError().message().c_str(), aClient.lastError().code());
      }
    }

    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}


// === Task: Read door status from Firebase
void TaskFirebaseReadDoor(void *pvParameters){
  char door_buffer[10] = "";
  while(1){
    if(app.ready()){
      String value = Database.get<String>(aClient, "/Door");
      if (aClient.lastError().code() == 0){
        value.trim();
        value.toCharArray(door_buffer, sizeof(door_buffer));
        if (xSemaphoreTake(doorMutex, portMAX_DELAY)) {
          strncpy(door, door_buffer , sizeof(door));
          xSemaphoreGive(doorMutex);
        }
        Serial.print("[Firebase Read Door]: ");
        Serial.println(value);
      }
      else {
        Firebase.printf("Error, msg: %s, code: %d\n", aClient.lastError().message().c_str(), aClient.lastError().code());
        Serial.println("Firebase read door failed");
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// === Task: Read Activation status from Firebase
void TaskFirebaseReadActivation(void *pvParameters){
  char activation_buffer[10];
  while(1){
    if(app.ready()){
      String value = Database.get<String>(aClient, "/Activation");
      if (aClient.lastError().code() == 0){
        value.trim();
        value.toCharArray(activation_buffer, sizeof(activation_buffer));
        // Determine the character to send
        if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
            strncpy(Activation, activation_buffer, sizeof(Activation));
            xSemaphoreGive(actMutex);
          }
        Serial.print("[Firebase Read Act]: ");
        Serial.println(value);
      }
      else{
        Firebase.printf("Error, msg: %s, code: %d\n", aClient.lastError().message().c_str(), aClient.lastError().code());
        Serial.println("Firebase read activation failed");
      }
    }          
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}


// === Task: Send to Tiva only if value changed ===
void TaskSendActivation(void *pvParameters) {
  char localCopy[10];
  char lastSentActivation[10] = "";
  while (1) {
    if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
      strncpy(localCopy, Activation, sizeof(localCopy));
      xSemaphoreGive(actMutex);
    }

    // Only send if the value has changed
    if (strncmp(localCopy, lastSentActivation, sizeof(localCopy)) != 0) {
      tivacSerial.write(localCopy);  // Send via UART
      strncpy(lastSentActivation, localCopy, sizeof(lastSentActivation));

      Serial.print("[UART] Sent Activation: ");
      Serial.println(localCopy);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// === Task: Rotate motor based on door status ===
void TaskControlServo(void *pvParameters) {
  char localCopy[10];  
  while (1) {
    // Take the mutex to safely copy the shared door value
    if (xSemaphoreTake(doorMutex, portMAX_DELAY)) {
      strncpy(localCopy, door, sizeof(localCopy));
      xSemaphoreGive(doorMutex);
    }
    if (strcmp(localCopy, "OPEN") == 0) {
      MyServo.write(50);
      Serial.println("[SERVO] Door is OPEN");
    } 
    vTaskDelay(400 / portTICK_PERIOD_MS);  // Delay before checking again
  }
}


// === Setup ===
void setup() {
  Serial.begin(115200);
  MyServo.attach(servoPin);
  tivacSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  mp3Serial.begin(9600,SERIAL_8N1, RXD1, TXD1);
  mp3player.begin(mp3Serial);
  mp3player.volume(30);
  pinMode(BUSY_PIN, INPUT);
  
  pinMode(LED, OUTPUT);
  
  ConnectToWiFi();
  FirebaseInit();

  msgMutex = xSemaphoreCreateMutex();
  actMutex = xSemaphoreCreateMutex();
  doorMutex = xSemaphoreCreateMutex();


  
  // Core 1 for tasks
  xTaskCreatePinnedToCore(TaskReadUART, "UART", 8192, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebase, "Firebase", 8192, NULL, 3, NULL, 1);
  

  xTaskCreatePinnedToCore(TaskFirebaseReadDoor, "FirebaseRead door", 8192, NULL, 2, NULL, 1);  // Read Firebase task
  xTaskCreatePinnedToCore(TaskFirebaseReadActivation, "FirebaseRead Activation", 8192, NULL, 2, NULL, 1);  // Read Firebase task
  
  xTaskCreatePinnedToCore(TaskSendActivation, "SendActivation", 4096, NULL, 5, NULL, 1);  // Send Activation task
  xTaskCreatePinnedToCore(TaskControlServo, "ControlServo", 4096, NULL, 1, NULL, 1);  // Control LED task
  
}

void loop() {
  // Idle
}


void processData(AsyncResult &aResult){
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available()){
    // Log the task and payload
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
  }
}
