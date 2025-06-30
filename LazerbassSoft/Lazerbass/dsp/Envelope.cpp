#include "Envelope.hpp"

namespace dsp {

void Envelope::Init(uint32_t sampleRate, uint32_t updateRate) {
    invUpdateRate_ = static_cast<float>(updateRate) / sampleRate;
}

void Envelope::Tick() {
    using enum State;

    switch (state_) {
    case kInit:
        output_ = 0.0f;
        break;
    case kAttack: {
        float time = SynthParams::EnvVal01ToTime(envParams_.attack.Get());
        if (time > envParams_.kMinTime) {
            float inc = 1.0f / time * invUpdateRate_;
            phase_ += inc;

            if (phase_ > 1.0f) {
                phase_ = 0.0f;
                state_ = kRelease;
            }
            output_ = phase_ * envParams_.peak.Get();
        }
        else {
            state_ = kRelease;
            phase_ = 0.0f;
        }

        if (state_ == kAttack) {
            break;
        }
    }
    [[fallthrough]];
    case kRelease: {
        float time = SynthParams::EnvVal01ToTime(envParams_.release.Get());
        if (time > envParams_.kMinTime) {
            float inc = 1.0f / time * invUpdateRate_;
            phase_ += inc;

            if (phase_ > 1.0f) {
                phase_ = 1.0f;
                state_ = kInit;
                output_ = 0.0f;
            }
            else {
                output_ = (1.0f - phase_) * envParams_.peak.Get();
            }
        }
        else {
            state_ = kInit;
            output_ = 0.0f;
        }
    }
    }

    if (envParams_.invert.Get()) {
        output_ = 1.0f - output_;
    }
}

void Envelope::GotoAttackState() {
    state_ = State::kAttack;
    phase_ = 0.0f;
}

void Envelope::GotoReleaseState() {
    state_ = State::kRelease;
    phase_ = 0.0f;
}

ModulatorDesc Envelope::GetModulatorDesc() {
    ModulatorDesc desc;
    desc.name = envParams_.name;
    desc.outputReg = &output_;
    return desc;
}

}