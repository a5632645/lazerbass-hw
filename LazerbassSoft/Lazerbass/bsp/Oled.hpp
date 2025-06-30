#pragma once
#include <cstdint>
#include "oled/OLEDDisplay.h"

namespace bsp {

class Oled {
public:
    static void Init();
    static void SendFrame();
    static OLEDDisplay& GetDisplay();
};

}
