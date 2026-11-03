#ifndef LUMI_FACE_ENGINE_H
#define LUMI_FACE_ENGINE_H

#include <lvgl.h>

#include <cstdint>

class LumiFaceEngine {
public:
    enum class GazeDirection : uint8_t {
        kCenter,
        kLeft,
        kRight,
        kUp,
        kDown,
    };

    // All methods that touch LVGL must be called while the owning display lock is held.
    bool Initialize(lv_obj_t* parent);
    void Deinitialize();
    void SetEmotion(const char* emotion);
    void LookAt(GazeDirection direction, uint32_t hold_ms = 650);
    bool IsInitialized() const { return parent_ != nullptr; }

private:
    struct Eye {
        lv_obj_t* shell = nullptr;
        lv_obj_t* iris = nullptr;
        lv_obj_t* pupil = nullptr;
        lv_obj_t* highlight = nullptr;
        lv_coord_t x = 0;
        lv_coord_t y = 0;
    };

    static void EventTimerCallback(lv_timer_t* timer);
    static void FrameTimerCallback(lv_timer_t* timer);
    static void GazeHoldTimerCallback(lv_timer_t* timer);

    void HandleScheduledEvent();
    void StartBlink();
    void StartGaze(float x, float y, uint32_t duration_ms, bool returning, uint32_t hold_ms);
    void StartCenterReturn();
    void EnsureFrameTimer();
    void UpdateFrame(uint32_t now);
    void UpdateEyes(float open_fraction);
    uint32_t RandomRange(uint32_t minimum, uint32_t maximum) const;

    lv_obj_t* parent_ = nullptr;
    Eye eyes_[2];
    lv_timer_t* event_timer_ = nullptr;
    lv_timer_t* frame_timer_ = nullptr;
    lv_timer_t* gaze_hold_timer_ = nullptr;

    float gaze_x_ = 0.0f;
    float gaze_y_ = 0.0f;
    float gaze_start_x_ = 0.0f;
    float gaze_start_y_ = 0.0f;
    float gaze_target_x_ = 0.0f;
    float gaze_target_y_ = 0.0f;
    uint32_t gaze_started_ms_ = 0;
    uint32_t gaze_duration_ms_ = 0;
    uint32_t blink_started_ms_ = 0;
    uint32_t gaze_hold_ms_ = 0;
    bool gaze_animating_ = false;
    bool gaze_returning_ = false;
    bool blinking_ = false;
};

#endif  // LUMI_FACE_ENGINE_H
