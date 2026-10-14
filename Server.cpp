#include "Server.hpp"

int g_signal;

std::string trimstr(const std::string &str) {
	size_t start = 0;
	
	while (start < str.length() && std::isspace(str[start]))
		++start;
	size_t end = str.length();
	while (end > start && std::isspace(str[end - 1]))
		--end;
	return str.substr(start, end - start);
}

int Server::getIndexOfClient(std::string nickname) const
{
	for (std::map<int, Client>::const_iterator it = client.begin(); it != client.end(); it++){
		if (it->second.getNickname() == nickname)
			return it->first;
	}
	return -1;
}

void set_nonblocking(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl(F_SETFL) failed");
	std::cout << "Set fd " << fd << " to non-blocking mode" << std::endl;
}

Server::Server(int port, const std::string &password){
	this->password = password;
	serv_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (serv_fd == -1)
		throw std::runtime_error("Socket creation failed");

	int opt = 1;
	if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (bind(serv_fd, (struct sockaddr *)(&server_addr), sizeof(server_addr)) == -1)
		throw std::runtime_error("Bind failed");

	if (listen(serv_fd, MAX_CLIENTS) == -1)
		throw std::runtime_error("Listen failed");

	set_nonblocking(serv_fd);

	struct pollfd serv_pfd;
	serv_pfd.fd = serv_fd;
	serv_pfd.events = POLLIN;
	serv_pfd.revents = 0;
	fds.push_back(serv_pfd);
}

void Server::add_new_client(){
	struct sockaddr_in clientadd;
	socklen_t client_len = sizeof(clientadd);
	int client_fd = accept(serv_fd, (struct sockaddr *)&clientadd, &client_len);
	if (client_fd == -1){
			std::cerr << "Accept client failed" << std::endl;
		return ;
	}
	if (fds.size() - 1 >= MAX_CLIENTS){
		std::cerr << "Max clients reached. Rejecting new client." << std::endl;
		close(client_fd);
		return ;
	}

	set_nonblocking(client_fd);

	struct pollfd client_pfd;
	client_pfd.fd = client_fd;
	client_pfd.events = POLLIN;
	client_pfd.revents = 0;
	fds.push_back(client_pfd);
	client[client_fd];
	client[client_fd].setFd(client_fd);

}

void Server::handle_client(int indx){
	char buf[BUF_SIZE + 1];
	std::string cmd;

	ssize_t n = recv(fds[indx].fd, buf, BUF_SIZE, 0);
	if (n <= 0){
		if (n == 0)
			std::cout << "Client disconnected, fd: " << fds[indx].fd << std::endl;
		else
			std::cerr << "Recv failed for fd: " << fds[indx].fd << std::endl;
		std::istringstream iss("Client disconnected CTRL+C");
		QUIT_cmd(iss, fds[indx].fd);
		return ;
	}
	buf[n] = '\0';
	cmd = client[fds[indx].fd].append(buf);
	while (cmd.find("\r\n") != std::string::npos){
		std::string p_cmd = cmd.substr(0, cmd.find("\r\n"));
		cmd = cmd.substr(cmd.find("\r\n") + 2);
		client[fds[indx].fd].append((char *)"");
		std::cout << "Received from fd " << fds[indx].fd 
			<< " : <<(" << p_cmd << ")>>" << std::endl;
		parse_cmd(p_cmd, fds[indx].fd);
	}


}

void Server::password_validation(std::istringstream &iss, int client_key){
	std::string pass;

	if (client[client_key].getPassed()){
		std::string msg = ":server 462 Password already set\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	if (iss.eof()){
		std::string msg = ":server 461 <unknown> PASS :Not enough parameters";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	iss >> pass;
	if (pass == this->password && iss.eof()){
		client[client_key].setPassed(true);
	} else {
		std::string msg = ":server 464 <unknown> :Password incorrect\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		client[client_key].pass_try++;
		if (client[client_key].pass_try >= 3){
			msg = "Closing connection\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			close(client_key);
			for (size_t j = 0; j < fds.size(); j++){
				if (fds[j].fd == client_key){
					fds.erase(fds.begin() + j);
					break ;
				}
			}
			client.erase(client_key);
		}
	}
}

bool Server::check_nick_in_use(std::string &nick, int client_key){
	for (std::map<int, Client>::iterator it = client.begin(); it != client.end(); it++){
		if (it->first != client_key && it->second.getNickname() == nick){
			std::string msg = ":server 433 <unknown> " + nick + " :Nickname is already in use\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			return true;
		}
	}
	return false;
}

bool check_valide_nick(std::string &nickname){
	if (nickname.empty() || nickname.length() > 9)
		return false;
	if (!std::isalpha(nickname[0]) && !std::ispunct(nickname[0]))
		return false;
	for (size_t i = 1; i < nickname.length(); i++){
		if (!std::isalnum(nickname[i]) && !std::ispunct(nickname[i]))
			return false;
	}
	if (nickname == "BOT")
		return false;
	return true;
}

void Server::NICK_cmd(std::istringstream &iss, int client_key){
	std::string nickname;
	if (client[client_key].getRegistered())
		return;
	if (!client[client_key].getPassed()){
		std::string msg = ":server 464 <unknown> :You need to set PASS first\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	if (iss.eof()){
		std::string msg = ":server 431 <unknown> :No nickname given\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	iss >> nickname;
	if (client[client_key].isNickSet()){
		std::string msg = ":server 433 <unknown> " + nickname + " :Nickname is already set. you can't change it.\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	} else if(check_valide_nick(nickname) == false){
		std::string msg = ":server 432 <unknown> " + nickname + " :Erroneous nickname\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	} else {
		if (check_nick_in_use(nickname, client_key))
			return ;
		client[client_key].setNickname(nickname);
		if (client[client_key].isUserSet()){
			client[client_key].setRegistered(true);
			std::string msg = ":server 001 " + client[client_key].getNickname() + " :Welcome to the server server\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
		}
	}
}

void Server::USER_cmd(std::istringstream &iss, int client_key){
	std::string username, realname, mode, unused;

	if (!client[client_key].getPassed()){
		std::string msg = ":server 464 <unknown> :You need to set PASS first\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	if (client[client_key].getRegistered()){
		std::string msg = ":server 462 <unknown> :You may not reregister\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	iss >> username;
	iss >> mode >> unused;
	if (iss.eof()){
		std::string msg = ":server 461 <unknown> USER :Not enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	std::getline(iss, realname);
	realname = trimstr(realname);
	if (realname.empty() || realname[0] != ':'){
		std::string msg = ":server 461 <unknown> USER :Not enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	realname = realname.substr(1);


	client[client_key].setUsername(username);
	client[client_key].setRealname(realname);
	if (client[client_key].isNickSet()){
		client[client_key].setRegistered(true);
		std::string msg = ":server 001 " + client[client_key].getNickname() + " :Welcome to the server server\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
	}
}

/*-------------- PRIVMSG -------------*/
void Server::PRIVMSG_cmd(std::istringstream &iss, int client_key){
	std::string target, message;

	if (!client[client_key].getRegistered()){
		std::string msg = ":server 451 <unknown> :You need to register first\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	iss >> target;
	if (iss.eof()){
		std::string msg = ":server 411 " + client[client_key].getNickname() + " :No recipient given (PRIVMSG)\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	std::getline(iss, message);
	message = trimstr(message);
	if (message.empty() || message[0] != ':'){
		std::string msg = ":server 412 " + client[client_key].getNickname() + " PRIVMSG :No text to send\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	message = message.substr(1);

	bool found = false;
	if (target[0] != '#' && target[0] != '&'){
		if (target == "BOT")
		{
			std::string msg = ":BOT PRIVMSG " + client[client_key].getNickname() + " :Je suis occupée… je te réponds dès que je peux\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			return ;
		}
		for (std::map<int, Client>::iterator it = client.begin(); it != client.end(); it++){
			if (it->second.getNickname() == target){
				std::string full_msg = ":" + client[client_key].getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
				send(it->first, full_msg.c_str(), full_msg.length(), 0);
				found = true;
				break ;
			}
		}
	} else {
		std::vector<Channel>::iterator it;
		for (it = channels.begin(); it != channels.end(); it++){
			if (it->getName() == target){
				if (!it->isMember(client[client_key])){
					std::string msg = ":server 404 " + client[client_key].getNickname() + " " + target + " :Cannot send to channel\r\n";
					send(client_key, msg.c_str(), msg.length(), 0);
					return ;
				}
				it->sendMessageToChannel(client[client_key], message);
				return ;
			}
		}
		if(it == channels.end()){
			std::string msg = ":server 403 " + client[client_key].getNickname() + " " + target + " :No such channel\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			return ;
		}
	}
	if (!found){
		std::string msg = ":server 401 " + client[client_key].getNickname() + " " + target + " :No such nick/channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
	}
}

/*-------------join start-------------*/
int Server::channels_size() const
{
	return (channels.size());
}
std::string  Server::channel_name(int index) const
{
	if (index < 0 || index >= channels_size())
		throw std::out_of_range("index invalid");
	return (channels[index].getName());
}

int Server::is_channel(std::string name, int client_key)
{
	std::string msg;
	int size = name.length();
	if (name.empty())
	{
		msg = ":server 403 <" + client[client_key].getNickname() + "> #(empty)" + name.substr(0, name.length() - 1) + " :No such channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return -2;
	}
	else if (name[size - 1] == ',')
	{
		msg = ":server 403 <" + client[client_key].getNickname() + "> #" + name.substr(0, name.length() - 1) + " :No such channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return -2;
	}
	for (int i = 0; i < channels_size(); i++)
	{
		if (name == channel_name(i))
			return i;
	}
	return -1;
}

void passwords(std::string & password, std::vector<std::string> & psswd)
{
	size_t flag;
	std::string pass;
	while (!password.empty())
	{
		flag = password.find(",");
		if (flag == std::string::npos)
		{
			pass = password;
			flag = 0;
		}
		else
			pass = password.substr(0, flag);
		for (size_t i = 0; i < pass.length(); i++)
		{
			if ((pass[i] >= 0 && pass[i] <= 31) || pass[i] == ',')
			{
				psswd.push_back("");
				break;
			}
		}
		psswd.push_back(pass);
		if (flag == 0)
			break;
		password = pass.substr(flag + 1);
	}
}

void canal(std::string & channel_name, std::vector<std::string> & channels)
{
	size_t flag;
	std::string name;
	while (!channel_name.empty())
	{
		flag = channel_name.find(",");
		if (flag == std::string::npos)
		{
			name = channel_name;
			flag = 0;
		}
		else
			name = channel_name.substr(0, flag);
		if (channel_name[0] != '#' && channel_name[0] != '&')
		{
			channels.push_back(name + ",");
			if (flag == 0)
				break ;
			else
			{
				channel_name = channel_name.substr(flag + 1);
				continue;
			}
		}
		for (size_t i = 0; i < name.length(); i++)
		{
			if ((name[i] >= 0 && name[i] <= 31) || name[i] == ',')
			{
				channels.push_back(name + ",");
				break;
			}
		}
		channel_name = channel_name.substr(flag + 1);
		channels.push_back(name);
		if (flag == 0)
			break;
	}
}

void Server::create_new_channel(std::string name, int client_key)
{
	Channel new_channel(name); 
	new_channel.addToChannel(client[client_key], client[client_key], "", "admin");
	channels.push_back(new_channel);
	std::cout << "Channel " << name << " created by " << client[client_key].getNickname() << std::endl;
}


void Server::JOIN_cmd(std::istringstream &iss, int client_key)
{
	int index;
	std::string channel_name, password;
	std::vector<std::string> psswd, channel;

	if (iss.eof()){
		std::string msg = ":server 461 " + client[client_key].getNickname() + " JOIN :No enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	iss >> channel_name;
	canal(channel_name, channel);
	if (!iss.eof())
	{
		iss >> password;
		passwords(password, psswd);
	}

	for (size_t i = 0; i < channel.size(); i++)
	{
		index = is_channel(channel[i], client_key);
		if (index >= 0)
		{
			if (i < psswd.size())
				channels[index].addToChannel(client[client_key], client[client_key], psswd[i], "client");
			else
				channels[index].addToChannel(client[client_key], client[client_key], "", "client");
		}
		else if (index == -1){
			create_new_channel(channel[i], client_key);
		}
	}
}

/*-------------join end-------------*/

/*-------------mode start-------------*/

bool check_password(std::string &password)
{
	if (password.empty())
		return false;
	for (size_t i = 0; i < password.length(); i++){
		if ((password[i] >= 0 && password[i] <= 31) || password[i] == ' ' || password[i] == ',')
			return false;
	}
	return true;
}

void Server::MODE_cmd(std::istringstream &iss, int client_key)
{
	bool flag;
	size_t max_client;
	int channel_index;
	std::string  channel, options, password, member, msg;

	if (iss.eof() || (iss >> channel && iss.eof()) || (iss >> options && iss.fail()))
	{
		msg = ":server 461 " +  client[client_key].getNickname() + " MODE :No enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	channel_index = is_channel(channel, client_key);
	if (channel_index < 0)
	{
		msg = ":server 403 " +  client[client_key].getNickname() + " " + channel + " :No such channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}

	if (options.empty() || options.length() != 2 || (options[0] != '+' && options[0] != '-'))
	{
		msg = ":server 472 " +  client[client_key].getNickname() + " " + options + " :unknown mode option\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	else
	{
		if (options[0] == '+')
			flag = true;
		else
			flag = false;
	}

	switch(options[1]){
		case 'i' :
			channels[channel_index].set_i(client[client_key], flag);
			break;
		case 't' :
			channels[channel_index].set_t(client[client_key], flag);
			break;
		case 'o' :
			if (!iss.eof() && iss >> member)
				channels[channel_index].set_o(flag, member, client[client_key]);
			else
			{
				msg = ":server 401 " + client[client_key].getNickname() + " " + options + " MODE :Not enough parameters(member)\r\n";
				send(client_key, msg.c_str(), msg.length(), 0);
			}
			break;
		case 'k' :
			if (!flag)
				channels[channel_index].set_k(client[client_key], flag, "");
			else if (!iss.eof() && iss >> password && check_password(password))
				channels[channel_index].set_k(client[client_key], flag, password);
			else
			{
				msg = ":server 461 " + client[client_key].getNickname() + " " + options + " MODE :Not enough parameters(password)\r\n";
				send(client_key, msg.c_str(), msg.length(), 0);
			}
			break;
		case 'l' :
			if (!flag)
				channels[channel_index].set_l(client[client_key], flag, 0);
			else if (!iss.eof() && iss >> max_client && !iss.fail() && max_client > 0)
				channels[channel_index].set_l(client[client_key], flag, max_client);
			else
			{
				msg = ":server 461 " + client[client_key].getNickname() + " " + options + " MODE :Not enough parameters(max_clients)\r\n";
				send(client_key, msg.c_str(), msg.length(), 0);
			}
			break;
		default :
			msg = ":server 472" + client[client_key].getNickname() + " " + options + " MODE :unknown mode option\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
	}
}
/*-------------mode end-------------*/


/*-------------kick start-------------*/
int Server::is_channel_for_(std::string name, int client_key)
{
	std::string msg;
	if (name.empty())
	{
		msg = ":server 403 " + client[client_key].getNickname() + " #(empty)" + name.substr(0, name.length() - 1) + " :No such a channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return -1;
	}
	for (int i = 0; i < channels_size(); i++)
	{
		if (name == channel_name(i))
			return i;
	}
	msg = ":server 403 " + client[client_key].getNickname() + name.substr(0, name.length() - 1) + " :No such a channel\r\n";
	send(client_key, msg.c_str(), msg.length(), 0);
	return -1;
}

void Server::KICK_cmd(std::istringstream &iss, int client_key)
{
	int index_of_channel, index_lmerhum;
	std::string channel, lmerhum, msg;

	if (iss.eof() || (iss >> channel && (iss.fail() || iss.eof())) || (iss >> lmerhum && iss.fail()))
	{
		msg = ":server 461 " + client[client_key].getNickname() + " KICK :No enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	index_of_channel = is_channel_for_(channel, client_key);
	if (index_of_channel < 0)
		return ;
	index_lmerhum = channels[index_of_channel].getIndexOfMember(lmerhum);
	if (index_lmerhum < 0)
	{
		if (lmerhum == "BOT")
		{
			std::cout << "Trying to kick the bot" << std::endl;
			msg = ":server 481 " + client[client_key].getNickname() + " " + lmerhum + " " + channel + " :Cannot kick the bot\r\n";
		}
		else
			msg = ":server 441 " + client[client_key].getNickname() + " " + lmerhum + " " + channel + " :is not on that channel\r\n";
		channels[index_of_channel].broadcast(msg);
		return ;
	}
	int fd = channels[index_of_channel].getFD(index_lmerhum);
	channels[index_of_channel].removeFromChannel(client[fd], client[client_key]);
}

/*-------------kick end-------------*/

/*-------------topic start-------------*/

void Server::TOPIC_cmd(std::istringstream &iss, int client_key)
{
	int index_of_channel;
	std::string channel, msg;

	if (iss.eof() || (iss >> channel && iss.fail()))
	{
		msg = ":server 461 " + client[client_key].getNickname() + " TOPIC :No enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	index_of_channel = is_channel_for_(channel, client_key);
	if (index_of_channel < 0)
		return ;
	if (iss.eof())
	{
		msg = channels[index_of_channel].show_topic();
		if (msg.empty())
			msg = ":server 331 " + client[client_key].getNickname() + " " + channel + " :No topic is set\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	if (std::getline(iss, msg))
	{
		if (msg == " :")
			channels[index_of_channel].setTopic("", client[client_key]);
		else
			channels[index_of_channel].setTopic(msg, client[client_key]);
	}
}
/*-------------topic end-------------*/

/*-------------invite start-------------*/

void Server::INVITE_cmd(std::istringstream &iss, int client_key)
{
	int index_of_channel, index_invited;
	bool flag = true;
	std::string channel, msg, invited;

	if (iss.eof() || (iss >> channel && iss.eof()) || (iss >> invited && iss.fail()))
	{
		msg = ":server 461 " + client[client_key].getNickname() + " INVITE :No enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}

	index_of_channel = is_channel_for_(channel, client_key);
	if (index_of_channel < 0)
		flag = false;
	if ((index_invited = getIndexOfClient(invited)) < 0)
	{
		//no such a client with this nickname
		msg = ":server 401 " + client[client_key].getNickname() + " " + invited + " :No such a client with this nickname in the server\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;	
	}
	if (flag) {
		if ((channels[index_of_channel].getIndexOfMember(invited)) >= 0)
		{
			msg = ":server 443 " + client[client_key].getNickname() + " " + invited + " " + channel + " :is already on channel\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			return ;
		}
		if (!channels[index_of_channel].isMember(client[client_key]))
		{
			msg = ":server 442 " + client[client_key].getNickname()+ " " + channel + " :You're not on that channel you can't invite others\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			return ;
		}
		if (channels[index_of_channel].invited_only() && !channels[index_of_channel].isOperator(client[client_key]))
		{
			msg = ":server 482 " + client[client_key].getNickname() + " " + channel + " :You're not a channel operator you can't invite others (+i)\r\n";
			send(client_key, msg.c_str(), msg.length(), 0);
			return ;
		}
		msg = ":" + client[client_key].getNickname() + " INVITE " + invited + " " + channel + "\r\n";
		send(index_invited, msg.c_str(), msg.length(), 0);
		channels[index_of_channel].add_to_invited_list(client[index_invited]);
	}
	else {
		msg = ":server 403 " + client[client_key].getNickname() + " " + channel + " :No such a channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
	}
}
/*-------------invite end-------------*/

/*-------------part start-------------*/
void Server::PART_cmd(std::istringstream &iss, int client_key)
{
	bool flag = false;
	std::string channel, escaper, msg;
	int index_of_channel;

	if (iss.eof() || (iss >> channel && iss.fail()))
	{
		msg = ":server 461 " + client[client_key].getNickname() + " PART :No enough parameters\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	index_of_channel = is_channel(channel, client_key);
	if (index_of_channel < 0)
	{
		msg = ":server 403 " + client[client_key].getNickname() + " " + channel + " :No such a channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	if (!iss.eof()) {
		std::getline(iss, msg);
		flag = true;
	}
	if (!channels[index_of_channel].isMember(client[client_key]))
	{
		msg = ":server 442 " + client[client_key].getNickname() + " " + channel + " :You're not on that channel\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	if (flag)
		msg = ":" + client[client_key].getNickname() + " PART " + channel + " :" + msg + "\r\n";
	else
		msg = ":" + client[client_key].getNickname() + " PART " + channel + "\r\n";
	channels[index_of_channel].broadcast(msg);
	channels[index_of_channel].part_From_Channel(client[client_key]);
}

/*-------------part end-------------*/

/*-------------quit start-------------*/
void Server::remove_client(int client_key)
{
	for (size_t i = 0; i < fds.size(); i++)
	{
		if (fds[i].fd == client_key){
			fds.erase(fds.begin() + i);
			break ;
		}
	}
	for (std::map<int, Client>::iterator it = client.begin(); it != client.end(); it++)
	{
		if (it->first == client_key){
			client.erase(it);
			break ;
		}
	}
}


void Server::QUIT_cmd(std::istringstream &iss, int client_key)
{
	int flag = false;
	std::string msg;

	if (!iss.eof() && std::getline(iss, msg))
		flag = true;
	for (size_t i = 0; i < channels.size(); i++)
	{
		if (channels[i].isMember(client[client_key]))
		{
			channels[i].part_From_Channel(client[client_key]);
			if (flag)
			{

				msg = ":" + client[client_key].getNickname() + " QUIT :" + msg + "\r\n";
				channels[i].broadcast(msg);
			}
			else
			{
				msg = ":" + client[client_key].getNickname() + " QUIT :Goodbye everyone!\r\n";
				channels[i].broadcast(msg);
			}
		}
	}
	remove_client(client_key);
	close(client_key);
}
/*-------------quit end-------------*/

void Server::parse_cmd(std::string cmd, int client_key){
	std::istringstream iss(cmd);
	std::string token;
	iss >> token;
	if (token == "PASS")
		password_validation(iss, client_key);
	else if (token == "NICK")
		NICK_cmd(iss, client_key);
	else if (token == "USER")
		USER_cmd(iss, client_key);
	else if (token == "CAP")
		;
	else if (token == "QUIT")
		QUIT_cmd(iss, client_key);
	else if (!client[client_key].getRegistered()){
		std::string msg = ":server 451 <unknown> :You may not reregister\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
		return ;
	}
	else if (token == "JOIN")
		JOIN_cmd(iss, client_key);
	else if (token == "PRIVMSG")
		PRIVMSG_cmd(iss, client_key);
	else if (token == "MODE")
		MODE_cmd(iss, client_key);
	else if (token == "KICK")
		KICK_cmd(iss, client_key);
	else if (token == "TOPIC")
		TOPIC_cmd(iss, client_key);
	else if (token == "INVITE")
		INVITE_cmd(iss, client_key);
	else if (token == "PART")
		PART_cmd(iss, client_key);
	else {
		std::string msg = ":server 421 " + token + " :Unknown command\r\n";
		send(client_key, msg.c_str(), msg.length(), 0);
	}
}

void handle_signal(int sig){
	(void)sig;
	std::cout << "Signal received, close fds and shutting down server..." << std::endl;
	g_signal = 1;
}

void Server::run(){
	signal(SIGQUIT, handle_signal);
	signal(SIGINT, handle_signal);
	while (1){
		if (g_signal)
			break;
		int n_fds = poll(fds.data(), fds.size(), 0);
		if (n_fds == -1)
			throw std::runtime_error("Poll failed");
		if (n_fds == 0)	
			continue ;
		for (size_t i = 0; i < fds.size(); i++){
			if (fds[i].revents & POLLIN){
				if (fds[i].fd == serv_fd)
					add_new_client();
				else
  					handle_client(i);
			}
		}
	}
	for (size_t i = 0; i < fds.size(); i++){
		close(fds[i].fd);
	}
}
