//
// Created by zarthrag on 2/25/20.
//

#ifndef DRYBOX_DISPLAYSTATE_H
#define DRYBOX_DISPLAYSTATE_H

typedef enum displayStatePriority_t { DP_INFO, DP_IMPORTANT, DP_CRITICAL};


void updateDisplayState(void);
void setInfoDisplayState(uint8_t priority, uint32_t timeout);

#endif //DRYBOX_DISPLAYSTATE_H
