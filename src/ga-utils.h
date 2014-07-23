#ifndef GAUTILS_
#define GAUTILS_

#include <vector>
#include <fstream>

#include "peer-server.hpp"

using namespace std;


typedef vector<float> FloatVector;
typedef vector<double> DoubleVector;

ostream& operator<<(ostream& os, const FloatVector& r);
ostream& operator<<(ostream& os, const DoubleVector& r);

//  Agner Fog's random number generator librares
#include "randoma.h"  // uniform generator
#define rand_init(seed)  SFMTgenRandomInit(seed,0)
#define rand_float()  SFMTgenRandom()                        // random float number in [0,1[
#define rand_int(l, b)  SFMTgenIRandom((l), (b))     // random integer number in [l,b]  (inexact)
#include "stocc.h"  // non-uniform generator

/*
// obsolete random number generator
#define rand_init() srand(time(0))
#define rand_float()  (rand()*(1.0/(RAND_MAX + 1.0)))  // random float number in [0,1]
#define rand_int(l, n)  ((int) ((rand() / (RAND_MAX + 1.0)) * ((n) + 1)))  // random integer number in [0,n]
*/

template <class IT, class OT> class GeneticAlgorithm;  // not necessary

class GAUtils
{
    //template <class IT, class OT> friend class GeneticAlgorithm;  // ORA SERVE PER FARE STAMPE DI DEBUG, MA ANDREBBE CANCELLATO
    public:
        enum SelectionFunctionType { SUS = 0, RWS };
        enum CrossoverFunctionType { CrossoverConvex = 0, CrossoverScattered };
        enum MutationFunctionType { MutationGaussian = 0, MUT_OTHER };

        unsigned int D;  // DA RENDERE PRIVATO!!! ORA E' PUBBLICO PERCHè LO UTILIZZANO LE STAMPE DI DEBUG IN GA

    private:
        PeerServer& server;
        double* mgScaleV;     // scale vector for mutationGaussian
        float shrink;        // shrink factor for mutationGaussian

        static const int GEN_BASE_SEED = 6536541;
        static const int GEN_MUL_SEED = 83;
        static const int NURG_SEED = 613788543;
        StochasticLib1 NURG;  // non-uniform random generator

    public:
        GAUtils(PeerServer &_server): D(0), server(_server), mgScaleV(NULL), shrink(1.0), NURG(NURG_SEED) { }   // DA FARE: NURG dovrebbe essere inizializzato in modo diverso su core diversi...

        template <class T> void mutationGaussian(const T& t, T& res);
        template <class T> void crossoverConvex(const T& t1, const T& t2, T& res);
        template <class T> void crossoverScattered(const T& t1, const T& t2, T& res);

        void refreshMutationGaussianParameters(int numGen, int maxGen);

        template <class T> void serializeAndCopy(uint8_t* buffer, T** pointersBuffer, int num);
        template <class T> void deserializeAndCopy(uint8_t* buffer, T** pointersBuffer, int num);

        void generate(unsigned int N, vector<float>& v, float lb, float ub);
        void generate(unsigned int popSize, vector<double>& v, double lb, double ub);
        void generate(unsigned int N, vector<FloatVector>& v, const vector<float>& lbs, const vector<float>& ubs);
        void generate(unsigned int N, vector<DoubleVector>& v, const vector<double>& lbs, const vector<double>& ubs);

        /*
           class RandomGenerator
           {
           public:
           void generate(int N, vector<float>& v, float lb, float ub);
           void generate(unsigned int popSize, vector<double>& v, double lb, double ub);
           void generate(int N, vector<FloatVector>& v, int dim, const vector<float>& lbs, const vector<float>& ubs);
           void generate(int N, vector<DoubleVector>& v, int dim, const vector<double>& lbs, const vector<double>& ubs);
           };
           RandomGenerator randomGenerator;*/
};


// SERIALIZATION
    template <class T>
void GAUtils::serializeAndCopy(uint8_t* buffer, T** pointersBuffer, int num)
{
    GAError("Error: You must provide a serialization function for this input data type");
}

template <>
void GAUtils::serializeAndCopy<float>(uint8_t* buffer, float** pointersBuffer, int num);

template <>
void GAUtils::serializeAndCopy<double>(uint8_t* buffer, double** pointersBuffer, int num);

template <>
void GAUtils::serializeAndCopy<FloatVector>(uint8_t* buffer, FloatVector** pointersBuffer, int num);

template <>
void GAUtils::serializeAndCopy<DoubleVector>(uint8_t* buffer, DoubleVector** pointersBuffer, int num);


// DESERIALIZATION
template <class T>
void GAUtils::deserializeAndCopy(uint8_t* buffer, T** pointersBuffer, int num)
{
    GAError("Error: You must provide a serialization function for this input data type");
}

template <>
void GAUtils::deserializeAndCopy<float>(uint8_t* buffer, float** pointersBuffer, int num);

template <>
void GAUtils::deserializeAndCopy<double>(uint8_t* buffer, double** pointersBuffer, int num);

template <>
void GAUtils::deserializeAndCopy<FloatVector>(uint8_t* buffer, FloatVector** pointersBuffer, int num);

template <>
void GAUtils::deserializeAndCopy<DoubleVector>(uint8_t* buffer, DoubleVector** pointersBuffer, int num);


// CONVEX CROSSOVER
template <class T>
void GAUtils::crossoverConvex(const T& t1, const T& t2, T& res)
{
    throw GAError("Convex crossover was not defined for this input type");
}

template <>
void GAUtils::crossoverConvex<float>(const float& t1, const float& t2, float& res);

template <>
void GAUtils::crossoverConvex<double>(const double& t1, const double& t2, double& res);

template <>
void GAUtils::crossoverConvex<FloatVector>(const FloatVector& t1, const FloatVector& t2, FloatVector& res);

template <>
void GAUtils::crossoverConvex<DoubleVector>(const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res);


// SCATTERED CROSSOVER
template <class T>
void GAUtils::crossoverScattered(const T& t1, const T& t2, T& res)
{
    throw GAError("Scattered crossover was not defined for this input type");
}

template <>
void GAUtils::crossoverScattered<float>(const float& t1, const float& t2, float& res);

template <>
void GAUtils::crossoverScattered<double>(const double& t1, const double& t2, double& res);

template <>
void GAUtils::crossoverScattered<FloatVector>(const FloatVector& t1, const FloatVector& t2, FloatVector& res);

template <>
void GAUtils::crossoverScattered<DoubleVector>(const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res);


// GAUSSIAN MUTATION
template <class T>
void GAUtils::mutationGaussian(const T& t, T& res)
{
    throw GAError("Gaussian mutation was not defined for this input type");
}

template <>
void GAUtils::mutationGaussian<float>(const float& t, float& res);

template <>
void GAUtils::mutationGaussian<double>(const double& t, double& res);

template <>
void GAUtils::mutationGaussian<FloatVector>(const FloatVector& t, FloatVector& res);

template <>
void GAUtils::mutationGaussian<DoubleVector>(const DoubleVector& t, DoubleVector& res);

#endif
