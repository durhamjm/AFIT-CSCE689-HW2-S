#ifndef PASSWDMGR_H
#define PASSWDMGR_H

#include <string>
#include <stdexcept>
#include "FileDesc.h"

/****************************************************************************************
 * PasswdMgr - Manages user authentication through a file
 *
 ****************************************************************************************/

class PasswdMgr {
   public:
      PasswdMgr(const char *pwd_file);
      ~PasswdMgr();

      bool checkUser(const char *name);
      bool checkPasswd(const char *name, const char *passwd);
      bool changePasswd(const char *name, const char *newpassd);
   
      void addUser(const char *name, const char *passwd);

      void hashArgon2(std::vector<uint8_t> &ret_hash, std::vector<uint8_t> &ret_salt, const char *passwd, const char *in_salt);

   private:
      bool findUser(const char *name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt);
      bool readUser(FileFD &pwfile, std::string &name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt);
      int writeUser(FileFD &pwfile, std::string &name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt);

      std::string _pwd_file;

      uint32_t t_cost = 3;
      uint32_t m_cost = 12;
      uint32_t parallelism = 1;
      void *dest;
};

#endif
