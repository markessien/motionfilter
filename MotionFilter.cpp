#include <windows.h>
#include <streams.h>
#include <initguid.h>
#if (1100 > _MSC_VER)
#include <olectlid.h>
#else
#include <olectl.h>
#endif
#include "MotionFilteruids.h"
#include "iMotionFilter.h"
#include "motionfilterprop.h"
#include "MotionFilter.h"
#include "resource.h"


// Setup information

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      1,                    // Number of types
      &sudPinTypes          // Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      1,                    // Number of types
      &sudPinTypes          // Pin information
    }
};

const AMOVIESETUP_FILTER sudMotionFilter =
{
    &CLSID_MotionFilter,         // Filter CLSID
    L"Erd Motion Filter",       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number of pins
    sudpPins                // Pin information
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

CFactoryTemplate g_Templates[] = {
    { L"Erd Motion Filter"
    , &CLSID_MotionFilter
    , CMotionFilter::CreateInstance
    , NULL
    , &sudMotionFilter }
  ,
    { L"Erd Motion Filter"
    , &CLSID_MotionFilterPropertyPage
    , CMotionFilterProperties::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Handles sample registry and unregistry
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


//
// Constructor
//
CMotionFilter::CMotionFilter(TCHAR *tszName,
                   LPUNKNOWN punk,
                   HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_MotionFilter),
    m_effect(IDC_RED),
    m_lBufferRequest(1),
    CPersistStream(punk, phr)
{
    char sz[60];
    GetProfileStringA("Quartz", "EffectStart", "2.0", sz, 60);
    m_effectStartTime = COARefTime(atof(sz));
    GetProfileStringA("Quartz", "EffectLength", "5.0", sz, 60);
    m_effectTime = COARefTime(atof(sz));

//	m_pBuffer = new BYTE[1024*1024];
	//memset(m_pBuffer, 0, 1024*1024);
	m_pBuffer = NULL;
	m_lBufferLen = 0;
} // (Constructor)


//
// CreateInstance
//
// Provide the way for COM to create a MotionFilter object
//
CUnknown *CMotionFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CMotionFilter *pNewObject = new CMotionFilter(NAME("Erd Motion Filter"), punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Reveals IIPEffect and ISpecifyPropertyPages
//
STDMETHODIMP CMotionFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IIPEffect) {
        return GetInterface((IIPEffect *) this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//
// Transform
//
// Copy the input sample into the output sample - then transform the output
// sample 'in place'. If we have all keyframes, then we shouldn't do a copy
// If we have cinepak or indeo and are decompressing frame N it needs frame
// decompressed frame N-1 available to calculate it, unless we are at a
// keyframe. So with keyframed codecs, you can't get away with applying the
// transform to change the frames in place, because you'll mess up the next
// frames decompression. The runtime MPEG decoder does not have keyframes in
// the same way so it can be done in place. We know if a sample is key frame
// as we transform because the sync point property will be set on the sample
//
HRESULT CMotionFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    // Copy the properties across

    HRESULT hr = Copy(pIn, pOut);
    if (FAILED(hr)) {
        return hr;
    }

//	return Transform(pOut);

	/*
    // Check to see if it is time to do the sample

 //   CRefTime tStart, tStop ;
   // pIn->GetTime((REFERENCE_TIME *) &tStart, (REFERENCE_TIME *) &tStop);

    //if (tStart >= m_effectStartTime) {
      //  if (tStop <= (m_effectStartTime + m_effectTime)) {
            return Transform(pOut);
        //}
    //}
	*/


	AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;

    BYTE *pDataOut;                // Pointer to the actual image buffer
    long lDataLenOut;              // Holds length of any given sample
	BYTE *pDataIn;                // Pointer to the actual image buffer
    long lDataLenIn;              // Holds length of any given sample

    pOut->GetPointer(&pDataOut);
    lDataLenOut = pOut->GetSize();
	memset(pDataOut, 1, lDataLenOut);

	pIn->GetPointer(&pDataIn);
    lDataLenIn = pIn->GetSize();
//	memset(pDataIn, 128, lDataLenIn);

    // Get the image properties from the BITMAPINFOHEADER

    int cxImage    = pvi->bmiHeader.biWidth;
    int cyImage    = pvi->bmiHeader.biHeight;
    int numPixels  = cxImage * cyImage;

	if (cyImage < 0)
		cyImage = -cyImage;

	m_rcCrop.left = 0;
	m_rcCrop.top = 0;
	m_rcCrop.right = 100;
	m_rcCrop.bottom = 100;

	m_bitmapScale.CropBitmap(pDataIn, m_pBuffer, cxImage, cyImage, m_rcCrop);


//	memcpy(pDataOut, m_pBuffer, lDataLenOut);

//	memcpy(m_pBuffer, pDataIn, lDataLenIn);
	

	int cxOut = cxImage;
	int cyOut = cyImage;
	ASSERT(pvi->bmiHeader.biBitCount == 24);
	m_bitmapScale.EnlargeDataInt(m_pBuffer, 0, 0, 
								 m_rcCrop.right - m_rcCrop.left, 
								 m_rcCrop.bottom - m_rcCrop.top,
								 pDataOut, cxImage, cyImage);

	/*
	WriteBitmapToFile("c:\\test.bmp", pDataOut, 
				  pvi->bmiHeader.biPlanes, 
				  pvi->bmiHeader.biBitCount,
				  cxImage,
				  cyImage);
*/
//	memcpy(pDataOut, m_pBuffer, lDataLenOut);

    return NOERROR;

} // Transform


//
// Copy
//
// Make destination an identical copy of source
//
HRESULT CMotionFilter::Copy(IMediaSample *pSource, IMediaSample *pDest) const
{
    // Copy the sample data

    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize = pSource->GetActualDataLength();

#ifdef DEBUG
    long lDestSize	= pDest->GetSize();
    ASSERT(lDestSize >= lSourceSize);
#endif

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

	// We don't need to copy
 //   CopyMemory( (PVOID) pDestBuffer,(PVOID) pSourceBuffer,lSourceSize);

    // Copy the sample times

    REFERENCE_TIME TimeStart, TimeEnd;
    if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd)) {
        pDest->SetTime(&TimeStart, &TimeEnd);
    }

    LONGLONG MediaStart, MediaEnd;
    if (pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
        pDest->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK) {
        pDest->SetSyncPoint(TRUE);
    }
    else if (hr == S_FALSE) {
        pDest->SetSyncPoint(FALSE);
    }
    else {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the media type

	
    AM_MEDIA_TYPE *pMediaType;
    hr = pSource->GetMediaType(&pMediaType);
//	ASSERT(pMediaType->cbFormat);

//	VIDEOINFOHEADER* pv = (VIDEOINFOHEADER*)pMediaType->pbFormat;
	//pv->bmiHeader.biWidth
//	pv->rcSource = m_rcCrop;

    pDest->SetMediaType(pMediaType);
    DeleteMediaType(pMediaType);

    // Copy the preroll property

    hr = pSource->IsPreroll();
    if (hr == S_OK) {
        pDest->SetPreroll(TRUE);
    }
    else if (hr == S_FALSE) {
        pDest->SetPreroll(FALSE);
    }
    else {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the discontinuity property

    hr = pSource->IsDiscontinuity();
    if (hr == S_OK) {
	pDest->SetDiscontinuity(TRUE);
    }
    else if (hr == S_FALSE) {
        pDest->SetDiscontinuity(FALSE);
    }
    else {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the actual data length

    long lDataLength = pSource->GetActualDataLength();
    pDest->SetActualDataLength(lDataLength);

    return NOERROR;

} // Copy


//
// Transform (in place)
//
// 'In place' apply the image effect to this sample
//
HRESULT CMotionFilter::Transform(IMediaSample *pMediaSample)
{
    AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;

    BYTE *pData;                // Pointer to the actual image buffer
    long lDataLen;              // Holds length of any given sample
    unsigned int grey,grey2;    // Used when applying greying effects
    int iPixel;                 // Used to loop through the image pixels
    int temp,x,y;               // General loop counters for transforms
    RGBTRIPLE *prgb;            // Holds a pointer to the current pixel

    pMediaSample->GetPointer(&pData);
    lDataLen = pMediaSample->GetSize();

    // Get the image properties from the BITMAPINFOHEADER

    int cxImage    = pvi->bmiHeader.biWidth;
    int cyImage    = pvi->bmiHeader.biHeight;
    int numPixels  = cxImage * cyImage;

    // int iPixelSize = pvi->bmiHeader.biBitCount / 8;
    // int cbImage    = cyImage * cxImage * iPixelSize;


    switch (m_effect)
    {
        case IDC_NONE: break;

        // Zero out the green and blue components to leave only the red
        // so acting as a filter - for better visual results, compute a
        // greyscale value for the pixel and make that the red component

        case IDC_RED:
						
            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels; iPixel++, prgb++) {
	        prgb->rgbtGreen = 0;
                prgb->rgbtBlue = 0;
            }
            break;
	
        case IDC_GREEN:

            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels; iPixel++, prgb++) {
                prgb->rgbtRed = 0;
                prgb->rgbtBlue = 0;
            }
            break;

        case IDC_BLUE:
            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels; iPixel++, prgb++) {
                prgb->rgbtRed = 0;
                prgb->rgbtGreen = 0;
            }
            break;

        // Bitwise shift each component to the right by 1
        // this results in the image getting much darker

	case IDC_DARKEN:
						
            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels; iPixel++, prgb++) {
                prgb->rgbtRed   = (BYTE) (prgb->rgbtRed >> 1);
                prgb->rgbtGreen = (BYTE) (prgb->rgbtGreen >> 1);
                prgb->rgbtBlue  = (BYTE) (prgb->rgbtBlue >> 1);
            }
            break;

        // Toggle each bit - this gives a sort of X-ray effect

        case IDC_XOR: 	
            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels; iPixel++, prgb++) {
                prgb->rgbtRed   = (BYTE) (prgb->rgbtRed ^ 0xff);
                prgb->rgbtGreen = (BYTE) (prgb->rgbtGreen ^ 0xff);
                prgb->rgbtBlue  = (BYTE) (prgb->rgbtBlue ^ 0xff);
            }
            break;

        // Zero out the five LSB per each component

        case IDC_POSTERIZE:
            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels; iPixel++, prgb++) {
                prgb->rgbtRed   = (BYTE) (prgb->rgbtRed & 0xe0);
                prgb->rgbtGreen = (BYTE) (prgb->rgbtGreen & 0xe0);
                prgb->rgbtBlue  = (BYTE) (prgb->rgbtBlue & 0xe0);
            }
            break;

        // Take pixel and its neighbor two pixels to the right and average
        // then out - this blurs them and produces a subtle motion effect

        case IDC_BLUR:
            prgb = (RGBTRIPLE*) pData;
            for (y = 0 ; y < pvi->bmiHeader.biHeight; y++) {
                for (x = 2 ; x < pvi->bmiHeader.biWidth; x++,prgb++) {
                    prgb->rgbtRed   = (BYTE) ((prgb->rgbtRed + prgb[2].rgbtRed) >> 1);
                    prgb->rgbtGreen = (BYTE) ((prgb->rgbtGreen + prgb[2].rgbtGreen) >> 1);
                    prgb->rgbtBlue  = (BYTE) ((prgb->rgbtBlue + prgb[2].rgbtBlue) >> 1);
		}
                prgb +=2;
            }
            break;

        // An excellent greyscale calculation is:
        //      grey = (30 * red + 59 * green + 11 * blue) / 100
        // This is a bit too slow so a faster calculation is:
        //      grey = (red + green) / 2

        case IDC_GREY: 	
            prgb = (RGBTRIPLE*) pData;
            for (iPixel=0; iPixel < numPixels ; iPixel++, prgb++) {
                grey = (prgb->rgbtRed + prgb->rgbtGreen) >> 1;
	        prgb->rgbtRed = prgb->rgbtGreen = prgb->rgbtBlue = (BYTE) grey;
            }
            break;

        // Really sleazy emboss - rather than using a nice 3x3 convulution
        // matrix, we compare the greyscale values of two neighbours. If
        // they are not different, then a mid grey (128, 128, 128) is
        // supplied.  Large differences get father away from the mid grey

        case IDC_EMBOSS:
            prgb = (RGBTRIPLE*) pData;
            for (y = 0 ; y < pvi->bmiHeader.biHeight; y++) {
                grey2 = (prgb->rgbtRed + prgb->rgbtGreen) >> 1;
                prgb->rgbtRed = prgb->rgbtGreen = prgb->rgbtBlue = (BYTE) 128;
                prgb++;
		for (x = 1 ; x < pvi->bmiHeader.biWidth; x++) {
                    grey = (prgb->rgbtRed + prgb->rgbtGreen) >> 1;
                    temp = grey - grey2;
                    if (temp > 127) temp = 127;
                    if (temp < -127) temp = -127;
                    temp += 128;
                    prgb->rgbtRed = prgb->rgbtGreen = prgb->rgbtBlue = (BYTE) temp;
                    grey2 = grey;
                    prgb++;
	        }
            }	
            break;
    }

    return NOERROR;

} // Transform (in place)


// Check the input type is OK - return an error otherwise

HRESULT CMotionFilter::CheckInputType(const CMediaType *mtIn)
{
    // check this is a VIDEOINFOHEADER type

    if (*mtIn->FormatType() != FORMAT_VideoInfo) {
        return E_INVALIDARG;
    }

    // Can we transform this type

    if (CanPerformMotion(mtIn)) {
    	return NOERROR;
    }
    return E_FAIL;
}


//
// Checktransform
//
// Check a transform can be done between these formats
//
HRESULT CMotionFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    if (CanPerformMotion(mtIn)) {
        if (*mtIn == *mtOut) {
            return NOERROR;
        }
    }
    return E_FAIL;

} // CheckTransform


//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CMotionFilter::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();
    ASSERT(pProperties->cbBuffer);

	if (m_pBuffer == NULL || m_lBufferLen != pProperties->cbBuffer)
	{
		if (m_pBuffer)
			delete [] m_pBuffer;

		m_pBuffer = new BYTE[pProperties->cbBuffer];
		memset(m_pBuffer, 0, pProperties->cbBuffer);
	}

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    ASSERT( Actual.cBuffers == 1 );

    if (pProperties->cBuffers > Actual.cBuffers ||
            pProperties->cbBuffer > Actual.cbBuffer) {
                return E_FAIL;
    }
    return NOERROR;

} // DecideBufferSize


//
// GetMediaType
//
// I support one type, namely the type of the input pin
// This type is only available if my input is connected
//
HRESULT CMotionFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pInput->CurrentMediaType();
    return NOERROR;

} // GetMediaType


//
// CanPerformMotionFilter
//
// Check if this is a RGB24 true colour format
//
BOOL CMotionFilter::CanPerformMotion(const CMediaType *pMediaType) const
{
    if (IsEqualGUID(*pMediaType->Type(), MEDIATYPE_Video)) {
        if (IsEqualGUID(*pMediaType->Subtype(), MEDIASUBTYPE_RGB24)) {
            VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();
            return (pvi->bmiHeader.biBitCount == 24);
        }
    }
    return FALSE;

} // CanPerformMotionFilter


#define WRITEOUT(var)  hr = pStream->Write(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;

#define READIN(var)    hr = pStream->Read(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;


//
// GetClassID
//
// This is the only method of IPersist
//
STDMETHODIMP CMotionFilter::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);
} // GetClassID


//
// ScribbleToStream
//
// Overriden to write our state into a stream
//
HRESULT CMotionFilter::ScribbleToStream(IStream *pStream)
{
    HRESULT hr;
    WRITEOUT(m_effect);
    WRITEOUT(m_effectStartTime);
    WRITEOUT(m_effectTime);
    return NOERROR;

} // ScribbleToStream


//
// ReadFromStream
//
// Likewise overriden to restore our state from a stream
//
HRESULT CMotionFilter::ReadFromStream(IStream *pStream)
{
    HRESULT hr;
    READIN(m_effect);
    READIN(m_effectStartTime);
    READIN(m_effectTime);
    return NOERROR;

} // ReadFromStream


//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CMotionFilter::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_MotionFilterPropertyPage;
    return NOERROR;

} // GetPages


//
// get_IPEffect
//
// Return the current effect selected
//
STDMETHODIMP CMotionFilter::get_IPEffect(int *IPEffect,REFTIME *start,REFTIME *length)
{
    CAutoLock cAutolock(&m_MotionFilterLock);
    CheckPointer(IPEffect,E_POINTER);
    CheckPointer(start,E_POINTER);
    CheckPointer(length,E_POINTER);

    *IPEffect = m_effect;
    *start = COARefTime(m_effectStartTime);
    *length = COARefTime(m_effectTime);

    return NOERROR;

} // get_IPEffect


//
// put_IPEffect
//
// Set the required video effect
//
STDMETHODIMP CMotionFilter::put_IPEffect(int IPEffect,REFTIME start,REFTIME length)
{
    CAutoLock cAutolock(&m_MotionFilterLock);

    m_effect = IPEffect;
    m_effectStartTime = COARefTime(start);
    m_effectTime = COARefTime(length);

    SetDirty(TRUE);
    return NOERROR;

} // put_IPEffect


HRESULT CMotionFilter::InPlaceStretch(IMediaSample *pMediaSample, RECT rcSource)
{
	// This function simply reads the square rcSource, and stretches it to the size
	// of the image.
	
	AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;

    BYTE *pData;                // Pointer to the actual image buffer
    long lDataLen;              // Holds length of any given sampl
    
	pMediaSample->GetPointer(&pData);
    lDataLen = pMediaSample->GetSize();

	/*
	int cxSource = 24;
	int cySource = 24;

    // Get the image properties from the BITMAPINFOHEADER

    int cxImage    = pvi->bmiHeader.biWidth;
    int cyImage    = pvi->bmiHeader.biHeight;
    int numPixels  = cxImage * cyImage;

	double dXSrc = 0;

	for (int nX = 0; nX < cxImage; nX++)
	{
		double dYSrc = 0;

		for (int nY = 0; nY < cyImage; nY++)
		{
			pData[nY * cxImage + nX] = CalcWeightedColor(pData, lDataLen, cxSource, cySource);
		}
	}
*/
	 return NOERROR;
}

void CMotionFilter::WriteBitmapToFile(char* pstrFile, BYTE *pRgbBuffer, long nPlanes, long nBitsPerPixel, long nWidth, long nHeight)
{
	long nSize = (nWidth * (nBitsPerPixel / 8)) * nHeight;
	BYTE* pBuffer = new BYTE[nSize + sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER )];

	// write out the file header
	BITMAPFILEHEADER bfh;
	memset( &bfh, 0, sizeof(bfh) );
	bfh.bfType = 'MB';
	bfh.bfSize = nSize + sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );
	bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );
	
	memcpy(pBuffer, &bfh, sizeof(bfh));

	// and the bitmap format
	//
	BITMAPINFOHEADER bih;
	memset( &bih, 0, sizeof( bih ) );
	bih.biSize = sizeof( bih );
	bih.biWidth = nWidth;
	bih.biHeight = nHeight;
	bih.biPlanes = nPlanes;
	bih.biBitCount = nBitsPerPixel;
	
	memcpy(&pBuffer[sizeof(bfh)], &bih, sizeof( bih ));
	memcpy(&pBuffer[sizeof(bfh) + sizeof(bih)], pRgbBuffer, nSize);

	HANDLE hFile = ::CreateFile(pstrFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (hFile  != INVALID_HANDLE_VALUE)
	{
		DWORD dwWritten = 0;
		WriteFile(hFile, pBuffer, nSize + sizeof(bfh) + sizeof(bih), &dwWritten, 0);
		ASSERT(dwWritten);

		CloseHandle(hFile);
		delete [] pBuffer;

		return;
	}

	delete [] pBuffer;
}
