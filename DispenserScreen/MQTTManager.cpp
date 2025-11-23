#include "MQTTManager.h"

MQTTManager::MQTTManager() : mqttClient(wifiClient) {
    dispenserId = -1;
    dispenserIdReceived = false;
    lastRegistrationAttempt = 0;
    onDispenserIdReceived = nullptr;
    onBeveragesReceived = nullptr;
    onWristbandVerified = nullptr;

    // Initialize wristband session data
    currentWristbandId = -1;
    currentWristbandActivityId = -1;
    currentRfidTag = "";
    currentDispenserBeverageId = -1;
    currentVolumeDispensed = 0.0;
}

void MQTTManager::init(const char* broker, int port, String hwId, const char* username, const char* password){
    hardwareId = hwId;
    mqttUser = username;
    mqttPass = password;

    // Increase MQTT buffer size for larger messages (default is usually 256)
    mqttClient.setBufferSize(4096);
    Serial.print("MQTT buffer size set to: ");
    Serial.println(mqttClient.getBufferSize());

    mqttClient.setServer(broker, port);

    // Set up callback with proper binding
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->messageCallback(topic, payload, length);
    });
}

void MQTTManager::connect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }

    if (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT broker...");
        String clientId = "Dispenser-" + hardwareId;

        if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
            Serial.println(" connected!");
            Serial.println("\n===== SUBSCRIBING TO TOPICS =====");
            Serial.print("Hardware ID: ");
            Serial.println(hardwareId);

            // Subscribe to response topics - these are the actual topics from the server
            // The server will publish to these topics with the hardware ID appended
            String hardwareResponseTopic = "disp/reg-hard-id/resp/" + hardwareId;
            if (mqttClient.subscribe(hardwareResponseTopic.c_str())) {
                Serial.println("✓ Subscribed to: " + hardwareResponseTopic);
            } else {
                Serial.println("✗ FAILED to subscribe to: " + hardwareResponseTopic);
            }

            // Subscribe to beverages response
            String dispenserBeveragesResponseTopic = "disp-bev/get-bev/resp/" + hardwareId;
            if (mqttClient.subscribe(dispenserBeveragesResponseTopic.c_str())) {
                Serial.println("✓ Subscribed to: " + dispenserBeveragesResponseTopic);
            } else {
                Serial.println("✗ FAILED to subscribe to: " + dispenserBeveragesResponseTopic);
            }

            // Subscribe to wristband verification response
            String wristbandIsActiveResponseTopic = "wrist/is-active/resp/" + hardwareId;
            if (mqttClient.subscribe(wristbandIsActiveResponseTopic.c_str())) {
                Serial.println("✓ Subscribed to: " + wristbandIsActiveResponseTopic);
            } else {
                Serial.println("✗ FAILED to subscribe to: " + wristbandIsActiveResponseTopic);
            }

            // Also subscribe to activity detail registration response
            String wristRegActDetailResponseTopic = "wrist/reg-act-det/resp/" + hardwareId;
            if (mqttClient.subscribe(wristRegActDetailResponseTopic.c_str())) {
                Serial.println("✓ Subscribed to: " + wristRegActDetailResponseTopic);
            } else {
                Serial.println("✗ FAILED to subscribe to: " + wristRegActDetailResponseTopic);
            }

            // // Also try subscribing to wildcard to see ALL messages (for debugging)
            // if (mqttClient.subscribe("#")) {
            //     Serial.println("✓ Subscribed to ALL topics (#) for debugging");
            // }
            Serial.println("================================\n");

        } else {
            Serial.print(" failed, rc=");
            Serial.println(mqttClient.state());
        }
    }
}

void MQTTManager::loop() {
    static unsigned long lastStatusCheck = 0;
    static bool wasConnected = false;

    bool currentlyConnected = mqttClient.connected();

    // Log connection state changes
    if (currentlyConnected != wasConnected) {
        if (currentlyConnected) {
            Serial.println("[MQTT] ✓ Connected to broker");
        } else {
            Serial.println("[MQTT] ✗ Disconnected from broker - attempting reconnect...");
        }
        wasConnected = currentlyConnected;
    }

    if (!currentlyConnected) {
        connect();
    }

    mqttClient.loop();

    // Periodic status check every 10 seconds
    if (millis() - lastStatusCheck > 10000) {
        lastStatusCheck = millis();
        if (currentlyConnected) {
            Serial.print("[MQTT] Status: Connected | Dispenser ID: ");
            if (dispenserIdReceived) {
                Serial.println(dispenserId);
            } else {
                Serial.println("Pending");
            }
        }
    }

    // Auto-register hardware every 30 seconds if we don't have dispenser ID
    if (!dispenserIdReceived && currentlyConnected) {
        if (millis() - lastRegistrationAttempt > registrationInterval) {
            Serial.println("[MQTT] No dispenser ID yet, retrying registration...");
            registerHardware();
        }
    }
}

bool MQTTManager::isConnected() {
    return mqttClient.connected();
}

void MQTTManager::registerHardware() {
    if (!isConnected()) {
        Serial.println("MQTT not connected, cannot register hardware");
        return;
    }

    StaticJsonDocument<256> doc;
    JsonObject userRequest = doc.createNestedObject("user_request");
    userRequest["user_id"] = 1;  // Default user_id, might need to be configurable

    JsonObject filter = doc.createNestedObject("filter");
    filter["dispenser_hardware_id"] = hardwareId;

    char buffer[512];
    serializeJson(doc, buffer);

    String topic = "disp/reg-hard-id/" + hardwareId;
    Serial.println("\n===== PUBLISHING HARDWARE REGISTRATION =====");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(buffer);
    Serial.print("Expected response on: disp/reg-hard-id/resp/");
    Serial.println(hardwareId);

    if (mqttClient.publish(topic.c_str(), buffer)) {
        Serial.println("✓ Published successfully");
    } else {
        Serial.println("✗ FAILED to publish");
    }
    Serial.println("============================================\n");
    lastRegistrationAttempt = millis();
}

void MQTTManager::requestBeverages() {
    if (!isConnected() || !dispenserIdReceived) {
        Serial.println("Cannot request beverages - no dispenser ID or not connected");
        return;
    }

    StaticJsonDocument<256> doc;
    JsonObject userRequest = doc.createNestedObject("user_request");
    userRequest["user_id"] = 1;

    JsonObject filter = doc.createNestedObject("filter");
    filter["dispenser_id"] = dispenserId;
    filter["beverage_enabled"] = true;

    char buffer[512];
    serializeJson(doc, buffer);

    Serial.println("Requesting beverages: " + String(buffer));
    String topic = "disp-bev/get-bev/" + hardwareId;
    Serial.println(topic);
    mqttClient.publish(topic.c_str(), buffer);
}

void MQTTManager::verifyWristband(String rfidTag) {
    if (!isConnected() || !dispenserIdReceived) {
        Serial.println("Cannot verify wristband - no dispenser ID or not connected");
        return;
    }

    // Store the RFID tag for this session
    currentRfidTag = rfidTag;

    StaticJsonDocument<512> doc;
    JsonObject userRequest = doc.createNestedObject("user_request");
    userRequest["user_id"] = 1;

    JsonObject filter = doc.createNestedObject("filter");
    filter["wristband_code"] = rfidTag;  // Changed from 'rfid' to 'wristband_code'

    char buffer[512];
    serializeJson(doc, buffer);

    String topic = "wrist/is-active/" + hardwareId;
    Serial.println("Verifying wristband on topic: " + topic);
    Serial.println("Payload: " + String(buffer));
    mqttClient.publish(topic.c_str(), buffer);
}

void MQTTManager::messageCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("\n========== MQTT MESSAGE RECEIVED ==========");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Length: ");
    Serial.println(length);

    // Check if message is too large
    const size_t MAX_MESSAGE_SIZE = 6020;
    if (length > MAX_MESSAGE_SIZE) {
        Serial.print("ERROR: Message too large! Size: ");
        Serial.print(length);
        Serial.print(" > Max: ");
        Serial.println(MAX_MESSAGE_SIZE);
        return;
    }

    // Allocate buffer on heap for larger messages
    char* message = new char[length + 1];
    if (!message) {
        Serial.println("ERROR: Failed to allocate memory for message");
        return;
    }

    memcpy(message, payload, length);
    message[length] = '\0';

    Serial.print("Payload: ");
    // Print first 200 chars for debugging if message is long
    if (length > 200) {
        char preview[201];
        memcpy(preview, message, 200);
        preview[200] = '\0';
        Serial.print(preview);
        Serial.println("... [truncated for display]");
    } else {
        Serial.println(message);
    }

    // Show expected topics for debugging
    Serial.println("Expected topics:");
    Serial.println("  - disp/reg-hard-id/resp/" + hardwareId);
    Serial.println("  - disp-bev/get-bev/resp/" + hardwareId);
    Serial.println("  - wrist/is-active/resp/" + hardwareId);
    Serial.println("  - wrist/reg-act-det/resp/" + hardwareId);

    String topicStr = String(topic);
    Serial.print("Checking topic: ");
    Serial.println(topicStr);

    // Check for hardware ID response
    if (topicStr == "disp/reg-hard-id/resp/" + hardwareId) {
        Serial.println("Processing hardware ID response...");
        // Use DynamicJsonDocument for larger responses
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, message);

        if (!error) {
            Serial.println("JSON parsed successfully");
            if (doc["success"] == true && doc.containsKey("data")) {
                JsonArray dataArray = doc["data"];
                if (dataArray.size() > 0) {
                    JsonObject dispenserData = dataArray[0];
                    dispenserId = dispenserData["dispenser_id"];
                    dispenserIdReceived = true;

                    Serial.print("Received Dispenser ID: ");
                    Serial.println(dispenserId);

                    const char* dispenserName = dispenserData["dispenser_name"];
                    Serial.print("Dispenser Name: ");
                    Serial.println(dispenserName);

                    if (onDispenserIdReceived) {
                        onDispenserIdReceived(dispenserId);
                    }

                    // Once we have the dispenser ID, request beverages
                    requestBeverages();
                } else {
                    Serial.println("No dispenser data in response");
                }
            } else {
                Serial.println("Registration failed or no data in response");
            }
        } else {
            Serial.print("Failed to parse hardware response: ");
            Serial.println(error.c_str());
            Serial.print("JSON size might be too large. Message length: ");
            Serial.println(strlen(message));
        }
    }
    // Check for beverages response
    else if (topicStr == "disp-bev/get-bev/resp/" + hardwareId) {
        Serial.println("Processing beverages response...");
        Serial.print("Beverages data length: ");
        Serial.println(strlen(message));
        if (onBeveragesReceived) {
            onBeveragesReceived(message);
        }
    }
    // Check for wristband verification response
    else if (topicStr == "wrist/is-active/resp/" + hardwareId) {
        Serial.println("Processing wristband verification response...");
        Serial.print("Wristband data length: ");
        Serial.println(strlen(message));

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, message);

        if (!error) {
            bool success = doc["success"] | false;

            if (success && doc.containsKey("data")) {
                JsonObject data = doc["data"];
                bool isActive = data["is_active"] | false;

                if (isActive) {
                    // Store wristband_activity_id for the dispensing session
                    currentWristbandActivityId = data["wristband_activity_id"] | -1;
                    currentWristbandId = data["wristband_id"] | -1;

                    Serial.print("Wristband active! Activity ID: ");
                    Serial.print(currentWristbandActivityId);
                    Serial.print(", Wristband ID: ");
                    Serial.println(currentWristbandId);
                }

                if (onWristbandVerified) {
                    onWristbandVerified(isActive, message);
                }
            } else {
                // Handle error response
                if (doc.containsKey("error_info")) {
                    JsonObject errorInfo = doc["error_info"];
                    const char* errorMsg = errorInfo["err_message"] | "Unknown error";
                    int errorCode = errorInfo["err_code"] | 0;

                    Serial.print("Wristband verification failed - Error ");
                    Serial.print(errorCode);
                    Serial.print(": ");
                    Serial.println(errorMsg);
                }

                if (onWristbandVerified) {
                    onWristbandVerified(false, message);
                }
            }
        } else {
            Serial.print("Failed to parse wristband response: ");
            Serial.println(error.c_str());
        }
    }
    // Check for activity registration response
    else if (topicStr == "wrist/reg-act-det/resp/" + hardwareId) {
        Serial.println("Processing consumption registration response...");
        Serial.print("Response length: ");
        Serial.println(strlen(message));

        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, message);

        if (!error) {
            if (doc["success"] == true) {
                if (doc.containsKey("data") && doc["data"].containsKey("numeric_id")) {
                    int numericId = doc["data"]["numeric_id"];
                    Serial.print("Consumption registered with ID: ");
                    Serial.println(numericId);
                }
                Serial.println("Use cycle complete - ready for cleaning");
            } else {
                Serial.println("Failed to register consumption");
                if (doc.containsKey("error_info")) {
                    JsonObject errorInfo = doc["error_info"];
                    const char* errorMsg = errorInfo["err_message"] | "Unknown error";
                    Serial.print("Error: ");
                    Serial.println(errorMsg);
                }
            }
        }
    }
    else {
        Serial.print("⚠ Unknown/unhandled topic: ");
        Serial.println(topicStr);
    }

    // Clean up allocated memory
    delete[] message;

    Serial.println("============================================\n");
}

void MQTTManager::setDispenserIdCallback(void (*callback)(int)) {
    onDispenserIdReceived = callback;
}

void MQTTManager::setBeveragesCallback(void (*callback)(const char*)) {
    onBeveragesReceived = callback;
}

void MQTTManager::setWristbandCallback(void (*callback)(bool, const char*)) {
    onWristbandVerified = callback;
}

void MQTTManager::clearWristbandSession() {
    currentWristbandId = -1;
    currentWristbandActivityId = -1;
    currentRfidTag = "";
    currentDispenserBeverageId = -1;
    currentVolumeDispensed = 0.0;
    Serial.println("Wristband session cleared");
}

void MQTTManager::registerConsumption(int dispenserBeverageId, float volumeMl) {
    if (!isConnected() || !dispenserIdReceived) {
        Serial.println("Cannot register consumption - no dispenser ID or not connected");
        return;
    }

    if (currentWristbandActivityId < 0) {
        Serial.println("Cannot register consumption - no active wristband session");
        return;
    }

    StaticJsonDocument<512> doc;
    JsonObject userRequest = doc.createNestedObject("user_request");
    userRequest["user_id"] = 1;

    JsonObject data = doc.createNestedObject("data");
    data["wristband_activity_id"] = currentWristbandActivityId;
    data["dispenser_beverage_id"] = dispenserBeverageId;
    data["volume_ml"] = volumeMl;

    char buffer[512];
    serializeJson(doc, buffer);

    String topic = "wrist/reg-act-det/" + hardwareId;
    Serial.println("Registering consumption on topic: " + topic);
    Serial.print("Activity ID: ");
    Serial.print(currentWristbandActivityId);
    Serial.print(", Beverage ID: ");
    Serial.print(dispenserBeverageId);
    Serial.print(", Volume: ");
    Serial.print(volumeMl);
    Serial.println(" ml");
    Serial.println("Payload: " + String(buffer));

    if (mqttClient.publish(topic.c_str(), buffer)) {
        Serial.println("Consumption registered successfully");
    } else {
        Serial.println("Failed to publish consumption");
    }
}

void MQTTManager::setCurrentBeverage(int dispenserBeverageId) {
    currentDispenserBeverageId = dispenserBeverageId;
    Serial.print("Current beverage set to: ");
    Serial.println(dispenserBeverageId);
}

void MQTTManager::addDispensedVolume(float volumeMl) {
    currentVolumeDispensed += volumeMl;
    Serial.print("Volume dispensed: ");
    Serial.print(currentVolumeDispensed);
    Serial.println(" ml total");
}