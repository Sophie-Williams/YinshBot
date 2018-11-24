#include<iostream>
#include<sstream>
#include<fstream>
#include<string>
#include<vector>
#include<iterator>
#include<algorithm>
#include<cstdlib>
#include<set>
#include<cmath>
#include<ctime>
#include<unordered_map>

float inf = 100000; // infinity to be put into beta
float minf = -100000; // infinity to be put into alpha
using namespace std;

;

float weights[3][5];
float boardWeights[11];

int player_id;
int board_size;
int game_timer;
int numberOfRings;
int sequenceLength;
int currSet;
int countMoves = 0;

float explore;

//Game Mechanics
static int ** board;
static int ** dummyBoard[2];

bool ringsPlaced = 0;
bool stalemate = 0;   

int depth; // Keeps track of depth for the search
typedef struct Coord { // Stores hex - value pairs
	Coord(int hex,int pos): hex(hex),pos(pos) {}
	Coord() {}
	int hex;
	int pos;
	bool operator ==(const Coord& a) {
		return  (this->hex == a.hex and this->pos == a.pos);
	}
	void operator=(const Coord& a) {
		this->hex = a.hex;
		this->pos = a.pos;
	}
	bool operator<(const Coord&a) const{
		if(this->hex<a.hex){
			return true;
		}
		else if(this->hex == a.hex){
			return this->pos < a.pos;
		}
		return false;
	}
} Coord;
Coord nullCoord = Coord(-1,-1);
Coord prevCoord = Coord(-1,-1);
typedef struct PlaceInfo {
	PlaceInfo(int player,Coord ringPlacement):player(player),ring(ringPlacement) {}
	PlaceInfo() {};
	Coord ring;
	int player;
	bool operator ==(const PlaceInfo& a) {
		return(this->player == a.player and this->ring.hex == a.ring.hex and this->ring.pos == a.ring.pos);
	}
	void operator=(const PlaceInfo& a) {
		this->ring = a.ring;
		this->player = a.player;
	}
} PlaceInfo;
typedef struct Move { // Stores move received from the client of other player (Not used anywhere else)
	Move(string moveType,int hex,int pos) : moveType(moveType),hex(hex),pos(pos) {}
	string moveType;
	int hex;
	int pos;
} Move;
typedef struct RowInfo { // Stores any row found in the board
	RowInfo(Coord start,Coord end,int direction,int player):start(start),end(end),direction(direction),player(player) {}
	Coord start;
	Coord end;
	int direction;
	int player;
	bool operator==(const RowInfo& a) {
		return (this->direction == a.direction and this->player == a.player and this->start == a.start and this->end == a.end);
	}
	void operator=(const RowInfo& a) {
		this->start = a.start;
		this->end = a.end;
		this->direction = a.direction;
		this->player = a.player;
	}
} RowInfo;
typedef struct Removal { // Stores info about an removal - remove rows and remove a ring
	Removal(RowInfo rowRemoval,Coord ringRemoval):rowRemoval(rowRemoval),ringRemoval(ringRemoval) {}
	RowInfo rowRemoval;
	Coord ringRemoval;
	bool operator==(const Removal& a) {
		return (this->ringRemoval == a.ringRemoval and this->rowRemoval == a.rowRemoval);
	}
	void operator=(const Removal& a) {
		this->ringRemoval = ringRemoval;
		this->rowRemoval = rowRemoval;
	}
} Removal;
typedef struct MoveInfo { // Stores info about a ring movement
	MoveInfo(Coord start,Coord end,int direction):start(start),end(end),direction(direction) {}
	MoveInfo() {
		start = Coord(-1,-1);
		end = Coord(-1,-1);
		direction = -1;
	}
	Coord start;
	Coord end;
	int direction; // Direction to move
	bool operator ==(const MoveInfo& a) {
		return (this->direction == a.direction and this->start == a.start and this->end == a.end);
	}
	void operator=(const MoveInfo& a) {
		this->direction = a.direction;
		this->start = a.start;
		this->end = a.end;
	}
} MoveInfo;
typedef struct Ply { // Is a ply (Either just a move) or (A sequence of row removals)
	Ply(MoveInfo moveInfo):moveInfo(moveInfo) {
		vector<Removal> rowRemovals;
		flag = 0;
		sortingFactor = 0;
	}
	Ply() {
		vector<Removal> rowRemovals;
		flag = -1;
		sortingFactor = 0;
	}
	Ply(Coord ringPlace,int player):ringPlacement(PlaceInfo(player,ringPlace)) {
		vector<Removal> rowRemovals;
		flag = 1;
		sortingFactor = 0;
	}
	bool operator==(const Ply &b) {
		if(this->flag != b.flag) {
			return 0;
		} else {
			if(this->flag == -1) {
				if(this->rowRemovals.size() != b.rowRemovals.size()) {
					return 0;
				}
				for(int i=0; i<this->rowRemovals.size(); i++) {
					if(!(this->rowRemovals[i] == b.rowRemovals[i])) {
						return 0;
					}
				}
				return 1;
			}
			if(b.flag == 0) {
				return this->moveInfo == b.moveInfo;
			} else {
				return this->ringPlacement == b.ringPlacement;
			}
		}
	}
	void operator=(const Ply& a) {
		this->flag = a.flag;
		if(this->flag == 0) {
			this->moveInfo = a.moveInfo;
		} else if(this->flag == 1) {
			this->ringPlacement = a.ringPlacement;
		} else {
			for(int i=0; i<a.rowRemovals.size(); i++) {
				Removal temp = a.rowRemovals[i];
				this->rowRemovals.push_back(temp);
			}
		}
		this->sortingFactor = a.sortingFactor;
	}
	int sortingFactor;
	MoveInfo moveInfo;
	PlaceInfo ringPlacement;
	vector <Removal> rowRemovals;
	int flag;
} Ply;
void resetDummy();
typedef struct Answer {
	Answer(int flag) {
		ply.flag = flag;
	}
	Ply ply;
	float value;
	bool operator=(const Answer& a) {
		this->ply = a.ply;
		this->value = a.value;
	}
} Answer;
struct myclass {
  bool operator() (const Ply &a,const Ply& b) { return (a.sortingFactor > b.sortingFactor);}
} sortPlysMax;
struct otherclass{
 bool operator() (const Ply& a,const Ply& b){ return (a.sortingFactor < b.sortingFactor);}
}sortPlysMin;

vector<Coord> positions;
void getPositions() {
	positions.push_back(Coord(0,0));
	for(int hex=1; hex<=board_size; hex++) {
		for(int pos=0; pos<6*hex; pos++) {
			if(hex == board_size and pos%hex == 0) {
				continue;
			}
			positions.push_back(Coord(hex,pos));
		}
	}
}
Coord moveDirection(Coord current,int direction);
class Player {
	public:
		int id;
		int ringsOnBoard; // number of rings on board at current time
		vector <Coord> positions; // list of ring positions
		Player() { // Constructor
			this->ringsOnBoard = 0;
		}
		void placeRing(int hex,int pos) { // Places rings on the board
			positions.push_back(Coord(hex,pos));
			ringsOnBoard++;
		}
		void removeRing(int hex,int pos) { // remove rings given a position
			vector<Coord> ::iterator j = positions.begin();
			int i=0;
			for(i = 0;i<this->ringsOnBoard;i++) {
				if(positions[i].hex == hex and positions[i].pos == pos){
					break;
				}
			}
			if(i<ringsOnBoard){
				positions.erase(j+i);
				ringsOnBoard--;
			}
		}
};
Player players[2];
void initiatePlayers() {
	players[0] = Player();
	players[1] = Player();
}
bool checkRow(RowInfo row);
Coord moveSouth(Coord current) {
	if(current.hex == 0) {
		return Coord(1,3);
	}
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	Coord answer;
	if(shift == 0) {
		switch(segment) {
			case 0 :
				answer= Coord(current.hex-1,current.pos);
				break;
			case 1:
				answer = Coord(current.hex,current.pos+1);
				break;
			case 2:
				answer =   Coord(current.hex+1,current.pos+3);
				break;
			case 3:
				answer =  Coord(current.hex+1,current.pos+3);
				break;
			case 4:
				answer =  Coord(current.hex+1,current.pos+3);
				break;
			case 5:
				answer =  Coord(current.hex,current.pos-1);
				break;
		}
	} else {
		switch(segment) {
			case 0:
				answer =  Coord(current.hex-1,current.pos);
				break;
			case 1:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 2:
			case 3:
				answer =  Coord(current.hex+1,current.pos+3);
				break;
			case 4:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 5:
				answer =  Coord(current.hex-1,current.pos-6);
				break;
		}
	}
	return answer;
}
Coord moveNorthEast(Coord current) {
	if(current.hex == 0) {
		return Coord(1,1);
	}
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	Coord answer;
	if(shift == 0) {
		switch(segment) {
			case 0 :
			case 1 :
			case 2 :
				answer =  Coord(current.hex+1,current.pos+1);
				break;
			case 3:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 4:
				answer =  Coord(current.hex-1,current.pos-4);
				break;
			case 5:
				answer =  Coord(current.hex,current.pos+1);
				break;
		}
	} else {
		switch(segment) {
			case 0 :
			case 1 :
				answer =  Coord(current.hex+1,current.pos+1);
				break;
			case 2:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 3:
			case 4 :
				answer =  Coord(current.hex-1,current.pos-4);
				break;
			case 5:
				answer =  Coord(current.hex,current.pos+1);
				break;
		}
	}
	return answer;
}
Coord moveNorthWest(Coord current) {
	if(current.hex==0) {
		return  Coord(1,5);
	}
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	Coord answer;
	if(shift == 0) {
		switch(segment) {
			case 0:
				answer =  Coord(current.hex+1,6*current.hex+5);
				break;
			case 1:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 2:
				answer =  Coord(current.hex-1,current.pos-2);
				break;
			case 3:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 4:
				answer =  Coord(current.hex+1,current.pos+5);
				break;
			case 5:
				answer =  Coord(current.hex+1,current.pos+5);
				break;
		}
	} else {
		switch(segment) {
			case 0 :
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 1 :
			case 2:
				answer =  Coord(current.hex-1,current.pos-2);
				break;
			case 3 :
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 4 :
			case 5:
				answer =  Coord(current.hex+1,current.pos+5);
				break;
		}
	}
	return answer;
}
Coord moveSouthEast(Coord current) {
	if(current.hex == 0) {
		return  Coord(1,2);
	}
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	Coord answer;
	if(shift == 0) {
		switch(segment) {
			case 0:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 1 :
			case 2 :
			case 3:
				answer =  Coord(current.hex+1,current.pos+2);
				break;
			case 4:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 5:
				answer =  Coord(current.hex-1,current.pos-5);
				break;
		}
	} else {
		switch(segment) {
			case 0:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 1:
			case 2:
				answer =  Coord(current.hex+1,current.pos+2);
				break;
			case 3 :
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 4:
				answer =  Coord(current.hex-1,current.pos-5);
				break;
			case 5:
				answer =  Coord(current.hex-1,current.pos-5);
				break;
		}
	}
	return answer;
}
Coord moveSouthWest(Coord current) {
	if(current.hex == 0) {
		return  Coord(1,4);
	}
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	Coord answer;
	if(shift == 0) {
		switch(segment) {
			case 0:
				answer =  Coord(current.hex,current.pos+6*current.hex - 1);
				break;
			case 1:
				answer =  Coord(current.hex-1,current.pos-1);
				break;
			case 2:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 3 :
			case 4 :
			case 5:
				answer =  Coord(current.hex+1,current.pos+4);
				break;
		}
	} else {
		switch(segment) {
			case 0 :
			case 1 :
				answer =  Coord(current.hex-1,current.pos-1);
				break;
			case 2:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 3 :
			case 4 :
				answer =  Coord(current.hex+1,current.pos+4);
				break;
			case 5:
				answer =  Coord(current.hex,current.pos-1);
				break;
		}
	}
	return answer;
}
Coord moveNorth(Coord current) {
	if(current.hex == 0) {
		return  Coord(1,0);
	}
	int segment = current.pos/current.hex;
	int shift = current.pos%current.hex;
	Coord answer;
	if(shift == 0) {
		switch(segment) {
			case 0 :
			case 1:
				answer =  Coord(current.hex+1,current.pos);
				break;
			case 2:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 3:
				answer =  Coord(current.hex-1,current.pos-3);
				break;
			case 4:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 5:
				answer =  Coord(current.hex+1,current.pos+6);
				break;
		}
	} else {
		switch(segment) {
			case 0:
				answer =  Coord(current.hex+1,current.pos);
				break;
			case 1:
				answer =  Coord(current.hex,current.pos-1);
				break;
			case 2 :
			case 3:
				answer =  Coord(current.hex-1,current.pos-3);
				break;
			case 4:
				answer =  Coord(current.hex,current.pos+1);
				break;
			case 5:
				answer =  Coord(current.hex+1,current.pos+6);
				break;
		}
	}
	return answer;
}
Coord moveDirection(Coord current,int direction) {
	if(current == nullCoord) {
		return prevCoord;
	}
	Coord answer;
	switch(direction) { // Observe how direction is opposite to (direction+3)%6. This is used a lot.
		case 0:
			answer =  moveNorth(current);
			break;
		case 1:
			answer =  moveNorthEast(current);
			break;
		case 2:
			answer =  moveSouthEast(current);
			break;
		case 3:
			answer =  moveSouth(current);
			break;
		case 4:
			answer = moveSouthWest(current);
			break;
		case 5:
			answer = moveNorthWest(current);
			break;
	}
	if(answer.hex>board_size) {
		prevCoord = current;
		return Coord(-1,-1);
	}
	if(answer.hex == board_size and answer.pos%answer.hex == 0) {
		prevCoord = current;
		return Coord(-1,-1);
	}
	if(answer.hex == 0) {
		return answer;
	}
	answer.pos = answer.pos%(6*answer.hex);
	return answer;
}
bool checkRow(RowInfo row) {
	int mark = (row.player == 0)?1:-1;
	Coord temp = row.start;
	int countMoves = 0;
	while(!(temp == nullCoord)) {
		if(board[temp.hex][temp.pos] !=mark) {
			return false;
		}
		countMoves ++;
		if(temp == row.end) {
			break;
		}
		temp = moveDirection(temp,row.direction);
	}
	if(temp == nullCoord) {
		return false;
	}
	if(countMoves == sequenceLength) {
		return true;
	}
}
class RowStorage {
	private:
		vector< vector < RowInfo> > myRows;
		vector< vector<RowInfo> > otherRows;
	public:
		RowStorage() {
			vector<RowInfo> temp;
			this->myRows.push_back(temp);
			this->myRows.push_back(temp);
			this->otherRows.push_back(temp);
			this->otherRows.push_back(temp);
		}
		void insert(int choice,int real,RowInfo row) {
			if(choice == 0) {
				myRows[real].push_back(row);
			} else {
				otherRows[real].push_back(row);
			}
		}
		void sync() {
			myRows[0].clear();
			otherRows[0].clear();
			for(int i=0;i<myRows[1].size();i++){
				myRows[0].push_back(myRows[1][i]);
			}
			for(int i=0;i<otherRows[1].size();i++){
				otherRows[0].push_back(otherRows[1][i]);
			}
		}
		void remove(int choice,int real,RowInfo row) {
			if(choice == 0) {
				vector<RowInfo> :: iterator j;
				for(j=myRows[real].begin();j!=myRows[real].end();j++) {
					if(row == (*j)) {
						RowInfo row = (*j);
						myRows[real].erase(j);
						return;
					}
				}
			} else if(choice == 1) {
				vector<RowInfo> :: iterator j;
				for(j=otherRows[real].begin();j!=otherRows[real].end();j++) {
					if(row == (*j)) {
						RowInfo row = (*j);
						otherRows[real].erase(j);
						return;
					}
				}
			}
		}
		int updateRows(int choice,int real) {
			if(choice == 0){
				int countMoveser = 0;
				vector<RowInfo>::iterator j = myRows[real].begin();
				while(countMoveser < myRows[real].size()){
					if(checkRow(myRows[real][countMoveser]) == 0){
						RowInfo row = myRows[real][countMoveser];						
						myRows[real].erase(j+countMoveser);
						j = myRows[real].begin();
					}
					else countMoveser++;
				}
			return myRows[real].size();
			}
			else if(choice == 1){
				int countMoveser = 0;
				vector<RowInfo>::iterator j = otherRows[real].begin();
				while(countMoveser < otherRows[real].size()){
					if(checkRow(otherRows[real][countMoveser]) == 0){
						RowInfo row = otherRows[real][countMoveser];							
						otherRows[real].erase(j+countMoveser);
						j = otherRows[real].begin();
					}
					else countMoveser++;
				}
			return otherRows[real].size();	
			}	
		}
		void updateAll() {
			updateRows(0,1);
			updateRows(1,1);
			sync();
		}
		void getCopy(int choice,int real,vector<RowInfo> &rows) {
			if(choice == 0) {
				rows = myRows[real];
			} else rows = otherRows[real];
		}
		int size(int choice,int real) {
			if(choice == 0) {
				return myRows[real].size();
			} else return otherRows[real].size();
		}
		void printRows(int choice,int real) {
			if(choice == 0) {
				for(int i=0; i<myRows[real].size(); i++) {
					RowInfo row = myRows[real][i];
				}
			} else if(choice == 1) {
				for(int i=0; i<otherRows[real].size(); i++) {
					RowInfo row = otherRows[real][i];
				}
			}
		}
};
RowStorage rowStorage = RowStorage();
void getEdgeControl(float edge[2]);
typedef struct EvalInfo{
	EvalInfo(float blocks=0,float falseBlocks=0){
		this->blocks = blocks;
		this->falseBlocks = falseBlocks;
	}
	float blocks;
	float falseBlocks;
}EvalInfo;
class Evaluation{
	public:
		unordered_map<int,EvalInfo> info[2];
		int teamwork;
		Evaluation(){
			teamwork = 0;
		}
		int getKey(Coord ring){
			if(ring.hex == 0 and ring.pos == 0){
				return 0;
			}
			return 2+3*(ring.hex*(ring.hex-1))+ring.pos;
		}
		void changeBoard(int player,int hex,int pos,int increment){
			if(player == 1){
				return;
			}
			teamwork = teamwork+ increment*2*dummyBoard[player][hex][pos]+1;
			dummyBoard[player][hex][pos] = dummyBoard[player][hex][pos] + increment;
		}
		void addRing(Coord ring,int player){
			Coord start = ring;
			teamwork = teamwork-pow(dummyBoard[0][ring.hex][ring.pos],2);
			dummyBoard[0][ring.hex][ring.pos] = 0;
			int key = getKey(ring);
			int my = (player == 0)?2:-2;
			float blocks = 0; // Count of blocks done
			float falseBlocks = 0; // Count of false blocks
			bool valid[2]= {0}; 
			bool playerf[2] = {0}; // Which ring is being blocked
			vector<Coord> one; // Stores coords for dummy changes
			vector<Coord> two;
			for(int direction = 0;direction<3;direction++){
				int count = 0;
				one.clear();
				two.clear();
				valid[0] = 0;
				valid[1] = 0;
				start = ring;
				while(1){
					start = moveDirection(start,direction);
					if(start == nullCoord){
						break;
					}
					else if(board[start.hex][start.pos] == my){
						if(count < 4){
							info[player][getKey(start)].falseBlocks += (4-count);
							falseBlocks+=(4-count);
						}
						valid[0] = 1;
						playerf[0] = player;
						break;
					}
					else if(board[start.hex][start.pos] == -1*my){
						if(count < 4){
							info[(player+1)%2][getKey(start)].blocks += (4-count);
							blocks+=(4-count);
						}
						valid[0] = 1;
						playerf[0]= (player+1)%2;
						break;
					}
					else{
						changeBoard(player,start.hex,start.pos,1);
						one.push_back(start);
					}
					count++;
				}
				count = 0;
				start = ring;
				while(1){
					start = moveDirection(start,(direction+3));
					if(start == nullCoord){
						break;
					}
					else if(board[start.hex][start.pos] == my){
						if(count < 4){
							info[player][getKey(start)].falseBlocks+= (4-count);
							falseBlocks+= (4-count);
						}
						valid[1] = 1;
						playerf[1] = player;
						break;
					}
					else if(board[start.hex][start.pos] == -1*my){
						if(count<4){
							info[(player+1)%2][getKey(start)].blocks+= 4-count;
							blocks+= 4-count;
						}
						valid[1] = 1;
						playerf[1] = (player+1)%2;
						break;
					}
					else{
						changeBoard(player,start.hex,start.pos,1);
						two.push_back(start);
					}
					count++;
				}
				if(valid[0] == 1){
					for(int i=0;i<two.size();i++){
						changeBoard(playerf[0],two[i].hex,two[i].pos,-1); // Cant access these rings
					}
				}
				if(valid[1] == 1){
					for(int i=0;i<one.size();i++){
						changeBoard(playerf[1],one[i].hex,one[i].pos,-1); // Cant access these rings
					}
				}
			}
			info[player][getKey(ring)] = EvalInfo(blocks,falseBlocks);
		}
		void deleteRing(Coord ring,int player){
			Coord start = ring;
			int my = (player == 0)?2:-2;
			Coord found[2];
			bool valid[2]= {0};
			bool playerf[2] = {0};
			vector<Coord> one;
			vector<Coord> two;
			for(int direction = 0;direction<3;direction++){
				one.clear();
				two.clear();
				valid[0] = 0;
				valid[1] = 0;
				start = ring;
				int count = 0;
				while(1){
					start = moveDirection(start,direction);
					if(start == nullCoord){
						break;
					}
					else if(board[start.hex][start.pos] == my){
						if(count<4){
							info[player][getKey(start)].falseBlocks-= (4-count);
						}
						valid[0] = 1;
						playerf[0] = player;
						break;
					}
					else if(board[start.hex][start.pos] == -1*my){
						if(count<4){
							info[(player+1)%2][getKey(start)].blocks-=(4-count); // Decrease blocks
						}
						valid[0] = 1;
						playerf[0]= (player+1)%2;
						break;
					}
					else{
						changeBoard(player,start.hex,start.pos,-1);
						one.push_back(start);
					}
					count++;
				}
				start = ring;
				count = 0;
				while(1){
					start = moveDirection(start,(direction+3));
					if(start == nullCoord){
						break;
					}
					else if(board[start.hex][start.pos] == my){
						if(count<4){
							info[player][getKey(start)].falseBlocks-=(4-count);
						}
						valid[1] = 1;
						playerf[1] = player;
						break;
					}
					else if(board[start.hex][start.pos] == -1*my){
						if(count<4){
							info[(player+1)%2][getKey(start)].blocks-=(4-count);
						}
						valid[1] = 1;
						playerf[1] = (player+1)%2;
						break;
					}
					else{
						changeBoard(player,start.hex,start.pos,-1);
						two.push_back(start);
					}
					count++;
				}
				if(valid[0] == 1){
					for(int i=0;i<two.size();i++){
						changeBoard(playerf[0],two[i].hex,two[i].pos,1);
					}
					changeBoard(playerf[0],ring.hex,ring.pos,1);
				}
				if(valid[1] == 1){
					for(int i=0;i<one.size();i++){
						changeBoard(playerf[1],one[i].hex,one[i].pos,1);
					}
					changeBoard(playerf[1],ring.hex,ring.pos,1);
				}
				
			}
			info[player].erase(getKey(ring));
		}
		void evaluate(float* answer){
			answer[0] = teamwork; 
			float blocks[2] = {0};
			float unblocked[2] = {0};
			for(int i=0;i<2;i++){
				unordered_map<int,EvalInfo> :: iterator it = info[i].begin();
				while(it!=info[i].end()){
					EvalInfo eval = it->second;
					blocks[i] = blocks[i] + pow(eval.blocks,2);
					if((it->second).blocks == 0){
						unblocked[i]++;;
					}
					it++;
				}
			}
			unblocked[0] = pow(2,unblocked[0]);
			answer[1] = blocks[0]; // Blocking other rings
			answer[2] = unblocked[0]; // Keeping my rings unblocked
		}
};
Evaluation evaluateRings = Evaluation();
void placeRing(int player,int hex,int pos) { // Given a hex-pos pair , place the ring on the board and update player
	if(player == 0) {
		board[hex][pos] = 2;
	} else if(player == 1) {
		board[hex][pos] = -2;
	}
	players[player].placeRing(hex,pos);
	if(ringsPlaced == 0){
		evaluateRings.addRing(Coord(hex,pos),player);
	}
}
void removeRing(int player,int hex,int pos) { // Remove the ring from the board
	board[hex][pos] = 0;
	players[player].removeRing(hex,pos);
	if(ringsPlaced == 0){
		evaluateRings.deleteRing(Coord(hex,pos),player);
	}
	return;
}
int findLine(int hex1,int pos1,int hex2,int pos2) { // Could be optimized. Finds the direction to reach from one place to other
	Coord start = Coord(hex1,pos1);
	Coord end = Coord(hex2,pos2);
	Coord north = moveDirection(start,0);
	Coord northEast = moveDirection(start,1);
	Coord southEast = moveDirection(start,2);
	Coord south = moveDirection(start,3);
	Coord southWest = moveDirection(start,4);
	Coord northWest = moveDirection(start,5);
	bool stop = 0;
	int index;
	while(stop == 0) {
		if(!(north==nullCoord)) {
			if(north.hex == hex2 and north.pos == pos2) {
				stop = 1;
				index = 0;
				break;
			}
			north = moveDirection(north,0);
		}
		if(!(northEast==nullCoord)) {
			if(northEast.hex == hex2 and northEast.pos == pos2) {
				stop = 1;
				index = 1;
				break;
			}
			northEast = moveDirection(northEast,1);
		}
		if(!(southEast==nullCoord)) {
			if(southEast.hex == hex2 and southEast.pos == pos2) {
				stop = 1;
				index = 2;
				break;
			}
			southEast = moveDirection(southEast,2);
		}
		if(!(south==nullCoord)) {
			if(south.hex == hex2 and south.pos == pos2) {
				stop = 1;
				index = 3;
				break;
			}
			south = moveDirection(south,3);
		}
		if(!(southWest==nullCoord)) {
			if(southWest.hex == hex2 and southWest.pos == pos2) {
				stop = 1;
				index = 4;
				break;
			}
			southWest = moveDirection(southWest,4);
		}
		if(!(northWest==nullCoord)) {
			if(northWest.hex == hex2 and northWest.pos == pos2) {
				stop = 1;
				index = 5;
				break;
			}
			northWest = moveDirection(northWest,5);
		}
	}
	return index;
}
void findRow(int direction,Coord temp,int real) {
	int marker = board[temp.hex][temp.pos]; // Get marker
	int player = (marker == 1)?0:1; // Get player based on marker(whose rows would have been formed)
	int directions[4]; //List of directions to check
	int pos = 0;
	for(int i=0; i<6; i++) {
		if(i == direction or i == (direction + 3)%6) { // dont move along the move direction as it will be repeated
			continue;
		}
		directions[pos] = i;
		pos++;
	}
	int answer =0;
	for(int i=0; i<2; i++) {
		int tempDirection1 = directions[i]; // first direction
		int tempDirection2 = directions[i+2]; //opposite
		Coord rowStart = temp;
		Coord rowEnd = temp;
		int countMoves1 = 0;
		int countMoves2 = 0;
		while(board[rowStart.hex][rowStart.pos] == marker) {
			rowStart = moveDirection(rowStart,tempDirection1);
			countMoves1++;
			if(rowStart == nullCoord) {
				break;
			}
		}
		rowStart = moveDirection(rowStart,tempDirection2); // Revert back rowStart
		while(board[rowEnd.hex][rowEnd.pos] == marker) {
			rowEnd = moveDirection(rowEnd,tempDirection2);
			countMoves2++;
			if(rowEnd == nullCoord) {
				break;
			}
		}
		rowEnd = moveDirection(rowEnd,tempDirection1); // Revert back rowEnd
		if(countMoves1 == sequenceLength) {
			rowStorage.insert(player,real,RowInfo(rowStart,temp,tempDirection2,player));
		}
		if(countMoves2 == sequenceLength) {
			rowStorage.insert(player,real,RowInfo(rowEnd,temp,tempDirection1,player));
		}
		if(countMoves1+countMoves2>sequenceLength and countMoves1<sequenceLength and countMoves2<sequenceLength) { // Row can be found
			Coord newTemp = temp;
			for(int j=0; j<sequenceLength-countMoves1; j++) {
				newTemp = moveDirection(newTemp,tempDirection2);
			}
			rowStorage.insert(player,real,RowInfo(rowStart,newTemp,tempDirection2,player));
			newTemp = temp;
			for(int j=0; j<sequenceLength-countMoves2; j++) {
				newTemp = moveDirection(newTemp,tempDirection1);
			}
			if(countMoves1+countMoves2 == sequenceLength+1) {
				return;
			}
			rowStorage.insert(player,real,RowInfo(rowEnd,newTemp,tempDirection1,player));
		}
	}
	return;
}
void findRows(int direction,int hex1,int pos1,int hex2,int pos2,int real) {
	Coord temp = Coord(hex1,pos1);
	while(temp.hex!=hex2 or temp.pos!=pos2) {
		if(board[temp.hex][temp.pos] !=0) {
			findRow(direction,temp,real); // Do it for the 4 directions not along the movement direc
		}
		temp = moveDirection(temp,direction);
	}
	temp = Coord(hex1,pos1);
	int marker = board[hex1][pos1];
	int player = (marker == 1)?0:1;
	Coord rowStart = temp;
	Coord rowEnd = temp;
	int countMoves1 = 0;
	while(board[rowStart.hex][rowStart.pos] == marker) {
		rowStart = moveDirection(rowStart,direction);
		countMoves1++;
		if(rowStart == nullCoord) {
			break;
		}
	}
	rowStart = moveDirection(rowStart,(direction+3)%6); // Take rowStart one step back
	int countMoves2 = 0;
	while(board[rowEnd.hex][rowEnd.pos] == marker) {
		rowEnd = moveDirection(rowEnd,(direction+3)%6);
		countMoves2++;
		if(rowEnd == nullCoord) {
			break;
		}
	}
	rowEnd = moveDirection(rowEnd,direction);
	if(countMoves1 == sequenceLength) {
		rowStorage.insert(player,real,RowInfo(rowStart,temp,(direction+3)%6,player));
	}
	if(countMoves2 == sequenceLength) {
		rowStorage.insert(player,real,RowInfo(rowEnd,temp,direction,player));

	} else if(countMoves1 +countMoves2>sequenceLength and countMoves1<sequenceLength and countMoves2<sequenceLength) {
		for(int i=0; i<sequenceLength-countMoves1; i++) {
			temp = moveDirection(temp,(direction+3)%6);
		}
		rowStorage.insert(player,real,RowInfo(rowStart,temp,(direction+3)%6,player));
		temp = Coord(hex1,pos1);
		for(int j=0; j<sequenceLength-countMoves2; j++) {
			temp = moveDirection(temp,direction);
		}
		if(countMoves1+countMoves2 == sequenceLength+1) {
			return;
		}
		rowStorage.insert(player,real,RowInfo(rowEnd,temp,direction,player));
	}
	return;
}
int moveRing(int player,int hex1,int pos1,int hex2,int pos2,int direction = -1) {
	if(hex1 == hex2 and pos1 == pos2) {
		return -1;
	}
	if(direction == -1) {
		direction = findLine(hex1,pos1,hex2,pos2);
	}
	int marker = (player == 0)?1:-1;
	int antiMarker = (marker == 1)?-1:1;
	Coord temp = Coord(hex1,pos1);
	while(temp.hex!=hex2 or temp.pos!=pos2) {
		if(board[temp.hex][temp.pos] == marker) { // Flip markers
			board[temp.hex][temp.pos] = antiMarker;
		} else if(board[temp.hex][temp.pos] == antiMarker) {
			board[temp.hex][temp.pos] = marker;
		}
		temp = moveDirection(temp,direction);
	}
	board[hex2][pos2] = (player == 0)?2:-2; // Place ring
	if(player == 0) {
		removeRing(0,hex1,pos1); // Update player database
		placeRing(0,hex2,pos2);
		board[hex1][pos1] = 1;

	} else {
		removeRing(1,hex1,pos1);
		placeRing(1,hex2,pos2);
		board[hex1][pos1] = -1;
	}
	return direction;
}
bool removeRow(int player,int hex1,int pos1,int hex2,int pos2,int direction = -1) { // Outputs true when the row is removed
	if(direction == -1) {
		direction = findLine(hex1,pos1,hex2,pos2);
	}
	if(checkRow(RowInfo(Coord(hex1,pos1),Coord(hex2,pos2),direction,player)) == 0) {
		return false;
	}
	Coord start = Coord(hex1,pos1);
    Coord end = Coord(hex2,pos2);
    Coord temp = start;
	while(!(temp==end)){
		board[temp.hex][temp.pos] = 0;
		temp = moveDirection(temp,direction);
	}
	board[end.hex][end.pos] = 0;
	return true;
}
void parseInput(string Moves,vector<Move> &moves) {
	vector<string> result;
	istringstream iss(Moves);
	for(string s; iss >> s;) {
		result.push_back(s);
	}
	vector<string>::iterator i1 = result.begin();
	while(i1 != result.end()) {
		string s1 = *i1;
		i1++;
		stringstream h(*i1);
		int r1;
		h >> r1;
		i1++;
		stringstream p(*i1);
		int r2 ;
		p >> r2;
		i1++;
		moves.push_back(Move(s1,r1,r2));
	}
}
void executeMoves(string Moves) { // Execute moves obtained from other player (Only use)
	vector<Move> moveSequence;
	parseInput(Moves,moveSequence);
	vector<Move>:: iterator move;
	for(move = moveSequence.begin(); move<moveSequence.end(); move++) {
		string moveType = (*move).moveType;
		vector<RowInfo> rows;
		int hex,pos,rowStart1,rowStart2,rowEnd1,rowEnd2;
		if(moveType == "P") {
			placeRing(1,(*move).hex,(*move).pos);
		}
		if(moveType =="S") {
			hex = (*move).hex;
			pos = (*move).pos;
		}
		if(moveType == "M") {
			int direction = moveRing(1,hex,pos,(*move).hex,(*move).pos);
			findRows(direction,hex,pos,(*move).hex,(*move).pos,1);
			rowStorage.sync();
		}
		if(moveType == "RS") {
			rowStart1 = (*move).hex;
			rowStart2 = (*move).pos;
		}
		if(moveType == "RE") {
			rowEnd1 = (*move).hex;
			rowEnd2 = (*move).pos;
			removeRow(1,rowStart1,rowStart2,rowEnd1,rowEnd2);
			rowStorage.updateAll();
		}
		if(moveType == "X") {
			removeRing(1,(*move).hex,(*move).pos);
		}
	}
}
void getAvailableMoves(int player,int hex,int pos,vector<MoveInfo> &listOfMoves) { // Get available moves from a start position
	for(int i=0; i<6; i++) {
		Coord temp = moveDirection(Coord(hex,pos),i);
		bool mark = 0;
		while(!(temp == nullCoord)) { // If no ring encountMovesered
			if(board[temp.hex][temp.pos] == 2 or board[temp.hex][temp.pos] == -2) {
				break;
			}
			if(mark == 0) { // if no mark encountMovesered yet
				if(board[temp.hex][temp.pos]==1 or board[temp.hex][temp.pos] == -1) { // If encountMovesered
					mark = 1;
				} else {
				listOfMoves.push_back(MoveInfo(Coord(hex,pos),temp,i));
				}
			}
			if(mark == 1 and board[temp.hex][temp.pos] == 0) { // If jumped over the marker
				listOfMoves.push_back(MoveInfo(Coord(hex,pos),temp,i));
				break;
			}
			
			temp = moveDirection(temp,i);
		}
	}
}
void selectRings(int number,int removeRings,vector<int> &selections) { // From a player's ring database, get a ring selection to remove
	bool countMoves = 0;
	for(int i=0; i<number and 1<=removeRings; i++) {
		for(int j=i+1; j<number and 2<=removeRings; j++) {
			for(int k=j+1; k<number and 3<=removeRings; k++) {
				for(int l=k+1; l<number and 4<=removeRings; l++) {
					for(int m = l+1; m<number and 5<=removeRings; m++) {
						for(int o = m+1; o<number and 6<=removeRings;o++){
							if(removeRings == 6){
								selections.push_back(i);
								selections.push_back(j);
								selections.push_back(k);
								selections.push_back(l);
								selections.push_back(m);
								selections.push_back(o);
							}
						}
						if(removeRings == 5) {
							selections.push_back(i);
							selections.push_back(j);
							selections.push_back(k);
							selections.push_back(l);
							selections.push_back(m);
						}
					}
					if(removeRings == 4) {
						selections.push_back(i);
						selections.push_back(j);
						selections.push_back(k);
						selections.push_back(l);
					}
				}
				if(removeRings == 3) {
					selections.push_back(i);
					selections.push_back(j);
					selections.push_back(k);
				}
			}
			if(removeRings == 2) {
				selections.push_back(i);
				selections.push_back(j);
			}
		}
		if(removeRings == 1) {
			selections.push_back(i);
		}
	}
}
void executePly(Ply ply,int real,vector<int> &removal) { // Given a ply, execute it on the board
	if(ply.flag == 0) { // Movement ply
		int player = (board[ply.moveInfo.start.hex][ply.moveInfo.start.pos] == 2)?0:1;
		moveRing(player,ply.moveInfo.start.hex,ply.moveInfo.start.pos,ply.moveInfo.end.hex,ply.moveInfo.end.pos,ply.moveInfo.direction);
		findRows(ply.moveInfo.direction,ply.moveInfo.start.hex,ply.moveInfo.start.pos,ply.moveInfo.end.hex,ply.moveInfo.end.pos,real);
		if(real == 1){
			rowStorage.sync();
		}
	} 
	else if (ply.flag == -1) { // Ring removal ply
		bool removed;
		for(int m=0;m<ply.rowRemovals.size();m++){
			RowInfo row = ply.rowRemovals[m].rowRemoval;
		}
		for(int m=0; m<ply.rowRemovals.size(); m++) {
			RowInfo rowInfo = ply.rowRemovals[m].rowRemoval;
			removed = removeRow(rowInfo.player,rowInfo.start.hex,rowInfo.start.pos,rowInfo.end.hex,rowInfo.end.pos,rowInfo.direction);
		if(removed==true){
			Coord ringRemoval = ply.rowRemovals[m].ringRemoval;
			removeRing(rowInfo.player,ringRemoval.hex,ringRemoval.pos);
			removal.push_back(m);
			}
		}
		rowStorage.updateRows(0,real);
		rowStorage.updateRows(1,real);
		if(real == 1){
			rowStorage.sync();
		}
	} 
	else {
		placeRing(ply.ringPlacement.player,ply.ringPlacement.ring.hex,ply.ringPlacement.ring.pos); // ring placement
	}
}
void reversePly(Ply &ply,int real,vector<int> &removal) { // Reverse ply
	if(ply.flag == -1) {
		int reverse = removal.size()-1;
		for(int i=ply.rowRemovals.size()-1; i>=0; i-- and reverse!=-1) { // Reverse through the removal vector
			if(i != removal[reverse]) {
				continue;
			}
			RowInfo rowInfo = ply.rowRemovals[i].rowRemoval;
			//assert(checkRow(rowInfo) == 0);		//Check row is not there;	
			Coord ringRemoval = ply.rowRemovals[i].ringRemoval;
			placeRing(rowInfo.player,ringRemoval.hex,ringRemoval.pos); // Place the ring back
			Coord temp = rowInfo.end;
			int mark = (rowInfo.player == 0)?1:-1;
			while(!(temp==rowInfo.start)) {
				board[temp.hex][temp.pos] = mark;
				temp = moveDirection(temp,(rowInfo.direction+3)%6); // Add the row back;
			}
			board[temp.hex][temp.pos] = mark;
			rowStorage.insert(rowInfo.player,real,rowInfo);
			reverse--;
		}
		reverse = removal.size()-1;
		for(int i = ply.rowRemovals.size()-1;i>=0;i--){
			RowInfo row = ply.rowRemovals[i].rowRemoval;
			//assert(checkRow(ply.rowRemovals[i].rowRemoval) == 1);
		}
		if(real == 1){
			rowStorage.sync();
		}
		return;
	} 
	else if(ply.flag == 0) { // Reverse ring movement
		MoveInfo move = ply.moveInfo;
		int player = (board[move.end.hex][move.end.pos] == 2)?0:1;
		board[move.start.hex][move.start.pos] = 0;
		moveRing(player,move.end.hex,move.end.pos,move.start.hex,move.start.pos,(move.direction+3)%6);
		rowStorage.updateRows(0,real);
		rowStorage.updateRows(1,real);
		board[move.end.hex][move.end.pos] = 0;
		if(real == 1){
			rowStorage.sync();
		}
		return;
	}
	else {
		int player = (board[ply.ringPlacement.ring.hex][ply.ringPlacement.ring.pos] == 2)?0:1;
		removeRing(player,ply.ringPlacement.ring.hex,ply.ringPlacement.ring.pos);
	}
}
void permute(vector<vector<Removal>> &rowRemoval,vector<RowInfo> &rows,bool player,int removeRings) {
	vector<int> selections;
	vector<vector<int> > togetherSelections1; // permutations of selections
	selectRings(players[player].ringsOnBoard,removeRings,selections);
	for(int i=0; i<selections.size(); i=i+removeRings) { // move through them
		vector<int> temp;
		for(int j=i; j<i+removeRings; j++) {
			temp.push_back(selections[j]);
		}
		togetherSelections1.push_back(temp); //push it in the selections list
	}
	selections.clear();
	selectRings(rows.size(),removeRings,selections);
	vector<vector<int> > togetherSelections2;
	for(int i=0; i<selections.size(); i=i+removeRings) { // move through them
		vector<int> temp;
		for(int j=i; j<i+removeRings; j++) {
			temp.push_back(selections[j]);
		}
		togetherSelections2.push_back(temp); //push it in the selections list
	}
	for(int i=0; i<togetherSelections1.size(); i++) {
		for(int j=0; j<togetherSelections2.size(); j++) {
			vector<int> temp1 = togetherSelections1[i]; // Ring selections
			vector<int> temp2 = togetherSelections2[j]; // Row selections
			sort(temp1.begin(),temp1.end());			
			bool end = 1;
			bool end1 = 1;
			vector<Removal> temp;			
			while(end == 1){
				sort(temp2.begin(),temp2.end());	
				while(end1==1){
					for(int k = 0;k<removeRings;k++){
						RowInfo row = rows[temp2[k]];
						Coord ring = players[player].positions[temp1[k]];
						temp.push_back(Removal(row,ring));
					}
					rowRemoval.push_back(temp);
					temp.clear();
					end1 = next_permutation(temp2.begin(),temp2.end());
				}
				end = next_permutation(temp1.begin(),temp1.end());
				end1 = 1;
			}
		}
	}
	return;
}
void compare(float value,float &alpha,float &beta,bool turn) { // Utility function for updating alpha beta
	if(turn == 0) { //  MaxNode
		alpha = (value>=alpha)?value:alpha;
	} else {
		beta = (value<=beta)?value:beta;
	}
}
bool change(float& nodeValue,float newValue,int turn) { // Utility function
	if(turn == 0) { // MaxNode
		if(newValue >= nodeValue) {
			nodeValue = newValue;
			return 1;
		}
		return 0;
	} else if(turn == 1) { // MinNode
		if(newValue <=nodeValue) {
			nodeValue = newValue;
			return 1;
		}
		return 0;
	}
}
bool available(Coord ring) {
	for(int i=0; i<players[0].ringsOnBoard; i++) {
		if(ring == players[0].positions[i]) {
			return false;
		}
	}
	for(int i=0; i<players[1].ringsOnBoard; i++) {
		if(ring == players[1].positions[i]) {
			return false;
		}
	}
	if(ring.hex == board_size and ring.pos%ring.hex == 0) {
		return false;
	}
	return true;
}
void initiateBoard() {
	board = new int*[board_size+1];
	board[0] = new int[1];
	board[0][0] = 0;
	for(int hex=1; hex<=board_size; hex++){
		board[hex] = new int[6*hex];
		for(int pos=0; pos<6*hex; pos++){
			board[hex][pos] = 0;
		}
	}
	for(int pos = 0;pos<2;pos++){
		dummyBoard[pos] = new int*[board_size+1];
	    for(int hex = 1; hex<= board_size; hex++){
			dummyBoard[pos][hex] = new int[6*hex];
		}
		dummyBoard[pos][0] = new int[1];
		dummyBoard[pos][0][0] = 0;
		for(int i = 1; i <= board_size; i++){
			for(int j =0 ; j <6*i; j++){
		    	dummyBoard[pos][i][j] = 0;
			}
	    }
	}
}
void resetDummy(){
	for(int pos = 0;pos<2;pos++){
		dummyBoard[pos][0][0] = 0;
		for(int i=0;i<=board_size;i++){
			for(int j=0;j<6*i;j++){
				dummyBoard[pos][i][j] = 0;
			}
		}
	}
}
// End of Game mechanics


// Algorithm Part
void evaluateTotal(float answer[11],bool first = 0);
float getScore(float* init,float *final,int phase){
	if(phase == 1){
		float answer = 0;
		for(int i=0;i<5;i++){
			answer = answer + weights[currSet][i]*(final[i]-init[i]);
		}
		return answer;
	}
	else if(phase == 2){
		float answer = 0;
		for(int i=0;i<11;i++){
			answer = answer + boardWeights[i]*(final[i]-init[i]);
		}
		return answer;
	}
}
int visited =0;
float getLengthDirection(Coord temp,int direction){
	int answer[2];
	int marker = board[temp.hex][temp.pos]; // Get marker
	int directions[4]; //List of directions to check
	int pos = 0;
	for(int i=0; i<6; i++) {
		if(i == direction or i == (direction + 3)%6) { // dont move along the move direction as it will be repeated
			continue;
		}
		directions[pos] = i;
		pos++;
	}
	for(int i=0; i<2; i++) {
		int tempDirection1 = directions[i]; // first direction
		int tempDirection2 = directions[i+2]; //opposite
		Coord rowStart = temp;
		Coord rowEnd = temp;
		int countMoves1 = 0;
		int countMoves2 = 0;
		while(board[rowStart.hex][rowStart.pos] == marker) {
			rowStart = moveDirection(rowStart,tempDirection1);
			countMoves1++;
			if(rowStart == nullCoord) {
				break;
			}
		}
		while(board[rowEnd.hex][rowEnd.pos] == marker) {
			rowEnd = moveDirection(rowEnd,tempDirection2);
			countMoves2++;
			if(rowEnd == nullCoord) {
				break;
			}
		}
		answer[i] = countMoves1+countMoves2-1;
	}
	if(answer[0] == answer[1]){
		return 1.2*answer[0];
	}
	else if(answer[0]>answer[1]){
		return answer[0];
	}
	else return answer[1];
}
void evaluateMove(Ply& ply,float answer[3]){
	float maxLength[2] = {0};
	int correct = 0;
	Coord temp = ply.moveInfo.start;
	Coord end = ply.moveInfo.end;
	int direction = ply.moveInfo.direction;
	while(!(temp==end)){
		if(board[temp.hex][temp.pos] == 1){
			correct++;
		}
		else if(board[temp.hex][temp.pos] == -1){
			correct--;
		}
		else if(board[temp.hex][temp.pos] == 0){
			temp = moveDirection(temp,direction);
			continue;
		}
		int a = getLengthDirection(temp,direction);
		if(a > maxLength[abs((board[temp.hex][temp.pos]-1)/2)]){
			maxLength[abs((board[temp.hex][temp.pos]-1)/2)] = a;
		}
		temp = moveDirection(temp,direction);
	}
	Coord rowStart = ply.moveInfo.start;
	Coord rowEnd = rowStart;
	int marker = (board[rowStart.hex][rowStart.pos] == 1)?1:-1;
	int countMoves1 = 0;
	while(board[rowStart.hex][rowStart.pos] == marker) {
		rowStart = moveDirection(rowStart,direction);
		countMoves1++;
		if(rowStart == nullCoord) {
			break;
		}
	}
	int countMoves2 = 0;
	while(board[rowEnd.hex][rowEnd.pos] == marker) {
		rowEnd = moveDirection(rowEnd,(direction+3)%6);
		countMoves2++;
		if(rowEnd == nullCoord) {
			break;
		}
	}
	if(countMoves1+countMoves2-1 == maxLength[abs((marker-1)/2)]){
		maxLength[abs((marker-1)/2)]+=0.2*(countMoves1+countMoves2-1);
	}
	if(countMoves1+countMoves2-1 > maxLength[abs((marker-1)/2)]){
		maxLength[abs((marker-1)/2)] = countMoves1+countMoves2-1;
	}
	answer[0] = pow(3,maxLength[0]);
	answer[1] = pow(3,maxLength[1]);
	answer[2] = pow(3,correct);
}
void getEdgeControl(float edge[2]){
	if(ringsPlaced == 0){
		if(player_id == 1){ // Player 1
			if(players[0].ringsOnBoard == 2){
				Coord& ring = players[0].positions.back();
				if(ring.hex == board_size-1){ // Second and Fifth ring at outer edges
					edge[0] += 100;
				}
				else if(ring.hex == board_size){
					if(board_size == 6 and sequenceLength == 5){
						edge[0] += 120;
						if((ring.pos+1)%6 == 0 or (ring.pos-1)%6 == 0){
								edge[0]+=50;
						}
					}
					else edge[0] += 60;
				}
			}
			else if(players[0].ringsOnBoard == numberOfRings){
				Coord& otherEdgeRing = players[0].positions[1]; // Second ring
				Coord& ring = players[0].positions[0];
				if(ring.hex == board_size-1){
					if(otherEdgeRing.hex !=0){
						int Edge = otherEdgeRing.pos/otherEdgeRing.hex;
						int myEdge = ring.pos/ring.hex;
						if(myEdge == Edge){
							edge[0] -= 50;
						}
					}
					
					edge[0]+= 100;
				}
				else if(ring.hex == board_size){
					if(otherEdgeRing.hex != 0){
						int Edge = otherEdgeRing.pos/otherEdgeRing.hex;
						int myEdge = ring.pos/ring.hex;
						if(myEdge == Edge){
							edge[0] -= 50;
						}
					}
					
					if(board_size == 6 and sequenceLength == 5){
						edge[0] += 100;
						if((ring.pos+1)%6 == 0 or (ring.pos-1)%6 == 0){
								edge[0]+=50;
						}
					}
					else edge[0] += 60;
				}
			}
		}
		else if(player_id == 2){
			if(players[0].ringsOnBoard == numberOfRings){
				Coord& ring = players[0].positions.back();
				if(ring.hex == board_size-1){ // Fifth ring at outer edges
					edge[0] += 100;
				}
				else if(ring.hex == board_size){
					if(board_size == 6 and sequenceLength == 5){
							edge[0] += 100;
							if((ring.pos+1)%6 == 0 or (ring.pos-1)%6 == 0){
								edge[0]+=50;
							}
					}
					else edge[0] += 60;
				}
			}
		}
		for(int i=0;i<players[1].positions.size();i++){
			Coord ring = players[1].positions[i];
			int key = evaluateRings.getKey(ring);
			EvalInfo eval = evaluateRings.info[1][key];
			if( eval.blocks == 0 and(ring.hex == board_size or ring.hex == board_size-1)){
				edge[1]+= 25;
			}
		}
	}
	else{
		int hex = board_size;
		int Edge = 0;
		int maxRows[2] = {0};
		for(int Edge=0;Edge<5;Edge++){
			int currMarker = 1;
			int markersInLine = 0;
			for(int i=1;i<board_size;i++){
				int pos = Edge*hex+i;
				if(board[hex][pos] == 1){
					if(currMarker == -1){
						if(markersInLine > maxRows[1]){
							maxRows[1] = markersInLine;
						}
						markersInLine = 1;
					}
					else if(currMarker == 0){
						markersInLine = 1;
					}
					else if(currMarker == 1){
						markersInLine++;
					}
					currMarker = 1;
				}
				else if(board[hex][pos] == -1){
                    if(currMarker == 1){
						if(markersInLine > maxRows[0]){
							maxRows[0] = markersInLine;
						}
						markersInLine = 1;
					}
					else if(currMarker == 0){
						markersInLine = 1;
					}
					else if(currMarker == -1){
						markersInLine++;
					}
					currMarker = 1;
				}
				else{
					if(currMarker == 1){
						if(markersInLine > maxRows[0]){
							maxRows[0] = markersInLine;
						}
						markersInLine = 0;
					}
					else if(currMarker == -1){
						if(markersInLine > maxRows[1]){
							maxRows[1] = markersInLine;
						}
						markersInLine = 0;
					}
					currMarker = 0;
				}
			}
		}
		hex = board_size-1;
		for(int Edge=0;Edge<5;Edge++){
			int currMarker = 1;
			int markersInLine = 0;
			for(int i=0;i<board_size;i++){
				int pos = Edge*hex+i;
				if(board[hex][pos] == 1){
					if(currMarker == -1){
						if(markersInLine > maxRows[1]){
							maxRows[1] = markersInLine;
						}
						markersInLine = 1;
					}
					else if(currMarker == 0){
						markersInLine = 1;
					}
					else if(currMarker == 1){
						markersInLine++;
					}
					currMarker = 1;
				}
				else if(board[hex][pos] == -1){
                    if(currMarker == 1){
						if(markersInLine > maxRows[0]){
							maxRows[0] = markersInLine;
						}
						markersInLine = 1;
					}
					else if(currMarker == 0){
						markersInLine = 1;
					}
					else if(currMarker == -1){
						markersInLine++;
					}
					currMarker = 1;
				}
				else{
					if(currMarker == 1){
						if(markersInLine > maxRows[0]){
							maxRows[0] = markersInLine;
						}
						markersInLine = 0;
					}
					else if(currMarker == -1){
						if(markersInLine > maxRows[1]){
							maxRows[1] = markersInLine;
						}
						markersInLine = 0;
					}
					currMarker = 0;
				}
			}
		}
		edge[0] = pow(3,maxRows[0]);
		edge[1] = pow(3,maxRows[1]);
	}
}
int countAhead(Coord ring,int direction,int player){ // Check if a row behind the ring is blocked (Useful for blocking edge row makers)
	int my = (player == 0)?1:-1;
	Coord temp = ring;
	int count1 = 0;
	while(1){
		ring = moveDirection(ring,direction);
		if(ring == nullCoord){
			break;
		}
		else if(board[ring.hex][ring.pos] == 2 or board[ring.hex][ring.pos] == -2){
			break;
		}
		else if(board[ring.hex][ring.pos] == -1*my){
			count1++;
		}
		else if(board[ring.hex][ring.pos] == my){
			break;
		}
		else if(board[ring.hex][ring.pos] == 0){
			break; 
		}
	}
	ring = temp;
	int count2 = 0;
	while(1){
		ring = moveDirection(ring,(direction+3)%6);
		if(ring == nullCoord){
			break;
		}
		else if(board[ring.hex][ring.pos] == 2 or board[ring.hex][ring.pos] == -2){
			break;
		}
		else if(board[ring.hex][ring.pos] == 1*my){
			count2++;
		}
		else if(board[ring.hex][ring.pos] == -1*my){
			break;
		}
		else if(board[ring.hex][ring.pos] == 0){
			break; 
		}
	}
	return 3*(count1+count2);
}
void evaluateBoard(float* answer){
	resetDummy();
	int teamwork[2] = {0};
	float edgeControl[2] = {0};
	getEdgeControl(edgeControl);
    int blocksDone[2] = {0};
    vector<float> scores[2];
	for(int player=0;player<2;player++){ // Player loop
		for(int pos=0;pos<players[player].positions.size();pos++){ // Rings loop
			int my = (player == 0)?1:-1;
			Coord ring = players[player].positions[pos];
			bool valid[6] = {0}; // Can move in this direction
			int rowsFound[6] = {0}; // Rows of markers found
			int thisScore = 0;
			for(int direction=0;direction<6;direction++){ // Direction loop
				vector<Coord> markers; // Store markers
				bool rowBlocks = 1;
				bool marked = 0;
				bool validNow = 0;
				int count = 0;
				int currRow = 4;
				int rowFound = 0;
				int maxRowsFound = 0;
				int maxCurrRow = 0;
				ring = players[player].positions[pos];
				while(1){
					ring = moveDirection(ring,direction);
					if(ring == nullCoord){
						break;
					}
					else if(board[ring.hex][ring.pos] == 2*my){
						if(count<4){
							thisScore -+ (4-count)*1; // Close rings have less score
						}
						break;
					}
					else if(board[ring.hex][ring.pos] == -2*my){
						if(rowBlocks == 1){ // If there is now empty space for the ring to move
							thisScore += countAhead(ring,direction,player);
						}
						else thisScore += 1;
						if(count<4){
							thisScore += count; 
						}
					}
					else if(board[ring.hex][ring.pos] == my){
						if(currRow == -1*my){
							if(rowFound > maxRowsFound){
								maxRowsFound = rowFound;
								maxCurrRow = currRow;
								rowFound = 1;
							}
						}
						else if(currRow == my){
							rowFound++;
						}
						else if(currRow == 4){
							rowFound=1;
							currRow=my;
						}
						currRow = my;
						marked = 1;
						markers.push_back(ring);
					}
					else if(board[ring.hex][ring.pos] == -1*my){
						if(currRow == my){
							if(rowFound > maxRowsFound){
								maxRowsFound = rowFound;
								maxCurrRow = currRow;
								rowFound = 1;
							}
						}
						else if(currRow == -1*my){
							rowFound++;
						}
						else if(currRow == 4){
							rowFound=1;
							currRow=-1*my;
						}
						currRow = -1*my;
						marked = 1;
						markers.push_back(ring);
					}
					else if(board[ring.hex][ring.pos] == 0){
						if(marked == 1){
							validNow = 1;
							break;
						}
						currRow = 4;
						rowFound = 0;
						rowBlocks = 0;
						teamwork[player] += 2*dummyBoard[player][ring.hex][ring.pos]+1;
						dummyBoard[player][ring.hex][ring.pos]++;
					}
					count++;
				}
				if(validNow == 1){
					valid[direction] == validNow;
					if(maxCurrRow == my){
						rowsFound[direction] = maxRowsFound;
					}
					else rowsFound[direction] = -1*maxRowsFound;
					for(int i=0;i<markers.size();i++){
						ring = markers[i];
						teamwork[player] += 2*dummyBoard[player][ring.hex][ring.pos]+1;
						dummyBoard[player][ring.hex][ring.pos]++;
					}
					thisScore +=count;
				}
				count = 0;
				markers.clear();
			}
			for(int i=0;i<6;i++){
				if(rowsFound[i] >0){
					if(valid[(i+3)%6] == 1){
						thisScore += pow(2,rowsFound[i]);
					}
				}
				else if(rowsFound[i] < 0){
					if(valid[i] == 1){
						thisScore += pow(2,abs(rowsFound[i]));
					}
				}
			}
			scores[player].push_back(thisScore);
		}
	}
	float scoresFinal[2];
	for(int i=0;i<2;i++){
		float max = -1000;
		float min = 1000;
		for(int j=0;j<scores[i].size();j++){
			max = (scores[i][j]>max)?scores[i][j]:max;
		}
		scoresFinal[i]= max;
	}
	answer[0] = edgeControl[0];
	answer[1] = edgeControl[1];
	answer[2] = scoresFinal[0];
	answer[3] = scoresFinal[1];
	answer[4] = teamwork[0];
	answer[5] = teamwork[1];
	answer[6] = numberOfRings-players[0].ringsOnBoard;
	answer[7] = numberOfRings-players[1].ringsOnBoard;
}
float evaluatePly(Ply& ply){
	if(ply.flag == 1){ // Ring placement
		float init[5] = {0};
		evaluateRings.evaluate(init);
		float final[5] = {0};
		vector<int> removal;
		executePly(ply,0,removal);
		evaluateRings.evaluate(final);
		float edge[2] = {0};
		getEdgeControl(edge);
		final[3] = edge[0];
		final[4] = edge[1];
		reversePly(ply,0,removal);
		return getScore(init,final,1);

	}
	float init[11] = {0};
	evaluateBoard(init);
	vector<int> removal;
	executePly(ply,0,removal);
	float final[11] = {0};
	if(ply.flag == 0){
		float para[3] = {0};
		evaluateMove(ply,para);
		for(int i=0;i<3;i++){
			final[11-3+i] = para[i];
		}
	}
	evaluateBoard(final);
	reversePly(ply,0,removal);
	return getScore(init,final,2);
}
Answer driveDFS(float alpha,float beta,bool turn,bool previousTurn,int currDepth,int flag,Ply &passed);
Answer DFSMove(float alpha,float beta,bool turn,bool previousTurn,int currDepth) {
	float currAlpha = alpha;
	float currBeta = beta;
	vector<MoveInfo> listOfMoves;
	vector<Ply> allPlys;
	Ply passed = Ply();
	vector<int> container;
	Answer answer = Answer(0);
	for(int i=0; i<players[turn].ringsOnBoard; i++) {
		getAvailableMoves(turn,players[turn].positions[i].hex,players[turn].positions[i].pos,listOfMoves);
	}
	for(int i=0;i<listOfMoves.size();i++){
		allPlys.push_back(Ply(listOfMoves[i]));
	}
	if(currDepth!= depth-1){
		for(int i=0;i<allPlys.size();i++){
			allPlys[i].sortingFactor = evaluatePly(allPlys[i]); // Evaluate each ply and give it a score
		}
		if(turn == 0){
			sort(allPlys.begin(),allPlys.end(),sortPlysMax);
		}
		else sort(allPlys.begin(),allPlys.end(),sortPlysMin);
	}
	if(allPlys.size() == 0 and currDepth!=0){ // If no moves left this state has score 0
		answer.value = 0;
		return answer;
	}
	answer.value = (turn == 0)?minf:inf;
	if(currDepth == depth-1){ // No sorting required
		for(int i=0;i<allPlys.size();i++){
			Ply ply = allPlys[i];
			Answer newAnswer = Answer(0);
			newAnswer.value = evaluatePly(ply);
			compare(newAnswer.value,currAlpha,currBeta,turn);
			if(change(answer.value,newAnswer.value,turn) == 1) {
				answer.ply = ply;
			}
			if(currAlpha>currBeta) {
				break;
			}
			visited++;
		}
	}
	else{
		for(int i=0; i<allPlys.size(); i++) {
			Ply ply = allPlys[i];
			executePly(ply,0,container);
			Answer newAnswer = driveDFS(currAlpha,currBeta,turn,previousTurn,currDepth+1,ply.flag,passed);
			newAnswer.value+= allPlys[i].sortingFactor;
			compare(newAnswer.value,currAlpha,currBeta,turn);
			if(change(answer.value,newAnswer.value,turn) == 1) {
				answer.ply = ply;
			}
			reversePly(ply,0,container);
			if(currAlpha>currBeta) {
				break;
			}
			visited++;
		}
	}
	answer.ply.flag = 0;
	return answer;
}
Answer DFSRemoval(float alpha,float beta,bool turn,bool previousTurn,int currDepth,Ply &passed) {
	float currAlpha = alpha;
	float currBeta = beta;
	Answer answer = Answer(-1);
	answer.value = (turn == 0)?minf:inf;
	vector<vector<Removal> > rowRemoval;
	vector<RowInfo> rows;
	rowStorage.getCopy(turn,0,rows);	
	permute(rowRemoval,rows,turn,rows.size());
	Answer newAnswer = Answer(-1);
	vector<Ply> allPlys;
	for(int i=0;i<rowRemoval.size();i++){
		allPlys.push_back(Ply());
		for(int j=0;j<rowRemoval[i].size();j++){
			allPlys.back().rowRemovals.push_back(rowRemoval[i][j]);
		}
	} 
	if(allPlys.size() == 0 and currDepth!=0){
		answer.value = 0;
		return answer;
	}
	vector<int> container;
	if(currDepth!= depth-1){
		for(int i=0;i<allPlys.size();i++){	
			allPlys[i].sortingFactor = evaluatePly(allPlys[i]);
		}
		if(turn == 0){
			sort(allPlys.begin(),allPlys.end(),sortPlysMax);
		}
		else sort(allPlys.begin(),allPlys.end(),sortPlysMin);
	}
	Ply newPassed = Ply();
	if(currDepth == depth-1){
		for(int i=0;i<allPlys.size();i++){
			newAnswer.value = evaluatePly(allPlys[i]);
			compare(newAnswer.value,currAlpha,currBeta,turn);
			if(change(answer.value,newAnswer.value,turn) == 1) {
				answer.ply.rowRemovals.clear();			
				for(int j=0;j<allPlys[i].rowRemovals.size();j++){
					answer.ply.rowRemovals.push_back(allPlys[i].rowRemovals[j]);
				}
			}
			if(currAlpha>currBeta) {
				break;
			}
			visited++;
		}
	}
	else{
		for(int i=0; i<allPlys.size(); i++) {
			executePly(allPlys[i],0,container);
			newAnswer = driveDFS(alpha,beta,turn,previousTurn,currDepth+1,allPlys[i].flag,newPassed);
			newAnswer.value+= allPlys[i].sortingFactor;
			compare(newAnswer.value,currAlpha,currBeta,turn);
			if(change(answer.value,newAnswer.value,turn) == 1) {
				answer.ply.rowRemovals.clear();			
				for(int j=0;j<allPlys[i].rowRemovals.size();j++){
					answer.ply.rowRemovals.push_back(allPlys[i].rowRemovals[j]);
				}
			}
			reversePly(allPlys[i],0,container);
			if(currAlpha>currBeta) {
				break;
			}
			container.clear();
			visited++;
		}
	}
	for(int i=0;i<answer.ply.rowRemovals.size();i++){
		passed.rowRemovals.push_back(answer.ply.rowRemovals[i]);		
	}
	answer.ply.flag = -1;
	return answer;
}
Answer DFSRing(float alpha,float beta,bool turn,bool previousTurn,int currDepth) {	
	float currAlpha = alpha;
	float currBeta = beta;
	Answer answer = Answer(1);
	answer.value = (turn == 0 )?minf:inf;
	vector <int> container;
	vector <Ply> allPlys;
	for(int i=0;i<positions.size();i++){
		if(available(positions[i]) == true){
			allPlys.push_back(Ply(positions[i],turn));
		}
	}
	if(currDepth!= depth-1){
		for(int i=0;i<allPlys.size();i++){
			allPlys[i].sortingFactor = evaluatePly(allPlys[i]);
		}
		if(turn == 0){
			sort(allPlys.begin(),allPlys.end(),sortPlysMax);
		}
		else sort(allPlys.begin(),allPlys.end(),sortPlysMin);
	}
	Ply passed = Ply();
	Answer newAnswer = Answer(1);
	if(currDepth == depth-1){
		for(int i=0;i<allPlys.size();i++){
			newAnswer.value = evaluatePly(allPlys[i]);
			compare(newAnswer.value,currAlpha,currBeta,turn);
			if(change(answer.value,newAnswer.value,turn) == 1) {
				answer.ply = allPlys[i];
			}
			if(currAlpha>currBeta) {
				break;
			}
			visited++;
		}
	}
	else{
		for(int i=0; i<allPlys.size(); i++) {
			executePly(allPlys[i],0,container);
			Answer newAnswer = driveDFS(alpha,beta,turn,previousTurn,currDepth+1,allPlys[i].flag,passed);
			newAnswer.value+= allPlys[i].sortingFactor;
			compare(newAnswer.value,currAlpha,currBeta,turn);
			if(change(answer.value,newAnswer.value,turn) == 1) {
				answer.ply = allPlys[i];
			}
			reversePly(allPlys[i],0,container);
			if(currAlpha>currBeta) {
				break;
			}
			visited++;
		}
	}
	answer.ply.flag = 1;
	return answer;
}
Answer driveDFS(float alpha,float beta,bool turn,bool previousTurn,int currDepth,int flag,Ply& passed) {
	if(players[0].ringsOnBoard < numberOfRings- 2 and ringsPlaced == 1){
		Answer a = Answer(0);
		a.value = 10000;
		return a;
	}
	else if(players[1].ringsOnBoard < numberOfRings- 2 and ringsPlaced == 1){
		Answer a = Answer(0);
		a.value = -10000;
		return a;
	}	
	else if((players[0].ringsOnBoard < numberOfRings or players[1].ringsOnBoard < numberOfRings) and ringsPlaced == 0) { // Rings not placed yet
		return DFSRing(alpha,beta,previousTurn,turn,currDepth);
	} else if(flag ==1) { // Final ring placed	
		return DFSMove(alpha,beta,!turn,turn,currDepth);
	} else if(flag == 0) { // Moved	
		if(turn == 0) { 
			if(rowStorage.size(0,0)!=0) {
				return DFSRemoval(alpha,beta,0,turn,currDepth,passed);
			} else if(rowStorage.size(1,0)!=0) {
				return DFSRemoval(alpha,beta,1,turn,currDepth,passed);
			} else return DFSMove(alpha,beta,!turn,turn,currDepth);
		} else if(turn == 1) {
			if(rowStorage.size(1,0)!=0) {
				return DFSRemoval(alpha,beta,1,turn,currDepth,passed);
			} else if(rowStorage.size(0,0)!=0) {
				return DFSRemoval(alpha,beta,0,turn,currDepth,passed);
			} else return DFSMove(alpha,beta,!turn,turn,currDepth);
		}
	}		
	if(flag == -1) {
		rowStorage.updateRows(turn,0);
		rowStorage.updateRows(!turn,0);
		if(rowStorage.size(!turn,0) == 0) { // No rows left
			return DFSMove(alpha,beta,!previousTurn,turn,currDepth); // Move
		} else {
			return DFSRemoval(alpha,beta,!turn,turn,currDepth,passed); // Remove rows
		}
	}	
}
Answer startDFS(float alpha,float beta,bool turn,bool previousTurn,int currDepth,int depthToSearch,Ply &passed) {
	depth = depthToSearch;
	Answer answer(-1);
	answer.value = -1;
	if(rowStorage.size(0,1)!=0) {
		answer = DFSRemoval(alpha,beta,turn,previousTurn,currDepth,passed);
	} else if(ringsPlaced==0) {
		answer =  DFSRing(alpha,beta,turn,previousTurn,currDepth);
	} else {
		answer = DFSMove(alpha,beta,turn,previousTurn,currDepth);
	}
	rowStorage.sync();
	if(visited == 0){
		stalemate = 1;
	}
	return answer;
}
string printMove(vector<Ply> &plys,vector <int> &container1,vector<int> &container2,int printMoves) {
	string output = "";
	bool changeContainer  = 0;
	vector<int> actual;
	for(int i=0; i<plys.size(); i++) {
		Ply ply = plys[i];
		if(ply.flag == 1) {
			string MoveType = "P";
			string ring = to_string(ply.ringPlacement.ring.hex) + " " + to_string(ply.ringPlacement.ring.pos);
			output = output + " " + MoveType + " " + ring;
		} else if(ply.flag == 0) {
			string Select = "S " + to_string(ply.moveInfo.start.hex) + " " + to_string(ply.moveInfo.start.pos);
			string Move = "M " + to_string(ply.moveInfo.end.hex) + " " + to_string(ply.moveInfo.end.pos);
			output = output+ " " + Select + " " + Move;
			changeContainer = 1;
		} else {
			if(changeContainer == 0){
				actual = container1;
			}
			else actual = container2;	
			for(int j=0; j<actual.size(); j++) {
				Removal removal = ply.rowRemovals[actual[j]];
				string RowStart = "RS " + to_string(removal.rowRemoval.start.hex) + " " + to_string(removal.rowRemoval.start.pos);
				string RowEnd = "RE " + to_string(removal.rowRemoval.end.hex) + " " + to_string(removal.rowRemoval.end.pos);
				string RowRemoval =  "X " + to_string(removal.ringRemoval.hex) + " " + to_string(removal.ringRemoval.pos);
				output = output + " " + RowStart + " " + RowEnd + " " + RowRemoval;
			}
		}
		if(i+1 == printMoves){ // Print only required moves
			break;
		}
	}
	return output;
}
int getDepth(){
	if(ringsPlaced == 0){
		if(board_size == 5){
			return 3;
		}
		else if(board_size == 6){
			return 2;
		}
	}
	if(board_size == 5 and sequenceLength == 5){
		if(countMoves<22){
			return 4;
		}
		else if(countMoves<35){
			return 5;
		}
		else return 6;
	}
	else if(board_size == 6){
		if(countMoves<25){
			return 4;
		}
		else if(countMoves<45){
			return 5;
		}
		else return 6;
	}
}
void CurrSet(){
	if(board_size == 5){
		currSet = 0;
	}
	else if(sequenceLength == 5){
		currSet = 1;
	}
	else currSet = 2;
}
void initRingWeights(){
	if(currSet == 0){
		weights[0][0] = 2.5;
		weights[0][1] = 1;
		weights[0][2] = 3;
		weights[0][3] = 1;
		weights[0][4] = -1;
	}
	else if(currSet == 1){
		weights[1][0] = 1.5;
		weights[1][1] = 1;
		weights[1][2] = 3;
		weights[1][3] = 1;
		weights[1][4] = -1;
	}
	else if(currSet == 2){
		weights[2][0] = 1.5;
		weights[2][1] = 1;
		weights[2][2] = 3;
		weights[2][3] = 1;
		weights[2][4] = -1;
	}
	boardWeights[0] = 1;
	boardWeights[1] = -2;
	boardWeights[2] = 2;
	boardWeights[3] = -3;
	boardWeights[4] = 0.5;
	boardWeights[5] = -0.1;
	boardWeights[6] = 100;
	boardWeights[7] = -100;
	boardWeights[8] = 10;
	boardWeights[9] = -5;
	boardWeights[10] = 5;
}
void initiate(){
	initiatePlayers();
	initiateBoard();
	getPositions();
	CurrSet();
	initRingWeights();
	return;
}

bool checkGameOver(){
	if(ringsPlaced == 1){
		if(players[0].ringsOnBoard < numberOfRings-2){
			return true;
		}
	}
	return false;
}
int main(){
	srand(time(0));
	cin >> player_id >> board_size >> game_timer >> sequenceLength;
	numberOfRings = board_size;
	initiate();
	vector<Ply> plys;
	Answer answer(0);
	float alpha = minf;
	float beta = inf;
	vector<int> container1;
	vector<int> container2;
	Ply passed = Ply();
	vector<int> empty;
	string input = "";
	bool turn = 0;
	int printMoves = 0;
	float para[11];
	if(player_id == 1){
		players[0].id = 1;
		players[1].id = 2;
		answer.ply = Ply(Coord(0,0),0);
		plys.push_back(answer.ply);
		executePly(answer.ply,1,empty);
		countMoves++;
		printMoves = 1;
		while(true){
			cout << printMove(plys,container1,container2,printMoves) << endl;
			printMoves = 0;
			if(ringsPlaced == 1 and (players[0].ringsOnBoard < numberOfRings-2 or players[1].ringsOnBoard < numberOfRings-2)){
				break;
			}
			plys.clear();
			container1.clear();
			container2.clear();
			passed.rowRemovals.clear();
			while(input.length()==0){
				getline(cin,input);
			}
			executeMoves(input);
			if(ringsPlaced == 1 and (players[0].ringsOnBoard < numberOfRings-2 or players[1].ringsOnBoard < numberOfRings-2)){
				break;
			}
			input = "";			
			if(ringsPlaced == 0){
				if(players[0].ringsOnBoard == numberOfRings and players[1].ringsOnBoard == numberOfRings){
					ringsPlaced = 1;
					countMoves = 0;
				}
			}
			if(rowStorage.size(0,1)!=0){
				answer = startDFS(alpha,beta,0,1,0,getDepth(),passed);	
				if(checkGameOver() == 0){ // Can print one move
					printMoves++;
				}
				executePly(passed,1,container1);
				countMoves++;
				plys.push_back(passed);
			}
			passed.rowRemovals.clear();
			answer = startDFS(alpha,beta,0,1,0,getDepth(),passed);
			if(stalemate == 1){
				cout << printMove(plys,container1,container2,printMoves) << endl; // No move found
				break;
			}
			if(checkGameOver() == 0){ // Can print one more move
				printMoves++;
			}
			executePly(answer.ply,1,empty);
			countMoves++;
			plys.push_back(answer.ply);
			if(rowStorage.size(0,1)!=0){
				answer = startDFS(alpha,beta,0,1,0,getDepth(),passed);
				if(checkGameOver() == 0){ // Can print one more move
					printMoves++;
				}
				executePly(passed,1,container2);
				countMoves++;
				plys.push_back(passed);
			}
		}
	}
	else if(player_id == 2){
		players[0].id = 2;
		players[1].id = 1;
		while(true){
			printMoves = 0; // Counts moves to print
			plys.clear();
			container1.clear();
			container2.clear();
			passed.rowRemovals.clear();
			while(input.length()==0){
				getline(cin,input);
			}
			executeMoves(input);
			if(ringsPlaced == 1 and (players[0].ringsOnBoard < numberOfRings-2 or players[1].ringsOnBoard < numberOfRings-2)){ // If game over
				break;
			}
			input = "";
			if(rowStorage.size(0,1) >0){
				answer = startDFS(alpha,beta,0,1,0,getDepth(),passed);
				if(checkGameOver() == 0){
					printMoves++;
				}
				executePly(passed,1,container1);
				countMoves++;
				plys.push_back(passed);
			}
			answer = startDFS(alpha,beta,0,1,0,getDepth(),passed);
			if(stalemate == 1){
				cout << printMove(plys,container1,container2,printMoves) << endl;
				break;
			}
			passed.rowRemovals.clear();
			if(checkGameOver() == 0){
				printMoves++;
			}
			executePly(answer.ply,1,empty);
			countMoves++;
			plys.push_back(answer.ply);
			if(rowStorage.size(0,1) >0){
				answer = startDFS(alpha,beta,0,1,0,getDepth(),passed);
				if(checkGameOver() == 0){
					printMoves++;
				}
				executePly(passed,1,container2);
				countMoves++;
				plys.push_back(passed);
			}
			if(ringsPlaced == 0){
				if(players[0].ringsOnBoard == numberOfRings and players[1].ringsOnBoard == numberOfRings){
					ringsPlaced = 1;
					countMoves = 0;
				}
			}
			cout << printMove(plys,container1,container2,printMoves) << endl;
			if(ringsPlaced == 1 and (players[0].ringsOnBoard < numberOfRings-2 or players[1].ringsOnBoard < numberOfRings-2)){
				break;
			}
		}
	}
	return 0;
}

