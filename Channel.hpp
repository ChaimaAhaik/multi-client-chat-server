#pragma once

#include "Server.hpp"

class Client;

class Channel
{
	private:
		std::vector<Client> members;
		std::vector<Client> operators;
		std::vector<Client> invited_list;
		std::string name;
		std::string topic;
		std::string password;
		bool i, t, k, o, l;
		size_t max;

	public:
		bool first_creation;
		Channel(std::string name);
		bool key();
		void set_i(Client & setter, bool);
		void set_k(Client & setter, bool, std::string password);
		void set_l(Client & setter, bool, size_t max);
		void set_o(bool, std::string member_name, Client & setter);
		void set_t(Client & setter, bool);
		void setTopic(std::string topic, Client & c);

		std::string  show_topic() const;
		std::string getName() const;
		std::string  getPassword() const;
		int getIndexOfMember(std::string) const;
		int getFD(int index) const;

		bool max_client() const;
		bool invited_only() const;
		bool isMember(Client & c) const;
		bool isOperator(Client & c) const;
		bool is_invited(Client & c);

		void part_From_Channel(Client & c);
		void add_to_invited_list(Client & c);
		void addOperator(Client & op , Client &to_be_added);
		bool removeOperator(Client & me, Client & c);
		void removeFromChannel(Client & c, Client & me);
		void addToChannel(Client & me, Client & c, std::string passwordd, std::string role);
		std::string members_channels(void);

		void broadcast(std::string msg);
		void sendMessageToChannel(Client &c, std::string &message);
		bool bot_detection(std::string msg, Client & c);

};