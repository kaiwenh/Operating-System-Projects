// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <sys/stat.h> 
#include <fcntl.h>  
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */



typedef enum lockType
{
	READING_LOCK,
	WRITING_LOCK
} lt;



//track all allocated resources, what are they, what kind  
//allocation it is
struct source_list
{
	char* w;
	lt type;
	struct source_list *next;
};


//record what command will be executed next.
typedef struct source_list* sl_t;

struct command_list
{
  command_t c;
  struct command_list *next;
};

typedef struct command_list* cl_t;




void execute_command_s(command_t c);
void execute_command_p(command_t c);
int checkDependence(command_t c, sl_t sl);
int LockResouce(char *w, sl_t sl, lt type);
void emptySl(sl_t sl);
cl_t preempt(command_t c);


int
command_status (command_t c)
{
  return c->status;
}


//0 means that executing command c wont mess up the resouces 
// that is allocated by previous function.
int checkDependence(command_t c, sl_t sl) 
{
  sl_t sl_it;
  sl_it = sl;
  int tempBool;
  tempBool = 0;
  switch(c->type)
  {  
    case SUBSHELL_COMMAND:
      tempBool = checkDependence(c->u.subshell_command, sl);  
    case SIMPLE_COMMAND:
    if(c->input)
      tempBool = tempBool | LockResouce(c->input, sl, READING_LOCK);

    if(c->output)
      tempBool = tempBool | LockResouce(c->output, sl, WRITING_LOCK);
      return tempBool;

    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
      tempBool = checkDependence(c->u.command[0], sl);  
      tempBool = checkDependence(c->u.command[1], sl);
      return tempBool;

    default:
    error(1, 0, "Dependence Check failed");
  }

  return tempBool;
}

//lock resource and register the source into source list
// return 1 if it is WAW, WAR, RAW ,0 if it is RAR
int LockResouce(char *w, sl_t sl, lt type)
{
  sl_t sl_it;
  sl_it = sl;
  int tempBool;
  tempBool = 0;
  while (sl_it->next)
  {
    if(!strcmp(sl_it->next->w, w))
    {
      if(sl_it->next->type == READING_LOCK)
        tempBool = 0;
      else 
        tempBool = 1;

      if (type == WRITING_LOCK)
      {
        sl_it->type = WRITING_LOCK;
        tempBool = 1;
      }

      break;
    }
    sl_it = sl_it->next;
  }
  
  if(sl_it->next == NULL)
  {
    sl_it->next = checked_malloc(sizeof(struct source_list));
    sl_it = sl_it->next;
    sl_it->w = checked_malloc(sizeof(char)*(strlen(w) + 1));
    strcpy(sl_it->w, w);
    sl_it->type = type;
    sl_it->next = NULL;
  }
  return tempBool;
}


//put first level Sequence command into a list
//later on commands on that list will be executed in parallel
cl_t preempt(command_t c)
{
  command_t c_it;
  c_it = c;
  cl_t returnCl, tempCl;
  returnCl = NULL;
  tempCl = checked_malloc(sizeof(struct command_list));

  while(c_it->type == SEQUENCE_COMMAND) //Preempt the command for the sequnce command
  {
    tempCl->c = c_it->u.command[1];
    tempCl->next = returnCl;
    returnCl = tempCl;
    c_it = c_it->u.command[0];
    tempCl = checked_malloc(sizeof(struct command_list));
  }

  tempCl->c = c_it;
  tempCl->next = returnCl;
  returnCl = tempCl;

  return returnCl;
}


//empty the sourcelist. An empty sourcelist means
//there is no source being hold
void emptySl(sl_t sl)
{
  if(sl == NULL)
    return;

  sl_t tempSl;
  while(sl->next)
  {
    tempSl = sl->next;
    sl->next=sl->next->next;
    free(tempSl->w);
    free(tempSl);
  }
  return;
}


//execute command parallelly 
void execute_command_p(command_t c)
{
  pid_t child;
  cl_t commandList, cl_it;
  sl_t sourceList;
  sourceList = checked_malloc(sizeof(struct source_list));
  sourceList->next = NULL;
  sourceList->w = NULL;
  commandList = checked_malloc(sizeof(struct command_list));
  commandList->next = preempt(c);
  cl_it = commandList;
  while(cl_it->next)
  {
    if(checkDependence(cl_it->next->c, sourceList))
    {
      while(wait(NULL)>0);
      emptySl(sourceList);
    }
    else
    {
      child = fork();
      if(child == 0)
      {
        execute_command_s(cl_it->next->c);
        exit(0);
      }
      else if(child > 0)
      {
        cl_it = cl_it->next;
      }
      else
      {
        error(1,0," Paralism crashes, HELP!\n");
      }
    }        
  }
  while(wait(NULL)>0);
  return;
}



//execute command serially
void execute_command_s(command_t c)
{
  int fd[2]; //fd[0] read, fd[1] write
  pid_t c_pid, c2_pid; //child 1 pid and child 2 pid

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
    execute_command_s(c->u.subshell_command);
    c->status = (c->u.subshell_command)->status;
    return;

    case AND_COMMAND:      // A && B
    execute_command_s(c->u.command[0]);
    if((c->u.command[0])->status == 0)
    {
      execute_command_s(c->u.command[1]);
    }
    c->status = ((c->u.command[0])->status != 0) || ((c->u.command[0])->status != 0);
    return;

    case SEQUENCE_COMMAND:    // A ; B
    execute_command_s(c->u.command[0]);
    execute_command_s(c->u.command[1]);
    c->status = ((c->u.command[0])->status != 0) && ((c->u.command[0])->status != 0);
    return;

    case OR_COMMAND:          // A || B
    execute_command_s(c->u.command[0]);
    if((c->u.command[0])->status != 0)
    {
      execute_command_s(c->u.command[1]);
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
      execute_command_s(c->u.command[0]);
      exit(0);
    }
    else if(c_pid < 0)
    {
      close(fd[0]);
      close(fd[1]);
      error(1,errno,"Failed to creat child Process!\n");
    }


    c_pid = fork();

    if(c_pid == 0)
    {
      if(dup2(fd[0], STDIN_FILENO) == -1)
        error(1,errno, "Write PIPE failed");

      close(fd[0]);
      close(fd[1]);
      execute_command_s(c->u.command[1]);
      exit(0);
    }
    else if(c_pid < 0)
    {    
      close(fd[0]);
      close(fd[1]);
      error(1,errno,"Failed to creat child Process!\n");
    }

    close(fd[0]);
    close(fd[1]);
    while(wait(NULL)>0);

    return;


    default:
    error(1,0,"Undefined Command\n");
  }
}


void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
   add auxiliary functions and otherwise modify the source code.
   You can also use external functions defined in the GNU C Library.  */

  //error (1, 0, "command execution not yet implemented");
   if(time_travel)
   {
    execute_command_p(c);
   }
   else
   {
    execute_command_s(c);
   }

   return;
}