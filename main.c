#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *is_executable(char *path_env, char *program);
int run_program(char *argv[]);
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

        char **argv = string_to_array(input);

        if (!argv || !argv[0])
            continue;

        char *program = argv[0];

        if (strcmp(program, "exit") == 0)
        {
            break;
        }

        if (strcmp(program, "echo") == 0)
        {
            for (int i = 1; argv[i]; i++)
            {
                printf("%s", argv[i]);

                if (argv[i + 1])
                    printf(" ");
            }

            printf("\n");
        }
        else if (strcmp(program, "type") == 0)
        {
            if (!argv[1])
                continue;

            int is_builtin = 0;
            for (int i = 0; i < num_of_builtin; i++)
            {
                if (strcmp(argv[1], builtin[i]) == 0)
                {
                    is_builtin = 1;
                    break;
                }
            }
            if (is_builtin)
            {
                printf("%s is a shell builtin\n", argv[1]);
            }
            else
            {
                char *filepath = is_executable(path_env, argv[1]);

                if (filepath)
                {
                    printf("%s is %s\n", argv[1], filepath);
                    free(filepath);
                }
                else
                {
                    printf("%s: not found\n", argv[1]);
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
            if (!argv[1])
                continue;

            char *path = argv[1];

            if (strcmp(argv[1], "~") == 0)
            {
                path = getenv("HOME");
            }

            if (chdir(path) != 0)
            {
                printf("cd: %s: No such file or directory\n", argv[1]);
            }
        }
        else
        {
            if (is_executable(path_env, program))
            {
                run_program(argv);
            }
            else
            {
                printf("%s: command not found\n", program);
            }
        }
    }

    return 0;
}

int run_program(char *argv[])
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        execvp(argv[0], argv);

        perror("execvp failed");

        exit(1);
    }
    else
    {
        int status;

        waitpid(pid, &status, 0);

        return status;
    }
}

// TODO: Fix memory leak
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
        if (i >= capacity - 1)
        {
            capacity *= 2;

            char **tmp = realloc(args, sizeof(char *) * capacity);

            if (!tmp)
            {
                free(args);
                free(str_dup);
                return NULL;
            }

            args = tmp;
        }

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
