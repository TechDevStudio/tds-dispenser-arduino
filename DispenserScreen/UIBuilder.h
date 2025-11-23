#ifndef UIBUILDER_H
#define UIBUILDER_H

#include <lvgl.h>
#include "Beverage.h"
#include "ui.h"  // SquareLine Studio exports
#include "ui_SC03Dispensar.h"  // For ui_SC03Title, ui_BarDispensed, ui_ImageBeverage

class UIBuilder {
private:
  BeverageManager* beverageManager;

public:
  UIBuilder(BeverageManager* manager) : beverageManager(manager) {}
  
  // void buildBeverageSelection() {
  //   Serial.println("[UIBuilder] Building beverage selection...");

  //   // Clear existing
  //   lv_obj_clean(ui_ContScrollOption);

  //   // Ensure scroll container is properly configured for touch
  //   lv_obj_add_flag(ui_ContScrollOption, LV_OBJ_FLAG_SCROLLABLE);
  //   lv_obj_remove_flag(ui_ContScrollOption, LV_OBJ_FLAG_SCROLL_MOMENTUM); // Keep this for smoother scrolling
  //   lv_obj_set_scroll_dir(ui_ContScrollOption, LV_DIR_HOR);

  //   // Add debug event callbacks to scroll container
  //   extern void scroll_container_event_cb(lv_event_t * e);
  //   lv_obj_add_event_cb(ui_ContScrollOption, scroll_container_event_cb, LV_EVENT_ALL, NULL);

  //   Serial.print("[UIBuilder] Creating ");
  //   Serial.print(beverageManager->getCount());
  //   Serial.println(" beverage cards");

  //   for (int i = 0; i < beverageManager->getCount(); i++) {
  //     Beverage* bev = beverageManager->getBeverage(i);

  //     Serial.print("[UIBuilder] Creating card ");
  //     Serial.print(i);
  //     Serial.print(": ");
  //     Serial.println(bev->beverage_name);

  //     // Create card
  //     lv_obj_t * card = lv_obj_create(ui_ContScrollOption);
  //     lv_obj_set_size(card, 220, 280);
  //     lv_obj_set_style_radius(card, 15, 0);

  //     uint32_t color = beverageManager->hexToColor(bev->beverage_color_code);
  //     lv_obj_set_style_bg_color(card, lv_color_hex(color), 0);
  //     lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

  //     lv_obj_set_style_border_width(card, 3, 0);
  //     lv_obj_set_style_border_color(card, lv_color_hex(0xFFFFFF), 0);

  //     // Make card clickable and remove scrollable flag from individual cards
  //     lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  //     lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  //     // Name
  //     lv_obj_t * nameLabel = lv_label_create(card);
  //     lv_label_set_text(nameLabel, bev->beverage_name.c_str());
  //     lv_obj_set_style_text_color(nameLabel, lv_color_hex(0xFFFFFF), 0);
  //     lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_20, 0);
  //     lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_WRAP);
  //     lv_obj_set_width(nameLabel, 180);
  //     lv_obj_align(nameLabel, LV_ALIGN_TOP_MID, 0, 30);
  //     lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_CENTER, 0);

  //     // Price
  //     lv_obj_t * priceLabel = lv_label_create(card);
  //     char priceText[30];
  //     sprintf(priceText, "$%.2f", bev->unit_price);
  //     lv_label_set_text(priceLabel, priceText);
  //     lv_obj_set_style_text_color(priceLabel, lv_color_hex(0xFFFFFF), 0);
  //     lv_obj_set_style_text_font(priceLabel, &lv_font_montserrat_24, 0);
  //     lv_obj_align(priceLabel, LV_ALIGN_CENTER, 0, 20);

  //     // Unit
  //     lv_obj_t * unitLabel = lv_label_create(card);
  //     char unitText[30];
  //     sprintf(unitText, "%d ml", bev->unit);
  //     lv_label_set_text(unitLabel, unitText);
  //     lv_obj_set_style_text_color(unitLabel, lv_color_hex(0xFFFFFF), 0);
  //     lv_obj_align(unitLabel, LV_ALIGN_BOTTOM_MID, 0, -20);

  //     // Store index as user data
  //     lv_obj_set_user_data(card, (void*)(uintptr_t)i);

  //     // Click event
  //     lv_obj_add_event_cb(card, beverage_card_clicked_static, LV_EVENT_CLICKED, NULL);

  //     Serial.print("[UIBuilder] Card ");
  //     Serial.print(i);
  //     Serial.println(" created successfully");
  //   }

  //   Serial.println("[UIBuilder] Beverage selection build complete");
  // }
  
void buildBeverageSelectionGrid() {
  Serial.println("[UIBuilder] Starting buildBeverageSelectionGrid");

  if (ui_ContOptions == NULL) {
    Serial.println("[ERROR] ui_ContOptions is NULL!");
    return;
  }

  // Clear existing content
  lv_obj_clean(ui_ContOptions);
  Serial.println("[UIBuilder] Container cleaned");

  // Ensure container is visible
  lv_obj_clear_flag(ui_ContOptions, LV_OBJ_FLAG_HIDDEN);

  // Configure container as flex layout for better centering
  lv_obj_set_layout(ui_ContOptions, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(ui_ContOptions, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(ui_ContOptions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  Serial.println("[UIBuilder] Flex layout set for centering");

  // Set padding for the container
  lv_obj_set_style_pad_all(ui_ContOptions, 10, 0);
  lv_obj_set_style_pad_column(ui_ContOptions, 15, 0);
  lv_obj_set_style_pad_row(ui_ContOptions, 10, 0);
  int beverageCount = beverageManager->getCount();
  Serial.print("[UIBuilder] Creating grid with ");
  Serial.print(beverageCount);
  Serial.println(" beverages");

  if (beverageCount == 0) {
    Serial.println("[WARNING] No beverages to display!");
    return;
  }

  // Create beverage cards in grid
  for (int i = 0; i < beverageCount && i < 8; i++) {  // Max 8 beverages (2x4 grid)
    Beverage* bev = beverageManager->getBeverage(i);

    Serial.print("[UIBuilder] Creating card ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(bev->beverage_name);

// Create card container with thin border
lv_obj_t * card = lv_obj_create(ui_ContOptions);
lv_obj_set_size(card, 175, 175);
lv_obj_set_style_radius(card, 12, 0);
lv_obj_set_style_bg_color(card, lv_color_hex(0x000000), 0);
lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
lv_obj_set_style_border_width(card, 1, 0);
lv_obj_set_style_border_color(card, lv_color_hex(0x666666), 0);
lv_obj_set_style_pad_all(card, 0, 0);  // NO padding
lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

// Create colored top section - simple approach
lv_obj_t * topSection = lv_obj_create(card);
lv_obj_set_width(topSection, lv_pct(100));  // 100% width
lv_obj_set_height(topSection, 87);  // Exactly half of 175
lv_obj_align(topSection, LV_ALIGN_TOP_MID, 0, 0);
lv_obj_clear_flag(topSection, LV_OBJ_FLAG_SCROLLABLE);
lv_obj_clear_flag(topSection, LV_OBJ_FLAG_CLICKABLE);

// Style the top section
uint32_t color = beverageManager->hexToColor(bev->beverage_color_code);
lv_obj_set_style_bg_color(topSection, lv_color_hex(color), 0);
lv_obj_set_style_bg_opa(topSection, LV_OPA_COVER, 0);
lv_obj_set_style_radius(topSection, 12, LV_PART_MAIN);  // Match card radius
lv_obj_set_style_border_width(topSection, 0, 0);
lv_obj_set_style_pad_all(topSection, 0, 0);

// Add category icon to top section
lv_obj_t * categoryIcon = lv_img_create(topSection);
const lv_img_dsc_t* img_src = nullptr;
if (bev->beverage_category_type_id == 1) {
  img_src = &ui_img_beer_mug_png;
} else if (bev->beverage_category_type_id == 2) {
  img_src = &ui_img_red_wine_png;
} else if (bev->beverage_category_type_id == 3) {
  img_src = &ui_img_cocktail_png;
}

if (img_src != nullptr) {
  lv_img_set_src(categoryIcon, img_src);
  lv_obj_align(categoryIcon, LV_ALIGN_CENTER, 0, 0);
  lv_img_set_zoom(categoryIcon, 200);
}

// Beverage name - positioned at top of bottom half
lv_obj_t * nameLabel = lv_label_create(card);
lv_label_set_text(nameLabel, bev->beverage_name.c_str());
lv_obj_set_style_text_color(nameLabel, lv_color_hex(0xFFFFFF), 0);
lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_14, 0);
lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_WRAP);
lv_obj_set_width(nameLabel, 160);
lv_obj_align(nameLabel, LV_ALIGN_TOP_MID, 0, 95);  // Just below colored section
lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_CENTER, 0);

// Category type - positioned below name
lv_obj_t * categoryLabel = lv_label_create(card);
lv_label_set_text(categoryLabel, bev->beverage_category_type_description.c_str());
lv_obj_set_style_text_color(categoryLabel, lv_color_hex(0xCCCCCC), 0);
lv_obj_set_style_text_font(categoryLabel, &lv_font_montserrat_10, 0);
lv_obj_align(categoryLabel, LV_ALIGN_TOP_MID, 0, 120);
lv_obj_set_style_text_align(categoryLabel, LV_TEXT_ALIGN_CENTER, 0);

// Description - positioned at bottom
lv_obj_t * descriptionLabel = lv_label_create(card);
lv_label_set_text(descriptionLabel, bev->beverage_description.c_str());
lv_obj_set_style_text_color(descriptionLabel, lv_color_hex(0xAAAAAA), 0);
lv_obj_set_style_text_font(descriptionLabel, &lv_font_montserrat_10, 0);
lv_label_set_long_mode(descriptionLabel, LV_LABEL_LONG_WRAP);
lv_obj_set_width(descriptionLabel, 160);
lv_obj_align(descriptionLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
lv_obj_set_style_text_align(descriptionLabel, LV_TEXT_ALIGN_CENTER, 0);

// Store index as user data
lv_obj_set_user_data(card, (void*)(uintptr_t)i);

// Click event
lv_obj_add_event_cb(card, beverage_card_clicked_static, LV_EVENT_CLICKED, NULL);

    Serial.print("[UIBuilder] Card ");
    Serial.print(i);
    Serial.println(" created successfully");
  }

  Serial.print("[UIBuilder] Beverage grid selection build complete. Created ");
  Serial.print(lv_obj_get_child_count(ui_ContOptions));
  Serial.println(" cards");

  // Force a screen refresh
  lv_obj_invalidate(ui_ContOptions);
}

  void updateDispenseScreen() {
    Beverage* bev = beverageManager->getSelectedBeverage();
    if (bev == nullptr) {
      Serial.println("[UIBuilder] No beverage selected!");
      return;
    }

    Serial.print("[UIBuilder] Updating dispense screen for: ");
    Serial.println(bev->beverage_name);

    char dispenseTitle[40];
    sprintf(dispenseTitle, "Dispensando %s", bev->beverage_name.c_str());
    lv_label_set_text(ui_SC03Title, dispenseTitle);

    // Set the progress bar color to match the selected beverage
    uint32_t color = beverageManager->hexToColor(bev->beverage_color_code);
    Serial.print("[UIBuilder] Setting bar color to: 0x");
    Serial.println(color, HEX);

    // Make sure to update the indicator part of the bar
    lv_obj_set_style_bg_color(ui_BarDispensed, lv_color_hex(color), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BarDispensed, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Set the beverage icon based on category type
    Serial.print("[UIBuilder] Category type ID: ");
    Serial.println(bev->beverage_category_type_id);

    const lv_img_dsc_t* img_src = nullptr;
    if (bev->beverage_category_type_id == 1) {
      img_src = &ui_img_beer_mug_png;
      Serial.println("[UIBuilder] Setting beer icon");
    } else if (bev->beverage_category_type_id == 2) {
      img_src = &ui_img_red_wine_png;
      Serial.println("[UIBuilder] Setting wine icon");
    } else if (bev->beverage_category_type_id == 3) {
      img_src = &ui_img_cocktail_png;
      Serial.println("[UIBuilder] Setting cocktail icon");
    }

    if (img_src != nullptr) {
      lv_img_set_src(ui_ImageBeverage, img_src);
      lv_img_set_zoom(ui_ImageBeverage, 400);
      Serial.println("[UIBuilder] Icon set successfully");
    } else {
      Serial.println("[UIBuilder] No icon found for this category");
    }

    // Force refresh of the UI elements
    lv_obj_invalidate(ui_BarDispensed);
    lv_obj_invalidate(ui_ImageBeverage);

  }

  // Static callback wrapper
  static void beverage_card_clicked_static(lv_event_t * e);
};

#endif