#include "common.hpp"
#include "remote.hpp"

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


class Member : public Remote {
    public:
        unsigned id;
        enum NodeColor color;

        Member();
        Member(const Remote& r);
};

Member::Member() : Remote(), color(MPL_BLACK), id(0)
{
}

Member::Member(const Remote& r) : Remote(r), color(MPL_BLACK), id(0)
{
}

class Manager
{
        list<Member> members;
        NodeColor next_color;
        unsigned next_id;

    public:
        Manager();
        void add_member(const Remote& remote);
};

Manager::Manager() : next_color(MPL_BLACK), next_id(1)
{
}

void Manager::add_member(const Remote& remote)
{
        Member member(remote);

        member.color = next_color;
        member.id = next_id;

        members.push_back(member);

        if (next_color == MPL_BLACK) {
                next_color = MPL_RED;
        } else {
                next_color = MPL_BLACK;
        }

        next_id++;
}

class ManagerServer : public Server {
        Manager manager;

    public:
        ManagerServer(short unsigned port) : Server(port) { }
        virtual int process_request(const RemoteConnection& connection);
};

int ManagerServer::process_request(const RemoteConnection& connection)
{
#define BUFSIZE 128
        char buffer[BUFSIZE];
        int n;

        cout << "Request received: " <<
                connection.remote.ip << ":"
                << connection.remote.port << "\n";

        n = connection.recv_message(buffer, sizeof(buffer));

        manager.add_member(connection.remote);

        n = connection.send_message(buffer, n);

        return 0;
}

int main()
{
        ManagerServer server(9863);

        server.run();

        return 0;
}
