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

using namespace std;


class Manager
{
        NodeColor next_color;
        unsigned int next_id;

    public:
        list<Member> members;

    public:
        Manager();
        int add_member(const Remote& remote);
        int del_member(const Remote& remote);
        void update_new_member(unsigned int new_id);
        void print_members();
};

Manager::Manager() : next_color(MPL_BLACK), next_id(1)
{
}

void
Manager::print_members()
{
    cout << "Members list" << endl;
    for (list<Member>::iterator it = members.begin();
            it != members.end(); it++) {
        cout << "    " << it->ip << ":" << it->port << endl;
    }
}

int
Manager::add_member(const Remote& remote)
{
    Member member(remote);

    member.color = next_color;
    member.id = next_id;

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (member == *it) {
            return -1;
        }
    }
    /*
       for (list<Member>::iterator it = members.begin();
       it != members.end(); it++) {
       RemoteConnection connection(*it);

       connection.close();
       }
       */
    members.push_back(member);

    print_members();

    if (next_color == MPL_BLACK) {
        next_color = MPL_RED;
    } else {
        next_color = MPL_BLACK;
    }

    return next_id++;
}

int
Manager::del_member(const Remote& remote)
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
Manager::update_new_member(unsigned int new_id)
{
    UpdateRequest request;
    list<Member>::iterator nit;

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (it->id != new_id) {
            request.add(it->ip, it->port);
        } else {
            nit = it;
        }
    }

    RemoteConnection connection(*nit);

    request.serialize(connection);
    connection.close();
}

class ManagerServer : public Server {
        Manager manager;

    public:
        ManagerServer(short unsigned port) : Server(port) { }
        virtual int process_request(RemoteConnection& connection);
};

int ManagerServer::process_request(RemoteConnection& connection)
{
#define BUFSIZE 128
    uint8_t opcode;

    cout << "Request received from : " << connection.remote.ip <<
        ":" << connection.remote.port << "\n";
    connection.deserialize(opcode);
    if (opcode == JOIN) {
        string content = "OK";
        JoinRequest request;
        int ret;

        /* Process the join request. */
        request.deserialize(connection);
        cout << "JOIN-REQUEST(" << request.ip << "," << request.port
                << ")" << endl;

        Remote remote(request.ip, request.port);

        ret = manager.add_member(remote);
        if (ret < 0) {
            content = "Already joined";
        }
        Response(content).serialize(connection);

        /* Close the current connection, since update_new_member() will
         * open another one towards the same member. */
        connection.close();

        manager.update_new_member(ret);

    } else if (opcode == LEAVE) {
        string content = "OK";
        LeaveRequest request;
        int ret;

        request.deserialize(connection);
        cout << "LEAVE-REQUEST(" << request.ip << "," << request.port
                << ")" << endl;
        ret = manager.del_member(Remote(request.ip, request.port));
        if (ret) {
            content = "No previous join";
        }
        Response(content).serialize(connection);
    }

    return 0;
}

int main()
{
    ManagerServer server(MANAGER_PORT);

    server.run();

    return 0;
}
