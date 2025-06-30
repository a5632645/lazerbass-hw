#include "FreeRTOS.h"
#include "task.h"
#include "bsp/DebugIO.hpp"
#include "SystemHook.hpp"

void vApplicationStackOverflowHook(TaskHandle_t xTask,char* pcTaskName) {
    bsp::DebugIO{}.Print(pcTaskName).Print(" stack overflow").NewLine()
        .FlashErrorLight();
}

void RTOS_Assert(const char* file, int line) {
    bsp::DebugIO{}.Print("[error]: rtos assert")
        .Print(file).Print(':').Print(line).NewLine()
        .FlashErrorLight();
}

void Error_Handler(void) {
    bsp::DebugIO{}.Print("[error]: error handler").NewLine()
        .FlashErrorLight();
}

void hook::DeviceError(const char* tag, const char* msg, const char* file, int line) {
    bsp::DebugIO{}.Print("[error]: ").Print(tag).Print(": ").Print(msg).Print(" at ").Print(file).Print(':').Print(line).NewLine()
        .FlashErrorLight();
}

void hook::DeviceErrorCode(const char* tag, const char* msg, int32_t code, const char* file, int line) {
    bsp::DebugIO{}.Print("[error]: ").Print(tag).Print(": ").Print(msg).Print(" code: ").Hex(code).Print(" at ").Print(file).Print(':').Print(line).NewLine()
        .FlashErrorLight();
}

void hook::AppError(const char* tag, const char* msg, const char* file, int line) {
    bsp::DebugIO::Write("[error]: %s: %s at %s:%d\r\n", tag, msg, file, line);
}

void hook::AppErrorCode(const char* tag, const char* msg, int32_t code, const char* file, int line) {
    bsp::DebugIO::Write("[error]: %s: %s code: %d at %s:%d\r\n", tag, msg, code, file, line);
}

void hook::DeviceLog(const char* tag, const char* msg) {
    bsp::DebugIO{}.Print('[').Print(tag).Print("]: ").Print(msg).NewLine();
}

void hook::AppLog(const char* tag, const char* msg) {
    bsp::DebugIO::Write("[%s]: %s\r\n", tag, msg);
}
