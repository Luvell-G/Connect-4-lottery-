// Connect 4.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ctime> 
#include <cstdlib>
using namespace std;

struct playerInfo
{
	char playerName[81];
	char playerID;
};
// prototype
int PlayerDrop(char board[][10], playerInfo activePlayer);
void CheckBellow(char board[][10], playerInfo activePlayer, int dropChoice);
void DisplayBoard(char board[][10]);
int CheckFour(char board[][10], playerInfo activePlayer);
int FullBoard(char board[][10]);
void PlayerWin(playerInfo activePlayer);
int restart(char board[][10]);

int main()
{
	playerInfo playerOne, playerTwo;
	char board[9][10];
	int trueWidth = 7;
	int trueLength = 6;
	int dropChoice, win, full, again;

	cout << "Let's Play Connect 4" << endl << endl;
	cout << "Player One please enter your name: ";
	cin >> playerOne.playerName;
	playerOne.playerID = 'X';
	cout << "Player Two please enter your name: ";
	cin >> playerTwo.playerName;
	playerTwo.playerID = 'O';

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
		win = CheckFour(board, playerOne);
		if (win == 1)
		{
			PlayerWin(playerOne);
			again = restart(board);
			if (again == 2)
			{
				break;
			}
		}
		// player two win check
		dropChoice = PlayerDrop(board, playerTwo);
		CheckBellow(board, playerTwo, dropChoice);
		DisplayBoard(board);
		win = CheckFour(board, playerTwo);
		if (win == 1)
		{
			PlayerWin(playerTwo);
			again = restart(board);
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
			again = restart(board);
		}

	} while (again != 2);



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
			srand(time(0));
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
				for (int i = 1; i <= 6; i++)
				{

					board[i][dropChoice] = '*';

				}
			}
			else
				// Roll fail
			{
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
			while (board[1][dropChoice] == 'X' || board[1][dropChoice] == 'O')
			{
				cout << "That row is full, please enter a new row: ";
				cin >> dropChoice;
			}
		}
		else
			// checking to make sure that the column is not full before advancing turn
			while (board[1][dropChoice] == 'X' || board[1][dropChoice] == 'O')
			{
				cout << "That row is full, please enter a new row: ";
				cin >> dropChoice;
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
			--length;
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
				board[i][ix] = '*';

			cout << board[i][ix];

		}

		cout << "|" << endl;
	}

}
// checking to see if there is any connect 4s
int CheckFour(char board[][10], playerInfo activePlayer)
{
	char XO;
	int win;

	XO = activePlayer.playerID;
	win = 0;

	for (int i = 8; i >= 1; --i)
	{

		for (int ix = 9; ix >= 1; --ix)
		{

			if (board[i][ix] == XO &&
				board[i - 1][ix - 1] == XO &&
				board[i - 2][ix - 2] == XO &&
				board[i - 3][ix - 3] == XO)
			{
				win = 1;
			}


			if (board[i][ix] == XO &&
				board[i][ix - 1] == XO &&
				board[i][ix - 2] == XO &&
				board[i][ix - 3] == XO)
			{
				win = 1;
			}

			if (board[i][ix] == XO &&
				board[i - 1][ix] == XO &&
				board[i - 2][ix] == XO &&
				board[i - 3][ix] == XO)
			{
				win = 1;
			}

			if (board[i][ix] == XO &&
				board[i - 1][ix + 1] == XO &&
				board[i - 2][ix + 2] == XO &&
				board[i - 3][ix + 3] == XO)
			{
				win = 1;
			}

			if (board[i][ix] == XO &&
				board[i][ix + 1] == XO &&
				board[i][ix + 2] == XO &&
				board[i][ix + 3] == XO)
			{
				win = 1;
			}
		}

	}

	return win;
}
// Seeing if board is full
int FullBoard(char board[][10])
{
	int full;
	full = 0;
	for (int i = 1; i <= 7; ++i)
	{
		if (board[1][i] != '*')
			++full;
	}

	return full;
}
// Saying what player won 
void PlayerWin(playerInfo activePlayer)
{
	cout << endl << activePlayer.playerName << " Connected Four, You Win!" << endl;
}
// Asking play to restart game
int restart(char board[][10])
{
	int restart;

	cout << "Would you like to restart? Yes(1) No(2): ";
	cin >> restart;
	if (restart == 1)
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
		cout << "Goodbye!" << endl;
	return restart;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
