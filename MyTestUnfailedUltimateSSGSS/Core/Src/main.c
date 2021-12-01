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
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stdlib.h"
//#define ARM_MATH_CM4 
//#include "arm_math.h"

#include "MCUFRIEND_kbv.h"
#include "math.h"
#include "shapes.h"
#include "tetris.h"

//#include "MCUFRIEND_kbv.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SNR_THRESHOLD_F32    75.0f
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
/* Must be a multiple of 16 */
#define NUM_TAPS_ARRAY_SIZE              32
#else
#define NUM_TAPS_ARRAY_SIZE              29
#endif
#define NUM_TAPS              29


#define TEST_LENGTH_SAMPLES  320
#define TABLE_LENGTH  4


//#define BLOCK_SIZE            1
//#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
//static float32_t firStateF32[2 * BLOCK_SIZE + NUM_TAPS - 1];
//#else
//static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];
//#endif 

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
///Project Variables
volatile int Rise_Fall_State = 0;
volatile int token25 =-1;
volatile int local_time = 0;

		//variables for X inputs
volatile float DifferenceX = 0.0;
volatile float IC_Val1 = 0.0;
volatile float IC_Val2 = 0.0;
volatile int Is_First_CapturedX = 0;
volatile float DistanceX = -9000.0;
volatile float tab_valueX[TABLE_LENGTH];
		//variables for Y inputs
volatile float DifferenceY = 0.0;
volatile float IC_Val3 = 0.0;
volatile float IC_Val4 = 0.0;
volatile int Is_First_CapturedY = 0;
volatile float DistanceY = -9000.0;
volatile float tab_valueY[TABLE_LENGTH];

volatile bool FromGaucheToDroite =false;
volatile bool FromGaucheToDroiteFast =false;
volatile bool FromDroiteToGauche =false;
volatile bool FromDroiteToGaucheFast =false;
volatile bool notDroiteGauche=false;
volatile bool notGaucheDroite=false;
volatile bool versLeBas=false;
volatile bool isRotating=false;

//TRansmission RX TX
volatile int compteur = 0;
volatile int compteur_end = 0;
volatile int flag_send = 0;
volatile int flag_done = 0;
volatile int flag_reset = 0;
static int y = 0;
int x = 0;
uint8_t data[FIELD_W+2];
volatile uint8_t Rx_data;
volatile uint8_t Rx_buffer[FIELD_W];
volatile int current_state = 0;
volatile int changement = 1;
volatile int flag_lineIsFull = 0;
volatile bool flag_Collision = false;
volatile unsigned int indexLine=0;
volatile unsigned int indexEmpty=0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#define SUPPORT_ECRAN_1

void print_array(uint8_t* myArray, int size);


void selectRow (int r) 
{
	if (r==1) HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_RESET);
	else HAL_GPIO_WritePin(R1_GPIO_Port, R1_Pin, GPIO_PIN_SET);
	if (r==2) HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_RESET);
	else HAL_GPIO_WritePin(R2_GPIO_Port, R2_Pin, GPIO_PIN_SET);
	if (r==3) HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_RESET);
	else HAL_GPIO_WritePin(R3_GPIO_Port, R3_Pin, GPIO_PIN_SET);
	if (r==4) HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_RESET);
	else HAL_GPIO_WritePin(R4_GPIO_Port, R4_Pin, GPIO_PIN_SET);
}

int readCol() 
{
	int result = 0;
	if (HAL_GPIO_ReadPin(C1_GPIO_Port, C1_Pin) == GPIO_PIN_RESET) result += 1;
	if (HAL_GPIO_ReadPin(C2_GPIO_Port, C2_Pin) == GPIO_PIN_RESET) result += 2;
//	if (HAL_GPIO_ReadPin(C3_GPIO_Port, C3_Pin) == GPIO_PIN_RESET) result += 4;
	if (HAL_GPIO_ReadPin(C4_GPIO_Port, C4_Pin) == GPIO_PIN_RESET) result += 8;
	return result;
}


int keyPressed() 
{
	int rowPos=1; //Current row positiom
	int rowValue; //Value read from the current row
	for (rowPos=1; rowPos <= 4; rowPos++) 
	{
		selectRow(rowPos);
		HAL_Delay(10);
//		while (token2 == 0); //attendre 10ms
//		token2 = 0;
		rowValue=readCol();
		if (rowValue != 0) 
		{ // a key is pressed
			int result = 4*(rowPos-1);
			while (!(rowValue & 1)) 
			{ // test if bit #0 is false
				result++;
				rowValue>>=1;
			}
			while (readCol() != 0); // key no more pressed
			return result;
		}
	}
	return -1;
}

void copy( uint8_t* myArray, int index)
{
	myArray[0]=255;
	for (int i=0; i<FIELD_W;i++){
		myArray[i+1]=Field[index][i];
	}
	myArray[FIELD_W+1]=254;
}

void paste(	uint8_t* buffer,  int row)
{	
	for(int i=0; i<FIELD_W;i++){
		if(buffer[i]!=0){
		Field[row][i]= buffer[i];//buffer[i];
		}
	}
}

bool updateField()
{
	
	
	
	// Faire tomber le tetrimino en mouvement.
	bool collision = displaceTetrimino(0, 1, &tetrimino);

	// S'il y a une collision vers le bas, il faut actualiser le puits.
	if (collision) 
		{
		// Remplacer le tetrimino mobile par des blocs dans "Field".
		for(int i = 0; i < 4; i++){
			for(int j = 0; j < 4; j++){
				short row = i + tetrimino.y - 1;
				short col = j + tetrimino.x;
				short rotation = tetrimino.rotation % N_ROTATIONS;
				bool isTetriminoBlockSolid = (
					TETRIMINOS[tetrimino.type][rotation][i][j]);
				if (isTetriminoBlockSolid && row < FIELD_H) {
					// On ajoute 1 au type parce que 0 correspond déjà à une
					// case vide dans le tableau Field.
					Field[row][col] = tetrimino.type + 1;
				}
			}
		}
		
		// Effacer des lignes si nécessaire.
		short nLinesToErase = 0;
		for(short row = FIELD_H - 1; row > 0; row--){
			bool isLineFull = true;
			for(short col = 0; col < FIELD_W; col++){
				if (!Field[row][col]){
					isLineFull = false;
					break;
				}
			}
			if (isLineFull) {
				eraseLine(row);
				nLinesToErase++;
				row++;
			}
		}
		if (nLinesToErase) {
			displayField();
			score += linePoints[nLinesToErase - 1];
			nDeletedLines += nLinesToErase;
			displayScore();
			displayNDeletedLines();
		}
		//find index of line to transmit AND index of line to put received line in
		for (int row = FIELD_H -1; row > 0; row--)
		{
			bool isLineEmpty = true;
			for (int col = 0; col < FIELD_W; col++)
			{
					if (Field[row][col] != 0)
					{
						isLineEmpty = false;
						break;
					}
					
			}
			if(isLineEmpty)
			{
				indexEmpty=row;
				indexLine=row+1;
				break;
			}
		}
		if ( nLinesToErase == 2) //commencer transmission 1 ligne
		{
			//appel copy
			copy((uint8_t*)&data, indexLine);
			HAL_UART_Transmit_DMA(&huart6, data, sizeof(data));
		}
		if ( nLinesToErase == 3) //commencer transmission 1 ligne
		{
			//appel copy
			copy((uint8_t*)&data, indexLine);
			HAL_UART_Transmit_DMA(&huart6, data, sizeof(data));
			eraseLine(indexLine);
		}
		if ( nLinesToErase == 4) //commencer transmission 1 ligne
		{
			//appel copy
			copy((uint8_t*)&data, indexLine);
			HAL_UART_Transmit_DMA(&huart6, data, sizeof(data));
			copy((uint8_t*)&data, indexLine+1);
			eraseLine(indexLine);
			HAL_UART_Transmit_DMA(&huart6, data, sizeof(data));
			
			eraseLine(indexLine+1);
		}
//		copy((uint8_t*)&data, indexLine);

		if(flag_done == 1) //réception complète
		{
			//appel paste
			flag_done =0;
			paste(((uint8_t*)&Rx_buffer), indexEmpty);
			displayField();
		}
		
		
			
		

		// Réinitialiser le tetrmino et vérifier s'il y a une erreur.
		bool lostGame = resetTetrimino(&tetrimino);
		if (lostGame) {
			return true;
		}
	}
	return false;
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
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART6_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	// LCD initialization
	LCD_Begin();
	HAL_Delay(20);
	LCD_SetRotation(0);
	LCD_FillScreen(BLACK);
	//LCD_DrawFastHLine(0, 160, 240, YELLOW);
//	LCD_DrawCircle(N/2, N/2, 15, WHITE);
	//LCD_DrawRect(20, 40, 202, 240, RED);
//	LCD_Printf("! HELLO ! ");
//	HAL_TIM_Base_Start(&htim2); 
//	HAL_TIM_Base_Start_IT(&htim2);

//	HAL_ADC_Start_DMA(&hadc1, tab, TABLE_LENGTH);

		//Tetris
	LCD_FillScreen(DARKGREY); 
	initializeField();
	displayField();

//	LCD_FillScreen(BLACK); 
	//PWM TRIG START
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	
	//START ECHO CAPTURE TIMERS
	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_3);
	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);
	
	
	HAL_UART_Receive_IT(&huart6, &Rx_data, sizeof(Rx_data));
	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_3);
	TIM1->CCR2 = TIM1->ARR/2;	unsigned int a = 0;
	
	printf("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ \r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//	
while (1)
{
//	if(flag_send == 1)
//		{

//				flag_send = 0;
////				if(x<20){
////					data[0] = 255;
////					data[2+x] = 254;
////					for(int j=0; j<x+1; j++){
////						data[j+1] = j+1;
////					}
////				x++;
////				}
//				
////					HAL_UART_Transmit_DMA(&huart6, data, sizeof(data));
//					
//					
//					
					
					
			
//		}		
//		while(token25 == 0);
//		token25 = 0;
//		printf("La distance3 en centimetres est : %f\r\n",tab_valueX[3]);
//		printf("La distance2 en centimetres est : %f\r\n",tab_valueX[2]);
		printf("InedxEmtpy is : %d\r\nInedxLine is : %d\r\n", indexEmpty, indexLine);
				///////////////////////SONAR_BEGIN/////////////////////////
		if(a%2 ==0)
		{
//			printf("\n\nLa distanceY3 en centimetres est : %f\r\nLa distanceY2 en centimetres est : %f\r\nLa distanceY1 en centimetres est : %f\r\nLa distanceY0 en centimetres est : %f\r\n",tab_valueY[3],tab_valueY[2],tab_valueY[1],tab_valueY[0]);
			printf("\n\nLa distanceX3 en centimetres est : %f\r\nLa distanceX2 en centimetres est : %f\r\nLa distanceX1 en centimetres est : %f\r\nLa distanceX0 en centimetres est : %f\r\n",tab_valueX[3],tab_valueX[2],tab_valueX[1],tab_valueX[0]);
//			printf("La distanceY3 en centimetres est : %f\r\nLa distanceY2 en centimetres est : %f\r\n",tab_valueY[3], tab_valueY[2]);
			//Écrire code logique ici
//			printf("La distance initiale1 est : %f\r\n La distance actuelle1 est : %f\r\nLa distance initiale2 est : %f\r\n La distance actuelle2 est : %f\r\n",tab_value[0], tab_value[1],tab_value[2],tab_value[3]);
//			bool FromGaucheToDroite = (tab_valueX[3] < tab_valueX[2]) && ((tab_valueX[2]-tab_valueX[3])> 3.0)
//															&& ((tab_valueX[2]-tab_valueX[3])< 60.0) && ((DistanceX < 100.0) );
//			bool FromDroiteToGauche = (tab_valueX[3] > tab_valueX[2]) && ((tab_valueX[3]-tab_valueX[2])> 3.0)
//															&& ((tab_valueX[3]-tab_valueX[2])< 60.0)  	&& (DistanceX < 100.0) ;
			if((FromGaucheToDroite == true) || (FromGaucheToDroiteFast == true)) //gauche a droite
			{
//				printf("on va vers la droite!\r\n");
				if (FromGaucheToDroite == true)
				{
					displaceTetrimino(1, 0, &tetrimino);
				}
				if (FromGaucheToDroiteFast == true)
				{
					displaceTetrimino(1, 0, &tetrimino);
					displaceTetrimino(1, 0, &tetrimino);
					displaceTetrimino(1, 0, &tetrimino);
//					printf("SANIC_SPEED_SANIC_SPEED_SANIC_SPEED_SANIC_SPEED_SANIC_SPEED!!!!! RIIGHT\r\n");
				}
			}
			if((FromDroiteToGauche == true) || (FromDroiteToGaucheFast == true)) //droite a gauche
			{
//				printf("on va vers la gauche!\r\n");
				if (FromDroiteToGauche == true)
				{
					displaceTetrimino(-1, 0, &tetrimino);
				}
				if (FromDroiteToGaucheFast == true)
				{
					displaceTetrimino(-1, 0, &tetrimino);
					displaceTetrimino(-1, 0, &tetrimino);
					displaceTetrimino(-1, 0, &tetrimino);
//					printf("SANIC_SPEED_SANIC_SPEED_SANIC_SPEED_SANIC_SPEED_SANIC_SPEED!!!!! LEEFT\r\n");
				}
			}
//			bool notDroiteGauche = !((tab_valueX[3] > tab_valueX[2]) && ((tab_valueX[3]-tab_valueX[2])> 3.0)
//															&& ((tab_valueX[3]-tab_valueX[2])< 60.0)  	&& (DistanceX < 100.0) );
//			bool notGaucheDroite = !((tab_valueX[3] < tab_valueX[2]) && ((tab_valueX[2]-tab_valueX[3])> 3.0)
//															&& ((tab_valueX[2]-tab_valueX[3])< 60.0) && ((DistanceX < 100.0) ));
//			bool versLeBas = (tab_valueY[3] < tab_valueY[2]) && (tab_valueY[2]-tab_valueY[3] < 50.0 ) 
//												&& (tab_valueY[2]-tab_valueY[3] > 5.0 ) && (DistanceY > 0.0) && (DistanceY < 100.0);
			if( versLeBas )	//éxécuter une descente vers le bas
			{
				printf("Descente vers le bas!!!!!!!!\r\n");
				for(  unsigned int i = 0; i < 9000; i++ )
				{
					
					if (displaceTetrimino(0, 1, &tetrimino)) //ne plus descendre si collision
					{
						break;
					}
					
				}
//				HAL_Delay(1000);
				while(token25 == 0); //Wait 2000 ms
				token25 = 0;
//				displaceTetrimino(0, 1, &tetrimino);
			}
//			bool isRotating = (tab_valueY[3]-tab_valueY[2] > 100.0) &&  notGaucheDroite && notDroiteGauche 
//													&& (DistanceY > 0.0) 	&& (tab_valueY[2] !=0.0);
			if((isRotating == true) && (!versLeBas)) 				
			{
				rotateTetrimino(&tetrimino);
//				printf("Executer Rotation:\r\n");
//				printf("La distanceY initiale 1 en centimetres est : %f\r\nLa distanceY finale 1 en centimetres est : %f\r\nLa distanceY initiale 2 en centimetres est : %f\r\nLa distanceY finale 2 en centimetres est : %f\r\n", tab_valueY[0], tab_valueY[1],tab_valueY[2],tab_valueY[3]);
				printf(" Rotation !!:\r\n");
			}
			a++;
		}
		else //do nothing
		{
			a++;
		}
		///////////////////////SONAR_END/////////////////////////
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
		//Random movements for Tetris
		if (updateField()){ // Quand "updateField" renvoie 1, la partie est perdue.
			LCD_FillScreen(DARKGREY); 
			initializeField();
			displayField();
    }
    HAL_Delay(25);
//    randomDisplaceOrRotate(); // Mouvements aléatoires pour simuler une partie.
		
		//tests
//		displaceTetrimino(1, 0, &tetrimino);
//		displaceTetrimino(-1, 0, &tetrimino);
//		displaceTetrimino(0, 1, &tetrimino);
//		displaceTetrimino(0, -1, &tetrimino);
		
		selectRow(1);
    HAL_Delay(10);

    HAL_Delay(125);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
* @brief Retargets the C library printf function to the USART.
* @param None
* @retval None
*/
PUTCHAR_PROTOTYPE
{
/* Place your implementation of fputc here */
/* e.g. write a character to the USART2 and Loop until the end
of transmission */
HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
return ch;
}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) 
//{
// if (htim->Instance == TIM2) 
//	{
//		HAL_ADC_Start(&hadc1);
//		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_5);
//	}
//}

void HAL_SYSTICK_Callback(void) 
{    
	local_time++;
	if (local_time%1000 == 0)
	{
		token25 = 1;
	}
	compteur++;
	
	if(compteur == 1000){
		flag_send = 1;
		compteur = 0;
		compteur_end++;
	}
	//Tetris with manual keyboard commands :
//	 static bool left = false;
//    static bool right = false;
//    static bool rotate = false;
//    int row = readCol();
//    if (row == 1){
//        if (!left)
//            displaceTetrimino(-1, 0, &tetrimino);
//        left = true;
//    }
//    else {
//        left = false;
//    }
//    if (row == 2){
//        if (!right)
//            displaceTetrimino(1, 0, &tetrimino);
//        right = true;
//    }
//    else {
//        right = false;
//    }
//    if (row == 4){
//        if (!rotate)
//            rotateTetrimino(&tetrimino);
//        rotate = true;
//    }
//    else {
//        rotate = false;
//    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 
{
	
	switch (current_state){
		case 0:
			for(unsigned int i = 0; i < FIELD_W; i++)
				{
					Rx_buffer[i] = 0;
				}
			if (Rx_data == 255){
				
				current_state = 1;
				y = 0;
			}
			break;
		case 1:
			if (Rx_data == 254){
				current_state = 0;
				flag_done = 1;
			}
			else {
				if(Rx_data > 7)
				{
					Rx_data = 0;					
				}
				
				Rx_buffer[y] = Rx_data;
				y++;
			}
			break;
			
	}
  HAL_UART_Receive_IT(&huart6, &Rx_data, sizeof(Rx_data)); 
}

void print_array(uint8_t* myArray, int size) {
    printf("[");
    for(int i=0; i<size-1; i++) {
        printf("%i, ", myArray[i]);
    }
    printf("%i]\n\r", myArray[size-1]); 
}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
//	if(htim->Instance == TIM3)
//	{
	    if(htim->Instance == TIM4){
        if(changement)HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
        else HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
    }
    changement = !changement;
		
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) //Channel 3 is for X inputs, linked to PC8
		{
			// TIM2 has captured either rising or falling edge of ECHO signal located at PC8 pin
			//TODO: store counter value in global variable
					if (Is_First_CapturedX==0) // if the first value is not captured
					{
	//					IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3); // read the first value
						IC_Val1 = TIM3->CCR3;

						Is_First_CapturedX = 1;  // set the first captured as true
					}

					else   // if the first is already captured
					{
	//					IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);  // read second value
						IC_Val2 = TIM3->CCR3;
						if (IC_Val2 > IC_Val1)
						{
							DifferenceX = IC_Val2-IC_Val1;
						}

						else if (IC_Val1 > IC_Val2)
						{
							DifferenceX = (0xffff - IC_Val1) + IC_Val2;
						}
						DistanceX = DifferenceX/58.0;
						tab_valueX[TABLE_LENGTH-4]=tab_valueX[TABLE_LENGTH-3];
						tab_valueX[TABLE_LENGTH-3]=tab_valueX[TABLE_LENGTH-2];
						tab_valueX[TABLE_LENGTH-2]=tab_valueX[TABLE_LENGTH-1];
						tab_valueX[TABLE_LENGTH-1]=DistanceX;
						Is_First_CapturedX = 0; // set it back to false
					}			
			
		}
		
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) // Channel 1 is for Y inputs, linked to PC6
		{
			// TIM2 has captured either rising or falling edge of ECHO signal located at PC8 pin
			//TODO: store counter value in global variable
					if (Is_First_CapturedY==0) // if the first value is not captured
					{
	//					IC_Val3 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1); // read the first value
						IC_Val3 = TIM3->CCR1;

						Is_First_CapturedY = 1;  // set the first captured as true
					}

					else   // if the first is already captured
					{
	//					IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);  // read second value
						IC_Val4 = TIM3->CCR1;
						if (IC_Val4 > IC_Val3)
						{
							DifferenceY = IC_Val4-IC_Val3;
						}

						else if (IC_Val3 > IC_Val4)
						{
							DifferenceY = (0xffff - IC_Val3) + IC_Val4;
						}
						DistanceY = DifferenceY/58.0;
						tab_valueY[TABLE_LENGTH-4]=tab_valueY[TABLE_LENGTH-3];
						tab_valueY[TABLE_LENGTH-3]=tab_valueY[TABLE_LENGTH-2];
						tab_valueY[TABLE_LENGTH-2]=tab_valueY[TABLE_LENGTH-1];
						tab_valueY[TABLE_LENGTH-1]=DistanceY;
						Is_First_CapturedY = 0; // set it back to false
					}			
		 }
		FromGaucheToDroite = (tab_valueX[3] < tab_valueX[2]) && ((tab_valueX[2]-tab_valueX[3])> 1.5)
															&& ((tab_valueX[2]-tab_valueX[3])< 15.0) && ((tab_valueX[3] < 80.0)&&(tab_valueX[2] < 80.0) );
		FromGaucheToDroiteFast = (tab_valueX[3] < tab_valueX[2]) && ((tab_valueX[2]-tab_valueX[3])>= 15.0 )
															&& ((tab_valueX[2]-tab_valueX[3])< 60.0) && ((tab_valueX[3] < 80.0)&&(tab_valueX[2] < 80.0));
		FromDroiteToGauche = (tab_valueX[3] > tab_valueX[2]) && ((tab_valueX[3]-tab_valueX[2])> 1.5)
															&& ((tab_valueX[3]-tab_valueX[2])< 15.0)  	&& ((tab_valueX[3] < 80.0)&&(tab_valueX[2] < 80.0)) ;
		FromDroiteToGaucheFast = (tab_valueX[3] > tab_valueX[2]) && ((tab_valueX[3]-tab_valueX[2])>= 15.0)
															&& ((tab_valueX[3]-tab_valueX[2])< 60.0)  	&& ((tab_valueX[3] < 80.0)&&(tab_valueX[2] < 80.0)) ;
//		notDroiteGauche = !((tab_valueX[3] > tab_valueX[2]) && ((tab_valueX[3]-tab_valueX[2])> 3.0)
//														&& ((tab_valueX[3]-tab_valueX[2])< 60.0)  	&& (DistanceX < 100.0) );
//		notGaucheDroite = !((tab_valueX[3] < tab_valueX[2]) && ((tab_valueX[2]-tab_valueX[3])> 3.0)
//														&& ((tab_valueX[2]-tab_valueX[3])< 60.0) && ((DistanceX < 100.0) ));
		versLeBas = (tab_valueY[3] < tab_valueY[2]) && (tab_valueY[2]-tab_valueY[3] < 50.0 ) 
											&& (tab_valueY[2]-tab_valueY[3] > 5.0 ) && (DistanceY > 0.0) && (tab_valueY[3] < 100.0) && (tab_valueY[2] < 100.0);
		isRotating =  (tab_valueY[3]-tab_valueY[2] > 50.0)	&& (DistanceY > 0.0) 	&& (tab_valueY[2] !=0.0);
//		}
//	}	
	
}

//void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
//	
//	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_3, (uint32_t *)tab, TABLE_LENGTH);

//	if (tab == tab_valueX1) tab = tab_value2;
//	else tab = tab_value1;
//	
//  flag_done	= 1;
//}
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
