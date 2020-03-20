//
// Created by zarthrag on 2/27/20.
//

#ifndef DRYBOX_HEATERFSM_H
#define DRYBOX_HEATERFSM_H

enum HeaterState_t { HEATER_OFF, HEATER_ON, HEATER_LIMIT };

enum ControlState_t {
    FSM_NONE,             // NULL state
    FSM_INIT,             // Starting state.
    FSM_TEMP_HEAT,        // Active heating state
    FSM_TEMP_TARGET,      // Temperature is at target
    FSM_TEMP_LIMIT,       // System is approaching temp limit.
    FSM_TEMP_OVERHEAT,    // System is too hot
    FSM_FATAL             // System is in an error state.
};

// Returns the state of the heater in the FSM.
enum HeaterState_t getHeaterState(void);

// Returns the state of the FSM.
enum ControlState_t getSystemState(void);

// Returns additional state data
char* getSystemStateString(void);

// Returns a % estimation of dessicant depletion.
float getDessicantState(void);

// Request to perform a drying cycle, likely in response to a filament change.
void cycleRequest(void);

// Request to perform a drying cycle, in response to new dessicant.
void dessicantReset(void);

#endif //DRYBOX_HEATERFSM_H
