#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>    //close   
#include <sys/time.h>
#include <stdio.h>
#define MAX 255 
#define PORT 8000 // server port
#define MAX_CLIENT 12
char nothing[5];
char errorMsg[MAX];
char* DNSaddr;
char* DNSport;
char msg2[MAX];

/*Node in queue */
int client_socket[MAX_CLIENT];
fd_set readfds;
struct node
{
	struct node* nxt;
	char domain[MAX];
	char IP[MAX];
};

typedef struct node Qnode;
Qnode* Qfront;
Qnode* Qend;
int Qsize;

/* function to match 2 strings */
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

/*function to delete a front node in queue*/
void deletes()
{
	Qnode* tmp = Qfront;
	Qfront = Qfront->nxt;
	Qsize--;
	tmp->nxt = NULL;
	free(tmp);
}

/*function to insert a node in queue*/
void insert(char* dnsr,char* cliq) 					  			// cliq is request recieved from client and dnsr is answer from dnsserver after cachemiss
{
	Qnode* newnode = (Qnode *)malloc(sizeof(Qnode)); 			//creating a new node 
	newnode->nxt = NULL;
	if(cliq[0]=='1'){											//type == '1' then store dnsr in IP
		for(int i = 2; ;i++) {
			newnode->domain[i-2] = cliq[i];
			if(cliq[i] == '\0') break;
		}
		for(int i=2;;i++) {
			newnode->IP[i-2] = dnsr[i];
			if(dnsr[i] == '\0') break;
		}
	}else if(cliq[0] == '2') {									//type == '2' then store dnsr in domain
		for(int i = 2; ;i++) {
			newnode->IP[i-2] = cliq[i];
			if(cliq[i] == '\0') break;
		}
		for(int i=2;;i++) {
			newnode->domain[i-2] = dnsr[i];
			if(dnsr[i] == '\0') break;
		}

	}
	Qend->nxt = newnode;										//adding new node to queue
	Qend = newnode;
	Qsize++;
	if(Qsize > 3){
		deletes();												//delete front element from queue on cache overflow
	}
}

/*function to search for element in cache,*/
char* search_cache(char* query)
{
	
	Qnode* itr = Qfront;										//iterator over cache queue
	char s[MAX];

	for(int i=0;;i++){											//trimming the query till '\0'
		s[i] = query[i+2];
		if(query[i+2] == '\0') break;
	}
	
	int type = query[0] - '0';
	while(itr != NULL) {										//searching in cache
		if(type == 1) {											//type == '1' then return IP addr of corresponding domain
			if(matches(itr->domain , s)) {
				return itr->IP;
			}
		}else if(type == 2) {									//type == '2' then return domain of corresponding IP addr
			if(matches(itr->IP , s)) {
				return itr->domain;
			}
		}
		itr = itr->nxt;
	}

	return nothing;												//return nothing array if cache miss occured
}

/*function to interact with the dns server*/
char* DNS_query(char *req,int sockfd)
{
	int len=0;
	for(len=0;;len++){											//calculate length of request
		if(req[len]=='\0') break;
	}

	char msg[len + 1];											
	memset(msg,'0',sizeof(msg));
	for(int i=0;i<len;i++) {									//store relevent data in msg ignoring 0's after null character
		msg[i] = req[i];
	}

	msg[len] = '\0';
	if(msg[0]!='0')printf("Message sending PROXY --> %s\n", msg);
	write(sockfd, msg, sizeof(msg));							//writing in dnsserver buffer
	read(sockfd, msg2, sizeof(msg2));							//reading what dns server has sent
	printf("%s ... this is the answer please\n", msg2);
	close(sockfd);
	return msg2;
}

/*function to connect with dns server in cache miss*/
char* connectDNS(char* qry, int sockfd11)
{
	printf("query sent -->> %s\n", qry);
	int sockfd , conError;
	struct sockaddr_in DNSserveraddr , pserveraddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);					//socket creation for connection with dnsserver from proxyserver

	if(sockfd < 0) {
		printf("Unable to create socket in proxy\n"); exit(0);
	}else {
		printf("Socket successful in proxy\n");
	}

	/*intializing  the server variables */
	memset(&DNSserveraddr, '0', sizeof(DNSserveraddr));
	DNSserveraddr.sin_family = AF_INET;
	DNSserveraddr.sin_addr.s_addr = inet_addr(DNSaddr);
	int DNS_PORT=atoi(DNSport);
	DNSserveraddr.sin_port = htons(DNS_PORT);
	conError = connect(sockfd , (struct sockaddr*)&DNSserveraddr , sizeof(DNSserveraddr));

	
	if(conError == -1) {
		printf("unable to connect to server in proxy");
		exit(0);
	}
	else printf("connection to server established in proxy\n");
	printf("Enter request type first and then the address in proxy\n");

	printf("query sent -->> %s\n", qry);
	return DNS_query(qry , sockfd);
}


void proxyserver(int connectionfd, int sockfd, int index){

	char msg[MAX];
	int len=0;
	int n;
	
	memset(msg , '0' , MAX);
	read(connectionfd , msg , sizeof(msg)); //reading client request from buffer 
	if(msg[0] == '0'){						//closing connection when client sends 0
		close(connectionfd);
		client_socket[index] = 0;
		return;
	}

	int lenm=0;

	for(lenm=0;;lenm++) {					//calculating length of client data in buffer 
		if(msg[lenm] == '\0') break;
	}

	char qry[lenm+1];						//storing relevant data in qry
	for(int i=0;i<lenm;i++) qry[i] = msg[i];
	qry[lenm] = '\0';

	printf("query --> %s\n", qry);
	printf("The message that came :- %s\n", msg);

	char *resp = search_cache(msg);  		//searching the cache

	memset(msg , '0' , sizeof(msg));

	if(resp[0] != '*') {

		msg[0] = '3';						//appending response type for e.g. 3 or 4
		msg[1] = '$';
		for(int i=0;;i++) {
			if(resp[i] == '\0'){
				msg[i+2] = resp[i];
				break;
			}
			msg[i+2] = resp[i];
		}
	}
	else {
		printf("cache miss.... connecting to dns server\n");

		char *DNSmsg; 										// message recieved from dns server after cache miss

		DNSmsg = connectDNS(qry, sockfd);
		if(DNSmsg[0] == '3') insert(DNSmsg , qry);
		
		for(len=0;;len++) {
			if(DNSmsg[len]=='\0')break;
		}
		char msg2[len+1];
		for(int i=0;i<len;i++){
			msg2[i] = DNSmsg[i];
		}
		msg2[len]='\0';
		write(connectionfd , msg2 , sizeof(msg2)); 			// writing request to client buffer after cache miss
		return;
	}

	/*writing to client buffer in case of cache hit*/
	for(len=0;;len++) {
		if(msg[len]=='\0')break;
	}
	char msg2[len+1]; 										// msg is trimmed till \0 and put in msg2 
	for(int i=0;i<len;i++){
		msg2[i] = msg[i];
	}
	msg2[len]='\0';
	write(connectionfd , msg2 , sizeof(msg2)); 
	
}

int main(int argc, char* argv[]){
	int opt = 1;
	Qsize = 1;
	DNSaddr = argv[1];
	DNSport = argv[2];
	nothing[0] = '*';
	Qnode* tmp = (Qnode *)malloc(sizeof(Qnode));
	tmp->domain[0] = '\0';
	tmp->IP[0] = '\0';
	tmp->nxt = NULL;
	Qfront = tmp;
	Qend = tmp;
	for(int i=0;i<MAX_CLIENT;i++) client_socket[i] = 0;


	////////////////////////////////////////////// Client /////////////////////////////////////////////////////////

	int sockfd, connectionfd, clientAddrlen, max_sd, sd, activity, newSocket;
	struct sockaddr_in pserveraddr, clientaddr, dnsserveraddr;
	errorMsg[0] = '*';
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //// Creating a endpoint (MASTER SOCKET) for communication [Socket]

	if(sockfd < 0) {
		printf("Unable to create socket\n"); exit(0);
	}else {
		printf("Socket successful\n");
	}
	

	memset(&pserveraddr , '0' , sizeof (pserveraddr));

	/*initialising the proxyserver variables*/
	pserveraddr.sin_family = AF_INET;
	pserveraddr.sin_addr.s_addr = INADDR_ANY;
	pserveraddr.sin_port = htons(PORT);

	printf("%d\n", pserveraddr.sin_addr.s_addr);
	
	//bind the socket to localhost port 12000
	int bindError = bind(sockfd , (struct sockaddr*)&pserveraddr , sizeof(pserveraddr)); // Assigning address to the socket
	if(bindError == -1) {
		printf("Bind error occured\n");
		exit(0);
	}else printf("Binding successful\n");

 															
	if(listen(sockfd , MAX_CLIENT) == -1) {												 //Putting the socket in passive (listening) mode and specifying the maximum no.client processed
		printf("Failed to listen\n");
		exit(0);
	}else printf("Listening\n");


	//accept the  connection from clients
	clientAddrlen = sizeof(clientaddr);
	while(1) {
		
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket sockfd to set
		FD_SET(sockfd , &readfds);
		max_sd = sockfd;

		//add client sockets to set 
		for(int i = 0; i < MAX_CLIENT; i++) {

			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if(sd > 0) {

				FD_SET(sd , &readfds);
			}

			//highest file descriptor number  
			if(sd > max_sd) max_sd = sd;
		}

		//wating for activity in any of the active client socket
		activity = select(max_sd + 1 , &readfds , NULL, NULL, NULL);

		if(activity < 0 ) {
			printf("activity accept error\n");
			//exit(0);
			continue;
		}

		//active incoming client connection
		if(FD_ISSET(sockfd , &readfds)) {

			newSocket = accept(sockfd , (struct sockaddr *)&clientaddr , (socklen_t*)&clientAddrlen);
			if(newSocket == -1) {
				printf("Error in new socket creation\n");
				exit(0);
			}else printf("Connection established new socket\n");

			for(int i=0;i < MAX_CLIENT; i++) {

				if(client_socket[i] == 0) {
					client_socket[i] = newSocket;
					printf("Added this new socket at index %d\n", i);
					break;
				}
			}

		}

		//add new socket to array
		for(int i=0;i<MAX_CLIENT;i++) {

			sd = client_socket[i];

			if(FD_ISSET(sd , &readfds)) {

				proxyserver(sd,sockfd, i);

			}
		}


	}
	return 0;
}
