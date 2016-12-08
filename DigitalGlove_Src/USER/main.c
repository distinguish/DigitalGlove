#include "led.h"
#include "delay.h"
#include "sys.h"
#include "includes.h"
#include <stdio.h>
#include "Lcd_Driver.h"
#include "GUI.h"
#include "TFT_demo.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"	
#include "hc05.h"
#include "mpu6050.h"
#include "inv_mpu.h"

//-------------------------------------UCOSII��������----------------------------------------------
//START ����
#define START_TASK_PRIO      						10 		//�����������ȼ�,��ʼ��������ȼ�����Ϊ���
#define START_STK_SIZE  								64		//���������ջ��С
OS_STK START_TASK_STK[START_STK_SIZE];				//�����ջ	
void start_task(void *pdata);									//������
 			   
//LED����
#define LED_TASK_PRIO   			    			9 
#define LED_STK_SIZE  		    					64
OS_STK LED_TASK_STK[LED_STK_SIZE];
void led_task(void *pdata);

//GUI����
#define GUI_TASK_PRIO       						8 
#define GUI_STK_SIZE  									128
OS_STK GUI_TASK_STK[GUI_STK_SIZE];
void gui_task(void *pdata);

//ADCת������
#define ADC_TASK_PRIO 									6
#define ADC_STK_SIZE										64
OS_STK ADC_TASK_STK[ADC_STK_SIZE];
void adc_task(void *pdata);

//MPU6050����
#define MPU6050_TASK_PRIO								5
#define MPU6050_STK_SIZE								128
OS_STK MPU6050_TASK_STK[MPU6050_STK_SIZE];
void mpu6050_task(void *pdata);

//HC05����
#define HC05_TASK_PRIO									7
#define HC05_STK_SIZE										64
OS_STK HC05_TASK_STK[HC05_STK_SIZE];
void hc05_task(void *pdata);


//-------------------------------------UCOSII�������ý���----------------------------------------------

//-----------------------------------------ȫ�ֱ���----------------------------------------------------

#define ADC_CHLS 9 							//9·ADC
#define ADC_CNTS 10 						//ÿ��ADCͨ��ȡ10��ֵ
u16 ADC_VALUES[ADC_CNTS][ADC_CHLS];			//�洢ADCת����M*N��������������
u16 ADC_VALUES_AVER[ADC_CHLS];					//ÿ��ADCͨ����ƽ��ֵ
float pitch, roll, yaw;
short pitch_s, roll_s, yaw_s;
short aacx, aacy, aacz;
short aacx_t, aacy_t, aacz_t;
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
	LED_Init();		  	//��ʼ����LED���ӵ�Ӳ���ӿ�
	Lcd_Init();
	Adc_Init();
	MyDMA_Init(DMA1_Channel1,(u32)&ADC1->DR, (u32)&ADC_VALUES, ADC_CHLS*ADC_CNTS);	//��ʼ��DMA
	while(MPU_Init() != 0) printf("mpu init error!");
	while(mpu_dmp_init() != 0)printf("mpu dmp init error!");
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
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(gui_task,(void *)0,(OS_STK*)&GUI_TASK_STK[GUI_STK_SIZE-1],GUI_TASK_PRIO);	 				   
	OSTaskCreate(adc_task,(void *)0,(OS_STK*)&ADC_TASK_STK[ADC_STK_SIZE-1],ADC_TASK_PRIO);	 				   
	OSTaskCreate(mpu6050_task,(void *)0,(OS_STK*)&MPU6050_TASK_STK[MPU6050_STK_SIZE-1],MPU6050_TASK_PRIO);	 				   
	OSTaskCreate(hc05_task,(void *)0,(OS_STK*)&HC05_TASK_STK[HC05_STK_SIZE-1],HC05_TASK_PRIO);	 				   
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
}

//gui����
void gui_task(void *pdata)
{	
	char pitch_str[16];
	char roll_str[16];
	char yaw_str[16];
	LCD_LED_SET;
	Lcd_Clear(GRAY0);
	Gui_DrawFont_GBK16(5,20,BLUE,GRAY0,"pitch:");
	Gui_DrawFont_GBK16(5,40,BLUE,GRAY0," roll:");
	Gui_DrawFont_GBK16(5,60,BLUE,GRAY0," yaw :");
	while(1){
		Gui_DrawFont_GBK16(70, 20, GRAY0, GRAY0, "          ");
		Gui_DrawFont_GBK16(70, 40, GRAY0, GRAY0, "          ");
		Gui_DrawFont_GBK16(70, 60, GRAY0, GRAY0, "          ");
		
		sprintf(pitch_str, "%+4d" , pitch_s);
		sprintf(roll_str, "%+4d", roll_s);
		sprintf(yaw_str, "%+4d", yaw_s);
		Gui_DrawFont_GBK16(70, 20, RED, GRAY0, pitch_str);
		Gui_DrawFont_GBK16(70, 40, RED, GRAY0, roll_str);
		Gui_DrawFont_GBK16(70, 60, RED, GRAY0, yaw_str);
		delay_ms(50);
	}
}

//LED����
void led_task(void *pdata)
{	  
	while(1)
	{
		LED=0;
		delay_ms(300);
		LED=1;
		delay_ms(300);
	};
}

void adc_task(void *pdata){
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������
	DMA_Cmd(DMA1_Channel1, ENABLE);				//ʹ��DMAͨ��
	while(1){
		ADC_Filter();
		//printf("%d %d %d %d\n", ADC_VALUES_AVER[0], ADC_VALUES_AVER[1], ADC_VALUES_AVER[2], ADC_VALUES_AVER[3] );
		delay_ms(30);
	}
}

void mpu6050_task(void *pdata){
	
	while(1){
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0){
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);
			pitch_s = (short)pitch + 90;
			roll_s = (short)roll + 180;
			yaw_s = (short)yaw + 180;
			aacx_t = aacx / 10 + 3276;
			aacy_t = aacy / 10 + 3276;
			aacz_t = aacz / 10 + 3276;
			//printf("pitch=%d roll=%d yaw=%d\n", pitch_s, roll_s, yaw_s);
			//printf("AacX=%d AacY=%d AacZ=%d\n", aacx_t, aacy_t, aacz_t);
			delay_ms(10);
		}
	}
}

void hc05_task(void *pdata){
	
	while(1){
		HC05_Send_Data(aacx_t, aacy_t, aacz_t, pitch_s, roll_s, yaw_s, 
										ADC_VALUES_AVER[0], ADC_VALUES_AVER[1], ADC_VALUES_AVER[2], 
										ADC_VALUES_AVER[3], ADC_VALUES_AVER[4], ADC_VALUES_AVER[5],
										ADC_VALUES_AVER[6], ADC_VALUES_AVER[7], ADC_VALUES_AVER[8]);
		delay_ms(20);
	}
}
