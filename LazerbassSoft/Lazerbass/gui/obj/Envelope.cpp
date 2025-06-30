#include "Envelope.hpp"
#include "gui/Styles.hpp"

namespace gui {

void Envelope::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12).WithHeight(10);

    styles::DrawTitleBar(display, box, targetEnvParam_->name);

    box = rect.RemoveFromTop(12).Reduce(3, 1).WithWidth(40);
    styles::DrawButton(display, box, targetEnvParam_->invert.Get(), "invert");

    box = rect.RemoveFromTop(12).WithHeight(10);
    float time = dsp::SynthParams::EnvVal01ToTime(targetEnvParam_->attack.Get());
    if (time > 1.0f) {
        styles::DrawFormatSlider(display, box, box.w * targetEnvParam_->attack.Get(), "{}: {}s", targetEnvParam_->attack.name, std::round(time));
    }
    else {
        styles::DrawFormatSlider(display, box, box.w * targetEnvParam_->attack.Get(), "{}: {}ms", targetEnvParam_->attack.name, time * 1000.0f);
    }

    box = rect.RemoveFromTop(12).WithHeight(10);
    time = dsp::SynthParams::EnvVal01ToTime(targetEnvParam_->release.Get());
    if (time > 1.0f) {
        styles::DrawFormatSlider(display, box, box.w * targetEnvParam_->release.Get(), "{}: {}s", targetEnvParam_->release.name, std::round(time));
    }
    else {
        styles::DrawFormatSlider(display, box, box.w * targetEnvParam_->release.Get(), "{}: {}ms", targetEnvParam_->release.name, time * 1000.0f);
    }

    box = rect.RemoveFromTop(12).WithHeight(10);
    styles::DrawFormatSlider(display, box, box.w * targetEnvParam_->peak.Get(), "{}: {}", targetEnvParam_->peak.name, targetEnvParam_->peak.Get());
}

void Envelope::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    using enum bsp::ControlIO::ButtonId;

    switch (e.id) {
    case kReset1:
        targetEnvParam_->invert.Reset();
        break;
    case kReset2:
        targetEnvParam_->attack.Reset();
        break;
    case kReset3:
        targetEnvParam_->release.Reset();
        break;
    case kReset4:
        targetEnvParam_->peak.Reset();
        break;
    default:
        break;
    }
}

void Envelope::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    using enum bsp::ControlIO::EncoderId;

    auto isAltDown = bsp::ControlIO::IsAltKeyDown();

    switch (id) {
    case kEncoder1:
        targetEnvParam_->invert.Add(dvalue);
        break;
    case kEncoder2:
        targetEnvParam_->attack.Add(dvalue, isAltDown);
        break;
    case kEncoder3:
        targetEnvParam_->release.Add(dvalue, isAltDown);
        break;
    case kEncoder4:
        targetEnvParam_->peak.Add(dvalue, isAltDown);
        break;
    }
}

}
