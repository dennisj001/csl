
#include "../include/csl.h"

void
CSL_IncludeFile ( )
{
   byte * filename = ( byte* ) DataStack_Pop ( ) ;
    CSL_ContextNew_IncludeFile (filename, 0) ;
}

int64
_File_Size ( FILE * file )
{
    fseek ( file, 0, SEEK_END ) ;
    int64 size = ftell ( file ) ;
    rewind ( file ) ;
    return size ;
}

int64
_File_Exists ( byte * fname )
{
    struct stat sbuf ;
    if ( ! stat ( ( char* ) fname, &sbuf ) )  return true ; // nb. this logic is the reverse of the system call
    else return false ;
}

byte *
_File_ReadNtoString_ ( FILE * file, int64 n )
{
    int64 size, result ;

    if ( ! n ) size = _File_Size ( file ) ;
    else size = n ;
    byte * fstr = Mem_Allocate ( size + 2, CONTEXT ) ; // 2 : an extra so readline doesn't read into another area of allocated mem
    result = fread ( fstr, 1, size, file ) ;
    fclose ( file ) ;
    if ( result != size ) return 0 ;
    return fstr ;
}

byte *
_File_ReadToString_ ( FILE * file )
{
    int64 size, result ;

    size = _File_Size ( file ) ;
    byte * fstr = Mem_Allocate ( size + 2, CONTEXT ) ; // 2 : an extra so readline doesn't read into another area of allocated mem
    result = fread ( fstr, 1, size, file ) ;
    fclose ( file ) ;
    if ( result != size ) return 0 ;
    return fstr ;
}

byte *
_File_ReadToString ( byte * name )
{
    FILE * file ;
    file = fopen ( ( char* ) name, "rb" ) ;
    return _File_ReadToString_ ( file ) ;
}

void
File_ReadToString ( )
{
    byte * filename = ( byte* ) DataStack_Pop ( ), *str ;
    str = _File_ReadToString ( filename ) ;
    DataStack_Push (( int64 ) str) ;
}

// ( filename n -- str )
void
File_ReadNtoString ( )
{
    int64 n = DataStack_Pop ( ) ;
    byte * filename = ( byte* ) DataStack_Pop ( ), *str ;
    FILE * file = fopen ( ( char* ) filename, "rb" ) ;
    str = _File_ReadNtoString_ ( file, n ) ;
    DataStack_Push (( int64 ) str) ;
}

void
_File_Open ( byte * filename, byte * mode )
{
    FILE * file = fopen ( ( char* ) filename, mode ) ;
    if ( file == NULL )
    {
        perror ( "\nFile_Open error : " ) ;
        CSL_Quit () ;
    }
    else DataStack_Push (( int64 ) file) ;
}

void
File_OpenModal ( )
{
    byte * filename = ( byte* ) DataStack_Pop ( ) ;
    byte * mode = ( byte* ) DataStack_Pop ( ) ;
    _File_Open ( filename, mode ) ;
}

void
File_Open ( )
{
    byte * filename = ( byte* ) DataStack_Pop ( ) ;
    _File_Open ( filename, "wb") ;
}

void
File_Close ( )
{
    FILE * file = ( FILE* ) DataStack_Pop ( ) ;
    fclose ( file ) ;
}

void
File_Read ( )
{
    int64 size = DataStack_Pop ( ) ;
    //int64 size = _File_Size ( file ) ;
    byte * ptr = ( byte * ) DataStack_Pop ( ) ;
    FILE * file = ( FILE * ) DataStack_Pop ( ) ;
    int64 result = fread ( ptr, size, 1, file ) ;
    DataStack_Push (result) ;
    if ( result != size )
    {
        if ( ! feof ( file ) )
        {
            if ( ferror ( file ) )
            {
                perror ( "\nFile_Read error : " ) ;
                CSL_Quit () ;
            }
        }
    }
}

void
File_Write ( )
{
    int64 size = DataStack_Pop ( ) ;
    byte * ptr = ( byte * ) DataStack_Pop ( ) ;
    FILE * file = ( FILE * ) DataStack_Pop ( ) ;
    fwrite ( ptr, size, 1, file ) ;
}

void
File_Seek ( )
{
    int64 offset = DataStack_Pop ( ) ;
    FILE * file = ( FILE * ) DataStack_Pop ( ) ;
    fseek ( file, offset, SEEK_SET ) ;
}

void
File_Tell ( )
{
    FILE * file = ( FILE * ) DataStack_Pop ( ) ;
    int64 result = ftell ( file ) ;
    DataStack_Push (result) ;
}

void
File_Size ( )
{
    FILE * file = ( FILE * ) DataStack_Pop ( ) ;
    DataStack_Push (_File_Size ( file )) ;
}

void
File_Exists ( )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    DataStack_Push (_File_Exists ( name )) ;
}

void
File_ChangeDirectory ()
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    chdir ( name ) ;
}

void
File_GetCurrentDirectory ()
{
    byte * b = Buffer_DataCleared ( _CSL_->StringInsertB7 ) ;
    getcwd ( b, BUFFER_SIZE) ;
    iPrintf ( " %s", b ) ;
}