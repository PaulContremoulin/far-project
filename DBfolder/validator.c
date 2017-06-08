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
#include "DB/Validation_client.c"

#define PORT 1123
#define SIZE 131072
#define ARDUINO "/dev/ttyACM0" //Le port où est connecté notre arduino
	//cu.usbmodem1411

/*
 * Interface d'écoute pour arduino et RFID
 * @param int fd [port sur lequel on se connecte]
 * @param int speed [la partie du port sur lequel on veut se connecter]
 */
int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


/*
 * Communication part socket avec les robots, partie client.
 * @param char* hostname [nom du serveur sur lequel on veut se connecter]
 *
 * @return char* [la réponse reçu du serveur]
 */
char* joueur_request(char *ip)
{
  char request[256]="GET";
  char* response=NULL;

  struct sockaddr_in sin;
  //struct hostent *hostinfo;

  sin.sin_port = htons(PORT);
  //hostinfo = gethostbyname(hostname);
  //sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  sin.sin_addr.s_addr = inet_addr(ip);
  sin.sin_family = AF_INET;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  connect(sock, (struct sockaddr*)&sin, sizeof(sin));
  printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

  response = malloc(256*sizeof(char));
  send(sock, request, strlen(request),0);
  recv(sock, response, 256, 0);
  printf("response : %s\n", response);
  close(sock);
  return response;
}


/*
 * Permet d'envoyer un message en cas d'erreur
 * @param char* msg [le message à envoyer]
 */
void error(const char *msg) { perror(msg); exit(0); }

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

/* Configuration du validateur de but
 * @param char* channel
 * @param char* robots [la liste des robots de la partie]
 */
void init(char *channel, char *robots){
    char *ressource = "msg";
    recvToBeebotte(channel, ressource, robots);
}

/* Permet de récupérer l'ip d'un robot à partir
 * de son RFID.
 * @param char* infosPartie [liste des robotos de la partie]
 * @param char* rfid
 * @return char* [IP du robot]
 */
char* getIPbyRFID(char *infosPartie, char *rfid){

	printf("Recherche de l'ip associé à %s..\n", rfid);

	char *line, *data, *rfid_cmp, *ip, joueurs[SIZE];
	char* tabdata[1500];
	int cpt = 0;
	ip = "";

	strcpy(joueurs, infosPartie);
	data = strtok (joueurs, "[]");
	data = strtok (data, "{}");
	data = strtok (NULL, "{}");

	//Chaque ligne de données est enregistrée dans tabdata
	while(data != NULL){
	    tabdata[cpt] = data;
	    data = strtok(NULL, "{}");
	    data = strtok(NULL, "{}");
	    cpt++;
	}
  /* Récupèration d'un message complet de beebotte*/
	for(int i = 0; i < cpt; i++){
	    line = strtok (tabdata[i], "\"");
      line = strtok (NULL, "\"");
	    line = strtok (NULL, ":\n\"");
      line = strtok (NULL, "\"");
	    tabdata[i] = line;
	}
	for(int i = 0; i < cpt; i++)
  {
    line = strtok (tabdata[i], "=,");
    if(line != NULL && strcmp(line, "    ") != 0 )
    {
      line = strtok (NULL, "=,");
      if(strcmp(line, "IP") == 0)
      {
		    line = strtok (NULL, "=,");
		    line = strtok (NULL, "=,");
		    line = strtok (NULL, "=,");
		    rfid_cmp = strtok (NULL, "=,");
		    if(strcmp(rfid_cmp, rfid) == 0)
        {
		    	line = strtok (NULL, "=,");
		    	ip = strtok (NULL, "=,");
		    	printf("IP trouvé : %s\n", ip);
	      }
      }
    }
  }
  return ip;
}

/* Lors de l'appel du fichier dans le terminal, il faut ajouter en argument :
 * le channel utilisé sur beebotte argv[1], l'adresse ip des distributeur de ballon argv[2].
 */
int main(int argc, char *argv[]){
  char *portname = ARDUINO;
  char robots[SIZE];
  int fd;
  int wlen;

  //Ouverture du port pour écoute du lecteur de carte RFID
  fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

  if (fd < 0) {
      printf("Error opening %s: %s\n", portname, strerror(errno));
      return -1;
  }

  /* baudrate 115200, 8 bits, no parity, 1 stop bit */
  set_interface_attribs(fd, B115200);

  /* Initialisation du validateur de but */
  printf("Initialisation....\n");
  init("partie0", robots);
  printf("Données récupérées\n\n");

  /* attente active */
  do {
      char rfid[16], idBall[256];
      char* ip;
      int rdlen;

      printf("En attente d'une RFID...\n");
      rdlen = read(fd, rfid, sizeof(rfid)-1);

      printf("nb lu : %d\n", rdlen);
      printf("valeur : %s\n", rfid);

      printf("\nValeur de la rfid : %s\n", rfid);

      //Recherche de l'IP associé au RFID
      ip = getIPbyRFID(robots, rfid);

      if(strlen(ip) <= 0){
          printf("Erreur de reception du RFID ou aucune IP associée.\n");
      }else{

          printf("\nIP associé : %s\n", ip);

          //On récupère l'idBall du joueur
          strcpy(idBall,joueur_request(ip));
          printf("idBall : %s\n", idBall);
          if(validationbut_1 ("162.38.111.64",idBall,rfid)){
              printf("But accepté !\n\n");
              sendValidBut("VBpartieTEST", ip); //VBpartieTest
          }else{
              printf("But refusé..\n\n");
          }
      }
  /* repeat for next rfid */
  } while (1);
}
