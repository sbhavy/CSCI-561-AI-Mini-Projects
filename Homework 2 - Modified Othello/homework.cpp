#include <iostream>
#include <sstream>
#include <fstream>
#include <queue>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <cfloat>
#include <bitset>

std::unordered_map<std::string, double> previousStates;

struct State
{
	char state[12][12];
	int move;
	double value;

	State(char state[12][12], int move, double value)
	{
		std::memcpy(this->state, state, 144 * sizeof(char));
		this->move = move; this->value = value;
	}
};

struct ComparePairs 
{
    bool operator()(const State& a, const State& b) const
    {
        return a.value < b.value;
    }
};

typedef std::bitset<144> bitboard;

bitboard cornerMask = 0x0;
bitboard CSquareMask = 0x0;
bitboard XSquareMask = 0x0;
bitboard edgeMask = 0x0;

void initMasks()
{
	cornerMask.set(143); cornerMask.set(0); cornerMask.set(11); cornerMask.set(132);

	for(int i = 2; i < 11; i++)
	{
		edgeMask.set(i); 
		edgeMask.set(132 + i);
		edgeMask.set(12*i); 
		edgeMask.set(12*i + 11);
	}
}

bitboard N(bitboard x) { return x << 12; }
bitboard S(bitboard x) { return x >> 12; }

bitboard E(bitboard x) 
{
	bitboard eastMap; eastMap.set();
	for(int i = 0; i < 12; i++) eastMap.reset(12*i);
	return (x & eastMap) >> 1;
}

bitboard W(bitboard x) 
{
	bitboard westMap; westMap.set();
	for(int i = 0; i < 12; i++) westMap.reset(11 + 12*i);
	return (x & westMap) << 1;
}

bitboard NW(bitboard x) { return N(W(x));}
bitboard NE(bitboard x) { return N(E(x));}
bitboard SW(bitboard x) { return S(W(x));}
bitboard SE(bitboard x) { return S(E(x));}

int num_moves(bitboard mine, bitboard their) 
{ 
  bitboard result = 0;
  bitboard empty = ~(mine | their);
  
  bitboard temp = N(mine) & their;
  for (int i=0; i<9; ++i) temp |= N(temp) & their;
  result |= N(temp) & empty;
 
  temp = S(mine) & their;
  for (int i=0; i<9; ++i) temp |= S(temp) & their;
  result |= S(temp) & empty;
 
  temp = W(mine) & their;
  for (int i=0; i<9; ++i) temp |= W(temp) & their;
  result |= W(temp) & empty;
 
  temp = E(mine) & their;
  for (int i=0; i<9; ++i) temp |= E(temp) & their;
  result |= E(temp) & empty;
 
  temp = NW(mine) & their;
  for (int i=0; i<9; ++i) temp |= NW(temp) & their;
  result |= NW(temp) & empty;
 
  temp = NE(mine) & their;
  for (int i=0; i<9; ++i) temp |= NE(temp) & their;
  result |= NE(temp) & empty;
 
  temp = SW(mine) & their;
  for (int i=0; i<9; ++i) temp |= SW(temp) & their;
  result |= SW(temp) & empty;
 
  temp = SE(mine) & their;
  for (int i=0; i<9; ++i) temp |= SE(temp) & their;
  result |= SE(temp) & empty;
 
  return result.count();
}

int num_frontiers(bitboard mine, bitboard their) 
{
  
  bitboard result = 0;
  bitboard empty = ~(mine | their);
  
  bitboard temp = N(mine); temp |= E(temp); temp |= W(temp); temp |= S(temp);
  temp |= NW(temp); temp |= NE(temp); temp |= SW(temp); temp |= SE(temp);
 
  return (temp & empty).count();
}

int num_edges(bitboard b) { return (b & edgeMask).count(); }

int XSquareHeuristic(bitboard mine, bitboard their)
{
	bitboard occupied = (mine | their);
	bitboard empty = ~occupied;

	bitboard bad_x = empty & cornerMask;
	bad_x = SE(bad_x); bad_x |= NE(bad_x); bad_x |= SW(bad_x); bad_x |= NW(bad_x);

	bitboard good_x = occupied & cornerMask;
	good_x = SE(good_x); good_x |= NE(good_x); good_x |= SW(good_x); good_x |= NW(good_x);

	return (mine & good_x).count() - (mine & bad_x).count();
}

int CSquareHeuristic(bitboard mine, bitboard their)
{
	bitboard occupied = (mine | their);
	bitboard empty = ~occupied;

	bitboard bad_c = empty & cornerMask;
	bad_c = N(bad_c); bad_c |= E(bad_c); bad_c |= W(bad_c); bad_c |= S(bad_c);

	bitboard good_c = occupied & cornerMask;
	good_c = N(good_c); good_c |= E(good_c); good_c |= W(good_c); good_c |= S(good_c);

	return (mine & good_c).count() - (mine & bad_c).count();
}

char player, other;

double Max(char board[12][12], double alpha, double beta, int depth);
double Min(char board[12][12], double alpha, double beta, int depth);

std::unordered_map<int, std::vector<int>> validMoves(char board[12][12], char color)
{
	std::unordered_map<int, std::vector<int>> movesList;

	for(int x = 0; x < 12; x++)
	{
		for(int y = 0; y < 12; y++)
		{
			std::vector<int> pivots = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
			if(board[x][y] != '.') continue;

			for(int dx = -1; dx <= 1; dx++)
			{
				for(int dy = -1; dy <= 1; dy++)
				{
					if (dx == 0 and dy == 0) continue;
					if (x + dx < 0 or x + dx > 11 or y + dy < 0 or y + dy > 11) continue;
					if (board[x + dx][y + dy] == '.' or board[x + dx][y + dy] == color) continue;

					for(int i = 2; i < 12; i++)
					{
						if (x + i*dx < 0 or x + i*dx > 11 or y + i*dy < 0 or y + i*dy > 11) break;
						if (board[x + i*dx][y + i*dy] == '.') break;
						if (board[x + i*dx][y + i*dy] == color) 
						{
								pivots[3*dx + dy + 4] = 12*(x + i*dx) + (y + i*dy);
								pivots[9] = 0; break;
						}
					}
				}
			}

			if (pivots[9] == 0) movesList[12*x + y] = pivots;
		}
	}

	return movesList;
}

void makeMove(char (&board)[12][12], char color, int x, int y, std::vector<int> pivots)
{
	if (pivots[9] == -1) return;
	board[x][y] = color;

	for(int dx = -1; dx <= 1; dx++)
	{
		for(int dy = -1; dy <= 1; dy++)
		{
			if (pivots[3*dx + dy + 4] == -1) continue;
			int x_lim = pivots[3*dx + dy + 4] / 12, y_lim = pivots[3*dx + dy + 4] % 12;
			for(int i = 1; i < 12 and !(x + i*dx == x_lim and y + i*dy == y_lim); i++)
				board[x + i*dx][y + i*dy] = color;
		}
	}
}

double stateEval(char board[12][12])
{
	std::string currState(&board[0][0], &board[11][11] + 1);
	if(previousStates.find(currState) != previousStates.end()) return previousStates[currState];

	double my_color = 0, other_color = 0;
	double my_stability = 0, other_stability = 0;
	bitboard mine, theirs;

	for(int i = 0; i < 12; i++)
		for(int j = 0; j < 12; j++)
		{
			if (board[i][j] == player)
			{
				my_color += 1;
				mine.set(144 - 12*i - j - 1);
			}

			else if (board[i][j] == other)
			{
				other_color += 1;
				theirs.set(144 - 12*i - j - 1);
			}
		}

	if (!my_color) return -DBL_MAX; if (!other_color) return DBL_MAX;

	double my_moves = num_moves(mine, theirs);
	double other_moves = num_moves(theirs, mine);

	if(!my_moves and !other_moves) return (my_color + (player == 'X' ? 1 : -1)) > other_color ? DBL_MAX: -DBL_MAX;

	double my_corners = (mine & cornerMask).count();
	double other_corners = (theirs & cornerMask).count();

	double my_CSquares = CSquareHeuristic(mine, theirs);
	double other_CSquares = CSquareHeuristic(theirs, mine);

	double my_XSquares = XSquareHeuristic(mine, theirs);
	double other_XSquares = XSquareHeuristic(theirs, mine);

	double my_frontiers = num_frontiers(mine, theirs);
	double other_frontiers = num_frontiers(theirs, mine);

	double my_edges = num_edges(mine);
	double other_edges = num_edges(theirs);

	double eval = 0;

	eval += 10*(my_corners - other_corners) / 4;
	eval += 3*(my_CSquares - other_CSquares) / 8;
	eval += 3*(my_XSquares - other_XSquares) / 4;
	if(my_moves + other_moves) eval += 7*(my_moves - other_moves)/(my_moves + other_moves);
	if(my_frontiers - other_frontiers) eval += -7*(my_frontiers - other_frontiers)/(my_frontiers + other_frontiers);
	if(my_color + other_color) eval += 5*(my_color - other_color)/(my_color + other_color);
	if(my_edges + other_edges) eval += 5*(my_edges - other_edges)/(my_edges + other_edges);
	previousStates[currState] = eval;
	return eval;
}

double Max(char board[12][12], double alpha, double beta, int depth)
{
	if (!depth) return stateEval(board);
	double A = alpha, B = beta;

	std::unordered_map<int, std::vector<int>> movesList = validMoves(board, player);
	if (!movesList.size()) return Min(board, A, B, depth - 1);

	double value = -DBL_MAX;

	std::priority_queue<State, std::vector<State>, ComparePairs> Q;

	for(const auto &pair: movesList)
	{
		char temp[12][12]; std::memcpy(temp, board, 144 * sizeof(char));
		makeMove(temp, player, pair.first / 12, pair.first % 12, pair.second);
		State newState = State(temp, pair.first, stateEval(temp));
		Q.push(newState);
	}

	while(!Q.empty())
	{
		State temp = Q.top(); Q.pop();
		value = std::max(value, Min(temp.state, A, B, depth - 1));
		if (value >= B) return value;
		A = std::max(value, A);
	}

	return value;
}

double Min(char board[12][12], double alpha, double beta, int depth)
{
	if (!depth) return stateEval(board);
	double A = alpha, B = beta;

	std::unordered_map<int, std::vector<int>> movesList = validMoves(board, other);
	if (!movesList.size()) return Max(board, A, B, depth - 1);

	double value = DBL_MAX;

	std::priority_queue<State, std::vector<State>, ComparePairs> Q;

	for(const auto &pair: movesList)
	{
		char temp[12][12]; std::memcpy(temp, board, 144 * sizeof(char));
		makeMove(temp, player, pair.first / 12, pair.first % 12, pair.second);
		State newState = State(temp, pair.first, -stateEval(temp));
		Q.push(newState);
	}

	while(!Q.empty())
	{
		State temp = Q.top(); Q.pop();
		value = std::min(value, Max(temp.state, A, B, depth - 1));
		if (A >= value) return value;
		B = std::min(value, B);
	}

	return value;
}

std::string alphaBetaSearch(char board[12][12], int depth)
{
	int move = -1; double value;
	double alpha = -DBL_MAX, beta = DBL_MAX;
	std::unordered_map<int, std::vector<int>> movesList = validMoves(board, player);

	if (depth == 1)
	{
		auto it = movesList.begin();
		std::advance(it, rand() % movesList.size());
		int move = it->first;
		int row = move / 12, col = move % 12;
		return char(col + 97) + std::to_string(row + 1);
	}

	std::priority_queue<State, std::vector<State>, ComparePairs> Q;

	for(const auto &pair: movesList)
	{
		char temp[12][12]; std::memcpy(temp, board, 144 * sizeof(char));
		makeMove(temp, player, pair.first / 12, pair.first % 12, pair.second);
		State newState = State(temp, pair.first, stateEval(temp));
		Q.push(newState);
	}

	while(!Q.empty())
	{
		State temp = Q.top(); Q.pop();
		value = Min(temp.state, alpha, beta, depth - 1);
		if (value >= alpha) 
		{
			alpha = value; move = temp.move;
			if (alpha >= beta) break;
		}	
	}

	int row = move / 12, col = move % 12;
	return char(col + 97) + std::to_string(row + 1);
}

int main()
{
	std::ifstream file("input.txt", std::ios::in|std::ios::binary); std::string str;

	getline(file, str); player = str[0]; other = (player == 'O') ? 'X' : 'O';
	getline(file, str); std::vector<std::string> times;
	std::istringstream is(str); std::string temp;
	while(getline(is, temp, ' ')) times.push_back(temp);
	initMasks(); char board[12][12]; double time = stod(times[0]); int count = 0;

	for(int i = 0; i < 12; i++)
	{
		getline(file, str);
		for(int j = 0; j < 12; j++) 
		{
			board[i][j] = str[j];	
			if (str[j] != '.') count++;
		}
	} 
	
	std::ofstream out("output.txt"); 

	if(time < 0.2) out << alphaBetaSearch(board, 1);
	else if(time < 1) out << alphaBetaSearch(board, 2);
	else if(time < 3) out << alphaBetaSearch(board, 3);
	else if(time < 20) out << alphaBetaSearch(board, 4);
	else if(time < 80) out << alphaBetaSearch(board, 5);
	else 
	{
		if(count > 48 and count < 96) out << alphaBetaSearch(board, 6);
		else out << alphaBetaSearch(board, 5);
	}

	return 0;
}