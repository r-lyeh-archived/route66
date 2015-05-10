// lightweight http server
// - rlyeh, zlib/libpng licensed. based on code by Ivan Tikhonov (zlib licensed)

// Sockets, taken from https://github.com/r-lyeh/knot

#if defined(_WIN32)

#   include <winsock2.h>
#   include <ws2tcpip.h>
#   include <windows.h>

#   ifndef _MSC_VER
    static
    const char* inet_ntop(int af, const void* src, char* dst, int cnt){
        struct sockaddr_in srcaddr;
        memset(&srcaddr, 0, sizeof(struct sockaddr_in));
        memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
        srcaddr.sin_family = af;
        if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
            DWORD rv = WSAGetLastError();
            return NULL;
        }
        return dst;
    }
#   endif

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

#   include <arpa/inet.h> //inet_addr, inet_ntop

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


#include <cstdlib>
#include <cstring>
#include <limits.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "route66.hpp"

namespace {
    // http://stackoverflow.com/questions/2673207/c-c-url-decode-library
    char * urldecode2(char *dst, const char *src) {
        char a, b;
        while (*src) {
            if ((*src == '%') &&
                ((a = src[1]) && (b = src[2])) &&
                (isxdigit(a) && isxdigit(b))) {
                    if (a >= 'a')
                            a -= 'a'-'A';
                    if (a >= 'A')
                            a -= ('A' - 10);
                    else
                            a -= '0';
                    if (b >= 'a')
                            b -= 'a'-'A';
                    if (b >= 'A')
                            b -= ('A' - 10);
                    else
                            b -= '0';
                    *dst++ = 16*a+b;
                    src+=3;
            } else {
                *dst++ = *src++;
            }
        }
        *dst++ = '\0';
        return dst - 1;
    }

    std::string &urldecode2( std::string &url ) {
        char *begin = &url[0], *end = urldecode2( begin, begin );
        url.resize( end - begin );
        return url;
    }

    // taken from https://github.com/r-lyeh/wire
    std::string replace( std::string s, const std::string &target, const std::string &repl ) {
        for( size_t it = 0, tlen = target.length(), rlen = repl.length(); ( it = s.find( target, it ) ) != std::string::npos; it += rlen ) {
            s.replace( it, tlen, repl );
        }
        return s;
    };    
    std::vector< std::string > tokenize( const std::string &self, const std::string &delimiters ) {
        std::string map( 256, '\0' );
        for( std::string::const_iterator it = delimiters.begin(), end = delimiters.end(); it != end; ++it ) {
            unsigned char ch( *it );
            map[ ch ] = '\1';
        }
        std::vector< std::string > tokens(1);
        for( std::string::const_iterator it = self.begin(), end = self.end(); it != end; ++it ) {
            unsigned char ch( *it );
            /**/ if( !map.at(ch)          ) tokens.back().push_back( char(ch) );
            else if( tokens.back().size() ) tokens.push_back( std::string() );
        }
        while( tokens.size() && !tokens.back().size() ) tokens.pop_back();
        return tokens;
    }
    bool match( const char *pattern, const char *str ) {
        if( *pattern=='\0' ) return !*str;
        if( *pattern=='*' )  return match(pattern+1, str) || (*str && match(pattern, str+1));
        if( *pattern=='?' )  return *str && (*str != '.') && match(pattern+1, str+1);
        return (*str == *pattern) && match(pattern+1, str+1);
    }
    bool matches( const std::string &self, const std::string &pattern ) {
        return match( pattern.c_str(), self.c_str() );
    }
    bool endswith( const std::string &self, const std::string &eof ) {
        return eof.size() < self.size() && self.substr( self.size() - eof.size() ) == eof;
    }
}

namespace route66 {

std::string tag( const std::string &pathfile ) {
    static std::map< std::string, std::string> mimes;
    if( mimes.empty() ) {
        // [ref] taken from http://en.wikipedia.org/wiki/Internet_media_type

        mimes[".avi"] = "video/avi";                  // Covers most Windows-compatible formats including .avi and .divx[16]
        mimes[".mpg"] = "video/mpeg";                 // MPEG-1 video with multiplexed audio; Defined in RFC 2045 and RFC 2046
        mimes[".mpeg"] = "video/mpeg";                // MPEG-1 video with multiplexed audio; Defined in RFC 2045 and RFC 2046
        mimes[".mp4"] = "video/mp4";                  // MP4 video; Defined in RFC 4337
        mimes[".mov"] = "video/quicktime";            // QuickTime video; Registered[17]
        mimes[".webm"] = "video/webm";                // WebM Matroska-based open media format
        mimes[".mkv"] = "video/x-matroska";           // Matroska open media format
        mimes[".wmv"] = "video/x-ms-wmv";             // Windows Media Video; Documented in Microsoft KB 288102
        mimes[".flv"] = "video/x-flv";                // Flash video (FLV files)

        mimes[".css"] = "text/css";                   // Cascading Style Sheets; Defined in RFC 2318
        mimes[".csv"] = "text/csv";                   // Comma-separated values; Defined in RFC 4180
        mimes[".htm"] = "text/html";                  // HTML; Defined in RFC 2854
        mimes[".html"] = "text/html";                 // HTML; Defined in RFC 2854
        mimes[".txt"] = "text/plain";                 // Textual data; Defined in RFC 2046 and RFC 3676
        mimes[".text"] = "text/plain";                // Textual data; Defined in RFC 2046 and RFC 3676
        mimes[".rtf"] = "text/rtf";                   // RTF; Defined by Paul Lindner
        mimes[".xml"] = "text/xml";                   // Extensible Markup Language; Defined in RFC 3023
        mimes[".tsv"] = "text/tab-separated-values";

        mimes[".gif"] = "image/gif";                  // GIF image; Defined in RFC 2045 and RFC 2046
        mimes[".jpg"] = "image/jpeg";                 // JPEG JFIF image; Defined in RFC 2045 and RFC 2046
        mimes[".jpeg"] = "image/jpeg";                // JPEG JFIF image; Defined in RFC 2045 and RFC 2046
        mimes[".pjpg"] = "image/pjpeg";               // JPEG JFIF image; Associated with Internet Explorer; Listed in ms775147(v=vs.85) - Progressive JPEG, initiated before global browser support for progressive JPEGs (Microsoft and Firefox).
        mimes[".pjpeg"] = "image/pjpeg";              // JPEG JFIF image; Associated with Internet Explorer; Listed in ms775147(v=vs.85) - Progressive JPEG, initiated before global browser support for progressive JPEGs (Microsoft and Firefox).
        mimes[".png"] = "image/png";                  // Portable Network Graphics; Registered,[13] Defined in RFC 2083
        mimes[".svg"] = "image/svg+xml";              // SVG vector image; Defined in SVG Tiny 1.2 Specification Appendix M
        mimes[".psd"] = "image/vnd.adobe.photoshop";

        mimes[".aif"] = "audio/x-aiff";
        mimes[".s3m"] = "audio/s3m";
        mimes[".xm"] = "audio/xm";
        mimes[".snd"] = "audio/x-adpcm";
        mimes[".mp3"] = "audio/mpeg3";
        mimes[".ogg"] = "audio/ogg";                  // Ogg Vorbis, Speex, Flac and other audio; Defined in RFC 5334
        mimes[".opus"] = "audio/opus";                // Opus audio
        mimes[".vorbis"] = "audio/vorbis";            // Vorbis encoded audio; Defined in RFC 5215
        mimes[".wav"] = "audio/x-wav";

        mimes[".atom"] = "application/atom+xml";      // Atom feeds
        mimes[".dart"] = "application/dart";          // Dart files [8]
        mimes[".ejs"] = "application/ecmascript";     // ECMAScript/JavaScript; Defined in RFC 4329 (equivalent to application/javascript but with stricter processing rules)
        mimes[".json"] = "application/json";          // JavaScript Object Notation JSON; Defined in RFC 4627
        mimes[".js"] = "application/javascript";      // ECMAScript/JavaScript; Defined in RFC 4329 (equivalent to application/ecmascript but with looser processing rules) It is not accepted in IE 8 or earlier - text/javascript is accepted but it is defined as obsolete in RFC 4329. The "type" attribute of the <script> tag in HTML5 is optional. In practice, omitting the media type of JavaScript programs is the most interoperable solution, since all browsers have always assumed the correct default even before HTML5.
        mimes[".bin"] = "application/octet-stream";   // Arbitrary binary data.[9] Generally speaking this type identifies files that are not associated with a specific application. Contrary to past assumptions by software packages such as Apache this is not a type that should be applied to unknown files. In such a case, a server or application should not indicate a content type, as it may be incorrect, but rather, should omit the type in order to allow the recipient to guess the type.[10]
        mimes[".pdf"] = "application/pdf";            // Portable Document Format, PDF has been in use for document exchange on the Internet since 1993; Defined in RFC 3778
        mimes[".ps"] = "application/postscript";      // PostScript; Defined in RFC 2046
        mimes[".rdf"] = "application/rdf+xml";        // Resource Description Framework; Defined by RFC 3870
        mimes[".rss"] = "application/rss+xml";        // RSS feeds
        mimes[".soap"] = "application/soap+xml";      // SOAP; Defined by RFC 3902
        mimes[".font"] = "application/font-woff";     // Web Open Font Format; (candidate recommendation; use application/x-font-woff until standard is official)
        mimes[".xhtml"] = "application/xhtml+xml";    // XHTML; Defined by RFC 3236
        mimes[".xml"] = "application/xml";            // XML files; Defined by RFC 3023
        mimes[".zip"] = "application/zip";            // ZIP archive files; Registered[11]
        mimes[".gz"] = "application/gzip";            // Gzip, Defined in RFC 6713
        mimes[".nacl"] = "application/x-nacl";        // Native Client web module (supplied via Google Web Store only)
        mimes[".pnacl"] = "application/x-pnacl";      // Portable Native Client web module (may supplied by any website as it is safer than x-nacl)
    };

    size_t ext = pathfile.find_last_of(".");
    if( ext != std::string::npos ) {
        std::map<std::string, std::string>::const_iterator found = mimes.find( &pathfile[ext] );
        if( found != mimes.end() ) {
            return found->second;
        }
    }
    return mimes[".txt"];
}

std::string mime( const std::string &filename_ext ) {
    return std::string("Content-Type: ") + tag(filename_ext) + "; charset=utf-8\r\n";
};

std::string interval_from_headers( const std::string &header, const std::string &text1, const std::string &text2 ) {
    size_t pos = header.find(text1);
    if( pos == std::string::npos ) {
        return std::string();
    }
    pos += text1.size();
    size_t pos2 = header.find(text2, pos);
    if( pos2 == std::string::npos ) {
        return header.substr(pos);
    } else {
        return header.substr(pos, pos2-pos);
    }
}

std::string extract_from_headers( const std::string &header, const std::string &text ) {
    size_t pos = header.find(text);
    if( pos == std::string::npos ) {
        return std::string();
    }
    pos += text.size();
    size_t pos2 = header.find( "\r\n", pos );
    if( pos2 == std::string::npos ) {
        return header.substr(pos);
    } else {
        return header.substr(pos, pos2-pos);        
    }
}

struct daemon {
    int socket;
    const std::map< std::string /*path*/, route66::callback /*fn*/ > *routers;
    std::string base;

    daemon() : socket(-1), routers(0)
    {}

    void operator()() {
        bool shutdown = false;
        while( !shutdown ) {
            enum { buflen = 1024 * 1024, SPACES = 2 /*>=2*/ };

            int child = ACCEPT(socket,0,0), o = 0, h[SPACES] = {}, hi = 0;
            if( child < 0 ) continue;

            std::string buf( buflen, '\0' );
            char *b = &buf[0];

            while(hi<SPACES&&o<buflen) {
                int n = READ(child,b+o,buflen-o);
                if(n<=0) { break; }
                else {
                    int i = o;
                    o+=n;
                    for(;i<n&&hi<SPACES;i++) {
                    if((hi < SPACES) && b[i] == ' ') { h[hi++] = i; }
                    }
                }
            }

            if(hi >= SPACES) {
                b[ o ] = '\0';
                buf.resize( o );

                const char *logmask = std::getenv("R66LOG");
                if( logmask ) {
                    if( matches(b, logmask) ) {
                        //__asm int 3;
                        //fprintf(stdout, "%s,%s\n", multipart.c_str(), boundary.c_str());
                    }
                }

                typedef std::map< std::string, std::string > filemap;
                static std::map< std::string /*boundary*/, filemap > boundaries;

                unsigned curlen = 0;

                std::string boundary = std::string("--") + extract_from_headers( buf, "Content-Type: multipart/form-data; boundary=");
                std::string eof = boundary + "--\r\n";
                std::string headers = buf.substr( 0, buf.find("\r\n\r\n") + 4 );
                std::string payload = buf.substr( headers.size() );

                while( boundary.size() > 2 && !endswith( payload, eof ) ) {
                    std::stringstream head1, head2;
                    head1 << "HTTP/1.1 100 Continue\r\n";
                    WRITE( child, head1.str().c_str(), head1.str().size() );

                    o = 0;
                    buf.clear();
                    buf.resize( buflen, '\0' );

                    int n;
                    n = READ(child,b+o,buflen-o);
                    if(n<=0) { break; }

                    buf.resize( n );

                    payload += buf;
                }

                route66::request rq;

                if( boundary.size() > 2 ) {
                    payload = interval_from_headers( payload, boundary + "\r\n", boundary );

                    {
                        std::string headers = payload.substr( 0, payload.find("\r\n\r\n") + 4 );
                        std::string file = extract_from_headers( headers, "Content-Disposition: form-data; name=");
                        file = file.substr( 1, file.size() - 2 ); // remove ""
                         
                        payload = payload.substr( headers.size() );
                        rq.multipart[ file ] = payload;
                    }

                    buf = headers;
                }

                rq.data = buf;

                char org = b[h[1]];
                b[h[1]] = '\0';

#if 1
                socklen_t len;
                struct sockaddr_storage addr;
                char ipstr[INET_ADDRSTRLEN];
                len = sizeof addr;
                getpeername(child, (struct sockaddr*) &addr, &len);
                struct sockaddr_in *s = (struct sockaddr_in *) &addr;
                inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
                if( std::strcmp("127.0.0.1", ipstr) == 0 ) rq.ip = "localhost";
                else rq.ip = ipstr;

                struct sockaddr_in l;
                len = sizeof(l);
                if (getsockname(child, (struct sockaddr *)&l, &len) != -1) {
                    rq.port = ntohs(l.sin_port);
                } else {
                    rq.port = 0;
                }
#endif

                std::string method( b, h[0] );
                std::string url( &b[h[0]+1] ); //skip "GET "
                b[h[1]] = org;

                std::string path, options;

                size_t mark = url.find_first_of('?');
                if( mark == std::string::npos ) {
                    path = url;
                } else {
                    path = url.substr( 0, mark );
                    options = url.substr( mark + 1 );
                }

                if( method == "PUT" || method == "POST" )  {
                    options = &b[ std::string(b).find("\r\n\r\n") + 4 ];
                }

                std::string route = method + " " + path;
                std::map< std::string, route66::callback >::const_iterator found = routers->find( route );
                if( found == routers->end() ) {
                    // bad route, try index wildcard matching
                    for( found = routers->begin(); found != routers->end(); ++found ) {
                        if( matches( route, found->first ) ) {
                            break;
                        }
                    }
                }
                if( found == routers->end() ) {
                    // bad route, try index fallback
                    found = routers->find("GET /");
                }

                if( found != routers->end() ) {

                    rq.method = method;
                    rq.url = url;
                    rq.uri = path;
                    rq.options = options;

                    urldecode2( rq.uri );

                    // arguments
                    std::vector< std::string > split = tokenize( options, "&" );
                    for( std::vector< std::string >::const_iterator it = split.begin(), end = split.end(); it != end; ++it ) {
                        const std::string &glued = *it;
                        std::vector< std::string > pair = tokenize( glued, "=" );
                        if( 2 == pair.size() ) {
                            rq.arguments[ urldecode2(pair[0]) ] = urldecode2(pair[1]);
                        } else {
                            rq.arguments[ urldecode2(pair[0]) ] = std::string();
                        }
                    }

                    // normalize & rebase (if needed)
                    if( rq.uri == "" || rq.uri == "/" ) rq.uri = "index.html";
                    rq.uri = base + rq.uri;

                    if( logmask ) {
                        char addr[64]; sprintf(addr, "%s:%d", rq.ip.c_str(), rq.port );
                        if( matches(addr, logmask) || matches(rq.uri, logmask) || matches(rq.data, logmask) ) {
                            fprintf(stdout, "%s - %s\n%s\n", addr, rq.uri.c_str(), rq.str().c_str());
                        }
                    }

                    {                    
                        const route66::callback &fn = found->second;
                        std::stringstream headers, contents;
                        int httpcode = fn( rq, headers, contents );

                        std::string datas = contents.str();
                        shutdown = ( 0 == httpcode ? true : false );

                        if( !shutdown && datas.size() ) {
                            std::stringstream head1, head2;
                            head1 << "HTTP/1.1 " << httpcode << " OK\r\n";
                            head2 << headers.str() << "Content-Length: " << datas.size() << "\r\n\r\n";

                            WRITE( child, head1.str().c_str(), head1.str().size() );
                            WRITE( child, head2.str().c_str(), head2.str().size() );
                            if( method != "HEAD" )
                            WRITE( child, datas.c_str(), datas.size() );
                        }
                    }

                }

                SHUTDOWN(child);
                CLOSE(child);
            }
        }
    }
};

namespace {

    typedef std::map< std::string /*path*/, route66::callback /*fn */> routers;

    bool create( unsigned port, const std::string &mask, const route66::callback &fn, std::string base ) {
        static std::map< unsigned, routers > daemons;

        if( daemons.find(port) == daemons.end() ) {
            INIT();
            int socket = SOCKET(PF_INET, SOCK_STREAM, IPPROTO_IP);
            if( socket < 0 ) {
                return "cannot init socket", false;
            }

            struct sockaddr_in l;
            std::memset( &l, 0, sizeof(sockaddr_in) );
            l.sin_family = AF_INET;
            l.sin_port = htons(port);
            l.sin_addr.s_addr = INADDR_ANY;
    #       if !defined(_WIN32)
            int r = 1; setsockopt(socket,SOL_SOCKET,SO_REUSEADDR,&r,sizeof(r));
    #       endif
            BIND(socket,(struct sockaddr *)&l,sizeof(l));
            LISTEN(socket,5);

            route66::daemon dm;
            dm.socket = socket;
            dm.routers = &daemons[ port ];
            dm.base = base;
            std::thread( dm ).detach();
        }

        daemons[ port ][ mask ] = fn;

        return "ok", true;
    }
}

bool create( unsigned port, const std::string &mask, const route66::callback &fn ) {
    return create( port, mask, fn, std::string() );
}

namespace {

    std::string get_app_folder() {
#       ifdef _WIN32
            char s_path[MAX_PATH] = {0};
            const size_t len = MAX_PATH;

            HMODULE mod = 0;
            ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&get_app_folder, &mod);
            DWORD size = ::GetModuleFileNameA(mod, s_path, (DWORD)len);

            while( size > 0 ) {
                if( s_path[size] == '\\' ) {
                    s_path[size+1] = '\0';
                    return s_path;
                }
                --size;
            }
            return std::string();
#       else
            char arg1[20];
            char exepath[PATH_MAX + 1] = {0};

            sprintf( arg1, "/proc/%d/exe", getpid() );
            readlink( arg1, exepath, 1024 );
            return std::string( exepath );
#       endif
    }

    int file_reader( route66::request &request, std::ostream &headers, std::ostream &contents ) {
        std::ifstream ifs( request.uri.c_str(), std::ios::binary );
        if( ifs.good() ) {
            headers << route66::mime(request.uri);
            contents << ifs.rdbuf();
            return 200;
        } else {
            headers << route66::mime(".text");
            contents << "404: not found" << std::endl;
            //contents << request.uri << std::endl;
            return 404;
        }
    }
}

bool fserve( unsigned port, const std::string &mask, const std::string &path ) {
    return create( port, mask, file_reader, get_app_folder() + path );
}

#undef INIT
#undef SOCKET
#undef ACCEPT
#undef CONNECT
#undef CLOSE
#undef RECV
#undef READ
#undef SELECT
#undef SEND
#undef WRITE
#undef GETSOCKOPT
#undef SETSOCKOPT

#undef BIND
#undef LISTEN
#undef SHUTDOWN
#undef SHUTDOWN_R
#undef SHUTDOWN_W

/*/ 1XX informational
/*/ const httpcode SHUTDOWN = { 0, "Shutdown requested" };
/*/ 1XX informational
/*/ const httpcode CONTINUE = { 100, "The request can be continued." }; /*/
/*/ const httpcode SWITCH_PROTOCOLS = { 101, "The server has switched protocols in an upgrade header." };
/*/ 2XX success
/*/ const httpcode OK = { 200, "The request completed successfully." }; /*/
/*/ const httpcode CREATED = { 201, "The request has been fulfilled and resulted in the creation of a new resource." }; /*/
/*/ const httpcode ACCEPTED = { 202, "The request has been accepted for processing, but the processing has not been completed." }; /*/
/*/ const httpcode PARTIAL = { 203, "The returned meta information in the entity-header is not the definitive set available from the originating server." }; /*/
/*/ const httpcode NO_CONTENT = { 204, "The server has fulfilled the request, but there is no new information to send back." }; /*/
/*/ const httpcode RESET_CONTENT = { 205, "The request has been completed, and the client program should reset the document view that caused the request to be sent to allow the user to easily initiate another input action." }; /*/
/*/ const httpcode PARTIAL_CONTENT = { 206, "The server has fulfilled the partial GET request for the resource." }; /*/
/*/ const httpcode WEBDAV_MULTI_STATUS = { 207, "During a World Wide Web Distributed Authoring and Versioning (WebDAV) operation, this indicates multiple status const httpcodes for a single response. The response body contains Extensible Markup Language (XML) that describes the status const httpcodes. For more information, see HTTP Extensions for Distributed Authoring." };
/*/ 3XX redirection
/*/ const httpcode AMBIGUOUS = { 300, "The requested resource is available at one or more locations." }; /*/
/*/ const httpcode MOVED = { 301, "The requested resource has been assigned to a new permanent Uniform Resource Identifier (URI), and any future references to this resource should be done using one of the returned URIs." }; /*/
/*/ const httpcode REDIRECT = { 302, "The requested resource resides temporarily under a different URI." }; /*/
/*/ const httpcode REDIRECT_METHOD = { 303, "The response to the request can be found under a different URI and should be retrieved using a GET HTTP verb on that resource." }; /*/
/*/ const httpcode NOT_MODIFIED = { 304, "The requested resource has not been modified." }; /*/
/*/ const httpcode USE_PROXY = { 305, "The requested resource must be accessed through the proxy given by the location field." }; /*/
/*/ const httpcode REDIRECT_KEEP_VERB = { 307, "The redirected request keeps the same HTTP verb. HTTP/1.1 behavior." };
/*/ 4XX client's fault
/*/ const httpcode BAD_REQUEST = { 400, "The request could not be processed by the server due to invalid syntax." }; /*/
/*/ const httpcode DENIED = { 401, "The requested resource requires user authentication." }; /*/
/*/ const httpcode PAYMENT_REQ = { 402, "Not implemented in the HTTP protocol." }; /*/
/*/ const httpcode FORBIDDEN = { 403, "The server understood the request, but cannot fulfill it." }; /*/
/*/ const httpcode NOT_FOUND = { 404, "The server has not found anything that matches the requested URI." }; /*/
/*/ const httpcode BAD_METHOD = { 405, "The HTTP verb used is not allowed." }; /*/
/*/ const httpcode NONE_ACCEPTABLE = { 406, "No responses acceptable to the client were found." }; /*/
/*/ const httpcode PROXY_AUTH_REQ = { 407, "Proxy authentication required." }; /*/
/*/ const httpcode REQUEST_TIMEOUT = { 408, "The server timed out waiting for the request." }; /*/
/*/ const httpcode CONFLICT = { 409, "The request could not be completed due to a conflict with the current state of the resource. The user should resubmit with more information." }; /*/
/*/ const httpcode GONE = { 410, "The requested resource is no longer available at the server, and no forwarding address is known." }; /*/
/*/ const httpcode LENGTH_REQUIRED = { 411, "The server cannot accept the request without a defined content length." }; /*/
/*/ const httpcode PRECOND_FAILED = { 412, "The precondition given in one or more of the request header fields evaluated to false when it was tested on the server." }; /*/
/*/ const httpcode REQUEST_TOO_LARGE = { 413, "The server cannot process the request because the request entity is larger than the server is able to process." }; /*/
/*/ const httpcode URI_TOO_LONG = { 414, "The server cannot service the request because the request URI is longer than the server can interpret." }; /*/
/*/ const httpcode UNSUPPORTED_MEDIA = { 415, "The server cannot service the request because the entity of the request is in a format not supported by the requested resource for the requested method." }; /*/
/*/ const httpcode RETRY_WITH = { 449, "The request should be retried after doing the appropriate action." };
/*/ 5XX server's fault
/*/ const httpcode SERVER_ERROR = { 500, "The server encountered an unexpected condition that prevented it from fulfilling the request." }; /*/
/*/ const httpcode NOT_SUPPORTED = { 501, "The server does not support the functionality required to fulfill the request." }; /*/
/*/ const httpcode BAD_GATEWAY = { 502, "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request." }; /*/
/*/ const httpcode SERVICE_UNAVAIL = { 503, "The service is temporarily overloaded." }; /*/
/*/ const httpcode GATEWAY_TIMEOUT = { 504, "The request was timed out waiting for a gateway." }; /*/
/*/ const httpcode VERSION_NOT_SUP = { 505, "The server does not support the HTTP protocol version that was used in the request message." };

}
