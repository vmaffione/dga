#include <iostream>
#include <time.h>
#include <stdlib.h>

#include "pga.h"

using namespace std;






/* FloatVector example */
float objfcn( const FloatVector& v )
  {
    return 3*(v[0])*(v[0]) + 5*(v[1])*(v[1]);
  }

///////////////////////



/* float example */
float obj( const float& f )
{return 2.0*f*f - 1.6*f + 2;}

/////////////////////////

/*
void buildPath_2DMesh( int NR, int NC, vector<int>& path )
  {
    int numCores = NR * NC;
    if ( path.size() != numCores )
      path.resize( numCores );
    
    
  }*/

void buildPath_SCC( int NTR, int NTC, vector<int>& path )
  {
    int numRows = NTR*2;
    int numCols = NTC;
    int numCores = numRows * numCols;
    if ( path.size() != numCores )
      path.resize( numCores );
    
    
    // builds SCC rank matrix
    int** scc;
    scc = new int*[numRows];
    for ( int i=0; i<numRows; i++ )
      scc[i] = new int[numCols];
    for ( int i=0; i<numRows; i++ )
      for ( int j=0; j<numCols; j++ )
        scc[i][j] = (i/2)*2*numCols + 2*j + i%2;
      
    for ( int i=0; i<numRows; i++ )
      {
        for ( int j=0; j<numCols; j++ )
          cout << scc[i][j] << " ";
        cout << "\n";
      }
                                  
    if ( NTC % 2 == 0 )
      {
  		// genera il path ad andamento verticale
		int k = 0;
		int x = 0, y = 0;
		while ( y < numRows )  
		  path[k++] = scc[y++][x];
		x++; y--;
		for ( ;; )
		  {
			while ( y > 1 )
			  path[k++] = scc[y--][x];
			if ( x == NTC-1 )
			  {
				while ( x )
				  {
					path[k++] = scc[y--][x];
					path[k++] = scc[y++][x--];
				  }
				break;
			  }
			x++; y++;
			while ( y < numRows )
			  path[k++] = scc[y++][x];
			x++; y--;    
		  }
       }
    else if ( NTR % 2 == 0 )
      {
        int k = 0;
        int xt = 0, yt = 0;
        while ( xt < NTC )
          {
            path[k++] = scc[0][xt];
            path[k++] = scc[1][xt++];
          }
        xt--; yt++;
        for ( ;; )
          {
            while ( xt > 0 )
              {
                path[k++] = scc[2*yt][xt];
                path[k++] = scc[2*yt+1][xt--];
              }
            if ( yt == NTR - 1 )
              {
                yt = 2*NTR - 1;  // change of meaning
                while ( yt > 1 )
                  path[k++] = scc[yt--][0]; 
                break;
              }
            xt++; yt++;
            while ( xt < NTC )
              {
                path[k++] = scc[2*yt][xt];
                path[k++] = scc[2*yt+1][xt++];
              }
            xt--; yt++;
          }
      }
    else
      {
        cout << "ERRORE!!!! Mesh dispari!!\n";
        system("PAUSE");
        exit(0);
      }
      
    
    for ( int i=0; i<numRows; i++ )
      delete [] scc[i];
    delete [] scc;
  }


int main()
{
  try
    { 
      int popsize = 50;
      int numIter = 10000;

      cout << "\n\n FLOATVECTOR EXAMPLE: \n"; 
      vector<float> lbs(2);  // lower bounds
      lbs[0] = -5.0; lbs[1] = -10;
      vector<float> ubs(2);   // upper bounds
      ubs[0] = 6.0; ubs[1] = 6.5;
      int elichi = ( ( (int)(popsize*0.04) ) > 1 ) ? ( (int)(popsize*0.04) ) : 1;  // number of elite children
      
      
        
      int numTilesRows = 4;
      int numTilesCols = 6;
      vector<int> SCCPathAll(2*numTilesRows*numTilesCols);
      
      
      buildPath_SCC( numTilesRows, numTilesCols, SCCPathAll );
      
      for ( int i=0; i<2*numTilesRows*numTilesCols; i++ ) cout << SCCPathAll[i] << " "; cout << "\n";
      
      system( "PAUSE" );
      
      
      //SCCPathAll[0]=1;SCCPathAll[1]=2;SCCPathAll[2]=3;SCCPathAll[3]=4;SCCPathAll[4]=5;SCCPathAll[5]=6;SCCPathAll[6]=7;SCCPathAll[7]=8;
      //SCCPathAll[8]=9;SCCPathAll[9]=10;SCCPathAll[10]=11;SCCPathAll[11]=22;SCCPathAll[22]=23;SCCPathAll[0]=1;

      
      
      GeneticAlgorithm<FloatVector, float> G( &objfcn, GAUtils::MutationGaussian, GAUtils::CrossoverScattered, GAUtils::SUS, SCCPathAll );
      vector<FloatVector> inipop;
      G.gaUtils.generate( popsize, inipop, 2, lbs, ubs );  // generates random initial population
      G.run( inipop, numIter, 0.8, elichi );              // run the algorithm
      
      
      
      cout << "\n\n FLOAT EXAMPLE: \n"; 
      //GeneticAlgorithm<float, float> G2( &obj, &mt, &cr, GAUtils::SUS );
      GeneticAlgorithm<float, float> G2( &obj, GAUtils::MutationGaussian, GAUtils::CrossoverConvex, GAUtils::SUS, SCCPathAll );
      vector<float> finipop;
      G2.gaUtils.generate( popsize, finipop, -10.0, 13.3 );
      G2.run( finipop, numIter, 0.8, elichi );
    }
  catch ( GAError ) {  }
  
  system( "PAUSE" );
  
  return 0;
}
