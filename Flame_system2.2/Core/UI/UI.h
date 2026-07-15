/*
 * UI.h
 *
 *  Created on: Jul 1, 2024
 *      Author: W
 */
#ifndef UI_UI_H_
#define UI_UI_H_

#include <stdint.h>

enum SensorType{
	IO,
	AD
};

enum ModeType{
	USR,
	SMART,
};

enum MQTTState{
	DISCONNECTED,
	CONNECTED,
	NOT
};

struct ThresholdT_t
{
    char AD_OR_IO;
    short buzzerofflevel;
    short buzzerOnlevel;
};

extern volatile uint8_t mqttconnetState;
extern volatile uint8_t popup_dismissed;

void UI_Init();

#endif /* UI_UI_H_ */