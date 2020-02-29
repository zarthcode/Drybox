//
// Created by zarthrag on 2/26/20.
//

#ifndef DRYBOX_DATALOG_H
#define DRYBOX_DATALOG_H
#define LOG_SIZE 128

#include <unistd.h>
#include <stdbool.h>


// Expresses the trend of a value
typedef enum { FLAT, DECREASING, INCREASING } trend_t;

// Enum of stored values
typedef enum { TEMPERATURE, PRESSURE, HUMIDITY, DEWPOINT } datatype_t;

// Initializes the data log
void dataLogInit(void);

// Updates the log with a new entry
void dataLogUpdate(void);

// Returns true if there are enough logs for trend data.
bool trendAvailable(void);

// Returns the requested value and trend
float dataLogGetValue(datatype_t type, trend_t* trend, float *max, float *min);

// Returns the humidity trend
trend_t humidityTrend(void);


#endif //DRYBOX_DATALOG_H
