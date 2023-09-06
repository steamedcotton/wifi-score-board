#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include "HomePage.h"

#define ONBOARD_LED 16

#define CLOCK 13
#define LATCH 12
#define DATA 14

const char *ssid = "<SSID>";
const char *password = "<PASSWORD>";

ESP8266WebServer server(80);

//Given a number, or '-', shifts it out to the display
void postNumber(byte number, boolean decimal)
{

  Serial.print("Printing number: ");
  Serial.println(number);

  //    -  A
  //   / / F/B
  //    -  G
  //   / / E/C
  //    -. D/DP

#define a  1<<0
#define b  1<<6
#define c  1<<5
#define d  1<<4
#define e  1<<3
#define f  1<<1
#define g  1<<2
#define dp 1<<7

  byte segments;

  switch (number)
  {
    case 1: segments = b | c; break;
    case 2: segments = a | b | d | e | g; break;
    case 3: segments = a | b | c | d | g; break;
    case 4: segments = f | g | b | c; break;
    case 5: segments = a | f | g | c | d; break;
    case 6: segments = a | f | g | e | c | d; break;
    case 7: segments = a | b | c; break;
    case 8: segments = a | b | c | d | e | f | g; break;
    case 9: segments = a | b | c | d | f | g; break;
    case 0: segments = a | b | c | d | e | f; break;
    case ' ': segments = 0; break;
    case 'c': segments = g | e | d; break;
    case '-': segments = g; break;
    default: segments = 0; break;
  }

  if (decimal) segments |= dp;

  //Clock these bits out to the drivers
  for (byte x = 0 ; x < 8 ; x++)
  {
    digitalWrite(CLOCK, LOW);
    digitalWrite(DATA, segments & 1 << (7 - x));
    digitalWrite(CLOCK, HIGH); //Data transfers to the register on the rising edge of SRCK
  }
}

//Takes a number and displays 4 numbers. Displays absolute value (no negatives)
void showNumber(float value)
{
  int number = abs(value); //Remove negative signs and any decimals

  for (byte x = 0 ; x < 4 ; x++)
  {
    int remainder = number % 10;

    postNumber(remainder, false);

    number /= 10;
  }

  // Latch the current segment data
  digitalWrite(LATCH, LOW);
  digitalWrite(LATCH, HIGH); // Register moves storage register on the rising edge of RCK
}


// Serve up the Home Page
void homepage() {
  Serial.println("Sending home webpage");
  server.send(200, "text/html", PAGE_MAIN);
}

// Serving Hello world
void updateNumber()
{
  String postBody = server.arg("plain");
  Serial.println(postBody);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  if (error)
  {
    // if the file didn't open, print an error:
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());

    String msg = error.c_str();

    server.send(400, F("text/html"),
                "Error in parsing json body! <br>" + msg);
  }
  else
  {
    JsonObject postObj = doc.as<JsonObject>();

    if (postObj.containsKey("number"))
    {
      float newNumber = postObj["number"];
      Serial.print(F("Got a new number: "));
      Serial.println(newNumber);
      showNumber(newNumber);

      DynamicJsonDocument doc(512);
      doc["status"] = "OK";

      Serial.print(F("Stream..."));
      String buf;
      serializeJson(doc, buf);

      server.send(201, F("application/json"), buf);
      Serial.print(F("done."));
    }
    else
    {
      DynamicJsonDocument doc(512);
      doc["status"] = "KO";
      doc["message"] = F("No data found, or incorrect!");

      Serial.print(F("Stream..."));
      String buf;
      serializeJson(doc, buf);

      server.send(400, F("application/json"), buf);
      Serial.print(F("done."));
    }
  }

  // server.send(200, "text/json", "{\"name\": \"Hello world\"}");

  // Blink LED
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.println("LED on");
  delay(500);
  digitalWrite(ONBOARD_LED, LOW);
  Serial.println("LED off");
  delay(500);
}

// Define routing
void restServerRouting()
{
  server.on("/", HTTP_GET, homepage);
  server.on(F("/update-number"), HTTP_POST, updateNumber);
}

// Manage not found URL
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void)
{
  Serial.begin(115200);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);

  // Setup LED pins
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(LATCH, OUTPUT);
  digitalWrite(CLOCK, LOW);
  digitalWrite(DATA, LOW);
  digitalWrite(LATCH, LOW);

  // Setup WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{
  server.handleClient();
}
