//
// Created by zarthrag on 2/25/20.
//

#ifndef DRYBOX_DISPLAYSTATE_H
#define DRYBOX_DISPLAYSTATE_H

typedef enum displayStatePriority_t { DP_INFO, DP_IMPORTANT, DP_CRITICAL};


char* getInfoString(void);
char* getErrorString(void);
void updateDisplayState(void);
void setInfoDisplayState(uint32_t timeout);
void setAlarmDisplayState(uint32_t timeout);

#endif //DRYBOX_DISPLAYSTATE_H
