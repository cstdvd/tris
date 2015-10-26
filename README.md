# Tris
## Description
The project consists of the development of a client/server. 
Both the server and the client must be mono-process and exploit the I/O multiplexing to handle multiple inputs and outputs simultaneously. 
The application to be developed is the game of TRIS following the paradigm peer2peer.

During a game each player has one of the two symbols of the game: 'X' or 'O'.
The server will be responsible for storing the users connected and the ports that will listen .
The exchange of information between client and server will be through TCP sockets. 
This information will only control information that will be used to implement communication peer2peer. 
The exchange of messages between clients will be via UDP socket.

## Client
The client must be started with the following syntax
```
./tris_client <server address> <port>
```
The commands available to the user should be:
```
!help - displays a list of available commands
!who - displays a list of connected users
!connect <username> - start a match with the user username
!disconnect - disconnects the client from the current game
!show_map - displays the game's map
!hit <num_cell> - tick checkbox num_cell (valid only when it is your turn)
!quit - disconnects the client from the server
```
While connecting the client has to enter his username and listen port for UDP commands for the game.

## Server
The program tris_server handles requests from clients. 
The server accepts new connections tris_server tcp, registers new users and manages the demands of various 
clients to open new matches.

The command syntax is as follows:
```
./tris_server <address> <port>
```
