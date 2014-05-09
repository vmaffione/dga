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
    static void MessagePassingLibraryInit( int* argcp, char*** argvp );
    static void MessagePassingLibraryFinalize();

    SccMeshInterface();
    void getMyMeshConfiguration( int& prev, int& succ, NodeColor& color ); // if there is only one core available, this function assign -1 to the arguments
    int getMyID();
    int numberOfActiveCores() { return 1; }
    void sendIndividuals( MeshByte* msg, size_t totalSize, int ID );
    void receiveIndividuals( MeshByte* msg, size_t totalSize, int ID );
    ~SccMeshInterface() { }
};

#endif
