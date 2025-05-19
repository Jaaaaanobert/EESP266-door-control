#include <Arduino.h>

#define PIN_OPEN D1
#define PIN_CLOSE D2

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ssid = "YOUR WIFI SSID";
const char* password= "YOUR WIFI PASSWORD";

const char* mqtt_broker = "BROKER URL";
const char* mqtt_topic = "MQTT TOPIC";
const char* mqtt_keepAlive_topic = "MQTT TOPIC FOR KEEP ALIVE";
const char* mqtt_username = "MQTT BROKER USERNAME";
const char* mqtt_password = "MQTT BROKER PASSWORD";
const int mqtt_port = 8883;
const int mqtt_KeepAlive = 1000*20;
const char* ntpServer = "pool.ntp.org";
const short DOOR_OPENS = 1;
const short DOOR_CLOSES = 2;
bool e_OPEN = 0;
bool e_CLOSE = 0;

const char* ca_cert = R"(
YOUR ROOT CERTIFICATE
)";

unsigned int currentMillis;
unsigned int lastMQTTkeepAlive = 0;
unsigned int eventStartedMillis = 0;
short state;
String mqttMessage;

void mqttCallback(char *topic, byte *payload, unsigned int length);
void connectMQTT();
void autoReconnect();
void setClock();
void mqttKeepAlive();
void events();
void eventHandler();

WiFiClientSecure wifiSecClient;
PubSubClient mqttClient(wifiSecClient);

void setup() {
  pinMode(PIN_OPEN, OUTPUT);
  pinMode(PIN_CLOSE, OUTPUT);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  Serial.print("Setting time from NTP-Server");


  configTime(0, 0, "pool.ntp.org");  // UTC
  setClock();  // Wait for time synchronization

  Serial.println("");

  wifiSecClient.setTrustAnchors(new BearSSL::X509List(ca_cert));
  //wifiSecClient.setInsecure();

  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setSocketTimeout(500);
  mqttClient.setKeepAlive(30);

  connectMQTT();
  mqttClient.subscribe(mqtt_topic);

}

void loop() {
  currentMillis = millis();
  mqttKeepAlive();
  autoReconnect();
  mqttClient.loop();
  events();
  eventHandler();
}

void events() {

  if(state == 0) {
    if(mqttMessage == "OPEN") {
      e_OPEN = true;
      state = DOOR_OPENS;
    }

    if(mqttMessage == "CLOSE") {
    e_CLOSE = true;
    state = DOOR_CLOSES;

    }
  }
  mqttMessage = "";
}

void eventHandler() {
  if(e_OPEN || e_CLOSE) {
    eventStartedMillis = currentMillis;
    if(state == DOOR_OPENS) {
      digitalWrite(PIN_CLOSE, LOW);
      digitalWrite(PIN_OPEN, HIGH);
    }
    if(state == DOOR_CLOSES) {
      digitalWrite(PIN_OPEN, LOW);
      digitalWrite(PIN_CLOSE, HIGH);
    }
  }

  if(currentMillis - eventStartedMillis > 1000) {
    state = 0;
  }

  //Activate relais for 1s

  if(state == 0) {
      digitalWrite(PIN_OPEN, LOW);
      digitalWrite(PIN_CLOSE, LOW);
  }

  e_OPEN = false;
  e_CLOSE = false;
}

void autoReconnect() {

//Wifi reconnect:
if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, trying to reconnect...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("Successfully reconnected");
}

//MQTT reconnect:
//MQTT reconnect waits until WiFi is reconnected

  if(!mqttClient.connected()) {
    Serial.println("MQTT-Connection lost, reconnecting...");
    String clientId = "YOUR CLIENT ID-";  // Create a random client ID
    clientId += String(random(0xffff), HEX);
      if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
    mqttClient.subscribe(mqtt_topic);
    Serial.println("MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void connectMQTT() {
  while(!mqttClient.connected()) {
  String clientId = "YOUR CLIENT ID-";  // Create a random client ID
  clientId += String(random(0xffff), HEX);

  Serial.print("The client connects to the mqtt broker\n");

  if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
    Serial.println("MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

  void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String messageTemp;
    for(int i = 0; i < length; i++) {
      messageTemp += (char)payload[i];
    }
    mqttMessage = messageTemp;
    Serial.println(mqttMessage);
}

//Send MQTT keepAlive to a topic so that the broker connection does not timeout
void mqttKeepAlive() {
  if(currentMillis - lastMQTTkeepAlive > mqtt_KeepAlive) {
  mqttClient.publish(mqtt_keepAlive_topic, "keepAlive");
  lastMQTTkeepAlive = currentMillis;
  }
}

//set NTP time

void setClock() {
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { 
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &timeinfo);
  Serial.println("Time initally set to: ");
  Serial.println(buffer);
}