#include "ga.hpp"
#include <iostream>

using namespace std;

/* This file contains the code that has to be written by the user. */


/* FloatVector example */
float objfcn(const FloatVector& v)
{
    if (v[0] > 5.0 && v[1] > 5.0) {
        return 5*(v[0] - 14.23)*(v[0] - 14.23) + 3*(v[1] - 31.98)*(v[1] - 31.98);
    }

    return 12*(v[0] - 1.3)*(v[0] - 1.3) + 14*(v[1] + 3.14)*(v[1] + 3.14);
}

///////////////////////


/* Float example */
float obj(const float& f)
{return 2.0*f*f - 1.6*f + 2;}

/////////////////////////


int work(GAPeerServer *serv)
{
    try
    {
        unsigned int popsize = 30;
        unsigned int numIter = 750;
        unsigned int elichi = (((unsigned int)(popsize*0.04)) > 1) ? ((unsigned int)(popsize*0.04)) : 1;  // number of elite children

#if 0
        /* Float vector example. */
        vector<float> lbs(2);  // lower bounds
        lbs[0] = -100.0; lbs[1] = -100.0;
        vector<float> ubs(2);   // upper bounds
        ubs[0] = 100.0; ubs[1] = 100.0;

        GeneticAlgorithm<FloatVector, float> G(&objfcn, GAUtils::MutationGaussian, GAUtils::CrossoverScattered, GAUtils::SUS, *serv);
        vector<FloatVector> inipop;
        G.gaUtils.generate(popsize, inipop, lbs, ubs);  // generates random initial population
        G.run(inipop, numIter, 0.8, elichi);              // run the algorithm
#else
        GeneticAlgorithm<float, float> G2(&obj, GAUtils::MutationGaussian, GAUtils::CrossoverConvex, GAUtils::SUS, *serv);
        vector<float> finipop;
        G2.gaUtils.generate(popsize, finipop, -10.0, 13.3);
        G2.run(finipop, numIter, 0.8, elichi);
#endif
    }
    catch (GAError) {  }

    return 0;
}
