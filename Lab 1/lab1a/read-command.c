// UCLA CS 111 Lab 1 command reading




#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
complete the incomplete type declaration in command.h.  */





typedef enum token_type
{
  EMPTY_NODE,          //Indicating the node hasn't been modified
  SIMPLE_TOKEN,        // a simple command
  PIPE_TOKEN,          // A | B
  OR_TOKEN,            // A || B
  AND_TOKEN,           // A && B
  SEQUENCE_TOKEN,      // A ; B
  OPEN_PAREN,          // (
  CLOSE_PAREN,         // )
  NEWLINE_TOKEN,       // \n
  INPUT_TOKEN,         // <
  OUTPUT_TOKEN,        // >
  HEAD_TOKEN,          //
  COMPLETE_TOKEN,      //\n\n
} token_type_t;


struct command_stream{
  token_type_t type;
  char *word;
  struct command_stream *next;
};



//fucntion declaration
command_stream_t createSimpleNode(char* command);

command_stream_t topNode(command_stream_t headNode);

void popTopNode(command_stream_t headNode);

command_stream_t
make_command_stream(int(*get_next_byte) (void *),
void *get_next_byte_argument);

command_t createSimpleCommand(command_stream_t tokenNode);

command_t makeCommand(command_stream_t headNode);

command_t makeCommandAOP(command_stream_t headNode, enum token_type tokenType);

command_t
read_command_stream(command_stream_t s);

//utility functions
command_stream_t createSimpleNode(char* command)
{
  command_stream_t tempNode;
  tempNode = checked_malloc(sizeof(struct command));
  tempNode->word = checked_malloc(sizeof(char)*(strlen(command)+1));
  tempNode->type = SIMPLE_TOKEN;
  tempNode->next = NULL;
  strcpy(tempNode->word, command);
  return tempNode;
}


command_stream_t topNode(command_stream_t headNode)
{
  if (headNode != NULL)
  {
    return headNode->next;
  }
  else
    return headNode;
}


void popTopNode(command_stream_t headNode)
{
  if (headNode != NULL)
  {
    command_stream_t tempNode;
    tempNode = topNode(headNode);
    if (tempNode != NULL)
    {
      headNode->next = tempNode->next;
      if (tempNode->word)
      {
        free(tempNode->word);
      }
      free(tempNode);
    }
  }
}

command_stream_t
make_command_stream(int(*get_next_byte) (void *),
void *get_next_byte_argument)
{
  int wordCount = 0;
  char* buffer; //a buffer to store simple command from the command stream
  buffer = checked_malloc(sizeof(char) * 140);

  command_stream_t headNode, tempNode, nodeIt; // nodeIt is node iterator
  tempNode = checked_malloc(sizeof(struct command_stream));
  tempNode->type = EMPTY_NODE;
  tempNode->word = NULL;
  tempNode->next = NULL;
  headNode = checked_malloc(sizeof(struct command_stream));
  headNode->type = HEAD_TOKEN;
  headNode->next = NULL;
  headNode->word = NULL;

  nodeIt = headNode;

  char tempChar, preChar;

  tempChar = get_next_byte(get_next_byte_argument);

  while (tempChar != EOF)
  {
    if(!(tempChar == 10 || tempChar == 32|| tempChar == 33 ||tempChar ==35 || tempChar == 37 || tempChar ==38 || ((tempChar >= 40 && tempChar <=124) && tempChar != 42 && tempChar != 63 && tempChar !=91 && tempChar !=93 && tempChar != 96 && tempChar != 123)))
    {
      error(1,0,"Invalid char");
    }
    switch (tempChar)
    {
    case ';':
    case '\n':
      if ((nodeIt->type == AND_TOKEN || nodeIt->type == OR_TOKEN || nodeIt->type == PIPE_TOKEN || nodeIt->type == COMPLETE_TOKEN)&& wordCount == 0)
      {
        tempChar = get_next_byte(get_next_byte_argument);
        continue;
      }
      else
        tempNode->type = SEQUENCE_TOKEN;
      break;

    case '|':
      if (nodeIt->type == PIPE_TOKEN && wordCount == 0)
      {
        nodeIt->type = OR_TOKEN;
        tempChar = get_next_byte(get_next_byte_argument);
        continue;
      }
      else
      {
        tempNode->type = PIPE_TOKEN;
      }
      break;

    case '(':
      tempNode->type = OPEN_PAREN;
      break;

    case ')':
      tempNode->type = CLOSE_PAREN;
      break;

    case '>':
      tempNode->type = OUTPUT_TOKEN;
      break;

    case '<':
      tempNode->type = INPUT_TOKEN;
      break;

    case '&':
      tempChar = get_next_byte(get_next_byte_argument);
      if (tempChar == '&')
      {
        tempNode->type = AND_TOKEN;
        break;
      }
      else
        error(1, 0, "Not valid operator \'&\'"); //TODO: replace the error with error later on linux
      break;

    case '#':
      do{
        tempChar = get_next_byte(get_next_byte_argument);
      } while (tempChar != '\n');
      tempChar = get_next_byte(get_next_byte_argument);
      continue;

    case ' ':
      if (wordCount == 0)
      {
        tempChar = get_next_byte(get_next_byte_argument);
        continue;
      }
      else if (buffer[wordCount - 1] == ' ')
      {
        tempChar = get_next_byte(get_next_byte_argument);
        continue;
      }

    default:

      buffer[wordCount] = tempChar;
      wordCount++;
      tempChar = get_next_byte(get_next_byte_argument);
      continue;
    }


    if (wordCount != 0)
    {
      if (buffer[wordCount - 1] == ' ')
      {
        buffer[wordCount - 1] = '\0';
      }
      else
      {
        buffer[wordCount] = '\0';
      }

      nodeIt->next = createSimpleNode(buffer);
      nodeIt = nodeIt->next;
    }
    preChar = tempChar;
    tempChar = get_next_byte(get_next_byte_argument);

    if (preChar == '\n' && tempChar == '\n')
    {
      tempNode->type = COMPLETE_TOKEN;
      while (tempChar == '\n')
      {
        tempChar = get_next_byte(get_next_byte_argument);
      }
    }




    wordCount = 0;

    nodeIt->next = tempNode;
    nodeIt = nodeIt->next;
    tempNode = checked_malloc(sizeof(struct command_stream));
    tempNode->type = EMPTY_NODE;
    tempNode->word = NULL;
    tempNode->next = NULL;
  }


  if (wordCount != 0)
  {
    if (buffer[wordCount - 1] == ' ')
    {
      buffer[wordCount - 1] = '\0';
    }
    else
    {
      buffer[wordCount] = '\0';
    }

    nodeIt->next = createSimpleNode(buffer);
    nodeIt = nodeIt->next;
  }

  switch (nodeIt->type)
  {
  case SEQUENCE_TOKEN:
    nodeIt->type = COMPLETE_TOKEN;
    nodeIt->next = NULL;
    free(tempNode);
  case COMPLETE_TOKEN:
    break;

  default:
    tempNode->type = COMPLETE_TOKEN;
    tempNode->next = NULL;
    tempNode->word = NULL;
    nodeIt->next = tempNode;
  }
  free(buffer);

  return headNode;

  /* FIXME: Replace this with your implementation.  You may need to
  add auxiliary functions and otherwise modify the source code.
  You can also use external functions defined in the GNU C Library.  */
  // error (1, 0, "command reading not yet implemented");
  // return 0;
}

//utility fucntion for reading command



command_t createSimpleCommand(command_stream_t tokenNode)
{
  if (tokenNode == NULL)
    return NULL;


  if (tokenNode->type != SIMPLE_TOKEN)
  {
    error(1, 0, "Can't creat the simple command with non Simple_TOKEN");
  }

  char *tempWord;
  tempWord = checked_malloc(sizeof(char*)*(1 + strlen(tokenNode->word)));
  strcpy(tempWord, tokenNode->word);

  command_t returnCommand;
  returnCommand = checked_malloc(sizeof(struct command));
  returnCommand->type = SIMPLE_COMMAND;
  returnCommand->input = NULL;
  returnCommand->output = NULL;
  returnCommand->status = 0; 

  int spaceCount;
  spaceCount = 2;
  int i;
  i = 0;
  while (tempWord[i] != '\0')
  {
    if (tempWord[i++] == ' ')
    {
      spaceCount++;
    }
  }

  char **w;
  w = checked_malloc(sizeof(char*)*(spaceCount));
  returnCommand->u.word = w;

  *w = strtok(tempWord, " ");

  while (*w++ != NULL)
  {
    *w = strtok(NULL, " ");
  }



  return returnCommand;
}




command_t makeCommand(command_stream_t headNode)
{
  command_t returnCommand;
  command_t tempCommand;
  command_stream_t nodeIt;

  returnCommand = NULL;

  nodeIt = topNode(headNode);
  if (nodeIt == NULL)
  {
    return NULL;
  }

  //nodetype that command can start with 
  switch (nodeIt->type)
  {
  case SIMPLE_TOKEN:
    returnCommand = createSimpleCommand(nodeIt);
    popTopNode(headNode);
    break;

  case OPEN_PAREN:
    returnCommand = checked_malloc(sizeof(struct command));
    returnCommand->type = SUBSHELL_COMMAND;
    returnCommand->input = NULL;
    returnCommand->output = NULL;
    returnCommand->status = 0;
    popTopNode(headNode);
    returnCommand->u.subshell_command = makeCommandAOP(headNode, OPEN_PAREN);
    break;

  default:
    error(1, 0,"Invalid word to start with"); //TODO: replace with error
  }


  if (returnCommand == NULL)
    return returnCommand;

  nodeIt = topNode(headNode);
  token_type_t tempType;
  tempType = nodeIt->type;

  if (tempType == INPUT_TOKEN)
  {
    if(returnCommand->output != NULL)
    {
      error(1, 0, "Input after output"); // TODO: replace it with error
    }
    popTopNode(headNode);
    nodeIt = topNode(headNode);
    if (nodeIt->type == SIMPLE_TOKEN)
    {
      returnCommand->input = checked_malloc(sizeof(char)*(strlen(nodeIt->word) + 1));
      strcpy(returnCommand->input, nodeIt->word);
    }
    else
    {
      error(1, 0, "Invalid input");
    }
    popTopNode(headNode);
    nodeIt = topNode(headNode);
    tempType = nodeIt->type;
  }

  if (tempType == OUTPUT_TOKEN)
  {
    popTopNode(headNode);
    nodeIt = topNode(headNode);
    if (nodeIt->type == SIMPLE_TOKEN)
    {
      returnCommand->output = checked_malloc(sizeof(char)*(strlen(nodeIt->word) + 1));
      strcpy(returnCommand->output, nodeIt->word);
    }
    popTopNode(headNode);
    nodeIt = topNode(headNode);
    tempType = nodeIt->type;
  }

  tempType;
  tempType = nodeIt->type;

  while (tempType == PIPE_TOKEN)
  {
    tempCommand = returnCommand;
    returnCommand = checked_malloc(sizeof(struct command));
    returnCommand->type = PIPE_COMMAND;
    returnCommand->input = NULL;
    returnCommand->output = NULL;
    returnCommand->u.command[0] = tempCommand;
    popTopNode(headNode);
    returnCommand->u.command[1] = makeCommand(headNode);
    nodeIt = topNode(headNode);
    tempType = nodeIt->type;
  }

  return returnCommand;
  //
}

//make Command for 'And Or Pipe' (AOP) 

command_t makeCommandAOP(command_stream_t headNode, enum token_type tokenType)
{
  command_t returnCommand;
  command_t tempCommand;
  command_stream_t nodeIt;
  returnCommand = makeCommand(headNode);
  if(returnCommand == NULL)
  {
    return returnCommand;
  }
  nodeIt = topNode(headNode);

  token_type_t tempType;
  tempType = nodeIt->type;

  while (tempType == AND_TOKEN || tempType == OR_TOKEN || tempType == SEQUENCE_TOKEN)
  {
    tempCommand = returnCommand;
    returnCommand = checked_malloc(sizeof(struct command));
    returnCommand->type = (tempType == AND_TOKEN) ? AND_COMMAND : (tempType == OR_TOKEN) ? OR_COMMAND : SEQUENCE_COMMAND;
    returnCommand->input = NULL;
    returnCommand->output = NULL;
    returnCommand->u.command[0] = tempCommand;
    popTopNode(headNode);
    returnCommand->u.command[1] = makeCommand(headNode);
    nodeIt = topNode(headNode);
    tempType = nodeIt->type;
  }

  if (tempType == COMPLETE_TOKEN)
  {
    if(tokenType == HEAD_TOKEN)
    {
      popTopNode(headNode);
      return returnCommand;
    }
    else 
    {
      error(1, 0,"An unfinished subshell_command"); //TODO: replace it with error
    }
  }

  if (tempType == CLOSE_PAREN)
  {
    if(tokenType == OPEN_PAREN)
    {
      popTopNode(headNode);
      return returnCommand;
    }
    else
    {
      error(1, 0, "An extra OPEN_PAREN");
    }
  }
  return returnCommand;
}



command_t
read_command_stream(command_stream_t s)
{


  command_t returnCommand;
  returnCommand = makeCommandAOP(s, HEAD_TOKEN);
  return returnCommand;

  /* FIXME: Replace this with your implementation too.  */
  // error (1, 0, "command reading not yet implemented");
  //return 0;
}
