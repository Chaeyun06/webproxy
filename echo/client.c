#include "csapp.h"

int main(int argc, char **argv) // argument count 입력한 인수의 개수, argument vector 입력한 값
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio; // rio 구조체 

    if (argc != 3) { // ?
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1]; // 호스트 정보
    port = argv[2]; // 포트 정보

    clientfd = Open_clientfd(host, port); // 클라이언트 소켓 파일 디스크립터 열기.
    Rio_readinitb(&rio, clientfd); // Rio_readlineb() 를 사용할 준비를 함.

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {
        Rio_writen(clientfd, buf, strlen(buf)); // 클라이언트 소켓에 데이터 쓰기
        Rio_readlineb(&rio, buf, MAXLINE); // 서버로부터 데이터 읽기
        Fputs(buf, stdout); // 에코된 데이터 화면에 출력
    }
    
    Close(clientfd); // 클라이언트 소켓 닫기
    exit(0); // 프로그램 종료
}