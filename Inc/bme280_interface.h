//
// Created by zarthrag on 2/25/20.
//

#ifndef DRYBOX_BME280_INTERFACE_H
#define DRYBOX_BME280_INTERFACE_H

#include <sched.h>
#include "bme280.h"

typedef struct {
    float temperature;
    float pressure;
    float humidity;
    float dewPoint;
    uint32_t time;
} bme280_data_t;


s32 bme280_interface_init(void);
s32 bme280_interface_deinit(void);

s32 bme280_interface_get_data(bme280_data_t* data);

#endif //DRYBOX_BME280_INTERFACE_H
