#include "remote.hpp"
#include "protocol.hpp"

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>
#include <list>
#include <signal.h>
#include <cerrno>
#include <ctime>

using namespace std;


class ManagerServer : public Server {
        bool joined;
        unsigned int join_port;
        Member me;

        list<Member> members;

    public:
        ManagerServer(unsigned int s_port, unsigned j_port);

        virtual int process_request(RemoteConnection& connection);

        list<Member>::iterator add_member(const Member& remote);
        int del_member(const Remote& remote);
        void sync_new_member(list<Member>::iterator nit);
        void notify_old_members_add(list<Member>::iterator nit);
        void notify_old_members_del(const Member& remote);
        int join();
        int leave();
        void print_members();
};

ManagerServer::ManagerServer(unsigned int s_port, unsigned int j_port) :
                        Server(s_port), joined(false), join_port(j_port),
                        me(Remote("127.0.0.1", s_port))
{
    add_member(me);
}

void
ManagerServer::print_members()
{
    cout << "Members list" << endl;
    for (list<Member>::iterator it = members.begin();
            it != members.end(); it++) {
        cout << "    " << it->ip << ":" << it->port << endl;
    }
}

list<Member>::iterator
ManagerServer::add_member(const Member& member)
{
    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (member == *it) {
            return it;
        }
    }
    members.push_back(member);
    print_members();

    return --members.end();
}

int
ManagerServer::del_member(const Remote& remote)
{
    Member member(remote);

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (member == *it) {
            members.erase(it);
            print_members();

            return 0;
        }
    }

    return -1;
}

void
ManagerServer::sync_new_member(list<Member>::iterator nit)
{
    UpdateRequest request;

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (it != nit) {
            request.members.push_back(*it);
        }
    }

    if (nit == members.end()) {
        cerr << __func__ << ": Internal error" << endl;
        exit(EXIT_FAILURE);
    }

    RemoteConnection connection(*nit);

    if (!connection.open) {
        cerr << __func__ << ": connection failed" << endl;
        return;
    }

    request.serialize(connection);
    connection.close();
}

void
ManagerServer::notify_old_members_add(list<Member>::iterator nit)
{
    if (nit == members.end()) {
        cerr << __func__ << ": Internal error" << endl;
        exit(EXIT_FAILURE);
    }

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (it != nit) {
            UpdateRequest request;
            RemoteConnection connection(*it);

            if (!connection.open) {
                cerr << __func__ << ": connection failed" << endl;
                continue;
            }

            request.members.push_back(*nit);
            request.serialize(connection);
            connection.close();
        }
    }
}

void
ManagerServer::notify_old_members_del(const Member& member)
{
    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        UpdateRequest request;
        RemoteConnection connection(*it);

        if (!connection.open) {
            cerr << __func__ << ": connection failed" << endl;
            continue;
        }

        request.add = false;
        request.members.push_back(member);
        request.serialize(connection);
        connection.close();
    }
}

int
ManagerServer::process_request(RemoteConnection& connection)
{
    uint8_t opcode;

    cout << "Request received from : " << connection.remote.ip <<
        ":" << connection.remote.port << endl;
    connection.deserialize(opcode);

    if (opcode == JOIN) {
        string content = "OK";
        JoinRequest request;
        list<Member>::iterator ret;

        /* Process the join request. */
        request.deserialize(connection);
        cout << "JOIN-REQUEST(" << request.ip << "," << request.port
                << ")" << endl;

        Member member(Remote(request.ip, request.port));

        ret = add_member(member);
        if (ret == members.end()) {
            content = "Already joined";
        }

        Response(content).serialize(connection);

        sync_new_member(ret);
        notify_old_members_add(ret);

    } else if (opcode == LEAVE) {
        string content = "OK";
        LeaveRequest request;
        int ret;

        request.deserialize(connection);
        cout << "LEAVE-REQUEST(" << request.ip << "," << request.port
                << ")" << endl;

        Member member(Remote(request.ip, request.port));

        ret = del_member(member);
        if (ret) {
            content = "No previous join";
        }
        Response(content).serialize(connection);

        notify_old_members_del(member);

    } else if (opcode == UPDATE) {
        UpdateRequest request;

        request.deserialize(connection);
        cout << "UPDATE-REQUEST-" << request.add << "-(" <<
                request.members.size() << ")" << endl;
        for (unsigned int i = 0; i < request.members.size(); i++) {
            const Member& m = request.members[i];
            cout << "   Member " << m.ip << " " << m.port << endl;
            if (request.add) {
                add_member(m);
            } else {
                del_member(m);
            }
        }
    }

    return 0;
}

static ManagerServer *server = NULL;

int
ManagerServer::join()
{
    Remote remote("127.0.0.1", join_port);
    RemoteConnection connection(remote);
    JoinRequest message("127.0.0.1", me.port);
    Response response;

    if (!connection.open) {
        cerr << __func__ << ": connection failed" << endl;
        return -1;
    }

    message.serialize(connection);

    response.deserialize(connection);

    connection.close();

    if (response.content != "OK") {
        cout << "Join error: " << response.content << endl;
        exit(EXIT_FAILURE);
    }

    joined = true;

    return 0;
}

int
ManagerServer::leave()
{
    if (!joined) {
        return 0;
    }

    Remote remote("127.0.0.1", join_port);
    RemoteConnection connection(remote);
    LeaveRequest request("127.0.0.1", me.port);
    Response response;

    if (!connection.open) {
        cerr << __func__ << ": connection failed" << endl;
        return -1;
    }

    request.serialize(connection);
    response.deserialize(connection);
    connection.close();

    if (response.content != "OK") {
        cout << "Leave error: " << response.content << endl;
        exit(EXIT_FAILURE);
    }

    joined = false;

    return 0;
}

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

int
main(int argc, char **argv)
{
    unsigned int s_port;
    unsigned int j_port = ~0U;
    struct sigaction sa;
    pthread_t server_tid;

    if (argc < 2) {
        exit_with_error("USAGE: program PORT [JOINPORT]");
    }
    s_port = atoi(argv[1]);
    if (s_port >= 65535) {
        errno = EINVAL;
        exit_with_error("PORT > 65535");
    }

    if (argc > 2) {
        j_port = atoi(argv[2]);
        if (j_port >= 65535) {
            errno = EINVAL;
            exit_with_error("PORT > 65535");
        }
    }

    server = new ManagerServer(s_port, j_port);

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

    /* Wait for the member server to complete - it still holds the memory
     * for 'server', which is ours. */
    if (pthread_join(server_tid, NULL)) {
        exit_with_error("pthread_join()");
    }

    delete server;

    return 0;
}
