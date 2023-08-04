
#include "../include/csl.h"

void
CSL_Class_New ( void )
{
    byte * name ;
    if ( C_SyntaxOn ) CSL_Token ( ) ; // for 'struct'
    name = ( byte* ) DataStack_Pop ( ) ;
    DataObject_New ( CLASS, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void //Word *
CSL_Class_Object_New ( )
{
    //if ( C_SyntaxOn ) CSL_Token ( ) ; // for 'struct'
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    Word * word = DataObject_New ( OBJECT, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void
CSL_Class_Clone ( void )
{
    //if ( C_SyntaxOn ) CSL_Token ( ) ; // for 'struct'
    byte * name = ( byte* ) DataStack_Pop ( ) ;
    DataObject_New ( CLASS_CLONE, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

void
CSL_DObject_New ( )
{
    // clone DObject -- create an object with DObject as it's prototype (but not necessarily as it's namespace)
    DataObject_New ( DOBJECT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
}

Namespace *
CSL_Type_New ( )
{
    byte * name ;
    Namespace * ns = 0 ;
    CSL_Token ( ) ;
    name = ( byte* ) DataStack_Pop ( ) ;
    if ( ( String_Equal ( "struct", name ) ) || ( String_Equal ( "class", name ) ) )
    {
        CSL_Token ( ) ;
        name = ( byte* ) DataStack_Pop ( ) ;
    }
    ns = DataObject_New ( C_TYPE, 0, name, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ;
    return ns ;
}

int64
Type_Create ( )
{
    int64 size = 0 ;
    byte * token = Lexer_ReadToken ( _Lexer_ ) ; // 
    if ( token [ 0 ] == '{' ) size = TDI_Type_Field () ; //Namespace_ActivateAsPrimary ( ( byte* ) "C_Typedef" ) ;
    return size ;
}

void
_CSL_Typedef (Word * word)
{
    int64 size = TDI_Type_Field () ;
    //DataStack_Push ( size ) ;
}

void
CSL_Typedef ( )
{
    //int64 size = CSL_TD_A_Typed_Field (0, 0, 0) ; // really, yes, but 
    //we want to route all new object creation thru DataObject_New for understandability and debugging
    DataObject_New ( C_TYPEDEF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, - 1 ) ; //--> _CSL_TypeDef ( ) ;
}


