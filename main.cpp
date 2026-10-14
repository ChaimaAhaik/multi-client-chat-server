#include "Server.hpp"

int check_args(char *port_str, char *password) {
	int port;
	std::stringstream ss(port_str);

	ss >> port;
	if (ss.fail() || !ss.eof())
		throw std::invalid_argument("Port must be a valid integer.");
	if (port <= 0 || port > 65535)
		throw std::out_of_range("Port must be in the range 1-65535.");
	if (std::strlen(password) == 0)
		throw std::invalid_argument("Password cannot be empty.");
	return port;
}

int main(int ac, char **av){
	if (ac != 3) {
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return 1;
	}
	try {
		Server server(check_args(av[1], av[2]), av[2]);
		server.run();
	}
	catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}