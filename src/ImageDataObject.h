// ImageDataObject.h: Impementation for IDataObject Interface to be used
//               in inserting bitmap to the RichEdit Control.
//
// Author : Hani Atassi  (atassi@arabteam2000.com)
//
// How to use : Just call the static member InsertBitmap with
//        the appropriate parrameters.
//
// Known bugs :
//
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

class CImageDataObject : IDataObject
{
public:
  // This static fumction accepts those parameters:
  // IRichEditOle* : a pointer to IRochEditOle interface for the RichEdit Control
  // HBITMAP : the bitmap handle.
  //
  // After calling the function, it inserts the image in the current
  //    position of the RichEdit
  //
  static HRESULT
    InsertBitmap ( IRichEditOle* pRichEditOle,
                   HBITMAP       hBitmap );

private:
  ULONG     m_ulRefCnt;
  BOOL      m_bRelease;

  // The data being bassed to the richedit
  //
  STGMEDIUM m_stgmed;
  FORMATETC m_fromat;

public:
   CImageDataObject (void) : m_ulRefCnt (0)
  {
    m_bRelease = FALSE;
  }

  ~CImageDataObject (void)
  {
    if (m_bRelease)
      ReleaseStgMedium (&m_stgmed);
  }

  // Methods of the IUnknown interface
  //
  STDMETHOD(QueryInterface)( REFIID    iid,
                             void** ppvObject )
  {
    if (iid == IID_IUnknown || iid == IID_IDataObject)
    {
      *ppvObject = this;
      AddRef ();

      return S_OK;
    }

    else
      return E_NOINTERFACE;
  }
  STDMETHOD_(ULONG, AddRef)(void)
  {
    InterlockedIncrement (&m_ulRefCnt);

    return m_ulRefCnt;
  }
  STDMETHOD_(ULONG, Release)(void)
  {
    ULONG ulRefCount =
      InterlockedDecrement (&m_ulRefCnt);

    if (m_ulRefCnt == 0)
      delete this;

    return ulRefCount;
  }

  // Methods of the IDataObject Interface
  //
  STDMETHOD(GetData)( FORMATETC* pformatetcIn,
                      STGMEDIUM* pmedium )
  {
    HANDLE hDst =
      OleDuplicateData (m_stgmed.hBitmap, CF_BITMAP, 0x0);

    if (hDst == NULL)
      return E_HANDLE;

    pmedium->tymed          = TYMED_GDI;
    pmedium->hBitmap        = (HBITMAP)hDst;
    pmedium->pUnkForRelease = nullptr;

    return S_OK;
  }
  STDMETHOD(GetDataHere)( FORMATETC* pformatetc,
                          STGMEDIUM* pmedium ) {
    return E_NOTIMPL;
  }
  STDMETHOD(QueryGetData)(FORMATETC*  pformatetc ) {
    return E_NOTIMPL;
  }
  STDMETHOD(GetCanonicalFormatEtc)( FORMATETC* pformatectIn,
                                    FORMATETC* pformatetcOut ) {
    return E_NOTIMPL;
  }
  STDMETHOD(SetData)( FORMATETC* pformatetc,
                      STGMEDIUM* pmedium,
                      BOOL       fRelease )
  {
    m_fromat = *pformatetc;
    m_stgmed = *pmedium;

    return S_OK;
  }
  STDMETHOD(EnumFormatEtc)( DWORD            dwDirection,
                            IEnumFORMATETC** ppenumFormatEtc ) {
    return E_NOTIMPL;
  }
  STDMETHOD(DAdvise)( FORMATETC*   pformatetc,
                      DWORD         advf,
                      IAdviseSink* pAdvSink,
                      DWORD*       pdwConnection) {
    return E_NOTIMPL;
  }
  STDMETHOD(DUnadvise)(DWORD dwConnection) {
    return E_NOTIMPL;
  }
  STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppenumAdvise) {
    return E_NOTIMPL;
  }

  // Some Other helper functions
  //
  void        SetBitmap    ( HBITMAP         hBitmap  );
  IOleObject* GetOleObject ( IOleClientSite* pOleClientSite,
                             IStorage*       pStorage );

};