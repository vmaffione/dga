#include "peer-server.hpp"

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

PeerServer::PeerServer(unsigned int s_port, unsigned int j_port) :
                        Server(s_port), join_port(j_port),
                        id(0), prev(members.end()), succ(members.end())
{
    add_member(Remote("127.0.0.1", s_port));
    me = members.begin();
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

int
PeerServer::join()
{
    Remote remote("127.0.0.1", join_port);
    RemoteConnection connection(remote);
    JoinRequest message("127.0.0.1", me->port);
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

    if (members.size() == 1 && *(members.begin()) == *me) {
        /* I'm the only one left, no LEAVE procedure
         * is necessary. */
        return 0;
    }

    if (members.size()) {
        set<Member>::iterator it = members.begin();

        while (it != members.end() && *it == *me) {
            it++;
        }
        assert(it != members.end());
        leave_port = it->port;
    }

    Remote remote("127.0.0.1", leave_port);
    RemoteConnection connection(remote);
    LeaveRequest request("127.0.0.1", me->port);
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

void
PeerServer::update_social()
{
}
