#include "Oled.hpp"
#include "stm32h7xx_hal.h"

#include "bsp/DebugIO.hpp"
#include "mcu/Memory.hpp"
#include "SystemHook.hpp"

#include <algorithm>

#include "FreeRTOS.h"
#include "task.h"

namespace bsp {

enum
{
    kOledCommandAdress = 0x78,
    kOledDataAdress = 0x79,

    kOledCmd_cmd = 0x80,
    kOledCmd_data = 0x40,

    kOledCmd_DisplayOn = 0xaf,
    kOledCmd_DisplayOff = 0xae,
    kOledCmd_DisplayFollowRam = 0xa4,
    kOledCmd_DisplayIgnoreRam = 0xa5,
    kOledCmd_DisplayNormal = 0xa6,
    kOledCmd_DisplayInverse = 0xa7,
    kOledCmd_Contrast = 0x81, /* this,01h~ffh */

    kOledCmd_ChargePumpSetting = 0x8d,
    kOledCmd_ChargePump_on = 0x14,
    kOledCmd_ChargePump_off = 0x10,

    kOledCmd_AddressingMode = 0x20,
    kOledCmd_AddressingModeHorizontal = 0b00,
    kOledCmd_AddressingModeVertical = 0b01,
    kOledCmd_AddressingModePage = 0b10,

    kOledCmd_HVModeSetColumnAddress = 0x21, /* this,start,end */
    kOledCmd_HVModeSetPageAddress = 0x22,   /* this,start,end */

    kOledCmd_AddressingMode_page = 0x02,
    kOledCmd_MultiRatio = 0xa8, /* this,[5:0] */
    kOledCmd_OscFreq = 0xd5,    /* [7:4]frequ, [3:0]divide */
    kOledCmd_xToRight = 0xa0,
    kOledCmd_rightToLeft = 0xa1,
    kOledCmd_ComScan_normal = 0xc0,
    kOledCmd_ComScan_inverse = 0xc8,
    kOledCmd_ComHarwareConfig = 0xda,
    kOledCmd_DisplayOffset = 0xd3, /* [5:0] */
    kOledCmd_PreChargePeriod = 0xd9,
    kOledCmd_VcomhDeselectLevel = 0xdb,
    kOledCmd_Nop = 0xe3
};

#define OLED_PAGE_MODE_LOW_COL(X) ((X) & 0x0f)
#define OLED_PAGE_MODE_HIGH_COL(X) (((X) & 0x7) | 0x10)
#define OLED_DISPLAY_START_LINE(X) (((X) & 0x3f) | 0x40)
#define OLED_PAGE_MODE_PAGE_START(X) (((X) & 0b111) | 0b10110000)

#define OLED_CMD(CMD) kOledCmd_cmd, CMD
#define OLED_CMDWP(CMD, PARAM) OLED_CMD(CMD), OLED_CMD(PARAM)
#define OLED_ADDRESS 0x78

static uint8_t init_cmds[] = {
    OLED_CMD(kOledCmd_DisplayOff),
    OLED_CMDWP(kOledCmd_MultiRatio, 0x3f),
    OLED_CMDWP(kOledCmd_OscFreq, 0xf0),
    OLED_CMD(kOledCmd_rightToLeft),
    OLED_CMD(kOledCmd_ComScan_inverse),
    OLED_CMDWP(kOledCmd_ComHarwareConfig, 0x10),
    OLED_CMDWP(kOledCmd_AddressingMode, kOledCmd_AddressingModeHorizontal),
    OLED_CMD(kOledCmd_HVModeSetColumnAddress), OLED_CMD(0), OLED_CMD(127),
    OLED_CMD(kOledCmd_HVModeSetPageAddress), OLED_CMD(0), OLED_CMD(7),
    OLED_CMD(OLED_DISPLAY_START_LINE(0)),
    OLED_CMDWP(kOledCmd_DisplayOffset, 0x00),
    OLED_CMDWP(kOledCmd_Contrast, 0x7f),
    OLED_CMD(kOledCmd_DisplayFollowRam),
    OLED_CMD(kOledCmd_DisplayNormal),
    OLED_CMDWP(kOledCmd_PreChargePeriod, 0x22),
    OLED_CMDWP(kOledCmd_VcomhDeselectLevel, 0x20),
    OLED_CMDWP(kOledCmd_ChargePumpSetting, kOledCmd_ChargePump_on),
    OLED_CMD(kOledCmd_DisplayOn)
};

static I2C_HandleTypeDef hi2c_;
static DMA_HandleTypeDef hdma_;

_DMA_SRAMD3 static uint8_t dmaBuffer_[OLEDDisplay::kTransferBufferSize];
static OLEDDisplay display_;

// --------------------------------------------------------------------------------
// IRQ
// --------------------------------------------------------------------------------
extern "C" void BDMA_Channel0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_);
}

extern "C" void I2C4_EV_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&hi2c_);
}

extern "C" void I2C4_ER_IRQHandler(void) {
    HAL_I2C_ER_IRQHandler(&hi2c_);
}

extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    DEVICE_ERROR_CODE("Oled", "HAL_I2C_ErrorCallback", hi2c->ErrorCode);
}

// --------------------------------------------------------------------------------
// oled interface
// --------------------------------------------------------------------------------
void Oled::Init()
{
    // i2c init
    __HAL_RCC_I2C4_CLK_ENABLE();
    hi2c_.Instance = I2C4;
    hi2c_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
    hi2c_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
    hi2c_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;
    hi2c_.Init.OwnAddress1 = 0;
    hi2c_.Init.OwnAddress2 = 0;
    hi2c_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c_.Init.Timing = 400000;
    HAL_I2C_Init(&hi2c_);

    // i2c gpio init
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef init{};
    init.Mode = GPIO_MODE_AF_OD;
    init.Pull = GPIO_PULLUP;
    init.Alternate = GPIO_AF4_I2C4;
    init.Pin = GPIO_PIN_13 | GPIO_PIN_12;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &init);

    // dma init
    __HAL_RCC_BDMA_CLK_ENABLE();
    hdma_.Instance = BDMA_Channel0;
    hdma_.Init.Request = BDMA_REQUEST_I2C4_TX;
    hdma_.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_.Init.MemInc = DMA_MINC_ENABLE;
    hdma_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_.Init.Mode = DMA_NORMAL;
    hdma_.Init.Priority = DMA_PRIORITY_LOW;
    hdma_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (auto res = HAL_DMA_Init(&hdma_);
        res != HAL_OK) {
        DEVICE_ERROR_CODE("Oled", "HAL_DMA_Init failed", res);
    }

    hi2c_.hdmatx = &hdma_;
    hdma_.Parent = &hi2c_;

    // nvic init
    HAL_NVIC_EnableIRQ(I2C4_EV_IRQn);
    HAL_NVIC_SetPriority(I2C4_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C4_ER_IRQn);
    HAL_NVIC_SetPriority(I2C4_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(BDMA_Channel0_IRQn);
    HAL_NVIC_SetPriority(BDMA_Channel0_IRQn, 0, 0);

    // power sequence
    uint8_t trys = 32;
    while (trys--) {
        if (auto res = HAL_I2C_Master_Transmit(&hi2c_, OLED_ADDRESS, init_cmds, sizeof(init_cmds), 1000);
            res == HAL_OK) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (trys == 0) {
        DEVICE_ERROR_CODE("Oled", "Power sequence failed", hi2c_.ErrorCode);
    }
}

void Oled::SendFrame() {
    if (hi2c_.State != HAL_I2C_STATE_READY) {
        return;
    }

    std::copy_n(display_.getTransferBuffer(), OLEDDisplay::kTransferBufferSize, dmaBuffer_);
    if (auto res = HAL_I2C_Master_Transmit_DMA(&hi2c_, OLED_ADDRESS, dmaBuffer_, OLEDDisplay::kTransferBufferSize);
        res != HAL_OK) {
        DEVICE_ERROR_CODE("Oled", "HAL_I2C_Master_Transmit_DMA failed", res);
    }
}

OLEDDisplay& Oled::GetDisplay() {
    return display_;
}

}
