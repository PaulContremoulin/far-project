// tcp_client.c

#include <unistd.h> /* read, write, close */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PORT 10000


//Recuperer sur le dweet IGFAR_PartieX tous les robots et eur couleur

//pouvoir definir la couleur du but

int sock;
char couleurEnbut[15] ;
char couleurEnnemie[15] ;
int penalty = 0;
static int cptmess = 1;

typedef struct Joueur
{
char rfidetud[15];
char idRobot[20];
char coulRobot[15];
} Joueur;

typedef struct Ballon
{
char refBallon[20];
} Ballon;

Joueur listeJoueurs[2];
Ballon listeBallons[2];

//tableau à construire en fonction de ce qui a été récupéré sur le dweet de la partie

       /* = { 
            {"21302029X","162.111.38.129","bleu"},
            {"45604567X","162.111.38.130","rouge"} 
          };
          */
//utiliser typedef ?

void recupBallons() {
Ballon b1;
Ballon b2;
strcpy(b1.refBallon, "1" );
strcpy(b2.refBallon, "0" );
listeBallons[0] = b1;
listeBallons[1] = b2;
}

void recupJoueurs() { // à faire en fction de beebotte
  char buffer[254]="";
  strcat(buffer,"curl \"http://api.beebotte.com/v1/public/data/read/vberry/testVB/msg\" | tee curly.txt" );
  system(buffer);

  Joueur j1;
  Joueur j2;
  strcpy(j1.rfidetud, "21302029X" );
  strcpy(j1.idRobot, "162.38.111.103" );
  strcpy(j1.coulRobot, "bleu" );
  listeJoueurs[0] = j1;

  strcpy(j2.rfidetud, "45604567X" );
  strcpy(j2.idRobot, "162.111.38.130" );
  strcpy(j2.coulRobot, "rouge" );
  listeJoueurs[1] = j2;
};
int existenceBallon(char* idBallon) {
  int trouve = 0;
  int i = 0;

  while ( (i<2) && (trouve == 0) ) {
    if ( strcmp(listeBallons[i].refBallon,idBallon) == 0) {
      trouve = 1;
    }
    else {i++;}
  }

  return trouve; // 1 si le ballon existe, 0 sinon
};

int existenceJoueur(char* rfidcomp) {
  int trouve = 0;
  int i = 0;

  while ( (i<2) && (trouve == 0) ) {
    if ( strcmp(listeJoueurs[i].rfidetud,rfidcomp) == 0) {
      trouve = 1;
    }
    else {i++;}
  }

  return trouve; // 1 si le joueur existe, 0 sinon
};

int choixJoueur(char* rfidcomp) {
  int trouve = 0;
  int i = 0;
  int temp;

  while ( (i<2) && (trouve == 0) ) {
    if ( strcmp(listeJoueurs[i].rfidetud,rfidcomp) == 0) {
      trouve = 1;
      temp = i;
    }
    else {i++;}
  }

  return temp;
};

void error(const char *msg) { perror(msg); exit(0); }

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

void close_exit (int n) {
  /* Fermeture de la socket client */
  shutdown(sock, 2);
  close(sock);
  exit(0);
}

int main(void) {

  printf("Bienvenue dans la version 1.0.0 du validateur de but de MM. EL MAHMOUDI et DYE \n");
  int err1 = 1;
  char confirm[5];
  while(err1 == 1) {
    printf("Pour commencer, rentrez la couleur des robots de votre coté (rouge/bleu) : \n") ;
    //ici, on demande la couleur du coté du terrain : si la couleur renseignée est bleue, alors on ne considèrera que les robots rouges, et vice-versa
    fgets(couleurEnbut, 15, stdin );
    char *p1 = strchr(couleurEnbut,'\n');
    *p1='\0';

    if (strcmp(couleurEnbut,"rouge") == 0) {
      printf("Vous vous situez donc du coté %s ? (oui/non) \n", couleurEnbut );
      fgets(confirm, 15, stdin );
      char *p2 = strchr(confirm,'\n');
      *p2='\0';
      if (strcmp(confirm,"oui")==0 ) {
        printf("Très bien. Bonne partie ! \n");
        strcpy(couleurEnnemie,"bleu");
        err1 = 0;
      }
    
      else if (strcmp(confirm,"non")==0 ) {
      printf("Nous vous invitons à réécrire votre couleur\n");
      }
    
      else {
        printf("Votre réponse ne correspond pas à ce qui vous a été proposé\n");
      }
    }

    else if (strcmp(couleurEnbut,"bleu") == 0) {
      printf("Vous vous situez donc du coté %s ? (oui/non) \n", couleurEnbut );
      fgets(confirm, 15, stdin );
      char *p2 = strchr(confirm,'\n');
      *p2='\0';
      if (strcmp(confirm,"oui")==0 ) {
        printf("Très bien. Bonne partie !\n");
        strcpy(couleurEnnemie,"rouge");
        err1 = 0;
      }
      else if (strcmp(confirm,"non")==0 ) {
        printf("Nous vous invitons à réécrire votre couleur\n"); 
      }
      else {
        printf("Votre réponse ne correspond pas à ce qui vous a été proposé\n");
      }
    }    
  
    else {
      printf("Votre réponse ne correspond pas à ce qui vous a été proposé.\n");
    }
  }
  //getToBebotte

  recupJoueurs();
  recupBallons();


  int ingame = 1;
  char game[5];
  char rfid[20];

  while(ingame==1) {
    printf("Quelqu'un doit-il se connecter ? (oui/non) \n");
    fgets(game, 5, stdin );
    char *p3 = strchr(game,'\n');
    *p3='\0';

    if (strcmp(game,"oui")==0) {
      printf("Qui se connecte (taper 21302029X pour un joueur existant)? \n");

      fgets(rfid, 25, stdin );
      char *p4 = strchr(rfid,'\n');
      *p4='\0';
      //char rfid[15] = "21302029X"; // id récupérée par arduino
      printf("Erreur ?\n");
      if (existenceJoueur(rfid) == 1) {// Si le joueur existe pour cette partie
        // COMMUNICATION SOCKET
        struct sockaddr_in sin;

        /* Rattraper le Ctrl-C */
        signal(SIGINT, close_exit);

        /* Creation de la socket */
        sock = socket(AF_INET, SOCK_STREAM, 0);

        /* Configuration de la connexion */ 
        sin.sin_addr.s_addr = inet_addr(listeJoueurs[choixJoueur(rfid)].idRobot); //
        sin.sin_family = AF_INET;
        sin.sin_port = htons(PORT);

        /* Tentative de connexion au serveur */
        connect(sock, (struct sockaddr*)&sin, sizeof(sin));
        printf("\nConnexion a %s sur le port %d\n", inet_ntoa(sin.sin_addr),
        htons(sin.sin_port));
        printf("Récupération de l'identifiant du Robot, de l'identifiant du Ballon et couleur du robot : \n");
        
        /* Reception de donnees du serveur */
        char recu[50] = "";
        recv(sock, recu, 32, 0);
        printf("%s\n", recu);
      
        //décomposer les données en tokens
        char* tok = strtok(recu, "/");
        printf("IDROBOT = %s\n", tok);

        //on stocke l'id du robot qui a marqué
        char mess4[40] ="data=";
        strcat(mess4,tok);

        tok = strtok(NULL, "/");
        printf("IDBALLON = %s\n", tok);

        if (existenceBallon(tok)==1) {

          tok = strtok(NULL, "/");
          printf("COULROBOT = %s\n", tok);
          
          if (strcmp(tok, couleurEnnemie)==0) {
            //imaginons qu'on a envoyé les ids au distributeur de ballon. 
            // Premier cas : les ids correspondent
            for(;;) {
            // Envoi de donnees au serveur 
              char buffer[32] = "1";
              printf("Confirmation de but marqué au robot ");
              //fgets(buffer, 32, stdin);
              //char *pos = strchr(buffer, '\n');
              //*pos = '\0';
              send(sock, buffer, 32, 0);
              break;
            }//for


            // sendToBebotte
      
            char mess3[40];
            sprintf(mess3,"num=%i",cptmess);
            cptmess++;

            char *infoApublier[4]; // exemple de 4 infos a publier
            infoApublier[0] = "type_msg=BUT";
            infoApublier[1] = "type_ent=VB";
            infoApublier[2] = mess3;
            infoApublier[3] = mess4;
            // (attention : strcpy(infoApublier[3],"data=163.11.111.11") 
            //    ne marche pas car pas d'espace memoire alloue encore a infoApublier[3] ) 


            // !! TO DO : mettre ici le nom du "channel" ou on veut envoyer des donneees
            char *channel = "testVB";
            /* Par convention dans FAR on parle sur ressource "msg"
            sur laquelle on envoie une chaine contenant les couples clef:valeur separes par des 			virgules */
            char *ressource = "msg"; 
            // !! TO DO : mettre ci-dessous le token du canal !!
            // canal partie0 : 1494793564147_KNl54g97mG89kQSZ
            // canal testVB : 1494771555601_5SGQdxJaJ8O1HBj4
            char *channelKey = "1494771555601_5SGQdxJaJ8O1HBj4";

            /* Envoie a BeeBotte en methode POST */
            sendToBeBotte(channel,channelKey,ressource,infoApublier);
          }// if the robot is on the right side of the field

          else {
            printf("Vous avez marqué contre votre camp :( Le but n'est pas comptabilisé.\n");
            char buffer[32] = "-1";
            //fgets(buffer, 32, stdin);
            //char *pos = strchr(buffer, '\n');
            //*pos = '\0';
            send(sock, buffer, 32, 0);
          }
        }

        else {
          printf("Le ballon n'existe pas pour cette partie\n");
          char buffer[32] = "0";
          send(sock, buffer, 32, 0);
        } 
      }// if the ballon exists

    else { 
      printf("Le Joueur n'existe pas pour cette partie.\n");
      char buffer[32] = "E";
      //fgets(buffer, 32, stdin);
      //char *pos = strchr(buffer, '\n');
      //*pos = '\0';
      send(sock, buffer, 32, 0);
    } 
  } // if we are always ingame
  
    else if  (strcmp(game,"non")==0) {
      printf("Vous considérez que la partie est terminée. \nNous espérons que vous vous êtes bien amusés !\n");
      ingame =0;
    }

    else {
      printf("Erreur lors de la sasie. Veuillez saisir oui ou non.\n");
    }   
  
  }//while ingame
 
  return EXIT_SUCCESS;

}//main
