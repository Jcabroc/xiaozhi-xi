#include "xi_body_display.h"

#include <esp_log.h>

#define TAG "XiBodyDisplay"

XiBodyDisplay::~XiBodyDisplay() {
    if (!face_engine_.IsInitialized()) return;
    DisplayLockGuard lock(this);
    face_engine_.Deinitialize();
}

void XiBodyDisplay::SetupUI() {
    SpiLcdDisplay::SetupUI();
    if (!setup_ui_called_ || emoji_box_ == nullptr) return;

    DisplayLockGuard lock(this);
    lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
    if (!face_engine_.Initialize(emoji_box_)) {
        ESP_LOGE(TAG, "Failed to initialize Lumi Face Engine");
        return;
    }
    ApplyFaceTheme();
}

void XiBodyDisplay::SetEmotion(const char* emotion) {
    if (!face_engine_.IsInitialized()) {
        SpiLcdDisplay::SetEmotion(emotion);
        return;
    }
    DisplayLockGuard lock(this);
    face_engine_.SetEmotion(emotion);
}

void XiBodyDisplay::SetTheme(Theme* theme) {
    SpiLcdDisplay::SetTheme(theme);
    if (!face_engine_.IsInitialized()) return;
    DisplayLockGuard lock(this);
    ApplyFaceTheme();
}

void XiBodyDisplay::ApplyFaceTheme() {
    const lv_color_t black = lv_color_hex(0x000000);
    const lv_color_t white = lv_color_hex(0xE8FCFF);
    const lv_color_t cyan = lv_color_hex(0x9AF8FC);

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, black, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(container_, black, 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_image_src(container_, nullptr, 0);

    if (top_bar_ != nullptr) {
        lv_obj_set_style_bg_opa(top_bar_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(network_label_, cyan, 0);
        lv_obj_set_style_text_color(mute_label_, cyan, 0);
        lv_obj_set_style_text_color(battery_label_, cyan, 0);
    }
    if (status_label_ != nullptr) lv_obj_set_style_text_color(status_label_, white, 0);
    if (notification_label_ != nullptr) lv_obj_set_style_text_color(notification_label_, white, 0);
    if (bottom_bar_ != nullptr) {
        lv_obj_set_style_bg_color(bottom_bar_, lv_color_hex(0x00171B), 0);
        lv_obj_set_style_bg_opa(bottom_bar_, LV_OPA_80, 0);
    }
    if (chat_message_label_ != nullptr) lv_obj_set_style_text_color(chat_message_label_, white, 0);
}
