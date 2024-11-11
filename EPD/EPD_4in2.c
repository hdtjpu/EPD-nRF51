/*****************************************************************************
* | File      	:   EPD_4in2.c
* | Author      :   Waveshare team
* | Function    :   4.2inch e-paper
* | Info        :
*----------------
* |	This version:   V3.0
* | Date        :   2019-06-13
* | Info        :
* -----------------------------------------------------------------------------
* V3.0(2019-06-13):
* 1.Change:
*    lut_vcomDC[]  => EPD_4IN2_lut_vcomDC[]
*    lut_ww[] => EPD_4IN2_lut_ww[]
*    lut_bw[] => EPD_4IN2_lut_bw[]
*    lut_wb[] => EPD_4IN2_lut_wb[]
*    lut_bb[] => EPD_4IN2_lut_bb[]
*    EPD_Reset() => EPD_4IN2_Reset()
*    EPD_SendCommand() => EPD_4IN2_SendCommand()
*    EPD_SendData() => EPD_4IN2_SendData()
*    EPD_WaitUntilIdle() => EPD_4IN2_ReadBusy()
*    EPD_SetFullReg() => EPD_4IN2_SetFullReg()
*    EPD_SetPartReg() => EPD_4IN2_SetPartReg()
*    EPD_TurnOnDisplay() => EPD_4IN2_TurnOnDisplay()
*    EPD_Init() => EPD_4IN2_Init()
*    EPD_Clear() => EPD_4IN2_Clear()
*    EPD_Display() => EPD_4IN2_Display()
*    EPD_Sleep() => EPD_4IN2_Sleep()
* 2.remove commands define:
*    #define PANEL_SETTING                               0x00
*    #define POWER_SETTING                               0x01
*    #define POWER_OFF                                   0x02
*    #define POWER_OFF_SEQUENCE_SETTING                  0x03
*    #define POWER_ON                                    0x04
*    #define POWER_ON_MEASURE                            0x05
*    #define BOOSTER_SOFT_START                          0x06
*    #define DEEP_SLEEP                                  0x07
*    #define DATA_START_TRANSMISSION_1                   0x10
*    #define DATA_STOP                                   0x11
*    #define DISPLAY_REFRESH                             0x12
*    #define DATA_START_TRANSMISSION_2                   0x13
*    #define VCOM_LUT                                    0x20
*    #define W2W_LUT                                     0x21
*    #define B2W_LUT                                     0x22
*    #define W2B_LUT                                     0x23
*    #define B2B_LUT                                     0x24
*    #define PLL_CONTROL                                 0x30
*    #define TEMPERATURE_SENSOR_CALIBRATION              0x40
*    #define TEMPERATURE_SENSOR_SELECTION                0x41
*    #define TEMPERATURE_SENSOR_WRITE                    0x42
*    #define TEMPERATURE_SENSOR_READ                     0x43
*    #define VCOM_AND_DATA_INTERVAL_SETTING              0x50
*    #define LOW_POWER_DETECTION                         0x51
*    #define TCON_SETTING                                0x60
*    #define RESOLUTION_SETTING                          0x61
*    #define GET_STATUS                                  0x71
*    #define AUTO_MEASURE_VCOM                           0x80
*    #define READ_VCOM_VALUE                             0x81
*    #define VCM_DC_SETTING                              0x82
*    #define PARTIAL_WINDOW                              0x90
*    #define PARTIAL_IN                                  0x91
*    #define PARTIAL_OUT                                 0x92
*    #define PROGRAM_MODE                                0xA0
*    #define ACTIVE_PROGRAM                              0xA1
*    #define READ_OTP_DATA                               0xA2
*    #define POWER_SAVING                                0xE3
* V2.0(2018-10-30):
* 1.Remove:ImageBuff[EPD_HEIGHT * EPD_WIDTH / 8]
* 2.Change:EPD_Display(UBYTE *Image)
*   Need to pass parameters: pointer to cached data
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_4in2.h"

static const unsigned char EPD_4IN2_lut_vcom0[] = {
    0x00, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x00, 0x0F, 0x0F, 0x00, 0x00, 0x01,	
	0x00, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 					
	};
static const unsigned char EPD_4IN2_lut_ww[] = {
	0x50, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x90, 0x0F, 0x0F, 0x00, 0x00, 0x01,	
	0xA0, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
static const unsigned char EPD_4IN2_lut_bw[] = {
	0x50, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x90, 0x0F, 0x0F, 0x00, 0x00, 0x01,	
	0xA0, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
static const unsigned char EPD_4IN2_lut_wb[] = {
	0xA0, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x90, 0x0F, 0x0F, 0x00, 0x00, 0x01,	
	0x50, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	};
static const unsigned char EPD_4IN2_lut_bb[] = {
	0x20, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x90, 0x0F, 0x0F, 0x00, 0x00, 0x01,	
	0x10, 0x08, 0x08, 0x00, 0x00, 0x02,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
	};

/******************************partial screen update LUT*********************************/
const unsigned char EPD_4IN2_Partial_lut_vcom1[] ={
    0x00, 0x01, 0x20, 0x01, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char EPD_4IN2_Partial_lut_ww1[] ={
    0x00, 0x01, 0x20, 0x01, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char EPD_4IN2_Partial_lut_bw1[] ={
    0x20, 0x01, 0x20, 0x01, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

const unsigned char EPD_4IN2_Partial_lut_wb1[] ={
    0x10, 0x01, 0x20, 0x01, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

const unsigned char EPD_4IN2_Partial_lut_bb1[] ={
    0x00, 0x01,0x20, 0x01, 0x00, 0x01, 
    0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00,0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
};


/******************************gray*********************************/
//0~3 gray
const unsigned char EPD_4IN2_4Gray_lut_vcom[] =
{
    0x00 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x60 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x00 ,0x14 ,0x00 ,0x00 ,0x00 ,0x01,
    0x00 ,0x13 ,0x0A ,0x01 ,0x00 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
				
};
//R21
const unsigned char EPD_4IN2_4Gray_lut_ww[] ={
    0x40 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x10 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0xA0 ,0x13 ,0x01 ,0x00 ,0x00 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R22H	r
const unsigned char EPD_4IN2_4Gray_lut_bw[] ={
    0x40 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x00 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0x99 ,0x0C ,0x01 ,0x03 ,0x04 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R23H	w
const unsigned char EPD_4IN2_4Gray_lut_wb[] ={
    0x40 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x00 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0x99 ,0x0B ,0x04 ,0x04 ,0x01 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R24H	b
const unsigned char EPD_4IN2_4Gray_lut_bb[] ={
    0x80 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x20 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0x50 ,0x13 ,0x01 ,0x00 ,0x00 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_4IN2_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
void EPD_4IN2_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
void EPD_4IN2_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_4IN2_ReadBusy(void)
{
    while(DEV_Digital_Read(EPD_BUSY_PIN) == 0) {      //LOW: idle, HIGH: busy
        DEV_Delay_ms(100);
    }
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_4IN2_TurnOnDisplay(void)
{
    EPD_4IN2_SendCommand(0x12);
    DEV_Delay_ms(100);
    EPD_4IN2_ReadBusy();
}

/******************************************************************************
function :	set the look-up tables
parameter:
******************************************************************************/
static void EPD_4IN2_SetLut(void)
{
	unsigned int count;
	EPD_4IN2_SendCommand(0x20);
	for(count=0;count<36;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_lut_vcom0[count]);}

	EPD_4IN2_SendCommand(0x21);
	for(count=0;count<36;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_lut_ww[count]);}   
	
	EPD_4IN2_SendCommand(0x22);
	for(count=0;count<36;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_lut_bw[count]);} 

	EPD_4IN2_SendCommand(0x23);
	for(count=0;count<36;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_lut_wb[count]);} 

	EPD_4IN2_SendCommand(0x24);
	for(count=0;count<36;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_lut_bb[count]);}  
}	
static void EPD_4IN2_Partial_SetLut(void)
{
	unsigned int count;
	EPD_4IN2_SendCommand(0x20);
	for(count=0;count<44;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_Partial_lut_vcom1[count]);}

	EPD_4IN2_SendCommand(0x21);
	for(count=0;count<42;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_Partial_lut_ww1[count]);}   
	
	EPD_4IN2_SendCommand(0x22);
	for(count=0;count<42;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_Partial_lut_bw1[count]);} 

	EPD_4IN2_SendCommand(0x23);
	for(count=0;count<42;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_Partial_lut_wb1[count]);} 

	EPD_4IN2_SendCommand(0x24);
	for(count=0;count<42;count++)	     
		{EPD_4IN2_SendData(EPD_4IN2_Partial_lut_bb1[count]);}   
}

//LUT download
static void EPD_4IN2_4Gray_lut(void)
{
	unsigned int count;	 
	{
		EPD_4IN2_SendCommand(0x20);							//vcom
		for(count=0;count<42;count++)
			{EPD_4IN2_SendData(EPD_4IN2_4Gray_lut_vcom[count]);}
		
	EPD_4IN2_SendCommand(0x21);							//red not use
	for(count=0;count<42;count++)
		{EPD_4IN2_SendData(EPD_4IN2_4Gray_lut_ww[count]);}

		EPD_4IN2_SendCommand(0x22);							//bw r
		for(count=0;count<42;count++)
			{EPD_4IN2_SendData(EPD_4IN2_4Gray_lut_bw[count]);}

		EPD_4IN2_SendCommand(0x23);							//wb w
		for(count=0;count<42;count++)
			{EPD_4IN2_SendData(EPD_4IN2_4Gray_lut_wb[count]);}

		EPD_4IN2_SendCommand(0x24);							//bb b
		for(count=0;count<42;count++)
			{EPD_4IN2_SendData(EPD_4IN2_4Gray_lut_bb[count]);}

		EPD_4IN2_SendCommand(0x25);							//vcom
		for(count=0;count<42;count++)
			{EPD_4IN2_SendData(EPD_4IN2_4Gray_lut_ww[count]);}
	}	         
}
/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_4IN2_Init(void)
{
	EPD_4IN2_Reset();
	EPD_4IN2_SendCommand(0x01);			//POWER SETTING 
	EPD_4IN2_SendData (0x03);	          
	EPD_4IN2_SendData (0x00);
	EPD_4IN2_SendData (0x2b);  
	EPD_4IN2_SendData (0x2b);

	EPD_4IN2_SendCommand(0x06);         //boost soft start
	EPD_4IN2_SendData (0x17);		//A
	EPD_4IN2_SendData (0x17);		//B
	EPD_4IN2_SendData (0x17);		//C       

	EPD_4IN2_SendCommand(0x04);  
	EPD_4IN2_ReadBusy();

	EPD_4IN2_SendCommand(0x00);			//panel setting
	EPD_4IN2_SendData(0xbf);		//KW-bf   KWR-2F	BWROTP 0f	BWOTP 1f


	EPD_4IN2_SendCommand(0x30);			
	EPD_4IN2_SendData (0x3c);      	// 3A 100HZ   29 150Hz 39 200HZ	31 171HZ

	EPD_4IN2_SendCommand(0x61);			//resolution setting
	EPD_4IN2_SendData (0x01);        	 
	EPD_4IN2_SendData (0x90);	 //400	
	EPD_4IN2_SendData (0x01);	 //300
	EPD_4IN2_SendData (0x2c);	   


	EPD_4IN2_SendCommand(0x82);			//vcom_DC setting  	
	EPD_4IN2_SendData (0x12);	

	EPD_4IN2_SendCommand(0X50);
	EPD_4IN2_SendData(0x97);

	EPD_4IN2_SetLut();		
}

void EPD_4IN2_Init_4Gray(void)
{
	EPD_4IN2_Reset();
	EPD_4IN2_SendCommand(0x01);			//POWER SETTING
	EPD_4IN2_SendData (0x03);
	EPD_4IN2_SendData (0x00);       //VGH=20V,VGL=-20V
	EPD_4IN2_SendData (0x2b);		//VDH=15V															 
	EPD_4IN2_SendData (0x2b);		//VDL=-15V
	EPD_4IN2_SendData (0x13);

	EPD_4IN2_SendCommand(0x06);         //booster soft start
	EPD_4IN2_SendData (0x17);		//A
	EPD_4IN2_SendData (0x17);		//B
	EPD_4IN2_SendData (0x17);		//C 

	EPD_4IN2_SendCommand(0x04);
	EPD_4IN2_ReadBusy();

	EPD_4IN2_SendCommand(0x00);			//panel setting
	EPD_4IN2_SendData(0x3f);		//KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

	EPD_4IN2_SendCommand(0x30);			//PLL setting
	EPD_4IN2_SendData (0x3c);      	//100hz 

	EPD_4IN2_SendCommand(0x61);			//resolution setting
	EPD_4IN2_SendData (0x01);		//400
	EPD_4IN2_SendData (0x90);     	 
	EPD_4IN2_SendData (0x01);		//300
	EPD_4IN2_SendData (0x2c);

	EPD_4IN2_SendCommand(0x82);			//vcom_DC setting
	EPD_4IN2_SendData (0x12);

	EPD_4IN2_SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING			
	EPD_4IN2_SendData(0x97);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_4IN2_Clear(void)
{
    UWORD Width, Height;
    Width = (EPD_4IN2_WIDTH % 8 == 0)? (EPD_4IN2_WIDTH / 8 ): (EPD_4IN2_WIDTH / 8 + 1);
    Height = EPD_4IN2_HEIGHT;
		EPD_4IN2_SendCommand(0x92);	
		EPD_4IN2_SetLut();
    EPD_4IN2_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2_SendData(0xFF);
        }
    }

    EPD_4IN2_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2_SendData(0xFF);
        }
    }

    EPD_4IN2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_4IN2_Display(UBYTE *Image)
{
    UWORD Width, Height;
    Width = (EPD_4IN2_WIDTH % 8 == 0)? (EPD_4IN2_WIDTH / 8 ): (EPD_4IN2_WIDTH / 8 + 1);
    Height = EPD_4IN2_HEIGHT;
		EPD_4IN2_SendCommand(0x92);	
		EPD_4IN2_SetLut();
    EPD_4IN2_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_4IN2_SendData(Image[i + j * Width]);
        }
    }

    EPD_4IN2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_4IN2_Display_Half(UBYTE *Image, UBYTE Region)
{
    UWORD Width, Height;
    Width = (EPD_4IN2_WIDTH % 8 == 0)? (EPD_4IN2_WIDTH / 8 ): (EPD_4IN2_WIDTH / 8 + 1);
    Height = EPD_4IN2_HEIGHT;
		
		if(Region == 1){
			EPD_4IN2_SendCommand(0x13);
			for (UWORD j = 0; j < Height/2; j++) {
					for (UWORD i = 0; i < Width; i++) {
							EPD_4IN2_SendData(Image[i + j * Width]);
					}
			}
		}else if(Region == 2){
			for (UWORD j = 0; j < Height/2; j++) {
					for (UWORD i = 0; i < Width; i++) {
							EPD_4IN2_SendData(Image[i + j * Width]);
					}
			}
		}
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_4IN2_UpdateDisplay(void)
{
	EPD_4IN2_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_4IN2_Sleep(void)
{
	EPD_4IN2_SendCommand(0x50); // DEEP_SLEEP
	EPD_4IN2_SendData(0XF7);

	EPD_4IN2_SendCommand(0x02); // POWER_OFF
	EPD_4IN2_ReadBusy();

	EPD_4IN2_SendCommand(0x07); // DEEP_SLEEP
	EPD_4IN2_SendData(0XA5);
}
