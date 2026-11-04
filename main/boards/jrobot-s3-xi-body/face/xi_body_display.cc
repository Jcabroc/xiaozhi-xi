#include "xi_body_display.h"

#include <esp_log.h>
#include <cstring>

#define TAG "XiBodyDisplay"

XiBodyDisplay::~XiBodyDisplay() {
    if (face_engine_.IsInitialized() || clock_label_ != nullptr) {
        DisplayLockGuard lock(this);
        if (face_engine_.IsInitialized()) face_engine_.Deinitialize();
        if (clock_label_ != nullptr) {
            lv_obj_del(clock_label_);
            clock_label_ = nullptr;
        }
    }
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
    // Xi Body uses a face-first layout. Status and subtitle text stay hidden;
    // critical notifications remain handled by the inherited notification UI.
    if (status_label_ != nullptr) {
        lv_label_set_text(status_label_, "");
        lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
    }
    if (notification_label_ != nullptr) lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
    if (notification_timer_ != nullptr) esp_timer_stop(notification_timer_);
    if (bottom_bar_ != nullptr) lv_obj_add_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);

    clock_label_ = lv_label_create(lv_screen_active());
    lv_obj_set_size(clock_label_, 88, 24);
    lv_obj_set_style_text_align(clock_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(clock_label_, lv_color_hex(0x9AF8FC), 0);
    lv_label_set_text(clock_label_, "");
    lv_obj_align(clock_label_, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_add_flag(clock_label_, LV_OBJ_FLAG_HIDDEN);

    ApplyFaceTheme();
}

void XiBodyDisplay::SetStatus(const char* status) {
    DisplayLockGuard lock(this);
    if (status_label_ != nullptr) {
        lv_label_set_text(status_label_, "");
        lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
    }

    // The base display sends HH:MM through SetStatus while idle. Route only
    // that clock value to the dedicated lower-face label.
    const bool is_clock = status != nullptr && std::strlen(status) == 5 &&
                          status[0] >= '0' && status[0] <= '9' &&
                          status[1] >= '0' && status[1] <= '9' && status[2] == ':' &&
                          status[3] >= '0' && status[3] <= '9' &&
                          status[4] >= '0' && status[4] <= '9' && status[5] == '\0';
    if (clock_label_ != nullptr && is_clock) {
        lv_label_set_text(clock_label_, status);
        lv_obj_remove_flag(clock_label_, LV_OBJ_FLAG_HIDDEN);
    }
    last_status_update_time_ = std::chrono::system_clock::now();
}

void XiBodyDisplay::SetChatMessage(const char* role, const char* content) {
    // Chat text is presentation-only; suppress it here without touching the
    // XiaoZhi conversation or state transitions.
    (void)role;
    (void)content;
    DisplayLockGuard lock(this);
    if (chat_message_label_ != nullptr) lv_label_set_text(chat_message_label_, "");
    if (bottom_bar_ != nullptr) lv_obj_add_flag(bottom_bar_, LV_OBJ_FLAG_HIDDEN);
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
    if (clock_label_ != nullptr) lv_obj_set_style_text_color(clock_label_, cyan, 0);
}
