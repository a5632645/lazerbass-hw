#pragma once
#include "gui/GuiDispatch.hpp"

namespace gui::styles {

void DrawOnOffButton(OLEDDisplay& display, Rectange aera, bool on, std::string_view onText, std::string_view offText);

void DrawButton(OLEDDisplay& display, Rectange aera, bool on, std::string_view text);

void DrawSlider(OLEDDisplay& display, Rectange aera, int16_t width, std::string_view text);

template<class... Args> USF_CPP14_CONSTEXPR
static void DrawFormatSlider(OLEDDisplay& display, Rectange aera, int16_t width, usf::StringView text, Args&&... args) {
    auto span = display.FormantToInternalBuffer(text, std::forward<Args>(args)...);
    DrawSlider(display, aera, width, std::string_view(span.data(), span.size()));
}

void DrawTitleBar(OLEDDisplay& display, Rectange aera, std::string_view text);

}
