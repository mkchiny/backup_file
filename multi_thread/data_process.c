#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/poll.h>

#include "data_process.h"

#define DEBUG(fmt, arg...) printf("[%s:%d]"fmt"", __FILE__, __LINE__, ##arg)

typedef struct
{
    int running;
    int fd[2];
    pthread_t thread_id;
}private_data_t;

static void *s_data_process_thread(void *arg)
{
    DATA_PROCESS_ST *p_process = (DATA_PROCESS_ST *)arg;
    private_data_t *p_private = NULL;
    int ret = 0;
    struct pollfd pfds;
    char buf[1024 * 2];

    if(p_process == NULL)
    {
        DEBUG("null param, exit thread!!!");
        return NULL;
    }

    p_private = (private_data_t *)p_process->private_data;
    pfds.fd = p_private->fd[1];
    pfds.events = POLLIN;
    DEBUG("read fd %d\n", p_private->fd[1]);

    while(p_private->running)
    {
        ret = poll(&pfds, 1, p_process->time_out);
		if (ret < 0)
			continue;

        if(ret == 0)//time out
        {
            memset(buf, 0, sizeof(buf));
            p_process->data_process_func(buf, 0, p_process->user_data);
            continue;
        }

        if (!(pfds.revents & POLLIN))
			continue;
		pfds.revents = 0;

		do
        {
            memset(buf, 0, sizeof(buf));
            ret = read(p_private->fd[1], buf, sizeof(buf));
            if(ret > 0)
            {
                p_process->data_process_func(buf, ret, p_process->user_data);
            }
        }while(p_private->running && ret > 0);
    }

    return NULL;
}

int data_process_init(DATA_PROCESS_ST *data_process)
{
    int ret = 0;
    private_data_t *p_private = NULL;

    if(data_process == NULL)
    {
        DEBUG("data_process is null!!!\n");
        return -1;
    }

    p_private = malloc(sizeof(private_data_t));
    if(p_private == NULL)
    {
        DEBUG("malloc failed(%m)!!!\n");
        return -2;
    }

    do
    {
        memset(p_private, 0, sizeof(private_data_t));

        //create a msg handle
        ret = socketpair(AF_UNIX, SOCK_STREAM, 0, p_private->fd);
        if(ret != 0)
        {
            DEBUG("socketpair failed!!!(%m)\n");
            ret = -3;
            break;
        }

        p_private->running = 1;
         data_process->private_data = p_private;
        //create a thread
        ret = pthread_create(&p_private->thread_id, NULL, s_data_process_thread, data_process);
        if(ret != 0)
        {
            DEBUG("pthread_create failed!!!(%m)\n");
            ret = -4;
            break;
        }

        return 0;
    }while(0);

    if(p_private)
    {
        p_private->running = 0;
        if(p_private->thread_id > 0)
            pthread_join(p_private->thread_id, NULL);

        if(p_private->fd[0] > 0)
            close(p_private->fd[0]);

        if(p_private->fd[1] > 0)
            close(p_private->fd[1]);

        free(p_private);
        p_private = NULL;
    }

    return ret;
}

int data_process_uninit(DATA_PROCESS_ST *data_process)
{
    private_data_t *p_private = NULL;

    if(data_process == NULL)
    {
        DEBUG("data_process is null!!!\n");
        return -1;
    }

    p_private = (private_data_t *)data_process->private_data;
    if(p_private == NULL)
    {
        return 0;
    }

    p_private->running = 0;
    if(p_private->thread_id > 0)
        pthread_join(p_private->thread_id, NULL);

    if(p_private->fd[0] > 0)
        close(p_private->fd[0]);

    if(p_private->fd[1] > 0)
        close(p_private->fd[1]);

    free(p_private);
    p_private = NULL;

    data_process->private_data = NULL;

    return 0;
}

int data_process_write(DATA_PROCESS_ST *data_process, char *data, int len)
{
    private_data_t *p_private = NULL;
    int ret = 0;

    if(data_process == NULL)
    {
        DEBUG("data_process is null!!!\n");
        return -1;
    }

    p_private = (private_data_t *)data_process->private_data;
    ret = write(p_private->fd[0], data, len);

    return ret;
}

