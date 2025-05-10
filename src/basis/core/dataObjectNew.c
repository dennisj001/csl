
#include "../../include/csl.h"

// Dynamic Object New : Word = Namespace = DObject : have a s_Symbol

// we run all new objects thru here ; good for debugging and understanding 

Word *
DataObject_New ( uint64 type, Word * word, byte * name, uint64 morphismAttributes, uint64 objectAttributes, uint64 lispAttributes,
    int64 index, uint64 value, Namespace * addToNs, int allocType, int64 tsrli, int64 scwi )
{
    Context * cntx = _Context_ ;
    switch ( type )
    {
        case T_LC_NEW:
        {
            word = _LO_New ( lispAttributes, morphismAttributes, objectAttributes, name, ( byte* ) value, word, ( allocType ? allocType : LISP_ALLOC ), 0, tsrli, scwi ) ; // all words are symbols
            break ;
        }
        case T_LC_DEFINE:
        {
            word = _LO_New ( lispAttributes, morphismAttributes, objectAttributes, name, ( byte* ) value, word, LISP, _LC_->LispDefinesNamespace, tsrli, scwi ) ; // all words are symbols
            break ;
        }
        case T_LC_LITERAL:
        {
            word = _LO_New_RawStringOrLiteral ( _Context_->Lexer0, name, value, index, tsrli, scwi ) ;
            break ;
        }
        case CSL_WORD:
        {
            word = _DObject_New ( name, value, morphismAttributes | BLOCK, objectAttributes, lispAttributes | BLOCK, BLOCK, 0, 0, 1, 0, DICTIONARY ) ;
            break ;
        }
        case NAMESPACE:
        {
            word = _Namespace_New ( name, addToNs ) ;
            break ;
        }
        case NAMESPACE_VARIABLE:
        {
            word = _CSL_Variable_New ( name, value, addToNs ) ;
            break ;
        }
        case LITERAL:
        {
            word = Literal_New ( _Context_->Lexer0, value ) ;
            break ;
        }
        case CONSTANT:
        {
            word = _DObject_New ( name, value, ( IMMEDIATE ), CONSTANT | objectAttributes, 0, CONSTANT, ( byte* ) _DataObject_Run, 0, 1, 0, DICTIONARY ) ;
            break ;
        }
        case OBJECT:
        {
            word = _Class_Object_New ( name, type ) ;
            break ;
        }
        case DOBJECT:
        {
            DObject_New ( ) ;
            break ;
        }
        case CLASS:
        {
            word = Class_New ( name, CLASS, 0 ) ;
            break ;
        }
        case TD_TYPE_FIELD:
        {
            word = CSL_ClassField_New ( name, addToNs, value, index ) ;
            break ;
        }
        case CLASS_CLONE:
        {
            word = Class_New ( name, CLASS_CLONE, 1 ) ;
            break ;
        }
        case C_CLASS:
        {
            word = Class_New ( name, C_CLASS, 0 ) ;
            break ;
        }
        case C_TYPE:
        {
            Compiler_Save_C_BackgroundNamespace ( cntx->Compiler0 ) ;
            word = Class_New ( name, C_TYPE, 0 ) ;
            Type_Create ( ) ;
            Compiler_SetAs_InNamespace_C_BackgroundNamespace ( cntx->Compiler0 ) ;
            break ;
        }
        case C_TYPEDEF:
        {
            _CSL_Typedef ( word ) ;
            break ;
        }
        case PARAMETER_VARIABLE: case LOCAL_VARIABLE: case (PARAMETER_VARIABLE | REGISTER_VARIABLE ): case (LOCAL_VARIABLE | REGISTER_VARIABLE ):
        {
            word = _Compiler_LocalWord_New ( _Compiler_, name, morphismAttributes, objectAttributes, lispAttributes, DICTIONARY ) ;
            //Word_SetTsrliScwi ( word, tsrli, scwi ) ;
            break ;
        }
        default: case (T_LISP_SYMBOL | PARAMETER_VARIABLE ): case (T_LISP_SYMBOL | LOCAL_VARIABLE ): // REGISTER_VARIABLE combinations with others in this case
        {
            word = Compiler_LocalWord_New ( _Compiler_, name, type, objectAttributes, lispAttributes, DICTIONARY ) ;
            //Word_SetTsrliScwi ( word, tsrli, scwi ) ;
            break ;
        }
    }
    Word_SetTsrliScwi ( word, tsrli, scwi ) ;
    return word ;
}

byte *
_CSL_NamelessObjectNew ( int64 size, int64 allocType )
{
    byte * obj = 0 ;
    if ( size ) obj = Mem_Allocate ( size, allocType ) ;
    return obj ;
}

Word *
_CSL_ObjectNew ( int64 size, byte * name, uint64 category, int64 allocType )
{
    byte * obj = _CSL_NamelessObjectNew ( size, allocType ) ;
    Word * word = _DObject_New ( name, ( int64 ) obj, ( IMMEDIATE | CPRIMITIVE ), ( OBJECT | category ), 0, OBJECT, ( byte* ) _DataObject_Run, 0, 1, 0, DICTIONARY ) ;
    word->CompiledDataFieldByteSize = size ;
    return word ;
}

void
_Class_Object_Init ( Word * word, Namespace * ins )
{
    Namespace * ns = ins ;
    Stack * nsstack = _Context_->Compiler0->InternalNamespacesStack ;
    int64 i ;
    Stack_Init ( nsstack ) ;
    // init needs to be done by the most super/internal class first successively down to the current/sub class 
    do
    {
        Word * initWord ;
        if ( ( initWord = _Finder_FindWord_InOneNamespace ( _Finder_, ns, ( byte* ) "init" ) ) )
        {
            _Stack_Push ( nsstack, ( int64 ) initWord ) ;
            initWord->W_TypeAttributes |= WT_INIT ;
        }
        ns = ns->ContainingNamespace ;
    }
    while ( ns ) ;
    for ( i = Stack_Depth ( nsstack ) ; i > 0 ; i -- )
    {
        Word * initWord = ( Word* ) _Stack_Pop ( nsstack ) ;
        DataStack_Push ( ( int64 ) word->W_Value ) ;
        _Context_->CurrentDisassemblyWord = initWord ;
        Word_Eval ( initWord ) ; // nb. must be Word_Eval (not Word_Run) because sigsetjmp is there and needed re. 'new' else active debugger will crash in/after 'new'
    }
    SetState ( _Debugger_, DEBUG_SHTL_OFF, false ) ;
    word->TypeNamespace = ins ;
}

// class object new

Word *
_Class_Object_New ( byte * name, uint64 category )
{
    int64 size ;
    byte * object ;
    Word * word ;
    CSL_PrintDataStack ( ) ;
    //Namespace * ns = Word_UnAlias ( _CSL_Namespace_InNamespaceGet ( ) ) ;
    Namespace * ns = _CSL_Namespace_InNamespaceGet ( ) ;
    size = _Namespace_VariableValueGet ( ns, ( byte* ) "size" ) ;
    word = _CSL_ObjectNew ( size, name, ( category | OBJECT ), CompileMode ? DICTIONARY : OBJECT_MEM ) ;
    //word = _CSL_ObjectNew ( size, name, ( category | OBJECT | O_POINTER ), CompileMode ? DICTIONARY : OBJECT_MEM ) ;
    object = ( byte* ) word->W_Value ;
    _Class_Object_Init ( word, ns ) ;
    _Namespace_VariableValueSet ( ns, ( byte* ) "this", ( int64 ) object ) ;
    CSL_PrintDataStack ( ) ;
    return word ;
}

Namespace *
Class_New ( byte * name, uint64 morphismFunctionType, int64 cloneFlag )
{
    Context * cntx = _Context_ ;
    if ( GetState ( cntx, C_SYNTAX ) ) Compiler_Save_C_BackgroundNamespace ( cntx->Compiler0 ) ;
    Namespace *sns, * ns ;
    int64 size ;
    byte * token ;
    //CSL_PrintDataStack () ;
    sns = _CSL_Namespace_InNamespaceGet ( ) ;
    ns = _Namespace_Find ( name, sns, 0 ) ;
    if ( ! ns )
    {
        if ( cloneFlag ) size = _Namespace_VariableValueGet ( sns, ( byte* ) "size" ) ;
        else size = 0 ;
        ns = _DObject_New ( name, 0, IMMEDIATE, CLASS | morphismFunctionType, 0, morphismFunctionType, ( byte* ) _DataObject_Run, -2, 0, sns, DICTIONARY ) ;
        //ns = _DObject_New ( name, 0, 0, CLASS | morphismFunctionType, 0, morphismFunctionType, ( byte* ) _DataObject_Run, -2, 0, sns, DICTIONARY ) ;
        Namespace_Do_Namespace ( ns ) ; // before "size", "this"
        Word *ws = _CSL_Variable_New ( ( byte* ) "size", size, 0 ) ; // start with size of the prototype for clone
        _Context_->Interpreter0->ThisNamespace = ns ;
        Word *wt = _CSL_Variable_New ( ( byte* ) "this", size, 0 ) ; // start with size of the prototype for clone
        wt->W_ObjectAttributes |= ( THIS ) ;
        token = Lexer_Peek_NextToken ( _Lexer_, 0, 1 ) ;
        if ( ( token[0] == '{' ) || ( token[1] == '{' ) || ( token[2] == '{' ) ) // consider "{", ":{" and +:{" tokens
        {
            if ( ! GetState ( _Context_, TDI_PARSING ) )
            {
                _TypeClassStructDef (ns, cloneFlag) ;
                ns->W_ObjectAttributes |= ( C_CLASS | STRUCT ) ; //( OBJECT ) ;
            }
        }
    }
    else if ( ns->CompiledDataFieldByteSize )
    {
        iPrintf ( "\nNamespace Error? : \'%s\' already exists! : \n  %s : size = %d : this namespace is %s.%s : size = %d : at %s\n",
            ns->Name, _Word_SourceCodeLocation_pbyte ( ns ), ns->CompiledDataFieldByteSize, ns->S_ContainingNamespace->Name, ns->Name, ns->CompiledDataFieldByteSize, Context_Location ( ) ) ;
        Namespace_Do_Namespace ( ns ) ;
    }
    CSL_WordList_Init ( 0 ) ;
    Compiler_SetAs_InNamespace_C_BackgroundNamespace ( cntx->Compiler0 ) ;
    //CSL_PrintDataStack () ;
    return ns ;
}

Word *
CSL_ClassField_New ( byte * token, Class * aclass, int64 size, int64 offset )
{
    Word * word = _DObject_New ( token, 0, ( IMMEDIATE | CPRIMITIVE ), OBJECT_FIELD, 0, OBJECT_FIELD, ( byte* ) _DataObject_Run, 0, 1, aclass, DICTIONARY ) ;
    word->TypeNamespace = aclass ;
    word->CompiledDataFieldByteSize = size ;
    if ( size == 1 ) word->W_ObjectAttributes |= T_BYTE ;
    else if ( size == 8 ) word->W_ObjectAttributes |= T_INT64 ;
    else if ( size == 4 ) word->W_ObjectAttributes |= T_INT32 ;
    else if ( size == 2 ) word->W_ObjectAttributes |= T_INT16 ;
    word->WD_Offset = offset ;
    return word ;
}

// this maybe should be in primitives/dobject.c

Word *
_CSL_Variable_New ( byte * name, int64 value, Namespace * addToNs )
{
    Word * word ;
    if ( CompileMode )
    {
        word = Compiler_LocalWord_New ( _Compiler_, name, 0, LOCAL_VARIABLE, 0, DICTIONARY ) ;
        SetState ( _Compiler_, VARIABLE_FRAME, true ) ;
    }
    //if ( addToNs ) word = _DObject_New ( name, value, ( IMMEDIATE ), NAMESPACE_VARIABLE, 0, NAMESPACE_VARIABLE, ( byte* ) _DataObject_Run, 0, 0, addToNs, DICTIONARY ) ;
    word = _DObject_New ( name, value, ( IMMEDIATE ), NAMESPACE_VARIABLE, 0, NAMESPACE_VARIABLE, ( byte* ) _DataObject_Run, 0, 1, addToNs, DICTIONARY ) ;
    word->CompiledDataFieldByteSize = 8 ;
    return word ;
}

byte *
_CSL_Label ( byte * lname )
{
    GotoInfo * gotoInfo = GotoInfo_NewAdd ( 0, 0, lname, GI_LABEL, 0 ) ;
    gotoInfo->LabeledAddress = Here ;
    Namespace * ns = Namespace_FindOrNew_SetUsing ( ( byte* ) "__labels__", _CSL_->Namespaces, 1 ) ;
    if ( _Finder_FindWord_InOneNamespace ( _Finder_, ns, lname ) )
    {
        byte bufferData [ 32 ] ;
        sprintf ( ( char* ) bufferData, "%s%d", lname, rand ( ) ) ;
        lname = bufferData ;
    }
    Word * word = _DObject_New ( lname, ( int64 ) gotoInfo, IMMEDIATE, CONSTANT, 0, CONSTANT, ( byte* ) _DataObject_Run, 0, 0, ns, COMPILER_TEMP ) ; //DICTIONARY ) ;

    return word->Name ;
}

Word *
Literal_New ( Lexer * lexer, uint64 uliteral )
{
    // nb.! : remember the compiler optimizes with the WordStack so everything has to be converted to a Word
    // _DObject_New : calls _Do_Literal which pushes the literal on the data stack or compiles a push ...
    Word * word ;
    byte _name [ 64 ], *name ;
    if ( ! ( lexer->L_ObjectAttributes & ( T_STRING | T_RAW_STRING | T_CHAR | T_INT | T_INT16 | T_INT32 | KNOWN_OBJECT ) ) )
    {
        snprintf ( ( char* ) _name, 64, "<unknown object type> : %lx", ( uint64 ) uliteral ) ;
        name = String_New ( _name, STRING_MEM ) ;
    }
    else
    {
        if ( lexer->L_ObjectAttributes & ( T_STRING | T_RAW_STRING ) )
        {
            uliteral = ( int64 ) String_New ( lexer->LiteralString, STRING_MEM ) ;
        }
        name = lexer->OriginalToken ;
    }
    word = _DObject_New ( name, uliteral, ( IMMEDIATE | lexer->L_MorphismAttributes ),
        ( LITERAL | CONSTANT | lexer->L_ObjectAttributes ), 0, LITERAL, ( byte* ) _DataObject_Run, 0, 0, 0, COMPILER_TEMP ) ; //( CompileMode ? INTERNAL_OBJECT_MEM : OBJECT_MEM ) ) ; // literals are compiled directly into machine code
    return word ;
}

