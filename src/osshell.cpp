#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <cctype>

#include <cstdint>
#include <sys/wait.h>
#include <filesystem>
#include <cctype>
#include <fstream>

bool fileExecutableExists(std::string file_path);
void splitString(std::string text, char d, std::vector<std::string>& result);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
void freeArrayOfCharArrays(char **array, size_t array_length);

int main (int argc, char **argv)
{
    // Get list of paths to binary executables
    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    // Create list to store history
    std::vector<std::string> history;

    // Load persisted history (if any)
    {
        std::ifstream histin("history.txt");
        std::string line;
        while (histin && std::getline(histin, line)) {
            if (!line.empty())
                history.push_back(line);
        }
    }

    // Create variables for storing command user types
    std::string user_command;               // to store command user types in
    std::vector<std::string> command_list;  // to store `user_command` split into its various parameters
    char **command_list_exec;               // to store `command_list` converted to an array of character arrays

    // Welcome message
    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");

    bool commandFound;

    // Repeat:
    while (true) {
        commandFound = false;

        //  Print prompt for user input: "osshell> " (no newline)
        std::cout << "osshell> ";

        //  Get user input for next command
        std::getline(std::cin, user_command);

        // If user doesn't type anything, jump to the next iteration of the loop
        if (user_command.empty())
        {
            continue;
        }

        history.push_back(user_command);
        {
            while (history.size() > 129) {
                history.erase(history.begin());
            }
            std::ofstream histout("history.txt", std::ios::app);
            if (histout) histout << user_command << '\n';
        }

        //  If command is `exit` exit loop / quit program
        if (user_command == "exit")
        {
            break;
        }

        //  For all other commands, check if an executable by that name is in one of the PATH directories
        // split the command
        splitString(user_command, ' ', command_list);
        
        //if the command is history
        if (command_list.size() > 0 && command_list[0] =="history") {
            //if the command is history (no integer)
            if (command_list.size() == 1) {
                for (size_t i=0; i < history.size()-1; ++i)
                    std::cout << "  " << (i+1) << ": " <<history[i] << std::endl;
                continue;
            }
            //if the command is history(integer)
            if (command_list.size() == 2) {
                if (command_list[1] == "clear") {
                    //clear the history
                    std::ofstream ofs;
                    ofs.open("history.txt", std::ofstream::out | std::ofstream::trunc);
                    ofs.close();
                    history.clear();
                    continue;
                }
                std::string arg = command_list[1];
                bool ok = !arg.empty();
                for (char ch : arg) if (!std::isdigit((unsigned char)ch)) 
                    {ok = false; break;}
                //if the command is history (without integer)
                if (!ok) {
                    std::cout << "Error: history expects an integer > 0 (or 'clear')" << std::endl;
                    continue;
                }
                int n = std::stoi(arg);
                //if the command is history (with negative integer)
                if (n<=0) {
                    std::cout << "Error: history expects an integer > 0 (or 'clear')" << std::endl;
                    continue;
                }
                int start = (int)history.size()-(n+1);
                if (start <0) start = 0;
                for (int i = start; i < (int)history.size()-1; ++i) {
                    std::cout << "  " << (i+1) << ": " << history[i] << std::endl;
                }
                continue;
            }
            //fallback case for weirdness
            else {
                std::cout << "history: too many arguments" << std::endl;
                continue;
            }
        }


        // extract the command name
        std::string command_name = command_list[0];

        std::string full_path_to_command;

        //checks if the "/" character is in the command char array (npos means "null" essentially)
        if(command_name.find('/') != std::string::npos){
            //only occurs if "/" WAS in char array
            //checks if given command after "/" exists in file directory
            if(fileExecutableExists(command_name)){
                //if command exists, set commandFound to true, and makes the PATH of the command to the command name
                commandFound = true;
                full_path_to_command = command_name;
            }else{
                //Error handling, if command is not found
                std::cout << command_name << ": Error command not found" << std::endl;
                continue;
            }
        }
        //only occurs if "/" was NOT in char array
        else{
                // search the PATH directories for that command
            for (int i = 0; i < os_path_list.size(); i++) {
                // build the full path
                full_path_to_command = os_path_list[i] + "/" + command_name;

                // checks if the command exists AND is executable AND is not a directory
                if (fileExecutableExists(full_path_to_command)) 
                {
                    commandFound = true;
                    break;
                } 
                
            }
        }

        //   If yes (an executable by that name is in one of the PATH directories), execute it
        if (commandFound) 
        {
            // fork(): create child process
            int pid = fork();

            // child process
            if (pid == 0) 
            {
                // prepare arguments for execv()
                vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);

                // call execv() to execute the command by replacing the current process (child process)
                execv(full_path_to_command.c_str(), command_list_exec);

                // If execv returns, it failed
                perror("execv failed");
                exit(1);
            }
            // parent process
            else 
            {
                // wait for child to finish
                int status;
                waitpid(pid, &status, 0);
            }   
        }
        //   If no, print error statement: "<command_name>: Error command not found" (do include newline)
        else 
        {
            std::cout << command_name << ": Error command not found" << std::endl;
        }

    }


    /************************************************************************************
     *   Example code - remove in actual program                                        *
     ************************************************************************************/
    // Shows how to loop over the directories in the PATH environment variable

    /* // remove this multi-line comment if you want to run the example code

    int i;
    for (i = 0; i < os_path_list.size(); i++)
    {
        printf("PATH[%2d]: %s\n", i, os_path_list[i].c_str());
    }
    printf("------\n");
    
    // Shows how to split a command and prepare for the execv() function
    std::string example_command = "ls -lh";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // use `command_list_exec` in the execv() function rather than looping and printing
    i = 0;
    while (command_list_exec[i] != NULL)
    {
        printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
        i++;
    }
    // free memory for `command_list_exec`
    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    printf("------\n");

    // Second example command - reuse the `command_list` and `command_list_exec` variables
    example_command = "echo \"Hello world\" I am alive!";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // use `command_list_exec` in the execv() function rather than looping and printing
    i = 0;
    while (command_list_exec[i] != NULL)
    {
        printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
        i++;
    }
    // free memory for `command_list_exec`
    freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    printf("------\n");

    */

    /************************************************************************************
     *   End example code                                                               *
     ************************************************************************************/


    return 0;
}

/*
   file_path: path to a file
   RETURN: true/false - whether or not that file exists and is executable
*/
bool fileExecutableExists(std::string file_path)
{
    bool exists = false;
    // check if `file_path` exists
    // if so, ensure it is not a directory and that it has executable permissions
    
    // access() checks if the file exists AND is executable. 0 means the command was found and is executable, -1 means the command was not found or is not executable
    // is_directory() checks if the file is a directory
    if ((access(file_path.c_str(), X_OK) == 0) && !(std::filesystem::is_directory(file_path))) 
    {
        exists = true;
    } 

    return exists;
}

/*
   text: string to split
   d: character delimiter to split `text` on
   result: vector of strings - result will be stored here
*/
void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }
}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

/*
   array: list of strings (array of character arrays) to be freed
   array_length: number of strings in the list to free
*/
void freeArrayOfCharArrays(char **array, size_t array_length)
{
    int i;
    for (i = 0; i < array_length; i++)
    {
        if (array[i] != NULL)
        {
            delete[] array[i];
        }
    }
    delete[] array;
}
