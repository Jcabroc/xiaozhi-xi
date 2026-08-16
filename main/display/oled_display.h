#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "lvgl_display.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <atomic>


class OledDisplay : public LvglDisplay {
private:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;

    lv_obj_t* top_bar_ = nullptr;
    lv_obj_t* status_bar_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* content_left_ = nullptr;
    lv_obj_t* content_right_ = nullptr;
    lv_obj_t* container_ = nullptr;
    lv_obj_t* side_bar_ = nullptr;
    lv_obj_t *emotion_label_ = nullptr;
    lv_obj_t* chat_message_label_ = nullptr;

    bool friendly_face_ = false;
    lv_obj_t* left_eye_ = nullptr;
    lv_obj_t* right_eye_ = nullptr;
    lv_obj_t* mouth_ = nullptr;
    lv_timer_t* face_timer_ = nullptr;
    uint16_t face_tick_ = 0;
    uint16_t next_blink_tick_ = 38;
    uint8_t blink_phase_ = 0;
    int eye_width_target_ = 32;
    int eye_height_target_ = 27;
    int eye_y_target_ = 7;
    int left_eye_x_target_ = 15;
    int right_eye_x_target_ = 81;
    int eye_width_current_ = 32;
    int eye_height_current_ = 27;
    int eye_y_current_ = 7;
    int left_eye_x_current_ = 15;
    int right_eye_x_current_ = 81;
    uint8_t face_mode_ = 0;
    bool voice_active_ = false;
    std::atomic<uint8_t> audio_level_{0};
    std::atomic<uint32_t> audio_packet_counter_{0};
    uint32_t last_audio_packet_counter_ = 0;
    uint8_t audio_silence_ticks_ = 3;
    uint8_t mouth_level_current_ = 0;
    bool speaking_ = false;

    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

    void SetupUI_128x64();
    void SetupUI_128x32();
    void SetupFriendlyFace();
    void UpdateFriendlyFace(const char* expression);
    void ShowFriendlyFace();
    void ShowProblemIcon(const char* icon);
    static bool IsFriendlyExpression(const char* expression);
    static void FaceTimerCallback(lv_timer_t* timer);

public:
    OledDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height,
        bool mirror_x, bool mirror_y, bool friendly_face = false);
    ~OledDisplay();

    virtual void SetupUI() override;
    virtual void SetStatus(const char* status) override;
    virtual void SetAudioLevel(uint8_t level) override;
    virtual void SetVoiceActivity(bool active) override;
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetTheme(Theme* theme) override;
};

#endif // OLED_DISPLAY_H
