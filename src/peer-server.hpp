#ifndef __PEER_SERVER_H__
#define __PEER_SERVER_H__

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


class PeerServer : public Server {
        Remote join_remote;

    protected:
        std::set<Member>::iterator me;
        std::set<Member> members;

        unsigned int id;
        std::set<Member>::iterator prev;
        std::set<Member>::iterator succ;

        void update_social();

    public:
        PeerServer(const std::string& s_ip, unsigned int s_port,
                   const std::string& j_ip, unsigned int j_port);

        virtual int process_request(RemoteConnection& connection);

        std::set<Member>::iterator add_member(const Member& remote);
        int del_member(const Remote& remote);
        void sync_new_member(std::set<Member>::iterator nit);
        void notify_old_members_add(std::set<Member>::iterator nit);
        void notify_old_members_del(const Member& remote);
        int join();
        int leave();
        void print_members();

        unsigned int get_unique() const { return id; }
        Member get_me() const { return *me; }
        Member get_prev() const;
        Member get_succ() const;
        unsigned int num_peers() const { return members.size(); }

        /* Currently unused. */
        bool master() const { return me == members.begin(); }
        Member get_master() const { return *(members.begin()); }

        virtual int process_message(uint8_t opcode,
                                    RemoteConnection& connection) = 0;
};

#endif  /* __PEER_SERVER_H__ */
