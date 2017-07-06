# exo-nico
for test purpose. It make an "echo" function between a client and the server

## How to build
To build exo-nico project run:
```bash
./autogen.sh 
./configure
make
make check # to run self-test
```
## Protocol 

The Server respond the same message to the client using the same way (stream or mailbox).

Ex : 
    If a client send a message with "HELLO" on a Stream, the server respond "HELLO"
    on the stream

If a NULL message is send, the server reply with the message "NULLVALUE"

##Architecture
They are an actor with 2 clients, one to send stream message, the second send mailbox message 

