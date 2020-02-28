//
// Created by zarthrag on 2/27/20.
//

#include <stm32f0xx_hal.h>
#include <stdio.h>
#include <DisplayState.h>
#include "HeaterFSM.h"
#include "bme280_interface.h"

typedef enum controlState_t { FSM_INIT,             // Starting state.
                              FSM_RH_TARGET,        // System RH is in optimal range.
                              FSM_RH_LIMIT,         // System is trying to return to optimal RH
                              FSM_RH_FALLING,
                              FSM_RH_FLAT,
                              FSM_TEMP_LIMIT,       // System is approaching temp limit.
                              FSM_FATAL             // System is in an error state.
                            };

enum controlState_t fsmState = FSM_INIT;
uint32_t stateTimeout = 0;

// The target RH the drybox will try to maintain.
#define RH_TARGET 10.0f

// The RH that the drybox will treat as an upper limit/hysteresis.
#define RH_LIMIT 15.0f

// The RH that the drybox will treat the dessicant as depleted.
#define RH_ALARM 18.0f

// The maximum allowable temperature of the filament. (Keep this below Tglass)
#define TEMP_MAX 50.0f

enum controlState_t getHeaterState()
{
    // Get current bme280 data.
    bme280_data_t data;
    if (bme280_interface_get_data(&data) != 0)
    {
        // Fatal error
        fsmState = FSM_FATAL;
        return FSM_FATAL;
    }

    // Temperature limit overrides all other states.
    if (data.temperature > TEMP_MAX)
    {
        // Over temperature.
        fsmState = FSM_TEMP_LIMIT;

        printf("\fOVER-TEMPERATURE\n   Heater Off");
        setInfoDisplayState(DP_INFO, 10000);

        // Setup a lockout timer.
        stateTimeout = HAL_GetTick() + 1000 * 60 * 15;      // 15 min timeout

    }

    switch(fsmState)
    {
        case FSM_INIT:
            if(data.humidity > RH_LIMIT)
            {
                fsmState = FSM_RH_LIMIT;
            }

            break;

        case FSM_TEMP_LIMIT:
            // Wait for the damn thing to cool off.
            if (data.temperature < TEMP_MAX && stateTimeout < HAL_GetTick())
            {
                fsmState = FSM_INIT;    // Reset FSM state
            }
            break;

        case FSM_RH_TARGET:
            // Within RH parameters.
            printf("\f TARGET REACHED\nHeater Off");
            setInfoDisplayState(DP_INFO, 5000);

            break;

        case FSM_RH_LIMIT:
            if ( data.humidity <= FSM_RH_TARGET)
            {
                // Reached RH target, we're done!
                fsmState = FSM_RH_TARGET;
            }
            // Keep the heater on until it's either forced off, or RH trends down and stabilizes.
            else if ( data.humidity < FSM_RH_LIMIT)
            {
                // Determine if RH is trending flat


                // Recalculate dessicant capacity.

            }
            //

            break;

        default:
            // Unknown state, reset
            fsmState = FSM_INIT;
            break;
    }



}

// Returns a % estimation of dessicant depletion.
float getDessicantState(void)
{

}

// Request to perform a drying cycle, likely in response to a filament change.
void cycleRequest(void)
{

}

// Request to perform a drying cycle, in response to new dessicant.
void dessicantReset(void)
{

}



