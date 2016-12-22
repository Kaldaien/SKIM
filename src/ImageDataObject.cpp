// ImageDataObject.h: Impementation for IDataObject Interface to be used
//                    in inserting bitmap to the RichEdit Control.
//
// Author : Hani Atassi  (atassi@arabteam2000.com)
//
// How to use : Just call the static member InsertBitmap with
//              the appropriate parrameters.
//
// Known bugs :
//
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <comdef.h>
#include <atlbase.h>

#include <Richedit.h>

#include <ole2.h>
#include <RichOle.h>

#include "ImageDataObject.h"

//////////////////////////////////////////////////////////////////////
// Static member functions
//////////////////////////////////////////////////////////////////////

HRESULT
CImageDataObject::InsertBitmap ( IRichEditOle* pRichEditOle,
                                 HBITMAP       hBitmap )
{
  SCODE sc;

  // Get the image data object
  //
  CImageDataObject* pods =
    new CImageDataObject ();

  CComPtr <IDataObject> lpDataObject = nullptr;

  pods->QueryInterface ( IID_PPV_ARGS (&lpDataObject) );
  pods->SetBitmap      (            hBitmap           );

  // Get the RichEdit container site
  //
  CComPtr <IOleClientSite>      pOleClientSite = nullptr;
  pRichEditOle->GetClientSite (&pOleClientSite);

  // Initialize a Storage Object
  //
  CComPtr <IStorage>   pStorage   = nullptr;
  CComPtr <ILockBytes> pLockBytes = nullptr;

  sc = CreateILockBytesOnHGlobal ( nullptr,
                                     FALSE,
                                       &pLockBytes );
  return sc;

  sc =
    StgCreateDocfileOnILockBytes ( pLockBytes,
                                     STGM_SHARE_EXCLUSIVE | STGM_CREATE |
                                     STGM_READWRITE,
                                       0,
                                         &pStorage );
  if (sc != S_OK)
    return sc;

  //assert (pStorage != NULL);

  // The final ole object which will be inserted in the richedit control
  //
  CComPtr <IOleObject> pOleObject =
    pods->GetOleObject (pOleClientSite, pStorage);

  // all items are "contained" -- this makes our reference to this object
  //  weak -- which is needed for links to embedding silent update.
  OleSetContainedObject (pOleObject, TRUE);

  // Now Add the object to the RichEdit
  //
  REOBJECT     reobject;
  ZeroMemory (&reobject,           sizeof REOBJECT);
               reobject.cbStruct = sizeof REOBJECT;

  CLSID clsid;

  sc =
    pOleObject->GetUserClassID (&clsid);

  if (sc != S_OK)
    return sc;

  reobject.clsid    = clsid;
  reobject.cp       = REO_CP_SELECTION;
  reobject.dvaspect = DVASPECT_CONTENT;
  reobject.poleobj  = pOleObject;
  reobject.polesite = pOleClientSite;
  reobject.pstg     = pStorage;

  // Insert the bitmap at the current location in the richedit control
  //
  pRichEditOle->InsertObject (&reobject);

  return sc;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void
CImageDataObject::SetBitmap (HBITMAP hBitmap)
{
  //assert (hBitmap != NULL);

  STGMEDIUM stgm;
  stgm.tymed          = TYMED_GDI;        // Storage medium = HBITMAP handle
  stgm.hBitmap        = hBitmap;
  stgm.pUnkForRelease = NULL;             // Use ReleaseStgMedium

  FORMATETC fm;
  fm.cfFormat         = CF_BITMAP;        // Clipboard format = CF_BITMAP
  fm.ptd              = NULL;             // Target Device = Screen
  fm.dwAspect         = DVASPECT_CONTENT; // Level of detail = Full content
  fm.lindex           = -1;               // Index = Not applicaple
  fm.tymed            = TYMED_GDI;        // Storage medium = HBITMAP handle

  this->SetData (&fm, &stgm, TRUE);
}

IOleObject*
CImageDataObject::GetOleObject ( IOleClientSite* pOleClientSite,
                                 IStorage*       pStorage )
{
  //assert (m_stgmed.hBitmap);

  SCODE       sc;
  IOleObject* pOleObject;

  sc =
    OleCreateStaticFromData ( this,
                                IID_IOleObject, OLERENDER_FORMAT,
                                  &m_fromat,
                                    pOleClientSite,
                                      pStorage,
                                        (void **)&pOleObject );
  if (sc != S_OK)
    throw _com_error (sc);

  return pOleObject;
}