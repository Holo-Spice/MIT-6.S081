#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int pleft[2])
{
    int p;
    read(pleft[0], &p, sizeof(p));
    if (p == -1)
    {
        exit(0);
    }
    printf("prime %d\n", p);

    int pright[2];
    pipe(pright);

    if (fork() == 0)
    {
        close(pright[1]);
        close(pleft[0]);
        sieve(pright);
    }
    else
    {
        close(pright[0]);
        int buf;

        while (read(pleft[0], &buf, sizeof(buf)) && buf != -1)
        {
            if (buf % p != 0)
            {
                write(pright[1], &buf, sizeof(buf));
            }
        }
        buf = -1;
        write(pright[1], &buf, sizeof(buf));
        wait(0);
        exit(0);
    }
}

int main(int argc, char **argv)
{
    int intput_pip[2];
    pipe(intput_pip);

    if (fork() == 0)
    {
        close(intput_pip[1]);
        sieve(intput_pip);
        exit(0);
    }
    else
    {
        close(intput_pip[0]);
        for (int i = 2; i <= 35; i++)
        {
            write(intput_pip[1], &i, sizeof(i));
        }
        int end = -1;
        write(intput_pip[1], &end, sizeof(end));
    }
    wait(0);
    exit(0);
}