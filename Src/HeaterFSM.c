//
// Created by zarthrag on 2/27/20.
//

#include <stm32f0xx_hal.h>
#include <stdio.h>
#include <DisplayState.h>
#include <dataLog.h>
#include "HeaterFSM.h"
#include "bme280_interface.h"

typedef enum controlState_t { FSM_INIT,             // Starting state.
                              FSM_RH_TARGET,        // System RH is in optimal range.
                              FSM_RH_LIMIT,         // System is trying to return to optimal RH
                              FSM_RH_FALLING,
                              FSM_RH_FLAT,
                              FSM_RH_INCREASING,
                              FSM_TEMP_LIMIT,       // System is approaching temp limit.
                              FSM_DESSICANT,
                              FSM_FATAL             // System is in an error state.
                            };

enum controlState_t fsmState = FSM_INIT;
enum controlState_t fsmPrev = FSM_INIT;     // Tracks the previous FSM state
uint32_t stateTimeout = 0;  // Init state doesn't time out until trend data is available.


// The target RH the drybox will try to maintain.
#define RH_TARGET 10.0f

// The RH that the drybox will treat as an upper limit/hysteresis.
#define RH_LIMIT 15.0f

// The RH that the drybox will treat the dessicant as depleted.
#define RH_ALARM 18.0f

// The maximum allowable temperature of the filament. (Keep this below Tglass)
#define TEMP_TARGET 50.0f

// Overheat temperature
#define TEMP_LIMIT 55.0f

// Trend & inflection data
trend_t previousTrend = INCREASING;
bool bPlateau = false;


enum HeaterState_t getHeaterState()
{
    // Get current bme280 data.
    bme280_data_t data;
    if (bme280_interface_get_data(&data) != 0)
    {
        // Fatal error
        fsmPrev = fsmState;
        fsmState = FSM_FATAL;
        return HEATER_OFF;
    }

    // Temperature limit overrides all other states.
    if (data.temperature > TEMP_MAX)
    {
        // Over temperature.
        fsmPrev = fsmState;
        fsmState = FSM_TEMP_LIMIT;

        printf("\fOVER-TEMPERATURE\n   Heater Off");
        setInfoDisplayState(DP_INFO, 10000);

        // Setup a lockout timer.
        stateTimeout = HAL_GetTick() + 1000 * 60 * 15;      // 15 min timeout

    }

    switch(fsmState)
    {
        case FSM_INIT:
            break;
        case FSM_RH_TARGET:
            if(data.humidity >= RH_LIMIT)
            {
                // Move to active drying state
                fsmState = FSM_RH_LIMIT;
                bPlateau = false;
                previousTrend = humidityTrend();
            }
            break;
        case FSM_RH_LIMIT:
            if(data.temperature >= TEMP_LIMIT)
            {
                // Over temperature
                fsmState = FSM_TEMP_LIMIT;
            }
            else
            {
                trend_t nextTrend = humidityTrend();
                if (previousTrend != nextTrend)
                {
                    // There has been a transition.

                    switch (nextTrend)
                    {

                        case INCREASING:
                            // If previously decreasing/flat, this is a bad sign
                            stateTimeout = 1000 * 60 * 30;      // 30min timeout to change things.
                            break;
                        case DECREASING:
                            // If previously flat/increasing, this is good.
                            break;
                        case FLAT:
                            switch (previousTrend)
                            {
                                case INCREASING:
                                    // Starting to get under control.
                                    break;
                                case DECREASING:
                                    // If previously decreasing, we're done.
                                    break;
                                default:
                                    break;
                            }

                            break;
                        default:
                            break;
                    }
                    previousTrend = nextTrend;
                }
                if (data.humidity <= RH_TARGET)
                {
                    // Reached target.
                    fsmState = FSM_RH_TARGET;
                }


            }


            //
            break;
        case FSM_TEMP_LIMIT:
            // Wait for it to cool off below TARGET.
            if(data.temperature < TEMP_TARGET)
            {
                fsmState = FSM_INIT;
            }
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



