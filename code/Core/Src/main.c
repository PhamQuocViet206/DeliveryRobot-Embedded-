/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NODE_COUNT 16
#define QUEUE_SIZE 16
#define RX_BUFFER_SIZE 32
#define EDGE_TIME_MS  5000
#define TURN_TIME_MS   700
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

const char nodeName[NODE_COUNT] = {
    'S','P','Q','A',
    'R','T','U','V',
    'W','B','X','Y',
    'Z','M','N','C'
};

typedef struct {
    char data[QUEUE_SIZE];
    int front;
    int rear;
    int count;
} TaskQueue;

TaskQueue taskQueue = { .front = 0, .rear = 0, .count = 0 };

uint8_t rxByte;

volatile char rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxHead = 0;
volatile uint8_t rxTail = 0;
volatile uint8_t rxOverflow = 0;
volatile char pendingCommand = 0;
volatile uint32_t uartRxCount = 0;

typedef enum {
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3
} Direction;

Direction robotDir = EAST;
int currentNode = 0;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
bool Queue_Push(char destination);
bool Queue_Pop(char *destination);

int NodeIndex(int x, int y);
bool AreNeighbors(int a, int b);
int BFS(int start, int goal, int path[NODE_COUNT]);

bool UART_GetCommand(char *command);
void ProcessIncomingCommands(void);
bool FollowPath(int path[], int pathLength);
int DestinationIndex(char destination);
void ExecuteDelivery(char destination);
bool MoveToNeighbor(int nextNode);
void UART_Print(const char *message);
void PrintQueue(void);

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void UART_Print(const char *message)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)message, (uint16_t)strlen(message), HAL_MAX_DELAY);
}

void PrintQueue(void)
{
    char message[96];
    int pos = 0;

    pos += snprintf(message + pos,
                    sizeof(message) - (size_t)pos,
                    "QUEUE (%d):", taskQueue.count);

    for (int i = 0; i < taskQueue.count && pos < (int)sizeof(message) - 4; i++)
    {
        int index = (taskQueue.front + i) % QUEUE_SIZE;
        pos += snprintf(message + pos, sizeof(message) - pos, " %c", taskQueue.data[index]);
    }

    if (taskQueue.count == 0)
        pos += snprintf(message + pos, sizeof(message) - pos, " EMPTY");

    snprintf(message + pos, sizeof(message) - pos, "\r\n");
    UART_Print(message);
}

void MotorA_Forward(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 500);
}

void MotorA_Reverse(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 500);
}

void MotorA_Stop(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
}

void MotorB_Forward(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 500);
}

void MotorB_Reverse(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 500);
}

void MotorB_Stop(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
}

void Robot_Forward(void)
{
    MotorA_Forward();
    MotorB_Forward();
}

void Robot_Reverse(void)
{
    MotorA_Reverse();
    MotorB_Reverse();
}

void Robot_Stop(void)
{
    MotorA_Stop();
    MotorB_Stop();
}

void Robot_TurnLeft(void)
{
    MotorA_Stop();
    MotorB_Forward();
}

void Robot_TurnRight(void)
{
    MotorA_Forward();
    MotorB_Stop();
}

void TurnLeftTimed(void)
{
    Robot_TurnLeft();
    HAL_Delay(TURN_TIME_MS);
    Robot_Stop();
    HAL_Delay(200);

    robotDir = (Direction)((robotDir + 3) % 4);
}

void TurnRightTimed(void)
{
    Robot_TurnRight();
    HAL_Delay(TURN_TIME_MS);
    Robot_Stop();
    HAL_Delay(200);

    robotDir = (Direction)((robotDir + 1) % 4);
}

void TurnAroundTimed(void)
{
    TurnRightTimed();
    TurnRightTimed();
}

void MoveOneEdge(void)
{
    Robot_Forward();
    HAL_Delay(EDGE_TIME_MS);
    Robot_Stop();
    HAL_Delay(200);
}

bool Queue_Push(char destination)
{
    if (taskQueue.count >= QUEUE_SIZE)
        return false;

    if (destination != 'A' &&
        destination != 'B' &&
        destination != 'C')
        return false;

    taskQueue.data[taskQueue.rear] = destination;
    taskQueue.rear = (taskQueue.rear + 1) % QUEUE_SIZE;
    taskQueue.count++;

    return true;
}

bool Queue_Pop(char *destination)
{
    if (taskQueue.count == 0)
        return false;

    *destination = taskQueue.data[taskQueue.front];
    taskQueue.front = (taskQueue.front + 1) % QUEUE_SIZE;
    taskQueue.count--;

    return true;
}

int NodeIndex(int x, int y)
{
    return y * 4 + x;
}

bool AreNeighbors(int a, int b)
{
    int ax = a % 4, ay = a / 4;
    int bx = b % 4, by = b / 4;

    int fin_x = (ax - bx) > 0 ? (ax - bx) : (bx - ax);
    int fin_y = (ay - by) > 0 ? (ay - by) : (by - ay);

    return ((fin_x + fin_y) == 1);
}

// path[] nhận danh sách nút từ start đến goal.
// Giá trị trả về là số nút trong đường đi; 0 nếu thất bại.

int BFS(int start, int goal, int path[NODE_COUNT])
{
    int queue[NODE_COUNT];
    int parent[NODE_COUNT];
    bool visited[NODE_COUNT] = { false };

    for (int i = 0; i < NODE_COUNT; i++)
        parent[i] = -1;

    int head = 0, tail = 0;

    queue[tail++] = start;
    visited[start] = true;

    while (head < tail)
    {
        int current = queue[head++];

        if (current == goal)
            break;

        for (int next = 0; next < NODE_COUNT; next++)
        {
            if (AreNeighbors(current, next) &&
                !visited[next])
            {
                visited[next] = true;
                parent[next] = current;
                queue[tail++] = next;
            }
        }
    }

    if (!visited[goal])
        return 0;

    int reversePath[NODE_COUNT];
    int count = 0;

    for (int node = goal; node != -1; node = parent[node])
        reversePath[count++] = node;

    for (int i = 0; i < count; i++)
        path[i] = reversePath[count - 1 - i];

    return count;
}

bool UART_GetCommand(char *command)
{
    if (rxTail == rxHead)
        return false;

    *command = rxBuffer[rxTail];
    rxTail = (rxTail + 1) % RX_BUFFER_SIZE;

    return true;
}

void ProcessIncomingCommands(void)
{
    char command;

    while (UART_GetCommand(&command))
    {
        char message[64];

        if (Queue_Push(command))
        {
            snprintf(message, sizeof(message), "\r\nDa nhan diem den: %c\r\n", command);
            UART_Print(message);
            PrintQueue();
        }
        else
        {
            snprintf(message, sizeof(message), "\r\nLOI: khong them duoc diem %c (queue day/lenh khong hop le).\r\n", command);
            UART_Print(message);
            PrintQueue();
        }
    }
}

bool FollowPath(int path[], int pathLength)
{
    if (pathLength <= 0)
        return false;

    if (path[0] != currentNode)
        return false;

    for (int i = 1; i < pathLength; i++)
    {
        if (!MoveToNeighbor(path[i]))
        {
        	Robot_Stop();
            return false;
        }
    }

    Robot_Stop();
    return true;
}

int DestinationIndex(char destination)
{
    for (int i = 0; i < NODE_COUNT; i++)
    {
        if (nodeName[i] == destination)
            return i;
    }

    return -1;
}

void ExecuteDelivery(char destination)
{
    int path[NODE_COUNT];

    int target = DestinationIndex(destination);

    if (target < 0)
        return;

    {
        char message[64];
        snprintf(message, sizeof(message), "\r\nBat dau giao den %c tu vi tri %c.\r\n",
                 destination, nodeName[currentNode]);
        UART_Print(message);
    }

    // Từ vị trí hiện tại đến điểm giao hàng.
    int length = BFS(currentNode, target, path);

    if (length == 0 || !FollowPath(path, length))
    {
        Robot_Stop();
        UART_Print("\r\nLOI: khong the di den diem dich.\r\n");
        return;
    }

    // Dừng tại điểm giao hàng.
    Robot_Stop();
    {
        char message[64];
        snprintf(message, sizeof(message), "\r\nDA DEN DIEM %c.\r\n", nodeName[currentNode]);
        UART_Print(message);
    }
    HAL_Delay(1000);

    // Tìm đường từ điểm giao hàng về S.
    length = BFS(currentNode, 0, path);

    if (length == 0 || !FollowPath(path, length))
    {
        Robot_Stop();
        UART_Print("\r\nLOI: khong the quay ve diem S.\r\n");
        return;
    }

    Robot_Stop();
    UART_Print("\r\nDa quay ve diem S.\r\n");
}

bool MoveToNeighbor(int nextNode)
{
    // Kiểm tra chỉ số nút hợp lệ
    if (nextNode < 0 || nextNode >= NODE_COUNT)
        return false;

    // Chỉ được di chuyển đến nút liền kề
    if (!AreNeighbors(currentNode, nextNode))
        return false;

    int x1 = currentNode % 4;
    int y1 = currentNode / 4;

    int x2 = nextNode % 4;
    int y2 = nextNode / 4;

    Direction requiredDir;

    // Xác định hướng cần đi
    if (x2 > x1)
        requiredDir = EAST;
    else if (x2 < x1)
        requiredDir = WEST;
    else if (y2 > y1)
        requiredDir = SOUTH;
    else
        requiredDir = NORTH;

    // Tính góc xoay cần thiết
    int diff = (requiredDir - robotDir + 4) % 4;

    if (diff == 1)
    {
        TurnRightTimed();
    }
    else if (diff == 3)
    {
        TurnLeftTimed();
    }
    else if (diff == 2)
    {
        TurnAroundTimed();
    }

    // Đi thẳng đến nút kế tiếp
    MoveOneEdge();

    // Cập nhật vị trí theo lộ trình dự kiến
    currentNode = nextNode;

    {
        char message[64];
        snprintf(message, sizeof(message), "Da den nut: %c\r\n", nodeName[currentNode]);
        UART_Print(message);
    }

    return true;
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
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  HAL_UART_Receive_IT(&huart1, &rxByte, 1);
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  Robot_Stop();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  ProcessIncomingCommands();

	      if (currentNode == 0)
	      {
	          char destination;

	          if (Queue_Pop(&destination))
	          {
	              ExecuteDelivery(destination);
	          }
	          else
	          {
	              Robot_Stop();
	          }
	      }

	  HAL_Delay(10);

//	  GPIO_PinState L = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
//	  GPIO_PinState C = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
//	  GPIO_PinState R = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);
//
//	  if (L == GPIO_PIN_SET &&
//	      C == GPIO_PIN_SET &&
//	      R == GPIO_PIN_SET) // Ngã tư
//	  {
//	      // Tạm dừng để kiểm thử trạng thái giao lộ
//	      Robot_Stop();
//	  }
//	  else if (L == GPIO_PIN_SET &&
//	  	       C == GPIO_PIN_RESET &&
//	  	       R == GPIO_PIN_SET) // Ngã 3 trái phải
//	  {
//	  	  // Tạm dừng để kiểm thử trạng thái giao lộ
//	  	  Robot_Stop();
//	  }
//	  else if (L == GPIO_PIN_RESET &&
//	           C == GPIO_PIN_SET &&
//	           R == GPIO_PIN_RESET) // Đi thẳng
//	  {
//	      Robot_Forward();
//	  }
//	  else if (L == GPIO_PIN_SET &&
//			   C == GPIO_PIN_SET &&
//			   R == GPIO_PIN_RESET) // Ngã 3 trái thẳng
//
//	  {
//	      Robot_TurnLeft();
//	  }
//	  else if (L == GPIO_PIN_RESET &&
//	  		   C == GPIO_PIN_SET &&
//	  		   R == GPIO_PIN_SET) // Ngã 3 phải thẳng
//
//	  {
//	  	  Robot_TurnRight();
//	  }
//	  else if (L == GPIO_PIN_SET &&
//	  		   C == GPIO_PIN_RESET &&
//	  		   R == GPIO_PIN_RESET) // Rẽ trái
//
//	  {
//	      Robot_TurnLeft();
//	  }
//	  else if (L == GPIO_PIN_RESET &&
//	  		   C == GPIO_PIN_RESET &&
//	  		   R == GPIO_PIN_SET) // Rẽ phải
//
//	  {
//	      Robot_TurnRight();
//	  }
//	  else
//	  {
//	      Robot_Stop();
//	  }
//
//	  HAL_Delay(20);
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  huart1.Init.BaudRate = 115200;
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        char c = (char)rxByte;
        uartRxCount++;

        /* Echo ký tự nhận được về Virtual Terminal để kiểm tra UART */
        HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, 10);

        if (c >= 'a' && c <= 'z')
            c = (char)(c - ('a' - 'A'));

        if (c == 'A' || c == 'B' || c == 'C')
        {
            uint8_t next = (uint8_t)((rxHead + 1U) % RX_BUFFER_SIZE);
            if (next != rxTail)
            {
                rxBuffer[rxHead] = c;
                rxHead = next;
            }
            else
            {
                rxOverflow = 1;
            }
        }
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}
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
#ifdef USE_FULL_ASSERT
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
