//------------------------------------------------------------------------------
// File: IEZ.h
//
// Desc: DirectShow sample code - custom interface to allow the user to
//       perform image special effects.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __IEZ__
#define __IEZ__

#ifdef __cplusplus
extern "C" {
#endif

	// {0D76AA8E-553F-4005-8174-7D284499CBAE}
	DEFINE_GUID(IID_IIPEffect, 
	0xd76aa8e, 0x553f, 0x4005, 0x81, 0x74, 0x7d, 0x28, 0x44, 0x99, 0xcb, 0xae);


    DECLARE_INTERFACE_(IIPEffect, IUnknown)
    {
        STDMETHOD(get_IPEffect) (THIS_
                    int *effectNum,	        // The current effect
                    REFTIME *StartTime,     // Start time of effect
                    REFTIME *Length         // length of effect
                 ) PURE;

        STDMETHOD(put_IPEffect) (THIS_
                    int effectNum,	        // Change to this effect
                    REFTIME StartTime,      // Start time of effect
                    REFTIME Length          // Length of effect
                 ) PURE;
    };

#ifdef __cplusplus
}
#endif

#endif // __IEZ__

