#include "stepper.h"
#include <stdio.h>

// PA1, PA3, PA4, PA5
#define IN1_PIN   GPIO_PIN_1
#define IN1_PORT  GPIOA
#define IN2_PIN   GPIO_PIN_3
#define IN2_PORT  GPIOA
#define IN3_PIN   GPIO_PIN_4
#define IN3_PORT  GPIOA
#define IN4_PIN   GPIO_PIN_5
#define IN4_PORT  GPIOA

// TIM1 handle is defined in main.c, declared extern here
extern TIM_HandleTypeDef htim2;

static const uint8_t step_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

static void microDelay(uint16_t delay)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < delay);
}

static void setPins(uint8_t step[4])
{
    HAL_GPIO_WritePin(IN1_PORT, IN1_PIN, step[0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_PORT, IN2_PIN, step[1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN3_PORT, IN3_PIN, step[2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_PORT, IN4_PIN, step[3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Stepper_Init(void)
{
    // GPIO already initialized by MX_GPIO_Init — just start TIM2
    HAL_TIM_Base_Start(&htim2);
}

static int seq_idx = 0;

void Stepper_Step(int steps, uint16_t delay_us, int direction)
{
    int seq_len = sizeof(step_sequence) / sizeof(step_sequence[0]);

    // printf("STEP: dir=%d start_idx=%d\r\n", direction, seq_idx); // Debug statement

    for (int x = 0; x < steps; x++)
    {
        setPins((uint8_t*)step_sequence[seq_idx]);
        microDelay(delay_us);

        seq_idx += direction;

        if (seq_idx >= seq_len) seq_idx = 0;
        if (seq_idx < 0)        seq_idx = seq_len - 1;
    }
}
