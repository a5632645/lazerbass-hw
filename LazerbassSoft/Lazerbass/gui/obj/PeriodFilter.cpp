#include "PeriodFilter.hpp"
#include "gui/PageSpliter.hpp"

namespace gui {

static PageSpliter sp {
    std::array {
        // ---------------------------------------- Page 0 ----------------------------------------
        PageObj {
            [](OLEDDisplay& display, Rectange& rect) {
                auto& params = gGuiDispatch.GetParams();

                auto box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.enable.name, params.periodFilter.enable.Get());

                box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.cycle.name, params.periodFilter.cycle.Get());

                box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.phaseShift.name, params.periodFilter.phaseShift.Get());

                box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.pinch.name, params.periodFilter.pinch.Get());
            },
            [](bsp::ControlIO::ButtonEvent e) {
                using enum bsp::ControlIO::ButtonId;

                auto& params = gGuiDispatch.GetParams();

                switch (e.id) {
                case kReset1:
                    params.periodFilter.enable.Reset();
                    bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kPeriodFilter, params.periodFilter.enable.Get());
                    break;
                case kReset2:
                    params.periodFilter.cycle.Reset();
                    break;
                case kMod2:
                    gGuiDispatch.EnterParamModulations(params.periodFilter.cycle);
                    break;
                case kReset3:
                    params.periodFilter.phaseShift.Reset();
                    break;
                case kMod3:
                    gGuiDispatch.EnterParamModulations(params.periodFilter.phaseShift);
                    break;
                case kReset4:
                    params.periodFilter.pinch.Reset();
                    break;
                case kMod4:
                    gGuiDispatch.EnterParamModulations(params.periodFilter.pinch);
                    break;
                default:
                    break;
                }
            },
            [](bsp::ControlIO::EncoderId id, int32_t dvalue) {
                using enum bsp::ControlIO::EncoderId;

                auto& params = gGuiDispatch.GetParams();
                auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

                switch (id) {
                case kEncoder1:
                    params.periodFilter.enable.Add(dvalue);
                    bsp::ControlIO::SetLed(bsp::ControlIO::LedId::kPeriodFilter, params.periodFilter.enable.Get());
                    break;
                case kEncoder2:
                    params.periodFilter.cycle.Add(dvalue, isAltDown);
                    break;
                case kEncoder3:
                    params.periodFilter.phaseShift.Add(dvalue, isAltDown);
                    break;
                case kEncoder4:
                    params.periodFilter.pinch.Add(dvalue, isAltDown);
                    break;
                default:
                    break;
                }
            }
        },
        // ---------------------------------------- Page 1 ----------------------------------------
        PageObj {
            [](OLEDDisplay& display, Rectange& rect) {
                auto& params = gGuiDispatch.GetParams();
                
                auto box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.apply.name, params.periodFilter.apply.Get());

                box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.stretch.name, params.periodFilter.stretch.Get());

                box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.blocks.name, params.periodFilter.blocks.Get());

                box = rect.RemoveFromTop(12);
                display.FormatString(box.x, box.y, "{}: {}", params.periodFilter.peak.name, params.periodFilter.peak.Get());
            },
            [](bsp::ControlIO::ButtonEvent e) {
                using enum bsp::ControlIO::ButtonId;

                auto& params = gGuiDispatch.GetParams();

                switch (e.id) {
                case kReset1:
                    params.periodFilter.apply.Reset();
                    break;
                case kMod1:
                    gGuiDispatch.EnterParamModulations(params.periodFilter.apply);
                    break;
                case kReset2:
                    params.periodFilter.stretch.Reset();
                    break;
                case kReset3:
                    params.periodFilter.blocks.Reset();
                    break;
                case kReset4:
                    params.periodFilter.peak.Reset();
                    break;
                case kMod4:
                    gGuiDispatch.EnterParamModulations(params.periodFilter.peak);
                    break;
                default:
                    break;
                }
            },
            [](bsp::ControlIO::EncoderId id, int32_t dvalue) {
                using enum bsp::ControlIO::EncoderId;

                auto& params = gGuiDispatch.GetParams();
                auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

                switch (id) {
                case kEncoder1:
                    params.periodFilter.apply.Add(dvalue, isAltDown);
                    break;
                case kEncoder2:
                    params.periodFilter.stretch.Add(dvalue);
                    break;
                case kEncoder3:
                    params.periodFilter.blocks.Add(dvalue);
                    break;
                case kEncoder4:
                    params.periodFilter.peak.Add(dvalue, isAltDown);
                    break;
                default:
                    break;
                }
            }
        }
    }
};

void PeriodFilter::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.FormatString(box.x, box.y, "Period Filter");
    display.setColor(kOledWHITE);

    sp.Draw(display, rect);
}

void PeriodFilter::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    switch (e.id) {
    case kUp:
        sp.PrevPage();
        break;
    case kDown:
        sp.NextPage();
        break;
    default:
        sp.BtnEvent(e);
        break;
    }
}

void PeriodFilter::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    sp.EncoderEvent(id, dvalue); 
}

}