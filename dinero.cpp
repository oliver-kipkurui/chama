#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;
using namespace std;

const string SYSTEM_PROMPT = "Dinero, Kenyan chama treasurer. Target 1000 KES per member.";
const string SUMMARY_PROMPT = "Dinero secretary, summarise chama session.";

struct Message { string role; string content; };
map<string, int> members;
vector<Message> history;
int totalCollected = 0;

string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) {
        s[i] = tolower(s[i]);
    }
    return s;
}

bool containsStr(string hay, string needle) {
    if (hay.find(needle) == string::npos) return false;
    return true;
}

int extractAmount(string input) {
    string num = "";
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (isdigit(c)) num += c;
        else {
            if (num.empty() == false) break;
        }
    }
    if (num.empty()) return 0;
    return stoi(num);
}

bool isSkipWord(string w) {
    if (w == "add") return true;
    if (w == "paid") return true;
    if (w == "pay") return true;
    if (w == "how") return true;
    if (w == "much") return true;
    if (w == "has") return true;
    if (w == "did") return true;
    if (w == "what") return true;
    if (w == "is") return true;
    if (w == "the") return true;
    if (w == "my") return true;
    if (w == "dinero") return true;
    if (w == "total") return true;
    if (w == "balance") return true;
    return false;
}

string extractName(string input) {
    stringstream ss(input);
    string w;
    while (ss >> w) {
        string clean = "";
        for (size_t i = 0; i < w.size(); i++) {
            if (ispunct(w[i]) == false) clean += w[i];
        }
        string low = toLower(clean);
        if (isSkipWord(low)) continue;
        if (clean.empty()) continue;
        if (isupper(clean[0])) return clean;
    }
    return "";
}

string getDateStr() {
    time_t t = time(0);
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M", localtime(&t));
    return string(buf);
}

void startSession() {
    history.clear();
    members.clear();
    totalCollected = 0;

    cout << "\n--- New Dinero Session ---\n";
    cout << "Type 'exit' to end session and get summary.\n";
    cout << "Example: 'Add Otis, he paid 500' or 'How much has Otis paid?'\n";

    int turn = 0;
    while (true) {
        cout << "\nYou (Turn " << turn + 1 << "): ";
        string input;
        getline(cin, input);
        if (input.empty()) {
            cout << "Empty input, try again.\n";
            continue;
        }
        string cmd = toLower(input);
        if (cmd == "exit") break;
        if (cmd == "quit") break;
        if (cmd == "end") break;

        history.push_back({"user", input});
        string low = toLower(input);
        string reply = "";

        bool isQuestion = false;
        if (containsStr(low, "how much")) isQuestion = true;
        if (containsStr(low, "total")) isQuestion = true;
        if (containsStr(low, "balance")) isQuestion = true;

        if (isQuestion) {
            string name = extractName(input);
            bool hasName = false;
            if (name.empty() == false) hasName = true;
            bool inMap = false;
            if (hasName) {
                if (members.count(name) > 0) inMap = true;
            }
            if (inMap) {
                int paid = members[name];
                reply = name + " has paid " + to_string(paid) + " KES. Balance remaining: " + to_string(1000 - paid) + " KES.";
            } else {
                if (containsStr(low, "total")) {
                    reply = "Total collected so far is " + to_string(totalCollected) + " KES.";
            } else {
                    reply = "I do not know that member yet. Add them first like 'Add Otis paid 500'.";
                }
            }
        } else {
            string name = extractName(input);
            int amount = extractAmount(input);
            bool okName = false;
            if (name.empty() == false) okName = true;
            bool okAmount = false;
            if (amount > 0) okAmount = true;
            if (okName) {
                if (okAmount) {
                    members[name] += amount;
                    totalCollected += amount;
                    reply = "Done! Added " + name + " paid " + to_string(amount) + " KES. Total for " + name + ": " + to_string(members[name]) + " KES.";
                } else {
                    reply = "Got it! Tell me like 'Add Otis paid 500' and I will track it.";
                }
            } else {
                reply = "Got it! Tell me like 'Add Otis paid 500' and I will track it.";
            }
        }

        cout << "\nDinero: " << reply << endl;
        cout << "Total collected so far: " << totalCollected << " KES" << endl;
        cout << "[JSON: reply=" << reply << " total_collected=" << totalCollected << "]" << endl;

        history.push_back({"assistant", reply});
        turn++;
    }

    if (history.size() < 2) {
        cout << "Session too short, not saving.\n";
        return;
    }

    cout << "\n--- Generating final summary... ---\n";
    cout << "Summary: Chama session with " << members.size() << " members.\n";
    cout << "Members table:\n";
    for (auto &m : members) {
        cout << " - " << m.first << " paid " << m.second << " KES, balance " << (1000 - m.second) << " KES\n";
    }
    cout << "Total: " << totalCollected << " KES\n";

    fs::create_directories("sessions");
    string filename = "sessions/session_" + getDateStr() + ".json";
    ofstream out(filename);
    out << "{\n  \"date\": \"" << getDateStr() << "\",\n";
    out << "  \"total_collected\": " << totalCollected << ",\n";
    out << "  \"members\": {";
    bool first = true;
    for (auto &m : members) {
        if (first == false) out << ", ";
        out << "\"" << m.first << "\": " << m.second;
        first = false;
    }
    out << "},\n  \"conversation\": [\n";
    for (size_t i = 0; i < history.size(); i++) {
        out << "    {\"role\": \"" << history[i].role << "\", \"content\": \"" << history[i].content << "\"}";
        if (i + 1 < history.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    out.close();
    cout << "\nSession saved to " << filename << endl;
}

void openPastSession() {
    vector<string> files;
    if (fs::exists("sessions")) {
        for (auto &e : fs::directory_iterator("sessions")) {
            files.push_back(e.path().string());
        }
    }
    if (files.empty()) {
        cout << "No past sessions found.\n";
        return;
    }
    sort(files.begin(), files.end());
    cout << "\n--- Past Sessions ---\n";
    for (size_t i = 0; i < files.size(); i++) {
        cout << i + 1 << ". " << files[i] << endl;
    }
    cout << "Enter number to open: ";
    string choice;
    getline(cin, choice);
    if (choice.empty()) return;
    int idx = stoi(choice) - 1;
    if (idx < 0) {
        cout << "Invalid number.\n";
        return;
    }
    if (idx >= (int)files.size()) {
        cout << "Invalid number.\n";
        return;
    }
    ifstream in(files[idx]);
    stringstream ss;
    ss << in.rdbuf();
    cout << "\n--- " << files[idx] << " ---\n" << ss.str() << endl;
}

int main() {
    while (true) {
        cout << "\n===== DINERO =====\n";
        cout << "1. Start New Session\n";
        cout << "2. Open Past Session\n";
        cout << "3. Exit\n";
        cout << "Choose 1-3: ";
        string choice;
getline(cin, choice);
        if (choice == "1") startSession();
        else {
            if (choice == "2") openPastSession();
            else {
                if (choice == "3") {
                    cout << "Thank you! Dinero closed.\n";
                    break;
                } else {
                    cout << "Please choose 1, 2 or 3.\n";
                }
            }
        }
    }
    return 0;
}