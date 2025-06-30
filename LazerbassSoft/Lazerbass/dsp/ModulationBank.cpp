#include "ModulationBank.hpp"

namespace dsp {

void ModulationBank::Tick() {
    for (uint32_t i = 0; i < numParamInfos_; ++i) {
        auto& paramInfo = paramInfos_[i];
        paramInfo.paramReg->modulationValue = 0.0f;
    }

    for (uint32_t i = 0; i < numLinks_; ++i) {
        auto& link = links_[i];
        if (link.enable) {
            float modulatorOutput = *link.sourceModulator.outputReg;
            float modulationValue = 0.0f;
            if (link.symmetric) {
                modulationValue = (modulatorOutput - 0.5f) * link.amount;
            }
            else {
                modulationValue = modulatorOutput * link.amount;
            }
            link.targetParamInfo->paramReg->modulationValue += modulationValue;
        }
    }
}

ModulationLink* ModulationBank::AddNewLink(ModulatorDesc sourceModulator, FloatParamDesc* targetParam, bool& exited) {
    if (numLinks_ >= kMaxNumModulations) {
        exited = false;
        return nullptr;
    }

    auto* linkExist = GetLink(sourceModulator, targetParam);
    if (linkExist != nullptr) {
        exited = true;
        return linkExist;
    }

    auto& allocLink = links_[numLinks_++];
    allocLink.sourceModulator = sourceModulator;
    allocLink.enable = true;
    allocLink.amount = 0.0f;
    allocLink.symmetric = false;

    auto* regInfo = TryAddNewParamRegInfo(targetParam);
    allocLink.targetParamInfo = regInfo;
    regInfo->numLinks++;

    return &allocLink;
}

ModulationLink* ModulationBank::GetLink(ModulatorDesc sourceModulator, FloatParamDesc* targetParam) {
    for (uint32_t i = 0; i < numLinks_; ++i) {
        if (links_[i].sourceModulator == sourceModulator 
            && links_[i].targetParamInfo->paramReg == targetParam) {
            return &links_[i];
        }
    }
    return nullptr;
}

uint32_t ModulationBank::GetLinkOfParam(FloatParamDesc* targetParam, std::span<ModulationLinkHandle> links) {
    uint32_t maxWrite = links.size();
    uint32_t write = 0;

    for (uint32_t i = 0; i < numLinks_; ++i) {
        if (links_[i].targetParamInfo->paramReg == targetParam) {
            if (write >= maxWrite) {
                break;
            }
            else {
                links[write++] = &links_[i];
            }
        }
    }

    return write;
}

uint32_t ModulationBank::GetLinkOfModulator(ModulatorDesc sourceModulator, std::span<ModulationLinkHandle> links) {
    uint32_t maxWrite = links.size();
    uint32_t write = 0;

    for (uint32_t i = 0; i < numLinks_; ++i) {
        if (links_[i].sourceModulator == sourceModulator) {
            if (write >= maxWrite) {
                break;
            }
            else {
                links[write++] = &links_[i];
            }
        }
    }

    return write;
}

void ModulationBank::RemoveLink(ModulationLink* link) {
    std::swap(*link, links_[numLinks_ - 1]);
    --numLinks_;

    --link->targetParamInfo->numLinks;
    if (link->targetParamInfo->numLinks == 0) {
        RemoveParamRegInfo(link->targetParamInfo);
    }
}

void ModulationBank::RemoveLink(ModulatorDesc sourceModulator, FloatParamDesc* targetParam)
{
    for (uint32_t i = 0; i < numLinks_; ++i) {
        if (links_[i].sourceModulator == sourceModulator 
            && links_[i].targetParamInfo->paramReg == targetParam) {
            RemoveLink(&links_[i]);
            break;
        }
    }
}

void ModulationBank::RemoveLinkOfParam(FloatParamDesc* targetParam) {
    for (uint32_t i = 0; i < numLinks_;) {
        if (links_[i].targetParamInfo->paramReg == targetParam) {
            RemoveLink(&links_[i]);
        }
        else {
            ++i;
        }
    }
}

void ModulationBank::RemoveLinkOfModulator(ModulatorDesc sourceModulator) {
    for (uint32_t i = 0; i < numLinks_;) {
        if (links_[i].sourceModulator == sourceModulator) {
            RemoveLink(&links_[i]);
        }
        else {
            ++i;
        }
    }
}

void ModulationBank::RemoveAllLinks() {
    numLinks_ = 0;
    numParamInfos_ = 0;
}

ParamRegInfo *ModulationBank::TryAddNewParamRegInfo(FloatParamDesc* targetParam) {
    for (uint32_t i = 0; i < numParamInfos_; ++i) {
        if (paramInfos_[i].paramReg == targetParam) {
            return &paramInfos_[i];
        }
    }

    auto& allocParamInfo = paramInfos_[numParamInfos_++];
    allocParamInfo.paramReg = targetParam;
    allocParamInfo.numLinks = 1;
    
    return &allocParamInfo;
}

void ModulationBank::RemoveParamRegInfo(ParamRegInfo* targetParam) {
    std::swap(*targetParam, paramInfos_[numParamInfos_ - 1]);
    --numParamInfos_;
}

void ModulationBank::RemoveParamRegInfo(FloatParamDesc* targetParam) {
    for (uint32_t i = 0; i < numParamInfos_;) {
        if (paramInfos_[i].paramReg == targetParam) {
            std::swap(paramInfos_[i], paramInfos_[numParamInfos_ - 1]);
            --numParamInfos_;
            break;
        }
    }
}

}