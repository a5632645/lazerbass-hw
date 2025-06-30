#include "PartBeating.hpp"

namespace gui {

void PartBeating::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);
    auto& params = gGuiDispatch.GetParams();

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.FormatString(box.x, box.y, "Part Beating");
    display.setColor(kOledWHITE);

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.partialBeating.enable.name, params.partialBeating.enable.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.partialBeating.parttern.name, params.partialBeating.parttern.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.partialBeating.amount.name, params.partialBeating.amount.Get());
}

void PartBeating::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    auto& params = gGuiDispatch.GetParams();

    switch (e.id) {
    case kReset1:
        params.partialBeating.enable.Reset();
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kBeating, params.partialBeating.enable.Get());
        break;
    case kReset2:
        params.partialBeating.parttern.Reset();
        break;
    case kReset3:
        params.partialBeating.amount.Reset();
        break;
    case kMod3:
        gGuiDispatch.EnterParamModulations(params.partialBeating.amount);
        break;
    default:
        break;
    }
}

void PartBeating::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    auto& params = gGuiDispatch.GetParams();
    auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

    switch (id) {
    case kEncoder1:
        params.partialBeating.enable.Add(dvalue);
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kBeating, params.partialBeating.enable.Get());
        break;
    case kEncoder2:
        params.partialBeating.parttern.Add(dvalue, isAltDown);
        break;
    case kEncoder3:
        params.partialBeating.amount.Add(dvalue, isAltDown);
        break;
    default:
        break;
    }
}

}