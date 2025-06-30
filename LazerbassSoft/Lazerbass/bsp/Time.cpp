#include "Time.hpp"
#include <numeric>
#include "stm32h7xx_hal.h"

namespace bsp {

static TIM_HandleTypeDef htim_;

void Time::Init() {
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim_.Instance = TIM2;
    htim_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    htim_.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_.Init.Prescaler = 10000 - 1;
    htim_.Init.Period = std::numeric_limits<uint32_t>::max();
    htim_.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&htim_);
    HAL_TIM_Base_Start(&htim_);
}

uint32_t Time::GetTick() {
    return __HAL_TIM_GET_COUNTER(&htim_);
}

void Time::ClearCounter() {
    __HAL_TIM_SET_COUNTER(&htim_, 0);
}

}