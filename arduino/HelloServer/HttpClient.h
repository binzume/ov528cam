#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H 1

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

class HttpResponse {
public:
  static const int STATUS_OK = 200;
  static const int STATUS_NOT_FOUND = 404;
  static const int STATUS_CONNECTION_FAILED = -1;
  
  int status;
  String body;
};

// HTTTP client class. **DO NOT REUSE INSTANCE**
class HttpClient : public WiFiClient {
  String headers;
public:

  HttpClient(int _dummy) {
    // Serial.print("init HttpClient");
    headers = String("");
  }

  void addHeader(const String &name, const String &value) {
    headers += name + ": " + value + "\r\n";
  }
  void setContentLength(int length) {
    addHeader(String("Content-Length"), String(length));
  }

  HttpResponse request(const char *method, const char *host, int port, const String &path, bool stream = false) {
    HttpResponse r;
    if (!connect(host, port)) {
      r.status = HttpResponse::STATUS_CONNECTION_FAILED;
      return r;
    }
  
    print(String(method) + " " + path + " HTTP/1.1\r\n");
    print(String("Host: ") + host + "\r\n");
    print(headers);
    print(String("Connection: close\r\n"));
    print(String("\r\n"));

    if (!stream) {
      readResponse(r);
    }
    return r;
  }

  void readResponse(HttpResponse &r) {
    r.body = String("");
    while(available()){
      String line = readStringUntil('\r');
      r.body += line;
    }
  }

  String get(const char *host, int port, const String &path) {
    HttpResponse r = request("GET", host, port, path);
    return r.body;
  }

  void post(const char *host, int port, const String &path, const String &data) {
    setContentLength(data.length());
    addHeader("Content-Type", "text/plain");
    HttpResponse r = request("POST", host, port, path, true);
    print(data);
    print(String("\r\n"));
    readResponse(r);
  }

};

#endif

