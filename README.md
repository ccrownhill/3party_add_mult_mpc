# Implementation of 3 Party Secure Addition and Multiplication

This implements the protocols described on pages 8-14 of the book "Secure Multiparty Computation and Secret Sharing" by Ronald Cramer, Ivan Bierre Damgard and Jesper Buus Nielsen.

These parties will calculate the sum of all of their numbers and the product of the numbers of party 1 and party 2.

The different implementations differ in the way the 3 parties (every party is running on a different process) communicate with each other:

* `tcpip.c` uses TCP/IP connections between the parties on different ports of localhost.
* `unixsock.c` uses paired UNIX sockets with the `socketpair` system call. (see `man 7 unix` and `man socketpair`)

## Usage

```
tcpip p1_num p2_num p3_num
```

## Communication Alternatives

Basically all methods of UNIX Interprocess Communication can be used.

For a summary on them see [Beej's Guide to Interprocess Communication](http://beej.us/guide/bgipc/).

For example you could also use [FIFOs](http://beej.us/guide/bgipc/html/multi/fifos.html) aka named pipes.

## Compilation

Just use

```
gcc tcpip.c -o tcpip
```

or

```
gcc unixsock.c -o unixsock
