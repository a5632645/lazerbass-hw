#include "ParamModulations.hpp"

#include "gui/Styles.hpp"

// TODO: OK和EC3不能一起调

namespace gui {

static struct ModulatorChoose : public GuiObj {
    void Draw(OLEDDisplay& display) override {
        auto aera = display.getDrawAera().ReduceRatio(0.2f, 0.2f);
        display.setColor(kOledBLACK);
        display.fillRect(aera.x, aera.y, aera.w, aera.h);
        display.setColor(kOledWHITE);
        display.drawRect(aera.x, aera.y, aera.w, aera.h);

        display.setColor(kOledWHITE);
        aera = aera.Reduce(1, 1);
        display.drawStringMaxWidth(aera.x, aera.y, aera.w, "press a modulator key.  DELETE is go back");
    }

    void BtnEvent(bsp::ControlIO::ButtonEvent e) override {
        auto& synth = gGuiDispatch.GetSynth();
        auto& modulationBank = synth.GetModulationBank();

        using enum bsp::ControlIO::ButtonId;
        using ModulatorId = dsp::ModulatorId;

        dsp::ModulatorDesc modulatorDesc;

        switch (e.id) {
        case kLFO1:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kLfo1);
            break;
        case kLFO2:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kLfo2);
            break;
        case kLFO3:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kLfo3);
            break;
        case kLFO4:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kLfo4);
            break;
        case kEnvelop1:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kEnv1);
            break;
        case kEnvelop2:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kEnv2);
            break;
        case kAmpEnvelop:
            modulatorDesc = synth.GetModulatorDesc(ModulatorId::kAmpEnv);
            break;
        // case kMODSequence:
        case kDelete:
            gGuiDispatch.RemoveOverlay(handle_);
            gGuiDispatch.EnableAutoSwitchPage();
            handle_ = nullptr;
            break;
        default:
            break;
        }

        if (modulatorDesc.outputReg == nullptr) {
            return;
        }

        bool existed = false;
        dsp::ModulationLink* link = modulationBank.AddNewLink(modulatorDesc, paramModulations_->GetTargetParam(), existed);
        if (existed) {
            gGuiDispatch.ShowMessage("modulator already existed", 1000);
        }
        else {
            if (link == nullptr) {
                gGuiDispatch.ShowMessage("num of link has reached max", 1000);
            }
            else {
                {
                    AudioLockGuide lockGuide{ gGuiDispatch.GetAudioLock() };
                    paramModulations_->AddLink(link);
                }
            }
        }
        gGuiDispatch.RemoveOverlay(handle_);
        gGuiDispatch.EnableAutoSwitchPage();
    }

    void EncoderEvent(bsp::ControlIO::EncoderId /*id*/, int32_t /*dvalue*/) override {}

    void SetHandle(GuiDispatch::OverlayObjHandle handle) { handle_ = handle; }
    void SetParent(ParamModulations* parent) { paramModulations_ = parent; }

    GuiDispatch::OverlayObjHandle handle_ = nullptr;
    ParamModulations* paramModulations_ = nullptr;
} modulatorChoose;

// --------------------------------------------------------------------------------
// ParamModulations
// --------------------------------------------------------------------------------
void ParamModulations::Draw(OLEDDisplay& display) {
    auto rect = display.getDrawAera();
    auto box = rect.RemoveFromTop(12);

    display.setColor(kOledWHITE);
    display.fillRect(box.x, box.y, box.w, box.h);
    display.setColor(kOledBLACK);
    display.drawString(box.x, box.y, "Modulations:mod4-alt key");
    display.setColor(kOledWHITE);

    box = rect.RemoveFromTop(12);
    if (numLinks_ == 0) {
        display.FormatString(box.x, box.y, "param:{}  0 of 0", targetParam_->name);
    }
    else {
        display.FormatString(box.x, box.y, "param:{}  {} of {}", targetParam_->name, listPos_ + 1, numLinks_);
    }

    // draw a link
    if (numLinks_ > 0) {
        auto link = links_[listPos_];

        box = rect.RemoveFromTop(12);
        styles::DrawFormatSlider(display, box, box.w * (link->amount + 1) / 2, "amount: {:2}", link->amount);

        box = rect.RemoveFromTop(12);
        auto left = box.RemoveFromLeft(box.w / 2);
        auto right = box;
        left.Reduced(2, 1);
        right.Reduced(2, 1);

        styles::DrawOnOffButton(display, left, link->enable, "enable", "disable");
        styles::DrawOnOffButton(display, right, link->symmetric, "symmetric", "asymmetric");

        display.setColor(kOledWHITE);
        left.Expand(1, 1);
        right.Expand(1, 1);
        display.drawRect(left.x, left.y, left.w, left.h);
        display.drawRect(right.x, right.y, right.w, right.h);

        box = rect.RemoveFromTop(12);
        auto modulationRange = link->GetModulationRange();
        display.FormatString(box.x, box.y, "range: {} ~ {}", targetParam_->GetFloatValue(modulationRange.min), targetParam_->GetFloatValue(modulationRange.max));
    }
}

void ParamModulations::BtnEvent(bsp::ControlIO::ButtonEvent e) {
    auto& synth = gGuiDispatch.GetSynth();
    auto& modulationBank = synth.GetModulationBank();

    using enum bsp::ControlIO::ButtonId;

    switch (e.id) {
    case kOk: {
        gGuiDispatch.ClearMessage();
        GuiDispatch::OverlayObjHandle handle = gGuiDispatch.AddNewOverlay(modulatorChoose);
        modulatorChoose.SetHandle(handle);
        modulatorChoose.SetParent(this);
        gGuiDispatch.DisableAutoSwitchPage();
        break;
    }
    case kDelete: { // TODO: 添加一个确认弹窗?
        if (numLinks_ > 0) {
            auto link = links_[listPos_];
            std::swap(link, links_[numLinks_ - 1]);
            numLinks_--;
            {
                AudioLockGuide audioLock{ gGuiDispatch.GetAudioLock() };
                modulationBank.RemoveLink(link);
            }
            if (listPos_ >= numLinks_) {
                listPos_ = numLinks_ - 1;
            }
            if (listPos_ < 0) {
                listPos_ = 0;
            }
        }
        break;
    }
    default:
        break;
    }

    if (numLinks_ != 0) {
        auto link = links_[listPos_];
        switch (e.id) {
        case kReset2:
            link->amount = 0.0f;
            break;
        case kReset3:
            link->enable = true;
            break;
        case kReset4:
            link->symmetric = false;
            break;
        default:
            break;
        }
    }
}

void ParamModulations::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    if (numLinks_ == 0) {
        return;
    }

    using enum bsp::ControlIO::EncoderId;

    auto isAltDown = bsp::ControlIO::IsButtonDown(bsp::ControlIO::ButtonId::kMod4);
    auto* link = links_[listPos_];

    switch (id) {
    case kEncoder1:
        listPos_ += dvalue;
        while (listPos_ < 0) {
            listPos_ += numLinks_;
        }
        while (listPos_ >= numLinks_) {
            listPos_ -= numLinks_;
        }
        break;
    case kEncoder2: // amount, snap to 0.001f
        if (isAltDown) {
            dvalue *= 25;
        }
        link->amount += dvalue * 0.001f;
        link->amount = dsp::ClampUncheck(link->amount, -1.0f, 1.0f);
        link->amount = std::round(link->amount * 1000) / 1000.0f;
        break;
    case kEncoder3:
        link->enable = dvalue > 0;
        break;
    case kEncoder4:
        link->symmetric = dvalue > 0;
        break;
    default:
        break;
    }
}

void ParamModulations::SetTargetParam(dsp::FloatParamDesc& targetParam) {
    targetParam_ = &targetParam;
    auto& synth = gGuiDispatch.GetSynth();
    numLinks_ = synth.GetModulationBank().GetLinkOfParam(targetParam_, std::span(links_));
    listPos_ = 0;
}

}
