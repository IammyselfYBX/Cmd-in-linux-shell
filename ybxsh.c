#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

void replace(char *src, char *replacement, char *result)
{
    char *p;
    char *buff;
    buff = src;
    p = strsep(&buff, " ");
    p = strsep(&buff, " ");
    memset(result, 0x00, sizeof(result));
    strncpy(result, replacement, strlen(replacement));
    while(p!=NULL)
    {
        strncat(result, p, strlen(p));
        strncat(result, " ", 1);
        p = strsep(&buff, " ");
    }
}

typedef struct mymesg
{
    long int mtype;
    char mtext[512];
}mymesg;

int main(int argc, char *argv[])
{
    /*
     * Step1:fork 
     *  这里父进程执行 execl 后台程序
     *  子进程执行下面的 前端程序(创建消息队列，有名管道输出结果)
     * */
    pid_t fpid;
    fpid = fork();
    if(fpid < 0)
    {
        perror("Fork error ");
        exit(EXIT_FAILURE);
    }else if (fpid > 0) { //父进程
        if(-1 == execl("./background.bin", "exec background", NULL))
        {
            perror("Execl error ");
        }
    }
    //接下来默认的就是 fipd == 0的子进程的操作


    /*
     * Step2:创建消息队列
     *  发送经过转化后的消息给background
     * */
    int id = 0;
    mymesg ybx_msg;
    key_t key = ftok("my_message", 52);
    id = msgget(key, IPC_CREAT | 0666); //打开或者创建队列
    if (-1 == id) {
        perror("create msg error");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        char msg[256];
        memset(msg, 0x00, sizeof(msg));
        ybx_msg.mtype = 1;
        printf("\033[01;35m[YBXshell]\033[0m \033[01;33m👉 \033[0m");
        fgets(msg, sizeof(msg), stdin);
        //下面实现Windows与Linux命令转换
        /*
         * 至少实现如下 Windows——Linux 对照命令:
         *    dir——ls
         *    rename——mv
         *    move——mv
         *    del——rm
         *    cd——cd(pwd)
         *    exit——exit
         * */
        if(!strncmp(msg, "dir", 3))
        {
            replace(msg, "ls ", ybx_msg.mtext);
        }else if(!strncmp(msg, "rename", 6))
        {
            replace(msg, "mv ", ybx_msg.mtext);
        }else if(!strncmp(msg, "move", 4))
        {
            replace(msg, "mv ", ybx_msg.mtext);
        }else if(!strncmp(msg, "del", 3))
        {
            replace(msg, "rm ", ybx_msg.mtext);
        }else
        {
            strcpy(ybx_msg.mtext,msg);
        }

        if (-1 == msgsnd(id, (void *)&ybx_msg, sizeof(ybx_msg.mtext), IPC_NOWAIT)) {
            perror("Send msg error: ");
            exit(EXIT_FAILURE);
        }

        if(strncmp(msg,"exit",4) == 0)
			  {
            break;
        }

        /*
         * 在 Step2 与 Step3 之间需要等待 background 的执行结果
         * */

        /*
         * Step3: 命名管道
         *  使用命名管道接收获得的执行的结果
         *  并显示在终端上面
         * */
        char *p_f = "my_fifo";
        int fd = open(p_f, O_RDONLY);
        if(-1 == fd)
        {
            perror("open error");
        }

        char buf[256];
        //判断输出结束
        while(1){
            memset(buf, 0x00, sizeof(buf));
            int ret = read(fd, buf, sizeof(buf));
            if(-1 == ret)
            {
                perror("read error");
            }

            if(strcmp(buf, "EOF"))
            {
              printf("%s", buf);
            }else{
                break;
            }
        }
        close(fd);

    }

    //最终释放消息队列
    if(-1 == msgctl(id, IPC_RMID, NULL)) //IPC_RMID从系统内核中移除消息队列
    {
        perror("Delete msg error ");
        exit(EXIT_FAILURE);
    }

    return 0;
}
