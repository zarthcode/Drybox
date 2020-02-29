//
// Created by zarthrag on 2/26/20.
//

#include <stm32f0xx_hal.h>
#include "dataLog.h"
#include "bme280_interface.h"
#include <limits.h>
#include <stdint.h>
#include <float.h>

#define FLAT_THRESHOLD 0.05
#define TREND_COUNT 3
#define LOG_PERIOD 225000

struct {

    float Temperature[LOG_SIZE];
    float Humidity[LOG_SIZE];
    float Pressure[LOG_SIZE];
    float DewPoint[LOG_SIZE];

    uint8_t entryCount;

    float maxTemperature;
    float minTemperature;
    trend_t TemperatureTrend;

    float maxHumidity;
    float minHumidity;
    trend_t HumidityTrend;

    float maxPressure;
    float minPressure;
    trend_t PressureTrend;

    float maxDewPoint;
    float minDewPoint;
    trend_t DewPointTrend;

} dryboxLog;

// Trend Timing function
uint32_t nextLog = 0;


// Trend calculation function
trend_t calculateTrend(const float* array);

bool trendAvailable(void)
{
    return (dryboxLog.entryCount >= TREND_COUNT);
}

void dataLogUpdate(void)
{

    // Get data from the BME280 and update the log
    bme280_data_t data;
    bme280_interface_get_data(&data);
    data.time = HAL_GetTick();

    // Check against max/min values
    dryboxLog.maxTemperature = (data.temperature > dryboxLog.maxTemperature) ? data.temperature : dryboxLog.maxTemperature;
    dryboxLog.minTemperature = (data.temperature < dryboxLog.minTemperature) ? data.temperature : dryboxLog.minTemperature;
    dryboxLog.maxHumidity = (data.humidity > dryboxLog.maxHumidity) ? data.humidity : dryboxLog.maxHumidity;
    dryboxLog.minHumidity = (data.humidity < dryboxLog.minHumidity) ? data.humidity : dryboxLog.minHumidity;
    dryboxLog.maxPressure = (data.pressure > dryboxLog.maxPressure) ? data.pressure : dryboxLog.maxPressure;
    dryboxLog.minPressure = (data.pressure < dryboxLog.minPressure) ? data.pressure : dryboxLog.minPressure;
    dryboxLog.maxDewPoint = (data.dewPoint > dryboxLog.maxDewPoint) ? data.dewPoint : dryboxLog.maxDewPoint;
    dryboxLog.minDewPoint = (data.dewPoint < dryboxLog.minDewPoint) ? data.dewPoint : dryboxLog.minDewPoint;

    // @todo Store log min/max data to flash memory.
    // @body Keep observed max/min data in flash for easy reference. Especially minHumidity.

    // @todo Add dessicant calculation to data log.
    // @body Calculating dessicant performance can yield info on how long dessicant lasts, total drying capacity, etc.

    // @todo Add cycle time calculation to data log.
    // @body Adding cycle time info could help determine how long it takes to perform an average drying cycle. This can be compared to dessicant capacity, too.

    // Add the entry to the log
    if(nextLog < HAL_GetTick())
    {

        dryboxLog.Temperature[dryboxLog.entryCount] = data.temperature;
        dryboxLog.Pressure[dryboxLog.entryCount] = data.pressure;
        dryboxLog.Humidity[dryboxLog.entryCount] = data.humidity;
        dryboxLog.DewPoint[dryboxLog.entryCount] = data.dewPoint;


        if (dryboxLog.entryCount >= TREND_COUNT)
        {
            // Calculate trends

            dryboxLog.HumidityTrend = calculateTrend(dryboxLog.Humidity);
            dryboxLog.TemperatureTrend = calculateTrend(dryboxLog.Temperature);
            dryboxLog.PressureTrend = calculateTrend(dryboxLog.Pressure);
            dryboxLog.DewPointTrend = calculateTrend(dryboxLog.DewPoint);

        }

        if (++dryboxLog.entryCount >= LOG_SIZE)
        {
            dryboxLog.entryCount = 0;
        }

        // Determine the time of the next log entry.
        nextLog = HAL_GetTick() + LOG_PERIOD;
    }

}

trend_t humidityTrend(void)
{
    return dryboxLog.HumidityTrend;
}

trend_t calculateTrend(const float* array)
{
    /*
     * To trend INCREASING/DECREASING, the value must be moving in the direction for
     * at least TREND_COUNT samples. Otherwise, the value is trending FLAT
     *
     * 24C 25C 26C - increasing
     *
     * 26C 25C 24C - decreasing
     *
     * 24C 26C 25C - flat
     */
    float delta = 0;
    float total = 0;

    if(dryboxLog.entryCount < TREND_COUNT)
        return FLAT;

    for(int i = dryboxLog.entryCount - 1; i > dryboxLog.entryCount - (TREND_COUNT-1); i--)
    {
        if(i > dryboxLog.entryCount - (TREND_COUNT-2))
        {
            delta += array[i] - array[i-1]; // Add the delta between the two entries.
        }
        total += array[i];
    }

    if (delta > ((total/TREND_COUNT) * FLAT_THRESHOLD))
    {
        return INCREASING;
    }
    else if (delta < -((total/TREND_COUNT) * FLAT_THRESHOLD))
    {
        return DECREASING;
    }
    else
    {
        return FLAT;
    }

}

void dataLogInit(void)
{
    // TODO - Restore max/min values from FLASH.
    dryboxLog.maxHumidity = FLT_MIN;
    dryboxLog.minHumidity = FLT_MAX;
    dryboxLog.HumidityTrend = FLAT;

    dryboxLog.maxDewPoint = FLT_MIN;
    dryboxLog.minDewPoint = FLT_MAX;
    dryboxLog.DewPointTrend = FLAT;

    dryboxLog.maxPressure = FLT_MIN;
    dryboxLog.minPressure = FLT_MAX;
    dryboxLog.PressureTrend = FLAT;

    dryboxLog.maxTemperature = FLT_MIN;
    dryboxLog.minTemperature = FLT_MAX;
    dryboxLog.TemperatureTrend = FLAT;

    dryboxLog.entryCount = 0;

    for (int i = 0; i < LOG_SIZE; ++i)
    {
        dryboxLog.Pressure[i] = 0;
        dryboxLog.DewPoint[i] = 0;
        dryboxLog.Temperature[i] = 0;
        dryboxLog.Humidity[i] = 0;
    }
}

float dataLogGetValue(datatype_t type, trend_t* trend, float *max, float *min)
{
    float *pData = NULL;
    switch(type)
    {
        case TEMPERATURE:
            pData = dryboxLog.Temperature;
            *max = dryboxLog.maxTemperature;
            *min = dryboxLog.minTemperature;
            break;
        case PRESSURE:
            pData = dryboxLog.Pressure;
            *max = dryboxLog.maxPressure;
            *min = dryboxLog.minPressure;
            break;
        case HUMIDITY:
            pData = dryboxLog.Humidity;
            *max = dryboxLog.maxHumidity;
            *min = dryboxLog.minHumidity;
            break;
        case DEWPOINT:
        default:
            pData = dryboxLog.DewPoint;
            *max = dryboxLog.maxDewPoint;
            *min = dryboxLog.minDewPoint;
            break;
    }

    *trend = calculateTrend(pData);

    return pData[dryboxLog.entryCount-1];

}

