#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#define MSG_MAX_SIZE 500

void send_data();
void receive_data();
void create_socket();
void check_result();

struct sockaddr_in server_addr, client_addr;
char file_name[50] = "hi.txt";
int sock, total_bytes_received=0, port;
char request[100], response[MSG_MAX_SIZE+1], server_ip[30]="129.79.247.5";

int main(int argc, char* argv[])
{  
 if(argc != 4)
 {
  printf("Invalid number of arguments passed\n");
  exit(0);
 }
 strcpy(server_ip, argv[1]);
 port = atoi(argv[2]);
 strcpy(file_name, argv[3]);
 printf("Server IP Address %s\nPort %d\nFilename %s\n",server_ip, port, file_name);
 server_addr.sin_family = PF_INET;
 server_addr.sin_addr.s_addr = inet_addr(server_ip);
 server_addr.sin_port = htons(port);
 create_socket();
 send_data();
 receive_data();
 close(sock);
}

void send_data()
{
 int request_length;
 sprintf(request, "GET /%s HTTP/1.1\n\n", file_name);
 printf("the request is %s\n", request);
 sendto(sock, request, strlen(request), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

void receive_data()
{
 int length;
 int client_addr_length = sizeof(client_addr);
 int bytes_per_request;
 for(;;)
 {
  bytes_per_request = recvfrom(sock, response, MSG_MAX_SIZE,0, (struct sockaddr*)&client_addr, &client_addr_length);
  total_bytes_received = total_bytes_received + bytes_per_request;
  response[MSG_MAX_SIZE+1] = '\0';
  printf("%s\n", response);
  if((strlen(response)==0) || (strlen(response)<MSG_MAX_SIZE)){
   break;
  }
  //printf("bytes in this request %d\n total bytes received %d\n",bytes_per_request, total_bytes_received);
 }
}

void create_socket()
{
 sock = socket(PF_INET, SOCK_DGRAM, 0);
 check_result("socket creation", sock);
}


void check_result(char* msg, int result)
{
if(result<0)
 {
  printf("%s failed with value %d\n",msg, result);
  perror(msg);
  exit(1);
 }
 else
 {
  printf("%s successful with value %d\n", msg, result);
 }
}

