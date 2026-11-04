#ifndef XI_BODY_DISPLAY_H
#define XI_BODY_DISPLAY_H

#include "display/lcd_display.h"
#include "lumi_face_engine.h"

class XiBodyDisplay : public SpiLcdDisplay {
public:
    using SpiLcdDisplay::SpiLcdDisplay;
    ~XiBodyDisplay() override;

    void SetupUI() override;
    void SetEmotion(const char* emotion) override;
    void SetTheme(Theme* theme) override;

private:
    void ApplyFaceTheme();

    LumiFaceEngine face_engine_;
#if CONFIG_XI_BODY_ORIENTATION_MARKER
    lv_obj_t* orientation_marker_ = nullptr;
    lv_obj_t* orientation_arrow_ = nullptr;
#endif
};

#endif  // XI_BODY_DISPLAY_H
