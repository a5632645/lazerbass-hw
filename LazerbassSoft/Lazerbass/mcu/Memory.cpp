#include <cstdint>
#include "Memory.hpp"
#include "stm32h7xx_hal.h"

extern "C" {

extern uint32_t _specify_sramd1_bss_start;
extern uint32_t _specify_sramd1_bss_end;

extern uint32_t _specify_sramd2_bss_start;
extern uint32_t _specify_sramd2_bss_end;

extern uint32_t _specify_sramd3_bss_start;
extern uint32_t _specify_sramd3_bss_end;

extern uint32_t _specify_itcmram_ram_start;
extern uint32_t _specify_itcmram_ram_end;
extern uint32_t _specify_itcmram_flash_start;

}

void MCUMemory::SRAM_D1_Init(void) {
    auto* pstart = &_specify_sramd1_bss_start;
    auto* pend = &_specify_sramd1_bss_end;
    for (auto* p = pstart; p < pend; ++p) *p = 0;
}

void MCUMemory::_SramD2_Init(void) {
    auto* pstart = &_specify_sramd2_bss_start;
    auto* pend = &_specify_sramd2_bss_end;
    for (auto* p = pstart; p < pend; ++p) *p = 0;
}

void MCUMemory::_SramD3_Init(void) {
    auto* pstart = &_specify_sramd3_bss_start;
    auto* pend = &_specify_sramd3_bss_end;
    for (auto* p = pstart; p < pend; ++p) *p = 0;
}

void MCUMemory::_ItcmRam_Init(void) {
    auto* pram_start = &_specify_itcmram_ram_start;
    auto* pram_end = &_specify_itcmram_ram_end;
    auto* pflash_start = &_specify_itcmram_flash_start;
    for (auto* p = pram_start; p < pram_end; ++p) {
        *p = *pflash_start;
        ++pflash_start;
    }
}

void MCUMemory::DMA_MPU_Init(void) {
    HAL_MPU_Disable();

    MPU_Region_InitTypeDef dmaRegion;
    dmaRegion.Enable = MPU_REGION_ENABLE;
    dmaRegion.BaseAddress = 0x24000000; // SRAM D1
    dmaRegion.Size = MPU_REGION_SIZE_32KB;
    dmaRegion.AccessPermission = MPU_REGION_FULL_ACCESS;
    dmaRegion.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    dmaRegion.IsCacheable = MPU_ACCESS_CACHEABLE;
    dmaRegion.IsShareable = MPU_ACCESS_SHAREABLE;
    dmaRegion.Number = MPU_REGION_NUMBER0;
    dmaRegion.TypeExtField = MPU_TEX_LEVEL0;
    dmaRegion.SubRegionDisable = 0x00;
    dmaRegion.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&dmaRegion);

    dmaRegion.BaseAddress = 0x38000000; // SRAM D3
    dmaRegion.Number = MPU_REGION_NUMBER1;
    dmaRegion.Size = MPU_REGION_SIZE_4KB;
    HAL_MPU_ConfigRegion(&dmaRegion);
    
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
