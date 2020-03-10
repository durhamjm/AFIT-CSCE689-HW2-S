#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include "Client.h"
#include "FileDesc.h"
#include "boost/multiprecision/cpp_int.hpp"


// The amount to read in before we send a packet
const unsigned int stdin_bufsize = 50;
const unsigned int socket_bufsize = 100;

class TCPClient : public Client
{
public:
   TCPClient();
   ~TCPClient();

   std::string msg2;
   std::stringstream num3;

   virtual void connectTo(const char *ip_addr, unsigned short port);
   virtual void handleConnection();
   virtual void closeConn();

   boost::multiprecision::uint256_t getnum(boost::multiprecision::uint256_t n);

   boost::multiprecision::uint256_t num, numclient, div1, div2;
   bool div_found = false;

   boost::multiprecision::uint256_t getPasswd(boost::multiprecision::uint256_t n);
   boost::multiprecision::uint256_t modularPow(boost::multiprecision::uint256_t base, boost::multiprecision::uint256_t exponent, boost::multiprecision::uint256_t modulus);
   void factor(boost::multiprecision::uint256_t n);
   bool isPrimeBF(boost::multiprecision::uint256_t n, boost::multiprecision::uint256_t &divisor);
   boost::multiprecision::uint256_t calcPollardsRho(boost::multiprecision::uint256_t n);
   void combinePrimes(std::list<boost::multiprecision::uint256_t> &dest);

   std::list<boost::multiprecision::uint256_t> primes;
   std::list<boost::multiprecision::uint256_t>::iterator iterator;
   boost::multiprecision::uint256_t check2;

private:
   int readStdin();

   // Stores the user's typing
   std::string _in_buf;

   // Class to manage our client's network connection
   SocketFD _sockfd;
 
   // Manages the stdin FD for user inputs
   TermFD _stdin;

};


#endif
