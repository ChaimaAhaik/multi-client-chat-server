#include "Client.hpp"

Client::Client(){
	this->fd = -1;
	this->is_registered = 0;
	this->is_passed = 0;
	this->is_nick_set = 0;
	this->is_user_set = 0;
	this->buffer = "";
	this->pass_try = 0;
}

bool Client::operator==(const Client& other) const {
	return (this->fd == other.fd);
}

std::string Client::append(char *buf){
	size_t pos = 0;
	if (buffer.find("\r\n") != std::string::npos){
		pos = buffer.find("\r\n") + 2;
		buffer = buffer.substr(pos);
	}
	buffer.append(buf);
	return buffer;
}

void Client::setFd(int fd){
	this->fd = fd;
}

int Client::getFd() const{
	return this->fd;
}

void Client::setNickname(const std::string& nickname){
	this->Nickame = nickname;
	this->is_nick_set = true;
}

std::string Client::getNickname() const{
	return this->Nickame;
}

bool Client::isNickSet() const{
	return this->is_nick_set;
}

void Client::setUsername(const std::string& username){
	this->username = username;
	this->is_user_set = true;
}
std::string Client::getUsername() const{
	return this->username;
}
void Client::setRealname(const std::string& realname){
	this->realname = realname;
}
std::string Client::getRealname() const{
	return this->realname;
}
bool Client::isUserSet() const{
	return this->is_user_set;
}
void Client::setRegistered(bool status)
{
    this->is_registered = status;
}
bool Client::getRegistered() const{
	return this->is_registered;
}
void Client::setPassed(bool status){
	this->is_passed = status;
}
bool Client::getPassed() const{
	return this->is_passed;
}