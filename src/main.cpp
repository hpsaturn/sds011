#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <SDS011.h>
#include <AsyncMqttClient.h>

const char SSID[] = "Skynet";
const char PASS[] = "121232aU..!++";

const IPAddress BROKER = {192, 168, 0, 162};

SDS011 sds011;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

bool connected = false;

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(SSID, PASS);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnected(bool sessionPresent) {
  connected = true;
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  connected = false;
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void setup() {
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnected);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(BROKER, 1883);

  sds011.setup(&Serial);
  sds011.onData([](float pm25Value, float pm10Value) {
    if (connected) {
      mqttClient.publish("/CANAIRIO/PM2_5", 1, false, String(pm25Value, 1).c_str());
      mqttClient.publish("/CANAIRIO/PM10", 1, false, String(pm10Value, 1).c_str());
    }
  });
  
  sds011.onError([](int8_t error){
    // error happened
    // -1: CRC error
  });
  sds011.setWorkingPeriod(5);

  connectToWifi();
}

void loop() {
  sds011.loop();
}

