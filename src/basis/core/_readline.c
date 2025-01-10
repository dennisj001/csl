#include "../../include/csl.h"

void
Readline_ZeroEndPosToEnd ( ReadLiner * rl )
{
    int64 i ;
    for ( i = rl->EndPosition ; i < BUFFER_SIZE ; i ++ ) rl->InputLine[i] = 0 ;
}

void
ReadLine_Set_ReadIndex ( ReadLiner * rl, int64 pos )
{
    rl->ReadIndex = pos ;
}

byte *
_ReadLine_pb_NextChar ( ReadLiner * rl )
{
    if ( rl->ReadIndex < BUFFER_SIZE ) return &rl->InputLine [ rl->ReadIndex ] ;
    else return 0 ;
}

byte
_ReadLine_NextChar ( ReadLiner * rl )
{
    byte c = 0 ;
    if ( rl->ReadIndex < BUFFER_SIZE ) c = rl->InputLine [ rl->ReadIndex ] ;
    return c ;
}

inline byte
_ReadLine_PeekIndexedChar ( ReadLiner * rl, int64 offset )
{
    byte c = rl->InputLine [ offset ] ;
    return c ;
}

byte
ReadLine_PeekIndexedChar ( ReadLiner * rl, int64 index )
{
    byte c = 0 ;
    if ( index < BUFFER_SIZE ) c = _ReadLine_PeekIndexedChar ( rl, index ) ; //rl->InputLine [ offset ] ;
    return c ;
}

byte
_ReadLine_PeekOffsetChar ( ReadLiner * rl, int64 offset )
{
    byte c = 0 ;
    if ( ( rl->ReadIndex + offset ) < BUFFER_SIZE ) c = _ReadLine_PeekIndexedChar ( rl, rl->ReadIndex + offset ) ;
    return c ;
}

byte
ReadLine_PeekNextChar ( ReadLiner * rl )
{
    byte c = _ReadLine_PeekOffsetChar ( rl, 0 ) ; //_ReadLine_NextChar ( rl );
    return c ;
}

byte
_ReadLine_GetNextChar ( ReadLiner * rl )
{
    byte c = _ReadLine_NextChar ( rl ) ;
    if ( c ) rl->ReadIndex ++ ;
    return c ;
}

void
_ReadLine_EndThisLine ( ReadLiner * rl )
{
    ReadLine_Set_ReadIndex ( rl, - 1 ) ;
}

byte
ReadLine_CurrentReadChar ( ReadLiner * rl )
{
    return rl->InputLine [ rl->ReadIndex ] ;
}

byte *
ReadLine_BytePointerToCurrentReadChar ( ReadLiner * rl )
{
    return &rl->InputLine [ rl->ReadIndex ] ;
}

byte
ReadLine_LastChar ( ReadLiner * rl )
{
    return rl->InputLine [ rl->ReadIndex - 1 ] ;
}

byte
ReadLine_LastReadChar ( ReadLiner * rl )
{
    return rl->InputLine [ rl->ReadIndex - 2 ] ;
}

byte
ReadLine_PeekNextNonWhitespaceChar ( ReadLiner * rl )
{
    int64 index = rl->ReadIndex ;
    byte atIndex = 0 ;
    do
    {
        if ( index >= BUFFER_SIZE ) break ;
        atIndex = rl->InputLine [ index ++ ] ;
    }
    while ( atIndex <= ' ' ) ;
    return atIndex ;
}

int64
ReadLine_IsThereNextChar ( ReadLiner * rl )
{
    if ( ! rl->InputLine ) return false ; // in case we are at in a _OpenVmTil_Pause
    char c = ReadLine_PeekNextChar ( rl ) ;
    return c || ( rl->InputFile && ( rl->InputKeyedCharacter != eof ) ) ; // || (c != '\n') ) ;
}

void
ReadLine_UnGetChar ( ReadLiner * rl )
{
    if ( rl->ReadIndex ) rl->ReadIndex -- ;
}

void
ReadLine_PushChar ( ReadLiner * rl, byte c )
{
    rl->InputLine [ rl->ReadIndex ] = c ;
}

void
_ReadLine_ShowCharacter ( ReadLiner * rl, byte chr )
{
    if ( GetState ( rl, CHAR_ECHO ) ) putc ( ( char ) chr, rl->OutputFile ) ;
}

void
ReadLine_ShowCharacter ( ReadLiner * rl )
{
    _ReadLine_ShowCharacter ( rl, rl->InputKeyedCharacter ) ;
}

void
_ReadLine_SetMaxEndPosition ( ReadLiner * rl )
{
    if ( rl->EndPosition > rl->MaxEndPosition ) rl->MaxEndPosition = rl->EndPosition ;
}

void
_ReadLine_SetEndPosition ( ReadLiner * rl )
{
    rl->EndPosition = Strlen ( ( char* ) rl->InputLine ) ;
    _ReadLine_SetOutputLineCharacterNumber ( rl ) ;
    _ReadLine_SetMaxEndPosition ( rl ) ; // especially for the case of show history node
}

byte
_ReadLine_CharAtCursor ( ReadLiner * rl )
{
    return rl->InputLine [ rl->CursorPosition ] ;
}

byte
_ReadLine_CharAtACursorPos ( ReadLiner * rl, int64 pos )
{
    return rl->InputLine [ pos ] ;
}

void
_ReadLine_CursorToEnd ( ReadLiner * rl )
{
    ReadLine_SetCursorPosition ( rl, rl->EndPosition ) ;
    //rl->InputLine [ rl->CursorPosition ] = 0 ;
    ReadLine_SetCharAtCursorPos ( rl, 0 ) ;
}

void
_ReadLine_CursorToStart ( ReadLiner * rl )
{
    ReadLine_SetCursorPosition ( rl, 0 ) ;
}

void
_ReadLine_CursorRight ( ReadLiner * rl )
{
    ReadLine_SetCursorPosition ( rl, rl->CursorPosition + 1 ) ;
}

void
_ReadLine_CursorLeft ( ReadLiner * rl )
{
    ReadLine_SetCursorPosition ( rl, rl->CursorPosition - 1 ) ;
}

int64
Switch_LookAroundFor_Logic ( Rllafl * r, int64 inc )
{
    int64 i, rtrn = 0, lparenMark = 0, rparenMark = 0, id = 0 ;
    byte * nc1 ;
    for ( i = 0 ; 1 ; i ++ )
    {
        nc1 = ( r->nc + ( inc * i ) ) ;
        if ( ( nc1 <= r->Ncll ) || ( nc1 >= r->Ncul ) ) break ;
        if ( ( inc == 1 ) && ( rtrn & ( LT_AND_NEXT | LT_OR_NEXT ) ) ) break ;
        else if ( ( inc == - 1 ) && ( rtrn & ( LT_AND_PREVIOUS | LT_OR_PREVIOUS ) ) ) break ;
        switch ( * nc1 )
        {
            case '&':
            {
                lparenMark = 0, rparenMark = 0 ;
                if ( ( *( nc1 += inc ) == '&' ) )
                {
                    if ( ( *r->nc == '&' ) && ( ( *( r->nc - 1 ) == '&' ) || ( *( r->nc + 1 ) == '&' ) ) ) rtrn |= LT_AND ;
                    else if ( ( inc == 1 ) && ( ! ( rtrn & LT_2_LPAREN_NEXT ) ) )
                        rtrn |= LT_AND_NEXT ;
                    else if ( inc == - 1 ) rtrn |= LT_AND_PREVIOUS ;
                }
                else break ;
            }
            case '|':
            {
                lparenMark = 0, rparenMark = 0 ;
                if ( ( *( nc1 += inc ) == '|' ) )
                {
                    if ( ( *r->nc == '|' ) && ( ( *( r->nc - 1 ) == '|' ) || ( *( r->nc + 1 ) == '|' ) ) ) rtrn |= LT_OR ;
                    else if ( ( inc == 1 ) && ( ! ( rtrn & LT_2_LPAREN_NEXT ) ) ) rtrn |= LT_OR_NEXT ;
                    else if ( inc == - 1 ) rtrn |= LT_OR_PREVIOUS ;
                }
                else break ;
            }
            case '{':
            {
                if ( C_SyntaxOn ) break ;
                //else fallthru ;
            }
            case '(':
            {
                rparenMark = 0 ;
                if ( inc == 1 ) r->pparenlvl ++ ;
                else
                {
                    //sob:
                    if ( -- r->mparenlvl < r->oparenlvl )
                    {
                        rtrn |= ( LT_START_OF_BLOCK ) ;
                        goto done ;
                    }
                }
                if ( ( rtrn & LT_LPAREN ) && ( lparenMark == 1 ) )
                {
                    rtrn = rtrn & ( ~ LT_LPAREN ) ; // for the printer, just set LT_2_LPAREN_x
                    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
                    bi->State |= BI_2_LPAREN ;
                    if ( inc == 1 )
                        rtrn |= LT_2_LPAREN_NEXT ;
                    else rtrn |= LT_2_LPAREN_PREVIOUS ;
                    if ( rtrn & ( LT_AND_NEXT | LT_AND_PREVIOUS | LT_OR_NEXT | LT_OR_PREVIOUS ) ) goto done ;
                    break ;
                }
                rtrn |= LT_LPAREN ;
                lparenMark = 1 ;
                break ;
            }
            case '}':
            {
                if ( C_SyntaxOn ) break ;
                //else fallthru ;
            }
            case ')':
            {
                lparenMark = 0 ;
                //eob:
                if ( inc == - 1 ) r->mparenlvl ++ ;
                else if ( -- r->pparenlvl < r->oparenlvl )
                {
                    //if ( ! id ) 
                    rtrn |= ( LT_END_OF_BLOCK ) ;
                    r->ncp = nc1 ;
                    goto done ;
                }
                if ( ( rtrn & LT_RPAREN ) && ( rparenMark == 1 ) )
                {
                    rtrn = rtrn & ( ~ LT_RPAREN ) ; // just set LT_2_RPAREN_x
                    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
                    bi->State |= BI_2_RPAREN ;
                    if ( inc == 1 )
                        rtrn |= LT_2_RPAREN_NEXT ;
                    else rtrn |= LT_2_RPAREN_PREVIOUS ;
                    if ( rtrn & ( LT_AND_NEXT | LT_AND_PREVIOUS | LT_OR_NEXT | LT_OR_PREVIOUS ) ) goto done ;
                    break ;
                }
                rtrn |= LT_RPAREN ;
                rparenMark = 1 ;
                break ;
            }
            case '=':
            {
                if ( ( *( nc1 += inc ) ) == '=' )
                {
                    i ++ ;
                    continue ;
                }
                else goto done ;
            }
            case ':':
            {
                if ( inc == - 1 )
                {
                    if ( ( *( nc1 + inc ) ) == 'd' )
                    {
                        i ++ ;
                        continue ;
                    }
                }
                if ( ! C_SyntaxOn ) goto done ;
                else continue ;
            }
            case ',':
            {
                if ( C_SyntaxOn ) goto done ;
                //else fallthru ;
            }
            case ';':
            {
                //if ( inc == 1 ) rtrn |= ( LT_END_OF_BLOCK ) ; 
                //else if ( inc == - 1 ) rtrn |= ( LT_START_OF_BLOCK ) ;
                goto done ;
            }
            case ' ':
            {
                id = 0 ;
                continue ;
            }
            default:
            {
                if ( isalnum ( *nc1 ) ) id = 1 ;
                //else if ( ( inc == 1 ) && id ) rtrn |= LT_ID, id = 0  ;
                //else if ( ( inc == 1 ) && id && (rtrn & LT_AND|LT_OR)) rtrn |= LT_ID, id = 0  ;
                if ( ( inc == 1 ) && id ) rtrn |= LT_ID ;
            }
        }
        if ( ( inc == 1 ) && ( rtrn & LT_END_OF_BLOCK ) ) break ;
        else if ( rtrn & LT_START_OF_BLOCK ) break ;
    }
done:
    r->i = i ;
    r->rtrn |= rtrn ;
    return r->rtrn ;
}

Rllafl *
ReadLiner_LookForLogicMarkers ()
{
    Rllafl * r = Rllafl_New (TEMPORARY) ;
    Switch_LookAroundFor_Logic ( r, 1 ) ; //p = ( pnc + i ), la_code, 1, &mparenLevel, &pparenLevel ) ;
    r->nc -- ;
    Switch_LookAroundFor_Logic ( r, - 1 ) ; //p = ( pnc + i ), la_code, 1, &mparenLevel, &pparenLevel ) ;
    if ( r->rtrn && ( Is_DebugOn || ( Verbosity ( ) & ( 1 << 4 ) ) ) )
        Print_Defined_LogicVariable ( r ) ;
    return r ;
}

Rllafl *
Rllafl_New (uint64 allocType)
{
    ReadLiner * rl = _ReadLiner_ ;
    Rllafl * r = ( Rllafl * ) Mem_Allocate ( sizeof (Rllafl ), allocType ) ;
    BlockInfo *bi = ( BlockInfo * ) Stack_Top ( _Compiler_->CombinatorBlockInfoStack ) ;
    bi->BI_Rllafl = r ;
    r->oparenlvl = bi->ParenLevel ;
    r->pparenlvl = r->mparenlvl = _Compiler_->ParenLevel ;
    r->rtrn = 0 ;
    r->nc = & rl->InputLineString [ rl->ReadIndex - 1 ] ;
    r->Ncul = & rl->InputLineString [ BUFFER_SIZE ] ;
    r->Ncll = & rl->InputLineString [ 0 ] ;
    return r ;
}

void
Print_Defined_LogicVariable ( Rllafl * r )
{
    if ( r->rtrn )
    {
        byte * start = String_RemoveFinalNewline ( String_New ( r->nc - r->i, TEMPORARY ) ) ;
        byte * actual = String_RemoveFinalNewline ( String_New ( r->nc - ( r->i / 2 ), TEMPORARY ) ) ;
        iPrintf ( "\nDebug at : \'%s\' \n\t: \'%s\' : \n%s : ", start, actual, Context_Location ( ) ) ;
        if ( r->rtrn & LT_START_OF_BLOCK ) iPrintf ( " LT_START_OF_BLOCK" ) ;
        if ( r->rtrn & LT_OR_PREVIOUS ) iPrintf ( " LT_OR_PREVIOUS" ) ;
        if ( r->rtrn & LT_AND_PREVIOUS ) iPrintf ( " LT_AND_PREVIOUS" ) ;
        if ( r->rtrn & LT_2_LPAREN_PREVIOUS ) iPrintf ( " LT_2_LPAREN_PREVIOUS" ) ;
        if ( r->rtrn & LT_2_LPAREN_NEXT ) iPrintf ( " LT_2_LPAREN_NEXT" ) ;
        else if ( r->rtrn & LT_LPAREN ) iPrintf ( " LT_LPAREN" ) ;
        if ( r->rtrn & LT_2_RPAREN_PREVIOUS ) iPrintf ( " LT_2_RPAREN_PREVIOUS" ) ;
        if ( r->rtrn & LT_2_RPAREN_NEXT ) iPrintf ( " LT_2_RPAREN_NEXT" ) ;
        else if ( r->rtrn & LT_RPAREN ) iPrintf ( " LT_RPAREN" ) ;
        if ( r->rtrn & LT_AND_NEXT ) iPrintf ( " LT_AND_NEXT" ) ;
        else if ( r->rtrn & LT_AND ) iPrintf ( " LT_AND" ) ;
        if ( r->rtrn & LT_OR_NEXT ) iPrintf ( " LT_OR_NEXT" ) ;
        else if ( r->rtrn & LT_OR ) iPrintf ( " LT_OR" ) ;
        if ( r->rtrn & LT_ID ) iPrintf ( " LT_ID" ) ;
        if ( r->rtrn & LT_END_OF_BLOCK ) iPrintf ( " LT_END_OF_BLOCK" ) ;
    }
}

Boolean
ReadLiner_IsTokenForwardDotted ( ReadLiner * rl, int64 index )
{
    int64 i = 0, space = 0 ;
    Boolean escSeqFlag = false, inArray = false ;
    ; //, quoteMode = false ;
    byte * nc = & rl->InputLineString [ index ] ;
    for ( i = 0 ; 1 ; i ++, nc ++ )
    {
        if ( escSeqFlag )
        {
            if ( ( *nc ) != 'm' ) continue ;
            else
            {
                escSeqFlag = false ;
                continue ;
            }
        }
        else
        {
            switch ( *nc )
            {
                case ESC:
                {
                    escSeqFlag = true ;
                    continue ;
                }
                case ']':
                {
                    inArray = false ;
                    continue ;
                }
                case '[': return true ; //{ inArray = true ; continue ; } //continue ;
                case 0: case ',': case ';': case '(': case ')': case '\n': case '\'': return false ;
                case '.':
                {
                    if ( i && ( *( nc + 1 ) != '.' ) )// watch for (double/triple) dot ellipsis
                        return true ;
                    break ;
                }
                case '-':
                {
                    if ( *( nc + 1 ) == '>' )// watch for (double/triple) dot ellipsis
                        return true ;
                    break ;
                }
                case '"':
                {
                    //if ( i > index ) 
                    return false ;
                    //else quoteMode = true ;
                    //break ;
                }
                case ' ':
                {
                    space ++ ;
                    break ;
                }
                case '@':
                {
                    if ( C_SyntaxOn ) return false ;
                    else continue ;
                }
                default:
                {
                    //if ( ( ! GetState ( _Compiler_, ARRAY_MODE ) ) && space && isgraph ( *nc ) ) return false ;
                    //if ( ( ! inArray ) && space && isgraph ( *nc ) ) return false ;
                    if ( space && isgraph ( *nc ) ) return false ;
                    else
                    {
                        space = 0 ;
                        break ;
                    }
                }
            }
        }
    }
    return false ;
}

