// uses shunting yard algorithm
// https://en.wikipedia.org/wiki/Shunting-yard_algorithm

#include <iostream>
#include <string>
#include <queue>
#include <cctype>

using namespace std;

bool parseEquation(string equation, int &result) {
    
}

int main() {
    cout << "Enter equation to parse:\n";
    string equation;
    getline(cin, equation);
    cout << "The equation entered is: '" << equation << "'\n";
    int answer = 0;
    bool success = parseEquation(equation, answer);
    if (success) {
        cout << "Answer: " << answer << "\n";
    }
    else {
        cout << "There was an error.\n";
    }
}