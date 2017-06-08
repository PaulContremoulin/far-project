/*
 * Permet d'envoyer un message en cas d'erreur
 * @param char* msg [le message à envoyer]
 */
void error(const char *msg) { perror(msg); exit(0); }
