#include "socket.hpp"
#include <iostream>
#include <vector>

using namespace std;

void SccMeshInterface::MessagePassingLibraryInit(int* argcp, char*** argvp)
{
}

void SccMeshInterface::MessagePassingLibraryFinalize()
{
    cout << "MACHETE DISTRUGGE!\n";
}

SccMeshInterface::SccMeshInterface()
{
}

void SccMeshInterface::getMyMeshConfiguration(int& prev, int& succ, NodeColor& color)
{
    prev = succ = -1;
}

int SccMeshInterface::getMyID()
{
    return 0;
}

void SccMeshInterface::sendIndividuals(MeshByte* msg, size_t totalSize, int ID)
{
/*
       if (MPDEBUG) {
       cout << RCCE_ue() << ": sent to " << ID << "\n";
       int i=0;
       float x;
       MeshByte* p = msg;
       while (i<totalSize)
       {
       memcpy(&x, p, sizeof(float));
       cout << "snd " << x << ", ";
       i += sizeof(float); // hardcoded of course
       p += sizeof(float);
       }
       cout << "\n"; }
*/

    if (MPDEBUG) { cout << getMyID() << ": sending to " << ID << " " << totalSize << " bytes\n"; }
}

void SccMeshInterface::receiveIndividuals(MeshByte* msg, size_t totalSize, int ID)
{
    if (MPDEBUG) { cout << getMyID() << ": receiving from " << ID << " " << totalSize << " bytes\n"; }
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
