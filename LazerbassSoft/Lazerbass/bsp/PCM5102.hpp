#pragma once
#include <span>
#include <cstdint>
#include "Types.hpp"

namespace bsp {

class PCM5102 {
public:
    static constexpr uint32_t kSampleRate = 32000;
    static constexpr uint32_t kBufferSize = 1024;
    static constexpr uint32_t kBlockSize = PCM5102::kBufferSize / 2;

    static void Init();
    static void Start();
    static void Stop();
    static void DeInit();

    static std::span<StereoSample> GetNextBlock();
};

}
