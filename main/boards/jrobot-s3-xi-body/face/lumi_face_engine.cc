#include "lumi_face_engine.h"

#include <esp_random.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr lv_coord_t kEyeWidth = 78;
constexpr lv_coord_t kEyeHeight = 96;
constexpr lv_coord_t kIrisWidth = 32;
constexpr lv_coord_t kIrisHeight = 40;
constexpr lv_coord_t kPupilWidth = 16;
constexpr lv_coord_t kPupilHeight = 22;
constexpr lv_coord_t kHighlightSize = 7;
constexpr lv_coord_t kLeftEyeX = 31;
constexpr lv_coord_t kRightEyeX = 131;
constexpr lv_coord_t kEyeY = 72;
constexpr uint32_t kFramePeriodMs = 33;
constexpr uint32_t kBlinkCloseMs = 95;
constexpr uint32_t kBlinkHoldMs = 35;
constexpr uint32_t kBlinkOpenMs = 155;
constexpr uint32_t kBlinkTotalMs = kBlinkCloseMs + kBlinkHoldMs + kBlinkOpenMs;
constexpr float kMaxGazeX = 12.0f;
constexpr float kMaxGazeY = 10.0f;

lv_obj_t* CreateRoundedShape(lv_obj_t* parent, lv_coord_t width, lv_coord_t height,
                             lv_color_t color, lv_coord_t radius) {
    lv_obj_t* shape = lv_obj_create(parent);
    lv_obj_remove_style_all(shape);
    lv_obj_set_size(shape, width, height);
    lv_obj_set_style_bg_color(shape, color, 0);
    lv_obj_set_style_bg_opa(shape, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shape, 0, 0);
    lv_obj_set_style_pad_all(shape, 0, 0);
    lv_obj_set_style_radius(shape, radius, 0);
    lv_obj_remove_flag(shape, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(shape, LV_OBJ_FLAG_CLICKABLE);
    return shape;
}

float SmoothStep(float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}
}  // namespace

bool LumiFaceEngine::Initialize(lv_obj_t* parent) {
    if (parent == nullptr || parent_ != nullptr) return false;

    parent_ = parent;
    lv_obj_set_size(parent_, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(parent_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(parent_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(parent_, 0, 0);
    lv_obj_set_style_pad_all(parent_, 0, 0);
    lv_obj_remove_flag(parent_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(parent_, LV_OBJ_FLAG_CLICKABLE);

    const lv_color_t cyan = lv_color_hex(0x58E7EE);
    const lv_color_t iris_color = lv_color_hex(0x087C88);
    const lv_color_t pupil_color = lv_color_hex(0x00323A);
    const lv_color_t highlight_color = lv_color_hex(0xD6FFFF);
    const lv_coord_t eye_x[] = {kLeftEyeX, kRightEyeX};

    for (size_t i = 0; i < 2; ++i) {
        Eye& eye = eyes_[i];
        eye.x = eye_x[i];
        eye.y = kEyeY;
        eye.shell = CreateRoundedShape(parent_, kEyeWidth, kEyeHeight, cyan, LV_RADIUS_CIRCLE);
        lv_obj_set_pos(eye.shell, eye.x, eye.y);

        eye.iris = CreateRoundedShape(parent_, kIrisWidth, kIrisHeight, iris_color, LV_RADIUS_CIRCLE);
        eye.pupil = CreateRoundedShape(parent_, kPupilWidth, kPupilHeight, pupil_color, LV_RADIUS_CIRCLE);
        eye.highlight = CreateRoundedShape(parent_, kHighlightSize, kHighlightSize,
                                           highlight_color, LV_RADIUS_CIRCLE);
    }

    UpdateEyes(1.0f);
    event_timer_ = lv_timer_create(EventTimerCallback, RandomRange(2600, 4200), this);
    if (event_timer_ == nullptr) {
        Deinitialize();
        return false;
    }
    return true;
}

void LumiFaceEngine::Deinitialize() {
    if (event_timer_ != nullptr) {
        lv_timer_del(event_timer_);
        event_timer_ = nullptr;
    }
    if (frame_timer_ != nullptr) {
        lv_timer_del(frame_timer_);
        frame_timer_ = nullptr;
    }
    if (gaze_hold_timer_ != nullptr) {
        lv_timer_del(gaze_hold_timer_);
        gaze_hold_timer_ = nullptr;
    }
    for (Eye& eye : eyes_) {
        if (eye.shell != nullptr) lv_obj_del(eye.shell);
        if (eye.iris != nullptr) lv_obj_del(eye.iris);
        if (eye.pupil != nullptr) lv_obj_del(eye.pupil);
        if (eye.highlight != nullptr) lv_obj_del(eye.highlight);
        eye = {};
    }
    parent_ = nullptr;
    blinking_ = false;
    gaze_animating_ = false;
}

void LumiFaceEngine::SetEmotion(const char* emotion) {
    if (emotion == nullptr || std::strcmp(emotion, "neutral") == 0 ||
        std::strcmp(emotion, "normal") == 0 || emotion[0] == '\0') {
        LookAt(GazeDirection::kCenter, 0);
    } else if (std::strcmp(emotion, "thinking") == 0 || std::strcmp(emotion, "curious") == 0) {
        LookAt(GazeDirection::kUp, 750);
    } else {
        // The v1 face keeps the emotion interface but has only the neutral eye shape.
        LookAt(GazeDirection::kCenter, 0);
    }
}

void LumiFaceEngine::LookAt(GazeDirection direction, uint32_t hold_ms) {
    float x = 0.0f;
    float y = 0.0f;
    switch (direction) {
        case GazeDirection::kLeft: x = -1.0f; break;
        case GazeDirection::kRight: x = 1.0f; break;
        case GazeDirection::kUp: y = -1.0f; break;
        case GazeDirection::kDown: y = 1.0f; break;
        case GazeDirection::kCenter: break;
    }
    StartGaze(x, y, 260, x == 0.0f && y == 0.0f, hold_ms);
}

void LumiFaceEngine::EventTimerCallback(lv_timer_t* timer) {
    auto* self = static_cast<LumiFaceEngine*>(lv_timer_get_user_data(timer));
    if (self != nullptr) self->HandleScheduledEvent();
}

void LumiFaceEngine::FrameTimerCallback(lv_timer_t* timer) {
    auto* self = static_cast<LumiFaceEngine*>(lv_timer_get_user_data(timer));
    if (self == nullptr) return;
    self->UpdateFrame(lv_tick_get());
    if (!self->blinking_ && !self->gaze_animating_) {
        self->frame_timer_ = nullptr;
        lv_timer_del(timer);
    }
}

void LumiFaceEngine::GazeHoldTimerCallback(lv_timer_t* timer) {
    auto* self = static_cast<LumiFaceEngine*>(lv_timer_get_user_data(timer));
    if (self == nullptr) return;
    self->gaze_hold_timer_ = nullptr;
    self->StartCenterReturn();
}

void LumiFaceEngine::HandleScheduledEvent() {
    if (RandomRange(0, 99) < 78) {
        StartBlink();
        lv_timer_set_period(event_timer_, RandomRange(2600, 4700));
        return;
    }

    const GazeDirection directions[] = {
        GazeDirection::kLeft, GazeDirection::kRight,
        GazeDirection::kUp, GazeDirection::kDown,
    };
    const auto direction = directions[RandomRange(0, 3)];
    float x = 0.0f;
    float y = 0.0f;
    switch (direction) {
        case GazeDirection::kLeft: x = -1.0f; break;
        case GazeDirection::kRight: x = 1.0f; break;
        case GazeDirection::kUp: y = -1.0f; break;
        case GazeDirection::kDown: y = 1.0f; break;
        case GazeDirection::kCenter: break;
    }
    StartGaze(x, y, 320, false, RandomRange(450, 850));
    lv_timer_set_period(event_timer_, RandomRange(4200, 6800));
}

void LumiFaceEngine::StartBlink() {
    if (blinking_) return;
    blinking_ = true;
    blink_started_ms_ = lv_tick_get();
    EnsureFrameTimer();
}

void LumiFaceEngine::StartGaze(float x, float y, uint32_t duration_ms, bool returning, uint32_t hold_ms) {
    if (gaze_hold_timer_ != nullptr) {
        lv_timer_del(gaze_hold_timer_);
        gaze_hold_timer_ = nullptr;
    }
    gaze_start_x_ = gaze_x_;
    gaze_start_y_ = gaze_y_;
    gaze_target_x_ = std::clamp(x, -1.0f, 1.0f);
    gaze_target_y_ = std::clamp(y, -1.0f, 1.0f);
    gaze_duration_ms_ = std::max<uint32_t>(duration_ms, 1);
    gaze_started_ms_ = lv_tick_get();
    gaze_hold_ms_ = hold_ms;
    gaze_returning_ = returning;
    gaze_animating_ = true;
    EnsureFrameTimer();
}

void LumiFaceEngine::StartCenterReturn() {
    StartGaze(0.0f, 0.0f, 300, true, 0);
}

void LumiFaceEngine::EnsureFrameTimer() {
    if (frame_timer_ == nullptr) {
        frame_timer_ = lv_timer_create(FrameTimerCallback, kFramePeriodMs, this);
    }
}

void LumiFaceEngine::UpdateFrame(uint32_t now) {
    if (gaze_animating_) {
        const float elapsed = static_cast<float>(now - gaze_started_ms_);
        const float progress = SmoothStep(elapsed / static_cast<float>(gaze_duration_ms_));
        gaze_x_ = gaze_start_x_ + (gaze_target_x_ - gaze_start_x_) * progress;
        gaze_y_ = gaze_start_y_ + (gaze_target_y_ - gaze_start_y_) * progress;
        if (elapsed >= gaze_duration_ms_) {
            gaze_x_ = gaze_target_x_;
            gaze_y_ = gaze_target_y_;
            gaze_animating_ = false;
            if (!gaze_returning_ && (gaze_x_ != 0.0f || gaze_y_ != 0.0f)) {
                gaze_hold_timer_ = lv_timer_create(GazeHoldTimerCallback, gaze_hold_ms_, this);
                if (gaze_hold_timer_ != nullptr) lv_timer_set_repeat_count(gaze_hold_timer_, 1);
            } else {
                gaze_returning_ = false;
            }
        }
    }

    float open_fraction = 1.0f;
    if (blinking_) {
        const uint32_t elapsed = now - blink_started_ms_;
        if (elapsed < kBlinkCloseMs) {
            open_fraction = 1.0f - SmoothStep(static_cast<float>(elapsed) / kBlinkCloseMs);
        } else if (elapsed < kBlinkCloseMs + kBlinkHoldMs) {
            open_fraction = 0.025f;
        } else if (elapsed < kBlinkTotalMs) {
            const float reopen = static_cast<float>(elapsed - kBlinkCloseMs - kBlinkHoldMs) / kBlinkOpenMs;
            open_fraction = 0.025f + 0.975f * SmoothStep(reopen);
        } else {
            open_fraction = 1.0f;
            blinking_ = false;
        }
    }
    UpdateEyes(open_fraction);
}

void LumiFaceEngine::UpdateEyes(float open_fraction) {
    const lv_coord_t height = std::max<lv_coord_t>(
        5, static_cast<lv_coord_t>(std::lround(kEyeHeight * std::clamp(open_fraction, 0.0f, 1.0f))));
    const lv_coord_t iris_dx = static_cast<lv_coord_t>(std::lround(gaze_x_ * kMaxGazeX));
    const lv_coord_t iris_dy = static_cast<lv_coord_t>(std::lround(gaze_y_ * kMaxGazeY));
    const bool show_iris = open_fraction > 0.58f;

    for (Eye& eye : eyes_) {
        lv_obj_set_size(eye.shell, kEyeWidth, height);
        lv_obj_set_pos(eye.shell, eye.x, eye.y + (kEyeHeight - height) / 2);
        const lv_coord_t iris_x = eye.x + (kEyeWidth - kIrisWidth) / 2 + iris_dx;
        const lv_coord_t iris_y = eye.y + (kEyeHeight - kIrisHeight) / 2 + iris_dy;
        lv_obj_set_pos(eye.iris, iris_x, iris_y);
        lv_obj_set_pos(eye.pupil, iris_x + (kIrisWidth - kPupilWidth) / 2,
                       iris_y + (kIrisHeight - kPupilHeight) / 2);
        lv_obj_set_pos(eye.highlight, iris_x + 5, iris_y + 5);
        if (show_iris) {
            lv_obj_remove_flag(eye.iris, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(eye.pupil, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(eye.highlight, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(eye.iris, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eye.pupil, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(eye.highlight, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

uint32_t LumiFaceEngine::RandomRange(uint32_t minimum, uint32_t maximum) const {
    if (maximum <= minimum) return minimum;
    return minimum + esp_random() % (maximum - minimum + 1);
}
