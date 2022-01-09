# tcp-server-boilerplate

Boilerplate of a TCP Server written in C++ using the epoll API.

## Requirements

-   cmake (if you want to use the provided `build.sh` script)

## Run

Server:

```
bash build.sh
./bin/server
```

Client (spawns 10 concurrent clients):

```
python src/client.py
```

## Streaming process

The server must receive messages with the size expressed as a 4 byte integer prepended to the full message:

```
hello --> 0005hello
```

This way the server can allocate a buffer big enough to receive the message.

## Internal workflow

The server works in the following way

![diagram](./assets/server.png)
