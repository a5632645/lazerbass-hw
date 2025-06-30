#pragma once
#include "params.hpp"
#include "ModulatorDesc.hpp"

namespace dsp {

class Envelope {
public:
    Envelope(SynthParams::EnvParamDesc& desc, SynthParams& params)
        : envParams_(desc), params_(params) {}

    void Init(uint32_t sampleRate, uint32_t updateRate);
    void Tick();

    void GotoAttackState();
    void GotoReleaseState();

    float* GetOutputReg() { return &output_; }
    ModulatorDesc GetModulatorDesc();

private:
    SynthParams::EnvParamDesc& envParams_;
    SynthParams& params_;
    float output_{};

    float invUpdateRate_{};
    enum class State {
        kInit,
        kAttack,
        kRelease
    } state_;
    float phase_{};
};

}
