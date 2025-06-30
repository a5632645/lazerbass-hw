#include "Oscillator.hpp"

#include "gui/PageSpliter.hpp"

namespace gui {

static PageSpliter sp {
    std::array {
        // ---------------------------------------- Page0 ----------------------------------------
        PageObj {
            [](OLEDDisplay& d, Rectange& r) {
                auto& params = gGuiDispatch.GetParams();
                auto oscType = params.oscillor.type.Get();

                auto box = r.RemoveFromTop(12);
                d.FormatString(box.x, box.y, "type: {}", params.oscillor.type.GetName(dsp::kOscillatorTypeNames));

                box = r.RemoveFromTop(12);
                d.FormatString(box.x, box.y, "partials: {}", params.oscillor.numPartials.Get());

                box = r.RemoveFromTop(12);
                using enum dsp::OscillatorType;
                switch (oscType) {
                case kMultiSaw:
                case kMultiSquare:
                    d.FormatString(box.x, box.y, "number: {}", params.oscillor.number.Get());
                    break;
                case kFullPulse:
                    break;
                default:
                    d.FormatString(box.x, box.y, "transport: {:2}", params.oscillor.transport.Get());
                    break;
                }

                box = r.RemoveFromTop(12);
                switch (oscType) {
                case kPwmSquare:
                    d.FormatString(box.x, box.y, "{}: {}", params.oscillor.pluseWidth.name, params.oscillor.pluseWidth.Get());
                    break;
                case kFullPulse:
                    break;
                default:
                    d.FormatString(box.x, box.y, "{}: {}", params.oscillor.beating.name, params.oscillor.beating.Get());
                    break;
                }
            },
            [](bsp::ControlIO::ButtonEvent e) {
                auto& params = gGuiDispatch.GetParams();
                auto oscType = params.oscillor.type.Get();
                
                using enum bsp::ControlIO::ButtonId;
                using enum dsp::OscillatorType;

                switch (e.id) {
                case kReset1:
                    params.oscillor.type.Reset();
                    break;
                case kReset2:
                    params.oscillor.numPartials.Reset();
                    break;
                case kReset3:
                    if (oscType == kMultiSaw || oscType == kMultiSquare) {
                        params.oscillor.number.Reset();
                    }
                    else if (oscType != kFullPulse) {
                        params.oscillor.transport.Reset();
                    }
                    break;
                case kMod3:
                    if (oscType == kMultiSaw || oscType == kMultiSquare) {
                    }
                    else if (oscType != kFullPulse) {
                        gGuiDispatch.EnterParamModulations(params.oscillor.transport);
                    }
                    break;
                case kReset4:
                    switch (oscType) {
                    case kPwmSquare:
                        params.oscillor.pluseWidth.Reset();
                        break;
                    case kFullPulse:
                        break;
                    default:
                        params.oscillor.beating.Reset();
                        break;
                    }
                    break;
                case kMod4:
                    switch (oscType) {
                    case kPwmSquare:
                        gGuiDispatch.EnterParamModulations(params.oscillor.pluseWidth);
                        break;
                    case kFullPulse:
                        break;
                    default:
                        gGuiDispatch.EnterParamModulations(params.oscillor.beating);
                        break;
                    }
                    break;
                default:
                    break;
                }
            },
            [](bsp::ControlIO::EncoderId id, int32_t dvalue) {
                using enum bsp::ControlIO::EncoderId;
                using enum dsp::OscillatorType;

                auto& params = gGuiDispatch.GetParams();
                auto oscType = params.oscillor.type.Get();
                auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

                switch (id) {
                case kEncoder1:
                    params.oscillor.type.Add(dvalue);
                    break;
                case kEncoder2:
                    dvalue *= 2;
                    params.oscillor.numPartials.Add(dvalue, isAltDown);
                    break;
                case kEncoder3:
                    if (oscType == kMultiSaw || oscType == kMultiSquare) {
                        params.oscillor.number.Add(dvalue, isAltDown);
                    }
                    else if (oscType != kFullPulse) {
                        params.oscillor.transport.Add(dvalue, isAltDown);
                    }
                    break;
                case kEncoder4:
                    switch (oscType) {
                    case kPwmSquare:
                        params.oscillor.pluseWidth.Add(dvalue, isAltDown);
                        break;
                    case kFullPulse:
                        break;
                    default:
                        params.oscillor.beating.Add(dvalue, isAltDown);
                        break;
                    }
                    break;
                }
            }
        },
        // ---------------------------------------- Page1 ----------------------------------------
        PageObj {
            [](OLEDDisplay& d, Rectange& r) {
                using enum dsp::OscillatorType;

                auto& param = gGuiDispatch.GetParams();
                auto box = r.RemoveFromTop(12);
                auto oscType = param.oscillor.type.Get();

                if (oscType != kFullPulse) {
                    d.FormatString(box.x, box.y, "{}: {}",param.oscillor.fundamental.name,  param.oscillor.fundamental.Get());
                }
                else {
                    d.FormatString(box.x, box.y, "NO SETTING HERE");
                }
            },
            [](bsp::ControlIO::ButtonEvent e) {
                using enum bsp::ControlIO::ButtonId;
                using enum dsp::OscillatorType;

                auto& params = gGuiDispatch.GetParams();
                auto oscType = params.oscillor.type.Get();

                switch (e.id) {
                case kReset1:
                    if (oscType != kFullPulse) {
                        params.oscillor.fundamental.Reset();
                    }
                    break;
                case kMod1:
                    if (oscType != kFullPulse) {
                        gGuiDispatch.EnterParamModulations(params.oscillor.fundamental);
                    }
                    break;
                default:
                    break;
                }
            },
            [](bsp::ControlIO::EncoderId id, int32_t dvalue) {
                using enum bsp::ControlIO::EncoderId;
                using enum dsp::OscillatorType;

                auto& params = gGuiDispatch.GetParams();
                auto oscType = params.oscillor.type.Get();
                auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::kAltKey);

                switch (id) {
                case kEncoder1:
                    if (oscType != kFullPulse) {
                        params.oscillor.fundamental.Add(dvalue, isAltDown);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
};

void Oscillator::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.drawString(box.x, box.y, "Oscillator");

    display.setColor(kOledWHITE);
    sp.Draw(display, rect);
}

void Oscillator::BtnEvent(bsp::ControlIO::ButtonEvent e) {
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

void Oscillator::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    sp.EncoderEvent(id, dvalue);
}

}