#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<string.h>
#define MAX_REQUEST_SIZE 30000

void handleConnection(int);
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

struct request_headers request_header;                     //structures to maintain data of current request and response being served
char fileName[30], body[3000];                             //fileName in GET request and 
char headers[100];                                         //HTTP response headers
char *fileContents, *response;                             //file contents of requested file and response
int file_contents_size = 500, response_content_size = 500; //initial size of file contents and response
int is_request_valid = 0;
/**
 * Creates, binds, listens and accepts the connections in an endless loop.
 * @param: void
 * @return: 0
 **/
int main()
{
  int i, sock, bind_result, accept_result, client_addr_length, recv_msd_size;
  //int REQUEST_SIZE = 30;
  struct sockaddr_in server_addr, client_addr;

  sock = socket(PF_INET, SOCK_STREAM, 0);
  printf("socket creation result %d\n", sock);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(65000);
  bind_result = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  printf("the bind result is %d\n", bind_result);
  if(bind_result<0)
   exit(0);
  printf("the listen result is %d\n",listen(sock, 7));
  client_addr_length = sizeof(client_addr);
  for(i=0;;i++)
  { 
    printf("accepting new connections\n");
    accept_result = accept(sock,(struct sockaddr *)&client_addr, &client_addr_length);
    //printf("%d connection request, accept result is %d\n", i+1, accept);
    handleConnection(accept_result);
  }
  printf("socket close result %d\n", close(sock));
  return 0;
}

/** 
 * Handles receiving and sending data from/to client
 * @param: file descriptor of socket
 * @return: void
**/
void handleConnection(int sock) {
 int recv_msd_size, is_persistent = 1;
 char request[MAX_REQUEST_SIZE]; 
 //int REQUEST_SIZE = 30;
 do {
  printf("waiting to receive data from client\n");
  recv_msd_size = recv(sock, request, MAX_REQUEST_SIZE, 0);
  if(recv_msd_size > 0){
   printf("HTTP REQUEST\n\n%s\n", request);
   parseHeaders(request);
  }
   if((recv_msd_size <= 0)) {           //TODO:implement persistent and non persisten here
    printf("***CLOSING CONNECTION****\n");
    close(sock);
    break;
   } 
   is_request_valid = getFileName(request, recv_msd_size);
   replyToClient(sock); 
   }while((recv_msd_size > 0)); //TODO: Close if response is sent and connection is np 
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
    //printf("break condition %d & %c\n",i, request[i], fileName[i-4]);
    break;
  }
   else
    fileName[i-5] = request[i];
  }
  //printf("the length is %d  \n", i);
  fileName[i-5] = '\0';
  printf("the file name is %s\n",fileName);
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
 int fileFound = 0;  //TODO: Bad request

 fileFound = prepBody(fileName);
 prepHeaders(fileFound); 
 response = (char*)calloc(strlen(headers)+strlen(fileContents)+100, sizeof(char));
 strcat(response, headers);
 if(is_request_valid==0)
  strcat(response, fileContents);
 printf("HTTP RESPONSE\n\n%s\n",response);
 send(sock, response, strlen(response),0);
 if(!(isPersistent()) || ((isPersistent()) && (fileFound == -1))){
  //printf("CLOSING CONNECTIOn\n");
  close(sock);
 }
 free(fileContents);
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
  sprintf(headers, "HTTP/1.1 %d BAD REQUEST\nContent-length: 0\n\n", 400);
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
 * fileContents character array will dynamically increase based on the size of the file
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

/**
 * Parses the HTTP request for connection header and populates request header structure
 * @param: HTTP request
 * @return: void
 **/
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

/**
 * Tell whether HTTP request is persistent or non-persisten connection
 * @param void
 * @return: 1 -> Persistent connection, 0 -> Non persistent connection
 **/
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

