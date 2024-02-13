#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_NUM 35

int main(int argc, char *argv[])
{
    int p = 2;
    printf("prime %d\n", p);
    int input_pipe[2] = {0};
    int output_pipe[2] = {0};
    int n = 3;
    while(n < MAX_NUM) {
        if(n % p != 0)
        {
            // if there is no right neighbor, create one
            if (output_pipe[0] == 0 && output_pipe[1] == 0)
            {
                pipe(output_pipe);
                int pid = fork();
                if (pid > 0)
                {
                    close(output_pipe[0]);
                }
                else if (pid == 0)
                {
                    input_pipe[0] = output_pipe[0];
                    input_pipe[1] = output_pipe[1];
                    output_pipe[0] = 0;
                    output_pipe[1] = 0;
                    close(input_pipe[1]);
                    p = n;
                    printf("prime %d\n", p);
                }
                else
                {
                    fprintf(2, "Error forking\n");
                    exit(1);
                }
            }
            // if there is a right neighbor, send the number to it
            else
            {
                write(output_pipe[1], &n, sizeof(n));
            }
        }
        if (p == 2)
        {
            n++;
        }
        else
        {
            read(input_pipe[0], &n, sizeof(n));
        }
    }
    // send the last number to the right neighbor to make it exit, and then close the needed pipes
    if (!(output_pipe[0] == 0 && output_pipe[1] == 0))
    {
        write(output_pipe[1], &n, sizeof(n));
        close(output_pipe[1]);
    }
    if (p != 2)
    {
        close(input_pipe[0]);
    }
    wait(0);
    exit(0);
}
