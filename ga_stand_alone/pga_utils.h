#ifndef GAUTILS_
#define GAUTILS_

#include "valarrayM.h"


//  Agner Fog's random number generator librares
#include "randoma.h"  // uniform generator
#define rand_init()  SFMTgenRandomInit(time(0),0)
#define rand_float()  SFMTgenRandom()                        // random float number in [0,1[
#define rand_int( l, b )  SFMTgenIRandom( ( l ), ( b ) )     // random integer number in [l,b]  (inexact)
#include "stocc.h"  // non-uniform generator

template <class IT, class OT> class GeneticAlgorithm;  // not necessary

class GAUtils
  {
      template <class IT, class OT> friend class GeneticAlgorithm;  // so that GeneticAlgorithm can modify mutationGaussian options
    public:
      enum SelectionFunctionType { SUS = 0, RWS };
      enum CrossoverFunctionType { CrossoverConvex = 0, CrossoverScattered };
      enum MutationFunctionType { MutationGaussian = 0, MUT_OTHER };
    private:
      double* mgScaleV;     // scale vector for mutationGaussian
      float shrink;        // shrink factor for mutationGaussian
      int D;

      StochasticLib1 NURG;  // non-uniform random generator

    public:
      GAUtils(): mgScaleV( NULL ), shrink( 1.0 ), D( 0 ), NURG( time( 0 ) ) { }
      
      template <class T> void mutationGaussian( const T& t, T& res );
      template <class T> void crossoverConvex( const T& t1, const T& t2, T& res );
      template <class T> void crossoverScattered( const T& t1, const T& t2, T& res );
      
      void generate( int N, vector<float>& v, float lb, float ub );
      void generate( int popSize, vector<double>& v, double lb, double ub );
      void generate( int N, vector<FloatVector>& v, int dim, const vector<float>& lbs, const vector<float>& ubs );
      void generate( int N, vector<DoubleVector>& v, int dim, const vector<double>& lbs, const vector<double>& ubs );
      
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
    res = t2;
    res -= t1;
    res *= rand_float();
    res += t1;
  }

template <>
void GAUtils::crossoverConvex<DoubleVector>( const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res )
  {
    res = t2;
    res -= t1;
    res *= rand_float();
    res += t1;
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
    for ( int i=0; i<D; i++ )
      res[i] = ( rand_float() < 0.5 ) ? t1[i] : t2[i];
  }
  
template <> 
void GAUtils::crossoverScattered<DoubleVector>( const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res )
  {
    for ( int i=0; i<D; i++ )
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
    for ( int i=0; i<D; i++ )  // D is safe here
      res[i] = t[i] + NURG.Normal( 0.0, mgScaleV[i] );
  }

template <>
void GAUtils::mutationGaussian<DoubleVector>( const DoubleVector& t, DoubleVector& res )
  {                         
    for ( int i=0; i<D; i++ )  // D is safe here
      res[i] = t[i] + NURG.Normal( 0.0, mgScaleV[i] );
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
    
    rand_init();
    for ( int i=0; i<popSize; i++ )
      v[i] = lb + ( ub - lb ) * rand_float();
      
    cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      cout << v[i] << ", ";
    cout << "\n";
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
    
    rand_init();
    for ( int i=0; i<popSize; i++ )
      v[i] = lb + ( ub - lb ) * rand_float();
      
    cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      cout << v[i] << ", ";
    cout << "\n";
  }

// FLOATVECTOR
void GAUtils::generate( int popSize, vector<FloatVector>& v, int dim, const vector<float>& lbs, const vector<float>& ubs )
  {
    if ( popSize < 1 )
      throw GAError( "Bad population size" );
    if ( dim < 1 )
      throw GAError( "Bad FloatVector size" );
    if ( lbs.size() != dim )
      throw GAError( "Bad lower bound vector size" );
    if ( ubs.size() != dim )
      throw GAError( "Bad upper bound vector size" );
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
    
    D = dim;

    // generates scaling vector for mutationGaussian   
    if ( mgScaleV != NULL )
      delete [] mgScaleV;
    mgScaleV = new double[dim];
    for ( int i=0; i<dim; i++ )
      mgScaleV[i] = ubs[i] - lbs[i];
    
    // generation of the initial population
    FloatVector r( dim );
    rand_init();
    for ( int i=0; i<popSize; i++ )
      {
        for ( int d=0; d<dim; d++ )
          r[d] = lbs[d] + ( ubs[d] - lbs[d] ) * rand_float();
        v[i] = r;
      }

    cout << "Initial population, randomly generated:\n";  
    for ( int i=0; i<popSize; i++ )
      {
        cout << "(";
        for ( int d=0; d<dim-1; d++ )
          cout << v[i][d] << ", ";
        cout << v[i][dim-1] << ")";
      }
  }

// DOUBLEVECTOR
void GAUtils::generate( int popSize, vector<DoubleVector>& v, int dim, const vector<double>& lbs, const vector<double>& ubs )
  {
   if ( popSize < 1 )
      throw GAError( "Bad population size" );
    if ( dim < 1 )
      throw GAError( "Bad FloatVector size" );
    if ( lbs.size() != dim )
      throw GAError( "Bad lower bound vector size" );
    if ( ubs.size() != dim )
      throw GAError( "Bad upper bound vector size" );
      
    // allocates memory for v if necessary
    if ( v.size() != popSize )
      v.resize( popSize );
    if ( v[0].size() != dim )
      {
        for ( int i=0; i<popSize; i++ )
          v[i].resize( dim );
      }
       
    D = dim;   
      
    // generates scaling vector for mutationGaussian   
    if ( mgScaleV != NULL )
      delete [] mgScaleV;
    mgScaleV = new double[dim];
    for ( int i=0; i<dim; i++ )
      mgScaleV[i] = ubs[i] - lbs[i];                      
                                
    // generation of the initial population
    DoubleVector r( dim );
    rand_init();
    for ( int i=0; i<popSize; i++ )
      {
        for ( int d=0; d<dim; d++ )
          r[d] = lbs[d] + ( ubs[d] - lbs[d] ) * rand_float();
        v[i] = r;
      }
      
    cout << "Initial population, randomly generated\n:";  
    for ( int i=0; i<popSize; i++ )
      {
        for ( int d=0; d<dim; d++ )
          cout << v[i][d] << ", ";
        cout << "\n";
      }
  }
////////////////////////////////////////////////////////////////////////////////


// obsolete random number generator
/*
#define drand()  (rand()*(1.0/(RAND_MAX + 1.0)))  // random float number in [0,1]
#define nrand( n )   ( (int) ( ( rand() / ( RAND_MAX + 1.0 ) ) * ( ( n ) + 1 ) ) )  // random integer number in [0,n]
*/

#endif
