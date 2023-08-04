#include "../../include/csl.h"
//===================================================================================================================
//| LO_Print
//===================================================================================================================

void
_LO_Print_Lambda_ToString ( LambdaCalculus * lc, ListObject * l0, int64 printValueFlag )
{
    lc->buffer [0] = 0 ;
    LC_sprintName ( lc->buffer, l0 ) ;
    LO_strcat ( lc->outBuffer, lc->buffer ) ;
    _LO_PrintListToString ( lc, ( ListObject * ) l0->Lo_LambdaParameters, 1, printValueFlag ) ; // 1 : lambdaFlag = 1 
    _LO_PrintListToString ( lc, ( ListObject * ) l0->Lo_LambdaBody, 1, printValueFlag ) ; // 1 : lambdaFlag = 1 
}

void
LO_PrintLiteralToString ( LambdaCalculus * lc, ListObject * l0 )
{
    if ( Namespace_IsUsing ( ( byte* ) "BigNum" ) ) _BigNum_FPrint ( ( mpfr_t * ) l0->W_Value ) ;
    else if ( ( l0->Lo_Integer < 0 ) || ( NUMBER_BASE_GET() == 16 ) )
        LC_snprintf1 ( lc->buffer, " 0x%016lx", ( uint64 ) l0->Lo_UInteger ) ;
    else LC_snprintf1 ( lc->buffer, ( ( l0->Lo_Integer < 0 ) ? " 0x%016lx" : " %ld" ), l0->Lo_Integer ) ;
}

void
LO_PrintValueToString ( LambdaCalculus * lc, ListObject * l0 )
{
    //LC_sprintName ( lc->buffer, l0 ) ;
    //LO_strcat ( lc->outBuffer, lc->buffer ) ;
    int64 * svDsp = _DspReg_ ;
    Word_Morphism_Run ( l0->Lo_CSL_Word ) ;
    int64 * tos = ( int64* ) DataStack_Pop ( ) ;
    l0->Lo_Value = * tos ;
    LO_PrintLiteralToString ( lc, l0 ) ;
    _DspReg_ = svDsp ;
}

void
_LO_Print_NonLambdaSymbol_ToString ( LambdaCalculus * lc, ListObject * l0, int64 printValueFlag )
{
    lc->buffer [0] = 0 ;
    if ( printValueFlag )
    {
        if ( *l0->Lo_PtrToValue != ( uint64 ) nil )
        {
            if ( ( ! *l0->Lo_PtrToValue ) && l0->Lo_CSL_Word )
            {
                if ( Verbosity () > 2 ) LC_snprintf2 ( lc->buffer, " %s = 0x%016lx", l0->Lo_CSL_Word->Lo_Name, ( int64 ) l0->Lo_CSL_Word ) ;
                else LC_sprintName ( lc->buffer, l0 ) ;
            }
                //else if ( ( l0->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) ) && ( l0->Lo_CSLWord ) && ( ! ( l0->Lo_CSLWord->W_LispAttributes & T_LISP_SYMBOL ) ) )
            else if ( ( l0->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) ) && ( ! ( l0->W_LispAttributes & T_LISP_SYMBOL ) ) )
            {
                LO_PrintValueToString ( lc, l0 ) ;
            }
            else if ( l0->W_LispAttributes & ( T_RAW_STRING ) ) LC_sprintString ( lc->buffer, *l0->Lo_PtrToValue ) ;
            else LC_sprintName ( lc->buffer, l0 ) ;
        }
        else
        {
            if ( Verbosity () > 2 ) LC_sprintf_String ( lc->buffer, " nil: %s", l0->Lo_Name ) ;
            else LC_sprintName ( lc->buffer, l0 ) ;
        }
    }
    else LC_sprintName ( lc->buffer, l0 ) ;
    LO_strcat ( lc->outBuffer, lc->buffer ) ;
}

void
_LO_PrintOneToString ( LambdaCalculus * lc, ListObject * l0, int64 in_a_LambdaFlag, int64 printSymbolValueFlag )
{
    if ( l0 )
    {
        if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) )
        {
            _LO_PrintListToString ( lc, l0, in_a_LambdaFlag, printSymbolValueFlag ) ;
        }
        else if ( ( l0 == _LC_->Nil ) || ( l0->W_LispAttributes & T_NIL ) )
        {
            if ( _AtCommandLine ( ) ) LC_sprintAString ( lc->buffer, " nil" ) ;
        }
        else if ( l0 == _LC_->True )
        {
            if ( _AtCommandLine ( ) ) LC_sprintAString ( lc->buffer, " true" ) ;
        }
        else if ( l0->W_LispAttributes == T_RAW_STRING ) LC_sprintString ( lc->buffer, l0->Name ) ; //l0->Lo_Value ) ;
        else if ( l0->W_LispAttributes & ( T_LC_DEFINE | T_LISP_COMPILED_WORD ) && ( ! GetState ( lc, LC_DEFINE_MODE ) ) )
        {
            LC_sprintName ( lc->buffer, l0 ) ;
        }
        else if ( l0->W_LispAttributes & T_LISP_SYMBOL )
        {
            if ( LO_IsQuoted ( l0 ) ) LC_sprintName ( lc->buffer, l0 ) ;
            else if ( ( ! in_a_LambdaFlag ) && l0->Lo_CSL_Word && ( l0->W_LispAttributes & T_LAMBDA ) ) _LO_Print_Lambda_ToString ( lc, l0, printSymbolValueFlag ) ;
            else _LO_Print_NonLambdaSymbol_ToString ( lc, l0, printSymbolValueFlag ) ;
        }
        else if ( ( l0->W_ObjectAttributes & ( T_STRING | T_RAW_STRING | T_LISP_SYMBOL ) ) ) //||( l0->W_LispAttributes & (T_LISP_SYMBOL) ))
        {
            if ( l0->State & UNQUOTED ) LC_sprintString ( lc->buffer, l0->Lo_String ) ;
            else LC_sprintf_String ( lc->buffer, " \"%s\"", l0->Lo_String ) ;
        }
        else if ( l0->W_MorphismAttributes & BLOCK ) LC_snprintf2 ( lc->buffer, " %s:#<BLOCK>:0x%016lx", l0->Lo_Name, ( uint64 ) l0->Lo_UInteger ) ;
        else if ( l0->W_ObjectAttributes & T_BIG_NUM )
        {
            //if ( l0->Name[0] ) // where does this come from ?? fix it!!
            {
                if ( printSymbolValueFlag && l0->Name[0] ) _BigNum_snfPrint2 ( lc->buffer, l0->Lo_Name, ( mpfr_t * ) l0->W_Value ) ; //_BigNum_FPrint ( ( mpfr_t * ) l0->W_Value ) ;
                else _BigNum_snfPrint2 ( lc->buffer, 0, ( mpfr_t * ) l0->W_Value ) ; //_BigNum_FPrint ( ( mpfr_t * ) l0->W_Value ) ;
                //else LC_sprintString ( lc->buffer, l0->Name ) ;
            }
        }
        else if ( l0->W_ObjectAttributes & T_INT )
        {
            if ( NUMBER_BASE_GET() == 16 ) LC_snprintf1 ( lc->buffer, " 0x%016lx", ( uint64 ) l0->Lo_UInteger ) ;
            else LC_snprintf1 ( lc->buffer, ( l0->Lo_Integer < 0 ) ? " 0x%016lx" : " %ld", l0->Lo_Integer ) ;
        }
        else if ( l0->W_ObjectAttributes & LITERAL ) LO_PrintLiteralToString ( lc, l0 ) ;
        else if ( l0->W_MorphismAttributes & ( CPRIMITIVE | CSL_WORD ) ) LC_sprintName ( lc->buffer, l0 ) ;
        //else if ( l0->W_LispAttributes & ( T_HEAD | T_TAIL ) ) ;
        else
        {
            if ( l0->W_WordData && l0->Lo_CSL_Word && l0->Lo_CSL_Word->Lo_Name ) LC_sprintString ( lc->buffer, l0->Lo_CSL_Word->Lo_Name ) ;
            else if ( l0->Name ) LC_sprintName ( lc->buffer, l0 ) ;
        }
    }
    LO_strcat ( lc->outBuffer, lc->buffer ) ;
}

void
_LO_PrintListToString ( LambdaCalculus * lc, ListObject * l0, int64 lambdaFlag, int64 printValueFlag )
{
    ListObject * l1, *lnext ;
    //lc->outBuffer[0] = 0 ;
    if ( l0 )
    {
        if ( l0->W_LispAttributes & ( LIST | LIST_NODE ) )
        {
            LC_sprintAString ( lc->buffer, "(" ) ;
            LO_strcat ( lc->outBuffer, lc->buffer ) ;
            for ( l1 = _LO_First ( l0 ) ; l1 ; l1 = lnext )
            {
                lnext = _LO_Next ( l1 ) ;
                _LO_PrintOneToString ( lc, l1, lambdaFlag, printValueFlag ) ;
            }
            LC_sprintAString ( lc->buffer, ")" ) ;
            LO_strcat ( lc->outBuffer, lc->buffer ) ;
        }
        else _LO_PrintOneToString ( lc, l0, lambdaFlag, printValueFlag ) ;
    }
}

byte *
LO_PrintListToString ( LambdaCalculus * lc, ListObject * l0, int64 lambdaFlag, int64 printValueFlag )
{
    if ( l0 )
    {
        Buffer_Init ( lc->PrintBuffer, 0 ) ;
        Buffer_Init ( lc->OutBuffer, 0 ) ;
        _LO_PrintListToString ( lc, l0, lambdaFlag, printValueFlag ) ;
        SetState ( lc, LC_PRINT_ENTERED, false ) ;
    }
    return lc->outBuffer ;
}

void
_LO_Print ( ListObject * l0, byte * prefix, byte * postfix, Boolean valueFlag, Boolean addQuoteFlag )
{
    LambdaCalculus * lc = _LC_ ;
    if ( l0 )
    {
        LO_PrintListToString ( lc, ( ListObject * ) l0, 0, valueFlag ) ;
        if ( addQuoteFlag )
        {
            byte * out ;
            if ( lc->outBuffer [0] == ' ' ) out = & lc->outBuffer [1] ;
            else out = lc->outBuffer ;
            iPrintf ( "%s\'%s\'%s", prefix, out, postfix ) ;
        }
        else iPrintf ( "%s%s%s", prefix, lc->outBuffer, postfix ) ;
    }
}

void
_LO_PrintWithValue ( ListObject * l0, byte * prefix, byte * postfix, Boolean indentFlag )
{
    //if ( _LC_->IndentDbgPrint && _LC_->ParenLevel ) 
    if ( l0 )
    {
        if ( indentFlag ) //&& _LC_->ParenLevel )
        {
            byte * b = Buffer_DataCleared ( _CSL_->StringInsertB3 ) ;
            snprintf ( b, BUFFER_IX_SIZE, "\n[%ld] %s", _LC_->ParenLevel, prefix ) ;
            prefix = b ;
        }
        _LO_Print ( l0, prefix, postfix, 1, 1 ) ;
    }
}

void
LO_PrintWithValue ( ListObject * l0 )
{
    _LO_PrintWithValue ( l0, ( byte* ) "", ( byte* ) "", 0 ) ;
}

byte *
_LO_PRINT_TO_STRING ( ListObject * l0 )
{
    return LO_PrintListToString ( _LC_, ( ListObject * ) l0, 0, 0 ) ;
}

byte *
_LO_PRINT_TO_STRING_WITH_VALUE ( ListObject * l0 )
{
    return LO_PrintListToString ( _LC_, ( ListObject * ) l0, 0, 1 ) ;
}

void
LO_Print ( ListObject * l0 )
{
    DefaultColors ;
    _LO_Print ( l0, ( byte* ) "", ( byte* ) "", 1, 0 ) ;
}

