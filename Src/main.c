/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <DisplayState.h>
#include "LCD.h"
#include "bme280.h"
#include "stdio.h"
#include "dataLog.h"
#include <bme280_interface.h>
#include <HeaterFSM.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum blinkSpeed_t {OFF, SLOW, FAST};
enum pressType_t {BTN_OFF, BTN_HELD, BTN_DEBOUNCE, BTN_SHORT, BTN_LONG };
enum buzzerState_t { BUZZ_OFF, BUZZ_BUZZING, BUZZ_PAUSING, BUZZ_SHORT, BUZZ_LONG, BUZZ_DOUBLE };

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
enum blinkSpeed_t blinkInterval = FAST;

struct {
    uint32_t pressStart;
    uint32_t pressEnd;
    enum pressType_t pressType;
} buttonB1_state;
uint32_t nextBlink = 0;

struct {
    enum buzzerState_t state;
    uint32_t freq;
    uint32_t timeout;
    uint32_t beepsRemaining;
    uint32_t duration;
} buzzerState;

#define BUTTON_DEBOUNCE 150
#define BUTTON_LONG 2000

#define SCREEN_INTERVAL 4000
#define LOG_INTERVAL 1000


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM1_Init(void);
void ShortBuzz(void);
void LongBuzz(void);
void DoubleBuzz(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ShortBuzz(void)
{
    if(buzzerState.state == BUZZ_OFF)
    {
        buzzerState.state = BUZZ_SHORT;
    }

}

void LongBuzz(void)
{
    if(buzzerState.state == BUZZ_OFF)
    {
        buzzerState.state = BUZZ_LONG;
    }
}

void DoubleBuzz(void)
{
    if(buzzerState.state == BUZZ_OFF)
    {
        buzzerState.state = BUZZ_DOUBLE;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint32_t nextLogUpdate = 0;
  uint32_t nextScreenUpdate = SCREEN_INTERVAL;

  buttonB1_state.pressType = BTN_OFF;
  buttonB1_state.pressEnd = 0;
  buttonB1_state.pressStart = 0;

  buzzerState.state = BUZZ_OFF;
  buzzerState.timeout = 0;
  buzzerState.freq = 1;
  buzzerState.beepsRemaining = 0;

  enum HeaterState_t heaterState = HEATER_OFF;

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  dataLogInit();

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  sprintf(getInfoString(),"\f  Black Widow\nDrybox Restarted");
  setInfoDisplayState(4000);

  // fflush(stdout);   // Either fflush(stdout) or stdbuf(stdout, NULL) needs to be called for printf to work properly.


  // Initialize the bme280
  s32 result = bme280_interface_init();
  if (result != 0)
    DoubleBuzz();
  else
    LongBuzz();

    /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

      // Watchdog Timer Reset
      // HAL_WWDG_Refresh(&hwwdg);

      // Update the data log and FSM
      if(nextLogUpdate < HAL_GetTick())
      {
          dataLogUpdate();
          // bme280_data_t data;
          // result = bme280_interface_get_data(&data);
          // printf("\f%5.2f  %5.2f\n%5.2f  %5.2f", data.humidity, data.temperature, data.dewPoint, data.pressure);
          nextLogUpdate = HAL_GetTick() + LOG_INTERVAL;
      }

      // Update the screen
      if(nextScreenUpdate < HAL_GetTick())
      {
          updateDisplayState();
          nextScreenUpdate = HAL_GetTick() + SCREEN_INTERVAL;
      }

      // Buzzer State
      // HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);

      // B1 - User button state
      // Short Press - Cycle Request
      // Long Press - Dessicant Reset
      switch (buttonB1_state.pressType)
      {
          case BTN_OFF:
              if(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET)
              {
                  // Button has been pressed.
                  buttonB1_state.pressType = BTN_HELD;
                  buttonB1_state.pressStart = HAL_GetTick();
              }
              break;
          case BTN_HELD:
              if(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
              {
                  // Button has been released.
                  buttonB1_state.pressEnd = HAL_GetTick();
                  buttonB1_state.pressType = BTN_DEBOUNCE;
              }
              break;
          case BTN_DEBOUNCE:
              if(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET)
              {
                  // Button has bounced.
                  buttonB1_state.pressType = BTN_HELD;
                  // TODO - Beep after BUTTON_LONG threshold.
              }
              else if (buttonB1_state.pressEnd + BUTTON_DEBOUNCE < HAL_GetTick())
              {
                  // Button is settled.
                  uint32_t pressTime = buttonB1_state.pressEnd - buttonB1_state.pressStart;
                  if(pressTime > BUTTON_LONG)
                  {
                      buttonB1_state.pressType = BTN_LONG;
                  }
                  else
                  {
                      buttonB1_state.pressType = BTN_SHORT;
                      // TODO - Short beep.
                  }
              }
              break;
          case BTN_SHORT:
              blinkInterval = blinkInterval == FAST ? SLOW : FAST;
              buttonB1_state.pressType = BTN_OFF;
              cycleRequest();
              ShortBuzz();
              break;
          case BTN_LONG:
              blinkInterval = OFF;
              buttonB1_state.pressType = BTN_OFF;
              dessicantReset();
              LongBuzz();
              break;
          default:
              buttonB1_state.pressType = BTN_OFF;
              buttonB1_state.pressStart = 0;
              buttonB1_state.pressEnd = 0;
      }

      // Buzzer State
      switch(buzzerState.state)
      {
          case BUZZ_OFF:
              // Nothing to do.
              break;
          case BUZZ_SHORT:
              buzzerState.duration = 250;
              buzzerState.beepsRemaining = 1;
              buzzerState.timeout = 0;
              buzzerState.freq = 1;
              buzzerState.state = BUZZ_PAUSING;
              break;
          case BUZZ_LONG:
              buzzerState.duration = 500;
              buzzerState.beepsRemaining = 1;
              buzzerState.timeout = 0;
              buzzerState.freq = 1;
              buzzerState.state = BUZZ_PAUSING;
              break;
          case BUZZ_DOUBLE:
              buzzerState.duration = 250;
              buzzerState.beepsRemaining = 2;
              buzzerState.timeout = 0;
              buzzerState.freq = 1;
              buzzerState.state = BUZZ_PAUSING;
              break;
          case BUZZ_BUZZING:
              // Check timeout
              if(buzzerState.timeout < HAL_GetTick())
              {
                  // Stop buzzing.
                  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

                  // Pause if there is more beeping to do.
                  if(--buzzerState.beepsRemaining >= 1)
                  {
                      buzzerState.state = BUZZ_PAUSING;
                      buzzerState.timeout = HAL_GetTick() + 200;
                  }
                  else
                  {
                      buzzerState.state = BUZZ_OFF;
                  }

              }
              break;
          case BUZZ_PAUSING:
              //
              if(buzzerState.timeout < HAL_GetTick())
              {

                  // Start buzzing.
                  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

                  // Pause if there is more beeping to do.
                  buzzerState.state = BUZZ_BUZZING;
                  buzzerState.timeout = HAL_GetTick() + buzzerState.duration;

              }

              break;
          default:
              break;

      }


      // Blinkin' LED
      unsigned int interval = 0;
      switch(blinkInterval)
      {
          case OFF:
              HAL_GPIO_WritePin(LD3_GPIO_Port,LD3_Pin,GPIO_PIN_RESET);
              break;
          case SLOW:
              interval = 1000;
              break;
          case FAST:
              interval = 500;
              break;
          default:
              interval = 5000;
              break;
      }
      if(blinkInterval != OFF)
      {
          if(nextBlink < HAL_GetTick())
          {
              // HAL_GPIO_TogglePin(LD3_GPIO_Port,LD3_Pin);
              nextBlink = HAL_GetTick() + interval;
          }

      }
      HAL_GPIO_TogglePin(LD3_GPIO_Port,LD3_Pin);

      // Heater Control FSM
      enum HeaterState_t newHeaterState = getHeaterState();
      if(newHeaterState != heaterState)
      {
          heaterState = newHeaterState;
          switch (heaterState)
          {
              case HEATER_ON:
                  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_SET);
                  break;
              // case HEATER_LIMIT:
              case HEATER_OFF:
              default:
                  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
                  break;
          }
      }

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

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x2010091A;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 24000;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 2;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LD4_Pin|LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin|LCD_RW_Pin|LCD_E_Pin|LCD_D4_Pin 
                          |LCD_D5_Pin|LCD_D6_Pin|LCD_D7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RS_Pin LCD_RW_Pin LCD_E_Pin LCD_D4_Pin 
                           LCD_D5_Pin LCD_D6_Pin LCD_D7_Pin */
  GPIO_InitStruct.Pin = LCD_RS_Pin|LCD_RW_Pin|LCD_E_Pin|LCD_D4_Pin 
                          |LCD_D5_Pin|LCD_D6_Pin|LCD_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  LCD_printf( "* FATAL ERROR *");
  while(1) {};

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
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
