#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <vector>
#include <fstream>

using namespace std;

const int WIDTH = 20;
const int HEIGHT = 20;
enum Direction { STOP = 0, LEFT, RIGHT, UP, DOWN };
enum Difficulty { EASY = 1, NORMAL = 2, HARD = 3 };
Direction dir;
Difficulty difficulty = NORMAL;  // Default to normal difficulty

struct Node {
    int x, y;
    Node* next;
    Node(int x, int y) : x(x), y(y), next(nullptr) {}
};

Node* head = nullptr;
Node* tail = nullptr;
int foodX, foodY;
bool gameOver = false;
int score = 0;
int level = 1;
int highScore = 0;
vector<pair<int, int>> walls;

// === Terminal Handling ===
void enableRawMode() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disableRawMode() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

bool kbhit() {
    int oldf = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int ch = getchar();
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }
    return false;
}

// === High Score ===
void LoadHighScore() {
    ifstream in("highscore.txt");
    if (in.is_open()) {
        in >> highScore;
        in.close();
    }
}

void SaveHighScore() {
    ofstream out("highscore.txt");
    if (out.is_open()) {
        out << highScore;
        out.close();
    }
}

// === Setup ===
void placeFood();
bool isWall(int x, int y);
bool isOnSnake(int x, int y);

void Setup() {
    dir = RIGHT;
    gameOver = false;
    score = 0;
    level = 1;
    // difficulty is set by menu selection, don't reset it here

    head = new Node(WIDTH / 2, 5);  // Start at y=5 instead of y=10 to avoid the wall
    tail = head;

    srand(time(0));
    placeFood();

    walls.clear();  // No walls - only border wrapping
}

bool isWall(int x, int y) {
    for (auto& wall : walls)
        if (wall.first == x && wall.second == y)
            return true;
    return false;
}

bool isOnSnake(int x, int y) {
    Node* temp = head;
    while (temp) {
        if (temp->x == x && temp->y == y)
            return true;
        temp = temp->next;
    }
    return false;
}

void placeFood() {
    do {
        foodX = rand() % WIDTH;
        foodY = rand() % HEIGHT;
    } while (isOnSnake(foodX, foodY));  // Only check snake collision, no walls
}

// === Draw ===
void Draw() {
    system("clear");
    for (int i = 0; i < WIDTH + 2; ++i) cout << "#";
    cout << "\n";

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (x == 0) cout << "#";

            if (x == foodX && y == foodY)
                cout << "F";
            else {
                Node* temp = head;
                bool printed = false;
                while (temp) {
                    if (temp->x == x && temp->y == y) {
                        cout << "O";
                        printed = true;
                        break;
                    }
                    temp = temp->next;
                }
                if (!printed) cout << " ";
            }

            if (x == WIDTH - 1) cout << "#";
        }
        cout << "\n";
    }

    for (int i = 0; i < WIDTH + 2; ++i) cout << "#";
    cout << "\nScore: " << score << "  Level: " << level << "  High Score: " << highScore;
    
    // Display difficulty
    cout << "  Mode: ";
    switch(difficulty) {
        case EASY: cout << "Easy"; break;
        case NORMAL: cout << "Normal"; break;
        case HARD: cout << "Hard"; break;
    }
    cout << "\n";
}

// === Input ===
void Input() {
    if (kbhit()) {
        char ch = getchar();
        switch (ch) {
            // WASD keys (case insensitive)
            case 'a': case 'A': if (dir != RIGHT) dir = LEFT; break;
            case 'd': case 'D': if (dir != LEFT) dir = RIGHT; break;
            case 'w': case 'W': if (dir != DOWN) dir = UP; break;
            case 's': case 'S': if (dir != UP) dir = DOWN; break;
            case 'x': case 'X': gameOver = true; break;
            
            // Arrow keys (ESC sequence)
            case 27: // ESC key
                if (kbhit()) {
                    ch = getchar();
                    if (ch == '[') {
                        ch = getchar();
                        switch (ch) {
                            case 'A': if (dir != DOWN) dir = UP; break;    // Up arrow
                            case 'B': if (dir != UP) dir = DOWN; break;    // Down arrow
                            case 'C': if (dir != LEFT) dir = RIGHT; break; // Right arrow
                            case 'D': if (dir != RIGHT) dir = LEFT; break; // Left arrow
                        }
                    }
                }
                break;
        }
    }
}

// === Logic ===
void Logic() {
    if (dir == STOP) return;

    int newX = head->x;
    int newY = head->y;

    switch (dir) {
        case LEFT:  newX--; break;
        case RIGHT: newX++; break;
        case UP:    newY--; break;
        case DOWN:  newY++; break;
        default: break;
    }

    // Wall wrapping - wrap around to opposite side
    if (newX < 0) newX = WIDTH - 1;
    if (newX >= WIDTH) newX = 0;
    if (newY < 0) newY = HEIGHT - 1;
    if (newY >= HEIGHT) newY = 0;

    // Only game over if snake hits itself
    if (isOnSnake(newX, newY)) {
        gameOver = true;
        return;
    }

    Node* newHead = new Node(newX, newY);
    newHead->next = head;
    head = newHead;

    if (newX == foodX && newY == foodY) {
        score += 10;
        if (score % 50 == 0) level++;
        placeFood();
    } else {
        Node* prev = nullptr;
        Node* curr = head;
        while (curr->next) {
            prev = curr;
            curr = curr->next;
        }
        delete curr;
        prev->next = nullptr;
    }
}

// === Cleanup ===
void Cleanup() {
    Node* temp;
    while (head) {
        temp = head;
        head = head->next;
        delete temp;
    }
}

// === Menu ===
char ShowMenu() {
    system("clear");
    cout << "===== SNAKE GAME =====\n";
    cout << "1. Start Game (Current Mode: ";
    switch(difficulty) {
        case EASY: cout << "Easy"; break;
        case NORMAL: cout << "Normal"; break;
        case HARD: cout << "Hard"; break;
    }
    cout << ")\n";
    cout << "2. Change Difficulty\n";
    cout << "3. Exit\n";
    cout << "\nControls: WASD or Arrow Keys\n";
    cout << "Quit: X\n";
    cout << "Select (1, 2, or 3): ";
    char choice;
    cin >> choice;
    return choice;
}

char SelectDifficulty() {
    system("clear");
    cout << "===== SELECT DIFFICULTY =====\n";
    cout << "1. Easy (Very Slow - 800ms)\n";
    cout << "2. Normal (Slow - 500ms)\n";
    cout << "3. Hard (Medium - 300ms)\n";
    cout << "Select (1, 2, or 3): ";
    char choice;
    cin >> choice;
    return choice;
}

char GameOverScreen() {
    if (score > highScore) {
        highScore = score;
        SaveHighScore();
    }

    cout << "\nGAME OVER!\n";
    cout << "Your Score: " << score << "\n";
    cout << "High Score: " << highScore << "\n";
    cout << "1. Play Again\n";
    cout << "2. Exit\n";
    cout << "Select (1 or 2): ";
    char choice;
    cin >> choice;
    return choice;
}

// === Main ===
int main() {
    LoadHighScore();
    enableRawMode();

    while (true) {
        char menu = ShowMenu();
        if (menu == '1') {
            // Start game with current difficulty
        } else if (menu == '2') {
            disableRawMode();  // switch to normal for cin
            char diffChoice = SelectDifficulty();
            if (diffChoice == '1') difficulty = EASY;
            else if (diffChoice == '2') difficulty = NORMAL;
            else if (diffChoice == '3') difficulty = HARD;
            enableRawMode();  // back to raw mode
            continue;  // go back to main menu
        } else if (menu == '3') {
            break;  // exit
        } else {
            continue;  // invalid choice, show menu again
        }

        Setup();

        while (!gameOver) {
            Draw();
            
            // Check input multiple times per frame for better responsiveness
            for (int i = 0; i < 10; i++) {
                Input();
                usleep(10000); // 10ms delay between input checks
            }
            
            Logic();
            
            // Calculate speed based on difficulty and level
            int baseSpeed;
            switch(difficulty) {
                case EASY: baseSpeed = 800000; break;    // 800ms - very slow
                case NORMAL: baseSpeed = 500000; break;  // 500ms - slow
                case HARD: baseSpeed = 300000; break;    // 300ms - medium
            }
            int speedIncrease = (level - 1) * 10000;  // smaller speed increase with levels
            usleep(baseSpeed - speedIncrease - 100000); // Subtract the time spent on input checks
        }

        disableRawMode();  // switch to normal for cin
        char post = GameOverScreen();
        if (post != '1') break;
        Cleanup();
        enableRawMode();  // back to raw for next game
    }

    Cleanup();
    disableRawMode();
    cout << "\nThanks for playing!\n";
    return 0;
}
