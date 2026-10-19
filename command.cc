
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"


void parse(std::vector<Token> &tokens)
{
    Command currentCommand;
    SimpleCommand *currentSimpleCommand = new SimpleCommand();
bool expectingFilename = false;
    TokenType redirectionType = TOKEN_EOF;
    bool valid = true;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        Token token = tokens[i];

        if (token.type == TOKEN_EOF)
        {
            break;
        }

        // 1. Handle redirection filenames
        if (expectingFilename)
        {
            if (//token.type != TOKEN_COMMAND &&
                token.type != TOKEN_ARGUMENT)
            {
                std::cerr << "Error: missing or invalid redirection filename\n";
                valid = false;
                break;
            }

            char *filename = strdup(token.value.c_str());

            if (redirectionType == TOKEN_REDIRECT)
            {
                if (currentCommand._outFile != nullptr)
                {
                    std::cerr << "Error: duplicate output redirection\n";
                    free(filename);
                    valid = false;
                    break;
                }

                currentCommand._outFile = filename;
            }
            else if (redirectionType == TOKEN_APPEND)
            {
                if (currentCommand._outFile != nullptr)
                {
                    std::cerr << "Error: duplicate output redirection\n";
                    free(filename);
                    valid = false;
                    break;
                }

                currentCommand._outFile = filename;
                currentCommand._append = 1;
            }
            else if (redirectionType == TOKEN_INPUT)
            {
                if (currentCommand._inputFile != nullptr)
                {
                    std::cerr << "Error: duplicate input redirection\n";
                    free(filename);
                    valid = false;
                    break;
                }

                currentCommand._inputFile = filename;
            }
            else if (redirectionType == TOKEN_ERROR)
            {
                if (currentCommand._errFile != nullptr)
                {
                    std::cerr << "Error: duplicate error redirection\n";
                    free(filename);
                    valid = false;
                    break;
                }

                currentCommand._errFile = filename;
            }
            else if (redirectionType == TOKEN_REDIRECT_AND_ERROR)
            {
                if (currentCommand._outFile != nullptr ||
                    currentCommand._errFile != nullptr)
                {
                    std::cerr << "Error: duplicate output/error redirection\n";
                    free(filename);
                    valid = false;
                    break;
                }

                currentCommand._outFile = filename;
                currentCommand._errFile = strdup(token.value.c_str());
                currentCommand._out_error = 1;
                currentCommand._append = 1;
            }

            expectingFilename = false;
            continue;
        }

        // 2. Handle pipes
        if (token.type == TOKEN_PIPE)
        {
            if (currentSimpleCommand->_numberOfArguments == 0)
            {
                std::cerr << "Error: missing command before pipe\n";
                valid = false;
                break;
            }

            currentCommand.insertSimpleCommand(currentSimpleCommand);
            currentSimpleCommand = new SimpleCommand();

            continue;
        }

        // 3. Handle redirection operators
        if (token.type == TOKEN_REDIRECT ||
            token.type == TOKEN_APPEND ||
            token.type == TOKEN_INPUT ||
            token.type == TOKEN_ERROR ||
            token.type == TOKEN_REDIRECT_AND_ERROR)
        {
            if (currentSimpleCommand->_numberOfArguments == 0)
            {
                std::cerr << "Error: redirection without a command\n";
                valid = false;
                break;
            }

            expectingFilename = true;
            redirectionType = token.type;

            continue;
        }

        // 4. Handle background execution
        if (token.type == TOKEN_BACKGROUND)
        {
            if (currentSimpleCommand->_numberOfArguments == 0)
            {
                std::cerr << "Error: missing command before &\n";
                valid = false;
                break;
            }

            if (i + 1 < tokens.size() &&
                tokens[i + 1].type != TOKEN_EOF)
            {
                std::cerr << "Error: unexpected token after &\n";
                valid = false;
                break;
            }

            currentCommand._background = 1;
            continue;
        }

        // 5. Handle command names and arguments
        if (token.type == TOKEN_COMMAND ||
            token.type == TOKEN_ARGUMENT)
        {
            currentSimpleCommand->insertArgument(
                strdup(token.value.c_str()));
        }
    }

    // 6. Check whether a redirection filename is missing
    if (expectingFilename)//should be false
    {
        std::cerr << "Error: missing redirection filename\n";
        valid = false;
    }

    // 7. Add the last simple command, after the pipe 
    if (valid)
    {
        if (currentSimpleCommand->_numberOfArguments > 0)
        {
            currentCommand.insertSimpleCommand(currentSimpleCommand);
            currentSimpleCommand = nullptr;
        }
        else if (currentCommand._numberOfSimpleCommands > 0)//currentsimple empty but the current command is not empty
        {
            std::cerr << "Error: missing command after pipe\n";
            valid = false;
        }
    }

    // 8. Print the command table for testing
    if (valid && currentCommand._numberOfSimpleCommands > 0)
    {
        currentCommand.print();
    }

  // Free any unfinished simple command
if (currentSimpleCommand != nullptr)
{
    for (int j = 0;
         j < currentSimpleCommand->_numberOfArguments;
         j++)
    {
        free(currentSimpleCommand->_arguments[j]);
    }

    free(currentSimpleCommand->_arguments);
    delete currentSimpleCommand;
}

// Execute only if parsing succeeded and a command exists
if (valid && currentCommand._numberOfSimpleCommands > 0)
{
    currentCommand.execute();
}
else
{
    currentCommand.clear();
}

    
    /*
      Implement this function 
      you must parse for all possible tokens scenario
      right now command.execute only print the format , you will inject it in your logic.

    */
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _numberOfSimpleCommands = 0;

    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _outFile = nullptr;
    _inputFile = nullptr;
    _errFile = nullptr;

    _out_error = 0;
    _background = 0;
    _append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    // Display the parsed command table
    print();

    // Make sure a command exists
    if (_numberOfSimpleCommands == 0)
    {
        clear();
        return;
    }

    // Pipes will be implemented in the next stage
    if (_numberOfSimpleCommands > 1)
    {
        std::cerr << "Error: pipes are not implemented yet\n";
        clear();
        return;
    }

    // Get the simple command and its arguments
    SimpleCommand *simpleCommand = _simpleCommands[0];

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        clear();
        return;
    }

    if (pid == 0)
    {

        // Handle output redirection using >
        if (_outFile != nullptr && _append == 0)
        {
          
            int fd = open(_outFile,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

           
            if (fd < 0)
            {
                perror("open output file");
                _exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) < 0)
            {
                perror("dup2 output");
                close(fd);
                _exit(1);
            }
            close(fd);
        }

   
        execvp(simpleCommand->_arguments[0],
               simpleCommand->_arguments);

  
        perror(simpleCommand->_arguments[0]);

        _exit(127);
    }
    else
    {
        // PARENT PROCESS
        if (_background == 0)
        {
         
            int status;

            while (waitpid(pid, &status, 0) == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                perror("waitpid");
                break;
            }
        }
        else
        {
            std::cout << "Started background process: "
                      << pid << std::endl;
        }
    }

    // Free the command's allocated memory
    clear();
}
void Command::prompt()
{
    printf("myshell>");
    fflush(stdout);
    std::string input;
    while (true)
    {
        std::getline(std::cin, input); 
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    Command::_currentCommand.prompt();
    return 0;
}
