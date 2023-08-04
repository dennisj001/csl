
#include "../../include/csl.h"

int64
_CheckArrayDimensionForVariables_And_UpdateCompilerState ( )
{
    if ( _Readline_CheckArrayDimensionForVariables ( _Context_->ReadLiner0 ) ) return true ;
    else return false ;
}

// offset is calculated using this formula :
// d1 + d2*(D1) + d3*(D2*D1) + d4*(D3*D2*D1) ...
// where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes

/*
 * nb. CURRENTLY USING 'LITTLE ENDIAN ARRAYS'
 * d1 + d2*(D1) + d3*(D2*D1) + d4*(D3*D2*D1) ...
 * where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes :: eg. :: declared as : array [D1][D2][D3]... ; in use as : array [d1][d2][d3]...

 *  * This is pretty compilicated so comments are necessary ...
 * What must be dealt with in ArrayBegin :
 * CompileMode or not; Variables in array dimensions or not => 4 combinations
 *      - each dimension produces an offset which is added to any previous AccumulatedOffset (in R8) which is finally added to the object reference pointer
 * so ...
 *  varaibleFlag = _CheckArrayDimensionForVariables_And_UpdateCompilerState ( )
 * so ... 
 *      generally if a dimension has variables it must be compiled
 *          if no variables then calculate the dimension's offset and increment AccumulatedOffset
 *  if ( variableFlag )
 *  {
 *      Compile the 
 *      ( if ( ! variableFlag ) SetHere at exit to startHere and use AccumulatedOffset mechanism ( AccumulatedOffsetFlag ? ) )
 *  }
 *  else
 *  {
 *      // just interpret it with :
 *      // assume accumulated offset is TOS and keep it there
 *      _DataStack_SetTop ( _DataStack_GetTop ( ) + increment ) ; 
 *  }
 *  set compileMode to necessary state ( saveCompileMode || variableFlag )
 *  rem if we are incoming CompileMode and no variables then we want to interpret - set compileMode false
 *      - so no matter what if ( ! variableFlag ) set compile mode off
 *  Interpret ( token ) 
 *  if ( ! variableFlag ) reset compileMode to incoming state
 *  if ( CompileMode && ( ! variableFlag ) )
 *  {
 *      SetHere ( start ) ;
 *      SetCurrentAccumulatedOffset ( totalIncrement ) ;
 *  }

 * nb. CURRENTLY USING 'LITTLE ENDIAN ARRAYS'
 * d1 + d2*(D1) + d3*(D2*D1) + d4*(D3*D2*D1) ...
 * where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes :: eg. :: declared as : array [D1][D2][D3]... ; in use as : array [d1][d2][d3]...
 * versions > 0.854.312 : 20180920
 * could switch from little endian arrays to big endian arrays where first, left to right variable refers to
 * the largest Dimension, etc. So offset from array pointer is (for a four dimensional array) : d4*(D3*D2*D1) + d3*(D2*D1) d2*(D1) + d1 
 * where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes
 * for an array a[3][3][3] with 27 total positions a[2][0][0] would refer to 2*3*3 = 18th position ; a[2][0][1] 19th; 
 * a[2][1][1] (2*3*3) + (1*3) + 1 = 22 ; a[2][2][1] (2*3*3) + (2*3) + 1 = 25 ; a[2][2][2] (2*3*3) + (2*3) + 2 = 26 + 0 indexed item = 27 ; 
 * remember 0 indexed arrays 
 * so total is 27.
 */

void
Compile_ArrayDimensionOffset ( Word * word, int64 dimSize, int64 objSize )
{
    if ( word && ( ( word->W_ObjectAttributes & LITERAL ) ? word->W_Value : 1 ) ) // if ! zero else 
    {
        int64 size = dimSize * objSize ;
        // assume arrayIndex has just been pushed to TOS
        // nb. if size is zero this complete processing of an array dimension adding its amount to the pointer-offset on the stack
        if ( word->StackPushRegisterCode )
        {
            SetHere (word->StackPushRegisterCode) ;
            //_Compile_IMULI ( int64 mod, int64 reg, int64 rm, int64 sib, int64 disp, int64 imm, int64 size )
            if ( size > 1 ) Compile_IMULI ( REG, ACC, ACC, 0, 0, size ) ;
            //Compile_ADD( toRegOrMem, mod, reg, rm, sib, disp, isize ) 
            Compile_ADD ( MEM, MEM, ACC, DSP, 0, 0, CELL ) ;
        }
        else
        {
            if ( size > 1 ) Compile_IMULI ( MEM, ACC, DSP, 0, 0, size ) ;
            _Compile_Stack_DropN ( DSP, 1 ) ; // drop the array index
            Compile_ADD ( MEM, MEM, ACC, DSP, 0, 0, CELL ) ;
        }
    }
    else SetHere (word->Coding) ;
}

int64
CalculateArrayDimensionSize ( Word * arrayBaseObject, int64 dimNumber )
{
#define BIG_ENDIAN_ARRAYS 1 
#define LITTLE_ENDIAN_ARRAYS (! BIG_ENDIAN_ARRAYS)
#if LITTLE_ENDIAN_ARRAYS   
    /*
     * nb. CURRENTLY USING 'LITTLE ENDIAN ARRAYS'
     * d1 + d2*(D1) + d3*(D2*D1) + d4*(D3*D2*D1) ...
     * where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes :: eg. :: declared as : array [D1][D2][D3]... ; 
     * in use as : array [d1][d2][d3]...
     */
    int64 dimSize ;
    // nb. this method produces a 'little endian array' where the earlier dimensions are smaller and increasing with dimNumber
    //while ( ( -- dimNumber ) >= 0 ) // -- : zero based ns->ArrayDimensions
    for ( dimSize = 1 ; dimNumber > 0 ; dimNumber -- )
    {
        dimSize *= arrayBaseObject->ArrayDimensions [ dimNumber ] ; // the parser created and populated this array in _CSL_Parse_ClassStructure 
    }
#else
    /*
     * the switch from little endian arrays to big endian arrays where first, left to right variable refers to
     * the largest Dimension, etc. So offset from array pointer is (for a four dimensional array) : d4*(D3*D2*D1) + d3*(D2*D1) d2*(D1) + d1 
     * where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes
     * for an array a[3][3][3] with 27 total positions a[2][0][0] would refer to 2*3*3 = 18th position ; a[2][0][1] 19th; 
     * a[2][1][1] (2*3*3) + (1*3) + 1 = 22 ; a[2][2][1] (2*3*3) + (2*3) + 1 = 25 ; a[2][2][2] (2*3*3) + (2*3) + 2 = 26 + 0 indexed item = 27 ; 
     * remember 0 indexed arrays 
     * so total is 27.
     */
    // D0, D1, D2, ... Dn : d0, d1, d2 ... dn => dn*(1*D(n-1)*D1*D2*..D(n-1)) 
    int64 dimSize ;
    for ( dimSize = 1 ; dimNumber < ( arrayBaseObject->ArrayNumberOfDimensions - 1 ) ; dimNumber ++ )
    {
        dimSize *= arrayBaseObject->ArrayDimensions [ dimNumber ] ; // the parser created and populated this array in _CSL_Parse_ClassStructure 
    }
#endif    
    return dimSize ;
}
// v.0.775.840

int64
Do_NextArrayToken ( Word * tokenWord, byte * token, Word * arrayBaseObject, int64 objSize, Boolean saveCompileMode, Boolean *variableFlag )
{
    Context * cntx = _Context_ ;
    Interpreter * interp = cntx->Interpreter0 ;
    Compiler *compiler = cntx->Compiler0 ;
    Word * word ;
    if ( tokenWord ) word = tokenWord ;
    else
    {
        word = Finder_Word_FindUsing ( cntx->Finder0, token, 0 ) ;
        if ( word )
        {
            word->W_RL_Index = _Lexer_->TokenStart_ReadLineIndex ;
            word->W_SC_Index = _Lexer_->SC_Index ;
        }
    }
    SetState ( compiler, ARRAY_MODE, true ) ;
    if ( token [0] == '[' ) // '[' == an "array begin"
    {
        //once we have a variable any place in an array reference the flag must remain until the end of the array 
        if ( ! ( *variableFlag ) ) *variableFlag = _CheckArrayDimensionForVariables_And_UpdateCompilerState ( ) ;
        return 0 ;
    }
    else if ( token [0] == ']' ) return CSL_ArrayEnd ( word, arrayBaseObject, objSize, variableFlag ) ;
    if ( *variableFlag ) Set_CompileMode ( true ) ;
    else Set_CompileMode ( false ) ;
    if ( word ) Interpreter_DoWord ( interp, word, word->W_RL_Index, word->W_SC_Index ) ;
    else Interpreter_InterpretAToken ( interp, token, _Lexer_->TokenStart_ReadLineIndex, _Lexer_->SC_Index ) ;
    Set_CompileMode ( saveCompileMode ) ;

    return 0 ;
}

void
Arrays_DoArrayArgs_NonLisp ( Lexer * lexer, byte * token, Word * arrayBaseObject, int64 objSize, Boolean saveCompileMode, Boolean *variableFlag )
{
    int64 result ;
    do
    {
        token = Lexer_ReadToken ( lexer ) ;
        result = Do_NextArrayToken ( 0, token, arrayBaseObject, objSize, saveCompileMode, variableFlag ) ;
    }
    while ( ! result ) ;
}

void
_CSL_ArrayBegin ( Boolean lispMode, Word **pl1, int64 *i )
{
    Context * cntx = _Context_ ;
    Compiler *compiler = cntx->Compiler0 ;
    Interpreter * interp = cntx->Interpreter0 ;
    Lexer * lexer = cntx->Lexer0 ;
    Word * baseObject = cntx->BaseObject, *l1, * arrayBaseObject ;
    Boolean saveCompileMode = GetState ( compiler, COMPILE_MODE ), svOpState = GetState ( _CSL_, OPTIMIZE_ON ) ;
    int64 objSize = 0 ;
    Boolean variableFlag = 0 ;
    byte * token ;
    if ( lispMode )
    {
        l1 = * pl1 ;
        arrayBaseObject = _LC_->ArrayBaseObject ; //( ( Word * ) ( LO_Previous ( l1 ) ) )->Lo_CSL_Word ;
        baseObject = _LC_->BaseObject ;
        token = l1->Name ;
    }
    else
    {
        int64 i ;
        for ( i = 0 ; ; i++ ) 
        {
            arrayBaseObject = CSL_WordList ( i ) ;
            if ( IS_OBJECT_TYPE (arrayBaseObject) ) break ;
        }
        //arrayBaseObject = interp->LastWord ;
        token = lexer->OriginalToken ;
    }
    if ( arrayBaseObject )
    {
        CSL_OptimizeOn ( ) ; // internal to arrays optimize must be on

        if ( ! arrayBaseObject->ArrayDimensions ) CSL_Exception ( ARRAY_DIMENSION_ERROR, 0, QUIT ) ;
        if ( interp->CurrentObjectNamespace ) objSize = interp->CurrentObjectNamespace->CompiledDataFieldByteSize ;
        if ( ! objSize ) CSL_Exception ( OBJECT_SIZE_ERROR, 0, QUIT ) ;
        variableFlag = _CheckArrayDimensionForVariables_And_UpdateCompilerState ( ) ;
        if ( lispMode ) Arrays_DoArrayArgs_Lisp ( pl1, l1, arrayBaseObject, objSize, saveCompileMode, &variableFlag ) ;
        else Arrays_DoArrayArgs_NonLisp ( lexer, token, arrayBaseObject, objSize, saveCompileMode, &variableFlag ) ;
        _CSL_WordList_PushWord ( _CSL_->RightBracket, SCN_IN_USE_FLAG_ALL ) ; // for the optimizer
        if ( CompileMode ) // update the baseObject offset 
        {
            if ( ! variableFlag )
            {
                CSL_OptimizeOn ( ) ;
                SetHere (baseObject->Coding) ;
                Debugger_Set_StartHere ( _Debugger_ ) ; // for Debugger_DisassembleAccumulated
                _Debugger_->EntryWord = baseObject ; // for Debugger_DisassembleAccumulated
                _Compile_GetVarLitObj_LValue_To_Reg ( baseObject, ACC, 0 ) ;
                if ( lispMode )
                {
                    Compile_Move_Reg_To_Reg ( LispRegParameterOrder ( ( *i ) ++ ), ACC, 0 ) ;
                }
                else _Word_CompileAndRecord_PushReg (baseObject, ACC, true , 0) ;
            }
            else _CSL_OptimizeOff ( ) ; // can't really be optimized any more anyway and optimize is turned back on after an =/store anyway
            DEBUG_SHOW ( baseObject, 1, 0 ) ;
            compiler->ArrayEnds = 0 ; // reset for next array word in the current word being compiled
        }
        if ( lispMode ) cntx->BaseObject = 0 ;
        else
        {
            if ( ! variableFlag )
            {
                _dllist_MapNodes_UntilWord ( dllist_First ( ( dllist* ) _CSL_->CSL_N_M_Node_WordList ),
                    ( VMapNodeFunction ) SCN_Set_NotInUseForOptimization, baseObject ) ; // old version : SCN_Set_NotInUse but now keep use for source code
            }
            SetState ( compiler, COMPILE_MODE, saveCompileMode ) ;
            //SetState ( compiler, ARRAY_MODE, false ) ;
        }
    }
    if ( ! variableFlag ) SetState ( _CSL_, OPTIMIZE_ON, svOpState ) ;
#if 0    
    if ( ! _Context_StringEqual_PeekNextToken ( cntx, ( byte* ) "[", 0 ) )
    {
        _CSL_ArrayEnd ( ) ;
    }
#endif    
}

void
CSL_ArrayBegin ( )
{
    _CSL_ArrayBegin ( 0, 0, 0 ) ;
}

int64
CSL_ArrayEnd ( Word * word, Word * arrayBaseObject, int64 objSize, Boolean *variableFlag )
{
    Context * cntx = _Context_ ;
    Word * baseObject = cntx->BaseObject ;
    Compiler *compiler = cntx->Compiler0 ;
    int64 arrayIndex, increment ;

    int64 dimNumber = compiler->ArrayEnds ; // dimNumber is used as an array index so it is also zero base indexed
    int64 dimSize = CalculateArrayDimensionSize ( arrayBaseObject, dimNumber ) ; // dimNumber is used as an array index so it is also zero base indexed
    compiler->ArrayEnds ++ ; // after, because arrayBaseObject->ArrayDimensions is a zero based array

    Debugger_PreSetup ( _Debugger_, word, 0, 0, 0, 0 ) ;
    if ( CompileMode && * variableFlag ) Compile_ArrayDimensionOffset ( _Context_CurrentWord ( cntx ), dimSize, objSize ) ;
    else
    {
        // big endian arrays where first, left to right variable refers to
        // the largest Dimension, etc. So offset from array pointer is (for a four dimensional array) : d4*(D3*D2*D1) + d3*(D2*D1) d2*(D1) + d1 
        // where d1, d2, d3, ... are the dimension variables and D1, D2, D3, ... are the Dimension sizes
        // D0, D1, D2, ... Dn : d0, d1, d2 ... dn => dn*(1*D(n-1)*D1*D2*..D(n-1)) 
        arrayIndex = DataStack_Pop ( ) ;
        if ( arrayIndex >= arrayBaseObject->ArrayDimensions [ dimNumber ] ) Error ( ": Array index out of bounds.", ABORT ) ;
        increment = arrayIndex * dimSize * objSize ;
        Compiler_IncrementCurrentAccumulatedOffset ( compiler, increment ) ;
        if ( ! CompileMode ) Array_Do_AccumulatedAddress ( baseObject->AccumulatedOffset ) ;
    }
    if ( Is_DebugModeOn && baseObject ) Word_PrintOffset ( word, increment, baseObject->AccumulatedOffset ) ;
    CSL_TypeStack_Pop ( ) ; // pop the index ; we want the object field type 
    if ( ! _Context_StringEqual_PeekNextToken ( cntx, ( byte* ) "[", 0 ) )
    {
        _CSL_ArrayEnd ( ) ;
        return 1 ; // breaks the calling function
    }
    return 0 ; // continues the calling function
}

void
CSL_ArrayModeOff_OptimizeOn ( void )
{
    SetState ( _Compiler_, ARRAY_MODE, false ) ;
    CSL_OptimizeOn ( ) ;
}

void
_CSL_ArrayEnd ( void )
{
    CSL_ArrayModeOff_OptimizeOn ( ) ;
}


