#pragma once
#include "params.hpp"
#include "ModulatorDesc.hpp"

namespace dsp {

class LFO {
public:
    LFO(SynthParams::LfoParamDesc& desc, SynthParams& params)
        : desc_(desc), params_(params) {}

    void Init(uint32_t sampleRate, uint32_t updateRate);
    void Tick();
    void ResetPhase();

    float* GetOutputReg() { return &output_; }
    ModulatorDesc GetModulatorDesc();

private:
    SynthParams::LfoParamDesc& desc_;
    SynthParams& params_;
    float output_{};

    float phase_{};
    float invUpdateRate_{};
    float lastRandom_{};
    float nowRandom_{};
};

}