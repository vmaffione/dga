#include <iostream>
#include <time.h>
#include <stdlib.h>

#include "peer-server.hpp"
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



int work(int argc, char **argv)
{
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

static PeerServer *server = NULL;

static void *
server_function(void *arg)
{
    server->run();

    return NULL;
}

static void
sigint_handler(int signum)
{
    server->leave();

    exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
    unsigned int s_port;
    unsigned int j_port = ~0U;
    struct sigaction sa;
    pthread_t server_tid;
    int ret;

    if (argc < 2) {
        exit_with_error("USAGE: program PORT [JOINPORT]");
    }
    s_port = atoi(argv[1]);
    if (s_port >= 65535) {
        errno = EINVAL;
        exit_with_error("PORT > 65535");
    }

    if (argc > 2) {
        j_port = atoi(argv[2]);
        if (j_port >= 65535) {
            errno = EINVAL;
            exit_with_error("PORT > 65535");
        }
    }

    server = new PeerServer(s_port, j_port);

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        cout << "   Warning: SIGINT handler registration failed" << endl;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        cout << "   Warning: SIGTERM handler registration failed" << endl;
    }

    /* Start the server. */
    if (pthread_create(&server_tid, NULL, server_function, NULL)) {
        exit_with_error("pthread_create()");
    }

    if (j_port != ~0U) {
        /* Carry out the join procedure with the manager server. */
        server->join();
    }

    ret = work(argc, argv);

    /* Wait for the member server to complete - it still holds the memory
     * for 'server', which is ours. */
    if (pthread_join(server_tid, NULL)) {
        exit_with_error("pthread_join()");
    }

    delete server;

    return ret;
}
