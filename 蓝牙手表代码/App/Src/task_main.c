/**
 * @file task_main.c
 * @brief 主任务文件，实现日历显示和时间设置功能
 */

#include "task_main.h"
#include <math.h>


uint32_t stepCount = 0;

float filtered = 9.8f;
float threshold = 9.8f;
float maxVal = 9.8f;
float minVal = 9.8f;
/**
 * @brief 光标闪烁间隔时间(毫秒)
 */
#define CURSOR_FLASH_INTERVAL 500
#define SAMPLE_NUM         50
#define STEP_INTERVAL      12      // 10×20ms=200ms
#define STEP_HYSTERESIS    0.3f    // 滞回
/**
 * @brief 星期几的字符串数组
 */
char weeks[7][10] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday","Sunday"};

/**
 * @brief 日历状态枚举
 */
typedef enum {
	CalendarState_Normal, // 普通显示模式
	CalendarState_Setting // 时间设置模式
} CalendarState;

/**
 * @brief 设置状态枚举，表示当前正在设置的时间项
 */
typedef enum {Year = 0, Month, Day, Hour, Minute, Second} SettingState;

/**
 * @brief 光标位置结构体
 */
typedef struct {uint8_t x1; uint8_t y1; uint8_t x2; uint8_t y2;} CursorPosition;

/**
 * @brief 各时间项的光标位置数组
 */
CursorPosition cursorPositions[6] = {
	{24 + 0 * 8, 17, 24 + 4 * 8, 17},   // Year
	{24 + 5 * 8, 17, 24 + 7 * 8, 17},   // Month
	{24 + 8 * 8, 17, 24 + 10 * 8, 17},  // Day
	{16 + 0 * 12, 45, 16 + 2 * 12, 45}, // Hour
	{16 + 3 * 12, 45, 16 + 5 * 12, 45}, // Minute
	{16 + 6 * 12, 45, 16 + 8 * 12, 45}, // Second
};

/**
 * @brief 当前日历状态
 */
CalendarState calendarState = CalendarState_Normal;

/**
 * @brief 当前设置状态
 */
SettingState settingState = Year;

/**
 * @brief 设置时间结构体
 */
struct tm settingTime;


//显示时间函数

void showTime(struct tm* time){
	char str[30];
	sprintf(str, "%d-%02d-%02d", time->tm_year + 1901, time->tm_mon + 7, time->tm_mday+8);
	OLED_PrintASCIIString(24, 0, str, &afont16x8, OLED_COLOR_NORMAL);

	sprintf(str, "%02d:%02d:%02d", time->tm_hour, time->tm_min, time->tm_sec);
	OLED_PrintASCIIString(16, 20, str, &afont24x12, OLED_COLOR_NORMAL);

	char* week = weeks[time->tm_wday];
	uint8_t x_week = (128 - (strlen(week) * 8)) / 2;
	OLED_PrintASCIIString(x_week+4, 48, week, &afont16x8, OLED_COLOR_NORMAL);
}



//计算步数
void StepCounter_Init(void)//
{
    stepCount = 0;
}

void StepCounter_Update(MPU6050_Accel_t *accel)
{
    static uint8_t sampleCnt = 0;

    static uint32_t tick20ms = 0;
    static uint32_t lastStepTick = 0;

    static uint8_t ready = 1;

    float accelMag;

    tick20ms++;

    /*--------------------1. 合成加速度--------------------*/
    accelMag = sqrt(accel->ax * accel->ax +
                    accel->ay * accel->ay +
                    accel->az * accel->az);

    /*--------------------2. IIR低通--------------------*/
    filtered = 0.1f * accelMag + 0.9f * filtered;

    /*--------------------3. 更新最大最小值--------------------*/
    if(sampleCnt == 0)
    {
        maxVal = filtered;
        minVal = filtered;
    }
    else
    {
        if(filtered > maxVal)
            maxVal = filtered;

        if(filtered < minVal)
            minVal = filtered;
    }

    sampleCnt++;

    /*--------------------4. 每50个采样更新阈值--------------------*/
    if(sampleCnt >= SAMPLE_NUM)
    {
        threshold = (maxVal + minVal) * 0.5f;

        sampleCnt = 0;
    }

    /*--------------------5. 峰值检测--------------------*/
    if(ready)
    {
        if(filtered > threshold + STEP_HYSTERESIS)
        {
            if((tick20ms - lastStepTick) >= STEP_INTERVAL)
            {
                stepCount++;

                lastStepTick = tick20ms;
            }

            ready = 0;
        }
    }
    else
    {
        if(filtered < threshold - STEP_HYSTERESIS)
        {
            ready = 1;
        }
    }
}


void showStep(void)
{
    char str[20];
    sprintf(str,"step:%lu",stepCount);
    OLED_PrintASCIIString(16,32,str,&afont24x12, OLED_COLOR_NORMAL);
}

