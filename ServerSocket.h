#ifndef _ServerSocket_H_
#define _ServerSocket_H_

#include "Tools.h"

class ServerSocket
{
public:
    static int init(int portno);
    static ServerSocket * create(int sockfd);
    virtual ~ServerSocket();

    void debug()
    {        
        std::cerr << "pos = " << pos << ", n = " << n << std::endl << "\"";
        for (unsigned i = 0; i < n; ++i)
            if (buffer[i] < 20)
                std::cerr << (int)buffer[i];
            else
                std::cerr << buffer[i];
        std::cerr << "\""<< std::endl;
    }


    inline char getc()
    {
        if (pos == n)
            readBuffer();
        if (n == 0)
            return 0;
        
        return buffer[pos++];
    }

    inline char peek()
    {
        if (pos == n)
            readBuffer();
        if (n == 0)
            return 0;

        return buffer[pos];
    }

    inline ulong getulong()
    {
        uchar c = getc();
        if (c >= (1u << 7))
            return (c ^ (1lu << 7));

        ulong u = 0;
        for (uchar i = 0; i < c; ++i)
        {
            ulong j = getc();
            u |= (j & 0xFFlu) << (8*i); // FIXME Should use CHAR_BIT
        }
        
/*        char c = getc();
        if (c != 'F')
        {
            std::cerr << "Expecting F byte" << std::endl;
            std::exit(1);
        }
        ulong u = 0;
        for (size_t i = 0; i < sizeof(ulong); ++i)
        {
            ulong j = getc();
            u |= (j & 0xFFlu) << (8*i); // FIXME Should use CHAR_BIT
            }*/
        return u;
    }

    inline std::string getstring()
    {
        std::string str;
        char c = getc();
        while (c != '.')
        {
            str += c;
            c = getc();
        }
        return str;
    }

    bool good()
    { return !eof; } 

    void writeHalt(ulong n, ulong depth)
    {
//        std::cerr << "sent message n = " << n << ", depth = " << depth << std::endl;
        ulong c[2];
        c[0] = ((ulong)'H') | (depth << 8);
        c[1] = n;
        writenonblocking(sockfd, (const char *)&c, sizeof(ulong)*2);
    }

protected:
    explicit ServerSocket(int);

    ssize_t readn(int fd, char *vptr, size_t n);
    void writenonblocking(int fd, const char *vptr, size_t n);
    void readBuffer();

    static const unsigned BUFFER_SIZE = 8*1024; //1024*1024;

    char buffer[BUFFER_SIZE];
    unsigned pos; // current read position in buffer
    unsigned n;   // number of bytes in buffer
    int sockfd;
    bool eof;

private:
    ServerSocket();
    // No copy constructor and assignment
    ServerSocket(ServerSocket const&);
    ServerSocket& operator = (ServerSocket const&);
};

#endif // _ServerSocket_H_
