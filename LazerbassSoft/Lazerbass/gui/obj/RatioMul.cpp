#include "RatioMul.hpp"

namespace gui {

void RatioMul::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);
    auto& params = gGuiDispatch.GetParams();

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.FormatString(box.x, box.y, "Ratio Multiplier");
    display.setColor(kOledWHITE);

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.ratioMul.enable.name, params.ratioMul.enable.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.ratioMul.parttern.name, params.ratioMul.parttern.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.ratioMul.amount.name, dsp::SynthParams::RatioMulAmountConvert(params.ratioMul.amount.Get()));
}

void RatioMul::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    auto& params = gGuiDispatch.GetParams();

    switch (e.id) {
    case kReset1:
        params.ratioMul.enable.Reset();
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kMul, params.ratioMul.enable.Get());
        break;
    case kReset2:
        params.ratioMul.parttern.Reset();
        break;
    case kReset3:
        params.ratioMul.amount.Reset();
        break;
    case kMod3:
        gGuiDispatch.EnterParamModulations(params.ratioMul.amount);
        break;
    default:
        break;
    }
}

void RatioMul::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    auto& params = gGuiDispatch.GetParams();
    auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

    switch (id) {
    case kEncoder1:
        params.ratioMul.enable.Add(dvalue);
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kMul, params.ratioMul.enable.Get());
        break;
    case kEncoder2:
        params.ratioMul.parttern.Add(dvalue, isAltDown);
        break;
    case kEncoder3:
        params.ratioMul.amount.Add(dvalue, isAltDown);
        break;
    default:
        break;
    }
}

}