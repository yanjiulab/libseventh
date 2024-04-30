#include "seventh.h"

#include <arpa/inet.h>  // for inet_ntoa
#include <netinet/in.h> // for struct sockaddr_in
#include <signal.h>
#include <sys/socket.h>

evloop_t* loop;
int r = 0, s = 0;

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

static void on_recv(evio_t* io) {
    r++;
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

static void on_send(evio_t* io) {
    printf("Send\n");
    s++;
}

int main() {

    /* Setting signals and clean */
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        exit(EXIT_FAILURE);
    }

    /* Create an event loop */
    printf("Create an event loop\n");
    loop = evloop_new(EVLOOP_FLAG_AUTO_FREE);

    // Add UDP echo server
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8888);
    bind(udp_sock, (struct sockaddr*)&servaddr, sizeof(servaddr));

    evio_read(loop, udp_sock, on_recv);
    evio_write(loop, udp_sock, on_send);

    /* Run event loop */
    printf("Run event loop with engine %s\n", evio_engine());
    evloop_run(loop);
    printf("recv: %d, send: %d\n", r, s);
    printf("Bye~\n");
}