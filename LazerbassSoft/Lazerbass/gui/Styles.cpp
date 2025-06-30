#include "Styles.hpp"

namespace gui::styles {

void DrawOnOffButton(OLEDDisplay& display, Rectange aera, bool on, std::string_view onText, std::string_view offText) {
    if (on) {
        display.setColor(kOledWHITE);
        display.fillRect(aera.x, aera.y, aera.w, aera.h);
        display.setColor(kOledBLACK);
        aera.Reduce(2,0);
        display.drawString(aera.x, aera.y, onText);
    }
    else {
        display.setColor(kOledBLACK);
        display.fillRect(aera.x, aera.y, aera.w, aera.h);
        display.setColor(kOledWHITE);
        display.drawRect(aera.x, aera.y, aera.w, aera.h);
        display.setColor(kOledWHITE);
        aera.Reduce(2,0);
        display.drawString(aera.x, aera.y, offText);
    }
}

void DrawButton(OLEDDisplay& display, Rectange aera, bool on, std::string_view text) {
    DrawOnOffButton(display, aera, on, text, text);
}

void DrawSlider(OLEDDisplay& display, Rectange aera, int16_t width, std::string_view text) {
    auto box = aera;
    box.Reduce(1, 1);
    display.setColor(kOledWHITE);
    display.drawString(box.x, box.y, text);

    display.setColor(kOledINVERSE);
    display.fillRect(aera.x, aera.y, width, aera.h);

    display.setColor(kOledWHITE);
    display.drawRect(aera.x, aera.y, aera.w, aera.h);
}


void DrawTitleBar(OLEDDisplay& display, Rectange aera, std::string_view text) {
    display.setColor(kOledWHITE);
    display.fillRect(aera.x, aera.y, aera.w, aera.h);
    display.setColor(kOledBLACK);
    display.drawString(aera.x, aera.y, text);
}

}
