#pragma once

namespace dsp {

struct ModulatorDesc {
    const char* name{};
    float* outputReg{};

    bool operator==(const ModulatorDesc& rhs) const { return outputReg == rhs.outputReg; }
};

}