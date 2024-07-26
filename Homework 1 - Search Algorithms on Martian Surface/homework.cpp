#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <cmath>

using namespace std;

struct Node
{
	string state;
	Node* parent;
	double cost;
	double heuristic;
	Node(string state, Node* parent, double cost = 0, double heuristic = 0) : state(state), parent(parent), cost(cost), heuristic(heuristic) {}
};

struct CompareCost
{
	bool operator()(Node* node1, Node* node2) 
	{ return (node1->cost + node1->heuristic) > (node2->cost + node2->heuristic); }
};

double dist(int x1, int x2, int y1, int y2, int z1 = 0, int z2 = 0)
{
	return sqrt(pow(x1 - x2, 2.0) + pow(y1 - y2, 2.0) + pow(z1 - z2, 2.0));
}

string Path(Node* temp)
{
	string ans = "goal";
	temp = temp->parent;
	string now;

	while(temp)
	{
		istringstream ss(temp->state);
		ss >> now; 
		ans = now + " " + ans;
		temp = temp->parent;
	}
			
	return ans;
}

string BFS(unordered_map <string, vector<int>> States, unordered_map <string, vector<string>> Edges, int fuel)
{
	queue<Node*> Q;
	unordered_set<string> visited;

	Q.push(new Node("start start", (Node*)NULL));
	visited.insert("start start");

	while(!Q.empty())
	{
		Node *temp = Q.front(); Q.pop();
		istringstream ss(temp->state);
		string curr, parent; ss >> curr; ss >> parent;

		if(curr == "goal") return Path(temp);

		for(string i: Edges[curr])
		{
			bool flag = false; 
			if(States[i][2] > States[curr][2])
			{
				if(fuel >= States[i][2] - States[curr][2]) flag = true;
				else if(fuel >= States[i][2] - States[parent][2]) flag = true;
			}
            
            else flag = true;
			   
			if(flag and (visited.find(i + " " + curr) == visited.end())) 
			{
				Q.push(new Node(i + " " + curr, temp));
				visited.insert(i + " " + curr);
			}
		}
	}
	return "FAIL";
}

string UCS(unordered_map <string, vector<int>> States, unordered_map <string, vector<string>> Edges, int fuel)
{
	priority_queue<Node*, vector<Node*>, CompareCost> Q;
	unordered_set<string> visited;

	Q.push(new Node("start start", (Node*)NULL, 0));
	visited.insert("start start");

	while(!Q.empty())
	{
		Node *temp = Q.top(); Q.pop();
		istringstream ss(temp->state);
		string curr, parent; ss >> curr; ss >> parent;

		if(curr == "goal") return Path(temp);

		for(string i: Edges[curr])
		{
			bool flag = false; 

			if(States[i][2] > States[curr][2])
			{
				if(fuel >= States[i][2] - States[curr][2]) flag = true;
				else if(fuel >= States[i][2] - States[parent][2]) flag = true;
			}
            
            else flag = true;
			   
			if(flag and (visited.find(i + " " + curr) == visited.end())) 
			{
				double extra = dist(States[i][0], States[curr][0], States[i][1], States[curr][1]);
				Q.push(new Node(i + " " + curr, temp, temp->cost + extra));
				visited.insert(i + " " + curr);
			}
		}
	}

	return "FAIL";
}

string AStar(unordered_map <string, vector<int>> States, unordered_map <string, vector<string>> Edges, int fuel)
{
	priority_queue<Node*, vector<Node*>, CompareCost> Q;
	unordered_set<string> visited;

	double h = dist(States["start"][0], States["goal"][0], States["start"][1], States["goal"][1], States["start"][2], States["goal"][2]);
	Q.push(new Node("start start", (Node*)NULL, 0, h)); visited.insert("start start");

	while(!Q.empty())
	{
		Node *temp = Q.top(); Q.pop();
		istringstream ss(temp->state);
		string curr, parent; ss >> curr; ss >> parent;

		if(curr == "goal") return Path(temp);	

		for(string i: Edges[curr])
		{
			bool flag = false; 

			if(States[i][2] > States[curr][2])
			{
				if(fuel >= States[i][2] - States[curr][2]) flag = true;
				else if(fuel >= States[i][2] - States[parent][2]) flag = true;
			}
            
            else flag = true;
			   
			if(flag and (visited.find(i + " " + curr) == visited.end())) 
			{
				double extra = dist(States[i][0], States[curr][0], States[i][1], States[curr][1], States[i][2], States[curr][2]);
				h = dist(States[i][0], States["goal"][0], States[i][1], States["goal"][1], States[i][2], States["goal"][2]);
				Q.push(new Node(i + " " + curr, temp, temp->cost + extra, h));
				visited.insert(i + " " + curr);
			}
		}
	}

	return "FAIL";
}

int main()
{
	ifstream file("input.txt", ios::in|ios::binary); string str;

	getline(file, str); string algo = str;
	getline(file, str); int fuel = stoi(str);
	getline(file, str); int numStates = stoi(str);

	unordered_map <string, vector<int>> States;

	for(int i = 0; i < numStates; i++)
	{
		vector<string> vals;
		getline(file, str);
		istringstream is(str);
		string temp;
		while(getline(is, temp, ' ')) vals.push_back(temp);
		States.insert({vals[0], {stoi(vals[1]), stoi(vals[2]), stoi(vals[3])}});	
	}	

	getline(file, str); int numEdges = stoi(str);
	
	unordered_map <string, vector<string>> Edges;

	for(int i = 0; i < numEdges; i++)
	{
		vector <string> vals;
		getline(file, str);
		istringstream is(str);
		string temp;
		while(getline(is, temp, ' ')) vals.push_back(temp);
		Edges[vals[0]].push_back(vals[1]);
	    Edges[vals[1]].push_back(vals[0]);
	}

	ofstream out("output.txt");
	if(algo == "BFS") out << BFS(States, Edges, fuel);
	else if(algo == "UCS") out << UCS(States, Edges, fuel);
	else out << AStar(States, Edges, fuel);

	return 0;
}
