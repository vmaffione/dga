#include "socket.hpp"
#include <iostream>
#include <vector>

using namespace std;


void SccMeshInterface::receiveIndividuals(MeshByte* msg, size_t totalSize, int ID)
{
    if (MPDEBUG) { cout << " receiving from " << ID << " " << totalSize << " bytes\n"; }
    memset(msg, 0, totalSize);

    /*
       if (MPDEBUG) {
       cout << RCCE_ue() << ": receiving from " << ID << "\n";
       int i=0;
       float x;
       MeshByte* p = msg;
       while (i<totalSize)
       {
       memcpy(&x, p, sizeof(float)); // hardcoded, of course
       cout << "rcv " << x << ", ";
       i += sizeof(float);
       p += sizeof(float);
       }
       cout << "\n"; } */
}
