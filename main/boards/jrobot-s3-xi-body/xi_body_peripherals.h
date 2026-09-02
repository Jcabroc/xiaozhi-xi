#ifndef XI_BODY_PERIPHERALS_H
#define XI_BODY_PERIPHERALS_H

#include <stdint.h>

// Data-only configuration. No servo is enabled or moved by this variant.
enum class XiServoChannel : uint8_t {
    kNeckYaw = 0, kNeckPitch, kLeftArmPitch, kRightArmPitch, kWaist, kExpression,
};

struct XiServoSafetyLimit {
    XiServoChannel channel;
    uint16_t minimum_us;
    uint16_t center_us;
    uint16_t maximum_us;
    uint16_t smooth_step_us;
    bool enabled;
};

constexpr XiServoSafetyLimit kXiServoSafetyLimits[] = {
    {XiServoChannel::kNeckYaw, 1000, 1500, 2000, 10, false},
    {XiServoChannel::kNeckPitch, 1000, 1500, 2000, 10, false},
    {XiServoChannel::kLeftArmPitch, 1000, 1500, 2000, 10, false},
    {XiServoChannel::kRightArmPitch, 1000, 1500, 2000, 10, false},
    {XiServoChannel::kWaist, 1000, 1500, 2000, 10, false},
    {XiServoChannel::kExpression, 1000, 1500, 2000, 10, false},
};

enum class XiFaceState : uint8_t {
    kIdle, kListening, kThinking, kSpeaking, kHappy, kSad, kAngry, kConfused,
    kSleepy, kSurprised, kLowBattery, kError, kCharging, kTouched, kMoved,
    kMotionDetected,
};

struct XiRgbColor { uint8_t red; uint8_t green; uint8_t blue; };
// Suggested semantic palette. Driver selection is deferred until RGB LED hardware is confirmed.
constexpr XiRgbColor kXiLedWifiConnecting{0, 128, 128};
constexpr XiRgbColor kXiLedListening{0, 0, 255};
constexpr XiRgbColor kXiLedSpeaking{0, 255, 0};
constexpr XiRgbColor kXiLedThinking{255, 180, 0};
constexpr XiRgbColor kXiLedError{255, 0, 0};

#endif  // XI_BODY_PERIPHERALS_H
