#include "csapp.h"

// 클라이언트로부터 받은 데이터를 다시 클라이언트에게 돌려줌.
extern void echo(int connfd) //클라이언트와 연결된 소켓 fd를 받음.
{
    size_t n;
    char buf[MAXLINE]; // 클라이언트로부터 받은 데이터 저장 버퍼
    rio_t rio;

    Rio_readinitb(&rio, connfd); // rio 라이브러리 초기화, 버퍼링된 입출력 설정
    while((n = Rio_readliineb(&rio, buf, MAXLINE)) != 0){ // n: 읽은 바이트 수
        printf("server received %d bytes \n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}