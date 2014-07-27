#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "ga.hpp"

int l = 0;

using namespace std;


/* FloatVector example */
float objfcn(const FloatVector& v)
{
    if (v[0] > 5.0 && v[1] > 5.0) {
        return 5*(v[0] - 14.23)*(v[0] - 14.23) + 3*(v[1] - 31.98)*(v[1] - 31.98);
    }

    return 12*(v[0] - 1.3)*(v[0] - 1.3) + 14*(v[1] + 3.14)*(v[1] + 3.14);
}

///////////////////////



/* float example */
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

static GAPeerServer *server = NULL;

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

static void
print_usage()
{
    cout << "Options usage:" << endl;
    cout << "\t-h: show this help" << endl;
    cout << "\t-p PORTNUM: port number of the peer server" << endl;
    cout << "\t-j PORTNUM: port number of the remote server to join" << endl;
    cout << endl;
}

int
main(int argc, char **argv)
{
    unsigned int s_port = ~0U;
    unsigned int j_port = ~0U;
    struct sigaction sa;
    pthread_t server_tid;
    int ret;
    int ch;

    while ((ch = getopt(argc, argv, "hp:j:")) != -1) {
        switch (ch) {
            case 'p':
                s_port = atoi(optarg);
                if (s_port >= 65535) {
                    errno = EINVAL;
                    exit_with_error("server port > 65535");
                }
                break;

            case 'j':
                j_port = atoi(optarg);
                if (j_port >= 65535) {
                    errno = EINVAL;
                    exit_with_error("join port > 65535");
                }
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
        }
    }

    if (s_port == ~0U) {
        cout << "server port missing" << endl;
        print_usage();
        exit(EXIT_FAILURE);
    }

    server = new GAPeerServer(s_port, j_port);

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

    ret = work(server);

    /* Wait for the member server to complete - it still holds the memory
     * for 'server', which is ours. */
    if (pthread_join(server_tid, NULL)) {
        exit_with_error("pthread_join()");
    }

    delete server;

    return ret;
}
