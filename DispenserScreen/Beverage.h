#ifndef BEVERAGE_H
#define BEVERAGE_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct Beverage {
  int dispenser_beverage_id;
  int dispenser_valve;
  int beverage_id;
  String beverage_name;
  String beverage_description;
  float unit_price;
  int unit;
  String beverage_color_code;
  String beverage_image_url;  // For URL-based images
  String beverage_image_b64;   // For base64 encoded images (stored but may not display due to memory limits)
  bool enabled;
};

class BeverageManager {
private:
  Beverage beverages[10];
  int beverageCount;
  int selectedIndex;

public:
  BeverageManager() : beverageCount(0), selectedIndex(-1) {}

  void clearBeverages() {
    beverageCount = 0;
    selectedIndex = -1;
  }

  void addBeverage(int valve, const char* name, const char* colorCode, float price) {
    if (beverageCount >= 10) return;

    beverages[beverageCount].dispenser_valve = valve;
    beverages[beverageCount].beverage_name = String(name);
    beverages[beverageCount].beverage_color_code = String(colorCode);
    beverages[beverageCount].unit_price = price;
    beverages[beverageCount].enabled = true;
    beverageCount++;
  }

  bool loadFromJson(String jsonString) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
      return false;
    }
    
    if (!doc["success"].as<bool>()) {
      Serial.println("API returned success=false");
      return false;
    }
    
    JsonArray data = doc["data"].as<JsonArray>();
    beverageCount = 0;
    
    for (JsonObject beverage : data) {
      if (!beverage["enabled"].as<bool>()) continue;

      beverages[beverageCount].dispenser_beverage_id = beverage["dispenser_beverage_id"];
      beverages[beverageCount].dispenser_valve = beverage["dispenser_valve"];
      beverages[beverageCount].beverage_id = beverage["beverage_id"];
      beverages[beverageCount].beverage_name = beverage["beverage_name"].as<String>();
      beverages[beverageCount].beverage_description = beverage["beverage_description"].as<String>();
      beverages[beverageCount].unit_price = beverage["unit_price"];
      beverages[beverageCount].unit = beverage["unit"];
      beverages[beverageCount].beverage_color_code = beverage["beverage_color_code"].as<String>();

      // Handle image data - prefer URL over base64 for memory efficiency
      if (beverage.containsKey("beverage_image") && !beverage["beverage_image"].isNull()) {
        beverages[beverageCount].beverage_image_url = beverage["beverage_image"].as<String>();
      }
      // Only store base64 if no URL and it's reasonably small (to avoid memory issues)
      else if (beverage.containsKey("beverage_image_b64") && !beverage["beverage_image_b64"].isNull()) {
        String b64 = beverage["beverage_image_b64"].as<String>();
        if (b64.length() < 10000) {  // Limit to ~10KB base64 strings
          beverages[beverageCount].beverage_image_b64 = b64;
        }
      }

      beverages[beverageCount].enabled = beverage["enabled"];

      beverageCount++;
      if (beverageCount >= 10) break;
    }
    
    Serial.printf("Loaded %d beverages\n", beverageCount);
    return true;
  }
  
  int getCount() { return beverageCount; }
  
  Beverage* getBeverage(int index) {
    if (index >= 0 && index < beverageCount) {
      return &beverages[index];
    }
    return nullptr;
  }
  
  void setSelected(int index) { selectedIndex = index; }
  
  int getSelected() { return selectedIndex; }
  
  Beverage* getSelectedBeverage() {
    return getBeverage(selectedIndex);
  }
  
  uint32_t hexToColor(String hexColor) {
    if (hexColor.startsWith("#")) hexColor = hexColor.substring(1);
    return strtoul(hexColor.c_str(), NULL, 16);
  }
};

#endif