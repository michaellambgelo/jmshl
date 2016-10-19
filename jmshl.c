/* 
Author: Michael Lamb
Date: 9/27/2016
Description:
	This program executes a shell which is able to execute system commands and
	has a custom implementation of cd

TODO:
	- input redirection (done)
	- piping (done)
	- audit log 
	- append ("<<")

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

void usr1Handler()
{
	FILE *filep = fopen("audit.txt","a");
	fclose(filep);
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
runpipe(fd,arg1,arg2)
accepts a file descriptor for piping, then forks a new process to execute the
first command and hand the output over to the second command
*/
void runpipe(int fd[], char **arg1, char **arg2)
{
	pipe(fd);
	switch(fork())
	{
		case 0:
			dup2(fd[1],STDOUT_FILENO);
			close(fd[0]);
			execvp(arg1[0], arg1);
			exit(1);
			break;

		case -1:
			fprintf(stderr,"Unable to pipe. Free up some memory and try again.\n");
			break;

		default:
			wait(NULL);
			dup2(fd[0],STDIN_FILENO);
			close(fd[1]);
			execvp(arg2[0], arg2);
			break;
	}

	return;
}

/*
main()
*/
int main()
{
	system("clear");
	int	pid, 
  		fd, //file descriptor
  		pfd[2],
  		cout = dup(STDOUT_FILENO), //get a reference to stdout
  		cin = dup(STDIN_FILENO), //get a reference to stdin
  		i,
  		j,
  		out = 0,
  		append = 0,
  		in = 0,
  		pipeflag = 0;
	char buffer[BUFFERSIZE], 
  		 *token, 
  		 *separator = " \t\n", 
  		 **args,
  		 **cmd1,
  		 **cmd2;
 	signal(SIGINT, SIG_IGN);
 	signal(SIGUSR1, usr1Handler);

	 // max 80 chars in buffer 

	args = (char **) malloc( 80 * sizeof(char *) );
	cmd1 = (char **) malloc( 80 * sizeof(char *) );
	cmd2 = (char **) malloc( 80 * sizeof(char *) );

	fprintf( stderr, "%s", prompt );

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
					fd = append ? open(token, O_WRONLY|O_APPEND) : creat(token, 0777);
					if(fd < 0)
						fprintf(stderr, "Unable to redirect output.\n");
					if(fd < 0 && append)
						fprintf(stderr, "The filename provided does not exist.\n");
					dup2(fd, STDOUT_FILENO);
					close(fd);
					out = 0;
					append = 0;
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
	 		    else if( token && strcmp(token,">>") == 0)
	 		    {
	 		    	out = 1;
	 		    	append = 1;
	 		    	args[i++] = (char *) NULL;

	 		    }
	 		    else if( token && strcmp(token,"<") == 0)
	 		    {
	 		    	in = 1;
	 		    	//args[i++] = (char *) NULL;
	 		    }
	 		    else if( token && strcmp(token,"|") == 0)
	 		    {
	 		    	cmd1 = args;
	 		    	pipeflag = 1;
	 		    	j = 0;

	 		    	while( token != NULL )
	 		    	{
	 		    		token = strtok( NULL, separator );
	 		    		cmd2[j++] = token;
	 		    	}

	 		    }
				else
			      	args[i++] = token;  //build command array 
		    }

		    args[i] = (char *) NULL;

		    //if the entered command is cd
		    if( strcmp(args[0], "cd") == 0 )
		    	cd(args); //then cd
		    else if( strcmp(args[0], "exit") == 0)
		    	break;
			else //execute it in a child process
			{
				switch( pid = fork() )
				{
					case 0:
						if(pipeflag)
						{
							runpipe(pfd,cmd1,cmd2);
							pipeflag = 0;
						}
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
						kill(getpid(), SIGUSR1);
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
