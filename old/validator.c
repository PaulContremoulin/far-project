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


//Envoi des données au monitoring
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
    sprintf   (request, "GET /dweet/for/robot17?rfid=%s&idball=%s HTTP/1.1\r\nHost:%s\r\n\r\n", rfid, idball, "www.dweet.io");
    printf("REQUETE : \n%s\n",request);
    printf("Valeur de rfid : %s\n",rfid);
    printf("Valeur de idball : %s\n\n",idball);
    /* Envoie du message au serveur */
    verif = send(sockD, request, strlen(request),0);
    close(sockD);
    return 1;
}

int main(void){

    char *portname = "/dev/ttyACM1";
    int fd;
    int wlen;

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


        rdlen = read(fd, rfid, sizeof(rfid)-1);

   	printf("nb lu : %d\n", rdlen);
   	printf("valeur : %s\n", rfid);
   	printf("taille : %lu\n", sizeof(rfid));

	printf("\nValeur de la rfid : --%s--\n", rfid);

	sendMonitoring(rfid, "4685484984");
        /* repeat for next rfid */
    } while (1);

/* Besoin de récupérer l'ip en fonction de la rfid

    char hostname[256] = "localhost";

    char* idball = joueur_request(hostname);
    printf("joueur vaut : %s\n", idball);
    sendMonitoring(hostname, idball);
*/
  
}

