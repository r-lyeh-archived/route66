// lightweight http server
// - rlyeh, zlib/libpng licensed. based on code by Ivan Tikhonov (zlib licensed)

/**/// <-- remove an asterisk in a code editor to highlight API

#pragma once

#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <sstream>

// taken from https://github.com/r-lyeh/bridge {
#if (__cplusplus < 201103L && !defined(_MSC_VER)) || (defined(_MSC_VER) && (_MSC_VER < 1700)) || (defined(__GLIBCXX__) && __GLIBCXX__ < 20130322L)
#   include <boost/functional.hpp> // if old libstdc++ or msc libs are found, use boost::function
#   include <boost/function.hpp>   // 
#   include <boost/thread.hpp>     // and boost::thread
namespace std {
    namespace placeholders {
        //...
    }
    using namespace boost;
}
#else
#   include <functional>       // else assume modern c++11 and use std::function<> instead
#   include <mutex>            // and std::mutex
#   include <thread>           // and std::thread
#endif
// }

namespace route66 {

    /*/ request:: 'class'
    /*/
    struct request {
        std::string data;                              /*/      .data 'raw received data, as is' /*/
        std::string method;                            /*/    .method 'raw method ("GET HEAD POST PUT ...")' /*/
        std::string url;                               /*/       .url 'raw request ("/search?req1=blabla&req2=what%20ever")' /*/
        std::string uri;                               /*/      .path 'decoded path uri ("/search")' /*/
        std::string options;                           /*/   .options 'raw GET/POST options url ("req1=blabla&req2=what%20ever")' /*/
        std::map<std::string, std::string> arguments;  /*/ .arguments 'decoded options decoupled as arguments ( {"req1","blabla"}, {"req2","what ever"} )' /*/
        std::map<std::string, std::string> multipart;  /*/ .multipart 'multipart files when POST' /*/
        std::string ip;                                /*/        .ip 'ip address of request' /*/
           unsigned port;                              /*/      .port 'number port of request' /*/
        std::string str() const {
            typedef std::map<std::string, std::string>::const_iterator cit;
            std::stringstream ss;
            ss << "route66::request[" << this << "] = {" << std::endl;
            ss << "\t" "ip: " << ip << std::endl;
            ss << "\t" "port: " << port << std::endl;
            ss << "\t" "method: " << method << std::endl;
            ss << "\t" "options: " << options << std::endl;
            ss << "\t" "uri: " << uri << std::endl;
            ss << "\t" "url: " << url << std::endl;
            for( cit it = arguments.begin(), end = arguments.end(); it != end; ++it ) {
                ss << "\t" "argument[" << it->first << "]: " << it->second << std::endl;
            }
            for( cit it = multipart.begin(), end = multipart.end(); it != end; ++it ) {
                ss << "\t" "multipart[" << it->first << "]: " << it->second << std::endl;
            }
            ss << "\t" "data: " << data << std::endl;
            ss << "}; //route66::request [" << this << "]" << std::endl;
            return ss.str();
        }
    };

    /*/ callback; 'alias'
    /*/ typedef std::function< int(request &,std::ostream &,std::ostream &) > callback;

    /*/ create() 'start a daemon at given port and route mask, callback will be invoked; returns false if failed.'
    /*/ bool create( unsigned port, const std::string &mask, const route66::callback &callback );

    /*/ fserve() 'start a file server daemon at given port and relative path, all file uri contents will be returned; returns false if failed.'
    /*/ bool fserve( unsigned port, const std::string &mask, const std::string &relpath );

    /*/ httpcode:: 'listing of HTTP result codes'
    /*/
    struct httpcode {
        unsigned code;
        const char *const description;
    };

    /*/ 0XX 'route66'
    /*/
    extern const httpcode SHUTDOWN;
    /*/ 1XX 'informational'
    /*/
    extern const httpcode CONTINUE;
    extern const httpcode SWITCH_PROTOCOLS;
    /*/ 2XX 'success'
    /*/
    extern const httpcode OK;
    extern const httpcode CREATED;
    extern const httpcode ACCEPTED;
    extern const httpcode PARTIAL;
    extern const httpcode NO_CONTENT;
    extern const httpcode RESET_CONTENT;
    extern const httpcode PARTIAL_CONTENT;
    extern const httpcode WEBDAV_MULTI_STATUS;
    /*/ 3XX 'redirection'
    /*/
    extern const httpcode AMBIGUOUS;
    extern const httpcode MOVED;
    extern const httpcode REDIRECT;
    extern const httpcode REDIRECT_METHOD;
    extern const httpcode NOT_MODIFIED;
    extern const httpcode USE_PROXY;
    extern const httpcode REDIRECT_KEEP_VERB;
    /*/ 4XX 'client error'
    /*/
    extern const httpcode BAD_REQUEST;
    extern const httpcode DENIED;
    extern const httpcode PAYMENT_REQ;
    extern const httpcode FORBIDDEN;
    extern const httpcode NOT_FOUND;
    extern const httpcode BAD_METHOD;
    extern const httpcode NONE_ACCEPTABLE;
    extern const httpcode PROXY_AUTH_REQ;
    extern const httpcode REQUEST_TIMEOUT;
    extern const httpcode CONFLICT;
    extern const httpcode GONE;
    extern const httpcode LENGTH_REQUIRED;
    extern const httpcode PRECOND_FAILED;
    extern const httpcode REQUEST_TOO_LARGE;
    extern const httpcode URI_TOO_LONG;
    extern const httpcode UNSUPPORTED_MEDIA;
    extern const httpcode RETRY_WITH;
    /*/ 5XX 'server error'
    /*/
    extern const httpcode SERVER_ERROR;
    extern const httpcode NOT_SUPPORTED;
    extern const httpcode BAD_GATEWAY;
    extern const httpcode SERVICE_UNAVAIL;
    extern const httpcode GATEWAY_TIMEOUT;
    extern const httpcode VERSION_NOT_SUP;

    /*/ mime() 'MIME string header based on file extension (ie, sample.json -> "Content-Type: application/json; charset=utf-8\r\n" )' /*/
    /*/ tag()  'MIME string tag based on file extension (ie, sample.json -> "application/json" )' /*/
    std::string mime( const std::string &filename_ext );
    std::string tag( const std::string &filename_ext );
}

