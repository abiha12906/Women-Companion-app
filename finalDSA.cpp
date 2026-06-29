#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <thread>
#include <chrono>
#include <climits>
#include <algorithm>

#ifdef _WIN32
#include <conio.h>
#endif
using namespace std;
const int MAX = 100;
const int N = 8;
const int HASH_SIZE = 101;   
void saveUsersToFile();
void loadUsersFromFile();
void saveContactsToFile();
void loadContactsFromFile();
int  getValidIndex(const string& message);
string getHiddenPassword()
{
    string password;
#ifdef _WIN32
    char ch;
    while (true)
    {
        ch = _getch();
        if (ch == '\r') break;
        else if (ch == '\b')
        {
            if (!password.empty())
            {
                password.pop_back();
                cout << "\b \b";
            }
        }
        else { password += ch; cout << '*'; }
    }
    cout << '\n';
#else
    
    system("stty -echo");
    getline(cin, password);
    system("stty echo");
    cout << '\n';
#endif
    return password;
}
bool onlyLetters(const string& s)
{
    if (s.empty()) return false;
    for (char c : s)
        if (!isalpha(c) && c != ' ') return false;
    return true;
}

bool validPhone(const string& p)
{
    if (p.length() != 11) return false;
    for (char c : p)
        if (!isdigit(c)) return false;
    return true;
}

bool validCNIC(const string& c)
{
    if (c.length() != 13) return false;
    for (char ch : c)
        if (!isdigit(ch)) return false;
    return true;
}
const string areas[N] =
{
    "Gulshan","Johar","Saddar","Clifton",
    "Defence","Korangi","Malir","Nazimabad"
};
const string hospitals[N] =
{
    "Aga Khan Hospital","Darul Sehat","Civil Hospital",
    "South City Hospital","NMC Hospital","JPMC",
    "Liaquat National Hospital","Abbasi Shaheed Hospital"
};
const string policeStations[N] =
{
    "Gulshan Police","Johar Police","Saddar Police",
    "Clifton Police","Defence Police","Korangi Police",
    "Malir Police","Nazimabad Police"
};

int graph[N][N] =
{
    { 0, 4, 9, 5, 6,12,11, 5},
    { 4, 0, 5, 8, 9,10, 6, 3},
    { 9, 5, 0, 4, 8, 6, 9, 5},
    { 5, 8, 4, 0, 3, 9,10, 9},
    { 6, 9, 8, 3, 0, 6,10,10},
    {12,10, 6, 9, 6, 0, 4, 9},
    {11, 6, 9,10,10, 4, 0, 9},
    { 5, 3, 5, 9,10, 9, 9, 0}
};

int dangerScore[N] = { 40,50,80,60,70,85,75,45 };
int crowdDensity[N] = { 8, 7, 9, 8, 7, 5, 4, 7 };
int streetLighting[N] = { 8, 6, 9, 9, 8, 5, 4, 6 };


struct User
{
    string name;
    string cnic;
    string password;
    string city;
};

User users[MAX];
int  userCount = 0;
int  currentUser = -1;

// ============================================================
//  DATA STRUCTURE 1: LINKED LIST – Contacts
// ============================================================
struct Contact
{
    string   name;
    string   phone;
    string   ownerCNIC;
    Contact* next = nullptr;
};

Contact* contactHead = nullptr;
struct AlertStack
{
    string data[MAX];
    int    top = -1;

    bool empty() const { return top == -1; }
    bool full()  const { return top == MAX - 1; }

    void push(const string& msg)
    {
        if (full()) { cout << "[Stack] Alert history full.\n"; return; }
        data[++top] = msg;
    }

    void display() const
    {
        if (empty()) { cout << "No alert history found.\n"; return; }
        cout << "\n========== ALERT HISTORY ==========\n";
        for (int i = top; i >= 0; --i)
            cout << "  [" << (top - i + 1) << "] " << data[i] << '\n';
    }
} alertStack;

// ============================================================
//  DATA STRUCTURE 3: QUEUE – SOS Requests
// ============================================================
struct SOSQueue
{
    string data[MAX];
    int    front = 0, rear = -1, count = 0;

    bool empty() const { return count == 0; }
    bool full()  const { return count == MAX; }

    void enqueue(const string& msg)
    {
        if (full()) { cout << "[Queue] SOS queue full.\n"; return; }
        rear = (rear + 1) % MAX;
        data[rear] = msg;
        ++count;
        alertStack.push("SOS: " + msg);
        cout << "SOS request queued successfully.\n";
    }

    void dequeue()
    {
        if (empty()) { cout << "No pending SOS requests.\n"; return; }
        cout << "Processing SOS: " << data[front] << '\n';
        front = (front + 1) % MAX;
        --count;
    }

    void display() const
    {
        if (empty()) { cout << "SOS queue is empty.\n"; return; }
        cout << "\n========== SOS QUEUE (" << count << " pending) ==========\n";
        int idx = front;
        for (int i = 0; i < count; ++i)
        {
            cout << "  [" << (i + 1) << "] " << data[idx] << '\n';
            idx = (idx + 1) % MAX;
        }
    }
} sosQueue;

// ============================================================
//  DATA STRUCTURE 4: MAX-HEAP – Priority Emergency Queue
// ============================================================
struct Emergency { string msg; int priority; };

struct PriorityQueue
{
    Emergency heap[MAX];
    int size = 0;

    void swap(Emergency& a, Emergency& b) { Emergency t = a; a = b; b = t; }

    void heapifyUp(int i)
    {
        while (i > 0)
        {
            int parent = (i - 1) / 2;
            if (heap[parent].priority < heap[i].priority)
            {
                swap(heap[parent], heap[i]);
                i = parent;
            }
            else break;
        }
    }

    void heapifyDown(int i)
    {
        int largest = i;
        int l = 2 * i + 1, r = 2 * i + 2;
        if (l < size && heap[l].priority > heap[largest].priority) largest = l;
        if (r < size && heap[r].priority > heap[largest].priority) largest = r;
        if (largest != i) { swap(heap[i], heap[largest]); heapifyDown(largest); }
    }

    void push(const string& msg, int priority)
    {
        if (size == MAX) { cout << "[Heap] Priority queue full.\n"; return; }
        heap[size] = { msg, priority };
        heapifyUp(size++);
        cout << "Priority emergency added (Level " << priority << ").\n";
    }

    void pop()
    {
        if (size == 0) { cout << "No priority emergencies.\n"; return; }
        cout << "Processing: " << heap[0].msg
            << "  [Priority " << heap[0].priority << "]\n";
        heap[0] = heap[--size];
        heapifyDown(0);
    }

    void display() const
    {
        if (size == 0) { cout << "No priority emergencies.\n"; return; }
        cout << "\n========== PRIORITY EMERGENCIES ==========\n";
        // Print in insertion order (heap order)
        for (int i = 0; i < size; ++i)
            cout << "  [P" << heap[i].priority << "] " << heap[i].msg << '\n';
    }
} priorityQueue;

// ============================================================
//  DATA STRUCTURE 5: HASH TABLE – Cyber Blacklist (Linear Probing)
// ============================================================
struct HashTable
{
    string table[HASH_SIZE];
    bool   used[HASH_SIZE] = {};

    int hash(const string& key) const
    {
        // djb2 hash – better distribution than simple sum
        unsigned long h = 5381;
        for (char c : key) h = h * 33 + (unsigned char)c;
        return (int)(h % HASH_SIZE);
    }

    void insert(const string& key)
    {
        int idx = hash(key);
        for (int i = 0; i < HASH_SIZE; ++i)
        {
            int probe = (idx + i) % HASH_SIZE;
            if (!used[probe])
            {
                table[probe] = key;
                used[probe] = true;
                return;
            }
            if (table[probe] == key) return; // already exists
        }
        cout << "[Hash] Table full.\n";
    }

    bool search(const string& key) const
    {
        int idx = hash(key);
        for (int i = 0; i < HASH_SIZE; ++i)
        {
            int probe = (idx + i) % HASH_SIZE;
            if (!used[probe])  return false;
            if (table[probe] == key) return true;
        }
        return false;
    }

    void display() const
    {
        cout << "\n========== CYBER BLACKLIST ==========\n";
        bool any = false;
        for (int i = 0; i < HASH_SIZE; ++i)
            if (used[i]) { cout << "  " << table[i] << '\n'; any = true; }
        if (!any) cout << "Blacklist is empty.\n";
    }
} blacklist;

// ============================================================
//  DATA STRUCTURE 6: AVL TREE – Emergency Teams by Distance
// ============================================================
struct TeamNode
{
    int       distance;
    string    teamName;
    TeamNode* left = nullptr;
    TeamNode* right = nullptr;
    int       height = 1;
};

TeamNode* teamRoot = nullptr;

int avlHeight(TeamNode* n) { return n ? n->height : 0; }
int avlBalance(TeamNode* n) { return n ? avlHeight(n->left) - avlHeight(n->right) : 0; }

void avlUpdateHeight(TeamNode* n)
{
    if (n) n->height = 1 + max(avlHeight(n->left), avlHeight(n->right));
}

TeamNode* avlRotateRight(TeamNode* y)
{
    TeamNode* x = y->left;
    TeamNode* t2 = x->right;
    x->right = y; y->left = t2;
    avlUpdateHeight(y); avlUpdateHeight(x);
    return x;
}

TeamNode* avlRotateLeft(TeamNode* x)
{
    TeamNode* y = x->right;
    TeamNode* t2 = y->left;
    y->left = x; x->right = t2;
    avlUpdateHeight(x); avlUpdateHeight(y);
    return y;
}

TeamNode* avlInsert(TeamNode* node, int dist, const string& name)
{
    if (!node) { TeamNode* n = new TeamNode(); n->distance = dist; n->teamName = name; return n; }
    if (dist < node->distance) node->left = avlInsert(node->left, dist, name);
    else if (dist > node->distance) node->right = avlInsert(node->right, dist, name);
    else { cout << "[AVL] Duplicate distance key – skipped.\n"; return node; }

    avlUpdateHeight(node);
    int bal = avlBalance(node);

    if (bal > 1 && dist < node->left->distance)                       return avlRotateRight(node);
    if (bal < -1 && dist > node->right->distance)                      return avlRotateLeft(node);
    if (bal > 1 && dist > node->left->distance) { node->left = avlRotateLeft(node->left);  return avlRotateRight(node); }
    if (bal < -1 && dist < node->right->distance) { node->right = avlRotateRight(node->right); return avlRotateLeft(node); }
    return node;
}

// In-order traversal – find node with distance closest to target
struct TeamResult { TeamNode* node = nullptr; int diff = INT_MAX; };

void avlNearest(TeamNode* root, int target, TeamResult& res)
{
    if (!root) return;
    avlNearest(root->left, target, res);
    int d = abs(root->distance - target);
    if (d < res.diff) { res.diff = d; res.node = root; }
    avlNearest(root->right, target, res);
}

// ============================================================
//  DATA STRUCTURE 7: GRAPH – Dijkstra shortest safe path
// ============================================================
void dijkstra(int src, int dist[], int prev[])
{
    bool visited[N] = {};
    for (int i = 0; i < N; ++i) { dist[i] = INT_MAX; prev[i] = -1; }
    dist[src] = 0;

    for (int iter = 0; iter < N; ++iter)
    {
        // Pick unvisited vertex with smallest distance
        int u = -1;
        for (int i = 0; i < N; ++i)
            if (!visited[i] && (u == -1 || dist[i] < dist[u])) u = i;
        if (u == -1 || dist[u] == INT_MAX) break;
        visited[u] = true;

        for (int v = 0; v < N; ++v)
        {
            if (graph[u][v] == 0 || visited[v]) continue;
            int newDist = dist[u] + graph[u][v];
            if (newDist < dist[v]) { dist[v] = newDist; prev[v] = u; }
        }
    }
}

// Reconstruct Dijkstra path
void printPath(int prev[], int dest)
{
    if (prev[dest] == -1) { cout << areas[dest]; return; }
    printPath(prev, prev[dest]);
    cout << " -> " << areas[dest];
}

// BFS – find nearest hospital/police for a given source
void bfsNearestShelter(int src)
{
    bool visited[N] = {};
    int  queue[N];
    int f = 0, r = 0;
    queue[r++] = src; visited[src] = true;

    cout << "\n========== BFS: NEAREST SAFE SHELTERS ==========\n";
    int found = 0;
    while (f < r && found < 3)
    {
        int cur = queue[f++];
        cout << "Area           : " << areas[cur] << '\n';
        cout << "Hospital       : " << hospitals[cur] << '\n';
        cout << "Police Station : " << policeStations[cur] << '\n';
        cout << "Danger Score   : " << dangerScore[cur] << "/100\n";
        cout << "--------------------------------------------------\n";
        ++found;

        for (int v = 0; v < N; ++v)
            if (graph[cur][v] > 0 && !visited[v])
            {
                visited[v] = true;
                queue[r++] = v;
            }
    }
}

// ============================================================
//  COMPANION SCORE  (crowd + lighting – travel weight)
// ============================================================
int companionScore(int from, int to)
{
    int travelPenalty = (graph[from][to] > 10) ? 4 :
        (graph[from][to] > 6) ? 2 : 0;
    return crowdDensity[to] + streetLighting[to] - travelPenalty;
}

// ============================================================
//  FILE I/O
// ============================================================
void saveUsersToFile()
{
    ofstream f("users.txt");
    for (int i = 0; i < userCount; ++i)
        f << users[i].name << '\n'
        << users[i].cnic << '\n'
        << users[i].password << '\n'
        << users[i].city << '\n';
}

void loadUsersFromFile()
{
    userCount = 0;
    ifstream f("users.txt");
    if (!f) return;
    while (userCount < MAX &&
        getline(f, users[userCount].name))
    {
        getline(f, users[userCount].cnic);
        getline(f, users[userCount].password);
        getline(f, users[userCount].city);
        ++userCount;
    }
}

void saveContactsToFile()
{
    ofstream f("contacts.txt");
    for (Contact* t = contactHead; t; t = t->next)
        f << t->ownerCNIC << '|' << t->name << '|' << t->phone << '\n';
}

void loadContactsFromFile()
{
    ifstream f("contacts.txt");
    if (!f) return;
    string line;
    Contact* last = nullptr;
    // advance to existing tail
    for (Contact* t = contactHead; t; t = t->next) last = t;

    while (getline(f, line))
    {
        size_t p1 = line.find('|');
        size_t p2 = line.find('|', p1 + 1);
        if (p1 == string::npos || p2 == string::npos) continue;

        Contact* n = new Contact();
        n->ownerCNIC = line.substr(0, p1);
        n->name = line.substr(p1 + 1, p2 - p1 - 1);
        n->phone = line.substr(p2 + 1);

        if (!contactHead) contactHead = last = n;
        else { last->next = n; last = n; }
    }
}

// ============================================================
//  HELPER
// ============================================================
void showAllAreas()
{
    cout << "\n--- AREAS ---\n";
    for (int i = 0; i < N; ++i)
        cout << "  [" << i << "] " << areas[i] << '\n';
}

int getValidIndex(const string& message)
{
    int x;
    while (true)
    {
        cout << message;
        if (cin >> x && x >= 0 && x < N) return x;
        cin.clear(); cin.ignore(1000, '\n');
        cout << "  Invalid! Enter a number between 0 and " << N - 1 << ".\n";
    }
}

void notifyContacts(const string& situation)
{
    cout << "\nEmergency alert sent to your trusted contacts:\n";
    bool found = false;
    for (Contact* t = contactHead; t; t = t->next)
    {
        if (t->ownerCNIC == users[currentUser].cnic)
        {
            cout << "  -> " << t->name << "  (" << t->phone << ")\n";
            found = true;
        }
    }
    if (!found) cout << "  [!] No trusted contacts found. Add contacts from the main menu.\n";
    cout << "Nearest response team and authorities alerted.\n";
}

// ============================================================
//  FEATURE 1: USER AUTH
// ============================================================
void registerUser()
{
    if (userCount == MAX) { cout << "User limit reached.\n"; return; }
    cin.ignore();
    cout << "\n========== REGISTER ==========\n";

    do { cout << "Full Name : "; getline(cin, users[userCount].name); } while (!onlyLetters(users[userCount].name));

    do { cout << "CNIC (13 digits): "; cin >> users[userCount].cnic; } while (!validCNIC(users[userCount].cnic));

    cin.ignore();
    do { cout << "City : "; getline(cin, users[userCount].city); } while (!onlyLetters(users[userCount].city));

    cout << "Password : ";
    users[userCount].password = getHiddenPassword();

    ++userCount;
    saveUsersToFile();
    cout << "Registration successful!\n";
}

bool loginUser()
{
    string cnic, password;
    cout << "\n========== LOGIN ==========\n";
    cout << "CNIC     : "; cin >> cnic;
    cout << "Password : "; password = getHiddenPassword();

    for (int i = 0; i < userCount; ++i)
    {
        if (users[i].cnic == cnic && users[i].password == password)
        {
            currentUser = i;
            cout << "\nLogin successful! Welcome, " << users[i].name << ".\n";
            return true;
        }
    }
    cout << "Invalid credentials. Please try again.\n";
    return false;
}

// ============================================================
//  FEATURE 2: CONTACTS (Linked List)
// ============================================================
void addContact()
{
    Contact* n = new Contact();
    cin.ignore();
    do { cout << "Contact Name  : "; getline(cin, n->name); } while (!onlyLetters(n->name));
    do { cout << "Phone Number  : "; cin >> n->phone; } while (!validPhone(n->phone));

    n->ownerCNIC = users[currentUser].cnic;
    n->next = nullptr;

    if (!contactHead) contactHead = n;
    else
    {
        Contact* t = contactHead;
        while (t->next) t = t->next;
        t->next = n;
    }
    cout << "Contact added successfully.\n";
    saveContactsToFile();
}

void viewContacts()
{
    bool found = false;
    for (Contact* t = contactHead; t; t = t->next)
    {
        if (t->ownerCNIC == users[currentUser].cnic)
        {
            if (!found) { cout << "\n========== YOUR CONTACTS ==========\n"; found = true; }
            cout << "  Name  : " << t->name << '\n';
            cout << "  Phone : " << t->phone << '\n';
            cout << "  ----------------------------\n";
        }
    }
    if (!found) cout << "No contacts found. Add contacts first.\n";
}

void deleteContact()
{
    string phone;
    cin.ignore();
    cout << "Enter phone number to delete: ";
    cin >> phone;

    Contact* cur = contactHead;
    Contact* prev = nullptr;
    while (cur)
    {
        if (cur->phone == phone && cur->ownerCNIC == users[currentUser].cnic)
        {
            if (prev) prev->next = cur->next;
            else      contactHead = cur->next;
            delete cur;
            saveContactsToFile();
            cout << "Contact deleted.\n";
            return;
        }
        prev = cur; cur = cur->next;
    }
    cout << "Contact not found.\n";
}

void manageContacts()
{
    int ch;
    do {
        cout << "\n===== CONTACT MANAGER =====\n"
            "1. Add Contact\n"
            "2. View Contacts\n"
            "3. Delete Contact\n"
            "4. Back\n"
            "Choice: ";
        cin >> ch;
        if (ch == 1) addContact();
        else if (ch == 2) viewContacts();
        else if (ch == 3) deleteContact();
    } while (ch != 4);
}

// ============================================================
//  FEATURE 3: SAFE ROUTE CHECKER (Graph + Dijkstra)
// ============================================================
void safeRouteChecker()
{
    cout << "\n========== SAFE ROUTE CHECKER ==========\n";
    showAllAreas();
    int s = getValidIndex("Current Area      : ");
    int d = getValidIndex("Destination Area  : ");

    // --- Dijkstra shortest path ---
    int dist[N], prev[N];
    dijkstra(s, dist, prev);

    cout << "\n[SHORTEST SAFE ROUTE]\n  ";
    printPath(prev, d);
    cout << "\n  Total Distance : " << dist[d] << " km\n";

    // --- Danger rating for direct segment ---
    int w = graph[s][d];
    string danger = (w == 0) ? "N/A (no direct road)" :
        (w <= 5) ? "LOW" :
        (w <= 8) ? "MODERATE" : "HIGH";
    cout << "\n[DIRECT SEGMENT INFO]\n";
    cout << "  Road Distance  : " << (w ? to_string(w) + " km" : "No direct link") << '\n';
    cout << "  Danger Level   : " << danger << '\n';

    // --- Companion score for destination ---
    int cs = companionScore(s, d);
    cout << "\n[DESTINATION SAFETY]\n";
    cout << "  Crowd Density  : " << crowdDensity[d] << "/10\n";
    cout << "  Street Lighting: " << streetLighting[d] << "/10\n";
    cout << "  Companion Score: " << cs
        << (cs >= 12 ? "  -> HIGH (Well-lit & Crowded)" :
            cs >= 7 ? "  -> MODERATE" :
            "  -> LOW (Avoid if possible)") << '\n';

    // --- Alternative routes ranked by companion score ---
    struct Alt { int via; int score; int totalKm; };
    Alt alts[N]; int altCount = 0;
    for (int v = 0; v < N; ++v)
    {
        if (v == s || v == d) continue;
        if (graph[s][v] == 0 || graph[v][d] == 0) continue;
        alts[altCount] = { v,
            companionScore(s,v) + companionScore(v,d),
            graph[s][v] + graph[v][d] };
        ++altCount;
    }
    // Sort descending by companion score
    for (int i = 0; i < altCount - 1; ++i)
        for (int j = i + 1; j < altCount; ++j)
            if (alts[j].score > alts[i].score)
                swap(alts[i], alts[j]);

    cout << "\n[ALTERNATIVE ROUTES – Ranked by Safety]\n";
    if (altCount == 0)
        cout << "  No viable alternative routes found.\n";
    else
    {
        for (int i = 0; i < altCount; ++i)
        {
            int v = alts[i].via;
            cout << "  " << areas[s] << " -> " << areas[v] << " -> " << areas[d] << '\n';
            cout << "    Distance : " << alts[i].totalKm << " km"
                << "  |  Crowd: " << crowdDensity[v] << "/10"
                << "  |  Lighting: " << streetLighting[v] << "/10"
                << "  |  Score: " << alts[i].score << '\n';
        }
        cout << "\n[RECOMMENDATION] Take the top-ranked route for maximum safety.\n";
    }

    alertStack.push("Route checked: " + areas[s] + " -> " + areas[d]);
}

// ============================================================
//  FEATURE 4: NEARBY SAFE SHELTER (BFS)
// ============================================================
void nearbySafeShelterFinder()
{
    cout << "\n========== NEARBY SAFE SHELTER FINDER ==========\n";
    showAllAreas();
    int src = getValidIndex("Your Current Area : ");
    bfsNearestShelter(src);
    alertStack.push("Shelter lookup from " + areas[src]);
}

// ============================================================
//  FEATURE 5: ONE-CLICK EMERGENCY
// ============================================================
void oneClickEmergency()
{
    string location;
    cin.ignore();
    cout << "\n========== ONE-CLICK EMERGENCY ==========\n";
    cout << "Enter your current location details: ";
    getline(cin, location);

    string msg = "EMERGENCY at " + location;
    sosQueue.enqueue(msg);
    priorityQueue.push(msg, 3);
    notifyContacts(msg);
    alertStack.push(msg);
}

// ============================================================
//  FEATURE 6: AI SAFETY CHATBOT (keyword scoring)
// ============================================================
void smartChatbot()
{
    string input;
    cin.ignore();
    cout << "\n========== AI SAFETY ANALYZER ==========\n";
    cout << "Describe your situation (press Enter to analyze):\n> ";
    getline(cin, input);

    // Lowercase
    string msg = input;
    for (char& c : msg) c = (char)tolower(c);

    struct Keyword { const char* word; int score; };
    const Keyword keywords[] = {
        {"attack",    8}, {"beating",  7}, {"hit",      6},
        {"stalk",     6}, {"follow",   5}, {"following",5},
        {"thief",     8}, {"thieves",  8}, {"robber",   8},
        {"steal",     7}, {"theft",    7},
        {"harass",    5}, {"annoy",    4},
        {"suspicious",4}, {"stranger", 4},
        {"scared",    3}, {"afraid",   3}, {"help",     3},
        {"alone",     2},
    };

    int riskScore = 0;
    for (auto& kw : keywords)
        if (msg.find(kw.word) != string::npos)
            riskScore += kw.score;

    // Context: car nearby
    if (msg.find("car") != string::npos &&
        (msg.find("near") != string::npos || msg.find("outside") != string::npos))
        riskScore += 5;

    cout << "\n[AI ANALYSIS COMPLETE] — Risk Score: " << riskScore << "/100\n";
    cout << "---------------------------------------------\n";

    if (riskScore >= 12)
    {
        cout << "  STATUS     : !! CRITICAL DANGER !!\n";
        cout << "  ACTION     : Call emergency services IMMEDIATELY\n";
        cout << "  ACTION     : Move to a crowded, well-lit area\n";
        cout << "  ACTION     : Share live location with trusted contacts\n";
        sosQueue.enqueue("AI: Critical emergency");
        priorityQueue.push("AI Critical Threat", 3);
        alertStack.push("AI Chatbot: CRITICAL DANGER");
    }
    else if (riskScore >= 7)
    {
        cout << "  STATUS     : HIGH RISK\n";
        cout << "  ACTION     : Stay alert, avoid being alone\n";
        cout << "  ACTION     : Contact a trusted person now\n";
        cout << "  ACTION     : Avoid confrontation\n";
        priorityQueue.push("AI High Risk Alert", 2);
        alertStack.push("AI Chatbot: HIGH RISK");
    }
    else if (riskScore >= 3)
    {
        cout << "  STATUS     : CAUTION\n";
        cout << "  ACTION     : Be aware of your surroundings\n";
        cout << "  ACTION     : Stay in safe public areas\n";
        alertStack.push("AI Chatbot: CAUTION");
    }
    else
    {
        cout << "  STATUS     : SAFE\n";
        cout << "  NOTE       : No immediate threat detected.\n";
        alertStack.push("AI Chatbot: SAFE");
    }

    const string tips[] = {
        "Avoid isolated areas at night.",
        "Share your live location with trusted contacts.",
        "Stay in well-lit, crowded public places.",
        "Keep emergency numbers on speed dial.",
        "Trust your instincts — if something feels wrong, act."
    };
    cout << "\n[SAFETY TIP] " << tips[riskScore % 5] << '\n';
}

// ============================================================
//  FEATURE 7: SMART TRAVEL TIMER
// ============================================================
void travelTimer()
{
    cout << "\n========== SMART TRAVEL TIMER ==========\n";
    showAllAreas();
    int s = getValidIndex("Current Area     : ");
    int d = getValidIndex("Destination Area : ");

    int dist[N], prev[N];
    dijkstra(s, dist, prev);

    int km = dist[d];
    int minutes = max(1, km / 2);   // assume ~30 km/h average in city
    int seconds = minutes * 60;

    cout << "\nRoute      : ";
    printPath(prev, d);
    cout << "\nDistance   : " << km << " km\n";
    cout << "Est. Time  : " << minutes << " min\n";
    cout << "\nTimer started. Press Ctrl+C to cancel.\n";

    for (int i = seconds; i >= 1; --i)
    {
        cout << "\r  Remaining: " << i << " sec   " << flush;
        this_thread::sleep_for(chrono::seconds(1));
    }
    cout << '\n';

    char reply;
    cout << "Did you arrive safely? (y/n): ";
    cin >> reply;

    if (reply == 'y' || reply == 'Y')
    {
        cout << "Safe arrival confirmed.\n";
        alertStack.push("Safe arrival: " + areas[s] + " -> " + areas[d]);
    }
    else
    {
        string emsg = "Travel emergency at " + areas[d];
        sosQueue.enqueue(emsg);
        priorityQueue.push(emsg, 3);
        notifyContacts(emsg);
        alertStack.push(emsg);
    }
}

// ============================================================
//  FEATURE 8: CYBER CRIME CHECK (Hash Table)
// ============================================================
void cyberCrimeCheck()
{
    string key;
    cout << "\n========== CYBER CRIME CHECKER ==========\n";
    cout << "Enter phone number / ID / URL to check: ";
    cin >> key;

    if (blacklist.search(key))
    {
        cout << "\n!! WARNING: BLACKLISTED RECORD FOUND !!\n";
        cout << "   This entry has been flagged as suspicious.\n";
        cout << "   Do NOT engage. Report to cybercrime authorities.\n";
        alertStack.push("Cyber check: BLACKLISTED – " + key);
    }
    else
    {
        cout << "\n  No blacklist record found for: " << key << '\n';
        cout << "  Exercise general caution online.\n";
        alertStack.push("Cyber check: CLEAR – " + key);
    }
}

// ============================================================
//  FEATURE 9: EMERGENCY TEAM ALLOCATION (AVL Tree)
// ============================================================
void emergencyTeamAllocation()
{
    cout << "\n========== EMERGENCY TEAM ALLOCATION ==========\n";
    showAllAreas();
    int ch = getValidIndex("Your current area : ");

    TeamResult res;
    avlNearest(teamRoot, ch, res);

    if (res.node)
    {
        cout << "\nAllocated Team   : " << res.node->teamName << '\n';
        cout << "Base Distance    : " << res.node->distance << " km\n";
        cout << "ETA              : ~" << (res.node->distance * 3) << " minutes\n";
        alertStack.push("Team allocated: " + res.node->teamName);
    }
    else
        cout << "No emergency teams available at this time.\n";
}

// ============================================================
//  ADMIN PANEL
// ============================================================
bool adminLogin()
{
    string id, pwd;
    cout << "\n========== ADMIN LOGIN ==========\n";
    cout << "Admin ID  : "; cin >> id;
    cout << "Password  : "; pwd = getHiddenPassword();
    return (id == "admin" && pwd == "admin123");
}

void adminPanel()
{
    int ch;
    do {
        cout << "\n===== ADMIN PANEL =====\n"
            " 1. View Registered Users\n"
            " 2. View SOS Queue\n"
            " 3. Process Next SOS\n"
            " 4. View Priority Emergencies\n"
            " 5. Process Highest-Priority Emergency\n"
            " 6. View Cyber Blacklist\n"
            " 7. Add to Blacklist\n"
            " 8. Update Area Danger Score\n"
            " 9. Update Crowd/Lighting Data\n"
            "10. Logout\n"
            "Choice: ";
        cin >> ch;

        if (ch == 1)
        {
            if (userCount == 0) { cout << "No users registered.\n"; continue; }
            cout << "\n========== REGISTERED USERS ==========\n";
            for (int i = 0; i < userCount; ++i)
                cout << (i + 1) << ". " << users[i].name
                << "  CNIC: " << users[i].cnic
                << "  City: " << users[i].city << '\n';
        }
        else if (ch == 2) sosQueue.display();
        else if (ch == 3) sosQueue.dequeue();
        else if (ch == 4) priorityQueue.display();
        else if (ch == 5) priorityQueue.pop();
        else if (ch == 6) blacklist.display();
        else if (ch == 7)
        {
            string key;
            cout << "Enter value to blacklist: "; cin >> key;
            blacklist.insert(key);
            cout << "Blacklist updated.\n";
        }
        else if (ch == 8)
        {
            showAllAreas();
            int area = getValidIndex("Area number: ");
            int score;
            do { cout << "New danger score (1-100): "; cin >> score; } while (score < 1 || score > 100);
            dangerScore[area] = score;
            cout << "Danger score updated for " << areas[area] << ".\n";
        }
        else if (ch == 9)
        {
            showAllAreas();
            int area = getValidIndex("Area number: ");
            int crowd, light;
            do { cout << "Crowd density (1-10): ";    cin >> crowd; } while (crowd < 1 || crowd > 10);
            do { cout << "Street lighting (1-10): "; cin >> light; } while (light < 1 || light > 10);
            crowdDensity[area] = crowd;
            streetLighting[area] = light;
            cout << "Companion data updated for " << areas[area] << ".\n";
        }
    } while (ch != 10);
}

// ============================================================
//  USER APP MENU
// ============================================================
void appMenu()
{
    int ch;
    do {
        cout << "\n===== WOMEN SAFETY APP — " << users[currentUser].name << " =====\n"
            " 1. Manage Contacts\n"
            " 2. Safe Route Checker\n"
            " 3. Nearby Safe Shelter Finder\n"
            " 4. One-Click Emergency Alert\n"
            " 5. AI Safety Chatbot\n"
            " 6. Smart Travel Timer\n"
            " 7. Cyber Crime Checker\n"
            " 8. Emergency Team Allocation\n"
            " 9. Alert History\n"
            "10. Logout\n"
            "Choice: ";
        cin >> ch;
        if (ch == 1) manageContacts();
        else if (ch == 2) safeRouteChecker();
        else if (ch == 3) nearbySafeShelterFinder();
        else if (ch == 4) oneClickEmergency();
        else if (ch == 5) smartChatbot();
        else if (ch == 6) travelTimer();
        else if (ch == 7) cyberCrimeCheck();
        else if (ch == 8) emergencyTeamAllocation();
        else if (ch == 9) alertStack.display();
    } while (ch != 10);
    currentUser = -1;
}

// ============================================================
//  MAIN
// ============================================================
int main()
{
    // Load persistent data
    loadUsersFromFile();
    loadContactsFromFile();

    // Seed AVL tree with emergency teams
    teamRoot = avlInsert(teamRoot, 3, "Women Protection Unit");
    teamRoot = avlInsert(teamRoot, 5, "Police Rapid Response");
    teamRoot = avlInsert(teamRoot, 7, "Search & Rescue Team");
    teamRoot = avlInsert(teamRoot, 2, "First Aid Unit");
    teamRoot = avlInsert(teamRoot, 9, "Mobile Crisis Team");

    // Seed cyber blacklist
    blacklist.insert("03123456789");
    blacklist.insert("fakeid123");
    blacklist.insert("http://phishing-site.com");

    int ch;
    do {
        cout << "\n===== WOMEN SAFETY COMPANION APP =====\n"
            "1. Register\n"
            "2. Login\n"
            "3. Admin Login\n"
            "4. Exit\n"
            "Choice: ";
        cin >> ch;

        if (ch == 1) registerUser();
        else if (ch == 2)
        {
            if (loginUser()) appMenu();
        }
        else if (ch == 3)
        {
            if (adminLogin()) adminPanel();
            else cout << "Invalid admin credentials.\n";
        }
        else if (ch == 4)
            cout << "Stay safe. Goodbye!\n";
        else
            cout << "Invalid option. Try again.\n";

    } while (ch != 4);

    return 0;
}