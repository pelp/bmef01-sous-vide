// ============= main.cpp ===============
// AUTHOR       :           Felix Malmsjö
// CREATE DATE  :              2022-03-21
// PURPOSE      :    Sous Vide controller
// COURSE       :                  BMEF01
// ======================================
#include <Arduino.h>
#include <PID_v1.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "CircularBuffer.h"

// Relay control pins
#define RELAY1_PIN 7
#define RELAY2_PIN 6
#define RELAY3_PIN 5
#define RELAY4_PIN 4

// Temperature ADC pins
#define TEMP1_PIN 34
#define TEMP2_PIN 35

// General constants
#define TEMPERATURE_DATA_LEN    100
#define SAMPLE_TIME             100
#define PID_TIME                5
#define DEFAULT_TEMPERATURE     70

// OLD LOG APPROX
// Not as good as the new cubic approx
/*
#define m 99.1104
#define l 833.729
#define b -673.374

#define VAL_TO_TEMP(x) (m * log(x + l) + b)
*/
// These constants are for a R2 value of 1.5kOhm,
// this will give the most accurate results in the 50-90 degree range.
#define a 5.0523e-9
#define b -2.7905e-5
#define c 7.7676e-2
#define d -1.0894e1

#define VAL_TO_TEMP(x) a*x*x*x + b*x*x + c*x + d
#define GET_TEMP(pin) (VAL_TO_TEMP((double)analogRead(pin)))

double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Timer variables
unsigned long sampleTimer = 0;
unsigned long pidTimer = 0;
unsigned long turnOffTimer = 0;
unsigned long turnOffTime = 0;

// State variables
bool running = false;

// Initialise circular buffer
CircularBuffer temperatureBuf = CircularBuffer(TEMPERATURE_DATA_LEN);

DNSServer dnsServer;
AsyncWebServer server(80);

// Function declarations
void allRelaysOff();

void setup()
{
    // Setup serial interface for logging and debugging
    Serial.begin(115200);

    // Initialise the filesystem where all the web files are
    Serial.print("Starting filesystem... ");
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    Serial.println("OK");

    // Setup PID controller
    Serial.print("Starting PID... ");
    pinMode(TEMP1_PIN, INPUT);
    pinMode(TEMP2_PIN, INPUT);
    Input = GET_TEMP(TEMP1_PIN);
    Setpoint = DEFAULT_TEMPERATURE;
    myPID.SetMode(AUTOMATIC);
    Serial.println("... OK");

    // Setup relays
    Serial.print("Preparing relays... ");
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(RELAY4_PIN, OUTPUT);
    allRelaysOff();
    Serial.println("OK");

    // Starting WiFi access point
    Serial.print("Starting AP... ");
    WiFi.softAP("esp-captive");
    Serial.println("OK");

    // Starting DNS server
    Serial.print("Starting DNS... ");
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("OK");
    
    // Setting up API endpoints
    Serial.print("Setting up HTTP handlers");

    // Make sure we can access regular files from the server
    server.serveStatic("/", SPIFFS, "/");

    server.on("/api/v1/get_data", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncResponseStream *response = 
                    request->beginResponseStream("text/plain");
                  double temps[temperatureBuf.len];
                  temperatureBuf.getBuf(temps);
                  for (int i = 0; i < temperatureBuf.len; i++)
                  {
                      response->printf(" %.1f", temps[i]);
                  }
                  request->send(response);
              });
    server.on("/api/v1/get_timestep",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                request->send(200, "text/plain", String(SAMPLE_TIME));
              });
    server.on("/api/v1/get_time",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                request->send(200, "text/plain", String(turnOffTime));
              });
    server.on("/api/v1/get_elapsed_time",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                request->send(200, "text/plain",
                              String(millis() - turnOffTimer));
              });
    server.on("/api/v1/get_temp",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                request->send(200, "text/plain", String(Setpoint));
              });
    server.on("/api/v1/start",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                if (running)
                {
                    request->send(200, "text/plain", "ERROR RUNNING");
                    return;
                }
                turnOffTimer = millis();
                running = true;
                request->send(200, "text/plain", "OK");
              });
    server.on("/api/v1/stop",
              HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                if (!running)
                {
                    request->send(200, "text/plain", "ERROR STOPPED");
                    return;
                }
                allRelaysOff();
                running = false;
                request->send(200, "text/plain", "OK");
              });
    server.on("/api/v1/set_temp",
              HTTP_POST,
              NULL,
              NULL,
              [](AsyncWebServerRequest *request, unsigned char* data,
                 unsigned int len, unsigned int index, unsigned int total) {
                if (running)
                {
                    request->send(200, "text/plain", "ERROR RUNNING");
                    return;
                }
                char body[len+1];
                for (int i = 0; i < len; i++)
                {
                    body[i] = data[index+i];
                }
                body[len] = 0;
                Setpoint = String(body).toDouble();
                request->send(200, "text/plain", String(Setpoint));
              });
    server.on("/api/v1/set_time",
              HTTP_POST,
              NULL,
              NULL,
              [](AsyncWebServerRequest *request, unsigned char* data,
                 unsigned int len, unsigned int index, unsigned int total) {
                if (running)
                {
                    request->send(200, "text/plain", "ERROR RUNNING");
                    return;
                }
                char body[len+1];
                for (int i = 0; i < len; i++)
                {
                    body[i] = data[index+i];
                }
                body[len] = 0;
                turnOffTime = String(body).toDouble();
                request->send(200, "text/plain", String(turnOffTime));
              });
    Serial.println("... OK");

    Serial.print("Starting web server");
    server.begin();
    Serial.println("... OK");
}

void loop()
{
    Input = GET_TEMP(TEMP1_PIN);

    // Only compute PID and turn on relay if running
    if (running) {
        if (millis() - pidTimer > PID_TIME)
        {
            myPID.Compute();
            if (Output > 128)
            {
                digitalWrite(RELAY1_PIN, HIGH);
            }
            else
            {
                digitalWrite(RELAY1_PIN, LOW);
            }
        }
    }

    // Only save data every SAMPLE_TIME milliseconds
    if (millis() - sampleTimer > SAMPLE_TIME)
    {
        sampleTimer = millis();
        temperatureBuf.put(Input);
    }
    // Default PID output 0-255 (default Arduino PWM range). Only turn on relay
    // if the output is above half
    dnsServer.processNextRequest();
}

void allRelaysOff()
{
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    digitalWrite(RELAY4_PIN, LOW);
}