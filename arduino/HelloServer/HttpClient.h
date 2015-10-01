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
class ESP8266HttpClient : public WiFiClient {
    String headers;
  public:

    ESP8266HttpClient() {
      // Serial.print("init HttpClient ()\n");
      headers = String("");
    }

    void setHeader(const String &name, const String &value) {
      headers += name + ": " + value + "\r\n";
    }
    void setContentLength(int length) {
      setHeader(String("Content-Length"), String(length));
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
      while (available()) {
        String line = readStringUntil('\r');
        r.body += line;
      }
    }

    String get(const String &url) {
      if (!url.startsWith("http://")) {
        return "";
      }
      int sp = url.indexOf('/', 7);
      int cp = url.indexOf(':', 7);
      int hp = sp;
      int port = 80;
      if (cp > 0 && cp < sp) {
        port = atoi(url.substring(cp + 1, sp).c_str());
        hp = cp;
      }

      HttpResponse r = request("GET", url.substring(7, hp).c_str(), port, url.substring(sp));
      return r.body;
    }

    HttpResponse post_start(const String &url) {
      if (!url.startsWith("http://")) {
        return HttpResponse();
      }
      int sp = url.indexOf('/', 7);
      int cp = url.indexOf(':', 7);
      int hp = sp;
      int port = 80;
      if (cp > 0 && cp < sp) {
        port = atoi(url.substring(cp + 1, sp).c_str());
        hp = cp;
      }

      return request("POST", url.substring(7, hp).c_str(), port, url.substring(sp), true);
    }

    void post(const String &url, const String &data) {
      setContentLength(data.length());
      HttpResponse r = post_start(url);
      print(data);
      print(String("\r\n"));
      readResponse(r);
    }

};

typedef ESP8266HttpClient HttpClient;

#endif


