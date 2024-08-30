#ifndef TCPIP_H_20240822204036_0X42110242
#define TCPIP_H_20240822204036_0X42110242

extern int setnonblocking(int sockfd);
extern int setnodelay(int sockfd);
extern int setreuse(int sockfd);
extern int utils_setsndbuf(int sockfd, int len);
extern int utils_setrcvbuf(int sockfd, int len);

#endif /* TCPIP_H_20240822204036_0X42110242 */
