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
#include <json-c/json.h>

#define PORT 1123

//Interface d'écoute pour arduino et RFID
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

  recv(sock, response, 256, 0);

  close(sock);

  return response;
}


//Fonction d'erreurs
void error(const char *msg) { perror(msg); exit(0); }

//Envoi de données a beebotte en POST
int sendToBeBotte(char *canal, char *clefCanal, char *ressource, char *data[]) {
  // data est un tableau de chaines (char[]), c-a-d un tableau de char a deux dimensions
  // printf("data[0] is %s\n",data[0]);
  //printf("data[3] is %s\n",data[3]);

  int i;
  char *host = "api.beebotte.com";
    /* !! TODO remplacer 'testVB' par le canal dans lequel publier (ex: partie12)
        (ici msg est la "ressource" que ce canal attend */
  char path[100] = "/v1/data/write/";
  strcat(path,canal);strcat(path,"/"); strcat(path,ressource);
  struct hostent *server;
  struct sockaddr_in serv_addr;
  int sockfd, bytes, sent, received, total, message_size;
  char *message, response[4096];

    // Necessaire pour envoyer des donnees sur beebottle.com (noter le token du canal a la fin) :
  char headers[300] ="Host: api.beebotte.com\r\nContent-Type: application/json\r\nX-Auth-Token: ";
  strcat(headers,clefCanal);strcat(headers,"\r\n"); 

  char donnees[4096] = "{\"data\":\""; // "data" est impose par beebotte.com
  for (i=0;i<3;i++) {
    strcat(donnees,data[i]);strcat(donnees,",");
  }
  strcat(donnees,data[3]);strcat(donnees,"\"}");

  /* How big is the whole HTTP message? (POST) */
  message_size=0;
  message_size+=strlen("%s %s HTTP/1.0\r\n")+strlen("POST")+strlen(path)+strlen(headers);
  message_size+=strlen("Content-Length: %d\r\n")+10+strlen("\r\n")+strlen(donnees); 
  /* allocate space for the message */
  message=malloc(message_size);

  /* Construit le message POST */
  sprintf(message,"POST %s HTTP/1.0\r\n",path); 
  sprintf(message+strlen(message), "%s",headers);
  sprintf(message+strlen(message),"Content-Length: %zu\r\n",strlen(donnees));
  strcat(message,"\r\n");              /* blank line     */
  strcat(message,donnees);             /* body           */

  /* What are we going to send? */
  printf("Request:\n%s\n-------------\n",message);

  /* create the socket */
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
    printf("Body reponse:\n%s\n\n",data);
}

//Envoi des données au monitoring
int sendMonitoring(char* rfid, char* idball) {

    int sockD, verif;
    struct sockaddr_in sin;
    struct hostent *hostinfo;
    
    /* Configuration de la connexion */
    sin.sin_port = htons(80);
    hostinfo = gethostbyname("api.beebotte.com");
    
    sin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    sin.sin_family = AF_INET;
    
    
    /* Creation de la socket */
    sockD = socket(AF_INET, SOCK_STREAM, 0);
    
    /* Tentative de connexion au serveur */
    connect(sockD, (struct sockaddr*)&sin, sizeof(sin));
    printf("Connexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));
    
    /* Creation de la requête */
    char request[256];
    sprintf   (request, "GET /dweet/for/robot17?rfid=%s&idball=%s HTTP/1.1\r\nHost:%s\r\n\r\n", rfid, idball, "www.dweet.io");
    printf("REQUETE : \n%s\n",request);
    printf("Valeur de rfid : %s\n",rfid);
    printf("Valeur de idball : %s\n\n",idball);
    /* Envoie du message au serveur */
    verif = send(sockD, request, strlen(request),0);
    close(sockD);
    return 1;
}

//Configuration du validateur de but
void init(char *robots){
    char *channel = "VB_TC";
    char *ressource = "msg";
    //Fonction beta de reception des rfids
    recvToBebotte(channel, ressource, robots);
}

char getIPbyRFID(char *robots, char *rfid){
	json_object *new_obj;
	new_obj = json_tokener_parse(robots);
	char *test = json_object_get_string(json_object_object_get(new_obj, "data"));
	printf("Valeur de la 1er ligne : %s\n", test
	);
}

int main(void){

    char *portname = "/dev/ttyACM1";
    char robots[4096];
    int fd;
    int wlen;

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
    printf("Robots : %s\n", robots);

    /* attente active */
    do {
        unsigned char rfid[16];
        int rdlen;

	printf("En attente d'une RFID...\n");
        rdlen = read(fd, rfid, sizeof(rfid)-1);

   	printf("nb lu : %d\n", rdlen);
   	printf("valeur : %s\n", rfid);
   	printf("taille : %lu\n", sizeof(rfid));

	printf("\nValeur de la rfid : --%s--\n", rfid);

	getIPbyRFID(robots, rfid);
	//sendMonitoring(rfid, "4685484984");
        /* repeat for next rfid */
    } while (1);

/* Besoin de récupérer l'ip en fonction de la rfid

    char hostname[256] = "localhost";

    char* idball = joueur_request(hostname);
    printf("joueur vaut : %s\n", idball);
    sendMonitoring(hostname, idball);
*/
  
}

