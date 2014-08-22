// lightweight http server
// - rlyeh, mit licensed. based on code by Ivan Tikhonov (zlib licensed)

// @todo : arguments

#include <thread>
#include <functional>
#include <string>
#include <map>

#include "route66.hpp"

// Sockets

#if defined(_WIN32)

#   include <winsock2.h>
#   include <ws2tcpip.h>
#   include <windows.h>

#   pragma comment(lib,"ws2_32.lib")

#   define INIT()                    do { static WSADATA wsa_data; static const int init = WSAStartup( MAKEWORD(2, 2), &wsa_data ); } while(0)
#   define SOCKET(A,B,C)             ::socket((A),(B),(C))
#   define ACCEPT(A,B,C)             ::accept((A),(B),(C))
#   define CONNECT(A,B,C)            ::connect((A),(B),(C))
#   define CLOSE(A)                  ::closesocket((A))
#   define RECV(A,B,C,D)             ::recv((A), (char *)(B), (C), (D))
#   define READ(A,B,C)               ::recv((A), (char *)(B), (C), (0))
#   define SELECT(A,B,C,D,E)         ::select((A),(B),(C),(D),(E))
#   define SEND(A,B,C,D)             ::send((A), (const char *)(B), (int)(C), (D))
#   define WRITE(A,B,C)              ::send((A), (const char *)(B), (int)(C), (0))
#   define GETSOCKOPT(A,B,C,D,E)     ::getsockopt((A),(B),(C),(char *)(D), (int*)(E))
#   define SETSOCKOPT(A,B,C,D,E)     ::setsockopt((A),(B),(C),(char *)(D), (int )(E))

#   define BIND(A,B,C)               ::bind((A),(B),(C))
#   define LISTEN(A,B)               ::listen((A),(B))
#   define SHUTDOWN(A)               ::shutdown((A),2)
#   define SHUTDOWN_R(A)             ::shutdown((A),0)
#   define SHUTDOWN_W(A)             ::shutdown((A),1)

    namespace
    {
        // fill missing api

        enum
        {
            F_GETFL = 0,
            F_SETFL = 1,

            O_NONBLOCK = 128 // dummy
        };

        int fcntl( int &sockfd, int mode, int value )
        {
            if( mode == F_GETFL ) // get socket status flags
                return 0; // original return current sockfd flags

            if( mode == F_SETFL ) // set socket status flags
            {
                u_long iMode = ( value & O_NONBLOCK ? 0 : 1 );

                bool result = ( ioctlsocket( sockfd, FIONBIO, &iMode ) == NO_ERROR );

                return 0;
            }

            return 0;
        }
    }

#else

#   include <fcntl.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netdb.h>
#   include <unistd.h>    //close

#   include <arpa/inet.h> //inet_addr

#   define INIT()                    do {} while(0)
#   define SOCKET(A,B,C)             ::socket((A),(B),(C))
#   define ACCEPT(A,B,C)             ::accept((A),(B),(C))
#   define CONNECT(A,B,C)            ::connect((A),(B),(C))
#   define CLOSE(A)                  ::close((A))
#   define READ(A,B,C)               ::read((A),(B),(C))
#   define RECV(A,B,C,D)             ::recv((A), (void *)(B), (C), (D))
#   define SELECT(A,B,C,D,E)         ::select((A),(B),(C),(D),(E))
#   define SEND(A,B,C,D)             ::send((A), (const int8 *)(B), (C), (D))
#   define WRITE(A,B,C)              ::write((A),(B),(C))
#   define GETSOCKOPT(A,B,C,D,E)     ::getsockopt((int)(A),(int)(B),(int)(C),(      void *)(D),(socklen_t *)(E))
#   define SETSOCKOPT(A,B,C,D,E)     ::setsockopt((int)(A),(int)(B),(int)(C),(const void *)(D),(int)(E))

#   define BIND(A,B,C)               ::bind((A),(B),(C))
#   define LISTEN(A,B)               ::listen((A),(B))
#   define SHUTDOWN(A)               ::shutdown((A),SHUT_RDWR)
#   define SHUTDOWN_R(A)             ::shutdown((A),SHUT_RD)
#   define SHUTDOWN_W(A)             ::shutdown((A),SHUT_WR)

#endif

namespace route66 {

std::string mime::html() { return "Content-Type: text/html; charset=utf-8\r\n"; }
std::string mime::text() { return "Content-Type: text/plain; charset=utf-8\r\n"; }
std::string mime::json() { return "Content-Type: application/json; charset=utf-8\r\n"; }

struct daemon {
    const int socket;
    const std::map< std::string /*path*/, std::function<void(route66::request &request)> /*fn*/ > *routers;

    void operator()() {
        bool shutdown = false;
        while( !shutdown ) {
            int child = ACCEPT(socket,0,0), o = 0, h[2], hi = 0;
            if( child < 0 ) continue;
            char b[1024];
            while(hi<2&&o<1024) {
                int n = READ(child,b+o,sizeof(b)-o);
                if(n<=0) { break; }
                else {
                    int i = o;
                    o+=n;
                    for(;i<n&&hi<2;i++) {
                    if((hi < 2) && b[i] == ' ') { h[hi++] = i; }
                    }
                }
            }
            if(hi == 2) {
                b[ o ] = '\0';

                char org = b[h[1]];
                b[h[1]] = '\0';
                /*
                std::cout << h[0] << std::endl;
                std::cout << h[1] << std::endl;
                */
                std::string url( &b[h[0]+1] ); //skip "GET "
                b[h[1]] = org;

                std::string path, options;

                auto mark = url.find_first_of('?');
                if( mark == std::string::npos ) {
                    path = url;
                } else {
                    path = url.substr( 0, mark );
                    options = url.substr( mark );
                }
                /*
                std::cout << "url: " << url << std::endl;
                std::cout << "path: " << path << std::endl;
                std::cout << "options: " << options << std::endl;
                */
                auto found = routers->find( path );
                if( found == routers->end() ) {
                    // bad route, try index fallback
                    found = routers->find("/");
                }

                if( found != routers->end() ) {
                    route66::request rq;
                    rq.url = url;
                    rq.path = path;
                    rq.options = options;
                    // @todo : arguments
                    rq.shutdown = [&]() {
                        shutdown = true;
                    };
                    rq.answer = [&]( unsigned httpcode, const std::string &headers, const std::string &content ) {
                        std::string head = std::string("HTTP/1.1 ") + std::to_string(httpcode) + " OK\r\n";
                        std::string heading = headers +
                            "Content-Length: " + std::to_string( content.size() ) + "\r\n"
                            "\r\n";

                        WRITE( child, head.c_str(), head.size() );
                        WRITE( child, heading.c_str(), heading.size() );
                        WRITE( child, content.c_str(), content.size() );

                        return true;
                    };

                    auto &fn = found->second;
                    fn( rq );
                }

                SHUTDOWN(child);
                CLOSE(child);
            }
        }
    }
};

bool create( unsigned port, const std::string &path, const std::function<void(route66::request &request)> &fn ) {
    using routers = std::map< std::string /*path*/, std::function<void(route66::request &request)> /*fn */>;
    static std::map< unsigned, routers > daemons;

    if( daemons.find(port) == daemons.end() ) {
        INIT();
        int socket = SOCKET(PF_INET, SOCK_STREAM, IPPROTO_IP);
        if( socket < 0 ) {
            return "cannot init socket", false;
        }

        struct sockaddr_in l;
        memset( &l, 0, sizeof(sockaddr_in) );
        l.sin_family = AF_INET;
        l.sin_port = htons(port);
        l.sin_addr.s_addr = INADDR_ANY;
    #if !defined(_WIN32)
        int r = 1; setsockopt(socket,SOL_SOCKET,SO_REUSEADDR,&r,sizeof(r));
    #endif
        BIND(socket,(struct sockaddr *)&l,sizeof(l));
        LISTEN(socket,5);

        std::thread( route66::daemon{ socket, &daemons[ port ] } ).detach();
    }

    daemons[ port ][ path ] = fn;
    return "ok", true;
}

}
