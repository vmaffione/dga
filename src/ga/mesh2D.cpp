#include "mesh2D.h"
#include <iostream>
#include <vector>

using namespace std;

#include "RCCE.h" // only this compilation unit knows what SCC really is

extern int RCCE_NP; // dichiarato in "RCCE_admin.c", è il numero di core partecipanti
extern int RC_COREID[RCCE_MAXNP]; // dichiarato in "RCCE_admin.c", coincide con la definizione di tableProcID

void SccMeshInterface::MessagePassingLibraryInit( int* argcp, char*** argvp )
{
    RCCE_init( argcp, argvp );
}

void SccMeshInterface::MessagePassingLibraryFinalize()
{
    cout << "MACHETE DISTRUGGE!\n";
    RCCE_finalize();
}

SccMeshInterface::SccMeshInterface() : numCores( RCCE_NP )
{ 
    int** meshRank = new int*[SccNumRows]; 
    for ( int i=0; i<SccNumRows; i++ )
        meshRank[i] = new int[SccNumCols];

    for ( int i=0; i<SccNumRows; i++ )
        for ( int j=0; j<SccNumCols; j++ )
            meshRank[i][j] = -1;

    for ( int i=numCores-1; i>=0; i-- )
    {
        //tableProcID[i] = RC_COREID[i];  // copia per semplicità, in modo da non utilizzare direttamente l'area di memoria globale di RCCE
        //xCoord[i] = ( tableProcID[i] % 12 ) / 2;
        //yCoord[i] = ( tableProcID[i] / 12 ) * 2 + tableProcID[i] % 2;
        meshRank[ ( RC_COREID[i] % 12 ) / 2 ][ ( RC_COREID[i] / 12 ) * 2 + RC_COREID[i] % 2 ] = i;  // MI SA CHE E' L'UNICA COSA UTILE
    }

    // looks for a unique contiguous rectangular subset made of all the available SCC cores
    int iFirst = 0, jFirst;
    bool firstFound = false;
    for ( ; iFirst<SccNumRows; iFirst++ )
    {
        for ( jFirst=0; jFirst<SccNumCols; jFirst++ )
            if ( meshRank[iFirst][jFirst] >= 0 )
            {
                firstFound = true;
                break;
            }
        if ( firstFound )
            break;
    }
    if ( !firstFound )
    {
        cout << "ERRORE: Da qualche parte c'e' un bug, perche' e' impossibile che non ci siano core assegnati a questa computazione!!\n";
        return;
    }
    int jLast = jFirst+1;
    while ( jLast < SccNumCols && meshRank[iFirst][jLast] >= 0 )
        jLast++;
    jLast--;
    int iLast = iFirst+1;
    while ( iLast < SccNumRows && meshRank[iLast][jFirst] >= 0 )
        iLast++;
    iLast--;
    bool contiguousRectangle = true;
    for ( int i=iFirst; i<=iLast; i++ )
        for ( int j=jFirst; j<=jLast; j++ )
            if ( meshRank[i][j] == -1 )
            {
                contiguousRectangle = false;
                i = iLast + 1;
                break;
            }

    path = new int[numCores]; // numCores !!!!!!!!!!!!!!!!!!!!!!!!

    if ( !contiguousRectangle || (iLast-iFirst+1)*(jLast-jFirst+1) < numCores )  // worst case: no contiguity
    {
        cout << "Urge strategia alternativa!!\n";
    }
    else 
    {
        /*
           for ( int i=0; i<SccNumRows; i++ )
           for ( int j=0; j<SccNumCols; j++ )
           meshRank[i][j] = i*SccNumRows+j;
           iFirst = 0;
           iLast = 2;
           jFirst = 0;
           jLast = 4;
           for ( int i=0; i<SccNumRows; i++ )
           {
           for ( int j=0; j<SccNumCols; j++ )
           cout << meshRank[i][j] << " ";
           cout << "\n";
           } cout << "\n"; 
           */ 

        int heigth = iLast - iFirst + 1,
            width = jLast - jFirst + 1;

        if ( width == 1 )  // a vertical row of cores
            for ( int i=iFirst; i<=iLast; i++ )
                path[i-iFirst] = meshRank[i][jFirst];
        else if ( heigth == 1 )  // an horizontal row of cores
            for ( int j=jFirst; j<=jLast; j++ )
                path[j-jFirst] = meshRank[iFirst][j];
        else if ( iFirst % 2 == 0 && heigth >= 4 && heigth % 2 == 0 && width % 2 == 0 )  // best case I: vertical tiles path
            verticalOptimalPath( meshRank, iFirst, iLast, jFirst, jLast, true );
        else if ( iFirst % 2 == 0 && heigth % 4 == 0 && width >= 2 )  //  best cast II: horizontal tiles path
            horizontalOptimalTilesPath( meshRank, iFirst/2, iLast/2, jFirst, jLast );
        else if ( width % 2 == 0 && heigth >= 2 )  // good case I: vertical path
            verticalOptimalPath( meshRank, iFirst, iLast, jFirst, jLast, false );
        else if ( heigth % 2 == 0 && width >= 2 )  // good case II: horizontal path
            horizontalOptimalPath( meshRank, iFirst, iLast, jFirst, jLast );
        else // not so bad: an odd number of cores in a rectangular contiguous set
            oddPath( meshRank, iFirst, iLast, jFirst, jLast );
    }


    if ( MPDEBUG )
    {
        cout << "Tables:\n";
        //for ( int i=0; i<numCores; i++ ) cout << tableProcID[i] << " " << xCoord[i] << " " << yCoord[i] << "\n"; cout << "\n\n";
        for ( int i=0; i<SccNumRows; i++ )
        {
            for ( int j=0; j<SccNumCols; j++ )
                cout << meshRank[i][j] << " ";
            cout << "\n";
        } cout << "\n";
        cout << "(" << jFirst << "," << iFirst << ") - (" << jLast << "," << iLast << ")\n";
        cout << "Path: "; for ( int i=0; i<numCores; i++ ) cout << path[i] << " "; cout << "\n";
    }

    delete [] meshRank;
}



void SccMeshInterface::verticalOptimalPath( int** scc, int yFirst, int yLast, int xFirst, int xLast, bool tilesPath )
{ 
    if ( MPDEBUG ) { if ( tilesPath ) cout << "Vertical tiles\n"; else cout << "Vertical\n"; }
    int offset = ( tilesPath ) ? 2 : 1;
    int k = 0;
    int x = xFirst, y = yFirst;
    path[k++] = scc[y++][x];
    while ( x <= xLast )
    {
        while ( y <= yLast )
            path[k++] = scc[y++][x];
        x++; y--;
        while ( y >= yFirst + offset )
            path[k++] = scc[y--][x];
        x++; y++;
    }
    x--; y--;
    if ( tilesPath )
        while ( x > xFirst )
        {
            path[k++] = scc[y--][x];
            path[k++] = scc[y++][x--];
        }
    else
        while ( x > xFirst )
            path[k++] = scc[y][x--];
}

void SccMeshInterface::horizontalOptimalTilesPath( int** scc, int ytFirst, int ytLast, int xtFirst, int xtLast )
{ 
    if ( MPDEBUG ) { cout << "Horizontal tiles\n"; }
    int k = 0;
    int xt = xtFirst, yt = ytFirst;
    path[k++] = scc[2*yt][xt];
    path[k++] = scc[2*yt+1][xt++];
    while ( yt <= ytLast )
    {
        while ( xt <= xtLast )
        {
            path[k++] = scc[2*yt][xt];
            path[k++] = scc[2*yt+1][xt++];
        }
        yt++; xt--;
        while ( xt > xtFirst )
        {
            path[k++] = scc[2*yt][xt];
            path[k++] = scc[2*yt+1][xt--];
        }
        yt++; xt++;
    }
    yt--;
    int y = 2*ytLast + 1;
    while ( y >= 2*ytFirst + 2 )
        path[k++] = scc[y--][xtFirst]; 
}

void SccMeshInterface::horizontalOptimalPath( int** scc, int yFirst, int yLast, int xFirst, int xLast )
{
    if ( MPDEBUG ) { cout << "Horizontal\n"; }
    int k = 0;
    int x = xFirst, y = yFirst;
    path[k++] = scc[y][x++];
    while ( y <= yLast )
    {
        while ( x <= xLast )
            path[k++] = scc[y][x++];
        y++; x--;
        while ( x > xFirst )
            path[k++] = scc[y][x--];
        y++; x++;
    }
    y--;
    while ( y > yFirst )
        path[k++] = scc[y--][xFirst]; 
}

void SccMeshInterface::oddPath( int** scc, int yFirst, int yLast, int xFirst, int xLast )
{
    if ( MPDEBUG ) { cout << "Odd number of cores\n"; }
    int k = 0;
    int x = xFirst, y = yFirst;
    while ( y <= yLast )
        path[k++] = scc[y++][x];
    x++; y--;
    while ( x < xLast - 1 )
    {
        while ( y >= yFirst + 1 )
            path[k++] = scc[y--][x];
        x++; y++;
        while ( y <= yLast )
            path[k++] = scc[y++][x];
        x++; y--;
    }
    while ( y > yFirst )
    {
        path[k++] = scc[y][x++];
        path[k++] = scc[y--][x];
        path[k++] = scc[y][x--];
        path[k++] = scc[y--][x];
    }
    x++;
    while ( x > xFirst )
        path[k++] = scc[y][x--];
}

void SccMeshInterface::getMyMeshConfiguration( int& prev, int& succ, NodeColor& color )
{ 
    if ( numCores == 1 )
    {
        prev = succ = -1;
        return;
    }
    if ( MPDEBUG ) { for ( int i=0; i<numCores; i++ ) cout << path[i] << " "; cout << "\n"; }
    int myRank = RCCE_ue();
    int k = 0;
    while ( path[k] != myRank )
        k++;
    prev = path[ ( ( k ) ? ( k - 1 ) : ( numCores - 1 ) ) ];
    succ = path[ ( k + 1 ) % numCores ];
    if ( k % 2 )
        color = MPL_BLACK;
    else
        color = MPL_RED;
}

int SccMeshInterface::getMyID()
{
    return RCCE_ue();
}

void SccMeshInterface::sendIndividuals( MeshByte* msg, size_t totalSize, int ID )
{
    /*
       if ( MPDEBUG ) {
       cout << RCCE_ue() << ": sent to " << ID << "\n";
       int i=0;
       float x;
       MeshByte* p = msg;
       while ( i<totalSize )
       {
       memcpy( &x, p, sizeof( float ) );
       cout << "snd " << x << ", ";
       i += sizeof( float ); // hardcoded of course
       p += sizeof( float );
       }
       cout << "\n"; } */

    if ( MPDEBUG ) { cout << RCCE_ue() << ": sending to " << ID << " " << totalSize << " bytes\n"; }

    RCCE_send( msg, totalSize, ID );
}

void SccMeshInterface::receiveIndividuals( MeshByte* msg, size_t totalSize, int ID )
{
    if ( MPDEBUG ) { cout << RCCE_ue() << ": receiving from " << ID << " " << totalSize << " bytes\n"; }
    RCCE_recv( msg, totalSize, ID );


    /*
       if ( MPDEBUG ) {
       cout << RCCE_ue() << ": receiving from " << ID << "\n";
       int i=0;
       float x;
       MeshByte* p = msg;
       while ( i<totalSize )
       {
       memcpy( &x, p, sizeof( float ) ); // hardcoded, of course
       cout << "rcv " << x << ", ";
       i += sizeof( float ); 
       p += sizeof( float );
       }
       cout << "\n"; } */
}
