#include <OneWire.h>

#include "ConfigManager.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

int DS18S20_Pin = 4; //DS18S20 Signal pin on digital 2

//Temperature chip i/o
OneWire ds(DS18S20_Pin); // on digital pin 2

// Update these with values suitable for your network.

#define RESET_BUTTON 5

struct Config
{
  char name[20];
  bool enabled;
  int8_t hour;
  char password[20];
  char mqtt_server[20];
} config;

struct Metadata
{
  int8_t version;
} meta;

ConfigManager configManager;

const char *ssid = config.name;
const char *password = config.password;
const char *mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void createCustomRoute(WebServer *server)
{
  Serial.println("Servier on");
  server->on("/custom", HTTPMethod::HTTP_GET, [server]() {
    server->send(200, "text/plain", "Hello, World!");
  });
}

void setup()
{
  pinMode(RESET_BUTTON, INPUT);

  Serial.begin(115200);
  Serial.println("Demo");

  meta.version = 3;

  // Setup config manager
  configManager.setAPName("Demo");
  configManager.setAPFilename("/index.html");
  configManager.addParameter("name", config.name, 20);
  configManager.addParameter("enabled", &config.enabled);
  configManager.addParameter("hour", &config.hour);
  configManager.addParameter("password", config.password, 20, set);
  configManager.addParameter("version", &meta.version, get);

  configManager.setAPCallback(createCustomRoute);
  configManager.setAPICallback(createCustomRoute);

  configManager.begin(config);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  configManager.loop();

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (digitalRead(RESET_BUTTON))
  {
    EEPROM.put(0, "  ");
    EEPROM.commit();

    ESP.restart();
  }

  long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;

    float temperature = getTemp(); //will take about 750ms to run
    Serial.println(temperature);
    snprintf(msg, sizeof(msg), "%3.2f", temperature);
    Serial.print("Publish temperature: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
  }

  // Add your loop code here
}

float getTemp()
{
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if (!ds.search(addr))
  {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if (OneWire::crc8(addr, 7) != addr[7])
  {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if (addr[0] != 0x10 && addr[0] != 0x28)
  {
    Serial.print("Device is not recognized");
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  delay(750); // Wait for temperature conversion to complete

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++)
  { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;
}
