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
