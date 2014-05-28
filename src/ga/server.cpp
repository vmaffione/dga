#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace std;


void exit_with_error(const char *errmsg)
{
        perror(errmsg);
        exit(EXIT_FAILURE);
}

int manage_request(int fd)
{
#define BUFSIZE 128
        char buffer[BUFSIZE];
        int n;

        n = read(fd, buffer, sizeof(buffer));
        if (n < 0) {
                exit_with_error("read()");
        }

        n = write(fd, buffer, n);
        if (n < 0) {
                exit_with_error("write()");
        }

        return 0;
}

int main()
{
        int listen_fd, connection_fd;
        short int port = 9863;
        struct sockaddr_in server_address;
        int optval;
        int ret;

        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
                exit_with_error("creating listening socket");
        }

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_port = htons(port);

        optval = 1;
        ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                         &optval, sizeof(optval));
        if (ret < 0) {
                exit_with_error("setsockopt()");
        }

        ret = bind(listen_fd, (struct sockaddr *)&server_address,
                        sizeof(server_address));
        if (ret < 0) {
                exit_with_error("bind()");
        }

        ret = listen(listen_fd, 10);
        if (ret < 0) {
                exit_with_error("listen()");
        }

        for (;;) {
                connection_fd = accept(listen_fd, NULL, NULL);
                if (connection_fd < 0) {
                        exit_with_error("accept()");
                }

                cout << "Request received\n";

                manage_request(connection_fd);

                ret = close(connection_fd);
                if (ret < 0) {
                        exit_with_error("close()");
                }
        }

        return 0;
}
