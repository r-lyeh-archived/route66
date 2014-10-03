#include <cassert>
#include <iostream>
#include "route66.hpp"

int GET( route66::request &request, std::ostream &headers, std::ostream &contents ) {
    headers << route66::mime(".html");
    contents << "<html><body><a href='/hello'>#</a></body></html>";
    return 200;
}

int GET_hello( route66::request &request, std::ostream &headers, std::ostream &contents ) {
    headers << route66::mime(".txt");
    contents << "hello world!";
    return 200;
}

int GET_echo( route66::request &request, std::ostream &headers, std::ostream &contents ) {
    headers << route66::mime(".txt");
    contents << request.url;
    return 200;
}

int main() {
    // create index route service
    assert(
        route66::create(8080, "GET /", GET )
    );

    // create hello world route service
    assert(
        route66::create(8080, "GET /hello*", GET_hello )
    );

    // create echo route service
    assert(
        route66::create(8080, "GET /echo*", GET_echo )
    );

    // do whatever in your app. notice that you could change any route behavior in runtime.
    std::cout << "server ready at localhost:8080 (/, /hello*, /echo*)" << std::endl;
    for( char ch ; std::cin >> ch ; )
    {}
}
