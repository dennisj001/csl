
#include "../include/csl.h"
void
shell ( )
{
    pid_t pid ;
    byte * cmd = Buffer_New_pbyte ( BUFFER_SIZE ) ;
    char *args[100], cmd_aux[200], *sptr1, *shmem ; 
    
    int t_process, len ;

    Context * cntx = _Context_ ;
    ReadLiner * rl = cntx->ReadLiner0 ;
    byte * svPrompt = ReadLine_GetPrompt ( rl ) ;
    ReadLine_SetPrompt ( rl, "$ " ) ;
    iPrintf ( "\n type \'.\' to exit" ) ;


    // Create shared memory
#define shmemSize 200    
    //nb! : must be shared mem
    shmem = mmap ( NULL, shmemSize, ( PROT_READ | PROT_WRITE ), ( MAP_SHARED | MAP_ANONYMOUS ), - 1, 0 ) ;
    //shmem = Mem_Allocate ( shmemSize, TEMPORARY ) ;

    // Defining signal handles
    //signal ( SIGINT, handle_sigint ) ;
    //signal ( SIGTSTP, handle_sigtstp ) ;
    signal ( SIGINT, SIG_IGN ) ;
    signal ( SIGTSTP, SIG_IGN ) ;

    // MYPATH
    setenv ( "MYPATH", getenv ( "PATH" ), 1 ) ;

    // Defining pgid
    pid = getpid ( ) ;
    setpgid ( pid, pid ) ;
    tcsetpgrp ( 0, pid ) ;

    while ( 1 )
    {
        // Create empty linked list to parse commands
        cnode *cmds, *ptr ;
        cmds = NULL ;
        // Start new shell input

        Context_DoPrompt ( cntx ) ;
        _ReadLine_GetLine ( rl, 0 ) ;
        byte * str = & rl->InputLine [0] ; //rl->ReadIndex] ; //String_New ( & rl->InputLine [rl->ReadIndex], TEMPORARY ) ;
        if ( String_Equal ( str, ".\n" ) ) goto done ; //return ; //break ;
        if ( get_type_process ( str ) == 0 )
        {
            strcpy ( cmd, "bash -i -c " ) ;
            strcat ( cmd, str ) ;
        }
        else strcpy ( cmd, str ) ;

        len = strlen ( cmd ) ;
        cmd[-- len] = '\0' ;

        // Ignore empty cmds
        if ( len == 0 ) continue ;
#if 1
        // Parse commands and insert it on a Linked List
        char * _cmd ;
        strcpy ( cmd_aux, cmd ) ;
        _cmd = strtok_r ( cmd_aux, "|", &sptr1 ) ;
        while ( _cmd != NULL )
        {
            add_cmd ( &cmds, _cmd ) ;
            _cmd = strtok_r ( NULL, "|", &sptr1 ) ;
        }
#endif        
        // Execute commands
        int fdd = 0 ;
        int fd[2] ;
        ptr = cmds ;
        while ( ptr != NULL )
        {
            // Create pipe between process
            pipe ( fd ) ;
            t_process = get_type_process ( ptr->cmd ) ;
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
                //PGID
                setpgid ( 0, pid ) ;
                // Child process gets input from output of previous process
                // if exists, else it gets input from stdin
                dup2 ( fdd, 0 ) ;
                // If there is a next process, change it output to the pipe
                // between them, else output is stdout
                if ( ptr->prox != NULL )
                {
                    dup2 ( fd[1], 1 ) ;
                }
                // Close reading pipe 
                close ( fd[0] ) ;
                // Arguments parser
#if 1                
                if ( parse_args ( ptr->cmd, args, t_process ) < 0 )
                {
                    perror ( args[0] ) ;
                    exit ( 0 ) ;
                }
#endif                
                // Process command
                switch ( t_process )
                {
                    case 2:
                    {
                        _cd ( args, shmem ) ;
                        break ;
                    }
                    case 0:
                    {
                        if ( execv ( args[0], args ) < 0 )
                        {
                            perror ( args[0] ) ;
                            exit ( 0 ) ;
                        }
                        break ;
                    }
                }
            }
            else
            {
                //Parent
                signal ( SIGCHLD, handle_sigchld ) ;
                setpgid ( pid, pid ) ;

                int status ;
                //{
                {
                    // Foreground
                    // Send job to fg
                    tcsetpgrp ( 0, pid ) ;
                    // Wait it finish
                    int status ;
                    waitpid ( pid, &status, WUNTRACED ) ;
                    // return shell to fg
                    signal ( SIGTTOU, SIG_IGN ) ;
                    tcsetpgrp ( 0, getpid ( ) ) ;
                    signal ( SIGTTOU, SIG_DFL ) ;
                    // Close writing pipe
                    close ( fd[1] ) ;
                    // Save the actual pipe for the next command parsed
                    fdd = fd[0] ;
                    if ( WIFEXITED ( status ) )
                    {
                        if ( t_process == 2 )
                        {
                            // cd
                            if ( chdir ( shmem ) != - 1 )
                            {
                                setenv ( "PWD", shmem, 1 ) ;
                            }
                            else perror ( "cd" ) ;
                        }
                    }
                    else if ( WIFSTOPPED ( status ) )
                    {
                        fprintf ( stdout, "PID [%d] stopped\n", pid ) ;
                        //insert ( &jobs, pid, 0 ) ;
                    }
                }
                // Go to next command parsed
                ptr = ptr->prox ;
            }
        }
        clear_cmd ( cmds ) ;
    }
done:
    ReadLine_SetPrompt ( rl, svPrompt ) ;
    iPrintf ( " leaving shell ..." ) ;
    munmap ( shmem, shmemSize ) ;
}

#if 0
void
handle_sigint ( int sig )
{
}

void
handle_sigtstp ( int sig )
{
}
#endif

void
handle_sigchld ( int sig )
{
    pid_t pid ;
    int status ;
    pid = waitpid ( - 1, &status, ( WNOHANG | WUNTRACED | WCONTINUED ) ) ;
}

int
get_type_process ( const char* cmd )
{
    if ( strncmp ( cmd, "cd", 2 ) == 0 )
    {
        return 2 ;
    }
    return 0 ;
}

int
parse_args ( char cmd[200], char *args[100], int t_process )
{
    char parser[50][100] ;
    memset ( parser, 0, sizeof (parser ) ) ;

    char *pch, *path ;
    char *sptr1, *sptr2 ;

    char aux[200], path_aux[200], cmd_aux[200], pg[100] ;
    FILE *file ;

    //Parse cmd
    strcpy ( cmd_aux, cmd ) ;
    pch = strtok_r ( cmd_aux, " ", &sptr2 ) ;

    int argc = 0, cod = 0 ;
    while ( pch != NULL )
    {
        if ( strncmp ( pch, ">", 1 ) == 0 )
        {
            cod = 1 ;
        }
        else if ( strncmp ( pch, "<", 1 ) == 0 )
        {
            cod = 2 ;
        }
        else if ( strncmp ( pch, "2>", 2 ) == 0 )
        {
            cod = 3 ;
        }
        else
        {
            strcpy ( parser[argc + cod], pch ) ;
            if ( cod == 0 )
            {
                argc ++ ;
            }
        }
        pch = strtok_r ( NULL, " ", &sptr2 ) ;
    }

    if ( parser[argc + 1][0] != '\0' )
    {
        file = fopen ( parser[argc + 1], "w" ) ;
        if ( file != NULL )
        {
            dup2 ( fileno ( file ), STDOUT_FILENO ) ;
            fclose ( file ) ;
        }
        else
        {
            fprintf ( stderr, "Error opening file\n" ) ;
            exit ( 1 ) ;
        }
    }

    if ( parser[argc + 2][0] != '\0' )
    {
        file = fopen ( parser[argc + 2], "r" ) ;
        if ( file != NULL )
        {
            dup2 ( fileno ( file ), STDIN_FILENO ) ;
            fclose ( file ) ;
        }
        else
        {
            fprintf ( stderr, "Error opening file\n" ) ;
            exit ( 1 ) ;
        }
    }

    if ( parser[argc + 3][0] != '\0' )
    {
        file = fopen ( parser[argc + 3], "w" ) ;
        if ( file != NULL )
        {
            dup2 ( fileno ( file ), STDERR_FILENO ) ;
            fclose ( file ) ;
        }
        else
        {
            fprintf ( stderr, "Error opening file\n" ) ;
            exit ( 1 ) ;
        }
    }

    for ( int i = 0 ; i < argc ; i ++ )
    {
        args[i] = parser[i] ;
    }
    args[argc] = ( char * ) NULL ;

    strcpy ( pg, args[0] ) ;

    if ( t_process )
    {
        return 0 ;
    }
    else if ( cmd[0] == '.' || cmd[0] == '/' )
    {

        if ( access ( args[0], F_OK ) == 0 )
        {
            return 0 ;
        }

    }
    else
    {
        strcpy ( path_aux, getenv ( "MYPATH" ) ) ;
        path = strtok_r ( path_aux, ":", &sptr1 ) ;

        while ( path != NULL )
        {

            strcpy ( aux, path ) ;
            strcat ( aux, "/" ) ;
            strcat ( aux, pg ) ;

            strcpy ( args[0], aux ) ;

            if ( access ( args[0], F_OK ) == 0 )
            {
                return 0 ;
            }

            path = strtok_r ( NULL, ":", &sptr1 ) ;
        }
    }

    strcpy ( args[0], pg ) ;
    return - 1 ;
}

void
_cd ( char** args, void* shmem )
{
    int len ;
    char path[500] ;

    if ( strncmp ( args[1], "..", 2 ) == 0 )
    {
        strcpy ( path, getenv ( "PWD" ) ) ;
        len = strlen ( path ) ;

        while ( path[len - 1] != '/' )
        {
            path[-- len] = '\0' ;
        }
        if ( len > 1 )
            path[-- len] = '\0' ;
    }
    else if ( args[1][0] == '/' )
    {
        strcpy ( path, args[1] ) ;
    }
    else
    {
        strcpy ( path, getenv ( "PWD" ) ) ;
        if ( strcmp ( path, "/" ) != 0 )
            strcat ( path, "/" ) ;
        strcat ( path, args[1] ) ;
    }

    memcpy ( shmem, path, sizeof (path ) ) ;
    exit ( 0 ) ;
}

// Linked list of commands

void
add_cmd ( cnode** list, char * val )
{
    //cnode* new_node = ( cnode* ) malloc ( sizeof (cnode ) ) ;
    cnode* new_node = ( cnode* ) Mem_Allocate ( sizeof ( cnode ), TEMPORARY ) ;
    strcpy ( new_node->cmd, val ) ;
    new_node->prox = NULL ;

    if ( *list == NULL )
    {
        *list = new_node ;
    }
    else
    {
        cnode* ptr = * list ;
        while ( ptr->prox != NULL )
        {
            ptr = ptr->prox ;
        }
        ptr->prox = new_node ;
    }
}

void
clear_cmd ( cnode* list )
{
    if ( list == NULL )
        return ;
    clear_cmd ( list->prox ) ;
    //free ( list ) ;
}
