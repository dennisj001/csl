
#include "../../include/csl.h"

Symbol *
DLList_FindName_InOneNamespaceList ( dllist * list, byte * name )
{
    Symbol * s = ( Symbol* ) Tree_Map_OneNamespace_OneArg ( ( Word* ) dllist_First ( ( dllist* ) list ),
        ( MapFunction_1 ) Symbol_CompareName, ( int64 ) name ) ;
    return s ;
}

Symbol *
_DLList_FindName_InOneNamespaceList ( dllist * list, byte * name )
{
    Symbol * s = ( Symbol* ) Tree_Map_OneNamespace_OneArg ( ( Word* ) dllist_First ( list ), ( MapFunction_1 ) Symbol_CompareName, ( int64 ) name ) ;
    return s ;
}

Symbol *
DLList_FindName_InOneNamespace ( Namespace * ns, byte * name )
{
    Symbol * s = 0 ;
    if ( ns && name )
    {
        s = DLList_FindName_InOneNamespaceList ( ( dllist* ) ns->W_List, name ) ;
        if ( ! s ) s = ( Symbol* ) Tree_Map_OneNamespace_TwoArgs ( _CSL_->Namespaces, ( MapFunction_2 ) Symbol_CompareName2, ( int64 ) name, ( int64 ) ns ) ;
    }
    return s ;
}

Symbol *
_Finder_CompareDefinitionAddress ( Symbol * symbol, byte * address )
{
    if ( symbol )
    {
        Word * word = ( Word * ) symbol ;
        //byte * codeStart = ( byte* ) word->Definition ; // nb. this maybe more accurate ??
        byte * codeStart = word->CodeStart ;
        d0 ( if ( String_Equal ( "iFactorialX", symbol->S_Name ) ) iPrintf ( "\naddress = 0x%016lx :: %s :: codeStart = 0x%016lx : codeSize = %d", address, symbol->S_Name, codeStart, word->S_CodeSize ) ) ;
        //if ( ((byte*) symbol == address) || ( codeStart && ( address >= codeStart ) && ( address <= ( codeStart + word->S_CodeSize ) ) ) )
        if ( ( ( byte* ) symbol == address ) || ( codeStart && ( address >= codeStart ) &&
            ( word->S_CodeSize ? address < ( codeStart + word->S_CodeSize ) : address == codeStart ) ) )
            //if ( codeStart && ( address >= codeStart ) && ( address < ( codeStart + word->S_CodeSize ) ) )
        {
            return symbol ;
        }
    }
    return 0 ;
}

Symbol *
_Finder_CompareDefinitionAddress_NoAlias ( Symbol * symbol, byte * address )
{
    Word * word = ( Word * ) symbol ;
    if ( ( ! ( word->W_MorphismAttributes & ALIAS ) ) ) return _Finder_CompareDefinitionAddress ( symbol, address ) ;
    else return 0 ;
}

Word *
Finder_FindWordFromAddress_InOneNamespace ( Finder * finder, Namespace * ns, byte * address )
{
    if ( ns ) return finder->FoundWord = Tree_Map_OneNamespace_OneArg ( ( Word* ) dllist_First ( ( dllist* ) ns->S_SymbolList ),
        ( MapFunction_1 ) _Finder_CompareDefinitionAddress, ( int64 ) address ) ;
    else return 0 ;
}

Word *
Finder_FindWordFromAddress_AnyNamespace ( Finder * finder, byte * address )
{
    finder->FoundWord = Tree_Map_State_OneArg ( USING | NOT_USING,
        ( MapFunction_1 ) _Finder_CompareDefinitionAddress, ( int64 ) address ) ;
    CSL_WordAccounting ( ( byte* ) "Finder_Address_FindAny" ) ;
    return finder->FoundWord ;
}

Word *
Finder_FindWordFromAddress_AnyNamespace_NoAlias ( Finder * finder, byte * address )
{
    return finder->FoundWord = Tree_Map_State_OneArg ( USING | NOT_USING,
        ( MapFunction_1 ) _Finder_CompareDefinitionAddress_NoAlias, ( int64 ) address ) ;
}

void
Finder_SetQualifyingNamespace ( Finder * finder, Namespace * ns )
{
    //ns = Word_UnAlias ( ns0 ) ;
    finder->QualifyingNamespace = ns ;
}

void
Finder_ClearQualifyingNamespace ( Finder * finder )
{
    finder->QualifyingNamespace = 0 ;
}

void
Finder_SetNamedQualifyingNamespace ( Finder * finder, byte * name )
{
    finder->QualifyingNamespace = Namespace_Find ( name ) ;
}

Namespace *
Finder_GetQualifyingNamespace ( Finder * finder )
{
    return finder->QualifyingNamespace ;
}

Word *
_Finder_Word_Find ( Finder * finder, uint64 state, byte * name )
{
    finder->FoundWord = Tree_Map_State_OneArg ( state, ( MapFunction_1 ) Symbol_CompareName, ( int64 ) name ) ;
    CSL_WordAccounting ( ( byte* ) "Finder_Word_FindUsing" ) ;
    return finder->FoundWord ;
}

Word *
_Finder_FindWord_InOneNamespace ( Finder * finder, Namespace * ns, byte * name )
{
    if ( ns && name ) return finder->FoundWord = DLList_FindName_InOneNamespace ( ns, name ) ;
    return 0 ;
}
// QID : (q)ualified (id) identifer

Word *
Finder_QID_Find ( Finder * finder, byte * token ) //, int64 flag, int64 saveQns )
{
    Word * word = 0 ;
    if ( token )
    {
        if ( ! ( word = ( Word* ) _CSL_ParseQid_Token ( token ) ) ) word = Finder_Word_FindUsing ( finder, token, 1 ) ;
        CSL_WordAccounting ( ( byte* ) "Finder_Word_FindUsing" ) ;
    }
    return word ;
}

Word *
Finder_Word_Find ( Finder * finder, byte * name, int64 flag, Boolean saveQns )
{
    Word * rword = 0 ;
    Namespace * typeNamespace ;
    if ( name )
    {
        // needed for parsing C functions type declarations within a block
        // the InNamespace takes precedence with this one exception but is this the best logic ??               
        if ( finder->QualifyingNamespace )
        {
            if ( _String_EqualSingleCharString ( name, '.' ) || String_Equal ( name, "->" ) ) 
            {
                rword = _Finder_Word_Find ( finder, flag, name ) ;
                if ( rword ) return rword ;
            }
            else
            {
                Namespace * unAliasNs = Word_UnAlias ( finder->QualifyingNamespace ) ;
                rword = _Finder_FindWord_InOneNamespace ( finder, unAliasNs, name ) ;
            }
        }
        if ( ( ! rword ) && _Context_->QidInNamespace ) rword = _Finder_FindWord_InOneNamespace ( finder, _Context_->QidInNamespace, name ) ;
        if ( ( ! rword ) && _CSL_->InNamespace ) rword = _Finder_FindWord_InOneNamespace ( finder, _CSL_->InNamespace, name ) ;
        if ( ! rword ) rword = _Finder_Word_Find ( finder, flag, name ) ;
        if ( rword && ( rword->W_ObjectAttributes & ( C_TYPE | C_CLASS | NAMESPACE ) ) ) Finder_SetQualifyingNamespace ( finder, rword ) ;
        else if ( rword && ( rword->W_ObjectAttributes & ( OBJECT_FIELD ) ) && ( typeNamespace = TypeNamespace_Get ( rword ) ) ) Finder_SetQualifyingNamespace ( finder, typeNamespace ) ;
        else if ( ( ! saveQns ) && ( ! GetState ( finder, QID ) ) && ( ! Lexer_IsTokenForwardDotted ( _Context_->Lexer0 ) ) )
            Finder_ClearQualifyingNamespace ( finder ) ; // nb. QualifyingNamespace is only good for one find unless we are in a quid
    }
    return rword ;
}

Word *
Finder_Word_FindUsing ( Finder * finder, byte * name, Boolean saveQns )
{
    return Finder_Word_Find ( finder, name, USING, saveQns ) ;
}

Word *
Finder_Word_FindAny ( Finder * finder, byte * name, Boolean saveQns )
{
    return Finder_Word_Find ( finder, name, ANY, saveQns ) ;
}

#if 0

Word *
Finder_FindQualifiedIDWord ( Finder * finder, byte * token )
{
    Word * word ;
    while ( ( word = Finder_Word_FindUsing ( finder, token, 0 ) ) )
    {
        if ( word->W_ObjectAttributes & ( NAMESPACE ) )
        {
            Namespace * ns = ( Namespace * ) word ;
            Finder_SetQualifyingNamespace ( finder, ns ) ;
        }
        else if ( word->W_ObjectAttributes & ( OBJECT ) )
        {
            Finder_SetQualifyingNamespace ( finder, word->ContainingNamespace ) ;
        }
        else if ( word->W_ObjectAttributes & ( OBJECT_FIELD ) )
        {
            Finder_SetQualifyingNamespace ( finder, word->TypeNamespace ) ;
        }
        else return word ;
        if ( Lexer_IsTokenForwardDotted ( _Context_->Lexer0 ) )
        {
            Lexer_ReadToken ( _Context_->Lexer0 ) ; // the '.'
            token = Lexer_ReadToken ( _Context_->Lexer0 ) ; // the namespace
            continue ;
        }
        else return word ;
    }
    return 0 ;
}
#endif

byte *
Finder_GetTokenDefinitionAddress ( Finder * finder, byte * token )
{
    byte * definitionAddress = 0 ;
    if ( token )
    {
        Word * word = Finder_Word_FindUsing ( finder, token, 1 ) ;

        if ( word ) definitionAddress = ( byte* ) word->Definition ;
    }
    return definitionAddress ;
}

Word *
Finder_FindToken_WithException ( Finder * finder, byte * token )
{
    if ( Finder_Word_FindUsing ( finder, token, 0 ) == 0 )
    {
        iPrintf ( "\n%s ?", ( char* ) token ) ;
        CSL_Using ( ) ;
        CSL_Exception ( NOT_A_KNOWN_OBJECT, 0, QUIT ) ;
    }
    return finder->FoundWord ;
}

Word *
Finder_FindToken ( Finder * finder, byte * token )
{
    return Finder_FindToken_WithException ( finder, token ) ;
}

Word *
Finder_FindWord_UsedNamespaces ( Finder * finder, byte * name )
{
    return _Finder_Word_Find ( finder, USING, name ) ;
}

Word *
Finder_FindWord_AnyNamespace ( Finder * finder, byte * name )
{
    return _Finder_Word_Find ( finder, ANY, name ) ;
}

Word *
CSL_FindInAnyNamespace ( byte * name )
{
    return _Finder_Word_Find ( _Finder_, ANY, name ) ;
}

Word *
Finder_FindWord_InOneNamespace ( Finder * finder, byte *nsName, byte * name )
{
    return _Finder_FindWord_InOneNamespace ( finder, Namespace_Find ( nsName ), name ) ;
}

void
CSL_Find ( )
{
    DataStack_Push ( ( int64 ) Finder_FindToken ( _Context_->Finder0, ( byte* ) DataStack_Pop ( ) ) ) ;
}

void
CSL_Find_Using ( )
{
    Word * word = Finder_Word_Find ( _Context_->Finder0, ( byte* ) DataStack_Pop ( ), USING, 0 ) ;
    DataStack_Push ( ( int64 ) word ) ;
}

void
CSL_Find_Any ( )
{
    Word * word = Finder_Word_Find ( _Context_->Finder0, ( byte* ) DataStack_Pop ( ), ANY, 0 ) ;
    DataStack_Push ( ( int64 ) word ) ;
}

void
Context_ClearQualifyingNamespace ( )
{
    Finder_ClearQualifyingNamespace ( _Context_->Finder0 ) ;
}

void
Finder_Init ( Finder * finder )
{
    memset ( finder, 0, sizeof (Finder ) ) ;
}

Finder *
Finder_New ( uint64 allocationType )
{
    Finder * finder = ( Finder * ) Mem_Allocate ( sizeof (Finder ), allocationType ) ;
    Finder_Init ( finder ) ; // not needed assuming _Mem_Allocate returns clear mem
    return finder ;
}

