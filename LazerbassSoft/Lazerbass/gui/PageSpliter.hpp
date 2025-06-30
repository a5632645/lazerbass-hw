#pragma once
#include <array>
#include "GuiDispatch.hpp"

namespace gui {

struct PageObj {
    void(*draw)(OLEDDisplay& display, Rectange& rect) = nullptr;
    void(*btnEvent)(bsp::ControlIO::ButtonEvent e) = nullptr;
    void(*encoderEvent)(bsp::ControlIO::EncoderId id, int32_t dvalue) = nullptr;
};

template<size_t N>
class PageSpliter {
public:
    static constexpr int32_t kMaxIndex = N - 1;

    PageSpliter(std::array<PageObj, N>&& objs) : objs_(std::move(objs)) {}

    void PrevPage() {
        if (page_ > 0) {
            --page_;
        }
    }

    void NextPage() {
        if (page_ < kMaxIndex) {
            ++page_;
        }
    }

    void Draw(OLEDDisplay& display, Rectange resetRect) {
        objs_[page_].draw(display, resetRect);
    }

    void BtnEvent(bsp::ControlIO::ButtonEvent e) {
        objs_[page_].btnEvent(e);
    }

    void EncoderEvent(bsp::ControlIO::EncoderId id, int32_t dvalue) {
        objs_[page_].encoderEvent(id, dvalue);
    }

    PageObj& GetPageObj() { return objs_[page_]; }
    void SetPageObj(PageObj& obj) { objs_[page_] = obj; }

    int32_t GetPageIndex() const { return page_; }
private:
    std::array<PageObj, N> objs_;
    int32_t page_{};
};

}