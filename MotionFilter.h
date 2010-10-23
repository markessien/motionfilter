#include <windows.h>
#include "BitmapScale.h"

class CMotionFilter : public CTransformFilter,
		 public IIPEffect,
		 public ISpecifyPropertyPages,
		 public CPersistStream
{

public:
	void WriteBitmapToFile(char* pstrFile, BYTE *pRgbBuffer, long nPlanes, long nBitsPerPixel, long nWidth, long nHeight);
	HRESULT InPlaceStretch(IMediaSample* pMediaSample, RECT rcSource);

    DECLARE_IUNKNOWN;
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

  
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // CPersistStream stuff
    HRESULT ScribbleToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);

    // Overrriden from CTransformFilter base class

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
			     ALLOCATOR_PROPERTIES *pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // These implement the custom IIPEffect interface

    STDMETHODIMP get_IPEffect(int *IPEffect, REFTIME *StartTime, REFTIME *Length);
    STDMETHODIMP put_IPEffect(int IPEffect, REFTIME StartTime, REFTIME Length);

    // ISpecifyPropertyPages interface
    STDMETHODIMP GetPages(CAUUID *pPages);

    // CPersistStream override
    STDMETHODIMP GetClassID(CLSID *pClsid);

private:

    // Constructor
    CMotionFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);

    // Look after doing the special effect
    BOOL CanPerformMotion(const CMediaType *pMediaType) const;
    HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) const;
    HRESULT Transform(IMediaSample *pMediaSample);

	long m_lBufferLen;
	RECT m_rcCrop;
	BYTE * m_pBuffer;
	CBitmapScale m_bitmapScale;
    CCritSec	m_MotionFilterLock;          // Private play critical section
    int         m_effect;               // Which effect are we processing
    CRefTime	m_effectStartTime;      // When the effect will begin
    CRefTime	m_effectTime;           // And how long it will last for
    const long m_lBufferRequest;	// The number of buffers to use

};

