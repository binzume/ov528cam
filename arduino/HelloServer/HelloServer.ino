#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include "HttpClient.h"

const char* ssid = "binzume.kstm.org";
const char* password = "************";
const char* POST_URL = "http://www.binzume.net:9000/post?apikey=hoge";
const char* HEARTBEAT_URL = "http://www.binzume.net:9000/hb?apikey=hoge";

MDNSResponder mdns;

ESP8266WebServer server(80);

Ticker uploadTicker;

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handleInit() {
  uploadTicker.detach();
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

void handlePostPhoto() {
  digitalWrite(led, 1);
  camera_get_data_p(POST_URL);
  server.send(200, "text/plain", "ok!");
  digitalWrite(led, 0);
}

void handlePostTest() {
  digitalWrite(led, 1);
  HttpClient req;
  req.post(POST_URL, "POST from esp-wroom-02");

  server.send(200, "text/plain", "ok!");
  digitalWrite(led, 0);
}

volatile uint8_t upload_trig = 0;
void upload() {
  upload_trig = 1;
}


void handlePostStart() {
  digitalWrite(led, 1);

  int d = atoi(server.arg("interval").c_str()) * 1000;
  if (d <= 0) {
    d = 60000;
  }
  uploadTicker.attach_ms(d, upload);

  server.send(200, "text/plain", "start!");
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  //  camera_sync();
  //  camera_init();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  if (mdns.begin("esp8266", WiFi.localIP())) {
    // Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/init", handleInit); // init camera
  server.on("/snap", handleSnapshot); // snapshot
  server.on("/get", handleGetJpeg); // get jpeg
  server.on("/post", handlePostPhoto); // post jpeg
  server.on("/posttest", handlePostTest); // post jpeg
  server.on("/start", handlePostStart); // post jpeg

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();

  HttpClient client;
  client.get(String(HEARTBEAT_URL) + "&ip=" + WiFi.localIP());
  Serial.println("started");
}

void loop(void) {
  server.handleClient();
  if (upload_trig) {
    upload_trig = 0;
    camera_snapshot();
    camera_get_data_p(POST_URL);
    // Serial.print("POST.");
  }
}

