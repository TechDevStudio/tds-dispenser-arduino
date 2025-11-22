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
  
  void buildBeverageSelectionGrid() {
    Serial.println("[UIBuilder] Building beverage selection grid (2x4)...");

    // Clear existing
    lv_obj_clean(ui_ContScrollOption);

    // Configure container as non-scrollable grid
    lv_obj_remove_flag(ui_ContScrollOption, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(ui_ContScrollOption, LV_LAYOUT_GRID);

    // Create 2x4 grid layout (2 columns, 4 rows)
    static int32_t column_dsc[] = {380, 380, LV_GRID_TEMPLATE_LAST};  // 2 columns of 380px each
    static int32_t row_dsc[] = {100, 100, 100, 100, LV_GRID_TEMPLATE_LAST};  // 4 rows of 100px each

    lv_obj_set_grid_dsc_array(ui_ContScrollOption, column_dsc, row_dsc);
    lv_obj_set_style_pad_column(ui_ContScrollOption, 20, 0);  // Horizontal spacing
    lv_obj_set_style_pad_row(ui_ContScrollOption, 15, 0);     // Vertical spacing

    Serial.print("[UIBuilder] Creating grid for ");
    Serial.print(beverageManager->getCount());
    Serial.println(" beverages (max 8)");

    // Create up to 8 beverage cards in grid layout
    int maxCards = beverageManager->getCount() > 8 ? 8 : beverageManager->getCount();

    for (int i = 0; i < maxCards; i++) {
      Beverage* bev = beverageManager->getBeverage(i);

      Serial.print("[UIBuilder] Creating grid card ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(bev->beverage_name);

      // Create card
      lv_obj_t * card = lv_obj_create(ui_ContScrollOption);
      lv_obj_set_size(card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
      lv_obj_set_style_radius(card, 10, 0);

      // Calculate grid position (row, column)
      int col = i % 2;  // 0 or 1
      int row = i / 2;  // 0, 1, 2, or 3
      lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, col, 1,
                                 LV_GRID_ALIGN_STRETCH, row, 1);

      uint32_t color = beverageManager->hexToColor(bev->beverage_color_code);
      lv_obj_set_style_bg_color(card, lv_color_hex(color), 0);
      lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

      lv_obj_set_style_border_width(card, 2, 0);
      lv_obj_set_style_border_color(card, lv_color_hex(0xFFFFFF), 0);

      // Make card clickable
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

      // Create horizontal flex layout inside card
      lv_obj_set_layout(card, LV_LAYOUT_FLEX);
      lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_set_style_pad_all(card, 15, 0);

      // Left side - Name
      lv_obj_t * nameLabel = lv_label_create(card);
      lv_label_set_text(nameLabel, bev->beverage_name.c_str());
      lv_obj_set_style_text_color(nameLabel, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_18, 0);
      lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(nameLabel, 200);

      // Right side container for price and volume
      lv_obj_t * rightContainer = lv_obj_create(card);
      lv_obj_set_size(rightContainer, 140, 70);
      lv_obj_set_style_bg_opa(rightContainer, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(rightContainer, 0, 0);
      lv_obj_set_style_pad_all(rightContainer, 0, 0);
      lv_obj_set_layout(rightContainer, LV_LAYOUT_FLEX);
      lv_obj_set_flex_flow(rightContainer, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(rightContainer, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

      // Price
      lv_obj_t * priceLabel = lv_label_create(rightContainer);
      char priceText[30];
      sprintf(priceText, "$%.2f", bev->unit_price);
      lv_label_set_text(priceLabel, priceText);
      lv_obj_set_style_text_color(priceLabel, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_font(priceLabel, &lv_font_montserrat_20, 0);

      // Volume
      lv_obj_t * unitLabel = lv_label_create(rightContainer);
      char unitText[30];
      sprintf(unitText, "%d ml", bev->unit);
      lv_label_set_text(unitLabel, unitText);
      lv_obj_set_style_text_color(unitLabel, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_text_font(unitLabel, &lv_font_montserrat_14, 0);

      // Store index as user data
      lv_obj_set_user_data(card, (void*)(uintptr_t)i);

      // Click event
      lv_obj_add_event_cb(card, beverage_card_clicked_static, LV_EVENT_CLICKED, NULL);

      Serial.print("[UIBuilder] Grid card ");
      Serial.print(i);
      Serial.print(" created at position (");
      Serial.print(row);
      Serial.print(",");
      Serial.print(col);
      Serial.println(")");
    }

    Serial.println("[UIBuilder] Beverage selection grid complete");
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