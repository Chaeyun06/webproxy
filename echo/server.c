#include "csapp.h"

extern void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd; // 연결을 듣는 서버의 fd, 클라이언트 소켓과 연결된 fd
    socklen_t clientlen; // 클라이언트 주소의 길이를 나타내는 변수
    struct sockaddr_storage clientaddr; // 소켓 주소 정보를 담는 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 포트 번호를 받아서 해당 포트에서 연결 대기
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 수락
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트의 호스트네임과 포트 번호 가져오기
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd); 
    }
    exit(0);
}