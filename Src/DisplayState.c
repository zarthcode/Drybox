//
// Created by zarthrag on 2/25/20.
//
#include "main.h"
#include "DisplayState.h"
#include "stdio.h"
#include "bme280_interface.h"
#include "dataLog.h"

#define DISPLAY_TIMEOUT 4000

typedef enum displayState_t { DS_INFO, DS_TEMP, DS_HUMIDITY, DS_PRESSURE, DS_DEWPT, DS_DESSICANT, DS_ERRSTATE};
enum displayState_t currentDisplayState = DS_TEMP;
enum displayState_t nextDisplayState;
uint32_t displayStateTimeout;

void setInfoDisplayState(uint8_t priority, uint32_t timeout)
{
    nextDisplayState = currentDisplayState; // Save the current display state
    currentDisplayState = DS_INFO;
    displayStateTimeout = HAL_GetTick() + timeout;
}
void updateDisplayState(void)
{

    if((currentDisplayState != DS_ERRSTATE) && displayStateTimeout && (HAL_GetTick() > displayStateTimeout))
    {
        // Move to the next display state.
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
            nextDisplayState = DS_TEMP;
                break;
        case DS_DESSICANT:
            /* Dessicant Status display has MIN CURRENT, trend arrow, and MAX
             * xxxxxxxxxxxxxxxxxx
             * x   Dessicant    x
             * x ff% ########## x
             * xxxxxxxxxxxxxxxxxx
             */
            // Show the estimated dessicant status.
        case DS_ERRSTATE:
            // An error message is on screen, leave it forever.
            break;
        default:
            printf("\fDisplayState\nError (%i)", currentDisplayState);
            break;
    }

}
