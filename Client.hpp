#pragma once

#include "Server.hpp"

class Client{
    private:
        std::string buffer;
        std::string Nickame;
        std::string username;
        std::string realname;
        int         fd;
        bool       is_registered;
        bool       is_passed;
        bool       is_nick_set;
        bool       is_user_set;
    
    public:
        int pass_try;
        Client();
        std::string append(char *buf);
        void setFd(int fd);
        int getFd() const;
        void setNickname(const std::string& nickname);
        std::string getNickname() const;
        bool isNickSet() const;
        void setUsername(const std::string& username);
        std::string getUsername() const;
        void setRealname(const std::string& realname);
        std::string getRealname() const;
        bool isUserSet() const;
        void setRegistered(bool status);
        bool getRegistered() const;
        void setPassed(bool status);
        bool getPassed() const;
        bool operator==(const Client& other) const;
};
