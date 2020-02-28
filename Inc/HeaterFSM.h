//
// Created by zarthrag on 2/27/20.
//

#ifndef DRYBOX_HEATERFSM_H
#define DRYBOX_HEATERFSM_H

typedef enum HeaterState_t { HEATER_OFF, HEATER_ON, HEATER_LIMIT, DESSICANT_LIMIT};

enum controlState_t getHeaterState();

// Returns a % estimation of dessicant depletion.
float getDessicantState(void);

// Request to perform a drying cycle, likely in response to a filament change.
void cycleRequest(void);

// Request to perform a drying cycle, in response to new dessicant.
void dessicantReset(void);

#endif //DRYBOX_HEATERFSM_H
