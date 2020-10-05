/*
 * 这是后台进程，作用是popen执行命令，并将命令返回结果通过 有名管道 发送给前台程序
 * */
#include <stdio.h>
#include <stdlib.h>
#define BUF_SIZE 1024
#define MAX_SIZE 1024

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/msg.h>
#include <sys/ipc.h>
typedef struct mymesg
{
    long int mtype;
    char mtext[512];
}mymesg;

int main(int argc, char *argv[])
{
    /*
     * Step1:使用消息队列接收消息
     *
     */
    int id = 0;
    mymesg ybx_msg;
    key_t key = ftok("my_message", 52);
    id = msgget(key, IPC_CREAT | 0666); //打开或者创建队列
    if (-1 == id) {
          perror("create msg error");
          exit(EXIT_FAILURE);
    }

    //一直接收消息，直到遇到 exit 终止
    while(1)
    {
        if (-1 == msgrcv(id, (void *)&ybx_msg, sizeof(ybx_msg.mtext), 1,0)) {
            perror("Recive msg error ");
            exit(EXIT_FAILURE);
        }
        if(strncmp(ybx_msg.mtext,"exit",4) ==0)
        {
            break;
        }
//        if(0 == strncmp(ybx_msg.mtext, "cd", 2))
//        {
//            char tmp_path[100];
//            char *p;
//            char *buff;
//            buff = ybx_msg.mtext;
//            p = strsep(&buff, " ");
//            p = strsep(&buff, " ");
//            memset(tmp_path, 0x00, sizeof(tmp_path));
//            strncat(tmp_path, p, strlen(p)-1);
//            //此时 tmp_path 是cd要到的目录
//            printf("You will cd %s,the length of path=%ld\n", tmp_path,       strlen(tmp_path));
//             if(-1 == chdir(tmp_path))
//             {
//                 perror("cd failed");
//             }
//        }

        //将每个命令后面追加 2>&1 
        // 这样 popen 就可以把错误信息也打印出来了
        char linux_cmd[MAX_SIZE] = {'\0'};
        strncpy(linux_cmd, ybx_msg.mtext, strlen(ybx_msg.mtext)-1); //-1的目的是因为 ybx_msg.mtext 的结尾是 \n ，这导致追加的信息打印不到终端上面。
        char append[6] = " 2>&1\n";
        strncat(linux_cmd, append, 6);

        /*
         * Step2：使用 popen 
         *        来执行 Linux 命令
         */
        char buf[BUF_SIZE];
        memset(buf, 0x00, sizeof(buf));
        FILE *popen_f = popen(linux_cmd, "r");
        if(!popen_f) //错误检测
        {
            fprintf(stderr, "Erro to popen");
        }
    
        /*
         * Step3：使用管道发送命令,写入到管道文件里面
         */
        char *p_f = "my_fifo";
        int fd = open(p_f, O_WRONLY| O_NONBLOCK, S_IRWXU);
        if (-1 == fd) { //错误检测
            perror("open error");
        }
        
        while(1)
        {
            //因为 popen 的执行结果可能是多行，所以需要while循环打出
            while(fgets(buf, BUF_SIZE, popen_f)!= NULL)
            {
                write(fd, buf, sizeof(buf));
                memset(buf, 0x00, sizeof(buf));

            }
            //发送一个信号量告诉 forward的 read 结束了，需要跳出循环
            write(fd, "EOF", sizeof("EOF"));
            break;
        }

        close(fd);
        pclose(popen_f);
    }

    return 0;
}

