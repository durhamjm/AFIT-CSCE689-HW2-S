#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <strings.h>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include "TCPServer.h"

TCPServer::TCPServer(){ // :_server_log("server.log", 0) {
}


TCPServer::~TCPServer() {
}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {

   struct sockaddr_in servaddr;
   
   // _server_log.writeLog("Server started.");

   // Set the socket to nonblocking
   _sockfd.setNonBlocking();

   // Load the socket information to prep for binding
   _sockfd.bindFD(ip_addr, port);
 
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() {

   bool online = true;
   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 100000000;
   int num_read = 0;

   // Start the server socket listening
   _sockfd.listenFD(5);
    
   while (online) {
      struct sockaddr_in clientaddr;
      socklen_t len = sizeof(clientaddr);

      if (_sockfd.hasData()) {
         TCPConn *new_conn = new TCPConn();
         if (!new_conn->accept(_sockfd)) {
            // _server_log.strerrLog("Data received on socket but failed to accept.");
            continue;
         }
         std::cout << "***Got a connection***\n";

         _connlist.push_back(std::unique_ptr<TCPConn>(new_conn));

         // Get their IP Address string to use in logging
         std::string ipaddr_str;
         new_conn->getIPAddrStr(ipaddr_str);


         new_conn->sendText("Welcome to the prime factor server!\n");
         // **Log connection (if/else for whitelist) with IP

         // Change this later
         std::string msg3;
         new_conn->getUsername(msg3);
      }

      // Loop through our connections, handling them
      std::string msg3;
      std::list<std::unique_ptr<TCPConn>>::iterator tptr2 = _connlist.begin();
      std::list<std::unique_ptr<TCPConn>>::iterator tptr = _connlist.begin();
      while (tptr != _connlist.end())
      {
         // If the user lost connection
         if (!(*tptr)->isConnected()) {
            // **Log it
            // Remove them from the connect list
            tptr = _connlist.erase(tptr);
            std::cout << "Connection disconnected.\n";
            continue;
         }
         (*tptr)->getUsername(msg3);
         tptr++;

         // if (tptr == _connlist.end()) {
         //    tptr--;
         //    tptr--;
         // }
         // (*tptr)->_connfd.writeFD(msg3);

         // Process any user inputs

         // while (tptr2 != _connlist.end()) {
         //    std::cout << "sending to all\n";
         //    (*tptr2)->sendNum(msg3);
         //    tptr2++;
         // }
         // tptr2 = _connlist.begin();
         // Increment our iterator
         // tptr++;
      }

      // So we're not chewing up CPU cycles unnecessarily
      //nanosleep(&sleeptime, NULL);
   }   
}


/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
void TCPServer::shutdown() {
   _sockfd.closeFD();
}


