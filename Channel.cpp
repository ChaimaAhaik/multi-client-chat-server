#include "Channel.hpp"

Channel::Channel(std::string name) 
{
	this->name = name;
	this->topic = "";
	this->password = "";
	this->i = false;
	this->t = false;
	this->k = false;
	this->l = false;
	this->o = true;
	this->max = 0;
	first_creation = true;

	Client bt;
	bt.setNickname("BOT");
	bt.setFd(-2);
	operators.push_back(bt);
}

bool Channel::bot_detection(std::string msg, Client & c)
{
	std::string word;
	std::istringstream iss(msg);
	std::string bad_words[5] = {"shit",  "bastard", "idiot" , "stupid",  "dumb"}; // add list of bad words libghity
	std::string message;
	for (int i = 0; i < 5; i++)
	{
		if (msg.find(bad_words[i]) != std::string::npos)
		{
			message = ":" + operators[0].getNickname() + "!bot@host" + " KICK " + getName() + " " + c.getNickname() + " :inappropriate message detected\r\n";
			send(c.getFd(), message.c_str(), message.length(), 0);
			removeFromChannel(c , operators[0]);
			return true;
		}
	}
	return false;
}

std::string Channel::members_channels(void)
{
	std::string list;
	list = "@BOT ";
	for (size_t i = 0; i < members.size(); i++)
	{
		if (isOperator(members[i]))
			list += "@" + members[i].getNickname() + " ";
		else
			list += members[i].getNickname() + " ";
	}
	if (!list.empty())
		list = list.substr(0, list.length() - 1);
	return list;
}

void Channel::addToChannel(Client & me, Client & c, std::string passwordd, std::string role)
{
	std::string msg;
	if (isMember(c))
	{
		msg = ":server 443 " + c.getNickname() + " " + name + " :is already on channel\r\n";
		// std::cout << msg + "<< " << me.getFd() << " >>" << std::endl;
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	else if (l && members.size() == max)
	{
		 msg = ":server 471 " + c.getNickname() + " " + name + " :Cannot be added to channel (+l)\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	else if (i && is_invited(c) &&!isOperator(me) && !first_creation)
	{
		 msg = ":server 473 " + c.getNickname() + " " + name + " :Cannot join channel (+i)\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	else if (k && passwordd != this->password)
	{
		 msg = ":server 475 " + c.getNickname() + " " + name + " :Cannot join channel (+k)\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	members.push_back(c);
	if (role == "admin")
		this->addOperator(me,c);
	msg = ":" + me.getNickname() + " JOIN :" + getName() + "\r\n";
	broadcast(msg);
	msg = ":server 332 " + c.getNickname() + " " + name + " :" + topic + "\r\n";
	msg += ":server 353 " + c.getNickname() + " = " + name + " :" + members_channels() + "\r\n";
	msg += ":server 366 " + c.getNickname() + " " + name + " :End of /NAMES list.\r\n";
	send(c.getFd(), msg.c_str(), msg.length(), 0);
	first_creation = false;
}

void Channel::removeFromChannel(Client & c, Client & me)
{
	std::string msg;
	if (me.getNickname() == operators[0].getNickname())
	{
		std::vector<Client>::iterator it = std::find(members.begin(), members.end(),  c);
		std::vector<Client>::iterator ito = std::find(operators.begin(), operators.end(),  c);
		if (it != members.end())
		{
			members.erase(it);
			if (ito != operators.end())
				operators.erase(ito);
		}
		return ;
	}
	if (c.getNickname() == operators[0].getNickname())
	{
		msg = ":server 481 " + me.getNickname() + " :You can't kick the bot\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	std::vector<Client>::iterator it = std::find(members.begin(), members.end(),  c);
	if (!isOperator(me))
	{
		msg = ":server 482 " + me.getNickname() + " " + name + " :you can't kick someone cause u're not an operator\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	else if (it != members.end())
	{
		if (!isOperator(c))
		{
			if (!removeOperator(me, c))
			return ;
		}
		members.erase(it);
		msg = ":" + me.getNickname() + " KICK " + name + " " + c.getNickname() + " : 😈 hehehehe" + "\r\n";
		broadcast(msg);
		return ;
	}
	else if (it == members.end())
	{
		msg = ":server 441  " + me.getNickname() + " " + c.getNickname() + " " + name + " :is not on that channel\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
}

bool Channel::removeOperator(Client & me, Client & c)
{
	std::vector<Client>::iterator itoper = std::find(operators.begin(), operators.end(), c);
	if (itoper == operators.end())
	{
		std::string msg = ":server 441 " + c.getNickname() + " : are not an operator on the channel\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return false;
	}
	else if ((me == operators[1]))
	{
		operators.erase(itoper);
		return true;
	}
	else
	{
		std::string msg = ":server 482 " + me.getNickname() + " :You're not the channel's super operator\r\n";
		send(me.getFd(), msg.c_str(), msg.length(), 0);
		return false;
	}
	return false;
}

void Channel::set_i(Client & setter, bool mode)
{
	if (!isOperator(setter))
	{
		std::string msg = ":server 482 " + setter.getNickname() + " " + name + " :You're not a channel operator\r\n";
		send(setter.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	if (mode)
		this->i = true;
	else
		this->i = false;
}

void Channel::set_k(Client & setter, bool mode, std::string password)
{
	if (!isOperator(setter))
	{
		std::string msg = ":server 482 " + setter.getNickname() + " " + name + " :You're not a channel operator\r\n";
		send(setter.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	if (mode)
	{
		this->k = true;
		this->password = password;
	}else {
		this->k = false;
		password.clear();
	}
}

void Channel::set_l(Client & setter, bool mode, size_t max)
{
	if (!isOperator(setter)) {
		std::string msg = ":server 482 " + setter.getNickname() + " " + name + " :You're not a channel operator\r\n";
		send(setter.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	if (mode)
	{
		this->l = true;
		this->max = max;
	}
	else
		this->l = false;

}

void Channel::set_o(bool mode, std::string member_name, Client & setter)
{
	int index = getIndexOfMember(member_name);
	if (index == -1)
	{
		std::string msg = ":server 441 " + setter.getNickname() + " " + name + " " + member_name + " :not a member on the channel\r\n";
		send(setter.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	if (mode)
		addOperator(setter, members[index]);
	else{
		if (removeOperator(setter, members[index])){
			std::string msg = ":" + setter.getNickname() + " MODE " + name + " -o " + member_name + "\r\n";
			broadcast(msg);
		}
	}
}

void Channel::set_t(Client & setter, bool mode)
{
	if (!isOperator(setter))
	{
		std::string msg = ":server 482 " + setter.getNickname() + " " + name + " :You're not a channel operator\r\n";
		send(setter.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	if (mode)
		this->t = true;
	else
		this->t = false;
}


void Channel::setTopic(std::string topic, Client & c)
{
	std::string msg;
	std::vector<Client>::iterator it = std::find(operators.begin(), operators.end(), c);
	if (this->t)
	{
		if (it == operators.end())
		{
			std::string msg = ":server 482 " + c.getNickname() + " " + name + " :You're not a channel operator\r\n";
			send(c.getFd(), msg.c_str(), msg.length(), 0);
			return ;
		}
	}
	if (topic.empty())
	{
		msg = ":" + c.getNickname() + " cleared the topic\r\n";
		broadcast(msg);
		this->topic = "";
		return ;
	}
	this->topic = topic;
	msg = ":" + c.getNickname() + " TOPIC " + name + " :" + topic + "\r\n";
	broadcast(msg);
}

std::string  Channel::getPassword() const
{
	return (this->password);
}

std::string  Channel::show_topic() const
{
	return (this->topic);
}

bool Channel::isOperator(Client & c) const
{
	std::vector<Client>::const_iterator it = std::find(operators.begin(), operators.end(), c);
	if (it != operators.end())
		return true;
	return false;
}

bool Channel::is_invited(Client & c)
{
	std::vector<Client>::iterator it = std::find(invited_list.begin(), invited_list.end(), c);
	if (it != invited_list.end())
	{
		invited_list.erase(it);
		return true;
	}
	return false;
}

bool Channel::isMember(Client & c) const
{
	std::vector<Client>::const_iterator it = std::find(members.begin(), members.end(), c);
	if (it != members.end())
		return true;
	return false;
}

std::string Channel::getName() const
{
	return name;
}

bool Channel::key()
{
	if (this->k)
		return true;
	return false;
}

bool Channel::max_client() const
{
	if (l && members.size() == max)
		return true;
	return false;
}

bool Channel::invited_only() const
{
	if (this->i)
		return true;
	return false;
}

void Channel::addOperator(Client & op , Client & to_be_added)
{
	std::string msg;
	if (!first_creation && (!isOperator(op) || !isMember(to_be_added))){
		std::string msg = ":server 482 " + op.getNickname() + " " + name + " :You're not a channel operator\r\n";
		send(op.getFd(), msg.c_str(), msg.length(), 0);
		return ;
	}
	if (!isOperator(to_be_added))
		operators.push_back(to_be_added);
	if (first_creation)
		return ;
	else
		msg = ":" + op.getNickname() + " MODE " + name + " +o " + to_be_added.getNickname() + "\r\n";
	broadcast(msg);
}

int Channel::getIndexOfMember(std::string nickname) const
{
	for (size_t i = 0; i < members.size(); i++)
	{
		if (members[i].getNickname() == nickname)
			return i;
	}
	return -1;
}

int Channel::getFD(int index) const{
    return members[index].getFd();
}

void Channel::add_to_invited_list(Client & c)
{
	if (!isMember(c))
		invited_list.push_back(c);
}

void Channel::part_From_Channel(Client & c)
{
	std::vector<Client>::iterator it = std::find(members.begin(), members.end(), c);
	std::vector<Client>::iterator itoper = std::find(operators.begin(), operators.end(), c);
	members.erase(it);
	if (itoper != operators.end())
		operators.erase(itoper);
}

void Channel::broadcast(std::string msg)
{
	for (size_t i = 0; i < members.size(); i++)
		send(members[i].getFd(), msg.c_str(), msg.length(), 0);
}

void Channel::sendMessageToChannel(Client &c, std::string &message){
	if (bot_detection(message, c))
	{
		message = ":" + operators[0].getNickname() + "!bot@host" + " KICK " + getName() + " " + c.getNickname() + " :inappropriate message detected\r\n";
		broadcast(message);
		return  ;
	}
	std::string full_msg = ":" + c.getNickname() + "!user@host" + " PRIVMSG " + getName() + " :" + message + "\r\n";
	for (size_t i = 0; i < members.size(); i++){
		if (members[i].getFd() != c.getFd()){
			send(members[i].getFd(), full_msg.c_str(), full_msg.length(), 0);
		}
	}
}
