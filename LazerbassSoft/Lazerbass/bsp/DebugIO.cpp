#include "DebugIO.hpp"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_dma.h"

#include <algorithm>
#include <stdarg.h>
#include <stdio.h>

#include "mcu/Memory.hpp"
#include "SystemHook.hpp"

#include "FreeRTOS.h"
#include "semphr.h"

#define RED_Pin GPIO_PIN_0
#define RED_GPIO_Port GPIOC
#define GREEN_Pin GPIO_PIN_1
#define GREEN_GPIO_Port GPIOC
#define BLUE_Pin GPIO_PIN_2
#define BLUE_GPIO_Port GPIOC

static void LED_R(int state) { HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, static_cast<GPIO_PinState>(state)); }
static void LED_G(int state) { HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, static_cast<GPIO_PinState>(state)); }
static void LED_B(int state) { HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, static_cast<GPIO_PinState>(state)); }

static UART_HandleTypeDef huart_;
static DMA_HandleTypeDef hdma;

_DMA_SRAMD1 static uint8_t sendBuffer[256];
static SemaphoreHandle_t dmaSemHandle_ = NULL;
static StaticSemaphore_t dmaSem_;

namespace bsp {

// --------------------------------------------------------------------------------
// RTOS IO
// --------------------------------------------------------------------------------
void DebugIO::StartRTOSIO() {
    /* 初始化DMA */
    {
        __HAL_RCC_DMA1_CLK_ENABLE();
        hdma.Instance = DMA1_Stream0;
        hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        hdma.Init.MemBurst = DMA_MBURST_SINGLE;
        hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma.Init.MemInc = DMA_MINC_ENABLE;
        hdma.Init.Mode = DMA_NORMAL;
        hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
        hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        hdma.Init.Request = DMA_REQUEST_USART2_TX;
        if (HAL_DMA_Init(&hdma) != HAL_OK) {
            DEVICE_ERROR("DebugIO", "HAL_DMA_Init failed");
        }
    }

    /* 链接DMA */
    {
        hdma.Parent = &huart_;
        huart_.hdmatx = &hdma;
    }

    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_NVIC_SetPriority(USART2_IRQn, 10, 0);

    dmaSemHandle_ = xSemaphoreCreateBinaryStatic(&dmaSem_);
    xSemaphoreGive(dmaSemHandle_);
}

void DebugIO::Write(const char* str, ...) {
    xSemaphoreTake(dmaSemHandle_, portMAX_DELAY);
    
    va_list args;
    va_start(args, str);
    auto len = vsnprintf((char*)sendBuffer, sizeof(sendBuffer), str, args);
    va_end(args);

    if (auto res = HAL_UART_Transmit_DMA(&huart_, sendBuffer, len);
        res != HAL_OK) {
        DEVICE_ERROR_CODE("DebugIO", "HAL_UART_Transmit_DMA failed", res);
    }
}

void DebugIO::Write(const uint8_t* buf, uint32_t len, bool newLine) {
    xSemaphoreTake(dmaSemHandle_, portMAX_DELAY);

    auto it = std::copy(buf, buf + len, sendBuffer);
    if (newLine) {
        *it++ = '\n';
        *it++ = '\r';
        len += 2;
    }

    if (auto res = HAL_UART_Transmit_DMA(&huart_, sendBuffer, len);
        res != HAL_OK) {
        DEVICE_ERROR_CODE("DebugIO", "HAL_UART_Transmit_DMA failed", res);
    }
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef*) {
    xSemaphoreGiveFromISR(dmaSemHandle_, nullptr);
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef*) {
    DEVICE_ERROR_CODE("DebugIO", "HAL_UART_ErrorCallback", HAL_UART_GetError(&huart_));
}

extern "C" void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma);
}

extern "C" void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart_);
}

// --------------------------------------------------------------------------------
// API
// --------------------------------------------------------------------------------
[[noreturn]]
void DebugIO::FlashErrorLight() {
    for (;;) {
        HAL_GPIO_TogglePin(RED_GPIO_Port, RED_Pin);
        Wait(1000, 240000000); // 假设系统时钟为240MHz
    }
}

void DebugIO::FlashGreenLight() {
    bsp::DebugIO i;
    for (;;) {
        HAL_GPIO_TogglePin(GREEN_GPIO_Port, GREEN_Pin);
        i.Wait(1000, 240000000); // 假设系统时钟为240MHz
    }
}

void DebugIO::SetLed(bool r, bool g, bool b) {
    LED_R(!r);
    LED_G(!g);
    LED_B(!b);
}

DebugIO& DebugIO::PrintStackAndRegisters() {
    uint32_t* sp = 0;
    asm volatile ("mov %0, sp" : "=r" (sp));
    uint32_t** sp_end_addr = reinterpret_cast<uint32_t**>(SCB->VTOR);
    uint32_t* sp_end = *sp_end_addr;

    Print("Stack:").NewLine();
    for (; sp < sp_end; ++sp) {
        Hex(reinterpret_cast<uint32_t>(sp)).Print(':').Hex(*sp).NewLine();
    }
    Print("Registers:").NewLine();

    uint32_t val = 0;
    asm volatile ("mov %0, r0" : "=r" (val));
    Print("R0: ").Hex(val).NewLine();
    asm volatile ("mov %0, r1" : "=r" (val));
    Print("R1: ").Hex(val).NewLine();
    asm volatile ("mov %0, r2" : "=r" (val));
    Print("R2: ").Hex(val).NewLine();
    asm volatile ("mov %0, r3" : "=r" (val));
    Print("R3: ").Hex(val).NewLine();
    asm volatile ("mov %0, r4" : "=r" (val));
    Print("R4: ").Hex(val).NewLine();
    asm volatile ("mov %0, r5" : "=r" (val));
    Print("R5: ").Hex(val).NewLine();
    asm volatile ("mov %0, r6" : "=r" (val));
    Print("R6: ").Hex(val).NewLine();
    asm volatile ("mov %0, r7" : "=r" (val));
    Print("R7: ").Hex(val).NewLine();

    asm volatile ("mov %0, r8" : "=r" (val));
    Print("R8: ").Hex(val).NewLine();
    asm volatile ("mov %0, r9" : "=r" (val));
    Print("R9: ").Hex(val).NewLine();
    asm volatile ("mov %0, r10" : "=r" (val));
    Print("R10: ").Hex(val).NewLine();
    asm volatile ("mov %0, r11" : "=r" (val));
    Print("R11: ").Hex(val).NewLine();
    asm volatile ("mov %0, r12" : "=r" (val));
    Print("R12: ").Hex(val).NewLine();

    asm volatile ("mov %0, pc" : "=r" (val));
    Print("PC: ").Hex(val).NewLine();
    
    asm volatile ("mov %0, lr" : "=r" (val));
    Print("LR: ").Hex(val).NewLine();

    return Print("End of registers").NewLine();
}

DebugIO &DebugIO::Init()
{
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        huart_.Instance = USART2;
        huart_.Init.BaudRate = 115200;
        huart_.Init.WordLength = UART_WORDLENGTH_8B;
        huart_.Init.StopBits = UART_STOPBITS_1;
        huart_.Init.Parity = UART_PARITY_NONE;
        huart_.Init.Mode = UART_MODE_TX;
        huart_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        huart_.Init.OverSampling = UART_OVERSAMPLING_16;
        huart_.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
        huart_.Init.ClockPrescaler = UART_PRESCALER_DIV1;
        huart_.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
        HAL_UART_Init(&huart_);
    }
    {
        // TX -> PA2
        // RX -> PA3
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitTypeDef init{
            .Pin = GPIO_PIN_2,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_HIGH,
            .Alternate = GPIO_AF7_USART2
        };
        HAL_GPIO_Init(GPIOA, &init);

        // RX Disable
    }
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        GPIO_InitTypeDef init{
            .Pin = RED_Pin | GREEN_Pin | BLUE_Pin,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_HIGH
        };
        HAL_GPIO_Init(GPIOC, &init);
    }

    return *this;
}

DebugIO& DebugIO::Print(char c) {
    huart_.Instance->TDR = c;
    while (__HAL_UART_GET_FLAG(&huart_, UART_FLAG_TC) == RESET) {}
    return *this;
}

DebugIO &DebugIO::Print(const char *str) {
    while (*str) {
        Print(*str++);
    }
    return *this;
}

DebugIO &DebugIO::Print(uint32_t num) {
    char buff[32]{};
    char* end = buff + 31;

    do {
        *--end = (num % 10) + '0';
        num /= 10;
    } while (num);
    return Print(end);
}

DebugIO &DebugIO::Print(int32_t num) {
    char buff[32]{};
    char* end = buff + 31;

    int32_t t = num;
    if (t < 0) {
        t = -t;
    }

    do {
        *--end = (t % 10) + '0';
        t /= 10;
    } while (t);
    if (num < 0) {
        *--end = '-';
    }
    return Print(end);
}

DebugIO &DebugIO::Hex(uint32_t num) {
    char buff[32]{};
    char* end = buff + 31;

    do {
        *--end = (num % 16) < 10 ? (num % 16) + '0' : (num % 16) - 10 + 'A';
        num /= 16;
    } while (num);
    *--end = 'x';
    *--end = '0';

    return Print(end);
}

DebugIO& DebugIO::hex(uint32_t t) {
    char buff[32]{};
    char* end = buff + 31;
    
    do {
        *--end = (t % 16) < 10 ? (t % 16) + '0' : (t % 16) - 10 + 'a';
        t /= 16;
    } while (t);
    *--end = 'x';
    *--end = '0';
    
    return Print(end);
}

DebugIO &DebugIO::Bin(uint32_t num) {
    char buffer[33]{};
    for (int i = 31; i >= 0; --i) {
        buffer[i] = (num & (1 << i)) ? '1' : '0';
    }
    buffer[32] = '\0';
    return Print(buffer);
}

DebugIO& DebugIO::NewLine() {
    Print("\n\r");
    return *this;
}

DebugIO &DebugIO::Print(const uint8_t* buf, uint32_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        Print(buf[i]);
    }
    return *this;
}

extern uint32_t SystemCoreClock;
uint32_t DebugIO::GetCpuSpeed() {
    return SystemCoreClock;
}

DebugIO &DebugIO::Wait(uint32_t ms) {
    return Wait(ms, GetCpuSpeed());
}

DebugIO& DebugIO::Wait(uint32_t ms, uint32_t cpuSpeed) {
    volatile uint32_t ticks = ms * cpuSpeed / 1000;
    while(ticks) {
        ticks = ticks - 1;
    }
    return *this;
}

DebugIO &DebugIO::HalWait(uint32_t ms) {
    HAL_Delay(ms);
    return *this;
}

}
