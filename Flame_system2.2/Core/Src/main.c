/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"
//#include "iwdg.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "../LVGL/lvgl.h"
#include "../touch/lcd_touch.h"
#include "../UI/UI.h"
#include "../LVGL/porting/lv_port_disp.h"
#include "../LVGL/porting/lv_port_indev.h"
#include "../esp8266/esp8266.h"
#include "../App/app_onenet_mqtt.h"
#include "../Middlewares/EmbATlink/at_driver.h"
#include "../cJSON/cJSON.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
__IO uint32_t I2CTimeout = I2C_Open_FLAG_TIMEOUT;

extern char CurrentMode;
extern struct ThresholdT_t Getthreshold;
extern char ThresholdCommit;

char recv_at[256] = {0};
char recv_at_cmd[256] = {0};
volatile uint8_t recv_finish = 0;
volatile uint32_t count = 0;
uint16_t flame_val;
uint16_t flame_valIO;
volatile uint8_t cmdflag = 0;
uint16_t buzzerCurrentstatus = 1;
volatile uint8_t mqttconnetState = NOT;
volatile uint8_t popup_dismissed = 0;
uint8_t *esp8266_rst = "AT+RST\r\n";

mqtt_info_t mqtt_info = {0};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t chip_id[5] = {0};
  HAL_StatusTypeDef ret = HAL_OK;
  uint8_t value = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART6_UART_Init();
  MX_FSMC_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();
// MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  htim6.Init.Prescaler = 83;
  htim6.Init.Period = 1000;
  HAL_TIM_Base_Init(&htim6);
  printf("=== Flame System Booting ===\n");

  at_init();

  delay_init(168);
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();

  init_gt911(GT911_I2C_SLAVE);
  ret = gt911_read_data(GT_PID_REG, chip_id, 3);
  if (ret == HAL_OK) {
  }
  gt911_send_data(GT_GSTID_REG, &value, 1);

  UI_Init();
  lv_refr_now(NULL);

  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
  __HAL_UART_CLEAR_IDLEFLAG(&huart2);

  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE  int __io_putchar(int ch)

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart6,(uint8_t*)&ch,1,HAL_MAX_DELAY);
    return ch;
}
#endif

void UART2_RxIRQHandler(void)
{
  if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == SET) {
    uint8_t ch = (uint8_t)(huart2.Instance->DR & 0xFF);
    at_uart_recv_handler(AT_LUN_ESP8266, &ch, 1);
    
    if (ch == '>') {
      recv_finish = 1;
    }
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&ch, 1);
  }

  if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE) == SET) {
    recv_finish = 1;
    if (strstr(at_get_recv_buffer(AT_LUN_ESP8266), "MQTTDISCONNECTED") != NULL) {
      mqttconnetState = DISCONNECTED;
    }
    if (strstr(at_get_recv_buffer(AT_LUN_ESP8266), "MQTTSUBRECV") != NULL) {
      cmdflag = 1;
      char *recv_buf = at_get_recv_buffer(AT_LUN_ESP8266);
      size_t len = strnlen(recv_buf, sizeof(recv_at_cmd) - 1);
      memcpy(recv_at_cmd, recv_buf, len + 1);
      recv_at_cmd[len] = '\0';
    }
    __HAL_UART_CLEAR_IDLEFLAG(&huart2);
  }
}

void Set_GPIO_MODER(GPIO_TypeDef* GPIOx, uint32_t moder, uint16_t PinNumber)
{
  assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
  assert_param(IS_GPIO_MODE(moder));

  uint32_t reg = GPIOx->MODER;
  reg &= ~((GPIO_MODER_MODE0_Msk) << (PinNumber * 2U));
  reg |= ((moder) << (PinNumber * 2U));
  GPIOx->MODER = reg;
}

int send_command(char *command, int cmd_len, char *response, char *response_command, int timeout)
{
    recv_finish = 0;
    at_clear_recv_buffer(AT_LUN_ESP8266);
    memset(recv_at, 0, sizeof(recv_at));
    
    if (cmd_len > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t *)command, cmd_len, HAL_MAX_DELAY);
        printf("Send: %s", command);
    }
    
    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < timeout) {
        if (recv_finish == 1) {
            recv_finish = 0;
            char *recv_buf = at_get_recv_buffer(AT_LUN_ESP8266);
            size_t len = strnlen(recv_buf, sizeof(recv_at) - 1);
            memcpy(recv_at, recv_buf, len + 1);
            recv_at[len] = '\0';
            printf("Recv: %s\n", recv_at);
            
            if (response != NULL && strstr(recv_at, response) != NULL) {
                return 0;
            }
            if (response_command != NULL && strstr(recv_at, response_command) != NULL) {
                return 0;
            }
            if (response == NULL && response_command == NULL) {
                return 0;
            }
            return -1;
        }
        HAL_Delay(10);
    }
    printf("Timeout!\n");
    return -1;
}

char* analysis_cmd(char* cmd, char* delim, int count)
{
    static char result[256] = {0};
    char *token = strtok(cmd, delim);
    int i = 0;
    
    while (token != NULL) {
        if (i == count) {
            strncpy(result, token, sizeof(result) - 1);
            result[sizeof(result) - 1] = '\0';
            return result;
        }
        token = strtok(NULL, delim);
        i++;
    }
    return NULL;
}

int Esp8266CheckState(void)
{
    char send_at[32] = {0};
    
    strcpy(send_at, "AT\r\n");
    if (send_command(send_at, strlen(send_at), "OK", NULL, 2000) != 0) {
        strcpy(send_at, "AT+RST\r\n");
        send_command(send_at, strlen(send_at), "OK", NULL, 2000);
        HAL_Delay(1000);
        
        strcpy(send_at, "AT\r\n");
        if (send_command(send_at, strlen(send_at), "OK", NULL, 2000) != 0) {
            return -1;
        }
    }
    return 0;
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */