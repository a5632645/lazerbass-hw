#include "GuiDispatch.hpp"

#include "bsp/Oled.hpp"
#include "GuiObjs.hpp"

namespace gui {

void GuiDispatch::Init(void* audioSem, dsp::SynthParams& params, dsp::Lazerbass& synth) {
    SetObj(GuiObjs::oscillator);
    audioLock_.SetRtosSemHandle(audioSem);
    params_ = &params;
    lazerbass_ = &synth;

    auto& display = bsp::Oled::GetDisplay();
    SetMessageRect(display.getDrawAera().ReduceRatio(0.2f, 0.2f));
}

void GuiDispatch::Update() {
    auto& display = bsp::Oled::GetDisplay();
    display.Fill(kOledBLACK);

    if (obj_ != nullptr) {
        obj_->Draw(display);
    }

    for (uint32_t i = 0; i < numOverlayObjs_; ++i) {
        overlay_[i]->Draw(display);
    }

    DrawMessage(display);
}

void GuiDispatch::DrawMessage(OLEDDisplay &display)
{
    if (msStillShow > 0)
    {
        display.setColor(kOledBLACK);
        display.fillRect(messageRect_.x, messageRect_.y, messageRect_.w, messageRect_.h);
        display.setColor(kOledWHITE);
        display.drawRect(messageRect_.x, messageRect_.y, messageRect_.w, messageRect_.h);

        auto box = messageRect_;
        auto titleBox = box.RemoveFromTop(12);
        display.setColor(kOledWHITE);
        display.fillRect(titleBox.x, titleBox.y, titleBox.w, titleBox.h);
        display.setColor(kOledBLACK);
        titleBox.Reduce(1,1);
        display.drawString(titleBox.x, titleBox.y, "Message");

        display.setColor(kOledWHITE);
        box.Reduce(1,1);
        display.drawStringMaxWidth(box.x, box.y, box.w, std::string_view(message_, messageLen_));
        msStillShow -= msEscape_;
    }
}

void GuiDispatch::BtnEvent(std::span<bsp::ControlIO::ButtonEvent> events) {
    if (numOverlayObjs_ > 0) {
        for (auto e : events) {
            if (e.IsAttack()) {
                if (e.id == bsp::ControlIO::ButtonId::kOk) {
                    ClearMessage();
                }
                overlay_[numOverlayObjs_ - 1]->BtnEvent(e);
            }
        }
        return;
    }

    if (obj_ == nullptr) {
        return;
    }

    if (autoSwitchPage_) {
        for (auto e : events) {
            if (e.IsAttack()) {
                if (e.id == bsp::ControlIO::ButtonId::kOk) {
                    ClearMessage();
                }
                PagePreEvent(e);
            }
        }
    }
    else {
        for (auto e : events) {
            if (e.IsAttack()) {
                if (e.id == bsp::ControlIO::ButtonId::kOk) {
                    ClearMessage();
                }
                obj_->BtnEvent(e);
            }
        }
    }
}

void GuiDispatch::EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
    if (numOverlayObjs_ > 0) {
        overlay_[numOverlayObjs_ - 1]->EncoderEvent(id, dvalue);
        return;
    }

    if (obj_ == nullptr) {
        return;
    }

    obj_->EncoderEvent(id, dvalue);
}

void GuiDispatch::EnterParamModulations(dsp::FloatParamDesc& targetParam) {
    GuiObjs::paramModulations.SetTargetParam(targetParam);
    SetObj(GuiObjs::paramModulations);
}

void GuiDispatch::TimeTick(uint32_t msEscape) {
    msEscape_ = msEscape;
    for (uint32_t i = 0; i < timeMessageTaskTop_;) {
        auto& task = timeMessageTask_[i];
        if (task.msStill > msEscape) {
            task.msStill -= msEscape;
            ++i;
        }
        else {
            task.callback(task.param, task.param2);
            timeMessageTask_[i] = timeMessageTask_[--timeMessageTaskTop_];
        }
    }
}

gui::TimeCallTaskHandle GuiDispatch::AddTimeCallTask(uint32_t ms, void (*callback)(void* param, void* param2), void* param, void* param2) {
    auto& allocTask = timeMessageTask_[timeMessageTaskTop_++];

    allocTask.msStill = ms;
    allocTask.callback = callback;
    allocTask.param = param;
    allocTask.param2 = param2;

    return &allocTask;
}

void GuiDispatch::RemoveTimeCallTask(TimeCallTaskHandle task) {
    std::swap(timeMessageTask_[timeMessageTaskTop_ - 1], *task);
    --timeMessageTaskTop_;
}

gui::GuiDispatch::OverlayObjHandle GuiDispatch::AddNewOverlay(GuiObj& obj) {
    auto& allocTask = overlay_[numOverlayObjs_++];

    allocTask = &obj;

    return &allocTask;
}

void GuiDispatch::RemoveOverlay(GuiObj& obj) {
    for (uint32_t i = 0; i < numOverlayObjs_; ++i) {
        if (overlay_[i] == &obj) {
            std::swap(overlay_[i], overlay_[numOverlayObjs_ - 1]);
            --numOverlayObjs_;
            return;
        }
    }
}

void GuiDispatch::RemoveOverlay(OverlayObjHandle handle) {
    std::swap(*handle, overlay_[numOverlayObjs_ - 1]);
    --numOverlayObjs_;
}

void GuiDispatch::RemoveAllOverlay() {
    numOverlayObjs_ = 0;
}

/* 切换页面用的 */
void GuiDispatch::PagePreEvent(bsp::ControlIO::ButtonEvent e) {
    GuiObj* targetObjShouldBe = nullptr;

    using enum bsp::ControlIO::ButtonId;

    switch (e.id) {
    case kOscillator:
        targetObjShouldBe = &GuiObjs::oscillator;
        break;
    case kMul:
        targetObjShouldBe = &GuiObjs::ratioMul;
        break;
    case kAdd:
        targetObjShouldBe = &GuiObjs::ratioAdd;
        break;
    case kDispersion:
        targetObjShouldBe = &GuiObjs::dispersion;
        break;
    case kBeating:
        targetObjShouldBe = &GuiObjs::partBeating;
        break;
    case kPhase:
        targetObjShouldBe = &GuiObjs::oscPhase;
        break;
    case kPeriodFilter:
        targetObjShouldBe = &GuiObjs::periodFilter;
        break;
    case kLFO1:
        targetObjShouldBe = &GuiObjs::lfo;
        GuiObjs::lfo.SetTargetLfoParam(params_->lfo1);
        break;
    case kLFO2:
        targetObjShouldBe = &GuiObjs::lfo;
        GuiObjs::lfo.SetTargetLfoParam(params_->lfo2);
        break;
    case kLFO3:
        targetObjShouldBe = &GuiObjs::lfo;
        GuiObjs::lfo.SetTargetLfoParam(params_->lfo3);
        break;
    case kLFO4:
        targetObjShouldBe = &GuiObjs::lfo;
        GuiObjs::lfo.SetTargetLfoParam(params_->lfo4);
        break;
    case kEnvelop1:
        targetObjShouldBe = &GuiObjs::envelope;
        GuiObjs::envelope.SetTargetEnvParam(params_->env1);
        break;
    case kEnvelop2:
        targetObjShouldBe = &GuiObjs::envelope;
        GuiObjs::envelope.SetTargetEnvParam(params_->env2);
        break;
    case kAmpEnvelop:
        targetObjShouldBe = &GuiObjs::envelope;
        GuiObjs::envelope.SetTargetEnvParam(params_->ampEnv);
        break;
    default:
        break;
    }

    if (targetObjShouldBe == nullptr || targetObjShouldBe == obj_) {
        obj_->BtnEvent(e);
    }
    else {
        SetObj(*targetObjShouldBe);
    }
}

GuiDispatch gGuiDispatch;

}