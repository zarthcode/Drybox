//
// Created by zarthrag on 2/27/20.
//

#include <stm32f0xx_hal.h>
#include <stdio.h>
#include <string.h>
#include <DisplayState.h>
#include <dataLog.h>
#include "HeaterFSM.h"
#include "bme280_interface.h"

/**
 * The HeaterFSM needs to monitor the humidity - if it's flat, do nothing.
 * Maintain the air at temp target, which should raise the humidity in the air, ensuring it isn't int he filament.
 */

enum ControlState_t fsmState = FSM_INIT;
enum ControlState_t fsmPrev = FSM_INIT;     // Tracks the previous FSM state
uint32_t stateTimeout = 0;  // Init state doesn't time out until trend data is available.
enum HeaterState_t HeaterState = HEATER_OFF;

// The target RH the drybox will try to maintain.
#define RH_TARGET 10.0f

// The RH that the drybox will treat as an upper limit/hysteresis.
#define RH_LIMIT 15.0f

// The RH that the drybox will treat the dessicant as depleted.
#define RH_ALARM 20.0f

// Minimum allowed temperature inside the drybox.
#define TEMP_MINIMUM 30.0f

// The maximum allowable temperature of the filament. (Keep this below Tglass)
#define TEMP_TARGET 51.0f

// Overheat temperature
#define TEMP_LIMIT 55.0f


// Trend & inflection data
trend_t previousTrend = INCREASING;
bool bPlateau = false;

char* stateData[32] = {'\0'};

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
    enum ControlState_t nextState = FSM_NONE;

    // FSM Implementation.
    switch(fsmState)
    {
        case FSM_INIT:
            // @todo Stay in FSM_INIT until trend is available.
            // @body The FSM should collect data before activating the heater, unless a request is received.
            if(data.temperature <= TEMP_TARGET)
            {
                sprintf(&stateData[0],"Heating to TGT");
                nextState = FSM_TEMP_HEAT;
            }
            else if (data.humidity > RH_TARGET)
            {
                sprintf(&stateData[0], "RH above TGT");
                nextState = FSM_TEMP_HEAT;
            }
            else
            {
                sprintf(stateData,"Temp at target");
                nextState = FSM_TEMP_TARGET;
            }
            HeaterState = HEATER_OFF;
            break;
        case FSM_TEMP_TARGET:   // Temperature is within bounds.
            if(data.temperature <= TEMP_MINIMUM)
            {
                // Move to active heating state
                sprintf(stateData," Temp below min");
                nextState = FSM_TEMP_HEAT;
                bPlateau = false;
                previousTrend = humidityTrend();
            }
            else if (data.humidity > RH_TARGET)
            {
                // RH is too high
                sprintf(stateData,"Humidity > TGT");
                nextState = FSM_TEMP_HEAT;
                bPlateau = false;
                previousTrend = humidityTrend();
            }
            HeaterState = HEATER_OFF;
            break;
        case FSM_TEMP_LIMIT:
            if(data.temperature >= TEMP_LIMIT)
            {
                // Over temperature
                sprintf(stateData, "Over Temperature");
                nextState = FSM_TEMP_OVERHEAT;
            }
            else if(data.temperature <= TEMP_TARGET)
            {
                // Temperature below minimum
                sprintf(stateData,"Temp at Target");
                nextState = FSM_TEMP_HEAT;
            }
            HeaterState = HEATER_OFF;
            break;
        case FSM_TEMP_HEAT:
            if(data.temperature >= TEMP_LIMIT)
            {
                // Over temperature
                sprintf(stateData, "Over Temperature");
                nextState = FSM_TEMP_OVERHEAT;
            }
            else if(data.temperature >= TEMP_TARGET)
            {
                sprintf(stateData, "Temp at Target");
                nextState = FSM_TEMP_TARGET;
            }
            else if(HAL_GetTick()/(1000*60) - stateTimeout > 60)
            {
                // It's been over 60 minutes
                sprintf(stateData, "Heat Timeout");
                nextState = FSM_TEMP_TARGET;
            }
            HeaterState = HEATER_ON;
            break;
        case FSM_TEMP_OVERHEAT:
            // Wait for it to cool off below TARGET.
            if(data.temperature <= TEMP_LIMIT)
            {
                sprintf(stateData," System temp OK");
                nextState = FSM_INIT;
            }
            HeaterState = HEATER_LIMIT;
            break;

        default:
            sprintf(stateData, "Unimpl State %i", fsmState);
            nextState = FSM_INIT;
            HeaterState = HEATER_OFF;

    }

    if(nextState != FSM_NONE)
    {
        // Reset time since last state change
        stateTimeout = HAL_GetTick()/(1000*60); // Determine time to the nearest minute.

        fsmPrev = fsmState;
        fsmState = nextState;
    }

    return HeaterState;


}

// Returns a % estimation of dessicant depletion.
float getDessicantState(void)
{
    // @todo Implement dessicant state function
}

// Request to perform a drying cycle, likely in response to a filament change.
void cycleRequest(void)
{
    // Immediately place the FSM into active drying, with a timeout.
    if(fsmState == FSM_TEMP_LIMIT)
    {
        sprintf(stateData,"!!OVERTEMP!!");
    }
    else
    {
        sprintf(stateData,"CYCLE REQUESTED");
        fsmState = FSM_TEMP_HEAT;
        // @todo Implement cycle timeout.
        // @body States should have a cycle timeout to prevent 100% duty cycle of the heater.
        stateTimeout = HAL_GetTick() + 1000 * 60 * 45;      // 45min cycle
    }

}

// Request to perform a drying cycle, in response to new dessicant.
void dessicantReset(void)
{
    // @todo Implement dessicant % reset in datalog
    // @body Signal to the controller that the dessicant is new/dry.

    // Immediately place the FSM into active drying, with a timeout.
    if(fsmState == FSM_TEMP_LIMIT)
    {
        sprintf(stateData,"DESSICANT CHANGE");
    }
    else
    {
        sprintf(stateData,"DESSICANT CHANGE");
        fsmState = FSM_TEMP_HEAT;
        // @todo Implement cycle timeout.
        // @body States should have a cycle timeout to prevent 100% duty cycle of the heater.
        stateTimeout = HAL_GetTick() + 1000 * 60 * 45;      // 45min cycle
    }

}

// Returns the state of the FSM.
enum ControlState_t getSystemState()
{
    return fsmState;
}

// Returns additional state data
char* getSystemStateString()
{
    return stateData;

}


