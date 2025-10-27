#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
//#include <HardwareSerial.h>

#include <WiFi.h>
#include <ArduinoOTA.h>

#include "Config.h"
#include "Beverage.h"
#include "UIBuilder.h"

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

// Forward declarations
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, unsigned char* data);
void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data);
// Screen setup - reduced buffer size for testing
//#define DRAW_BUF_SIZE (SCREEN_WIDTH * 20 * sizeof(uint16_t))  // 20 lines buffer

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
  // Touch temporarily disabled
  data->state = LV_INDEV_STATE_RELEASED;
  return;

  if (touch_has_signal()) {
    if (touch_touched()) {
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      data->state = LV_INDEV_STATE_PRESSED;
    } else if (touch_released()) {
      data->state = LV_INDEV_STATE_RELEASED;
    }
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// RF Reader Setup
HardwareSerial rfidSerial(2);
volatile bool dataAvailable = false;
String rfidTag = "";
bool readingTag = false;

void IRAM_ATTR onRFIDData() {
  dataAvailable = true;
}
int screen_status = -1;
String dispenserId = "";

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

  // Temporarily disable touch to avoid GPIO errors
  // touch_init();
  Serial.println("Touch temporarily disabled");

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
   Serial.println("UI initialized - should show SC01Pulsera screen");

  // Initialize RFID serial on pin 38 (RX only)
  rfidSerial.begin(9600, SERIAL_8N1, RX_RFREADER_PIN, TX_RFREADER_PIN);
  // Attach interrupt to UART RX
  rfidSerial.onReceive(onRFIDData);
  Serial.println("RDM6300 RFID Reader initialized on pin 38");

  uint64_t chip_id = ESP.getEfuseMac();
  char id[32];
  sprintf(id, "%04X%08X",(uint16_t)(chip_id >> 32), (uint32_t) chip_id);
  dispenserId = id;
  Serial.println(String("Dispenser ID: ") + id);
  delay(1000);
  
  screen_status = 0;
  
}

void loop() {
  static unsigned long lastScreenChange = 0;
  static unsigned long stateTimer = 0;
  static int previousScreenStatus = -1;

  lv_tick_inc(millis() - lastTick);
  lastTick = millis();
  lv_timer_handler();

  // Check if screen status changed
  if (screen_status != previousScreenStatus) {
    lastScreenChange = millis();
    stateTimer = millis();
    previousScreenStatus = screen_status;
  }

  switch (screen_status){
    case 0:
      // Show dispenser ID for 5 seconds
      if (screen_status != previousScreenStatus) {
        lv_label_set_text(ui_LabelId, dispenserId.c_str());
      }
      if (millis() - stateTimer > 5000) {
        screen_status = 1;
      }
      break;

    case 1:
      // Show scan screen and wait for RFID
      if (screen_status != previousScreenStatus) {
        lv_scr_load_anim(ui_SC01Pulsera, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
      }
      if (dataAvailable){
        processRFIDReading();
      }
      break;

    case 2:
      // Show selection screen
      if (screen_status != previousScreenStatus) {
        lv_scr_load_anim(ui_SC02Selection, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        stateTimer = millis();
      }
      // Optional: auto-advance after delay
      // if (millis() - stateTimer > 500) {
      //   screen_status = 3;
      // }
      break;

    case 3:
      screen_status = 4;
      break;

    case 4:
      // Clean dispenser for 5 seconds
      if (screen_status != previousScreenStatus) {
        Serial.println("CLEANDISPENSER");
        stateTimer = millis();
      }
      if (millis() - stateTimer > 5000) {
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
  while (rfidSerial.available()) {
    char c = rfidSerial.read();

    if (c == 0x02) {  // Start byte
      rfidTag = "";
      readingTag = true;
    }
    else if (c == 0x03) {  // End byte
      if (readingTag && rfidTag.length() == 10) {
        Serial.print("RFID Tag: ");
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
      }
      readingTag = false;
    }
    else if (readingTag && rfidTag.length() < 10) {
      rfidTag += c;
    }
  }
}

void handleRFIDTag(String tagID) {
  // Send to your backend API
  // Trigger solenoid valve
  // Update display, etc.

  Serial.print("Processing tag: ");
  Serial.println(tagID);
  if (tagID == "0003540239"){
    screen_status = 2;
  }else{
    char errMessage[50];
    sprintf(errMessage,"Pulsera %s no esta activa", tagID.c_str() );
    lv_label_set_text(ui_LblErrorMessage, errMessage);
  }

  
}