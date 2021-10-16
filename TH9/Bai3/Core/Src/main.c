/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include "string.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* FTP variables */
char APNCommand[100] = {0};
char FTPCommand[100] = {0};
char FTPServer[50] = "yourhost";
char FTPPort[6] = "21";
char Username[32] = "user";
char Pass[32] = "pass";

/* Sim variables */
char Sim_response[500] = {0};
char Sim_Rxdata[2] = {0};

/* File variables */
char fileName[20] = "yourfile.txt";
char filePath[50] = "/public_html/DATA/";
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void display(void* data)																								
{
	HAL_UART_Transmit(&huart2,(uint8_t *)data,(uint16_t)strlen(data),1000);
}

void deleteBuffer(char* buf)
{
	int len = strlen(buf);
	for(int i = 0; i < len; i++)
	{
		buf[i] = 0;
	}
}

int8_t Sim_sendCommand(char* command,char* response,uint32_t timeout)
{
	uint8_t answer = 0, count  = 0;
	deleteBuffer(Sim_response);
	uint32_t time = HAL_GetTick();
	uint32_t time1 = HAL_GetTick();
	HAL_UART_Transmit(&huart1, (uint8_t *)command, (uint16_t)strlen(command), 1000);
	HAL_UART_Transmit(&huart1,(uint8_t *)"\r\n",(uint16_t)strlen("\r\n"),1000);
	do
	{
		while(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata, 1, 1000) != HAL_OK)
		{
			if(HAL_GetTick() - time > timeout) 
			{
				return 0;
			}
		}
                time = HAL_GetTick();
		Sim_response[count++] = Sim_Rxdata[0];
		while((HAL_GetTick() - time < timeout))
		{
			if(HAL_UART_Receive(&huart1, (uint8_t *)Sim_Rxdata, 1, 1000) == HAL_OK)
			{
				Sim_response[count++] = Sim_Rxdata[0];
				time1 = HAL_GetTick();
			}
			else 
			{
				if(HAL_GetTick() - time1 > 100)
				{
					if(strstr(Sim_response,response) != NULL) 
					{
						answer = 1;
					}
					else 
					{
						answer = 0;
					}
					break;
				}
			}
		}
	}
	while(answer == 0);
	display(Sim_response);
	return answer;
}

void Sim_configGPRSnFTP(void)
{
	if(Sim_sendCommand("AT","OK",5000))
	{
		HAL_Delay(10);
		if(Sim_sendCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"","OK",5000))	//Configure GPRS
		{
			HAL_Delay(10);
			if(Sim_sendCommand("AT+SAPBR=3,1,\"APN\",\"CMNET\"","OK",5000))			
			{
				deleteBuffer(APNCommand);
				HAL_Delay(10);
				strcpy(APNCommand,"AT+SAPBR=3,1,\"USER\",\"");	//Set username for APN
				strcat(APNCommand,Username);
				strcat(APNCommand,"\"");
				if(Sim_sendCommand(APNCommand,"OK",5000))
				{
					deleteBuffer(APNCommand);
					HAL_Delay(10);
					strcpy(APNCommand,"AT+SAPBR=3,1,\"PWD\",\"");		//Set password for APN
					strcat(APNCommand,Pass);
					strcat(APNCommand,"\"");
					if(Sim_sendCommand(APNCommand,"OK",5000))
					{
						deleteBuffer(APNCommand);
						HAL_Delay(10);
						while(Sim_sendCommand("AT+SAPBR=1,1","OK",5000) != 1);	//Open bearer									+ BO SUNG TIME OUT
						if(Sim_sendCommand("AT+FTPCID=1","OK",5000))		//Set FTP bearer profile identifier
						{
							strcpy(FTPCommand,"AT+FTPSERV=\"");	//Set FTP server address
							strcat(FTPCommand,FTPServer);
							strcat(FTPCommand,"\"");
							if(Sim_sendCommand(FTPCommand,"OK",5000))
							{
								deleteBuffer(FTPCommand);
								HAL_Delay(10);
								strcpy(FTPCommand,"AT+FTPPORT=");	//Set FTP server port
								strcat(FTPCommand,FTPPort);
								if(Sim_sendCommand(FTPCommand,"OK",5000))
								{
									deleteBuffer(FTPCommand);
									HAL_Delay(10);
									strcpy(FTPCommand,"AT+FTPUN=\"");	//Set FTP username
									strcat(FTPCommand,Username);
									strcat(FTPCommand,"\"");
									if(Sim_sendCommand(FTPCommand,"OK",5000))
									{
										deleteBuffer(FTPCommand);
										HAL_Delay(10);
										strcpy(FTPCommand,"AT+FTPPW=\"");	//Set FTP password
										strcat(FTPCommand,Pass);
										strcat(FTPCommand,"\"");
										if(Sim_sendCommand(FTPCommand,"OK",5000))
										{
											deleteBuffer(FTPCommand);
											HAL_Delay(10);
											display("Configuration done!\r\n");
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}


void Sim_uploadFileFromFTP(char* data)
{
	deleteBuffer(FTPCommand);
	HAL_Delay(10);
	strcpy(FTPCommand,"AT+FTPPUTNAME=\"");	//Set upload file name
	strcat(FTPCommand,fileName);
	strcat(FTPCommand,"\"");
	if(Sim_sendCommand(FTPCommand,"OK",5000))
	{
		deleteBuffer(FTPCommand);
		HAL_Delay(10);
		strcpy(FTPCommand,"AT+FTPPUTPATH=\"");	//Set upload file path
		strcat(FTPCommand,filePath);
		strcat(FTPCommand,"\"");
		if(Sim_sendCommand(FTPCommand,"OK",5000))
		{
			if(Sim_sendCommand("AT+FTPPUT=1","+FTPPUT: 1,1,",30000))
			{
				Sim_sendCommand("AT+FTPPUT=2,10","+FTPPUT: 2,10",30000);
				Sim_sendCommand(data,"+FTPPUT: 1,1",30000);
				Sim_sendCommand("AT+FTPPUT=2,0","+FTPPUT: 1,0",30000);
			}
		}
	}
}

void Sim_downloadFileFromFTP(void)
{
	deleteBuffer(FTPCommand);
	HAL_Delay(10);
	strcpy(FTPCommand,"AT+FTPGETNAME=\"");	//Get file name
	strcat(FTPCommand,fileName);
	strcat(FTPCommand,"\"");
	if(Sim_sendCommand(FTPCommand,"OK",5000))
	{
		deleteBuffer(FTPCommand);
		HAL_Delay(10);
		strcpy(FTPCommand,"AT+FTPGETPATH=\"");	//Get file path
		strcat(FTPCommand,filePath);
		strcat(FTPCommand,"\"");
		if(Sim_sendCommand(FTPCommand,"OK",5000))
		{
			deleteBuffer(FTPCommand);
			HAL_Delay(10);
			if(Sim_sendCommand("AT+FTPGET=1","+FTPGET: 1,1",30000))
			{
				int eof = 0;
				while(eof == 0)
				{
					Sim_sendCommand("AT+FTPGET=2,20","+FTPGET: 2,",30000);
					if(strstr(Sim_response,"+FTPGET: 2,0") != NULL)
					{
						eof = 1;
					}
				}
			}
		}
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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	//upload file
	Sim_configGPRSnFTP();
	Sim_uploadFileFromFTP("a12b34c56d");
        //Sim_downloadFileFromFTP();
	HAL_Delay(3000);
	Sim_sendCommand("AT+SAPBR=0,1","OK",5000);	//Close bearer
  
	//download file
	Sim_configGPRSnFTP();
        //Sim_uploadFileFromFTP("a12b34c56d");
	Sim_downloadFileFromFTP();
	HAL_Delay(3000);
	Sim_sendCommand("AT+SAPBR=0,1","OK",5000);	//Close bearer
	/* USER CODE END 2 */

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
