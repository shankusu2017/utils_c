#ifndef TCPIP_H_20240822204036_0X42110242
#define TCPIP_H_20240822204036_0X42110242

extern int uc_setnonblocking(int sockfd);
extern int uc_setnodelay(int sockfd);
extern int uc_setreuse(int sockfd);
extern int uc_setsndbuf(int sockfd, int len);
extern int uc_setrcvbuf(int sockfd, int len);

#endif /* TCPIP_H_20240822204036_0X42110242 */
