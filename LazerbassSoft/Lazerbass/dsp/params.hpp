#pragma once
#include "ParamDesc.hpp"
#include <cmath>

namespace dsp {

enum class OscillatorType {
    kFullSaw = 0,
    kDualSaw,
    kMultiSaw,
    kFullSquare,
    kDualSquare,
    kMultiSquare,
    kPwmSquare,
    kFullPulse,
    kCount
};
static constexpr const char* kOscillatorTypeNames[] = {
    "FullSaw",
    "DualSaw",
    "MultiSaw",
    "FullSquare",
    "DualSquare",
    "MultiSquare",
    "PwmSquare",
    "FullPulse"
};

enum class LFOType {
    kSawTri = 0,
    kSampleAndHode,
    kNoise,
    kCount
};
static constexpr const char* kLFOTypeNames[] = {
    "sawTri",
    "s&h",
    "noise"
};

enum class ModulatorId {
    kLfo1 = 0,
    kLfo2,
    kLfo3,
    kLfo4,
    kAmpEnv,
    kEnv1,
    kEnv2,
    kCount
};

struct SynthParams {
    uint32_t bpm = 120;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        FloatParamDesc amount               { "amount",         0.0f,   16.0f,      0.01f,      0.0f,       10 };
        IntParamDesc parttern               { "parttern",       2,      64,                     2,          5 };
    } ratioAdd;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        FloatParamDesc amount               { "amount",         -4.0f,  5.0f,       0.5f,       1.0f,       10 }; // 负数表示除法
        IntParamDesc parttern               { "parttern",       2,      64,                     2,          5 };
    } ratioMul;
    static constexpr float RatioMulAmountConvert(float amount) {
        if (amount < 0.4f) { // 0.0 <- -4.0
            amount -= 1.0f;
            amount = -amount;
            amount = 1.0f / amount;
        }
        return amount;
    }
    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        FloatParamDesc amount               { "amount",         0.0f,   16.0f,      0.01f,      0.0f,       10 };
        IntParamDesc parttern               { "parttern",       2,      64,                     2,          10 };
    } partialBeating;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        FloatParamDesc amount               { "amount",         -1.0f,  1.0f,       0.01f,      0.0f,       10 };
        FloatParamDesc key                  { "key",            0.0f,   1.0f,       0.01f,      1.0f,       10 };
        FloatParamDesc shape                { "shape",          -1.0f,  1.0f,       0.01f,      0.0f,       10 };
    } dispersion;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        EnumParamDesc<OscillatorType> type  { "type",                                           OscillatorType::kFullSaw };
        IntParamDesc numPartials            { "numPartials",    2,      256,                    256,        8 }; // mul is 2
        IntParamDesc number                 { "number",         2,      6,                      2,          1 };
        FloatParamDesc transport            { "transport",      -24.0f, 24.0f,      0.01f,      0.0f,       25 };
        FloatParamDesc fundamental          { "fundamental",    0.0f,   1.0f,       0.01f,      1.0f,       10 };
        FloatParamDesc beating              { "beating",        0.0f,   16.0f,      0.01f,      0.0f,       25 };
        FloatParamDesc pluseWidth           { "pluseWidth",     0.0f,   1.0f,       0.01f,      1.0f,       10 };
    } oscillor;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        IntParamDesc balance                { "balance",        0,      100,                    50,         10 };
        IntParamDesc parttern               { "parttern",       2,      64,                     2,          10 };
        FloatParamDesc symmetry             { "symmetry",       -1.0f,  1.0f,       0.01f,      0.0f,       10 };
    } attenuation;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        FloatParamDesc random               { "random",         0.0f,   1.0f,       0.01f,      0.0f,       10 };
        IntParamDesc parttern               { "parttern",       2,      64,                     2,          10 };
        FloatParamDesc symmetry             { "symmetry",       0.0f,   1.0f,       0.01f,      0.5f,       10 };
    } oscPhase;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        FloatParamDesc brightness           { "brightness",     0.0f,   1.0f,       0.01f,      1.0f,       10 };
        FloatParamDesc key                  { "key",            0.0f,   1.0f,       0.01f,      1.0f,       10 };
        FloatParamDesc floor                { "floor",          0.0f,   1.0f,       0.01f,      1.0f,       10 };
    } filter;

    struct {
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc enable                { "enable",                                         false };
        BoolParamDesc stretch               { "stretch",                                        true };
        BoolParamDesc blocks                { "blocks",                                         false };
        FloatParamDesc apply                { "apply",          0.0f,   1.0f,       0.01f,      1.0f,       10 };
        FloatParamDesc peak                 { "peak",           0.0f,   1.0f,       0.01f,      0.0f,       10 };
        FloatParamDesc cycle                { "cycle",          0.0f,   162.0f,     0.01f,      6.0f,       25 };
        FloatParamDesc phaseShift           { "phaseShift",     0.0f,   1.0f,       0.01f,      0.0f,       10 };
        FloatParamDesc pinch                { "pinch",          -1.0f,  1.0f,       0.01f,      0.0f,       10 };
    } periodFilter;

    struct LfoParamDesc {
        const char* const name;
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc bpm                   { "bpm",                                            false };
        BoolParamDesc snap                  { "snap",                                           false };
        BoolParamDesc restart               { "restart",                                        false };
        EnumParamDesc<LFOType> type         { "type",                                           LFOType::kSawTri };
        FloatParamDesc rate                 { "rate",           0.0f,   1.0f,       0.005f,     0.5f,       10 };
        IntParamDesc times                  { "times",          0,      2,                      1,          1 }; // 0.5x 1x 2x
        IntParamDesc dotTrip                { "dotTrip",        0,      2,                      1,          1 }; // 2/3x 1x 3/2x
        FloatParamDesc shape                { "shape",          0.0f,   1.0f,       0.01f,      0.5f,       10 };
    };
    LfoParamDesc lfo1 { .name = "lfo1" };
    LfoParamDesc lfo2 { .name = "lfo2" };
    LfoParamDesc lfo3 { .name = "lfo3" };
    LfoParamDesc lfo4 { .name = "lfo4" };
    static constexpr float GetLfoFrequency(uint32_t bpm, LfoParamDesc& desc, float rate) {
        float baseFreq = 0.0f;
        if (desc.bpm.value) {
            float mul0 = rate * 4.0f; // 2^0 ~ 2^4
            if (desc.snap.Get()) {
                mul0 = std::round(mul0);
            }
            float div = std::exp2(mul0);
            float freq = 60.0f / bpm;
            baseFreq = freq * div;
        }
        else {
            constexpr float alpha = 2.2f;
            // constexpr float invMul = 1.0f / (gcem::exp2(alpha) - 1.0f);
            constexpr float invMul = 1.0f / 8.02501349943412f;
            float bendVal01Up = std::exp(alpha * rate) - 1.0f;
            float bendVal01 = bendVal01Up * invMul;
            baseFreq = bendVal01 * 8.0f;
            baseFreq = std::round(baseFreq * 100.0f) / 100.0f;
        }

        switch (desc.times.Get()) {
        case 0:
            baseFreq *= 0.5f;
            break;
        case 1:
            baseFreq *= 1.0f;
            break;
        case 2:
            baseFreq *= 2.0f;
            break;
        }

        switch(desc.dotTrip.Get()) {
        case 0:
            baseFreq *= (2.0f / 3.0f);
            break;
        case 1:
            baseFreq *= 1.0f;
            break;
        case 2:
            baseFreq *= (3.0f / 2.0f);
            break;
        }

        return baseFreq;
    }

    struct EnvParamDesc {
        static constexpr float kMinTime = 1.0f / 1000.0f;
        const char* const name;
//                                          | name            |  min  |  max  |   step      |   default   | altMul
        BoolParamDesc invert                { "invert",                                         false };
        FloatParamDesc attack               { "attack",         0.0f,   1.0f,       0.005f,     0.5f,       20 };
        FloatParamDesc peak                 { "peak",           0.0f,   1.0f,       0.01f,      1.0f,       10 };
        FloatParamDesc release              { "release",        0.0f,   1.0f,       0.005f,     0.5f,       20 };
    };
    EnvParamDesc ampEnv { .name = "ampEnv" };
    EnvParamDesc env1 { .name = "env1" };
    EnvParamDesc env2 { .name = "env2" };
    static constexpr float EnvVal01ToTime(float val01) {
        constexpr float alpha = 4.4f;
        // constexpr float invMul = 1.0f / (gcem::exp2(alpha) - 1.0f);
        constexpr float invMul = 1.0f / 80.450868665f;
        float bendVal01Up = std::exp(alpha * val01) - 1.0f;
        float bendVal01 = bendVal01Up * invMul;
        return bendVal01 * 10.0f; // 10秒
    }

//                                          | name            |  min  |  max  |   step      |   default   | altMul
};

}
