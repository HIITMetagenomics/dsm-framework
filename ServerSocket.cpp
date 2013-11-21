#include "ServerSocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

#define BACKLOG 256     // how many pending connections queue will hold

void error(const char *msg)
{
    cerr << "error: " << msg << endl;
    exit(1);
}

int ServerSocket::init(int port)
{
    int sockfd;                    // listen on sock_fd
    struct sockaddr_in my_addr;    // my address information
    int yes=1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        error("socket failed");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        error("setsockopt failed");
    /*if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) == -1)
      error("setsockopt KEEPALIVE failed");*/
    
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(port);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) 
    {
        printf("Is the server already running?\n");
        error("bind failed");
    }

    if (listen(sockfd, BACKLOG) == -1)
        error("listen failed");
    return sockfd;
}

ServerSocket * ServerSocket::create(int sockfd)
{
    struct sockaddr_in their_addr; // connector's address information
    socklen_t sin_size = sizeof(struct sockaddr_in);

    int new_fd = 0;
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) 
        error("accept");

    char const * ip = inet_ntoa(their_addr.sin_addr); 
    cerr << "server: New connection from " << ip << ", socket " << new_fd << endl;

    /* Checking KEEPALIVE
    int optval = 0;
    socklen_t optsize = sizeof(int);
    if(getsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optsize) < 0)
        error("getsockopt failed");
        cerr << "SO_KEEPALIVE is " << (optval ? "ON" : "OFF") << endl;*/
    return new ServerSocket(new_fd);
}

ServerSocket::ServerSocket(int sfd)
    : pos(0), n(0), sockfd(sfd), eof(false)
{ 
    for (unsigned i = 0; i < BUFFER_SIZE; ++i)
        buffer[0] = 0;
}


 /* Read [1..n] bytes from a descriptor. (UNIX Network Programming, Andrew M. Rudoff, Bill Fenner, W. Richard Stevens, 2004) */
ssize_t ServerSocket::readn(int fd, char *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft == n) {
        if ( (nread = recv(fd, ptr, nleft, 0)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);      /* return >= 0 */
}


/* Non-blocking write */
void ServerSocket::writenonblocking(int fd, const char *vptr, size_t n)
{
    send(fd, vptr, n, MSG_DONTWAIT);
}


void ServerSocket::readBuffer()
{
/*    cerr << "BUFFER UPDATE!!!!" << endl << "\"";
    for (unsigned i = 0; i < n; ++i)
        if (buffer[i] < 20)
            std::cerr << (int)buffer[i];
        else
            std::cerr << buffer[i];
            cerr << "\"" << endl;*/

    if ((n = readn(sockfd, buffer, BUFFER_SIZE)) < 1)
        eof = true;

    pos = 0;
}

ServerSocket::~ServerSocket()
{
    close(sockfd);
}



