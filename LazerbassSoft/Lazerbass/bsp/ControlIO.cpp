#include "ControlIO.hpp"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_gpio.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"

#include "SystemHook.hpp"
#include "mcu/Memory.hpp"
#include "bsp/DebugIO.hpp"

namespace bsp {

static constexpr auto kButtonSamplePin = GPIO_PIN_13;    // B13
static constexpr auto kButtonChipSelectPin = GPIO_PIN_1; // E1
static constexpr auto kLedUpdatePin = GPIO_PIN_11;       // B11

static constexpr auto kNumLeds = 8 * 6;
static constexpr auto kNumButtons = 8 * 9;
static constexpr auto kBufferSize = std::max(kNumLeds, kNumButtons) / 8;

static SPI_HandleTypeDef hspi_;
static DMA_HandleTypeDef hdmaTx_;
static DMA_HandleTypeDef hdmaRx_;

_DMA_SRAMD1 static uint8_t leds_[kBufferSize];
_DMA_SRAMD1 static uint8_t buttons_[kBufferSize];
static uint8_t lastButtons_[kBufferSize];
static int32_t encoderValues_[4];

static TimerHandle_t spiTimerHandle_ = NULL;
static StaticTimer_t spiTimer_;
static StaticSemaphore_t bspHandlerSem_;
static SemaphoreHandle_t bspHandlerSemHandle_ = NULL;

static void OS_SpiTimerCallback(TimerHandle_t timer);
static void HAL_SpiHandler(SPI_HandleTypeDef *hspi);

static ControlIO::ButtonEvent buttonEvents_[kNumButtons];
static uint32_t numButtonEvents_ = 0;

// --------------------------------------------------------------------------------
// Bsp Handler
// --------------------------------------------------------------------------------
void ControlIO::WaitForNextEventBlock() {
    xSemaphoreTake(bspHandlerSemHandle_, portMAX_DELAY);
    numButtonEvents_ = 0;

    // always clear 43 to 46
    // idx is 5, mask is 0b11100001
    buttons_[5] &= 0b11100001;
    lastButtons_[5] &= 0b11100001;
    for (int i = 0; i < kBufferSize; ++i) {
        uint8_t changes = buttons_[i] ^ lastButtons_[i];
        while (changes) {
            auto idx = __builtin_ffs(changes);
            auto id = ControlIO::ButtonId(i * 8 + idx - 1);
            auto mask = 1 << (idx - 1);
            auto state = buttons_[i] & mask ? ControlIO::ButtonState::kAttack : ControlIO::ButtonState::kRelease;
            buttonEvents_[numButtonEvents_++] = ControlIO::ButtonEvent{state, id};
            changes &= ~mask;
        }
    }

    xTimerStartFromISR(spiTimerHandle_, nullptr);
}

std::span<ControlIO::ButtonEvent> ControlIO::GetBtnEvents() {
    return std::span(buttonEvents_, numButtonEvents_);
}

std::span<int32_t, 4> ControlIO::GetEncoderValues() {
    return std::span(encoderValues_);
}

// --------------------------------------------------------------------------------
// Spi IRQ handler
// --------------------------------------------------------------------------------
static void OS_SpiTimerCallback(TimerHandle_t) {
    std::copy(buttons_, buttons_ + kBufferSize, lastButtons_);
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_RESET); // ↓ sample
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_RESET); // ↓ sample
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_RESET); // ↓ sample
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_RESET); // ↓ sample
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, kButtonSamplePin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOB, kLedUpdatePin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, kButtonChipSelectPin, GPIO_PIN_RESET); // chip select

    hspi_.TxRxCpltCallback = HAL_SpiHandler;
    auto res = HAL_SPI_TransmitReceive_DMA(&hspi_, leds_, buttons_, kBufferSize);
    if (res != HAL_OK) {
        DEVICE_ERROR_CODE("ControlIO", "HAL_SPI_TransmitReceive_DMA", res);
    }
}

extern "C" void SPI2_IRQHandler(void) {
    HAL_SPI_IRQHandler(&hspi_);
}

static void HAL_SpiHandler(SPI_HandleTypeDef*) {
    HAL_GPIO_WritePin(GPIOE, kButtonChipSelectPin, GPIO_PIN_SET); // disable chip select
    HAL_GPIO_WritePin(GPIOB, kLedUpdatePin, GPIO_PIN_SET); // ↑ update
    xSemaphoreGiveFromISR(bspHandlerSemHandle_, nullptr);
}

extern "C" void DMA1_Stream1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdmaTx_);
}

extern "C" void DMA1_Stream2_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdmaRx_);
}

void ControlIO::Init() {
    SpiInit();
    EncoderInit();
    
    // start spi timer
    OS_SpiTimerCallback(nullptr);

    bspHandlerSemHandle_ = xSemaphoreCreateBinaryStatic(&bspHandlerSem_);
    if (bspHandlerSemHandle_ == nullptr) {
        DEVICE_ERROR("ControlIO", "xSemaphoreCreateBinaryStatic");
    }

    // clear values
    std::fill_n(buttons_, kBufferSize, 0);
    std::fill_n(lastButtons_, kBufferSize, 0);
}

void ControlIO::SpiInit() {
    // spi peripheral init
    __HAL_RCC_SPI2_CLK_ENABLE();
    hspi_.Instance = SPI2;
    hspi_.Init.Mode = SPI_MODE_MASTER;
    hspi_.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_.Init.NSS = SPI_NSS_SOFT;
    hspi_.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi_.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi_.Init.CRCPolynomial = 7;
    hspi_.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi_.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi_.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi_.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    HAL_SPI_Init(&hspi_);
    
    // spi gpio init
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitTypeDef gpioInit{};
    gpioInit.Pin = GPIO_PIN_10 | GPIO_PIN_15 | GPIO_PIN_14; // CLK, MOSI, MISO
    gpioInit.Mode = GPIO_MODE_AF_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    gpioInit.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpioInit);

    gpioInit.Pin = kButtonSamplePin | kLedUpdatePin;
    gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpioInit);

    gpioInit.Pin = kButtonChipSelectPin;
    gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOE, &gpioInit);

    // spi dma init
    __HAL_RCC_DMA1_CLK_ENABLE();
    hdmaTx_.Instance = DMA1_Stream1;
    HAL_DMA_DeInit(&hdmaTx_);
    hdmaTx_.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdmaTx_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdmaTx_.Init.MemBurst = DMA_MBURST_SINGLE;
    hdmaTx_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdmaTx_.Init.MemInc = DMA_MINC_ENABLE;
    hdmaTx_.Init.Mode = DMA_NORMAL;
    hdmaTx_.Init.PeriphBurst = DMA_PBURST_SINGLE;
    hdmaTx_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdmaTx_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdmaTx_.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdmaTx_.Init.Request = DMA_REQUEST_SPI2_TX;
    if (auto res = HAL_DMA_Init(&hdmaTx_);
        res != HAL_OK) {
        DEVICE_ERROR_CODE("ControlIO", "SPI DMA TX init", res);
    }

    hdmaRx_.Instance = DMA1_Stream2;
    HAL_DMA_DeInit(&hdmaRx_);
    hdmaRx_.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdmaRx_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdmaRx_.Init.MemBurst = DMA_MBURST_SINGLE;
    hdmaRx_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdmaRx_.Init.MemInc = DMA_MINC_ENABLE;
    hdmaRx_.Init.Mode = DMA_NORMAL;
    hdmaRx_.Init.PeriphBurst = DMA_PBURST_SINGLE;
    hdmaRx_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdmaRx_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdmaRx_.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdmaRx_.Init.Request = DMA_REQUEST_SPI2_RX;
    if (auto res = HAL_DMA_Init(&hdmaRx_);
        res != HAL_OK) {
        DEVICE_ERROR_CODE("ControlIO", "SPI DMA RX init", res);
    }

    hdmaTx_.Parent = &hspi_;
    hdmaRx_.Parent = &hspi_;
    hspi_.hdmarx = &hdmaRx_;
    hspi_.hdmatx = &hdmaTx_;

    // nvic init
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
    HAL_NVIC_SetPriority(SPI2_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 10, 0);
    
    // spi os init
    spiTimerHandle_ = xTimerCreateStatic("spiTimer", pdMS_TO_TICKS(50), pdFALSE, nullptr, OS_SpiTimerCallback, &spiTimer_);
    if (spiTimerHandle_ == nullptr) {
        DEVICE_ERROR("ControlIO", "SPI timer init");
    }
}

void ControlIO::SetLed(LedId led, bool on) {
    SetLed(static_cast<uint32_t>(led), on);
}

void ControlIO::SetLed(uint32_t led, bool on) {
    if (on) {
        leds_[kBufferSize - led / 8 - 1] &= ~(1 << (led % 8));
    }
    else {
        leds_[kBufferSize - led / 8 - 1] |= 1 << (led % 8);
    }
}

void ControlIO::SetAllLeds(bool on) {
    if (on) {
        std::fill_n(leds_, kBufferSize, 0);
    }
    else {
        std::fill_n(leds_, kBufferSize, 0xff);
    }
}

void ControlIO::TestShift() {
    static uint8_t shift = 0;
    SetAllLeds(false);
    SetLed(shift, true);
    shift = (shift + 1) % kNumLeds;
}

bool ControlIO::IsButtonDown(ButtonId id) {
    auto idx = static_cast<uint32_t>(id);
    return buttons_[idx / 8] & (1 << (idx % 8));
}

// --------------------------------------------------------------------------------
// Encoder
//
// 正转          反转
// A0 --____--    A0 --____--
// A1 ----____    A1 ____----
// --------------------------------------------------------------------------------
/* GPIOB */
#define ENCODER_1_A0 GPIO_PIN_7
#define ENCODER_1_A1 GPIO_PIN_6
#define ENCODER_4_A0 GPIO_PIN_9
#define ENCODER_4_A1 GPIO_PIN_8

/* GPIOE */
#define ENCODER_2_A0 GPIO_PIN_9
#define ENCODER_2_A1 GPIO_PIN_11
#define ENCODER_3_A0 GPIO_PIN_13
#define ENCODER_3_A1 GPIO_PIN_14

#define DEBOUNCE_TIME_MS 50

void ControlIO::EncoderInit() {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef init{};
    init.Mode = GPIO_MODE_INPUT;
    init.Pin = ENCODER_1_A0 | ENCODER_4_A0;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &init);

    init.Pin = ENCODER_2_A0 | ENCODER_3_A0;
    HAL_GPIO_Init(GPIOE, &init);

    init.Pin = ENCODER_1_A1 | ENCODER_4_A1;
    init.Mode = GPIO_MODE_IT_RISING_FALLING;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &init);

    init.Pin = ENCODER_2_A1 | ENCODER_3_A1;
    HAL_GPIO_Init(GPIOE, &init);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static struct {
    bool flag1 : 1;
    bool init_nu1 : 1;
    bool flag2 : 1;
    bool init_nu2 : 1;
    bool flag3 : 1;
    bool init_nu3 : 1;
    bool flag4 : 1;
    bool init_nu4 : 1;
} encoderIrqFlags{};

extern "C" void EXTI9_5_IRQHandler(void) {
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6) == SET) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);

        auto gpiob = LL_GPIO_ReadInputPort(GPIOB);
        auto keya = gpiob & ENCODER_1_A1;
        auto keyb = gpiob & ENCODER_1_A0;

        if (!encoderIrqFlags.init_nu1 && !keya) {
            encoderIrqFlags.flag1 = false;
            if (keyb) {
                encoderIrqFlags.flag1 = true;
            }
            encoderIrqFlags.init_nu1 = true;
        }

        if (encoderIrqFlags.init_nu1 && keya) {
            if (!keyb && encoderIrqFlags.flag1) {
                encoderValues_[3]++;
            }
            if (keyb && !encoderIrqFlags.flag1) {
                encoderValues_[3]--;
            }
            encoderIrqFlags.init_nu1 = false;
        }
    }

    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_8) == SET) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_8);

        auto gpiob = LL_GPIO_ReadInputPort(GPIOB);
        auto keya = gpiob & ENCODER_4_A1;
        auto keyb = gpiob & ENCODER_4_A0;

        if (!encoderIrqFlags.init_nu4 && !keya) {
            encoderIrqFlags.flag4 = false;
            if (keyb) {
                encoderIrqFlags.flag4 = true;
            }
            encoderIrqFlags.init_nu4 = true;
        }

        if (encoderIrqFlags.init_nu4 && keya) {
            if (!keyb && encoderIrqFlags.flag4) {
                encoderValues_[2]++;
            }
            if (keyb && !encoderIrqFlags.flag4) {
                encoderValues_[2]--;
            }
            encoderIrqFlags.init_nu4 = false;
        }
    }
}

extern "C" void EXTI15_10_IRQHandler(void) {
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_11) == SET) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);

        auto gpioe = LL_GPIO_ReadInputPort(GPIOE);
        auto keya = gpioe & ENCODER_2_A1;
        auto keyb = gpioe & ENCODER_2_A0;

        if (!encoderIrqFlags.init_nu2 && !keya) {
            encoderIrqFlags.flag2 = false;
            if (keyb) {
                encoderIrqFlags.flag2 = true;
            }
            encoderIrqFlags.init_nu2 = true;
        }

        if (encoderIrqFlags.init_nu2 && keya) {
            if (!keyb && encoderIrqFlags.flag2) {
                encoderValues_[0]++;
            }
            if (keyb && !encoderIrqFlags.flag2) {
                encoderValues_[0]--;
            }
            encoderIrqFlags.init_nu2 = false;
        }
    }

    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_14) == SET) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_14);

        auto gpioe = LL_GPIO_ReadInputPort(GPIOE);
        auto keya = gpioe & ENCODER_3_A1;
        auto keyb = gpioe & ENCODER_3_A0;

        if (!encoderIrqFlags.init_nu3 && !keya) {
            encoderIrqFlags.flag3 = false;
            if (keyb) {
                encoderIrqFlags.flag3 = true;
            }
            encoderIrqFlags.init_nu3 = true;
        }

        if (encoderIrqFlags.init_nu3 && keya) {
            if (!keyb && encoderIrqFlags.flag3) {
                encoderValues_[1]++;
            }
            if (keyb && !encoderIrqFlags.flag3) {
                encoderValues_[1]--;
            }
            encoderIrqFlags.init_nu3 = false;
        }
    }
}

}
