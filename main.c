#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *is_executable(char *path_env, char *command_args);
int run_program(char *program, char *args[]);
char **string_to_array(char *str);

int main(int argc, char *argv[])
{
    const char *builtin[] = {"echo", "exit", "type", "pwd", "cd"};
    int num_of_builtin = sizeof(builtin) / sizeof(builtin[0]);

    // Flush after every printf
    setbuf(stdout, NULL);

    while (1)
    {

        printf("$ ");

        char input[1024];
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        char *path_env = getenv("PATH");

        char *program = input;
        char *command_args = NULL;
        char *space = strchr(input, ' ');

        if (space)
        {
            *space = '\0';
            command_args = space + 1;
        }

        if (strcmp(program, "exit") == 0)
        {
            break;
        }

        if (strcmp(program, "echo") == 0)
        {
            printf("%s\n", command_args);
        }
        else if (strcmp(program, "type") == 0)
        {
            int is_builtin = 0;
            for (int i = 0; i < num_of_builtin; i++)
            {
                if (strcmp(command_args, builtin[i]) == 0)
                {
                    is_builtin = 1;
                    break;
                }
            }
            if (is_builtin)
            {
                printf("%s is a shell builtin\n", command_args);
            }
            else
            {
                char *filepath = is_executable(path_env, command_args);

                if (filepath)
                {
                    printf("%s is %s\n", command_args, filepath);
                    free(filepath);
                }
                else
                {
                    printf("%s: not found\n", command_args);
                }
            }
        }
        else if (strcmp(program, "pwd") == 0)
        {
            char cwd[1024];
            printf("%s\n", getcwd(cwd, sizeof(cwd)));
        }
        else if (strcmp(program, "cd") == 0)
        {
            char *path = command_args;

            if (strcmp(command_args, "~") == 0)
            {
                path = getenv("HOME");
            }

            if (chdir(path) != 0)
            {
                printf("cd: %s: No such file or directory\n", command_args);
            }
        }
        else
        {
            if (is_executable(path_env, program))
            {
                run_program(program, string_to_array(command_args));
            }
            else
            {
                printf("%s: command not found\n", program);
            }
        }
    }

    return 0;
}

int run_program(char *program, char *args[])
{
    int count = 0;

    if (args)
    {
        while (args[count])
            count++;
    }

    char **argv = malloc(sizeof(char *) * (count + 2));

    argv[0] = program;

    for (int i = 0; i < count; i++)
        argv[i + 1] = args[i];

    argv[count + 1] = NULL;

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        execvp(program, argv);

        perror("execvp failed");

        exit(1);
    }
    else
    {
        int status;

        waitpid(pid, &status, 0);

        free(argv);

        return status;
    }
}

char **string_to_array(char *str)
{
    if (!str)
    {
        return NULL;
    }

    int capacity = 10;
    char **args = malloc(sizeof(char *) * capacity);
    char *str_dup = strdup(str);
    int i = 0;
    char *token;

    token = strtok(str_dup, " ");

    while (token)
    {
        args[i] = token;
        i++;
        token = strtok(NULL, " ");
    }

    args[i] = NULL;

    return args;
}

char *is_executable(char *path_env, char *program)
{

    if (!path_env || !program)
    {
        return NULL;
    }

    char *path_copy = strdup(path_env);

    char *token = strtok(path_copy, ":");

    while (token)
    {
        size_t len = strlen(token) + strlen(program) + 2;
        char *filepath = malloc(len);

        if (!filepath)
        {
            free(path_copy);
            return NULL;
        }

        snprintf(filepath, len, "%s/%s", token, program);
        if (access(filepath, X_OK) == 0)
        {

            free(path_copy);
            return filepath;
        }

        free(filepath);
        token = strtok(NULL, ":");
    }

    free(path_copy);

    return NULL;
}
