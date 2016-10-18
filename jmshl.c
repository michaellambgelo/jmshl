/* 
Author: Michael Lamb
Date: 9/27/2016
Description:
	This program executes a shell which is able to execute system commands and
	has a custom implementation of cd

TODO:
	-input redirection (done)
	-piping
	-command history

This shell extends a basic shell written by:
T. Ritter 10/6/2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h> //for handling ctrl + c
#define BUFFERSIZE 81

/* global prompt */
char *prompt = "jmshl$ ";

/*
sigintHandler(int)
accepts a signal interruption and handles it
*/
void sigintHandler(int sig_num)
{
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler);
    system("clear");
    printf("\nAny running processes have been terminated.\n");
    printf("To exit jmshl, use Ctrl + D\n");
	fprintf( stderr, "%s", prompt );

    return;
}

/*
cd(char)
accepts a pointer to an array of arguments to implement
changing directories
*/
int cd(char **args)
{
	if(args[1] == NULL)
	{
		chdir(getenv("HOME"));
	}
	else
	{
		chdir(args[1]);
	}

    return 0;
}

/*
main()
*/
int main()
{
	system("clear");
	int	pid, 
  		fd, //file descriptor
  		cout = dup(STDOUT_FILENO), //get a reference to stdout
  		cin = dup(STDIN_FILENO), //get a reference to stdin
  		i,
  		out = 0,
  		in = 0;
	char buffer[BUFFERSIZE], 
  		 *token, 
  		 *separator = " \t\n", 
  		 **args;
 	signal(SIGINT, SIG_IGN);

	 // max 80 chars in buffer 

	args = (char **) malloc( 80 * sizeof(char *) );
	fprintf( stderr, "%s", prompt );
	fflush(stdin);

	while ( fgets( buffer, 80, stdin ) != NULL ) //wait for new commands
	{
		// get the first token 
		if((token = strtok( buffer, separator )) != NULL)
		{
		    i = 0; //reset iterator in parent
		    args[i++] = token;   //build command array 
		    while( token != NULL )  //walk through other tokens 
		    {
		      	token = strtok( NULL, separator );
		      	//if there is a new token and the output flag is set
		     	if( token && out )
		     	{
		     		//perform output redirection
					fd = creat(token, 644);
					dup2(fd, STDOUT_FILENO);
					close(fd);
					out = 0;
				}
				else if( token && in )
				{
					//perform input redirection
					fd = open(token, O_RDONLY);
					dup2(fd, STDIN_FILENO);
					close(fd);
					in = 0;
				}

				//if there is a new token and it is output redirection
			    if( token && strcmp(token,">") == 0)
			    {
			    	//set output flag and indicate the end of the command array
			    	out = 1;
			    	args[i++] = (char *) NULL;
	 		    }
	 		    else if( token && strcmp(token,"<") == 0)
	 		    {
	 		    	in = 1;
	 		    	args[i++] = (char *) NULL;
	 		    }
				else
			      	args[i++] = token;  //build command array 
		    }

		    args[i] = (char *) NULL;

		    //if the entered command is cd
		    if( strcmp(args[0], "cd") == 0 )
		    	cd(args); //then cd
			else //execute it in a child process
			{
				switch( pid = fork() )
				{
					case 0:
						execvp( args[0], args );   //child
						fprintf( stderr, "ERROR %d no such program\n", errno);
						exit(1);
						break;

					case -1: 
						fprintf( stderr, "ERROR can't create child process!\n" ); 
						break;

					default: 
						wait(NULL);
						dup2(cout, STDOUT_FILENO);
						dup2(cin, STDIN_FILENO);
				}
			}
		}

		if(errno) //print non-fatal errors to the console
		{
			fprintf( stderr, "An error occurred: %d\n", errno );
			errno = 0;
		}
		fprintf( stderr, "%s", prompt );

	}//end while

	system("clear");
	exit(0);

} //end main
