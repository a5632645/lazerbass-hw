#pragma once

#include "obj/Oscillator.hpp"
#include "obj/RatioMul.hpp"
#include "obj/RatioAdd.hpp"
#include "obj/PartBeating.hpp"
#include "obj/Dispersion.hpp"
#include "obj/OscPhase.hpp"
#include "obj/PeriodFilter.hpp"
#include "obj/ParamModulations.hpp"
#include "obj/LazerbassLogo.hpp"
#include "obj/LFO.hpp"
#include "obj/Envelope.hpp"

namespace gui {

struct GuiObjs {
    inline static Oscillator oscillator;
    inline static RatioMul ratioMul;
    inline static RatioAdd ratioAdd;
    inline static PartBeating partBeating;
    inline static Dispersion dispersion;
    inline static OscPhase oscPhase;
    inline static PeriodFilter periodFilter;
    inline static LazerbassLogo lazerbassLogo;
    inline static ParamModulations paramModulations;
    inline static LFO lfo;
    inline static Envelope envelope;
};

}