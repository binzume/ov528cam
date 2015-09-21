#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
 
const char* ssid = "binzume.kstm.org";
const char* password = "************";
MDNSResponder mdns;

ESP8266WebServer server(80);

const int led = 13;

void handleInit() {
  digitalWrite(led, 1);

  camera_sync();
  camera_init();

  server.send(200, "text/plain", "init ok!");
  digitalWrite(led, 0);
}

void handleSnapshot() {
  digitalWrite(led, 1);
  camera_snapshot();
  server.send(200, "text/plain", "ok!");
  digitalWrite(led, 0);
}


void handleGetJpeg() {
  digitalWrite(led, 1);
  camera_get_data();
  digitalWrite(led, 0);
}
void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);

//  camera_sync();
//  camera_init();

//  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
//    Serial.print(".");
  }
//  Serial.println("");
//  Serial.print("Connected to ");
//  Serial.println(ssid);
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
//    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  server.on("/init", handleInit); // init camera
  server.on("/snap", handleSnapshot); // snapshot
  server.on("/get", handleGetJpeg); // get jpeg
  
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);
  
  server.begin();
//  Serial.println("HTTP server started");
}
 
void loop(void){
  server.handleClient();
}


