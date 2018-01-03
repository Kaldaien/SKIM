#pragma once


#define SKIM_MAJOR 0
#define SKIM_MINOR 7
#define SKIM_BUILD 5
#define SKIM_REV   12





#define _A2(a)     #a
#define  _A(a)  _A2(a)
#define _L2(w)  L ## w
#define  _L(w) _L2(w)


#if (SKIM_REV > 0)
#define SKIM_VERSION_STR_A    _A(SKIM_MAJOR) "." _A(SKIM_MINOR) "." _A(SKIM_BUILD) "." _A(SKIM_REV)
#else
#define SKIM_VERSION_STR_A    _A(SKIM_MAJOR) "." _A(SKIM_MINOR) "." _A(SKIM_BUILD)
#endif

#define SKIM_VERSION_STR_W _L(SKIM_VERSION_STR_A)


#define SKIM_FILE_VERSION     SKIM_MAJOR,SKIM_MINOR,SKIM_BUILD,SKIM_REV
#define SKIM_PRODUCT_VERSION  SKIM_MAJOR,SKIM_MINOR,SKIM_BUILD,SKIM_REV
