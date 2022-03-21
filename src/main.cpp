#include <Arduino.h>
#include <PID_v1.h>
#include <DNSServer.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ESPAsyncWebServer.h"


#define RELAY1_PIN 7
#define RELAY2_PIN 6
#define RELAY3_PIN 5
#define RELAY4_PIN 4

#define TEMP1_PIN 0
#define TEMP2_PIN 1

#define m 32.3263
#define b 41.4603
#define l 0.165968

#define VAL_TO_TEMP(x) (m*log(x+l)+b)
#define GET_TEMP(pin) (VAL_TO_TEMP(analogRead(pin)))

double Setpoint, Input, Output;
double Kp=2, Ki=5, Kd=0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

DNSServer dnsServer;
AsyncWebServer server(80);

void setup() {
  // put your setup code here, to run once:
  pinMode(RELAY1_PIN, OUTPUT);
  Input = GET_TEMP(TEMP1_PIN);
  Setpoint = 100;
  myPID.SetMode(AUTOMATIC);

  WiFi.softAP("esp-captive");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  //more handlers...
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  Input = GET_TEMP(TEMP1_PIN);
  myPID.Compute();
  if (Output > 128) {
    digitalWrite(RELAY1_PIN, HIGH);
  }
  else
  {
    digitalWrite(RELAY1_PIN, LOW);
  }
  dnsServer.processNextRequest();
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
    response->print("<p>This is out captive portal front page.</p>");
    response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
    response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
    response->print("</body></html>");
    request->send(response);
  }
};