
#include "../../include/csl.h"

/* from macros.h
#define dobject_Get_M_Slot( dobj, m ) (((dobject*) dobj)->do_iData [m]) 
#define dobject_Set_M_Slot( dobj, m, value ) (((dobject*) dobj)->do_iData [m] = ((int64)value) ) 
#define List_Set_N_Node_M_Slot( list, n, m, value ) _dllist_Set_N_Node_M_Slot ( list, 0, 0, value ) 
#define List_Get_N_Node_M_Slot( list, n, m ) _dllist_Get_N_Node_M_Slot ( (dllist * )list, (int64) n, (int64) m )
 * from dllist.c
int64 _dllist_Get_N_Node_M_Slot ( dllist * list, int64 n, int64 m )
void _dllist_Set_N_Node_M_Slot ( dllist * list, int64 n, int64 m, int64 value )
 */

byte *
_object_Allocate ( int64 size, int64 allocType )
{
    return Mem_Allocate ( size, allocType ) ;
}

dobject *
dobject_Allocate ( int64 doType, int64 slots, uint64 allocType )
{
    int64 size = sizeof ( dobject ) + ( slots * sizeof ( int64 ) ) ;
    dobject * dobj = ( dobject * ) _object_Allocate ( size, allocType ) ;
    dobj->do_iData = ( int64* ) ( ( dobject* ) dobj + 1 ) ;
    dobj->do_Slots = ( Boolean ) slots ;
    dobj->do_Size = ( int16 ) size ;
    dobj->do_Type = ( int32 ) doType ;
    dobj->do_InUseFlag = ( Boolean ) true ;
    return dobj ;
}

dobject *
dobject_New_M_Slot ( int64 allocType, int64 typeCode, int64 m_slots, ... )
{
    int64 i, value ;
    va_list args ;
    va_start ( args, m_slots ) ;
    dobject * dobj = dobject_Allocate ( typeCode, m_slots, allocType ) ;
    for ( i = 0 ; i < m_slots ; i ++ )
    {
        value = va_arg ( args, int64 ) ;
        dobj->do_iData[i] = value ;
    }
    va_end ( args ) ;
    return dobj ;
}

void
_dobject_Print ( dobject * dobj )
{
    int64 i ;
    iPrintf ( "\n\ndobject  = 0x%08x : word Name = %s", dobj, dobj->do_iData[2] ? ( ( Word* ) dobj->do_iData[2] )->Name : ( byte* ) "" ) ;
    iPrintf ( "\nType     = %d", dobj->do_Type ) ;
    iPrintf ( "\nSlots    = %d", dobj->do_Slots ) ;
    iPrintf ( "\nSize     = %d", dobj->do_Size ) ;
    for ( i = 0 ; i < dobj->do_Slots ; i ++ )
    {
        iPrintf ( "\nSlot [%d] = 0x%08x", i, dobj->do_iData[i] ) ;
    }
    //_Printf ( "\n" ) ;
}
#if 0
void
DoNamespace ( Word * word )
{
    if ( Compiling ) 
    {
        if ( ! C_SyntaxOn ) 
        {
            if ( ReadLiner_IsTokenForwardDotted ( _ReadLiner_, word->W_RL_Index ) )
                _DataObject_Run ( word ) ; // at compile time
            else Compile_CallCFunctionWithParameter_TestAlignRSP2 ( ( byte* ) _DataObject_Run, word ) ; 
            //else Compile_CallCFunctionWithParameter_TestAlignRSP2 ( ( byte* ) _DataObject_Run, word ) ; 
        }
        else _DataObject_Run ( word ) ; //_DataObject_Run ( word ) ;
    }
    else _DataObject_Run ( word ) ; //_DataObject_Run ( word ) ; //( ( void ( * ) ( Word * ) )( function ) ) ( word ) ;
}
#else
void
DoNamespace ( Word * word )
{
    if ( Compiling ) 
    {
        if ( ! C_SyntaxOn ) 
        {
            if ( ReadLiner_IsTokenForwardDotted ( _ReadLiner_, word->W_RL_Index ) )
                Namespace_Do_Namespace ( word ) ; // at compile time
            else Compile_CallCFunctionWithParameter_TestAlignRSP2 ( ( byte* ) Namespace_Do_Namespace, word ) ; 
            //else Compile_CallCFunctionWithParameter_TestAlignRSP2 ( ( byte* ) _DataObject_Run, word ) ; 
        }
        else _DataObject_Run ( word ) ;
    }
    else _DataObject_Run ( word ) ; //( ( void ( * ) ( Word * ) )( function ) ) ( word ) ;
}
#endif
// remember : Word = DynamicObject = DObject = Namespace

void
_DObject_C_StartupCompiledWords_DefInit ( byte * function, int64 arg, Word * word )
{
    if ( arg == - 1 )
    {
        ( ( void ( * )( ) )( function ) ) ( ) ;
    }
#if 1    
    else if ( arg == - 2 ) // namespace
    {
        Compile_CallCFunctionWithParameter_TestAlignRSP2 ( ( byte* ) DoNamespace, word ) ;
    }
#endif    
    else
    {
        ( ( void ( * ) ( int64 ) )( function ) ) ( arg ) ;
    }
}

DObject *
_CSL_Do_DynamicObject_ToReg ( DObject * dobject0, uint8 reg )
{
    Context * cntx = _Context_ ;
    Lexer * lexer = cntx->Lexer0 ;
    DObject *dobject = dobject0, * ndobject ;
    byte * token ;
    //Compiler_Word_SCHCPUSCA ( dobject0, 0 ) ;
    while ( Lexer_IsTokenForwardDotted ( lexer ) )
    {
        Lexer_ReadToken ( lexer ) ; // the '.'
        token = Lexer_ReadToken ( lexer ) ;
        if ( String_Equal ( "prototype", ( char* ) token ) )
        {
            dobject = dobject->ContainingNamespace ;
            continue ;
        }
        if ( ! ( ndobject = _DObject_FindSlot_BottomUp ( dobject, token ) ) )
        {
            dobject = _DObject_NewSlot ( dobject, token, 0 ) ;
        }
        else dobject = ndobject ;
    }
    Compiler_Word_SCHCPUSCA ( dobject0, 0 ) ;
    if ( CompileMode ) _Compile_Move_Literal_Immediate_To_Reg ( reg, ( int64 ) & dobject->W_Value, 0 ) ;
    cntx->Interpreter0->CurrentObjectNamespace = TypeNamespace_Get ( dobject ) ; 

    return dobject ;
}

void
CSL_Do_DynamicObject ( DObject * dobject0, Boolean reg )
{
    DObject * dobject = _CSL_Do_DynamicObject_ToReg ( dobject0, reg ) ;
    if ( CompileMode ) _Word_CompileAndRecord_PushReg (dobject0, reg, true , 0) ;
    else DataStack_Push (( int64 ) & dobject->W_Value) ; 
    //CSL_TypeStackPush ( dobject ) ;
}

void
_DObject_ValueDefinition_Init ( Word * word, uint64 value, uint64 ftype, byte * function, int64 arg )
// using a variable that is a type or a function 
{
    word->W_PtrToValue = & word->W_Value ; // lvalue
    if ( ftype & BLOCK )
    {
        word->Definition = ( block ) ( function ? function : ( byte* ) value ) ; 
        word->CodeStart = ( byte* ) word->Definition ;
        if ( NamedByteArray_CheckAddress ( _O_CodeSpace, word->CodeStart ) ) word->S_CodeSize = Here - word->CodeStart ; 
        else word->S_CodeSize = 0 ;
        word->W_Value = ( uint64 ) word->Definition ; // rvalue
    }
    else
    {
        word->W_Value = value ; // rvalue
        d0 ( iPrintf ( "\n_DObject_ValueDefinition_Init :" ) ) ;
        ByteArray * svcs = _O_CodeByteArray ;
        int64 sscm = GetState ( _CSL_, DEBUG_SOURCE_CODE_MODE ) ;
        _NBA_SetCompilingSpace_MakeSureOfRoom ( _O_->InternalObjectSpace, 1 * K ) ; 
        Word_SetCoding ( word, Here ) ;
        word->CodeStart = Here ;
        word->Definition = ( block ) Here ;
        if ( arg ) _DObject_C_StartupCompiledWords_DefInit ( function, arg, word ) ;
        else Compile_CallCFunctionWithParameter_TestAlignRSP2 ( ( byte* ) _DataObject_Run, word ) ;
        _Compile_Return ( ) ;
        //if ( Is_DebugOn ) Word_Disassemble ( word ) ; //_Debugger_Disassemble ( _Debugger_, ( byte* ) word->Definition, 64, 1 ) ;
        word->S_CodeSize = Here - word->CodeStart ; // for use by inline
        Set_CompilerSpace ( svcs ) ;
        SetState ( _CSL_, DEBUG_SOURCE_CODE_MODE, sscm ) ;
    }
}

void
DObject_Finish ( Word * word )
{
    Context * cntx = _Context_ ;
    Compiler * compiler = cntx->Compiler0 ;
    if ( ! ( word->W_MorphismAttributes & CPRIMITIVE ) )
    {
        if ( GetState ( _CSL_, CO_ON ) ) SetState ( word, COMPILED_OPTIMIZED, true ) ;
        if ( GetState ( _CSL_, INLINE_ON ) ) SetState ( word, COMPILED_INLINE, true ) ;
        if ( GetState ( _CSL_, JCC8_ON ) ) SetState ( word, W_JCC8_ON, true ) ;
        if ( GetState ( _Context_, INFIX_MODE ) ) SetState ( word, W_INFIX_MODE, true ) ;
        if ( GetState ( _Context_, C_SYNTAX ) )
        {
            SetState ( word, W_C_SYNTAX, true ) ;
            word->W_TypeAttributes |= WT_C_SYNTAX ;
        }
        if ( IsSourceCodeOn ) SetState ( word, W_SOURCE_CODE_MODE, true ) ;
    }
    word->W_NumberOfNonRegisterArgs = compiler->NumberOfNonRegisterArgs ;
    word->W_NumberOfNonRegisterLocals = compiler->NumberOfNonRegisterLocals ;
    word->W_NumberOfVariables = compiler->NumberOfVariables ;
    if ( GetState ( _Context_, INFIX_MODE ) ) word->W_MorphismAttributes |= INFIX_WORD ;
    _CSL_->LastFinished_DObject = word ;
    _CSL_SetSourceCodeWord ( word ) ;
}

Word *
_DObject_Init ( Word * word, uint64 value, uint64 ftype, byte * function, int64 arg )
{
    // remember : Word = Namespace = DObject : each have an s_Symbol
    _DObject_ValueDefinition_Init ( word, value, ftype, function, arg ) ;
    DObject_Finish ( word ) ; // don't need to fully finish here ??
    word->RunType = ftype ;
    return word ;
}

// DObject : dynamic object
// remember : Word = Namespace = DObject has a s_Symbol

Word *
_DObject_New ( byte * name, int64 value, uint64 morphismType, uint64 objectType, uint64 lispType, uint64 functionType, byte * function, int64 arg,
    int64 addToInNs, Namespace * addToNs, uint64 allocType )
{
    //if ( String_Equal ( name, "n_After" ) ) iPrintf ( "" ) ;
    Word * word = _Word_New ( name, morphismType, objectType, lispType, addToInNs, addToNs, allocType ) ;
    _DObject_Init ( word, value, functionType, function, arg ) ;
    _CSL_->DObjectCreateCount ++ ;
    return word ;
}

DObject *
_DObject_FindSlot_BottomUp ( DObject * dobject, byte * name )
{
    Word * word ;
    do
    {
        if ( ( word = _Finder_FindWord_InOneNamespace ( _Finder_, dobject, name ) ) ) break ;
        dobject = dobject->ContainingNamespace ;
    }
    while ( dobject ) ;

    return ( DObject * ) word ;
}

DObject *
_DObject_SetSlot ( DObject * dobject, byte * name, int64 value )
{
    DObject * ndobject = _DObject_FindSlot_BottomUp ( dobject, name ) ;
    if ( ! ndobject ) return _DObject_NewSlot ( dobject, name, value ) ;

    else return ndobject ;
}

void
DObject_SubObjectInit ( DObject * dobject, Word * parent )
{
    if ( ! parent ) parent = _CSL_Namespace_InNamespaceGet ( ) ;
    else if ( ! ( parent->W_ObjectAttributes & NAMESPACE ) )
    {
        parent->W_List = dllist_New ( ) ;
        parent->W_ObjectAttributes |= DOBJECT_FIELD | NAMESPACE ;
        _Namespace_AddToNamespacesTail ( parent ) ;
    }
    Namespace_DoAddWord ( parent, dobject ) ;
    dobject->W_MorphismAttributes |= parent->W_MorphismAttributes ;
    dobject->W_ObjectAttributes |= T_ANY ;
    dobject->S_NumberOfSlots = parent->S_NumberOfSlots ;
    Namespace_SetState_AdjustListPosition ( parent, USING, 1 ) ;
}

DObject *
DObject_Sub_New ( DObject * proto, byte * name, uint64 objectAttributes )
{
    DObject * dobject = _DObject_New ( name, 0, IMMEDIATE, objectAttributes | DOBJECT | T_ANY, 0, DOBJECT, ( byte* ) _DataObject_Run, 0, 0, 0, DICTIONARY ) ;
    DObject_SubObjectInit ( dobject, proto ) ;
    return dobject ;
}

// types an object as a DOBJECT
// but all variables and objects are created as DOBJECTs so this is not necessary

void
CSL_SetPropertiesAsDObject ( )
{
    DObject * o = ( DObject * ) DataStack_Pop ( ) ;
    o->W_ObjectAttributes |= DOBJECT | T_ANY ;
}

DObject *
_DObject_NewSlot ( DObject * proto, byte * name, int64 value )
{
    DObject * dobject = DObject_Sub_New ( proto, name, DOBJECT ) ;
    dobject->W_DObjectValue = value ;
    dobject->W_PtrToValue = & dobject->W_DObjectValue ;
    proto->S_NumberOfSlots ++ ;

    return dobject ;
}

void
CSL_DObject_Clone ( )
{
    DObject * proto = ( DObject * ) DataStack_Pop ( ) ;
    byte * name = ( byte * ) DataStack_Pop ( ) ;
    if ( ! ( proto->W_ObjectAttributes & DOBJECT ) )
    {
        byte * buffer = Buffer_DataCleared ( _CSL_->ScratchB3 ) ;
        snprintf ( buffer, BUFFER_IX_SIZE, "\nCloning Alert : \'%s\' is not a dynamic object.\n\n", proto->Name ) ;
        //Error ( ( byte* ) "\nCloning Alert : \'%s\' is not a dynamic object.\n\n", proto->Name, PAUSE ) ;
        Error ( buffer, PAUSE ) ;
    }
    DObject_Sub_New ( proto, name, DOBJECT ) ;
}

void
DObject_NewClone ( DObject * proto )
{
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    DObject_Sub_New ( proto, name, DOBJECT ) ;
}

void
DObject_New ( )
{
    DObject_NewClone ( 0 ) ;
}

