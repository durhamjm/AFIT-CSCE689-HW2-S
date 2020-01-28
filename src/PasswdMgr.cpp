#include <argon2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <list>
#include "PasswdMgr.h"
#include "FileDesc.h"
#include "strfuncts.h"
#include <fstream>
#include <cstdio>

const int hashlen = 32;
const int saltlen = 16;

PasswdMgr::PasswdMgr(const char *pwd_file):_pwd_file(pwd_file) {
}


PasswdMgr::~PasswdMgr() {
}

/*******************************************************************************************
 * checkUser - Checks the password file to see if the given user is listed
 *
 *    Throws: pwfile_error if there were unanticipated problems opening the password file for
 *            reading
 *******************************************************************************************/

bool PasswdMgr::checkUser(const char *name) {
   std::vector<uint8_t> passwd, salt;

   bool result = findUser(name, passwd, salt);

   return result;
}

/*******************************************************************************************
 * checkPasswd - Checks the password for a given user to see if it matches the password
 *               in the passwd file
 *
 *    Params:  name - username string to check (case insensitive)
 *             passwd - password string to hash and compare (case sensitive)
 *    
 *    Returns: true if correct password was given, false otherwise
 *
 *    Throws: pwfile_error if there were unanticipated problems opening the password file for
 *            reading
 *******************************************************************************************/

bool PasswdMgr::checkPasswd(const char *name, const char *passwd) {
   std::vector<uint8_t> userhash; // hash from the password file
   std::vector<uint8_t> passhash; // hash derived from the parameter passwd
   std::vector<uint8_t> salt;

   // Check if the user exists and get the passwd string
   if (!findUser(name, userhash, salt))
      return false;

   hashArgon2(passhash, salt, passwd);

   if (userhash == passhash)
      return true;

   return false;
}

/*******************************************************************************************
 * changePasswd - Changes the password for the given user to the password string given
 *
 *    Params:  name - username string to change (case insensitive)
 *             passwd - the new password (case sensitive)
 *
 *    Returns: true if successful, false if the user was not found
 *
 *    Throws: pwfile_error if there were unanticipated problems opening the password file for
 *            writing
 *
 *******************************************************************************************/

bool PasswdMgr::changePasswd(const char *name, const char *passwd) {

   // Insert your insane code here
   // Ask user for new password

   // Run it through hash
   // hashArgon2(ret_hash?, ret_salt?, passwd, in_salt?)

   // write it to file
   // writeUser(pwfile, name, hash, salt)
   std::cout << "Password updated successfully!\n";
   return true;
}

/*****************************************************************************************************
 * readUser - Taking in an opened File Descriptor of the password file, reads in a user entry and
 *            loads the passed in variables
 *
 *    Params:  pwfile - FileDesc of password file already opened for reading
 *             name - std string to store the name read in
 *             hash, salt - vectors to store the read-in hash and salt respectively
 *
 *    Returns: true if a new entry was read, false if eof reached 
 * 
 *    Throws: pwfile_error exception if the file appeared corrupted
 *
 *****************************************************************************************************/

bool PasswdMgr::readUser(FileFD &pwfile, std::string &name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt)
{
   // Insert your perfect code here!
   if((pwfile.readStr(name) <= 0)) {
      return false;
   } 
   // Read the /n
   std::vector<uint8_t> tempByte;
   if ((pwfile.readBytes<uint8_t>(tempByte, 1) <= 0)) {

   }

   if((pwfile.readBytes<uint8_t>(hash, 32) <= 0)) {
      return false;
   }

   if((pwfile.readBytes<uint8_t>(salt, 16) <= 0)) {
      return false;
   }

   // read the /n
   if ((pwfile.readBytes<uint8_t>(tempByte, 1) <= 0)) {

   }
   return true;
}

/*****************************************************************************************************
 * writeUser - Taking in an opened File Descriptor of the password file, writes a user entry to disk
 *
 *    Params:  pwfile - FileDesc of password file already opened for writing
 *             name - std string of the name 
 *             hash, salt - vectors of the hash and salt to write to disk
 *
 *    Returns: bytes written
 *
 *    Throws: pwfile_error exception if the writes fail
 *
 *****************************************************************************************************/

int PasswdMgr::writeUser(FileFD &pwfile, std::string &name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt)
{
   int results = 0;

   // Insert your wild code here!
   //Open file
   //FileFD pwfile2(pwfile.c_str());

   //Find end of file, start new line
   std::ofstream out;
   // out.open(pwfile std::ios::app); ** gives error about type

   //out << username << \n << { << hash << }{ << salt << }\n

   return results; 
}

/*****************************************************************************************************
 * findUser - Reads in the password file, finding the user (if they exist) and populating the two
 *            passed in vectors with their hash and salt
 *
 *    Params:  name - the username to search for
 *             hash - vector to store the user's password hash
 *             salt - vector to store the user's salt string
 *
 *    Returns: true if found, false if not
 *
 *    Throws: pwfile_error exception if the pwfile could not be opened for reading
 *
 *****************************************************************************************************/

bool PasswdMgr::findUser(const char *name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt) {

   FileFD pwfile(_pwd_file.c_str());

   // You may need to change this code for your specific implementation

   if (!pwfile.openFile(FileFD::readfd))
      throw pwfile_error("Could not open passwd file for reading\n");

   // Password file should be in the format username\n{32 byte hash}{16 byte salt}\n
   bool eof = false;
   while (!eof) {
      std::string uname;
      //std::cout << "About to check username\n";
      if (!readUser(pwfile, uname, hash, salt)) {
         eof = true;
         continue;
      }
      //std::cout << "About to check username2\n";
      if (!uname.compare(name)) {
         pwfile.closeFD();
         return true;
      }
   }
   std::cout << "Leaving findUser()\n";
   hash.clear();
   salt.clear();
   pwfile.closeFD();
   return false;
}


/*****************************************************************************************************
 * hashArgon2 - Performs a hash on the password using the Argon2 library. Implementation algorithm
 *              taken from the http://github.com/P-H-C/phc-winner-argon2 example. 
 *
 *    Params:  dest - the std string object to store the hash
 *             passwd - the password to be hashed
 *
 *    Throws: runtime_error if the salt passed in is not the right size
 *****************************************************************************************************/
void PasswdMgr::hashArgon2(std::vector<uint8_t> &ret_hash, std::vector<uint8_t> &ret_salt, 
                           const char *in_passwd) {
   // Hash those passwords!!!!

   std::vector<uint8_t> *salt, *hash;
   int i = 0;
   argon2i_hash_raw(t_cost, m_cost, parallelism, in_passwd, sizeof(in_passwd), salt, saltlen, hash, hashlen);

   for (i; i < saltlen; i++) {
      salt[i] = ((rand() % 93) + 33);
   }

   ret_hash.clear();
   ret_salt.clear();
   //ret_hash = hash;
   for (i; i < hashlen; i++) {
      ret_hash[i] = hash[i];
   }
   //ret_salt = salt;
   for (i; i < saltlen; i++) {
      ret_salt[i] = salt[i];
   }
}

/****************************************************************************************************
 * addUser - First, confirms the user doesn't exist. If not found, then adds the new user with a new
 *           password and salt
 *
 *    Throws: pwfile_error if issues editing the password file
 ****************************************************************************************************/

void PasswdMgr::addUser(const char *name, const char *passwd) {
   // Add those users!

   //Ask for new username, then add it (with a \n)
   std::cout << "Adding new user...\n";
   std::cout << "Enter username: \n";
   scanf("%c", name);

   //Ask for new password, run in through the hash w/ salt 
   std::cout << "Enter password: \n";
   scanf("%c", passwd);

   FileFD pwfile(_pwd_file.c_str());

   //create the hash
   //writeUser(pwfile, name, hash, salt);

}

