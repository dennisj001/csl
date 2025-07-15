
#include "../../include/csl.h"

void
TimerInit ( struct timespec * timer )
{
    clock_gettime ( CLOCK_REALTIME, timer ) ;
}

void
_System_TimerInit ( System * system, int64 i )
{
    TimerInit ( &system->Timers [ i ] ) ;
}

void
System_InitTime ( System * system )
{
    int64 i ;
    for ( i = 0 ; i < 8 ; i ++ ) _System_TimerInit ( system, i ) ;
}

void
Time ( struct timespec * timer, char * format, byte * toString )
{
    __time_t seconds, seconds2 ;
    int64 nseconds, nseconds2 ;
    seconds = timer->tv_sec ;
    nseconds = timer->tv_nsec ;
    TimerInit ( timer ) ;
    seconds2 = timer->tv_sec ;
    nseconds2 = timer->tv_nsec ;
    //clock_settime ( CLOCK_REALTIME, &system->Timers [ timer ] ) ;
    if ( nseconds > nseconds2 )
    {
        seconds2 -- ;
        nseconds2 += 1000000000 ;
    }
    sprintf ( ( char* ) toString, format, seconds2 - seconds, nseconds2 - nseconds ) ;
}

void
_CSL_Time ( struct timespec * timer, uint64 utimer, char * string )
{
    byte buffer [ 256 ] ;
    Time ( timer, ( char* ) "%ld.%09ld", buffer ) ;
    if ( Verbosity ( ) )
    {
        if ( utimer ) iPrintf ( "\n%s [ %d ] : elapsed time = %s seconds at %s", string, (utimer & 0xf), buffer, Context_Location ( ) ) ;
        else iPrintf ( "\n%s : elapsed time = %s seconds at %s", string, buffer, Context_Location ( ) ) ;
    }
    //_O_->Pbf8[0] = '\n' ;
    SetPromptNewline ( ) ;
}

void
System_Time ( System * system, uint64 utimer, char * string )
{
    if ( utimer < 8 ) _CSL_Time ( &system->Timers [ (utimer ) ], (utimer + 0x10 ), string ) ; // so we can use timer 0
    else Throw ( ( byte* ) "_System_Time : ", ( byte* ) "Error: timer index must be less than 8\n", QUIT ) ;
}

void
OVT_Time ( char * string )
{
    _CSL_Time ( &_O_->Timer, 0, string ) ;
}

void
_System_Copy ( System * system, System * system0 )
{
    MemCpy ( system, system0, sizeof (System ) ) ;
}

System *
System_Copy ( System * system0, uint64 type )
{
    System * system = ( System * ) Mem_Allocate ( sizeof ( System ), type ) ;
    _System_Copy ( system, system0 ) ;
    return system ;
}

// BigNumWidth is a parameter to mpfr_printf; it works like printf and sets minimum number of characters to print

void
_System_Init ( System * system )
{
    system->NumberBase = 10 ;
}

void
System_Init ( System * system )
{
    system->State = 0 ;
    SetState ( system, ADD_READLINE_TO_HISTORY | ALWAYS_ABORT_ON_EXCEPION, true ) ;
    _System_Init ( system ) ;
}

System *
System_New ( uint64 type )
{
    System * system = ( System * ) Mem_Allocate ( sizeof (System ), type ) ;
    System_Init ( system ) ;
    System_InitTime ( system ) ;
    return system ;
}


