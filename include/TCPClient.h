#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include "Client.h"
#include "FileDesc.h"

// The amount to read in before we send a packet
const unsigned int stdin_bufsize = 50;
const unsigned int socket_bufsize = 100;

class TCPClient : public Client
{
public:
   TCPClient();
   ~TCPClient();

   virtual void connectTo(const char *ip_addr, unsigned short port);
   virtual void handleConnection();
   virtual void closeConn();

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
