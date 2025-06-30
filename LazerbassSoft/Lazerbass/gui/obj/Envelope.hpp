#pragma once
#include "gui/GuiDispatch.hpp"
#include "dsp/params.hpp"

namespace gui {

class Envelope : public GuiObj {
public:
    void Draw(OLEDDisplay& display) override;
    void BtnEvent(bsp::ControlIO::ButtonEvent e) override;
    void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) override;

    void SetTargetEnvParam(dsp::SynthParams::EnvParamDesc& targetLfoParam) { targetEnvParam_ = &targetLfoParam; }
private:
    dsp::SynthParams::EnvParamDesc* targetEnvParam_{};
};

}
