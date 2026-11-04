#include "physical_reaction_audio.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <sdkconfig.h>

#include <string_view>

#include "application.h"
#include "assets/lang_config.h"

namespace {
constexpr char kTag[] = "XiReactionAudio";
constexpr uint32_t kTaskStackBytes = 4096;
constexpr UBaseType_t kQueueLength = 1;
constexpr uint32_t kTaskPollCooldownMs = CONFIG_XI_BODY_PHYSICAL_REACTION_COOLDOWN_MS;
constexpr uint32_t kPendingReactionMaxAgeMs = 1000;
}

bool PhysicalReactionAudio::Start() {
    if (queue_ != nullptr || task_ != nullptr) return false;
    queue_ = xQueueCreate(kQueueLength, sizeof(QueuedReaction));
    if (queue_ == nullptr) return false;
    if (xTaskCreate(TaskEntry, "xi_reaction_audio", kTaskStackBytes, this, 2, &task_) != pdPASS) {
        vQueueDelete(queue_);
        queue_ = nullptr;
        return false;
    }
    return true;
}

bool PhysicalReactionAudio::Queue(PhysicalInteractionEvent event) {
    if (queue_ == nullptr || event == PhysicalInteractionEvent::kTouch1Short) return false;
    QueuedReaction request{event, static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    return xQueueSend(queue_, &request, 0) == pdPASS;
}

void PhysicalReactionAudio::TaskEntry(void* context) {
    static_cast<PhysicalReactionAudio*>(context)->Run();
}

bool PhysicalReactionAudio::ResolveClip(PhysicalInteractionEvent event, std::string_view& clip) const {
    switch (event) {
        case PhysicalInteractionEvent::kTouch2Short:
#if defined(XI_BODY_HAS_XI_T2_LAUGH)
            clip = Lang::Sounds::OGG_XI_T2_LAUGH;
            return !clip.empty();
#else
            return false;
#endif
        case PhysicalInteractionEvent::kShake:
#if defined(XI_BODY_HAS_XI_IMU_SHAKE)
            clip = Lang::Sounds::OGG_XI_IMU_SHAKE;
            return !clip.empty();
#else
            return false;
#endif
        case PhysicalInteractionEvent::kTouch1Short:
            return false;
    }
    return false;
}

void PhysicalReactionAudio::Run() {
    QueuedReaction request{};
    while (true) {
        if (xQueueReceive(queue_, &request, portMAX_DELAY) != pdPASS) continue;
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        if (now_ms - request.queued_at_ms > kPendingReactionMaxAgeMs) {
            if (request.event == PhysicalInteractionEvent::kTouch2Short)
                ESP_LOGW(kTag, "T2 reaction discarded: request expired");
            continue;
        }

        std::string_view clip;
        if (!ResolveClip(request.event, clip)) {
            if (request.event == PhysicalInteractionEvent::kTouch2Short)
                ESP_LOGW(kTag, "T2 reaction discarded: no clip configured");
            else
                ESP_LOGD(kTag, "No local clip configured for %s", PhysicalInteractionEventName(request.event));
            continue;
        }

        if (last_played_ms_ != 0 && now_ms - last_played_ms_ < kTaskPollCooldownMs) {
            if (request.event == PhysicalInteractionEvent::kTouch2Short)
                ESP_LOGI(kTag, "T2 reaction discarded: cooldown active");
            continue;
        }

        auto& app = Application::GetInstance();
        if (app.GetDeviceState() != kDeviceStateIdle) {
            if (request.event == PhysicalInteractionEvent::kTouch2Short)
                ESP_LOGI(kTag, "T2 reaction discarded: XiaoZhi state is not idle (%d)", app.GetDeviceState());
            continue;
        }
        if (!app.GetAudioService().CanPlayLocalSound()) {
            if (request.event == PhysicalInteractionEvent::kTouch2Short)
                ESP_LOGI(kTag, "T2 reaction discarded: audio service is busy");
            continue;
        }

        const bool played = app.GetAudioService().PlayLocalSoundWhenIdle(clip, [&app]() {
            return app.GetDeviceState() == kDeviceStateIdle;
        });
        if (played) {
            last_played_ms_ = now_ms;
            if (request.event == PhysicalInteractionEvent::kTouch2Short)
                ESP_LOGI(kTag, "T2 reaction clip started");
        } else if (request.event == PhysicalInteractionEvent::kTouch2Short) {
            ESP_LOGW(kTag, "T2 reaction discarded: local playback did not start");
        }
    }
}
