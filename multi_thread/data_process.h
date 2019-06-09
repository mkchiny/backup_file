#ifndef DATA_PROCESS_H_INCLUDED
#define DATA_PROCESS_H_INCLUDED

#include <pthread.h>

typedef struct
{
    int time_out;
    void *user_data;
    int (*data_process_func)(char *data, int data_len, void *user_data);

    void *private_data;
}DATA_PROCESS_ST;

extern int data_process_init(DATA_PROCESS_ST *data_process);
extern int data_process_uninit(DATA_PROCESS_ST *data_process);
extern int data_process_write(DATA_PROCESS_ST *data_process, char *data, int len);

#endif // DATA_PROCESS_H_INCLUDED
