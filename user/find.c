#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


#define DIRSIZ 14

struct dirent {
    ushort inum;
    char name[DIRSIZ];
};

char* get_file_name(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path) - 1; p >= path && *p != '/'; p--);
    if (p != path)
    {
        p++;
    }

    if(strlen(p) >= DIRSIZ)
    {
        return p;
    }
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), 0, DIRSIZ-strlen(p));
    return buf;
}

void find(char *path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        exit(1);
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit(1);
    }

    switch (st.type)
    {
    case T_DEVICE:
    case T_FILE:
        if (strcmp(get_file_name(path), name) == 0)
        {
            printf("%s\n", path);
        }
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if(de.inum == 0)
            {
                continue;
            }
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0)
            {
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            if (!(strcmp(get_file_name(buf), ".") == 0 || strcmp(get_file_name(buf), "..") == 0))
            {
                find(buf, name);
            }            
        }
        break;
    }
    close(fd);
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(2, "Usage: find <path> <name>\n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}