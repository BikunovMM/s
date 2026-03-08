# http/1.1 web server
Written in C99.

## Used libs:
- libuv  (for event-loop and IO)
- llhttp (for http 1.1 parse)

## Build:
1) cmake

    example: cmake -DCMAKE_BUILD_TYPE=Release

2) make

    example: make

## Supported OS:
- Windows 10
- Unix based

### TODO:
- add https/1.1 support
- add http/2 support
