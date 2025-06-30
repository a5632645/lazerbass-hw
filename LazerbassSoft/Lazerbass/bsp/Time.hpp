#pragma once
#include <cstdint>

namespace bsp {

class Time {
public:
    static constexpr uint32_t kUsPerTick = 100;

    static void Init();
    static uint32_t GetTick();
    static void ClearCounter();

    static constexpr uint32_t Tick2Ms(uint32_t tick) {
        return tick * kUsPerTick / 1000;
    }
};

}