#include "OscPhase.hpp"

namespace gui {

void OscPhase::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);
    auto& params = gGuiDispatch.GetParams();

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.FormatString(box.x, box.y, "Osc Phase");
    display.setColor(kOledWHITE);

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.oscPhase.enable.name, params.oscPhase.enable.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.oscPhase.random.name, params.oscPhase.random.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.oscPhase.parttern.name, params.oscPhase.parttern.Get());

    box = rect.RemoveFromTop(12);
    display.FormatString(box.x, box.y, "{}: {}", params.oscPhase.symmetry.name, params.oscPhase.symmetry.Get());
}

void OscPhase::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    auto& params = gGuiDispatch.GetParams();

    switch (e.id) {
    case kReset1:
        params.oscPhase.enable.Reset();
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kPhase, params.oscPhase.enable.Get());
        break;
    case kReset2:
        params.oscPhase.random.Reset();
        break;
    case kReset3:
        params.oscPhase.parttern.Reset();
        break;
    case kReset4:
        params.oscPhase.symmetry.Reset();
    default:
        break;
    }
}

void OscPhase::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    auto& params = gGuiDispatch.GetParams();
    auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

    switch (id) {
    case kEncoder1:
        params.oscPhase.enable.Add(dvalue);
        bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kPhase, params.oscPhase.enable.Get());
        break;
    case kEncoder2:
        params.oscPhase.random.Add(dvalue, isAltDown);
        break;
    case kEncoder3:
        params.oscPhase.parttern.Add(dvalue, isAltDown);
        break;
    case kEncoder4:
        params.oscPhase.symmetry.Add(dvalue, isAltDown);
        break;
    default:
        break;
    }
}

}