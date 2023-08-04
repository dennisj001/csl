
#include "../include/csl.h"

void
MultipleEscape ( )
{
    _MultipleEscape ( _Context_->Lexer0 ) ;
}

void
CSL_Strlen ( )
{
    byte * o = (char*) DataStack_Pop ( ) ;
    int64 lo = (int64) Strlen ( o  ) ;
    DataStack_Push ( lo ) ;
}

void
CSL_Strcmp ( )
{
#if 0    
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
    if ( bi && bi->JccTtt )
    {
        bi->JccNegFlag = ! bi->JccNegFlag ;
    }
#endif    
    DataStack_Push ( (int64) Strcmp ( (byte*) DataStack_Pop ( ), (byte*) DataStack_Pop ( ) ) ) ;
}

void
CSL_Stricmp ( )
{
    DataStack_Push ( (int64) Stricmp ( (byte*) DataStack_Pop ( ), (byte*) DataStack_Pop ( ) ) ) ;
}

//char * strcat ( char * destination, const char * source );
void
CSL_StrCat ( )
{
    byte * buffer = Buffer_DataCleared ( _CSL_->StrCatBuffer );  byte *str ;
    char * src = (char*) DataStack_Pop ( ) ;
    char * dst = (char*) DataStack_Pop ( ) ;
    strncpy ( (char*) buffer, dst, BUFFER_IX_SIZE ) ;
    if (src) strncat ( (char *) buffer, src, BUFFER_IX_SIZE ) ; 
    str = String_New ( buffer, TEMPORARY ) ; //String_New ( (byte*) buffer, DICTIONARY ) ;
    DataStack_Push ((int64) str) ;
}

void
CSL_StrCpy ( )
{
    // !! nb. this cant really work !! what do we want here ??
    DataStack_Push ( (int64) strcpy ( (char*) DataStack_Pop ( ), (char*) DataStack_Pop ( ) ) ) ;
}

void
String_GetStringToEndOfLine ( )
{
    DataStack_Push ((int64) _String_Get_ReadlineString_ToEndOfLine ( )) ;
}
