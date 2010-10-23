// BitmapScale.h: interface for the CBitmapScale class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BITMAPSCALE_H__A3AE51AD_FBA5_4D14_A40B_A6D7AB460BB0__INCLUDED_)
#define AFX_BITMAPSCALE_H__A3AE51AD_FBA5_4D14_A40B_A6D7AB460BB0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CBitmapScale  
{
public:
	CBitmapScale();
	virtual ~CBitmapScale();

public:
	bool CropBitmap2(BYTE *pRGB24, BYTE* pRGB24Dest, int nWidth, int nHeight, const RECT & rcCropRect);
	bool CropBitmap(BYTE* pRGB24, BYTE* pRGB24Dest, int nWidth, int nHeight, const RECT & rcCropRect);
	HBITMAP ScaleBitmapInt(HBITMAP hBmp, WORD wNewWidth, WORD wNewHeight);

	BITMAPINFO *PrepareRGBBitmapInfo(WORD wWidth, WORD wHeight);
	int* CreateCoeffInt(int nLen, int nNewLen, BOOL bShrink);
	void ShrinkDataInt(BYTE *pInBuff, WORD wWidth, WORD wHeight, BYTE *pOutBuff, WORD wNewWidth, WORD wNewHeight);
	void EnlargeDataInt(BYTE *pInBuff, WORD wLeft, WORD wTop, WORD wWidth, WORD wHeight, BYTE *pOutBuff, WORD wNewWidth, WORD wNewHeight);

};

#endif // !defined(AFX_BITMAPSCALE_H__A3AE51AD_FBA5_4D14_A40B_A6D7AB460BB0__INCLUDED_)
