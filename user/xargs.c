#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 运行指定程序
void run(char *program, char **args)
{
    if (fork() == 0)
    {
        exec(program, args);
        fprintf(2, "xargs: exec %s failed\n", program);
        exit(0);
    }
}
int main(int argc, char **argv)
{
    char buf[2048];               // 缓存标准输入
    char *p = buf, *last_p = buf; // p指向当前读入的位置，last_p指向当前参数的起始位置
    char *argsbuf[128];           // 存储参数
    char **args = argsbuf;        // 指向当前参数存储的位置

    for (int i = 1; i < argc; i++)
    {
        *args = argv[i];
        args++;
    }

    // 记录当前参数位置
    char **pa = args;

    // 从标准输入读参数
    while (read(0, p, 1) != 0)
    {
        if (*p == ' ' || *p == '\n')
        {
            char c = *p; // 保存分隔符
            *p = '\0';

            if (last_p != p)
            {
                // 避免空参数
                *(pa++) = last_p;
            }
            last_p = p + 1;

            if (c == '\n')
            {
                *pa = 0;
                run(argv[1], argsbuf);
                pa = args;

                // 避免buf 溢出
                p = buf;
                last_p = buf;
                continue;
            }
        }
        p++;
    }
    if (pa != args)
    {
        *p = '\0';
        *(pa++) = last_p;
        *pa = 0;
        run(argv[1], argsbuf);
    }
    while (wait(0) != -1)
    {
    };
    exit(0);
}