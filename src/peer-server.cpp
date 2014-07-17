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
#include <vector>
#include <set>
#include <signal.h>
#include <cerrno>
#include <ctime>
#include <cassert>

using namespace std;


class PeerServer : public Server {
        unsigned int join_port;
        Member me;

        set<Member> members;

    public:
        PeerServer(unsigned int s_port, unsigned j_port);

        virtual int process_request(RemoteConnection& connection);

        set<Member>::iterator add_member(const Member& remote);
        int del_member(const Remote& remote);
        void sync_new_member(set<Member>::iterator nit);
        void notify_old_members_add(set<Member>::iterator nit);
        void notify_old_members_del(const Member& remote);
        int join();
        int leave();
        void print_members();
};

PeerServer::PeerServer(unsigned int s_port, unsigned int j_port) :
                        Server(s_port), join_port(j_port),
                        me(Remote("127.0.0.1", s_port))
{
    add_member(me);
}

void
PeerServer::print_members()
{
    cout << "Members list" << endl;
    for (set<Member>::iterator it = members.begin();
            it != members.end(); it++) {
        cout << "    " << it->ip << ":" << it->port << endl;
    }
}

set<Member>::iterator
PeerServer::add_member(const Member& member)
{
    pair< set<Member>::iterator, bool> ret;

    ret = members.insert(member);
    if (ret.second) {
        print_members();
    }

    return ret.first;
}

int
PeerServer::del_member(const Remote& remote)
{
    Member member(remote);
    size_t erased;

    erased = members.erase(member);
    if (erased) {
        print_members();
        return 0;
    }

    return -1;
}

void
PeerServer::sync_new_member(set<Member>::iterator nit)
{
    UpdateRequest request;

    for (set<Member>::iterator it = members.begin();
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
PeerServer::notify_old_members_add(set<Member>::iterator nit)
{
    if (nit == members.end()) {
        cerr << __func__ << ": Internal error" << endl;
        exit(EXIT_FAILURE);
    }

    for (set<Member>::iterator it = members.begin();
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
PeerServer::notify_old_members_del(const Member& member)
{
    for (set<Member>::iterator it = members.begin();
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
PeerServer::process_request(RemoteConnection& connection)
{
    uint8_t opcode;

    cout << "Request received from : " << connection.remote.ip <<
        ":" << connection.remote.port << endl;
    connection.deserialize(opcode);

    if (opcode == JOIN) {
        string content = "OK";
        JoinRequest request;
        set<Member>::iterator ret;

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

static PeerServer *server = NULL;

int
PeerServer::join()
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

    return 0;
}

int
PeerServer::leave()
{
    unsigned int leave_port = join_port;

    if (members.size() == 1 && *(members.begin()) == me) {
        /* I'm the only one left, no LEAVE procedure
         * is necessary. */
        return 0;
    }

    if (members.size()) {
        set<Member>::iterator it = members.begin();

        while (it != members.end() && *it == me) {
            it++;
        }
        assert(it != members.end());
        leave_port = it->port;
    }

    Remote remote("127.0.0.1", leave_port);
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

    server = new PeerServer(s_port, j_port);

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
