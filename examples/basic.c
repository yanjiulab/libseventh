#include "seventh.h"

#include <arpa/inet.h>  // for inet_ntoa
#include <netinet/in.h> // for struct sockaddr_in
#include <signal.h>
#include <sys/socket.h>

evloop_t* loop;

static void print_data(unsigned char* data, int size) {
    int i, j;
    for (i = 0; i < size; i++) {
        if (i != 0 && i % 20 == 0)
            printf("\n");
        if (i % 20 == 0)
            printf("   ");
        printf(" %02X", (unsigned int)data[i]);
        if (i == size - 1)
            printf("\n");
    }
}

static void sig_int() {
    evloop_stop(loop);
}

static void on_idle(evidle_t* idle) {
    printf("on_idle: event_id=%llu\tpriority=%d\tuserdata=%ld\n", LLU(event_id(idle)), event_priority(idle),
           (long)(intptr_t)(event_userdata(idle)));
}

static void on_timer(evtimer_t* timer) {
    evloop_t* loop = event_loop(timer);
    printf("on_timer: event_id=%llu\tpriority=%d\tuserdata=%ld\ttime=%llus\thrtime=%lluus\n", LLU(event_id(timer)),
           event_priority(timer), (long)(intptr_t)(event_userdata(timer)), LLU(evloop_now(loop)),
           LLU(evloop_now_hrtime(loop)));
    printf("Hello World %ld\n", LU(event_userdata(timer)));
}

static void on_period(evtimer_t* timer) {
    evloop_t* loop = event_loop(timer);
    printf("on_period: event_id=%llu\tpriority=%d\tuserdata=%ld\ttime=%llus\thrtime=%lluus\n", LLU(event_id(timer)),
           event_priority(timer), (long)(intptr_t)(event_userdata(timer)), LLU(evloop_now(loop)),
           LLU(evloop_now_hrtime(loop)));
}

static void on_udp(evio_t* io) {
    char recvline[1024] = {0};
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(cliaddr);

    int n = recvfrom(io->fd, recvline, 1024, 0, (struct sockaddr*)&cliaddr, &addrlen);

    printf("Recv %d bytes from client [%s:%d]:\n", n, inet_ntoa(cliaddr.sin_addr),
           ntohs(((struct sockaddr_in*)&cliaddr)->sin_port));
    print_data(recvline, n);
    int s = sendto(io->fd, recvline, n, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
    // evio_del(io, EV_READ);
}

int main() {

    /* Setting signals and clean */
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        exit(EXIT_FAILURE);
    }

    /* Create an event loop */
    printf("Create an event loop\n");
    loop = evloop_new(EVLOOP_FLAG_AUTO_FREE);

    /* Add idle event */
    for (int i = -2; i <= 2; ++i) {
        evidle_t* idle = evidle_add(loop, on_idle, 1); // repeate times: 1
        event_set_priority(idle, i);
        event_set_userdata(idle, (void*)(intptr_t)(i * i));
    }

    /* Add timer event */
    evtimer_t* timer;

    // Add timer timeout
    for (int i = 1; i <= 3; ++i) {
        timer = evtimer_add(loop, on_timer, 1000, 3);
        event_set_userdata(timer, (void*)(intptr_t)i);
    }
    // Add timer period (every minute)
    timer = evtimer_add_period(loop, on_period, -1, -1, -1, -1, -1, 1);

    // Add UDP echo server
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8888);
    bind(udp_sock, (struct sockaddr*)&servaddr, sizeof(servaddr));
    evio_read(loop, udp_sock, on_udp);

    /* Run event loop */
    printf("Run event loop with engine %s\n", evio_engine());
    evloop_run(loop);
    printf("Bye~\n");
}