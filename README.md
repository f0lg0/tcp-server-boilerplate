# tcp-server-boilerplate

Boilerplate of a TCP Server written in C++ using the epoll API.

## Requirements

-   cmake (if you want to use the provided `build.sh` script)

## Run

```
bash build.sh
```

## Streaming process

The server must receive messages with the size expressed as a 4 byte integer prepended to the full message:

```
hello --> 0005hello
```
