#include "led.h"
#include "delay.h"
#include "sys.h"
#include "includes.h"
#include "Lcd_Driver.h"
#include "GUI.h"
#include "TFT_demo.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"	


//-------------------------------------UCOSII��������----------------------------------------------
//START ����
#define START_TASK_PRIO      						10 		//�����������ȼ�,��ʼ��������ȼ�����Ϊ���
#define START_STK_SIZE  								64		//���������ջ��С
OS_STK START_TASK_STK[START_STK_SIZE];				//�����ջ	
void start_task(void *pdata);									//������
 			   
//LED0����
#define LED0_TASK_PRIO   			    			7 
#define LED0_STK_SIZE  		    					64
OS_STK LED0_TASK_STK[LED0_STK_SIZE];
void led0_task(void *pdata);

//LED1����
#define LED1_TASK_PRIO       						6 
#define LED1_STK_SIZE  									64
OS_STK LED1_TASK_STK[LED1_STK_SIZE];
void led1_task(void *pdata);

//adcת������
#define ADC_TASK_PRIO 									8
#define ADC_STK_SIZE										64
OS_STK ADC_TASK_STK[ADC_STK_SIZE];
void adc_task(void *pdata);
//-------------------------------------UCOSII�������ý���----------------------------------------------

//-----------------------------------------ȫ�ֱ���----------------------------------------------------

#define ADC_CHLS 9 							//9·ADC
#define ADC_CNTS 10 						//ÿ��ADCͨ��ȡ10��ֵ
u16 ADC_VALUES[ADC_CNTS][ADC_CHLS];			//�洢ADCת����M*N��������������
u16 ADC_VALUES_AVER[ADC_CHLS];					//ÿ��ADCͨ����ƽ��ֵ

//---------------------------------------ȫ�ֱ����������---------------------------------------------

//ADC�˲�
void ADC_Filter(void)
{
	u8 i, j;
	u16 sum = 0;											
	for(i=0; i < ADC_CHLS; i++)							//ÿ��ͨ������ѭ���˲�
	{
		for(j = 0; j < ADC_CNTS; j++)					//����N��ѭ��
		{
			 sum += ADC_VALUES[j][i];						//�����N�β���ֵ���ܺ�
    }
    ADC_VALUES_AVER[i]= sum / ADC_CNTS;  //���ͨ����������ƽ��ֵ
    sum = 0;                             //��������´����¼���
  }
}



//------------------------------------------������-----------------------------------------------------

 int main(void)
 {	
	delay_init();	    	 //��ʱ������ʼ��	
  NVIC_Configuration();	 
	uart_init(9600);
	//while(HC05_Init() != 0);
	LED_Init();		  	//��ʼ����LED���ӵ�Ӳ���ӿ�
	Lcd_Init();
	Adc_Init();
	MyDMA_Init(DMA1_Channel1,(u32)&ADC1->DR, (u32)&ADC_VALUES, ADC_CHLS*ADC_CNTS);	//��ʼ��DMA
	
	OSInit();   
 	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();	
 }

	  
//��ʼ����
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
  	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)    
 	OSTaskCreate(led0_task,(void *)0,(OS_STK*)&LED0_TASK_STK[LED0_STK_SIZE-1],LED0_TASK_PRIO);						   
 	OSTaskCreate(led1_task,(void *)0,(OS_STK*)&LED1_TASK_STK[LED1_STK_SIZE-1],LED1_TASK_PRIO);	 				   
	OSTaskCreate(adc_task,(void *)0,(OS_STK*)&ADC_TASK_STK[ADC_STK_SIZE-1],ADC_TASK_PRIO);	 				   
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
}

//LED0����
void led0_task(void *pdata)
{	 	
	while(1){
		TFT_Test_Demo();
	}
	
	//OSTimeDly(60);
}

//LED1����
void led1_task(void *pdata)
{	  
	while(1)
	{
		LED=0;
		delay_ms(300);
		LED=1;
		delay_ms(300);
		//OSTimeDly(30);
	};
}

void adc_task(void *pdata){
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������
	DMA_Cmd(DMA1_Channel1, ENABLE);				//ʹ��DMAͨ��
	while(1){
		ADC_Filter();
		printf("%d %d %d\n", ADC_VALUES_AVER[1], ADC_VALUES_AVER[3], ADC_VALUES_AVER[8]);
		OSTimeDly(30);
	}
}



