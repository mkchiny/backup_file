#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "data_process.h"

typedef struct
{
    int fd;
    struct sockaddr_in addr;
}client_t;

static int running = 0;

unsigned int get_system_time(void)
{
    struct timespec time = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &time);

    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}


int check_valid_packet(unsigned char *buf)
{
    int sync_byte = buf[0];
	int transport_error_indicator = buf[1] >> 7;
	int payload_unit_start_indicator = (buf[1] >> 6) & 1;
	int transport_priority = (buf[1] >> 5) & 1;
	int PID = (buf[1] & 0x1f) << 8 | buf[2];
	int transport_scrambling_control = buf[3] >> 6;
	int adaptation_field_control = (buf[3] >> 4) & 0x03;
	int continuity_counter = buf[3] & 0x0f;

	if(sync_byte != 0x47 || transport_error_indicator == 1)
	printf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", sync_byte, transport_error_indicator, payload_unit_start_indicator, \
        transport_priority, PID, transport_scrambling_control, adaptation_field_control, \
        continuity_counter);

    return 0;
}

int read_ts_packet(FILE *fp, char *buf)
{
	int func_ret = -1;
	char ch = 0;
	char read_buf[256];
	int ret = 0;
	int skip_byte = 0;

	while(running)
	{
AGAIN:
		ret = fread(&ch, 1, 1, fp);
		if(ret != 1)
		{
			fseek(fp, 0, SEEK_SET);
			break;
		}

		if(ch != 0x47)
		{
		    skip_byte++;
			continue;
		}

		memset(read_buf, 0, 188);
		ret = fread(read_buf, 1, 187, fp);
		if(ret != 187)
		{
			fseek(fp, 0, SEEK_SET);
			break;
		}

		ret = fread(&ch, 1, 1, fp);
		if(ret != 1)
		{
			fseek(fp, 0, SEEK_SET);
			break;
		}

		if(ch != 0x47)
		{
			fseek(fp, -188, SEEK_CUR);
			goto AGAIN;
		}

		if(skip_byte > 0)
            printf("\033[32m[%s:%d]\033[36m skip_byte = %d byte\033[0m\n", __FUNCTION__, __LINE__, skip_byte);
		buf[0] = ch;
		memcpy(&buf[1], read_buf, 187);
		fseek(fp, -1, SEEK_CUR);
		func_ret = 0;
		check_valid_packet((unsigned char *)buf);
		break;
	}

	return func_ret;
}

void print_send_speed(int send_len)
{
    static unsigned int time_pre = 0;
    static unsigned int time_print = 0;
    static unsigned int total_len = 0;

    if(send_len > 0)
        total_len += send_len;

    if(time_pre == 0)
        time_pre = get_system_time();

    if(time_print == 0)
         time_print = get_system_time();
    else
    {
        if(get_system_time() - time_print > 2 * 1000)
        {
            time_print = 0;

            //todo print speed
            unsigned int time_diff = get_system_time() - time_pre;
            double speed = 0;
            if(time_diff > 0)
            {
                speed = 1.0 *total_len / time_diff;//byte/ms
 //               printf("\033[32m[%s:%d]\033[36m speed = %f byte/ms\033[0m\n", __FUNCTION__, __LINE__, speed);
                speed = speed * 1000;
                int int_speed = (int)speed;
                if(int_speed < 1024)
                    printf("\033[32m[%s:%d]\033[36m speed = %f Byte/s\033[0m\n", __FUNCTION__, __LINE__, speed);
                else if(int_speed < 1024 * 1024)
                    printf("\033[32m[%s:%d]\033[36m speed = %f KB/s\033[0m\n", __FUNCTION__, __LINE__, speed / 1024);
                else
                    printf("\033[32m[%s:%d]\033[36m speed = %f MB/s\033[0m\n", __FUNCTION__, __LINE__, speed / (1024 * 1024));
            }
        }
    }
}

int udp_send_data(int fd, struct sockaddr_in addr, char *data, int data_len)
{
    int max_send_len = 7 * 188;
    static char buf[512*188];
    static int exist_len = 0;
    int send_len = 0;
    int ret = 0;

    while(data_len > 0)
    {
        if(data_len >= (max_send_len - exist_len))
        {
            memcpy(buf + exist_len, data, max_send_len - exist_len);
            data += (max_send_len - exist_len);
            send_len += max_send_len;
            data_len = data_len - (max_send_len - exist_len);
            exist_len = 0;
        }
        else
        {
            memcpy(buf + exist_len, data, data_len);
            data += data_len;
            exist_len += data_len;
            data_len = 0;
            send_len = exist_len;
            return send_len;
        }

        ret = sendto(fd, buf, max_send_len, MSG_NOSIGNAL, (struct sockaddr*)&addr, sizeof(addr));
        if(ret <= 0)
        {
            printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
            break;
        }
        if(ret != max_send_len)
            printf("\033[32m[%s:%d]\033[36m ret = %d\033[0m\n", __FUNCTION__, __LINE__, ret);
     //   usleep(1);
        print_send_speed(max_send_len);
    }

    return send_len;
}

int udp_send_func(char *data, int data_len, void *user_data)
{
    client_t *client = user_data;

    if(client == NULL || data == NULL || data_len <= 0)
    {
        return -1;
    }

    udp_send_data(client->fd, client->addr, data, data_len);
    return 0;
}

void udp_send(FILE *fp)
{
    client_t client;
    DATA_PROCESS_ST stProcess;
    int ret = 0;

    client.fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(client.fd <= 0)
	{
		printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
		return;
	}

	client.addr.sin_family = AF_INET;
	client.addr.sin_port = htons(1121);
	inet_aton("224.0.1.160", &client.addr.sin_addr);

	stProcess.time_out = 0;
	stProcess.user_data = &client;
	stProcess.data_process_func = udp_send_func;
    ret = data_process_init(&stProcess);
    if(ret != 0)
    {
        close(client.fd);
        printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
        return;
    }

    char buf[1024 * 3];

    while (running)
    {
        memset(buf, 0, sizeof(buf));
#if 1
        ret = read_ts_packet(fp, buf);
		if(ret != 0)
		{
			printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
			continue;
		}
		ret = 188;
#else
        ret = fread(buf, 1, sizeof(buf), fp);
        if(ret <= 0)
        {
            printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
            fseek(fp, 0, SEEK_SET);
            continue;
        }
#endif
        data_process_write(&stProcess, buf, ret);
    }

    data_process_uninit(&stProcess);
    close(client.fd);
}

void data_server_test(void)
{
    char *file_name = "test.ts";
    file_name = "[TS]10bits_H265_1080p60__10M.ts";
	FILE *fp = NULL;

    fp = fopen(file_name, "r");
	if(fp == NULL)
	{
		printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
		return;
	}

	running = 1;
	udp_send(fp);

	fclose(fp);
}
