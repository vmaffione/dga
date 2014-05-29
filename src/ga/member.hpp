#ifndef __MEMBER_HPP__
#define __MEMBER_HPP__

#include "remote.hpp"


#define MANAGER_PORT    9863

class Member : public Remote {
    public:
        unsigned id;
        enum NodeColor color;

        Member() : Remote(), color(MPL_BLACK), id(0) { }
        Member(const Remote& r) : Remote(r), color(MPL_BLACK), id(0) { }
};

#endif  /* __MEMBER_HPP__ */
