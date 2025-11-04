#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
//#include <HardwareSerial.h>

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "Beverage.h"
#include "UIBuilder.h"
#include "MQTTManager.h"

// #define SCREEN_WIDTH  800
// #define SCREEN_HEIGHT 480

// //pin definition
// #define TFT_BL_PIN 2
// #define RX_RFREADER_PIN 38  // Your RX pin
// #define TX_RFREADER_PIN -1  // Not needed for RDM6300, set to -1

// Define LGFX class FIRST
class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;

  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      cfg.pin_d0  = GPIO_NUM_15;
      cfg.pin_d1  = GPIO_NUM_7;
      cfg.pin_d2  = GPIO_NUM_6;
      cfg.pin_d3  = GPIO_NUM_5;
      cfg.pin_d4  = GPIO_NUM_4;
      cfg.pin_d5  = GPIO_NUM_9;
      cfg.pin_d6  = GPIO_NUM_46;
      cfg.pin_d7  = GPIO_NUM_3;
      cfg.pin_d8  = GPIO_NUM_8;
      cfg.pin_d9  = GPIO_NUM_16;
      cfg.pin_d10 = GPIO_NUM_1;
      cfg.pin_d11 = GPIO_NUM_14;
      cfg.pin_d12 = GPIO_NUM_21;
      cfg.pin_d13 = GPIO_NUM_47;
      cfg.pin_d14 = GPIO_NUM_48;
      cfg.pin_d15 = GPIO_NUM_45;
      cfg.pin_henable = GPIO_NUM_41;
      cfg.pin_vsync = GPIO_NUM_40;
      cfg.pin_hsync = GPIO_NUM_39;
      cfg.pin_pclk = GPIO_NUM_0;
      cfg.freq_write = 12000000;
      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch = 40;
      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch = 13;
      cfg.pclk_active_neg = 1;
      cfg.de_idle_high = 0;
      cfg.pclk_idle_high = 0;
      _bus_instance.config(cfg);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width = SCREEN_WIDTH;
      cfg.memory_height = SCREEN_HEIGHT;
      cfg.panel_width = SCREEN_WIDTH;
      cfg.panel_height = SCREEN_HEIGHT;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      _panel_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

// Declare lcd object
LGFX lcd;

// NOW include touch.h - it can see lcd
#include "touch.h"

// Then include ui.h
#include "ui.h"

// Global instances
BeverageManager beverageManager;
UIBuilder uiBuilder(&beverageManager);
MQTTManager mqttManager;

// Forward declarations
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, unsigned char* data);
void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data);

static uint16_t draw_buf[DRAW_BUF_SIZE / sizeof(uint16_t)];
uint32_t lastTick = 0;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, unsigned char* data) {
  static bool firstFlush = true;
  if (firstFlush) {
    Serial.println("First flush called!");
    firstFlush = false;
  }
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  lv_draw_sw_rgb565_swap(data, w*h);
  lcd.pushImageDMA(area->x1, area->y1, w, h, (uint16_t*) data);
  lv_display_flush_ready(disp);
}

void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
  static bool lastTouchState = false;
  static unsigned long lastTouchDebug = 0;
  static unsigned long lastErrorReport = 0;

  // Default to released state
  data->state = LV_INDEV_STATE_RELEASED;

  if (touch_has_signal()) {
    if (touch_touched()) {
      // Check for obviously invalid coordinates (negative or way too large)
      if (touch_last_x < -10000 || touch_last_x > 10000 ||
          touch_last_y < -10000 || touch_last_y > 10000) {

        // Report error periodically (not every frame to avoid spam)
        if (millis() - lastErrorReport > 2000) {
          Serial.print("[TOUCH ERROR] Corrupted coordinates: (");
          Serial.print(touch_last_x);
          Serial.print(", ");
          Serial.print(touch_last_y);
          Serial.println(") - GT911 communication issue");
          lastErrorReport = millis();
        }

        data->state = LV_INDEV_STATE_RELEASED;
        return;
      }

      // Ensure coordinates are within valid screen range
      if (touch_last_x >= 0 && touch_last_x < SCREEN_WIDTH &&
          touch_last_y >= 0 && touch_last_y < SCREEN_HEIGHT) {
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
        data->state = LV_INDEV_STATE_PRESSED;

        // Debug output (throttled to avoid spam)
        if (!lastTouchState || millis() - lastTouchDebug > 500) {
          Serial.print("[TOUCH] Valid press at (");
          Serial.print(touch_last_x);
          Serial.print(", ");
          Serial.print(touch_last_y);
          Serial.println(")");
          lastTouchDebug = millis();
        }
        lastTouchState = true;
      } else {
        // Coordinates out of screen bounds but not corrupted
        Serial.print("[TOUCH] Out of bounds: (");
        Serial.print(touch_last_x);
        Serial.print(", ");
        Serial.print(touch_last_y);
        Serial.print(") - Screen: ");
        Serial.print(SCREEN_WIDTH);
        Serial.print("x");
        Serial.println(SCREEN_HEIGHT);
        lastTouchState = false;
      }
    } else if (touch_released()) {
      if (lastTouchState) {
        Serial.println("[TOUCH] Released");
        lastTouchState = false;
      }
    }
  } else {
    // No signal from touch controller
    if (lastTouchState) {
      lastTouchState = false;
    }
  }
}

// RF Reader Setup
HardwareSerial rfidSerial(2);
volatile bool dataAvailable = false;
String rfidTag = "";
bool readingTag = false;
unsigned long lastRFIDCheck = 0;

// Valve Controller Serial Communication
//HardwareSerial valveSerial(1);  // UART1 for valve controller
#define VALVE_RX_PIN 18  // Adjust based on your wiring
#define VALVE_TX_PIN 17  // Adjust based on your wiring
String valveCommand = "";
//bool waitingForDispenseComplete = false;

void IRAM_ATTR onRFIDData() {
  dataAvailable = true;
}

// Forward declarations for serial communication
void sendCommandToValveController(String command);
void processValveControllerResponse();
void onDispenseComplete(float volumeMl);
void onValveControllerReady();

// Forward declarations for UI (using UIBuilder)
void onBeverageCardClicked(int beverageIndex);
int screen_status = -1;
String hardwareId = "";
int dispenserId = -1;
bool beveragesLoaded = false;

// Dispensing session variables
float totalVolumeDispensed = 0.0;
int selectedDispenserBeverageId = -1;
unsigned long lastDispenseTime = 0;
unsigned long inactivityTimeout = 10000;  // 10 seconds of inactivity to end session

void LogMessages(){
  if (DEBUG){

  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" Failed to connect to WiFi");
  }
}

void onDispenserIdReceived(int id) {
  dispenserId = id;
  Serial.print("Dispenser ID set to: ");
  Serial.println(dispenserId);

  // Update UI with dispenser ID on the registration screen
  char idStr[50];
  sprintf(idStr, "Dispenser ID: %d", dispenserId);
  lv_label_set_text(ui_LabelId, idStr);
  Serial.print("[DEBUG] Updated ui_LabelId with: ");
  Serial.println(idStr);
}

void onBeveragesReceived(const char* jsonData) {
  Serial.println("Beverages received, parsing...");

  // Use the BeverageManager's loadFromJson method which already handles the correct structure
  if (beverageManager.loadFromJson(String(jsonData))) {
    beveragesLoaded = true;
    Serial.print("Successfully loaded ");
    Serial.print(beverageManager.getCount());
    Serial.println(" beverages");

    // Log each beverage for debugging
    for (int i = 0; i < beverageManager.getCount(); i++) {
      Beverage* bev = beverageManager.getBeverage(i);
      if (bev) {
        Serial.print("  Valve ");
        Serial.print(bev->dispenser_valve);
        Serial.print(": ");
        Serial.print(bev->beverage_name);
        Serial.print(" - $");
        Serial.print(bev->unit_price);
        Serial.print(" per ");
        Serial.print(bev->unit);
        Serial.print("ml, Color: #");
        Serial.println(bev->beverage_color_code);
      }
    }

    // Move to wristband scanning screen after beverages are loaded
    Serial.println("[DEBUG] Setting screen_status to 1 (RFID scan)");
    screen_status = 1;
  } else {
    Serial.println("Failed to parse beverages or success=false");
    lv_label_set_text(ui_LblErrorMessage, "Error loading beverages");
  }
}

void onWristbandVerified(bool valid, const char* jsonData) {
  if (valid) {
    Serial.println("Wristband verified successfully");
    Serial.print("Activity ID: ");
    Serial.println(mqttManager.getWristbandActivityId());
    screen_status = 2; // Move to selection screen
  } else {
    Serial.println("Wristband verification failed");

    // Parse response for error message
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonData);

    if (!error) {
      if (doc.containsKey("error_info") && !doc["error_info"].isNull()) {
        JsonObject errorInfo = doc["error_info"];
        const char* errorMsg = errorInfo["err_message"] | "Wristband not active";
        lv_label_set_text(ui_LblErrorMessage, errorMsg);

        // Log error details
        int errorCode = errorInfo["err_code"] | 0;
        Serial.print("Error code: ");
        Serial.print(errorCode);
        Serial.print(" - ");
        Serial.println(errorMsg);
      } else {
        lv_label_set_text(ui_LblErrorMessage, "Wristband not active");
      }
    } else {
      lv_label_set_text(ui_LblErrorMessage, "Error reading response");
    }

    // Clear wristband session on failure
    mqttManager.clearWristbandSession();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial
  Serial.println("Starting setup...");

  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);
  Serial.println("Backlight enabled");

  lcd.begin();
  Serial.println("LCD begin done");
/*
  // More extensive LCD hardware test
  Serial.println("Testing LCD with color patterns...");

  // Test 1: Fill with RED
  lcd.fillScreen(TFT_RED);
  Serial.println("Screen should be RED");
  delay(1000);

  // Test 2: Fill with GREEN
  lcd.fillScreen(TFT_GREEN);
  Serial.println("Screen should be GREEN");
  delay(1000);

  // Test 3: Fill with BLUE
  lcd.fillScreen(TFT_BLUE);
  Serial.println("Screen should be BLUE");
  delay(1000);

  // Test 4: Fill with WHITE
  lcd.fillScreen(TFT_WHITE);
  Serial.println("Screen should be WHITE");
  delay(1000);
*/
  // Test 5: Draw some rectangles
  lcd.fillScreen(TFT_BLACK);
  lcd.fillRect(100, 100, 200, 100, TFT_YELLOW);
  lcd.fillRect(400, 200, 200, 100, TFT_CYAN);
  Serial.println("Screen should show yellow and cyan rectangles on black");
  delay(2000);

  lcd.fillScreen(TFT_BLACK);
  Serial.println("LCD hardware test completed - screen now black");

  // Initialize touch with proper coordinates
  Serial.println("Initializing GT911 touch controller...");
  Serial.println("Touch pins: SDA=19, SCL=20, INT=-1, RST=-1");

  touch_init();
  touch_last_x = 0;
  touch_last_y = 0;

  Serial.println("Touch initialization completed");

  // Test touch controller communication
  delay(100);
  if (touch_has_signal()) {
    Serial.println("✓ GT911 touch controller responding");
  } else {
    Serial.println("✗ GT911 touch controller not responding");
  }

  // Initialize LVGL and UI
  lv_init();
  Serial.println("LVGL initialized");

  lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);
  Serial.println("Display created");

  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);
  Serial.println("Touch input device created");

   ui_init();
   Serial.println("UI initialized");

   // Load the registration screen first instead of the default RFID screen
   lv_scr_load(ui_SC00Registrar);
   Serial.println("Loaded registration screen (SC00Registrar)");

  // Initialize RFID serial on pin 38 (RX only)
  rfidSerial.begin(9600, SERIAL_8N1, RX_RFREADER_PIN, TX_RFREADER_PIN);
  rfidSerial.setRxBufferSize(256);
  // Attach interrupt to UART RX
  rfidSerial.onReceive(onRFIDData, false);
  Serial.println("RDM6300 RFID Reader initialized on pin 38");
  Serial.println("Waiting for RFID cards...");

  // Initialize Valve Controller serial communication
  //valveSerial.begin(9600, SERIAL_8N1, VALVE_RX_PIN, VALVE_TX_PIN);
  //valveSerial.setRxBufferSize(256);
  Serial.println("Valve controller serial initialized on pins 17/18");
  sendCommandToValveController("READY");  // Send ready signal

  uint64_t chip_id = ESP.getEfuseMac();
  char id[32];
  sprintf(id, "%04X%08X",(uint16_t)(chip_id >> 32), (uint32_t) chip_id);
  hardwareId = id;
  Serial.println(String("Hardware ID: ") + hardwareId);

  // Display hardware ID on the registration screen
  char hwIdDisplay[50];
  sprintf(hwIdDisplay, "HW: %s", hardwareId.c_str());
  lv_label_set_text(ui_LabelId, hwIdDisplay);
  Serial.print("Setting ui_LabelId to: ");
  Serial.println(hwIdDisplay);

  // Connect to WiFi
  connectToWiFi();

  // Initialize MQTT
  if (WiFi.status() == WL_CONNECTED) {
    mqttManager.init(MQTT_BROKER, MQTT_PORT, hardwareId, MQTT_USER, MQTT_PASSWORD);
    mqttManager.setDispenserIdCallback(onDispenserIdReceived);
    mqttManager.setBeveragesCallback(onBeveragesReceived);
    mqttManager.setWristbandCallback(onWristbandVerified);
    mqttManager.connect();

    // Start registration immediately
    mqttManager.registerHardware();
  }

  // Add click handler to Next button
  //lv_obj_add_event_cb(ui_BtnNextComp, btn_sc03finalizar_clicked, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_BtnFinalizr, btn_sc03finalizar_clicked, LV_EVENT_CLICKED, NULL);
  
  screen_status = 0;
  
}

void loop() {
  static unsigned long lastScreenChange = 0;
  static unsigned long stateTimer = 0;
  static int previousScreenStatus = -1;
  static unsigned long lastStatusDebug = 0;

  lv_tick_inc(millis() - lastTick);
  lastTick = millis();
  lv_timer_handler();

  // Handle MQTT communication
  if (WiFi.status() == WL_CONNECTED) {
    mqttManager.loop();
  }
 

  // Debug status every 5 seconds
  if (millis() - lastStatusDebug > 5000) {
    lastStatusDebug = millis();
    Serial.print("[STATUS] Screen: ");
    Serial.print(screen_status);
    Serial.print(", Dispenser ID: ");
    Serial.print(mqttManager.hasDispenserId() ? "YES" : "NO");
    Serial.print(", Beverages: ");
    Serial.print(beveragesLoaded ? "YES" : "NO");
    Serial.print(" (");
    Serial.print(beverageManager.getCount());
    Serial.println(" loaded)");
  }

  // Check if screen status changed
  /*if (screen_status != previousScreenStatus) {
    Serial.print("[DEBUG] Screen status changed from ");
    Serial.print(previousScreenStatus);
    Serial.print(" to ");
    Serial.println(screen_status);
    lastScreenChange = millis();
    stateTimer = millis();
    previousScreenStatus = screen_status;
  }*/

  switch (screen_status){
    case 0:
      // Wait for dispenser ID and beverages - Show registration screen
      if (screen_status != previousScreenStatus) {
        // Make sure we're on the registration screen
        lv_scr_load_anim(ui_SC00Registrar, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);

        char statusMsg[100];
        if (!mqttManager.hasDispenserId()) {
          sprintf(statusMsg, "Registering... HW: %s", hardwareId.c_str());
        } else if (!beveragesLoaded) {
          sprintf(statusMsg, "Loading beverages... ID: %d", dispenserId);
        } else {
          sprintf(statusMsg, "Ready - Dispenser: %d", dispenserId);
        }
        lv_label_set_text(ui_LabelId, statusMsg);
        Serial.print("[DEBUG] Status screen 0 - Label set to: ");
        Serial.println(statusMsg);
      }

      // Only proceed when we have both dispenser ID and beverages
      if (mqttManager.hasDispenserId() && beveragesLoaded) {
        Serial.println("[DEBUG] Both dispenser ID and beverages loaded, checking timer...");
        if (millis() - stateTimer > 2000) { // Show ready message for 2 seconds
          Serial.println("[DEBUG] Timer expired, moving to RFID screen");
          screen_status = 1;
        }
      } else {
        if (!mqttManager.hasDispenserId()) {
          Serial.println("[DEBUG] Still waiting for dispenser ID");
        }
        if (!beveragesLoaded) {
          Serial.println("[DEBUG] Still waiting for beverages");
        }
      }
      break;

    case 1:
      // Show scan screen and wait for RFID
      if (screen_status != previousScreenStatus) {
        Serial.println("[DEBUG] Loading RFID scan screen (SC01Pulsera)");
        lv_scr_load_anim(ui_SC01Pulsera, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        Serial.println("[DEBUG] SC01Pulsera loaded - Waiting for RFID scan");
        lastScreenChange = millis();
        stateTimer = millis();
        previousScreenStatus = screen_status;
      }

      // Check for RFID data
      if (dataAvailable){
        Serial.println("[DEBUG] RFID interrupt triggered, processing...");
        processRFIDReading();
      }

      // Also poll the serial buffer directly every 100ms as backup
      if (millis() - lastRFIDCheck > 100) {
        lastRFIDCheck = millis();
        if (rfidSerial.available() > 0) {
          Serial.print("[DEBUG] Direct poll found ");
          Serial.print(rfidSerial.available());
          Serial.println(" bytes available");
          dataAvailable = true;
          processRFIDReading();
        }
      }
      break;

    case 2:
      // Show selection screen
      if (screen_status != previousScreenStatus) {
        Serial.println("[DEBUG] Loading beverage selection screen");
        lv_scr_load_anim(ui_SC02Selection, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);

        // Populate with available beverages using UIBuilder
        uiBuilder.buildBeverageSelection();

        stateTimer = millis();
        totalVolumeDispensed = 0.0;  // Reset volume for new session
        Serial.println("[DEBUG] Beverage selection screen loaded - waiting for user selection");
        lastScreenChange = millis();
        stateTimer = millis();
        previousScreenStatus = screen_status;
      }
      break;

    case 3:
      // Dispensing state - waiting for valve controller feedback
      if (screen_status != previousScreenStatus) {
        onDispensePartial(0);
        lv_scr_load_anim(ui_SC03Dispensar, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        Serial.println("[DEBUG] Dispensing screen shown, waiting for valve controller...");
        //waitingForDispenseComplete = true;
        previousScreenStatus = screen_status;
      }

      // Process valve controller responses
      processValveControllerResponse();
      break;

    case 4:
      // Finalizing - show completion screen
      if (screen_status != previousScreenStatus) {
        lv_scr_load_anim(ui_SC04Finalizar, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        Serial.println("[DEBUG] Showing finalization screen");

        // Send cleaning command to valve controller
        sendCommandToValveController("CLEAN");
        stateTimer = millis();
      }

      if (millis() - stateTimer > 3000) {
        Serial.println("[DEBUG] Returning to RFID scan");

        // Clear the session
        mqttManager.clearWristbandSession();
        totalVolumeDispensed = 0.0;
        selectedDispenserBeverageId = -1;
        //waitingForDispenseComplete = false;

        // Send ready signal to valve controller
        sendCommandToValveController("READY");

        // Return to RFID scanning
        screen_status = 1;
      }
      break;

    default:
      break;
  }

  delay(5);
}

void processRFIDReading() {
  dataAvailable = false;
  Serial.println("[DEBUG] Processing RFID reading...");
  Serial.print("[DEBUG] Available bytes: ");
  Serial.println(rfidSerial.available());

  while (rfidSerial.available()) {
    char c = rfidSerial.read();
    Serial.print("[DEBUG] Read byte: 0x");
    Serial.println(c, HEX);

    if (c == 0x02) {  // Start byte
      rfidTag = "";
      readingTag = true;
      Serial.println("[DEBUG] Start byte detected, beginning tag read");
    }
    else if (c == 0x03) {  // End byte
      Serial.println("[DEBUG] End byte detected");
      if (readingTag && rfidTag.length() == 10) {
        Serial.print("[DEBUG] Valid RFID Tag detected: ");
        Serial.println(rfidTag);
        // Convert hex string to decimal (last 8 digits typically)
      String hexString = String(rfidTag);
      unsigned long cardID = strtoul(hexString.substring(2).c_str(), NULL, 16);
      
      // Format to 10 digits with leading zeros
      char cardIDStr[11];
      sprintf(cardIDStr, "%010lu", cardID);
      
      Serial.print("Card ID: ");
      Serial.println(cardIDStr);
        handleRFIDTag(cardIDStr);
      } else {
        Serial.print("[DEBUG] Invalid tag - readingTag: ");
        Serial.print(readingTag);
        Serial.print(", length: ");
        Serial.println(rfidTag.length());
      }
      readingTag = false;
    }
    else if (readingTag && rfidTag.length() < 10) {
      rfidTag += c;
      Serial.print("[DEBUG] Building tag, current: ");
      Serial.println(rfidTag);
    }
  }
  Serial.println("[DEBUG] RFID reading complete");
}

void handleRFIDTag(String tagID) {
  Serial.print("[DEBUG] Processing RFID tag ID: ");
  Serial.println(tagID);
  Serial.print("[DEBUG] Current screen status: ");
  Serial.println(screen_status);

  // Show processing message
  lv_label_set_text(ui_LblErrorMessage, "Verificando pulsera...");

  // Verify wristband via MQTT
  if (mqttManager.isConnected() && mqttManager.hasDispenserId()) {
    Serial.println("[DEBUG] MQTT connected and dispenser ID available, verifying wristband...");
    mqttManager.verifyWristband(tagID);
  } else {
    Serial.print("[DEBUG] Cannot verify - MQTT connected: ");
    Serial.print(mqttManager.isConnected());
    Serial.print(", Has dispenser ID: ");
    Serial.println(mqttManager.hasDispenserId());
    lv_label_set_text(ui_LblErrorMessage, "Sistema no conectado");
  }
}

void selectBeverage(int beverageIndex) {
  Beverage* bev = beverageManager.getBeverage(beverageIndex);
  if (bev) {
    selectedDispenserBeverageId = bev->dispenser_beverage_id;
    mqttManager.setCurrentBeverage(selectedDispenserBeverageId);

    Serial.print("[DEBUG] Selected beverage: ");
    Serial.print(bev->beverage_name);
    Serial.print(" (ID: ");
    Serial.print(selectedDispenserBeverageId);
    Serial.print(", Valve: ");
    Serial.print(bev->dispenser_valve);
    Serial.println(")");

    // Send command to valve controller to open specific valve
    String command = "CM_VALVE0" + String(bev->dispenser_valve);
    sendCommandToValveController(command);

    // Move to dispensing screen
    screen_status = 3;
  }
}

// Serial Communication Functions
void sendCommandToValveController(String command) {
  //Serial.print("[VALVE_CMD] Sending: ");
  Serial.println(command);
  //valveSerial.println(command);
}

void processValveControllerResponse() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      // Process complete command
      if (valveCommand.length() > 0) {
        Serial.println(valveCommand);

        // Parse command
        if (valveCommand.startsWith("RSP_TOTAL_VOLUME:")) {
          // Format: DISPENSE_COMPLETE:250.5 (ml)
          String volumeStr = valveCommand.substring(17);
          float volume = volumeStr.toFloat();
          onDispenseComplete(volume);
        }else if (valveCommand.startsWith("RSP_PARTIAL_VOLUME:")) {
          // Format: DISPENSE_COMPLETE:250.5 (ml)
          String volumeStr = valveCommand.substring(19);
          float volume = volumeStr.toFloat();
          onDispensePartial(volume);
        }
        else if (valveCommand == "CM_READY") {
          onValveControllerReady();
        }
        else if (valveCommand == "CLEAN_STARTED") {
          Serial.println("[DEBUG] Cleaning complete");
          // Move to finalization screen
          screen_status = 4;
        }

        valveCommand = "";
      }
    } else if (c != '\r') {
      valveCommand += c;
    }
  }
}

void onDispensePartial(float volumeMl) {
  lv_spinbox_set_value(ui_Spinbox1, volumeMl); 
  int percentage = map(volumeMl, 0, 2000, 0, 100);
  lv_bar_set_value(ui_BarDispensed, percentage, LV_ANIM_ON);
}

void onDispenseComplete(float volumeMl) {
  //Serial.print("[DEBUG] Dispense complete. Volume: ");
  //Serial.print(volumeMl);
  //Serial.println(" ml");

  totalVolumeDispensed = volumeMl;

  // Register consumption with MQTT
  if (selectedDispenserBeverageId > 0 && mqttManager.hasActiveSession()) {
    mqttManager.registerConsumption(selectedDispenserBeverageId, volumeMl);
  }
}

void onValveControllerReady() {
  Serial.println("[DEBUG] Valve controller is ready");
  screen_status = 0;
  // Can be used to ensure valve controller is ready before operations
}

// ============ BEVERAGE SELECTION CALLBACK ============

void onBeverageCardClicked(int beverageIndex) {
  Serial.print("[UI] Beverage selected - Index: ");
  Serial.println(beverageIndex);

  Beverage* bev = beverageManager.getBeverage(beverageIndex);
  if (bev) {
    Serial.print("[UI] Selected: ");
    Serial.println(bev->beverage_name);

    // Call the selection function
    selectBeverage(beverageIndex);
  }
}

// Handler function - goes to selection screen
void btn_sc03finalizar_clicked(lv_event_t * e) {
  Serial.println("CM_FORCEEND");
  //lv_scr_load_anim(ui_SC03Dispensar, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
  screen_status = 4;
}