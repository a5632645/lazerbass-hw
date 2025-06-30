#pragma once
#include <cstdint>
#include <concepts>

namespace bsp {

class DebugIO {
public:
    // ---------------------------------------- Init ----------------------------------------
    DebugIO& Init();

    // ---------------------------------------- Dump ----------------------------------------
    DebugIO& PrintStackAndRegisters();

    // ---------------------------------------- Print ----------------------------------------
    DebugIO& Print(char c);
    DebugIO& Print(const char* str);
    DebugIO& Print(uint32_t num);
    DebugIO& Print(int32_t num);
    DebugIO& Hex(uint32_t num);
    DebugIO& hex(uint32_t num);
    DebugIO& Bin(uint32_t num);
    DebugIO& NewLine();

    DebugIO& Print(const uint8_t* buf, uint32_t len);
    
    template<std::integral T>
    DebugIO& Print(T num) { return Print(static_cast<uint32_t>(num)); }

    template<std::integral T>
    DebugIO& Hex(T num) { return Hex(static_cast<uint32_t>(num)); }

    template<std::integral T>
    DebugIO& hex(T num) { return hex(static_cast<uint32_t>(num)); }

    template<std::integral T>
    DebugIO& Bin(T num) { return Bin(static_cast<uint32_t>(num)); }

    // ---------------------------------------- Time ----------------------------------------
    uint32_t GetCpuSpeed();
    DebugIO& Wait(uint32_t ms);
    DebugIO& Wait(uint32_t ms, uint32_t cpuSpeed);
    DebugIO& HalWait(uint32_t ms);

    // ---------------------------------------- Debug ----------------------------------------
    [[noreturn]] void FlashErrorLight();
    [[noreturn]] static void FlashGreenLight();
    void SetLed(bool r, bool g, bool b);


    // --------------------------------------------------------------------------------
    // RTOS IO
    // --------------------------------------------------------------------------------
    static void StartRTOSIO();
    static void Write(const char* str, ...);
    static void Write(const uint8_t* buf, uint32_t len, bool newLine);
};

}