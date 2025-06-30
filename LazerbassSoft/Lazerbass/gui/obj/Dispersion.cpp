#include "Dispersion.hpp"

namespace gui {

void Dispersion::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);
    auto& params = gGuiDispatch.GetParams();

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.FormatString(box.x, box.y, "Dispersion");
    display.setColor(kOledWHITE);

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.dispersion.enable.name, params.dispersion.enable.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.dispersion.amount.name, params.dispersion.amount.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.dispersion.key.name, params.dispersion.key.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.dispersion.shape.name, params.dispersion.shape.Get());
}

void Dispersion::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    auto& params = gGuiDispatch.GetParams();

    switch (e.id) {
    case kReset1:
        params.dispersion.enable.Reset();
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kDispersion, params.dispersion.enable.Get());
        break;
    case kReset2:
        params.dispersion.amount.Reset();
        break;
    case kMod2:
        gGuiDispatch.EnterParamModulations(params.dispersion.amount);
        break;
    case kReset3:
        params.dispersion.key.Reset();
        break;
    case kMod3:
        gGuiDispatch.EnterParamModulations(params.dispersion.key);
        break;
    case kReset4:
        params.dispersion.shape.Reset();
        break;
    case kMod4:
        gGuiDispatch.EnterParamModulations(params.dispersion.shape);
        break;
    default:
        break;
    }
}

void Dispersion::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    auto& params = gGuiDispatch.GetParams();
    auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

    switch (id) {
    case kEncoder1:
        params.dispersion.enable.Add(dvalue);
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kDispersion, params.dispersion.enable.Get());
        break;
    case kEncoder2:
        params.dispersion.amount.Add(dvalue, isAltDown);
        break;
    case kEncoder3:
        params.dispersion.key.Add(dvalue, isAltDown);
        break;
    case kEncoder4:
        params.dispersion.shape.Add(dvalue, isAltDown);
        break;
    default:
        break;
    }
}

}