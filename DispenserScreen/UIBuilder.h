#ifndef UIBUILDER_H
#define UIBUILDER_H

#include <lvgl.h>
#include "Beverage.h"
#include "ui.h"  // SquareLine Studio exports
#include "ui_SC03Dispensar.h"  // For ui_SC03Title

class UIBuilder {
private:
  BeverageManager* beverageManager;

public:
  UIBuilder(BeverageManager* manager) : beverageManager(manager) {}
  
  void buildBeverageSelection() {
    Serial.println("[UIBuilder] Building beverage selection...");

    // Clear existing
    lv_obj_clean(ui_ContScrollOption);

    // Ensure scroll container is properly configured for touch
    lv_obj_add_flag(ui_ContScrollOption, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ui_ContScrollOption, LV_OBJ_FLAG_SCROLL_MOMENTUM); // Keep this for smoother scrolling
    lv_obj_set_scroll_dir(ui_ContScrollOption, LV_DIR_HOR);

    // Add debug event callbacks to scroll container
    extern void scroll_container_event_cb(lv_event_t * e);
    lv_obj_add_event_cb(ui_ContScrollOption, scroll_container_event_cb, LV_EVENT_ALL, NULL);

    Serial.print("[UIBuilder] Creating ");
    Serial.print(beverageManager->getCount());
    Serial.println(" beverage cards");

    for (int i = 0; i < beverageManager->getCount(); i++) {
      Beverage* bev = beverageManager->getBeverage(i);

      Serial.print("[UIBuilder] Creating card ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(bev->beverage_name);

      // Create card
      lv_obj_t * card = lv_obj_create(ui_ContScrollOption);
      lv_obj_set_size(card, 220, 280);
      lv_obj_set_style_radius(card, 15, 0);

      uint32_t color = beverageManager->hexToColor(bev->beverage_color_code);
      lv_obj_set_style_bg_color(card, lv_color_hex(color), 0);
      lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

      lv_obj_set_style_border_width(card, 3, 0);
      lv_obj_set_style_border_color(card, lv_color_hex(0xFFFFFF), 0);

      // Make card clickable and remove scrollable flag from individual cards
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

      // Name
      lv_obj_t * nameLabel = lv_label_create(card);
      lv_label_set_text(nameLabel, bev->beverage_name.c_str());
      lv_obj_set_style_text_color(nameLabel, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_20, 0);
      lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(nameLabel, 180);
      lv_obj_align(nameLabel, LV_ALIGN_TOP_MID, 0, 30);
      lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_CENTER, 0);

      // Price
      lv_obj_t * priceLabel = lv_label_create(card);
      char priceText[30];
      sprintf(priceText, "$%.2f", bev->unit_price);
      lv_label_set_text(priceLabel, priceText);
      lv_obj_set_style_text_color(priceLabel, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_font(priceLabel, &lv_font_montserrat_24, 0);
      lv_obj_align(priceLabel, LV_ALIGN_CENTER, 0, 20);

      // Unit
      lv_obj_t * unitLabel = lv_label_create(card);
      char unitText[30];
      sprintf(unitText, "%d ml", bev->unit);
      lv_label_set_text(unitLabel, unitText);
      lv_obj_set_style_text_color(unitLabel, lv_color_hex(0xFFFFFF), 0);
      lv_obj_align(unitLabel, LV_ALIGN_BOTTOM_MID, 0, -20);

      // Store index as user data
      lv_obj_set_user_data(card, (void*)(uintptr_t)i);

      // Click event
      lv_obj_add_event_cb(card, beverage_card_clicked_static, LV_EVENT_CLICKED, NULL);

      Serial.print("[UIBuilder] Card ");
      Serial.print(i);
      Serial.println(" created successfully");
    }

    Serial.println("[UIBuilder] Beverage selection build complete");
  }
  
  void updateDispenseScreen() {
    Beverage* bev = beverageManager->getSelectedBeverage();
    if (bev == nullptr) return;
    
    char dispenseTitle[40];
    sprintf(dispenseTitle, "Dispensando %s", bev->beverage_name.c_str());
    lv_label_set_text(ui_SC03Title, dispenseTitle);
    
  }
  
  // Static callback wrapper
  static void beverage_card_clicked_static(lv_event_t * e);
};

#endif