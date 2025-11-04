#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MQTTManager {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    String hardwareId;
    String mqttUser;
    String mqttPass;
    int dispenserId;
    bool dispenserIdReceived;
    unsigned long lastRegistrationAttempt;
    unsigned long registrationInterval = 30000; // 30 seconds

    // Current wristband session data
    int currentWristbandId;
    int currentWristbandActivityId;
    String currentRfidTag;
    int currentDispenserBeverageId;
    float currentVolumeDispensed;

    // Callback function pointers
    void (*onDispenserIdReceived)(int);
    void (*onBeveragesReceived)(const char*);
    void (*onWristbandVerified)(bool, const char*);
/*
// Subscribe to topics
                await _mqttService.SubscribeAsync("disp/reg-hard-id");
                await _mqttService.SubscribeAsync("disp-bev/get-bev");
                await _mqttService.SubscribeAsync("wrist/is-active");
                await _mqttService.SubscribeAsync("wrist/reg-act-det");
*/

public:
    MQTTManager();

    void init(const char* broker, int port, String hwId, const char* username, const char* password);
    void connect();
    void loop();
    bool isConnected();

    // Registration and setup
    void registerHardware();
    void requestBeverages();

    // Wristband verification and activity
    void verifyWristband(String rfidTag);
    void registerConsumption(int dispenserBeverageId, float volumeMl);  // Register final consumption
    void setCurrentBeverage(int dispenserBeverageId);  // Set the selected beverage for this session
    void addDispensedVolume(float volumeMl);  // Track volume during dispensing

    // Callback setters
    void setDispenserIdCallback(void (*callback)(int));
    void setBeveragesCallback(void (*callback)(const char*));
    void setWristbandCallback(void (*callback)(bool, const char*));

    // MQTT callback
    void messageCallback(char* topic, byte* payload, unsigned int length);

    // Getters
    int getDispenserId() { return dispenserId; }
    bool hasDispenserId() { return dispenserIdReceived; }
    int getWristbandActivityId() { return currentWristbandActivityId; }
    int getWristbandId() { return currentWristbandId; }
    String getCurrentRfidTag() { return currentRfidTag; }
    float getCurrentVolumeDispensed() { return currentVolumeDispensed; }
    bool hasActiveSession() { return currentWristbandActivityId > 0; }

    // Session management
    void clearWristbandSession();
};

#endif