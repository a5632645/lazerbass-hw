#include <cmath>
#include <numbers>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "mcu/MCUInit.hpp"
#include "mcu/Memory.hpp"

#include "bsp/DebugIO.hpp"
#include "bsp/ControlIO.hpp"
#include "bsp/PCM5102.hpp"
#include "bsp/Oled.hpp"
#include "bsp/USBMidi.hpp"
#include "bsp/Time.hpp"

#include "gui/GuiDispatch.hpp"
#include "dsp/Lazerbass.hpp"

_NOINIT_SRAMD1 static StackType_t _audioStack[8192];
static StaticTask_t _audioTcb;
static dsp::Lazerbass bass_;
static StereoSample _buffer[bsp::PCM5102::kBlockSize];
static StaticSemaphore_t audioLock_;
static SemaphoreHandle_t audioLockHandle_ = NULL;

static uint32_t audioTickCounter = 0;
static void AudioTask(void*) {
    bsp::PCM5102::Init();
    bsp::PCM5102::Start();

    bass_.Init(bsp::PCM5102::kSampleRate, 200);
    
    for (;;) {
        auto buf = bsp::PCM5102::GetNextBlock();

        xSemaphoreTake(audioLockHandle_, portMAX_DELAY);

        bsp::Time::ClearCounter();

        bass_.Process(std::span{_buffer, std::size(_buffer)});

        audioTickCounter = bsp::Time::GetTick();

        xSemaphoreGive(audioLockHandle_);

        std::copy_n(_buffer, std::size(_buffer), buf.begin());
    }

    bsp::PCM5102::Stop();
    bsp::PCM5102::DeInit();
}
 
_NOINIT_SRAMD1 static StackType_t _testStack[1024];
static StaticTask_t _testTcb;
static void TestTask(void*) {
    for (;;) {
        bsp::DebugIO::Write("[debug] audio task take %dms\n\r", bsp::Time::Tick2Ms(audioTickCounter));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

_NOINIT_SRAMD1 static StackType_t _bspStack[8192];
static StaticTask_t _bspTcb;
static void BspTask(void*) {
    bsp::Oled::Init();
    gui::gGuiDispatch.Init(audioLockHandle_, bass_.GetParams(), bass_);
    gui::gGuiDispatch.EnableEventProcessing();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xLastUpdateTime = xTaskGetTickCount();
    for (;;) {
        bsp::ControlIO::WaitForNextEventBlock();
        auto btnEvents = bsp::ControlIO::GetBtnEvents();
        auto encoderValues = bsp::ControlIO::GetEncoderValues();

        if (gui::gGuiDispatch.IsEventProcessingEnabled()) {
            gui::gGuiDispatch.BtnEvent(btnEvents);
            for (int i = 0; i < 4; ++i) {
                if (encoderValues[i] != 0) {
                    gui::gGuiDispatch.EncoderEvent(bsp::ControlIO::EncoderId(i), encoderValues[i]);
                    encoderValues[i] = 0;
                }
            }
        }

        TickType_t xCurrentTime = xTaskGetTickCount();
        gui::gGuiDispatch.TimeTick(pdTICKS_TO_MS(xCurrentTime - xLastWakeTime));
        xLastWakeTime = xCurrentTime;

        if (xCurrentTime - xLastUpdateTime > pdMS_TO_TICKS(gui::gGuiDispatch.kMsPerFrame)) {
            xLastUpdateTime = xCurrentTime;
            gui::gGuiDispatch.Update();
            bsp::Oled::SendFrame();
        }
    }
}

_NOINIT_SRAMD1 static StackType_t _usbStack[1024];
static StaticTask_t _usbTcb;
static void UsbTask(void*) {
    bsp::USBMidi::Init();
    for (;;) {
        bsp::USBMidi::WaitForNextBlock();

        xSemaphoreTake(audioLockHandle_, portMAX_DELAY);
        for (auto* e = bsp::USBMidi::GetNextEvent();
             e != nullptr;
             e = bsp::USBMidi::GetNextEvent()) {
            if (e->IsNoteOn()) {
                bass_.NoteOn(e->GetNote(), e->GetVelocity() / 127.0f);
                bsp::DebugIO::Write("[midi] note on: %d\n\r", e->GetNote());
            }
            else if (e->IsNoteOff()) {
                bass_.NoteOff(e->GetNote(), e->GetVelocity() / 127.0f);
                bsp::DebugIO::Write("[midi] note off: %d\n\r", e->GetNote());
            }
        }
        xSemaphoreGive(audioLockHandle_);
    }
}

// --------------------------------------------------------------------------------
// AppMain
// --------------------------------------------------------------------------------
static void AppMain(void*) {
    bsp::DebugIO{}.Print("[info]: init debug io rtos").NewLine();
    bsp::DebugIO::StartRTOSIO();
    bsp::DebugIO{}.Print("[info]: init control io rtos").NewLine();
    bsp::DebugIO::Write("[info]: Test DebugIO RTOS Print\n\r");

    bsp::ControlIO::Init();
    bsp::ControlIO::SetAllLeds(false);

    bsp::Time::Init();

    audioLockHandle_ = xSemaphoreCreateBinaryStatic(&audioLock_);
    xSemaphoreGive(audioLockHandle_);

    xTaskCreateStatic(AudioTask, "audio", std::size(_audioStack), nullptr, 0, _audioStack, &_audioTcb);
    xTaskCreateStatic(TestTask, "test", std::size(_testStack), nullptr, 0, _testStack, &_testTcb);
    xTaskCreateStatic(BspTask, "control", std::size(_bspStack), nullptr, 0, _bspStack, &_bspTcb);
    xTaskCreateStatic(UsbTask, "usb", std::size(_usbStack), nullptr, 0, _usbStack, &_usbTcb);

    bsp::DebugIO{}.Print("[info]: test final print").NewLine();
    bsp::DebugIO::Write("[info]: test rtos final print\n\r");

    vTaskDelete(nullptr);
}

// --------------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------------
int main()
{
    MCUInit();
    MCUMemory::SRAM_D1_Init();
    MCUMemory::DMA_MPU_Init();
    
    bsp::DebugIO{}.Init().SetLed(false, false, false);
    bsp::DebugIO{}.Print("[info]: Test DebugIO Print").NewLine();

    if (auto res = xTaskCreate(AppMain, "app", 1024, nullptr, 0, nullptr);
        res != pdPASS)  {
        bsp::DebugIO{}.Print("[error]: create app failed: ").Print(res).NewLine().FlashErrorLight();
    }
    else {
        bsp::DebugIO{}.Print("[info]: app created").NewLine();
    }

    vTaskStartScheduler();
}
