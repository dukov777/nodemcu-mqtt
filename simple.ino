
#include "ConfigManager.h"

#define LED 2
#define PIN_RESET 5

struct Config {
    char name[20];
    bool enabled;
    int8_t hour;
    char password[20];
} config;

struct Metadata {
    int8_t version;
} meta;

ConfigManager configManager;

void createCustomRoute(WebServer *server) {
    Serial.println("Servier on");
    server->on("/custom", HTTPMethod::HTTP_GET, [server](){
        server->send(200, "text/plain", "Hello, World!");
    });
}

#define RESET_BUTTON 5

void setup() {
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


    //
}

void loop() {
    configManager.loop();

    if(digitalRead(RESET_BUTTON)) {
        EEPROM.put(0, "  ");
        EEPROM.commit();
        
        ESP.restart();
    }

    // Add your loop code here
}
