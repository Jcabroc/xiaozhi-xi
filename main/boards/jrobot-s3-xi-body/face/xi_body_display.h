#ifndef XI_BODY_DISPLAY_H
#define XI_BODY_DISPLAY_H

#include "display/lcd_display.h"
#include "lumi_face_engine.h"

class XiBodyDisplay : public SpiLcdDisplay {
public:
    using SpiLcdDisplay::SpiLcdDisplay;
    ~XiBodyDisplay() override;

    void SetupUI() override;
    void SetStatus(const char* status) override;
    void SetChatMessage(const char* role, const char* content) override;
    void SetEmotion(const char* emotion) override;
    void SetTheme(Theme* theme) override;

private:
    void ApplyFaceTheme();

    LumiFaceEngine face_engine_;
    lv_obj_t* clock_label_ = nullptr;
};

#endif  // XI_BODY_DISPLAY_H
