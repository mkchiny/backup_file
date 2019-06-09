#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/un.h>

#define max_num 4

typedef struct
{
    int fd;
    struct sockaddr_in addr;
    int addr_len;
}local_client_t;

void *read_process(void *arg)
{
    local_client_t *handle = (local_client_t *)arg;
    char buf[128];
    int ret = 0;

    while(1)
    {
        memset(buf, 0, sizeof(buf));
        ret = recvfrom(handle->fd, buf, sizeof(buf), 0, (struct sockaddr*)&handle->addr, &handle->addr_len);
        if(ret > 0)
        {
            printf("handle = %d, %s\n", handle->fd, buf);
        }
        else
        {
            printf("handle = %d, ret = %d(%d:%m)\n", handle->fd, ret, __LINE__);
        }
    }
    return NULL;
}

void *write_process(void *arg)
{
    local_client_t *handle = (local_client_t *)arg;
    char buf[128];
    int len = 0;
    int ret = 0;

    while(1)
    {
        snprintf(buf, sizeof(buf), "write %d", handle->fd);
        len = strlen(buf);
        ret = sendto(handle->fd, buf, len, 0, (struct sockaddr*)&handle->addr, handle->addr_len);
        if(ret != len)
        {
            printf("handle = %d, ret = %d, len = %d (%d:%m)\n", handle->fd, ret, len, __LINE__);
        }
        sleep(1);
    }
    return NULL;
}

int local_test()
{
    pthread_t write_id[max_num];
    pthread_t read_id[max_num];
    local_client_t handle[max_num];
    int i = 0;
    int ret = 0;

    for(i = 0; i < max_num; i++)
    {
        handle[i].fd = socket(AF_INET, SOCK_DGRAM, 0);

        handle[i].addr.sin_family = AF_INET;
        handle[i].addr.sin_addr.s_addr = htonl(INADDR_ANY);
        handle[i].addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        handle[i].addr.sin_port = htons(0);
        handle[i].addr_len = sizeof(struct sockaddr_in);
        ret = bind(handle[i].fd, (struct sockaddr*)&(handle[i].addr), handle[i].addr_len);
        if(ret != 0)
        {
            printf("ret = %d, (%d:%m)\n", ret, __LINE__);
        }

        printf("addr_len = %d\n", handle[i].addr_len);
        getsockname(handle[i].fd, (struct sockaddr*)&(handle[i].addr), &handle[i].addr_len);
        printf("addr_len = %d\n", handle[i].addr_len);
        printf("i = %d addr = %s port = %d\n", i, inet_ntoa(handle[i].addr.sin_addr), handle[i].addr.sin_port);
        ret = pthread_create(&write_id[i], NULL, write_process, &handle[i]);
        if(ret != 0)
        {
            printf("ret = %d, (%d:%m)\n", ret, __LINE__);
        }
        ret = pthread_create(&read_id[i], NULL, read_process, &handle[i]);
        if(ret != 0)
        {
            printf("ret = %d, (%d:%m)\n", ret, __LINE__);
        }
    }

    for(i = 0; i < max_num; i++)
    {
        pthread_join(write_id[i], NULL);
        pthread_join(read_id[i], NULL);
        close(handle[i].fd);
    }

    return 0;
}
