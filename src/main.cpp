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

// Relay control pins
#define RELAY1_PIN 7
#define RELAY2_PIN 6
#define RELAY3_PIN 5
#define RELAY4_PIN 4

// Temperature ADC pins
#define TEMP1_PIN 34
#define TEMP2_PIN 35

// General constants
#define TEMPERATURE_DATA_LEN 100
#define SAMPLE_TIME 100
#define DEFAULT_TEMPERATURE 70

// OLD LOG APPROX
// Not as good as the new cubic approx
/*
#define m 99.1104
#define l 833.729
#define b -673.374

#define VAL_TO_TEMP(x) (m * log(x + l) + b)
*/

// Constants for the cubic approximation model
#define a 4.4037e-9
#define b -2.3798e-5
#define c 6.6659e-2
#define d -2.0150e1

// Converts analogRead values to a temperature
#define VAL_TO_TEMP(x) (x * (c + x * (b + x * a)) + d)
#define GET_TEMP(pin) (VAL_TO_TEMP((double)analogRead(pin)))

double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);


// Implement a circular buffer for the temperature sampling.
// By using a circular buffer we can be sampling data indefinently without
// having to worry about buffer length.
class CircularBuffer
{
    private:
    double *data;
    int ptr;

    public:
    int len;

    CircularBuffer(int size)
    {
        len = size;
        ptr = 0;
        data = new double[len];
        data = {0};
    }

    void put(double value)
    {
        if (data == nullptr)
        {
            data = new double[len];
            ptr = 0;
        }
        data[ptr] = value;
        ptr = (ptr+1) % len;
    }

    void getBuf(double *tmp)
    {
        for (int i = 0; i < len; i++)
        {
            tmp[i] = data[(i + ptr) % len];
        }
    }
    
    ~CircularBuffer()
    {
        if(data != nullptr){
            delete data;
        }
    }
};


// Timer variables
unsigned long long sampleTimer;

// Initialise circular buffer
CircularBuffer temperatureBuf = CircularBuffer(TEMPERATURE_DATA_LEN);

DNSServer dnsServer;
AsyncWebServer server(80);

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
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html"); });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/style.css", "text/stylesheet"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/script.js", "text/javascript"); });

    server.on("/api/v1/get_data", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncResponseStream *response = request->beginResponseStream("text/plain");
                  double temps[temperatureBuf.len];
                  temperatureBuf.getBuf(temps);
                  for (int i = 0; i < temperatureBuf.len; i++)
                  {
                      response->printf(" %.1f", temps[i]);
                  }
                  request->send(response);
              });
    server.on("/api/v1/get_timestep", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                request->send(200, "text/plain", String(SAMPLE_TIME));
              });
    server.on("/api/v1/set_temp",
              HTTP_POST,
              [](AsyncWebServerRequest *request){}, // On request
              NULL,                                 // On upload
              [](AsyncWebServerRequest *request, unsigned char* data, unsigned int len, unsigned int index, unsigned int total) { // On body
                char body[len+1];
                for (int i = 0; i < len; i++)
                {
                    body[i] = data[index+i];
                }
                body[len] = 0;
                Setpoint = String(body).toDouble();
                request->send(200, "text/plain", String(Setpoint));
              });
    Serial.println("... OK");
    Serial.print("Starting web server");
    server.begin();
    Serial.println("... OK");
}

void loop()
{
    // put your main code here, to run repeatedly:
    Input = GET_TEMP(TEMP1_PIN);
    myPID.Compute();
    if (millis() - sampleTimer > SAMPLE_TIME)
    {
        sampleTimer = millis();
        temperatureBuf.put(Input);
    }
    if (Output > 128)
    {
        digitalWrite(RELAY1_PIN, HIGH);
    }
    else
    {
        digitalWrite(RELAY1_PIN, LOW);
    }
    dnsServer.processNextRequest();
}