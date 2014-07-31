#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ga.hpp"

int l = 0;

using namespace std;


int work(GAPeerServer *serv);

static GAPeerServer *server = NULL;

static void *
server_function(void *arg)
{
    server->run();

    return NULL;
}

static void
sigint_handler(int signum)
{
    server->leave();

    exit(EXIT_SUCCESS);
}

static void
print_usage()
{
    cout << "Options usage:" << endl;
    cout << "\t-h: show this help" << endl;
    cout << "\t-p PORTNUM: port number of the peer server" << endl;
    cout << "\t-l IPADDR : ip address of the local peer server" << endl;
    cout << "\t-j PORTNUM: port number of the remote server to join" << endl;
    cout << "\t-r IPADDR : ip address of the remote server to join" << endl;
    cout << endl;
}

static bool is_ipaddr(const string& s)
{
    int ret;
    char buf[sizeof(struct in_addr)];

    ret = inet_pton(AF_INET, s.c_str(), buf);

    return ret > 0;
}

int
main(int argc, char **argv)
{
    unsigned int s_port = ~0U;
    unsigned int j_port = ~0U;
    string s_ip("127.0.0.1");
    string j_ip("127.0.0.1");
    struct sigaction sa;
    pthread_t server_tid;
    int ret;
    int ch;

    while ((ch = getopt(argc, argv, "hp:j:l:r:")) != -1) {
        switch (ch) {
            case 'p':
                s_port = atoi(optarg);
                if (s_port >= 65535) {
                    errno = EINVAL;
                    exit_with_error("server port > 65535");
                }
                break;

            case 'j':
                j_port = atoi(optarg);
                if (j_port >= 65535) {
                    errno = EINVAL;
                    exit_with_error("join port > 65535");
                }
                break;

            case 'l':
                s_ip = string(optarg);
                if (!is_ipaddr(s_ip)) {
                    errno = EINVAL;
                    exit_with_error("invalid server IP address");
                }
                break;

            case 'r':
                j_ip = string(optarg);
                if (!is_ipaddr(j_ip)) {
                    errno = EINVAL;
                    exit_with_error("invalid remote IP address");
                }
                break;

            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
                break;
        }
    }

    if (s_port == ~0U) {
        cout << "server port missing" << endl;
        print_usage();
        exit(EXIT_FAILURE);
    }

    server = new GAPeerServer(s_ip, s_port, j_ip, j_port);

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        cout << "   Warning: SIGINT handler registration failed" << endl;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        cout << "   Warning: SIGTERM handler registration failed" << endl;
    }

    /* Start the server. */
    if (pthread_create(&server_tid, NULL, server_function, NULL)) {
        exit_with_error("pthread_create()");
    }

    if (j_port != ~0U) {
        /* Carry out the join procedure with the manager server. */
        server->join();
    }

    ret = work(server);

    /* Wait for the member server to complete - it still holds the memory
     * for 'server', which is ours. */
    if (pthread_join(server_tid, NULL)) {
        exit_with_error("pthread_join()");
    }

    delete server;

    return ret;
}
