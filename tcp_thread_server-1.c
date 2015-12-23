#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#define MAX_THREADS 100                       //max number of threads before we reuse pthreads array 
#define MAX_REQUEST_SIZE 3000

void *handleConnection(void*);
int getFileName(char*, int);
void replyToClient(int);
int prepBody();
void prepHeaders(int);
void readFile(FILE*);
void parseHeaders(char*);

struct request_headers
{
 char* connection;
 int isPersistent;
};

struct response_headers
{
 char* connection;
};

pthread_t threads[MAX_THREADS];                          //array of pthreads
struct request_headers request_header;
char fileName[30], body[3000];
char headers[100];
char *fileContents, *response;
int file_contents_size = 500;                             //intial file contents size
int is_request_valid = 0, port =65000;                    // request vaild or invalid, port number of server

/**
 * Creates, binds, listens and accepts and creates new threads to handle send and receive data.
 * @param: void
 * @return:  0 if exits normally
 **/
int main()
{
  int i, sock, bind_result, client_addr_length, recv_msd_size, t;
  int REQUEST_SIZE = 30, accept_connection;
  struct sockaddr_in server_addr, client_addr;
  int *accept_result;

  sock = socket(PF_INET, SOCK_STREAM, 0);
  printf("socket creation result %d\n", sock);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  bind_result = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  printf("the bind result is %d\n", bind_result);
  if(bind_result<0)
   exit(0);
  printf("the listen result is %d\n",listen(sock, 7));
  client_addr_length = sizeof(client_addr);
  printf("SERVER IS RUNNING AT PORT %d\n",port);
  for(i=0;;i++)
  { 
    printf("accepting new connections\n");
    accept_connection = accept(sock,(struct sockaddr *)&client_addr, &client_addr_length);
    accept_result = &accept_connection;
    if(i<MAX_THREADS) {
     t = pthread_create(&threads[i], NULL, handleConnection, (void*)accept_result);
   }else{
     i=0;
     t = pthread_create(&threads[i], NULL, handleConnection, (void*)accept_result);
   } 
  }
  printf("socket close result %d\n", close(sock));
  return 0;
}

/** 
 * Handles receiving and sending data from/to client
 * @param: file descriptor of socket
 * @return: void
**/
void *handleConnection(void *accept_result) {
 int sock = *((int*)accept_result);
 int recv_msd_size, is_persistent = 1;
 char request[MAX_REQUEST_SIZE]; 

 printf("This request is being handled by thread ID %lu\n",pthread_self());
 do {
  printf("waiting to receive data from client\n");
  recv_msd_size = recv(sock, request, MAX_REQUEST_SIZE, 0);
  if(recv_msd_size > 0)
   printf("\nHTTP REQUEST\n%s\n", request);
  parseHeaders(request);
  if((recv_msd_size <= 0)) {      
   printf("***CLOSING CONNECTION****\n");
   close(sock);
   break;
  } 
  is_request_valid = getFileName(request, recv_msd_size);
  replyToClient(sock);
  }while((recv_msd_size > 0) && (isPersistent())); 
}

/**
 * Parses the HTTP request and gets the requested file name
 * Only GET requests are supported
 * @param: http request contents and size
 * @return: if filename is found 0 else -1
**/
int getFileName(char* request, int size)
{
 int i=0, fileNameLength;
 fileName[0]= '\0';
 if((request[0] == 'G') && (request[1] == 'E') && (request[2] == 'T') && (request[3] == ' ') && (request[4] == '/')) {
  for(i=5;i<size;i++){
 // printf("looping %d & %c & %c\n",i, request[i], fileName[i-4]);
   if(request[i] == ' '){ 
   // printf("break condition %d & %c\n",i, request[i], fileName[i-4]);
    break;
  }
   else
    fileName[i-5] = request[i];
  }
  //printf("the length is %d  \n", i);
  fileName[i-5] = '\0';
  //printf("the file name is %s\n",fileName);
  return 0;
 }
 else {
 return -1;
 }
}

/**
 * Responds to client
 * @param: socket file descriptor
 * @return: void
**/ 
void replyToClient(int sock)
{
 char *nameOfFile, *tempFileContents;
 char* fileInList = NULL;
 int fileFound = 0;

 fileFound = prepBody(fileName);
 prepHeaders(fileFound);
 response = (char*)calloc((strlen(headers)+strlen(fileContents)+100), sizeof(char));
 response[0]='\0';
 strcat(response, headers);
 if(is_request_valid != -1){
  strcat(response, fileContents);
 }
 printf("HTTP RESPONSE\n\n%s\n",response);
 send(sock, response, strlen(response),0);
 if(!(isPersistent()) || ((isPersistent()) && (fileFound == -1))){
  printf("CLOSING CONNECTIOn\n");
  close(sock);
 }
 free(fileContents);
 //printf("response will be\n%s\n",response);
 free(response);
}

/** Prepares the HTTP response headers
 * @param: An Integer. 0 means file was found. -1 means file wasn't found
 * @return: void
 **/
void prepHeaders(int fileFound){
 int content_length;
 content_length = (((strlen(fileContents))*(((sizeof(char))*8)/8)));
 if(is_request_valid == -1){
  sprintf(headers, "HTTP/1.1 %d BAD REQUEST\n",400);
  return;
 }
 if(fileFound == 0) {
  sprintf(headers, "HTTP/1.1 200 OK\nConnection: %s\nContent-length: %d\n\n",request_header.connection, content_length) ;
 }
 else {
 sprintf(headers, "HTTP/1.1 404 Not Found\nConnection: %s\nContent-length: %d\n\n", request_header.connection, content_length);
 }
}

/**
 * Prepares HTTP response body. If file name is not found in server file system, then not_found.html http file 
 * inserted in HTTP response body.
 * @param: void
 * @return: An integer. if file found then 0 else -1.
 **/
int prepBody(char* fileName)
{
 FILE *fp;
 fp = fopen(fileName, "r");
 if(fp == NULL){
  fp = fopen("not_found.html", "r");
  readFile(fp);
  return -1;
 }
 readFile(fp);
 return 0;
} 


/**
 * Reads the files contents and populates fileContents global variable
 * fileContents character array is dynamically increased based on the size of the file contents.
 * @param: file pointer
 * @return: void
 **/
void readFile(FILE *fp){
 char ch;
 int i=0, temp_file_size;
 fileContents = (char*)calloc(file_contents_size,sizeof(char));
 temp_file_size = file_contents_size;
 while((ch = getc(fp))!= EOF){
  fileContents[i] = ch;
  i++;
  if(i == temp_file_size-1){
   temp_file_size = temp_file_size + 4;
   fileContents = realloc(fileContents, temp_file_size*sizeof(char));
  }
 }
 fileContents[i] = '\0';
}

void parseHeaders(char* request)
{
 int a=0,b=0;
 char* header;
 request_header.connection = header = NULL;
 header = strstr(request, "Connection:");
 if(header != NULL){
  b = strlen("Connection:");
  while((header[b] != '\n') && (header[b] != '\r'))
  {
   if(header[b] != ' ')
   {
    header[a++] = header[b];
   }
   b++;
  }
  header[a] = '\0';
  request_header.connection = header;
 }
 else {
  request_header.connection = "close";
 }
  //printf("\nconnection header is %s\n", request_header.connection);
}

int isPersistent(){
 if(strcmp(request_header.connection,"close") == 0){
  //printf("NON-PERSISTENT REQUEST\n");
  return 0;
 }
 else if(strcmp(request_header.connection,"keep-alive") == 0) {
  //printf("PERSISTENT CONNECTION\n");
  return 1;
 }
}

