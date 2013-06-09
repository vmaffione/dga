#ifndef MESH_2D_H
#define MESH_2D_H

#define MPDEBUG 0

#include <iostream>
#include <cstring>

using namespace std;

enum NodeColor { MPL_RED = 0, MPL_BLACK = 1 };
typedef char MeshByte;


class SccMeshInterface
  {
      static const int SccNumRows = 8;
      static const int SccNumCols = 6;
	  
	  int numCores;  // number of cores which take part into the genetic algorithm
	  int* path;  // closed path of SCC cores
	  
	  void verticalOptimalPath( int** scc, int yFirst, int yLast, int xFirst, int xLast, bool tilesPath );
	  void horizontalOptimalTilesPath( int** scc, int ytFirst, int ytLast, int xtFirst, int xtLast );
	  void horizontalOptimalPath( int** scc, int yFirst, int yLast, int xFirst, int xLast );
	  void oddPath( int** scc, int yFirst, int yLast, int xFirst, int xLast );
	  
    public:
	  static void MessagePassingLibraryInit( int* argcp, char*** argvp );
	  static void MessagePassingLibraryFinalize();
	  
	  SccMeshInterface();
	  void getMyMeshConfiguration( int& prev, int& succ, NodeColor& color ); // if there is only one core available, this function assign -1 to the arguments
	  int getMyID();
	  int numberOfActiveCores() { return numCores; }
	  void sendIndividuals( MeshByte* msg, size_t totalSize, int ID );
	  void receiveIndividuals( MeshByte* msg, size_t totalSize, int ID );
	  ~SccMeshInterface() { delete [] path; }
  };
 
#endif
