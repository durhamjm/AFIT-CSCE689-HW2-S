#include <stdexcept>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "TCPConn.h"
#include "strfuncts.h"
#include "PasswdMgr.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <time.h>

// The filename/path of the password & log file
const char pwdfilename[] = "passwd";
PasswdMgr pmgr(pwdfilename);

TCPConn::TCPConn(){ 
   //LogMgr &server_log):_server_log(server_log) {
}


TCPConn::~TCPConn() {
}

/**********************************************************************************************
 * accept - simply calls the acceptFD FileDesc method to accept a connection on a server socket.
 *
 *    Params: server - an open/bound server file descriptor with an available connection
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

bool TCPConn::accept(SocketFD &server) {
   return _connfd.acceptFD(server);
}

/**********************************************************************************************
 * sendText - simply calls the sendText FileDesc method to send a string to this FD
 *
 *    Params:  msg - the string to be sent
 *             size - if we know how much data we should expect to send, this should be populated
 *
 *    Throws: runtime_error for unrecoverable errors
 **********************************************************************************************/

int TCPConn::sendText(const char *msg) {
   return sendText(msg, strlen(msg));
}

int TCPConn::sendText(const char *msg, int size) {
   if (_connfd.writeFD(msg, size) < 0) {
      return -1;  
   }
   return 0;
}

/**********************************************************************************************
 * startAuthentication - Sets the status to request username
 *
 *    Throws: runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPConn::startAuthentication() {
   // Get client IP
   std::string IP;
   getIPAddrStr(IP);
   // std::cout << IP << std::endl;

   // Check whitelist, allow access if listed, log attempt
   if (!whitelist()) {
      _connfd.writeFD("You do not have access to this system.\n");
      disconnect();
      std::string log = IP + " - unauthorized IP.";
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      }
   } else {
      std::string log = IP + " - Access from known IP.";
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      }
   }
   
   _status = s_username;
}

/**********************************************************************************************
 * handleConnection - performs a check of the connection, looking for data on the socket and
 *                    handling it based on the _status, or stage, of the connection
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::handleConnection() {

   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 100000000;

   try {
      switch (_status) {
         case s_username:
            getUsername();
            break;

         case s_passwd:
            getPasswd();
            break;
   
         case s_changepwd:
         case s_confirmpwd:
            changePassword();
            break;

         case s_menu:
            getMenuChoice();

            break;

         default:
            throw std::runtime_error("Invalid connection status!");
            break;
      }
   } catch (socket_error &e) {
      std::cout << "Socket error, disconnecting.";
      disconnect();
      return;
   }

   nanosleep(&sleeptime, NULL);
}

/**********************************************************************************************
 * getUsername - called from handleConnection when status is s_username--if it finds user data,
 *               it expects a username and compares it against the password database
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::getUsername() {
   // Get client IP
   std::string IP;
   getIPAddrStr(IP);
   // std::cout << IP << std::endl;
  
   // Get username
   _connfd.writeFD("Username: ");
   if (getUserInput(_username) == false) {
      _connfd.writeFD("Error getting username.\n");
      std::cout << "Error getting username.\n";
   }

   // Compare username to known users, log if unknown
   if (!pmgr.checkUser(_username.c_str())) {
      _connfd.writeFD("Unknown user.\n");
      std::string log = IP + " - Invalid user - " + _username;
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      }
   // When username is good, ask for password
   } else {
      _status = s_passwd;
      handleConnection();
   }
}

/**********************************************************************************************
 * getPasswd - called from handleConnection when status is s_passwd--if it finds user data,
 *             it assumes it's a password and hashes it, comparing to the database hash. Users
 *             get two tries before they are disconnected
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::getPasswd() {
   // Get password 
   _connfd.writeFD("Password: ");
   if (getUserInput(_newpwd) == false) {
      std::cout << "Error getting password.\n";
   }

   // Check password limiting to 2 attempts
   if (!pmgr.checkPasswd(_username.c_str(),_newpwd.c_str())) {
      if (_pwd_attempts == 0) {
         _connfd.writeFD("\nIncorrect password. 1 attempt remaining.\n");
         _pwd_attempts += 1;
      } 
      // Log incorrect login attempt and disconect user
      else if (_pwd_attempts == 1) {
         _connfd.writeFD("\nIncorrect password. Goodbye.\n");
         std::string IP;
         getIPAddrStr(IP);
         std::string log = IP + " - Validation failed - " + _username;
         if (!writeLog(log)) {
            std::cout << "Unable to write to log.\n";
         }
         disconnect();
      } 
   } 
   // On success, log authentication and pass user to menu
   else {
         std::cout << "Successfully authenticated.\n";
         _connfd.writeFD("Success!");
         std::string IP;
         getIPAddrStr(IP);
         std::string log = IP + " - Authenticated - " + _username;
         if (!writeLog(log)) {
            std::cout << "Unable to write to log.\n";
         }
         sendMenu();
         _status = s_menu;
         handleConnection();
      }
}

/**********************************************************************************************
 * changePassword - called from handleConnection when status is s_changepwd or s_confirmpwd--
 *                  if it finds user data, with status s_changepwd, it saves the user-entered
 *                  password. If s_confirmpwd, it checks to ensure the saved password from
 *                  the s_changepwd phase is equal, then saves the new pwd to the database
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::changePassword() {
   std::string _newpwd2;

   // Get password once
   _connfd.writeFD("Enter new Password: ");
   fflush(stdout);
   getUserInput(_newpwd);
   clrNewlines(_newpwd); 

   // Get password second time
   _connfd.writeFD("Enter password again: ");
   fflush(stdout);
   getUserInput(_newpwd2);
   clrNewlines(_newpwd2);

   // If passwords match, change it
   if (_newpwd == _newpwd2) {
      if (!pmgr.changePasswd(_username.c_str(), _newpwd.c_str())) {
      std::cout << "Error changing password.\n";
      }
   } else {
      _connfd.writeFD("Passwords did not match. Returning to menu.\nEnter a command.\n");
      _status = s_menu;
      return;
   }
   
   // Log password changed and send user back to menu
   _connfd.writeFD("Password changed successfully!\nEnter a command.\n");
   std::string IP;
   getIPAddrStr(IP);
   std::string log = IP + " - Password changed - " + _username;
   if (!writeLog(log)) {
      std::cout << "Unable to write to log.\n";
   }
   _status = s_menu;
}


/**********************************************************************************************
 * getUserInput - Gets user data and includes a buffer to look for a carriage return before it is
 *                considered a complete user input. Performs some post-processing on it, removing
 *                the newlines
 *
 *    Params: cmd - the buffer to store commands - contents left alone if no command found
 *
 *    Returns: true if a carriage return was found and cmd was populated, false otherwise.
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

bool TCPConn::getUserInput(std::string &cmd) {
   std::string readbuf;

   // read the data on the socket
   _connfd.readFD(readbuf);

   // concat the data onto anything we've read before
   _inputbuf += readbuf;

   // If it doesn't have a carriage return, then it's not a command
   int crpos;
   if ((crpos = _inputbuf.find("\n")) == std::string::npos)
      return false;

   cmd = _inputbuf.substr(0, crpos);
   _inputbuf.erase(0, crpos+1);

   // Remove \r if it is there
   clrNewlines(cmd);

   return true;
}

/**********************************************************************************************
 * getMenuChoice - Gets the user's command and interprets it, calling the appropriate function
 *                 if required.
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::getMenuChoice() {
   if (!_connfd.hasData())
      return;
   std::string cmd;
   if (!getUserInput(cmd))
      return;
   lower(cmd);      

   std::string msg;
   if (cmd.compare("hello") == 0) {
      _connfd.writeFD("Hello and welcome to the client authentication server!\n");
   } else if (cmd.compare("menu") == 0) {
      sendMenu();
   } else if (cmd.compare("exit") == 0) {
      _connfd.writeFD("So long and thanks for all the fish!\n");
      std::string IP;
      getIPAddrStr(IP);
      std::string log = IP + " - Disconnected - " + _username;
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      }
      disconnect();
   } else if (cmd.compare("passwd") == 0) {
      _status = s_changepwd;
   } else if (cmd.compare("1") == 0) {
      _connfd.writeFD("It can be on time, on budget, or functional, but it can't be all three.\n");
   } else if (cmd.compare("2") == 0) {
      _connfd.writeFD("Why did Adele cross the road? To say hello from the other side.\n");
   } else if (cmd.compare("3") == 0) {
      msg = getTime();
      msg += "\n";
      _connfd.writeFD(msg);
   } else if (cmd.compare("4") == 0) {
      _connfd.writeFD("Why would the server care? *wink*\n");
      std::string IP;
      getIPAddrStr(IP);
      std::string log = IP + " - User is bored! - " + _username;
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      }
   } else if (cmd.compare("5") == 0) {
      _connfd.writeFD("run warcraft.exe\n");
   } else {
      msg = "Unrecognized command: ";
      msg += cmd;
      msg += "\n";
      _connfd.writeFD(msg);
   }

}

/**********************************************************************************************
 * sendMenu - sends the menu to the user via their socket
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
void TCPConn::sendMenu() {
   std::string menustr;

   // Make this your own!
   menustr += "Acceptable commands: \n";
   menustr += "  1 - Know your limits\n";
   menustr += "  2 - Hear a joke\n";
   menustr += "  3 - Get the time\n";
   menustr += "  4 - Tell the server you're bored\n";
   menustr += "  5 - Play a game\n";
   menustr += "Misc. commands: \n";
   menustr += "  Hello - welcome message\n";
   menustr += "  Passwd - change your password\n";
   menustr += "  Menu - display this menu\n";
   menustr += "  Exit - disconnect\n\n";

   _connfd.writeFD(menustr);
}


/**********************************************************************************************
 * disconnect - cleans up the socket as required and closes the FD
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
void TCPConn::disconnect() {
   _connfd.closeFD();
}


/**********************************************************************************************
 * isConnected - performs a simple check on the socket to see if it is still open 
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
bool TCPConn::isConnected() {
   return _connfd.isOpen();
}

/**********************************************************************************************
 * getIPAddrStr - gets a string format of the IP address and loads it in buf
 *
 **********************************************************************************************/
void TCPConn::getIPAddrStr(std::string &buf) {
   return _connfd.getIPAddrStr(buf);
}

bool TCPConn::whitelist() {
   // Adapted from http://cplusplus.com/forum/beginner/80838/
   // Get client's IP from getIPAddrStr()
   // Open whitelist file
   std::string clientIP, list; 
   _connfd.getIPAddrStr(clientIP);

   std::ifstream whitelist;
   whitelist.open("whitelist");

   // Go through the list, remove trailing \n, compare it to client IP
   while (whitelist >> list) {
      list.erase(std::remove(list.begin(), list.end(), '\n'), list.end());
      // std::cout << list;
      // std::cout << clientIP;
      if (list == clientIP) {
         std::cout << "Good on whitelist.\n";
         return true;
      } 
   }
   std::cout << "Bad on whitelist.\n";
   return false;
}

bool TCPConn::writeLog(std::string &buf){
   // Find end of file, start new line
   std::ofstream out("server.log", std::ios::app);

      out << getTime();
      out << " - ";
      out << buf << std::endl;
      out.close();
      return true;

}

// Get current date/time in format YYYY-MM-DD.HH:mm:ss
// Adapted from https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c/
const std::string TCPConn::getTime(){
   std::time_t time = std::time(0);
   struct tm tstruct;
   char buf2[80];
   tstruct = *localtime(&time);
   strftime(buf2, sizeof(buf2), "%Y-%m-%d.%X", &tstruct);
   return buf2;
}