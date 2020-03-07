#ifndef TCPCONN_H
#define TCPCONN_H

#include "FileDesc.h"
#include <list>
#include "boost/multiprecision/cpp_int.hpp"

//using namespace boost::multiprecision;

const int max_attempts = 2;

// Methods and attributes to manage a network connection, including tracking the username
// and a buffer for user input. Status tracks what "phase" of login the user is currently in
class TCPConn 
{
public:
   TCPConn(/* LogMgr &server_log*/);
   ~TCPConn();

   int getNumber();

   boost::multiprecision::uint256_t num, div1, div2;
   bool div_found = false;

   bool accept(SocketFD &server);

   int sendText(const char *msg);
   int sendText(const char *msg, int size);

   void handleConnection();
   void startAuthentication();
   boost::multiprecision::uint256_t getUsername();
   boost::multiprecision::uint256_t getPasswd(boost::multiprecision::uint256_t n);
   void sendMenu();
   void getMenuChoice();
   void setPassword();
   void changePassword();
   
   bool getUserInput(std::string &cmd);

   void disconnect();
   bool isConnected();

   unsigned long getIPAddr() { return _connfd.getIPAddr(); };
   void getIPAddrStr(std::string &buf);
   const char *getUsernameStr() { return _username.c_str(); };

   bool whitelist();
   bool writeLog(std::string &buf);
   const std::string getTime();

   boost::multiprecision::uint256_t modularPow(boost::multiprecision::uint256_t base, boost::multiprecision::uint256_t exponent, boost::multiprecision::uint256_t modulus);
   void factor(boost::multiprecision::uint256_t n);
   bool isPrimeBF(boost::multiprecision::uint256_t n, boost::multiprecision::uint256_t &divisor);
   boost::multiprecision::uint256_t calcPollardsRho(boost::multiprecision::uint256_t n);
   void combinePrimes(std::list<boost::multiprecision::uint256_t> &dest);

   std::list<boost::multiprecision::uint256_t> primes;
   std::list<boost::multiprecision::uint256_t>::iterator iterator;
   boost::multiprecision::uint256_t check2;


private:

   enum statustype { s_getnumber, s_username, s_changepwd, s_confirmpwd, s_passwd, s_menu };

   statustype _status = s_username;

   SocketFD _connfd;
 
   std::string _username; // The username this connection is associated with

   std::string _inputbuf;

   std::string _newpwd; // Used to store user input for changing passwords

   int _pwd_attempts = 0;
};


#endif
