#include "../../include/csl.h"

// we don't want the classes semantics when interpreting

void
CSL_ClassStructureEnd ( void )
{
    Namespace_RemoveFromUsingList_WithCheck ( ( byte* ) "Class" ) ;
}

void
Object_Size_Set ( Namespace * classNs, int64 size )
{
    //static svClassNs ;
    if ( classNs && size )
    {
        //if ( classNs->ObjectByteSize != size )
        {
            _Namespace_VariableValueSet ( classNs, ( byte* ) "size", size ) ;
            classNs->ObjectByteSize = size ;
            //dbg (4, classNs, size ) ;
        }
    }
}

void
_TypeClassStructDef (Namespace * ns, Boolean cloneFlag)
{
    byte * token = _Context_->CurrentEvalWord ? _Context_->CurrentEvalWord->Name : (byte*) "" ; //, *name = ns ? ns->Name : 0 ;
    if ( String_Equal ( token, "+:{" ) ) { cloneFlag = 1 ; } //Pause () ; }
    //else cloneFlag = 0 ;
    iPrintf ( "\n_ClassTypedef : at %s : token = %s : cloneFlag = %b", Context_Location(), token, cloneFlag ) ;
    TDI * tdi = TDI_Start ( ns, 0, ( cloneFlag ? TDI_CLONE_FLAG : 0 ) ) ;
    if ( cloneFlag || String_Equal ( token, "class" ) )
    {
        TD_Struct_Union_Def ( TDI_STRUCT ) ;
    }
    else 
    {
        tdi->TdiToken = token ;
        TD_Field ( TDI_STRUCT ) ;
    }
    tdi = TDI_Finalize ( ) ;
}

void
CSL_CloneStructureBegin ( void )
{
    _TypeClassStructDef (0, 0) ;
}

void
CSL_ClassTypedef ( )
{
    _TypeClassStructDef (0, 0) ;
}
