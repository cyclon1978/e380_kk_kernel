//********************************************************************************
//
//		<< LC89821x Step Move module >>
//	    Program Name	: AfSTMV.c
//		Design			: Y.Yamada
//		History			: First edition						2009.07.31 Y.Tashita
//		History			: LC898211 changes					2012.06.11 YS.Kim
//		History			: LC898212 changes					2013.07.19 Rex.Tang
//********************************************************************************
//**************************
//	Include Header File		
//**************************
#include	"AfSTMV.h"
#include	"AfDef.h"
//#include	"AfInter.h"

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

extern u8 s4LC898212AF_Reg_Write(u8 addr, u8 data);
extern u8 s4LC898212AF_Reg_Read(u8 addr);
extern u16 s4LC898212AF_RAM_Read(u8 addr);
extern u8  s4LC898212AF_RAM_Write(u8 addr, u16 data);

//**************************
//	Definations					
//**************************
#define	abs(x)	((x) < 0 ? -(x) : (x))
#define	LC898211_fs	234375

/*--------------------------
    Local defination
---------------------------*/
static 	stSmvPar StSmvPar;

/* Step Move to Finish Check Function */
static 	unsigned char	StmvEnd( unsigned char ) ;


/*-----------------------------------------------------------
    Function Name   : StmvSet, StmvTo, StmvEnd
	Description     : StepMove Setting, Execute, Finish, Current Limit 
    Arguments       : Stepmove Mode Parameter
	Return			: Stepmove Mode Parameter
-----------------------------------------------------------*/
//StmvSet -> StmvTo -> StmvEnd -> StmvTo -> StmvEnd ->ÅEÅEÅE

//********************************************************************************
// Function Name 	: StmvSet
// Retun Value		: NON
// Argment Value	: NON
// Explanation		: Stpmove parameter Setting Function
// History			: First edition 						2012.06.12 YS.Kim
// History			: Changes								2013.07.19 Rex.Tang
//********************************************************************************
void StmvSet( stSmvPar StSetSmv )
{
	unsigned char	UcSetEnb;
	unsigned char	UcSetSwt;
	unsigned short	UsParSiz;
	unsigned char	UcParItv;
    short 			SsParStt;	// StepMove Start Position

    StSmvPar.UsSmvSiz = StSetSmv.UsSmvSiz;
    StSmvPar.UcSmvItv = StSetSmv.UcSmvItv;
    StSmvPar.UcSmvEnb = StSetSmv.UcSmvEnb;
    
	s4LC898212AF_Reg_Write( AFSEND_211	, 0x00 );										// StepMove Enable Bit Clear
	
	UcSetEnb=s4LC898212AF_Reg_Read(ENBL_211);
	UcSetEnb 	&= (unsigned char)0xFD ;
	s4LC898212AF_Reg_Write( ENBL_211	,	UcSetEnb );										// Measuremenet Circuit1 Off
	
	UcSetSwt=s4LC898212AF_Reg_Read(SWTCH_211);
	UcSetSwt	&= (unsigned char)0x7F ;
	s4LC898212AF_Reg_Write( SWTCH_211 , UcSetSwt );										// RZ1 Switch Cut Off
	
	SsParStt=s4LC898212AF_RAM_Read( RZ_211H);										// Get Start Position
	UsParSiz	= StSetSmv.UsSmvSiz ;										// Get StepSize
	UcParItv	= StSetSmv.UcSmvItv ;										// Get StepInterval
	
	s4LC898212AF_RAM_Write( ms11a_211H	, (unsigned short)0x0800 );						// Set Coefficient Value For StepMove
	s4LC898212AF_RAM_Write( MS1Z22_211H	, (unsigned short)SsParStt );					// Set Start Positon
	s4LC898212AF_RAM_Write( MS1Z12_211H	, UsParSiz );									// Set StepSize
	s4LC898212AF_Reg_Write( STMINT_211	, UcParItv );									// Set StepInterval
	
	UcSetSwt	|= (unsigned char)0x80;
	s4LC898212AF_Reg_Write( SWTCH_211, UcSetSwt );										// RZ1 Switch ON
}



//********************************************************************************
// Function Name 	: StmvTo
// Retun Value		: Stepmove Parameter
// Argment Value	: Stepmove Parameter, Target Position
// Explanation		: Stpmove Function
// History			: First edition 						2012.06.12 YS.Kim
// History			: Changes								2013.07.19 Rex.Tang
//********************************************************************************
unsigned char StmvTo( short SsSmvEnd )
{
	unsigned short	UsSmvTim;
	unsigned short	UsSmvDpl;
    short 			SsParStt;	// StepMove Start Position
	
	//PIOA_SetOutput(_PIO_PA29);													// Monitor I/O Port
	
	SsParStt=s4LC898212AF_RAM_Read( RZ_211H );											// Get Start Position
	UsSmvDpl = abs( SsParStt - SsSmvEnd );
	
	if( ( UsSmvDpl <= StSmvPar.UsSmvSiz ) && (( StSmvPar.UcSmvEnb & STMSV_ON ) == STMSV_ON ) ){
		if( StSmvPar.UcSmvEnb & STMCHTG_ON ){
			s4LC898212AF_Reg_Write( MSSET_211	, INI_MSSET_211 | (unsigned char)0x01 );
		}
		s4LC898212AF_RAM_Write( MS1Z22_211H, SsSmvEnd );										// Handling Single Step For ES1
		StSmvPar.UcSmvEnb	|= STMVEN_ON;										// Combine StepMove Enable Bit & StepMove Mode Bit
	} else {
		if( SsParStt < SsSmvEnd ){												// Check StepMove Direction
			s4LC898212AF_RAM_Write( MS1Z12_211H	, StSmvPar.UsSmvSiz );
		} else if( SsParStt > SsSmvEnd ){
			s4LC898212AF_RAM_Write( MS1Z12_211H	, -StSmvPar.UsSmvSiz );
		}
		
		s4LC898212AF_RAM_Write( STMVENDH_211, SsSmvEnd );									// Set StepMove Target Positon
		StSmvPar.UcSmvEnb	|= STMVEN_ON;										// Combine StepMove Enable Bit & StepMove Mode Bit
		s4LC898212AF_Reg_Write( STMVEN_211	, StSmvPar.UcSmvEnb );							// Start StepMove
	}
	
	UsSmvTim=(UsSmvDpl/STMV_SIZE)*((STMV_INTERVAL+1)*10000 / LC898211_fs);			// Stepmove Operation time
	mdelay( UsSmvTim );
	//TRACE("STMV Operation Time = %d \n", UsSmvTim ) ;
	
	return StmvEnd( StSmvPar.UcSmvEnb );
}



//********************************************************************************
// Function Name 	: StmvEnd
// Retun Value		: Stepmove Parameter
// Argment Value	: Stepmove Parameter
// Explanation		: Stpmove Finish Check Function
// History			: First edition 						2012.06.12 YS.Kim
// History			: Changes								2013.07.19 Rex.Tang
//********************************************************************************
unsigned char StmvEnd( unsigned char UcParMod )
{
	unsigned char	UcChtGst;
	unsigned short  i = 0;
	
	while( (UcParMod & (unsigned char)STMVEN_ON ) && (i++ < 100))								// Wait StepMove operation end
	{
		UcParMod=s4LC898212AF_Reg_Read( STMVEN_211);
	}

	if( ( UcParMod & (unsigned char)0x08 ) == (unsigned char)STMCHTG_ON ){		// If Convergence Judgement is Enabled
        for(i=0; i<CHTGOKN_WAIT; i++)
		{
        	UcChtGst=s4LC898212AF_Reg_Read( MSSET_211);
        	if(!(UcChtGst & 0x01))	break;
        	mdelay(1);	
		}
	}
	
	if( UcChtGst & 0x01 ){
		UcParMod	|= (unsigned char)0x80 ;									// STMV Success But Settling Time Over
		//PIOA_ClearOutput(_PIO_PA29);											// Monitor I/O Port
	}else{
		UcParMod	&= (unsigned char)0x7F ;									// STMV Success 
	}
	
	return UcParMod;															// Bit0:0 Successful convergence Bit0:1 Time Over
}


