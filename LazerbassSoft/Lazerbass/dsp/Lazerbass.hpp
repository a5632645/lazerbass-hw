#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include "Types.hpp"
#include "dsp/params.hpp"
#include "dsp/ModulationBank.hpp"
#include "dsp/LFO.hpp"
#include "dsp/Envelope.hpp"

namespace dsp {

class Lazerbass {
public:
    static constexpr int kMaxNumPartials = 256;
    static constexpr int kMaxOrignalNumPartials = 324;
    static constexpr float kMaxFreq = 12000.0f;
    static constexpr uint32_t kInvalidNoteNumber = 1024;

    Lazerbass();

    void Init(uint32_t sampleRate, uint32_t updateRate);
    void Process(std::span<StereoSample> block);

    SynthParams& GetParams() { return params_; }
    ModulationBank& GetModulationBank() { return modulationBank_; }
    ModulatorDesc GetModulatorDesc(ModulatorId id);

    void NoteOn(uint32_t noteNumber, float velocity);
    void NoteOff(uint32_t noteNumber, float velocity);
    void SetPitchBend(float pitchBend) { pitchBend_ = pitchBend; }
private:
    void Tick();
    void UpdateModulators();
    void AudioGen(StereoSample* out, uint32_t numSamples);
    void ResetPhase();
    void ResetModulators();

    void OscillatorProcessing(uint32_t numProcess);
    void RatioProcessing(uint32_t numProcess);
    void BeatingProcessing(uint32_t numProcess);
    void PhaseProcessing(uint32_t numProcess);
    
    void PeriodFilterProcessing(uint32_t numProcess);
    void FilterProcessing(uint32_t numProcess);

    uint32_t NoteEnqueue(uint32_t noteNumber);
    uint32_t NoteDequeue(uint32_t noteNumber);

    uint32_t sampleRate_{};
    float twoPiInvSampleRate_{};
    float maxRadiusFreqs_{};
    uint32_t tickPos_{};
    uint32_t tickPreiod_{};

    // mcf
    float sin0_[kMaxNumPartials]{};
    float sin1_[kMaxNumPartials]{};
    float coefs_[kMaxNumPartials]{};

    // sines
    float freqs_[kMaxNumPartials]{};
    float oldFreqs_[kMaxNumPartials]{};
    float phase_[kMaxNumPartials]{};
    bool enable_[kMaxNumPartials]{};

    // notes
    bool output_{};
    float velocity_{};
    bool hasNoteOn_{};

    // pitch and fundamental
    uint32_t noteNumber_{};
    float pitchBend_{};
    float pitch_{};
    float fundamental_{};

    // processings
    float gains_[kMaxNumPartials]{};
    float ratio_[kMaxNumPartials]{};

    // note statck
    std::vector<uint8_t> noteStack_;

    // parameters
    SynthParams params_;
    ModulationBank modulationBank_;
    LFO lfo1_;
    LFO lfo2_;
    LFO lfo3_;
    LFO lfo4_;
    Envelope ampEnv_;
    Envelope env1_;
    Envelope env2_;
};

}
