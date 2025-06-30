#pragma once
#include "gui/GuiDispatch.hpp"

namespace gui {

class ParamModulations : public GuiObj {
public:
    void Draw(OLEDDisplay& display) override;
    void BtnEvent(bsp::ControlIO::ButtonEvent e) override;
    void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) override;

    void SetTargetParam(dsp::FloatParamDesc& targetParam);
    dsp::FloatParamDesc* GetTargetParam() { return targetParam_; }

    void AddLink(dsp::ModulationLinkHandle link) { links_[numLinks_++] = link; }
private:
    dsp::FloatParamDesc* targetParam_ = nullptr;
    dsp::ModulationLinkHandle links_[dsp::ModulationBank::kMaxNumModulations];
    int32_t numLinks_ = 0;
    int32_t listPos_ = 0;
};

}