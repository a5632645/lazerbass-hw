#pragma once

void MCUInit(void);
void Error_Handler(void);
void MCUPostInit(void);

#ifdef USE_FULL_ASSERT
void assert_failed(const char* file, uint32_t line);
#endif /* USE_FULL_ASSERT */