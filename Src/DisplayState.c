//
// Created by zarthrag on 2/25/20.
//
#include <HeaterFSM.h>
#include "main.h"
#include "DisplayState.h"
#include "stdio.h"
#include "bme280_interface.h"
#include "dataLog.h"
#include "HeaterFSM.h"

#define DISPLAY_TIMEOUT 4000

enum displayState_t { DS_INFO, DS_SYSTEM, DS_TEMP, DS_HUMIDITY, DS_PRESSURE, DS_DEWPT, DS_DESSICANT, DS_ERRSTATE};
enum displayState_t currentDisplayState = DS_TEMP;
enum displayState_t nextDisplayState;
uint32_t displayStateTimeout = DISPLAY_TIMEOUT;

char infoScreen[128] = {'\0'};
bool infoScreenEnabled = false;
uint32_t infoScreenTimeout = 0;

char errorScreen[128];
bool errorScreenEnabled = false;
uint32_t errorScreenTimeout = 0;

char* fsmToString(enum ControlState_t state);

char* getInfoString(void)
{

    return infoScreen;
}

char* fsmToString(enum ControlState_t state)
{
    switch (state)
    {

        case FSM_INIT:
            return " Reinitializing ";
        case FSM_RH_TARGET:
            return " #### IDLE #### ";
        case FSM_RH_LIMIT:
            return " ### DRYING ### ";
        case FSM_TEMP_LIMIT:
            return " ## OVERHEAT ## ";
        case FSM_FATAL:
            return " # FATAL ERROR #";
        default:
            return " Unimplemented!";
            break;
    }

}

char* getErrorString(void)
{
    return errorScreen;
}

void setInfoDisplayState(uint32_t timeout)
{
    nextDisplayState = currentDisplayState;
    currentDisplayState = DS_INFO;
    // TODO - Short beep
    displayStateTimeout = timeout == 0 ? timeout : HAL_GetTick() + timeout;
    printf(infoScreen);

}

void setErrorDisplayState(uint32_t timeout)
{

    // @todo Implement beep for error display
    // @body A long beep needs to sound during DS_ERRSTATE.
    nextDisplayState = currentDisplayState;
    currentDisplayState = DS_ERRSTATE;
    displayStateTimeout = timeout == 0 ? timeout : HAL_GetTick() + timeout;
    printf(errorScreen);
}

void updateDisplayState(void)
{

    if(displayStateTimeout && (HAL_GetTick() > displayStateTimeout))
    {
        // Move to the next display state, unless there's an error screen, which needs to alternate with every other display.
        currentDisplayState = nextDisplayState;
        displayStateTimeout = DISPLAY_TIMEOUT;
    }

    float min = 0;
    float max = 0;
    float current = 0;
    trend_t trend = FLAT;

    switch(currentDisplayState)
    {
        case DS_INFO:
            // Display has an informational message already, leave it alone.
            printf(infoScreen);
            nextDisplayState = DS_SYSTEM;
            break;
        case DS_SYSTEM:
            /* System display has Current state and action information.
             * xxxxxxxxxxxxxxxxxx
             * x   SYS_STATE    x
             * x  STATE REASON  x
             * xxxxxxxxxxxxxxxxxx
             */
            printf("\f%s\n%s",fsmToString(getSystemState()),getSystemStateString());
            nextDisplayState = DS_TEMP;


            break;
        case DS_TEMP:
            /* Temp display has min, current, trend arrow, and max
             * xxxxxxxxxxxxxxxxxx
             * x   Temp (*C)    x
             * xff.f<<ff.f>>ff.fx
             * xxxxxxxxxxxxxxxxxx
             */
            // Get Temp values
            current = dataLogGetValue(TEMPERATURE, &trend, &max, &min);
            // Display the current temperature.
            printf("\f   Temp  (\337C)   \n%4.1f  %4.1f%c %4.1f", min, current, ' ', max);
            nextDisplayState = DS_HUMIDITY;
            break;
        case DS_HUMIDITY:
            /* Humidity display has MIN CURRENT, trend arrow, and MAX
             * xxxxxxxxxxxxxxxxxx
             * x Humidity (%RH) x
             * xff.f<<ff.f>>ff.fx
             * xxxxxxxxxxxxxxxxxx
             */
            current = dataLogGetValue(HUMIDITY, &trend, &max, &min);
            // Display the humidity
            printf("\f Humidity (%%RH)\n%04.1f  %04.1f%c %04.1f", min, current, ' ', max);
            nextDisplayState = DS_PRESSURE;

            break;
        case DS_PRESSURE:
            /* Pressure display has MIN, CURRENT, trend arrow, and MAX
             * xxxxxxxxxxxxxxxxxx
             * x Pressure (inHg)x
             * xff.f<ff.ff> ff.fx
             * xxxxxxxxxxxxxxxxxx
             */
            // Display the pressure
            current = dataLogGetValue(PRESSURE, &trend, &max, &min);
            printf("\f Pressure (inHg)\n%04.1f %05.2f%c %04.1f", min, current, ' ', max);
            nextDisplayState = DS_DEWPT;

            break;
        case DS_DEWPT:
            /* Dew Point display has MIN CURRENT, trend arrow, and MAX
             * xxxxxxxxxxxxxxxxxx
             * x Dew Point (*C) x
             * xff.f <ff.f> ff.fx
             * xxxxxxxxxxxxxxxxxx
             */
            current = dataLogGetValue(DEWPOINT, &trend, &max, &min);
            printf("\f Dew Point (\337C) \n%04.1f  %04.1f%c %04.1f", min, current, ' ', max);
            nextDisplayState = DS_DESSICANT;
                break;
        case DS_DESSICANT:
            /* Dessicant Status display has MIN CURRENT, trend arrow, and MAX
             * xxxxxxxxxxxxxxxxxx
             * x   Dessicant    x
             * x ff% ########## x
             * xxxxxxxxxxxxxxxxxx
             */
            // Show the estimated dessicant status.
            printf("\f  Dessicant (\337C) \n Unimplemented", min, current, ' ', max);
            nextDisplayState = DS_SYSTEM;

        case DS_ERRSTATE:
            // If an error message is available, show it.
            printf(errorScreen);
            break;
        default:
            printf("\fDisplayState\nError (%i)", currentDisplayState);
            break;
    }

}
