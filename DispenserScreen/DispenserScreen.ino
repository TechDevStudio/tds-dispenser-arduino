#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
#define TFT_BL_PIN 2

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

// Rest of your code...
#define DRAW_BUF_SIZE ((SCREEN_WIDTH * SCREEN_HEIGHT / 10) * (sizeof(uint16_t)))
static uint16_t draw_buf[DRAW_BUF_SIZE];
uint32_t lastTick = 0;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, unsigned char* data) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  lv_draw_sw_rgb565_swap(data, w*h);
  lcd.pushImageDMA(area->x1, area->y1, w, h, (uint16_t*) data);
  lv_disp_flush_ready(disp);
}

void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
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

void setup() {
  Serial.begin(115200);
  
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);
  
  lcd.begin();
  touch_init();

  // Initialize LVGL and UI
  lv_init();
  lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);
  
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);
  
  ui_init();
  delay(100);

  // Add click handler to Next button
  lv_obj_add_event_cb(ui_BtnNextComp, btn_next_clicked, LV_EVENT_CLICKED, NULL);
  
}

void loop() {
  lv_tick_inc(millis() - lastTick);
  lastTick = millis();
  lv_timer_handler();
  delay(5);
}

// Handler function - goes to selection screen
void btn_next_clicked(lv_event_t * e) {
  Serial.println("Next button clicked - going to selection screen");
  //lv_scr_load_anim(ui_SC03Dispensar, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}