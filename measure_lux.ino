#include "user_interface.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";

// ENVISION INFORMATION
const char* host = "envision-iot.herokuapp.com";
const int httpsPort = 443;
const char* fingerprint = "08 3B 71 72 02 43 6E CA ED 42 86 93 BA 7E DF 81 C4 BC 62 30";
String token = "";
String apiUsername = "";
String apiPassword = "";

int brightness = 0;

void setup() {
  Serial.begin(115200);
  connectWiFi();
}

void loop() {
  if (token == "") {
    String authResponse = postAuth();
    token = getToken(authResponse);
  }
  
  brightness = system_adc_read();
  postSensorData(brightness, token);

  delay(60000);
}

String postAuth() {
  const char* uri = "/api/auth/login";

  // Ref: https://arduinojson.org/v5/doc/encoding/
  StaticJsonDocument<256> doc;
  doc["username"] = apiUsername;
  doc["password"] = apiPassword;

  String dummy = "dummy";
  
  return postRequest(uri, doc, dummy);
}

String postSensorData(int brightness, String token) {
  const char* uri = "/api/envs";

  StaticJsonDocument<256> doc;
  const int objCap = JSON_OBJECT_SIZE(2);
  StaticJsonDocument<256> obj;
  obj["type"] = "lux";
  obj["value"] = brightness;
  doc.add(obj);

  postRequest(uri, doc, token);
}

String getToken(String authResponse){
  return authResponse.substring(22,150);
}

String postRequest(const char* uri, JsonDocument& doc, String token) {
  Serial.println(uri);
  serializeJson(doc, Serial);

  // REQUEST VARIABLES
  WiFiClientSecure httpsClient;
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000);

  Serial.println("HTTPS Connecting");
  int retryCounter = 0;
  while((!httpsClient.connect(host, httpsPort)) && (retryCounter < 3)){
    delay(100);
    Serial.print(".");
    retryCounter++;
  }
  if(retryCounter==3) {
    Serial.println("Connection failed");
  }
  else {
    Serial.println("Connected to web");
  }

  httpsClient.print("POST ");
  httpsClient.print(uri);
  httpsClient.println(" HTTP/1.1");
  httpsClient.print("Host: ");
  httpsClient.println(host);
  httpsClient.println("Connection: close");
  httpsClient.print("Content-Length: ");
  httpsClient.println(measureJson(doc));
  httpsClient.println("Content-Type: application/json");
  httpsClient.print("x-access-token: ");
  httpsClient.println(token);
  httpsClient.println();
  
  serializeJson(doc, httpsClient);
  
  while (httpsClient.connected()) {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
 
  Serial.println("reply was:");
  Serial.println("==========");
  String response;
  while(httpsClient.available()){        
    response = httpsClient.readStringUntil('\n');
    Serial.println(response);
  }
  Serial.println("==========");
  Serial.println("closing connection");

  return response;
}

void connectWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Waiting for connection");
  }
}
