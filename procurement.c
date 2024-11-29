//---------------------------------------------------------------------
// Assignment : PA-04 Threads-UDP
// Date       : 12/3/2024
// Author     : Mason Scofield (scofi2ml@dukes.jmu.edu)
//              Zach Putz (putzzs@dukes.jmu.edu)
// File Name  : procurement.c
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

#define MAXFACTORIES    20

typedef struct sockaddr SA ;

typedef struct {
    int id;
    int partsMade;
    int iterations;
} result_t;

struct timeval start;
struct timeval end;

/*-------------------------------------------------------*/
int main( int argc , char *argv[] )
{
    int     numFactories ,      // Total Number of Factory Threads
            activeFactories ,   // How many are still alive and manufacturing parts
            iters[ MAXFACTORIES+1 ] = {0} ,  // num Iterations completed by each Factory
            partsMade[ MAXFACTORIES+1 ] = {0} , totalItems = 0;
    int iterations = 0;
    int totalnummade = 0;

    char  *myName = "Mason Scofield + Zach Putz" ; 
    printf("\nPROCUREMENT: Started. Developed by %s\n\n" , myName );    
    char myUserName[30] ;
    getlogin_r ( myUserName , 30 ) ;
    time_t  now;
    time( &now ) ;
    fprintf( stdout , "Logged in as user '%s' on %s\n\n" , myUserName ,  ctime( &now)  ) ;
    fflush( stdout ) ;
    
    if ( argc < 4 )
    {
        printf("PROCUREMENT Usage: %s  <order_size> <FactoryServerIP>  <port>\n" , argv[0] );
        exit( -1 ) ;  
    }

    printf("Attempting Factory server at \'%s\' : %s\n\n", argv [2], argv[3]);

    unsigned        orderSize  = atoi( argv[1] ) ;
    char	       *serverIP   = argv[2] ;
    unsigned short  port       = (unsigned short) atoi( argv[3] ) ;

    /* Set up local and remote sockets done*/

    int sd = socket( AF_INET, SOCK_DGRAM , 0 ) ;
    if (sd < 0)
		err_sys( "Could NOT create socket" ) ;


    // Prepare the server's socket address structure done
    struct sockaddr_in srvSkt ;	   /* Server's socket structrue  */
    memset( (void *) & srvSkt , 0 , sizeof( srvSkt ) );

    srvSkt.sin_family   = AF_INET;
    srvSkt.sin_port     = htons( port ) ;


    if( inet_pton( AF_INET, serverIP , (void *) & srvSkt.sin_addr.s_addr ) != 1 )
      err_sys( "Invalid server IP address" ) ;


    gettimeofday(&start, NULL);

    // Send the initial request to the Factory Server
    msgBuf  msg1;
    msg1.purpose = htonl(REQUEST_MSG);
    msg1.orderSize = htonl(orderSize);
    sendto(sd, &msg1, sizeof(msg1), 0, (SA*) &srvSkt, sizeof(srvSkt));
    

    printf("\nPROCUREMENT Sent this message to the FACTORY server: \n"  );
    printMsg( & msg1 );  puts("");


    /* Now, wait for oreder confirmation from the Factory server */
    msgBuf  msg2;
    printf ("\nPROCUREMENT is now waiting for order confirmation ...\n" );

    socklen_t alen2 = sizeof( srvSkt ) ;
    if (recvfrom ( sd, &msg2 , sizeof(msg2) , 0 , (SA *) & srvSkt , &alen2 ) <= 0){
        printf("Failed to recieve confirmation message from factory\n");
    }





    printf("PROCUREMENT received this from the FACTORY server: \n"  );
    printMsg( & msg2 );  puts("\n");



    numFactories = ntohl(msg2.numFac);
    
    result_t results[numFactories];
    for (int i = 0; i < numFactories; i++) {
        results[i].id = i;
        results[i].partsMade = 0;
        results[i].iterations = 0;
    }
    activeFactories = numFactories;

    msgBuf msg3;
    // Monitor all Active Factory Lines & Collect Production Reports
    while ( activeFactories > 0 ) // wait for messages from sub-factories
    {
    socklen_t alen3 = sizeof( srvSkt ) ;
        recvfrom(sd, &msg3 , sizeof(msg2) , 0 , (SA *) &srvSkt , &alen3);
        if ( ntohl(msg3.purpose) == PRODUCTION_MSG){
           iters[ntohl(msg3.facID)]++;

           totalItems += ntohl(msg3.partsMade);
           partsMade[1] += ntohl(msg3.partsMade);
           int numParts = ntohl(msg3.partsMade);
           int facnum = ntohl(msg3.facID);
           int dur = ntohl(msg3.duration);
           iterations++;
           totalnummade += ntohl(msg3.partsMade);
           results[facnum-1].iterations += 1;
           results[facnum-1].partsMade += numParts;
           printf("PROCUREMENT: Factory #%d produced %d parts in %d milliSecs\n", facnum, numParts, dur);
        } else if( ntohl(msg3.purpose) == COMPLETION_MSG){
            printf("PROCUREMENT: Factory #%d        COMPLETED its task\n", ntohl(msg3.facID));
            activeFactories--;
        } else if (ntohl(msg3.purpose) == PROTOCOL_ERR){
            printf("PROCUREMENT: Received invalid msg ");
            printMsg(&msg3); puts("");
            close(sd);
            exit(0);
        }
    }

    // Print the summary report
    //totalItems  = 0 ;
    printf("\n\n****** PROCUREMENT Summary Report ******\n");
    printf("   Sub-Factory        Parts Made        Iterations\n");
    for (int i = 0; i < numFactories; i++){
                printf("           %2d              %3d                %2d\n", results[i].id+1, results[i].partsMade,  results[i].iterations);
        // printf("Factory # %d made a total of %d parts in %d iterations\n", results[i].id+1, results[i].partsMade, results[i].iterations);
    }
    


    printf("==============================\n") ;


    printf("Grand total parts made =    %d   vs     order size of   %d \n", orderSize, totalnummade);
    gettimeofday(&end, NULL);
    long secs = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;

    if (micros < 0) {
        micros += 1000000;
        secs--;
    }

    double total_time = secs * 1000.0 + micros / 1000.0;
    printf("\nOrder-To-Completion time %.1f milliSeconds\n", total_time);


    printf( "\n>>> Procurement Terminated\n");



    // missing code goes here


    return 0 ;
}
