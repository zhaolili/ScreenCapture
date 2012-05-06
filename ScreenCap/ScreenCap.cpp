/*
Author:			Zhao Lili <zhao86.job@gmail.com, v-liliz@microsoft.com>
Date:			Mar. 31, 2011
Declaration:	Capture the screen and save it as a YUV file.
*/

#include "stdafx.h"

//#define	_MMX
#define ENABLE_TIMER
#define  MAXVALUE 256
#define	ABS(x) (x>0 ? x : -x)
#define	CLIP(x, a, b) ((x)>a ? ((x)<b ? x : b) : a)

INT		iYR[MAXVALUE], iYG[MAXVALUE], iYB[MAXVALUE];
INT		iUR[MAXVALUE], iUG[MAXVALUE], iUB[MAXVALUE];
INT		iVR[MAXVALUE], iVG[MAXVALUE], iVB[MAXVALUE];

LARGE_INTEGER frequency;
LARGE_INTEGER startime;
LARGE_INTEGER endtime;

VOID table_init()
{
	int		j;
//#ifdef _MMX
//	__m128i	mmxyr, mmxyg, mmxyb, mmxur, mmxug, mmxub, mmxvr, mmxvg, mmxvb;
//	__m128i	mmxj, mmxconst; 
//
//	mmxyr	= _mm_set1_epi32(66);
//	mmxyg	= _mm_set1_epi32(129);
//	mmxyb	= _mm_set1_epi32(15);
//	mmxur	= _mm_set1_epi32(-38);
//	mmxug	= _mm_set1_epi32(-74);
//	mmxub	= _mm_set1_epi32(112);
//	mmxvr	= _mm_set1_epi32(112);
//	mmxvg	= _mm_set1_epi32(-94);
//	mmxvb	= _mm_set1_epi32(-18);
//	for ()
//	{
//		mmxj	= _mm_set_epi32(j+3, j+2, j+1, j);
//
//	}
//
//#else
	for (j=0; j<MAXVALUE; j++)
	{
		iYR[j] = (j*66);
		iYG[j] = (j*129);
		iYB[j] = (j*25);

		iUR[j] = (-j*38);
		iUG[j] = (-j*74);
		iUB[j] = (j*112);

		iVR[j] = (j*112);
		iVG[j] = (-j*94);
		iVB[j] = (-j*18);
		/*iYR[j] = (j<<6)+(j<<1);
		iYG[j] = (j<<7)+j;
		iYB[j] = (j<<4)+(j<<3)+j;

		iUR[j] = -((j<<5)+(j<<2)+(j<<1));
		iUG[j] = -((j<<6)+(j<<3)+(j<<1));
		iUB[j] = -((j<<6)+(j<<5)+(j<<4));

		iVR[j] = (j<<6)+(j<<5)+(j<<4);
		iVG[j] = (-j*94);
		iVB[j] = (-j*18);*/
	}
//#endif
}
/*
Description: convert RGBA to YUV 4:2:0
pDest:	pointer to converted YUV buffer
pSrc:	pointer to RGB source
iWidth:	image width in pixels
iHeight:	image height in pixels
iXsize:	space occupied by one line in bytes, 4-bytes aligned
*/
DWORD rgba2yuv420(BYTE *&pDest, BYTE *pSrc, WORD iWidth, WORD iHeight, WORD iXsize, BOOL bTopDown)
{
	DWORD dwImgSize;
	DWORD	dwYCompSize;
	//calculate buffer size
	dwYCompSize	= iWidth*iHeight;
	dwImgSize		= dwYCompSize+(dwYCompSize>>1);
	iWidth	<<= 2;

	//memory allocation
	if (!pDest)
	{
		pDest = (BYTE *)calloc(dwImgSize, sizeof(BYTE));
		assert(pDest);
	}

#ifdef ENABLE_TIMER
	QueryPerformanceCounter(&startime);
#endif

	SHORT		cx, cy;
	LONG			ycnt, ucnt, vcnt;
	INT				sy, su, sv;	
	BYTE			*pYComp, *pUComp, *pVComp;
	BYTE			*pRGB = pSrc;
	//point to Y, U, V components 
	pYComp	= pDest;
	pUComp	= pDest + dwYCompSize;
	pVComp	= pUComp + (dwYCompSize>>2);

	//table initialization
	table_init();

	//conversion
	if (bTopDown)			//top down
	{
		ycnt = 0;
		for (cy=0; cy<iHeight; cy++)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<iWidth; cx+=4)
			{
				sy		=	((iYR[pRGB[cx+2]] + iYG[pRGB[cx+1]] + iYB[pRGB[cx]]+128)>>8) +16;
				pYComp[ycnt++]	= CLIP(sy, 0, 255);
			}
		}
		ucnt = vcnt = 0;
		for (cy=0; cy<iHeight; cy+=2)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<iWidth; cx+=8)
			{
				su		= ((iUR[pRGB[cx+2]] + iUG[pRGB[cx+1]] + iUB[pRGB[cx]]+128)>>8)+128;
				sv		= ((iVR[pRGB[cx+2]] + iVG[pRGB[cx+1]] + iVB[pRGB[cx]]+128)>>8)+128;
				pUComp[ucnt++]	= CLIP(su, 0, 255);				
				pVComp[vcnt++]	= CLIP(sv, 0, 255);
			}
		}
	}
	else							//bottom up
	{
#ifdef  _MMX
		WORD	wInteg	=	(iWidth>>6)<<6;
		INT				sy1, sy2, sy3, sy4, sy5, sy6, sy7;	
		__m128i	mmxycomp, mmxucomp, mmxcomp;
		__m64		mmx0, mmx1;
		ycnt = 0;
		for (cy=iHeight-1; cy>=0; cy--)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<wInteg; cx+=64)
			{
				sy		=	((iYR[pRGB[cx+2]] + iYG[pRGB[cx+1]] + iYB[pRGB[cx]]+128)>>8) +16;
				sy1	=	((iYR[pRGB[cx+6]] + iYG[pRGB[cx+5]] + iYB[pRGB[cx+4]]+128)>>8) +16;
				sy2	=	((iYR[pRGB[cx+10]] + iYG[pRGB[cx+9]] + iYB[pRGB[cx+8]]+128)>>8) +16;
				sy3	=	((iYR[pRGB[cx+14]] + iYG[pRGB[cx+13]] + iYB[pRGB[cx+12]]+128)>>8) +16;
				sy4	= ((iYR[pRGB[cx+18]] + iYG[pRGB[cx+17]] + iYB[pRGB[cx+16]]+128)>>8) +16;
				sy5	= ((iYR[pRGB[cx+22]] + iYG[pRGB[cx+21]] + iYB[pRGB[cx+20]]+128)>>8) +16;
				sy6	= ((iYR[pRGB[cx+26]] + iYG[pRGB[cx+25]] + iYB[pRGB[cx+24]]+128)>>8) +16;
				sy7	= ((iYR[pRGB[cx+30]] + iYG[pRGB[cx+29]] + iYB[pRGB[cx+28]]+128)>>8) +16;
				sy		=	CLIP(sy, 0, 255);
				sy1	=	CLIP(sy1, 0, 255);
				sy2	=	CLIP(sy2, 0, 255);
				sy3	=	CLIP(sy3, 0, 255);
				sy4	=	CLIP(sy4, 0, 255);
				sy5	=	CLIP(sy5, 0, 255);
				sy6	=	CLIP(sy6, 0, 255);
				sy7	=	CLIP(sy7, 0, 255);
				mmx0	= _mm_set_pi8(sy7, sy6, sy5, sy4, sy3, sy2, sy1, sy);

				sy		=	((iYR[pRGB[cx+34]] + iYG[pRGB[cx+33]] + iYB[pRGB[cx+32]]+128)>>8) +16;
				sy1	=	((iYR[pRGB[cx+38]] + iYG[pRGB[cx+37]] + iYB[pRGB[cx+36]]+128)>>8) +16;
				sy2	=	((iYR[pRGB[cx+42]] + iYG[pRGB[cx+41]] + iYB[pRGB[cx+40]]+128)>>8) +16;
				sy3	=	((iYR[pRGB[cx+46]] + iYG[pRGB[cx+45]] + iYB[pRGB[cx+44]]+128)>>8) +16;
				sy4	= ((iYR[pRGB[cx+50]] + iYG[pRGB[cx+49]] + iYB[pRGB[cx+48]]+128)>>8) +16;
				sy5	= ((iYR[pRGB[cx+54]] + iYG[pRGB[cx+53]] + iYB[pRGB[cx+52]]+128)>>8) +16;
				sy6	= ((iYR[pRGB[cx+58]] + iYG[pRGB[cx+57]] + iYB[pRGB[cx+56]]+128)>>8) +16;
				sy7	= ((iYR[pRGB[cx+62]] + iYG[pRGB[cx+61]] + iYB[pRGB[cx+60]]+128)>>8) +16;
				sy		=	CLIP(sy, 0, 255);
				sy1	=	CLIP(sy1, 0, 255);
				sy2	=	CLIP(sy2, 0, 255);
				sy3	=	CLIP(sy3, 0, 255);
				sy4	=	CLIP(sy4, 0, 255);
				sy5	=	CLIP(sy5, 0, 255);
				sy6	=	CLIP(sy6, 0, 255);
				sy7	=	CLIP(sy7, 0, 255);
				mmx1	= _mm_set_pi8(sy7, sy6, sy5, sy4, sy3, sy2, sy1, sy);

				mmxycomp	= _mm_setr_epi64(mmx0, mmx1);
				_mm_store_si128((__m128i *)(pYComp+ycnt), mmxycomp);
				/*mmxycomp	= _mm_min_epi32(_mm_set_epi32(sy3, sy2, sy1, sy), mmx255);
				mmxycomp	= _mm_max_epi32(mmxycomp, _mm_setzero_si128());*/

				ycnt += 16;
				//pYComp[ycnt++]	= CLIP(sy, 0, 255);
			}
			for (cx = wInteg; cx<iWidth; cx+=4)
			{
				sy		=	((iYR[pRGB[cx+2]] + iYG[pRGB[cx+1]] + iYB[pRGB[cx]]+128)>>8) +16;
				pYComp[ycnt++]	= CLIP(sy, 0, 255);
			}
		}
		ucnt = vcnt = 0;
		for (cy=iHeight-1; cy>=0; cy-=2)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=4; cx<iWidth; cx+=8)
			{
				su		= ((iUR[pRGB[cx+2]] + iUG[pRGB[cx+1]] + iUB[pRGB[cx]]+128)>>8)+128;
				sv		= ((iVR[pRGB[cx+2]] + iVG[pRGB[cx+1]] + iVB[pRGB[cx]]+128)>>8)+128;
				pUComp[ucnt++]	= CLIP(su, 0, 255);				
				pVComp[vcnt++]	= CLIP(sv, 0, 255);

				/*pUComp[ucnt++]	= ((iUR[pRGB[cx+2]] + iUG[pRGB[cx+1]] + iUB[pRGB[cx]]+128)>>8)+128;
				pVComp[vcnt++]	= ((iVR[pRGB[cx+2]] + iVG[pRGB[cx+1]] + iVB[pRGB[cx]]+128)>>8)+128;*/
			}
		}
#else
		ycnt = 0;
		for (cy=iHeight-1; cy>=0; cy--)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<iWidth; cx+=4)
			{
				sy		=	((iYR[pRGB[cx+2]] + iYG[pRGB[cx+1]] + iYB[pRGB[cx]]+128)>>8) +16;
				pYComp[ycnt++]	= CLIP(sy, 0, 255);
			}
		}
		ucnt = vcnt = 0;
		for (cy=iHeight-1; cy>=0; cy-=2)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=4; cx<iWidth; cx+=8)
			{
				su		= ((iUR[pRGB[cx+2]] + iUG[pRGB[cx+1]] + iUB[pRGB[cx]]+128)>>8)+128;
				sv		= ((iVR[pRGB[cx+2]] + iVG[pRGB[cx+1]] + iVB[pRGB[cx]]+128)>>8)+128;
				pUComp[ucnt++]	= CLIP(su, 0, 255);				
				pVComp[vcnt++]	= CLIP(sv, 0, 255);
			}
		}
#endif
	}//else	

#ifdef ENABLE_TIMER
	QueryPerformanceCounter(&endtime);
	QueryPerformanceFrequency(&frequency);
#ifdef _MMX
	long			fElapTime		= (endtime.QuadPart - startime.QuadPart);
	fprintf(stdout, "freq.:	%ld\n", frequency.QuadPart);
	fprintf(stdout, "%ld\n",  fElapTime);
#else
	float			fElapTime		= (endtime.QuadPart - startime.QuadPart);
	fprintf(stdout, "%f	s\n", fElapTime/frequency.QuadPart);
#endif
#endif
	// return yuv buffer size
	return dwImgSize;		
}

/*
Description: convert RGB to YUV 4:2:0
pDest:	pointer to converted YUV buffer
pSrc:	pointer to RGB source
iWidth:	image width in pixels
iHeight:	image height in pixels
iXsize:	space occupied by one line in bytes, 4-bytes aligned
*/
DWORD rgb2yuv420(BYTE *&pDest, BYTE *pSrc, WORD iWidth, WORD iHeight, WORD iXsize, BOOL bTopDown)
{
	DWORD	dwImgSize;
	DWORD	dwYCompSize;

	//calculate buffer size
	dwYCompSize	= iWidth*iHeight;
	dwImgSize		= dwYCompSize + (dwYCompSize>>1);

	//memory allocation
	if (!pDest)
	{
		pDest	= (BYTE *)calloc(dwImgSize, sizeof(BYTE));
		assert(pDest);
	}

	SHORT		cx, cy;
	LONG			ycnt, ucnt, vcnt;
	INT				sy, su, sv;	
	BYTE			*pYComp, *pUComp, *pVComp;
	BYTE			*pRGB = pSrc;

	//point to Y, U, V components
	pYComp	= pDest;
	pUComp	= pDest +	dwYCompSize;
	pVComp	= pUComp + (dwYCompSize>>2);

	//table initialization
	table_init();

	//conversion
	if (bTopDown)
	{
		ycnt = 0;
		for (cy=0; cy<iHeight; cy++)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<iWidth; cx+=3)
			{
				sy		=	((iYR[pRGB[cx+2]] + iYG[pRGB[cx+1]] + iYB[pRGB[cx]]+128)>>8) +16;
				pYComp[ycnt++]	= CLIP(sy, 0, 255);
			}
		}
		ucnt = vcnt = 0;
		for (cy=0; cy<iHeight; cy+=2)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<iWidth; cx+=6)
			{
				su		= ((iUR[pRGB[cx+2]] + iUG[pRGB[cx+1]] + iUB[pRGB[cx]]+128)>>8)+128;
				sv		= ((iVR[pRGB[cx+2]] + iVG[pRGB[cx+1]] + iVB[pRGB[cx]]+128)>>8)+128;
				pUComp[ucnt++]	= CLIP(su, 0, 255);				
				pVComp[vcnt++]	= CLIP(sv, 0, 255);
			}
		}
	}
	else
	{
		ycnt = 0;
		for (cy=iHeight-1; cy>=0; cy--)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=0; cx<iWidth; cx+=3)
			{
				sy		=	((iYR[pRGB[cx+2]] + iYG[pRGB[cx+1]] + iYB[pRGB[cx]]+128)>>8) +16;
				pYComp[ycnt++]	= CLIP(sy, 0, 255);
			}
		}
		ucnt = vcnt = 0;
		for (cy=iHeight-1; cy>=0; cy-=2)
		{
			pRGB = pSrc + cy*iXsize;
			for (cx=4; cx<iWidth; cx+=6)
			{
				su		= ((iUR[pRGB[cx+2]] + iUG[pRGB[cx+1]] + iUB[pRGB[cx]]+128)>>8)+128;
				sv		= ((iVR[pRGB[cx+2]] + iVG[pRGB[cx+1]] + iVB[pRGB[cx]]+128)>>8)+128;
				pUComp[ucnt++]	= CLIP(su, 0, 255);				
				pVComp[vcnt++]	= CLIP(sv, 0, 255);
			}
		}
	}

	return dwImgSize;
}

DWORD get_screen_pixel(HBITMAP hBitmap, BYTE *&pBufferYUV)
{
	HDC								hdc;
	LPVOID							pBuf = NULL;				//pixel buffer
	BITMAPINFO				bmpInfo;			
	//BITMAPINFOHEADER	bmpFileHeader;
	DWORD							dwImgSize;					//space occupied by image, 4-bytes aligned
	WORD							wImgWid;						//space occupied by each line, 4-bytes aligned
	BOOL								bTopDown = FALSE;					//top-down dib or bottom-up dib

	hdc	= GetDC(NULL);
	ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	//fill  the bitmapinfo structure
	GetDIBits(hdc, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

	// biSizeImage may be set to zero for BI_RGB compression, so here we calculate image size manually
	assert(bmpInfo.bmiHeader.biBitCount>=24);				//24bitRGB or RGBA
	wImgWid		= (WORD)bmpInfo.bmiHeader.biWidth*((bmpInfo.bmiHeader.biBitCount+7)>>3);
	wImgWid		= (((wImgWid<<3)+31)>>5)<<2;
	dwImgSize	= ABS(bmpInfo.bmiHeader.biHeight)*wImgWid;

	if (bmpInfo.bmiHeader.biHeight < 0)
		bTopDown = TRUE;

	//memory allocation 
	pBuf	= (BYTE *)calloc(dwImgSize, sizeof(BYTE));
	assert(pBuf);
	
	//get data 
	bmpInfo.bmiHeader.biCompression	= BI_RGB;
	GetDIBits(hdc, hBitmap, 0, bmpInfo.bmiHeader.biHeight, pBuf, &bmpInfo, DIB_RGB_COLORS);

	//convert from RGB to YUV420
	dwImgSize = rgba2yuv420(pBufferYUV, (BYTE *)pBuf, bmpInfo.bmiHeader.biWidth, ABS(bmpInfo.bmiHeader.biHeight), wImgWid, bTopDown);

	//release resource
	if (hdc)
		ReleaseDC(NULL, hdc);
	if (pBuf)
		free(pBuf);

	//return yuv buffer size
	return dwImgSize;
}

VOID capture_screen(BYTE *&pYUVBuffer, DWORD &rFileSize)
{
	//screen size
	int		iScreenWidth	= GetSystemMetrics(SM_CXSCREEN);
	int		iScreenHeight	= GetSystemMetrics(SM_CYSCREEN);

	//get desktop handle and DC
	HWND	hDesktopWnd	= GetDesktopWindow();
	HDC		hDesktopDC	= GetDC(hDesktopWnd);

	//create a compatible DC and bmp
	HDC			hCaptureDC			= CreateCompatibleDC(hDesktopDC);
	HBITMAP	hCaptureBitmap	= CreateCompatibleBitmap(hDesktopDC, iScreenWidth, iScreenHeight);
	
	SelectObject(hCaptureDC, hCaptureBitmap);
	BitBlt(hCaptureDC, 0, 0, iScreenWidth, iScreenHeight, hDesktopDC, 0, 0, SRCCOPY);

	//save screen pixels
	rFileSize = get_screen_pixel(hCaptureBitmap, pYUVBuffer);

	//release
	ReleaseDC(hDesktopWnd, hDesktopDC);
	DeleteDC(hCaptureDC);
	DeleteObject(hCaptureBitmap);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<2)
	{
		printf("[exe] [yuv file]\n");
		return 0;
	}
	char		*cfilename=argv[1];
	FILE	*fp;
	if (!(fp = fopen(cfilename, "wb")))
	{
		printf("open output file failed.\n");
		return 0;
	}

	BYTE		*pYUVBuffer = NULL;
	DWORD	rFileSize;
	
	capture_screen(pYUVBuffer, rFileSize);

	if (1!= fwrite(pYUVBuffer,  rFileSize, sizeof(BYTE), fp))
	{
		printf("%d	file writing failed.\n", rFileSize);
		exit(-1);
	}

	if (pYUVBuffer)
		free(pYUVBuffer);
	if (fp)
	{
		fflush(fp);
		fclose(fp);
	}

	return 0;
}

