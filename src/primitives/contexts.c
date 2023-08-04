
#include "../include/csl.h"

void
CSL_Contex_New_RunWord ( )
{
    Word * word = ( Word * ) DataStack_Pop ( ) ;
    _CSL_Contex_NewRun_Void ( _CSL_, word ) ;
}

