#include "control.h"
/*Ŀ�ģ����������ӿڣ�ѧϰ�걾�ں�Զ�����ʵ���԰�������Ʃ��ƽ��С����Ѳ�ߡ�׷�������𵽰������á�*/
/*

*/
float Med_Angle=0;		//��е��ֵ��---�������޸���Ļ�е��ֵ���ɡ�
float Target_Speed=0;	//�����ٶȣ���������---���ο����ӿڣ����ڿ���С��ǰ�����˼����ٶȡ�
float Turn_Speed=0;		//�����ٶȣ�ƫ����

float 
	Vertical_Kp=-400,//ֱ����KP��KD
	Vertical_Kd=-1.92;//-1.7;
float 
	Velocity_Kp=-0.44,//�ٶȻ�KP��KI
	Velocity_Ki=-0.0022;//0.01;
float 
	Turn_Kd=0.6,//ת��KP��KD
	Turn_Kp=20;
extern int length;
#define SPEED_Y 20 //����(ǰ��)����趨�ٶ�
#define SPEED_Z 100//ƫ��(����)����趨�ٶ� 

int xunji_s,xunji_t;
extern int length;
extern uint8_t xunji_flag;
uint8_t tcrt_state;
int Vertical_out,Velocity_out,Turn_out;//ֱ����&�ٶȻ�&ת�� ���������

int Vertical(float Med,float Angle,float gyro_Y);//��������
int Velocity(int Target,int encoder_left,int encoder_right);
int Turn(int gyro_Z,int RC); 

void EXTI9_5_IRQHandler(void)
{
	int PWM_out;
	if(EXTI_GetITStatus(EXTI_Line5)!=0)//һ���ж�
	{
		if(PBin(5)==0)//�����ж�
		{
			EXTI_ClearITPendingBit(EXTI_Line5);//����жϱ�־λ
			
			//1���ɼ�����������&MPU6050�Ƕ���Ϣ��
			Encoder_Left=-Read_Speed(2);//�������԰�װ���պ����180�ȣ�Ϊ�˱������������һ�£�����Ҫ������һ��ȡ����
			Encoder_Right=Read_Speed(4);
			
			mpu_dmp_get_data(&Roll,&Pitch,&Yaw);			//�Ƕ�
			MPU_Get_Gyroscope(&gyroy,&gyrox,&gyroz);	//������
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//���ٶ�

			//1.5����������
			/*********************************************************************************************/
			/*ǰ��*/
			if((Fore==0)&&(Back==0))
				Target_Speed=0;
//				if(length<50)
//					Target_Speed++;
//				else
//					if(Target_Speed!=0)
//						Target_Speed--;//δ���ܵ�ǰ������ָ��-->�ٶ����㣬����ԭ��
			if(Fore==1)
			{
				if(length<50)Target_Speed++;
				else	Target_Speed--;
			}
			
			if(Back==1){Target_Speed++;}//
			Target_Speed=Target_Speed>SPEED_Y?SPEED_Y:(Target_Speed<-SPEED_Y?(-SPEED_Y):Target_Speed);//�޷�
			
			/*����*/
			if((Left==0)&&(Right==0))Turn_Speed=0;
			if(Left==1)Turn_Speed+=30;	//��ת
			if(Right==1)Turn_Speed-=30;	//��ת
			Turn_Speed=Turn_Speed>SPEED_Z?SPEED_Z:(Turn_Speed<-SPEED_Z?(-SPEED_Z):Turn_Speed);//�޷�( (20*100) * 100   )
			
			/*ת��Լ��*/
//			if((Left==0)&&(Right==0))Turn_Kd=-0.6;//��������ת��ָ�����ת��Լ��
//			else if((Left==1)||(Right==1))Turn_Kd=0;//������ת��ָ����յ�����ȥ��ת��Լ��
			/*********************************************************************************************/
			if(xunji_flag)
			{
				tcrt_state=GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1);
				tcrt_state=(tcrt_state<<1)|GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
				tcrt_state=(tcrt_state<<1)|GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7);
				tcrt_state=(tcrt_state<<1)|GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6);
				tcrt_state=(tcrt_state<<1)|GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5);
				tcrt_state=(~tcrt_state)&0x1f;  // 0001 1111
				switch(tcrt_state)
				{
					case 0x00:   // 0000 0000
					case 0x11:	//  0001 0001
					case 0x01:	//  0000 0001
					case 0x10:	//  0001 0000
						Target_Speed=-6;
						Turn_Speed=0;
						break;
					case 0x13:	// 0001 0011
						Target_Speed=-6;
						Turn_Speed=-80;
						break;
					case 0x17:	//0001 0111
					case 0x07:
					case 0x0f:
						Target_Speed=-6;
						Turn_Speed=-80;
						break;
					case 0x19:	// 0001 1001
						Target_Speed=-6;
						Turn_Speed=80;
						break;
					case 0x1d:
					case 0x1c:
					case 0x1e:
						Target_Speed=-6;
						Turn_Speed=80;
						break;
					case 0x1b: 	// 0001 1011
						Target_Speed=-30;
						Turn_Speed=0;
						break;
					case 0x1f :// 0001 1111
						Target_Speed=-2;
						Turn_Speed=0;
						break;
					default:
						Target_Speed=-6;
						Turn_Speed=0;
						break;
				}
				Target_Speed=(int)(0.9*xunji_s+0.1*Target_Speed);
				Turn_Speed=(int)(0.9*xunji_t+0.1*Turn_Speed);
				xunji_s=Target_Speed;
				xunji_t=Turn_Speed;
			}
				
				
				
				
			//2��������ѹ��ջ������У�����������������
			Velocity_out=Velocity(Target_Speed,Encoder_Left,Encoder_Right);	//�ٶȻ�
			Vertical_out=Vertical(Velocity_out+Med_Angle,Pitch,gyroy);			//ֱ����
			Turn_out=Turn(gyroz,Turn_Speed);																//ת��
			
			PWM_out=Vertical_out;//�������
			//3���ѿ�����������ص�����ϣ�������յĵĿ��ơ�
			MOTO1=PWM_out-Turn_out;//����
			MOTO2=PWM_out+Turn_out;//�ҵ��
			Limit(&MOTO1,&MOTO2);	 //PWM�޷�			
			Load(MOTO1,MOTO2);		 //���ص�����ϡ�
			
			Stop(&Med_Angle,&Pitch);//��ȫ���
			
		}
	}
}




/*********************
ֱ����PD��������Kp*Ek+Kd*Ek_D

��ڣ������Ƕȡ���ʵ�Ƕȡ���ʵ���ٶ�
���ڣ�ֱ�������
*********************/
int Vertical(float Med,float Angle,float gyro_Y)
{
	int PWM_out;
	
	PWM_out=Vertical_Kp*(Angle-Med)+Vertical_Kd*(gyro_Y-0);
	return PWM_out;
}


/*********************
�ٶȻ�PI��Kp*Ek+Ki*Ek_S
*********************/
int Velocity(int Target,int encoder_left,int encoder_right)
{
	static int Encoder_S,EnC_Err_Lowout_last,PWM_out,Encoder_Err,EnC_Err_Lowout;
	float a=0.7;
	
	//1.�����ٶ�ƫ��
	Encoder_Err=((encoder_left+encoder_right)-Target);//��ȥ���--�ҵ���⣺�ܹ����ٶ�Ϊ"0"�ĽǶȣ����ǻ�е��ֵ��
	//2.���ٶ�ƫ����е�ͨ�˲�
	//low_out=(1-a)*Ek+a*low_out_last;
	EnC_Err_Lowout=(1-a)*Encoder_Err+a*EnC_Err_Lowout_last;//ʹ�ò��θ���ƽ�����˳���Ƶ���ţ���ֹ�ٶ�ͻ�䡣
	EnC_Err_Lowout_last=EnC_Err_Lowout;//��ֹ�ٶȹ����Ӱ��ֱ����������������
	//3.���ٶ�ƫ����֣����ֳ�λ��
	Encoder_S+=EnC_Err_Lowout;
	//4.�����޷�
	Encoder_S=Encoder_S>10000?10000:(Encoder_S<(-10000)?(-10000):Encoder_S);
	
	if(stop==1)Encoder_S=0,stop=0;//���������
	
	//5.�ٶȻ������������
	PWM_out=Velocity_Kp*EnC_Err_Lowout+Velocity_Ki*Encoder_S;
	return PWM_out;
}



/*********************
ת�򻷣�ϵ��*Z����ٶ�+ϵ��*ң������
*********************/
int Turn(int gyro_Z,int RC)
{
	int PWM_out;
	//�ⲻ��һ���ϸ��PD��������Kd��Ե���ת���Լ������Kp��Ե���ң�ص�ת��
	PWM_out=Turn_Kd*gyro_Z + Turn_Kp*RC;
	return PWM_out;
}

