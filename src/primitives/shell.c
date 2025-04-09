
#include "../include/csl.h"

void
init_shell ( )
{
    struct termios shell_tmodes ;
    /* See if we are running interactively. */
    int shell_terminal = STDIN_FILENO ;

    int shell_is_interactive = isatty ( shell_terminal ) ;
    pid_t shell_pgid ;
    if ( shell_is_interactive )
    {
        /* Loop until we are in the foreground. */
        while ( tcgetpgrp ( shell_terminal ) != ( shell_pgid = getpgrp ( ) ) )
            kill ( - shell_pgid, SIGTTIN ) ;
        /* Ignore interactive and job-control signals. */
        signal ( SIGINT, SIG_IGN ) ;
        signal ( SIGQUIT, SIG_IGN ) ;
        signal ( SIGTSTP, SIG_IGN ) ;
        signal ( SIGTTIN, SIG_IGN ) ;
        signal ( SIGTTOU, SIG_IGN ) ;
        signal ( SIGCHLD, SIG_IGN ) ;

        // */

        /* Put ourselves in our own process group. */
        shell_pgid = getpid ( ) ;
        if ( setpgid ( shell_pgid, shell_pgid ) < 0 )
        {
            perror ( "\nCouldn't put the shell in its own process group" ) ;
            //exit ( 1 ) ;
        }
        /* Grab control of the terminal. */
        tcsetpgrp ( shell_terminal, shell_pgid ) ;
        /* Save default terminal attributes for shell. */
        tcgetattr ( shell_terminal, &shell_tmodes ) ;
    }
}

void
shell ( )
{
    //pid_t pid ;
    Context * cntx = _Context_ ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * svPrompt = ReadLine_GetPrompt ( rl ) ;
    ReadLine_SetPrompt ( rl, "$ " ) ;
    iPrintf ( " shell : type \'.\' to exit" ) ;

    pid_t pid, shell_pgid, tcgid ;
    int shell_is_interactive = isatty ( STDIN_FILENO ) ;
    if ( shell_is_interactive )
    {
        /* Loop until we are in the foreground. */
        while (1) 
        { 
            tcgid = tcgetpgrp ( STDIN_FILENO ) ;
            shell_pgid = getpgrp ( ) ;
            if ( tcgid == shell_pgid ) break ;
            else kill ( - shell_pgid, SIGTTIN ) ;
        }
         /* Ignore interactive and job-control signals. */
        signal ( SIGINT, SIG_IGN ) ;
        signal ( SIGQUIT, SIG_IGN ) ;
        signal ( SIGTSTP, SIG_IGN ) ;
        signal ( SIGTTIN, SIG_IGN ) ;
        signal ( SIGTTOU, SIG_IGN ) ;
        signal ( SIGCHLD, SIG_IGN ) ;
        /* Put ourselves in our own process group. */
        pid = getpid ( ) ;
        setpgid ( pid, shell_pgid ) ; // ignore error here
#if 0        
        if ( setpgid ( pid, shell_pgid ) < 0 )
        {
            perror ( "\nCouldn't put the shell in its own process group" ) ;
            //exit ( 1 ) ;
            goto done ;
        }
#endif        
        /* Grab control of the terminal. */
        tcsetpgrp ( STDIN_FILENO, shell_pgid ) ;
    }
    
    while ( 1 )
    {
        _DoPrompt ( ) ;
        _ReadLine_GetLine ( rl, 0 ) ;
        byte * str = & rl->InputLine [0] ;
        if ( String_Equal ( str, ".\n" ) ) goto done ;
        // Create pipe between process
        int fd[2] ;
        pipe ( fd ) ;
        pid = fork ( ) ;
        if ( pid < 0 )
        {
            perror ( "fork" ) ;
            exit ( 0 ) ;
        }
        else if ( pid == 0 )
        {
            //Child
            signal ( SIGINT, SIG_DFL ) ;
            signal ( SIGQUIT, SIG_DFL ) ;
            signal ( SIGTSTP, SIG_DFL ) ;
            signal ( SIGTTIN, SIG_DFL ) ;
            signal ( SIGTTOU, SIG_DFL ) ;
            signal ( SIGCHLD, SIG_DFL ) ;
            setpgid ( 0, pid ) ;
            // Close reading pipe 
            close ( fd[0] ) ;
            //if ( execv ( args[0], args ) < 0 )
            if ( execl ( "/bin/bash", "bash", "-i", "-c", str, ( char * ) NULL ) )
            {
                perror ( str ) ;
                exit ( 0 ) ;
            }
        }
        //?? i don't fully understand this stuff yet but this works    
        else
        {
            //Parent
            signal ( SIGCHLD, handle_sigchld ) ;
            setpgid ( pid, pid ) ;

            int status ;
            // Send job to fg
            tcsetpgrp ( 0, pid ) ;
            // Wait it finish
            waitpid ( pid, &status, WUNTRACED ) ;
            // return shell to fg
            signal ( SIGTTOU, SIG_IGN ) ;
            //tcsetpgrp ( 0, getpid ( ) ) ; // original
            tcsetpgrp ( 0, shell_pgid ) ; // libc init_shell
            signal ( SIGTTOU, SIG_DFL ) ;
            // Close writing pipe
            close ( fd[1] ) ;
        }
    }
done:
    ReadLine_SetPrompt ( rl, svPrompt ) ;
    Lexer_Init ( _Lexer_, 0, 0, CONTEXT ) ;
    iPrintf ( " leaving shell ..." ) ;
}

void
handle_sigchld ( int sig )
{
    pid_t pid ;
    int status ;
    pid = waitpid ( - 1, &status, ( WNOHANG | WUNTRACED | WCONTINUED ) ) ;
}

#if 0 // original unedited shell.c from ... thank you!
MIT License

Copyright ( c ) 2021 Luiz Santos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files ( the "Software" ), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
#endif