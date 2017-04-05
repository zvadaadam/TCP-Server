//
//  main.cpp
//  TCP Server
//
//  Created by Adam Zvada on 09.03.17.
//  Copyright © 2017 Adam Zvada. All rights reserved.
//

#include <iostream>
#include <string>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <vector>

#define ECHOPORT            3999
#define	BUFFSIZE            1000
#define BUFFER_SIZE         128

#define SYNTAX_ERROR        -1
#define FINISH              0


#define SERVER_USER         "100 LOGIN\r\n"         //Výzva k zadání uživatelského jména
#define SERVER_PASSWORD     "101 PASSWORD\r\n"      //Výzva k zadání uživatelského hesla
#define SERVER_MOVE         "102 MOVE\r\n"          //Příkaz pro pohyb o jedno pole vpřed
#define SERVER_TURN_LEFT	"103 TURN LEFT\r\n"     //Příkaz pro otočení doleva
#define SERVER_TURN_RIGHT	"104 TURN RIGHT\r\n"    //Příkaz pro otočení doprava
#define SERVER_PICK_UP      "105 GET MESSAGE\r\n"	//Příkaz pro vyzvednutí zprávy
#define SERVER_OK           "200 OK\r\n"            //Kladné potvrzení
#define SERVER_LOGIN_FAILED	"300 LOGIN FAILED\r\n"	//Chybné heslo
#define SERVER_SYNTAX_ERROR	"301 SYNTAX ERROR\r\n"	//Chybná syntaxe zprávy
#define SERVER_LOGIC_ERROR	"302 LOGIC ERROR\r\n"	//Zpráva odeslaná ve špatné situaci

#define CLIENT_USER         "<user name>\r\n"       //Zpráva s uživatelským jménem. Jméno může být libovolná sekvence znaků kromě kromě dvojice \r\n.	Umpa_Lumpa\r\n	100
#define USER_SIZE           100

#define CLIENT_PASSWORD     "<password>\r\n"        //Zpráva s uživatelským heslem. Ověření hesla je popsáno v sekci autentizace.
#define PASSWORD_SIZE       7                       //Zpráva může obsahovat maximálně 5 čísel a ukončovací sekvenci \r\n.	1009\r\n	7

#define CLIENT_CONFIRM      "OK <x> <y>\r\n"        //Potvrzení o provedení pohybu, kde x a y jsou souřadnice robota po provedení pohybového příkazu.	OK -3 -1\r\n	12
#define CONFIRM_SIZE        12

#define CLIENT_RECHARGING	"RECHARGING\r\n"        //Robot se začal dobíjet a přestal reagovat na zprávy.		12
#define RECHARGING_SIZE

#define CLIENT_FULL_POWER	"FULL POWER\r\n"        //Robot doplnil energii a opět příjímá příkazy.		12

#define CLIENT_MESSAGE      "<text>\r\n"            //Text vyzvednutého tajného vzkazu. Může obsahovat jakékoliv znaky kromě ukončovací sekvence \r\n.	Haf!\r\n	100
#define MESSAGE_SIZE        100

#define TIMEOUT             1                       //Server i klient očekávají od protistrany odpověď po dobu tohoto intervalu.
#define TIMEOUT_RECHARGING  5                       //Časový interval, během kterého musí robot dokončit dobíjení.

#define POSTFIX             "\r\n"

#define TIMEOUT_EXCEPTION   10
#define LOGIC_EXCEPTION     11



struct Position {
    int x;
    int y;
};

//---------------------CLIENT---------------------------

class Client {
public:
    bool initParam();
    bool authUser(int acceptedSocketfd);
    bool navigateRobotToZero();
private:
    int _acceptedSocketfd;
    
    fd_set              fdset;
    
    timeval             tv;
    timeval             tvRecharging;

    std::string _msgBuffer;
    
    Position _robot;
    
    bool _moveRobot();
    int _zeroDirection();
    
    bool _turnRight();
    
    bool _sendMessage(const char * msg, const int sizeMsg) const;
    std::string _readMessage(const int sizeMsg);
    
    bool _fillMsgBuffer(const int sizeMsg, const int timeout);
    
    bool _handleRecharing(std::string & outputMsg, const int sizePrevMsg);
    
    bool _validateMsg(std::string msg) const;
    bool _validatePasswdMsg(std::string passwd) const;
    
    std::string _getPasswdUser(std::string usr) const;
    
    bool _updateRobotsPos(std::string pos);
    bool _pickSecretMsg();
    
    bool _containsPostfix(std::string msg) const;    // postfix - \r\n
    
    bool _isRobotInFinish() const;
    
    void _printMsg(const char * msg, const int sizeMsg) const;
};


/*
 *  Set timeout for select func etc.
 */
bool Client::initParam() {
    
    FD_ZERO(&fdset);
    FD_SET(_acceptedSocketfd, &fdset);
    
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    tvRecharging.tv_sec = TIMEOUT_RECHARGING;
    tvRecharging.tv_usec = 0;
    
    _msgBuffer = "";

    return true;
}


/*
 *  Handle authorization of user on given socket
 */
bool Client::authUser(int acceptedSocketfd) {
    std::cout << "User authentication starting..." << std::endl;
    
    this->_acceptedSocketfd = acceptedSocketfd;
    initParam();
    
    _sendMessage(SERVER_USER, sizeof(SERVER_USER));
    
    std::string user = _readMessage(USER_SIZE);
    if(!_validateMsg(user)) {
        std::cout << "---Authantication failed---" << std::endl;
        return false;
    }
    
    _sendMessage(SERVER_PASSWORD, sizeof(SERVER_PASSWORD));
    
    std::string password = _readMessage(PASSWORD_SIZE);
    if(!_validateMsg(password)) {
        std::cout << "---Authantication failed---" << std::endl;
        return false;
    }
    
    if(!_validatePasswdMsg(password)) {
        std::cout << "---Authantication failed---" << std::endl;
        return false;
    }
    
    
    if (password != _getPasswdUser(user)) {
        
        std::cout << "ERROR : Password of user " << user.substr(0, user.length()-3) << " doesn't match" << std::endl;
        _sendMessage(SERVER_LOGIN_FAILED, sizeof(SERVER_LOGIN_FAILED));
        std::cout << "---Authantication failed---" << std::endl;
        return false;
    }
    
    std::cout << "---Authantication Succesful---" << std::endl;
    _sendMessage(SERVER_OK, sizeof(SERVER_OK));
    
    return true;
}


/*
 *  Send instractions to robot so it gets to [0,0]
 */
bool Client::navigateRobotToZero() {

    //move robot in zero direction
    
    int rVal = _zeroDirection();
    if (rVal == FINISH) {
        if(!_pickSecretMsg()) return false;
        return true;
    } else if (rVal == SYNTAX_ERROR) {
        return false;
    }
    
    _turnRight();
    
    rVal = _zeroDirection();
    if (rVal == SYNTAX_ERROR) return false;
    
    if(!_pickSecretMsg()) {
        return false;
    }
    
    return true;
}

/*
 *  We don't know robot's direction, so turn and get it to [x,0] or [y,0]
 */
int Client::_zeroDirection() {
    
    if(!_moveRobot()) return SYNTAX_ERROR;
    
    if (_isRobotInFinish()) return FINISH;
    
    
    Position startPos;
    startPos.x = _robot.x;
    startPos.y = _robot.y;
    
    while (startPos.x == _robot.x && startPos.y == _robot.y) {
        if(!_moveRobot()) return SYNTAX_ERROR;
    }
    if (_isRobotInFinish()) return FINISH;

    if (abs(_robot.x) > abs(startPos.x) || abs(_robot.y) > abs(startPos.y)) {
        if(!_turnRight()) return SYNTAX_ERROR;
        if(!_turnRight()) return SYNTAX_ERROR;
    }
    
    if (_robot.x - startPos.x) {
        while (_robot.x != 0) {
            if(!_moveRobot()) return SYNTAX_ERROR;
        }
    } else {
        while (_robot.y != 0) {
            if(!_moveRobot()) return SYNTAX_ERROR;
        }
    }
    
    if (_isRobotInFinish()) return FINISH;
    
    return true;
}


bool Client::_moveRobot() {
    
    _sendMessage(SERVER_MOVE, sizeof(SERVER_MOVE));
    
    std::string pos = _readMessage(CONFIRM_SIZE);
    if(!_validateMsg(pos))
        return false;

    if(!_updateRobotsPos(pos))
        return false;
    
    return true;
}


bool Client::_turnRight() {
    _sendMessage(SERVER_TURN_RIGHT, sizeof(SERVER_TURN_RIGHT));
    
    std::string pos = _readMessage(CONFIRM_SIZE);
    if(!_validateMsg(pos))
        return false;
    
    if(!_updateRobotsPos(pos))
        return false;
    
    return true;
}

/*
 *  Send message to client
 */
bool Client::_sendMessage(const char * msg, const int sizeMsg) const {
    std::cout << "Sending msg: "; _printMsg(msg, sizeMsg-3); std::cout << std::endl; //sizeMsg-3 cuz trimming postfix
    
    if(send(_acceptedSocketfd, msg, sizeMsg-1, 0) == -1) {
        std::cout << "ERROR : FAILED to send message, " << *msg << std::endl;
        return false;
    }
    
    return true;
}

/*
 *  Read message from client.
 */
std::string Client::_readMessage(const int sizeMsg) {
    
    std::string outputMsg = "";
    
    if (!_containsPostfix(_msgBuffer)) {
        if(!_fillMsgBuffer(sizeMsg, TIMEOUT))
            return "";
    }
    
    outputMsg.append(_msgBuffer.substr(0, _msgBuffer.find(POSTFIX)+2));
    _msgBuffer.erase(0, _msgBuffer.find(POSTFIX)+2);

    if (outputMsg != CLIENT_RECHARGING && outputMsg.size() > sizeMsg) {
        std::cout << "ERROR : Output message size exceeded."  << std::endl;
        return "";
    }
    _handleRecharing(outputMsg, sizeMsg);
    
    return outputMsg;
}


/*
 *  Set timeout for select func etc.
 */
bool Client::_fillMsgBuffer(const int sizeMsg, const int timeout) {
    
    char buffer[BUFFER_SIZE];
    
    int MAX_SIZE = MESSAGE_SIZE;
    
    int curSize = 0;
    while (!_containsPostfix(_msgBuffer)) {
        
        int n;
        if (timeout == TIMEOUT) {
            n = select(_acceptedSocketfd + 1, (struct fd_set *)&fdset, nullptr, nullptr, (struct timeval *)&tv);
        } else {
            n = select(_acceptedSocketfd + 1, (struct fd_set *)&fdset, nullptr, nullptr, (struct timeval *)&tvRecharging);
        }
        
        if(n == 0) {
            std::cout << "ERROR : Timeout finished." << std::endl;
            throw TIMEOUT_EXCEPTION;
        } else if (n == -1) {
            std::cout << "ERROR : Select" << std::endl;
            return false;
        } else {
            int len = (int)recv(_acceptedSocketfd, buffer, MAX_SIZE, 0);
            if (len > 0) {
                for (int i = 0; i < len; i++) {
                    _msgBuffer += buffer[i];
                }
            }
        }
        
        if (_containsPostfix(_msgBuffer)) {
            curSize = (int)_msgBuffer.substr(0, _msgBuffer.find(POSTFIX)).size();
        } else {
            curSize = (int)_msgBuffer.size();
        }
        
        
        if (_containsPostfix(_msgBuffer) && _msgBuffer.substr(0, _msgBuffer.find(POSTFIX)+2) == CLIENT_RECHARGING) {
            return true;
        }
        
        if (curSize > sizeMsg) {
            std::cout << "ERROR: Message size exceeded. " << std::endl;
            return false;
        }
        
        if ( (!_containsPostfix(_msgBuffer) && _msgBuffer.back() == '\r' && curSize + 1 > sizeMsg )
                || (!_containsPostfix(_msgBuffer) && _msgBuffer.back() != '\r' && curSize + 2 > sizeMsg)) {
            std::cout << "ERROR: Message size will be exceeded! " <<  curSize << std::endl;
            return false;
        }
    }
    
    
    
    return true;
}

/*
 *  When recharing is on
 */
bool Client::_handleRecharing(std::string & outputMsg, const int sizePrevMsg) {
    
    if (outputMsg == CLIENT_RECHARGING) {
        std::cout << "ROBOT needs to be RECHARGED..." << std::endl;
        _fillMsgBuffer(sizeof(CLIENT_FULL_POWER)-1, TIMEOUT_RECHARGING);
        
        if (_msgBuffer.substr(0, _msgBuffer.find(POSTFIX)+2) == CLIENT_FULL_POWER) {
            std::cout << "ROBOT fully chared!" << std::endl;
            _msgBuffer.erase(0, _msgBuffer.find(POSTFIX)+2);
            outputMsg = _readMessage(sizePrevMsg);
        } else {
            _sendMessage(SERVER_LOGIC_ERROR, sizeof(SERVER_LOGIC_ERROR));
            throw LOGIC_EXCEPTION;
        }
    }

    return true;
}


bool Client::_validateMsg(std::string msg) const {
    
    
    std::cout << "Recived Message: " << msg.substr(0, msg.length()-2) << std::endl;
    
    if (!_containsPostfix(msg)) {
        std::cout << "ERROR :  Message doesn't contain postfix" << std::endl;
        _sendMessage(SERVER_SYNTAX_ERROR, sizeof(SERVER_SYNTAX_ERROR));
        return false;
    }

    msg = msg.substr(0, msg.length()-2);
    
    return true;
}


bool Client::_validatePasswdMsg(std::string passwd) const {
    
    std::cout << "Recived Message: " << passwd.substr(0, passwd.length()-2) << std::endl;
    
    std::string trimPasswd = passwd.substr(0, passwd.length()-2);
    
    if(!std::all_of(passwd.begin(), passwd.end() - 2, ::isdigit)) {
        std::cout << "ERROR : Password format is not valid."<< std::endl;
        _sendMessage(SERVER_SYNTAX_ERROR, sizeof(SERVER_SYNTAX_ERROR));
        return false;
    }

    return true;
}


std::string Client::_getPasswdUser(std::string usr) const {
    
    long passwd = 0;
    for (int i = 0; i < usr.size()-2; i++) {
        passwd += usr.c_str()[i];
    }
    
    return std::to_string(passwd).append(POSTFIX);
}


bool Client::_updateRobotsPos(std::string pos) {
    
    if (pos.substr(0, 3) != "OK ") {
        std::cout << "ERROR : Confirm message not valid." << std::endl;
        _sendMessage(SERVER_SYNTAX_ERROR, sizeof(SERVER_SYNTAX_ERROR));
        return false;
    }
    
    pos.erase(0, 3);
    
    int i = 0;
    std::string xPos = "";
    int xSign = 1;
    if (pos[i] == '-') {
        xSign = -1;
        i++;
    }
    while (pos[i] != ' ') {
        xPos += pos[i];
        i++;
    }
    
    std::string yPos = "";
    i++;
    int ySign = 1;
    if (pos[i] == '-') {
        ySign = -1;
        i++;
    }
    
    while (pos[i] != '\r') {
        yPos += pos[i];
        i++;
    }

    if(!std::all_of(xPos.begin(), xPos.end(), ::isdigit) || !std::all_of(yPos.begin(), yPos.end(), ::isdigit)) {
        std::cout << "ERROR : Robot positon format is not valid." << std::endl;
        _sendMessage(SERVER_SYNTAX_ERROR, sizeof(SERVER_SYNTAX_ERROR));
        return false;
    }
    
    int x = std::stoi(xPos);
    int y = std::stoi(yPos);
    
    _robot.x = x * xSign;
    _robot.y = y * ySign;
    
    return true;
}


bool Client::_pickSecretMsg() {
    
    _sendMessage(SERVER_PICK_UP, sizeof(SERVER_PICK_UP));
    
    std::string secretMsg = _readMessage(MESSAGE_SIZE);
    if(!_validateMsg(secretMsg)) {
        return false;
    }
    
    _sendMessage(SERVER_OK, sizeof(SERVER_OK));
    
    return true;
}


bool Client::_containsPostfix(std::string msg) const {
    
    if (msg.find(POSTFIX) != std::string::npos) {
        return true;
    }
    
    return false;
}


bool Client::_isRobotInFinish() const {
    
    if (_robot.x == 0 && _robot.y == 0) {
        std::cout << "Robot is in finish and it's ready to pick up secret message!" << std::endl;
        return true;
    }

    return false;
}


void Client::_printMsg(const char * msg, const int sizeMsg) const {
    for (int i = 0; i < sizeMsg; i++) {
        std::cout << msg[i];
    }
}

//---------------------SERVER---------------------------

class Server {
public:
    int port;
    
    bool makeConnection(int port);
    bool acceptConnection();
    bool authClient();
    bool handleRobot();
    
    int getAcceptedSocketfd() const;
    int getSocketfd() const;
private:
    Client _client;
    
    int _sockfd;                                //id socket file descriptor
    int _acceptedSockfd;                        //aceppted socekt file descriptor
    
    sockaddr_in _myAddr;
    
    sockaddr_in _acceptedAddr;
    socklen_t _acceptedAddrLen;
};



bool Server::makeConnection(int port) {
    
    //creation of socket, koncovy bod dvousmerne komunikace
    _sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_sockfd == -1) {
        std::cout << "Error creating socket!" << std::endl;
    } else {
        std::cout << "Socked successfully created..." << std::endl;
    }
    
    bzero(&_myAddr, sizeof(_myAddr));
    _myAddr.sin_family = AF_INET;
    _myAddr.sin_port = htons(port);
    
    //bind defines the local port and interface address(ip) for the connection
    if (bind(_sockfd, (struct sockaddr *)&_myAddr, sizeof(_myAddr)) == -1) {
        perror("bind error");
        close(_sockfd);
        exit(1);
    } else {
        std::cout << "Bind procceded successfully..." << std::endl;
    }
    
    //listen for connections on a socket
    if (listen(_sockfd, 5) == -1) {
        perror("listen error");
        close(_sockfd);
        exit(1);
    } else {
        std::cout << "Listen procceded successfully..." << std::endl;
    }

    return true;
}

bool Server::acceptConnection() {
    
    _acceptedAddrLen = sizeof(_acceptedAddr);
    
    _acceptedSockfd = accept(_sockfd, (struct sockaddr *)&_acceptedAddr, &_acceptedAddrLen);
    
    if (_acceptedSockfd == -1) {
        std::cout << "ERROR : acception of socked " << _sockfd << std::endl;
        perror("accept error");
        close(_sockfd);
        exit(1);
    } else {
        std::cout << "Succesfully accepted connection of socked..." << _sockfd <<std::endl;
    }
    
    return true;
    
}

bool Server::authClient() {
    
    try {
        if(!_client.authUser(_acceptedSockfd)) {
            return false;
        }
    } catch(int e) {
        return false;
    }
    
    return true;
}

bool Server::handleRobot() {
    try {
        if(!_client.navigateRobotToZero()) {
            return false;
        }
    } catch(int e) {
        return false;
    }

    return true;
}

int Server::getAcceptedSocketfd() const {
    return _acceptedSockfd;
}

int Server::getSocketfd() const {
    return _sockfd;
}

//---------------------MAIN---------------------------

int main(int argc, char *argv[]) {
    
    int port;
    
    std::cout << "Give me server port: ";
    std::cin >> port;
    
    Server server;
    server.makeConnection(port);
    
    
    bool isRunning = true;
    while (isRunning) {
        server.acceptConnection();
        
        switch (fork()) {
            case 0:
                //parent
                if(!server.authClient()) {
                    perror("Error on auth");
                    close(server.getAcceptedSocketfd());
                    continue;
                }
                
                if(!server.handleRobot()) {
                    perror("Error on client");
                    close(server.getAcceptedSocketfd());
                    continue;
                }

                isRunning = false;
                
                close(server.getSocketfd());
                break;
            case -1:
                //fork error
                std::cout << "ERROR: Cannot be run in parallel!" << std::endl;
                //do single process
                if(!server.authClient()) {
                    perror("Error on auth");
                    close(server.getAcceptedSocketfd());
                    continue;
                }
                
                if(!server.handleRobot()) {
                    perror("Error on client");
                    close(server.getAcceptedSocketfd());
                    continue;
                }

                
                break;
            default:
                //child
                close(server.getAcceptedSocketfd());
                break;
        }
        
        
        
        //close(server.getAcceptedSocketfd());
    }
    
    
    return 0;
    
}

    
