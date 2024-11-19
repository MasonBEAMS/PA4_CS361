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

#include "wrappers.h"
#include "message.h"

#define MAXSTR     200
#define IPSTRLEN    50

typedef struct sockaddr SA ;

int minimum( int a , int b)
{
    return ( a <= b ? a : b ) ; 
}

void subFactory( int factoryID , int myCapacity , int myDuration ) ;

void factLog( char *str )
{
    printf( "%s" , str );
    fflush( stdout ) ;
}

/*-------------------------------------------------------*/

// Global Variable for Future Thread to Shared
int   remainsToMake , // Must be protected by a Mutex
      actuallyMade ;  // Actually manufactured items

int   numActiveFactories = 1 , orderSize ;
sem_t mutex;

int   sd ;      // Server socket descriptor
struct sockaddr_in  
             srvrSkt,       /* the address of this server   */
             clntSkt;       /* remote client's socket       */

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
    Sem_destroy( &mutex         ) ;
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
    Sem_init(&mutex, pshared, 1);

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

    printf("I will attempt to accept orders at port %d and use %d sub-factories.", port, N);

    
    if ( bind( sd, (SA *) & srvrSkt , sizeof(srvrSkt) ) < 0 )
    {
        printf("Could NOT bind to port %d", port );
    }
    inet_ntop( AF_INET, (void *) & srvrSkt.sin_addr.s_addr , ipStr , IPSTRLEN ) ;
    printf( "Bound socket %d to IP %s Port %d\n" , sd , ipStr , ntohs( srvrSkt.sin_port ) );



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

        // missing code goes here
        orderSize = ntohl(msg1.orderSize);
        Sem_wait(&mutex);
        remainsToMake = orderSize;
        Sem_post(&mutex);

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
       for(int i = 0; i < N; i++){
            int factory_num = i+1;
            //TODO:
            //int factory_cap = Maximum Capacity of items to make in one iteration (  10 <= Random <= 50 )
            //int factory_duration = The duration ( 500 <= Random <= 1200 MILLISECONDS ) it takes the sub-factory thread in one iteration ( regardless of how many items it will actually make in an iteration)
            // printf("Created factory thread # %d with capacity = %d parts & duration = %d mSec", factory id, capacity, duration);
            //create thread(subFactory(factory_num, factory_capacity, factory_duration));
            // subFactory( 1 , 50 , 350 ) ;  // Single factory, ID=1 , capacity=50, duration=350 ms
       }
        
        // ^ 
        /*
        Activates a single sub-factory, implemented as a regular function() call in factory.c , 
        providing it with its FactoryID ( 1 for this PA ), its capacity (say 50 items), and the duration 
        in milliseconds to make that capacity (or part of) of items.
        */
    }


    return 0 ;
}

void subFactory( int factoryID , int myCapacity , int myDuration )
{
    char    strBuff[ MAXSTR ] ;   // snprint buffer
    int     partsImade = 0 , myIterations = 0 ;
    msgBuf  msg;

    while ( 1 )
    {
        int make;
        // See if there are still any parts to manufacture
        if ( remainsToMake <= 0 )
            break ;   // Not anymore, exit the loop

        make = minimum(remainsToMake, myCapacity);
        remainsToMake -= make;
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
        printf("Factory #%d produced  %d parts in %d milliseconds\n", factoryID, make, myDuration);


        // missing code goes here
        partsImade += make;
        myIterations += 1;


    }

    // Send a Completion Message to Supervisor
    msg.purpose = htonl(COMPLETION_MSG);
    msg.facID = htonl(factoryID);
    if (sendto(sd, &msg, sizeof(msg), 0, (SA *)&clntSkt, sizeof(clntSkt)) < 0)
        err_sys("sendto failed");


    snprintf( strBuff , MAXSTR , ">>> Factory # %-3d: Terminating after making total of %-5d parts in %-4d iterations\n" 
          , factoryID, partsImade, myIterations);
    factLog( strBuff ) ;

    // ***** FACTORY server summary report *************
    // Sub factory Parts Made Iterations
    // 1             X       X
    // 2             X       X
    // 3             X       X
    // 4             X       X
    // =================================
    // Grand total parts made = X vs order size X
    // Order to completion time; X
    
    
}

