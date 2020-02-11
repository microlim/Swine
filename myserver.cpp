#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


void error( char *msg ) {
  perror(  msg );
  exit(1);
}

int func( int a ) {
   return 2 * a;
}

void sendData( int sockfd, int x ) {
  int n;

  char buffer[32];
  sprintf( buffer, "%d\n", x );
  if ( (n = write( sockfd, buffer, strlen(buffer) ) ) < 0 )
    error( const_cast<char *>( "ERROR writing to socket") );
  buffer[n] = '\0';
}

int getData( int sockfd ) {
  char buffer[32];
  int n;

  if ( (n = read(sockfd,buffer,31) ) < 0 )
    error( const_cast<char *>( "ERROR reading from socket") );
  buffer[n] = '\0';
  return atoi( buffer );
}

// json data 
#define MAX_JSONOBJ  10

char tokenptr[MAX_JSONOBJ][256];
int tokencnt=0;

void json_catdata(char *jsondata,int catidx)
{
	char *ptr;


	ptr = strtok(str, ",");

	


	if( ptr != NULL )
	{
		if( strchr(ptr,'}') != NULL )
		{
			memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
			
			tokencnt++;					
			
		} else {

			memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
			tokencnt++;

		}
	}

	while(ptr != NULL ){

		//printf( "%s\n" , ptr);

		ptr = strtok(NULL, ",");
			
		
		if( ptr != NULL )
		{
			if( strchr(ptr,'{') != NULL )
			{
				
				memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
				
				tokencnt++;					
				
			} else {

				memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
				tokencnt++;

			}
		}

	}

}

int main(int argc, char *argv[]) {
    
	printf( " main string test \n" );

      char str[] = "{\"test1\":\"1\",\"test2\":\"2\",\"test3\":{\"data1\":\"1\",\"data2\":\"2\"},\"test4\":\"4\"}";

        char *ptr;
		char *res;
		
        int tmp;

        printf("orignal : %s\n" , str) ;



        //ptr = strtok(str, ",");

        //printf("%s\n" , ptr);

		for( int i = 0 ; i < MAX_JSONOBJ ;i++) memset( tokenptr[i],256,0x00) ; 

        ptr = strtok(str, ",");

		


		if( ptr != NULL )
		{
			if( strchr(ptr,'{') != NULL )
			{
				memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
				
				tokencnt++;					
				
			} else {

				memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
				tokencnt++;

			}
		}

        while(ptr != NULL ){

			//printf( "%s\n" , ptr);

			ptr = strtok(NULL, ",");
				
			
			if( ptr != NULL )
			{
				if( strchr(ptr,'{') != NULL )
				{
					
					memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
					
					tokencnt++;					
					
				} else {

					memcpy( tokenptr[tokencnt],ptr,strlen(ptr) ) ; 
					tokencnt++;

				}
			}

        }


		for( int j=0; j < tokencnt ;j++	 )
		{
			printf("find --> %s \n" , tokenptr[j]);
		}

     




/*	 
	 int sockfd, newsockfd, portno = 5000, clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     int data;

     printf( "using port #%d\n", portno );
    
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
         error( const_cast<char *>("ERROR opening socket") );
     bzero((char *) &serv_addr, sizeof(serv_addr));

     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons( portno );
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
       error( const_cast<char *>( "ERROR on binding" ) );
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
  
     //--- infinite wait on a connection ---
     while ( 1 ) {
        printf( "waiting for new client...\n" );
        if ( ( newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen) ) < 0 )
            error( const_cast<char *>("ERROR on accept") );
        printf( "opened new communication with client\n" );
        while ( 1 ) {
             //---- wait for a number from client ---
             data = getData( newsockfd );
             printf( "got %d\n", data );
             if ( data < 0 ) 
                break;
                
             data = func( data );

             //--- send new data back --- 
             printf( "sending back %d\n", data );
             sendData( newsockfd, data );
        }
        close( newsockfd );

        //--- if -2 sent by client, we can quit ---
        if ( data == -2 )
          break;
     }

*/
     return 0; 
}
