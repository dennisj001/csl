
#include "../include/csl.h"

void
shell ( )
{
    pid_t pid ;
    Context * cntx = _Context_ ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * svPrompt = ReadLine_GetPrompt ( rl ) ;
    ReadLine_SetPrompt ( rl, "$ " ) ;
    iPrintf ( " shell : type \'.\' to exit" ) ;

    // Defining signal handles
    signal ( SIGINT, SIG_IGN ) ;
    signal ( SIGTSTP, SIG_IGN ) ;

    // Defining pgid
    pid = getpid ( ) ;
    setpgid ( pid, pid ) ;
    tcsetpgrp ( 0, pid ) ;

    while ( 1 )
    {
        _O_->Pbf8 [0] = 0 ; //reset for prompt
        Context_DoPrompt ( cntx ) ;
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
            break ;
        }
#if 1  //?? i don't fully understand this stuff yet but this works    
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
            tcsetpgrp ( 0, getpid ( ) ) ;
            signal ( SIGTTOU, SIG_DFL ) ;
            // Close writing pipe
            close ( fd[1] ) ;
        }
        _O_->Pbf8 [0] = 0 ; //reset for prompt
#elif 0    
        else 
        {
            int status ;
            signal ( SIGCHLD, handle_sigchld ) ;
            setpgid ( pid, pid ) ;
            tcsetpgrp ( 0, pid ) ;
            waitpid ( pid, &status, WUNTRACED ) ;
            signal ( SIGTTOU, SIG_IGN ) ;
            tcsetpgrp ( 0, getpid ( ) ) ;
            signal ( SIGTTOU, SIG_DFL ) ;
            close ( fd[1] ) ;
        }
#endif      
    }
done:
    ReadLine_SetPrompt ( rl, svPrompt ) ;
    iPrintf ( " leaving shell ..." ) ;
    CSL_Prompt (_CSL_, 1, 1 ) ;
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