#ifndef GAUTILS_
#define GAUTILS_

typedef vector<float> FloatVector;
typedef vector<double> DoubleVector;

ostream& operator<<( ostream& os, const FloatVector& r )
  {
    std::cout << "(";
    for ( int d=0 ;d<r.size()-1; d++ )
      std::cout << r[d] << ", ";
    std::cout << r[r.size()-1] << ")";
  }

ostream& operator<<( ostream& os, const DoubleVector& r )
  {
    std::cout << "(";
    for ( int d=0 ;d<r.size()-1; d++ )
      std::cout << r[d] << ", ";
    std::cout << r[r.size()-1] << ")";
  }

  
#include "mesh2D.h"

//  Agner Fog's random number generator librares
#include "randoma.h"  // uniform generator
#define rand_init(seed)  SFMTgenRandomInit(seed,0)
#define rand_float()  SFMTgenRandom()                        // random float number in [0,1[
#define rand_int( l, b )  SFMTgenIRandom( ( l ), ( b ) )     // random integer number in [l,b]  (inexact)
#include "stocc.h"  // non-uniform generator

/*
  // obsolete random number generator
  #define rand_init() srand(time(0))
  #define rand_float()  (rand()*(1.0/(RAND_MAX + 1.0)))  // random float number in [0,1]
  #define rand_int( l, n )  ( (int) ( ( rand() / ( RAND_MAX + 1.0 ) ) * ( ( n ) + 1 ) ) )  // random integer number in [0,n]
*/

template <class IT, class OT> class GeneticAlgorithm;  // not necessary

class GAUtils
  {
      //template <class IT, class OT> friend class GeneticAlgorithm;  // ORA SERVE PER FARE STAMPE DI DEBUG, MA ANDREBBE CANCELLATO
    public:
      enum SelectionFunctionType { SUS = 0, RWS };
      enum CrossoverFunctionType { CrossoverConvex = 0, CrossoverScattered };
      enum MutationFunctionType { MutationGaussian = 0, MUT_OTHER };

    private:
      double* mgScaleV;     // scale vector for mutationGaussian
      float shrink;        // shrink factor for mutationGaussian

	  static const int GEN_BASE_SEED = 6536541;
	  static const int GEN_MUL_SEED = 83;
	  static const int NURG_SEED = 613788543;
      StochasticLib1 NURG;  // non-uniform random generator

    public:
      GAUtils(): mgScaleV( NULL ), shrink( 1.0 ), D( 0 ), NURG( NURG_SEED ) { }   // DA FARE: NURG dovrebbe essere inizializzato in modo diverso su core diversi...
      
	  int D;  // DA RENDERE PRIVATO!!! ORA E' PUBBLICO PERCHè LO UTILIZZANO LE STAMPE DI DEBUG IN GA
	  
      template <class T> void mutationGaussian( const T& t, T& res );
      template <class T> void crossoverConvex( const T& t1, const T& t2, T& res );
      template <class T> void crossoverScattered( const T& t1, const T& t2, T& res );
	  
	  void refreshMutationGaussianParameters( int numGen, int maxGen );
	  
	  template <class T> void serializeAndCopy( MeshByte* buffer, T** pointersBuffer, int num );
	  template <class T> void deserializeAndCopy( MeshByte* buffer, T** pointersBuffer, int num );
      
      void generate( int N, vector<float>& v, float lb, float ub );
      void generate( int popSize, vector<double>& v, double lb, double ub );
      void generate( int N, vector<FloatVector>& v, const vector<float>& lbs, const vector<float>& ubs );
      void generate( int N, vector<DoubleVector>& v, const vector<double>& lbs, const vector<double>& ubs );
      
	  void setMeshInterfacePointer( SccMeshInterface* mip ) { meshInterfacePointer = mip; }
	  SccMeshInterface* meshInterfacePointer;
	  
	  static bool is_MPL_up;
	  static void frameworkInit( int* argcp, char*** argvp );
	  
      /*
      class RandomGenerator
        {
          public:
            void generate( int N, vector<float>& v, float lb, float ub );
            void generate( int popSize, vector<double>& v, double lb, double ub );
            void generate( int N, vector<FloatVector>& v, int dim, const vector<float>& lbs, const vector<float>& ubs );
            void generate( int N, vector<DoubleVector>& v, int dim, const vector<double>& lbs, const vector<double>& ubs );
        };
      RandomGenerator randomGenerator;*/
  };

bool GAUtils::is_MPL_up = false;  
  
void GAUtils::frameworkInit( int* argcp, char*** argvp )
  { 
    if ( !GAUtils::is_MPL_up )	
	  {
		SccMeshInterface::MessagePassingLibraryInit( argcp, argvp );
		GAUtils::is_MPL_up = true;
	  }
}


// SERIALIZATION  
template <class T>
void GAUtils::serializeAndCopy( MeshByte* buffer, T** pointersBuffer, int num )
  {
    GAError( "Error: You must provide a serialization function for this input data type" );
  }

template <>
void GAUtils::serializeAndCopy<float>( MeshByte* buffer, float** pointersBuffer, int num )
  {
    float* floatBuffer = reinterpret_cast< float* >( buffer );
	for ( int i=0; i<num; i++, floatBuffer++ )
	  *floatBuffer = *( pointersBuffer[i] );
  }

template <>
void GAUtils::serializeAndCopy<double>( MeshByte* buffer, double** pointersBuffer, int num )
  {
    double* doubleBuffer = reinterpret_cast< double* >( buffer );
	for ( int i=0; i<num; i++, doubleBuffer++ )
	  *doubleBuffer = *( pointersBuffer[i] );
  }

template <>
void GAUtils::serializeAndCopy<FloatVector>( MeshByte* buffer, FloatVector** pointersBuffer, int num )
  {
    int floatVectorSize = sizeof( float ) * D;
	for ( int i=0; i<num; i++, buffer += floatVectorSize )
	  memcpy( buffer, &( pointersBuffer[i]->operator[]( 0 ) ), floatVectorSize );
  }

template <>
void GAUtils::serializeAndCopy<DoubleVector>( MeshByte* buffer, DoubleVector** pointersBuffer, int num )
  {
    int doubleVectorSize = sizeof( double ) * D;
	for ( int i=0; i<num; i++, buffer += doubleVectorSize )
	  memcpy( buffer, &( pointersBuffer[i]->operator[]( 0 ) ), doubleVectorSize );
  }


// DESERIALIZATION
template <class T>
void GAUtils::deserializeAndCopy( MeshByte* buffer, T** pointersBuffer, int num )
  {
	GAError( "Error: You must provide a serialization function for this input data type" );
  }

template <>
void GAUtils::deserializeAndCopy<float>( MeshByte* buffer, float** pointersBuffer, int num )
  {
    float* floatBuffer = reinterpret_cast< float* >( buffer );
    for ( int i=0; i<num; i++, floatBuffer++ )
	  *( pointersBuffer[i] ) = *floatBuffer;
  }

template <>
void GAUtils::deserializeAndCopy<double>( MeshByte* buffer, double** pointersBuffer, int num )
  {
    double* doubleBuffer = reinterpret_cast< double* >( buffer );
    for ( int i=0; i<num; i++, doubleBuffer++ )
	  *( pointersBuffer[i] ) = *doubleBuffer;
  }

template <>
void GAUtils::deserializeAndCopy<FloatVector>( MeshByte* buffer, FloatVector** pointersBuffer, int num )
  {
    int floatVectorSize = sizeof( float ) * D;
	for ( int i=0; i<num; i++, buffer += floatVectorSize )
	  memcpy( &( pointersBuffer[i]->operator[]( 0 ) ), buffer, floatVectorSize );
  }

template <>
void GAUtils::deserializeAndCopy<DoubleVector>( MeshByte* buffer, DoubleVector** pointersBuffer, int num )
  {
    int doubleVectorSize = sizeof( double ) * D;
	for ( int i=0; i<num; i++, buffer += doubleVectorSize )
	  memcpy( &( pointersBuffer[i]->operator[]( 0 ) ), buffer, doubleVectorSize );
  }  


// CONVEX CROSSOVER
template <class T>
void GAUtils::crossoverConvex( const T& t1, const T& t2, T& res )
  {
    throw GAError( "Convex crossover was not defined for this input type" );
  }

template <>
void GAUtils::crossoverConvex<float>( const float& t1, const float& t2, float& res )
  { 
    res = t1 + rand_float() * ( t2 - t1 );
  }
 
template <>
void GAUtils::crossoverConvex<double>( const double& t1, const double& t2, double& res )
  { 
    res = t1 + rand_float() * ( t2 - t1 );
  }

template <>
void GAUtils::crossoverConvex<FloatVector>( const FloatVector& t1, const FloatVector& t2, FloatVector& res )
  {
    float beta = rand_float();
	for ( int i=0; i<t1.size(); i++ )
	  res[i] = t1[i] * beta + ( 1 - beta ) * t2[i]; 
  }

template <>
void GAUtils::crossoverConvex<DoubleVector>( const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res )
  {
    float beta = rand_float();
	for ( int i=0; i<t1.size(); i++ )
	  res[i] = t1[i] * beta + ( 1 - beta ) * t2[i];
  }


// SCATTERED CROSSOVER
template <class T> 
void GAUtils::crossoverScattered( const T& t1, const T& t2, T& res )
  {
    throw GAError( "Scattered crossover was not defined for this input type" );
  }
  
template <>
void GAUtils::crossoverScattered<float>( const float& t1, const float& t2, float& res )
  {
    res = ( rand_float() < 0.5 ) ? t1 : t2;
  }
  
template <>
void GAUtils::crossoverScattered<double>( const double& t1, const double& t2, double& res )
  {
    res = ( rand_float() < 0.5 ) ? t1 : t2;
  }

template <> 
void GAUtils::crossoverScattered<FloatVector>( const FloatVector& t1, const FloatVector& t2, FloatVector& res )
  {
    for ( int i=0; i<t1.size(); i++ )
      res[i] = ( rand_float() < 0.5 ) ? t1[i] : t2[i];
  }
  
template <> 
void GAUtils::crossoverScattered<DoubleVector>( const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res )
  {
    for ( int i=0; i<t1.size(); i++ )
      res[i] = ( rand_float() < 0.5 ) ? t1[i] : t2[i];
  }  


// GAUSSIAN MUTATION
template <class T>
void GAUtils::mutationGaussian( const T& t, T& res )
  {
    throw GAError( "Gaussian mutation was not defined for this input type" );
  }

template <>
void GAUtils::mutationGaussian<float>( const float& t, float& res )
  {
    res = t + NURG.Normal( 0.0, mgScaleV[0] );
  }

template <>
void GAUtils::mutationGaussian<double>( const double& t, double& res )
  {
    res = t + NURG.Normal( 0.0, mgScaleV[0] );
  }

template <>
void GAUtils::mutationGaussian<FloatVector>( const FloatVector& t, FloatVector& res )
  {                         
    for ( int i=0; i<t.size(); i++ )
      res[i] = t[i] + NURG.Normal( 0.0, mgScaleV[i] );
  }

template <>
void GAUtils::mutationGaussian<DoubleVector>( const DoubleVector& t, DoubleVector& res )
  {                         
    for ( int i=0; i<t.size(); i++ )
      res[i] = t[i] + NURG.Normal( 0.0, mgScaleV[i] );
  }

void GAUtils::refreshMutationGaussianParameters( int numGen, int maxGen )
  {
    for ( int i=0; i<D; i++ )
      mgScaleV[i] *= 1 - ( shrink * numGen ) / maxGen;
  }
  


/******************************************************************************/
/*                        RANDOM POPULATION GENERATORS                        */
/******************************************************************************/

// FLOAT
void GAUtils::generate( int popSize, vector<float>& v, float lb, float ub )
  {
    if ( popSize < 1 )
      throw GAError( "Bad population size" );
    if ( lb > ub )
      throw GAError( "The upper bound must be greater or equal than the lower bond" );
    
    // allocate memory for v if necessary
    if ( v.size() != popSize )
      v.resize( popSize );
    
    if ( mgScaleV == NULL )
      mgScaleV = new double[1];
    mgScaleV[0] = ub - lb;
    D = 1;
    
    rand_init( GEN_BASE_SEED + GEN_MUL_SEED * ( meshInterfacePointer->getMyID() ) );
    for ( int i=0; i<popSize; i++ )
      v[i] = lb + ( ub - lb ) * rand_float();
    
    /*	
    cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      cout << v[i] << ", ";
    cout << "\n"; */
  }

// DOUBLE
void GAUtils::generate( int popSize, vector<double>& v, double lb, double ub )
  {
    if ( popSize < 1 )
      throw GAError( "Bad population size" );
    if ( lb > ub )
      throw GAError( "The upper bound must be greater or equal than the lower bond" );
    
    // allocate memory for v if necessary
    if ( v.size() != popSize )
      v.resize( popSize );
    
    if ( mgScaleV == NULL )
      mgScaleV = new double[1];
    mgScaleV[0] = ub - lb;
    D = 1;
    
    rand_init( GEN_BASE_SEED + GEN_MUL_SEED * ( meshInterfacePointer->getMyID() ) );
    for ( int i=0; i<popSize; i++ )
      v[i] = lb + ( ub - lb ) * rand_float();
    
	/*
    cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      cout << v[i] << ", ";
    cout << "\n"; */
  }

// FLOATVECTOR
void GAUtils::generate( int popSize, vector<FloatVector>& v, const vector<float>& lbs, const vector<float>& ubs )
  {
    if ( popSize < 1 )
      throw GAError( "Bad population size" );
	int dim = lbs.size();
    if ( dim < 1 )
      throw GAError( "Bad FloatVector size" );
    if ( ubs.size() != dim )
      throw GAError( "Upper bounds vector and lower bounds vector are not of the same size" );
    for ( int i=0; i<dim; i++ )
      if ( lbs[i] > ubs[i] )
        throw GAError( "An upper bound must be greater or equal than corresponding lower bound" );
      
    // allocates memory for v if necessary
    if ( v.size() != popSize )
      v.resize( popSize );
    if ( v[0].size() != dim )
      {
        for ( int i=0; i<popSize; i++ )
          v[i].resize( dim );
      }

    // allocates mempory for scaling vector only if necessary
    if ( dim != D )
	  {
		if ( mgScaleV )
		  delete [] mgScaleV;
		mgScaleV = new double[dim];
	  }
	// generates scaling vector for mutationGaussian
	for ( int i=0; i<dim; i++ )
      mgScaleV[i] = ubs[i] - lbs[i];
    
	D = dim;
	
    // generation of the initial population
    rand_init( GEN_BASE_SEED + GEN_MUL_SEED * ( meshInterfacePointer->getMyID() ) );
    for ( int i=0; i<popSize; i++ )
      for ( int d=0; d<dim; d++ )
		v[i][d] = lbs[d] + ( ubs[d] - lbs[d] ) * rand_float();
	

    /*cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      {
        cout << "(";
        for ( int d=0; d<dim-1; d++ )
          cout << v[i][d] << ", ";
        cout << v[i][dim-1] << ")";
      } */
  }

// DOUBLEVECTOR
void GAUtils::generate( int popSize, vector<DoubleVector>& v, const vector<double>& lbs, const vector<double>& ubs )
  {
   if ( popSize < 1 )
      throw GAError( "Bad population size" );
	int dim = lbs.size();
    if ( dim < 1 )
      throw GAError( "Bad FloatVector size" );
    if ( ubs.size() != dim )
      throw GAError( "Upper bounds vector and lower bounds vector are not of the same size" );
    for ( int i=0; i<dim; i++ )
      if ( lbs[i] > ubs[i] )
        throw GAError( "An upper bound must be greater or equal than corresponding lower bound" );
      
    // allocates memory for v if necessary
    if ( v.size() != popSize )
      v.resize( popSize );
    if ( v[0].size() != dim )
      {
        for ( int i=0; i<popSize; i++ )
          v[i].resize( dim );
      }

    // generates scaling vector for mutationGaussian   
    if ( dim != D )
	  {
		if ( mgScaleV )
		  delete [] mgScaleV;
		mgScaleV = new double[dim];
	  }
    for ( int i=0; i<dim; i++ )
      mgScaleV[i] = ubs[i] - lbs[i];
    
	D = dim;
	
    // generation of the initial population
    rand_init( GEN_BASE_SEED + GEN_MUL_SEED * ( meshInterfacePointer->getMyID() ) );
    for ( int i=0; i<popSize; i++ )
      for ( int d=0; d<dim; d++ )
		v[i][d] = lbs[d] + ( ubs[d] - lbs[d] ) * rand_float();
	
	/*
    cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      {
        cout << "(";
        for ( int d=0; d<dim-1; d++ )
          cout << v[i][d] << ", ";
        cout << v[i][dim-1] << ")";
      } */
  }
////////////////////////////////////////////////////////////////////////////////


/* after the the main function execution, the destructor of "destroyer" closes
   the message passing library without burden the user*/
class Destroyer
  {
    public:
      ~Destroyer() { SccMeshInterface::MessagePassingLibraryFinalize(); }
  };
  
Destroyer destroyer;


#endif
