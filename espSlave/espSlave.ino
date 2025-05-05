#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseClient.h>
#include <ESP32Servo.h>

#define API_KEY         "AIzaSyAdD1Th2M7EX9F6waL4N0JY3naGUR2IDPg"
#define DATABASE_URL    "https://homesecurity-dfb93-default-rtdb.firebaseio.com/"
#define USER_EMAIL      "tarekshalaby2015@gmail.com"
#define USER_PASS       "t12345678"
#define WIFI_SSID       "tarek_EXT"
#define WIFI_PASSWORD   "Ahmed1488"

#define LED             2
#define RXD2            16
#define TXD2            17

Servo MyServo;
static const int servoPin = 13;
int angle = 0;

void processData(AsyncResult &aResult);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

HardwareSerial tivacSerial(2);
LiquidCrystal_I2C lcd(0x27, 16, 2);

char sharedMessage[32] = "";
SemaphoreHandle_t msgMutex;

char Activation[10] = "";
SemaphoreHandle_t actMutex;

char door[10] = "";
SemaphoreHandle_t doorMutex;

int intValue = 0;
float floatValue = 0.0f;
String stringValue = "";

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
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  if(app.ready()) Serial.println("Firebase Signup ok");
  else Serial.println("Firebase Signup Failed");
}

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

void TaskFirebase(void *pvParameters) {
  char localCopy[32];
  char lastSent[32] = "";
  while (1) {
    if (xSemaphoreTake(msgMutex, portMAX_DELAY)) {
      strncpy(localCopy, sharedMessage, sizeof(localCopy));
      xSemaphoreGive(msgMutex);
    }

    if(strcmp(localCopy, lastSent) != 0 && app.ready()){
      Database.set<String>(aClient, "/HomeStatus", localCopy, processData, "RTDB_Send_String");
      Serial.print("[Firebase] Uploaded: ");
      Serial.println(localCopy);
      strncpy(lastSent, localCopy, sizeof(lastSent));
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

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
      lcd.print(String(localCopy).substring(0, 16));
      strncpy(lastDisplayed, localCopy, sizeof(lastDisplayed));
      digitalWrite(LED, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      digitalWrite(LED, LOW);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void TaskFirebaseReadDoor(void *pvParameters){
  while(1){
    if(app.ready()){
      Database.get(aClient, "/Door", processData, false, "RTDB_GetDoor");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TaskFirebaseReadActivation(void *pvParameters){
  while(1){
    if(app.ready()){
      Database.get(aClient, "/Activation", processData, false, "RTDB_GetAct");
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void TaskSendActivation(void *pvParameters) {
  char localCopy[10];
  char lastSentActivation[10] = " ";
  while (1) {
    if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
      strncpy(localCopy, Activation, sizeof(localCopy));
      xSemaphoreGive(actMutex);
    }

    if (strncmp(localCopy, lastSentActivation, sizeof(localCopy)) != 0) {
      tivacSerial.write(localCopy);
      strncpy(lastSentActivation, localCopy, sizeof(lastSentActivation));
      Serial.print("[UART] Sent Activation: ");
      Serial.println(localCopy);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TaskControlServo(void *pvParameters) {
  if (!MyServo.attached()) {
    MyServo.attach(servoPin);
  }
  char localCopy[10];
  char lastStatus[10] = "";
  while (1) {
    if (xSemaphoreTake(doorMutex, portMAX_DELAY)) {
      strncpy(localCopy, door, sizeof(localCopy));
      xSemaphoreGive(doorMutex);
    }

    if (strcmp(localCopy, "OPEN") == 0) {
      MyServo.write(0);
    } else if (strcmp(localCopy, "CLOSED") == 0) {
      angle = (angle + 10) % 180;
      MyServo.write(angle);
    }
    strncpy(lastStatus, localCopy, sizeof(lastStatus));
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}

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

  xTaskCreatePinnedToCore(TaskReadUART, "UART", 4096, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebase, "Firebase", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(TaskLCD, "LCD", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebaseReadDoor, "FirebaseReadDoor", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskFirebaseReadActivation, "FirebaseReadAct", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskSendActivation, "SendAct", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskControlServo, "Servo", 4096, NULL, 1, NULL, 1);
}

void loop() {}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;

  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  }

  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available()) {
    String payload = aResult.c_str();
    if (aResult.uid() == "RTDB_GetDoor") {
      if (xSemaphoreTake(doorMutex, portMAX_DELAY)) {
        strncpy(door, payload.c_str(), sizeof(door));
        xSemaphoreGive(doorMutex);
      }
    } else if (aResult.uid() == "RTDB_GetAct") {
      if (xSemaphoreTake(actMutex, portMAX_DELAY)) {
        strncpy(Activation, payload.c_str(), sizeof(Activation));
        xSemaphoreGive(actMutex);
      }
    }
  }
}
