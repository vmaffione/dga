#include <iostream>
#include <time.h>
#include <stdlib.h>

#include "ga.h"

int l = 0;

using namespace std;


/* FloatVector example */
float objfcn(const FloatVector& v)
{
    return 3*(v[0])*(v[0]) + 5*(v[1])*(v[1]);
}

///////////////////////



/* float example */
float obj(const float& f)
{return 2.0*f*f - 1.6*f + 2;}

/////////////////////////



int main(int argc, char **argv)
{
    GAUtils::frameworkInit(&argc, &argv);

    try
    {
        unsigned int popsize = 30;
        unsigned int numIter = 750;
        unsigned int elichi = (((unsigned int)(popsize*0.04)) > 1) ? ((unsigned int)(popsize*0.04)) : 1;  // number of elite children

        /*
           cout << "\n\n FLOATVECTOR EXAMPLE: \n";
           vector<float> lbs(2);  // lower bounds
           lbs[0] = -5.0; lbs[1] = -10;
           vector<float> ubs(2);   // upper bounds
           ubs[0] = 6.0; ubs[1] = 6.5;

           GeneticAlgorithm<FloatVector, float> G(&objfcn, GAUtils::MutationGaussian, GAUtils::CrossoverScattered, GAUtils::SUS);
           vector<FloatVector> inipop;
           G.gaUtils.generate(popsize, inipop, lbs, ubs);  // generates random initial population
           G.run(inipop, numIter, 0.8, elichi);              // run the algorithm
           */

        //GeneticAlgorithm<float, float> G2(&obj, &mt, &cr, GAUtils::SUS);
        GeneticAlgorithm<float, float> G2(&obj, GAUtils::MutationGaussian, GAUtils::CrossoverConvex, GAUtils::SUS);
        vector<float> finipop;
        G2.gaUtils.generate(popsize, finipop, -10.0, 13.3);
        G2.run(finipop, numIter, 0.8, elichi);
    }
    catch (GAError) {  }

    cout << "Ultima!\n";

    return 0;
}
