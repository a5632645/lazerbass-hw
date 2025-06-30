#pragma once
#include "gui/GuiDispatch.hpp"

namespace gui {

struct RatioMul : public GuiObj {
    void Draw(OLEDDisplay& display) override;
    void BtnEvent(bsp::ControlIO::ButtonEvent e) override;
    void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) override;
};

}