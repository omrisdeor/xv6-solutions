#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int pipe_fd[2] = {0};
    pipe(pipe_fd);
    int pid = fork();
    if(pid > 0)
    {
        char buf[1] = {'a'};
        write(pipe_fd[1], buf, 1);
        close(pipe_fd[1]);
        read(pipe_fd[0], buf, 1);
        close(pipe_fd[0]);
        printf("%d: received pong\n", getpid());
        exit(0);
    }
    else if(pid == 0)
    {
        char buf[1];
        read(pipe_fd[0], buf, 1);
        close(pipe_fd[0]);
        printf("%d: received ping\n", getpid());
        write(pipe_fd[1], buf, 1);
        close(pipe_fd[1]);
        exit(0);
    }
    else
    {
        fprintf(2, "Error forking\n");
        exit(1);
    }
}
