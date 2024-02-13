#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char c = 0;
    char *args[MAXARG];
    for (int i = 1; i < argc; i++)
    {
        args[i - 1] = argv[i];
    }
    char *current_arg = malloc(128);
    for (int i = 0; i < 128; i++)
    {
        current_arg[i] = 0;
    }
    int current_arg_pos = argc - 1;
    int curr_pos = 0;
    int end_of_input = read(0, &c, 1);

    while (end_of_input > 0)
    {
        if (c == ' ')
        {
            args[current_arg_pos] = current_arg;
            current_arg = malloc(128);
            for (int i = 0; i < 128; i++)
            {
                current_arg[i] = 0;
            }
            current_arg_pos++;
            curr_pos = 0;
        }
        else if (c == '\n')
        {
            args[current_arg_pos] = current_arg;
            current_arg = malloc(128);
            for (int i = 0; i < 128; i++)
            {
                current_arg[i] = 0;
            }
            current_arg_pos = argc - 1;
            curr_pos = 0;
            int pid = fork();
            if (pid == 0)
            {
                exec(args[0], args);
            }
            else if (pid > 0)
            {
                wait(0);
            }
            else
            {
                fprintf(2, "fork failed\n");
                exit(1);
            }
        }
        else if (c != -1)
        {
            current_arg[curr_pos] = c;
            curr_pos++;
        }
        end_of_input = read(0, &c, 1);
    }
    exit(0);
}
