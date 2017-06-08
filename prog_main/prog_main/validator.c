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
#define SIZE 131072
#define ARDUINO "/dev/ttyACM0" //Le port où est connecté notre arduino
	//cu.usbmodem1411

#include "DB/Validation_client.c"
#include "VB/error.c"
#include "VB/communicationArduino.c"
#include "VB/communicationBeebotte.c"
#include "VB/communicationJoueur.c"
#include "VB/utile.c"






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
