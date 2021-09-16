#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <stdio.h>
#define MAX 255 
#define PORT 12000 // DNS server port

char domain[MAX];
char IPaddr[MAX];
char errorMsg[MAX];
 
//Utility function for Matching two strings

int matches(char* a, char* b)
{
	int ok=1;
	for(int i=0;;i++){
		if(a[i] != b[i]) {
			ok=0; break;
		}
		if(a[i] == '\0') break;
	}
	return ok;
}

// Utility function for searching database for a domain or an IP

char* process_request(char *msg)
{
	int len = 0;
	for(int i=0;;i++) {                              //Calculating length of msg(query)
		if(msg[i] == '\0') break;
		len++;
	}
	int type = msg[0] - '0';                         //Finding type of query
	char arr[MAX];
	for(int i=2;i<len;i++) {
		arr[i-2] = msg[i];
	}
	arr[len-2] = '\0';
	FILE *datafile = fopen("Database.txt", "r");
	if(!datafile) {
		printf("file cant be opened\n"); exit(0);
	}
	char entry[MAX];
	while(fgets(entry , MAX , datafile) != NULL) {   // Reading from database 

		int j=0;
		memset(domain , '0' , sizeof domain);
		memset(IPaddr , '0' , sizeof IPaddr);
		for(int i=0;;i++) {
			if(entry[i] == ' ') {
				domain[i] = '\0';
				j = i+1; break;
			}
			domain[i] = entry[i];                    // Storing domain name of each entry  
		}
		for(int i=j;;i++) {
			if(entry[i] == '\n'){
				IPaddr[i-j] = '\0';
				break;
			}

			IPaddr[i-j] = entry[i];                   // Storing IP of each entry
		}
		if(type == 1) {                               // If the query is of type 1, and query matches with domain returning the corresponding IP
			if(matches(domain , arr)) {
				return IPaddr;
			}
		}else if(type == 2) {                         // If the query is of type 2, and query matches with IP returning the corresponding domain
			if(matches(arr , IPaddr)) {
				return domain;
			}
		}
	}
	return errorMsg;

}

//Utility function for reading and writing data in socket 

void DNS_response(int connectionfd)
{
	char msg[MAX];
	int len=0;
	int n;
	while(1) {
		memset(msg , '0' , MAX);
		read(connectionfd , msg , sizeof(msg));
		printf("%s\n", msg);               // Reading input from the client
		if(msg[0] == '0'){                                    //If message is 0 it means client wants to close connection
			return;
		}
		printf("The message that came :- %s\n", msg);
		char *resp = process_request(msg);                    // Storing the response of the query in resp
		memset(msg , '0' , sizeof(msg));
		if(resp[0] != '*') {                                  // If answer to the query is found in database  

			msg[0] = '3';
			msg[1] = '$';
			for(int i=0;;i++) {
				if(resp[i] == '\0'){
					msg[i+2] = resp[i];
					break;
				}
				msg[i+2] = resp[i];
			}
		}else {                                              // If answer to query is not found in database return error message 4

			msg[0] = '4';
			msg[1] = '\0';
		}
		for(len=0;;len++){                                   //Calculating length of response msg
			if(msg[len]=='\0')break;
		}
		char msg2[len+1];                                    // Trimming the response and storing it in msg2
		for(int i=0;i<len;i++){
			msg2[i] = msg[i];
		}
		msg2[len]='\0';
		write(connectionfd , msg2 , sizeof(msg2));	     // Sending back response to the client
	}
}


int main()
{

	int sockfd, connectionfd, clientAddrlen;
	struct sockaddr_in serveraddr, clientaddr;
	errorMsg[0] = '*';

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creating a endpoint for communication [Socket]

	if(sockfd < 0) {
		printf("Unable to create socket\n"); exit(0);
	}else {
		printf("Socket successful\n");
	}
	memset(&serveraddr , '0' , sizeof (serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY; 
	serveraddr.sin_port = htons(PORT);

	printf("%d\n", serveraddr.sin_addr.s_addr);
	
	int bindError = bind(sockfd , (struct sockaddr*)&serveraddr , sizeof(serveraddr)); // Assigning address to the socket

	if(bindError == -1) {
		printf("Bind error occured\n");
		exit(0);
	}else printf("Binding successful\n");

	if(listen(sockfd , 10) == -1) {            //Putting the socket in passive (listening) mode
		printf("Failed to listen\n");
		exit(0);
	}else printf("Listening\n");

	clientAddrlen = sizeof(clientaddr);

	//Loop for accepting multiple connections
	while(1) {
		connectionfd = accept(sockfd , (struct sockaddr*)&clientaddr , &clientAddrlen); // Accepting connections on the socket

		if(connectionfd == -1) {
			printf("cant accept\n");
			exit(0);
		}else printf("accepted\n");

		DNS_response(connectionfd);      // Function for serving a query
		close(connectionfd);             // Closing the connection 
		printf("Waiting for new connection\n");
	}
	return 0;
}
