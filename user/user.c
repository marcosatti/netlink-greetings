#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define NETLINK_TEST_PROTOCOL 31

void* main_recv(void *_sock_fd) {
        int sock_fd = (int)_sock_fd;
        int msg_length = NLMSG_HDRLEN + 256;

        struct nlmsghdr *nlh_recv = (struct nlmsghdr *)calloc(1,msg_length);

        printf("Recv thread started ok\n");

        while (1) {
                memset(nlh_recv, 0, msg_length);

                struct iovec iov;
                memset(&iov, 0, sizeof(iov));
                iov.iov_base = (void *) nlh_recv;
                iov.iov_len = msg_length;

                struct msghdr outermsghdr;
                memset(&outermsghdr, 0, sizeof(outermsghdr));
                outermsghdr.msg_iov = &iov;
                outermsghdr.msg_iovlen = 1;
                outermsghdr.msg_name = NULL;
                outermsghdr.msg_namelen = 0;

                int recv_ret = recvmsg(sock_fd, &outermsghdr, 0);
                if (recv_ret < 0) {
                        printf("Error receiving message\n");
                        exit(EXIT_FAILURE);
                }

                char *message = NLMSG_DATA(nlh_recv);
                printf("Read %d bytes\n", recv_ret);
                printf("Message: %s\n", message);
        }
}

static int send_netlink_msg_to_kernel(int sock_fd, char *msg, uint32_t msg_size, int nlmsg_type, uint16_t flags) {
        struct sockaddr_nl dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.nl_family = AF_NETLINK;
        dest_addr.nl_pid = 0;

        struct nlmsghdr *nlh = (struct nlmsghdr *)calloc(1, NLMSG_HDRLEN + NLMSG_SPACE(msg_size));
        nlh->nlmsg_len = NLMSG_HDRLEN + NLMSG_SPACE(msg_size);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_type = nlmsg_type;
        nlh->nlmsg_seq = 0;
        nlh->nlmsg_flags = flags;

        strncpy(NLMSG_DATA(nlh), msg, msg_size);

        struct iovec iov;
        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;

        struct msghdr outermsghdr;
        memset(&outermsghdr, 0, sizeof(struct msghdr));
        outermsghdr.msg_name = (void *)&dest_addr;
        outermsghdr.msg_namelen = sizeof(dest_addr);
        outermsghdr.msg_iov = &iov;
        outermsghdr.msg_iovlen = 1;

        return sendmsg(sock_fd, &outermsghdr, 0);
}

int main(char *argv[], int argc) {
        int sock_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_TEST_PROTOCOL);
        if (sock_fd < 0) {
                printf("Failed to create socket\n");
                return EXIT_FAILURE;
        }

        struct sockaddr_nl src_addr;
        memset(&src_addr, 0, sizeof(src_addr));
        src_addr.nl_family = AF_NETLINK;
        src_addr.nl_pid = getpid();

        int bind_ret = bind(sock_fd, (const struct sockaddr*)&src_addr, sizeof(src_addr));
        if (bind_ret < 0) {
                printf("Failed to bind socket\n");
                return EXIT_FAILURE;
        }

        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

        pthread_t thread_id;
        pthread_create(&thread_id, &thread_attr, main_recv, (void *)sock_fd);

        int count = 0;

        while (1) {
                printf("Press enter to send message\n");
                char ch = getchar();

                if (ch == 'q') {
                        printf("Quitting\n");
                        return EXIT_SUCCESS;
                }

                if (ch == '\n') {
                        char message[256] = { 0 };
                        int message_size = sizeof(message);
                        snprintf(message, message_size, "Hello world %d", count);
                        count++;
                        
                        uint16_t flags = NLM_F_ACK;

                        int send_ret = send_netlink_msg_to_kernel(sock_fd, message, message_size, 1, flags);
                        if (send_ret < 0) {
                                printf("Failed to send message\n");
                                return EXIT_FAILURE;                        
                        }
                }
        }

        return EXIT_SUCCESS;
}