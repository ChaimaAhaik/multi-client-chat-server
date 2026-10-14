#pragma once

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <poll.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <sstream>
#include <signal.h>
#include "my_macros.hpp"
#include "Client.hpp"
#include "Channel.hpp"

class Client;
class Channel;

class Server{
	private:
		int serv_fd;
		std::string password;
		std::vector<struct pollfd> fds;
		std::map<int, Client> client;
		std::vector<Channel> channels;
	
	public:
		Server(int port, const std::string &password);
		void run();
		void add_new_client();
		void remove_client(int client_key);
		void handle_client(int indx);
		void parse_cmd(std::string cmd, int client_key);
		void password_validation(std::istringstream &iss, int client_key);
		bool check_nick_in_use(std::string &nick, int client_key);
		int is_channel(std::string name, int client_key);
		int channels_size() const;
		std::string channel_name(int index) const;
		void create_new_channel(std::string name, int client_key);
		int is_channel_for_(std::string name, int client_key);
		int getIndexOfClient(std::string nickname) const;


		void NICK_cmd(std::istringstream &iss, int client_key);
		void USER_cmd(std::istringstream &iss, int client_key);
		void PRIVMSG_cmd(std::istringstream &iss, int client_key);
		void JOIN_cmd(std::istringstream &iss, int client_key);
		void MODE_cmd(std::istringstream &iss, int client_key);
		void KICK_cmd(std::istringstream &iss, int client_key);
		void TOPIC_cmd(std::istringstream &iss, int client_key);
		void INVITE_cmd(std::istringstream &iss, int client_key);
		void PART_cmd(std::istringstream &iss, int client_key);
		void QUIT_cmd(std::istringstream &iss, int client_key);
};

void handle_signal(int sig);
void passwords(std::string & password, std::vector<std::string> & psswd);
void canal(std::string & channel_name, std::vector<std::string> &channels);
