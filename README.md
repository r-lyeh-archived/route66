route66
=======

- Route66 is a lightweight embeddable HTTP server. Written in C++11.
- Route66 is tiny. One header and one source file.
- Route66 is cross-platform.
- Route66 is MIT licensed.

### API
take a look into [route66.hpp header](route66.hpp)

### sample
```c++
#include <cassert>
#include <iostream>
#include <string>
#include "route66.hpp"

int main() {
    // create index route service
    assert(
        route66::create(8080, "/", [&]( route66::request &request ) {
            request.answer( 200, route66::mime::html(), "<html><body><a href='/hello'>#</a></body></html>" );
        } )
    );

    // create hello world route service
    assert(
        route66::create(8080, "/hello", [&]( route66::request &request ) {
            request.answer( 200, route66::mime::text(), "hello world!" );
        } )
    );

    // create echo route service
    assert(
        route66::create(8080, "/echo", [&]( route66::request &request ) {
            request.answer( 200, route66::mime::text(), request.url );
        } )
    );

    // do whatever in your app. notice that you could change routes behavior in runtime.
    std::cout << "server ready at localhost:8080 (/, /hello, /echo)" << std::endl;
    for( char ch ; std::cin >> ch ; )
    {}
}
```

