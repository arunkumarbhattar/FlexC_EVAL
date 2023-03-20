CoreHTTP TCP/IP HTTP Server
by Frank Yaul (frank723@gmail.com) Fri 5 Aug 2005
Licensed under the Academic Free License version 1.2
File: readme.txt
===========================================

Documentation
----------------------------
readme.txt     
	This file - contains description and program idea.
changelog.txt  
	Version description. Make SURE to read the notes for the
	current version, as features described in other
	documentation may or may NOT be implemented!
corehttp.7
	Manual page for corehttp. After typing make install,
	you may view this using "man corehttp" and before this
	you can just do man -l corehttp.7
afl.txt
	The Academic Free License under which this software is distributed.

Quick Description
----------------------------
This is a minimalist HTTP server using the single-process server concurrency model and non-blocking sockets for optimal memory usage and speed. It is not designed to be fleshed out with features; merely able to serve pages quickly and reliably. 

Project Case
----------------------------
This HTTP server focuses on portability, reliability, speed, and will be developed with a minimalist philosophy. It will not flaunt the features of other larger servers such as Apache, but will contain a minimum amount of features required to present web content. Security and speed are the two main pillars. The code (in C) will be modular enough as to work with common CGI applications, and the server will use the single process with non-blocking I/O model for concurrency.

The intended end product audience is also different from other "compact" HTTP servers such as thttpd. Those servers have support for multiple users and bandwidth throttling, keeping the idea that the user might want to start a website hosting service. This server is more minimal than that - perhaps someone owns a regular PC and decides he wants to host his own website - and nothing more. Or, perhaps the simplicity could help someone understand how the web works. 

The server is a bare-bones server, meant to do nothing more than serve files through HTTP efficiently.

Concept
---------------------------
The main loop is basically a round-robin scheduler, alloting an equal amount of processing time (basically one non-blocking run through) for each socket. All sockets, whether server sockets that are listening or client sockets that are already communicating are stored in the same data structure, which is called a sprocket. Each open socket is handled by the program as a sprocket.

The sprocket has settings in it to determine what the function of the socket is - to be a server or client socket - and settings to say what state the socket is in - like a server can be in the listening state, and a client can be in the transferring or idle states. There are many states not mentioned but it should be noted these states have nothing to do with the socket itself, but rather the protocol. For example, in HTTP, a client socket would be in the recieve state until the characters '\r\n\r\n' are recieved. Each processing slice of time during the recieve state would be spent on a non-blocking poll of the socket for more data to recieve. 

A serving sprocket in the listening state would have its processing time alloted to do a non-blocking poll of its port and would create a new client sprocket and add it to the linked list of sprockets. All of the sprockets are linked together into a double-linked-list structure. This way, it is easy to add new sprockets, remove sprockets, and also use select(). They are also linked together as parent-child - the parent being the sprocket that created the child sprocket, so they can communicate in a manager-worker thread style.

For I/O polling, this server uses one select() function which checks all open file descriptors. This server's front end for the select function are the "watch" functions which basically set and unset file descriptors to watch, and keep track of the highest file descriptor value, which is the most annoying "feature" of select(). Select is only called once per loop. After select is called, the program then cycles through all of the sprockets and handles them accordingly.
 
The server does not use threads - it is a single process.  On a side note, the server doesn't really use non-blocking sockets, though before any I/O operation it polls the socket for readiness, thus simulating non-blocking I/O. 
