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
#include "../LVGL/lvgl.h"
#include "../touch/lcd_touch.h"
#include "../UI/UI.h"
#include "../esp8266/esp8266.h"
#include "../App/app_onenet_mqtt.h"
#include "../Middlewares/EmbATlink/at_driver.h"
#include "../cJSON/cJSON.h"
#include "stdio.h"
#include "stdlib.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FLAME_SAMPLE_INTERVAL_MS 200
#define FLAME_FILTER_SIZE 5
#define FLAME_UPDATE_THRESHOLD 5

#define LED_ON_THRESHOLD_0 500
#define LED_ON_THRESHOLD_1 1024
#define LED_ON_THRESHOLD_2 2048
#define LED_ON_THRESHOLD_3 3072
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMutexId lvglMutexHandle;

extern char CurrentMode;
extern struct ThresholdT_t Getthreshold;
extern char ThresholdCommit;
extern lv_obj_t *FlamestateLabel;
extern lv_obj_t *buzzerstateLabel;
extern lv_obj_t *SliderLabel;
extern lv_obj_t *buzzerSlider;
extern lv_obj_t *MqttConnectState;
extern lv_obj_t *MqttConnectBtn;
extern lv_obj_t *Msgbox;
extern lv_obj_t *ADType;
extern lv_obj_t *IOType;
extern lv_obj_t *ThresholdADvalpanel;
extern lv_obj_t *ThresholdIOvalpanel;
extern lv_obj_t *UsrCtl;
extern lv_obj_t *SmartCtl;
extern lv_obj_t *ThresholdSureBtn;
extern lv_obj_t *ThresholdCancelBtn;

extern uint16_t flame_val;
extern uint16_t flame_valIO;
extern volatile uint8_t cmdflag;
extern uint16_t buzzerCurrentstatus;
extern volatile uint8_t mqttconnetState;
extern uint8_t touch_event;

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim;

uint16_t flame_val_filtered;
uint16_t flame_val_last_display;
uint16_t flame_history[FLAME_FILTER_SIZE];
uint8_t flame_history_idx;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId lvglTaskHandle;
osThreadId flameTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartDefaultTask(void const * argument);
void StartLvglTask(void const * argument);
void StartFlameTask(void const * argument);
void UpdateLEDsByFlameValue(uint16_t value);
void TimerUpdateData(void);
void Command_Parsing(void);
uint8_t onenet_mqtt_user_handle(onenet_pub_data_t *data);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartLvglTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  osMutexDef(lvglMutex);
  lvglMutexHandle = osMutexCreate(osMutex(lvglMutex));
  if (lvglMutexHandle == NULL) {
    Error_Handler();
  }
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(lvglTask, StartLvglTask, osPriorityAboveNormal, 0, 4096);
  lvglTaskHandle = osThreadCreate(osThread(lvglTask), NULL);

  osThreadDef(flameTask, StartFlameTask, osPriorityNormal, 0, 1024);
  flameTaskHandle = osThreadCreate(osThread(flameTask), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void StartLvglTask(void const * argument)
{
  static uint8_t initialReportDone = 0;
  if (lvglMutexHandle == NULL) {
    for(;;) {
      osDelay(100);
    }
  }
  for(;;)
  {
    gt911_scanning_event();

    osMutexWait(lvglMutexHandle, osWaitForever);
    lv_timer_handler();

    static uint8_t last_mqttState = NOT;
    if (popup_dismissed == 0 && mqttconnetState == DISCONNECTED) {
      mqttconnetState = NOT;
      lv_obj_t *text = lv_msgbox_get_text(Msgbox);
      lv_label_set_text(text, "MQTT DISCONNECTED");
      lv_obj_clear_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(MqttConnectState, "未连接");
      lv_obj_align_to(MqttConnectState, MqttConnectBtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
      lv_obj_add_flag(MqttConnectBtn, LV_OBJ_FLAG_CLICKABLE);
    }
    if (mqttconnetState == CONNECTED && last_mqttState != CONNECTED) {
      lv_obj_add_flag(Msgbox, LV_OBJ_FLAG_HIDDEN);
      popup_dismissed = 0;
    }
    last_mqttState = mqttconnetState;
    osMutexRelease(lvglMutexHandle);

    osDelay(5);
  }
}

void StartFlameTask(void const * argument)
{
  for(;;)
  {
    TimerUpdateData();
    UpdateLEDsByFlameValue(flame_val);
    if (cmdflag == 1) {
      cmdflag = 0;
      Command_Parsing();
    }
    osDelay(200);
  }
}

void UpdateLEDsByFlameValue(uint16_t value)
{
  HAL_GPIO_WritePin(GPIOG, LED1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, LED2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, LED3_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET);

  if (value >= LED_ON_THRESHOLD_3) {
    HAL_GPIO_WritePin(GPIOG, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, LED2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, LED3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
  } else if (value >= LED_ON_THRESHOLD_2) {
    HAL_GPIO_WritePin(GPIOG, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, LED2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, LED3_Pin, GPIO_PIN_SET);
  } else if (value >= LED_ON_THRESHOLD_1) {
    HAL_GPIO_WritePin(GPIOG, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, LED2_Pin, GPIO_PIN_SET);
  } else if (value > LED_ON_THRESHOLD_0) {
    HAL_GPIO_WritePin(GPIOG, LED1_Pin, GPIO_PIN_SET);
  }
}

void TimerUpdateData(void)
{
  uint8_t IsSmartMode;

  if ((CurrentMode == SMART) && (ThresholdCommit == 1)) {
    IsSmartMode = 1;
  } else {
    IsSmartMode = 0;
  }

  if (Getthreshold.AD_OR_IO == AD)
  {
    HAL_ADC_Start(&hadc1);
    if (HAL_OK == HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY))
    {
      uint16_t raw_val = HAL_ADC_GetValue(&hadc1);

      flame_history[flame_history_idx] = raw_val;
      flame_history_idx = (flame_history_idx + 1) % FLAME_FILTER_SIZE;

      uint32_t sum = 0;
      for (uint8_t i = 0; i < FLAME_FILTER_SIZE; i++) {
        sum += flame_history[i];
      }
      flame_val_filtered = sum / FLAME_FILTER_SIZE;
      flame_val = flame_val_filtered;

      if (abs(flame_val_filtered - flame_val_last_display) >= FLAME_UPDATE_THRESHOLD)
      {
        osMutexWait(lvglMutexHandle, osWaitForever);
        lv_label_set_text_fmt(FlamestateLabel, "火焰传感器:%d", flame_val_filtered);
        osMutexRelease(lvglMutexHandle);
        flame_val_last_display = flame_val_filtered;
      }
    }
    if (CurrentMode == SMART) {
      if (flame_val >= Getthreshold.buzzerOnlevel) {
        buzzerCurrentstatus = 2000;
      }
      if (flame_val <= Getthreshold.buzzerofflevel) {
        buzzerCurrentstatus = 1;
      }
    }
  }
  else if (Getthreshold.AD_OR_IO == IO)
  {
    GPIO_PinState PinState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);
    if (PinState == GPIO_PIN_SET) {
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text(FlamestateLabel, "火焰传感器:有火焰");
      osMutexRelease(lvglMutexHandle);
      flame_valIO = 1;
      flame_val = 4095;
      if (CurrentMode == SMART) {
        buzzerCurrentstatus = 2000;
      }
    }
    else if (PinState == GPIO_PIN_RESET)
    {
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text(FlamestateLabel, "火焰传感器:无火焰");
      osMutexRelease(lvglMutexHandle);
      flame_valIO = 0;
      flame_val = 0;
      if (CurrentMode == SMART) {
        buzzerCurrentstatus = 1;
      }
    }
  }

  static uint16_t last_buzzer_status = 0;
  if (IsSmartMode && buzzerCurrentstatus != last_buzzer_status) {
    HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
    htim.Instance->ARR = 1000000 / buzzerCurrentstatus;
    if (buzzerCurrentstatus > 20) {
      __HAL_TIM_SetCompare(&htim, TIM_CHANNEL_1, 1000000 / buzzerCurrentstatus / 2);
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text(buzzerstateLabel, "报警器状态:打开");
      osMutexRelease(lvglMutexHandle);
    } else {
      __HAL_TIM_SetCompare(&htim, TIM_CHANNEL_1, 0);
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text(buzzerstateLabel, "报警器状态:关闭");
      osMutexRelease(lvglMutexHandle);
    }
    osMutexWait(lvglMutexHandle, osWaitForever);
    lv_slider_set_value(buzzerSlider, buzzerCurrentstatus, LV_ANIM_OFF);
    lv_label_set_text_fmt(SliderLabel, "%dHz", buzzerCurrentstatus);
    osMutexRelease(lvglMutexHandle);
    HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1);
    last_buzzer_status = buzzerCurrentstatus;
  }

  if (mqttconnetState == CONNECTED)
  {
    static uint16_t upload_tick = 0;
    
    onenet_data[0].value.f32_val = (float)flame_val;
    onenet_data[1].value.i32_val = (int32_t)buzzerCurrentstatus;
    onenet_data[2].value.bool_val = (CurrentMode == USR) ? 1 : 0;
    onenet_data[3].value.bool_val = (Getthreshold.AD_OR_IO == IO) ? 1 : 0;
    
    if (++upload_tick >= 25) {  // 200ms*25=5秒定时上传
      onenet_mqtt_publish_post_data(onenet_data, ONENET_DATA_NUM);
      upload_tick = 0;
      // 上传时同步更新LCD，确保显示一致
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text_fmt(FlamestateLabel, "火焰传感器:%d", (int)flame_val);
      lv_slider_set_value(buzzerSlider, buzzerCurrentstatus, LV_ANIM_OFF);
      lv_label_set_text_fmt(SliderLabel, "%dHz", buzzerCurrentstatus);
      osMutexRelease(lvglMutexHandle);
      flame_val_last_display = flame_val;
    }
  }
}

uint8_t onenet_mqtt_user_handle(onenet_pub_data_t *data)
{
  if (strcmp(data->identifier, "beep") == 0)
  {
    int buzzercmd = data->value.u32_val;
    buzzerCurrentstatus = buzzercmd;
    HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
    htim.Instance->ARR = 1000000 / buzzercmd;
    if (buzzercmd > 20) {
      __HAL_TIM_SetCompare(&htim, TIM_CHANNEL_1, 1000000 / buzzercmd / 2);
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text(buzzerstateLabel, "报警器状态:打开");
      osMutexRelease(lvglMutexHandle);
    } else {
      __HAL_TIM_SetCompare(&htim, TIM_CHANNEL_1, 0);
      osMutexWait(lvglMutexHandle, osWaitForever);
      lv_label_set_text(buzzerstateLabel, "报警器状态:关闭");
      osMutexRelease(lvglMutexHandle);
    }
    osMutexWait(lvglMutexHandle, osWaitForever);
    lv_slider_set_value(buzzerSlider, buzzercmd, LV_ANIM_OFF);
    lv_label_set_text_fmt(SliderLabel, "%dHz", buzzercmd);
    osMutexRelease(lvglMutexHandle);
    HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1);
  }

  if (strcmp(data->identifier, "aiuser") == 0)
  {
    osMutexWait(lvglMutexHandle, osWaitForever);
    if (data->value.bool_val == 1) {
      CurrentMode = USR;
      lv_obj_add_state(UsrCtl, LV_STATE_CHECKED);
      lv_obj_clear_state(SmartCtl, LV_STATE_CHECKED);
      lv_obj_add_flag(SmartCtl, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(UsrCtl, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_state(buzzerSlider, LV_STATE_DISABLED);
      lv_obj_add_state(ThresholdSureBtn, LV_STATE_DISABLED);
      lv_obj_add_state(ThresholdCancelBtn, LV_STATE_DISABLED);
    } else {
      CurrentMode = SMART;
      lv_obj_add_state(SmartCtl, LV_STATE_CHECKED);
      lv_obj_clear_state(UsrCtl, LV_STATE_CHECKED);
      lv_obj_add_flag(UsrCtl, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_clear_flag(SmartCtl, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_state(buzzerSlider, LV_STATE_DISABLED);
      lv_obj_clear_state(ThresholdSureBtn, LV_STATE_DISABLED);
      lv_obj_clear_state(ThresholdCancelBtn, LV_STATE_DISABLED);
    }
    osMutexRelease(lvglMutexHandle);
    HAL_TIM_PWM_Stop(&htim, TIM_CHANNEL_1);
    htim.Instance->ARR = 1000000;
    __HAL_TIM_SetCompare(&htim, TIM_CHANNEL_1, 0);
    HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1);
    buzzerCurrentstatus = 1;
  }

  if (strcmp(data->identifier, "ADIO") == 0)
  {
    if (data->value.bool_val == 1) {
      // true = IO 触发
      lv_obj_add_state(IOType, LV_STATE_CHECKED);
      lv_obj_clear_state(ADType, LV_STATE_CHECKED);
      Getthreshold.AD_OR_IO = IO;
      lv_obj_add_flag(ThresholdADvalpanel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ThresholdIOvalpanel, LV_OBJ_FLAG_HIDDEN);
    } else {
      // false = AD 触发
      lv_obj_add_state(ADType, LV_STATE_CHECKED);
      lv_obj_clear_state(IOType, LV_STATE_CHECKED);
      Getthreshold.AD_OR_IO = AD;
      lv_obj_add_flag(ThresholdIOvalpanel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(ThresholdADvalpanel, LV_OBJ_FLAG_HIDDEN);
    }
  }

  return 0;
}

void Command_Parsing(void)
{
  uint8_t ret = 0;
  onenet_mqtt_topic_data_t onenet_topic_data = {0};
  onenet_mqtt_propertySet_t onenet_propertySet = {0};

  char *str = at_get_recv_buffer(AT_LUN_ESP8266);
  if (str == NULL || str[0] == '\0') return;

  ret = onenet_mqtt_get_topic_data((char *)mqtt_info.subscribe_topic_set, str, &onenet_topic_data);
  if (ret == 0)
  {
    onenet_mqtt_topic_log(&onenet_topic_data);
    ret = onenet_mqtt_parse_propertySet_topic(onenet_topic_data.json_data, &onenet_propertySet);
    if (ret == 0)
    {
      ret = onenet_mqtt_user_handle(&onenet_propertySet.data);
      onenet_mqtt_response_topic(onenet_propertySet.msg_id, 200, "success");
    }
  }

  ret = onenet_mqtt_get_topic_data((char *)mqtt_info.subscribe_topic_post, str, &onenet_topic_data);
  if (ret == 0)
    onenet_mqtt_topic_log(&onenet_topic_data);

  at_clear_recv_buffer(AT_LUN_ESP8266);
}
/* USER CODE END Application */