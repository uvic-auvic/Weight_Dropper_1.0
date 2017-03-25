/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "main.h"

#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "ADC.h"
#include "I2C_controller.h"
#include "status_leds.h"
#include "stm32f0xx_tim.h"
#include "Buffer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/*
 * I2C2_semaphore_control is used when a task want to take control of the I2C2 bus
 * This does not necessarily mean that the task is using the I2C2 bus it just
 * means that it has a use for it and needs to block it from other tasks
 */
static SemaphoreHandle_t I2C2_semaphore_control;

void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/

void CreateSemaphores(void){
	//The I2C2 Semaphore to safeguard tasks from overlapping usage of the bus
	I2C2_semaphore_control = xSemaphoreCreateBinary();
	//Just double check that initialized successfully
	configASSERT(I2C2_semaphore_control);
	//For some reason the semaphore start empty so we must free it
	xSemaphoreGive(I2C2_semaphore_control);
}

int main(void)
{
	blink_led_C8_C9_init();

	/*
	SetClockForADC();
	SetClockForADC();
	CalibrateADC();
	EnableADC();
	ConfigureADC();
	ConfigureTIM15();
	ADC1->CR |= ADC_CR_ADSTART;
	*/


	Configure_GPIO_I2C2();
	Configure_I2C2_Master();

	//Configure_GPIO_USART1();
	//Configure_USART1();

	//safety checks

	vBootTaskInit();
	CreateSemaphores();

	/* Start the kernel.  From here on, only tasks and interrupts will run. */
	vTaskStartScheduler();

	//This should never happen
	while(1);
}


void bootUpSeq(void *dummy){

	vGeneralTaskInit();
	//vdoADCTask(); should never have been here
	vTaskDelete(NULL);
}

void blinkyTask(void *dummy){
	while(1){
		GPIOC->ODR ^= GPIO_ODR_8;
		/* maintain LED3 status for 200ms */
		vTaskDelay(500);
	}
}

void doADC(void *dummy){
//	while(1){
//		GPIOC->ODR ^= GPIO_ODR_9;
//		vTaskDelay(500);
//	}

	//myADC_Init();

		ADC_InitTypeDef ADC_InitStructure;

		/*peripheral clock for ADC1 */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

		/* Deinitializes ADC1 peripheral registers to their default reset values*/
		ADC_DeInit(ADC1);

		ADC_StructInit(&ADC_InitStructure);

		/* ADC1 Configuration Settings  */
		ADC_InitStructure.ADC_Resolution = ADC_Resolution_10b;
		ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
		ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
		ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;

		ADC_Init(ADC1, &ADC_InitStructure);

		/* Using channel 3 with 239.5 Cycles sampling time */
		ADC_ChannelConfig(ADC1, ADC_Channel_3 , ADC_SampleTime_239_5Cycles);
		//channel 3 for PA3 confirmed on datasheet

		/* Enable the ADC */
		ADC_Cmd(ADC1, ENABLE);

		/* Wait for the ADRDY flag to check ADC is ready */
		while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));

		/*Start conversion */
		ADC_StartOfConversion(ADC1);
		/*Attempting to use while loop to change channels continuously between channel 3 and 5 */
		while(1){
			ADC_ChannelConfig(ADC1, ADC_Channel_3 , ADC_SampleTime_239_5Cycles);
			//channel change --> might require turning adc on/off
			unsigned int voltage_steps = ADC1->DR; //get value from adc
			int resistance = (int)(voltage_steps*(5000.0/1024.0));

			ADC_ChannelConfig(ADC1, ADC_Channel_5 , ADC_SampleTime_239_5Cycles); //PA5 for second channel measurement

			voltage_steps = ADC1->DR; //get value from adc
			resistance = (int)(voltage_steps*(5000.0/1024.0));
		}

}

void vBootTaskInit(void){
	xTaskCreate(bootUpSeq,
		(const signed char *)"boot",
		configMINIMAL_STACK_SIZE,
		NULL,                 // pvParameters
		tskIDLE_PRIORITY + 1, // uxPriority
		NULL               );///* pvCreatedTask */
}

void vGeneralTaskInit(void){
    xTaskCreate(blinkyTask,
		(const signed char *)"blinkyTask",
		configMINIMAL_STACK_SIZE,
		NULL,                 // pvParameters
		tskIDLE_PRIORITY + 1, // uxPriority
		NULL              ); // pvCreatedTask */
    xTaskCreate(doADC,
    		(const signed char *)"doADC",
    		configMINIMAL_STACK_SIZE,
    		NULL,
    		tskIDLE_PRIORITY +1,
    		NULL			);
}

