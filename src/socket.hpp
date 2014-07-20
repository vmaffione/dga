#ifndef SOCKET_H
#define SOCKET_H

#define MPDEBUG 0

#include <iostream>
#include <cstring>

using namespace std;


enum NodeColor { MPL_RED = 0, MPL_BLACK = 1 };
typedef char MeshByte;

class SccMeshInterface
{
    public:
        int getMyID();
        int numberOfActiveCores() { return 1; }
        void sendIndividuals(MeshByte* msg, size_t totalSize, int ID);
        void receiveIndividuals(MeshByte* msg, size_t totalSize, int ID);
};

#endif
