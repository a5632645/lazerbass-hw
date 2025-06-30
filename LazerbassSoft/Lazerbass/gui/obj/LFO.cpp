#include "LFO.hpp"

#include "gui/Styles.hpp"

namespace gui {

// --------------------------------------------------------------------------------
// LFO
// --------------------------------------------------------------------------------
void LFO::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(10);
    auto& params = gGuiDispatch.GetParams();

    styles::DrawTitleBar(display, box, targetLfoParam_->name);

    // researt bpm snap
    int16_t page0Y = rect.y;
    {
        rect.RemoveFromTop(2);
        box = rect.RemoveFromTop(12);
        int16_t w = box.w / 3;
        styles::DrawButton(display, box.RemoveFromLeft(w).Reduce(1,0), targetLfoParam_->restart.Get(), "restart");
        styles::DrawOnOffButton(display, box.RemoveFromLeft(w).Reduce(1,0), targetLfoParam_->bpm.Get(), "bpm", "herz");
        styles::DrawButton(display, box.RemoveFromLeft(w).Reduce(1, 0), targetLfoParam_->snap.Get(), "snap");
    }
    // a frequency bar
    {
        rect.RemoveFromTop(2);
        box = rect.RemoveFromTop(12).Reduce(1, 0);
        float val01 = targetLfoParam_->rate.Get();
        if (targetLfoParam_->snap.Get() && targetLfoParam_->bpm.Get()) {
            val01 = std::round(val01 * 4.0f) / 4.0f;
        }

        display.setColor(kOledWHITE);
        if (targetLfoParam_->bpm.Get()) {
            switch (targetLfoParam_->times.Get()) {
            case 0:
                display.drawString(box.x, box.y, "2/1  1/1     1/2   1/4   1/8");
                break;
            case 1:
                display.drawString(box.x, box.y, "1/1  1/2     1/4   1/8  1/16");
                break;
            case 2:
                display.drawString(box.x, box.y, "1/2  1/4     1/8  1/16  1/32");
                break;
            default:
                break;
            }
        }
        else {
            display.FormatString(box.x, box.y, "rate: {:3} hz", dsp::SynthParams::GetLfoFrequency(params.bpm, *targetLfoParam_, targetLfoParam_->rate.Get()));
        }

        int16_t fillWidth = static_cast<int16_t>(val01 * box.w);
        display.setColor(kOledINVERSE);
        display.fillRect(box.x, box.y, fillWidth, box.h);
        display.setColor(kOledWHITE);
        display.drawRect(box.x, box.y, box.w, box.h);
    }
    int16_t page1Y = rect.y;
    // times dot-trip type
    {
        rect.RemoveFromTop(2);
        box = rect.RemoveFromTop(12);
        int16_t w = box.w / 3;
        styles::DrawSlider(display, box.RemoveFromLeft(w).Reduce(1, 0), w * targetLfoParam_->times.GetValueAsNormalized(), "times");
        styles::DrawSlider(display, box.RemoveFromLeft(w).Reduce(1, 0), w * targetLfoParam_->dotTrip.GetValueAsNormalized(), "dotTrip");

        display.setColor(kOledWHITE);
        box.Reduce(1, 0);
        display.drawRect(box.x, box.y, box.w, box.h);
        display.drawString(box.x, box.y, targetLfoParam_->type.GetName(dsp::kLFOTypeNames));
    }
    // shape
    {
        rect.RemoveFromTop(2);
        box = rect.RemoveFromTop(12);
        styles::DrawFormatSlider(display, box, box.w * targetLfoParam_->shape.GetValueAsNormalized(),
                                "{}: {}", targetLfoParam_->shape.name, targetLfoParam_->shape.Get());
    }

    rect = display.getDrawAera();
    if (page_ == 0) {
        display.setColor(kOledWHITE);
        display.drawRect(rect.x, page0Y, rect.w, page1Y - page0Y);
    }
    else {
        display.drawRect(rect.x, page1Y, rect.w, rect.h - page1Y);
    }
}

void LFO::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    switch (e.id) {
    case kUp:
        if (page_ > 0) {
            --page_;
        }
        break;
    case kDown:
        if (page_ < kMaxPageIndex) {
            ++page_;
        }
        break;
    default:
        if (page_ == 0) {
            switch (e.id) {
            case kReset1:
                targetLfoParam_->restart.Reset();
                break;
            case kReset2:
                targetLfoParam_->bpm.Reset();
                break;
            case kReset3:
                targetLfoParam_->snap.Reset();
                break;
            case kReset4:
                targetLfoParam_->rate.Reset();
                break;
            case kMod4:
                gGuiDispatch.EnterParamModulations(targetLfoParam_->rate);
                break;
            default:
                break;
            }
        }
        else {
            switch (e.id) {
            case kReset1:
                targetLfoParam_->times.Reset();
                break;
            case kReset2:
                targetLfoParam_->dotTrip.Reset();
                break;
            case kReset3:
                targetLfoParam_->type.Reset();
                break;
            case kReset4:
                targetLfoParam_->shape.Reset();
                break;
            case kMod4:
                gGuiDispatch.EnterParamModulations(targetLfoParam_->shape);
                break;
            default:
                break;
            }
        }
        break;
    };
}

void LFO::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    bool isAltDown = bsp::ControlIO::IsAltKeyDown();

    if (page_ == 0) {
        switch (id) {
        case kEncoder1:
            targetLfoParam_->restart.Add(dvalue);
            break;
        case kEncoder2:
            targetLfoParam_->bpm.Add(dvalue);
            break;
        case kEncoder3:
            targetLfoParam_->snap.Add(dvalue);
            break;
        case kEncoder4:
            if (targetLfoParam_->snap.Get() && targetLfoParam_->bpm.Get()) {
                targetLfoParam_->rate.Add(25 * dvalue, false);
            }
            else {
                targetLfoParam_->rate.Add(dvalue, isAltDown);
            }
            break;
        default:
            break;
        }
    }
    else {
        switch (id) {
        case kEncoder1:
            targetLfoParam_->times.Add(dvalue, isAltDown);
            break;
        case kEncoder2:
            targetLfoParam_->dotTrip.Add(dvalue, isAltDown);
            break;
        case kEncoder3:
            targetLfoParam_->type.Add(dvalue);
            break;
        case kEncoder4:
            targetLfoParam_->shape.Add(dvalue, isAltDown);
            break;
        default:
            break;
        }
    }
}

}
