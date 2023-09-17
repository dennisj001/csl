
#include "../../include/csl.h"

// rvalue - rhs value - right hand side of '=' - the actual value, used on the right hand side of C statements

// we run all new objects thru here ; good for debugging and understanding 

Word *
Context_DoTypeCast ( Context * cntx, Word * word )
{
    CSL_WordLists_PopWord ( ) ;
    word = _CopyDuplicateWord ( word, 1 ) ;
    word->TypeNamespace = _Context_->TypeCastNamespace ;
    CSL_WordList_PushWord ( word ) ;
    cntx->TypeCastNamespace = 0 ;
    SetState ( cntx, TYPE_CAST, false ) ;
    return word ;
}

// based on attributes not (just) types!?

void
Word_ObjectRun ( Word * word )
{
    Context * cntx = _Context_ ;
    cntx->Interpreter0->w_Word = word ;
    if ( GetState ( cntx, TYPE_CAST ) ) word = Context_DoTypeCast ( cntx, word ) ;
    Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    if ( word->W_ObjectAttributes & ( LITERAL | CONSTANT ) ) CSL_Do_LiteralWord ( word ) ;
    else
    {
        Context_GetState ( cntx, word ) ;
        if ( word->W_ObjectAttributes & LOCAL_OBJECT ) CSL_Do_LocalObject ( word ) ;
        else if ( ( word->W_ObjectAttributes & T_LISP_SYMBOL ) || ( word->W_LispAttributes & T_LISP_SYMBOL ) ) //lambda variables are parsed as CAttribute & T_LISP_SYMBOL
        {
            if ( ! GetState ( cntx, LC_CSL ) ) CSL_Do_LispSymbol ( word ) ;
            else CSL_Do_Variable ( word ) ;
        }
        else if ( word->W_ObjectAttributes & DOBJECT ) CSL_Do_DynamicObject ( word, ACC ) ;
        else if ( word->W_ObjectAttributes & OBJECT_FIELD ) CSL_Do_ObjectField ( word ) ;
        else if ( word->W_ObjectAttributes & ( C_TYPE | C_CLASS ) ) CSL_Do_C_Type ( word ) ;
        else if ( word->W_ObjectAttributes & ( THIS | OBJECT | STRUCT ) ) CSL_Do_Object ( word ) ;
        else if ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE | LOCAL_VARIABLE | PARAMETER_VARIABLE ) ) CSL_Do_Variable ( word ) ;

        if ( word->W_ObjectAttributes & ( NAMESPACE | CLASS | CLASS_CLONE ) ) Namespace_Do_Namespace ( word ) ; // namespaces are the only word that needs to run the word set DObject Compile_SetCurrentlyRunningWord_Call_TestRSP created word ??

        Context_ClearState ( cntx ) ;
    }
    CSL_TypeStackPush ( word ) ;
}

void
_DataObject_Run ( Word * word0 )
{
    Word * word = _Context_CurrentWord ( _Context_ ) ; // seems we don't need to compile definition code for object type words
    Word_ObjectRun ( word ) ;
}

void
DataObject_Run ( )
{
    Word * word = ( Word * ) DataStack_Pop ( ) ;
    //_DataObject_Run ( word ) ;
    Word_ObjectRun ( word ) ;
}

void
_Do_LocalObject_AllocateInit ( Namespace * typeNamespace, byte ** value, int64 size )
{
    Word * word = _CSL_ObjectNew ( size, ( byte* ) "<object>", 0, TEMPORARY ) ;
    _Class_Object_Init ( word, typeNamespace ) ;
    * value = ( byte* ) word->W_Value ;
}

void
CSL_Do_LocalObject ( Word * word )
{
    if ( ( word->W_ObjectAttributes & LOCAL_VARIABLE ) && ( ! ( word->W_ObjectAttributes & O_POINTER ) ) && ( ! GetState ( word, W_INITIALIZED ) ) ) // this is a local variable so it is initialed at creation 
    {
        int64 size = _Namespace_VariableValueGet ( word->TypeNamespace, ( byte* ) "size" ) ;
        Compile_MoveImm_To_Reg ( RDI, ( int64 ) word->TypeNamespace, CELL ) ;
        _Compile_LEA ( RSI, FP, 0, LocalVar_Disp ( word ) ) ;
        //_Compile_Move_Rm_To_Reg ( RSI, RSI, 0 ) ;
        Compile_MoveImm_To_Reg ( RDX, ( int64 ) size, CELL ) ;
        Compile_Call_TestRSP ( ( byte* ) _Do_LocalObject_AllocateInit ) ; // we want to only allocate this object once and only at run time; and not at compile time
        SetState ( word, W_INITIALIZED, true ) ;
    }
    CSL_Do_Object ( word ) ;
}

void
CSL_LocalObject_Init ( Word * word, Namespace * typeNamespace )
{
    int64 size ;
    word->TypeNamespace = typeNamespace ;
    word->W_MorphismAttributes |= typeNamespace->W_MorphismAttributes ;
    if ( typeNamespace->W_ObjectAttributes & CLASS ) word->W_ObjectAttributes |= OBJECT ;
    if ( Compiling ) word->W_ObjectAttributes |= LOCAL_OBJECT ;
    //if ( Compiling && ( ! ( word->W_ObjectAttributes & STRUCTURE ) ) ) word->W_ObjectAttributes |= LOCAL_OBJECT ;
    size = _Namespace_VariableValueGet ( word, ( byte* ) "size" ) ;
    word->ObjectByteSize = size ? size : typeNamespace->ObjectByteSize ;
    //_DObject_Init ( Word * word, uint64 value, uint64 ftype, byte * function, int64 arg )
    _DObject_Init ( word, ( int64 ) 0, LOCAL_OBJECT, ( byte* ) _DataObject_Run, 0 ) ;
    _Word_Add ( word, 1, 0 ) ; //?? is this necessary??
}

void
_Do_Compile_Variable ( Word * word )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    if ( GetState ( cntx, C_SYNTAX | INFIX_MODE ) || GetState ( compiler, LC_ARG_PARSING ) )
    {
        if ( GetState ( cntx, IS_RVALUE ) ) Compile_GetVarLitObj_RValue_To_Reg ( word, ACC, 0 ) ;
        else
        {
            //Compiler_Word_SCHCPUSCA ( word, 1 ) ;
            if ( ( word->W_ObjectAttributes & ( OBJECT | THIS | QID ) ) || GetState ( word, QID ) ) _Compile_GetVarLitObj_LValue_To_Reg ( word, ACC, 0 ) ;
            else return ; //// this compilation is delayed to _CSL_C_Infix_Equal/Op
        }
    }
    else _Compile_GetVarLitObj_LValue_To_Reg ( word, ACC, 0 ) ;

    if ( ! Compiler_Var_Compile_LogicTest ( compiler ) )
        _Word_CompileAndRecord_PushReg ( word, ( word->W_ObjectAttributes & REGISTER_VARIABLE ) ? word->RegToUse : ACC, true, 0 ) ;
}

void
Do_Variable ( Word * word )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    int64 value = 0 ;
    if ( CompileMode )
    {
        if ( GetState ( cntx, ( C_SYNTAX | INFIX_MODE ) )
            && ( ! GetState ( cntx, ( IS_FORWARD_DOTTED | IS_RVALUE ) ) ) && ( ! compiler->LHS_Word ) )
        {
            Compiler_Set_LHS ( word ) ;
        }
        _Do_Compile_Variable ( word ) ;
    }
    else
    {
        if ( word->W_ObjectAttributes & ( OBJECT | THIS ) )
        {
            if ( word->W_ObjectAttributes & THIS )
            {
                if ( GetState ( cntx, IS_RVALUE ) ) value = ( int64 ) word->W_Value ;
                value = ( int64 ) word->W_PtrToValue ;
            }
            else value = ( int64 ) word->W_Value ;
            if ( ! compiler->LHS_Word ) Compiler_Set_LHS ( word ) ;
        }
        else
        {
            if ( GetState ( cntx, ( C_SYNTAX | INFIX_MODE ) ) )
            {
                if ( GetState ( cntx, IS_RVALUE ) ) value = ( int64 ) * word->W_PtrToValue ;
                else
                {
                    if ( ! compiler->LHS_Word ) Compiler_Set_LHS ( word ) ;
                    value = ( int64 ) word->W_PtrToValue ;
                }
            }
            else if ( GetState ( cntx, IS_RVALUE ) ) value = word->W_Value ;
            else value = ( int64 ) word->W_PtrToValue ;
        }
        if ( GetState ( cntx, IS_REVERSE_DOTTED ) && ( cntx->BaseObject && ( cntx->BaseObject != word ) ) ) TOS = value ;
        else DataStack_Push ( value ) ;
    }
#if 0
    if ( ( word->W_ObjectAttributes & ( C_TYPE | C_CLASS | STRUCT ) ) || GetState ( cntx, IS_FORWARD_DOTTED ) )
        //Finder_SetQualifyingNamespace ( cntx->Finder0, word->TypeNamespace ) ;
        Finder_SetQualifyingNamespace ( cntx->Finder0, word->TypeNamespace ? word->TypeNamespace : word ) ; //word->TypeNamespace ) ;
#else    
    //if ( ( word->W_ObjectAttributes & ( CLASS_TYPE ) ) || GetState ( cntx, IS_FORWARD_DOTTED ) ) Finder_SetQualifyingNamespace ( cntx->Finder0, word->TypeNamespace ) ;
    if ( ( word->W_ObjectAttributes & ( STRUCT | OBJECT | O_POINTER ) ) || GetState ( cntx, IS_FORWARD_DOTTED ) )
        Finder_SetQualifyingNamespace ( cntx->Finder0, word->TypeNamespace ? word->TypeNamespace : word ) ; //word->TypeNamespace ) ;
#endif    
}

void
CSL_Do_Variable ( Word * word )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Interpreter * interp = cntx->Interpreter0 ;
    if ( ( word->W_ObjectAttributes & ( STRUCTURE | THIS ) ) || ( ! ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) ) ) )
    {
        if ( word->W_ObjectAttributes & ( STRUCTURE | OBJECT | THIS | QID ) || GetState ( word, QID ) ) //|| GetState ( cntx, IS_FORWARD_DOTTED ) ) 
        {
            word->AccumulatedOffset = 0 ;
            interp->CurrentObjectNamespace = TypeNamespace_Get ( word ) ;
            Compiler_Init_AccumulatedOffsetPointers ( compiler, word ) ;
            word->W_ObjectAttributes |= OBJECT ;
            if ( word->W_ObjectAttributes & THIS ) word->S_ContainingNamespace = _Context_->Interpreter0->ThisNamespace ;
        }
    }
    if ( ( ! GetState ( compiler, ARRAY_MODE ) ) && ( ! GetState ( cntx, IS_FORWARD_DOTTED ) )
        && ( ! GetState ( cntx, IS_REVERSE_DOTTED ) ) ) cntx->BaseObject = 0 ;
    Do_Variable ( word ) ;
}

void
CSL_Do_Object ( Word * word )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    Interpreter * interp = cntx->Interpreter0 ;
    if ( ( word->W_ObjectAttributes & THIS ) || ( ! ( word->W_ObjectAttributes & ( NAMESPACE_VARIABLE ) ) ) )
    {
        if ( word->W_ObjectAttributes & ( OBJECT | THIS | QID ) || GetState ( word, QID ) ) //|| GetState ( cntx, IS_FORWARD_DOTTED ) ) 
        {
            interp->CurrentObjectNamespace = TypeNamespace_Get ( word ) ;
            Compiler_Init_AccumulatedOffsetPointers ( compiler, word ) ;
            if ( word->W_ObjectAttributes & THIS ) word->S_ContainingNamespace = interp->ThisNamespace ;
        }
    }
    if ( ( ! GetState ( compiler, ARRAY_MODE ) ) && ( ! GetState ( cntx, IS_FORWARD_DOTTED ) )
        && ( ! GetState ( cntx, IS_REVERSE_DOTTED ) ) && ( ! word->W_ObjectAttributes & LOCAL_OBJECT ) ) cntx->BaseObject = 0 ;
    Do_Variable ( word ) ;
}

void
_Do_LiteralValue ( int64 value )
{
    if ( CompileMode )
    {
        Compile_MoveImm_To_Reg ( ACC, value, CELL ) ;
        CSL_CompileAndRecord_PushAccum ( ) ; // does word == top of word stack always
    }
    else DataStack_Push ( value ) ;
}

void
CSL_Do_LiteralWord ( Word * word )
{
    _Do_LiteralValue ( word->W_Value ) ;
    //CSL_TypeStackPush ( word ) ;
}

void
Compile_C_FunctionDeclaration ( byte * token1 )
{
    byte * token ;
    _Compiler_->C_FunctionBackgroundNamespace = _Compiler_->C_BackgroundNamespace ;
    SetState ( _Compiler_, C_COMBINATOR_PARSING, true ) ;
    CSL_C_Syntax_On ( ) ;
    Word * word = Word_New ( token1 ) ; // "("
    CSL_WordList_PushWord ( word ) ;
    Compiler_Word_SCHCPUSCA ( word, 0 ) ;
    DataStack_Push ( ( int64 ) word ) ;
    CSL_BeginBlock ( ) ; // nb! before CSL_LocalsAndStackVariablesBegin
    CSL_LocalsAndStackVariablesBegin ( ) ;
    do // the rare occurence of any tokens between closing locals right paren ')' and beginning block '}'
    {
        if ( token = Lexer_ReadToken ( _Lexer_ ) )
        {
            if ( String_Equal ( token, "s{" ) )
            {
                Interpreter_InterpretAToken ( _Interpreter_, token, - 1, - 1 ) ;
                break ;
            }
            else if ( token [ 0 ] == '{' ) break ; // take nothing else (would be Syntax Error ) -- we have already done CSL_BeginBlock
            else _Lexer_ConsiderDebugAndCommentTokens ( token, 1 ) ;
        }
    }
    while ( token ) ;
}

// this is currently kinda rough
// this also interprets the rest of a c function after a type declaration

byte *
_Compile_C_TypeDeclaration ( )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    byte * token ;
    while ( token = Interpret_C_Until_NotIncluding_Token5 ( cntx->Interpreter0, ( byte* ) ",", ( byte* ) ";", ( byte* ) "{", ( byte* ) "}", ( byte* ) "#", 0, 0, 1 ) )
    {
        if ( _String_EqualSingleCharString ( token, ';' ) )
        {
            Lexer_ReadToken ( _Lexer_ ) ;
            Interpreter_InterpretAToken ( cntx->Interpreter0, token, _Lexer_->TokenStart_ReadLineIndex, _Lexer_->SC_Index ) ;
            break ;
        }
        else if ( _String_EqualSingleCharString ( token, ',' ) )
        {
            Lexer_ReadToken ( _Lexer_ ) ;
            break ;
        }
        else
        {
            if ( _String_EqualSingleCharString ( token, ')' ) && GetState ( compiler, DOING_A_PREFIX_WORD ) ) CSL_PushToken_OnTokenList ( token ) ;
            if ( GetState ( cntx, C_SYNTAX ) ) Compiler_Save_C_BackgroundNamespace ( compiler ) ;
            break ;
        }
        compiler->LHS_Word = 0 ;
    }
    return token ;
}

// nb.Compiling !!
// for type declarations not function declarations??

void
Compile_C_TypeDeclaration ( Namespace * ns, byte * token )
{
    Interpreter * interp = _Interpreter_ ;
    Word * word ;
    byte * token1 ;
    if ( token && Compiling )
    {
        if ( token[0] == ')' )
        {
            //  C cast code here ; 
            // nb! : we mostly have no (fully) implemented operations on operand size less than 8 bytes
            token1 = Lexer_ReadToken ( _Lexer_ ) ;
            if ( token1 )
            {
                Word * word0 = _Interpreter_TokenToWord ( interp, token1, - 1, - 1 ) ;
                word = Compiler_CopyDuplicatesAndPush ( word0, _Lexer_->TokenStart_ReadLineIndex, _Lexer_->SC_Index ) ;
                if ( word )
                {
                    word->ObjectByteSize = CSL_GetAndSet_ObjectByteSize ( word ) ;
                    Interpreter_DoWord ( interp, word, - 1, - 1 ) ;
                }
            }
            else SetState ( _Context_->Lexer0, END_OF_LINE, true ) ;
        }
        else
        {
            Namespace * lns = Compiler_LocalsNamespace_FindOrNew ( _Compiler_ ) ;
            do // reorganize this ??
            {
                int64 objectAttributes ;
                if ( token[0] == '*' )
                {
                    objectAttributes = O_POINTER ;
                    token = Lexer_ReadToken ( _Lexer_ ) ;
                }
                else objectAttributes = 0 ;
                if ( token[0] == ';' ) break ;
                Word * word = Lexer_ParseToken_ToWord ( _Lexer_, token, - 1, - 1 ) ;
                if ( word->W_MorphismAttributes & ( DEBUG_WORD ) )
                {
                    Word_Morphism_Run ( word ) ; //Interpreter_InterpretAToken ( _Interpreter_, token1, - 1, - 1 ) ;
                    token = Lexer_ReadToken ( _Lexer_ ) ;
                    continue ;
                }
                else if ( _Lexer_->L_ObjectAttributes & ( T_RAW_STRING | KNOWN_OBJECT ) )
                {
                    if ( ns->W_ObjectAttributes & ( OBJECT | STRUCT ) ) objectAttributes |= ( OBJECT | STRUCT ) ;
                    word->W_ObjectAttributes = ( LOCAL_VARIABLE | objectAttributes ) ; //| ns->W_ObjectAttributes ) ;
                    Compiler_LocalWord_UpdateCompiler ( _Compiler_, word, LOCAL_VARIABLE | objectAttributes ) ;
                    if ( ns->W_ObjectAttributes & ( OBJECT | STRUCT ) ) CSL_LocalObject_Init ( word, ns ) ;
                    else _Word_Add ( word, 0, lns ) ;
                }
                token = Lexer_Peek_NextToken ( _Lexer_, 1, 1 ) ;
                if ( strchr ( token, '=' ) )
                {
                    Compiler_Set_LHS ( word ) ;
                    token = _Compile_C_TypeDeclaration ( ) ;
                }
#if 0 // not yet              
                else if ( strchr ( token, '[' ) )
                {
                    //_CSL_ArrayBegin ( 0, 0, 0 ) ;
                }
#endif                 
                else token = Lexer_ReadToken ( _Lexer_ ) ;

                if ( token && ( token[0] == ',' ) ) token = Lexer_ReadToken ( _Lexer_ ) ;
            }
            while ( token && ( token[0] != ';' ) ) ;
        }
    }
}

void
_CSL_Do_C_Type ( Namespace * ns )
{
    Lexer * lexer = _Lexer_ ;
    byte * token1, *token2 ;
    // in case we have a number of C types in a row (why? i don't know but...) the last should be the primary namespace
    while ( 1 )
    {
        token1 = _Lexer_Next_Token ( lexer, 0, 1, 0, 1 ) ;
        if ( token1[0] == ')' )
        {
            CSL_PushToken_OnTokenList ( token1 ) ;
            SetState ( _Context_, TYPE_CAST, true ) ;
            _Context_->TypeCastNamespace = ns ;
            // nb : this doesn't do anything to cast the type now just correctly parses it so we can more easily insert C code directly
            //C_TypeCast () ; //set sizeof for next object/variable for typechecking possibly convert to that size ?? to be implemented
            return ;
        }
        Word * word = _Interpreter_TokenToWord ( _Interpreter_, token1, - 1, - 1 ) ;
        if ( word->W_ObjectAttributes & ( C_TYPE | C_CLASS ) ) ns = word ;
        else if ( word->W_MorphismAttributes & ( DEBUG_WORD ) )
        {
            Interpreter_InterpretAToken ( _Interpreter_, token1, - 1, - 1 ) ;
            break ;
        }
        else break ;
    }
    if ( ! Compiling ) _Namespace_ActivateAsPrimary ( ns ) ;
    SetState ( _Compiler_, DOING_C_TYPE_DECLARATION, true ) ;
    token2 = Lexer_Peek_NextToken ( lexer, 1, 1 ) ;
    if ( token2 && ( token2 [0] == '(' ) ) Compile_C_FunctionDeclaration ( token1 ) ;
    else
    {
        if ( Compiling ) Compile_C_TypeDeclaration ( ns, token1 ) ;
        else Interpreter_InterpretAToken ( _Interpreter_, token1, - 1, - 1 ) ; //_Lexer_->TokenStart_ReadLineIndex, _Lexer_->SC_Index ) ;
    }
    SetState ( _Compiler_, DOING_C_TYPE_DECLARATION, false ) ;
}

void
CSL_Do_C_Type ( Namespace * ns )
{
    Context * cntx = _Context_ ;
    if ( C_SyntaxOn && ( ! GetState ( cntx, IS_FORWARD_DOTTED ) ) )
    {
        Compiler * compiler = cntx->Compiler0 ;
        if ( GetState ( cntx, C_SYNTAX ) ) Compiler_Save_C_BackgroundNamespace ( compiler ) ;
        if ( ( ! Compiling ) || ( ! GetState ( _CSL_, SOURCE_CODE_STARTED ) ) ) CSL_InitSourceCode_WithCurrentInputChar ( _CSL_, 0 ) ; // must be here for wdiss and add addToHistory
        if ( ! GetState ( compiler, DOING_C_TYPE ) )
        {
            SetState ( compiler, DOING_C_TYPE, true ) ;
            if ( ! GetState ( compiler, LC_ARG_PARSING ) )
            {
                _LC_Delete ( _LC_ ) ;
                if ( ! Compiling )
                {
                    Compiler_Init ( compiler, 0 ) ;
                    CSL_SourceCode_Init ( ) ;
                    CSL_WordList_Init ( 0 ) ;
                }
                _CSL_Do_C_Type ( ns ) ;
            }
            SetState ( compiler, DOING_C_TYPE, false ) ;
        }
        if ( GetState ( cntx, C_SYNTAX ) ) Compiler_SetAs_InNamespace_C_BackgroundNamespace ( compiler ) ;
    }
    else Namespace_Do_Namespace ( ns ) ;
}

void
Context_CheckAddressDeReference ( Context * cntx, Word * word )
{
    if ( C_SyntaxOn && GetState ( word, IS_RVALUE ) && ( ! ( GetState ( cntx, ADDRESS_OF_MODE ) ) )
        && GetState ( cntx, IS_REVERSE_DOTTED ) && ( ! GetState ( cntx, IS_FORWARD_DOTTED ) ) )
    {
        int8 reg = word->W_BaseObject->RegToUse ;
        //iPrintf ( "\nCheckAddressDeReference : at %s", Context_Location () ) ;
        SetHere ( word->W_BaseObject->StackPushRegisterCode ) ;
        Compile_Move_Rm_To_Reg ( reg, reg, 0, 8 ) ;
        cntx->BaseObject->W_BaseObject = word ;
        Word_Set_StackPushRegisterCode_To_Here ( word->W_BaseObject ) ;
        _Compile_Stack_PushReg ( DSP, RAX, 0 ) ;
    }
}

void
CSL_Do_ObjectField ( Word * word )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    byte * offsetPtr = 0 ;
    cntx->Interpreter0->CurrentObjectNamespace = word ; // update this namespace 
    compiler->ArrayEnds = 0 ;

    if ( GetState ( cntx, ( C_SYNTAX | INFIX_MODE ) ) && ( ! compiler->LHS_Word )
        && ( ! GetState ( cntx, IS_FORWARD_DOTTED ) ) && ( ! GetState ( cntx, IS_RVALUE ) ) ) Compiler_Set_LHS ( word ) ;
    if ( word->WD_Offset ) offsetPtr = Compiler_IncrementCurrentAccumulatedOffset ( compiler, word->WD_Offset ) ;
    if ( ! ( CompileMode || GetState ( compiler, LC_ARG_PARSING ) ) )
        CSL_Do_AccumulatedAddress ( word, ( byte* ) TOS, word->WD_Offset ) ;
    if ( GetState ( cntx, IS_FORWARD_DOTTED ) ) Finder_SetQualifyingNamespace ( cntx->Finder0, word->TypeNamespace ) ;
    word->W_BaseObject = cntx->BaseObject ;
    CSL_TypeStack_SetTop ( word ) ;
    Word_SetSourceCoding ( word, offsetPtr - 3 ) ; // 3 : sizeof add immediate insn with rex ; set to point to the actual offset value
    if ( Compiling )
    {
        int32 offset ;
        if ( ( word->W_ObjectAttributes & ( STRUCT | OBJECT | O_POINTER ) ) && GetState ( cntx, IS_FORWARD_DOTTED ) )
        {
            //DBI_ON ;
            int8 reg = word->W_BaseObject->RegToUse ;
            SetHere ( word->W_BaseObject->StackPushRegisterCode ) ;
            if ( offsetPtr )
            {
                offset = ( * ( int32 * ) offsetPtr ) ;
                if ( offset )
                {
                    Compile_Move_Rm_To_Reg ( reg, reg, 0, 8 ) ;
                    Compile_ADDI ( REG, reg, 0, 0, INT32_SIZE ) ; // only a 32 bit offset ??
                    compiler->AccumulatedOffsetPointer = ( int32* ) ( Here - INT32_SIZE ) ; // offset will be calculated as we go along by ClassFields and Array accesses
                }
                //else oPrintf ("") ; //debug
            }
            Word_Set_StackPushRegisterCode_To_Here ( word->W_BaseObject ) ;
            _Compile_Stack_PushReg ( DSP, reg, 0 ) ;
            //DBI_OFF ;
        }
        else if ( word->W_BaseObject ) Context_CheckAddressDeReference ( cntx, word ) ;
    }
#if 0    
    dbg ( 0, ( int64 ) word, 0 ) ;
    if ( ( word->W_ObjectAttributes & ( STRUCT | OBJECT | O_POINTER ) ) || GetState ( cntx, IS_FORWARD_DOTTED ) )
        Finder_SetQualifyingNamespace ( cntx->Finder0, word ) ; //word->TypeNamespace ) ;
#endif    
}

// a constant is, of course, a literal

void
CSL_Do_LispSymbol ( Word * word )
{
    // rvalue - rhs for stack var
    if ( Compiling )
    {
        _Compile_GetVarLitObj_RValue_To_Reg ( word, ACC, 0 ) ;
        _Word_CompileAndRecord_PushReg ( word, ACC, true, 0 ) ;
    }
    //CSL_TypeStackPush ( word ) ;
}

void
Array_Do_AccumulatedAddress ( int64 totalOffset )
{
    byte * address = ( ( byte* ) _Context_->BaseObject->W_Value ) + totalOffset ;
    TOS = ( uint64 ) address ; // C lvalue
}

void
Do_AccumulatedAddress ( byte * accumulatedOffsetPointer, int64 offset, byte size )
{
    if ( accumulatedOffsetPointer )
    {
        Context * cntx = _Context_ ;
        accumulatedOffsetPointer += offset ;
        if ( GetState ( cntx, IS_RVALUE ) )
        {
            if ( size == 1 ) TOS = ( int64 ) ( ( * ( byte* ) accumulatedOffsetPointer ) ) ; // C rvalue
            else if ( size == 2 ) TOS = ( int64 ) ( ( * ( int16* ) accumulatedOffsetPointer ) ) ; // C rvalue
            else if ( size == 4 ) TOS = ( int64 ) ( ( * ( int32* ) accumulatedOffsetPointer ) ) ; // C rvalue
                //else if ( size == 8 ) TOS = ( int64 ) ( ( * ( int64* ) accumulatedOffsetPointer ) ) ; // C rvalue
            else TOS = ( * ( int64* ) accumulatedOffsetPointer ) ; // default and 8 bytes - cell
        }
        else TOS = ( uint64 ) accumulatedOffsetPointer ; // C lvalue
    }
}

void
CSL_Do_AccumulatedAddress ( Word * word, byte * accumulatedAddress, int64 offset )
{
    //if ( accumulatedAddress )
    {
        Namespace * ns = TypeNamespace_Get ( word ) ;
        byte size = 0 ;
        size = ( ns ? ( int64 ) _Namespace_VariableValueGet ( ns, ( byte* ) "size" ) : CELL ) ;
        if ( ( word->W_ObjectAttributes & O_POINTER ) || ( ns->W_ObjectAttributes & OBJECT ) ) size = 8 ;
        //else size = 8 ;

        Do_AccumulatedAddress ( accumulatedAddress, offset, ( size <= 8 ) ? size : 8 ) ;
    }
}

// nb. 'word' is the previous word to the '.' (dot) cf. CSL_Dot so it can be recompiled, a little different maybe, as an object
// .
#if 1 // this is handled by Word_ObjectRun

void
CSL_Dot ( )
{
}

void
_CSL_Do_Dot ( Context * cntx, Word * word )
{
}
#elif 0

void
_CSL_Do_Dot ( Context * cntx, Word * word ) // .
{
    if ( word && ( ! cntx->BaseObject ) )
    {
        if ( word->W_ObjectAttributes & NAMESPACE_TYPE ) Finder_SetQualifyingNamespace ( cntx->Finder0, word ) ;
        else cntx->BaseObject = word ;
    }
}

void
CSL_Dot ( ) // .
{
    Context * cntx = _Context_ ;
    SetState ( cntx, CONTEXT_PARSING_QID, true ) ;
    Word * word = Compiler_PreviousNonDebugWord ( 0 ) ; // 0 : rem: we just popped the WordStack above
    _CSL_Do_Dot ( cntx, word ) ;
}

#else

void
_CSL_Do_Dot ( Context * cntx, Word * prevWord ) // .
{
    if ( prevWord && ( ! cntx->BaseObject ) )
    {
        if ( prevWord->W_ObjectAttributes & CLASS_TYPE )
        {
            oPrintf ( "\nprevWord = %s", prevWord->Name ) ;
            Finder_SetQualifyingNamespace ( cntx->Finder0, prevWord ) ;
        }
        else if ( ( ! cntx->BaseObject->W_ObjectAttributes & CLASS_TYPE ) )
        {
            //oPrintf ( "\nprevWord = %s", prevWord->Name ) ;
            //Finder_SetQualifyingNamespace ( cntx->Finder0, prevWord ) ;
            //else 
            cntx->BaseObject = prevWord ;
        }
    }
}

void
CSL_Dot ( ) // .
{
    Context * cntx = _Context_ ;
    SetState ( cntx, CONTEXT_PARSING_QID, true ) ;
    //Word * prevWord = Compiler_PreviousNonDebugWord ( 0 ) ; // 0 : rem: we just popped the WordStack above
    //_CSL_Do_Dot ( cntx, prevWord ) ; // this is handled bye Word_ObjectRun/Do_Variable
}
#endif

void
_Word_CompileAndRecord_PushReg ( Word * word, int64 reg, Boolean recordFlag, int64 size )
{
    if ( word )
    {
        if ( recordFlag ) word->StackPushRegisterCode = Here ; // nb. used! by the rewriting optInfo
        if ( word->W_ObjectAttributes & REGISTER_VARIABLE ) _Compile_Stack_PushReg ( DSP, word->RegToUse, size ) ;
        else
        {

            _Compile_Stack_PushReg ( DSP, reg, size ) ;
            word->RegToUse = reg ;
        }
    }
}

void
Compiler_Save_C_BackgroundNamespace ( Compiler * compiler )
{
    compiler->C_BackgroundNamespace = _Namespace_FirstOnUsingList ( ) ; //nb! must be before CSL_LocalsAndStackVariablesBegin else CSL_End_C_Block will 
}

void
Compiler_SetAs_InNamespace_C_BackgroundNamespace ( Compiler * compiler )
{
    if ( compiler->C_BackgroundNamespace ) _CSL_Namespace_InNamespaceSet ( compiler->C_BackgroundNamespace ) ;
}

void
Compiler_Clear_Qid_BackgroundNamespace ( Compiler * compiler )
{
    compiler->Qid_BackgroundNamespace = 0 ;
}

void
Compiler_Save_Qid_BackgroundNamespace ( Compiler * compiler )
{
    //if ( ! compiler->Qid_BackgroundNamespace ) 
    compiler->Qid_BackgroundNamespace = _CSL_Namespace_InNamespaceGet ( ) ; // _Namespace_FirstOnUsingList ( ) ; 
}

void
Compiler_SetAs_InNamespace_Qid_BackgroundNamespace ( Compiler * compiler )
{
    if ( compiler->Qid_BackgroundNamespace ) _CSL_Namespace_InNamespaceSet ( compiler->Qid_BackgroundNamespace ) ;
}

