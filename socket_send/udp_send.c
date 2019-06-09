#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/poll.h>
#include <signal.h>
#include <time.h>

int running = 1;

static void terminate(int sig)
{
	printf("oops:  terminate\n");

	running = 0;
	exit(1);
}

unsigned int get_system_time(void)
{
    struct timespec time = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &time);

    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

unsigned int get_system_time_us(void)
{
    struct timespec time = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &time);

    return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

uint64_t get_pcr_value(unsigned char *pos)
{
    /*
    48 bit 0, 1, 2, 3, 4, 5
    33bit pcr base 6bit revered 9bit pcr ext
    |----0----|----1----|----2----|----3----|- ---4--- -|----5----|
    */
    uint64_t pcr_base = (pos[0] << 25) | (pos[1] << 17) | (pos[2] << 9 )  | (pos[3] << 1) | ((pos[4] & 0x80) >> 7);
    uint64_t pcr_ext = ((pos[4] & 0x01) << 8) | pos[5];

    return pcr_base * 300 + pcr_ext;
}

#define DOUBLE_EPS 1e-5
static double ts_rate = 0;
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

    static uint64_t pcr_pre = 0;
    static uint64_t pcr_cur = 0;
    static int pcr_pid = -1;
    static int count = 0;

    if(pcr_pid != -1 && pcr_pre != 0)
    {
        count++;
    }

    if(adaptation_field_control == 0x2 || adaptation_field_control == 0x3)
    {
        unsigned char *p_adaptation_field = buf +4;
        int pcr_flag = (p_adaptation_field[1] >> 4) & 0x01;

        if(pcr_flag == 1)
        {
            if(pcr_pre == 0)
            {
                pcr_pre = get_pcr_value(p_adaptation_field + 2);
                if(pcr_pid == -1)
                    pcr_pid = PID;
            }
            else if(pcr_pid == PID)
            {
                pcr_cur = get_pcr_value(p_adaptation_field + 2);
            }
        }
    }

    if(pcr_pre != 0 && pcr_cur != 0)
    {
        uint64_t abs_pcr = (pcr_pre > pcr_cur) ? (pcr_pre - pcr_cur) : (pcr_cur - pcr_pre);
        if(abs_pcr > 0)
        {
            ts_rate = 1.0 * (count * 188 * 27) / abs_pcr;//MBps
//            printf("%fkbps\n", ts_rate * 1024 * 8);
        }
        else
        {
            printf("\033[32m[%s:%d]\033[36m abs_pcr = %lu byte\033[0m\n", __FUNCTION__, __LINE__, abs_pcr);
        }

        pcr_pre = 0;
        pcr_cur = 0;
        count = 0;
    }

    return 1;
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
            //todo print speed
            unsigned int time_diff = get_system_time() - time_pre;
            double speed = 0;
            if(time_diff > 0)
            {
                speed = 1.0 *total_len / time_diff;//byte/ms
 //               printf("\033[32m[%s:%d]\033[36m speed = %f byte/ms\033[0m\n", __FUNCTION__, __LINE__, speed);
                speed = speed * 1000;
#if 1
                printf("\033[32m[%s:%d]\033[36m speed = %f MB/s ts_rate = %f MB/s\033[0m\n", __FUNCTION__, __LINE__, speed / (1024 * 1024), ts_rate);
#else
                int int_speed = (int)speed;
                if(int_speed < 1024)
                    printf("\033[32m[%s:%d]\033[36m speed = %f Byte/s\033[0m\n", __FUNCTION__, __LINE__, speed);
                else if(int_speed < 1024 * 1024)
                    printf("\033[32m[%s:%d]\033[36m speed = %f KB/s\033[0m\n", __FUNCTION__, __LINE__, speed / 1024);
                else
                    printf("\033[32m[%s:%d]\033[36m speed = %f MB/s\033[0m\n", __FUNCTION__, __LINE__, speed / (1024 * 1024));
#endif // 1
            }

            time_pre = 0;
            total_len = 0;
            time_print = 0;
        }
    }
}

void time_to_sleep_us(unsigned int time_us)
{
    struct timespec ts;

    ts.tv_nsec = (time_us % 1000000) * 1000;
    ts.tv_sec = time_us / 1000000;
    nanosleep(&ts, NULL);
}

int udp_send_data(int fd, struct sockaddr_in addr, char *data, int data_len)
{
    int max_send_len = 7 * 188;
    static char buf[7*188 * 100];
    static int exist_len = 0;
    int send_len = 0;
    int ret = 0;

    while(data_len > 0)
    {
        if(data_len >= (max_send_len - exist_len))
        {
            memcpy(buf + exist_len, data, max_send_len - exist_len);
            data += (max_send_len - exist_len);
            send_len += (max_send_len - exist_len);
            data_len = data_len - (max_send_len - exist_len);
            exist_len = 0;
        }
        else
        {
            memcpy(buf + exist_len, data, data_len);
            data += data_len;
            exist_len += data_len;
            send_len += data_len;
            data_len = 0;
            return send_len;
        }

        static unsigned int time_pre_send = 0;
        static double time_to_send = 0;

        if(time_pre_send > 0)
        {
            int time_to_sleep = time_to_send * 1000000 - (get_system_time_us() - time_pre_send);
            if(time_to_sleep > 0)
            {
                //time_to_sleep_us(time_to_sleep);
                //usleep(time_to_sleep);
            }
        }

        time_pre_send = get_system_time_us();
        time_to_send = 0;
        if(ts_rate > DOUBLE_EPS)
            time_to_send = 1.0 * max_send_len / (ts_rate * 1024 *1024/*MBps*/);//s

        ret = sendto(fd, buf, max_send_len, MSG_NOSIGNAL, (struct sockaddr*)&addr, sizeof(addr));
        if(ret <= 0)
        {
            printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
            break;
        }

        if(ret != max_send_len)
            printf("\033[32m[%s:%d]\033[36m ret = %d\033[0m\n", __FUNCTION__, __LINE__, ret);
        print_send_speed(max_send_len);
    }

    return send_len;
}

void udp_send(FILE *fp)
{
	int socket_fd = -1;
	struct sockaddr_in 	addr;
	char buf[1024 * 3];
	int ret = 0;

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_fd <= 0)
	{
		printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
		return;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(1121);
	inet_aton("224.0.1.160", &addr.sin_addr);

	while (running)
	{
		memset(buf, 0, sizeof(buf));
#if 0
		ret = read_ts_packet(fp, buf);
		if(ret != 0)
		{
			printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
			continue;
		}
		ret = 188;
		ret = udp_send_data(socket_fd, addr, buf, ret);
#else
		ret = fread(buf, 1, sizeof(buf), fp);
		if(ret <= 0)
		{
			printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
			fseek(fp, 0, SEEK_SET);
			continue;
		}

		ret = udp_send_data(socket_fd, addr, buf, ret);
		if(ret <= 0)
		{
			printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
			usleep(1000 * 1000);
			continue;
		}
#endif
	}

	if(socket_fd != -1)
		close(socket_fd);
}

void udp_send_new(FILE *fp)
{
	int socket_fd = -1;
	struct sockaddr_in 	addr;
	char buf[7 * 188 * 100];
	int ret = 0;

	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_fd <= 0)
	{
		printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
		return;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(1121);
	inet_aton("224.0.1.160", &addr.sin_addr);

	int exist_len = 0;
	int offset = 0;
	int total = 0;
	int parse_len = 0;

	while (running)
	{
		memset(buf + exist_len, 0, sizeof(buf) - exist_len);
		ret = fread(buf + exist_len, 1, sizeof(buf) - exist_len, fp);
		if(ret <= 0)
		{
		    printf("\033[32m[%s:%d]\033[36m exist_len = %d ret = %d\033[0m\n", __FUNCTION__, __LINE__, exist_len, ret);
			fseek(fp, 0, SEEK_SET);
			continue;
		}

		total = ret + exist_len;
		parse_len = 0;
		exist_len = 0;
		offset = 0;

		unsigned int skip_bytes = 0;

		while((offset + 189) < total)
        {
            if(buf[offset] != 0x47 || buf[offset + 188] != 0x47)
            {
                offset++;
                parse_len++;
                skip_bytes++;
                continue;
            }

            if(check_valid_packet((unsigned char *)(buf + offset)))
            {
                ret = udp_send_data(socket_fd, addr, buf + offset, 188);
                if(ret != 188)
                {
                    printf("\033[32m[%s:%d]\033[36m ret = %d\033[0m\n", __FUNCTION__, __LINE__, ret);
                }
            }

            if(skip_bytes > 0)
                printf("skip bytes %u\n", skip_bytes);
            offset += 188;
            parse_len += 188;
            skip_bytes = 0;
        }

        exist_len = total - parse_len;
	}

	if(socket_fd != -1)
		close(socket_fd);
}

void http_response(int fd)
{
	char buf[1024];

	snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\r\nContent-Type: video/mpeg\r\n\r\n");
	send(fd, buf, strlen(buf), MSG_NOSIGNAL);
	printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
}

void http_send(FILE *fp)
{
	int http_listenfd = 0;
	struct sockaddr_in server_addr;
	int ret = 0;
	struct sockaddr_in client_addr;
	socklen_t client_length = sizeof(client_addr);
	int clientfd = 0;
	char buf[1024 * 3];

	http_listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(http_listenfd < 0)
		return;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(8880);
	ret = bind(http_listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret < 0)
	{
		close(http_listenfd);
		return;
	}

	ret = listen(http_listenfd, 100);
	if (ret<0)
	{
		close(http_listenfd);
		return;
	}

	while(running)
	{
		clientfd = accept(http_listenfd, (struct sockaddr*)&client_addr, &client_length);
		if(clientfd <= 0)
		{
			usleep(1000 * 10);
			continue;
		}
		http_response(clientfd);

		while (running)
		{
			memset(buf, 0, sizeof(buf));
#if 1
			ret = read_ts_packet(fp, buf);
			if(ret != 0)
			{
				printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
				usleep(1000 * 10);
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
			ret = send(clientfd, buf, ret, MSG_NOSIGNAL);
			if(ret <= 0)
			{
				switch (errno)
				{
					case ECONNRESET:
					case EPIPE:
						//running = 0;
						break;
					default:
						break;
				}
				printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
				close(clientfd);
				usleep(1000 * 1000);
				break;
			}
			//usleep(1000 * 30);
		}
	}

	close(http_listenfd);
}

int main(int argc, char *argv[])
{
	char *file_name = NULL;
	FILE *fp = NULL;

	signal(SIGINT , terminate); /* Interrupt (ANSI).    */

#if 0
	if(argc <= 1)
	{
		printf("\033[32m[%s:%d]\033[36m \033[0m\n", __FUNCTION__, __LINE__);
		return -1;
	}
	file_name = argv[1];
#else
    file_name = "E:/ts/test.ts";
#endif

	fp = fopen(file_name, "r");
	if(fp == NULL)
	{
		printf("\033[32m[%s:%d]\033[36m %m\033[0m\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(argc >= 3)
	{
		http_send(fp);
	}
	else
	{
		//udp_send(fp);
		//udp_send_new(fp);
		http_send(fp);
	}

	if(fp)
		fclose(fp);

	return 0;
}
