#ifndef INC_TASK_MAIN_H_
#define INC_TASK_MAIN_H_

#include "kk_rtc.h"
#include "oled.h"
#include "usart.h"
#include "kk_knob.h"
#include "mpu6050.h"


#include <stdio.h>
#include <string.h>

extern uint32_t stepCount;

void StepCounter_Init(void);
void StepCounter_Update(MPU6050_Accel_t *accel);
void showStep(void);

void MainTaskInit();
void MainTask();

#endif /* INC_TASK_MAIN_H_ */
