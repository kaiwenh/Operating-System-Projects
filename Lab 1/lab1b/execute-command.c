// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <sys/stat.h> 
#include <fcntl.h>  
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
   add auxiliary functions and otherwise modify the source code.
   You can also use external functions defined in the GNU C Library.  */

  //error (1, 0, "command execution not yet implemented");

   int fd[2]; //fd[0] read, fd[1] write
   pid_t c_pid;

   switch(c->type)
   {
    case SIMPLE_COMMAND:      // a simple command
      c_pid = fork();
      if(c_pid == 0) // creat a child thread to protect parent from error
      {
        if(c->input)
        {
          fd[1] = open(c->input, O_RDONLY);
          if(fd[1] == -1)
          {
            error(1, errno, "Open input file error\n");
          }
          else
          {
            dup2(fd[1],STDIN_FILENO);
            close(fd[1]);
          }
        }

        if(c->output)
        {
          fd[0] = open(c->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if(fd[0] == -1)
          {
            error(1, errno, "Failed to creat a output file\n");
          }
          else
          {
            dup2(fd[0], STDOUT_FILENO);
            close(fd[0]);
          }
        }

        execvp(c->u.word[0], c->u.word);
        c->status = errno;
        exit(c->status);
      }
      else if(c_pid > 0)
      {
        waitpid(c_pid, &(c->status), 0);
      }
      else
      {
        error(1,0,"Failed to creat a child process\n");
      }
      return;

    case SUBSHELL_COMMAND:  // ( A )
      if(c->input)
      {
        fd[1] = open(c->input, O_RDONLY);
        if(fd[1] == -1)
        {
          error(1, errno, "Open input file error\n");
        }
        else
        {
          dup2(fd[1],STDIN_FILENO);
          close(fd[1]);
        }
      }

      if(c->output)
      {
        fd[0] = open(c->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(fd[0] == -1)
        {
          error(1, errno, "Failed to creat a output file\n");
        }
        else
        {
          dup2(fd[0], STDOUT_FILENO);
          close(fd[0]);
        }
      }
      execute_command(c->u.subshell_command, time_travel);
      c->status = (c->u.subshell_command)->status;
      return;

    case AND_COMMAND:      // A && B
      execute_command(c->u.command[0], time_travel);
      if((c->u.command[0])->status == 0)
      {
      	execute_command(c->u.command[1], time_travel);
      }
      c->status = ((c->u.command[0])->status != 0) || ((c->u.command[0])->status != 0);
      return;

    case SEQUENCE_COMMAND:    // A ; B
      execute_command(c->u.command[0], time_travel);
      execute_command(c->u.command[1], time_travel);
      c->status = ((c->u.command[0])->status != 0) && ((c->u.command[0])->status != 0);
      return;

    case OR_COMMAND:          // A || B
      execute_command(c->u.command[0], time_travel);
      if((c->u.command[0])->status != 0)
      {
      	execute_command(c->u.command[1], time_travel);
      }
      c->status = ((c->u.command[0])->status != 0) && ((c->u.command[0])->status != 0);
      return;



    case PIPE_COMMAND:        // A | B
      if(pipe(fd))
      {
        c->status = errno;
        error(1,errno,"PIPE failed\n");
      }

      c_pid = fork();

      if(c_pid == 0)
      {
        if(dup2(fd[1], STDOUT_FILENO) == -1)
          error(1,0, "Write PIPE failed");
        close(fd[0]);
        close(fd[1]);
        execute_command(c->u.command[0], time_travel);
        exit(0);
      }
      else if(c_pid > 0)
      {
        waitpid(c_pid, NULL, 0);
        c_pid = fork();

        if(c_pid == 0)
        {
          if(dup2(fd[0], STDIN_FILENO) == -1)
            error(1,errno, "Write PIPE failed");

          close(fd[0]);
          close(fd[1]);
          execute_command(c->u.command[1], time_travel);
          exit(0);
        }
        else if(c_pid > 0)
        {
          close(fd[0]);
          close(fd[1]);
          waitpid(c_pid, NULL, 0);
        }

      }
      else
      {
        error(1,errno,"Failed to creat child Process!\n");
      }
      return;


    default:
    error(1,0,"Undefined Command\n");
  }
}
