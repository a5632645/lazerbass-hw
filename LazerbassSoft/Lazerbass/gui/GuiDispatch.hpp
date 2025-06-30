#pragma once
#include "bsp/oled/OLEDDisplay.h"
#include "bsp/ControlIO.hpp"
#include "dsp/params.hpp"
#include "dsp/Lazerbass.hpp"
#include "AudioLock.hpp"

namespace gui {

struct GuiObj {
    virtual ~GuiObj() = default;
    virtual void Draw(OLEDDisplay& display) = 0;
    virtual void BtnEvent(bsp::ControlIO::ButtonEvent e) = 0;
    virtual void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) = 0;
};

struct TimeCallTask {
    uint32_t msStill = 0;
    void(*callback)(void* param, void* param2) = nullptr;
    void* param = nullptr;
    void* param2 = nullptr;
};
using TimeCallTaskHandle = TimeCallTask*;

class GuiDispatch {
public:
    static constexpr uint32_t kFps = 10;
    static constexpr uint32_t kMsPerFrame = 1000 / kFps;

    void Init(void* audioSem, dsp::SynthParams& params, dsp::Lazerbass& synth);
    void Update();
    void SetObj(GuiObj& obj) { obj_ = &obj; }
    dsp::SynthParams& GetParams() { return *params_; }
    dsp::Lazerbass& GetSynth() { return *lazerbass_; }
    AudioLock& GetAudioLock() { return audioLock_; }

    /* 所有的按键都是只有按下事件 */
    void BtnEvent(std::span<bsp::ControlIO::ButtonEvent> events);
    void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue);

    void EnterParamModulations(dsp::FloatParamDesc& targetParam);
    template<class... Args> requires (sizeof...(Args) > 0)
    void ShowMessage(usf::StringView fmt, Args&&... args, uint32_t ms) {
        auto span = usf::format_to(usf::StringSpan(message_, sizeof(message_)), fmt, std::forward<Args>(args)...);
        messageLen_ = span.size();
        msStillShow = ms;
    }
    void ShowMessage(std::string_view str, uint32_t ms) {
        std::copy(str.begin(), str.end(), message_);
        messageLen_ = str.size();
        msStillShow = ms;
    }
    void SetMessageRect(Rectange rect) { messageRect_ = rect; }
    void ClearMessage() { msStillShow = 0; }

    void EnableEventProcessing() { eventProcessing_ = true; }
    void DisableEventProcessing() { eventProcessing_ = false; }
    bool IsEventProcessingEnabled() { return eventProcessing_; }

    void EnableAutoSwitchPage() { autoSwitchPage_ = true; }
    void DisableAutoSwitchPage() { autoSwitchPage_ = false; }

    void TimeTick(uint32_t msEscape);
    uint32_t GetMsEscape() { return msEscape_; }
    TimeCallTaskHandle AddTimeCallTask(uint32_t ms, void(*callback)(void* param, void* param2), void* param, void* param2);
    void RemoveTimeCallTask(TimeCallTaskHandle task);

    using OverlayObjHandle = GuiObj**;
    OverlayObjHandle AddNewOverlay(GuiObj& obj);
    void RemoveOverlay(GuiObj& obj);
    void RemoveOverlay(OverlayObjHandle handle);
    void RemoveAllOverlay();
private:
    void PagePreEvent(bsp::ControlIO::ButtonEvent event);
    void DrawMessage(OLEDDisplay& display);

    dsp::SynthParams* params_ = nullptr;
    dsp::Lazerbass* lazerbass_ = nullptr;

    bool eventProcessing_ = true;
    bool autoSwitchPage_ = true;

    static constexpr size_t kMaxOverlay = 16;
    std::array<GuiObj*, kMaxOverlay> overlay_;
    uint32_t numOverlayObjs_ = 0;

    static constexpr size_t kMaxTimeMessage = 16;
    std::array<TimeCallTask, kMaxTimeMessage> timeMessageTask_;
    uint32_t timeMessageTaskTop_ = 0;

    GuiObj* obj_ = nullptr;
    uint32_t msEscape_ = 0;

    // 消息显示
    static constexpr size_t kMaxMessageLen = 32;
    char message_[kMaxMessageLen] = {};
    uint32_t messageLen_ = 0;
    int32_t msStillShow = 0;
    Rectange messageRect_ = {};

    // 锁
    AudioLock audioLock_;
};

extern GuiDispatch gGuiDispatch;

}