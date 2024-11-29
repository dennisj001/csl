
#include "../../include/csl.h"

void
_Interpreter_LC_InterpretWord ( Interpreter *interp, ListObject * l0 )
{
    Word * word = l0->Lo_CSL_Word ;
    byte c ;
    if ( ! word ) word = l0 ;
    if ( kbhit ( ) )
        Pause ( ) ;
    Interpreter_DoWord ( interp, word, word->W_RL_Index, word->W_SC_Index ) ;
}

void
_LO_CompileOrInterpret_One ( ListObject *l0, int64 functionFlag )
{
    // just interpret the non-nil, non-list objects
    // nil means that it doesn't need to be interpreted any more
    if ( l0 && ( ! ( l0->W_LispAttributes & ( LIST | LIST_NODE | T_NIL ) ) ) ) _Interpreter_LC_InterpretWord ( _Interpreter_, l0 ) ;
}

void
LO_CompileOrInterpretArgs ( ListObject * largs )
{
    ListObject * arg ;
    for ( arg = LO_First ( largs ) ; arg ; arg = LO_Next ( arg ) )
    {
        if ( GetState ( _O_->OVT_LC, LC_INTERP_DONE ) ) return ; // for LO_CSL
        _LO_CompileOrInterpret_One ( arg, 0 ) ; // research : how does CAttribute get set to T_NIL?
    }
}

void
_LO_CompileOrInterpret ( ListObject *lfunction, ListObject * largs )
{
    ListObject *lfword = lfunction->Lo_CSL_Word ;
    if ( largs && lfword && ( lfword->W_MorphismAttributes & ( CATEGORY_OP_ORDERED | CATEGORY_OP_UNORDERED ) ) ) // ?!!? 2 arg op with multi-args : this is not a precise distinction yet : need more types ?!!?
    {
        Boolean svTcs = GetState ( _CSL_, TYPECHECK_ON ) ; // sometimes ok but for now off here
        SetState ( _CSL_, TYPECHECK_ON, false ) ; // sometimes ok but for now off here
        _LO_CompileOrInterpret_One ( largs, 0 ) ;
        while ( ( largs = LO_Next ( largs ) ) )
        {
            _LO_CompileOrInterpret_One ( largs, 0 ) ; // two args first then op, then after each arg the operator : nb. assumes word can take unlimited args 2 at a time
            _LO_CompileOrInterpret_One ( lfword, 1 ) ;
        }
        SetState ( _CSL_, TYPECHECK_ON, svTcs ) ;
    }
    else
    {
        LO_CompileOrInterpretArgs ( largs ) ;
        if ( lfword && ( ! ( lfword->W_ObjectAttributes & T_LISP_CSL ) ) ) _LO_CompileOrInterpret_One ( lfword, 1 ) ;
    }
    _O_->OVT_LC->LastInterpretedWord = lfword ;
}

ListObject *
_LO_Do_FunctionBlock ( ListObject *lfunction, ListObject * largs )
{
    LambdaCalculus *lc = _O_->OVT_LC ;
    ListObject *vReturn = nil, *lfargs = LO_First ( largs ) ;
    _LO_CompileOrInterpret ( lfunction, lfargs ) ;
    lc->ParenLevel -- ;
    // this is necessary in "lisp" mode : eg. if user hits return but needs to be clarified, refactored, maybe renamed, etc.
    if ( ! GetState ( lc, LC_INTERP_DONE ) )
    {
        LO_CheckEndBlock ( ) ;
        _Compiler_->ReturnVariableWord = lfunction ;
        vReturn = LO_PrepareReturnObject ( ) ;
    }
    //else vReturn = nil ;
    return vReturn ;
}

void
LC_Substitute ( )
{
    LambdaCalculus * lc = _LC_ ;
    ListObject *funcParameters = lc->FunctionParameters, * funcArgs = lc->FunctionArgs ;
    LC_Debug ( lc, LC_SUBSTITUTE, 0 ) ;
    while ( funcParameters && funcArgs )
    {
        // ?!? this may not be the right idea but we want it so that we can have transparent lists in the parameters, ie. 
        // no affect with a parenthesized list or just unparaenthesized parameters of the same number
        if ( funcParameters->W_LispAttributes & ( LIST | LIST_NODE ) )
        {
            funcParameters = LO_First ( funcParameters ) ; // can something like this work
            if ( funcArgs->W_LispAttributes & ( LIST | LIST_NODE ) ) funcArgs = LO_First ( funcArgs ) ;
            //else Error ( "\nLO_Substitute : funcCallValues list structure doesn't match parameter list", QUIT ) ;
        }
        else if ( funcArgs->W_LispAttributes & ( LIST | LIST_NODE ) )
        {
            funcArgs = LO_First ( funcArgs ) ;
            if ( funcParameters->W_LispAttributes & ( LIST | LIST_NODE ) ) funcParameters = LO_First ( funcParameters ) ; // can something like this work
            //else Error ( "\nLO_Substitute : funcCallValues list structure doesn't match parameter list", QUIT ) ;
        }
        // just preserve the name of the arg for the finder
        // so we now have the call values with the parameter names - parameter names are unchanged 
        // so when we eval/print these parameter names they will have the function calling values -- lambda calculus substitution - beta reduction
        funcArgs->Lo_Name = funcParameters->Lo_Name ;
        funcParameters = LO_Next ( funcParameters ) ;
        funcArgs = LO_Next ( funcArgs ) ;
    }
    LC_Debug ( lc, LC_SUBSTITUTE, 1 ) ;
}

ListObject *
LO_PrepareReturnObject ( )
{
    uint64 type = 0 ;
    int64 value ;
    ListObject * l0 = nil ;
    if ( ! CompileMode )
    {
        if ( Namespace_IsUsing ( ( byte* ) "BigNum" ) ) type = T_BIG_NUM ;
        value = DataStack_Pop ( ) ;
        l0 = DataObject_New ( T_LC_NEW, 0, 0, 0, LITERAL | type, LITERAL | type, 0, value, 0, 0, 0, - 1 ) ;
    }
    return l0 ;
}

void
LO_BeginBlock ( )
{
    if ( ! Compiler_BlockLevel ( _Compiler_ ) ) _LC_->SavedCodeSpace = _O_CodeByteArray ;
    CSL_BeginBlock ( ) ;
    //if ( Is_DebugOn ) Stack_Print ( _Compiler_->CombinatorBlockInfoStack, c_d ( "compiler->CombinatorBlockInfoStack" ), 0 ) ;
}

void
LO_EndBlock ( )
{
    Compiler * compiler = _Context_->Compiler0 ;
    if ( _LC_ && _LC_->SavedCodeSpace )
    {
        BlockInfo * bi = ( BlockInfo * ) Stack_Top ( compiler->BlockStack ) ;
        CSL_EndBlock ( ) ;
        if ( ! LC_CompileMode )
        {
            CSL_BlockRun ( ) ;
        }
        if ( ! Compiler_BlockLevel ( compiler ) ) Set_CompilerSpace ( _LC_->SavedCodeSpace ) ;
        bi->LogicCodeWord = _LC_->LastInterpretedWord ;
    }
    //if ( Is_DebugOn ) Stack_Print ( _Compiler_->CombinatorBlockInfoStack, c_d ( "compiler->CombinatorBlockInfoStack" ), 0 ) ;
}

void
LO_CheckEndBlock ( )
{
    if ( CompileMode )
    {
        Compiler * compiler = _Context_->Compiler0 ;
        if ( GetState ( compiler, LC_COMBINATOR_MODE ) )
        {
            LambdaCalculus * lc = _LC_ ;
            int64 sdcifs = Stack_Depth ( lc->CombinatorInfoStack ) ;
            if ( sdcifs )
            {
                int64 cii = Stack_Top ( lc->CombinatorInfoStack ) ;
                CombinatorInfo ci ; // remember sizeof of CombinatorInfo = 4 bytes
                ci.CI_i32_Info = cii ;
                if ( ( lc->ParenLevel == ci.ParenLevel ) && ( Compiler_BlockLevel ( compiler ) > ci.BlockLevel ) )
                {
                    LO_EndBlock ( ) ;
                }
            }
        }
    }
}

int64
_LO_CheckBeginBlock ( )
{
    if ( CompileMode )
    {
        LambdaCalculus * lc = _LC_ ;
        Compiler * compiler = _Context_->Compiler0 ;
        int64 sdcifs = Stack_Depth ( lc->CombinatorInfoStack ) ;
        if ( sdcifs )
        {
            int64 cii = Stack_Top ( lc->CombinatorInfoStack ) ;
            CombinatorInfo ci ; // remember sizeof of CombinatorInfo = 4 bytes
            ci.CI_i32_Info = cii ;
            if ( ( lc->ParenLevel == ci.ParenLevel ) && ( Compiler_BlockLevel ( compiler ) == ci.BlockLevel ) )
            {
                return true ;
            }
        }
    }
    return false ;
}

int64
LO_CheckBeginBlock ( )
{
    if ( _LO_CheckBeginBlock ( ) )
    {
        LO_BeginBlock ( ) ;
        return true ;
    }
    return false ;
}

void
LC_InitForCombinator ( LambdaCalculus * lc )
{
    Compiler *compiler = _Context_->Compiler0 ;
    SetState ( compiler, LC_COMBINATOR_MODE, true ) ;
    CombinatorInfo ci ; // remember sizeof of CombinatorInfo = 4 bytes
    ci.BlockLevel = Compiler_BlockLevel ( compiler ) ; //compiler->BlockLevel ;
    ci.ParenLevel = lc->ParenLevel ;
    _Stack_Push ( lc->CombinatorInfoStack, ( int64 ) ci.CI_i32_Info ) ; // this stack idea works because we can only be in one combinator at a time
}

#if 0

void
Arrays_DoArrayArgs_Lisp ( Word ** pl1, Word * l1, Word * arrayBaseObject, int64 objSize, Boolean saveCompileMode, Boolean * variableFlag )
{
    do
    {
        if ( Do_NextArrayToken ( l1, l1->Name, arrayBaseObject, objSize, saveCompileMode, variableFlag ) ) break ;
    }
    while ( l1 = LO_Next ( l1 ) ) ;
    *pl1 = l1 ;
}
#else

void
Arrays_DoArrayArgs_Lisp ( Word ** pl1, Word * l1, Word * arrayBaseObject, int64 objSize, Boolean saveCompileMode, Boolean * variableFlag )
{
    byte * token ;
    while ( l1 = LO_Next ( l1 ) )
    {
        token = l1->Name ;
        if ( Do_NextArrayToken ( l1, token, arrayBaseObject, objSize, saveCompileMode, variableFlag ) ) break ;
    }
    *pl1 = l1 ;
}
#endif

void
_LO_Apply_ArrayArg ( ListObject ** pl1, int64 * i )
{
    if ( ! _LC_->ArrayBaseObject ) _LC_->ArrayBaseObject = ( ( Word * ) ( LO_Previous ( *pl1 ) ) )->Lo_CSL_Word ;
    if ( ! _LC_->BaseObject ) _LC_->BaseObject = _Context_->BaseObject ;
    _CSL_ArrayBegin ( 1, pl1, i ) ;
}

// compile mode on

void
_LO_Apply_NonMorphismArg ( ListObject ** pl1, int64 * i )
{
    Context * cntx = _Context_ ;
    //Lexer * lexer = cntx->Lexer0 ;
    ListObject *l1 = * pl1 ;
    Word * word = l1->Lo_CSL_Word ;
    byte * here = Here ;
    word = Compiler_CopyDuplicatesAndPush ( word, l1->W_RL_Index, l1->W_SC_Index ) ;
    Word_Eval ( word ) ;
    Word *baseObject = _Context_->BaseObject ;
    if ( ( word->W_ObjectAttributes & OBJECT_TYPE ) )
    {
        if ( ( word->Name [0] == '\"' ) || ( ! ReadLiner_IsTokenForwardDotted ( _ReadLiner_, word->W_RL_Index ) ) ) //( ! _Lexer_IsTokenForwardDotted ( cntx->Lexer0, l1->W_RL_Index + Strlen ( word->Name ) - 1 ) ) ) // ( word->Name[0] == '\"' ) : sometimes strings have ".[]" chars within but are still just strings
        {
            //Boolean regOrder [] = { RDI, RSI, RDX, RCX, R8D, R9D, R10D, R11D } ;
            byte reg = LispRegParameterOrder ( ( *i ) ++ ) ;
            if ( word->StackPushRegisterCode ) SetHere ( word->StackPushRegisterCode ) ;
            else if ( baseObject && baseObject->StackPushRegisterCode ) SetHere ( baseObject->StackPushRegisterCode ) ;
            Compile_Move_Reg_To_Reg ( reg, ACC, 0 ) ;
            if ( baseObject ) _Debugger_->SpecialPreHere = baseObject->Coding ;
            else _Debugger_->SpecialPreHere = here ;
        }
    }
}

void
_LC_Apply_Arg ( ListObject ** pl1, int64 * i )
{
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    ListObject *l1 = * pl1, * l2 ;
    Word * word = l1 ;

    lexer->TokenStart_ReadLineIndex = l1->W_RL_Index ; // nb : needed eg. to correctly check is forward dotted to set a QualifyingNamespace ; it is reset at ReadToken
    Set_CompileMode ( true ) ;
    if ( l1->W_LispAttributes & ( LIST | LIST_NODE ) )
    {
        Set_CompileMode ( false ) ;
        l2 = LC_Eval ( l1, 0, 1 ) ;
        _Debugger_->SpecialPreHere = Here ;
        Compiler_Word_SCHCPUSCA ( l2, 0 ) ;
        if ( ! l2 || ( l2->W_LispAttributes & T_NIL ) ) Compile_MoveImm_To_Reg ( LispRegParameterOrder ( ( *i ) ++ ), DataStack_Pop ( ), CELL_SIZE ) ;
        else Compile_MoveImm_To_Reg ( LispRegParameterOrder ( ( *i ) ++ ), ( int64 ) * l2->Lo_PtrToValue, CELL_SIZE ) ;
        DEBUG_SHOW ( l2, 1, 0 ) ;
    }
    else if ( ( l1->W_ObjectAttributes & NON_MORPHISM_TYPE ) ) _LO_Apply_NonMorphismArg ( pl1, i ) ;
        //else if ( ( l1->W_ObjectAttributes & OBJECT_TYPE ) ) _LO_Apply_NonMorphismArg ( pl1, i ) ;
    else if ( ( l1->Name [0] == '.' ) || ( l1->Name [0] == '&' ) || String_Equal ( l1->Name, "->" ) )
        Interpreter_DoWord ( cntx->Interpreter0, l1->Lo_CSL_Word, l1->W_RL_Index, l1->W_SC_Index ) ;
    else if ( ( l1->Name[0] == '[' ) ) _LO_Apply_ArrayArg ( pl1, i ) ;
    else
    {
        word = Compiler_CopyDuplicatesAndPush ( word, l1->W_RL_Index, l1->W_SC_Index ) ;
        DEBUG_SETUP ( word, 0 ) ;
        _Compile_Move_StackN_To_Reg ( LispRegParameterOrder ( ( *i ) ++ ), 0, DSP, 0 ) ;
        DEBUG_SHOW ( word, 1, 0 ) ;
    }
done:
    _Debugger_->PreHere = Here ;
}
// for calling 'C' functions such as printf or other system functions
// where the arguments are pushed first from the end of the list like 'C' arguments
// this is a little confusing : the args are LO_Read left to Right for C we want them right to left except qid word which remain left to right

ListObject *
_LC_Apply_C_LtoR_ArgList ( LambdaCalculus * lc, ListObject * l0, Word * word )
{
    Context * cntx = _Context_ ;
    ListObject *l1 ;
    ByteArray * scs = _O_CodeByteArray ;
    Compiler * compiler = cntx->Compiler0 ;
    int64 i, svcm = CompileMode ;

    d0 ( if ( Is_DebugModeOn ) LO_Debug_ExtraShow ( 0, 2, 0, ( byte* ) "\nEntering _LO_Apply_ArgList..." ) ) ;
    if ( l0 )
    {
        Set_CompileMode ( true ) ;
        if ( ! svcm ) CSL_BeginBlock ( ) ;
        if ( word->W_MorphismAttributes & ( DLSYM_WORD | C_PREFIX ) ) Set_CompileMode ( true ) ;
        //_Debugger_->PreHere = Here ;
        for ( i = 0, l1 = LO_First ( l0 ) ; l1 ; l1 = LO_Next ( l1 ) ) _LC_Apply_Arg ( &l1, &i ) ;
        Set_CompileMode ( true ) ;
        _Debugger_->SpecialPreHere = Here ;
        //System V ABI : "%rax is used to indicate the number of vector arguments passed to a function requiring a variable number of arguments"
        if ( ( String_Equal ( word->Name, "printf" ) || ( String_Equal ( word->Name, "sprintf" ) ) ) ) Compile_MoveImm_To_Reg ( RAX, i, CELL ) ;
        word = Compiler_CopyDuplicatesAndPush ( word, word->W_RL_Index, word->W_SC_Index ) ;
        Word_Eval ( word ) ;
        if ( word->W_MorphismAttributes & RAX_RETURN ) _Word_CompileAndRecord_PushReg ( word, ACC, true, 0 ) ;
        if ( ! svcm )
        {
            CSL_EndBlock ( ) ;
            block b = ( block ) DataStack_Pop ( ) ;
            Set_CompileMode ( svcm ) ;
            Set_CompilerSpace ( scs ) ;
            Word_DbgBlock_Eval ( word, b ) ;
        }
        d0 ( if ( Is_DebugModeOn ) LO_Debug_ExtraShow ( 0, 2, 0, ( byte* ) "\nLeaving _LO_Apply_ArgList..." ) ) ;
        SetState ( compiler, LC_ARG_PARSING, false ) ;
    }
    return nil ;
}

Word *
Word_CompileRun_C_ArgList ( Word * word ) // C protocol - x64 : left to right arguments put into registers 
{
    Namespace * backgroundNamespace = _CSL_Namespace_InNamespaceGet ( ) ;
    LambdaCalculus * lc = LC_Init_Runtime ( ) ;
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    Compiler * compiler = cntx->Compiler0 ;
    byte *svDelimiters = lexer->TokenDelimiters, csyntax ;
    ListObject * l0 ;

    byte * token = _Lexer_ReadToken ( lexer, ( byte* ) " ,;\n\r\t" ) ;
    if ( word->W_MorphismAttributes & ( C_PREFIX | C_PREFIX_RTL_ARGS ) )
    {
        if ( ( ! token ) || strcmp ( "(", ( char* ) token ) ) Error ( " : Syntax error : C RTL Args : no '('", ABORT ) ; // should be '('
        csyntax = C_SyntaxOn ;
        if ( ! csyntax ) Namespace_ActivateAsPrimary ( "C_Syntax" ), SetState ( cntx, C_SYNTAX, true ) ;
    }
    lc->ParenLevel = 1 ;
    if ( word->W_MorphismAttributes & ( C_PREFIX | C_PREFIX_RTL_ARGS ) )
    {
        if ( ! Compiling ) Compiler_Init ( compiler, 0 ) ;
        SetState ( compiler, LC_ARG_PARSING, true ) ;
        int64 svcm = CompileMode ;
        Set_CompileMode ( false ) ; // we must have the arguments pushed and not compiled for _LO_Apply_C_Rtl_ArgList which will compile them for a C_Rtl function
        int64 svDs = GetState ( _CSL_, _DEBUG_SHOW_ ) ;
        DebugShow_Off ;
        cntx->BaseObject = 0 ; // nb! very important !! // but maybe shouldn't be done here -> Context_DoDotted_Post
        l0 = LC_Read ( ) ;
        SetState ( _CSL_, _DEBUG_SHOW_, svDs ) ;
        Set_CompileMode ( svcm ) ; // we must have the arguments pushed and not compiled for _LO_Apply_C_Rtl_ArgList which will compile them for a C_Rtl function
        _LC_Apply_C_LtoR_ArgList ( lc, l0, word ) ;
        LC_LispNamespacesOff ( ) ;
        SetState ( compiler, LC_ARG_PARSING | LC_C_RTL_ARG_PARSING, false ) ;
    }
    if ( word->W_MorphismAttributes & ( C_PREFIX | C_PREFIX_RTL_ARGS ) )
    {
        if ( ! csyntax ) Namespace_RemoveFromUsingList ( "C_Syntax" ), SetState ( cntx, C_SYNTAX, false ) ;
    }
    _CSL_Namespace_InNamespaceSet ( backgroundNamespace ) ;
    Lexer_SetTokenDelimiters ( lexer, svDelimiters, COMPILER_TEMP ) ;
    LC_RestoreStackPointer ( lc ) ;
    return word ;
}

// assumes list contains only one application 

block
CompileLispBlock ( ListObject *args, ListObject * body )
{
    LambdaCalculus * lc = _LC_ ;
    block code ;
    byte * here = Here ;
    Word * word = _Context_->CurrentWordBeingCompiled ;
    LO_BeginBlock ( ) ; // must have a block before local variables if there are register variables because _CSL_Parse_LocalsAndStackVariables will compile something
    SetState ( lc, ( LC_COMPILE_MODE | LC_BLOCK_COMPILE ), true ) ; // before _CSL_Parse_LocalsAndStackVariables
    Namespace * locals = _CSL_Parse_LocalsAndStackVariables ( 1, 1, args, 0, 0 ) ;
    word->W_MorphismAttributes = BLOCK ;
    word->W_LispAttributes |= T_LISP_COMPILED_WORD ;
    LC_Eval ( body, locals, 1 ) ;
    if ( _LC_CompileMode ( lc ) )
    {
        LO_EndBlock ( ) ;
        code = ( block ) DataStack_Pop ( ) ;
    }
    //else // nb. LISP_COMPILE_MODE : this state can change with some functions that can't be compiled yet
    if ( ( ! code ) || ( ! GetState ( lc, LC_COMPILE_MODE ) ) )
    {
        SetHere ( here ) ; //recover the unused code space
        code = 0 ;
        word->W_LispAttributes &= ~ T_LISP_COMPILED_WORD ;
        if ( Verbosity ( ) > 1 )
        {
            AlertColors ;
            iPrintf ( "\nLisp can not compile this word yet : %s : -- interpreting ...\n ", _Word_SourceCodeLocation_pbyte ( word ) ) ;
            DefaultColors ;
        }
    }
    _Word_DefinitionStore ( word, ( block ) code ) ; // not _Word_InitFinal because this is already CSL->CurrentWordCompiling with W_OriginalCodeText, etc.
    SetState ( lc, ( LC_BLOCK_COMPILE | LC_COMPILE_MODE ), false ) ; // necessary !!
    return code ;
}

