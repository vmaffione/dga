#ifndef SOCKET_H
#define SOCKET_H

#define MPDEBUG 0

#include <iostream>
#include <cstring>
#include <stdint.h>

using namespace std;


enum NodeColor { MPL_RED = 0, MPL_BLACK = 1 };

class SccMeshInterface
{
    public:
        void receiveIndividuals(uint8_t* msg, size_t totalSize, int ID);
};

#endif
