#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <strings.h>
#include <stropts.h>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdexcept>
#include "TCPClient.h"
#include "TCPConn.h"
#include <iostream>
#include <sstream>


/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
 **********************************************************************************************/
TCPClient::TCPClient() {
}

/**********************************************************************************************
 * TCPClient (destructor) - No cleanup right now
 *
 **********************************************************************************************/
TCPClient::~TCPClient() {
}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
 **********************************************************************************************/
void TCPClient::connectTo(const char *ip_addr, unsigned short port) {
   if (!_sockfd.connectTo(ip_addr, port))
      throw socket_error("TCP Connection failed!");
}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() {
   
   bool connected = true;
   int sin_bufsize = 0;
   ssize_t rsize = 0;

   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 1000000;

   primes = {};
   primes.resize(0);
   check2 = 1;
   div_found = false;

   // Loop while we have a valid connection
   while (connected) {

      // If the connection was closed, exit
      if (!_sockfd.isOpen())
         break;

      // Send any user input
      if ((sin_bufsize = readStdin()) > 0)  {
         std::string subbuf = _in_buf.substr(0, sin_bufsize+1);
         _sockfd.writeFD(subbuf);
         _in_buf.erase(0, sin_bufsize+1);
      }

      // Read any data from the socket and display to the screen and handle errors
      std::string buf;
      if (_sockfd.hasData()) {
         if ((rsize = _sockfd.readFD(buf)) == -1) {
            throw std::runtime_error("Read on client socket failed.");
         }

         // Select indicates data, but 0 bytes...usually because it's disconnected
         if (rsize == 0) {
            closeConn();
            break;
         }

         // Display to the screen
         if (rsize > 0) {
            printf("%s", buf.c_str());
            //std::cout << "buf is: " << buf.c_str();
            fflush(stdout);
            //std::size_t found = buf.find("Get to work!");
            //if (found != std::string::npos) {
               std::stringstream num2(buf);
               num2 >> num;
               num2.str("");
               //std::cin >> num;
               std::cout << "We have a number of " << num << std::endl;
               getPasswd(num);
               iterator = primes.begin();
               check2 = 1;
               for (int i = 0; i < primes.size(); i++) {
                  if (*iterator >= 1) {
                     num3 << *iterator;
                     msg2 += num3.str();
                     msg2 += ", ";
                     // _sockfd.writeFD(msg2);
                     // _sockfd.writeFD2(msg2);
                     // _sockfd.writeFD3(msg2);
                     // _sockfd.writeFD(*iterator);
                     // _sockfd.writeFD(", ");
                     //std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        msg2 += "\n";
                        _sockfd.writeFD(msg2);
                        std::cout << msg2;
                        //std::cout << std::endl;
                        // _status = s_menu;
                        //div_found = true;
                        //handleConnection();
                        //std::cout << "Crashing now, don't mind me." << std::endl;
                        //handleConnection();
                        return;
                     }
                  }
                  std::advance(iterator, 1);
               }
            // }
         }
      }
      // So we're not chewing up CPU cycles unnecessarily
      nanosleep(&sleeptime, NULL);
   }
}

// boost::multiprecision::uint256_t getnum(boost::multiprecision::uint256_t n) {
//    num = n;
//    std::cout << "Number is " << num << ". Now get to work!" << std::endl;
// }



boost::multiprecision::uint256_t TCPClient::modularPow(boost::multiprecision::uint256_t base, boost::multiprecision::uint256_t exponent, boost::multiprecision::uint256_t modulus) {
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

bool TCPClient::isPrimeBF(boost::multiprecision::uint256_t n, boost::multiprecision::uint256_t &divisor) {
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


boost::multiprecision::uint256_t TCPClient::calcPollardsRho(boost::multiprecision::uint256_t n) {
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


void TCPClient::combinePrimes(std::list<boost::multiprecision::uint256_t> &dest) {
   dest.insert(dest.end(), primes.begin(), primes.end());
}


void TCPClient::factor(boost::multiprecision::uint256_t n) {
   // already prime
   if (n == 1) {
      return;
   }
   if (div_found) {
      std::cout << "Stop calling me!" << std::endl;
      handleConnection();
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
                              // _status = s_menu;
                              div_found = true;
                              handleConnection();
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
                        // _status = s_menu;
                        div_found = true;
                        handleConnection();
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
   handleConnection();
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

boost::multiprecision::uint256_t TCPClient::getPasswd(boost::multiprecision::uint256_t n) {
   if (n <= 3) {
      return n;
   }



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
                        // _status = s_menu;
                        div_found = true;
                        handleConnection();
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
                  //std::cout << "Test #" << i << std::endl;
                  if (*iterator >= 1) {
                     std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        std::cout << std::endl;
                        // _status = s_menu;
                        div_found = true;
                        //handleConnection();
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
               msg2 = "";
               for (int i = 0; i < primes.size(); i++) {
                  if (*iterator >= 1) {
                     num3 << *iterator;
                     msg2 += num3.str();
                     num3.str("");
                     // _sockfd.writeFD(msg2);
                     // _sockfd.writeFD2(msg2);
                     // _sockfd.writeFD3(msg2);
                     // _sockfd.writeFD(*iterator);
                     // _sockfd.writeFD(", ");
                     //std::cout << *iterator << ", ";
                     check2 *= *iterator;
                     if (check2 == num) {
                        msg2 += "\n";
                        _sockfd.writeFD(msg2);
                        std::cout << msg2;
                        //std::cout << std::endl;
                        // _status = s_menu;
                        div_found = false;
                        check2 = 1;
                        primes = {};
                        primes.resize(0);
                        std::cout << "Crashing now, don't mind me." << std::endl;
                        msg2 = "";
                        //handleConnection();
                        num = 0;
                        return 0;
                     }
                  }
                  std::advance(iterator, 1);
                  msg2 += ", ";
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
}





/**********************************************************************************************
 * closeConnection - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() {
    _sockfd.closeFD(); 
}

/******************************************************************************
 * readStdin - takes input from the user and stores it in a buffer. We only send
 *             the buffer after a carriage return.
 *
 *    Return: 0 if not ready to send, buffer length if ready.
 *****************************************************************************/
int TCPClient::readStdin() {

   if (!_stdin.hasData()) {
      return 0;
   }

   // More input, get it and concat it to the buffer
   std::string readbuf;
   int amt_read;
   if ((amt_read = _stdin.readFD(readbuf)) < 0) {
      throw std::runtime_error("Read on stdin failed unexpectedly.");
   }

   _in_buf += readbuf;

   // Did we either fill up the buffer or is there a newline/carriage return?
   int sendto;
   if (_in_buf.length() >= stdin_bufsize)
      sendto = _in_buf.length();
   else if ((sendto = _in_buf.find("\n")) == std::string::npos) {
      return 0;
   }
   
   return sendto;
}