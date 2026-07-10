/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "task_main.h"
#include "kk_knob.h"
#include "adc.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t work_mode = 0;
uint8_t oled_mode = 0; //0--菜单 1--功能
struct tm* now ;
uint32_t counter;

//uint16_t adc_val;
//float battery_v;
//
//int v_int;
//int v_dec;

#define MENU_NUM 4
#define COUNTER_INIT_VALUE 65535/2
#define OLED_SLEEP_TIME    10000    //10秒

uint32_t lastOperateTick = 0;
uint8_t oled_sleep = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

void showVoltage(void)
{

	char str[20];

	sprintf(str, "Voltage:%d.%02dV", v_int, v_dec);
	OLED_PrintASCIIString(16,32,str,&afont16x8, OLED_COLOR_NORMAL);
}


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for UITask */
osThreadId_t UITaskHandle;
const osThreadAttr_t UITask_attributes = {
  .name = "UITask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for mpu6050Task */
osThreadId_t mpu6050TaskHandle;
const osThreadAttr_t mpu6050Task_attributes = {
  .name = "mpu6050Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for ServiceTask */
osThreadId_t ServiceTaskHandle;
const osThreadAttr_t ServiceTask_attributes = {
  .name = "ServiceTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartUITask(void *argument);
void Startmpu6050Task(void *argument);
void StartServiceTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of UITask */
  UITaskHandle = osThreadNew(StartUITask, NULL, &UITask_attributes);

  /* creation of mpu6050Task */
  mpu6050TaskHandle = osThreadNew(Startmpu6050Task, NULL, &mpu6050Task_attributes);

  /* creation of ServiceTask */
  ServiceTaskHandle = osThreadNew(StartServiceTask, NULL, &ServiceTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartUITask */
/**
  * @brief  Function implementing the UITask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartUITask */
void StartUITask(void *argument)
{
  /* USER CODE BEGIN StartUITask */
		 HAL_Delay(20);
		 OLED_Init();
		 Knob_Init();


		 lastOperateTick = HAL_GetTick();
		 oled_sleep = 0;

		 for(;;)
		 {
		     counter = __HAL_TIM_GET_COUNTER(&htim1);

		     /*==========================
		       编码器顺时针
		     ==========================*/
		     if(counter >= COUNTER_INIT_VALUE + 4)
		     {
		         /* 如果OLED已经休眠，则先唤醒 */
		         if(oled_sleep)
		         {
		             OLED_DisPlay_On();

		             work_mode = 0;
		             oled_mode = 0;

		             __HAL_TIM_SET_COUNTER(&htim1, COUNTER_INIT_VALUE);

		             lastOperateTick = HAL_GetTick();

		             oled_sleep = 0;

		             continue;
		         }

		         work_mode++;

		         if(work_mode >= MENU_NUM)
		             work_mode = 0;

		         __HAL_TIM_SET_COUNTER(&htim1, COUNTER_INIT_VALUE);

		         lastOperateTick = HAL_GetTick();
		     }

		     /*==========================
		       编码器逆时针
		     ==========================*/
		     else if(counter <= COUNTER_INIT_VALUE - 4)
		     {
		         if(oled_sleep)
		         {
		             OLED_DisPlay_On();

		             work_mode = 0;
		             oled_mode = 0;

		             __HAL_TIM_SET_COUNTER(&htim1, COUNTER_INIT_VALUE);

		             lastOperateTick = HAL_GetTick();

		             oled_sleep = 0;

		             continue;
		         }

		         if(work_mode == 0)
		             work_mode = MENU_NUM - 1;
		         else
		             work_mode--;

		         __HAL_TIM_SET_COUNTER(&htim1, COUNTER_INIT_VALUE);

		         lastOperateTick = HAL_GetTick();
		     }

		     /*==========================
		       按键
		     ==========================*/
		     if(HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
		     {
		         osDelay(10);

		         if(HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
		         {
		             if(oled_sleep)
		             {
		                 OLED_DisPlay_On();

		                 work_mode = 0;
		                 oled_mode = 0;

		                 __HAL_TIM_SET_COUNTER(&htim1, COUNTER_INIT_VALUE);

		                 lastOperateTick = HAL_GetTick();

		                 oled_sleep = 0;

		                 while(HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET);

		                 continue;
		             }

		             oled_mode = !oled_mode;

		             lastOperateTick = HAL_GetTick();

		             while(HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET);
		         }
		     }

		     /*==========================
		       10秒无操作自动熄屏
		     ==========================*/
		     if((HAL_GetTick() - lastOperateTick) >= OLED_SLEEP_TIME)
		     {
		         if(!oled_sleep)
		         {
		             OLED_DisPlay_Off();

		             oled_sleep = 1;

		             /* 恢复默认界面 */
		             work_mode = 0;
		             oled_mode = 0;

		             __HAL_TIM_SET_COUNTER(&htim1, COUNTER_INIT_VALUE);
		         }
		     }

		     /*==========================
		       熄屏后停止刷新OLED
		     ==========================*/
		     if(oled_sleep)
		     {
		         osDelay(20);
		         continue;
		     }

		     /*==========================
		       OLED显示
		     ==========================*/
		     OLED_NewFrame();

		     if(work_mode == 0)
		     {
		         if(oled_mode == 0)
		         {
		             OLED_DrawImage(50,20,&xyyImg,OLED_COLOR_NORMAL);
		             OLED_PrintString(20,0,"xyy智能手表",&font16x16,OLED_COLOR_NORMAL);

		         }
		         else
		         {
		             OLED_PrintString(20,20,"xyy智能手表",&font16x16,OLED_COLOR_NORMAL);
		         }
		     }
		     else if(work_mode == 1)
		     {
		         if(oled_mode == 0)
		         {
//		             OLED_DrawImage(32,10,&timeImg,OLED_COLOR_NORMAL);
		        	 showTime(now);
		         }
		         else
		         {
		             showTime(now);
		         }
		     }
		     else if(work_mode == 2)
		     {
		         if(oled_mode == 0)
		         {
		             OLED_DrawImage(80,10,&countImg,OLED_COLOR_NORMAL);
		             OLED_PrintString(0,0,"步数",&font16x16,OLED_COLOR_NORMAL);
		             showStep();
		         }
		         else
		         {
		             OLED_PrintString(0,0,"步数",&font16x16,OLED_COLOR_NORMAL);
		             showStep();
		         }
		     }
		     else if(work_mode == 3)
		     {
		         if(oled_mode == 0)
		         {
		             OLED_DrawImage(32,10,&gameImg,OLED_COLOR_NORMAL);
		             OLED_PrintString(0,0,"I love Embedded Systems",&font16x16,OLED_COLOR_NORMAL);
		         }
		         else
		         {
		             OLED_PrintString(0,0,"I love Embedded Systems",&font16x16,OLED_COLOR_NORMAL);
//		             showVoltage();
		         }
		     }

		     OLED_ShowFrame();

		     osDelay(10);
		 }
  /* USER CODE END StartUITask */
}

/* USER CODE BEGIN Header_Startmpu6050Task */
/**
* @brief Function implementing the mpu6050Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Startmpu6050Task */
void Startmpu6050Task(void *argument)
{
  /* USER CODE BEGIN Startmpu6050Task */
	MPU6050_Accel_t accel;
	MPU6050_Init();
	StepCounter_Init();
  /* Infinite loop */
  for(;;)
  {
	  //计算步数
	  if(MPU6050_ReadAccel(&accel)==0)
	  {

	    StepCounter_Update(&accel);

	   }
	  osDelay(20);
  }
  /* USER CODE END Startmpu6050Task */
}

/* USER CODE BEGIN Header_StartServiceTask */
/**
* @brief Function implementing the ServiceTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartServiceTask */
void StartServiceTask(void *argument)
{
  /* USER CODE BEGIN StartServiceTask */
	KK_RTC_Init();
	char txBuf[32];
  /* Infinite loop */
  for(;;)
  {
	  //时间读取
	  now = KK_RTC_GetTime();

	  //蓝牙发送

		  sprintf(txBuf,"STEP:%lu\r\n",stepCount);
		  HAL_UART_Transmit(&huart2, (uint8_t *)txBuf, strlen(txBuf), HAL_MAX_DELAY);
		  osDelay(500);      //500ms发送一次

  }
  /* USER CODE END StartServiceTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

