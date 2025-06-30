#pragma once
#include <cstdint>
#include <algorithm>
#include <span>

namespace bsp {

class ControlIO {
public:
    enum class ButtonId : uint8_t {
        kDelete = 0,
        kUp,
        kOscillator,
        kPan,
        kDown,
        kMod1,
        kMod2,
        kOk,
        kAdd,
        kMul,
        kBeating,
        kFilter,
        kPeriodFilter,
        kMod3,
        kMod4,
        kMaster,
        kDistortion,
        kChrous,
        kDelay,
        kPhase,
        kReverb,
        kGlide,
        kLegato,
        kPreset,
        kEnvelop2,
        kLFO1,
        kLFO2,
        kLFO3,
        kEnvelop1,
        kAmpEnvelop,
        kAttenuation,
        kDispersion,
        kPlay,
        kReset4,
        kReset3,
        kReset2,
        kLFO4,
        kMODSequence,
        kReset1,
        kMarco,
        kPitchDown,
        kRecord,
        kStop,
        kPitchUp = kStop + 5,
        kSeq7,
        kSeq6,
        kSeq5,
        kSeq4,
        kSeq3,
        kSeq2,
        kSeq1,
        kSeq0,
        kSeq23,
        kSeq22,
        kSeq21,
        kSeq20,
        kSeq19,
        kSeq18,
        kSeq17,
        kSeq16,
        kSeq15,
        kSeq14,
        kSeq13,
        kSeq12,
        kSeq11,
        kSeq10,
        kSeq9,
        kSeq8
    };
    static constexpr auto kAltKey = ButtonId::kOk;

    enum class LedId : uint8_t {
        kRecord = 4,
        kMarco,
        kLegato,
        kPlay,
        kPeriodFilter,
        kDistortion,
        kChrous,
        kDelay,
        kMODSequence = kDelay + 2,
        kLFO3Invert,
        kLFO2Invert,
        kDispersion,
        kAttenuation,
        kPhase,
        kReverb,
        kLFO1Invert,
        kLFO4Invert,
        kEnvelop2Invert,
        kEnvelop2AR,
        kAdd,
        kMul,
        kBeating,
        kFilter,
        kAMPEnvelopAR,
        kAMPEnvelopInvert,
        kEnvelop1Invert,
        kEnvelop1AR,
        kSeq0,
        kSeq1,
        kSeq2,
        kSeq3,
        kSeq4,
        kSeq5,
        kSeq6,
        kSeq7,
        kSeq8,
        kSeq9,
        kSeq10,
        kSeq11,
        kSeq12,
        kSeq13,
        kSeq14,
        kSeq15
    };

    enum class EncoderId : uint8_t {
        kEncoder1 = 0,
        kEncoder2,
        kEncoder3,
        kEncoder4,
    };

    enum class ButtonState : uint8_t {
        kAttack,
        kRelease
    };

    struct ButtonEvent {
        ButtonState state : 1;
        ButtonId id : 7;

        bool IsAttack() const { return state == ButtonState::kAttack; }
        bool IsRelease() const { return state == ButtonState::kRelease; }
    };

    static void Init();

    static void SetLed(LedId, bool on);
    static void SetLed(uint32_t led, bool on);
    static void SetAllLeds(bool on);
    static void TestShift();

    static bool IsButtonDown(ButtonId id);
    static bool IsAltKeyDown() { return IsButtonDown(kAltKey); }

    static void WaitForNextEventBlock();
    static std::span<ButtonEvent> GetBtnEvents();
    static std::span<int32_t, 4> GetEncoderValues();
private:
    static void EncoderInit();
    static void SpiInit();
};

}
