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
#include "boost/math/common_factor.hpp"
#include "boost/multiprecision/cpp_int.hpp"

//using namespace boost::multiprecision;

// The filename/path of the password & log file
// const char pwdfilename[] = "passwd";
// PasswdMgr pmgr(pwdfilename);

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
   /* if (!whitelist()) {
      _connfd.writeFD("You do not have access to this system.\n");
      disconnect();
      std::string log = IP + " - unauthorized IP.";
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      }
   } else { */
      std::string log = IP + " - Access from known IP.";
      if (!writeLog(log)) {
         std::cout << "Unable to write to log.\n";
      //}
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
            // case s_getnumber:
            //    num = getNumber();
            //    break;

         case s_username:
            num = getUsername();
            break;

         case s_passwd:
            div1 = getPasswd(num);
            break;
   
         // case s_changepwd:
         // case s_confirmpwd:
         //    changePassword();
         //    break;

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

boost::multiprecision::uint256_t TCPConn::getUsername() {
   primes = {};
   primes.resize(0);
   check2 = 1;
   div_found = false;

   std::cout << "What is your starting number?" << std::endl;
   boost::multiprecision::uint256_t n;
   std::cin >> n;
   std::cout << "You entered " << n << std::endl;
   if (n <= 2) {
      std::cout << "Invalid. Please enter a number greater than 2." << std::endl;
      n = 1;
   }
   
   if (n > 2) {
      //std::string msg = "The number is " + std::to_string(n) + ". Now get to work!\n";
      //msg += n;
      //msg += " . Now get to work!\n";
      //_connfd.writeFD(msg);
      _status = s_passwd;
   }
   return n;
   //handleConnection();
   
   // // Get client IP
   // std::string IP;
   // getIPAddrStr(IP);
   // // std::cout << IP << std::endl;
  
   // // Get username
   // _connfd.writeFD("Username: ");
   // if (getUserInput(_username) == false) {
   //    _connfd.writeFD("Error getting username.\n");
   //    std::cout << "Error getting username.\n";
   // }

   // Compare username to known users, log if unknown
   // if (!pmgr.checkUser(_username.c_str())) {
   //    _connfd.writeFD("Unknown user.\n");
   //    std::string log = IP + " - Invalid user - " + _username;
   //    if (!writeLog(log)) {
   //       std::cout << "Unable to write to log.\n";
   //    }
   // When username is good, ask for password
   // /* } */ else {
   //    _status = s_menu;
   //    handleConnection();
   // }
}


/********************************************************************************************
 * modularPow - function to gradually calculate (x^n)%m to avoid overflow issues for
 *              very large non-prime numbers using the stl function pow (floats)
 *
 *    Params:  base - the base value x in the above function
 *             exponent - the exponent n
 *             modulus - the modulus m
 *
 *    Returns: resulting number
 ********************************************************************************************/
boost::multiprecision::uint256_t TCPConn::modularPow(boost::multiprecision::uint256_t base, boost::multiprecision::uint256_t exponent, boost::multiprecision::uint256_t modulus) {
   boost::multiprecision::uint256_t result = 1;

   while (exponent > 0) {

      // If the exponent is odd, calculate our results 
      if (exponent & 1) {
         result = (result * base) % modulus;
      }

      // exponent = exponent / 2 (rounded down)
      exponent = exponent >> 1;

      base = (base * base) % modulus;
   }
   return result;

}

bool TCPConn::isPrimeBF(boost::multiprecision::uint256_t n, boost::multiprecision::uint256_t &divisor) {
   std::cout << "Checking if prime: " << n << std::endl;

   divisor = 0;

   // Take care of simple cases
   if (n <= 3) {
      return n > 1;
   }
   else if ((n % 2) == 0) {
      divisor = 2;
      return false;
   } else if ((n & 3) == 0) {
      divisor = 3;
      return false;
   } /* else if ((n % 5) == 0) {
      divisor = 5;
      return false;
   } */

   // Assumes all primes are to either side of 6k. Using 256 bit to avoid overflow
   // issues when calculating max range
   boost::multiprecision::uint256_t n_256t = n;
   for (boost::multiprecision::uint256_t k=5; k * k < n_256t; k = k+6) {
      if ((n_256t % k == 0) || (n_256t % (k+2) == 0)) {
         divisor = (boost::multiprecision::uint256_t) k;
         return false;
      }
   }
   return true;
}


boost::multiprecision::uint256_t TCPConn::calcPollardsRho(boost::multiprecision::uint256_t n) {
   if (n <= 3)
      return n;

   // Initialize our random number generator
   srand(time(NULL));

   // pick a random number from the range [2, N)
   boost::multiprecision::uint256_t x = (rand()%(n-2)) + 2;
   boost::multiprecision::uint256_t y = x;    // Per the algorithm

   // random number for c = [1, N)
   boost::multiprecision::uint256_t c = (rand()%(n-1)) + 1;

   boost::multiprecision::uint256_t d = 1;

   // Loop until either we find the gcd or gcd = 1
   while (d == 1) {
      // "Tortoise move" - Update x to f(x) (modulo n)
      // f(x) = x^2 + c f
      x = (modularPow(x, 2, n) + c + n) % n;

      // "Hare move" - Update y to f(f(y)) (modulo n)
      y = (modularPow(y, 2, n) + c + n) % n;
      y = (modularPow(y, 2, n) + c + n) % n;

      // Calculate GCD of |x-y| and tn
      boost::multiprecision::uint256_t z = (boost::multiprecision::uint256_t) x - (boost::multiprecision::uint256_t) y;
      if (z < 0)
         d = boost::math::gcd((boost::multiprecision::uint256_t) -z, (boost::multiprecision::uint256_t) n);
      else
         d = boost::math::gcd((boost::multiprecision::uint256_t) z, (boost::multiprecision::uint256_t) n);

      // If we found a divisor, factor primes out of each side of the divisor
      if ((d != 1) && (d != n)) {
         return (boost::multiprecision::uint256_t) d;

      }

   }
   return (boost::multiprecision::uint256_t) d;
}


void TCPConn::combinePrimes(std::list<boost::multiprecision::uint256_t> &dest) {
   dest.insert(dest.end(), primes.begin(), primes.end());
}


void TCPConn::factor(boost::multiprecision::uint256_t n) {
   // already prime
   if (n == 1) {
      return;
   }
   if (div_found) {
      std::cout << "Stop calling me!" << std::endl;
      num = getUsername();
   }
   //std::cout << "Factoring: " << n << std::endl;

   
   bool check;
   boost::multiprecision::uint256_t iters = 0;

   while (!div_found) {
      //std::cout << "Starting iteration: " << iters << std::endl;

      // If we have tried Pollards Rho a specified number of times, run the
      // costly prime check to see if this is a prime number. Also, increment
      // iters after the check
      boost::multiprecision::uint256_t primecheck_depth = sqrt(n); 
      if (iters++ == primecheck_depth) {
	   // std::cout << "Pollards rho timed out, checking if the following is prime: " << n << std::endl;
	   boost::multiprecision::uint256_t divisor;
         if (isPrimeBF(n, divisor)) {
	       std::cout << "Prime found1: " << n << std::endl;
            primes.push_back(n);
            iterator = primes.begin();
            check2 = 1;
            for (int i = 0; i < primes.size(); i++) {
               if (*iterator >= 1) {
                  check2 *= *iterator;
                  if (check2 <= num * 10) {
                     // std::cout << "List2 size: " << primes.size() << std::endl;
                     // std::cout << "List2 at " << i << " : " << *iterator << std::endl;
                     // std::cout << "Check2 = " << check2 << std::endl;
                     }
                  std::advance(iterator, 1);
                  if (check2 == num) {
                     std::cout << "Mission complete!1 Primes are: " << std::endl;
                     iterator = primes.begin();
                     check2 = 1;
                     for (int i = 0; i < primes.size(); i++) {
                        if (*iterator >= 1) {
                           std::cout << *iterator << ", ";
                           check2 *= *iterator;
                           if (check2 == num) {
                              std::cout << std::endl;
                              _status = s_menu;
                              div_found = true;
                              num = getUsername();
                              std::cout << "Crashing now, don't mind me." << std::endl;
                              //handleConnection();
                              return;
                           }
                        }
                        std::advance(iterator, 1);
                     }
                  }
               }
            }
            
	    return;
	 } else {   // We found a prime divisor, save it and keep finding primes
	       std::cout << "Prime found2: " << divisor << std::endl;
	    primes.push_back(divisor);
      iterator = primes.begin();
      for (int i = 0; i < primes.size(); i++) {
         if (*iterator >= 1) {
            check2 *= *iterator;
            if (check2 <= num * 10) {
               // std::cout << "List3 size: " << primes.size() << std::endl;
               // std::cout << "List3 at " << i << " : " << *iterator << std::endl;
               // std::cout << "Check3 = " << check2 << std::endl;
            }
            std::advance(iterator, 1);
            if (check2 == num) {
               std::cout << "Mission complete!2 Primes are: " << std::endl;
               iterator = primes.begin();
               check2 = 1;
               for (int i = 0; i < primes.size(); i++) {
                  if (*iterator >= 1) {
                     std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        std::cout << std::endl;
                        _status = s_menu;
                        div_found = true;
                        num = getUsername();
                        std::cout << "Crashing now, don't mind me." << std::endl;
                        return;
                     }
                  }
                  std::advance(iterator, 1);
               }
            }
         }
      }
       if (check2 != num) {
         //std::cout << "Second check" << std::endl;
	      return factor(n / divisor);
       }
	 }				
      }

      if (check2 != num) {
         //std::cout << "Third check" << std::endl;
         // We try to get a divisor using Pollards Rho
         boost::multiprecision::uint256_t d = calcPollardsRho(n);
         if (d != n) {
            std::cout << "Divisor found: " << d << std::endl;

            // Factor the divisor
            factor(d);

            // Now the remaining number
            factor((boost::multiprecision::uint256_t) (n/d));

            return;
         }
      }
      // If d == n, then we re-randomize and continue the search up to the prime check depth
   }
   num = getUsername();
   throw std::runtime_error("Reached end of function--this should not have happened.");
   return;
}


/**********************************************************************************************
 * getPasswd - called from handleConnection when status is s_passwd--if it finds user data,
 *             it assumes it's a password and hashes it, comparing to the database hash. Users
 *             get two tries before they are disconnected
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

boost::multiprecision::uint256_t TCPConn::getPasswd(boost::multiprecision::uint256_t n) {
   if (n <= 3) {
      return n;
   }

   // std::cout << "Checking if prime: " << n << std::endl;

   // div1 = 0;
   // bool isprime;

   // // Take care of simple cases
   // if ((n % 2) == 0) {
   //    div1 = 2;
   //    isprime = false;
   // } else if ((n & 3) == 0) {
   //    div1 = 3;
   //    isprime = false;
   // }

   // // Assumes all primes are to either side of 6k. Using 256 bit to avoid overflow
   // // issues when calculating max range
   
   // int n_256t = n;
   // for (int k=5; k * k < n_256t; k = k+6) {
   //    if ((n_256t % k == 0) || (n_256t % (k+2) == 0)) {
   //       div1 = (int) k;
   //       isprime = false;
   //    }
   // }
   // isprime = true;

   // if (!isprime) {
   //    std::cout << n << " is prime. Exiting." << std::endl;
   //    return 0;
   // }


// First, take care of the '2' factors
   boost::multiprecision::uint256_t newval = n;
   
   while (newval % 2 == 0) {
      check2 = 1;
      primes.push_front(2);
      std::cout << "Prime Found: 2\n";
      newval = newval / 2;
      iterator = primes.begin();
      for (int i = 0; i < primes.size(); i++) {
         if (*iterator >= 1) {
            check2 *= *iterator;
            if (check2 <= num * 10) {
               // std::cout << "List3 size: " << primes.size() << std::endl;
               // std::cout << "List3 at " << i << " : " << *iterator << std::endl;
               // std::cout << "Check3 = " << check2 << std::endl;
            }
            std::advance(iterator, 1);
            if (check2 == num) {
               std::cout << "Mission complete!3 Primes are: " << std::endl;
               iterator = primes.begin();
               check2 = 1;
               for (int i = 0; i < primes.size(); i++) {
                  if (*iterator >= 1) {
                     std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        std::cout << std::endl;
                        _status = s_menu;
                        div_found = true;
                        num = getUsername();
                        return 0;
                        std::cout << "Crashing now, don't mind me." << std::endl;
                        //return;
                     }
                  }
                  std::advance(iterator, 1);
               }
            }
         }
      }
   } 

   
   // Now the 3s
   while (newval % 3 == 0) {
      check2 = 1;
      primes.push_front(3);
      std::cout << "Prime Found: 3\n";
      newval = newval / 3;
      iterator = primes.begin();
      for (int i = 0; i < primes.size(); i++) {
         if (*iterator >= 1) {
            check2 *= *iterator;
            if (check2 <= num * 10) {
               // std::cout << "List4 size: " << primes.size() << std::endl;
               // std::cout << "List4 at " << i << " : " << *iterator << std::endl;
               // std::cout << "Check4 = " << check2 << std::endl;
            }
            std::advance(iterator, 1);
            if (check2 == num) {
               std::cout << "Mission complete!4 Primes are: " << std::endl;
               iterator = primes.begin();
               check2 = 1;
               for (int i = 0; i < primes.size(); i++) {
                  std::cout << "Test #" << i << std::endl;
                  if (*iterator >= 1) {
                     std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        std::cout << std::endl;
                        _status = s_menu;
                        div_found = true;
                        num = getUsername();
                        return 0;
                        std::cout << "Crashing now, don't mind me." << std::endl;
                        //return;
                     }
                  }
                  std::advance(iterator, 1);
               }
            }
         }
      }
   }

   
   // Now the 5s
   while (newval % 5 == 0) {
      check2 = 1;
      primes.push_front(5);
      std::cout << "Prime Found: 5\n";
      newval = newval / 5;
      iterator = primes.begin();
      for (int i = 0; i < primes.size(); i++) {
         if (*iterator >= 1) {
            check2 *= *iterator;
            if (check2 <= num * 10) {
               // std::cout << "List3 size: " << primes.size() << std::endl;
               // std::cout << "List3 at " << i << " : " << *iterator << std::endl;
               // std::cout << "Check3 = " << check2 << std::endl;
            }
            std::advance(iterator, 1);
            if (check2 == num) {
               std::cout << "Mission complete!5 Primes are: " << std::endl;
               iterator = primes.begin();
               check2 = 1;
               for (int i = 0; i < primes.size(); i++) {
                  // std::cout << "Test #" << i << std::endl;
                  if (*iterator >= 1) {
                     std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        std::cout << std::endl;
                        _status = s_menu;
                        div_found = true;
                        num = getUsername();
                        return 0;
                        std::cout << "Crashing now, don't mind me." << std::endl;
                        //return;
                     }
                  }
                  std::advance(iterator, 1);
               }
            }
         }
      }
   }
   check2 = 1;
   // std::cout << "Ready for more math!@!!!!" << std::endl;
   // exit;

   // Now use Pollards Rho to figure out the rest. As it's stochastic, we don't know
   // how long it will take to find an answer. Should return the final two primes
   if (check2 != num ) {
      //std::cout << "First check" << std::endl;
      factor(newval);
      }

   
//    // Get password 
//    _connfd.writeFD("Password: ");
//    if (getUserInput(_newpwd) == false) {
//       std::cout << "Error getting password.\n";
//    }

//    // Check password limiting to 2 attempts
//    if (!pmgr.checkPasswd(_username.c_str(),_newpwd.c_str())) {
//       if (_pwd_attempts == 0) {
//          _connfd.writeFD("\nIncorrect password. 1 attempt remaining.\n");
//          _pwd_attempts += 1;
//       } 
//       // Log incorrect login attempt and disconect user
//       else if (_pwd_attempts == 1) {
//          _connfd.writeFD("\nIncorrect password. Goodbye.\n");
//          std::string IP;
//          getIPAddrStr(IP);
//          std::string log = IP + " - Validation failed - " + _username;
//          if (!writeLog(log)) {
//             std::cout << "Unable to write to log.\n";
//          }
//          disconnect();
//       } 
//    } 
//    // On success, log authentication and pass user to menu
//    else {
//          std::cout << "Successfully authenticated.\n";
//          _connfd.writeFD("Success!");
//          std::string IP;
//          getIPAddrStr(IP);
//          std::string log = IP + " - Authenticated - " + _username;
//          if (!writeLog(log)) {
//             std::cout << "Unable to write to log.\n";
//          }
//          sendMenu();
//          _status = s_menu;
//          handleConnection();
//       }
}

/**********************************************************************************************
 * changePassword - called from handleConnection when status is s_changepwd or s_confirmpwd--
 *                  if it finds user data, with status s_changepwd, it saves the user-entered
 *                  password. If s_confirmpwd, it checks to ensure the saved password from
 *                  the s_changepwd phase is equal, then saves the new pwd to the database
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

// void TCPConn::changePassword() {
//    std::string _newpwd2;

//    // Get password once
//    _connfd.writeFD("Enter new Password: ");
//    fflush(stdout);
//    getUserInput(_newpwd);
//    clrNewlines(_newpwd); 

//    // Get password second time
//    _connfd.writeFD("Enter password again: ");
//    fflush(stdout);
//    getUserInput(_newpwd2);
//    clrNewlines(_newpwd2);

//    // If passwords match, change it
//    if (_newpwd == _newpwd2) {
//       if (!pmgr.changePasswd(_username.c_str(), _newpwd.c_str())) {
//       std::cout << "Error changing password.\n";
//       }
//    } else {
//       _connfd.writeFD("Passwords did not match. Returning to menu.\nEnter a command.\n");
//       _status = s_menu;
//       return;
//    }
   
//    // Log password changed and send user back to menu
//    _connfd.writeFD("Password changed successfully!\nEnter a command.\n");
//    std::string IP;
//    getIPAddrStr(IP);
//    std::string log = IP + " - Password changed - " + _username;
//    if (!writeLog(log)) {
//       std::cout << "Unable to write to log.\n";
//    }
//    _status = s_menu;
// }


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
   // } else if (cmd.compare("passwd") == 0) {
   //    _status = s_changepwd;
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
      // std::string log = IP + " - User is bored! - " + _username;
      // if (!writeLog(log)) {
      //    std::cout << "Unable to write to log.\n";
      // }
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

// bool TCPConn::whitelist() {
//    // Adapted from http://cplusplus.com/forum/beginner/80838/
//    // Get client's IP from getIPAddrStr()
//    // Open whitelist file
//    std::string clientIP, list; 
//    _connfd.getIPAddrStr(clientIP);

//    std::ifstream whitelist;
//    whitelist.open("whitelist");

//    // Go through the list, remove trailing \n, compare it to client IP
//    while (whitelist >> list) {
//       list.erase(std::remove(list.begin(), list.end(), '\n'), list.end());
//       // std::cout << list;
//       // std::cout << clientIP;
//       if (list == clientIP) {
//          //std::cout << "Good on whitelist.\n";
//          return true;
//       } 
//    }
//    std::cout << "Attempted connection from unknown IP. Incident logged.\n";
//    return false;
// }

bool TCPConn::writeLog(std::string &buf){
   // Find end of file, start new line
   std::ofstream out("server.log", std::ios::app);

   // Get the time, add formatting, print IP/message/user from buf
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

int getNumber() {
   std::cout << "What is your starting number?" << std::endl;
   int n;
   std::cin >> n;
   return n;
}