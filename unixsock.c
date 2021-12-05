#include <stdio.h>
#include <unistd.h> // for fork
#include <errno.h> // for errno var
#include <stdlib.h> // for exit and random things
#include <string.h> // for strerror
#include <sys/wait.h> // for waitpid
#include <time.h>
#include <sys/socket.h> // for socket fns

#define P 23 // prime number for all modulo calculations

void error(char *msg)
{
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

void usage()
{
  fprintf(stderr, "Usage: unixsock p1_num p2_num p3_num\n");
  exit(1);
}

void send_num(int socket, int num)
{
  if (send(socket, &num, sizeof(int), 0) == -1)
    error("Can't send number");
}

int recv_num(int socket)
{
  int num;
  if (recv(socket, &num, sizeof(int), 0) == -1)
    error("Can't read number");
  return num;
}

void get_secret_shares(int num, int shares[])
{
  shares[0] = random() % P;
  shares[1] = random() % P;
  shares[2] = (num - shares[0] - shares[1]) % P;
  if (shares[2] < 0) // if it is negative
    shares[2] += P; // make it positive
}


/**
 * Functions telling every party how to share their shares with the other parties
 */
int p1_share(int p1_num, int p2_connection, int p3_connection, int p1_shares[], int p2_2and3_shares[], int p3_2and3_shares[])
{
  get_secret_shares(p1_num, p1_shares);

  // send shares
  send_num(p2_connection, p1_shares[0]);
  send_num(p2_connection, p1_shares[2]);
  send_num(p3_connection, p1_shares[0]);
  send_num(p3_connection, p1_shares[1]);

  // recieve shares from p2
  p2_2and3_shares[0] = recv_num(p2_connection);
  p2_2and3_shares[1] = recv_num(p2_connection);

  // recieve shares from p3
  p3_2and3_shares[0] = recv_num(p3_connection);
  p3_2and3_shares[1] = recv_num(p3_connection);
}

int p2_share(int p2_num, int p1_connection, int p3_connection, int p2_shares[], int p1_1and3_shares[], int p3_1and3_shares[])
{
  get_secret_shares(p2_num, p2_shares);

  // receive shares from p1
  p1_1and3_shares[0] = recv_num(p1_connection);
  p1_1and3_shares[1] = recv_num(p1_connection);

  // send shares
  send_num(p1_connection, p2_shares[1]);
  send_num(p1_connection, p2_shares[2]);
  send_num(p3_connection, p2_shares[0]);
  send_num(p3_connection, p2_shares[1]);

  // receive shares from p3
  p3_1and3_shares[0] = recv_num(p3_connection);
  p3_1and3_shares[1] = recv_num(p3_connection);
}

int p3_share(int p3_num, int p1_connection, int p2_connection, int p3_shares[], int p1_1and2_shares[], int p2_1and2_shares[])
{
  get_secret_shares(p3_num, p3_shares);

  // receive shares from p1
  p1_1and2_shares[0] = recv_num(p1_connection);
  p1_1and2_shares[1] = recv_num(p1_connection);

  // receive shares from p2
  p2_1and2_shares[0] = recv_num(p2_connection);
  p2_1and2_shares[1] = recv_num(p2_connection);

  // send shares
  send_num(p1_connection, p3_shares[1]);
  send_num(p1_connection, p3_shares[2]);
  send_num(p2_connection, p3_shares[0]);
  send_num(p2_connection, p3_shares[2]);
}


/**
 * Functions containing all the things every party must do so that they can
 * compute the sum of their private input values together
 */
int p1_add(int p1_num, int p2_connection, int p3_connection)
{
  // distribute shares of secret value
  int p1_shares[3], p2_2and3_shares[2], p3_2and3_shares[2];
  p1_share(p1_num, p2_connection, p3_connection, p1_shares, p2_2and3_shares, p3_2and3_shares);
  
  // sum received shares and share them
  int share_sum2 = (p1_shares[1] + p2_2and3_shares[0] + p3_2and3_shares[0]) % P;
  send_num(p2_connection, share_sum2);
  send_num(p3_connection, share_sum2);
  int share_sum3 = recv_num(p2_connection);
  int share_sum1 = recv_num(p3_connection);

  int final_sum = (share_sum1 + share_sum2 + share_sum3) % P;

  return final_sum;
}

int p2_add(int p2_num, int p1_connection, int p3_connection)
{
  // distribute shares of secret value
  int p2_shares[3], p1_1and3_shares[2], p3_1and3_shares[2];
  p2_share(p2_num, p1_connection, p3_connection, p2_shares, p1_1and3_shares, p3_1and3_shares);

  // sum received shares and share them
  int share_sum2 = recv_num(p1_connection); 
  int share_sum3 = (p2_shares[2] + p1_1and3_shares[1] + p3_1and3_shares[1]) % P;
  send_num(p1_connection, share_sum3);
  send_num(p3_connection, share_sum3);
  int share_sum1 = recv_num(p3_connection);

  int final_sum = (share_sum1 + share_sum2 + share_sum3) % P;

  return final_sum;
}

int p3_add(int p3_num, int p1_connection, int p2_connection)
{
  // distribute shares of the secret value
  int p3_shares[3], p1_1and2_shares[2], p2_1and2_shares[2];
  p3_share(p3_num, p1_connection, p2_connection, p3_shares, p1_1and2_shares, p2_1and2_shares);
  
  // sum received shares and share them
  int share_sum2 = recv_num(p1_connection);
  int share_sum3 = recv_num(p2_connection);
  int share_sum1 = (p3_shares[0] + p1_1and2_shares[0] + p2_1and2_shares[0]) % P;
  send_num(p1_connection, share_sum1);
  send_num(p2_connection, share_sum1);

  int final_sum = (share_sum1 + share_sum2 + share_sum3) % P;

  return final_sum;
}


/**
 * These functions contain all steps each party must perform to multiply
 * 2 numbers.
 * The first number will be input to party 1 and the second to party 2.
 * Party 3 does compute a result with the others but has no input value
 */
int p1_mult(int p1_num, int p2_connection, int p3_connection)
{
  // distribute shares of p1_num
  int p1_shares[3], p2_2and3_shares[2], p3_shares_dummy[2]; // shares of P3 not used for multiplication
  p1_share(p1_num, p2_connection, p3_connection, p1_shares, p2_2and3_shares, p3_shares_dummy);

  int product_part_sum1 = (p1_shares[1]*p2_2and3_shares[0] + p1_shares[1]*p2_2and3_shares[1] + p1_shares[2]*p2_2and3_shares[0]) % P;

  return p1_add(product_part_sum1, p2_connection, p3_connection);
}

int p2_mult(int p2_num, int p1_connection, int p3_connection)
{
  // distribute shares of p2_num
  int p2_shares[3], p1_1and3_shares[2], p3_shares_dummy[2];
  p2_share(p2_num, p1_connection, p3_connection, p2_shares, p1_1and3_shares, p3_shares_dummy);

  int product_part_sum2 = (p1_1and3_shares[1]*p2_shares[2] + p1_1and3_shares[0]*p2_shares[2] + p1_1and3_shares[1]*p2_shares[0]) % P;

  return p2_add(product_part_sum2, p1_connection, p3_connection);
}

int p3_mult(int p1_connection, int p2_connection)
{
  // receive shares from p1 and p2
  int p3_shares_dummy[3], p1_1and2_shares[2], p2_1and2_shares[2];
  p3_share(0, p1_connection, p2_connection, p3_shares_dummy, p1_1and2_shares, p2_1and2_shares);

  int product_part_sum3 = (p1_1and2_shares[0]*p2_1and2_shares[0] + p1_1and2_shares[0]*p2_1and2_shares[1] + p1_1and2_shares[1]*p2_1and2_shares[0]) % P;

  return p3_add(product_part_sum3, p1_connection, p2_connection);
}

int main(int argc, char *argv[])
{
  if (argc != 4) // check for exactly 3 arguments (the program name does not count)
    usage();

  printf("Calculating sum of given numbers modulo %d\n"
         "and the product of the first 2 numbers modulo %d\n", P, P);

  int p1_num = atoi(argv[1]);
  int p2_num = atoi(argv[2]);
  int p3_num = atoi(argv[3]);

  // make connections between processes using "socketpair" syscall
  // with Unix sockets
  int p12_connection[2], p13_connection[2], p23_connection[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, p12_connection) == -1)
    error("Can't create socket pair for connecting party 1 and 2");
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, p13_connection) == -1)
    error("Can't create socket pair for connecting party 1 and 3");
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, p23_connection) == -1)
    error("Can't create socket pair for connecting party 2 and 3");

  // party 1 process
  pid_t pid1 = fork();
  if (pid1 == -1)
    error("Can't fork party 1 process");
  if (!pid1) {
    // set random number seed
    srandom(time(0));

    int sum1 = p1_add(p1_num, p12_connection[0], p13_connection[0]);
    printf("Sum calculated by party 1: %d\n", sum1);

    int prod1 = p1_mult(p1_num, p12_connection[0], p13_connection[0]);
    printf("Product calculated by party 1: %d\n", prod1);

    close(p13_connection[0]);
    close(p12_connection[0]);
    exit(0);
  }

  // party 2 process
  pid_t pid2 = fork();
  if (pid2 == -1)
    error("Can't fork party 2 process");
  if (!pid2) {
    // set random number seed
    srandom(time(0) + 20);

    int sum2 = p2_add(p2_num, p12_connection[1], p23_connection[0]);
    printf("Sum calculated by party 2: %d\n", sum2);

    int prod2 = p2_mult(p2_num, p12_connection[1], p23_connection[0]);
    printf("Product calculated by party 2: %d\n", prod2);

    close(p12_connection[1]);
    close(p23_connection[0]);
    exit(0);
  }

  // party 3 process
  pid_t pid3 = fork();
  if (pid3 == -1)
    error("Can't fork party 3 process");
  if (!pid3) {
    // set random number seed
    srandom(time(0) + 30);

    int sum3 = p3_add(p3_num, p13_connection[1], p23_connection[1]);
    printf("Sum calculated by party 3: %d\n", sum3);

    int prod3 = p3_mult(p13_connection[1], p23_connection[1]);
    printf("Product calculated by party 3: %d\n", prod3);

    close(p13_connection[1]);
    close(p23_connection[1]);
    exit(0);
  }

  // wait for the processes to exit
  int pid_status;
  if (waitpid(pid1, &pid_status, 0) == -1)
    error("Error waiting for party 1 process");
  if (waitpid(pid2, &pid_status, 0) == -1)
    error("Error waiting for party 2 process");
  if (waitpid(pid3, &pid_status, 0) == -1)
    error("Error waiting for party 3 process");
}
