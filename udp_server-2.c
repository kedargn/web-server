#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<math.h>
#define MSG_MAX_SIZE 500                  //Max size of message sent in each HTTP response

void create_socket();
void bind_socket();
void check_result(char*, int);
int get_file_name();
void reply();
int read_file();
int get_file_name();
void send_response();
void prep_headers(int);

struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
int sock, port = 65000;
int client_addr_length;
char request[500];// = "GET /persistent.txt HTTP/1.1\nHost: sadsa.dsadsa.com\nConnection: alive\n\n";
char* response;
char file_name[50], headers[50];
int is_valid;                          //whether request is valid or not
char* file_contents;

/**
 * creates and binds the socket
 * @param void
 * @return 1 upon normal exit
 **/
int main()
{ 
 int received_bytes, file_found;
 server_addr.sin_family = AF_INET;
 server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 server_addr.sin_port = htons(port);
 client_addr_length = sizeof(client_addr); 
 printf("enter the PORT number\n");
 scanf("%d",&port);
 create_socket();
 bind_socket();
 printf("SOCKET RUNNING AT PORT %d\n", port);
 for(;;){
  printf("waiting to receive data from client \n");
  received_bytes = recvfrom(sock, request, MSG_MAX_SIZE,0, (struct sockaddr*)&client_addr, &client_addr_length);
  printf("the request is %s\n", request);
  get_file_name();
  printf("filename is %s\n",file_name);
  reply();
 }
 close(sock);
 exit(1);
}

/** reads the file and initiates HTTP response
 **/ 
void reply()
{
 int file_found;
 file_found = read_file();
 send_response(file_found);
}

/**
 * sends the HTTP response to client.
 * If the contents of file are greater than MSG_MSX_SIZE then content greater than MSG_MAX_SIZE will sent it next HTTP response
 * In other words each HTTP response will broken down into size of MSG_MAX_SIZE
 * @param file_found :- Interger which tells whether the requested file was found or not
 * @return: void
 **/
void send_response(int file_found)
{
 char EOF_delim[10]="";
 int client_addr1,file_length, sent_bytes = 0, i=1;
 client_addr1 = sizeof(client_addr);
 prep_headers(file_found);
 response = (char*)calloc(strlen(headers)+strlen(file_contents)+50, sizeof(char));
 strcat(response, headers);
 strcat(response, file_contents);
 file_length = strlen(response);
 printf("HTTP response\n\n %s\n", response);
 if(file_length<=MSG_MAX_SIZE){
  sendto(sock, response, MSG_MAX_SIZE, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
 }
 else {
  while(sent_bytes < file_length){
   sendto(sock, &response[sent_bytes], MSG_MAX_SIZE, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
   sent_bytes += MSG_MAX_SIZE;
  }
  if((file_length%MSG_MAX_SIZE)==0){
   sendto(sock, EOF_delim, MSG_MAX_SIZE, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
  }
 }
 free(response);
}

/**
 * Helper function to prep the HTTP response headers based on file_found and is_request_valid variables
 * @param file_found: Integer signifies whether requested file was found or not in server's directory. 0 if it is found. -1 if file not found.
 * @return: void
 **/
void prep_headers(int file_found)
{
 char str[50];
 if(is_valid == -1)
  strcpy(headers, "HTTP/1.1 400 BAD REQUEST\n\n");
 else if(file_found == 0)
  strcpy(headers, "HTTP/1.1 200 OK\n\n");
 else
  strcpy(headers,"HTTP/1.1 404 NOT FOUND\n\n");
}

/**
 * Helper function to read the requested file.
 * The file contents will be copied to file_contents variable. 
 * Based on the size of file, the file_contents variable size will be dynamically increased so that it never runs out of memeory
 **/
int read_file()
{
 FILE *fp;
 char ch;
 int i=0, file_contents_size;
 file_contents_size = 500;
 file_contents = (char*)calloc(file_contents_size, sizeof(char));
 fp = fopen(file_name, "r");
 if(fp == NULL){
  return -1;
 }
 while((ch=getc(fp))!=EOF)
 {
  file_contents[i++] = ch;
  if(i == (file_contents_size-5)){
    file_contents_size += file_contents_size;
    file_contents = realloc(file_contents, file_contents_size);
   }
 }
 file_contents[i] = '\0';
 return 0;
}

/**
 * Helper function to get filename requested in HTTP request
 * if HTTP request is bad then is_request_valid will equated to -1 value
 * else file_name variable will be equated with requested file name
 **/
int get_file_name()
{
 int i = 0;
 if((request[0] == 'G') && (request[1] == 'E') && (request[2] == 'T') && (request[3] == ' ') && (request[4] == '/'))
 {
  while(request[i+5] != ' ')
  {
   file_name[i] = request[i+5];
   i++;
  }
  file_name[i]='\0';
  is_valid = 0;
 }
 else {
  is_valid = -1;
 }
}

/**
 * creates socket
 **/
void create_socket()
{
 sock = socket(PF_INET, SOCK_DGRAM, 0);
 check_result("socket creation", sock);
}

/**
 * Binds socket.
 * If bind is unsuccesful then program will exit
 **/
void bind_socket()
{
 int bind_result;
 bind_result = bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
 check_result("Socket binding", bind_result);
}

/**
 * helper function to check results of socket creation and bind
 **/
void check_result(char* msg, int result)
{
if(result<0)
 {
  printf("%s failed with value %d\n",msg, result);
  perror(msg);
  exit(0);
 }
 else
 {
  printf("%s successful with value %d\n", msg, result);
 }
}
