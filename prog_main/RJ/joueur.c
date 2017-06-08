// tcp_serveur.c

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
#define TAILLE 256

int main(void) {

  pid_t pid;
  int sock, csock;

  char buffer[TAILLE];
  char error[TAILLE] = "404";
  char path[TAILLE] = "public/idball";

  FILE* fichier = NULL;

  struct sockaddr_in sin;
  struct sockaddr_in csin;

  socklen_t recsize = sizeof(sin);
  socklen_t crecsize = sizeof(csin);

  sock = socket(AF_INET, SOCK_STREAM, 0);

  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);
  bind(sock, (struct sockaddr*)&sin, recsize); 


  listen(sock, 1);

  printf("Serveur %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));
 

  while(1){

    csock = accept(sock, (struct sockaddr*)&csin, &crecsize);

    printf("Un client se connecte avec la socket %d de %s:%d\n", csock, inet_ntoa(csin.sin_addr), htons(csin.sin_port));

    pid = fork();

    if(pid == 0){

      int n = recv(csock, buffer, TAILLE-1 ,0);
      buffer[n] = '\0';

      printf("Message du client n°%d: %s\n", csock, buffer);

      if(strcmp(buffer, "GET") == 0){

        fichier = fopen(path, "r");

        if (fichier != NULL){

          fgets(buffer, TAILLE, fichier);
            printf("valeur de ballon : %s\n", buffer);
          send(csock, buffer, strlen(buffer)-1, 0);
          fclose(fichier);

        }else{

          send(csock, error, TAILLE, 0);

        }

      }else{  

        send(csock, error, TAILLE, 0);

      }

      shutdown(csock,2);
      close(csock);
      exit(0);
    }//if pid

  }//while

close(sock);

return EXIT_SUCCESS;

}//main
