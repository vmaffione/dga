// 
//   Author: Vincenzo Maffione
//   Date:   3 March 2011
// 
//
#include <string.h>
#include <stdio.h>
#include "RCCE.h"
#include <sched.h>


#define MAXBUFSIZE 4096

const int numCores = 48; // numero di core sui quali si effettua un test

const int numRows = 8;
const int numColumns = 6;
double RTTMatrix[numRows][numColumns]; // la struttura di questa matrice rispecchia la rete di core, vista come rete 8 righe per 6 colonne (invece che 4x6)


int tableProcID[numCores]; // indicizzata con il rank di un core, restituisce il suo processor ID
int xCoord[numCores]; // indicizzata con il rank di un core, restituisce la sua coordinata x nella rete
int yCoord[numCores]; // indicizzata con il rank di un core, restituisce la sua coordinata y nella rete ( y è un numero tra 0 e 7, non tra 0 e 3 )
extern int RCCE_NP; // dichiarato in "RCCE_admin.c", è il numero di core partecipanti
extern int RC_COREID[RCCE_MAXNP]; // dichiarato in "RCCE_admin.c", coincide con la definizione di tableProcID

void buildTables() // cotruisce le tabelle indicizzate al rank
  {
    for ( int i=RCCE_NP-1; i>=0; i-- )
	  {
		tableProcID[i] = RC_COREID[i];  // copia per semplicità, in modo da non utilizzare direttamente l'aria di memoria globale di RCCE
		xCoord[i] = ( tableProcID[i] % 12 ) / 2;
		yCoord[i] = ( tableProcID[i] / 12 ) * 2 + tableProcID[i] % 2;
	  }
  }

  


int RCCE_APP(int argc, char **argv)
{
  int myRank,
      yourRank,
	  nrounds = 10000,
	  round,
	  bufsize = 32,
	  i, j;
  double timer;
  char buffer[MAXBUFSIZE+1];

  RCCE_init( &argc, &argv );
  buildTables(); // deve essere chiamata subito dopo la RCCE_init

  //  RCCE_debug_set(RCCE_DEBUG_ALL);
  myRank = RCCE_ue();

  if ( argc > 1 ) // primo argomento opzionale: dimensione del messaggio
    {
	  bufsize = atoi( *++argv );
	  argc--;
	  if ( bufsize < 1 || bufsize > MAXBUFSIZE )
	    {
		  if ( myRank == 0 ) printf( "Buffer size must be >0 and <=4096; try again\n" );
		  return 1;
		}
	}
  
  if ( argc > 1 ) // secondo argomento opzionale: numero di turni
    {
	  nrounds = atoi( *++argv );
	  argc--;
	  if ( nrounds < 1 ) 
		{
		  if ( myRank == 0 ) printf( "Pingpong needs at least 1 round; try again\n" );
		  return 1;
		}
	}
  
  int king = 1;
  if ( argc > 1 ) // terzo argomento opzionale: settaggio dello scheduler
    {
	  king = atoi( *++argv );
	  argc--;
	  if ( king < 0 || king > 1 ) 
		{
		  if ( myRank == 0 ) printf( "King parameter must be either 0 or 1; try again\n" );
		  return 1;
		}
	}
  
  if ( king )
    {
	  sched_param *psp = (sched_param*) malloc( sizeof( sched_param ) );
	  psp->sched_priority = 99; // maximum priority
	  int sc = sched_setscheduler( 0, SCHED_FIFO, psp );
	  if ( sc )
		printf( "Critical error: unable to set scheduler\n" );
	  else
	    system( "echo -1 > /proc/sys/kernel/sched_rt_runtime_us" ); // disabling real-time throttling
	}
  
  
  if ( RCCE_num_ues() != numCores ) 
	{
      if ( myRank == 0 ) printf( "Pingpong needs exactly two UEs; try again\n" );
      return 1;
	}

  for ( i=1; i<numCores; i++ )
    {
	  // synchronize before starting the timer
	  RCCE_barrier( &RCCE_COMM_WORLD );
	  
	  if ( myRank == 0 )  // sono il master
		{
		  timer = RCCE_wtime();
		  for ( round=0; round < nrounds; round++ ) 
			{
			  RCCE_send( buffer, bufsize, i );
			  RCCE_recv( buffer, bufsize, i );
			}
		  timer = RCCE_wtime() - timer;
		  RTTMatrix[ yCoord[yourRank] ][ xCoord[yourRank] ] = ( timer * 1000000 ) / nrounds; // tempo espresso in microsecondi
		}	
	  else if ( myRank == i ) // sono lo slave giusto
		{
		  for ( round=0; round < nrounds; round++ )
			{
			  RCCE_recv( buffer, bufsize, 0 );
			  RCCE_send( buffer, bufsize, 0 );
			}
		}
	  
	  //if ( myRank == 0 ) // è giusto che sia primo core (master) a stampare il tempo
		//printf( "MSGSZ = %d bytes, NROUNDS = %d, AVGRTT = %1.9lf\n", bufsize, nrounds, timer/nrounds );
	}

  if ( myRank == 0 )
    {
	  for ( i=0; i<numRows; i++ )
	    {
	      for ( j=0; j<numColumns; j++ )
		    printf( "%1.9lf ", RTTMatrix[i][j] );
		}
	  printf( "\n" );
	}
	
  RCCE_finalize();

  return 0;
}

