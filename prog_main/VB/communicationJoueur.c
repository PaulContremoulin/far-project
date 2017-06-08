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
