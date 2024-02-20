
#include "../../include/csl.h"

void
TimerInit ( struct timespec * timer )
{
    clock_gettime ( CLOCK_REALTIME, timer ) ;
    //clock_gettime ( CLOCK_MONOTONIC, &system->Timers [ i ] ) ;
}

void
_System_TimerInit ( System * system, int64 i )
{
    clock_gettime ( CLOCK_REALTIME, &system->Timers [ i ] ) ;
    //clock_gettime ( CLOCK_MONOTONIC, &system->Timers [ i ] ) ;
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
    //clock_settime ( CLOCK_MONOTONIC, &system->Timers [ timer ] ) ;
    //clock_settime ( CLOCK_REALTIME, &system->Timers [ timer ] ) ;
    if ( nseconds > nseconds2 )
    {
        seconds2 -- ;
        nseconds2 += 1000000000 ;
    }
    sprintf ( ( char* ) toString, format, seconds2 - seconds, nseconds2 - nseconds ) ;
}

void
_System_Time ( System * system, uint64 timer, char * format, byte * toString )
{
    if ( timer < 8 )
    {
        __time_t seconds, seconds2 ;
        int64 nseconds, nseconds2 ;
        seconds = system->Timers [ timer ].tv_sec ;
        nseconds = system->Timers [ timer ].tv_nsec ;
        _System_TimerInit ( system, timer ) ;
        seconds2 = system->Timers [ timer ].tv_sec ;
        nseconds2 = system->Timers [ timer ].tv_nsec ;
        //clock_settime ( CLOCK_MONOTONIC, &system->Timers [ timer ] ) ;
        //clock_settime ( CLOCK_REALTIME, &system->Timers [ timer ] ) ;
        if ( nseconds > nseconds2 )
        {
            seconds2 -- ;
            nseconds2 += 1000000000 ;
        }
        sprintf ( ( char* ) toString, format, seconds2 - seconds, nseconds2 - nseconds ) ;
    }
    else Throw ( ( byte* ) "_System_Time : ", ( byte* ) "Error: timer index must be less than 8\n", QUIT ) ;
}

void
System_Time ( System * system, uint64 timer, char * string, int64 tflag )
{
    byte buffer [ 64 ] ;
    _System_Time ( system, timer, ( char* ) "%ld.%09ld", buffer ) ;
    if ( tflag && ( Verbosity () ) ) iPrintf ( "%s [ %d ] : elapsed time = %s seconds\n", string, timer, buffer ) ;
}

void
OVT_Time ( char * string, int64 tflag )
{
    byte buffer [ 64 ] ;
    //_System_Time ( system, timer, ( char* ) "%ld.%09ld", buffer ) ;
    Time ( &_O_->Timer,  ( char* ) "%ld.%09ld", buffer ) ;
    if ( tflag && ( Verbosity () ) ) iPrintf ( "%s : elapsed time = %s seconds", string, buffer ) ;
}

void
System_InitTime ( System * system )
{
    int64 i ;
    for ( i = 0 ; i < 8 ; i ++ ) _System_TimerInit ( system, i ) ;
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


