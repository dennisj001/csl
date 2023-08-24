#define LO_Last( lo ) (ListObject*) dllist_Last ( (dllist*) lo->Lo_List )
#define LO_Previous( lo ) ( ListObject* ) dlnode_Previous ( ( dlnode* ) lo )
#define LO_Next( lo ) (( ListObject* ) dlnode_Next ( ( dlnode* ) lo ))
#define LO_AddToTail( lo, lo1 ) dllist_AddNodeToTail ( (( ListObject * ) lo)->Lo_List, ( dlnode* ) (lo1) ) 
#define LO_AddToHead( lo, lo1 ) dllist_AddNodeToHead ( (( ListObject * ) lo)->Lo_List, ( dlnode* ) (lo1) ) 
#define LO_New( lType, object ) (ListObject *) DataObject_New (T_LC_NEW, 0, 0, 0, 0, lType, 0, (int64) object, 0, 0, 0 , -1)
#define LambdaArgs( proc ) proc->p[0]
#define LambdaProcedureBody( proc ) proc->p[1]
#define LambdaVals( proc ) proc->p[2]
#define LISP_ALLOC (_LC_ ? (GetState (_LC_, (LC_DEFINE_MODE|LC_READ)) ? LISP : LISP_TEMP) : LISP_TEMP)
#define LO_CopyOne( l0 ) _LO_CopyOne ( l0, LISP_ALLOC )
#define LO_Copy( l0 ) _LO_Copy ( l0, LISP_ALLOC )
#define nil (_LC_ ? _LC_->Nil : 0)
#define LC_SaveStackPointer( lc ) _LC_SaveDsp ( lc )
#define LC_RestoreStackPointer( lc ) _LC_ResetStack ( lc ) 
#define Lisp_Alloc( size ) Mem_Allocate ( size, LISP )

#define LC_snprintf2( buffer, format, value1, value2 ) snprintf ( ( char* ) buffer, BUFFER_IX_SIZE, ((char*) (format)), value1, value2 ) 
#define LC_snprintf1( buffer, format, value ) snprintf ( ( char* ) buffer, BUFFER_IX_SIZE, ((char*) (format)), value ) 
#define LC_sprintf_String( buffer, format, str ) LC_snprintf1 ( buffer, (format), ((char*) str) ) 
#define LC_sprintString( buffer, str ) LC_sprintf_String (buffer, " %s", (str) ) 
#define LC_sprintAString( buffer, str ) LC_sprintString (buffer, (str) ) 
#define LC_sprintName( buffer, l0 ) LC_sprintString ( buffer, l0->Lo_Name ) 

#define csl_eq( a, b ) String_Equal ( a, b )
#define csl_cons_p(lo) ( lo->W_LispAttributes & ( LIST | LIST_NODE ) ) 
#define csl_car( lo ) csl_cons_p(lo)?  dllist_First ( ( dllist* ) ( dllist * ) l0->Lo_List ) : 0
#define csl_cdr( lo ) dlnode_Next ( ( dlnode* ) lo )
//#define lc_eval( lo ) LO_CopyOne ( LC_Eval ( lo, 0, 1) )
#define lc_eval( lo ) LC_Eval ( lo, 0, 1) 
#define _LO_Car( lo ) LO_First (lo)
#define _LO_Cdr( lo ) LO_Next ( _LO_Car( lo ))













