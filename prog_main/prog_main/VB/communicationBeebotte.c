#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

/* Envoi de données vers beebotte en POST, par socket.
* @param char* canal [le canal de communication beebotte où envoyer le message]
* @param char* clefCanal [la clef pour accéder au canal]
* @param char* ressource ?
* @param char[]* data [tableau de ?]
*/
int sendToBeebotte(char *canal, char *clefCanal, char *ressource, char *data[])
{
  int i;
  char *host = "api.beebotte.com";
  char path[100] = "/v1/data/write/";
  strcat(path,canal);strcat(path,"/"); strcat(path,ressource);
  struct hostent *server;
  struct sockaddr_in serv_addr;
  int sockfd, bytes, sent, received, total, message_size;
  char *message, response[4096];

  /* Nécessaire pour envoyer des données sur beebottle.com (noter le token du canal à la fin) */
  char headers[300] ="Host: api.beebotte.com\r\nContent-Type: application/json\r\nX-Auth-Token: ";
  strcat(headers,clefCanal);strcat(headers,"\r\n");

  char donnees[4096] = "{\"data\":\""; // "data" est impose par beebotte.com
  for (i=0;i<3;i++) {
    strcat(donnees,data[i]);strcat(donnees,",");
  }
  strcat(donnees,data[3]);strcat(donnees,"\"}");

  /* Récupération de la taille du message HTML */
  message_size=0;
  message_size+=strlen("%s %s HTTP/1.0\r\n")+strlen("POST")+strlen(path)+strlen(headers);
  message_size+=strlen("Content-Length: %d\r\n")+10+strlen("\r\n")+strlen(donnees);
  /* On alloue la taille nécessaire au message */
  message=malloc(message_size);

  /* Construction du message POST */
  sprintf(message,"POST %s HTTP/1.0\r\n",path);
  sprintf(message+strlen(message), "%s",headers);
  sprintf(message+strlen(message),"Content-Length: %zu\r\n",strlen(donnees));
  strcat(message,"\r\n");              /* ligne blanche */
  strcat(message,donnees);             /* body */

  /* création de la socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");

  /* vérification de l'host */
  server = gethostbyname(host);
  if (server == NULL) error("ERROR, no such host");

  /* Structure */
  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(80); // port 80
  memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

  /* Connexion de la socket */
  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) error("ERROR connecting");

  /* envoi du message */
  total = strlen(message);
  sent = 0;
  do {
    bytes = write(sockfd,message+sent,total-sent);
    if (bytes < 0) error("ERROR writing message to socket");
    if (bytes == 0) break;
    sent+=bytes;
  } while (sent < total);

  /* réception de la réponse */
  memset(response,0,sizeof(response));
  total = sizeof(response)-1;
  received = 0;
  do {
    bytes = read(sockfd,response+received,total-received);
    if (bytes < 0) error("ERROR reading response from socket");
    if (bytes == 0) break;
    received+=bytes;
  } while (received < total);

  if (received == total) error("ERROR storing complete response from socket");

  /* fermeture de la socket */
  close(sockfd);

  free(message);
  return 0;
}

/* Récupération des données sur Beebotte
 * @param char* channel [le channel beebotte où l'on veut récupérer les données]
 * @param char* ressource [?]
 * @param char* data [?]
 */
void recvToBeebotte(char *channel, char *ressource, char *data) {

    int sockD, verif, total, received, bytes;
    struct sockaddr_in sin;
    struct hostent *hostinfo;

    char *hostname = "api.beebotte.com";
    char response[SIZE];
    char *pos;

    /* Configuration de la connexion */
    sin.sin_port = htons(80);
    hostinfo = gethostbyname(hostname);

    sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    sin.sin_family = AF_INET;


    /* Création de la socket */
    sockD = socket(AF_INET, SOCK_STREAM, 0);

    /* Tentative de connexion au serveur */
    connect(sockD, (struct sockaddr*)&sin, sizeof(sin));
    printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

    /* Création de la requête */
    char request[256];
    sprintf(request, "GET /v1/public/data/read/vberry/%s/%s HTTP/1.1\r\nHost: %s\r\n\r\n", channel, ressource, hostname);
    printf("REQUETE : \n%s\n",request);

    /* Envoie du message au serveur */
    verif = send(sockD, request, strlen(request),0);

    /* réception de la réponse */
    memset(response,0,sizeof(response));
    total = SIZE*sizeof(char);
    received = 0;
    do {
    	bytes = recv(sockD,response+received,total-received,0); //MSG_PEEK);
        if (bytes < 0) error("ERROR reading response from socket");
      	if (bytes == 0) break;
      	received+=bytes;
      	printf("Message = %s\n", response);
      	printf("receved = %d\n", received);
    } while (received < total && (pos=strchr(response, ']')) == NULL);

    if (received >= total) error("ERROR storing complete response from socket");

    /* close the socket */
    close(sockD);

    /*Get body response */
    pos=strchr(response, '[');
    memcpy(data, pos, sizeof(response));
}

/* Envoi la validation du but sur beebotte
 * @param char* channel [le channel où l'on veut publier la réponse]
 * @param char* joueur [l'ip du joueur qui a marqué]
 */
void sendValidBut(char* channel, char* joueur) {
    char *infoAPublier[4];
    char *ressource = "msg";
    // VB_TC : 1496436248369_f6FG4rK7bYcY9R1u
    // VBpartieTEST : 1496856372525_RTFvkGaUTGiDzni8
    // RJpartieTEST : 1496856856204_4VjiqoDtPoWVTeHD
    char *channelKey = "1496856372525_RTFvkGaUTGiDzni8";

    char datenow[20];
    time_t now = time(NULL);
    strftime(datenow, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

    char data[256] = "JOUEUR=";
    strcat(data,joueur);strcat(data,";");strcat(data,"TIME=");strcat(data,datenow);
    //printf("\nValeur de DATA   : %s\n", data);

    infoAPublier[0] = "type_msg=BUT";
    infoAPublier[1] = "type_ent=VB";
    infoAPublier[2] = "num=1";
    infoAPublier[3] = data;

    sendToBeebotte(channel, channelKey, ressource, infoAPublier);
}
