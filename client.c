#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <string.h> 
#define MAX 255

void DNS_query(int type, char *req,int sockfd)
{
	int len=0;													// length of the message to send

	for(len=0;;len++){
		if(req[len]=='\0') break;
	}

	char msg[len+3];											// "Trimmed" message to be sent without any extra characters
	char msg2[MAX];												// for reading what the proxy server sent to us
	memset(msg,'0',sizeof(msg));

	msg[0] = '0' + type;										// Appending the type of the query to the message
	msg[1] = '$';												// for seperating the 'type' and the actual query we seperate them by '$'

	/*Trimming req*/
	for(int i=2;;i++) {
		int j=i-2;
		if(req[j] == '\0') {
			msg[i] = '\0'; break;
		}
		msg[i] = req[j];
	}

	if(msg[0]!='0')printf("Message sending --> %s\n", msg);

	write(sockfd, msg, sizeof(msg));							// writing trimmed message to the server
	
	read(sockfd, msg2, sizeof(msg2));							// reading message from the server into msg2

	if(msg[0] != '0') printf("%s : ", req);

	if(msg2[0] != '4'){
		if(msg[0] != '0') printf("%s , response type  = 3\n", msg2 + 2);
	}else if(msg2[0] == '4') {
		printf("No entry found in databse, response type = 4\n");
	}
	
	return;
}

int main(int argc, char const *argv[]){

	int sockfd , conError;

	if(argc != 3) {
		printf("Please enter in the correct format\n\n<./a.out> <IP_ADDR> <PORT>");
		exit(0);																		// if argument count is not equal to 3, exit
	}

	struct sockaddr_in serveraddr , clientaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);											// creating a connection end point (Socket)

	if(sockfd < 0) {
		printf("Unable to create socket\n"); exit(0);
	}else {
		printf("Socket successful\n");
	}

	memset(&serveraddr, '0', sizeof(serveraddr));

	/*Setting up server address variables*/
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	int PORT=atoi(argv[2]);
	serveraddr.sin_port = htons(PORT);

	conError = connect(sockfd , (struct sockaddr*)&serveraddr , sizeof(serveraddr));	// connecting socket to server

	if(conError == -1) {
		printf("unable to connect to server");
		exit(0);
	}else printf("connection to server established\n");


	printf("Enter request type first and then the address\n");

	/*Taking mulitple queries from the user*/
	while(1) {
		char req[MAX]; // requests
		memset(req,'0',sizeof req);
		int type;
		scanf(" %d", &type);
		if(type==0) {											// if type given by user is 0, this means user wants to close the connection
			DNS_query(0,req,sockfd);							// sending a request of type '0' to proxy server to let it know that client is offline
			break;
		}
		scanf("%s", req);
		DNS_query(type , req, sockfd);							// function for sending requests to the proxy server
	}
	close(sockfd);
	return 0;
}