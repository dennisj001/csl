
#include "../../include/csl.h"

void
_Symbol_NameInit ( Symbol * symbol, byte * name )
{
    symbol->S_Name = name ;
}

void
_Symbol_Init_AllocName ( Symbol * symbol, byte * name, uint64 allocType )
{
    if ( name )
    {
        byte* sname = String_New ( name, allocType ) ;
        _Symbol_NameInit ( symbol, sname ) ;
    }
}

// doesn't allocate name

Symbol *
__Symbol_New ( uint64 allocType )
{
    return ( Symbol * ) Mem_Allocate ( sizeof (Symbol ), allocType ) ;
}

Symbol *
_Symbol_New ( byte * name, uint64 allocType )
{
    Symbol * symbol = __Symbol_New ( allocType ) ;
    _Symbol_Init_AllocName ( symbol, name, allocType ) ;
    return symbol ;
}

// doesn't allocate name

Symbol *
Symbol_New ( byte * name )
{
    return _Symbol_New ( name, DICTIONARY ) ;
}

Symbol *
Symbol_NewValue ( int64 value, uint64 allocType )
{
    Symbol * sym = __Symbol_New ( allocType ) ;
    sym->W_Value = value ;
    return sym ;
}

Symbol *
Symbol_CompareName2 ( Symbol * symbol, byte * name, Namespace * ns )
{
    //d1 ( if ( Verbosity () > 3 ) _Printf ( (byte*) "\n symbol name = %s : name = %s", symbol->S_Name, name ) ) ;
    d0 ( if ( Is_DebugOn && String_Equal ( symbol->S_Name, "int" ) ) { iPrintf ( "\n symbol name = %s : name = %s", symbol->S_Name, name ) ; Pause ( ) ; } ) ;
    d0 ( if ( Is_DebugOn ) { iPrintf ( "\n symbol name = %s.%s : name = %s", symbol->S_ContainingNamespace ? symbol->S_ContainingNamespace->Name : ( byte* ) "", symbol->S_Name, name ) ; } ) ; //Pause () ; } ) ;
    if ( name && symbol && symbol->S_Name && ( String_Equal ( symbol->S_Name, name ) ) && (symbol->S_ContainingNamespace == ns) )
    {
        d0 ( if ( Is_DebugOn ) { iPrintf ( "\n FOUND  : symbol name = %s.%s : name = %s", symbol->S_ContainingNamespace ? symbol->S_ContainingNamespace->Name : ( byte* ) "", symbol->S_Name, name ) ; } ) ; //Pause () ; } ) ;
        return symbol ;
    }
    else return 0 ;
}

Symbol *
Symbol_CompareName ( Symbol * symbol, byte * name )
{
    //d1 ( if ( Verbosity () > 3 ) _Printf ( (byte*) "\n symbol name = %s : name = %s", symbol->S_Name, name ) ) ;
    //d0 ( if ( Is_DebugOn && String_Equal ( symbol->S_Name, "int" ) ) { _Printf ( "\n symbol name = %s : name = %s", symbol->S_Name, name ) ; Pause ( ) ; } ) ;
    //d0 ( if ( Is_DebugOn ) { _Printf ( "\n symbol name = %s.%s : name = %s",
    //        symbol->S_ContainingNamespace ? symbol->S_ContainingNamespace->Name : ( byte* ) "", symbol->S_Name, name ) ; } ) ; //Pause () ; } ) ;
    if ( symbol && symbol->S_Name && ( String_Equal ( symbol->S_Name, name ) ) )
    {
        //d0 ( if ( Is_DebugOn ) { _Printf ( "\n FOUND : symbol name = %s.%s : name = %s",
        //    symbol->S_ContainingNamespace ? symbol->S_ContainingNamespace->Name : ( byte* ) "", symbol->S_Name, name ) ; } ) ; //Pause () ; } ) ;
        return symbol ;
    }
    else return 0 ;
}
#if 0
Symbol *
Symbol_CompareName ( Symbol * symbol, byte * name )
{
    Symbol * symbol1 ;
    if ( symbol1 = Symbol_CompareName ( symbol, name ) )
    {
#if 0        
        Word * word = (Word*) symbol1 ;
        return word ; //= Word_UnAlias ( word ) ;
#else        
        return symbol ; // remember Symbol == Word == Namespace, etc.
#endif        
    }
    return 0 ;
}
#endif
