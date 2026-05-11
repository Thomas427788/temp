#include "main.h"
#include "cmsis_os.h"
#include "stepper.h"
#include "shared_state.h"
#include <stdio.h>
#include <math.h>

#define REVERSE_DURATION_S 6
#define APPROACH_FAST_MS   5000   // fast approach duration — tune this
#define STEP_DELAY_US      1000

typedef enum {
    STATE_IDLE,
    STATE_APPROACH_FAST,
    STATE_FORWARD,
    STATE_DWELL,
    STATE_REVERSE
} MotorState;

void StartStepperTask(void *argument)
{
    Stepper_Init();

    MotorState state     = STATE_IDLE;
    uint32_t   state_end = 0;
    uint32_t   dwell_ms  = 0;   // captured from command at run start

    for (;;)
    {
        switch (state)
        {
        case STATE_IDLE:
            osEventFlagsWait(g_cmd_event, CMD_READY_FLAG,
                             osFlagsWaitAll | osFlagsNoClear, osWaitForever);
            osEventFlagsClear(g_cmd_event, CMD_READY_FLAG);

            taskENTER_CRITICAL();
            dwell_ms     = (uint32_t)(g_command_params.duration_s * 1000.0f);
            g_target_psi = g_command_params.pressure;
            taskEXIT_CRITICAL();

            printf("RUN STARTED: %s | PRESSURE:%.2f | DWELL:%.1fs\r\n",
                   g_command_params.name,
                   g_command_params.pressure,
                   g_command_params.duration_s);

            // Start fast approach — 10 seconds of full speed
            state     = STATE_APPROACH_FAST;
            state_end = osKernelGetTickCount() + APPROACH_FAST_MS;
            break;

        case STATE_APPROACH_FAST:
            if (osKernelGetTickCount() >= state_end)
            {
                // 10 seconds up — switch to fine control
                printf("APPROACH DONE — switching to fine control\r\n");
                state = STATE_FORWARD;
            }
            else if (g_current_psi >= g_target_psi)
            {
                // Hit target early during fast approach — skip straight to dwell
                state     = STATE_DWELL;
                state_end = osKernelGetTickCount() + dwell_ms;
                printf("TARGET REACHED EARLY %.4f PSI — dwelling %lums\r\n", g_current_psi, dwell_ms);
            }
            else
            {
                // Full speed bursts
                Stepper_Step(64, 500, -1);
            }
            break;

			case STATE_FORWARD:
			{
			    float error = g_target_psi - g_current_psi;

			    if (fabsf(error) <= 0.3f)
			    {
			    	// On target — settle briefly before starting dwell timer
			    	osDelay(300);   // let mechanics settle before committing

			    	// Re-read after settling
			    	if (fabsf(g_target_psi - g_current_psi) <= 0.3f)
			    	{
			    	    state     = STATE_DWELL;
			    	    state_end = osKernelGetTickCount() + dwell_ms;
			    	    printf("TARGET REACHED %.4f PSI — dwelling %lums\r\n", g_current_psi, dwell_ms);
			    	}
			    	// else loop again — we bounced out of deadband during settle
			    }
			    else if (error > 2.0f)
			    {
			        // Far below target — full speed forward
			        Stepper_Step(1, 4000, -1);
			    }
			    else if (error > 0.5f)
			    {
			        // Getting close — slow down
			        Stepper_Step(8, 2000, -1);
			        osDelay(50);
			    }
			    else if (error > 0.05f)
			    {
			        // Very close — single step forward
			        Stepper_Step(1, 3000, -1);
			        osDelay(50);
			    }
			    else if (error < -0.05f)
			    {
			        // Overshot — single step back
			        Stepper_Step(1, 3000, 1);
			        osDelay(50);
			    }
			    break;
			}

			case STATE_DWELL:
			{
			    static float last_error = 0.0f;
			    float error      = g_target_psi - g_current_psi;
			    float derivative = error - last_error;   // rate of change
			    last_error       = error;

			    if (osKernelGetTickCount() >= state_end)
			    {
			        last_error = 0.0f;   // reset for next run
			        state      = STATE_REVERSE;
			        state_end  = osKernelGetTickCount() + (REVERSE_DURATION_S * 1000);
			        printf("DWELL DONE — reversing %ds\r\n", REVERSE_DURATION_S);
			    }
			    else if (fabsf(error) <= 0.3f && derivative > -0.1f)
			    {
			        // On target and not falling fast — hold
			        osDelay(50);
			    }
			    else if (error > 0.3f || derivative < -0.3f)
			    {
			        // Below target OR pressure dropping fast — nudge forward
			        Stepper_Step(1, 3000, -1);
			        osDelay(50);
			    }
			    else if (error < -0.3f || derivative > 0.3f)
			    {
			        // Above target OR pressure rising fast — nudge back
			        Stepper_Step(1, 3000, 1);
			        osDelay(50);
			    }
			    break;
			}

            case STATE_REVERSE:
                if (osKernelGetTickCount() >= state_end)
                {
                    state = STATE_IDLE;
                    printf("RUN COMPLETE — awaiting next command\r\n");
                }
                else
                {
                    Stepper_Step(64, 500, 1);
                }
                break;
        }
    }
}
