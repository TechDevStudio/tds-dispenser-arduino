#include "UIBuilder.h"

// Global reference to access the callback from main file
extern void onBeverageCardClicked(int beverageIndex);

// Debug callback for scroll container
void scroll_container_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        Serial.println("[DEBUG] Scroll container pressed");
    } else if (code == LV_EVENT_SCROLL_BEGIN) {
        Serial.println("[DEBUG] Scroll begin");
    } else if (code == LV_EVENT_SCROLL_END) {
        Serial.println("[DEBUG] Scroll end");
    }
}

void UIBuilder::beverage_card_clicked_static(lv_event_t * e) {
    // Get the card object that was clicked
    lv_obj_t* card = (lv_obj_t*)lv_event_get_target(e);

    // Get the beverage index stored as user data in the card object
    int beverageIndex = (int)(uintptr_t)lv_obj_get_user_data(card);

    // Add visual feedback - highlight selected card
    lv_obj_set_style_border_color(card, lv_color_hex(0xFFD700), LV_PART_MAIN | LV_STATE_DEFAULT); // Gold border
    lv_obj_set_style_border_width(card, 5, LV_PART_MAIN | LV_STATE_DEFAULT);

    Serial.print("[UIBuilder] Card clicked for beverage index: ");
    Serial.println(beverageIndex);

    // Call the external callback
    onBeverageCardClicked(beverageIndex);
}