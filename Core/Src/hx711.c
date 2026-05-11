#include "hx711.h"
#include "cmsis_os.h"

void HX711_Init(void)
{
    // Enable GPIOD clock (already done in MX_GPIO_Init, but safe to repeat)
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // CLK as output
    GPIO_InitStruct.Pin   = HX711_CLK_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HX711_CLK_PORT, &GPIO_InitStruct);

    // DOUT as input
    GPIO_InitStruct.Pin  = HX711_DOUT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(HX711_DOUT_PORT, &GPIO_InitStruct);

    // CLK starts low
    HAL_GPIO_WritePin(HX711_CLK_PORT, HX711_CLK_PIN, GPIO_PIN_RESET);
}

uint8_t HX711_IsReady(void)
{
    // DOUT low means data is ready
    return (HAL_GPIO_ReadPin(HX711_DOUT_PORT, HX711_DOUT_PIN) == GPIO_PIN_RESET);
}

int32_t HX711_ReadRaw(void)
{
    uint32_t raw = 0;

    // Wait until ready — yield to RTOS while waiting
    while (!HX711_IsReady())
        osDelay(1);

    // Clock in 24 bits, MSB first
    for (int i = 0; i < 24; i++)
    {
    	HAL_GPIO_WritePin(HX711_CLK_PORT, HX711_CLK_PIN, GPIO_PIN_SET);
    	for (volatile int d = 0; d < 20; d++) __NOP();  // wait for DOUT to settle
    	raw = (raw << 1) | HAL_GPIO_ReadPin(HX711_DOUT_PORT, HX711_DOUT_PIN);
    	HAL_GPIO_WritePin(HX711_CLK_PORT, HX711_CLK_PIN, GPIO_PIN_RESET);
    	for (volatile int d = 0; d < 20; d++) __NOP();
    }

    // 25th pulse — sets gain=128, channel A for next reading
    HAL_GPIO_WritePin(HX711_CLK_PORT, HX711_CLK_PIN, GPIO_PIN_SET);
    __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(HX711_CLK_PORT, HX711_CLK_PIN, GPIO_PIN_RESET);
    __NOP(); __NOP(); __NOP();

    // Sign-extend from 24-bit two's complement to int32_t
    if (raw & 0x800000)
        raw |= 0xFF000000;

    return (int32_t)raw;
}
