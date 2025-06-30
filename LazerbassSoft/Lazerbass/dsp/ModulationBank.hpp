#pragma once
#include <cstdint>
#include <array>
#include <span>
#include "params.hpp"
#include "ModulatorDesc.hpp"

namespace dsp {

struct ParamRegInfo {
    FloatParamDesc* paramReg;
    uint32_t numLinks;
};

struct ModulationLink {
    bool enable{};
    bool symmetric{};
    float amount{}; // -1~1
    ModulatorDesc sourceModulator{};
    ParamRegInfo* targetParamInfo{};

    auto GetModulationRange() const {
        struct ReturnStruct {
            float min;
            float max;
        } ret;

        if (symmetric) {
            ret.min = -amount * 0.5f;
            ret.max = amount * 0.5f;
        }
        else {
            ret.min = 0.0f;
            ret.max = amount;
        }

        return ret;
    }
};
using ModulationLinkHandle = ModulationLink*;

class ModulationBank {
public:
    static constexpr uint32_t kMaxNumModulations = 16;

    /**
     * @brief 更新参数
     */
    void Tick();

    /**
     * @brief 如果数量达到上限,返回nullptr, 如果已经存在,返回已经存在的并且设置exited为true
     * @param sourceModulator !nullptr
     * @param targetParam !nullptr
     * @param exitsed 返回
     * @return 
     */
    ModulationLink* AddNewLink(ModulatorDesc sourceModulator, FloatParamDesc* targetParam, bool& exitsed);

    /**
     * @brief 如果没有找到,返回nullptr
     * @param sourceModulator 
     * @param targetParam 
     * @return 
     */
    ModulationLink* GetLink(ModulatorDesc sourceModulator, FloatParamDesc* targetParam);

    /**
     * @brief 获取指定参数的所有link
     * @param targetParam !nullptr
     * @param links 缓冲区
     * @return 缓冲区内写入的数量
     */
    uint32_t GetLinkOfParam(FloatParamDesc* targetParam, std::span<ModulationLinkHandle> links);

    /**
     * @brief 获取指定modulator的所有link
     * @param sourceModulator !nullptr
     * @param links 缓冲区
     * @return 缓冲区内写入的数量
     */
    uint32_t GetLinkOfModulator(ModulatorDesc sourceModulator, std::span<ModulationLinkHandle> links);

    /**
     * @brief 获取所有link
     * @return 
     */
    std::span<ModulationLink> GetLinks() { return std::span<ModulationLink>(links_.data(), numLinks_); }

    /**
     * @brief 移除link
     * @param link nullptr
     */
    void RemoveLink(ModulationLink* link);

    /**
     * @brief 移除指定的link
     * @param sourceModulator nullptr
     * @param targetParam nullptr
     */
    void RemoveLink(ModulatorDesc sourceModulator, FloatParamDesc* targetParam);

    /**
     * @brief 移除参数的所有link
     * @param targetParam nullptr
     */
    void RemoveLinkOfParam(FloatParamDesc* targetParam);

    /**
     * @brief 移除modulator的所有link
     * @param sourceModulator nullptr
     */
    void RemoveLinkOfModulator(ModulatorDesc sourceModulator);

    /**
     * @brief 移除所有link
     */
    void RemoveAllLinks();
private:
    ParamRegInfo* TryAddNewParamRegInfo(FloatParamDesc* targetParam);
    void RemoveParamRegInfo(ParamRegInfo* targetParam);
    void RemoveParamRegInfo(FloatParamDesc* targetParam);

    std::array<ModulationLink, kMaxNumModulations> links_;
    uint32_t numLinks_{};

    std::array<ParamRegInfo, kMaxNumModulations> paramInfos_{};
    uint32_t numParamInfos_{};
};

}