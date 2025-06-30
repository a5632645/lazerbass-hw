#pragma once
#include <span>
#include <cstdint>

namespace bsp {

struct MidiEvent {
    uint8_t codeIndexNumber : 4;
    uint8_t cableNumber : 4;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;

    bool IsNoteOn() const { return codeIndexNumber == 9; }
    bool IsNoteOff() const { return codeIndexNumber == 8; }
    uint32_t GetNote() const { return data2; }
    uint32_t GetChannel() const { return data1 & 0xf; }
    uint32_t GetVelocity() const { return data3; }

    bool IsPitchBend() const { return codeIndexNumber == 0xe; }
    uint32_t GetPitchBend() const { return (data2 & 0x7f) + ((data1 & 0x7f) << 7); }
};

class USBMidi {
public:
    static void Init();
    static void WaitForNextBlock();
    static MidiEvent* GetNextEvent();
};

}