
#include "../include/csl.h"

void
LogicalAnd ( ) // and
{
    _DspReg_ [ - 1 ] = _DspReg_ [ - 1 ] && _DspReg_ [ 0 ] ; //TOS ;
    //DataStack_Drop ( ) ;
    _DspReg_ -= 1 ;

}

void
CSL_Power_03 ( ) // **
{
    int64 pow = _DspReg_ [ 0 ], base = _DspReg_ [ -1 ], n ;
    for ( n = base ; -- pow ; )
    {
        n *= base ;
    }
    _DspReg_ [ -1 ] = n ;
    DataStack_Drop ( ) ;
}
 
int64
_CFib_O3 ( int64 n )
{
    if ( n < 2 ) return n ;
    else return ( _CFib_O3 ( n - 1 ) + _CFib_O3 ( n - 2 ) ) ; 
}

void
CFib_O3 ( )
{
    TOS = ( _CFib_O3 ( TOS ) ) ;
}

int64
_CFib2_O3 ( int64 n )
{
    int64 fn, fn1, fn2 ;
    for ( fn = 0, fn1 = 0, fn2 = 1 ; n ; n-- ) 
    {   
        fn1 = fn2 ; 
        fn2 = fn ;
        fn = fn1 + fn2 ; 
    }
    return fn ;
}

void
CFib2_O3 ( )
{
    TOS = ( _CFib2_O3 ( TOS ) ) ;
}

void
CFactorial_O3 ( )
{
    int64 n = TOS ;
    if ( n > 1 )
    {
        TOS = TOS - 1 ;
        CFactorial_O3 ( ) ;
        TOS *= n ;
    }
    else TOS = 1 ;
}

int64
_CFactorial_O3 ( int64 n )
{
    if ( n > 1 ) return ( n * _CFactorial_O3 ( n - 1 ) ) ;
    else return 1 ;
}

void
CFactorial2_O3 ( )
{
    TOS = ( _CFactorial_O3 ( TOS ) ) ;
}

void
CFactorial3_O3 ( void )
{
    int64 rec1 = 1, n = TOS ;
    while ( n > 1 ) rec1 *= n -- ;
    TOS = rec1 ;
}

void
CFactorial4_O3 ( void ) 
{
    int64 rec = 1, n = TOS ;
    if ( n > 1 )
   {
        do
        { 
            rec = rec * n ;  n-- ; 
        } 
        while ( n > 1 ) ;
    }
    TOS = rec ;
}

int64
_CFib ( int64 n )
{
    if ( n < 2 ) return n ;
    else return ( _CFib ( n - 1 ) + _CFib ( n - 2 ) ) ;
}

void
CFib ( )
{

    TOS = ( _CFib ( TOS ) ) ;
}

void
CSL_Power ( ) // **
{
    int64 pow = _DspReg_ [ 0 ], base = _DspReg_ [ - 1 ], n ;
    for ( n = base ; -- pow ; )
    {

        n *= base ;
    }
    _DspReg_ [ - 1 ] = n ;
    DataStack_Drop ( ) ;
}

void
CFactorial ( )
{
    int64 n = TOS ;
    if ( n > 1 )
    {
        TOS = TOS - 1 ;
        CFactorial ( ) ;
        TOS *= n ;
    }

    else TOS = 1 ;
}

int64
_CFactorial ( int64 n )
{
    if ( n > 1 ) return ( n * _CFactorial ( n - 1 ) ) ;

    else return 1 ;
}

void
CFactorial2 ( )
{

    TOS = ( _CFactorial ( TOS ) ) ;
}

void
CFactorial3 ( void )
{
    int64 rec1 = 1, n = TOS ;
    while ( n > 1 )
    {

        rec1 *= n -- ;
    }
    TOS = rec1 ;
}


int64
fd ( int koc)
{
   int64 rtrn  = 0 ;
   if ( koc == 1) rtrn = 1 ;
   else if ( koc == 2) rtrn = 5 ;
   else if ( koc == 3) rtrn = 10 ;
   else if ( koc == 4) rtrn = 25 ;
   else if ( koc == 5) rtrn = 50 ;
   return rtrn ;
}
//pwi fd sp0
/*
(define (cc amount kinds-of-coins)                                                                                                                        
  (cond ((= amount 0) 1)                                                                                                                                  
        ((or (< amount 0) (= kinds-of-coins 0)) 0)                                                                                                        
        (else (+ (cc amount                                                                                                                               
                     (- kinds-of-coins 1))                                                                                                                
                 (cc (- amount (first-denomination kinds-of-coins))
                     kinds-of-coins)
))))
*/

int64
cc1 ( int64 amount, int64 koc)
{
    int64 rtrn = 0 ;
    if (amount == 0) rtrn = 1  ;
    else if ( ( amount < 0) || ( koc == 0) ) rtrn = 0 ;
    else rtrn = cc1 ( amount, koc - 1) + cc1 (amount - fd (koc), koc) ;
    //printf ( "\namount = %ld : koc = %ld :: rtrn = %ld", amount, koc, rtrn ) ;
    return rtrn ;
}
//pwi cc1 

int64 
countChangec ( int64 amount)
{
  int64 rtrn = cc1 ( amount, 5)  ;
  return rtrn ;
}
//pwi countchange
void
CSL_CountChange ()
{
    int64 value0 ;
    value0 = countChangec ( TOS ) ;
    printf ( "\n%ld", value0 ) ;
    fflush ( stdout ) ;
}
