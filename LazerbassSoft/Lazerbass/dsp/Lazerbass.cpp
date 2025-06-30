#include "Lazerbass.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

#include "bsp/DebugIO.hpp"

namespace dsp {

/**
 * @brief parabolic warp
 * @param val01 0 to 1
 * @param warp -1 to 1
 * @return 
 */
static constexpr float ParabolaWarp(float val01, float warp) {
    return ((warp + 1) - std::abs(val01) * warp) * val01;
}

/**
 * @brief Computes a value based on the difference between input and breakpoint.
 * 
 * @param v The input value, typically between 0 and 1.
 * @param bp The breakpoint value, typically between 0 and 1.
 * @return A computed float value based on the relationship between v and bp.
 */
static constexpr float YUpBp1(float v, float bp) {
    float v0 = 1.0f - v;
    float v1 = 1.0f - bp;
    if (v0 > v1) {
        return 1.0f - (v0 - v1) / bp;
    }
    else {
        return 1.0f - v0;
    }
}

/**
 * @brief Converts a decibel (dB) value to a linear gain.
 * 
 * @param db The decibel value to convert.
 * @return The linear gain value corresponding to the input decibel value.
 * 
 * @details The conversion is done using the formula gain = 10^(db/20).
 */
static constexpr float Db2Gain(float db) {
    return std::pow(10.0f, db / 20.0f);
}

static constexpr float Semitone2Hz(float semitone) {
    return 8.17579891564f * std::exp2(semitone / 12.0f);
}

static constexpr float Semitone2Ratio(float deltaSemitone) {
    return std::exp2(deltaSemitone / 12.0f);
}

// --------------------------------------------------------------------------------
// Lazerbass
// --------------------------------------------------------------------------------
Lazerbass::Lazerbass()
    : lfo1_(params_.lfo1, params_)
    , lfo2_(params_.lfo2, params_)
    , lfo3_(params_.lfo3, params_)
    , lfo4_(params_.lfo4, params_)
    , ampEnv_(params_.ampEnv, params_)
    , env1_(params_.env1, params_)
    , env2_(params_.env2, params_) {
}

void Lazerbass::Init(uint32_t sampleRate, uint32_t updateRate) {
    sampleRate_ = sampleRate;
    tickPos_ = 0;
    tickPreiod_ = sampleRate / updateRate;
    twoPiInvSampleRate_ = std::numbers::pi_v<float> * 2.0f / sampleRate_;
    hasNoteOn_ = false;
    output_ = false;
    maxRadiusFreqs_ = kMaxFreq * twoPiInvSampleRate_;

    noteStack_.reserve(64);

    std::fill_n(oldFreqs_, std::size(oldFreqs_), -1.0f);

    lfo1_.Init(sampleRate, updateRate);
    lfo2_.Init(sampleRate, updateRate);
    lfo3_.Init(sampleRate, updateRate);
    lfo4_.Init(sampleRate, updateRate);
    ampEnv_.Init(sampleRate, updateRate);
    env1_.Init(sampleRate, updateRate);
    env2_.Init(sampleRate, updateRate);
}

void Lazerbass::Process(std::span<StereoSample> block) {
    uint32_t samplePos = 0;
    const uint32_t blockSize = static_cast<uint32_t>(block.size());
    while (samplePos < blockSize) {
        if (tickPos_ <= 0) {
            Tick();
            tickPos_ = tickPreiod_;
        }
        uint32_t numSamples = std::min(tickPos_, blockSize - samplePos);
        tickPos_ -= numSamples;
        AudioGen(block.data() + samplePos, numSamples);
        samplePos += numSamples;
    }
}

static float LimitCosConvert(float sin) {
    auto e = 1.0f - sin * sin;
    if (e < 0.0f) {
        return 0.0f;
    }
    else if (e > 1.0f) {
        return 1.0f;
    }
    else {
        return std::sqrt(e);
    }
}

void Lazerbass::AudioGen(StereoSample* out, uint32_t numSamples) {
    std::fill_n(out, numSamples, StereoSample{});

    if (!output_) {
        return;
    }
    
    const auto numPartials = static_cast<uint32_t>(params_.oscillor.numPartials.Get());
    /* 在第一个采样处执行频率更改
     *       phi     = (pi - w) / 2
     *       phi_new = (pi - w_new) / 2
     * Now:  x(n) = sin(phi_now)
     *       y(n) = sin(phi_now - phi)
     *       c = 2 * sin(w / 2)
     * Next: x(m) = sin(phi_now) = x(n)
     *       y(m) = sin(phi_now - phi_new)
     *            = sin(phi_now) * cos(phi_new) - cos(phi_now) * sin(phi_new)
     *            = x(n) * cos(phi_new) - cos(x(n)) * sin(phi_new)
     *            = x(n) * sin(w_new / 2) - cos(x(n)) * cos(w_new / 2)
     *       c_new = 2 * sin(w_new / 2)
     * PredCos: x(n) > x(n-1) ? |cos(x(n))| : -|cos(x(n))|
     *          |Cos(x(n))| = sqrt(1 - x(n)^2)
     */
    {
        float firstSampleOut = 0.0f;
        for (uint32_t i = 0; i < numPartials; ++i) {
            auto ret = sin0_[i];
            firstSampleOut += ret * gains_[i];

            sin0_[i] -= coefs_[i] * sin1_[i];
            sin1_[i] += coefs_[i] * sin0_[i];

            if (oldFreqs_[i] != freqs_[i]) {
                bool freqOutOfRange = freqs_[i] > maxRadiusFreqs_ || freqs_[i] < 0.0f;
                enable_[i] = !freqOutOfRange;

                if (sin0_[i] > ret) {
                    float predCos = LimitCosConvert(sin0_[i]);
                    coefs_[i] = 2.0f * std::sin(freqs_[i] / 2.0f);
                    sin1_[i] = sin0_[i] * std::sin(freqs_[i] / 2.0f) - predCos * std::cos(freqs_[i] / 2.0f);
                }
                else {
                    float predCos = -LimitCosConvert(sin0_[i]);
                    coefs_[i] = 2.0f * std::sin(freqs_[i] / 2.0f);
                    sin1_[i] = sin0_[i] * std::sin(freqs_[i] / 2.0f) - predCos * std::cos(freqs_[i] / 2.0f);
                }

                oldFreqs_[i] = freqs_[i];
            }
        }

        auto int16FirstSampleOut = static_cast<int16_t>(firstSampleOut * std::numeric_limits<int16_t>::max() / 8);
        out[0].left = int16FirstSampleOut;
        out[0].right = int16FirstSampleOut;
    }

    /* 在其他采样处执行恒定频率MCF
     * x(n+1) = x(n-1) - y(n)   * c
     * y(n+1) = y(n-1) + x(n+1) * c
    */
    for (uint32_t i = 0; i < numPartials; ++i) {
        auto c = coefs_[i];
        auto x = sin0_[i];
        auto y = sin1_[i];
        auto g = gains_[i];

        if (enable_[i]) {
            /* 计算振幅 */
            for (uint32_t sampleIdx = 1; sampleIdx < numSamples; ++sampleIdx) {
                // output
                auto oscOut = x * g;
                auto int16OscOut = static_cast<int16_t>(oscOut * std::numeric_limits<int16_t>::max() / 8);
                out[sampleIdx].left += int16OscOut;

                // mcf
                x -= y * c;
                y += x * c;
            }
        }
        else {
            /* 只计算相位 */
            for (uint32_t sampleIdx = 1; sampleIdx < numSamples; ++sampleIdx) {
                x -= y * c;
                y += x * c;
            }
        }

        sin0_[i] = x;
        sin1_[i] = y;
    }

    for (uint32_t i = 0; i < numSamples; ++i) {
        out[i].right = out[i].left;
    }
}

void Lazerbass::ResetPhase() {
    const auto numPartials = static_cast<uint32_t>(params_.oscillor.numPartials.Get());

    PhaseProcessing(numPartials);

    /* 修改起始相位
     * phi = (pi - w) / 2
     * x(0) = sin(phi_init)
     * y(0) = sin(phi_init - phi)
     */
    for (uint32_t i = 0; i < numPartials; ++i) {
        float phi = (std::numbers::pi_v<float> - freqs_[i]) / 2.0f;
        float phiInit = phase_[i];
        sin0_[i] = std::sin(phiInit);
        sin1_[i] = std::sin(phiInit - phi);
    }
}

void Lazerbass::ResetModulators() {
    lfo1_.ResetPhase();
    lfo2_.ResetPhase();
    lfo3_.ResetPhase();
    lfo4_.ResetPhase();
    ampEnv_.GotoAttackState();
    env1_.GotoAttackState();
    env2_.GotoAttackState();
}

uint32_t Lazerbass::NoteEnqueue(uint32_t noteNumber) {
    auto pos = std::ranges::find(noteStack_, noteNumber);
    if (pos == noteStack_.end()) {
        noteStack_.push_back(noteNumber);
    }
    else {
        noteStack_.erase(pos);
        noteStack_.push_back(noteNumber);
    }
    return noteNumber;
}

uint32_t Lazerbass::NoteDequeue(uint32_t noteNumber) {
    auto pos = std::ranges::find(noteStack_, noteNumber);
    if (pos != noteStack_.end()) {
        noteStack_.erase(pos);
    }

    if (noteStack_.empty()) {
        return kInvalidNoteNumber;
    }
    else {
        return noteStack_.back();
    }
}

ModulatorDesc Lazerbass::GetModulatorDesc(ModulatorId id) {
    using enum ModulatorId;

    switch (id) {
    case kLfo1:
        return lfo1_.GetModulatorDesc();
    case kLfo2:
        return lfo2_.GetModulatorDesc();
    case kLfo3:
        return lfo3_.GetModulatorDesc();
    case kLfo4:
        return lfo4_.GetModulatorDesc();
    case kAmpEnv:
        return ampEnv_.GetModulatorDesc();
    case kEnv1:
        return env1_.GetModulatorDesc();
    case kEnv2:
        return env2_.GetModulatorDesc();
    default:
        return lfo1_.GetModulatorDesc();
    }
}

void Lazerbass::NoteOn(uint32_t noteNumber, float velocity)
{
    noteNumber_ = NoteEnqueue(noteNumber);
    velocity_ = velocity;
    hasNoteOn_ = true;

    // TODO: adsr envelope
    output_ = true;
}

void Lazerbass::NoteOff(uint32_t noteNumber, float /*velocity*/) {
    auto note = NoteDequeue(noteNumber);
    if (note == kInvalidNoteNumber) {
        output_ = false;
        ampEnv_.GotoReleaseState();
        env1_.GotoReleaseState();
        env2_.GotoReleaseState();
    }
    else if (note != noteNumber_) {
        // same as note on
        noteNumber_ = note;
        hasNoteOn_ = true;
    }
}

void Lazerbass::Tick()
{
    const auto numPartials = static_cast<uint32_t>(params_.oscillor.numPartials.Get());

    // step-1: update modulator and parameters
    UpdateModulators();
    modulationBank_.Tick();
    
    // step0: calculate pitch and fundemental frequency
    pitch_ = noteNumber_;
    fundamental_ = Semitone2Hz(pitch_);

    // step1: oscilator -> ratio and gain
    OscillatorProcessing(numPartials);

    // step2 ratio processing
    RatioProcessing(numPartials);

    // step3 filter processing
    FilterProcessing(numPartials);
    PeriodFilterProcessing(numPartials);

    // step4 update freqs
    auto radixFundamental = fundamental_ * twoPiInvSampleRate_;
    for (uint32_t i = 0; i < numPartials; ++i) {
        freqs_[i] = ratio_[i] * radixFundamental;
    }

    // step5 part beating process
    BeatingProcessing(numPartials);

    // step6 update sines
    if (hasNoteOn_) {
        ResetPhase();
        ResetModulators();
        hasNoteOn_ = false;
    }
}

void Lazerbass::UpdateModulators() {
    lfo1_.Tick();
    lfo2_.Tick();
    lfo3_.Tick();
    lfo4_.Tick();
    ampEnv_.Tick();
    env1_.Tick();
    env2_.Tick();
}

// --------------------------------------------------------------------------------
// Processing Units
// --------------------------------------------------------------------------------
static constexpr auto kSawGainTable = []{
    std::array<float, Lazerbass::kMaxNumPartials * 2> ret;
    for (uint32_t i = 0; i < Lazerbass::kMaxNumPartials * 2; ++i) {
        ret[i] = 1.0f / (1.0f + i);
    }
    return ret;
}();

void Lazerbass::OscillatorProcessing(uint32_t numProcess) {
    using enum dsp::OscillatorType;
    switch (params_.oscillor.type.Get()) {
    case kFullSaw: {
        std::copy_n(kSawGainTable.cbegin(), numProcess, gains_);

        /* 计算偶次谐波偏移 */
        auto ratioBeating = params_.oscillor.beating.GetWithModulation() / fundamental_ + 1.0f;
        ratioBeating *= Semitone2Ratio(params_.oscillor.transport.GetWithModulation());
        for (uint32_t i = 0; i < numProcess; i += 2) {
            ratio_[i] = i + 1.0f;
            ratio_[i + 1] = (i + 2.0f) * ratioBeating;
        }
        break;
    }
    case kDualSaw: {
        uint32_t partialIdx = 0;
        auto ratioBeating = params_.oscillor.beating.GetWithModulation() / fundamental_ + 1.0f;
        ratioBeating *= Semitone2Ratio(params_.oscillor.transport.GetWithModulation());
        for (uint32_t i = 0; i < numProcess; i += 2) {
            gains_[i] = kSawGainTable[partialIdx];
            gains_[i + 1] = kSawGainTable[partialIdx];
            ratio_[i] = partialIdx + 1.0f;
            ratio_[i + 1] = (partialIdx + 1.0f) * ratioBeating;
            ++partialIdx;
        }
        break;
    }
    case kMultiSaw: {
        uint32_t numOsc = params_.oscillor.number.Get();
        float lowestDeltaPitch = -params_.oscillor.beating.GetWithModulation();
        float lowestRatio = Semitone2Ratio(lowestDeltaPitch);
        float pitchInterval = (params_.oscillor.beating.GetWithModulation() * 2) / (numOsc - 1);
        float ratioInterval = Semitone2Ratio(pitchInterval);

        float oscRatio = lowestRatio;
        for (uint32_t i = 0; i < numOsc; ++i) {
            uint32_t partialIdx = 0;
            for (uint32_t j = i; j < numProcess; j += numOsc) {
                gains_[j] = kSawGainTable[partialIdx];
                ratio_[j] = (partialIdx + 1.0f) * oscRatio;
                ++partialIdx;
            }
            oscRatio *= ratioInterval;
        }
        break;
    }
    case kFullSquare: {
        auto ratioBeating = params_.oscillor.beating.GetWithModulation() / fundamental_ + 1.0f;
        ratioBeating *= Semitone2Ratio(params_.oscillor.transport.GetWithModulation());
        for (uint32_t i = 0; i < numProcess; i += 2) {
            gains_[i] = kSawGainTable[2 * i];
            gains_[i + 1] = kSawGainTable[2 * i + 3];
            ratio_[i] = 2 * i + 1.0f;
            ratio_[i + 1] = (2 * i + 3.0f) * ratioBeating;
        }
        break;
    }
    case kDualSquare: {
        uint32_t partialIdx = 0;
        auto ratioBeating = params_.oscillor.beating.GetWithModulation() / fundamental_ + 1.0f;
        ratioBeating *= Semitone2Ratio(params_.oscillor.transport.GetWithModulation());
        for (uint32_t i = 0; i < numProcess; i += 2) {
            gains_[i] = kSawGainTable[partialIdx];
            gains_[i + 1] = kSawGainTable[partialIdx];

            ratio_[i] = partialIdx + 1.0f;
            ratio_[i + 1] = (partialIdx + 1.0f) * ratioBeating;
            partialIdx += 2;
        }
        break;
    }
    case kMultiSquare: {
        uint32_t numOsc = params_.oscillor.number.Get();
        float lowestDeltaPitch = -params_.oscillor.beating.GetWithModulation();
        float lowestRatio = Semitone2Ratio(lowestDeltaPitch);
        float pitchInterval = (params_.oscillor.beating.GetWithModulation() * 2) / (numOsc - 1);
        float ratioInterval = Semitone2Ratio(pitchInterval);

        float oscRatio = lowestRatio;
        for (uint32_t i = 0; i < numOsc; ++i) {
            uint32_t partialIdx = 0;
            for (uint32_t j = i; j < numProcess; j += numOsc) {
                gains_[j] = kSawGainTable[partialIdx];
                ratio_[j] = (partialIdx + 1.0f) * oscRatio;
                partialIdx += 2;
            }
            oscRatio *= ratioInterval;
        }
        break;
    }
    case kPwmSquare: {
        auto ratioBeating = params_.oscillor.beating.GetWithModulation() / fundamental_ + 1.0f;
        ratioBeating *= Semitone2Ratio(params_.oscillor.transport.GetWithModulation());

        constexpr float pi = std::numbers::pi_v<float>;
        float pulseWidth = params_.oscillor.pluseWidth.GetWithModulation();
        float mul0 = pulseWidth * pi;

        for (uint32_t i = 0; i < numProcess; i += 2) {
            ratio_[i] = i + 1.0f;
            ratio_[i + 1] = (i + 2.0f) * ratioBeating;

            gains_[i] = kSawGainTable[i] * (std::cos(mul0 * (i + 1.0f)) - 1.0f) * 0.5f;
            gains_[i + 1] = kSawGainTable[i + 1] * (std::cos(mul0 * (i + 2.0f)) - 1.0f) * 0.5f;
        }
        break;
    }
    case kFullPulse: {
        std::fill_n(gains_, numProcess, 0.5f);
        for (uint32_t i = 0; i < numProcess; ++i) {
            ratio_[i] = i + 1.0f;
        }
        break;
    }
    default:
        break;
    }
    gains_[0] *= params_.oscillor.fundamental.Get();
}

void Lazerbass::RatioProcessing(uint32_t numProcess) {
    if (params_.dispersion.enable.Get()) {
        float dp = pitch_ - 60;
        float dpff = Semitone2Ratio(dp);
        float l = LerpUncheck(1, 1.0f / dpff, params_.dispersion.key.GetWithModulation());
        float shape = params_.dispersion.shape.GetWithModulation();
        float amount = params_.dispersion.amount.GetWithModulation();
        float absAmount = std::abs(amount);
        for (uint32_t i = 0; i < numProcess; ++i) {
            float idx01 = i / static_cast<float>(kMaxOrignalNumPartials);
            float mul0 = ParabolaWarp(idx01, shape) * l;
            float val1 = absAmount * 4 * mul0 + 1;
            if (amount > 0) {
                ratio_[i] *= val1;
            }
            else {
                ratio_[i] /= val1;
            }
        }
    }

    if (params_.ratioMul.enable.Get()) {
        uint32_t parttern = params_.ratioMul.parttern.Get();
        uint32_t notApply = parttern / 2;
        uint32_t apply = parttern - notApply;
        float amount = params_.ratioMul.amount.GetWithModulation();
        amount = SynthParams::RatioMulAmountConvert(amount);

        uint32_t i = 0;
        while (i < numProcess) {
            i += notApply;
            for (uint32_t j = 0; j < apply && i < numProcess; ++j) {
                ratio_[i] *= amount;
                ++i;
            }
        }
    }

    if (params_.ratioAdd.enable.Get()) {
        uint32_t parttern = params_.ratioAdd.parttern.Get();
        uint32_t notApply = parttern / 2;
        uint32_t apply = parttern - notApply;
        float amount = params_.ratioAdd.amount.GetWithModulation();

        uint32_t i = 0;
        while (i < numProcess) {
            i += notApply;
            for (uint32_t j = 0; j < apply && i < numProcess; ++j) {
                ratio_[i] += amount;
                ++i;
            }
        }
    }
}

void Lazerbass::BeatingProcessing(uint32_t numProcess) {
    if (params_.partialBeating.enable.Get()) {
        uint32_t parttern = params_.partialBeating.parttern.Get();
        uint32_t notApply = parttern / 2;
        uint32_t apply = parttern - notApply;
        float amount = params_.partialBeating.amount.GetWithModulation();
        float radixFreq = amount * twoPiInvSampleRate_;

        uint32_t i = 0;
        while (i < numProcess) {
            i += notApply;
            for (uint32_t j = 0; j < apply && i < numProcess; ++j) {
                freqs_[i] += radixFreq;
                ++i;
            }
        }
    }
}

void Lazerbass::PhaseProcessing(uint32_t numProcess) {
    if (params_.oscPhase.enable.Get()) {
        float randomAmount = params_.oscPhase.random.GetWithModulation();
        float symmetry = params_.oscPhase.symmetry.GetWithModulation();
        uint32_t parttern = params_.oscPhase.parttern.Get();

        uint32_t applyLeft = parttern / 2;
        uint32_t applyRight = parttern - applyLeft;
        float maxRadix = std::numbers::pi_v<float> * randomAmount;
        float leftAmount = (1.0f - symmetry) * 2.0f * maxRadix;
        float rightAmount = symmetry * 2.0f * maxRadix;

        uint32_t i = 0;
        while (i < numProcess) {
            uint32_t loopLeft = std::min(applyLeft, numProcess - i);
            for (uint32_t j = 0; j < loopLeft; ++j) {
                phase_[i++] = leftAmount * rand() / static_cast<float>(RAND_MAX);
            }

            uint32_t loopRight = std::min(applyRight, numProcess - i);
            for (uint32_t j = 0; j < loopRight; ++j) {
                phase_[i++] = rightAmount * rand() / static_cast<float>(RAND_MAX);
            }
        }
    }
}

void Lazerbass::PeriodFilterProcessing(uint32_t numProcess) {
    if (params_.periodFilter.enable.Get()) {
        float argPeak = params_.periodFilter.peak.GetWithModulation();
        float argApply = params_.periodFilter.apply.GetWithModulation();
        bool argBlocks = params_.periodFilter.blocks.Get();
        float argPinch = params_.periodFilter.pinch.GetWithModulation();
        bool argStretch = params_.periodFilter.stretch.Get();
        float argCycle = params_.periodFilter.cycle.GetWithModulation();
        float argPhaseShift = params_.periodFilter.phaseShift.GetWithModulation();

        float log2NumProcess = 1.0f / std::log2(numProcess);

        const float magFloor = LerpUncheck(24.0f, 300.0f, argPeak);
        const float lerpVal0 = YUpBp1(argPeak, 0.5f);
        const float val0 = 1.0f + argApply * 0.3f
                        + argPeak * 1.2f * (1.0f - argBlocks);
        const float val1 = val0 * argApply;
        const float val2 = 1.0f - argApply;

        for (uint32_t i = 0; i < numProcess; ++i) {
            float idx01 = i / static_cast<float>(kMaxOrignalNumPartials);
            float val0 = ParabolaWarp(idx01, argPinch);
            float phase0 = val0 * argCycle + argPhaseShift;
            if (argStretch) {
                phase0 = val0 * argCycle * numProcess + 1;
                float val1 = std::log2(phase0);
                float val2 = argCycle * log2NumProcess;
                phase0 = val1 * val2 + argPhaseShift;
            }
            float phaseRound = phase0 - static_cast<int32_t>(phase0);
            float waveVal = phaseRound > 0.5f ? 1.0f : 0.0f;
            if (!argBlocks) {
                waveVal = std::cos(phaseRound * std::numbers::pi_v<float> * 2.0f);
                waveVal = (waveVal + 1.0f) * 0.5f;
            }

            float mag = (waveVal - 1.0f) * magFloor;
            float level = Db2Gain(mag);
            float level0 = LerpUncheck(waveVal, level, lerpVal0);
            float level1 = level0 * val1 + val2;
            gains_[i] *= level1;
        }
    }
}

void Lazerbass::FilterProcessing(uint32_t numProcess) {
}

}
