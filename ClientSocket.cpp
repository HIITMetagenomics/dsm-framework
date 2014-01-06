#include "ClientSocket.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

using namespace std;

void error(const char *msg)
{
    cerr << "error: " << msg << endl;
    exit(1);
}

ClientSocket::ClientSocket(std::string const &host, int port)
    : n(0), sockfd(0)
{
    for (unsigned i = 0; i < BUFFER_SIZE; ++i)
        buffer[0] = 0;

    /** 
     * Init the socket connection
     */
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(host.c_str());
    if (server == NULL)
        error("ERROR, no such host");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
}

/* Write "n" bytes to a descriptor. (UNIX Network Programming, Andrew M. Rudoff, Bill Fenner, W. Richard Stevens, 2004) */
ssize_t writen(int fd, const char *vptr, size_t n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
#ifdef MSG_NOSIGNAL
        if ( (nwritten = send(fd, ptr, nleft, MSG_NOSIGNAL)) <= 0) {
#else
        if ( (nwritten = send(fd, ptr, nleft, 0)) <= 0) {
#endif
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       /* and call write() again */
            else
                return(-1);         /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}

/* Nonblocking read */
ssize_t ClientSocket::readnonblocking(int fd, char *vptr, size_t n)
{
    //ssize_t nread = recv(fd, vptr, n, MSG_DONTWAIT);
    //return nread;    
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft == n) {
        if ( (nread = recv(fd, ptr, nleft, MSG_DONTWAIT)) < 0) {
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

void ClientSocket::flushBuffer()
{
    /*cerr << "BUFFER UPDATE!!!!" << endl << "\"";
    for (unsigned i = 0; i < n; ++i)
        if (buffer[i] < 20)
            std::cerr << (int)buffer[i];
        else
            std::cerr << buffer[i];
            cerr << "\"" << endl;*/

    if (writen(sockfd, buffer, n) != n)
        error("ERROR writing the output");

    n = 0;
}

ClientSocket::~ClientSocket()
{
    if (n)
        flushBuffer(); // Pending bytes in buffer
    close(sockfd);
}

