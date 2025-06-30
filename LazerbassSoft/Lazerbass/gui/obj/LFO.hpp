#pragma once
#include "gui/GuiDispatch.hpp"
#include "dsp/params.hpp"

namespace gui {

class LFO : public GuiObj {
public:
    void Draw(OLEDDisplay& display) override;
    void BtnEvent(bsp::ControlIO::ButtonEvent e) override;
    void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) override;

    void SetTargetLfoParam(dsp::SynthParams::LfoParamDesc& targetLfoParam) { targetLfoParam_ = &targetLfoParam; }
private:
    dsp::SynthParams::LfoParamDesc* targetLfoParam_{};
    int32_t page_{};
    static constexpr int32_t kMaxPageIndex = 1;
};

}
