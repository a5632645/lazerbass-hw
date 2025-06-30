#include "RatioAdd.hpp"

namespace gui {

void RatioAdd::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);
    auto& params = gGuiDispatch.GetParams();

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.FormatString(box.x, box.y, "Ratio Add");
    display.setColor(kOledWHITE);

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.ratioAdd.enable.name, params.ratioAdd.enable.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.ratioAdd.parttern.name, params.ratioAdd.parttern.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.ratioAdd.amount.name, params.ratioAdd.amount.Get());
}

void RatioAdd::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    auto& params = gGuiDispatch.GetParams();

    switch (e.id) {
    case kReset1:
        params.ratioAdd.enable.Reset();
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kAdd, params.ratioAdd.enable.Get());
        break;
    case kReset2:
        params.ratioAdd.parttern.Reset();
        break;
    case kReset3:
        params.ratioAdd.amount.Reset();
        break;
    case kMod3:
        gGuiDispatch.EnterParamModulations(params.ratioAdd.amount);
        break;
    default:
        break;
    }
}

void RatioAdd::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    auto& params = gGuiDispatch.GetParams();
    auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

    switch (id) {
    case kEncoder1:
        params.ratioAdd.enable.Add(dvalue);
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kAdd, params.ratioAdd.enable.Get());
        break;
    case kEncoder2:
        params.ratioAdd.parttern.Add(dvalue, isAltDown);
        break;
    case kEncoder3:
        params.ratioAdd.amount.Add(dvalue, isAltDown);
        break;
    default:
        break;
    }
}

}