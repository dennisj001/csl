
#include "../../include/csl.h"

// SEMI :: ';'
// VOID :: ' '
// PTR_CHAR :: '*' 
// NUMERIC :: DIGIT *
// _ID :: ALPHA_NUMERIC *
// PTR :: [STRUCT_UNDION_HEADER] '*' _ID
// ID :: _ID | PTR
// TYPE_ID :: ID
// PARAMETER_ID :: _ID | _ID PTR_CHAR | VOID
// ARRAY :: ('[' NUMERIC ] ']')*
// ARRAY_FIELD :: ID ARRAY
// BITFIELD :: ( ['unsigned'] | ['unsigned'] TYPE_ID ) ID ':' NUMERIC SEMI
// ARRAY_FIELDS :: ARRAY_FIELD (',' ARRAY_FIELD)*
// _ID_OR_ARRAY_FIELD : ID | ARRAY_FIELD | PARAMETER_ID
// ID_FIELDS : _ID_OR_ARRAY_FIELD ( ',' _ID_OR_ARRAY_FIELD )*
// PARAMETER_FIELDS : ID_FIELDS
// FUNCTION_ID :: [PTR_CHAR] '(' PTR ')' '(' PARAMETER_FIELDS ')' 
// SIMPLE_TYPE_FIELD :: TYPE_ID ( ID_FIELDS | FUNCTION_ID )
// STRUCT_DEF :: '{' FIELD* '}' | '{' BITFIELD* '}'
// FORWARD_REF :: _ID 
// STRUCT_UNION_DEF :: _ID STRUCT_DEF | _ID STRUCT_DEF ID_FIELDS | STRUCT_DEF ID_FIELDS | FORWARD_REF
// STRUCT_UNION_FIELD :: SIMPLE_TYPE_FIELD | STRUCT_UNION_DEF
// STRUCT_UNDION_HEADER :: ( 'struct' | 'union' ) 
// STRUCT_UNION :: STRUCT_UNDION_HEADER STRUCT_UNION_FIELD
// FIELD :: STRUCT_UNION | SIMPLE_TYPE_FIELD 
// TYPEDEF :: 'typedef' FIELD SEMI
// TYPE :: 'type' FIELD SEMI
// TYPED_FIELD :: TYPEDEF | TYPE

// we have read the idField : identifier = token

void
TD_PostStructId_Init ( Word * id )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte *identifier = tdi->TdiToken ;
    Object_Size_Set ( id, tdi->Tdi_StructureUnion_Size ) ; //+= ( tdi->Tdi_StructureUnion_Size % 8 ) ) ;
    id->W_ObjectAttributes |= ( STRUCT | C_TYPE | C_CLASS | NAMESPACE ) ; //??
    id->WD_Offset = tdi->Tdi_Offset ; //- tdi->Tdi_StructureUnion_Size ; // ??
    _CSL_Set_WordSourceCode ( _CSL_, tdi->Tdi_StructureUnion_Namespace, 1 ) ;
}

void
TD_Identifier ( int64 t_type )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte *identifier = tdi->TdiToken ;
    Word * id, *addToNs ;

    //if ( String_Equal ( identifier, "(" ) || String_Equal ( identifier, ")" ) ) return ; // better syntax detection wouldn't need this??
    //Pause ( ) ;
    tdi->FieldName = identifier ;
    if ( GetState ( tdi, TDI_PRINT ) )
    {
        if ( t_type & TD_PRE_STRUCTURE_ID ) oPrintf ( "\n%s", identifier ) ;
        if ( t_type & TD_ARRAY_FIELD ) TDI_ReadToken ( ), TD_Array ( ) ;
        if ( t_type & ( TD_TYPE_FIELD | TD_FIELD_ID ) ) TDI_Print_Field ( t_type, tdi->Tdi_Field_Size ) ;
        if ( t_type & TD_ARRAY_FIELD )
            Dump ( &tdi->DataPtr [ tdi->Tdi_Offset ], tdi->Tdi_Field_Size, 16 ) ;
    }
    else if ( t_type & ( TD_TYPE_FIELD | TD_FIELD_ID | TD_FUNCTION_ID_FIELD | TD_BIT_FIELD ) )
    {
        //if ( ( t_type == TD_FUNCTION_ID_FIELD ) && ( _Namespace_Find ( identifier, 0, 0 ) ) ) return ; // better syntax detection wouldn't need this??

        // with a nameless struct or union identifier are added to the background namespace
        addToNs = ( tdi->Tdi_StructureUnion_Namespace && tdi->Tdi_StructureUnion_Namespace->Name ) ?
            tdi->Tdi_StructureUnion_Namespace : ( ( t_type == TD_FIELD_ID ) && tdi->Tdi_Field_Type_Namespace ) ? tdi->Tdi_Field_Type_Namespace : tdi->Tdi_InNamespace ;
        tdi->Tdi_Field_Object = id = DataObject_New ( TD_TYPE_FIELD, 0, identifier, 0, 0, 0, tdi->Tdi_Offset, tdi->Tdi_Field_Size, addToNs, 0, 0, 0 ) ;
        TypeNamespace_Set ( id, tdi->Tdi_Field_Type_Namespace ) ;
        if ( GetState ( tdi, TDI_POINTER ) )
        {
            id->W_ObjectAttributes |= ( tdi->Tdi_Field_Type_Namespace && ( tdi->Tdi_Field_Type_Namespace->W_ObjectAttributes & ( STRUCT ) ) ) ? ( O_POINTER | OBJECT | STRUCT ) : O_POINTER ;
        }
        Object_Size_Set ( id, tdi->Tdi_Field_Size ) ; //+=  ( tdi->Tdi_Field_Size % 8 ) ) ;
        id->WD_Offset = tdi->Tdi_Offset ;
        //id->ContaingingStructNamespace = tdi->Tdi_StructureUnion_Namespace ;
    }
    else if ( t_type & TD_PRE_STRUCTURE_ID )
    {
        tdi->Tdi_StructureUnion_Namespace = id = DataObject_New ( CLASS, 0, identifier, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
        id->W_ObjectAttributes |= ( STRUCT | C_TYPE | C_CLASS | NAMESPACE ) ; //??
        id->WD_Offset = tdi->Tdi_Offset ;
        tdi->StructureUnionName = identifier ;

    }
    else if ( t_type & TD_POST_STRUCTURE_ID ) //&& GetState ( tdi, TDI_STRUCTURE_COMPLETED ) )
    {
        if ( tdi->Tdi_StructureUnion_Namespace ) id = TD_Do_IdentifierAlias ( identifier, tdi->Tdi_StructureUnion_Size ) ;
        else tdi->Tdi_StructureUnion_Namespace = id = DataObject_New ( CLASS, 0, identifier, 0, 0, 0, 0, 0, tdi->Tdi_InNamespace, 0, - 1, - 1 ) ;
        TD_PostStructId_Init ( id ) ;
        dbg ( 5, ( int64 ) tdi, 0 ) ;
    }
    else CSL_TD_Error ( "TD_Identifier : No identifier type given", identifier ) ;

    if ( ( Verbosity ( ) > 1 ) && ( ! GetState ( tdi, TDI_PRINT ) ) ) TDI_DebugPrintWord ( id ) ; // print class field
}

void
TD_Array ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    //TDI_ReadToken ( ) ;
    int64 arrayDimensions [ 32 ] ; // 32 : max dimensions for now
    //if ( GetState ( tdi, TDI_PRINT ) ) iPrintf ( "" ) ; //debug
    int64 size = tdi->Tdi_Field_Size, i, arrayDimensionSize ;
    byte *token = tdi->TdiToken ;
    memset ( arrayDimensions, 0, sizeof (arrayDimensions ) ) ;

    for ( i = 0 ; 1 ; i ++ )
    {
        if ( token && ( token[0] == '[' ) )
        {
            token = TDI_ReadToken ( ) ;
            arrayDimensionSize = atoi ( token ) ;
            size = size * arrayDimensionSize ;
            tdi->Tdi_Field_Size = size ;
            token = TDI_ReadToken ( ) ;
            if ( ! String_Equal ( ( char* ) token, "]" ) ) CSL_Exception ( SYNTAX_ERROR, 0, 1 ) ;
            else arrayDimensions [ i ] = arrayDimensionSize ;
            token = TDI_ReadToken ( ) ;
        }
        else
        {
            if ( i )
            {
                tdi->Tdi_ArrayDimensions = ( int64 * ) Mem_Allocate ( i * sizeof (int64 ), DICTIONARY ) ; //tdi->Tdi_Field_Size, DICTIONARY ) ;
                MemCpy ( tdi->Tdi_ArrayDimensions, arrayDimensions, i * sizeof (int64 ) ) ; //tdi->Tdi_Field_Size ) ;
                tdi->Tdi_ArrayNumberOfDimensions = i ;
                if ( tdi->Tdi_Field_Object )
                {
                    tdi->Tdi_Field_Object->ArrayDimensions = tdi->Tdi_ArrayDimensions ; //tdi->Tdi_Field_Size, DICTIONARY ) ;
                    tdi->Tdi_Field_Object->ArrayNumberOfDimensions = tdi->Tdi_ArrayNumberOfDimensions ;
                }
            }
            break ;
        }
    }
}

void
TD_BitField ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    //if ( GetState ( tdi, TDI_PRINT ) ) return ;
    byte * token = TDI_ReadToken ( ) ;
    int64 size = atoi ( token ) ;
    SetState ( tdi, TDI_BITFIELD, true ) ;
    if ( tdi->Tdi_Field_Object )
    {
        tdi->Tdi_Field_Object->W_ObjectAttributes |= BITFIELD ;
        tdi->Tdi_Field_Object->BitFieldOffset = tdi->Tdi_BitFieldOffset ;
        tdi->Tdi_BitFieldOffset += size ;
        tdi->Tdi_Field_Object->BitFieldSize = ( int8 ) size ;
        tdi->Tdi_Field_Size = tdi->Tdi_BitFieldOffset / 8 ;
    }
}

byte
PeekCheckForStars ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token = TDI_PeekToken ( ), rtrn = 0 ;
    if ( token[0] == '*' )
    {
        SetState ( tdi, TDI_POINTER, true ) ;
        rtrn = '*' ;
    }
    else rtrn = 0 ;
    return rtrn ;
}

byte
Check_ForAndRemove_Stars ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token, rtrn = 0 ;
    if ( tdi->TdiToken[0] == '*' ) rtrn = '*' ;
    while ( token = TDI_PeekToken ( ) )
    {
        if ( token [0] != '*' ) break ;
        else rtrn = '*' ;
        TDI_ReadToken ( ) ;
    }
    return rtrn ;
}

Boolean
TD_CheckForPointer ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    Namespace * type0 ;
    byte * token = tdi->TdiToken ;
    Boolean rtrn ; //= false ;
    if ( Check_ForAndRemove_Stars ( ) ) //( token [0] == '*' ) || ( token1 = TDI_PeekToken ( ), ( token1 [0] == '*' ) ) )
    {
        //token = TDI_ReadToken ( ) ; // the '*' 
        tdi->Tdi_Field_Size = CELL ;
        SetState ( tdi, TDI_POINTER, true ) ;
        token = TDI_ReadToken ( ) ; // the '*' 
        rtrn = true ;
    }
    else rtrn = false ;
    return rtrn ;
}

void
TD_Type_Id ( int64 t_type )
{
    Context * cntx = _Context_ ;
    TDI * tdi = TDI_GetTop ( ) ;
    Namespace * type0 ;
    byte * token = tdi->TdiToken, *token1 ;

    if ( ( token [0] != '*' ) && ( token [0] != '{' ) && ( token [0] != ')' ) && ( token [0] != '(' ) )
    {
        type0 = _Namespace_Find ( token, 0, 0 ) ;
        if ( type0 && ( ! ( t_type & TD_POST_STRUCTURE_ID ) ) )
        {
            tdi->Tdi_Field_Type_Namespace = type0 ;
            tdi->Tdi_Field_Size = CSL_Get_Namespace_SizeVar_Value ( type0 ) ;
        }
        else //... ( t_type & TD_POST_STRUCTURE_ID ) 
        {
            TD_Identifier ( t_type ) ;
        }
        token = TDI_ReadToken ( ) ;
    }
}

int64
TD_Identifier_Or_Array_Field ( int64 t_type )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte *token = tdi->TdiToken ;
    if ( token && ( token [0] != ';' ) )
    {
        TD_Type_Id ( t_type ) ;
        TD_CheckForPointer ( ) ;
        if ( tdi->TdiToken [0] != ';' )
        {
            if ( tdi->TdiToken [0] == ',' ) TDI_ReadToken ( ) ;
            if ( ( tdi->TdiToken [0] == '(' ) || ( tdi->TdiToken [0] == ')' ) ) t_type = TD_FUNCTION_ID_FIELD ; //TD_FunctionId_Field
            if ( ! (( t_type == TD_FUNCTION_ID_FIELD ) && ( _Namespace_Find ( tdi->TdiToken, 0, 0 ) ) ) ) 
            {
                if ( ! ( ( tdi->TdiToken [0] == '(' ) || ( tdi->TdiToken [0] == ')' ) ) ) 
                TD_Identifier ( t_type ) ;
            }
            token = TDI_PeekToken ( ) ;
            if ( token && ( token [0] == '[' ) ) TDI_ReadToken ( ), TD_Array ( ) ;
            else if ( token && ( token [0] == ':' ) )
            {
                TD_Identifier ( TD_BIT_FIELD ) ;
                token = TDI_ReadToken ( ) ;
                TD_BitField ( ) ;
            }
            else TDI_ReadToken ( ) ;
        }
        TD_IntraStructFieldAccounting ( ) ;
    }
    return t_type ;
}

int64
TD_Identifier_Fields ( int64 t_type )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token = tdi->TdiToken ;
    while ( token[0] != ';' )
    {
        if ( tdi->TdiToken [0] == ',' ) TDI_ReadToken ( ) ;
        t_type = TD_Identifier_Or_Array_Field ( t_type ) ;
        token = tdi->TdiToken ;
        if ( token )
        {
            if ( token[0] == ',' ) TDI_ReadToken ( ) ;
            if ( token[0] == ';' ) break ;
        }
        else break ;
    }
    return t_type ;
}

void
TD_Parameter_Fields ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token ;
    while ( 1 )
    {
        if ( tdi->TdiToken[0] == ')' )
        {
            token = TDI_ReadToken ( ) ; // ';'
            break ;
        }
        TD_Type_Id ( TD_TYPE_FIELD ) ;
        TD_CheckForPointer ( ) ;
        token = tdi->TdiToken ;
        if ( ( token [0] == ';' ) || ( token [0] == ',' ) ) TDI_ReadToken ( ) ;
    }

}

void
TD_FunctionId_Field ( int64 t_type )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token ;
    TD_Identifier_Or_Array_Field ( t_type ) ;
    if ( tdi->TdiToken[0] == ')' )
    {
        token = TDI_ReadToken ( ) ; // '('
        TDI_ReadToken ( ) ;
        TD_Parameter_Fields ( ) ;
    }
}

void
TD_Type_Def ( )
{
    TDI * tdi = ( TDI * ) Stack_Top ( _Context_->TDI_StructUnionStack ) ;
    byte * ptoken = TDI_PeekToken ( ) ;
    if ( ( ptoken[0] == '(' ) ) TD_FunctionId_Field ( TD_FUNCTION_ID_FIELD ) ;
    else
    {
        int64 t_type = TD_Identifier_Fields ( TD_TYPE_FIELD ) ;
        if ( ( t_type == TD_TYPE_FIELD ) && ( ! tdi->Tdi_Field_Type_Namespace ) ) CSL_TD_Error ( "TD_Type_Field : No type field namespace.", tdi->TdiToken ) ;
    }
}
// '{' token may or may not have already been parsed

void
_TD_Struct_Fields ( ) // 'struct' or 'union'
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token = tdi->TdiToken ;
    do
    {
        TD_Field ( 0 ) ;
        tdi = TDI_GetTop ( ) ;
        if ( tdi->TdiToken && ( tdi->TdiToken [0] == ';' ) ) TDI_ReadToken ( ) ;
    }
    while ( tdi->TdiToken && ( tdi->TdiToken [0] != '}' ) ) ;
    TD_PostStructUnion_Accounting ( ) ;
}

void
TD_Struct_Union_Def ( int64 structOrUnionTypeFlag ) // 'struct' or 'union'
{
    TDI * tdi = TD_PreStruct_Accounting ( structOrUnionTypeFlag ) ;
    byte * token = tdi->TdiToken ;
    if ( ! token ) token = TDI_ReadToken ( ) ; //nb! for class ":{" 
    if ( ( token [0] == '{' ) || ( token[1] == '{' ) || ( token[2] == '{' ) ) token = TDI_ReadToken ( ) ; // consider ":{" and +:{" tokens
    else
    {
        TD_Identifier ( TD_PRE_STRUCTURE_ID ) ;
        token = TDI_ReadToken ( ) ;
        if ( ( token [0] == '{' ) || ( token[1] == '{' ) || ( token[2] == '{' ) ) token = TDI_ReadToken ( ) ; // consider ":{" and +:{" tokens
    }
    byte * ptoken = TDI_PeekToken ( ) ;
    if ( ptoken [0] == ';' )
    {
        TD_Identifier ( TD_FORWARD_REF ) ;
        token = TDI_ReadToken ( ) ;
        return ; // forward ref
    }
    _TD_Struct_Fields ( ) ;
    token = TDI_ReadToken ( ) ;
    if ( token && ( token[0] != ';' ) ) TD_Post_Struct_Identifiers ( ) ;
}

// 'struct' or 'union' tokens should have already been parsed

int64
TD_CheckForStructUnionHeader ( byte * token )
{
    int64 structOrUnionTypeFlag = 0 ;
    if ( String_Equal ( ( char* ) token, "struct" ) || String_Equal ( ( char* ) token, "class" ) ) structOrUnionTypeFlag = TDI_STRUCT ;
    else if ( String_Equal ( ( char* ) token, "union" ) ) structOrUnionTypeFlag = TDI_UNION ;
    if ( structOrUnionTypeFlag && ( ! String_Equal ( ( char* ) token, "class" ) ) )
        TDI_ReadToken ( ), token = TDI_PeekToken ( ) ;
    if ( String_Equal ( token, "*" ) )
        structOrUnionTypeFlag = 0 ; //, TDI_ReadToken ( ) ;  
    return structOrUnionTypeFlag ;
}

void
TD_Field ( int64 structOrUnionTypeFlag )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte * token = tdi->TdiToken ? tdi->TdiToken : TDI_ReadToken ( ) ;
    if ( token && token[0] != ';' )
    {
        if ( ! structOrUnionTypeFlag ) structOrUnionTypeFlag = TD_CheckForStructUnionHeader ( token ) ;
        else if ( ( token [0] != '{' ) && ( token[1] != '{' ) && ( token[2] != '{' ) ) // 'class :{' | 'class +:{' 
            token = TDI_ReadToken ( ) ;
        if ( ( structOrUnionTypeFlag & TDI_UNION ) && GetState ( tdi, TDI_PRINT ) )
            iPrintf ( "\n%s", token ) ;
        if ( structOrUnionTypeFlag ) TD_Struct_Union_Def ( structOrUnionTypeFlag ) ; //TD_Struct_Union_Field ( structOrUnionTypeFlag ) ;
        else TD_Type_Def ( ) ;
    }
    else TDI_ReadToken ( ) ;
}

int64
TDI_Type_Field ( )
{
    Word * word = _Context_->CurrentEvalWord ;
    TDI * tdi = TDI_Start ( word, 0, 0 ) ;
    TD_Field ( 0 ) ;
    TDI_Finalize ( ) ;

    return tdi->Tdi_StructureUnion_Size ;
}

// Struct : in functions here refers to struct or union types

TDI *
TD_PreStruct_Accounting ( int64 structOrUnionTypeFlag )
{
    TDI * ctdi, *ntdi ;
    Context * cntx = _Context_ ;
    SetState ( cntx, TDI_PARSING, true ) ;
    //int64 depth = Stack_Depth ( _CONTEXT_TDI_STACK ) ;
    ctdi = TDI_GetTop ( ) ; // current -> previous
    ntdi = TDI_Push_New ( ) ; // adding one to the Stack - new current
    //if ( ns ) ntdi->Tdi_StructureUnion_Namespace = ns ;
    if ( ctdi ) // previous tdi - Tdi_PreStructureUnion_Namespace
    {
        ntdi->Tdi_Offset = ctdi->Tdi_Offset ; //0 ; //ctdi->Tdi_Offset ;
        ntdi->TdiToken = ctdi->TdiToken ;
        ntdi->State = ctdi->State & ( ~ ( TDI_UNION | TDI_STRUCT ) ) ; // transfer the non - struct/union state only
        ntdi->State |= structOrUnionTypeFlag ;
        ntdi->DataPtr = ctdi->DataPtr ;
        ntdi->Tdi_InNamespace = ctdi->Tdi_StructureUnion_Namespace ? ctdi->Tdi_StructureUnion_Namespace : ctdi->Tdi_InNamespace ; //_CSL_Namespace_InNamespaceGet ( ) ;
        if ( ! ntdi->Tdi_StructureUnion_Namespace ) ntdi->Tdi_StructureUnion_Namespace = ctdi->Tdi_StructureUnion_Namespace ;
    }
    ntdi->State |= structOrUnionTypeFlag ; //ctdi->State & ( TDI_UNION | TDI_STRUCT | TDI_PRINT ) ; //ctdi->State ; //|= structUnionTypeFlag ;

    return ntdi ;
}

void
_TD_PostStructUnion_Accounting ( TDI * jf_tdi )
{
    TDI * sctdi ;
    int64 offset = 0, sd ;
    do
    {
        if ( ( sd = Stack_Depth ( _CONTEXT_TDI_STACK ) ) > offset )
        {
            sctdi = TDI_Pick ( offset ++ ) ;
            if ( sctdi && GetState ( sctdi, TDI_UNION ) )
            {
                if ( jf_tdi->Tdi_StructureUnion_Size > sctdi->Tdi_StructureUnion_Size )
                    sctdi->Tdi_StructureUnion_Size = jf_tdi->Tdi_StructureUnion_Size ;
            }
            else break ;
        }
        else break ;
    }
    while ( sctdi ) ;
}

void
TD_PostStructUnion_Accounting ( )
{
    TDI * jf_tdi = 0, *ctdi ;
    int64 depth = Stack_Depth ( _CONTEXT_TDI_STACK ) ;
    if ( depth > 1 ) jf_tdi = TDI_Pop ( ) ; // just finished TDSCI
    ctdi = TDI_GetTop ( ) ; // now 'current'
    //A_Parser_Debug ( ) ;
    if ( jf_tdi && ( ! GetState ( ctdi, TDI_POST_STRUCT ) ) ) // always should be there but check anyway
    {
        if ( GetState ( jf_tdi, TDI_UNION ) ) //|| GetState ( ctdi, TDI_UNION ) )
        {
            if ( GetState ( ctdi, TDI_UNION ) ) _TD_PostStructUnion_Accounting ( jf_tdi ) ;
            else ctdi->Tdi_StructureUnion_Size += jf_tdi->Tdi_StructureUnion_Size ;
            if ( GetState ( ctdi, TDI_PRINT ) ) iPrintf ( "\n" ) ;
            //SetState ( jf_tdi, TDI_UNION, false ) ;
        }
        else if ( GetState ( jf_tdi, TDI_STRUCT ) )
        {
            if ( GetState ( ctdi, TDI_UNION ) ) _TD_PostStructUnion_Accounting ( jf_tdi ) ;
            else ctdi->Tdi_StructureUnion_Size += jf_tdi->Tdi_StructureUnion_Size ;
            //SetState ( jf_tdi, TDI_STRUCT, false ) ;
        }
        ctdi->Tdi_Structure_Size = ctdi->Tdi_StructureUnion_Size ;
        if ( GetState ( ctdi, TDI_UNION ) ) ctdi->Tdi_Offset = 0 ; //, SetState ( jf_tdi, TDI_UNION, false ) ;
        else ctdi->Tdi_Offset = ctdi->Tdi_StructureUnion_Size ;
        ctdi->TdiToken = jf_tdi->TdiToken ;
        if ( ! ctdi->Tdi_StructureUnion_Namespace ) ctdi->Tdi_StructureUnion_Namespace = jf_tdi->Tdi_StructureUnion_Namespace ;
    }
    //A_Parser_Debug ( ) ;
}

void
TD_IntraStructFieldAccounting ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    //A_Parser_Debug ( ) ;
    if ( ! GetState ( tdi, TDI_POST_STRUCT ) )
    {
        if ( GetState ( tdi, TDI_UNION ) )
        {
            if ( tdi->Tdi_Field_Size > tdi->Tdi_StructureUnion_Size ) tdi->Tdi_StructureUnion_Size = tdi->Tdi_Field_Size ;
        }
        else // not TDI_UNION // field may be a struct
        {
            tdi->Tdi_StructureUnion_Size += tdi->Tdi_Field_Size ;
            tdi->Tdi_Offset += tdi->Tdi_Field_Size ;
        }
        SetState ( tdi, TDI_POINTER, false ) ;
    }
    //A_Parser_Debug ( ) ;
}

void
TDI_Init ( TypeDefInfo * tdi )
{
    //tdi->State = 0 ;
    tdi->Tdi_InNamespace = _CSL_Namespace_InNamespaceGet ( ) ;
}

TypeDefInfo *
TypeDefStructCompileInfo_New ( uint64 allocType )
{
    TypeDefInfo * tdi = ( TypeDefInfo * ) Mem_Allocate ( sizeof (TypeDefInfo ), allocType ) ;
    TDI_Init ( tdi ) ;
    return tdi ;
}

TypeDefInfo *
TDI_Push_New ( )
{
    TypeDefInfo *tdi = TypeDefStructCompileInfo_New ( COMPILER_TEMP ) ;
    Stack_Push ( _CONTEXT_TDI_STACK, ( int64 ) tdi ) ;
    return tdi ;
}

TDI *
TDI_Start ( Namespace * ns, byte* objectBitData, int64 stateFlags )
{
    Context * cntx = _Context_ ;
    TDI * tdi ;
    TDI_STACK_INIT ( ) ;
    tdi = TDI_Push_New ( ) ;
    if ( ns && ( ns->W_ObjectAttributes & NAMESPACE ) )
        tdi->Tdi_StructureUnion_Namespace = ns ;
    if ( stateFlags & TDI_CLONE_FLAG )
    {
        tdi->Tdi_Offset = _Namespace_VariableValueGet ( tdi->Tdi_InNamespace, ( byte* ) "size" ) ; // allows for cloning - prototyping
        tdi->Tdi_StructureUnion_Size = tdi->Tdi_Offset ;
    }
    tdi->State |= stateFlags ;
    tdi->DataPtr = objectBitData ;
    SetState ( _Context_, TDI_PARSING, true ) ;
    Lexer_SetTokenDelimiters ( cntx->Lexer0, ( byte* ) " ,\n\r\t", COMPILER_TEMP ) ;
    if ( ! ( stateFlags & TDI_PRINT ) )
    {
        CSL_Lexer_SourceCodeOn ( ) ;
        CSL_InitSourceCode_WithName ( _CSL_, ns ? ns->Name : 0, 1 ) ;
    }
    return tdi ;
}

void
TD_Post_Struct_Identifiers ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    SetState ( tdi, TDI_POST_STRUCT, true ) ;
    byte * token = tdi->TdiToken ; //TDI_ReadToken ( ) ; //tdi->TdiToken ;
    if ( token && ( token[0] != ';' ) && ( ! String_Equal ( token, "struct" ) ) && ( ! String_Equal ( token, "union" ) ) )
        TD_Identifier_Fields ( TD_POST_STRUCTURE_ID ) ;
    SetState ( tdi, TDI_POST_STRUCT, false ) ;
}

TDI *
TDI_Finalize ( )
{
    //TD_Post_Struct_Identifiers ( ) ;
    TDI * tdi = TDI_GetTop ( ), * one = TDI_Pick ( 1 ) ; // top was dummy //TDI_GetTop ( ) ;
    Namespace * ns = tdi->Tdi_StructureUnion_Namespace ? tdi->Tdi_StructureUnion_Namespace : one->Tdi_StructureUnion_Namespace ;
    CSL_Finish_WordSourceCode ( _CSL_, ns, 1 ) ;
    if ( ! GetState ( tdi, TDI_PRINT ) ) Object_Size_Set ( ns ? ns : tdi->Tdi_InNamespace, tdi->Tdi_StructureUnion_Size ) ;
    SetState ( _Context_, TDI_PARSING, false ) ;
    return 0 ; //tdi ; // should return the initial tdi from TDI_Start with the final fields filled in
}

TDI *
TDI_GetTop ( )
{
    TDI * tdi ;
    return tdi = ( TDI * ) Stack_Top ( _CONTEXT_TDI_STACK ) ;
}

TDI *
TDI_Pick ( int64 offset )
{
    TDI * tdi ;
    return tdi = ( TDI * ) _Stack_Pick ( _CONTEXT_TDI_STACK, offset ) ;
}

TDI *
TDI_Pop ( )
{
    TDI * tdi ;
    return tdi = ( TDI* ) Stack_Pop ( _CONTEXT_TDI_STACK ) ;
}

void
TDI_STACK_INIT ( )
{
    Stack_Init ( _CONTEXT_TDI_STACK ) ;
}

void
TDI_Print_StructNameEtc ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    iPrintf ( "\n\t%16s : %s : size = %d : at %016lx",
        tdi->Tdi_Field_Type_Namespace->Name, tdi->TdiToken,
        ( int64 ) _CSL_VariableValueGet ( tdi->Tdi_Field_Type_Namespace->Name, ( byte* ) "size" ), //CSL_GetAndSet_ObjectByteSize ( tdi->Tdi_Field_Type_Namespace ),
        &tdi->DataPtr [ tdi->Tdi_Offset ] ) ;
}

void
TDI_Print_StructField ( )
{
    TDI * tdi = TDI_GetTop ( ) ;
    CSL_NewLine ( ) ;
    //TDI_DebugPrintWord ( cntx, tdi->Tdi_Field_Type_Namespace ) ;
    TDI_Print_StructNameEtc ( ) ;
    Word * word = Word_UnAlias ( tdi->Tdi_Field_Type_Namespace ) ;
    Object_PrintStructuredData ( & tdi->DataPtr [ tdi->Tdi_Offset ], word ) ; //->W_OriginalCodeText ) ;
    //CSL_NewLine ( ) ;
}

void
TDI_Print_Field ( int64 t_type, int64 size )
{
#define FRMT "\n0x%016lx  %-16s%-3s  %-24s"    
#define FRMT_CHAR2 "0x%02x"    
    TDI * tdi = TDI_GetTop ( ) ;
    byte *token = tdi->TdiToken, *format ; //, *format1 = "\n0x%016lx\t%12s%s\t%16s" ;
    //if ( String_Equal ( token, "AliasOf" ) ) iPrintf ( "" ) ;
    int64 value ;
    if ( tdi->Tdi_Field_Type_Namespace )
    {
        byte * dataPtr = & tdi->DataPtr [ tdi->Tdi_Offset ] ;
        if ( t_type & TD_ARRAY_FIELD ) TDI_PrintArrayField ( dataPtr ) ;
        else if ( ( ! GetState ( tdi, TDI_POINTER ) ) && ( tdi->Tdi_Field_Type_Namespace
            && ( tdi->Tdi_Field_Type_Namespace->W_ObjectAttributes & STRUCTURE_TYPE )
            && ( ! ( tdi->Tdi_Field_Type_Namespace->W_ObjectAttributes & O_POINTER ) ) ) )
        {
            TDI_Print_StructField ( ) ;
        }
        else
        {
            switch ( size )
            {
                case 1:
                {
                    format = ( byte* ) FRMT " = \'%c\' : " FRMT_CHAR2 ;
                    value = * ( ( int8* ) ( dataPtr ) ) ; //( &tdi->DataPtr [ tdi->Tdi_Offset ] ) ) ;
                    break ;
                }
                case 2:
                {
                    format = ( byte* ) FRMT " = 0x%04x" ;
                    value = * ( ( int16* ) ( dataPtr ) ) ; //( &tdi->DataPtr [ tdi->Tdi_Offset ] ) ) ;
                    break ;
                }
                case 4:
                {
                    format = ( byte* ) FRMT " = 0x%08lx" ;
                    value = * ( ( int32* ) ( dataPtr ) ) ; //( &tdi->DataPtr [ tdi->Tdi_Offset ] ) ) ;
                    break ;
                }
                default:
                case CELL:
                {
                    format = ( byte* ) FRMT " = 0x%016lx" ;
                    value = * ( ( int64* ) ( dataPtr ) ) ; //( &tdi->DataPtr [ tdi->Tdi_Offset ] ) ) ;
                    break ;
                }
            }
            if ( size == 1 )
            {
                iPrintf ( format, dataPtr, tdi->Tdi_Field_Type_Namespace->Name,
                    ( GetState ( tdi, TDI_POINTER ) ? " * " : "" ), token, value, value ) ;
            }
            else iPrintf ( format, dataPtr, tdi->Tdi_Field_Type_Namespace->Name,
                ( GetState ( tdi, TDI_POINTER ) ? " * " : "" ), token, value ) ;
            if ( ! ( t_type & ( TD_POST_STRUCTURE_ID | TD_PRE_STRUCTURE_ID ) ) ) tdi->Tdi_Field_Size = size ;
            //if ( Is_DebugOn ) 
            oPrintf ( "\t\toffset = 0x%lx : dataPtr = 0x%lx", tdi->Tdi_Offset, dataPtr ) ;
            if ( String_Equal ( tdi->Tdi_Field_Type_Namespace->Name, "byte" ) && GetState ( tdi, TDI_POINTER ) )
                if ( IsString ( ( byte* ) ( * ( int64 * ) dataPtr ) ) )
                    oPrintf ( "\n\t%s = \'%s\'", tdi->FieldName, ( ( byte* ) ( * ( int64 * ) dataPtr ) ) ) ;
        }
        SetState ( tdi, TDI_POINTER, false ) ;
    }
}

void
TDI_PrintArrayField ( byte * dataPtr )
{
    TDI * tdi = TDI_GetTop ( ) ;
    byte *fieldName = tdi->FieldName ;
    char ads [64], adn [ 32 ] ;
    int64 i ;
    for ( ads[0] = 0, i = 0 ; i < tdi->Tdi_ArrayNumberOfDimensions ; i ++ )
    {
        snprintf ( adn, 32, "[%-ld]", tdi->Tdi_ArrayDimensions[i] ) ;
        strncat ( ads, adn, 63 ) ;
    }
    int64 slfn = Strlen ( fieldName ), slad = Strlen ( ads ) ;
    if ( slfn > 24 ) slfn = 24 ;
    iPrintf ( "\n0x%016lx  %-16s%-3s  %-*s %s%*s = ", dataPtr, tdi->Tdi_Field_Type_Namespace->Name,
        ( GetState ( tdi, TDI_POINTER ) ? " * " : "" ), slfn, fieldName, ads, 23 - slfn - slad, "" ) ;
}

void
TDI_DebugPrintWord ( Word * word )
{
    TDI * tdi = TDI_GetTop ( ) ;
    if ( word ) iPrintf ( "\n%s.%s : field size = %ld : structure size = %ld : total size = %ld : dataPtr = 0x%09x : offset == %ld : at %s",
        ( word->TypeNamespace ? word->TypeNamespace->Name : word->S_ContainingNamespace->Name ),
        word->Name, tdi->Tdi_Field_Size, tdi->Tdi_StructureUnion_Size,
        tdi->Tdi_StructureUnion_Size, tdi->DataPtr, tdi->Tdi_Offset, Context_Location ( ) ) ;
}

Boolean
Parser_Check_Do_CommentDebugWord ( Word * word )
{
    Boolean rtrn ;
    SetState ( _Context_, TDI_PARSING, false ) ;
    if ( word && ( ( word->W_MorphismAttributes & ( COMMENT | DEBUG_WORD ) ) || ( word->W_TypeAttributes & ( W_COMMENT | W_PREPROCESSOR ) ) ) )
    {
        Interpreter_DoWord ( _Interpreter_, word, _Lexer_->TokenStart_ReadLineIndex, _Lexer_->SC_Index ) ;
        rtrn = true ;
    }
    else rtrn = false ;
    SetState ( _Context_, TDI_PARSING, true ) ;
    return rtrn ;
}

Boolean
Parser_Check_Do_Debug_Token ( byte * token )
{
    Word * word = Finder_Word_FindUsing ( _Finder_, token, 0 ) ;
    return Parser_Check_Do_CommentDebugWord ( word ) ;
}

byte *
TDI_PeekToken ( )
{
    Context * cntx = _Context_ ;
    byte * token = Lexer_Peek_NextToken ( cntx->Lexer0, 0, 1 ) ;
    return token ;
}

byte *
TDI_ReadToken ( )
{
    Context * cntx = _Context_ ;
    TDI * tdi = TDI_GetTop ( ) ;
    do
    {
        tdi->TdiToken = Lexer_ReadToken ( cntx->Lexer0 ) ;
        DEBUG_SETUP_TOKEN ( tdi->TdiToken, 0 ) ;
    }
    while ( Parser_Check_Do_Debug_Token ( tdi->TdiToken ) ) ;
    //tdi = TDI_GetTop ( ) ;
    tdi->NextChar = _ReadLine_pb_NextChar ( cntx->ReadLiner0 ) ;
    return tdi->TdiToken ;
}

Word *
_CSL_TypedefAlias ( Word * word0, byte * name, Namespace * addToNs, int64 size )
{
    TDI * tdi = TDI_GetTop ( ) ;
    Word * word = Word_UnAlias ( word0 ), * alias = 0 ;
    if ( word && word->Definition )
    {
        if ( ! ( alias = _Finder_FindWord_InOneNamespace ( _Finder_, tdi->Tdi_StructureUnion_Namespace, name ) ) )
            if ( ! ( alias = _Finder_FindWord_InOneNamespace ( _Finder_, tdi->Tdi_InNamespace, name ) ) )
                alias = _Word_New ( name, word->W_MorphismAttributes | ALIAS, word->W_ObjectAttributes, word->W_LispAttributes, 0, addToNs, DICTIONARY ) ; // inherit type from original word
        _Word_DefinitionStore ( alias, ( block ) word->Definition ) ;
        DObject_Finish ( alias ) ;
        CSL_TypeStackReset ( ) ;
        _CSL_->LastFinished_Word = alias ;
        alias->S_CodeSize = word->S_CodeSize ;
        alias->W_AliasOf = word ;
        alias->ObjectByteSize = word->ObjectByteSize = size ;
        Object_Size_Set ( alias, size ) ;
        alias->S_SymbolList = word->S_SymbolList ;
    }
    else Exception ( USEAGE_ERROR, ABORT ) ;

    return alias ;
}

Word *
TD_Do_IdentifierAlias ( byte * token, int64 size )
{
    TDI * tdi = TDI_GetTop ( ) ;
    Namespace * alias = _CSL_TypedefAlias ( tdi->Tdi_StructureUnion_Namespace, token, _CSL_Namespace_InNamespaceGet ( ), size ) ; //tdi->Tdi_InNamespace ) ;
    TypeNamespace_Set ( alias, tdi->Tdi_StructureUnion_Namespace ) ;
    return alias ;
}

void
CSL_TD_Error ( byte * msg, byte * token )
{
    byte * buffer = Buffer_DataCleared ( _CSL_->ScratchB1 ) ;
    snprintf ( ( char* ) buffer, BUFFER_IX_SIZE, "\nCSL_TD_Error : %s : \'%s\' at %s", ( char* ) msg, token, Context_Location ( ) ) ;
    _Debugger_->Token = token ;
    _SyntaxError ( ( byte* ) buffer, 1 ) ; // else structure component size error
}

TDI *
_Object_Continue_PrintStructuredData ( byte * objectBits )
{
    TDI * tdi = TDI_Start ( 0, objectBits, TDI_PRINT ) ;
    byte * token = TDI_ReadToken ( ) ; // read 'typedef' token
    if ( String_Equal ( token, "typedef" ) ) token = TDI_ReadToken ( ) ;
    TD_Field ( 0 ) ;
    tdi = TDI_Finalize ( ) ;
    return tdi ;
}

TDI *
Object_PrintStructuredData ( byte * objectBits, Word * typedefWord ) //byte * typedefString )
{
    TDI * tdi = 0 ;
    byte * typedefString = typedefWord->W_OriginalCodeText ;
    if ( objectBits && typedefString && typedefString[0] )
    {
        Context * cntx0 = _Context_, * cntx = CSL_Context_PushNew ( _CSL_ ) ;
        cntx->State = cntx0->State ; // preserve C_SYNTAX, etc
        ReadLiner * rl = cntx->ReadLiner0 ;

        byte * b = Buffer_DataCleared ( _CSL_->ScratchB4 ) ;
        snprintf ( b, BUFFER_IX_SIZE, "%s : <source code text>", typedefWord->Name ) ;
        rl->Filename = b ; //(byte*) "<source code text>" ; //typedefWord->W_WordData->Filename ;
        //rl->LineNumber = typedefWord->W_WordData->LineNumber ;
        Readline_Setup_OneStringInterpret ( rl, typedefString ) ; //typedefString ) ;

        tdi = _Object_Continue_PrintStructuredData ( objectBits ) ;

        Readline_Restore_InputLine_State ( rl ) ;
        CSL_Context_PopDelete ( _CSL_ ) ;
    }
    return tdi ;
}

#if 0

void
A_Parser_Debug ( )
{
    TDI * tdi = TDI_GetTop ( ) ; // current -> previous
    Namespace * ins = Namespace_Find ( "_Identifier" ) ;
    if ( ins && ( ins == tdi->Tdi_StructureUnion_Namespace ) )
    {
        int64 size = _Namespace_VariableValueGet ( ins, ( byte* ) "size" ) ;
        if ( size || tdi->Tdi_StructureUnion_Size ) iPrintf ( "\n%s : size = %d : struct size = %d", ins->Name, size, tdi->Tdi_StructureUnion_Size ) ;
        //if ( ( size >= 184 ) || ( tdi->Tdi_StructureUnion_Size >= 184 ) )
        //    Pause ( ) ;
    }
}
#endif



