#pragma once

#define _BSS_SRAMD1 __attribute__ ((section (".sramd1.zero")))
#define _BSS_SRAMD2 __attribute__ ((section (".sramd2.zero")))
#define _BSS_SRAMD3 __attribute__ ((section (".sramd3.zero")))
#define _CODE_ITCM __attribute__ ((section (".itcmram.code")))

#define _DATA_SRAMD1 __attribute__ ((section (".sramd1.nonzero")))
#define _DATA_SRAMD2 __attribute__ ((section (".sramd2.nonzero")))
#define _DATA_SRAMD3 __attribute__ ((section (".sramd3.nonzero")))

#define _NOINIT_SRAMD1 __attribute__ ((section (".sramd1.noinit")))
#define _NOINIT_SRAMD2 __attribute__ ((section (".sramd2.noinit")))
#define _NOINIT_SRAMD3 __attribute__ ((section (".sramd3.noinit")))

#define _DMA_SRAMD1 __attribute__ ((section (".sramd1.dma")))
#define _DMA_SRAMD2 __attribute__ ((section (".sramd2.dma")))
#define _DMA_SRAMD3 __attribute__ ((section (".sramd3.dma")))

class MCUMemory {
public:
    static void SRAM_D1_Init(void);
    static void _SramD2_Init(void);
    static void _SramD3_Init(void);
    static void _ItcmRam_Init(void);
    static void DMA_MPU_Init(void);
};
