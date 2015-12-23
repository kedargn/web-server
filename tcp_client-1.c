#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/time.h>
#define MAX_RESPONSE_SIZE 6000

struct sockaddr_in server_address;
int sock;
char server_ip[30];/* =  "129.79.247.5"*/
 char fileName[20];// = "persistent.txt";
char fileContents[1000];
char* response;
char connection_type[2] = "p";
int port;
void check_args(int, char**);
void print_result(int, char*); 
void create_and_connect();
void request_receive_file();
void readFile();
void getContentLength(char*);

int content_length=0;

/**
 * Calls all other functions
 * @param Command line arguments in server_ip port connection_type filename order
 * @return 0
 **/
int main(int argc, char *argv[]){
 printf("Enter arguments in server_ip port connection_type file_name order\n");
 check_args(argc, argv);
 server_address.sin_family = AF_INET;
 server_address.sin_addr.s_addr = inet_addr(server_ip);
 server_address.sin_port = htons(port);
 create_and_connect();
 request_receive_file();
 return 0;
}

/**
 * Check command line arguments and stores it in corresponsding variables
 * @param: argc: no. of commad line arguments. argv: pointer to all arguments
 * @return: void
 **/
void check_args(int argc, char *argv[])
{
 int i=0;
 if(argc != 5)
 {
  printf("Invalid number of arguments passed\n");
  exit(0);
 }
 strcpy(server_ip, argv[1]);
 port = atoi(argv[2]);
 if((strcmp(argv[3], "np")==0) || (strcmp(argv[3], "p")==0))
 {
  strcpy(connection_type, argv[3]);  
 }
 else
 {
  printf("invalid connection type\n");
  exit(0);
 }
 strcpy(fileName, argv[4]);
 printf("Server IP Address %s\nPort %d\nConnection type %s\nFilename %s\n",server_ip, port, connection_type, fileName);
}


/**
 * Creates socket and connects to server
 **/
void create_and_connect()
{
 int connection_result;
 sock = socket(PF_INET, SOCK_STREAM, 0);
 print_result(sock,"create socket");
 if(sock<0)
  exit(0);
 connection_result = connect(sock, (struct sockaddr*)&server_address, sizeof(server_address));
 print_result(connection_result, "connect to server");
}

/**
 * Send and receives HTTP request and response respectively.
 * if connection type is non-persistent, then "Connection: close" is added to header and connection will be closed after response is received
 * if connection type is persistent, then "Connection: keep-alive" is added and connection is kept open until last file is received.
 * @param: void
 * @void: void
 **/
void request_receive_file()
{
 struct timeval request_time;
 struct timeval response_time;
 int request_sent_time, response_received_time;
 int request_result, received_bytes=0;
 char headers[100];
 char *tempFileContents, *requestFileName;
  if((strcmp(connection_type,"np")==0)) {
  sprintf(headers,"GET /%s HTTP/1.1\nConnection: %s\n\n",fileName, "close");
  printf("\nHTTP REQUEST\n%s\n\n",headers);
  gettimeofday(&request_time, NULL);
  request_result = send(sock, headers, strlen(headers), 0);
  received_bytes = 0;
  content_length = 0;
  do {
   response = (char*)calloc(MAX_RESPONSE_SIZE, sizeof(char));
   received_bytes += recv(sock, response, 6000,0);
   getContentLength(response);
   if(received_bytes > 0){
    printf("HTTP Response\n%s\n", response);
    free(response);
   }
  }while(content_length!=received_bytes);
  gettimeofday(&response_time, NULL);
  printf("\nSERVER RESPONSE TIME is %d seconds\n",response_time.tv_sec-request_time.tv_sec);
  close(sock);
 } else {
  readFile();
  tempFileContents = (char*)calloc(strlen(fileContents), sizeof(char));
  requestFileName = strtok(fileContents,"\n");
  while(requestFileName != NULL){
   sprintf(headers,"GET /%s HTTP/1.1\nConnection: keep-alive\n\n",requestFileName);
  // printf("persistent request header %s\n", headers);
   printf("\nHTTP REQUEST\n%s\n", headers);
   gettimeofday(&request_time, NULL);
   request_result = send(sock, headers, strlen(headers), 0);
   received_bytes = 0;
   content_length = 0;
   do {
    response=(char*)calloc(MAX_RESPONSE_SIZE, sizeof(char));
    received_bytes += recv(sock, response, 6000, 0);
    getContentLength(response);
   // received_bytes = (received_bytes/sizeof(char));
    printf("HTTP Response\n%s\n",response);
    free(response);
   }while((content_length != received_bytes));
   gettimeofday(&response_time, NULL);
   printf("\nSERVER RESPONSE TIME is %d seconds\n",(response_time.tv_sec-request_time.tv_sec));
   requestFileName = strtok(NULL, "\n");
  } 
  close(sock);
 }
}

void getContentLength(char* response)
{
 char* temp;
 char content_length_string[10];
 int a=0, b=0;
 temp = strstr(response, "Content-length:");
 if( temp!=NULL)
 {
  a=strlen("Content-length:");
  while((temp[a]!='\n')&&(temp[a]!='\r'))
  {
   content_length_string[b++] = temp[a++];
  }
 content_length_string[b] = '\0';
 content_length = atoi(content_length_string);
 temp = NULL;
 temp=strstr(response,"\n\n");
 content_length += (temp - response)+2;
 }
}
/**
 * Open and reads file which is requested by client in HTTP request
 * @param: void
 * @return: void
 **/
void readFile(){
 FILE *fp;
 int i=0;
 char ch;
 fp = fopen(fileName, "r");
 while((ch=getc(fp))!=EOF){
  fileContents[i++] = ch;
 }
 fclose(fp);
}

/**
 * Helper function to print helpful messages to stdout
 **/
void print_result(int value, char* message)
{
 printf("%s result is %d\n",message, value);
}
