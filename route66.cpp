// lightweight http server
// - rlyeh, BOOST licensed. based on code by Ivan Tikhonov (zlib licensed)

// @todo : arguments

#include <thread>
#include <functional>
#include <string>
#include <map>
#include <sstream>

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
    const std::map< std::string /*path*/, route66::callback /*fn*/ > *routers;

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

                    auto &fn = found->second;
                    std::stringstream headers, contents;
                    int httpcode = fn( rq, headers, contents );

                    std::string datas = contents.str();
                    shutdown = ( 0 == httpcode ? true : false );

                    if( !shutdown && datas.size() ) {
                        std::string head1 = std::string("HTTP/1.1 ") + std::to_string(httpcode) + " OK\r\n";
                        std::string head2 = headers.str() +
                            "Content-Length: " + std::to_string( datas.size() ) + "\r\n"
                            "\r\n";

                        WRITE( child, head1.c_str(), head1.size() );
                        WRITE( child, head2.c_str(), head2.size() );
                        WRITE( child, datas.c_str(), datas.size() );
                    }
                }

                SHUTDOWN(child);
                CLOSE(child);
            }
        }
    }
};

bool create( unsigned port, const std::string &path, const route66::callback &fn ) {
    using routers = std::map< std::string /*path*/, route66::callback /*fn */>;
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

#undef SHUTDOWN

/*/ 1XX informational
/*/ httpcode SHUTDOWN(){ return httpcode { 0, "Shutdown requested" }; }
/*/ 1XX informational
/*/ httpcode CONTINUE(){ return httpcode { 100, "The request can be continued." }; } /*/
/*/ httpcode SWITCH_PROTOCOLS(){ return httpcode { 101, "The server has switched protocols in an upgrade header." }; }
/*/ 2XX success
/*/ httpcode OK(){ return httpcode { 200, "The request completed successfully." }; } /*/
/*/ httpcode CREATED(){ return httpcode { 201, "The request has been fulfilled and resulted in the creation of a new resource." }; } /*/
/*/ httpcode ACCEPTED(){ return httpcode { 202, "The request has been accepted for processing, but the processing has not been completed." }; } /*/
/*/ httpcode PARTIAL(){ return httpcode { 203, "The returned meta information in the entity-header is not the definitive set available from the originating server." }; } /*/
/*/ httpcode NO_CONTENT(){ return httpcode { 204, "The server has fulfilled the request, but there is no new information to send back." }; } /*/
/*/ httpcode RESET_CONTENT(){ return httpcode { 205, "The request has been completed, and the client program should reset the document view that caused the request to be sent to allow the user to easily initiate another input action." }; } /*/
/*/ httpcode PARTIAL_CONTENT(){ return httpcode { 206, "The server has fulfilled the partial GET request for the resource." }; } /*/
/*/ httpcode WEBDAV_MULTI_STATUS(){ return httpcode { 207, "During a World Wide Web Distributed Authoring and Versioning (WebDAV) operation, this indicates multiple status httpcodes for a single response. The response body contains Extensible Markup Language (XML) that describes the status httpcodes. For more information, see HTTP Extensions for Distributed Authoring." }; }
/*/ 3XX redirection
/*/ httpcode AMBIGUOUS(){ return httpcode { 300, "The requested resource is available at one or more locations." }; } /*/
/*/ httpcode MOVED(){ return httpcode { 301, "The requested resource has been assigned to a new permanent Uniform Resource Identifier (URI), and any future references to this resource should be done using one of the returned URIs." }; } /*/
/*/ httpcode REDIRECT(){ return httpcode { 302, "The requested resource resides temporarily under a different URI." }; } /*/
/*/ httpcode REDIRECT_METHOD(){ return httpcode { 303, "The response to the request can be found under a different URI and should be retrieved using a GET HTTP verb on that resource." }; } /*/
/*/ httpcode NOT_MODIFIED(){ return httpcode { 304, "The requested resource has not been modified." }; } /*/
/*/ httpcode USE_PROXY(){ return httpcode { 305, "The requested resource must be accessed through the proxy given by the location field." }; } /*/
/*/ httpcode REDIRECT_KEEP_VERB(){ return httpcode { 307, "The redirected request keeps the same HTTP verb. HTTP/1.1 behavior." }; }
/*/ 4XX client's fault
/*/ httpcode BAD_REQUEST(){ return httpcode { 400, "The request could not be processed by the server due to invalid syntax." }; } /*/
/*/ httpcode DENIED(){ return httpcode { 401, "The requested resource requires user authentication." }; } /*/
/*/ httpcode PAYMENT_REQ(){ return httpcode { 402, "Not implemented in the HTTP protocol." }; } /*/
/*/ httpcode FORBIDDEN(){ return httpcode { 403, "The server understood the request, but cannot fulfill it." }; } /*/
/*/ httpcode NOT_FOUND(){ return httpcode { 404, "The server has not found anything that matches the requested URI." }; } /*/
/*/ httpcode BAD_METHOD(){ return httpcode { 405, "The HTTP verb used is not allowed." }; } /*/
/*/ httpcode NONE_ACCEPTABLE(){ return httpcode { 406, "No responses acceptable to the client were found." }; } /*/
/*/ httpcode PROXY_AUTH_REQ(){ return httpcode { 407, "Proxy authentication required." }; } /*/
/*/ httpcode REQUEST_TIMEOUT(){ return httpcode { 408, "The server timed out waiting for the request." }; } /*/
/*/ httpcode CONFLICT(){ return httpcode { 409, "The request could not be completed due to a conflict with the current state of the resource. The user should resubmit with more information." }; } /*/
/*/ httpcode GONE(){ return httpcode { 410, "The requested resource is no longer available at the server, and no forwarding address is known." }; } /*/
/*/ httpcode LENGTH_REQUIRED(){ return httpcode { 411, "The server cannot accept the request without a defined content length." }; } /*/
/*/ httpcode PRECOND_FAILED(){ return httpcode { 412, "The precondition given in one or more of the request header fields evaluated to false when it was tested on the server." }; } /*/
/*/ httpcode REQUEST_TOO_LARGE(){ return httpcode { 413, "The server cannot process the request because the request entity is larger than the server is able to process." }; } /*/
/*/ httpcode URI_TOO_LONG(){ return httpcode { 414, "The server cannot service the request because the request URI is longer than the server can interpret." }; } /*/
/*/ httpcode UNSUPPORTED_MEDIA(){ return httpcode { 415, "The server cannot service the request because the entity of the request is in a format not supported by the requested resource for the requested method." }; } /*/
/*/ httpcode RETRY_WITH(){ return httpcode { 449, "The request should be retried after doing the appropriate action." }; }
/*/ 5XX server's fault
/*/ httpcode SERVER_ERROR(){ return httpcode { 500, "The server encountered an unexpected condition that prevented it from fulfilling the request." }; } /*/
/*/ httpcode NOT_SUPPORTED(){ return httpcode { 501, "The server does not support the functionality required to fulfill the request." }; } /*/
/*/ httpcode BAD_GATEWAY(){ return httpcode { 502, "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request." }; } /*/
/*/ httpcode SERVICE_UNAVAIL(){ return httpcode { 503, "The service is temporarily overloaded." }; } /*/
/*/ httpcode GATEWAY_TIMEOUT(){ return httpcode { 504, "The request was timed out waiting for a gateway." }; } /*/
/*/ httpcode VERSION_NOT_SUP(){ return httpcode { 505, "The server does not support the HTTP protocol version that was used in the request message." }; }

}
