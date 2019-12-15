/* A simple echo server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <errno.h>
#include <dirent.h>

#define SERVER_TCP_PORT 3000 //well-known port
#define BUFLEN		256	//buffer length 

#define Err1 "PDU type not recognized"
#define Err2 "File not found"

//This is where fstat comes from! wahoo
struct stat filestat;

/*------------------------------------------------------------------------
 * Setting up the Struct for pdu
 *------------------------------------------------------------------------
 */
struct pdu
{
	char type;
	int length;//mad
	char data[150];	
}rpdu, tpdu;

int echod(int);
void reaper(int);

/*------------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------------
 */

//clear the pdu
void clearPDU(){
    //clear everything
    bzero((char *)&tpdu, sizeof(tpdu));
    bzero((char *)&rpdu, sizeof(rpdu));
}

//Delete File
void deleteUploadedFromClient(){
    //int remv = remove("fromServer.txt");
    int remv = unlink("uploadedFromClient.txt");
/*    fprintf(stdout, "\n%d DID REMOVE? ", remv);
    if (remv == 0) 
        printf("\nDeleted fromServer"); 
    else
        printf("\nUnable to delete fromServer"); 
*/
}

//Server ready
void serverReady(){
    //tpdu side send over we ready!
        tpdu.type = 'R';
        tpdu.length = 0;
        strcpy(tpdu.data, "");//everything empty for now
}

/*------------------------------------------------------------------------
 * main - TCP server for Project
 *------------------------------------------------------------------------
 */
int main(int argc, char **argv)
{
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;



	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %d [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

	while(1) {
	  client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client \n");
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
		(void) close(sd);
		exit(echod(new_sd));
	  default:		/* parent */
		(void) close(new_sd);
		break;
	  case -1:
		fprintf(stderr, "fork: error\n");
	  }
	}
}


//////
//echod program
int echod(int sd){
//Set up for file conversation using pdu types - copy from Lab5
	struct sockaddr_in fsin;	/* the from address of a client	*/
	char	*service = "3000";	/* service name or port number	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock,n;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type;        /* socket descriptor and socket type    */
	u_short	portbase = 0;	/* port base, for non-root servers	*/
//

    int insideSize;
    int temp;
    char toUpload[100]; 

//Actual program 
    while(1){
        //fprintf(stdout, "bro print\n");
        read(sd, &rpdu, 102);//read from the streaming socket, accept data from client
                            //102 - ???? why not 101 wtf?!?!?!?!
//D TYPE - CLIENT REQUEST TO DOWNLOAD     
        if(rpdu.type == 'D'){
            fprintf(stdout, "\nServer got request from client for the file: %s ", rpdu.data);
            fflush(stdout);
        //open file and read contents
            int openCFile = open(rpdu.data, O_RDONLY);

        //file not found
            if (openCFile < 0){
                //fprintf(stdout, "Server E ");
                //fflush(stdout);
                tpdu.type = 'E';
                tpdu.length = strlen(Err2);
            //size of error (stupid requirement)
                //fprintf(stdout, "Error Length = %d", tpdu.length);
                //fflush(stdout);
                strcpy(tpdu.data, Err2);

            //send off the error pdu
                write(sd, &tpdu, sizeof(tpdu));
                continue; 
            }

        //fstat: system call that is used to determine information about
            //a file based on its file descriptor.
            fstat(openCFile, &filestat);
        //Get inside size of file
            insideSize = filestat.st_size;
            //use st_size insted of int when dealing with object sizes
        
            tpdu.type = 'F';

        //Readning the file until end, then sending the data pdu
            while(1){
                //WHY 50 work!?!?! AMERICA EXPLAIN
                temp = read(openCFile, tpdu.data, 50);
                tpdu.length = temp;
                insideSize = insideSize - temp;
                if (insideSize == 0){
                //send the F final type since read everything    
                    write(sd, &tpdu, sizeof(tpdu));
                    break;//break freeee!
                }
                else{
                //tis a yikes then, send data
                    write(sd, &tpdu, sizeof(tpdu));    
                }

            }

        }

//U TYPE - CLIENT REQUEST TO UPLOAD
        else if(rpdu.type == 'U'){
            //deleteUploadedFromClient();
            fprintf(stdout, "\nServer got this to upload from client: %s", rpdu.data);
            fflush(stdout);
            strcpy(toUpload, rpdu.data);
        //tpdu side send over we ready!
            serverReady();    
            write(sd, &tpdu, sizeof(tpdu));
        }
//F TYPE - FILE DATA FROM CLIENT
        else if(rpdu.type == 'F'){
            int upFile = open("uploadedFromClient.txt", O_CREAT | O_RDWR, 0666);
            fprintf(stdout, "\nUPLOADED - Name: %s, Type: %c, Data: %s, Length: %d ", toUpload, rpdu.type, rpdu.data, rpdu.length);
            fflush(stdout);
            write(upFile, rpdu.data, strlen(rpdu.data));
            close(upFile);
        }


//P TYPE - CHANGE DIRECTORY FOR CLIENT
        else if(rpdu.type == 'P'){
            int chkDir = chdir(rpdu.data);
        //if error occured while changing directories    
            if(chkDir != 0){
                //how print error lmao
                //fprintf(stdout, "\nERROR no such direcory.");
                //fflush(stdout);
            //sending over the error pdu
                tpdu.type = 'E';
                tpdu.length = strlen("NO SUCH DIRECTORY FOUND");
                strcpy(tpdu.data, "NO SUCH DIRECTORY FOUND");
                write(sd, &tpdu, sizeof(tpdu));
            }
        //No error occured!    
            else{
            //Get the current directory (if worked then is should have changed)    
                char currPathDir[500];
                getcwd(currPathDir, sizeof(currPathDir));
                fprintf(stdout, "\nCurrent Directory: %s", currPathDir);
                fflush(stdout);

            //send back to client ready pdu
                //Requirement from project idk why we needed this > : ( )    
                serverReady();
                write(sd, &tpdu, sizeof(tpdu));
            }
        }

//L TYPE - LIST FILES IN CURRENT DIRECTORY FOR CLIENT
        else if(rpdu.type == 'L'){
            //int chkDir;
            DIR *d;
            struct dirent *dir;
            char output[120];


            d = opendir(rpdu.data);

            //memset used to fill a block of memory with a partiular value
                // in this case, output is the starting address, 
                //filled with 0, and the number of bytes to be filled
            memset(&output, 0, sizeof(output));

        //I TYPE - FILES LISTED!
            if(d != NULL){
                char temp[100];
            //fill buffer with file names    
                while((dir = readdir(d)) != NULL){
                    strcpy(temp, dir->d_name);
                    //strcat: returns a pointer to the resulting 
                        //string destination (1st param)
                    strcat(temp, ", ");
                    strcat(output, temp);
                }
                closedir(d);

                fprintf(stdout, "\nOUTPUT Variable inside: %s ", output);
                fflush(stdout);
                
            //Write to PDU type I and send off to client with file list
                tpdu.type = 'I';
                tpdu.length = strlen(output);
                strcpy(tpdu.data, output);
                //printf("tpdu.data = %s\n", tpdu.data);
                //fflush(stdout);
                write(sd, &tpdu, sizeof(tpdu));
            }
        //E TYPE - ERROR DIRECTORY DOES NOT EXIST    
            else{
            //Write to pdu and send off to client    
                tpdu.length = strlen("DIRECTORY DOES NOT EXIST");
                tpdu.type = 'E';
                strcpy(tpdu.data, "DIRECTORY DOES NOT EXIST");
                write(sd, &tpdu, sizeof(tpdu));
            }
        }

        clearPDU();

    }


	close(sd);

	return(0);
}

/////

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
