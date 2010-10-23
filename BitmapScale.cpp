// BitmapScale.cpp: implementation of the CBitmapScale class.
//
//////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "BitmapScale.h"
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBitmapScale::CBitmapScale()
{

}

CBitmapScale::~CBitmapScale()
{

}

HBITMAP CBitmapScale::ScaleBitmapInt(HBITMAP hBmp, WORD wNewWidth, WORD wNewHeight)
{
	BITMAP bmp;
	::GetObject(hBmp, sizeof(BITMAP), &bmp);

	// check for valid size
	if((bmp.bmWidth > wNewWidth && bmp.bmHeight < wNewHeight) ||
		bmp.bmWidth < wNewWidth && bmp.bmHeight > wNewHeight)
		return NULL;

	HDC hDC = ::GetDC(NULL);
	BITMAPINFO *pbi = PrepareRGBBitmapInfo((WORD)bmp.bmWidth, (WORD)bmp.bmHeight);
	BYTE *pData = new BYTE[pbi->bmiHeader.biSizeImage];

	::GetDIBits(hDC, hBmp, 0, bmp.bmHeight, pData, pbi, DIB_RGB_COLORS);

	delete pbi;
	pbi = PrepareRGBBitmapInfo(wNewWidth, wNewHeight);
	BYTE *pData2 = new BYTE[pbi->bmiHeader.biSizeImage];

	if (bmp.bmWidth >= wNewWidth && bmp.bmHeight >= wNewHeight)
	{
		ShrinkDataInt(pData, 
			         (WORD)bmp.bmWidth, 
				     (WORD)bmp.bmHeight,
					 pData2, 
					 wNewWidth, 
					 wNewHeight);
	}
	else
	{
		EnlargeDataInt(pData, 0, 0,
                 (WORD)bmp.bmWidth, 
                 (WORD)bmp.bmHeight,
                 pData2, 
                 wNewWidth, 
                 wNewHeight);
	}

	delete pData;

	HBITMAP hResBmp = ::CreateCompatibleBitmap(hDC, wNewWidth, wNewHeight);
	::SetDIBits( hDC, 
				 hResBmp, 
				 0, 
				 wNewHeight, 
				 pData2, 
				 pbi, 
				 DIB_RGB_COLORS );

	::ReleaseDC(NULL, hDC);

	delete pbi;
	delete pData2;

	return hResBmp;
}


BITMAPINFO* CBitmapScale::PrepareRGBBitmapInfo(WORD wWidth, WORD wHeight)
{
	BITMAPINFO *pRes = new BITMAPINFO;
	::ZeroMemory(pRes, sizeof(BITMAPINFO));
	pRes->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pRes->bmiHeader.biWidth = wWidth;
	pRes->bmiHeader.biHeight = wHeight;
	pRes->bmiHeader.biPlanes = 1;
	pRes->bmiHeader.biBitCount = 24;

	pRes->bmiHeader.biSizeImage = ((3 * wWidth + 3) & ~3) * wHeight;
 
	return pRes;
}

int* CBitmapScale::CreateCoeffInt(int nLen, int nNewLen, BOOL bShrink)
{
	int nSum = 0, nSum2;
	int *pRes = new int[2 * nLen];
	int *pCoeff = pRes;
	int nNorm = (bShrink) ? (nNewLen << 12) / nLen : 0x1000;
	int	nDenom = (bShrink)? nLen : nNewLen;

	::ZeroMemory(pRes, 2 * nLen * sizeof(int));
	for(int i = 0; i < nLen; i++, pCoeff += 2)
	{
		nSum2 = nSum + nNewLen;
		if(nSum2 > nLen)
		{
			*pCoeff = ((nLen - nSum) << 12) / nDenom;
			pCoeff[1] = ((nSum2 - nLen) << 12) / nDenom;
			nSum2 -= nLen;
		}
		else
		{
			*pCoeff = nNorm;

			if(nSum2 == nLen)
			{
			    pCoeff[1] = -1;
				nSum2 = 0;
			}
		}

		nSum = nSum2;
	}
 
	return pRes;
}

void CBitmapScale::ShrinkDataInt( BYTE *pInBuff, 
								   WORD wWidth, 
								   WORD wHeight,
								   BYTE *pOutBuff, 
								   WORD wNewWidth, 
								   WORD wNewHeight )
{
	BYTE  *pLine = pInBuff, *pPix;
	BYTE  *pOutLine = pOutBuff;
	DWORD dwInLn = (3 * wWidth + 3) & ~3;
	DWORD dwOutLn = (3 * wNewWidth + 3) & ~3;
	int   x, y, i, ii;
	BOOL  bCrossRow, bCrossCol;
	
	int   *pRowCoeff = CreateCoeffInt(wWidth, wNewWidth, TRUE);
	int   *pColCoeff = CreateCoeffInt(wHeight, wNewHeight, TRUE);
	
	int   *pXCoeff, *pYCoeff = pColCoeff;
	DWORD dwBuffLn = 3 * wNewWidth * sizeof(DWORD);
	DWORD *pdwBuff = new DWORD[6 * wNewWidth];
	DWORD *pdwCurrLn = pdwBuff, *pdwCurrPix, *pdwNextLn = pdwBuff + 3 * wNewWidth;
	DWORD dwTmp, *pdwNextPix;

	::ZeroMemory(pdwBuff, 2 * dwBuffLn);

	y = 0;
	while(y < wNewHeight)
	{
		pPix = pLine;
		pLine += dwInLn;

		pdwCurrPix = pdwCurrLn;
		pdwNextPix = pdwNextLn;

		x = 0;
		pXCoeff = pRowCoeff;
		bCrossRow = pYCoeff[1] > 0;

		while(x < wNewWidth)
		{
			dwTmp = *pXCoeff * *pYCoeff;

			for(i = 0; i < 3; i++)
				pdwCurrPix[i] += dwTmp * pPix[i];

			bCrossCol = pXCoeff[1] > 0;
			if(bCrossCol)
			{
				dwTmp = pXCoeff[1] * *pYCoeff;
				for(i = 0, ii = 3; i < 3; i++, ii++)
					pdwCurrPix[ii] += dwTmp * pPix[i];
			}

			if(bCrossRow)
			{
				dwTmp = *pXCoeff * pYCoeff[1];
				for(i = 0; i < 3; i++)
					pdwNextPix[i] += dwTmp * pPix[i];
				if(bCrossCol)
				{
					dwTmp = pXCoeff[1] * pYCoeff[1];
					for(i = 0, ii = 3; i < 3; i++, ii++)
						pdwNextPix[ii] += dwTmp * pPix[i];
				}
			}
			
			if(pXCoeff[1])
			{
				x++;
				pdwCurrPix += 3;
				pdwNextPix += 3;
			}
			
			pXCoeff += 2;
			pPix += 3;
		} // while

		if(pYCoeff[1])
		{
			// set result line
			pdwCurrPix = pdwCurrLn;
			pPix = pOutLine;
			for(i = 3 * wNewWidth; i > 0; i--, pdwCurrPix++, pPix++)
				*pPix = ((LPBYTE)pdwCurrPix)[3];

			// prepare line buffers
			pdwCurrPix = pdwNextLn;
			pdwNextLn = pdwCurrLn;
			pdwCurrLn = pdwCurrPix;
			::ZeroMemory(pdwNextLn, dwBuffLn);

			y++;
			pOutLine += dwOutLn;
		}

		pYCoeff += 2;
	} // while

	delete [] pRowCoeff;
	delete [] pColCoeff;
	delete [] pdwBuff;
} 

///////////////////////////////////////////////////////////

void CBitmapScale::EnlargeDataInt( BYTE *pInBuff,
									WORD wLeft,
									WORD wTop,
									WORD wWidth, 
									WORD wHeight,
									BYTE *pOutBuff, 
									WORD wNewWidth, 
									WORD wNewHeight )
{
	// pLine, pPix point at the beggining of the bitmap buffer
	BYTE  *pLine = pInBuff, 
		  *pPix = pLine, 
		  *pPixOld, 
		  *pUpPix, 
		  *pUpPixOld;

	// pOutLine points at the beginning of pOutBuff, pOutPix is declared as BYTE*
	BYTE  *pOutLine = pOutBuff, *pOutPix;

	// dwLineIn is the size in bytes of the image, but reduced to the lowest
	// multiple of 4. Same with dwOutLn.
	DWORD dwInLn = (3 * wWidth + 3) & ~3;
	DWORD dwOutLn = (3 * wNewWidth + 3) & ~3;

	int   x, y, i;
	BOOL  bCrossRow, bCrossCol;
	int   *pRowCoeff = CreateCoeffInt( wNewWidth, 
		                               wWidth, 
			                           FALSE);

	int   *pColCoeff = CreateCoeffInt( wNewHeight, 
		                               wHeight, 
			                           FALSE);

	int   *pXCoeff, *pYCoeff = pColCoeff;
	DWORD dwTmp, dwPtTmp[3];

	y = 0;
	while (y < wHeight)
	{
		bCrossRow = pYCoeff[1] > 0;
		x = wLeft;
		pXCoeff = pRowCoeff;
		pOutPix = pOutLine;
		pOutLine += dwOutLn;
		pUpPix = pLine;

		if (pYCoeff[1])
		{
			y++;
			pLine += dwInLn;	
			pPix	= pLine;
		}

		while(x < wWidth)
		{
			bCrossCol = pXCoeff[1] > 0;
			pUpPixOld = pUpPix;
			pPixOld = pPix;

			if(pXCoeff[1])
			{
				x++;
				pUpPix += 3;
				pPix += 3;
			}
   
			dwTmp = *pXCoeff * *pYCoeff;
   
			for(i = 0; i < 3; i++)
				dwPtTmp[i] = dwTmp * pUpPixOld[i];
   
			if(bCrossCol)
			{
				dwTmp = pXCoeff[1] * *pYCoeff;

				for(i = 0; i < 3; i++)
					dwPtTmp[i] += dwTmp * pUpPix[i];
			}

			if(bCrossRow)
			{
				dwTmp = *pXCoeff * pYCoeff[1];
				
				for(i = 0; i < 3; i++)
					dwPtTmp[i] += dwTmp * pPixOld[i];

				if(bCrossCol)
				{
					dwTmp = pXCoeff[1] * pYCoeff[1];
					
					for(i = 0; i < 3; i++)
						dwPtTmp[i] += dwTmp * pPix[i];
				}
			}
   
			for(i = 0; i < 3; i++, pOutPix++)
				*pOutPix = ((LPBYTE)(dwPtTmp + i))[3];
   
			pXCoeff += 2;
		} // while
		
		pYCoeff += 2;
	} // while (outer)

	delete [] pRowCoeff;
	delete [] pColCoeff;
} 

bool CBitmapScale::CropBitmap2(BYTE *pRGB24, BYTE* pRGB24Dest, int nWidth, int nHeight, const RECT & rcCropRect)
{
	int dstHeight = rcCropRect.bottom - rcCropRect.top;
	int dstWidth  = rcCropRect.right - rcCropRect.left;
	int nBitsPerPixel = 24;
	int nBytesPerPixel = nBitsPerPixel/8;
	int srcWidthBytes = nWidth*nBytesPerPixel;

	int y;
	for (y = rcCropRect.top; y < rcCropRect.bottom; y++)
	{
		BYTE * pSrcLine = &pRGB24[y * srcWidthBytes];
		BYTE * pDstLine = &pRGB24Dest[(y-rcCropRect.top) * srcWidthBytes];
		
		memcpy (pDstLine, pSrcLine + (rcCropRect.left*nBytesPerPixel), (rcCropRect.right-rcCropRect.left)*nBytesPerPixel);
	}

	return true;
}

bool CBitmapScale::CropBitmap(BYTE *pRGB24, BYTE* pRGB24Dest, int nWidth, int nHeight, const RECT & rcCropRect)
{
	// With rgb24, each pixel is 3 bytes

	// Get the length of each source horizontal line in bytes
	int nSrcLineLength = nWidth * 3;

	// get the length of the target horizontal lines in bytes
	int nDstLineLength = (rcCropRect.right - rcCropRect.left) * 3;

	// get the offset at which we start in the source buffer
	long nDstLineStartInSrc = (nSrcLineLength * rcCropRect.top) + (rcCropRect.left * 3);
	
	// point pSrcLinePtr at the location we are to start copying from
	BYTE* pSrcLinePtr = &pRGB24[nDstLineStartInSrc];

	// point pDstLinePtr to the beginning of the target buffer
	BYTE* pDstLinePtr = pRGB24Dest;

	// get the end of the destination buffer
	long nDstHeight = rcCropRect.bottom - rcCropRect.top;
	BYTE* pEndPtr = pDstLinePtr + (nDstLineLength * nDstHeight);

	int nLineCount = 0;
	// check that pDstLinePtr is less than pEndPtr
	while (pDstLinePtr < pEndPtr)
	{
		// copy a single line into the pDstLinPtr from the source
		memcpy(pDstLinePtr, &pSrcLinePtr[nDstLineStartInSrc], nDstLineLength);

		// increment pDstLinePtr by the byte value of a single dst line
		pDstLinePtr+= nDstLineLength;
		// we move pSrcLinePtr by the length of the entire source line. Remember
		// that pSrcLinePtr is not pointing at the start of the line, so when we
		// move it by this value, it is pointing again at the same location, but
		// on the next line
		pSrcLinePtr+= nSrcLineLength;

		_ASSERT(pDstLinePtr <= pEndPtr);

	//	if (nLineCount == 240)
	//		break;

		nLineCount++;
	}

	return true;
}
