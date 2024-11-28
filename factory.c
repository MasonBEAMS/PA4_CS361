//---------------------------------------------------------------------
// Assignment : PA-04 Threads-UDP
// Date       : 12/3/2024
// Author     : Mason Scofield (scofi2ml@dukes.jmu.edu)
//              Zach Putz (putzzs@dukes.jmu.edu)
// File Name  : factory.c
//---------------------------------------------------------------------

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/time.h>

#include "wrappers.h"
#include "message.h"

#define MAXSTR     200
#define IPSTRLEN    50

typedef struct sockaddr SA ;

int minimum( int a , int b)
{
    return ( a <= b ? a : b ) ; 
}

void subFactory( void * arg ) ;

void factLog( char *str )
{
    printf( "%s" , str );
    fflush( stdout ) ;
}

/*-------------------------------------------------------*/

// Global Variable for Future Thread to Shared
int   remainsToMake , // Must be protected by a Mutex
      actuallyMade ;  // Actually manufactured items

int   numActiveFactories = 0 , orderSize ;
// sem_t mutex;
pthread_mutex_t factoryMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct timeval start;
struct timeval end;


int   sd ;      // Server socket descriptor
struct sockaddr_in  
             srvrSkt,       /* the address of this server   */
             clntSkt;       /* remote client's socket       */

typedef  struct {
    int id;
    int dur;
    int cap;
}  arg_t ;

typedef struct {
    int id;
    int partsMade;
    int iterations;
} result_t;

arg_t *threadResults;


//------------------------------------------------------------
//  Handle Ctrl-C or KILL 
//------------------------------------------------------------
void goodbye(int sig) 
{
    /* Mission Accomplished */
    printf( "\n### I (%d) have been nicely asked to TERMINATE. "
           "goodbye\n\n" , getpid() );  

    // missing code goes here
    msgBuf byeMsg;
    byeMsg.purpose = htonl(PROTOCOL_ERR);
    socklen_t alen = sizeof( clntSkt ) ;
    sendto(sd, &byeMsg, sizeof(byeMsg), 0, (SA *)&clntSkt, alen);
    close(sd);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&factoryMutex);
    exit(0);
    //The factory server should capture any signals meant to INTerrupt or TERMinate the server in the proper way.


}

/*-------------------------------------------------------*/
int main( int argc , char *argv[] )
{
    char  *myName = "Mason Scofield & Zachary Putz" ; 
    unsigned short port = 50015 ;      /* service port number  */
    int    N = 1 ;                     /* Num threads serving the client */
    char    ipStr[ IPSTRLEN ] ;    /* dotted-dec IP addr. */

    printf("\nThis is the FACTORY server developed by %s\n\n" , myName ) ;
    char myUserName[30] ;
    getlogin_r ( myUserName , 30 ) ;
    time_t  now;
    time( &now ) ;
    fprintf( stdout , "Logged in as user '%s' on %s\n\n" , myUserName ,  ctime( &now)  ) ;
    fflush( stdout ) ;
    int pshared = 1;

	switch (argc) 
	{
      case 1:
        break ;     // use default port with a single factory thread
      
      case 2:
        N = atoi( argv[1] ); // get from command line
        port = 50015;            // use this port by default
        break;

      case 3:
        N    = atoi( argv[1] ) ; // get from command line
        port = atoi( argv[2] ) ; // use port from command line
        break;

      default:
        printf( "FACTORY Usage: %s [numThreads] [port]\n" , argv[0] );
        exit( 1 ) ;
    }

        

    // missing code goes here
    sigactionWrapper(SIGINT, goodbye);
    sigactionWrapper(SIGTERM, goodbye);

    //set up socket
    
    sd = socket( AF_INET, SOCK_DGRAM  , 0 ) ;
    if ( sd < 0 )
        err_sys( "Could NOT create socket" ) ;

    memset(&srvrSkt, 0, sizeof(srvrSkt));
    srvrSkt.sin_family = AF_INET;
    srvrSkt.sin_addr.s_addr = htonl(INADDR_ANY);
    srvrSkt.sin_port = htons(port);

    printf("I will attempt to accept orders at port %d and use %d sub-factories.\n\n", port, N);

    
    if ( bind( sd, (SA *) & srvrSkt , sizeof(srvrSkt) ) < 0 )
    {
        printf("Could NOT bind to port %d", port );
    }
    inet_ntop( AF_INET, (void *) & srvrSkt.sin_addr.s_addr , ipStr , IPSTRLEN ) ;
    printf( "Bound socket %d to IP %s Port %d\n" , sd , ipStr , ntohs( srvrSkt.sin_port ) );

    pthread_t threadIDs[N];

    // threadIDs = malloc(N * sizeof(pthread_t));
    threadResults = malloc(N * sizeof(arg_t));
    if(threadIDs == NULL || threadResults == NULL) {
        fprintf(stderr, "Failed to allocate memory\n"); 
        exit(EXIT_FAILURE);  
    }

    int forever = 1;
    while ( forever )
    {
        printf( "\nFACTORY server waiting for Order Requests\n" ) ; 

        msgBuf msg1;

        socklen_t alen = sizeof( clntSkt ) ;

        if ( recvfrom( sd , &msg1 , sizeof(msg1) , 0 , (SA *) & clntSkt , & alen ) < 0 )
            err_sys( "recvfrom failed" ) ;
        inet_ntop( AF_INET, (void *) & clntSkt.sin_addr.s_addr , ipStr , IPSTRLEN ) ;
        printf("\n\nFACTORY server received: " ) ;
        printMsg( & msg1 );  puts("");
        printf("\tFrom IP %s Port %d\n" , ipStr , ntohs( clntSkt.sin_port ) ) ;
        gettimeofday(&start, NULL);

        // missing code goes here
        orderSize = ntohl(msg1.orderSize);
        pthread_mutex_lock(   & mutex  ) ;
        remainsToMake = orderSize;
        pthread_mutex_unlock(   & mutex  ) ;

        msgBuf msg2; //idk if i make a new msg or just replace the old one?
        msg2.numFac = htonl(N);
        msg2.purpose = htonl(ORDR_CONFIRM);
        if (sendto(sd, &msg2, sizeof(msg1), 0, (SA *)&clntSkt, alen) < 0)
            err_sys("sendto failed");



        printf("\n\nFACTORY sent this Order Confirmation to the client " );
        printMsg(  & msg2 );  puts("");
        /*
        immediately responds to that client with a ORDR_CONFIRM message over UDP containing the number N 
        of sub-factories that will manufacture the requested order. 
        For this project, N is always = 1. The server initializes the remainsToMake to be the same as the  orderSize.
        */
       srand(time(NULL));

       for(int i = 0; i < N; i++){
            int factory_num = i+1;
            //TODO:
            int factory_cap = (rand() % 41) + 10;//Maximum Capacity of items to make in one iteration (  10 <= Random <= 50 )
            int factory_duration = (rand() % 701) + 500;  //The duration ( 500 <= Random <= 1200 MILLISECONDS ) it takes the sub-factory thread in one iteration ( regardless of how many items it will actually make in an iteration)
            pthread_t thrdID;
            arg_t *arg = (arg_t *)malloc(sizeof(arg_t));
            if (arg == NULL) { 
                fprintf(stderr, "Failed to allocate memory\n"); 
                exit(EXIT_FAILURE); 
            } 
            arg->id = factory_num;
            arg->cap = factory_cap;
            arg->dur = factory_duration;

            pthread_mutex_lock(&factoryMutex);
            numActiveFactories++;
            pthread_mutex_unlock(&factoryMutex);

            Pthread_create( &thrdID, NULL, (void *) subFactory, (void *) arg);
            threadIDs[i] = thrdID;

            //create thread(subFactory(factory_num, factory_capacity, factory_duration));
            // subFactory( 1 , 50 , 350 ) ;  // Single factory, ID=1 , capacity=50, duration=350 ms
        }

        
        
        // ^ 
        /*
        Activates a single sub-factory, implemented as a regular function() call in factory.c , 
        providing it with its FactoryID ( 1 for this PA ), its capacity (say 50 items), and the duration 
        in milliseconds to make that capacity (or part of) of items.
        */


               // ***** FACTORY server summary report *************
    

    while ( 1 )
    {
        pthread_mutex_lock(   & factoryMutex  ) ;
        if ( numActiveFactories == 0 )
        {
            pthread_mutex_unlock( & factoryMutex  ) ;
            break ;
        }            ;
        pthread_mutex_unlock( & factoryMutex  ) ;
        fflush(stdout);
        Usleep( 1 ) ;   //  try again after 1 seconds
        
    }


// Sub factory Parts Made Iterations
    // 1             X       X
    // 2             X       X
    // 3             X       X
    // 4             X       X
    // =================================
    // Grand total parts made = X vs order size X
    // Order to completion time XXX.x
    

    printf("\n\n****** FACTORY Server Summary Report ******\n");
    printf("   Sub-Factory        Parts Made        Iterations\n");
    int total = 0;
    result_t* factoryResult;
    for(int i = 0; i < N; i++){
        void* result;
        Pthread_join(threadIDs[i], &result);

        // Cast result back to arg_t* and store in threadResults
        factoryResult = (result_t*)result;
        printf("           %2d              %3d                %2d\n", factoryResult->id, factoryResult->partsMade, factoryResult->iterations);
        total+=factoryResult->partsMade;
    }
    free(factoryResult);
    printf("============================================================\n");
    printf("Grand total parts made   =   %3d  vs order size of %3d\n", total, orderSize);
    gettimeofday(&end, NULL);
    long secs = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;

    if (micros < 0) {
        micros += 1000000;
        secs--;
    }

    double total_time = secs * 1000.0 + micros / 1000.0;
    printf("\nOrder-To-Completion time %.1f milliSeconds\n", total_time);

    fflush(stdout);

    }


    // free(threadIDs);
    free(threadResults);


    return 0 ;
}

void subFactory(void* arg )
{
    char    strBuff[ MAXSTR ] ;   // snprint buffer
    int     partsImade = 0 , myIterations = 0 ;
    msgBuf  msg;

    arg_t *p = (arg_t *) arg;
    int factoryID = p->id;
    int myCapacity = p->cap;
    int myDuration = p->dur;

    // Pthread_detach(  pthread_self() ) ;

    printf("Created Factory Thread #%3d with capacity =%4d parts & duration =%4d mSec\n", factoryID, myCapacity, myDuration);

    while ( 1 )
    {
        int make;
        // See if there are still any parts to manufacture
        pthread_mutex_lock(   & mutex  ) ;
        if ( remainsToMake <= 0 )
            break ;   // Not anymore, exit the loop

        make = minimum(remainsToMake, myCapacity);
        remainsToMake -= make;
        pthread_mutex_unlock(   & mutex  ) ;
        Usleep(myDuration*1000);// missing code goes here

        //After waking up, the sub-factory sends a  PRODUCTION_MSG message over UDP containing its 
        // FactoryID  , its capacity, the actual number of items it has just made in this iteration, 
        // and the duration in milliseconds it took to make that many items.
        // Send a Production Message to Supervisor
        msg.purpose = htonl(PRODUCTION_MSG);
        msg.facID = htonl(factoryID);
        msg.capacity = htonl(myCapacity);
        msg.partsMade = htonl(make);
        msg.duration = htonl(myDuration);

        if (sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, sizeof(clntSkt)) < 0)
            err_sys("sendto failed");
        printf("Factory #%3d: Going to make %4d parts in %3d mSec\n", factoryID, make, myDuration);


        // missing code goes here
        partsImade += make;
        myIterations += 1;


    }

    result_t* result = (result_t*)malloc(sizeof(result_t));
    result->id = factoryID;
    result->partsMade = partsImade;
    result->iterations = myIterations;
        pthread_mutex_unlock(   & mutex  ) ;
        // threadResults[factoryID-1].id = factoryID;
        // threadResults[factoryID-1].cap = partsImade;
        // threadResults[factoryID-1].dur = myIterations; //TODO: make another struct and put this in there instead of being lazy

    // Send a Completion Message to Supervisor
    msg.purpose = htonl(COMPLETION_MSG);
    msg.facID = htonl(factoryID);
    if (sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, sizeof(clntSkt)) < 0)
        err_sys("sendto failed");


    snprintf( strBuff , MAXSTR , ">>> Factory # %-3d: Terminating after making total of %-5d parts in %-4d iterations\n" 
          , factoryID, partsImade, myIterations);
    factLog( strBuff ) ;


    pthread_mutex_lock(   & factoryMutex  ) ;
    numActiveFactories--;
    pthread_mutex_unlock( & factoryMutex  ) ;

    free(p);
    pthread_exit((void*)result);
    // pthread_exit( NULL ) ;
    
}

