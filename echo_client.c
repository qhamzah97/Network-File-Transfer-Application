#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>                                                                      
#include <netinet/in.h>
#include <arpa/inet.h>                                                                          
#include <netdb.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <errno.h>
#include <dirent.h>

#define	BUFSIZE 64
#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		256	/* buffer length */

    struct  stat filestat;

/*------------------------------------------------------------------------
 * Setting up the Struct for pdu
 *------------------------------------------------------------------------
 */
struct pdu{
    char type;
    int length;//mad
    char data[100];
};

    struct pdu rpdu, tpdu;

/*------------------------------------------------------------------------
 * Variables
 *------------------------------------------------------------------------
 */
	int 	n, n2, bytes_to_read, z, tempsize;
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;

	char	*host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], buf[BUFLEN], userInp[2];

/*------------------------------------------------------------------------
 * Functions
 *------------------------------------------------------------------------
 */

//Delete File
void deleteFromServer(){
    //int remv = remove("fromServer.txt");
    int remv = unlink("fromServer.txt");
    //int remv = unlink("uploadedFromClient.txt");
    //fprintf(stdout, "\n%d DID REMOVE? ", remv);
/*    if (remv == 0) 
        printf("\nDeleted fromServer"); 
    else
        printf("\nUnable to delete fromServer"); 
*/
}

//delete uploaded file
void deleteUploadedFromClient(){
    int remv = unlink("uploadedFromClient.txt");
}

//clear the pdu
void clearPDU(){
    //clear everything
    bzero((char *)&tpdu, sizeof(tpdu));
    bzero((char *)&rpdu, sizeof(rpdu));
    bzero((char *)&buf, sizeof(buf));
}

/*------------------------------------------------------------------------
 * main - TCP client for Project
 *------------------------------------------------------------------------
 */
int main(int argc, char **argv){
//////taken from lab 4   
    switch(argc){
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host)) 
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}
//////



//Actual Program start
    while(1){
    //Clear the pdu
        clearPDU();  
    //Get User input    
        //fprintf(stdout, "\nWhat you want, numbers only: \n");
        fprintf(stdout, "\n--------------------------------------");
        fprintf(stdout, "\n|      Welcome to the Main Menu      |");
        fprintf(stdout, "\n|        Here are the options;       |");
        fprintf(stdout, "\n|\t1 - File Download            |");
        fprintf(stdout, "\n|\t2 - File Upload              |");
        fprintf(stdout, "\n|\t3 - Change Directory         |");
        fprintf(stdout, "\n|\t4 - List Files in a Directory|");
        fprintf(stdout, "\n| Please input a number from one of  |");
        fprintf(stdout, "\n|   the above options!               |");
        fprintf(stdout, "\n--------------------------------------\n");
        scanf("\n%s", userInp);
        //fprintf(stdout, "userinput was: %s", userInp);

//1 TYPE - FILE DOWNLOAD
    if(userInp[0] == '1'){
        deleteFromServer(); 
        char tempName[100];
        fprintf(stdout, "\nFile name lets go: ");
        scanf("%s", &buf);
    //store file name into tpdu data and tmep variable
        strcpy(tpdu.data, buf);
        strcpy(tempName, buf);//added, 
    //set up the rest of the tpdu    
        tpdu.type = 'D';
        tpdu.length = strlen(tpdu.data);
    //send filename to server and read server response
        write(sd, &tpdu, sizeof(tpdu));
        read(sd, &rpdu, sizeof(rpdu));
    //read response from sever
        //n = read(sd, &rpdu, 102);
    //SERVER RESPONSE TYPE E - ERROR
        if(rpdu.type == 'E'){
            fprintf(stdout, "ERROR: %s", rpdu.data);
            continue;
        }
    //SERVER RESPONSE TYPE F - WRITE TO FILE
        else if(rpdu.type == 'F'){
            int writeFile = open("fromServer.txt", O_CREAT | O_RDWR, 0666);
            write(writeFile, rpdu.data, strlen(rpdu.data));
            close(writeFile);
            //continue;
        }
        fprintf(stdout, "\nClient got this from server: %s", rpdu.data);
    }

//2 TYPE - FILE UPLOAD
    else if (userInp[0] == '2'){
        deleteUploadedFromClient();
        fprintf(stdout, "\nEnter upload file nme plz: ");
        scanf("%s", &buf);

    //fill in the tpdu with user response
        strcpy(tpdu.data, buf);
        tpdu.type = 'U';
        tpdu.length = strlen(tpdu.data);

    //send to server and read the response
        write(sd, &tpdu, sizeof(tpdu));
        read(sd, &rpdu, sizeof(rpdu));

    //SERVER RESPONSE TYPE R - SERVER READY  
        if(rpdu.type == 'R'){    
            fprintf(stdout, "\nSERVER READY UPLOADING NOW!");
            int writeUpload = open(buf, O_RDONLY);

            //if file upload does exist..
            if(writeUpload >= 0){
                //clearing, idk why tbh lmao worked like this
                bzero((char *)&tpdu, sizeof(tpdu));
                fstat(writeUpload, &filestat);
                tempsize = filestat.st_size;//idk if i need
                tpdu.type = 'F';

            //place data to file contents and send over the data pdu
                read(writeUpload, tpdu.data, 100);
                tpdu.length = strlen(tpdu.data);
                write(sd, &tpdu, sizeof(tpdu));
            }
            //file upload does not EXIST
            else if(writeUpload < 0){
                fprintf(stdout, "\nFile does not exist");
                //continue;
                clearPDU();
            }
        }   
    }

//3 TYPE - CHANGE DIRECTORY
    else if (userInp[0] == '3'){
        fprintf(stdout, "\nEneter Directory path: ");
        fflush(stdout);
        scanf("%s", &buf);

    //fill in tpdu with user information
        strcpy(tpdu.data, buf);
        tpdu.length = strlen(tpdu.data);
        tpdu.type = 'P';

    //Send over pdu to server and read server response
        write(sd, &tpdu, sizeof(tpdu));
        read(sd, &rpdu, sizeof(rpdu));

    //if server send E type - ERROR
        if(rpdu.type == 'E'){
            fprintf(stdout, "\nERROR MAN: %s", rpdu.data);
        }
    //if server send R type - READY
        else if(rpdu.type == 'R'){
            fprintf(stdout, "\nSERVER CHANGED DIRECTORY! WOW ");
        }
    }

//4 TYPE - LIST FILES IN DIRECTORY
     else if (userInp[0] == '4'){
         fprintf(stdout, "\nDirecotry you want to see contents inside: ");
         scanf("%s", &buf);

    //prepare the L type pdu
        tpdu.type = 'L';
        strcpy(tpdu.data, buf);
        tpdu.length = strlen(tpdu.data);

    //Send pdu to server and read response form server
        write(sd, &tpdu, sizeof(tpdu));
        read(sd, &rpdu, sizeof(rpdu));

        //printf("\n%s", rpdu.data);
        //fflush(stdout);

    //if server sends E - ERROR
        if(rpdu.type == 'E'){
            fprintf(stdout, "\nERROR INSIDE PDU DATA: %s", rpdu.data);
            fflush(stdout);
        }
    //if server sends I - LIST OF FILE NAMES
        else if (rpdu.type == 'I'){
            fprintf(stdout, "\nList of files: \n%s ", rpdu.data);
            fflush(stdout);
        }
     }
     else{
        fprintf(stdout, "\nInvalid input try again!");
     }
    //
    //getchar();

    clearPDU();

    }

	close(sd);
	return(0);

}