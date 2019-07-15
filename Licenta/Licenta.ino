#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTTYPE DHT11   
const char* ssid = "QDDASFASF";
const char* password = "albacazapada";
const char* mqtt_server = "192.168.1.119";
int dryValue = 459;
int wetValue = 150;
int friendlyDryValue = 0;
int friendlyWetValue = 100;
int RAWValue = analogRead(A0);
boolean if_pump_auto = true;


WiFiClient espClient;
PubSubClient client(espClient);

const int ledGPIO5 = 5;
const int ledGPIO4 = 4;
const int DHTPin = 14;

DHT dht(DHTPin, DHTTYPE);

long now = millis();
long lastMeasure = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if(topic=="esp8266/4"){
      Serial.print("Changing GPIO 4 to ");
      if(messageTemp == "1"){
        digitalWrite(ledGPIO4, HIGH);
        Serial.print("On");
        if_pump_auto = false;
      }
      else if(messageTemp == "0"){
        digitalWrite(ledGPIO4, LOW);
        Serial.print("Off");
        if_pump_auto = true; 
      }
  }
  if(topic=="esp8266/5"){
      Serial.print("Changing GPIO 5 to ");
      if(messageTemp == "1"){
        digitalWrite(ledGPIO5, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "0"){
        digitalWrite(ledGPIO5, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}

void auto_pump(){
  if(if_pump_auto){
        if(RAWValue<=280){
        digitalWrite(ledGPIO4, LOW);
        Serial.print("Off"); 
        }
        else{
          digitalWrite(ledGPIO4, HIGH);
          Serial.print("Off");
        }
      }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
   
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      client.subscribe("esp8266/4");
      client.subscribe("esp8266/5");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  dht.begin();
  pinMode(ledGPIO4, OUTPUT);
  pinMode(ledGPIO5, OUTPUT);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");
  now = millis();
  int rawValue = analogRead(A0);
  RAWValue = analogRead(A0);

  Serial.print("Raw: ");
  Serial.print(rawValue);
  Serial.print(" | ");
  
  int friendlyValue = map(rawValue, dryValue, wetValue, friendlyDryValue, friendlyWetValue);

  Serial.print("Friendly: ");
  Serial.print(friendlyValue);
  Serial.println("%");

  if (now - lastMeasure > 1000) {
    lastMeasure = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature(); 
    float f = dht.readTemperature(true);
    auto_pump();

    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    float hic = dht.computeHeatIndex(t, h, false);
   
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);

    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    static char moisture[7];
    dtostrf(friendlyValue, 6, 2, moisture);

    client.publish("/esp8266/temperature", temperatureTemp);
    client.publish("/esp8266/humidity", humidityTemp);
    client.publish("/esp8266/moisture", moisture);
    
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
  }
}
