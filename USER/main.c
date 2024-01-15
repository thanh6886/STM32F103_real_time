#include "stm32f10x.h"                  // Device header
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio.h"
#include "delay.h"
#include "ds1307.h"
#include "LiquidCrystal_I2C.h"

char 	vrc_Getc; 			// bien kieu char, dung de nhan du lieu tu PC gui xuong;
int		vri_Stt = 0; 			// bien danh dau trang thai. 
int 	vri_Count = 0; 		// bien diem
char	vrc_Res[100];
char receivedChar;

uint16_t UARTx_Getc(USART_TypeDef* USARTx);
void uart_Init(void);
void USART1_IRQHandler(void);
void UART_SendString(const char *str);

uint8_t day, date, month, year, hour, min, sec;
uint8_t thu, ngay, thang, nam, gio, phut, giay;
char *arr[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
char bufferdate[50];
char buffertime[50];
char bufferday[50];

void uart_setTime(void *p);
void Task_LCD(void *p);


QueueHandle_t uartQueue;

int main(void){
	uartQueue = xQueueCreate(128, sizeof(char));
	LCDI2C_init(0x27,16,2); 
	ds1307_init();	
	LCDI2C_backlight();
	xTaskCreate(uart_setTime, (const char*)"SetTime",128 , NULL,tskIDLE_PRIORITY, NULL);
	xTaskCreate(Task_LCD, (const char*)"Display",128 , NULL, 1, NULL);
	vTaskStartScheduler(); 
	while(1){
	}
}

void Task_LCD(void *p){
	while(1){	
		ds1307_get_calendar_date(&thu, &ngay, &thang, &nam);
		ds1307_get_time_24(&gio, &phut, &giay);	
		sprintf(bufferday,"%s-",arr[thu-1]);
		LCDI2C_setCursor(1, 1);
		LCDI2C_write_String(bufferday);
		sprintf(bufferdate, "%02hhu/%02hhu/20%02hhu",ngay, thang, nam);
		LCDI2C_setCursor(5, 1);
		LCDI2C_write_String(bufferdate);
		sprintf(buffertime, "%02hhu:%02hhu:%02hhu", gio, phut, giay);
		LCDI2C_setCursor(4, 0);				
		LCDI2C_write_String(buffertime);
		vTaskDelay(5);		
		}
} 

void uart_setTime(void *p){
	uart_Init();
	while (1) {
		  if(xQueueReceive(uartQueue, (void*)&receivedChar, (portTickType)0xFFFFFFFF)){
				if(receivedChar=='!'){
						vri_Stt = 1;
						receivedChar = NULL;
						vri_Count=0;
				}
				else{
					vrc_Res[vri_Count]= receivedChar;
					vri_Count++;			
				}
				if(vri_Stt ==1){	
					sscanf(vrc_Res, "%hhu-%02hhu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu", &date, &day,
														&month, &year,&hour, &min, &sec);	
					ds1307_set_calendar_date(date, day, month, year);
					ds1307_set_time_24(hour, min, sec);					
			
					vri_Stt = 0;
          vri_Count = 0;
          memset(vrc_Res, 0, sizeof(vrc_Res));
			}
		}
	}
}



uint16_t UARTx_Getc(USART_TypeDef* USARTx){
	return USART_ReceiveData(USARTx);
}

void USART1_IRQHandler(void) {
 if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        vrc_Getc = UARTx_Getc(USART1);
			xQueueSend(uartQueue, (void*)&vrc_Getc,(portTickType)0);
		}
 }

void uart_Init(void){
	GPIO_InitTypeDef gpio_typedef;
	USART_InitTypeDef usart_typedef;
	// enable clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	// congifgure pin Tx - A9;
	gpio_typedef.GPIO_Pin = GPIO_Pin_9;
	gpio_typedef.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&gpio_typedef);	
	// configure pin Rx - A10;
	gpio_typedef.GPIO_Pin = GPIO_Pin_10;
	gpio_typedef.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&gpio_typedef);
	// usart configure
	usart_typedef.USART_BaudRate = 9600;
	usart_typedef.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_typedef.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; 
	usart_typedef.USART_Parity = USART_Parity_No;
	usart_typedef.USART_StopBits = USART_StopBits_1;
	usart_typedef.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &usart_typedef);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(USART1_IRQn);
	USART_Cmd(USART1, ENABLE);
}
