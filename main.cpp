#include <iostream>
#include <mysql.h>
#include <mysqld_error.h>
#include <windows.h>
#include <sstream>
#include <iomanip> // For setw

using namespace std;

// Define color constants
#define FOREGROUND_BLACK 0x00

#define FOREGROUND_CYAN 0x03

#define FOREGROUND_MAGENTA 0x05
#define FOREGROUND_YELLOW 0x06
#define FOREGROUND_WHITE 0x07
#define BACKGROUND_BLACK 0x00
#define BACKGROUND_BLUE 0x10
#define BACKGROUND_GREEN 0x20
#define BACKGROUND_CYAN 0x30
#define BACKGROUND_RED 0x40
#define BACKGROUND_MAGENTA 0x50
#define BACKGROUND_YELLOW 0x60
#define BACKGROUND_WHITE 0x70

// Function to set console text color
void SetColor(int textColor, int bgColor) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (textColor | (bgColor << 4)));
}

const char* HOST = "localhost";
const char* USER = "root";
const char* PW = "#(love1sgod)"; // Replace with your MySQL password
const char* DB = "movie_ticket_booking"; // Use your actual database name

class Seats {
private:
    int Seat[5][10];
public:
    Seats() {
        resetSeats(); // Initialize seats during object creation
    }
    void resetSeats() {
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 10; j++) {
                Seat[i][j] = 1; // Initially, every seat is available
            }
        }
    }
    void setSeats(int row, int seatNumber, int status) {
        Seat[row - 1][seatNumber - 1] = status;
    }
    int getSeatStatus(int row, int seatNumber) {
        return Seat[row - 1][seatNumber - 1];
    }
    void reserveSeat(int row, int seatNumber) {
        Seat[row - 1][seatNumber - 1] = 0;
    }
    void display() {
        SetColor(FOREGROUND_YELLOW, BACKGROUND_BLACK); // Yellow text on black background
        cout << " ";

        for (int i = 0; i < 10; i++) {
            cout << " " << i + 1;
        }
        cout << endl;
        for (int row = 0; row < 5; row++) {
            cout << row + 1 << " ";
            for (int col = 0; col < 10; col++) {
                cout << (Seat[row][col] == 1 ? "- " : "X ");
            }
            cout << "|" << endl;
        }
        cout << "----------------------" << endl;
        SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK); // Reset to default
    }
    void getDB(MYSQL* conn, int movie_id) {
        resetSeats(); // Clear current seat status before loading new data
        stringstream query;
        query << "SELECT RowNumber, SeatNumber, Seat FROM Ticket WHERE movie_id = " << movie_id;
        if (mysql_query(conn, query.str().c_str())) {
            SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
            cout << "Error: " << mysql_error(conn) << endl;
            SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
            return;
        }
        MYSQL_RES* result = mysql_store_result(conn);
        if (!result) {
            SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
            cout << "Error: " << mysql_error(conn) << endl;
            SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
            return;
        }
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            setSeats(atoi(row[0]), atoi(row[1]), atoi(row[2]));
        }
        mysql_free_result(result);
    }
    void updateDB(MYSQL* conn, int row, int seatNumber, int movie_id) {
        stringstream ss;
        ss << "INSERT INTO Ticket (movie_id, RowNumber, SeatNumber, Seat) VALUES ("
           << movie_id << ", " << row << ", " << seatNumber << ", 0) "
           << "ON DUPLICATE KEY UPDATE Seat=0";
        if (mysql_query(conn, ss.str().c_str())) {
            SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
            cout << "Error: " << mysql_error(conn) << endl;
            SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
        }
    }
};

// Count available seats in the Seats class
int countAvailableSeats(Seats &s) {
    int count = 0;
    for (int row = 1; row <= 5; row++) {
        for (int col = 1; col <= 10; col++) {
            if (s.getSeatStatus(row, col) == 1) { // Seat is available
                count++;
            }
        }
    }
    return count;
}

// Display movie list and retrieve movie info from database
void viewMovies(MYSQL *conn) {
    MYSQL_ROW row;
    MYSQL_RES *res;
    string query = "SELECT * FROM movies";
    if (mysql_query(conn, query.c_str())) {
        SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
        cerr << "MySQL query error: " << mysql_error(conn) << endl;
        SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
        return;
    }
    res = mysql_store_result(conn);
    SetColor(FOREGROUND_CYAN, BACKGROUND_BLACK); // Cyan text on black background
    cout << setw(10) << left << "Movie ID"
         << setw(25) << left << "Movie Name"
         << setw(10) << left << "Rating"
         << setw(15) << left << "Total Seats"
         << setw(20) << left << "Available Seats"
         << endl;
    cout << string(80, '=') << endl;
    SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
    while ((row = mysql_fetch_row(res))) {
        cout << setw(10) << left << row[0]
             << setw(25) << left << row[1]
             << setw(10) << left << row[2]
             << setw(15) << left << row[3]
             << setw(20) << left << row[4]
             << endl;
    }
    mysql_free_result(res);
}

// Update the available seats in the movies table after booking
void updateAvailableSeats(MYSQL* conn, int movie_id, int bookedSeats) {
    stringstream ss;
    ss << "UPDATE movies SET available_seats = available_seats - " << bookedSeats
       << " WHERE movie_id = " << movie_id;
    if (mysql_query(conn, ss.str().c_str())) {
        SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
        cout << "Error updating available seats: " << mysql_error(conn) << endl;
        SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
    }
}
int main() {
    Seats s;
    MYSQL* conn;
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, HOST, USER, PW, DB, 3306, NULL, 0)) {
        SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
        cout << "Error: " << mysql_error(conn) << endl;
        SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
        return 1;
    }

    SetColor(FOREGROUND_GREEN, BACKGROUND_BLACK);
    cout << "Logged into database" << endl;
    SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);

    while (true) {
        system("cls");
        SetColor(FOREGROUND_YELLOW, BACKGROUND_BLACK);
        cout << "\nWelcome to the movie ticket reservation system\n";
        cout << "**********************************************\n";
        cout << "1. View Movies and Reserve Tickets\n";
        cout << "2. Exit\n";
        cout << "Enter your choice: ";
        SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);

        int choice;
        cin >> choice;

        if (choice == 1) {
            viewMovies(conn);
            int movie_id;
            
    do {
        cout << "Enter Movie ID to select a movie (1-8): ";
        cin >> movie_id;

        if (movie_id < 1 || movie_id > 8) {
            cout << "Invalid Movie ID. Only 8 screens are available. Please enter a valid ID.\n";
        }
    } while (movie_id < 1 || movie_id > 8);


            if (movie_id != -1) {
                s.getDB(conn, movie_id);
                s.display();

                // Count available seats
                int availableSeats = countAvailableSeats(s);

                int tickets;
                cout << "How many tickets would you like to book? ";
                cin >> tickets;

                // Check if enough seats are available
                if (tickets > availableSeats) {
                    SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
                    cout << "Sorry, only " << availableSeats << " seats are available." << endl;
                    cout << "Would you like to book " << availableSeats << " tickets instead? (y/n): ";
                    SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);

                    char choice;
                    cin >> choice;
                    if (choice == 'y' || choice == 'Y') {
                        tickets = availableSeats; // Proceed with available seats
                    } else {
                        continue; // Cancel booking if user chooses not to proceed
                    }
                }

                for (int i = 0; i < tickets; i++) {
                    cout << "Enter row number: ";
                    int row;
                    cin >> row;
                    cout << "Enter seat number: ";
                    int seat;
                    cin >> seat;

                    if (s.getSeatStatus(row, seat) == 0) {
                        SetColor(FOREGROUND_RED, BACKGROUND_BLACK);
                        cout << "Seat already booked! Choose another seat." << endl;
                        SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
                        i--; // Allow user to re-enter for this seat
                    } else {
                        s.reserveSeat(row, seat);
                        s.updateDB(conn, row, seat, movie_id);
                    }
                }

                // Update available seats in database after booking
                updateAvailableSeats(conn, movie_id, tickets);

                SetColor(FOREGROUND_GREEN, BACKGROUND_BLACK);
                cout << "Thank you for booking! Enjoy your movie!" << endl;
                SetColor(FOREGROUND_WHITE, BACKGROUND_BLACK);
                system("pause");
            }
        } else if (choice == 2) {
            cout << "Thank you for using our system!" << endl;
            break;
        }
    }
    mysql_close(conn);
    return 0;
}

