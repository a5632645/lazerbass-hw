#include "LFO.hpp"

namespace dsp {

void LFO::Init(uint32_t sampleRate, uint32_t updateRate) {
    invUpdateRate_ = static_cast<float>(updateRate) / sampleRate;
    lastRandom_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    nowRandom_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void LFO::Tick() {
    float lfoRate = SynthParams::GetLfoFrequency(params_.bpm, desc_, desc_.rate.GetWithModulation());
    phase_ += lfoRate * invUpdateRate_;
    if (phase_ > 1.0f) {
        phase_ -= 1.0f;
        // 生成新的随机数
        lastRandom_ = nowRandom_;
        nowRandom_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    switch (desc_.type.Get()) {
    case LFOType::kSawTri: {
        float shape = desc_.shape.GetWithModulation();
        if (shape == 0.0f) {
            output_ = 1.0f - phase_;
        }
        else if (shape == 1.0f) {
            output_ = phase_;
        }
        else {
            if (phase_ < shape) {
                output_ = phase_ / shape;
            }
            else {
                output_ = (1.0f - phase_) / (1.0f - shape);
            }
        }
        break;
    }
    case LFOType::kSampleAndHode:
        output_ = nowRandom_;
        break;
    case LFOType::kNoise:
        output_ = dsp::LerpUncheck(lastRandom_, nowRandom_, phase_);
        break;
    default:
        output_ = 0.0f;
        break;
    }

}

void LFO::ResetPhase() {
    if (desc_.restart.Get()) {
        phase_ = 0.0f;
        lastRandom_ = nowRandom_;
        nowRandom_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
}

ModulatorDesc LFO::GetModulatorDesc() {
    ModulatorDesc desc;

    desc.outputReg = &output_;
    desc.name = desc_.name;

    return desc;
}

}
