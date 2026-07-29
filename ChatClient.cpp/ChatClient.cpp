// ChatClient.cpp 
//Basic TCP chat client using Winsock
//This is the client app that connects to the chat server using an IP address.
//It asks for a temporary username, sends messages, and recieves messages.

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <WinSock2.h>
#include <WS2tcpip.h>
//This is where I'm storing all the inports for the other cpp files I intend to inlcude
#include "ExtensionHeaderFiles/UpdateChecker.h"

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

//TODO: Update this with every new Update and release!!!
//Version variable for the version of this application
const string CURRENT_VERSION = "v1.1.0";

//Port must match the server's port
//Need to update to allow for choosing port potentially
const int PORT = 54000;
//Max. message size the client can recieve
const int buffer_Size = 4096;
//Controls weather the client will keep running
//atomic<bool> is used because both main thread and recieve thread will use thus value
atomic<bool> running(true);

//Function will constantly listen for messages from the server.
//Runs on it's own thread so the client can recieve messages while the user types.
void recieveMessages(SOCKET clientSocket) {
    //Buffer will temp stores incoming messages from server
    char buffer[buffer_Size];
    //keep recieving message while the client is running
    while (running) {
        //clear the buffer before recieving a new message
        ZeroMemory(buffer, buffer_Size);

        //waits for a message from the server
        //recv() returns the number of the bytes recieved from server
        int bytesReceived = recv(clientSocket, buffer, buffer_Size, 0);
        //If bytesRecieved is 0 or less, client will disconnect
        //or if connection error occurs
        if (bytesReceived <= 0) {
            //Informs user about being disconnected
            cout << endl << "Disconnected from server." << endl;
            //Stops the client loop from running
            running = false;
            //Ends this recieve thread
            break;
        }

        //Converts the recieved data into a C++ string
        string message(buffer, bytesReceived);
        //Prints the server messge to the client's console/app
        cout << endl << message << endl;
        //Reprints the input marker so the user knows they can keep typing
        cout << "> ";

    }

}

//Main body function
int main()
{
    //calls the checkForUpdates function
    checkForUpdates(CURRENT_VERSION);
    cout << endl;
    //Stores the temp username for this chat session
    string username;
    //Stores the IP address of the server the user wants to connect to.
    string serverIP;

    //Asks the user for their temp name
    cout << "Enter temporary username: ";
    getline(cin, username);

    //If user leaves username blank, uses a default name.
    if (username.empty()) {
        username = "UknownUser";
    }

    //Asks user for the server IP address.
    cout << "Enter the server IP: ";
    getline(cin, serverIP);
    //If user leaves server IP blank, defaults to local host
    //127.0.0.1 means the same computer, hence local
    if (serverIP.empty()) {
        serverIP = "127.0.0.1";
    }

    //WSADATA stores the information about the Winsock Version being used.
    WSADATA wsaData;
    //Starts Winsock version 2.2.
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    //Checks if winsock failed to start
    if (result != 0) {
        cout << "WSAStartup failed. Error: " << result << endl;
        return 1;
    }

    //Creates the client socket.
    //AF_INET means the IPv4
    //SOCK_STREAM means TCP
    //IPPROTO_TCP means the socket uses the TCP protocol.
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //Checks if the socket failed to create.
    if (clientSocket == INVALID_SOCKET) {
        //Reports that clientSocket failed
        cout << "Failed to create socket. Error: " << WSAGetLastError() << endl;
        //Cleans up Winsock before ending the program.
        WSACleanup();
        return 1;
    }

    //Stores the Server's IP address and port info
    sockaddr_in serverAddress;
    //Uses IPv4
    serverAddress.sin_family = AF_INET;
    //Sets the server port.
    //This must match the server's PORT value.
    serverAddress.sin_port = htons(PORT);
    //Converts the server IP from text into a format Winsock can use
    result = inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr);

    //Checks if the IP address was invalid.
    if (result <= 0) {
        //Informs the client that they entered an invalid IP address." << endl;
        //Closes the client socket.
        closesocket(clientSocket);
        //Cleans up Winsock.
        WSACleanup();
        return 1;
    }

    //Prints connecting to server message
    cout << "Connecting to server..." << endl;

    //Attempts to connect to the server using the IP and port
    result = connect(
    clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));
    //Checks if the connection had failed
    if (result == SOCKET_ERROR) {
        //Prints failed message
        cout << "Could not connect to server. Error: " << WSAGetLastError() << endl;
        //Closes the client socket
        closesocket(clientSocket);
        //Cleans up Winsock
        WSACleanup();
        return 1;
    }

    //Greets the client to server
    cout << "Connect to the server." << endl;
    cout << "Type /quite to leave the chat." << endl;

    //Will send the username to the server first.
    //The server expects the first message to be the username.
    send(clientSocket, username.c_str(), static_cast<int>(username.size()), 0);
    //Starts a seperate thread to recieve message from the server
    thread receiveThread(recieveMessages, clientSocket);
    //Stores message typed by the user.
    string message;

    //Main message-sending loop
    while (running) {
        //Shows the input marker.
        cout << "> ";
        //Gets a full line of text from the user.
        getline(cin, message);
        //if the receive thread already stopped the client, exit loop
        if (!running) {
            break;
        }
        //Ignores blank messages
        if (message.empty()) {
            continue;
        }

        //Sends the user's message to the server
        send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0);
        //if the user types the /quit command, stops the client.
        if (message == "/quit"){
            running = false;
            break;
        }
    }

    //Ending of program, cleans up the program before ending
    //Closes the socket connection
    closesocket(clientSocket);
    //Cleans up Winsock before exiting
    WSACleanup();

    //Waits for the receive thread to finish if it is still running.
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
    //Prints the closing of the program
    cout << "Client closed." << endl;
    return 0;

}