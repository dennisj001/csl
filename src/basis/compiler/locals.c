#include "../../include/csl.h"

// ?? is the frame pointer needed ?? 
// remember LocalsStack is not pushed or popped so ...

/* ------------------------------------------------------
 *     a Locals Stack Frame on the DataStack - referenced by DSP
 * ------------------------------------------------------
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *     higher memory adresses -- nb. reverse from intel push/pop where push moves esp to a lower memory address and pop moves esp to a higher memory address. This seemed more intuitive.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * s <------------------------< new DSP - dsp [ n+1|n+2]   --- used by the new function
 * t           "saved esp"        slot n+1 fp [ n+1]   --- with 'return' function
 * a    "local variable n"        slot n   fp [  n ]
 * c    "local variable 3"        slot 3   fp [  3 ]
 * k    "local variable 2"        slot 2   fp [  2 ]
 *      "local variable 1"        slot 1   fp [  1 ]
 * f  -------------------------
 * r
 * a    saved pre fp = r15 //edi           fp [  0 ]  <--- new fp - FP points here > == r15[0] //edi[0] <pre dsp [ 1] --->
 * m                                                  <--- stack frame size = number of locals + 1 (fp) + 1 (esp)  --->
 * e <-----------------------------------------------
 *      "parameter variable"      slot 1   fp [ -1 ]   --- already on the "locals function" incoming stack     <pre dsp [ 0] == pre esi [0]> 
 *      "parameter variable"      slot 2   fp [ -2 ]   --- already on the "locals function" incoming stack     <pre dsp [-1]>
 *      "parameter variable"      slot x   fp [-etc]   --- already on the "locals function" incoming stack     <pre dsp [-2]>
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *     lower memory addresses  on DataStack - referenced by DSP
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      notations : fp = FP = Fp   = R15 //32 bit = EDI 
 *                                  sp = DSP = Dsp = R14 //32 bit = ESI
 */

int64
_ParameterVar_Offset ( Word * word, int64 numberOfArgs, Boolean frameFlag )
{
    int64 offset ;
    offset = - ( numberOfArgs - word->Index + ( frameFlag ? 1 : 0 ) ) ; // is this totally correct??
    return offset ;
}

int64
Compiler_ParameterVar_Offset ( Compiler * compiler, Word * word )
{
    int64 offset ;
    offset = _ParameterVar_Offset ( word, compiler->NumberOfArgs, Compiler_IsFrameNecessary ( compiler ) ) ;
    return offset ;
}

inline int64
LocalVar_FpOffset ( Word * word )
{
    return word->Index ;
}

inline int64
LocalVar_Disp ( Word * word )
{
    return ( word->Index * CELL ) ;
}

inline int64
ParameterVar_Disp ( Word * word )
{
    return ( Compiler_ParameterVar_Offset ( _Compiler_, word ) * CELL ) ;
}

inline int64
_LocalOrParameterVar_Offset ( Word * word, int64 numberOfArgs, Boolean frameFlag )
{
    return ( ( word->W_ObjectAttributes & LOCAL_VARIABLE ) ? ( LocalVar_FpOffset ( word ) ) : ( _ParameterVar_Offset ( word, numberOfArgs, frameFlag ) ) ) ;
}

inline int64
LocalOrParameterVar_Offset ( Word * word )
{
    //return ( ( word->CAttribute & LOCAL_VARIABLE ) ? ( LocalVar_FpOffset ( word ) ) : ( ParameterVar_Offset ( word ) ) ) ;
    return _LocalOrParameterVar_Offset ( word, _Compiler_->NumberOfArgs, Compiler_IsFrameNecessary ( _Compiler_ ) ) ;
}

inline int64
LocalOrParameterVar_Disp ( Word * word )
{
    int64 offset = LocalOrParameterVar_Offset ( word ) ;
    return ( offset * CELL_SIZE ) ;
}

Word *
Compiler_LocalWord_UpdateCompiler ( Compiler * compiler, Word * word, int64 objectType )
{
    compiler->NumberOfVariables ++ ;
    if ( objectType & REGISTER_VARIABLE )
    {
        compiler->NumberOfRegisterVariables ++ ;
        if ( objectType & LOCAL_VARIABLE )
        {
            word->Index = ++ compiler->NumberOfRegisterLocals ;
            ++ compiler->NumberOfLocals ;
        }
        else
        {
            word->Index = ++ compiler->NumberOfArgs ;
            ++ compiler->NumberOfRegisterArgs ;
        }
    }
    else
    {
        compiler->NumberOfNonRegisterVariables ++ ;
        if ( objectType & LOCAL_VARIABLE )
        {
            word->Index = ++ compiler->NumberOfNonRegisterLocals ;
            compiler->NumberOfLocals ++ ;
            if ( _CSL_->SC_Word ) _CSL_->SC_Word->W_NumberOfNonRegisterLocals = compiler->NumberOfNonRegisterLocals ;
            word->W_NumberOfNonRegisterLocals = compiler->NumberOfNonRegisterLocals ;
        }
        else
        {
            word->Index = ++ compiler->NumberOfArgs ;
            compiler->NumberOfNonRegisterArgs ++ ;
            if ( _CSL_->SC_Word ) _CSL_->SC_Word->W_NumberOfNonRegisterArgs = compiler->NumberOfNonRegisterArgs ;
            word->W_NumberOfNonRegisterArgs = compiler->NumberOfNonRegisterArgs ;
        }
    }
    word->W_ObjectAttributes |= RECYCLABLE_LOCAL ;
    return word ;
}

Word *
_Compiler_LocalWord_New ( Compiler * compiler, byte * name, int64 morphismType, int64 objectType, int64 lispType, int64 allocType )
{
    Word *word = _DObject_New ( name, 0, ( morphismType | IMMEDIATE ), objectType, lispType, LOCAL_VARIABLE, ( byte* ) _DataObject_Run, 0, 1, 0, allocType ) ;
    Compiler_LocalWord_UpdateCompiler ( compiler, word, objectType ) ;
    return word ;
}

Namespace *
Compiler_LocalsNamespace_FindOrNew ( Compiler * compiler )
{
    Namespace * ns = Namespace_FindOrNew_Local ( compiler->LocalsCompilingNamespacesStack, 1 ) ;
    Finder_SetQualifyingNamespace ( _Finder_, ns ) ;
    return ns ;
}

Word *
Compiler_LocalWord_New ( Compiler * compiler, byte * name, int64 morphismAttributes, int64 objectAttributes, int64 lispAttributes, int64 allocType )
{
    if ( ( ! GetState ( compiler, DOING_C_TYPE ) && ( ! GetState ( _LC_, LC_BLOCK_COMPILE ) ) ) ) compiler->LocalsNamespace = Compiler_LocalsNamespace_FindOrNew ( compiler ) ;
    Word * word = _Compiler_LocalWord_New ( compiler, name, morphismAttributes, objectAttributes, lispAttributes, allocType ) ;
    return word ;
}

//nb. correct only if Compiling !!

inline Boolean
IsFrameNecessary ( int64 numberOfNonRegisterLocals, int64 numberOfNonRegisterArgs )
{
    return ( ( numberOfNonRegisterLocals || numberOfNonRegisterArgs ) ? true : false ) ;
}

inline Boolean
Compiler_IsFrameNecessary ( Compiler * compiler )
{
    //return ( ( compiler->NumberOfNonRegisterLocals || compiler->NumberOfNonRegisterArgs ) ? true : false ) ;
    return IsFrameNecessary ( compiler->NumberOfNonRegisterLocals, compiler->NumberOfNonRegisterArgs ) ;
}

void
Compile_Init_LocalRegisterParamenterVariables ( Compiler * compiler )
{
    Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
    dllist * list = compiler->RegisterParameterList ;
    dlnode * node ;
    Boolean frameFlag = Compiler_IsFrameNecessary ( compiler ) ; // compiler->NumberOfNonRegisterLocals ; 
    for ( node = dllist_First ( ( dllist* ) list ) ; node ; node = dlnode_Next ( node ) ) //, i -- )
    {
        Word * word = ( Word* ) dobject_Get_M_Slot ( ( dobject* ) node, SCN_T_WORD ) ;
        _Compile_Move_StackN_To_Reg (word->RegToUse, ( frameFlag ? FP : DSP ), Compiler_ParameterVar_Offset ( compiler, word ) , 0) ;
    }
}

void
_Compiler_AddLocalFrame ( Compiler * compiler )
{
    Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
    _Compile_Move_Reg_To_StackN (DSP, 1, FP , 0) ; // save pre fp
    _Compile_LEA ( FP, DSP, 0, CELL ) ; // set new fp
    Compile_ADDI ( REG, DSP, 0, 1 * CELL, INT32_SIZE ) ; // 1 : fp - add stack frame -- this value is going to be reset 
    compiler->FrameSizeCellOffset = ( int64* ) ( Here - INT32_SIZE ) ; // in case we have to add to the framesize with nested locals
    d0 ( if ( Is_DebugOn ) Compile_Call_TestRSP ( ( byte* ) _CSL_Debugger_Locals_Show ) ) ;
}

void
Compiler_SetLocalsFrameSize_AtItsCellOffset ( Compiler * compiler )
{
    int64 size = compiler->NumberOfNonRegisterLocals ;
    int64 fsize = compiler->LocalsFrameSize = ( ( ( size <= 0 ? 0 : size ) + 1 ) * CELL ) ; //1 : the frame pointer 
    if ( fsize ) *( ( int32* ) ( compiler->FrameSizeCellOffset ) ) = fsize ; //compiler->LocalsFrameSize ; //+ ( IsSourceCodeOn ? 8 : 0 ) ;
}

void
CSL_DoReturnWord ( Word * word )
{
    Compiler * compiler = _Compiler_ ;
    //if ( word && (word->Name[0] != ';' ))
    {
        Word * word1 = CSL_Parse_Interpret_KeywordOperand ( word, 1 ) ;
        if ( word1 )
        {
            compiler->ReturnVariableWord = word1 ;
            if ( word1->W_ObjectAttributes & REGISTER_VARIABLE ) SetHere ( word1->Coding ) ;
            else Word_Check_ReSet_To_Here_StackPushRegisterCode ( word1 ) ;
            if ( ! _Readline_Is_AtEndOfBlock ( _Context_->ReadLiner0 ) ) _CSL_CompileCallJmpGoto ( 0, GI_RETURN ) ;
            return ;
        }
    }
    if ( word && ( word->W_MorphismAttributes & ( T_TOS ) ) )
    {
        //compiler->ReturnWord = word ;
        SetState ( compiler, RETURN_TOS, true ) ;
        byte mov_r14_rax [] = { 0x49, 0x89, 0x06 } ; //mov [r14], rax
        if ( memcmp ( mov_r14_rax, Here - 3, 3 ) )
        {
            ///compiler->ReturnWord = Compiler_CopyDuplicatesAndPush ( compiler->ReturnWord, - 1, - 1 ) ; // want to have a copy because 'return may be responsible for another code output in Compiler_RemoveLocalFrame
            //Compiler_Word_SCHCPUSCA ( compiler->ReturnWord, 0 ) ;
            Compile_Move_TOS_To_ACCUM ( DSP ) ; // save TOS to ACCUM so we can set return it as TOS below
        }
    }
    if ( word && ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE | LOCAL_VARIABLE | PARAMETER_VARIABLE ) ) )
    {
        if ( GetState ( _CSL_, TYPECHECK_ON ) )
        {
            Word * cwbc = _Context_->CurrentWordBeingCompiled ;
            if ( ( word->W_ObjectAttributes & LOCAL_VARIABLE ) && cwbc )
            {
                cwbc->W_TypeSignatureString [_Compiler_->NumberOfArgs] = '.' ;
                int8 swtsCodeSize = Tsi_ConvertTypeSigCodeToSize ( Tsi_Convert_Word_TypeAttributeToTypeLetterCode ( word ) ) ;
                int8 cwbctsCodeSize = Tsi_ConvertTypeSigCodeToSize ( cwbc->W_TypeSignatureString [_Compiler_->NumberOfArgs + 1] ) ;
                if ( swtsCodeSize > cwbctsCodeSize )
                    cwbc->W_TypeSignatureString [_Compiler_->NumberOfArgs + 1] = Tsi_Convert_Word_TypeAttributeToTypeLetterCode ( word ) ;
            }
        }
    }
    else if ( ! _Readline_Is_AtEndOfBlock ( _Context_->ReadLiner0 ) ) _CSL_CompileCallJmpGoto ( 0, GI_RETURN ) ;
}

// Compiler_RemoveLocalFrame : the logic definitely needs to be simplified???
// Compiler_RemoveLocalFrame has "organically evolved" it need to be logically simplified??
// honestly this function was not fully designed; it evolved : don't fully understand it yet ?? : 
// did what was necessary to make it work and it does seem to work for everything so far but
// it is probably the messiest function in csl along with DBG_PrepareSourceCodeString

void
Compiler_RemoveLocalFrame ( Compiler * compiler )
{
    Compiler_WordStack_SCHCPUSCA ( 0, 1 ) ;
    if ( ! GetState ( _Compiler_, LISP_MODE ) ) Compiler_WordStack_SCHCPUSCA ( 0, 0 ) ;
    int64 parameterVarsSubAmount = 0 ;
    Word * returnVariable = compiler->ReturnVariableWord ; //? compiler->ReturnVariableWord : compiler->ReturnWord ; //?  compiler->ReturnLParenVariableWord :  compiler->ReturnWord ;
    Boolean returnValueFlag = GetState ( compiler, RETURN_TOS ) || returnVariable ;
    if ( compiler->NumberOfArgs ) parameterVarsSubAmount = ( compiler->NumberOfArgs - returnValueFlag ) * CELL ;
    if ( compiler->NumberOfNonRegisterLocals || compiler->NumberOfNonRegisterArgs )
    {
        if ( ( returnValueFlag ) && ( ! IsWordRecursive ) ) //if ( ( returnValueFlag ) && ! ( _LC_ ) )
        {
            byte add_r14_0x8__mov_r14_rax [ ] = { 0x49, 0x83, 0xc6, 0x08, 0x49, 0x89, 0x06 } ; //"add r14, 0x8,  mov [r14], rax"
            if ( ! memcmp ( add_r14_0x8__mov_r14_rax, Here - 7, 7 ) )
            {
                CSL_AdjustDbgSourceCodeAddress ( Here, Here - 7 ) ;
                _ByteArray_UnAppendSpace ( _O_CodeByteArray, 7 ) ;
            }
        }
        // remove the incoming parameters -- like in C
        _Compile_LEA ( DSP, FP, 0, - CELL ) ; // restore sp - release locals stack frame
        _Compile_Move_StackN_To_Reg (FP, DSP, 1 , 0) ; // restore the saved pre fp - cf AddLocalsFrame
    }
    if ( ( parameterVarsSubAmount > 0 ) && ( ! IsWordRecursive ) ) Compile_SUBI ( REG, DSP, 0, parameterVarsSubAmount, 0 ) ; // remove stack variables ; recursive handled below
        // add a place on the stack for return value
    else if ( parameterVarsSubAmount < 0 ) Compile_ADDI ( REG, DSP, 0, abs ( parameterVarsSubAmount ), 0 ) ;
    else if ( ( parameterVarsSubAmount == 0 ) && returnValueFlag && ( ! compiler->NumberOfNonRegisterArgs ) && ( ! compiler->NumberOfArgs ) )
        Compile_ADDI ( REG, DSP, 0, CELL, 0 ) ;
    // nb : stack was already adjusted accordingly for this above by reducing the SUBI subAmount or adding if there weren't any parameter variables
    if ( returnValueFlag || IsWordRecursive )
    {
        Compiler_Word_SCHCPUSCA ( returnVariable, 0 ) ; // compiler->ReturnWord, 0 ) ;
        if ( returnVariable )
        {
            if ( returnVariable->W_ObjectAttributes & REGISTER_VARIABLE )
            {
                _Compile_Move_Reg_To_StackN (DSP, 0, returnVariable->RegToUse , 0) ;
                return ;
            }
        }
        Compile_Move_ACC_To_TOS ( DSP ) ;
    }
}

void
CSL_LocalsAndStackVariablesBegin ( )
{
    _CSL_Parse_LocalsAndStackVariables ( 1, 0, 0, 0, 0 ) ;
}

void
CSL_LocalVariablesBegin ( )
{

    _CSL_Parse_LocalsAndStackVariables ( 0, 0, 0, 0, 0 ) ;
}

// old docs :
// parse local variable notation to a temporary "_locals_" namespace
// calculate necessary frame size
// the stack frame (Fsp) will be just above TOS -- at higer addresses
// save entry Dsp in a CSL variable (or at Fsp [ 0 ]). Dsp will be reset to just
// above the framestack during duration of the function and at the end of the function
// will be reset to its original value on entry stored in the CSL variable (Fsp [ 0 ])
// so that DataStack pushes and pops during the function will be accessing above the top of the new Fsp
// initialize the words to access a slot in the framesize so that the
// compiler can use the slot number in the function being compiled
// compile a local variable such that when used at runtime it pushes
// the slot address on the DataStack

Boolean
ScanParametersForREGvars ( )
{
    byte * ils = String_New ( _ReadLiner_->InputLineString, TEMPORARY ) ;
    return ( strstr ( ils, "REG" ) || strstr ( ils, "REG" ) ) ;
}

Namespace *
_CSL_Parse_LocalsAndStackVariables ( int64 svf, int64 lispMode, ListObject * args, Stack * nsStack, Namespace * localsNs ) // svf : stack variables flag
{
    // number of stack variables, number of locals, svf : stack variable flag
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Lexer * lexer = cntx->Lexer0 ;
    Finder * finder = cntx->Finder0 ;
    int64 objectAttributes = 0, lispAttributes = 0, numberOfRegisterVariables = 0, numberOfVariables = 0 ;
    int64 svff = 0, getReturn = 0, regToUseIndex = 0, scm = IsSourceCodeOn ;
    byte * svDelimiters = lexer->TokenDelimiters, *token ;
    Word * word ;
    Boolean regFlag = false ;
    Namespace *typeNamespace = 0, *objectTypeNamespace = 0, *saveInNs = _CSL_->InNamespace ;
    //CSL_DbgSourceCodeOff ( ) ;
    //if ( ! CompileMode ) Compiler_Init ( compiler, 0 ) ;
    CSL_RightBracket ( ) ;

    if ( svf ) svff = 1 ;
    if ( ! localsNs ) localsNs = Namespace_FindOrNew_Local ( nsStack ? nsStack : compiler->LocalsCompilingNamespacesStack, 1 ) ;
    if ( lispMode == 2 ) args = ( ListObject * ) args ;
    else if ( lispMode ) args = ( ListObject * ) args->Lo_List->Head ;
#if 0 // use all reg vars ?? need to save regs before function calls and restore on return    
    Boolean rFlag = ScanParametersForREGvars ( ) ;
    if ( ( ! rFlag ) && GetState ( _CSL_, CO_ON ) && ( ! ( Namespace_IsUsing ( ( byte* ) "BigNum" ) ) ) )
        regFlag = true ;
#endif

    while ( ( lispMode ? ( int64 ) ( args = _LO_Next ( args ) ) : 1 ) )
    {
        if ( lispMode )
        {
            if ( args->W_LispAttributes & ( LIST | LIST_NODE ) ) args = _LO_First ( args ) ;
            token = ( byte* ) args->Lo_Name ;
            CSL_AddStringToSourceCode ( _CSL_, token ) ;
        }
        else token = _Lexer_ReadToken ( lexer, ( byte* ) " ,\n\r\t" ) ;
        if ( token )
        {
            if ( _String_EqualSingleCharString ( token, '(' ) ) continue ;
            if ( _String_EqualSingleCharString ( token, '*' ) )
            {
                objectAttributes |= O_POINTER ;
                continue ;
            }
            if ( _String_EqualSingleCharString ( token, '|' ) )
            {
                svff = 0 ; // set stack variable flag to off -- no more stack variables ; begin local variables
                continue ; // don't add a node to our temporary list for this token
            }
            if ( String_Equal ( ( char* ) token, "-t" ) ) // for setting W_TypeSignatureString
            {
                if ( lispMode )
                {
                    args = _LO_Next ( args ) ;
                    if ( args->W_LispAttributes & ( LIST | LIST_NODE ) ) args = _LO_First ( args ) ;
                    token = ( byte* ) args->Lo_Name ;
                    CSL_AddStringToSourceCode ( _CSL_, token ) ;
                }
                else token = _Lexer_LexNextToken_WithDelimiters ( lexer, 0, 1, 0, 1, LEXER_ALLOW_DOT ) ;
                strncpy ( ( char* ) _Context_->CurrentWordBeingCompiled->W_TypeSignatureString, ( char* ) token, 8 ) ;
                continue ; // don't add a node to our temporary list for this token
            }
            if ( _String_EqualSingleCharString ( token, ')' ) ) break ;
            if ( String_Equal ( ( char* ) token, "REG" ) || String_Equal ( ( char* ) token, "REGISTER" ) )
            {
                if ( GetState ( _CSL_, CO_ON ) ) regFlag = true ;
                continue ;
            }
            if ( ( ! GetState ( _Context_, C_SYNTAX ) ) && ( _String_EqualSingleCharString ( token, '{' ) ) ||
                ( _String_EqualSingleCharString ( token, ';' ) ) )
            {
                //_Printf ( "\nLocal variables syntax error : no closing parenthesis ')' found" ) ;
                CSL_Exception ( SYNTAX_ERROR, "\nLocal variables syntax error : no closing parenthesis ')' found", 1 ) ;
                break ;
            }
            if ( ! lispMode )
            {
                word = Finder_Word_FindUsing ( finder, token, 1 ) ; // 1: saveQns ?? find after Literal - eliminate making strings or numbers words ??
                if ( word && ( word->W_ObjectAttributes & ( NAMESPACE | CLASS | STRUCTURE ) ) ) //&& ( CharTable_IsCharType ( ReadLine_PeekNextChar ( lexer->ReadLiner0 ), CHAR_ALPHA ) ) )
                {
                    if ( word->W_ObjectAttributes & STRUCTURE )
                    {
                        //objectAttributes |= word->W_ObjectAttributes ;
                        objectTypeNamespace = word ;
                    }
                    else typeNamespace = word ;
                    continue ;
                }
            }
            if ( svff )
            {
                objectAttributes |= PARAMETER_VARIABLE ; // aka an arg
                if ( lispMode ) lispAttributes |= T_LISP_SYMBOL ; // no ltype yet for _CSL_LocalWord
            }
            else
            {
                objectAttributes |= LOCAL_VARIABLE ;
                if ( lispMode ) lispAttributes |= T_LISP_SYMBOL ; // no ltype yet for _CSL_LocalWord
            }
            if ( regFlag == true )
                //if ( ( regFlag == true ) || ((numberOfVariables < 4) && ( GetState ( _CSL_, CO_ON ) ) ) )
            {
                objectAttributes |= REGISTER_VARIABLE ;
                numberOfRegisterVariables ++ ;
                numberOfVariables -- ;
            }
            ///if ( ! objectTypeNamespace )
            word = DataObject_New ( LOCAL_VARIABLE, 0, token, 0, objectAttributes, lispAttributes, 0, 0, 0, DICTIONARY, - 1, - 1 ) ;
            //if ( lispMode ) 
            dllist_AddNodeToTail ( localsNs->W_List, ( dlnode* ) word ) ;
            if ( _Context_->CurrentWordBeingCompiled ) _Context_->CurrentWordBeingCompiled->W_TypeSignatureString [numberOfVariables ++] = '_' ;
            if ( regFlag == true )
            {
                //word->RegToUse = LocalsRegParameterOrder ( regToUseIndex ++ ) ; //, numberOfVariables ) ;
                word->RegToUse = compiler->Lrpo ( regToUseIndex ++ ) ; //, numberOfVariables ) ;
                if ( word->W_ObjectAttributes & PARAMETER_VARIABLE )
                {
                    //if ( ! compiler->RegisterParameterList ) compiler->RegisterParameterList = _dllist_New ( TEMPORARY ) ;
                    _List_PushNew_ForWordList ( compiler->RegisterParameterList, word, 1 ) ;
                }
                regFlag = false ;
            }
            if ( objectTypeNamespace )
            {
                Compiler_TypedObjectInit ( word, objectTypeNamespace ) ;
                Word_TypeChecking_SetSigInfoForAnObject ( word ) ;
            }
            else if ( typeNamespace ) word->CompiledDataFieldByteSize = typeNamespace->CompiledDataFieldByteSize ;
            if ( String_Equal ( token, "this" ) ) word->W_ObjectAttributes |= THIS ;
            typeNamespace = 0 ;
            objectTypeNamespace = 0 ;
            objectAttributes = 0 ;
        }
        else return 0 ; // Syntax Error or no local or parameter variables
    }
    compiler->State |= getReturn ;

    // we support nested locals and may have locals in other blocks so the indices are cumulative
    if ( numberOfRegisterVariables ) Compile_Init_LocalRegisterParamenterVariables ( compiler ) ; // word->NumberOfRegisterVariables = numberOfRegisterVariables

    finder->FoundWord = 0 ;
    Lexer_SetTokenDelimiters ( lexer, svDelimiters, COMPILER_TEMP ) ;
    SetState ( compiler, VARIABLE_FRAME, true ) ;
    SetState ( _CSL_, DEBUG_SOURCE_CODE_MODE, scm ) ;
    _CSL_->InNamespace = saveInNs ;
    compiler->LocalsNamespace = localsNs ;
    Qid_Save_Set_InNamespace ( localsNs ) ;
    return localsNs ;
}
