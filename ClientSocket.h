#ifndef _ClientSocket_H_
#define _ClientSocket_H_

#include "Tools.h"

class ClientSocket
{
public:
    ClientSocket(std::string const &, int );
    virtual ~ClientSocket();

    inline void putc(char c)
    {
        if (n == BUFFER_SIZE)
            flushBuffer();

        buffer[n++] = c;
    }

    inline void putulong(ulong u)
    {
        if (u < (1u << 7))
        {
            uchar c = (u & 0xFFlu) | (1lu << 7);
            putc(c);
            return;
        }

        char l = 0;
        ulong tmp = u;
        do {
            ++l; 
        } while ((u >>= 8));
        putc(l);
        u = tmp;
        do {
            putc(u & 0xFFlu);  
        } while ((u >>= 8));
    }

    inline void putstring(std::string const &str)
    {
        for (size_t i = 0; i < str.size(); ++i)
            putc(str[i]);
        putc('.');
    }

    ulong checkHalt(unsigned &depth)
    {
        ulong u[2];
        u[0] = 0;
        u[1] = 0;
        ssize_t nread = readnonblocking(sockfd, (char *)&u, sizeof(ulong)*2);
        if (nread <= 0)
            return 0;

        if (nread != sizeof(ulong)*2)
        {
            std::cerr << "ClientSocket::checkHalt(): nonblocking read failed!" << std::endl;
            std::exit(1);
        }

        if ((u[0] & 0xFF) != (ulong)'H')
        {
            std::cerr << "ClientSocket::checkHalt(): malformed halt!" << std::endl;
            std::exit(1);
        }
        
        depth = (unsigned) (u[0]>>8);
        if (depth == 0)
        {
            std::cerr << "ClientSocket::checkHalt(): depth == 0?!" << std::endl;
            std::exit(1);
        }

        return u[1];
    }

protected:
    void flushBuffer();

    static const unsigned BUFFER_SIZE = 16*1024; ///1024*1024;

    ssize_t readnonblocking(int fd, char *vptr, size_t n);

    char buffer[BUFFER_SIZE];
    unsigned n; // number of bytes in buffer
    int sockfd;

private:
    ClientSocket();
    // No copy constructor and assignment
    ClientSocket(ClientSocket const&);
    ClientSocket& operator = (ClientSocket const&);
};

#endif // _ClientSocket_H_
