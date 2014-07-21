#include "ga-utils.h"


ostream& operator<<(ostream& os, const FloatVector& r)
{
    os << "(";
    for (unsigned int d=0 ; d<r.size()-1; d++)
        os << r[d] << ", ";
    os << r[r.size()-1] << ")";

    return os;
}

ostream& operator<<(ostream& os, const DoubleVector& r)
{
    os << "(";
    for (unsigned int d=0; d<r.size()-1; d++)
        os << r[d] << ", ";
    os << r[r.size()-1] << ")";

    return os;
}

template <>
void GAUtils::serializeAndCopy<float>(MeshByte* buffer, float** pointersBuffer, int num)
{
    float* floatBuffer = reinterpret_cast< float* >(buffer);
    for (int i=0; i<num; i++, floatBuffer++)
        *floatBuffer = *(pointersBuffer[i]);
}

template <>
void GAUtils::serializeAndCopy<double>(MeshByte* buffer, double** pointersBuffer, int num)
{
    double* doubleBuffer = reinterpret_cast< double* >(buffer);
    for (int i=0; i<num; i++, doubleBuffer++)
        *doubleBuffer = *(pointersBuffer[i]);
}

template <>
void GAUtils::serializeAndCopy<FloatVector>(MeshByte* buffer, FloatVector** pointersBuffer, int num)
{
    int floatVectorSize = sizeof(float) * D;
    for (int i=0; i<num; i++, buffer += floatVectorSize)
        memcpy(buffer, &(pointersBuffer[i]->operator[](0)), floatVectorSize);
}

template <>
void GAUtils::serializeAndCopy<DoubleVector>(MeshByte* buffer, DoubleVector** pointersBuffer, int num)
{
    int doubleVectorSize = sizeof(double) * D;
    for (int i=0; i<num; i++, buffer += doubleVectorSize)
        memcpy(buffer, &(pointersBuffer[i]->operator[](0)), doubleVectorSize);
}

template <>
void GAUtils::deserializeAndCopy<float>(MeshByte* buffer, float** pointersBuffer, int num)
{
    float* floatBuffer = reinterpret_cast< float* >(buffer);
    for (int i=0; i<num; i++, floatBuffer++)
        *(pointersBuffer[i]) = *floatBuffer;
}

template <>
void GAUtils::deserializeAndCopy<double>(MeshByte* buffer, double** pointersBuffer, int num)
{
    double* doubleBuffer = reinterpret_cast< double* >(buffer);
    for (int i=0; i<num; i++, doubleBuffer++)
        *(pointersBuffer[i]) = *doubleBuffer;
}

template <>
void GAUtils::deserializeAndCopy<FloatVector>(MeshByte* buffer, FloatVector** pointersBuffer, int num)
{
    int floatVectorSize = sizeof(float) * D;
    for (int i=0; i<num; i++, buffer += floatVectorSize)
        memcpy(&(pointersBuffer[i]->operator[](0)), buffer, floatVectorSize);
}

template <>
void GAUtils::deserializeAndCopy<DoubleVector>(MeshByte* buffer, DoubleVector** pointersBuffer, int num)
{
    int doubleVectorSize = sizeof(double) * D;
    for (int i=0; i<num; i++, buffer += doubleVectorSize)
        memcpy(&(pointersBuffer[i]->operator[](0)), buffer, doubleVectorSize);
}

template <>
void GAUtils::crossoverConvex<float>(const float& t1, const float& t2, float& res)
{
    res = t1 + rand_float() * (t2 - t1);
}

template <>
void GAUtils::crossoverConvex<double>(const double& t1, const double& t2, double& res)
{
    res = t1 + rand_float() * (t2 - t1);
}

template <>
void GAUtils::crossoverConvex<FloatVector>(const FloatVector& t1, const FloatVector& t2, FloatVector& res)
{
    float beta = rand_float();

    for (unsigned int i=0; i<t1.size(); i++)
        res[i] = t1[i] * beta + (1 - beta) * t2[i];
}

template <>
void GAUtils::crossoverConvex<DoubleVector>(const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res)
{
    float beta = rand_float();

    for (unsigned int i=0; i<t1.size(); i++)
        res[i] = t1[i] * beta + (1 - beta) * t2[i];
}

template <>
void GAUtils::crossoverScattered<float>(const float& t1, const float& t2, float& res)
{
    res = (rand_float() < 0.5) ? t1 : t2;
}

template <>
void GAUtils::crossoverScattered<double>(const double& t1, const double& t2, double& res)
{
    res = (rand_float() < 0.5) ? t1 : t2;
}

template <>
void GAUtils::crossoverScattered<FloatVector>(const FloatVector& t1, const FloatVector& t2, FloatVector& res)
{
    for (unsigned int i=0; i<t1.size(); i++)
        res[i] = (rand_float() < 0.5) ? t1[i] : t2[i];
}

template <>
void GAUtils::crossoverScattered<DoubleVector>(const DoubleVector& t1, const DoubleVector& t2, DoubleVector& res)
{
    for (unsigned int i=0; i<t1.size(); i++)
        res[i] = (rand_float() < 0.5) ? t1[i] : t2[i];
}

template <>
void GAUtils::mutationGaussian<float>(const float& t, float& res)
{
    res = t + NURG.Normal(0.0, mgScaleV[0]);
}

template <>
void GAUtils::mutationGaussian<double>(const double& t, double& res)
{
    res = t + NURG.Normal(0.0, mgScaleV[0]);
}

template <>
void GAUtils::mutationGaussian<FloatVector>(const FloatVector& t, FloatVector& res)
{
    for (unsigned int i=0; i<t.size(); i++)
        res[i] = t[i] + NURG.Normal(0.0, mgScaleV[i]);
}

template <>
void GAUtils::mutationGaussian<DoubleVector>(const DoubleVector& t, DoubleVector& res)
{
    for (unsigned int i=0; i<t.size(); i++)
        res[i] = t[i] + NURG.Normal(0.0, mgScaleV[i]);
}

void GAUtils::refreshMutationGaussianParameters(int numGen, int maxGen)
{
    for (unsigned int i=0; i<D; i++)
        mgScaleV[i] *= 1 - (shrink * numGen) / maxGen;
}

/******************************************************************************/
/*                        RANDOM POPULATION GENERATORS                        */
/******************************************************************************/

// FLOAT
void GAUtils::generate(unsigned int popSize, vector<float>& v, float lb, float ub)
{
    if (popSize < 1)
        throw GAError("Bad population size");
    if (lb > ub)
        throw GAError("The upper bound must be greater or equal than the lower bond");

    // allocate memory for v if necessary
    if (v.size() != popSize)
        v.resize(popSize);

    if (mgScaleV == NULL)
        mgScaleV = new double[1];
    mgScaleV[0] = ub - lb;
    D = 1;

    rand_init(GEN_BASE_SEED + GEN_MUL_SEED * (server.get_unique()));
    for (unsigned int i=0; i<popSize; i++)
        v[i] = lb + (ub - lb) * rand_float();

    /*
        cout << "Initial population, randomly generated:\n";
        for (int i=0; i<popSize; i++)
        cout << v[i] << ", ";
        cout << "\n"; */
}

// DOUBLE
void GAUtils::generate(unsigned int popSize, vector<double>& v, double lb, double ub)
{
    if (popSize < 1)
        throw GAError("Bad population size");
    if (lb > ub)
        throw GAError("The upper bound must be greater or equal than the lower bond");

    // allocate memory for v if necessary
    if (v.size() != popSize)
        v.resize(popSize);

    if (mgScaleV == NULL)
        mgScaleV = new double[1];
    mgScaleV[0] = ub - lb;
    D = 1;

    rand_init(GEN_BASE_SEED + GEN_MUL_SEED * (server.get_unique()));
    for (unsigned int i=0; i<popSize; i++)
        v[i] = lb + (ub - lb) * rand_float();

    /*
       cout << "Initial population, randomly generated:\n";
       for (int i=0; i<popSize; i++)
       cout << v[i] << ", ";
       cout << "\n"; */
}

// FLOATVECTOR
void GAUtils::generate(unsigned int popSize, vector<FloatVector>& v, const vector<float>& lbs, const vector<float>& ubs)
{
    if (popSize < 1)
        throw GAError("Bad population size");
    unsigned int dim = lbs.size();
    if (dim < 1)
        throw GAError("Bad FloatVector size");
    if (ubs.size() != dim)
        throw GAError("Upper bounds vector and lower bounds vector are not of the same size");
    for (unsigned int i=0; i<dim; i++)
        if (lbs[i] > ubs[i])
            throw GAError("An upper bound must be greater or equal than corresponding lower bound");

    // allocates memory for v if necessary
    if (v.size() != popSize)
        v.resize(popSize);
    if (v[0].size() != dim)
    {
        for (unsigned int i=0; i<popSize; i++)
            v[i].resize(dim);
    }

    // allocates mempory for scaling vector only if necessary
    if (dim != D)
    {
        if (mgScaleV)
            delete [] mgScaleV;
        mgScaleV = new double[dim];
    }
    // generates scaling vector for mutationGaussian
    for (unsigned int i=0; i<dim; i++)
        mgScaleV[i] = ubs[i] - lbs[i];

    D = dim;

    // generation of the initial population
    rand_init(GEN_BASE_SEED + GEN_MUL_SEED * (server.get_unique()));
    for (unsigned int i=0; i<popSize; i++)
        for (unsigned int d=0; d<dim; d++)
            v[i][d] = lbs[d] + (ubs[d] - lbs[d]) * rand_float();


    /*cout << "Initial population, randomly generated:\n";
      for (int i=0; i<popSize; i++)
      {
      cout << "(";
      for (int d=0; d<dim-1; d++)
      cout << v[i][d] << ", ";
      cout << v[i][dim-1] << ")";
      } */
}

// DOUBLEVECTOR
void GAUtils::generate(unsigned int popSize, vector<DoubleVector>& v, const vector<double>& lbs, const vector<double>& ubs)
{
    unsigned int dim = lbs.size();

    if (popSize < 1)
        throw GAError("Bad population size");
    if (dim < 1)
        throw GAError("Bad FloatVector size");
    if (ubs.size() != dim)
        throw GAError("Upper bounds vector and lower bounds vector are not of the same size");
    for (unsigned int i=0; i<dim; i++)
        if (lbs[i] > ubs[i])
            throw GAError("An upper bound must be greater or equal than corresponding lower bound");

    // allocates memory for v if necessary
    if (v.size() != popSize)
        v.resize(popSize);
    if (v[0].size() != dim)
    {
        for (unsigned int i=0; i<popSize; i++)
            v[i].resize(dim);
    }

    // generates scaling vector for mutationGaussian
    if (dim != D)
    {
        if (mgScaleV)
            delete [] mgScaleV;
        mgScaleV = new double[dim];
    }
    for (unsigned int i=0; i<dim; i++)
        mgScaleV[i] = ubs[i] - lbs[i];

    D = dim;

    // generation of the initial population
    rand_init(GEN_BASE_SEED + GEN_MUL_SEED * (server.get_unique()));
    for (unsigned int i=0; i<popSize; i++)
        for (unsigned int d=0; d<dim; d++)
            v[i][d] = lbs[d] + (ubs[d] - lbs[d]) * rand_float();

    /*
       cout << "Initial population, randomly generated:\n";
       for (int i=0; i<popSize; i++)
       {
       cout << "(";
       for (int d=0; d<dim-1; d++)
       cout << v[i][d] << ", ";
       cout << v[i][dim-1] << ")";
       } */
}
////////////////////////////////////////////////////////////////////////////////

