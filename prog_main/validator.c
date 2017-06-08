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

#define PORT 1123

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
 * Communication part socket avec les robots
 * Partie client
 * @param char* hostname [nom du serveur sur lequel on veut se connecter]
 */
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

  recv(sock, response, 256, 0);

  close(sock);

  return response;
}


/*
 * Permet d'envoyer un message en cas d'erreur
 * @param char* msg [le message à envoyer]
 */
void error(const char *msg) { perror(msg); exit(0); }

/* Envoi de données vers beebotte en POST
 * @param char* canal [le canal de communication beebotte où envoyer le message]
 * @param char* clefCanal [la clef pour accéder au canal]
 * char* ressource
 * char[]* data [tableau de chaines]
 */
int sendToBeeBotte(char *canal, char *clefCanal, char *ressource, char *data[]) {
  // data est un tableau de chaines (char[]), c-a-d un tableau de char a deux dimensions
  // printf("data[0] is %s\n",data[0]);
  //printf("data[3] is %s\n",data[3]);

  int i;
  char *host = "api.beebotte.com";
  char path[100] = "/v1/data/write/";
  strcat(path,canal);strcat(path,"/"); strcat(path,ressource);
  struct hostent *server;
  struct sockaddr_in serv_addr;
  int sockfd, bytes, sent, received, total, message_size;
  char *message, response[4096];

  // Nécessaire pour envoyer des données sur beebotte.com (noter le token du canal à la fin) :
  char headers[300] = "Host: api.beebotte.com\r\nContent-Type: application/json\r\nX-Auth-Token: ";
  strcat(headers,clefCanal);
  strcat(headers,"\r\n");

  char donnees[4096] = "{\"data\":\""; // "data" est imposée par beebotte.com
  for (i=0;i<3;i++) {
    strcat(donnees,data[i]);
    strcat(donnees,",");
  }
  strcat(donnees,data[3]);
  strcat(donnees,"\"}");

  /* Récupération de la taille du message HTML */
  message_size=0;
  message_size+=strlen("%s %s HTTP/1.0\r\n")+strlen("POST")+strlen(path)+strlen(headers);
  message_size+=strlen("Content-Length: %d\r\n")+10+strlen("\r\n")+strlen(donnees);
  /* On alloue la taille nécessaire au message */
  message=malloc(message_size);

  /* Construit le message POST */
  sprintf(message,"POST %s HTTP/1.0\r\n",path);
  sprintf(message+strlen(message), "%s",headers);
  sprintf(message+strlen(message),"Content-Length: %zu\r\n",strlen(donnees));
  strcat(message,"\r\n");              /* blank line     */
  strcat(message,donnees);             /* body           */

  /* Affichage du message à envoyé */
  printf("Request:\n%s\n-------------\n",message);

  /* création de la socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error("ERROR opening socket");

  /* lookup the ip address */
  server = gethostbyname(host);
  if (server == NULL) error("ERROR, no such host");

  /* fill in the structure */
  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(80); // port 80
  memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

  /* connect the socket */
  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) error("ERROR connecting");

  /* send the request */
  total = strlen(message);
  sent = 0;
  do {
    bytes = write(sockfd,message+sent,total-sent);
    if (bytes < 0) error("ERROR writing message to socket");
    if (bytes == 0) break;
    sent+=bytes;
  } while (sent < total);

  /* receive the response */
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

  /* close the socket */
  close(sockfd);

  /* process response */
  printf("Response:\n%s\n",response);

  free(message);
  return 0;
}

//Fonction pour récupérer les données sur bebotte
void recvToBebotte(char *channel, char *ressource, char *data) {

    int sockD, verif, total, received, bytes;
    struct sockaddr_in sin;
    struct hostent *hostinfo;

    char *hostname = "api.beebotte.com";
    char response[4096];
    char *pos;

    /* Configuration de la connexion */
    sin.sin_port = htons(80);
    hostinfo = gethostbyname(hostname);

    sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    sin.sin_family = AF_INET;


    /* Creation de la socket */
    sockD = socket(AF_INET, SOCK_STREAM, 0);

    /* Tentative de connexion au serveur */
    connect(sockD, (struct sockaddr*)&sin, sizeof(sin));
    printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

    /* Creation de la requête */
    char request[256];
    sprintf(request, "GET /v1/public/data/read/vberry/%s/%s HTTP/1.1\r\nHost: %s\r\n\r\n", channel, ressource, hostname);
    printf("REQUETE : \n%s\n",request);

    /* Envoie du message au serveur */
    verif = send(sockD, request, strlen(request),0);

    /* receive the response */
    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    //do {
	//printf("receved = %d\n", received);
	//printf("Message = %s\n", response);
    	bytes = recv(sockD,response+received,total-received,MSG_PEEK);
        if (bytes < 0) error("ERROR reading response from socket");
	//if (bytes == 0) break;
	received+=bytes;
    //} while (received < total);

    if (received == total) error("ERROR storing complete response from socket");

    /* close the socket */
    close(sockD);

    /*Get body response */
    pos=strchr(response, '[');
    memcpy(data, pos, sizeof(response));

    /* process response */
    //printf("Body reponse:\n%s\n\n",data);
}

//Envoi la validation du but
int sendValidBut(char* joueur) {
    char *infoAPublier[4];
    char *channel = "VBpartieTEST";
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
    printf("\nValeur de DATA   : %s\n", data);

    infoAPublier[0] = "type_msg=BUT";
    infoAPublier[1] = "type_ent=VB";
    infoAPublier[2] = "num=1";
    infoAPublier[3] = data;

    sendToBeBotte(channel, channelKey, ressource, infoAPublier);
}

//Configuration du validateur de but
void init(char *robots){
    char *channel = "RJpartieTEST";
    char *ressource = "msg";
    //Fonction beta de reception des rfids
    recvToBebotte(channel, ressource, robots);
}

char* getIPbyRFID(char *robots, char *rfid){
	printf("Recherche de l'ip associé à %s..\n", rfid);
	char *line, *data, *info, *rfid_cmp, *ip, joueurs[4096];
	char* tabdata[50];
	int cpt = 0;

	strcpy(joueurs, robots);
	data = strtok (joueurs, "[]");
	line = strtok (data, "{}");
	line = strtok(NULL, "{}");
	while(line != NULL){
	    tabdata[cpt] = line;
	    line = strtok(NULL, "{}");
	    line = strtok(NULL, "{}");
	    cpt++;
	}

	for(int i = 0; i < cpt; i++){
	    info = strtok(tabdata[i], "\" :");
	    while(strcmp(info, "data")!=0){
	    	info = strtok(NULL, "\" :");
	    }
	    info = strtok(NULL, "\" :");
	    rfid_cmp = strtok(info, "=,;");
	    while(strcmp(rfid_cmp, "RFID")){
		rfid_cmp = strtok(NULL, "=,;");
	    }
	    rfid_cmp = strtok(NULL, "=,;");
	    if(strcmp(rfid, rfid_cmp) == 0){
		ip = strtok(NULL, "=,;");
		ip = strtok(NULL, "=,;");
		printf("IP associé :%s\n", ip);
		break;
	    }
	}

	return ip;
}

int main(void){

    char *portname = "/dev/ttyACM0";
    char robots[4096];
    int fd;
    int wlen;

    //Ouverture du port pour écoute du lecteur de carte RFID
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(fd, B115200);


    /*Configuration du validateur de but*/
    printf("Initialisation....\n");
    init(robots);
    printf("Données récupérées\n\n");

    /* attente active */
    do {
        unsigned char rfid[16];
	unsigned char* ip, idball;
        int rdlen;

	printf("En attente d'une RFID...\n");
        rdlen = read(fd, rfid, sizeof(rfid)-1);

   	printf("nb lu : %d\n", rdlen);
   	printf("valeur : %s\n", rfid);

	printf("\nValeur de la rfid : %s\n", rfid);

	//Recherche de l'IP associé à la RFID
	ip = getIPbyRFID(robots, rfid);

	if(strlen(ip) <= 0){
		printf("Erreur de reception de la RFID ou aucune IP associée.\n");
	}else{

		printf("\nIP associé : %s\n", ip);

		//On récupère l'idBall du joueur
		idball = joueur_request(ip);

		if(1){ //validationbut_1 ("localhost",idball) == 0){
			printf("But accepté !\n");
			sendValidBut(ip);
		}else{
			printf("But refusé..\n");
		}
	}
	//sendMonitoring(rfid, "4685484984");
        /* repeat for next rfid */
    } while (1);

}
