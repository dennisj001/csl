
#include "../include/csl.h"
#if 0
AtomStringNode *
_dllist_Atom_Intern ( dllist * dllist, byte * name )
{
    AtomStringNode * asn = ( AtomStringNode * ) Mem_Allocate ( sizeof ( AtomStringNode ), ATOM_MEMORY, 0 ) ;
    name = String_New ( name, ATOM_MEMORY ) ;
    _Symbol_Init ( ( Symbol* ) asn, name ) ;
    asn->S_Attribute = STRING ;
    dllist_AddNodeToTail ( dllist, ( dlnode* ) asn ) ;
    return asn ;
}

byte *
_dllist_Atom_Find ( dllist * dllist, register byte * name )
{
    register byte * lname ;
    AtomStringNode * asn ;
    register dlnode * tnode ;
    if ( name && name [0] )
    {
        for ( tnode = dllist_First ( (dllist*) dllist ) ; tnode ; tnode = dlnode_Next ( tnode ) ) // index = dlnode_NextNode ( &_Q->AtomList, (dlnode *) index ) )
        {
            asn = ( AtomStringNode* ) tnode ;
            lname = asn->S_Name ;
            if ( lname && ( string_Equal ( lname, name ) ) )
            {
                return lname ;
            }
        }
    }
    return 0 ;
}

AtomStringNode *
_Atom_Intern ( byte * name )
{
    return _dllist_Atom_Intern ( _O_->AtomList, name ) ;
}

byte *
_Atom_Find ( byte * name )
{
    return _dllist_Atom_Find ( _O_->AtomList, name ) ;
}

byte *
_dllist_Atomize ( dllist * dllist, byte * istring )
{
    AtomStringNode * asn ;
    byte * atom ;
    if ( istring ) //&& strcmp ( ( char* ) istring, "" ) ) // don't add blank lines to history
    {
        atom = _dllist_Atom_Find ( dllist, istring ) ;
        if ( ! atom )
        {
            asn = _dllist_Atom_Intern ( dllist, istring ) ;
            return asn->S_Name ;
        }
        else return atom ;
    }
    return 0 ;
}

byte *
_OpenVmTil_Atomize ( byte * istring )
{
    return _dllist_Atomize ( _O_->AtomList, istring ) ;
}

/*
void
OpenVmTil_Atom ( )
{
    byte * string = ( byte* ) _DataStack_Pop ( ) ;
    _OpenVmTil_Atom ( string ) ;
}
 */
#endif