#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#define PORT 1123

//function client vers joueur
char* joueur_request(char *hostname)
{

  char request[256]="GET";
  char* response=NULL;
  
  struct sockaddr_in sin;
  struct hostent *hostinfo;

  sin.sin_port = htons(PORT);
  hostinfo = gethostbyname(hostname);
  sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  sin.sin_family = AF_INET;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  connect(sock, (struct sockaddr*)&sin, sizeof(sin));
  printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

  response = malloc(256*sizeof(char));

  send(sock, request, strlen(request),0);

  recv(sock, response, sizeof(request), 0);

  close(sock);

  return response;
}

int sendMonitoring(char* rfid, char* idball) {
    
    int sockD, verif;
    struct sockaddr_in sin;
    struct hostent *hostinfo;
    
    /* Configuration de la connexion */
    sin.sin_port = htons(80);
    hostinfo = gethostbyname("www.dweet.io");
    
    sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    sin.sin_family = AF_INET;
    
    
    /* Creation de la socket */
    sockD = socket(AF_INET, SOCK_STREAM, 0);
    
    /* Tentative de connexion au serveur */
    connect(sockD, (struct sockaddr*)&sin, sizeof(sin));
    printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));
    
    /* Creation de la requête */
    char request[256];
    sprintf   (request, "GET /dweet/for/robot16?rfid=%s&idball=%sHTTP/1.1\r\nHost:%s\r\n\r\n", rfid, idball, "www.dweet.io");
    printf("REQUETE : \n%s\n",request);
    printf("Valeur de rfid : %s\n",rfid);
    printf("Valeur de idball : %s\n\n",idball);
    /* Envoie du message au serveur */
    verif = send(sockD, request, strlen(request),0);
    close(sockD);
    return 1;
}

int main(void){

  char hostname[256] = "localhost";

  char* idball = joueur_request(hostname);
  printf("joueur vaut : %s\n", idball);
  sendMonitoring(hostname, idball);
  
  
}

