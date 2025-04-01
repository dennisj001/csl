#ifndef __CSL_H__
#define __CSL_H__

#include "os.h"
#include "ext.h"


#include "defines.h"
#include "types.h"
#include "macros.h"
#include "machineCode.h"
#include "machineCodeMacros.h"
#include "prototypes.h"
#include "lc.h"

extern OVT_MemSystem *_OMS_ ;
extern OVT_StaticMemSystem * _OSMS_ ;
extern OpenVmTil * _O_;
extern struct termios SavedTerminalAttributes ;
extern CPrimitive CPrimitives [];
extern MachineCodePrimitive MachineCodePrimitives [];

// ls9
extern Boolean cli, lf ;
extern int lic, csl_returnValue, lv ; // last value
extern FILE * f ;

// memspace.c
extern int64 mmaped ;

// simple shell : utils.c
extern char **environ ;
extern snode* jobs ;

#endif


