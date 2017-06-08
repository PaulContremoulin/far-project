#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/stat.h>
#include <time.h>

#define PORT 10000

typedef struct Joueur
{
char rfid[15];
char idRobot[20];
char coulRobot[15];
} Joueur;

Joueur listeJoueurs[2];

void error(const char *msg) { perror(msg); exit(0); }
int main(void) {
  getDonneesRobots();
}


/* Lancer en début de partie
 * Récupère les ips et numeros RFID des robots et les places dans un tableau à 2 dimensions
 */
int getDonneesRobots() {
  int sock;
  char sendbuffer[4096] = "", recvbuffer[8192] = "";
  /* Saisie des infos de connexion */
  char *host="api.beebotte.com";
  char file[100]="/v1/public/data/read/vberry/testVB/msg?";
  int port=80;
  char ipHost[100] = {0};


  /* Calcul de l'IP de l'hote */
  struct hostent *hostinfo;
  hostinfo = gethostbyname(host);
  if (hostinfo) {
    struct in_addr  **pptr;
    pptr = (struct in_addr **)hostinfo->h_addr_list;

    strcpy(ipHost, inet_ntoa(**(pptr)));
    printf("Adresse IP de l'hote : %s\n",ipHost);
  }




	/* Creation de la socket */
  		if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) <0 )
    	error("can't create socket");

    	/* Configuration de la connexion */
  		struct sockaddr_in sin;

  		sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  		sin.sin_family = AF_INET;
  		if ( (sin.sin_port = htons(port)) == 0)
    		error("unknown service");

  			/* Tentative de connexion au serveur */
  			if (connect(sock, (struct sockaddr*)&sin,sizeof(sin))<0)
    		error("can't connect");
  			else printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr),
         		htons(sin.sin_port));

  		/* Envoi de donnees au serveur */
      strcpy(sendbuffer, "");
      strcat(sendbuffer, "GET /v1/public/data/read/vberry/testVB/msg HTTP/1.0\r\n");
      strcat(sendbuffer, "Host: api.beebotte.com\r\n\r\n");
      send(sock, sendbuffer, strlen(sendbuffer), 0);
      printf("\nRequest sent (sending datas) :\n%s", sendbuffer);

      /* Réception de la réponse */
      strcpy(recvbuffer, "");
      recv(sock, recvbuffer, 8192, 0);
      printf("\nDatas received :\n%s", recvbuffer);

      /********
      ** Récupération des messages postés un par un qui seront placés dans un tableau 'datas'
      *********/

      char * msg;
      char  msgTemp[300];
      int cpt = 0,
      j = 0;
      char* datas[50];
      char * ligneData[50]; // la ligne du msg qui nous intéresse

      // On initialise le tableau data avec des valeurs vides
      for(int j = 0; j < 50; j++) {
        datas[j] = "";
      }

      msg = strtok (recvbuffer, "{}");
      char* res;
      // on rempli notre tableau data avec les différentes messages postée.
      // datas[j] = un message sur beebotte
      while (msg != NULL) {
        if(cpt != 0 && cpt%2 != 0) {//On evite l'entete de la reponse et les virgules separant les json
          //strncpy(msgTemp, msg, );
          //ligneData = strtok (msgTemp, "\n");
          //printf("\n\nLigne data :\n%s", ligneData);
          printf("\n\nMessage :\n%s", msg);
          // datas[j] = ligneData;
          datas[j] = msg;
          j = j + 1;
        }
        cpt = cpt + 1;
        msg = strtok (NULL, "{}");
      }

      /*Affichage des datas recus*/
      for(int i = 0; i < 50; i++) {
        if(datas[i] != "") {
          // printf("\n\nRobots datas from beebotte api:\n%s", strtok(strtok (datas[i], "\n"), ","));
          ligneData[i] = strtok (datas[i], "\n");
          printf("\n\nligne Data :\n%s", ligneData[i]);
        }
      }
  close(sock);

  // On récupère le contenue de la partie
  // char buffer[254  ]="";
  // strcat(buffer,"curl \"https://api.beebotte.com/v1/public/data/read/vberry/testVB/msg\" | tee donneesRobots.txt" );
  // system(buffer);

  // On récupère la ligne data

  // Joueur j1;
  // Joueur j2;
  // strcpy(j1.rfid, "21302029X" );
  // strcpy(j1.idRobot, "162.38.111.103" );
  // strcpy(j1.coulRobot, "bleu" );
  // listeJoueurs[0] = j1;
  //
  // strcpy(j2.rfid, "45604567X" );
  // strcpy(j2.idRobot, "162.111.38.130" );
  // strcpy(j2.coulRobot, "rouge" );
  // listeJoueurs[1] = j2;

  /* Envoi des données */

  return 1;
};
