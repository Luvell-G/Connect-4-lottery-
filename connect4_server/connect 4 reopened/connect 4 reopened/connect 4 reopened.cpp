// Connect 4.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ctime>
#include <cstdlib>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using SocketHandle = SOCKET;
const SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using SocketHandle = int;
const SocketHandle INVALID_SOCKET_HANDLE = -1;
#endif

using namespace std;

struct playerInfo
{
    char playerName[81];
    char playerID;
};

class ServerToServerConnection
{
private:
    SocketHandle listenSocket;
    SocketHandle peerSocket;
    thread networkThread;
    mutex socketMutex;
    atomic<bool> running;
    bool socketSystemReady;

    bool initializeSocketSystem()
    {
        if (socketSystemReady)
        {
            return true;
        }

#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            cerr << "Unable to initialize Winsock." << endl;
            return false;
        }
#endif
        socketSystemReady = true;
        return true;
    }

    void closeSocket(SocketHandle& socketHandle)
    {
        if (socketHandle == INVALID_SOCKET_HANDLE)
        {
            return;
        }

#ifdef _WIN32
        closesocket(socketHandle);
#else
        close(socketHandle);
#endif
        socketHandle = INVALID_SOCKET_HANDLE;
    }

    void receiveLoop()
    {
        char buffer[1024];
        string pending;

        while (running)
        {
            SocketHandle currentPeer;
            {
                lock_guard<mutex> lock(socketMutex);
                currentPeer = peerSocket;
            }

            if (currentPeer == INVALID_SOCKET_HANDLE)
            {
                break;
            }

            int bytesReceived = recv(currentPeer, buffer, static_cast<int>(sizeof(buffer) - 1), 0);
            if (bytesReceived <= 0)
            {
                break;
            }

            buffer[bytesReceived] = '\0';
            pending.append(buffer, bytesReceived);

            size_t newlinePosition;
            while ((newlinePosition = pending.find('\n')) != string::npos)
            {
                string message = pending.substr(0, newlinePosition);
                pending.erase(0, newlinePosition + 1);

                if (message == "PING")
                {
                    sendMessage("PONG");
                }
                else
                {
                    cout << "\n[Peer Server] " << message << endl;
                }
            }
        }

        lock_guard<mutex> lock(socketMutex);
        closeSocket(peerSocket);

        if (running)
        {
            cout << "\nPeer server disconnected." << endl;
        }
    }

public:
    ServerToServerConnection()
        : listenSocket(INVALID_SOCKET_HANDLE),
          peerSocket(INVALID_SOCKET_HANDLE),
          running(false),
          socketSystemReady(false)
    {
    }

    ~ServerToServerConnection()
    {
        shutdownConnection();
    }

    bool listenForPeer(unsigned short port)
    {
        if (!initializeSocketSystem())
        {
            return false;
        }

        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET_HANDLE)
        {
            cerr << "Unable to create the peer listening socket." << endl;
            return false;
        }

        int reuseAddress = 1;
        setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&reuseAddress), sizeof(reuseAddress));

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);

        if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0)
        {
            cerr << "Unable to bind peer server to port " << port << "." << endl;
            closeSocket(listenSocket);
            return false;
        }

        if (listen(listenSocket, 1) < 0)
        {
            cerr << "Unable to listen for a peer server." << endl;
            closeSocket(listenSocket);
            return false;
        }

        running = true;
        cout << "Peer server listener started on port " << port << "." << endl;
        cout << "Waiting for another server to connect..." << endl;

        networkThread = thread([this]()
        {
            sockaddr_in peerAddress{};
#ifdef _WIN32
            int peerAddressLength = sizeof(peerAddress);
#else
            socklen_t peerAddressLength = sizeof(peerAddress);
#endif

            SocketHandle acceptedSocket = accept(
                listenSocket,
                reinterpret_cast<sockaddr*>(&peerAddress),
                &peerAddressLength);

            if (acceptedSocket == INVALID_SOCKET_HANDLE)
            {
                if (running)
                {
                    cerr << "Failed to accept peer server connection." << endl;
                }
                return;
            }

            {
                lock_guard<mutex> lock(socketMutex);
                peerSocket = acceptedSocket;
            }

            char addressBuffer[INET_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &peerAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
            cout << "\nPeer server connected from " << addressBuffer
                 << ":" << ntohs(peerAddress.sin_port) << "." << endl;

            receiveLoop();
        });

        return true;
    }

    bool connectToPeer(const string& host, unsigned short port)
    {
        if (!initializeSocketSystem())
        {
            return false;
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* results = nullptr;
        string portText = to_string(port);
        if (getaddrinfo(host.c_str(), portText.c_str(), &hints, &results) != 0)
        {
            cerr << "Unable to resolve peer server host: " << host << endl;
            return false;
        }

        SocketHandle connectedSocket = INVALID_SOCKET_HANDLE;
        for (addrinfo* current = results; current != nullptr; current = current->ai_next)
        {
            SocketHandle candidate = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
            if (candidate == INVALID_SOCKET_HANDLE)
            {
                continue;
            }

            if (connect(candidate, current->ai_addr, static_cast<int>(current->ai_addrlen)) == 0)
            {
                connectedSocket = candidate;
                break;
            }

            closeSocket(candidate);
        }

        freeaddrinfo(results);

        if (connectedSocket == INVALID_SOCKET_HANDLE)
        {
            cerr << "Unable to connect to peer server at " << host << ":" << port << "." << endl;
            return false;
        }

        {
            lock_guard<mutex> lock(socketMutex);
            peerSocket = connectedSocket;
        }

        running = true;
        cout << "Connected to peer server at " << host << ":" << port << "." << endl;

        networkThread = thread([this]()
        {
            receiveLoop();
        });

        sendMessage("SERVER_HELLO|Connect4 peer online");
        return true;
    }

    bool sendMessage(const string& message)
    {
        lock_guard<mutex> lock(socketMutex);

        if (peerSocket == INVALID_SOCKET_HANDLE)
        {
            return false;
        }

        string packet = message + "\n";
        size_t totalSent = 0;

        while (totalSent < packet.size())
        {
            int bytesSent = send(
                peerSocket,
                packet.c_str() + totalSent,
                static_cast<int>(packet.size() - totalSent),
                0);

            if (bytesSent <= 0)
            {
                return false;
            }

            totalSent += static_cast<size_t>(bytesSent);
        }

        return true;
    }

    void shutdownConnection()
    {
        running = false;

        {
            lock_guard<mutex> lock(socketMutex);
#ifdef _WIN32
            if (peerSocket != INVALID_SOCKET_HANDLE)
            {
                shutdown(peerSocket, SD_BOTH);
            }
            if (listenSocket != INVALID_SOCKET_HANDLE)
            {
                shutdown(listenSocket, SD_BOTH);
            }
#else
            if (peerSocket != INVALID_SOCKET_HANDLE)
            {
                shutdown(peerSocket, SHUT_RDWR);
            }
            if (listenSocket != INVALID_SOCKET_HANDLE)
            {
                shutdown(listenSocket, SHUT_RDWR);
            }
#endif
            closeSocket(peerSocket);
            closeSocket(listenSocket);
        }

        if (networkThread.joinable())
        {
            networkThread.join();
        }

#ifdef _WIN32
        if (socketSystemReady)
        {
            WSACleanup();
        }
#endif
        socketSystemReady = false;
    }
};

// prototypes
int PlayerDrop(char board[][10], playerInfo activePlayer);
void CheckBellow(char board[][10], playerInfo activePlayer, int dropChoice);
void DisplayBoard(char board[][10]);
int CheckFour(char board[][10], playerInfo activePlayer);
int FullBoard(char board[][10]);
void PlayerWin(playerInfo activePlayer);
int restart(char board[][10]);
string BoardState(char board[][10]);

int main()
{
    playerInfo playerOne, playerTwo;
    char board[9][10] = {};
    int dropChoice, win, full, again;
    ServerToServerConnection peerServer;

    cout << "Let's Play Connect 4" << endl << endl;
    cout << "Player One please enter your name: ";
    cin >> playerOne.playerName;
    playerOne.playerID = 'X';
    cout << "Player Two please enter your name: ";
    cin >> playerTwo.playerName;
    playerTwo.playerID = 'O';

    cout << endl;
    cout << "Server-to-server connectivity:" << endl;
    cout << "  1 - Listen for another Connect 4 server" << endl;
    cout << "  2 - Connect to an existing Connect 4 server" << endl;
    cout << "  3 - Play without server connectivity" << endl;
    cout << "Choose an option: ";

    int serverMode;
    cin >> serverMode;

    if (serverMode == 1)
    {
        int localPort;
        cout << "Enter the local TCP port to listen on (example 5555): ";
        cin >> localPort;

        if (localPort > 0 && localPort <= 65535)
        {
            peerServer.listenForPeer(static_cast<unsigned short>(localPort));
        }
        else
        {
            cout << "Invalid port. Continuing without server connectivity." << endl;
        }
    }
    else if (serverMode == 2)
    {
        string peerHost;
        int peerPort;

        cout << "Enter peer server hostname or IP address: ";
        cin >> peerHost;
        cout << "Enter peer server TCP port: ";
        cin >> peerPort;

        if (peerPort > 0 && peerPort <= 65535)
        {
            if (!peerServer.connectToPeer(peerHost, static_cast<unsigned short>(peerPort)))
            {
                cout << "Continuing without server connectivity." << endl;
            }
        }
        else
        {
            cout << "Invalid port. Continuing without server connectivity." << endl;
        }
    }

    full = 0;
    win = 0;
    again = 0;
    DisplayBoard(board);

    do
    {
        // player 1 win check
        dropChoice = PlayerDrop(board, playerOne);
        CheckBellow(board, playerOne, dropChoice);
        DisplayBoard(board);
        peerServer.sendMessage(
            "MOVE|" + string(playerOne.playerName) + "|X|column=" + to_string(dropChoice) +
            "|board=" + BoardState(board));

        win = CheckFour(board, playerOne);
        if (win == 1)
        {
            PlayerWin(playerOne);
            peerServer.sendMessage("WIN|" + string(playerOne.playerName) + "|X");
            again = restart(board);
            peerServer.sendMessage(again == 1 ? "RESTART" : "GAME_END");
            if (again == 2)
            {
                break;
            }
        }

        // player two win check
        dropChoice = PlayerDrop(board, playerTwo);
        CheckBellow(board, playerTwo, dropChoice);
        DisplayBoard(board);
        peerServer.sendMessage(
            "MOVE|" + string(playerTwo.playerName) + "|O|column=" + to_string(dropChoice) +
            "|board=" + BoardState(board));

        win = CheckFour(board, playerTwo);
        if (win == 1)
        {
            PlayerWin(playerTwo);
            peerServer.sendMessage("WIN|" + string(playerTwo.playerName) + "|O");
            again = restart(board);
            peerServer.sendMessage(again == 1 ? "RESTART" : "GAME_END");
            if (again == 2)
            {
                break;
            }
        }

        // Draw check
        full = FullBoard(board);
        if (full == 7)
        {
            cout << "The board is full, it is a draw!" << endl;
            peerServer.sendMessage("DRAW|board=" + BoardState(board));
            again = restart(board);
            peerServer.sendMessage(again == 1 ? "RESTART" : "GAME_END");
        }

    } while (again != 2);

    peerServer.shutdownConnection();
    return 0;
}

// players choose what column to drop token in
int PlayerDrop(char board[][10], playerInfo activePlayer)
{
    int dropChoice;
    do
    {
        // choose what player gets to go
        cout << activePlayer.playerName << "'s Turn ";
        cout << "Please enter a number between 1 and 7 or enter 777 for a chance to get a speacial token: ";
        cin >> dropChoice;

        // roll check for special token
        if (dropChoice == 777)
        {
            int xRan;
            srand(static_cast<unsigned int>(time(0)));
            // Dice roll
            xRan = rand() % 25 + 1;
            cout << "Rolling dice for a number 1-25: " << xRan << endl;
            if (xRan == 25)
            {
                // Roll Win
                cout << "You win A special token" << endl;
                cout << "Please enter a number between 1 and 7 " << endl;
                cin >> dropChoice;
                // Making sure that player can only roll once
                if (dropChoice == 777)
                {
                    do
                    {
                        dropChoice = 0;
                        cout << "You can only roll once per turn" << endl;
                        cout << "Please enter a number between 1 and 7 " << endl;
                        cin >> dropChoice;
                    } while (dropChoice == 777);
                }
                // clearing column for special token
                if (dropChoice >= 1 && dropChoice <= 7)
                {
                    for (int i = 1; i <= 6; i++)
                    {
                        board[i][dropChoice] = '*';
                    }
                }
            }
            else
            {
                // Roll fail
                cout << " You did not win a special token" << endl;
                cout << "Please enter a number between 1 and 7 " << endl;
                cin >> dropChoice;
                // Making sure that player can only roll once
                if (dropChoice == 777)
                {
                    do
                    {
                        dropChoice = 0;
                        cout << "You can only roll once per turn" << endl;
                        cout << "Please enter a number between 1 and 7 " << endl;
                        cin >> dropChoice;
                    } while (dropChoice == 777);
                }
            }

            // checking to make sure that the column is not full before
            if (dropChoice >= 1 && dropChoice <= 7)
            {
                while (board[1][dropChoice] == 'X' || board[1][dropChoice] == 'O')
                {
                    cout << "That row is full, please enter a new row: ";
                    cin >> dropChoice;
                }
            }
        }
        else if (dropChoice >= 1 && dropChoice <= 7)
        {
            // checking to make sure that the column is not full before advancing turn
            while (board[1][dropChoice] == 'X' || board[1][dropChoice] == 'O')
            {
                cout << "That row is full, please enter a new row: ";
                cin >> dropChoice;
            }
        }

    } while (dropChoice < 1 || dropChoice > 7);

    return dropChoice;
}

// Checking whose turn it is
void CheckBellow(char board[][10], playerInfo activePlayer, int dropChoice)
{
    int length, turn;
    length = 6;
    turn = 0;

    do
    {
        if (board[length][dropChoice] != 'X' && board[length][dropChoice] != 'O')
        {
            board[length][dropChoice] = activePlayer.playerID;
            turn = 1;
        }
        else
        {
            --length;
        }
    } while (turn != 1);
}

// Displaying the gameboard
void DisplayBoard(char board[][10])
{
    int rows = 6, columns = 7, i, ix;

    for (i = 1; i <= rows; i++)
    {
        cout << "|";
        for (ix = 1; ix <= columns; ix++)
        {
            if (board[i][ix] != 'X' && board[i][ix] != 'O')
            {
                board[i][ix] = '*';
            }

            cout << board[i][ix];
        }

        cout << "|" << endl;
    }
}

string BoardState(char board[][10])
{
    string state;
    state.reserve(42);

    for (int row = 1; row <= 6; ++row)
    {
        for (int column = 1; column <= 7; ++column)
        {
            char cell = board[row][column];
            state += (cell == 'X' || cell == 'O') ? cell : '*';
        }
    }

    return state;
}

// checking to see if there is any connect 4s
int CheckFour(char board[][10], playerInfo activePlayer)
{
    char XO = activePlayer.playerID;

    // horizontal
    for (int row = 1; row <= 6; ++row)
    {
        for (int column = 1; column <= 4; ++column)
        {
            if (board[row][column] == XO &&
                board[row][column + 1] == XO &&
                board[row][column + 2] == XO &&
                board[row][column + 3] == XO)
            {
                return 1;
            }
        }
    }

    // vertical
    for (int row = 1; row <= 3; ++row)
    {
        for (int column = 1; column <= 7; ++column)
        {
            if (board[row][column] == XO &&
                board[row + 1][column] == XO &&
                board[row + 2][column] == XO &&
                board[row + 3][column] == XO)
            {
                return 1;
            }
        }
    }

    // diagonal down-right
    for (int row = 1; row <= 3; ++row)
    {
        for (int column = 1; column <= 4; ++column)
        {
            if (board[row][column] == XO &&
                board[row + 1][column + 1] == XO &&
                board[row + 2][column + 2] == XO &&
                board[row + 3][column + 3] == XO)
            {
                return 1;
            }
        }
    }

    // diagonal up-right
    for (int row = 4; row <= 6; ++row)
    {
        for (int column = 1; column <= 4; ++column)
        {
            if (board[row][column] == XO &&
                board[row - 1][column + 1] == XO &&
                board[row - 2][column + 2] == XO &&
                board[row - 3][column + 3] == XO)
            {
                return 1;
            }
        }
    }

    return 0;
}

// Seeing if board is full
int FullBoard(char board[][10])
{
    int full = 0;
    for (int i = 1; i <= 7; ++i)
    {
        if (board[1][i] != '*')
        {
            ++full;
        }
    }

    return full;
}

// Saying what player won
void PlayerWin(playerInfo activePlayer)
{
    cout << endl << activePlayer.playerName << " Connected Four, You Win!" << endl;
}

// Asking player to restart game
int restart(char board[][10])
{
    int restartChoice;

    cout << "Would you like to restart? Yes(1) No(2): ";
    cin >> restartChoice;
    if (restartChoice == 1)
    {
        for (int i = 1; i <= 6; i++)
        {
            for (int ix = 1; ix <= 7; ix++)
            {
                board[i][ix] = '*';
            }
        }
    }
    else
    {
        cout << "Goodbye!" << endl;
    }
    return restartChoice;
}
