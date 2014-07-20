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

    protected:
        set<Member>::iterator me;
        set<Member> members;

        unsigned int id;
        set<Member>::iterator prev;
        set<Member>::iterator succ;

        void update_social();

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
