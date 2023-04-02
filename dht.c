#include <reg52.h>
#include <intrins.h>
#include <absacc.h>
#include <math.h>			   //包含头文件
#include "0832.h"			   //包含ad转换函数
  

#define uchar unsigned char
#define uint unsigned int		//宏定义

#include "eeprom52.h"

typedef unsigned char unint8;
typedef unsigned char unint16;
unsigned char str1[]      = {" "};
unsigned char str2[]      = {" "};
unsigned char code dis1[] = {"2-# H: "};
unsigned char code dis2[] = {"618 T: "};//温湿度的一些定义

//定义LCD1602端口
sbit E=P2^6;		//1602使能引脚
sbit RS=P2^7;		//1602数据/命令选择引脚
sbit RW=P2^5;		//1602读写控制引脚

sbit LED_DUST=P1^5;

sbit LED_RED=P2^2;
sbit LED_YELLOW=P2^1;
sbit LED_GREEN=P2^0;		//指示灯引脚定义
sbit SET_KEY=P3^4;
sbit ADD_KEY=P3^5;
sbit SUB_KEY=P3^6;			//按键引脚定义
sbit BUZZ=P1^7;				//蜂鸣器引脚定义
int WARNING;			//初始报警值
int FC;					//转换的数值存储变量
bit FlagStartRH=0;			//开始转换标志位
bit flag_BJ=0;				//报警标志位
bit set=0;					//设置状态标志位
void L1602_string(uchar hang,uchar lie,uchar *p);  //液晶显示函数声明

/******************把数据保存到单片机内部eeprom中******************/
void write_eeprom()
{
	SectorErase(0x2000);
	byte_write(0x2000, WARNING/255);
	byte_write(0x2001, WARNING%255);
	byte_write(0x2060, a_a);	
}

/******************把数据从单片机内部eeprom中读出来*****************/
void read_eeprom()
{
	WARNING = byte_read(0x2000)*255+byte_read(0x2001);
	a_a      = byte_read(0x2060);

}

/**************开机自检eeprom初始化*****************/
void init_eeprom() 
{
	read_eeprom();		//先读
	if(a_a != 1)		//新的单片机初始单片机内问eeprom
	{
		WARNING = 500;
		a_a = 1;
		write_eeprom();	   //保存数据
	}	
}

//定时器0初始化
void Timer0_Init()
{
	ET0 = 1;        //允许定时器0中断
	TMOD = 0x11;       //定时器工作方式选择
	TL0 = 0xb0;     
	TH0 = 0x3c;     //定时器赋予初值50ms
	TR0 = 1;        //启动定时器
	TL1 = 0xb0;     
	TH1 = 0x3c;     //定时器赋予初值
	ET1=1;			//打开T1中断允许
	TR1=0;			//关闭T1计时开关（关闭蜂鸣器）
}

//定时器0中断
void Timer0_ISR (void) interrupt 1 using 0
{
	uchar RHCounter;	   //定义计时变量
	TL0 = 0xb0;
	TH0 = 0x3c;     //定时器赋予初值
	RHCounter++;
	//每1秒钟启动一次转换
    if (RHCounter >= 20) //定时器初值是50ms，这个变量加20次就是1000ms也就是1s
    {
       FlagStartRH = 1;	 //转换标志位置1，启动转换
	   RHCounter = 0;	 //计时变量清零
    }
}
//定时器1中断
void Timer1_ISR (void) interrupt 3
{
	uchar RHCounter1;
	TL1 = 0xb0;
	TH1 = 0x3c;     //定时器赋予初值
	RHCounter1++;
	if(RHCounter1>=10)	  //加到10次，就是500ms也就是0.5s
	{
		RHCounter1=0;	  //变量清零
		BUZZ=!BUZZ;		  //控制蜂鸣器每500ms取反一次，也就是1秒内响0.5s停0.5s
	}
}

void delay_us(unsigned int us)
{
	while(us--)
	_nop_();
}


/********************************************************************
* 文件名  ： 液晶1602显示.c
* 描述    :  该程序实现了对液晶1602的控制。
***********************************************************************/


/********************************************************************
* 名称 : delay()
* 功能 : 延时,延时时间大概为140US。
* 输入 : 无
* 输出 : 无
***********************************************************************/

void delay()					 //延时函数
{
	int i,j;
	for(i=0; i<=10; i++)
	for(j=0; j<=2; j++);
}

void delay_ms(uint ms)			  //延时函数
{
	uint i,j;
	for(i=0;i<ms;i++)
	for(j=0;j<121;j++);
} 	  
	
/********************************************************************
* 名称 : enable(uchar del)
* 功能 : 1602命令函数
* 输入 : 输入的命令值
* 输出 : 无
***********************************************************************/

void enable(uchar del)		  
{
	P0 = del;			   //将数据赋值P0口
	RS = 0;				   //RS引脚拉低
	E = 1;				   //E引脚高电平
	delay();			   //短延时
	E = 0;				   //E引脚拉低
	delay();			   //短延时
}

/********************************************************************
* 名称 : write(uchar del)
* 功能 : 1602写数据函数
* 输入 : 需要写入1602的数据
* 输出 : 无
***********************************************************************/

void write(uchar del)
{
	P0 = del;				 //将数据赋值到P0口
	RS = 1;					 //RS引脚高电平
	E = 1;					 //E引脚高电平
	delay();				 //短延时
	E = 0;					 //E引脚低电平
	delay();				 //短延时
}

/********************************************************************
* 名称 : L1602_char(uchar hang,uchar lie,char sign)
* 功能 : 改变液晶中某位的值，如果要让第一行，第五个字符显示"b" ，调用该函数如下
		 L1602_char(1,5,'b')
* 输入 : 行，列，需要输入1602的数据
* 输出 : 无
***********************************************************************/
void L1602_char(uchar hang,uchar lie,char sign)
{
	uchar a;
	if(hang == 1) a = 0x80;		//第一行地址
	if(hang == 2) a = 0xc0;		//第二行地址
	a = a + lie - 1;			//行列地址加到一起
	enable(a);					//发送行列地址数据给液晶
	write(sign);				//发送要显示的数据给液晶
}

/********************************************************************
* 名称 : L1602_string(uchar hang,uchar lie,uchar *p)
* 功能 : 改变液晶中某位的值，如果要让第一行，第五个字符开始显示"ab cd ef" ，调用该函数如下
	 	 L1602_string(1,5,"ab cd ef;")
* 输入 : 行，列，需要输入1602的数据
* 输出 : 无
***********************************************************************/
void L1602_string(uchar hang,uchar lie,uchar *p)
{
	uchar a;
	if(hang == 1) a = 0x80;	   //第一行地址
	if(hang == 2) a = 0xc0;	   //第二行地址
	a = a + lie - 1;		   //行列地址加到一起
	enable(a);				   //发送行列地址数据给液晶
	while(1)				   //进入循环
	{
		if(*p == '\0') break;  //如果字符串发送完，就执行break，退出while循环
		write(*p);			   //如果字符串没有发送完，程序就按顺序发送字符串数据到液晶显示
		p++;				   //每发一个字符串，字符串指针都加1，准备发下一个字符
	}
}

/********************************************************************
* 名称 : L1602_init()
* 功能 : 1602初始化，请参考1602的资料
* 输入 : 无
* 输出 : 无
***********************************************************************/
void L1602_init(void)
{
	enable(0x38);						  //设置液晶工作模式，意思：16*2行显示，5*7点阵，8位数据
	enable(0x0c);						  //开显示不显示光标
	enable(0x06); 						  //整屏不移动，光标自动右移
	enable(0x01); 						  //清屏
	enable(0x80);						  //指定显示起始地址为第一行第0列
	L1602_string(1,1," PM2.5:   0ug/m3"); //第一行显示内容
	L1602_string(2,1,"HPM2.5:    ug/m3"); //第二行显示内容

	if(WARNING%10000/1000!=0)					  //显示报警值部分显示和上面的类似，注释略
	L1602_char(2,8,WARNING%10000/1000+0x30);
	else
	L1602_char(2,8,' ');
	if(WARNING%10000/100!=0)
	L1602_char(2,9,WARNING%1000/100+0x30);
	else
	L1602_char(2,9,' ');
	if(WARNING%10000/10!=0)
	L1602_char(2,10,WARNING%100/10+0x30);
	else
	L1602_char(2,10,' ');
	L1602_char(2,11,WARNING%10+0x30);
}

void display()						 			  //显示函数
{
	if(FC%10000/1000!=0)						  //如果数据取余10000再除以1000得到千位数据不等于0
	L1602_char(1,8,FC%10000/1000+0x30);			  //将要显示的数据取余10000再除以1000得到千位数据，将千位数据+0x30得到数字对应的字符
	else										  //如果数据取余10000再除以1000得到千位数据等于0
	L1602_char(1,8,' ');						  //那么千位就不显示
	if(FC%10000/100!=0)							  //如果数据取余10000再除以100得到的千位和百位数据不等于0
	L1602_char(1,9,FC%1000/100+0x30);			  //那么就显示百位数据
	else										  //否则
	L1602_char(1,9,' ');						  //如果千位和百位都等于0，不显示百位数据
	if(FC%10000/10!=0)							  //如果数据取余10000再除以10得到的数据不等于0
	L1602_char(1,10,FC%100/10+0x30);			  //显示十位数据
	else										  //否则
	L1602_char(1,10,' ');						  //如果千位百位十位数据都等于0，不显示10位数据
	L1602_char(1,11,FC%10+0x30);				  //显示个位数据

	if(WARNING%10000/1000!=0)					  //显示报警值部分显示和上面的类似，注释略
	L1602_char(2,8,WARNING%10000/1000+0x30);
	else
	L1602_char(2,8,' ');
	if(WARNING%10000/100!=0)
	L1602_char(2,9,WARNING%1000/100+0x30);
	else
	L1602_char(2,9,' ');
	if(WARNING%10000/10!=0)
	L1602_char(2,10,WARNING%100/10+0x30);
	else
	L1602_char(2,10,' ');
	L1602_char(2,11,WARNING%10+0x30);
}

//按键函数
void Key()
{
	if(SET_KEY==0)	//如果设置按键按下
	{
		delay_ms(20);		 //延时去抖
		if(SET_KEY==0)		 //再次判断按键是否按下
		{
			TR1=0;
			BUZZ=0;		//蜂鸣器响
			delay_ms(100);
			BUZZ=1;	 //蜂鸣器关
			set=!set;	//设置的变量取反，等于1时进入设置状态
			TR0=!set;	//定时器0会在进入设置状态后关闭，退出设置状态后打开
			if(set==1)							  //等于1进入设置状态
			{
				enable(0x80+0x40+7);			  //显示的地址，第二行第七列
				if(WARNING%10000/1000!=0)		  //如果报警值的千位不等于0
				write(WARNING%10000/1000+0x30);	  //显示千位数据
				else							  //否则
				write(' ');						  //不显示千位数据
				if(WARNING%10000/100!=0)		  //原理都可以显示函数类似，注释略
				write(WARNING%1000/100+0x30);
				else
				write(' ');
				if(WARNING%10000/10!=0)
				write(WARNING%100/10+0x30);
				else
				write(' ');
				write(WARNING%10+0x30);
				enable(0x0f);//打开显示 无光标 光标闪烁
				enable(0x80+0x40+10);//因为上面有写数据到液晶，闪烁的位置会变动，所以在写完一次显示后要指定我们需要闪烁的位置
			}
			else								 //不是设置状态时
			{
				enable(0x0c);//打开显示 无光标 光标闪烁
				write_eeprom();			   //保存数据
			}
			while(SET_KEY==0);//等待按键释放
		}
	}
	if(ADD_KEY==0&&set!=0)	  //在设置的状态下按下加
	{
		delay_ms(20);	   //延时去抖
		if(ADD_KEY==0&&set!=0)	  //再次判断加按键按下
		{
			BUZZ=0;		//蜂鸣器响
			delay_ms(100);
			BUZZ=1;	 //蜂鸣器关
			WARNING+=10;	//报警值加10
			if(WARNING>=1000)  //如果报警值大于等于1000
			WARNING=1000;		 //报警值等于1000
			enable(0x80+0x40+7);			 //选择要显示的地址
			if(WARNING%10000/1000!=0)		 //报警值的千位不等于0
			write(WARNING%10000/1000+0x30);	 //显示千位数据
			else							 //否则
			write(' ');						 //不显示
			if(WARNING%10000/100!=0)
			write(WARNING%1000/100+0x30);
			else
			write(' ');
			if(WARNING%10000/10!=0)
			write(WARNING%100/10+0x30);
			else
			write(' ');
			write(WARNING%10+0x30);
			enable(0x80+0x40+10);//调整位置
		}
		while(ADD_KEY==0);	  //等待按键释放
	}
	if(SUB_KEY==0&&set!=0)	  //在设置的状态下按下加
	{
		delay_ms(20);
		if(SUB_KEY==0&&set!=0)
		{
			BUZZ=0;		//蜂鸣器响
			delay_ms(100);
			BUZZ=1;	 //蜂鸣器关
			WARNING-=10;	//报警值减10
			if(WARNING<10)  //如果报警值小于10
			WARNING=0;		 //报警值归零
			enable(0x80+0x40+7);
			if(WARNING%10000/1000!=0)
			write(WARNING%10000/1000+0x30);
			else
			write(' ');
			if(WARNING%10000/100!=0)
			write(WARNING%1000/100+0x30);
			else
			write(' ');
			if(WARNING%10000/10!=0)
			write(WARNING%100/10+0x30);
			else
			write(' ');
			write(WARNING%10+0x30);
			enable(0x80+0x40+10);//调整位置
			delay_ms(100);
//			write_eeprom();			   //保存数据
		}
		while(SUB_KEY==0);	  //等待按键释放
	}
}

void ALARM()				 //报警函数
{
	if(FC>=WARNING)				   //如果当前数值大于等于报警值
	{
		TR1=1;				   //报警变量置1，准备报警
		LED_RED=0;				   //红色指示灯点亮
		LED_YELLOW=1;
		LED_GREEN=1;			   //黄色和绿色指示灯熄灭
	}
	else						   //当前数值小于报警值
	{
		if(FC<WARNING&&FC>=WARNING/2) //当前数值是否大于报警值的50%
		{
			LED_RED=1;
			LED_YELLOW=0;		   //黄灯点亮提醒
			LED_GREEN=1;
		}
		else					   //当前数值小于报警值的50%
		{
			LED_RED=1;
			LED_YELLOW=1;
			LED_GREEN=0;			//绿灯点亮
		}
		TR1=0;				   //报警变量清零，关闭报警
		BUZZ=1;					   //关闭蜂鸣器
	}
}

/********************************************************************
* 名称 : Main()
* 功能 : 主函数
***********************************************************************/
void main()
{
	uchar FC_NUM;				  //定义变量
	long SUM;
    EA = 0;				 //关闭定时器总开关
	Timer0_Init();  //定时器0初始化
	init_eeprom();  //开始初始化保存的数据
	EA = 1;			   //打开定时器总开关
	RW=0;			   //液晶读写引脚拉低，一直是写的状态，因为我们一般不从液晶读数据出来
	L1602_init();	   //液晶初始化
	while(1)		   //进入while循环
	{
		 if (FlagStartRH == 1&&set==0)	 //转换标志是否为1
		 {
		    TR0 = 0;					 //定时器关闭
			FlagStartRH=0;
			for(FC_NUM=0;FC_NUM<80;FC_NUM++)
			{
				LED_DUST=0;		  //打开LED
				delay_us(20);	 //11.0592M，大约280us
				FC=(float)(adc0832(0)*0.0196-0.1)/5*1000; //AD数值乘以0.0196得到电压   减去  无灰尘时电压  整体除以K值（K值是小数 单位里是0.1mg所以要乘以10放大）  乘以1000得到ug/m3
				LED_DUST=1;				 //关闭LED
				delay_ms(10);	   //10ms 间隔
				SUM=SUM+FC;		   //累加数值
				if(SET_KEY==0) break;
			}
			FC=SUM/FC_NUM;			   //取平均值
			if(FC>999) FC=999;	   //正常达不到999
			SUM=0;				   //累加值清零
			ALARM();
			TR0 = 1;					 //打开定时器
		}
		Key();							 //扫描按键
		if(set==0)					 //没有进入设置状态时
		display();					 //扫描显示函数
	}
}