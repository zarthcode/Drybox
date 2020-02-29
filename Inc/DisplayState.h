//
// Created by zarthrag on 2/25/20.
//

#ifndef DRYBOX_DISPLAYSTATE_H
#define DRYBOX_DISPLAYSTATE_H

char* getInfoString(void);
char* getErrorString(void);
void updateDisplayState(void);
void setInfoDisplayState(uint32_t timeout);
void setErrorDisplayState(uint32_t timeout);

#endif //DRYBOX_DISPLAYSTATE_H
