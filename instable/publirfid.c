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

int main(void){

    char *portname = "/dev/ttyACM0";
    int fd;
    int wlen;
    char *pos;
    char ipvalue[256];
    char *infoAPublier[4];
    char *channel = "partie0";
    char *ressource = "msg";
    // VB_TC : 1496436248369_f6FG4rK7bYcY9R1u
    // RJpartieTEST : 1496856856204_4VjiqoDtPoWVTeHD
    // partie 0 : 1494793564147_KNl54g97mG89kQSZ
   char *channelKey = "1494793564147_KNl54g97mG89kQSZ";

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(fd, B115200);

    /* attente active */
    do {
        unsigned char rfid[16];
        int rdlen;

	printf("En attente d'une carte RFID...\n");

        rdlen = read(fd, rfid, sizeof(rfid)-1);

   	printf("nb lu : %d\n", rdlen);
   	printf("valeur : %s\n", rfid);
   	printf("taille : %lu\n", sizeof(rfid));


	printf("Entrez l'IP correspondant à l'RFID : ");
	fgets(ipvalue, 256, stdin);

	if ((pos=strchr(ipvalue, '\n')) != NULL) *pos = '\0';

	printf("\nValeur de la rfid : %s\n", rfid);
	printf("\nValeur de l'IP    : %s\n", ipvalue);

	char data[256] = "data=";
	char num[256] = "num=";
	strcat(data,ipvalue);
	strcat(num, rfid);
	printf("\nValeur de DATA   : %s\n", data);

	infoAPublier[0] = "type_msg=IP";
	infoAPublier[1] = "type_ent=RJ";
	infoAPublier[2] = num;
	infoAPublier[3] = data;

	sendToBeBotte(channel, channelKey, ressource, infoAPublier);

        /* repeat for next rfid */
    } while (1);
  
}
