#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <string_view>

#include "physical_interactions.h"

class PhysicalReactionAudio {
public:
    bool Start();
    bool Queue(PhysicalInteractionEvent event);

private:
    struct QueuedReaction {
        PhysicalInteractionEvent event;
        uint32_t queued_at_ms;
    };

    static void TaskEntry(void* context);
    void Run();
    bool ResolveClip(PhysicalInteractionEvent event, std::string_view& clip) const;

    QueueHandle_t queue_ = nullptr;
    TaskHandle_t task_ = nullptr;
    uint32_t last_played_ms_ = 0;
};
