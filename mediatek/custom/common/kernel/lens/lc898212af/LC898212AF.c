/*
 * MD218A voice coil motor driver
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "LC898212AF.h"
//#include "../camera/kd_camera_hw.h"

#include "AfDef.h"
#include "AfSTMV.h"

#define LENS_I2C_BUSNUM 1
//static struct i2c_board_info __initdata kd_lens_dev={ I2C_BOARD_INFO("LC898212AF", (0xE4>>1))};
static struct i2c_board_info __initdata kd_lens_dev={ I2C_BOARD_INFO("LC898212AF", 0x72)};
static struct i2c_board_info __initdata af_eeprom_dev={ I2C_BOARD_INFO("AF_EEPROM", (0xA2>>1))};


#define LC898212AF_DRVNAME "LC898212AF"
#define AF_EEPROM_DRVNAME "AF_EEPROM"
#define LC898212AF_VCM_WRITE_ID          0x72
#define AF_EEPROMF_VCM_WRITE_ID          (0xA2>>1)

#define LC898212AF_DEBUG
#ifdef LC898212AF_DEBUG
#define LC898212AFDB printk
#else
#define LC898212AFDB(x,...)
#endif

static spinlock_t g_LC898212AF_SpinLock;

static struct i2c_client * g_pstLC898212AF_I2Cclient = NULL;
static struct i2c_client * g_pstAF_EEPROM_I2Cclient = NULL;


static dev_t g_LC898212AF_devno;
static struct cdev * g_pLC898212AF_CharDrv = NULL;
static struct class *actuator_class = NULL;

static int  g_s4LC898212AF_Opened = 0;
static long g_i4MotorStatus = 0;
static long g_i4Dir = 0;
static unsigned long g_u4LC898212AF_INF = 0;
static unsigned long g_u4LC898212AF_MACRO = 1023;
static unsigned long g_release_inf = 0;
static unsigned long g_u4TargetPosition = 0;
static unsigned long g_u4CurrPosition   = 0;

static int g_sr = 3;

//extern s32 mt_set_gpio_mode(u32 u4Pin, u32 u4Mode);
//extern s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut);
//extern s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir);


u8 HallOff;	  		//Hall Offset
u8 HallBiase;			//Hall Biase 
u16 Hall_Max=0x6000;     		//Infinity position
u16 Hall_Min=0xA000; 	   		//Macro position
static u16 Hall_Max_Pos=0;     		//Macro position
static u16 Hall_Min_Pos=0; 	   		//Infinity position

#define	Min_Pos		0
#define Max_Pos		1023

#define AF_INFO_PROC
#ifdef AF_INFO_PROC
#include <linux/proc_fs.h>
#include "kd_camera_hw.h"
#define AF_INFO_PROC_FILE    "driver/af_info"
static struct proc_dir_entry *af_info_proc = NULL;
static u8 af_sorting_val=0;
static u16 af_read_val=0;
static int count_loop=0;
static int stage=0;
static int count_macro_fail=0;
static int count_inf_fail=0;

void af_power_on()
{

    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,"kd_camera_hw")) {                                           
        printk("[CAMERA SENSOR] Fail to enable digital power:CAMERA_POWER_VCAM_D2\n");                          
    }                                                                                                           
    mdelay(8);                                                                                                   
    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,"kd_camera_hw")) {                                            
        printk("[CAMERA SENSOR] Fail to enable analog power:CAMERA_POWER_VCAM_A\n");                            
    }                                                                                                           
    mdelay(5);                                                                                                  
    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1100,"kd_camera_hw")) {                                            
        printk("[CAMERA SENSOR] Fail to enable digital power:CAMERA_POWER_VCAM_D\n");                           
    }                                                                                                     
    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_3300,"kd_camera_hw")) {                                            
        printk("[CAMERA SENSOR] Fail to enable digital power:CAMERA_POWER_VCAM_A2\n");                           
    }                                                                                                           
    mdelay(5);                                                                                                  

}
void af_power_off()
{
    if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,"kd_camera_hw")) {
        printk("[CAMERA SENSOR] Fail to OFF analog power\n");
    }
    if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,"kd_camera_hw"))
    {
        printk("[CAMERA SENSOR] Fail to enable analog power\n");
    }
    if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, "kd_camera_hw")) {
        printk("[CAMERA SENSOR] Fail to OFF digital power\n");
    }
    if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,"kd_camera_hw"))
    {
        printk("[CAMERA SENSOR] Fail to enable digital power\n");
    }
}

#endif

u16 AF_convert_1(u16 position)// 10bit to 16bit complement
{
#if 0  	// 1: Move from 0x8001 -> 0x7FFF 
	u16 temp;
	temp=(((position - Min_Pos) * (Hall_Max - Hall_Min) / (Max_Pos - Min_Pos)) + Hall_Min) & 0xFFFF;
	
	LC898212AFDB("[LC898212AF] AF_convert 1: position=%d, temp=%d\n",position,temp);

   	return temp;
#else  // 0: Move from  0x7FFF -> 0x8001
#if 1
	u16 temp;

	LC898212AFDB("[LC898212AF] AF_convert_1: position 1=%d\n",position);

	if(position<=511) 
	{
		position=(position-((Max_Pos - Min_Pos)/2))*((Hall_Max+(0xFFFF-Hall_Min))/2)/((Max_Pos - Min_Pos)/2);
		position = ~((position*-1))+1;
	}
	else
	{
		position=(position-((Max_Pos - Min_Pos)/2))*((Hall_Max+(0xFFFF-Hall_Min))/2)/((Max_Pos - Min_Pos)/2);
	}
	
	LC898212AFDB("[LC898212AF] AF_convert_1: position 2=%d\n",position);

	return position;
#else
u16 temp;

temp=(((Max_Pos - position) * (Hall_Max - Hall_Min) / (Max_Pos - Min_Pos)) + Hall_Min) & 0xFFFF;

LC898212AFDB("[LC898212AF] AF_convert 1: position=%d, temp=%d\n",position,temp);

return temp;
#endif
#endif
}

u16 AF_convert_2(u16 position)// 16bit complement to 10 bit
{
	u16 temp;

	//LC898212AFDB("[LC898212AF] AF_convert_2: position 1=%d\n",position);
#if 0
	if(position>0x7FFF)
	{
		//position=~(position-1); 
		//position=position-1
		//position=0xFFFF-position;
		temp=(u16)(~(position-1)); 
		position=(position+1)-0x8001;

		//LC898212AFDB("[LC898212AF] AF_convert_2: position 1A=%d,temp=%d\n",position,temp);
		position=position*512/(Hall_Max);
		//LC898212AFDB("[LC898212AF] AF_convert_2: position 1B=%d\n",position);

	}
	else
	{
		position=position*512/(Hall_Max)+511;
		//LC898212AFDB("[LC898212AF] AF_convert_2: position 1C=%d\n",position);

	}

	//LC898212AFDB("[LC898212AF] AF_convert_2: position 2=%d\n",position);
	return position;
#else

	LC898212AFDB("[LC898212AF] AF_convert 2: position 1=%d\n",position);

	if(position>0x7FFF)
	{
		position=position+1-0x8001;
		position=position*((Max_Pos - Min_Pos)/2)/((Hall_Max+(0xFFFF-Hall_Min))/2);
	}
	else
	{
		position=position*((Max_Pos - Min_Pos)/2)/((Hall_Max+(0xFFFF-Hall_Min))/2)+((Max_Pos - Min_Pos)/2);
	}
	
	LC898212AFDB("[LC898212AF] AF_convert 2: position 2=%d\n",position);

	return position;
#endif

	
}


static int i2c_read(u8 a_u2Addr , u8 * a_puBuff)
{
	int  i4RetValue = 0;
	char puReadCmd[1] = {(char)(a_u2Addr)};
	i4RetValue = i2c_master_send(g_pstLC898212AF_I2Cclient, puReadCmd, 1);
	if (i4RetValue != 2) {
	    LC898212AFDB(" I2C write failed!! \n");
	    return -1;
	}
	//
	i4RetValue = i2c_master_recv(g_pstLC898212AF_I2Cclient, (char *)a_puBuff, 1);
	if (i4RetValue != 1) {
	    LC898212AFDB(" I2C read failed!! \n");
	    return -1;
	}

}

u8 read_data(u8 addr)
{
	u8 get_byte=0;
	i2c_read( addr ,&get_byte);
	LC898212AFDB("[FM50AF]  get_byte %d \n",  get_byte);
	return get_byte;
}
extern int iReadCAM_CAL(u8 a_u2Addr, u32 ui4_length, u8 * a_puBuff);
u8 s4AF_EEPROM_Reg_Read(u8 addr)
{
	u8 data;
	
	//i2c_smbus_read_i2c_block_data(g_pstAF_EEPROM_I2Cclient,addr,1,&data);
	iReadCAM_CAL(addr, 1, &data);
	return data;
}

u8 s4AF_EEPROM_Reg_Write(u8 addr, u8 data)
{
	int err;
	
	err=i2c_smbus_write_i2c_block_data(g_pstAF_EEPROM_I2Cclient,addr,1,&data);
	
	return err;
}

u8 s4LC898212AF_Reg_Read(u8 addr)
{
	u8 data;
	
	i2c_smbus_read_i2c_block_data(g_pstLC898212AF_I2Cclient,addr,1,&data);
	
	return data;
}

u16 s4LC898212AF_RAM_Read(u8 addr)
{
	/*u16 data_read;

	data_read=i2c_smbus_read_word_data(g_pstLC898212AF_I2Cclient,addr);

	return data_read;*/
	u16 data_read;
        u16 data_temp;

         data_temp = i2c_smbus_read_word_data(g_pstLC898212AF_I2Cclient,addr);
         data_read =((data_temp & 0x00FF)<<8) | ((data_temp & 0xFF00)>>8);

         return data_read;
}

u8 s4LC898212AF_Reg_Write(u8 addr, u8 data)
{
	int err;
	
	err=i2c_smbus_write_i2c_block_data(g_pstLC898212AF_I2Cclient,addr,1,&data);
	
	return err;
}

u8 s4LC898212AF_RAM_Write(u8 addr, u16 data)
{
	int err1,err2,err;
	u8 temp;

	u16 data_temp;

#if 0		
	temp=data & 0xFF00;
	temp=temp>>8;
	err1=s4LC898212AF_Reg_Write(addr,temp);

	temp=data & 0x00FF;
	err2=s4LC898212AF_Reg_Write(addr+1,temp);

	return (err1 && err2);
#else
	data_temp=((data & 0x00FF)<<8) | ((data & 0xFF00)>>8);

	//err=i2c_smbus_write_word_data(g_pstLC898212AF_I2Cclient,addr,data);
	err=i2c_smbus_write_word_data(g_pstLC898212AF_I2Cclient,addr,data_temp);
	return err;
#endif
	
	
}


static void readCalibrationData()
{
	u8 eeprom_reg_read_data1;
	u8 eeprom_reg_read_data2;
	u8 eeprom_reg_read_data1_complement;
	u8 eeprom_reg_read_data2_complement;


	u8 i=0;
	for(i=0;i<=0x0A;)
	{
		eeprom_reg_read_data1=s4AF_EEPROM_Reg_Read(i);
		eeprom_reg_read_data2=s4AF_EEPROM_Reg_Read(++i);

		eeprom_reg_read_data1_complement=((u8)~eeprom_reg_read_data1)+1;
		eeprom_reg_read_data2_complement=((u8)~eeprom_reg_read_data2)+1;
		
		LC898212AFDB("[LC898212AF] readCalibrationData:Reg[%x] =%x, Reg[%x]'s complement=%x\n",(i-1),eeprom_reg_read_data1,(i-1),eeprom_reg_read_data1_complement);
		LC898212AFDB("[LC898212AF] readCalibrationData:Reg[%x] =%x, Reg[%x]'s complement=%x\n",i,eeprom_reg_read_data2,i,eeprom_reg_read_data2_complement);

		i++;
	}

#if 0
	HallOff=s4AF_EEPROM_Reg_Read(0x00);
	s4LC898212AF_Reg_Write(0x28,HallOff);

	LC898212AFDB("[LC898212AF] readCalibrationData:HallOff =%x\n",HallOff);


	HallBiase=s4AF_EEPROM_Reg_Read(0x01);
	s4LC898212AF_Reg_Write(0x29,HallBiase);
	LC898212AFDB("[LC898212AF] readCalibrationData:HallBiase =%x\n",HallBiase);	
#else
	HallBiase=s4AF_EEPROM_Reg_Read(0x00);
	s4LC898212AF_Reg_Write(0x29,HallBiase);
	LC898212AFDB("[LC898212AF] readCalibrationData:HallBiase =%x\n",HallBiase);


	HallOff=s4AF_EEPROM_Reg_Read(0x01);
	s4LC898212AF_Reg_Write(0x28,HallOff);
	LC898212AFDB("[LC898212AF] readCalibrationData:HallOff =%x\n",HallOff);

#endif


#if 0
	eeprom_reg_read_data1=s4AF_EEPROM_Reg_Read(0x02);
	eeprom_reg_read_data2=s4AF_EEPROM_Reg_Read(0x03);
	g_u4LC898212AF_INF=((eeprom_reg_read_data2<<8) | eeprom_reg_read_data1);

	if(g_u4LC898212AF_INF>=50)
		g_u4LC898212AF_INF=g_u4LC898212AF_INF-50;
	else
		g_u4LC898212AF_INF=0;
	
	LC898212AFDB("[LC898212AF] readCalibrationData:g_u4LC898212AF_INF =%x\n",g_u4LC898212AF_INF);


	eeprom_reg_read_data1=s4AF_EEPROM_Reg_Read(0x04);
	eeprom_reg_read_data2=s4AF_EEPROM_Reg_Read(0x05);
	g_u4LC898212AF_MACRO=((eeprom_reg_read_data2<<8) | eeprom_reg_read_data1);
	
	if(g_u4LC898212AF_MACRO<=973)
		g_u4LC898212AF_MACRO=g_u4LC898212AF_MACRO+50;
	else
		g_u4LC898212AF_MACRO=1023;
	
	LC898212AFDB("[LC898212AF] readCalibrationData:g_u4LC898212AF_MACRO =%x\n",g_u4LC898212AF_MACRO);
	
#endif	
	eeprom_reg_read_data1=s4AF_EEPROM_Reg_Read(0x02);
	eeprom_reg_read_data2=s4AF_EEPROM_Reg_Read(0x03);
	g_release_inf=((eeprom_reg_read_data2<<8) | eeprom_reg_read_data1);
	
	//if(g_release_inf>=50)
	//	g_release_inf=g_release_inf-50;
	//else
	//	g_release_inf=0;
	
	LC898212AFDB("[LC898212AF] readCalibrationData:g_release_inf =%x\n",g_release_inf);

	eeprom_reg_read_data1=s4AF_EEPROM_Reg_Read(0x08/*0x06*//*0x02*/);
	eeprom_reg_read_data2=s4AF_EEPROM_Reg_Read(0x09/*0x07*//*0x03*/);
	Hall_Max_Pos=(eeprom_reg_read_data2<<8) | eeprom_reg_read_data1;
	Hall_Max=0x7FFF;
	
	LC898212AFDB("[LC898212AF] readCalibrationData:Hall_Max_Pos =%x\n",Hall_Max_Pos);


	eeprom_reg_read_data1=s4AF_EEPROM_Reg_Read(0x0A/*0x08*//*0x04*/);
	eeprom_reg_read_data2=s4AF_EEPROM_Reg_Read(0x0B/*0x09*//*0x05*/);
	Hall_Min_Pos=(eeprom_reg_read_data2<<8) | eeprom_reg_read_data1;
	Hall_Min=0x8001;

	LC898212AFDB("[LC898212AF] readCalibrationData:Hall_Min_Pos =%x\n",Hall_Min_Pos);
	Hall_Max_Pos=AF_convert_2(Hall_Max_Pos);
	Hall_Min_Pos=AF_convert_2(Hall_Min_Pos);
	LC898212AFDB("[LC898212AF] readCalibrationData22:Hall_Max_Pos=%d,Hall_Min_Pos =%d\n",Hall_Max_Pos,Hall_Min_Pos);

}

static int s4LC898212AF_ReadReg(unsigned short * a_pu2Result)
{
#if 0
    int  i4RetValue = 0;
    char pBuff[2];

    i4RetValue = i2c_master_recv(g_pstLC898212AF_I2Cclient, pBuff , 2);

    if (i4RetValue < 0) 
    {
        LC898212AFDB("[LC898212AF] I2C read failed!! \n");
        return -1;
    }

    *a_pu2Result = (((u16)pBuff[0]) << 4) + (pBuff[1] >> 4);
#else
u16 ram_read_data=0;
ram_read_data=s4LC898212AF_RAM_Read(0x3C);

* a_pu2Result=AF_convert_2(ram_read_data);
#endif

LC898212AFDB("s4LC898212AF_ReadReg\n");

    return 0;
}

static int s4LC898212AF_WriteReg(u16 a_u2Data)
{
#if 0
    int  i4RetValue = 0;

    char puSendCmd[2] = {(char)(a_u2Data >> 4) , (char)(((a_u2Data & 0xF) << 4)+g_sr)};

    //LC898212AFDB("[LC898212AF] g_sr %d, write %d \n", g_sr, a_u2Data);
    g_pstLC898212AF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG;
    i4RetValue = i2c_master_send(g_pstLC898212AF_I2Cclient, puSendCmd, 2);
	
    if (i4RetValue < 0) 
    {
        LC898212AFDB("[LC898212AF] I2C send failed!! \n");
        return -1;
    }
#else
u16 ram_write_data=0;
u16 ram_read_data=0;


ram_write_data=AF_convert_1(a_u2Data);
//s4LC898212AF_RAM_Write(0x04,ram_write_data);
StmvTo(ram_write_data);

#endif

LC898212AFDB("s4LC898212AF_WriteReg\n");


    return 0;
}

inline static int getLC898212AFInfo(__user stLC898212AF_MotorInfo * pstMotorInfo)
{
    stLC898212AF_MotorInfo stMotorInfo;
    stMotorInfo.u4MacroPosition   = g_u4LC898212AF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4LC898212AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
    stMotorInfo.bIsSupportSR      = 1;

	if (g_i4MotorStatus == 1)	{stMotorInfo.bIsMotorMoving = 1;}
	else						{stMotorInfo.bIsMotorMoving = 0;}

	if (g_s4LC898212AF_Opened >= 1)	{stMotorInfo.bIsMotorOpen = 1;}
	else						{stMotorInfo.bIsMotorOpen = 0;}

    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stLC898212AF_MotorInfo)))
    {
        LC898212AFDB("[LC898212AF] copy to user failed when getting motor information \n");
    }

    return 0;
}

inline static int moveLC898212AF(unsigned long a_u4Position)
{
    int ret = 0;
    
    if((a_u4Position > g_u4LC898212AF_MACRO) || (a_u4Position < g_u4LC898212AF_INF))
    {
        LC898212AFDB("[LC898212AF] out of range \n");
        return -EINVAL;
    }

    if (g_s4LC898212AF_Opened == 1)
    {
        unsigned short InitPos;
        ret = s4LC898212AF_ReadReg(&InitPos);
	    
        spin_lock(&g_LC898212AF_SpinLock);
        if(ret == 0)
        {
            LC898212AFDB("[LC898212AF] Init Pos %6d \n", InitPos);
            g_u4CurrPosition = (unsigned long)InitPos;
        }
        else
        {		
            g_u4CurrPosition = 0;
        }
        g_s4LC898212AF_Opened = 2;
        spin_unlock(&g_LC898212AF_SpinLock);
    }

    if (g_u4CurrPosition < a_u4Position)
    {
        spin_lock(&g_LC898212AF_SpinLock);	
        g_i4Dir = 1;
        spin_unlock(&g_LC898212AF_SpinLock);	
    }
    else if (g_u4CurrPosition > a_u4Position)
    {
        spin_lock(&g_LC898212AF_SpinLock);	
        g_i4Dir = -1;
        spin_unlock(&g_LC898212AF_SpinLock);			
    }
    else
    {return 0;}

    spin_lock(&g_LC898212AF_SpinLock);    
    g_u4TargetPosition = a_u4Position;
    spin_unlock(&g_LC898212AF_SpinLock);	

#if 0	
    if (a_u4Position < Hall_Min_Pos) 
    {
        g_u4TargetPosition = Hall_Min_Pos+5;
    }
    else if (a_u4Position > Hall_Max_Pos)
    {
        g_u4TargetPosition = Hall_Max_Pos-5;
    }
#endif

    LC898212AFDB("[LC898212AF] move [curr] %d [target] %d\n", g_u4CurrPosition, g_u4TargetPosition);

    spin_lock(&g_LC898212AF_SpinLock);
    g_sr = 3;
    g_i4MotorStatus = 0;
    spin_unlock(&g_LC898212AF_SpinLock);	

    if(s4LC898212AF_WriteReg((unsigned short)g_u4TargetPosition) == 0)
    {
        spin_lock(&g_LC898212AF_SpinLock);		
        g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
        spin_unlock(&g_LC898212AF_SpinLock);				
    }
    else
    {
        LC898212AFDB("[LC898212AF] set I2C failed when moving the motor \n");			
        spin_lock(&g_LC898212AF_SpinLock);
        g_i4MotorStatus = -1;
        spin_unlock(&g_LC898212AF_SpinLock);				
    }

    return 0;
}

inline static int setLC898212AFInf(unsigned long a_u4Position)
{
    spin_lock(&g_LC898212AF_SpinLock);
    g_u4LC898212AF_INF = a_u4Position;
    spin_unlock(&g_LC898212AF_SpinLock);	
    return 0;
}

inline static int setLC898212AFMacro(unsigned long a_u4Position)
{
    spin_lock(&g_LC898212AF_SpinLock);
    g_u4LC898212AF_MACRO = a_u4Position;
    spin_unlock(&g_LC898212AF_SpinLock);	
    return 0;	
}

////////////////////////////////////////////////////////////////
static long LC898212AF_Ioctl(
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    long i4RetValue = 0;

    switch(a_u4Command)
    {
        case LC898212AFIOC_G_MOTORINFO :
            i4RetValue = getLC898212AFInfo((__user stLC898212AF_MotorInfo *)(a_u4Param));
        break;

        case LC898212AFIOC_T_MOVETO :
            i4RetValue = moveLC898212AF(a_u4Param);
        break;
 
        case LC898212AFIOC_T_SETINFPOS :
            i4RetValue = setLC898212AFInf(a_u4Param);
        break;

        case LC898212AFIOC_T_SETMACROPOS :
            i4RetValue = setLC898212AFMacro(a_u4Param);
        break;
		
        default :
      	    LC898212AFDB("[LC898212AF] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    return i4RetValue;
}

#ifdef AF_INFO_PROC
static int macro_max=0x7fff; 
static int macro_min=0x4800;
static int inf_max=0xA200;
static int inf_min=0x8001;
static int fail_count=5;
int AF_Runin(void)
{
	u16 ram_read_data;
	u16 i;
	af_sorting_val = 0;
	count_inf_fail=0;
	count_macro_fail=0;
	for(i=0; i<200; i++)
	{
            count_loop = i+1;
            s4LC898212AF_Reg_Write(0x87,0x05);
            ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            LC898212AFDB("[LC898212AFSORTING] Test_AF_Runin count_loop=%d, (0x3C) initial position=0x%x\n",count_loop,ram_read_data);//Grant
            stage =1;
            
            s4LC898212AF_RAM_Write(0x02,0x7fff);
            mdelay(100);
            ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            LC898212AFDB("[LC898212AFSORTING] Test_AF_Runin count_loop=%d, (0x3C) MACRO=0x%x\n",count_loop,ram_read_data);//Grant
            stage =2;
           // if((ram_read_data > (0x6a00+0x0e00)) ||(ram_read_data < (0x6a00-0x0e00)))
            if((ram_read_data > macro_max) ||(ram_read_data < macro_min))
            {
                af_read_val = ram_read_data;
                af_sorting_val = 1;
		count_macro_fail++;
                LC898212AFDB("[LC898212AFSORTING] Test_AF_Runin count_loop=%d, count_macro_fail=%d,(0x3C) move to MACRO: NG\n",count_loop,count_macro_fail);
                //break; 
            }
            
            s4LC898212AF_RAM_Write(0x02,0x8001);
            mdelay(100);
            ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            LC898212AFDB("[LC898212AFSORTING] Test_AF_Runin count_loop=%d, (0x3C) INF=0x%x\n",count_loop,ram_read_data);//Grant
            stage =3;
            //if((ram_read_data > (0x9600+0x0e00)) ||(ram_read_data < (0x9600-0x0e00)))
            if((ram_read_data > inf_max ) ||(ram_read_data < inf_min ))
            {
                af_read_val = ram_read_data;
                af_sorting_val = 1;
		count_inf_fail++;
                LC898212AFDB("[LC898212AFSORTING] Test_AF_Runin count_loop=%d, count_inf_fail=%d,(0x3C) move to INF: NG\n",count_loop,count_inf_fail);   
                //break; 
            }	

            af_read_val = ram_read_data;
            
      }
	if (af_sorting_val  == 0)
            af_sorting_val = 2;

	LC898212AFDB("[LC898212AFSORTING] Test_AF_Runin count_macro_fail=%d, count_inf_fail=%d\n",count_macro_fail,count_inf_fail);

}

//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
// 3.Update f_op pointer.
// 4.Fill data structures into private_data
//CAM_RESET
static int LC898212AF_Openn()
{
    u16 ram_read_data;
    u8 reg_read_data;
    u16 CurPos=0;

    stSmvPar StSmvPar;	

    int i=0;
	
    af_power_on();
    //readCalibrationData();

    LC898212AFDB("[LC898212AF] LC898212AF_Open - Start\n");
#if 0
    spin_lock(&g_LC898212AF_SpinLock);

    if(g_s4LC898212AF_Opened)
    {
        spin_unlock(&g_LC898212AF_SpinLock);
        LC898212AFDB("[LC898212AF] the device is opened \n");
        return -EBUSY;
    }

    g_s4LC898212AF_Opened = 1;
	
    spin_unlock(&g_LC898212AF_SpinLock);
#endif	
    reg_read_data=s4LC898212AF_Reg_Read(0x80);
    
    s4LC898212AF_Reg_Write(0x80,0x34);
    s4LC898212AF_Reg_Write(0x81,0x20);
    s4LC898212AF_Reg_Write(0x84,0xE0);
    s4LC898212AF_Reg_Write(0x87,0x05);
    s4LC898212AF_Reg_Write(0xA4,0x24);
    
    s4LC898212AF_RAM_Write(0x3A,0x0000);
    s4LC898212AF_RAM_Write(0x04,0x0000);//Grant
    s4LC898212AF_RAM_Write(0x02,0x0000); 
    s4LC898212AF_RAM_Write(0x18,0x0000);//Grant
    s4LC898212AF_Reg_Write(0x88,0x50);
    
    s4LC898212AF_RAM_Write(0x28,/*0x7E60*/0x8080);
    s4LC898212AF_RAM_Write(0x4C,0x2000);//Grant		// 500 1000
    
    s4LC898212AF_Reg_Write(0x83,0x2D);// 0x2c
    s4LC898212AF_Reg_Write(0x85,0xC0);
    
    mdelay(2);
    
    reg_read_data=s4LC898212AF_Reg_Read(0x85);
    
    LC898212AFDB("[LC898212AF] s4LC898212AF_Reg_Read(0x85)=%x\n",reg_read_data);
    
    while((reg_read_data !=0) && (i <10))
    {
        mdelay(2);
        reg_read_data=s4LC898212AF_Reg_Read(0x85);
        LC898212AFDB("[LC898212AF] s4LC898212AF_Reg_Read(0x85) %d=%x\n",i,reg_read_data);
        i++;
    }
    
    s4LC898212AF_Reg_Write(0x84,0xE3);
    s4LC898212AF_Reg_Write(0x97,0x00);
    s4LC898212AF_Reg_Write(0x98,0x42);
    s4LC898212AF_Reg_Write(0x99,0x00);
    s4LC898212AF_Reg_Write(0x9A,0x00);
    s4LC898212AF_Reg_Write(0x92,0x10);//Grant
    //CsHalReg
    s4LC898212AF_Reg_Write(0x76,0x0C);
    s4LC898212AF_Reg_Write(0x77,0x50);
    s4LC898212AF_Reg_Write(0x78,0x40);
    s4LC898212AF_Reg_Write(0x86,0x40);
    s4LC898212AF_Reg_Write(0xF0,0x00);
    s4LC898212AF_Reg_Write(0xF1,0x00);
    
    //CsHalFil
    s4LC898212AF_RAM_Write(0x30,0x0000);
    s4LC898212AF_RAM_Write(0x40,0x7FF0);
    s4LC898212AF_RAM_Write(0x42,0x5C00);
    s4LC898212AF_RAM_Write(0x44,0xA450);
    s4LC898212AF_RAM_Write(0x46,0x6730);
    s4LC898212AF_RAM_Write(0x48,0x50C0);
    s4LC898212AF_RAM_Write(0x4A,0x2030/*0x32F0*/);
    s4LC898212AF_RAM_Write(0x4C,0x2000);
    s4LC898212AF_RAM_Write(0x4E,0x8010);
    s4LC898212AF_RAM_Write(0x50,0x04F0);
    s4LC898212AF_RAM_Write(0x52,0x7610);
    s4LC898212AF_RAM_Write(0x54,0x0CD0/*0x1010*/);
    s4LC898212AF_RAM_Write(0x56,0x0000);
    s4LC898212AF_RAM_Write(0x58,0x7FF0);
    s4LC898212AF_RAM_Write(0x5A,0x0680);
    s4LC898212AF_RAM_Write(0x5C,0x72F0);
    s4LC898212AF_RAM_Write(0x5E,0x7F70);
    s4LC898212AF_RAM_Write(0x60,0x7ED0);
    s4LC898212AF_RAM_Write(0x62,0x7FF0);
    s4LC898212AF_RAM_Write(0x64,0x0000);
    s4LC898212AF_RAM_Write(0x66,0x0000);
    s4LC898212AF_RAM_Write(0x68,0x5130);
    s4LC898212AF_RAM_Write(0x6A,0x72F0);
    s4LC898212AF_RAM_Write(0x6C,0x8010);
    s4LC898212AF_RAM_Write(0x6E,0x0000);
    s4LC898212AF_RAM_Write(0x70,0x0000);
    s4LC898212AF_RAM_Write(0x72,0x1570);
    s4LC898212AF_RAM_Write(0x74,0x5530);
    
    s4LC898212AF_Reg_Write(0x86,0x60);
    //s4LC898212AF_RAM_Write(0x28,0x7E60/*0x8080*/);
    
    
    readCalibrationData();
    
    // Samual Test for 0x3C re-write 20140110 Start
    #if 1	// this is better
    mdelay(1);
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 1=%x\n",ram_read_data);//Grant
    
    s4LC898212AF_RAM_Write(0x04,ram_read_data);
    s4LC898212AF_RAM_Write(0x18,ram_read_data);
    
    ram_read_data=s4LC898212AF_RAM_Read(0x02);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x02) 1=%x\n",ram_read_data);//Grant
    
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 2=%x\n",ram_read_data);
    
    if(AF_convert_2(ram_read_data)>512)
    {//means close to Marco.
        s4LC898212AF_RAM_Write(0x04,0x7000);
        s4LC898212AF_RAM_Write(0x18,0x7000);
        s4LC898212AF_RAM_Write(0x02,0x1000);
    }
    else{//means close to INF
        s4LC898212AF_RAM_Write(0x04,0x9000);
        s4LC898212AF_RAM_Write(0x18,0x9000);
        s4LC898212AF_RAM_Write(0x02,0xF000);
    }
    ram_read_data=s4LC898212AF_RAM_Read(0x02);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x02) 1-1=%x\n",ram_read_data);//Grant
    
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 2-1=%x\n",ram_read_data);
    
    ram_read_data=s4LC898212AF_RAM_Read(0x04);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x04) 2-1=%x\n",ram_read_data);
    
    ram_read_data=s4LC898212AF_RAM_Read(0x18);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x18) 2-1=%x\n",ram_read_data);
    
    // Samual Test for 0x3C re-write 20140110 End
    
     // Step move parameter set
    StSmvPar.UsSmvSiz	= STMV_SIZE ;
    StSmvPar.UcSmvItv	= STMV_INTERVAL ;
    StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;
    
    //StmvSet( StSmvPar );
    
    s4LC898212AF_Reg_Write(0x85,0x80);
    mdelay(1);
    s4LC898212AF_Reg_Write(0x87,0x85);
    mdelay(150);
    
    
    s4LC898212AF_RAM_Write(0x4C,0x4030);//Grant
    s4LC898212AF_Reg_Write(0x92,0x00);//Grant
    //StmvTo(0x3000);
    
    //mdelay(200);
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 2=%x\n",ram_read_data);
    //StmvTo(ram_read_data);
    StmvSet(StSmvPar);	

    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 3=%x\n",ram_read_data);
    i=0;
    if(AF_convert_2(ram_read_data) > g_release_inf && AF_convert_2(ram_read_data) > 400)//900
    {
        while(1)
        {
            i++;
            CurPos= AF_convert_2(ram_read_data);
            if(CurPos<400||i > 16)//900
            { 
                mdelay(30);
                break;
            }
            else 
            {
                s4LC898212AF_WriteReg(CurPos-140);    //70
    		ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            }
        }
    }
    else if(AF_convert_2(ram_read_data) < g_release_inf&& AF_convert_2(ram_read_data) < 100)
    {
        while(1)
        {
            i++;
            CurPos= AF_convert_2(ram_read_data);
            if(CurPos>100 ||i > 16)
	    {
                mdelay(30);
                break;
            }
            else
            {
		s4LC898212AF_WriteReg(CurPos+30);
	        ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            }
        }
    }
    s4LC898212AF_WriteReg(g_release_inf);

//s4LC898212AF_ReadReg(&InitPos);

//LC898212AFDB("[LC898212AF] s4LC898212AF_ReadReg=%d\n",InitPos);

//s4LC898212AF_WriteReg(InitPos);

  AF_Runin();
  af_power_off();
#endif

#if 0
s4LC898212AF_RAM_Write(0x4C,0x2000);
mdelay(1);

ram_read_data=s4LC898212AF_RAM_Read(0x3C);
s4LC898212AF_RAM_Write(0x04,ram_read_data);
s4LC898212AF_RAM_Write(0x18,ram_read_data);

// Step move parameter set
StSmvPar.UsSmvSiz	= STMV_SIZE ;
StSmvPar.UcSmvItv	= STMV_INTERVAL ;
StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;

StmvSet( StSmvPar );

mdelay(50);
s4LC898212AF_Reg_Write(0x85,0x80);
s4LC898212AF_Reg_Write(0x87,0x85);
s4LC898212AF_RAM_Write(0x4C,0x4030);
#endif

#if 0
s4LC898212AF_RAM_Write(0x4C,0x2000);
mdelay(1);

ram_read_data=s4LC898212AF_RAM_Read(0x3C);
s4LC898212AF_RAM_Write(0x04,ram_read_data);
s4LC898212AF_RAM_Write(0x18,ram_read_data);

// Step move parameter set
StSmvPar.UsSmvSiz	= STMV_SIZE ;
StSmvPar.UcSmvItv	= STMV_INTERVAL ;
StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;

StmvSet( StSmvPar );

s4LC898212AF_Reg_Write(0x85,0x80);
mdelay(1);
s4LC898212AF_Reg_Write(0x87,0x85);
mdelay(50);
s4LC898212AF_RAM_Write(0x4C,0x4030);
#endif


    LC898212AFDB("[LC898212AF] LC898212AF_Open - End\n");

    return 0;
}
#endif

static int LC898212AF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    u16 ram_read_data;
    u8 reg_read_data;
    u16 CurPos=0;

    stSmvPar StSmvPar;	

    int i=0;

    //readCalibrationData();

    LC898212AFDB("[LC898212AF] LC898212AF_Open - Start\n");

    spin_lock(&g_LC898212AF_SpinLock);

    if(g_s4LC898212AF_Opened)
    {
        spin_unlock(&g_LC898212AF_SpinLock);
        LC898212AFDB("[LC898212AF] the device is opened \n");
        return -EBUSY;
    }

    g_s4LC898212AF_Opened = 1;
		
    spin_unlock(&g_LC898212AF_SpinLock);

    reg_read_data=s4LC898212AF_Reg_Read(0x80);
    
    s4LC898212AF_Reg_Write(0x80,0x34);
    s4LC898212AF_Reg_Write(0x81,0x20);
    s4LC898212AF_Reg_Write(0x84,0xE0);
    s4LC898212AF_Reg_Write(0x87,0x05);
    s4LC898212AF_Reg_Write(0xA4,0x24);
    
    s4LC898212AF_RAM_Write(0x3A,0x0000);
    s4LC898212AF_RAM_Write(0x04,0x0000);//Grant
    s4LC898212AF_RAM_Write(0x02,0x0000); 
    s4LC898212AF_RAM_Write(0x18,0x0000);//Grant
    s4LC898212AF_Reg_Write(0x88,0x50);
    
    s4LC898212AF_RAM_Write(0x28,/*0x7E60*/0x8080);
    s4LC898212AF_RAM_Write(0x4C,0x2000);//Grant		// 500 1000
    
    s4LC898212AF_Reg_Write(0x83,0x2D);// 0x2c
    s4LC898212AF_Reg_Write(0x85,0xC0);
    
    mdelay(2);
    
    reg_read_data=s4LC898212AF_Reg_Read(0x85);
    
    LC898212AFDB("[LC898212AF] s4LC898212AF_Reg_Read(0x85)=%x\n",reg_read_data);
    
    while((reg_read_data !=0) && (i <10))
    {
        mdelay(2);
        reg_read_data=s4LC898212AF_Reg_Read(0x85);
        LC898212AFDB("[LC898212AF] s4LC898212AF_Reg_Read(0x85) %d=%x\n",i,reg_read_data);
        i++;
    }
    
    s4LC898212AF_Reg_Write(0x84,0xE3);
    s4LC898212AF_Reg_Write(0x97,0x00);
    s4LC898212AF_Reg_Write(0x98,0x42);
    s4LC898212AF_Reg_Write(0x99,0x00);
    s4LC898212AF_Reg_Write(0x9A,0x00);
    s4LC898212AF_Reg_Write(0x92,0x10);//Grant
    //CsHalReg
    s4LC898212AF_Reg_Write(0x76,0x0C);
    s4LC898212AF_Reg_Write(0x77,0x50);
    s4LC898212AF_Reg_Write(0x78,0x40);
    s4LC898212AF_Reg_Write(0x86,0x40);
    s4LC898212AF_Reg_Write(0xF0,0x00);
    s4LC898212AF_Reg_Write(0xF1,0x00);
    
    //CsHalFil
    s4LC898212AF_RAM_Write(0x30,0x0000);
    s4LC898212AF_RAM_Write(0x40,0x7FF0);
    s4LC898212AF_RAM_Write(0x42,0x5C00);
    s4LC898212AF_RAM_Write(0x44,0xA450);
    s4LC898212AF_RAM_Write(0x46,0x6730);
    s4LC898212AF_RAM_Write(0x48,0x50C0);
    s4LC898212AF_RAM_Write(0x4A,0x2030/*0x32F0*/);
    s4LC898212AF_RAM_Write(0x4C,0x2000);
    s4LC898212AF_RAM_Write(0x4E,0x8010);
    s4LC898212AF_RAM_Write(0x50,0x04F0);
    s4LC898212AF_RAM_Write(0x52,0x7610);
    s4LC898212AF_RAM_Write(0x54,0x0CD0/*0x1010*/);
    s4LC898212AF_RAM_Write(0x56,0x0000);
    s4LC898212AF_RAM_Write(0x58,0x7FF0);
    s4LC898212AF_RAM_Write(0x5A,0x0680);
    s4LC898212AF_RAM_Write(0x5C,0x72F0);
    s4LC898212AF_RAM_Write(0x5E,0x7F70);
    s4LC898212AF_RAM_Write(0x60,0x7ED0);
    s4LC898212AF_RAM_Write(0x62,0x7FF0);
    s4LC898212AF_RAM_Write(0x64,0x0000);
    s4LC898212AF_RAM_Write(0x66,0x0000);
    s4LC898212AF_RAM_Write(0x68,0x5130);
    s4LC898212AF_RAM_Write(0x6A,0x72F0);
    s4LC898212AF_RAM_Write(0x6C,0x8010);
    s4LC898212AF_RAM_Write(0x6E,0x0000);
    s4LC898212AF_RAM_Write(0x70,0x0000);
    s4LC898212AF_RAM_Write(0x72,0x1570);
    s4LC898212AF_RAM_Write(0x74,0x5530);
    
    s4LC898212AF_Reg_Write(0x86,0x60);
    //s4LC898212AF_RAM_Write(0x28,0x7E60/*0x8080*/);
    
    
    readCalibrationData();
    
    // Samual Test for 0x3C re-write 20140110 Start
    #if 1	// this is better
    mdelay(1);
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 1=%x\n",ram_read_data);//Grant
    
    s4LC898212AF_RAM_Write(0x04,ram_read_data);
    s4LC898212AF_RAM_Write(0x18,ram_read_data);
    
    ram_read_data=s4LC898212AF_RAM_Read(0x02);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x02) 1=%x\n",ram_read_data);//Grant
    
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 2=%x\n",ram_read_data);
    
    if(AF_convert_2(ram_read_data)>512)
    {//means close to Marco.
        s4LC898212AF_RAM_Write(0x04,0x7000);
        s4LC898212AF_RAM_Write(0x18,0x7000);
        s4LC898212AF_RAM_Write(0x02,0x1000);
    }
    else{//means close to INF
        s4LC898212AF_RAM_Write(0x04,0x9000);
        s4LC898212AF_RAM_Write(0x18,0x9000);
        s4LC898212AF_RAM_Write(0x02,0xF000);
    }
    ram_read_data=s4LC898212AF_RAM_Read(0x02);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x02) 1-1=%x\n",ram_read_data);//Grant
    
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 2-1=%x\n",ram_read_data);
    
    ram_read_data=s4LC898212AF_RAM_Read(0x04);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x04) 2-1=%x\n",ram_read_data);
    
    ram_read_data=s4LC898212AF_RAM_Read(0x18);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x18) 2-1=%x\n",ram_read_data);
    
    // Samual Test for 0x3C re-write 20140110 End
    
     // Step move parameter set
    StSmvPar.UsSmvSiz	= STMV_SIZE ;
    StSmvPar.UcSmvItv	= STMV_INTERVAL ;
    StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;
    
    //StmvSet( StSmvPar );
    
    s4LC898212AF_Reg_Write(0x85,0x80);
    mdelay(1);
    s4LC898212AF_Reg_Write(0x87,0x85);
    mdelay(150);
    
    
    s4LC898212AF_RAM_Write(0x4C,0x4030);//Grant
    s4LC898212AF_Reg_Write(0x92,0x00);//Grant
    //StmvTo(0x3000);
    
    //mdelay(200);
    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 2=%x\n",ram_read_data);
    //StmvTo(ram_read_data);
    StmvSet(StSmvPar);	

    ram_read_data=s4LC898212AF_RAM_Read(0x3C);
    
    LC898212AFDB("[LC898212AF] s4LC898212AF_RAM_Read(0x3C) 3=%x\n",ram_read_data);
    i=0;
    if(AF_convert_2(ram_read_data) > g_release_inf && AF_convert_2(ram_read_data) > 400)//900
    {
        while(1)
        {
            i++;
            CurPos= AF_convert_2(ram_read_data);
            if(CurPos<400||i > 16)//900
            { 
                mdelay(30);
                break;
            }
            else 
            {
                s4LC898212AF_WriteReg(CurPos-140);    //70
    		ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            }
        }
    }
    else if(AF_convert_2(ram_read_data) < g_release_inf&& AF_convert_2(ram_read_data) < 100)
    {
        while(1)
        {
            i++;
            CurPos= AF_convert_2(ram_read_data);
            if(CurPos>100 ||i > 16)
	    {
                mdelay(30);
                break;
            }
            else
            {
		s4LC898212AF_WriteReg(CurPos+30);
	        ram_read_data=s4LC898212AF_RAM_Read(0x3C);
            }
        }
    }
    s4LC898212AF_WriteReg(g_release_inf);

//s4LC898212AF_ReadReg(&InitPos);

//LC898212AFDB("[LC898212AF] s4LC898212AF_ReadReg=%d\n",InitPos);

//s4LC898212AF_WriteReg(InitPos);

#endif

#if 0
s4LC898212AF_RAM_Write(0x4C,0x2000);
mdelay(1);

ram_read_data=s4LC898212AF_RAM_Read(0x3C);
s4LC898212AF_RAM_Write(0x04,ram_read_data);
s4LC898212AF_RAM_Write(0x18,ram_read_data);

// Step move parameter set
StSmvPar.UsSmvSiz	= STMV_SIZE ;
StSmvPar.UcSmvItv	= STMV_INTERVAL ;
StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;

StmvSet( StSmvPar );

mdelay(50);
s4LC898212AF_Reg_Write(0x85,0x80);
s4LC898212AF_Reg_Write(0x87,0x85);
s4LC898212AF_RAM_Write(0x4C,0x4030);
#endif

#if 0
s4LC898212AF_RAM_Write(0x4C,0x2000);
mdelay(1);

ram_read_data=s4LC898212AF_RAM_Read(0x3C);
s4LC898212AF_RAM_Write(0x04,ram_read_data);
s4LC898212AF_RAM_Write(0x18,ram_read_data);

// Step move parameter set
StSmvPar.UsSmvSiz	= STMV_SIZE ;
StSmvPar.UcSmvItv	= STMV_INTERVAL ;
StSmvPar.UcSmvEnb	= STMCHTG_SET | STMSV_SET | STMLFF_SET ;

StmvSet( StSmvPar );

s4LC898212AF_Reg_Write(0x85,0x80);
mdelay(1);
s4LC898212AF_Reg_Write(0x87,0x85);
mdelay(50);
s4LC898212AF_RAM_Write(0x4C,0x4030);
#endif


    LC898212AFDB("[LC898212AF] LC898212AF_Open - End\n");

    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int LC898212AF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    LC898212AFDB("[LC898212AF] LC898212AF_Release - Start\n");

#if 0
    if (g_s4LC898212AF_Opened)
    {
        LC898212AFDB("[LC898212AF] free \n");
        g_sr = 5;
	    s4LC898212AF_WriteReg(200);
        msleep(10);
	    s4LC898212AF_WriteReg(100);
        msleep(10);
            	            	    	    
        spin_lock(&g_LC898212AF_SpinLock);
        g_s4LC898212AF_Opened = 0;
        spin_unlock(&g_LC898212AF_SpinLock);

    }
#else
    if (g_s4LC898212AF_Opened)
    {
        LC898212AFDB("[LC898212AF] free: g_u4CurrPosition=%d \n",g_u4CurrPosition);
        g_sr = 5;
#if 0
        if (g_u4CurrPosition > 700)  {
            s4LC898212AF_WriteReg(700);
            msleep(3);
        }
        else if (g_u4CurrPosition > 600)  {
            s4LC898212AF_WriteReg(600);
            msleep(3);
        }
        else if (g_u4CurrPosition > 500)  {
            s4LC898212AF_WriteReg(500);
            msleep(3);
        }
        else if (g_u4CurrPosition > 400)  {
            s4LC898212AF_WriteReg(400);
            msleep(3);
        }
        else if (g_u4CurrPosition > 300)  {
            s4LC898212AF_WriteReg(300);
            msleep(3);
        }
        else if (g_u4CurrPosition > 200)  {
	        s4LC898212AF_WriteReg(200);
            msleep(3);
        }
        else if (g_u4CurrPosition > 100)   {
	        s4LC898212AF_WriteReg(100);
            msleep(3);
        }
#endif
	s4LC898212AF_WriteReg(900);
            msleep(3);
	s4LC898212AF_Reg_Write(0x87,0x05);
        spin_lock(&g_LC898212AF_SpinLock);
        g_s4LC898212AF_Opened = 0;
        spin_unlock(&g_LC898212AF_SpinLock);

    }
#endif

    LC898212AFDB("[LC898212AF] LC898212AF_Release - End\n");

    return 0;
}

static const struct file_operations g_stLC898212AF_fops = 
{
    .owner = THIS_MODULE,
    .open = LC898212AF_Open,
    .release = LC898212AF_Release,
    .unlocked_ioctl = LC898212AF_Ioctl
};

inline static int Register_LC898212AF_CharDrv(void)
{
    struct device* vcm_device = NULL;

    LC898212AFDB("[LC898212AF] Register_LC898212AF_CharDrv - Start\n");

    //Allocate char driver no.
    if( alloc_chrdev_region(&g_LC898212AF_devno, 0, 1,LC898212AF_DRVNAME) )
    {
        LC898212AFDB("[LC898212AF] Allocate device no failed\n");

        return -EAGAIN;
    }

    //Allocate driver
    g_pLC898212AF_CharDrv = cdev_alloc();

    if(NULL == g_pLC898212AF_CharDrv)
    {
        unregister_chrdev_region(g_LC898212AF_devno, 1);

        LC898212AFDB("[LC898212AF] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pLC898212AF_CharDrv, &g_stLC898212AF_fops);

    g_pLC898212AF_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pLC898212AF_CharDrv, g_LC898212AF_devno, 1))
    {
        LC898212AFDB("[LC898212AF] Attatch file operation failed\n");

        unregister_chrdev_region(g_LC898212AF_devno, 1);

        return -EAGAIN;
    }

    actuator_class = class_create(THIS_MODULE, "actuatordrv");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        LC898212AFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }

    vcm_device = device_create(actuator_class, NULL, g_LC898212AF_devno, NULL, LC898212AF_DRVNAME);

    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    LC898212AFDB("[LC898212AF] Register_LC898212AF_CharDrv - End\n");    
    return 0;
}

inline static void Unregister_LC898212AF_CharDrv(void)
{
    LC898212AFDB("[LC898212AF] Unregister_LC898212AF_CharDrv - Start\n");

    //Release char driver
    cdev_del(g_pLC898212AF_CharDrv);

    unregister_chrdev_region(g_LC898212AF_devno, 1);
    
    device_destroy(actuator_class, g_LC898212AF_devno);

    class_destroy(actuator_class);

    LC898212AFDB("[LC898212AF] Unregister_LC898212AF_CharDrv - End\n");    
}

//////////////////////////////////////////////////////////////////////

static int LC898212AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int LC898212AF_i2c_remove(struct i2c_client *client);
static int AF_EEPROM_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int AF_EEPROM_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id LC898212AF_i2c_id[] = {{LC898212AF_DRVNAME,0},{}}; 

static const struct i2c_device_id AF_EEPROM_i2c_id[] = {{AF_EEPROM_DRVNAME,0},{}};   


struct i2c_driver LC898212AF_i2c_driver = {                       
    .probe = LC898212AF_i2c_probe,                                   
    .remove = LC898212AF_i2c_remove,                           
    .driver.name = LC898212AF_DRVNAME,                 
    .id_table = LC898212AF_i2c_id,                             
};  


struct i2c_driver AF_EEPROM_i2c_driver = {                       
    .probe = AF_EEPROM_i2c_probe,                                   
    .remove = AF_EEPROM_i2c_remove,                           
    .driver.name = AF_EEPROM_DRVNAME,                 
    .id_table = AF_EEPROM_i2c_id,                             
};  



#if 0 
static int LC898212AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {         
    strcpy(info->type, LC898212AF_DRVNAME);                                                         
    return 0;                                                                                       
}      
#endif 
static int LC898212AF_i2c_remove(struct i2c_client *client) {
    return 0;
}


static int AF_EEPROM_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int i4RetValue = 0;


    LC898212AFDB("[LC898212AF] AF_EEPROM_i2c_probe\n");
	
    g_pstAF_EEPROM_I2Cclient = client;

    LC898212AFDB("[LC898212AF] AF_EEPROM_i2c_probe:AF_EEPROM_i2c_probe->addr=%d\n",g_pstAF_EEPROM_I2Cclient->addr);
	
	
    return 0;
}


static int AF_EEPROM_i2c_remove(struct i2c_client *client) {
    return 0;
}
#ifdef AF_INFO_PROC

static int af_info_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *ptr = page;
    char *name;

    s32 ret;

    static s32 proc_count=0;

    if(proc_count < 1)
        LC898212AF_Openn();

    proc_count++;
    if(proc_count == 2)
        proc_count = 0;

    printk("%s\n", __func__);
    if(af_sorting_val==2)
        ptr += sprintf( ptr, "AF Test PASS!\nAF_read_val=0x%x;count_loop=%d;stage=%d,count_inf_fail=%d,count_macro_fail=%d\n",af_read_val,count_loop,stage,count_inf_fail,count_macro_fail);
    else if((af_sorting_val==1) && (count_inf_fail+count_macro_fail)>fail_count)
        ptr += sprintf( ptr, "AF Test FAIL!\nAF_read_val=0x%x;count_loop=%d;stage=%d,count_inf_fail=%d,count_macro_fail=%d\n",af_read_val,count_loop,stage,count_inf_fail,count_macro_fail);
    else
        ptr += sprintf( ptr, "AF Test PASS!\nAF_read_val=0x%x;count_loop=%d;stage=%d,count_inf_fail=%d,count_macro_fail=%d\n",af_read_val,count_loop,stage,count_inf_fail,count_macro_fail);

    *eof = 1;
    return ( ptr - page );
}

static int af_info_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char parabuf[64]={'\0'};
	int i=0,cnt;

	if (copy_from_user(parabuf, buffer, count))
        return -EFAULT;
	sscanf(parabuf, "%x %x %x %x %d", &macro_max,&macro_min,&inf_max,&inf_min,&fail_count) ;

	printk("tengdq macro_max=%x,macro_min=%x,inf_max=%x,inf_min=%x,fail_cnt=%d \n", macro_max,macro_min,inf_max,inf_min,fail_count);
	
	return count;
}

#endif

/* Kirby: add new-style driver {*/
static int LC898212AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int i4RetValue = 0;

    LC898212AFDB("[LC898212AF] LC898212AF_i2c_probe\n");

    /* Kirby: add new-style driver { */
    g_pstLC898212AF_I2Cclient = client;

    LC898212AFDB("[LC898212AF] g_pstLC898212AF_I2Cclient addr 1=%x\n",g_pstLC898212AF_I2Cclient->addr);
	
    
    //g_pstLC898212AF_I2Cclient->addr = g_pstLC898212AF_I2Cclient->addr >> 1;

    //LC898212AFDB("[LC898212AF] g_pstLC898212AF_I2Cclient addr 2=%x\n",g_pstLC898212AF_I2Cclient->addr);

    
    //Register char driver
    i4RetValue = Register_LC898212AF_CharDrv();

    if(i4RetValue){

        LC898212AFDB("[LC898212AF] register char device failed!\n");

        return i4RetValue;
    }

    spin_lock_init(&g_LC898212AF_SpinLock);
	
#ifdef AF_INFO_PROC
	// Create proc file system
	af_info_proc = create_proc_entry( AF_INFO_PROC_FILE , 0666, NULL);

	if (af_info_proc == NULL )
	{
		printk("create_proc_entry %s failed\n", AF_INFO_PROC_FILE );
	}
	else 
	{
		af_info_proc ->read_proc = af_info_read_proc;
		af_info_proc ->write_proc = af_info_write_proc;
	}
#endif

    LC898212AFDB("[LC898212AF] Attached!! \n");

    return 0;
}

static int LC898212AF_probe(struct platform_device *pdev)
{
    LC898212AFDB("[LC898212AF] LC898212AF_probe!! \n");

	//i2c_add_driver(&AF_EEPROM_i2c_driver);

    return i2c_add_driver(&LC898212AF_i2c_driver);
}

static int LC898212AF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&LC898212AF_i2c_driver);
   // i2c_del_driver(&AF_EEPROM_i2c_driver);
    return 0;
}

static int LC898212AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int LC898212AF_resume(struct platform_device *pdev)
{
    return 0;
}

// platform structure
static struct platform_driver g_stLC898212AF_Driver = {
    .probe		= LC898212AF_probe,
    .remove	= LC898212AF_remove,
    .suspend	= LC898212AF_suspend,
    .resume	= LC898212AF_resume,
    .driver		= {
        .name	= "lens_actuator",
        .owner	= THIS_MODULE,
    }
};

static int __init LC898212AF_i2C_init(void)
{
        LC898212AFDB("LC898212AF_i2C_init\n");


    i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
    //i2c_register_board_info(LENS_I2C_BUSNUM, &af_eeprom_dev, 1);
	
    if(platform_driver_register(&g_stLC898212AF_Driver)){
        LC898212AFDB("failed to register LC898212AF driver\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit LC898212AF_i2C_exit(void)
{
	platform_driver_unregister(&g_stLC898212AF_Driver);
}

module_init(LC898212AF_i2C_init);
module_exit(LC898212AF_i2C_exit);

MODULE_DESCRIPTION("LC898212AF lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");


