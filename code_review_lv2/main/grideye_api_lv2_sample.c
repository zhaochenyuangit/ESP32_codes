/*******************************************************************************
 Copyright (C) <2015>, <Panasonic Corporation>
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3.	The name of copyright holders may not be used to endorse or promote products derived from this software without specific prior written permission.
4.	This software code may only be redistributed and used in connection with a grid-eye product.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS �AS IS� AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR POFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND OR ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the authors and should not be interpreted as representing official policies, either expressed or implied, of the FreeBSD Project. 

 ******************************************************************************/

/*******************************************************************************
	include file
*******************************************************************************/
#include	"grideye_api_common.h"
#include	"grideye_api_lv1.h"
#include	"grideye_api_lv2.h"


/*******************************************************************************
	macro definition
*******************************************************************************/
#define		TRUE				(1)
#define		FALSE				(0)

/* Grid-EYE's I2C slave address */
//#define		GRIDEYE_ADR_GND		(0xD0)	/* AD_SELECT pin connect to GND		*/
//#define		GRIDEYE_ADR_VDD		(0xD2)	/* AD_SELECT pin connect to VDD		*/
#define		GRIDEYE_ADR			(0x69)

/* Grid-EYE's register address */
#define		GRIDEYE_REG_THS00	(0x0E)	/* head address of thermistor  resister	*/
#define		GRIDEYE_REG_TMP00	(0x80)	/* head address of temperature resister	*/

/* Grid-EYE's register size */
#define		GRIDEYE_REGSZ_THS	(0x02)	/* size of thermistor  resister		*/
#define		GRIDEYE_REGSZ_TMP	(0x80)	/* size of temperature resister		*/

/* Grid-EYE's number of pixels */
#define		SNR_SZ_X			(8)
#define		SNR_SZ_Y			(8)
#define		SNR_SZ				(SNR_SZ_X * SNR_SZ_Y)

/* Setting size of human detection */
#define		IMG_SZ_X			(SNR_SZ_X * 2 - 1)
#define		IMG_SZ_Y			(SNR_SZ_Y * 2 - 1)
#define		IMG_SZ				(IMG_SZ_X * IMG_SZ_Y)
#define		OUT_SZ_X			(8)
#define		OUT_SZ_Y			(8)
#define		OUT_SZ				(OUT_SZ_X * OUT_SZ_Y)

/* Parameters of human detection */
#define		TEMP_FRAME_NUM		(8)
#define		TEMP_MEDIAN_FILTER	(2)
#define		TEMP_SMOOTH_COEFF	(0.6f)
#define		DIFFTEMP_THRESH		(2.0f)
#define		DETECT_MARK			((UCHAR)0xFF)
#define		LABELING_THRESH		(3)
#define		OUTPUT_THRESH		(6)
#define		BKUPDT_COEFF		(0.1f)


/*******************************************************************************
	variable value definition
*******************************************************************************/

#if	defined(MCU_TEST)
extern ULONG	g_ulFrameNum;
extern short	g_a2shRawTemp[TEMP_FRAME_NUM][SNR_SZ];
extern short	g_ashSnrAveTemp[SNR_SZ];
extern short	g_ashAveTemp   [IMG_SZ];
extern short	g_ashBackTemp  [IMG_SZ];
extern short	g_ashDiffTemp  [IMG_SZ];
extern UCHAR	g_aucDetectImg [IMG_SZ];
extern UCHAR	g_aucOutputImg [OUT_SZ];
extern USHORT	g_ausWork      [IMG_SZ];
#else	/* !defined(MCU_TEST) */
ULONG	g_ulFrameNum = 0;
short	g_a2shRawTemp[TEMP_FRAME_NUM][SNR_SZ];
short	g_ashSnrAveTemp[SNR_SZ];
short	g_ashAveTemp   [IMG_SZ];
short	g_ashBackTemp  [IMG_SZ];
short	g_ashDiffTemp  [IMG_SZ];
UCHAR	g_aucDetectImg [IMG_SZ];
UCHAR	g_aucOutputImg [OUT_SZ];
USHORT	g_ausWork      [IMG_SZ];
#endif	/*  defined(MCU_TEST) */


/*******************************************************************************
	method
 ******************************************************************************/

/*------------------------------------------------------------------------------
	Sample program of human detection.
------------------------------------------------------------------------------*/
BOOL bAMG_PUB_SMP_InitializeHumanDetectLv2Sample( void )
{
	USHORT	usCnt = 0;

	/* Initialize data */
	g_ulFrameNum = 0;
	for( usCnt = 0; usCnt < SNR_SZ; usCnt++ )
	{
		UCHAR ucFrameCnt = 0;
		for( ucFrameCnt = 0; ucFrameCnt < TEMP_FRAME_NUM; ucFrameCnt++ )
		{
			g_a2shRawTemp[ucFrameCnt][usCnt] = 0;
		}
		g_ashSnrAveTemp[usCnt] = 0;
	}
	for( usCnt = 0; usCnt < IMG_SZ; usCnt++ )
	{
		g_ashAveTemp   [usCnt] = 0;
		g_ashBackTemp  [usCnt] = 0;
		g_ashDiffTemp  [usCnt] = 0;
		g_aucDetectImg [usCnt] = 0;
	}
	for( usCnt = 0; usCnt < OUT_SZ; usCnt++ )
	{
		g_aucOutputImg [usCnt] = 0;
	}

	return( TRUE );
}

/*------------------------------------------------------------------------------
	Sample program of human detection.
------------------------------------------------------------------------------*/
BOOL bAMG_PUB_SMP_ExecuteHumanDetectLv2Sample( void )
{
	USHORT	usCnt = 0;
	UCHAR	ucDetectNum = 0;

	/* Get temperature register value. */
	if( ESP_OK != bAMG_PUB_I2C_Read( GRIDEYE_ADR, GRIDEYE_REG_TMP00, GRIDEYE_REGSZ_TMP, (UCHAR*)g_ausWork ) )
	{
		return( FALSE );				/* Communication NG */
	}

	/* Convert temperature register value. */
	vAMG_PUB_TMP_ConvTemperature64( (UCHAR*)g_ausWork, g_a2shRawTemp[g_ulFrameNum % TEMP_FRAME_NUM] );

	/* Increment number of measurement. */
	g_ulFrameNum++;
	if( TEMP_FRAME_NUM > g_ulFrameNum )
	{
		return( FALSE );				/* Initial process */
	}

	/* Calculate average. */
	for( usCnt = 0; usCnt < SNR_SZ; usCnt++ )
	{
		short shAveTemp = shAMG_PUB_CMN_CalcAve( &g_a2shRawTemp[0][usCnt], TEMP_FRAME_NUM, SNR_SZ, TEMP_MEDIAN_FILTER, (BOOL*)g_ausWork );
		if( TEMP_FRAME_NUM == g_ulFrameNum )
		{
			g_ashSnrAveTemp[usCnt] = shAveTemp;//第一次满8张照片，平均值直接赋值，
		}
		else // 之后是用滑动平均，逐张累加
		{
			g_ashSnrAveTemp[usCnt] = shAMG_PUB_CMN_CalcIIR( g_ashSnrAveTemp[usCnt], shAveTemp, shAMG_PUB_CMN_ConvFtoS(TEMP_SMOOTH_COEFF) );
		}
	}

	/* Linear interpolation. */ // 从8x8 扩展到15x15
	if( FALSE == bAMG_PUB_IMG_LinearInterpolationSQ15( g_ashSnrAveTemp, g_ashAveTemp ) )
	{
		return( FALSE );				/* Program NG */
	}

	/* Initialize background temperature. */
	if( TEMP_FRAME_NUM == g_ulFrameNum ) //以第一次满8张这一瞬间的温度初始化背景温度
	{
		for( usCnt = 0; usCnt < IMG_SZ; usCnt++ )
		{
			g_ashBackTemp[usCnt] = g_ashAveTemp[usCnt];
		}
		return( FALSE );				/* Initial process */
	}

	/* Object detection. */ //计算 （现在温度-背景温度） 之差，大于0.3度即认为有物体
	vAMG_PUB_ODT_CalcDiffImage( IMG_SZ, g_ashAveTemp, g_ashBackTemp, g_ashDiffTemp );
	vAMG_PUB_ODT_CalcDetectImage1( IMG_SZ, g_ashDiffTemp, shAMG_PUB_CMN_ConvFtoS(DIFFTEMP_THRESH), DETECT_MARK, g_aucDetectImg );

	/* Labeling. */
	ucDetectNum = ucAMG_PUB_ODT_CalcDataLabeling8( IMG_SZ_X, IMG_SZ_Y, DETECT_MARK, LABELING_THRESH, g_aucDetectImg, g_ausWork );

	/* Initialize output image. */
	for( usCnt = 0; usCnt < OUT_SZ; usCnt++ )
	{
		g_aucOutputImg[usCnt] = 0;
	}

	/* Calculate features and judge human. */
	for( usCnt = 1; usCnt <= ucDetectNum; usCnt++ )
	{
		USHORT	usArea = 0;
		UCHAR	aucCenter[2];
		short	ashCenter[2];
		/* Calculate features. */
		if( FALSE == bAMG_PUB_FEA_CalcArea( IMG_SZ, usCnt, g_aucDetectImg, &usArea ) )
		{
			return( FALSE );			/* Program NG */
		}
		/* Judge human. */
		if( TRUE == bAMG_PUB_HDT_JudgeHuman( usArea, OUTPUT_THRESH ) )
		{
			/* Calculate features. */
			if( FALSE == bAMG_PUB_FEA_CalcCenterTemp( IMG_SZ_X, IMG_SZ_Y, usCnt, g_aucDetectImg, g_ashDiffTemp, ashCenter ) )
			{
				return( FALSE );		/* Program NG */
			}
			/* Update output image. */
			if( FALSE == bAMG_PUB_OUT_CalcOutImage( IMG_SZ_X, IMG_SZ_Y, OUT_SZ_X, OUT_SZ_Y, ashCenter, aucCenter ) )
			{
				return( FALSE );		/* Program NG */
			}
			g_aucOutputImg[ aucCenter[0] + aucCenter[1] * OUT_SZ_X ] = 1;
		}
	}

	/* Update background temperature. */
	if( FALSE == bAMG_PUB_IMG_ImageDilation1( IMG_SZ_X, IMG_SZ_Y, g_aucDetectImg, (UCHAR*)g_ausWork ) )
	{
		return( FALSE );				/* Program NG */
	}
	if( FALSE == bAMG_PUB_BGT_UpdateBackTemp( IMG_SZ, (UCHAR*)g_ausWork, g_ashDiffTemp, shAMG_PUB_CMN_ConvFtoS(BKUPDT_COEFF), g_ashBackTemp ) )
	{
		/* Don't update background temperature for all pixels detection. */
	}

	return( TRUE );						/* human detection OK */
}
