#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MSG_BUF 256

void ParseHostName(char hostName[], struct addrinfo* addressInfo)
{
    char workingHostName[MSG_BUF];
    if (strncmp(hostName, "localhost", 9) == 0) 
    {
        strncpy(workingHostName, "127.0.0.1", MSG_BUF);
    } 
    else
    {
        strncpy(workingHostName, hostName, MSG_BUF);
    }
    struct addrinfo hint;
    memset (&hint, 0, sizeof (hint));
    hint.ai_family = PF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags |= AI_CANONNAME;
    int errcode;
    char addrstr[100];
    void *ptr;
    
    errcode = getaddrinfo(workingHostName, NULL, &hint, &addressInfo);
    if (errcode != 0)
    {
        printf("getaddrinfo error\n");
        exit(0);
    }
    switch (addressInfo->ai_family)
    {
    case AF_INET:
        ptr = &((struct sockaddr_in *) addressInfo->ai_addr)->sin_addr;
        break;
    case AF_INET6:
        ptr = &((struct sockaddr_in6 *) addressInfo->ai_addr)->sin6_addr;
        break;
    }
    inet_ntop (addressInfo->ai_family, ptr, addrstr, 100);
    printf("Address received: %s\n", addrstr);
}

int main(int argc, char *argv[])
{
    struct addrinfo addressInfo;
    ParseHostName("ui11.cs.ualberta.ca", &addressInfo);
    ParseHostName("localhost", &addressInfo);
    ParseHostName("127.0.0.1", &addressInfo);
    ParseHostName("216.58.217.46", &addressInfo);
}
