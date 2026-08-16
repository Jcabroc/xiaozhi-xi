#include "oled_display.h"
#include "assets/lang_config.h"
#include "lvgl_theme.h"
#include "lvgl_font.h"

#include <string>
#include <algorithm>

#include <esp_log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <font_awesome.h>

#define TAG "OledDisplay"

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);
LV_FONT_DECLARE(BUILTIN_ICON_FONT);
LV_FONT_DECLARE(font_awesome_30_1);

OledDisplay::OledDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
    int width, int height, bool mirror_x, bool mirror_y, bool friendly_face)
    : panel_io_(panel_io), panel_(panel), friendly_face_(friendly_face) {
    width_ = width;
    height_ = height;

    auto text_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_TEXT_FONT);
    auto icon_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_ICON_FONT);
    auto large_icon_font = std::make_shared<LvglBuiltInFont>(&font_awesome_30_1);
    
    auto dark_theme = new LvglTheme("dark");
    dark_theme->set_text_font(text_font);
    dark_theme->set_icon_font(icon_font);
    dark_theme->set_large_icon_font(large_icon_font);

    auto& theme_manager = LvglThemeManager::GetInstance();
    theme_manager.RegisterTheme("dark", dark_theme);
    current_theme_ = dark_theme;

    ESP_LOGI(TAG, "Initialize LVGL");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
    port_cfg.task_stack = 6144;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Adding OLED display");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .control_handle = nullptr,
        .buffer_size = static_cast<uint32_t>(width_ * height_),
        .double_buffer = false,
        .trans_size = 0,
        .hres = static_cast<uint32_t>(width_),
        .vres = static_cast<uint32_t>(height_),
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };

    display_ = lvgl_port_add_disp(&display_cfg);
    if (display_ == nullptr) {
        ESP_LOGE(TAG, "Failed to add display");
        return;
    }

    // Note: SetupUI() should be called by Application::Initialize(), not in constructor
    // to ensure lvgl objects are created after the display is fully initialized.
}

void OledDisplay::SetupUI() {
    // Prevent duplicate calls - if already called, return early
    if (setup_ui_called_) {
        ESP_LOGW(TAG, "SetupUI() called multiple times, skipping duplicate call");
        return;
    }
    
    Display::SetupUI();  // Mark SetupUI as called
    if (height_ == 64) {
        SetupUI_128x64();
    } else {
        SetupUI_128x32();
    }
    if (friendly_face_ && height_ == 64) {
        SetupFriendlyFace();
    }
}

OledDisplay::~OledDisplay() {
    if (face_timer_ != nullptr) {
        lv_timer_delete(face_timer_);
        face_timer_ = nullptr;
    }
    if (content_ != nullptr) {
        lv_obj_del(content_);
    }

    bool is_128x64_layout = (top_bar_ != nullptr);
    if (status_bar_ != nullptr && is_128x64_layout) {
        status_label_ = nullptr;
        notification_label_ = nullptr;
        lv_obj_del(status_bar_);
    }
    if (top_bar_ != nullptr) {
        network_label_ = nullptr;
        mute_label_ = nullptr;
        battery_label_ = nullptr;
        lv_obj_del(top_bar_);
    }
    if (side_bar_ != nullptr) {
        if (!is_128x64_layout) {
            status_label_ = nullptr;
            notification_label_ = nullptr;
            network_label_ = nullptr;
            mute_label_ = nullptr;
            battery_label_ = nullptr;
        }
        lv_obj_del(side_bar_);
    }
    if (container_ != nullptr) {
        lv_obj_del(container_);
    }

    if (panel_ != nullptr) {
        esp_lcd_panel_del(panel_);
    }
    if (panel_io_ != nullptr) {
        esp_lcd_panel_io_del(panel_io_);
    }
    lvgl_port_deinit();
}

void OledDisplay::SetupFriendlyFace() {
    // Zhi uses the whole OLED for its face. Status is conveyed through expression;
    // exceptional conditions temporarily replace the face with a large icon.
    lv_obj_add_flag(top_bar_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(status_bar_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(content_, LV_VER_RES);
    lv_obj_add_flag(emotion_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(content_right_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(content_left_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(content_left_, lv_color_white(), 0);

    auto make_feature = [this](int width, int height, int x, int y, int radius) {
        lv_obj_t* feature = lv_obj_create(content_left_);
        lv_obj_set_size(feature, width, height);
        lv_obj_set_pos(feature, x, y);
        lv_obj_set_style_pad_all(feature, 0, 0);
        lv_obj_set_style_border_width(feature, 0, 0);
        lv_obj_set_style_bg_color(feature, lv_color_black(), 0);
        lv_obj_set_style_radius(feature, radius, 0);
        lv_obj_set_scrollbar_mode(feature, LV_SCROLLBAR_MODE_OFF);
        return feature;
    };

    // Rounded rectangles evoke Vector-like eyes without copying its exact artwork.
    left_eye_ = make_feature(28, 23, 19, 9, 8);
    right_eye_ = make_feature(28, 23, 81, 9, 8);
    mouth_ = make_feature(24, 3, 52, 49, 2);
    face_timer_ = lv_timer_create(FaceTimerCallback, 70, this);
    UpdateFriendlyFace("neutral");
}

void OledDisplay::FaceTimerCallback(lv_timer_t* timer) {
    auto self = static_cast<OledDisplay*>(lv_timer_get_user_data(timer));
    if (self == nullptr || self->left_eye_ == nullptr) {
        return;
    }

    self->face_tick_++;
    if (lv_obj_has_flag(self->left_eye_, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }

    // Blinks have a short closing and opening phase. Between blinks the eyes
    // ease toward the expression targets instead of snapping to a fixed shape.
    if (self->face_tick_ >= self->next_blink_tick_ && self->blink_phase_ == 0) {
        self->blink_phase_ = 1;
    }
    int desired_height = self->eye_height_target_;
    int desired_y = self->eye_y_target_;
    int left_x = self->left_eye_x_target_;
    int right_x = self->right_eye_x_target_;

    // Tiny state-dependent movements give the eyes intention without making
    // the 128x64 face visually noisy.
    if (self->face_mode_ == 1) { // listening
        if (self->voice_active_) {
            desired_height += ((self->face_tick_ / 3) % 2) ? 2 : 0;
            desired_y -= 1;
        }
    } else if (self->face_mode_ == 2) { // thinking
        const int glance = ((self->face_tick_ / 12) % 2) ? 4 : -2;
        left_x += glance;
        right_x += glance;
        if ((self->face_tick_ / 9) % 3 == 0) {
            desired_height -= 4;
            desired_y += 2;
        }
    } else if (self->face_mode_ == 3) { // speaking
        const uint8_t level = self->audio_level_.load(std::memory_order_relaxed);
        desired_height -= level / 28;
        desired_y += level / 55;
    } else if (self->face_mode_ == 0) { // idle: an occasional gentle glance
        const uint16_t idle_phase = self->face_tick_ % 150;
        if (idle_phase > 115 && idle_phase < 135) {
            left_x += 2;
            right_x += 2;
        }
    }
    if (self->blink_phase_ != 0) {
        if (self->blink_phase_ <= 2) {
            desired_height = 3;
            desired_y = 20;
        }
        if (++self->blink_phase_ > 4) {
            self->blink_phase_ = 0;
            // Vary the interval deterministically so the face does not look robotic.
            self->next_blink_tick_ = self->face_tick_ + 42 + ((self->face_tick_ * 13) % 46);
        }
    }

    auto ease = [](int current, int target, int step) {
        if (current < target) return std::min(current + step, target);
        if (current > target) return std::max(current - step, target);
        return current;
    };
    self->eye_width_current_ = ease(self->eye_width_current_, self->eye_width_target_, 2);
    self->eye_height_current_ = ease(self->eye_height_current_, desired_height, 5);
    self->eye_y_current_ = ease(self->eye_y_current_, desired_y, 3);
    self->left_eye_x_current_ = ease(self->left_eye_x_current_, left_x, 1);
    self->right_eye_x_current_ = ease(self->right_eye_x_current_, right_x, 1);
    int left_height = self->eye_height_current_;
    int right_height = self->eye_height_current_;
    int left_y = self->eye_y_current_;
    int right_y = self->eye_y_current_;
    if (self->blink_phase_ == 0) {
        if (self->face_mode_ == 2) {
            // A raised/squinted eye reads as curiosity on a tiny monochrome face.
            right_height = std::max(8, right_height - 7);
            right_y += 4;
        } else if (self->face_mode_ == 3) {
            // Asymmetric micro-squints keep speech from looking like a static mask.
            if ((self->face_tick_ / 3) % 2) {
                left_height = std::max(10, left_height - 2);
                left_y += 1;
            } else {
                right_height = std::max(10, right_height - 2);
                right_y += 1;
            }
        } else if (self->face_mode_ == 0 && (self->face_tick_ % 190) > 168) {
            left_height = std::max(12, left_height - 4);
            left_y += 2;
        }
    }
    lv_obj_set_size(self->left_eye_, self->eye_width_current_, left_height);
    lv_obj_set_size(self->right_eye_, self->eye_width_current_, right_height);
    lv_obj_set_pos(self->left_eye_, self->left_eye_x_current_, left_y);
    lv_obj_set_pos(self->right_eye_, self->right_eye_x_current_, right_y);

    // Mouth motion is gated by actual PCM packets sent to Xi's speaker, never
    // by the logical conversation state. Microphone activity cannot trigger it.
    const uint32_t packet_counter = self->audio_packet_counter_.load(std::memory_order_relaxed);
    if (packet_counter != self->last_audio_packet_counter_) {
        self->last_audio_packet_counter_ = packet_counter;
        self->audio_silence_ticks_ = 0;
    } else if (self->audio_silence_ticks_ < 255) {
        self->audio_silence_ticks_++;
    }
    const uint8_t incoming = self->audio_level_.exchange(0, std::memory_order_relaxed);
    if (self->audio_silence_ticks_ <= 2) {
        // Consume the loudest packet received since the last frame. Attack is
        // immediate; release is deliberately slower so syllables remain visible.
        if (incoming > self->mouth_level_current_) {
            self->mouth_level_current_ = incoming;
        } else {
            self->mouth_level_current_ = self->mouth_level_current_ > 8
                ? self->mouth_level_current_ - 8 : 0;
        }
        const uint8_t level = self->mouth_level_current_;
        if (level < 5) {
            lv_obj_set_size(self->mouth_, 20, 2);
            lv_obj_set_pos(self->mouth_, 54, 51);
            lv_obj_set_style_radius(self->mouth_, 1, 0);
        } else {
            // Alternate wide and rounded visemes while amplitude controls size.
            int width = 10 + level * 22 / 100;
            int height = 4 + level * 13 / 100;
            if ((self->face_tick_ / 2) % 3 == 0) {
                width = std::max(8, width - 7);
                height = std::min(18, height + 3);
            }
            lv_obj_set_size(self->mouth_, width, height);
            lv_obj_set_pos(self->mouth_, (128 - width) / 2, 57 - height);
            lv_obj_set_style_radius(self->mouth_, height / 2, 0);
        }
    } else {
        self->mouth_level_current_ = 0;
        // Rest/listen mouth: intentionally quiet, short and delicate.
        const int rest_width = self->face_mode_ == 1 ? 12 : 16;
        lv_obj_set_size(self->mouth_, rest_width, 2);
        lv_obj_set_pos(self->mouth_, (128 - rest_width) / 2, 51);
        lv_obj_set_style_radius(self->mouth_, 1, 0);
    }
}

void OledDisplay::SetAudioLevel(uint8_t level) {
    if (friendly_face_) {
        // Peak hold: a trailing quiet packet must not erase a vowel peak before
        // the 70 ms face timer has had a chance to display it.
        uint8_t current = audio_level_.load(std::memory_order_relaxed);
        while (level > current && !audio_level_.compare_exchange_weak(
            current, level, std::memory_order_relaxed)) {
        }
        audio_packet_counter_.fetch_add(1, std::memory_order_relaxed);
    }
}

void OledDisplay::SetVoiceActivity(bool active) {
    if (!friendly_face_) {
        return;
    }
    DisplayLockGuard lock(this);
    voice_active_ = active;
    if (face_mode_ == 1) {
        eye_width_target_ = active ? 32 : 30;
        eye_height_target_ = active ? 30 : 28;
        left_eye_x_target_ = active ? 14 : 16;
        right_eye_x_target_ = 82;
    }
}

bool OledDisplay::IsFriendlyExpression(const char* expression) {
    static const char* expressions[] = {
        "neutral", "happy", "laughing", "funny", "sad", "angry", "crying",
        "loving", "embarrassed", "surprised", "shocked", "thinking", "winking",
        "cool", "relaxed", "delicious", "kissy", "confident", "sleepy", "silly",
        "confused", "listening", "listening_active", "speaking", "connecting", "microchip_ai"
    };
    for (const char* known : expressions) {
        if (strcmp(expression, known) == 0) {
            return true;
        }
    }
    return false;
}

void OledDisplay::ShowFriendlyFace() {
    lv_obj_add_flag(emotion_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(mouth_, LV_OBJ_FLAG_HIDDEN);
}

void OledDisplay::ShowProblemIcon(const char* icon) {
    const char* utf8 = font_awesome_get_utf8(icon);
    if (utf8 == nullptr) {
        utf8 = FONT_AWESOME_TRIANGLE_EXCLAMATION;
    }
    speaking_ = false;
    lv_obj_add_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mouth_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(emotion_label_, utf8);
    lv_obj_set_style_text_font(emotion_label_,
        static_cast<LvglTheme*>(current_theme_)->large_icon_font()->font(), 0);
    lv_obj_center(emotion_label_);
    lv_obj_remove_flag(emotion_label_, LV_OBJ_FLAG_HIDDEN);
}

void OledDisplay::UpdateFriendlyFace(const char* expression) {
    if (!friendly_face_ || left_eye_ == nullptr || expression == nullptr) {
        return;
    }

    ShowFriendlyFace();
    speaking_ = strcmp(expression, "speaking") == 0;
    face_mode_ = 0;
    int eye_width = 28;
    int eye_height = 23;
    int left_x = 19;
    int right_x = 81;
    int mouth_width = 16;
    int mouth_height = 2;
    int mouth_x = 56;
    int mouth_y = 51;

    if (strcmp(expression, "listening") == 0) {
        face_mode_ = 1;
        eye_width = 30;
        eye_height = 28;
        left_x = 16;
        right_x = 82;
        mouth_width = 12;
        mouth_height = 2;
        mouth_x = 58;
        mouth_y = 51;
    } else if (strcmp(expression, "listening_active") == 0) {
        face_mode_ = 1;
        voice_active_ = true;
        eye_width = 32;
        eye_height = 30;
        left_x = 14;
        right_x = 82;
        mouth_width = 12;
        mouth_height = 2;
        mouth_x = 58;
        mouth_y = 51;
    } else if (strcmp(expression, "thinking") == 0) {
        face_mode_ = 2;
        eye_width = 28;
        eye_height = 22;
        left_x = 19;
        right_x = 79;
        mouth_width = 10;
        mouth_height = 2;
        mouth_x = 59;
        mouth_y = 51;
    } else if (strcmp(expression, "connecting") == 0) {
        face_mode_ = 4;
        eye_width = 22;
        left_x = 22;
        right_x = 84;
        mouth_width = 24;
        mouth_x = 52;
    } else if (strcmp(expression, "sleepy") == 0) {
        eye_height = 4;
        mouth_width = 20;
        mouth_x = 54;
    } else if (speaking_) {
        face_mode_ = 3;
    }

    eye_width_target_ = eye_width;
    eye_height_target_ = eye_height;
    eye_y_target_ = eye_height <= 4 ? 19 : 7;
    left_eye_x_target_ = left_x;
    right_eye_x_target_ = right_x;
    lv_obj_set_size(mouth_, mouth_width, mouth_height);
    lv_obj_set_pos(mouth_, mouth_x, mouth_y);
    lv_obj_set_style_radius(left_eye_, 6, 0);
    lv_obj_set_style_radius(right_eye_, 6, 0);
}

void OledDisplay::SetStatus(const char* status) {
    LvglDisplay::SetStatus(status);
    if (!friendly_face_ || status == nullptr) {
        return;
    }
    if (strcmp(status, Lang::Strings::LISTENING) == 0) {
        UpdateFriendlyFace("listening");
    } else if (strcmp(status, Lang::Strings::SPEAKING) == 0) {
        UpdateFriendlyFace("speaking");
    } else if (strcmp(status, Lang::Strings::CONNECTING) == 0) {
        UpdateFriendlyFace("connecting");
    } else if (strcmp(status, Lang::Strings::STANDBY) == 0) {
        UpdateFriendlyFace("neutral");
    }
}

bool OledDisplay::Lock(int timeout_ms) {
    return lvgl_port_lock(timeout_ms);
}

void OledDisplay::Unlock() {
    lvgl_port_unlock();
}

void OledDisplay::SetChatMessage(const char* role, const char* content) {
    DisplayLockGuard lock(this);
    if (chat_message_label_ == nullptr) {
        return;
    }

    // Replace all newlines with spaces
    std::string content_str = content;
    std::replace(content_str.begin(), content_str.end(), '\n', ' ');

    if (content_right_ == nullptr) {
        lv_label_set_text(chat_message_label_, content_str.c_str());
    } else {
        if (content == nullptr || content[0] == '\0') {
            lv_obj_add_flag(content_right_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(chat_message_label_, content_str.c_str());
            lv_obj_remove_flag(content_right_, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void OledDisplay::SetupUI_128x64() {
    DisplayLockGuard lock(this);

    auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
    auto text_font = lvgl_theme->text_font()->font();
    auto icon_font = lvgl_theme->icon_font()->font();
    auto large_icon_font = lvgl_theme->large_icon_font()->font();

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, text_font, 0);
    lv_obj_set_style_text_color(screen, lv_color_black(), 0);

    /* Container */
    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_row(container_, 0, 0);

    /* Layer 1: Top bar - for status icons */
    top_bar_ = lv_obj_create(container_);
    lv_obj_set_size(top_bar_, LV_HOR_RES, 16);
    lv_obj_set_style_radius(top_bar_, 0, 0);
    lv_obj_set_style_bg_opa(top_bar_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_bar_, 0, 0);
    lv_obj_set_style_pad_all(top_bar_, 0, 0);
    lv_obj_set_flex_flow(top_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(top_bar_, LV_SCROLLBAR_MODE_OFF);

    network_label_ = lv_label_create(top_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, icon_font, 0);

    lv_obj_t* right_icons = lv_obj_create(top_bar_);
    lv_obj_set_size(right_icons, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_icons, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_icons, 0, 0);
    lv_obj_set_style_pad_all(right_icons, 0, 0);
    lv_obj_set_flex_flow(right_icons, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_icons, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    mute_label_ = lv_label_create(right_icons);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, icon_font, 0);

    battery_label_ = lv_label_create(right_icons);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, icon_font, 0);

    /* Layer 2: Status bar - for center text labels */
    status_bar_ = lv_obj_create(screen);
    lv_obj_set_size(status_bar_, LV_HOR_RES, 16);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_style_bg_opa(status_bar_, LV_OPA_TRANSP, 0);  // Transparent background
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_scrollbar_mode(status_bar_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_layout(status_bar_, LV_LAYOUT_NONE, 0);  // Use absolute positioning
    lv_obj_align(status_bar_, LV_ALIGN_TOP_MID, 0, 0);  // Overlap with top_bar_

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_width(notification_label_, LV_HOR_RES);
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_align(notification_label_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_width(status_label_, LV_HOR_RES);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);
    lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, 0);

    /* Content */
    content_ = lv_obj_create(container_);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_style_pad_all(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_main_place(content_, LV_FLEX_ALIGN_CENTER, 0);

    content_left_ = lv_obj_create(content_);
    lv_obj_set_size(content_left_, 32, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(content_left_, 0, 0);
    lv_obj_set_style_border_width(content_left_, 0, 0);

    emotion_label_ = lv_label_create(content_left_);
    lv_obj_set_style_text_font(emotion_label_, large_icon_font, 0);
    lv_label_set_text(emotion_label_, FONT_AWESOME_MICROCHIP_AI);
    lv_obj_center(emotion_label_);
    lv_obj_set_style_pad_top(emotion_label_, 8, 0);

    content_right_ = lv_obj_create(content_);
    lv_obj_set_size(content_right_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(content_right_, 0, 0);
    lv_obj_set_style_border_width(content_right_, 0, 0);
    lv_obj_set_flex_grow(content_right_, 1);
    lv_obj_add_flag(content_right_, LV_OBJ_FLAG_HIDDEN);

    chat_message_label_ = lv_label_create(content_right_);
    lv_label_set_text(chat_message_label_, "");
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(chat_message_label_, width_ - 32);
    lv_obj_set_style_pad_top(chat_message_label_, 14, 0);

    // Start scrolling subtitle after a delay
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_delay(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_obj_set_style_anim(chat_message_label_, &a, LV_PART_MAIN);
    lv_obj_set_style_anim_duration(chat_message_label_, lv_anim_speed_clamped(60, 300, 60000), LV_PART_MAIN);

    low_battery_popup_ = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, text_font->line_height * 2);
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(low_battery_popup_, lv_color_black(), 0);
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);
    low_battery_label_ = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label_, Lang::Strings::BATTERY_NEED_CHARGE);
    lv_obj_set_style_text_color(low_battery_label_, lv_color_white(), 0);
    lv_obj_center(low_battery_label_);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
}

void OledDisplay::SetupUI_128x32() {
    DisplayLockGuard lock(this);

    auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
    auto text_font = lvgl_theme->text_font()->font();
    auto icon_font = lvgl_theme->icon_font()->font();
    auto large_icon_font = lvgl_theme->large_icon_font()->font();

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, text_font, 0);

    /* Container */
    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_column(container_, 0, 0);

    /* Emotion label on the left side */
    content_ = lv_obj_create(container_);
    lv_obj_set_size(content_, 32, 32);
    lv_obj_set_style_pad_all(content_, 0, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_style_radius(content_, 0, 0);

    emotion_label_ = lv_label_create(content_);
    lv_obj_set_style_text_font(emotion_label_, large_icon_font, 0);
    lv_label_set_text(emotion_label_, FONT_AWESOME_MICROCHIP_AI);
    lv_obj_center(emotion_label_);

    /* Right side */
    side_bar_ = lv_obj_create(container_);
    lv_obj_set_size(side_bar_, width_ - 32, 32);
    lv_obj_set_flex_flow(side_bar_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(side_bar_, 0, 0);
    lv_obj_set_style_border_width(side_bar_, 0, 0);
    lv_obj_set_style_radius(side_bar_, 0, 0);
    lv_obj_set_style_pad_row(side_bar_, 0, 0);

    /* Status bar */
    status_bar_ = lv_obj_create(side_bar_);
    lv_obj_set_size(status_bar_, width_ - 32, 16);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_column(status_bar_, 0, 0);

    status_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(status_label_, 1);
    lv_obj_set_style_pad_left(status_label_, 2, 0);
    lv_label_set_text(status_label_, Lang::Strings::INITIALIZING);

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_flex_grow(notification_label_, 1);
    lv_obj_set_style_pad_left(notification_label_, 2, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);

    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, icon_font, 0);

    network_label_ = lv_label_create(status_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, icon_font, 0);

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, icon_font, 0);

    chat_message_label_ = lv_label_create(side_bar_);
    lv_obj_set_size(chat_message_label_, width_ - 32, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_left(chat_message_label_, 2, 0);
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(chat_message_label_, "");

    // Start scrolling subtitle after a delay
    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_delay(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_obj_set_style_anim(chat_message_label_, &a, LV_PART_MAIN);
    lv_obj_set_style_anim_duration(chat_message_label_, lv_anim_speed_clamped(60, 300, 60000), LV_PART_MAIN);
}

void OledDisplay::SetEmotion(const char* emotion) {
    if (friendly_face_) {
        DisplayLockGuard lock(this);
        if (emotion != nullptr && IsFriendlyExpression(emotion)) {
            UpdateFriendlyFace(emotion);
        } else {
            ShowProblemIcon(emotion);
        }
        return;
    }
    const char* utf8 = font_awesome_get_utf8(emotion);
    DisplayLockGuard lock(this);
    if (emotion_label_ == nullptr) {
        return;
    }
    if (utf8 != nullptr) {
        lv_label_set_text(emotion_label_, utf8);
    } else {
        lv_label_set_text(emotion_label_, FONT_AWESOME_NEUTRAL);
    }
}

void OledDisplay::SetTheme(Theme* theme) {
    DisplayLockGuard lock(this);

    auto lvgl_theme = static_cast<LvglTheme*>(theme);
    auto text_font = lvgl_theme->text_font()->font();

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, text_font, 0);
}
