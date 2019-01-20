/*****************************************************************************\
 * Theory of Computer Games: Fall 2012
 * Chinese Dark Chess Search Engine Template by You-cheng Syu
 *
 * This file may not be used out of the class unless asking
 * for permission first.
 *
 * Modify by Iou-Shiuan,Liu, December 2013
\*****************************************************************************/
#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<climits>
#include<algorithm>
#include<random>
#include<list>
#include"anqi.hh"
#include"Hash.h"
#include"Protocol.h"
#include"ClientSocket.h"

#ifdef _WINDOWS
#include<windows.h>
#else
#include<ctime>
#endif

static const POS ADJ[32][4]={//right,up,left,down
	{ 1,-1,-1, 4},{ 2,-1, 0, 5},{ 3,-1, 1, 6},{-1,-1, 2, 7},
	{ 5, 0,-1, 8},{ 6, 1, 4, 9},{ 7, 2, 5,10},{-1, 3, 6,11},
	{ 9, 4,-1,12},{10, 5, 8,13},{11, 6, 9,14},{-1, 7,10,15},
	{13, 8,-1,16},{14, 9,12,17},{15,10,13,18},{-1,11,14,19},
	{17,12,-1,20},{18,13,16,21},{19,14,17,22},{-1,15,18,23},
	{21,16,-1,24},{22,17,20,25},{23,18,21,26},{-1,19,22,27},
	{25,20,-1,28},{26,21,24,29},{27,22,25,30},{-1,23,26,31},
	{29,24,-1,-1},{30,25,28,-1},{31,26,29,-1},{-1,27,30,-1}
};

using namespace std;
int totalPly = 60;
int nowRound = 0;
int remain_time;
int darkChessCnt = 32, brightChessCnt = 0;
bool isEndGame = false;

const int DEFAULTTIME = 15;
typedef  int SCORE;
static const SCORE INF=1000001;
static const SCORE WIN=1000000;
SCORE SearchMax(const BOARD&,int,int);
SCORE SearchMin(const BOARD&,int,int);

#define BOARDPLACE 32
#define TYPEOFFIN 14//14 type of FIN
#define PLAYERCNT 2
Hash hashTable;

unsigned long long int randFin[16][BOARDPLACE];
unsigned long long int randTurn[PLAYERCNT];

unsigned long long int generateRandomULL(){
    static mt19937_64 randDevice(random_device{}());
    return uniform_int_distribution<unsigned long long int>(0, ULLONG_MAX)(randDevice);
}
SCORE NegaScout(const BOARD &B,  SCORE alpha,  SCORE beta, const int dep, const int cut, const unsigned long long int & nowHash);
void init_hashSeed(){
	for(int i=0;i<16;i++){
		for(int j=0;j<BOARDPLACE;j++){
			randFin[i][j] = generateRandomULL();
		}
	}
	for(int i=0;i<PLAYERCNT;i++)
		randTurn[i] = generateRandomULL();
	return;
}


#ifdef _WINDOWS
DWORD Tick;     // 開始時刻
int   TimeOut;  // 時限
#else
clock_t Tick;     // 開始時刻
clock_t TimeOut;  // 時限
#endif
MOV   BestMove; // 搜出來的最佳著法

bool TimesUp() {
#ifdef _WINDOWS
	return GetTickCount()-Tick>=TimeOut;
#else
	return clock() - Tick >= TimeOut;
#endif
}
//                   0:帥將,1:仕士,2:相象,3:硨車,4:傌馬,5:炮砲,6:兵卒
int material[7] = {5095,4047,1523,761,450,820,200};
int dynamic_material[14] = {5095,4047,1523,761,450,820,200,5095,4047,1523,761,450,820,200};
#define pawnValueWithoutKing 100
SCORE BfsOfManhattan(const BOARD &B, POS p){
	if(GetLevel(B.fin[p])==LVL_C) return 0;

	POS finPosArray[BOARDPLACE];
	int pointerOfPOS = 0, totalPos = 0;
	SCORE score = 0;
	int dist[BOARDPLACE];
	POS nowP, nextP;
	FIN nowF, nextF;
	for(int i=0;i<BOARDPLACE;i++) dist[i]=INF;
	
	finPosArray[totalPos] = p, dist[p] = 0;//由該點擴散來算距離
	totalPos++;

	while(pointerOfPOS<totalPos){
		nowP = finPosArray[pointerOfPOS];
		nowF=B.fin[nowP];
		for(int i=0;i<4;i++) {
			nextP=ADJ[nowP][i];
			if(nextP==-1 || dist[nextP]!=INF) continue; //boarder
			nextF=B.fin[nextP];
			if((nowF<14 && !ChkEats(nowF,nextF)) || nextF==FIN_X) continue;//不能被吃
			dist[nextP]=dist[nowP]+1;//best fit search 距離
			finPosArray[totalPos] = nextP;
			totalPos++;
			if(ChkEats(nowF,nextF))
				score+=(10-dist[nextP])*dynamic_material[nextF]/50;
		}
		pointerOfPOS++;
	}
	return score;
}

SCORE getAttackValue(const BOARD &B){
	if(!isEndGame) return 0;

	POS pos[PLAYERCNT][BOARDPLACE];
	int cnt[PLAYERCNT]={0, 0};
	SCORE attack[PLAYERCNT]={0, 0};

	for(int i=0;i<BOARDPLACE;i++){
		if(B.fin[i]>=FIN_X) continue; //is FIN_E or FIN_X
		CLR clr = GetColor(B.fin[i]);
		pos[clr][cnt[clr]++]=i;
	}

	int self=B.who, op=B.who^1;
	for(int i=0;i<cnt[self];i++) 
		attack[self]+=BfsOfManhattan(B, pos[self][i]);
	for(int i=0;i<cnt[op];i++) 
		attack[op]+=BfsOfManhattan(B, pos[op][i]);
	
	return attack[self]-attack[op];
}

SCORE getMaterialValue(const BOARD &B){
	int cnt[PLAYERCNT]={0,0};
	int aliveCnt[TYPEOFFIN]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};//每個棋子還在盤面上的數量，包含未翻開
	int material_strength;
	int Pawn[PLAYERCNT]={0,0}, Pawn_strength;
	int King[PLAYERCNT]={0,0}, King_strength;
	int BestFin[PLAYERCNT] = {FIN_E,FIN_E}, BestPos[PLAYERCNT]={0,0}, Best2Fin[PLAYERCNT] = {FIN_E,FIN_E}, Best2Pos[PLAYERCNT]={0,0}, mustWin_strength = 0;
	//盤面上
	for(POS p=0;p<BOARDPLACE;p++){
		const CLR c = GetColor(B.fin[p]);
		if(c!=-1){
			cnt[c] += dynamic_material[B.fin[p]];
			aliveCnt[B.fin[p]]++;
		}
	}
	//未翻開
	for(int i=0;i<7;i++){
		cnt[0] += B.cnt[i] * dynamic_material[i];
		aliveCnt[i]+= B.cnt[i];
	}
	for(int i=7;i<TYPEOFFIN;i++){
		cnt[1] += B.cnt[i] * dynamic_material[i];
		aliveCnt[i]+= B.cnt[i];
	}

	material_strength = cnt[B.who]-cnt[B.who^1];
	
	//對方王死，我的兵要變少分，不然就要增加分數
	for(int i=0;i<PLAYERCNT;i++){
		if(aliveCnt[(i^1)*7]==0){
			Pawn[i] -= (material[6]-pawnValueWithoutKing) * aliveCnt[i*7+6];//敵方王已死，自己小子要變成 pawnValueWithoutKing
		}
		else{
			Pawn[i] +=  100 * aliveCnt[i*7+6];
		}
	}
	Pawn_strength = Pawn[B.who]-Pawn[B.who^1];

	//對方卒死，我的王就萬能，要增加分數
	for(int i=0;i<PLAYERCNT;i++){
		if(aliveCnt[(i^1)*7+6]==0)
			King[i] = 1000 * aliveCnt[i*7];
	}
	King_strength = King[B.who]-King[B.who^1];

	if(isEndGame){
		//取得雙方目前最大棋子
		bool hasBestFin[2] = {false, false};
		bool has2Fin[2] = {false, false};
		for(int pl=0;pl<PLAYERCNT;pl++){
			for (int i=0; i<7; i++){
				if(aliveCnt[pl*7+i]!=0){
					BestFin[pl] = i;
					if(aliveCnt[pl*7+i]==2){
						Best2Fin[pl] = i;
						hasBestFin[pl] = true;
						break;
					}
					for(int j=i+1;j<7;j++){
						if(aliveCnt[pl*7+j]!=0){
							Best2Fin[pl] = j;
							has2Fin[pl] = true;
							break;
						}
					}
				}
			}
		}

		bool findBiggest;
		for(POS p=0;p<BOARDPLACE;p++){
			findBiggest = false;
			for(int pl=0;pl<PLAYERCNT;pl++){
				if(!findBiggest && B.fin[p]==(pl*7+BestFin[pl])){
					BestPos[pl] = p;
					findBiggest = true;
					continue;
				}
				if(B.fin[p]==(pl*7+Best2Fin[pl]))
					Best2Pos[pl] = p;
			}
		}

		if((BestFin[B.who]<BestFin[B.who^1] && Best2Fin[B.who]<=Best2Fin[B.who^1]) ||
			(BestFin[B.who]<=BestFin[B.who^1] && Best2Fin[B.who]<Best2Fin[B.who^1])){
			int dis, remainDis = 0;
			if(hasBestFin[B.who^1]==false)//敵方沒有子
				dis = 0;
			else if(has2Fin[B.who^1]==false){//敵方只剩一子
				if(has2Fin[B.who]==false){//我只剩一子
					dis = (abs(BestPos[B.who]/4-BestPos[B.who^1]/4)+abs(BestPos[B.who]%4-BestPos[B.who^1]%4));
					remainDis = 10-dis;
				}else{
					dis = (abs(BestPos[B.who]/4-BestPos[B.who^1]/4)+abs(BestPos[B.who]%4-BestPos[B.who^1]%4));
					dis += (abs(Best2Pos[B.who]/4-BestPos[B.who^1]/4)+abs(Best2Pos[B.who]%4-BestPos[B.who^1]%4));
					remainDis = 20-dis;
				}
			}else{//敵方有兩子
				if(has2Fin[B.who]==false){//我只剩一子
					dis = (abs(BestPos[B.who]/4-BestPos[B.who^1]/4)+abs(BestPos[B.who]%4-BestPos[B.who^1]%4));
					remainDis = 10-dis;
				}else{
					dis = (abs(BestPos[B.who]/4-BestPos[B.who^1]/4)+abs(BestPos[B.who]%4-BestPos[B.who^1]%4));
					dis += (abs(Best2Pos[B.who]/4-BestPos[B.who^1]/4)+abs(Best2Pos[B.who]%4-BestPos[B.who^1]%4));
					remainDis = 20-dis;
				}
			}
			mustWin_strength = remainDis * material[BestFin[B.who^1]] / 100;

			
		}
	}

	return material_strength + Pawn_strength + King_strength + mustWin_strength;
}

SCORE Eval(const BOARD &B) {
	if (B.ChkLose()) return -WIN; //check lose

	return getMaterialValue(B) + getAttackValue(B);
}
//計算每個位置的翻出來有多少個是安全的(我不被吃且可能敵方被我吃)
POS flipChess(const BOARD &B){
	POS pos=-1,p;
	
	if(darkChessCnt==1){
		for(p=0;p<BOARDPLACE;p++){
			if(B.fin[p]==FIN_X){
				return p;
			}
		}
	}

	bool OpCannonKillMePos[32];
	int OpCannonOpenCnt = 2 - B.cnt[(B.who^1)*7+5];
	int cntCanTemp = 0;
	for(int p=0;p<BOARDPLACE;p++){
		OpCannonKillMePos[p]=false;
	}
	fprintf(stderr,"Cannon:");
	for(int p=0;p<BOARDPLACE && cntCanTemp<OpCannonOpenCnt ;p++){
		if(B.fin[p]%7==5 && GetColor(B.fin[p])==(B.who^1)){
			if( (p+2<32) && (((p+2)/4)==(p/4)) ){
				OpCannonKillMePos[p+2]=true;
				fprintf(stderr,"%d ",p+2);
			}
			if( (p-2>=0) && (((p-2)/4)==(p/4)) ){
				OpCannonKillMePos[p-2]=true;
				fprintf(stderr,"%d ",p-2);
			}
			if(p+8<32){
				OpCannonKillMePos[p+8]=true;
				fprintf(stderr,"%d ",p+8);
			}
			if(p-8>=0){
				OpCannonKillMePos[p-8]=true;
				fprintf(stderr,"%d ",p-8);
			}
			cntCanTemp++;
		}
	}
	fprintf(stderr,"\n");
	int c = darkChessCnt;
	int bestScore = -INF, currentScore;
	int OpBig, mySmall, myBig;
	bool OpHasPawn;
	POS nextP;
	for(int p=0;p<BOARDPLACE;p++){
		if(B.fin[p]==FIN_X && c>0){
			currentScore = 0;

			OpBig = INF;
			mySmall = -INF;
			myBig = INF;
			//敵人的砲
			OpHasPawn = false;
			for(int i=0;i<4;i++) { //adjacent location
				nextP=ADJ[p][i];
				if(nextP==-1)
					continue;
				if ( B.fin[nextP]%7==6 && GetColor(B.fin[nextP])==(B.who^1) )
					OpHasPawn = true;
				if ( GetColor(B.fin[nextP])==(B.who^1) && OpBig>(B.fin[nextP]%7))
					OpBig = B.fin[nextP]%7;
				else if ( GetColor(B.fin[nextP])==(B.who)){
					if(mySmall<(B.fin[nextP]%7))
						mySmall = B.fin[nextP]%7;
					if(myBig>(B.fin[nextP]%7))
						myBig = B.fin[nextP]%7;
				}
			}
			fprintf(stderr,"[pos %d],OpCannonKillMePos=%s, OpHasPawn=%s, OpBig=%d, mySmall=%d, myBig=%d\n",p,OpCannonKillMePos[p]?"true":"false",OpHasPawn?"true":"false", LVL(OpBig),LVL(mySmall),LVL(myBig) );
			if(OpCannonKillMePos[p]==false){//敵方的砲不會攻擊這裡，所以我的子才有可能有用
				if(OpBig!=INF){//敵方最大子，找我方不被吃的子
					for(int i=OpBig-1;i>=0;i--){
						currentScore += B.cnt[B.who*7+i];
					}
					//敵方有小兵的話，應該要扣掉我的王
					if(OpHasPawn)
						currentScore -= B.cnt[B.who*7];
				}
				else{
					for(int i=0;i<7;i++){
						currentScore += B.cnt[B.who*7+i];
					}
				}
			}
			
				
			if(mySmall!=-INF){//我有子，找敵方
				for(int i=mySmall+1;i<7;i++){
					currentScore += B.cnt[(B.who^1)*7+i];
				}
				if(mySmall>5)//敵方的砲不會會吃我
					currentScore += B.cnt[(B.who^1)*7+5];
				if(mySmall==6 && myBig==INF)//卒，且只有卒
					currentScore += B.cnt[(B.who^1)*7];
			}else{//周圍沒有我方子
				for(int i=6;i>=0;i--){
					currentScore += B.cnt[(B.who^1)*7+i];
				}
			}
			cout<<currentScore<<" ";
			fprintf(stderr,",%d:%d ",p,currentScore);
			if(currentScore>bestScore){
				bestScore = currentScore;
				pos = p;
			}
			c--;
		}
	}
	cout<<endl<<"bestScore: "<<bestScore<<endl;
	fprintf(stderr,"\nbestScore:%d\n",bestScore);
	return pos;
}

void checkIsCorrectHash(const unsigned long long int & nowHash, const BOARD &B){
	unsigned long long int testHash = 0;
	for(POS p=0;p<BOARDPLACE;p++){
		testHash ^= randFin[B.fin[p]][p];
	}
	if((testHash^randTurn[B.who])!=nowHash){
		cout<<"[checkIsCorrectHash] "<<(testHash^randTurn[B.who^1])<<", "<<(testHash^randTurn[B.who])<<", "<<nowHash<<endl;
	}
	return;
}

SCORE Max(SCORE a, SCORE b){
	if(a>b) return a;
	else return b;
}
SCORE Min(SCORE a, SCORE b){
	if(a<b) return a;
	else return b;
}

bool compare_SmallToBig (const FIN& first, const FIN& second)
{
  return ( first > second );
}

int SEE(const BOARD &B, const MOVLST &lst, const int moveIndex){
	POS location = lst.mov[moveIndex].ed;
	std::list<FIN> mylist, Oplist;
	//Find the list of my pieces that can capture Op piece at location
	int totalNum, OptotalNum;
	for(int i=0;i<lst.num;i++){
		if(lst.mov[i].ed == location){
			mylist.push_back(B.fin[lst.mov[i].st]);
		}
	}
	totalNum = mylist.size();
	mylist.sort(compare_SmallToBig);
	//Find the list of Op pieces that can capture my piece at location
	BOARD N(B);
	N.fin[lst.mov[moveIndex].ed]=N.fin[lst.mov[moveIndex].st];
	N.fin[lst.mov[moveIndex].st]=FIN_E;
	N.who^=1;

	MOVLST Oplst;
	N.CapMoveGen(Oplst);
	for(int i=0;i<Oplst.num;i++){
		if(Oplst.mov[i].ed == location){
			Oplist.push_back(N.fin[Oplst.mov[i].st]);
		}
	}
	OptotalNum = Oplist.size();
	if(OptotalNum == 0)
		return 0;
	Oplist.sort(compare_SmallToBig);
	
	int gain = 0;
	FIN piece = B.fin[location];
	bool searchmyFindEat, searchOpFindEat;
	std::list<FIN>::iterator it;
	while(mylist.size()!=0){
		it=mylist.begin();
		searchmyFindEat = false;
		while (it!=mylist.end()){
			if(ChkEats(*it,piece)){//capture piece at location using the first element w in R
				gain += material[piece%7];
				piece = *it;
				it = mylist.erase (it);//remove w from R
				searchmyFindEat = true;
				break;
			}else{
				it++;
			}
		}
		if(!searchmyFindEat)//我方沒有子有能力可以吃得下敵方上個子
			break;
		if(Oplist.size()!=0){
			it=Oplist.begin();
			searchOpFindEat = false;
			while (it!=Oplist.end()){
				if(ChkEats(*it,piece)){//capture piece at location using the first element h in B
					gain -= material[piece%7];
					piece = *it;
					it = mylist.erase (it);//remove h form B
					searchOpFindEat = true;
					break;
				}else{
					it++;
				}
			}
			if(!searchOpFindEat)//敵方沒有子有能力可以吃得下我上個子
				break;
		}else
			break;
		
	}
	return gain;//the net gain of material values during the exchange
}

SCORE Quiescent(const BOARD &B,  SCORE alpha,  SCORE beta){
	
	MOVLST lst;
	if(B.CapMoveGen(lst)==0)
		return Eval(B);
	SCORE m = -INF;
	SCORE n = alpha;
	int quies = 0;
	for(int i=0;i<lst.num;i++){
		if(SEE(B, lst, i)>=0){//not a quiescent position, search deeper (我認為吃了有加分，所以要繼續搜)
			// cout<<"In Quiescent not a quiescent position"<<endl;
			BOARD N(B);
			N.fin[lst.mov[i].ed]=N.fin[lst.mov[i].st];
			N.fin[lst.mov[i].st]=FIN_E;
			N.who^=1;
			const SCORE tmp = -Quiescent(N, -beta, -n);
			if(tmp>m)
				m = tmp;
			if(m>=beta){//lower bound
				return m;//cut-off
			}
			if(tmp>n)//n=max(n,tmp)
				n = tmp;
		}else{
			quies ++;
		}
	}
	if(quies==lst.num)
		return Eval(B);
	else
		return m;
}

void printTab(int dep){
	if(dep<4){
		for(int i=0;i<dep;i++)
			fprintf(stderr,"\t");
	}
}

SCORE ChanceSearch(const BOARD &B, const MOV& flipMov, SCORE alpha,  SCORE beta, const int dep, const int cut, const unsigned long long int & nowHash){

	int c = 0;
	for(int f=0;f<TYPEOFFIN;f++){
		c += B.cnt[f];
	}
	SCORE Vmin = -WIN, Vmax = WIN;
	SCORE A_bound = c * (alpha - Vmax) + Vmax, B_bound = c * (beta - Vmin) + Vmin;
	SCORE m = Vmin, M = Vmax;
	SCORE vsum = 0;//current sum of expected values
	SCORE tmp;
	for(int f=0;f<TYPEOFFIN;f++){
		if(B.cnt[f]==0)
			continue;
		BOARD N(B);
		unsigned long long int nextHash = nowHash;
		N.Move(flipMov, nextHash, FIN(f));
		tmp = -NegaScout(N, -Min(B_bound,Vmax), -Max(A_bound, Vmin), dep+1, cut-1, nextHash);
		m += B.cnt[f] * (tmp - Vmin)/c, M += B.cnt[f] * (tmp - Vmax)/c; 
		if(tmp>=B_bound)//failed high, chance node cut off I
			return m;
		if(tmp<=A_bound)//failed low, chance node cut off II
			return M;
		vsum += B.cnt[f] * tmp;
		A_bound += B.cnt[f] * (Vmax - tmp), B_bound += B.cnt[f] * (Vmin - tmp);
	}
	return vsum/c;
}

// dep=現在在第幾層
// cut=還要再走幾層
#define SCOREWHENREPEAT -1000
SCORE NegaScout(const BOARD &B,  SCORE alpha,  SCORE beta, const int dep, const int cut, const unsigned long long int & nowHash) {
	if(B.ChkLose()){//no child, end game
		return -WIN;
	}
	
	MOVLST lst;
	if(cut==0||dep>20||B.MoveGenWithCapFirst(lst)==0){
		return +Quiescent(B, alpha, beta);
	}
	
	SCORE m = -INF;//the current lower bound; fail soft
	SCORE n = beta; // the current upper bound
	MOV tmpBestMov = lst.mov[0];
	
	int exact_Hash, hashIndex_Index;
	unsigned long long int hashIndex = nowHash & HASHSIZE;
	exact_Hash = hashTable.getIsAvailable(nowHash,hashIndex,hashIndex_Index);
	
	if(TimesUp()){
		cout<<"TimesUp NegaScout: "<< clock()<<", "<<Tick<<", "<<TimeOut<<", "<<((clock() - Tick))<<endl;
		if(exact_Hash!=0){
			return hashTable.getValue(hashIndex,hashIndex_Index);
		}
		return +Quiescent(B, alpha, beta);
	}
	if(B.visitcnt>=28)
		return 0;
	if(exact_Hash!=0){
		if(dep!=0 && dep%2==0 && (hashTable.checkVisit(nowHash)>1)){
			cout<<"Repeat in hash: "<<nowHash<<",value:"<<hashTable.getValue(hashIndex,hashIndex_Index)<<", visit: "<<hashTable.checkVisit(nowHash)<<endl;
			// m = SCOREWHENREPEAT;
			hashTable.insertHash(cut-1, exact_Hash, m, nowHash, tmpBestMov);
			return SCOREWHENREPEAT;
		}
		// cout<<"Not first time, "<<hashIndex_Index<<endl;
		tmpBestMov = hashTable.getBestMove(hashIndex,hashIndex_Index);
		if(cut<=hashTable.getDepth(hashIndex,hashIndex_Index)){
			if(exact_Hash==1){//exact value
				if(dep==0) BestMove=hashTable.getBestMove(hashIndex,hashIndex_Index);
				return hashTable.getValue(hashIndex,hashIndex_Index);
			}else{//exact_Hash==2 or 3//Is bound
				if(exact_Hash==3){//beta cut
					alpha = Max(alpha,hashTable.getValue(hashIndex,hashIndex_Index));
				}
				else{//alpha cut
					beta = Min(beta, hashTable.getValue(hashIndex,hashIndex_Index));
				}
			}
		}
		else{
			if(exact_Hash==1){//exact value
				if(dep==0) BestMove=hashTable.getBestMove(hashIndex,hashIndex_Index);
				m = hashTable.getValue(hashIndex,hashIndex_Index);
			}
		}

		//先做上次的步
		BOARD N(B);
		unsigned long long int nextHash = nowHash;
		N.Move(tmpBestMov,nextHash);
		const SCORE tmp = -NegaScout(N, -n, -Max(alpha,m), dep+1 , cut-1, nextHash);
		if(tmp==WIN && dep==0){
			BestMove=tmpBestMov;
			hashTable.insertHash(cut-1, 1, tmp, nowHash, BestMove);
			return WIN;
		}
		if(tmp>m && !TimesUp()){
			if(n==beta || cut<3 || tmp>=beta)
				m = tmp;
			else
				m = -NegaScout(N, -beta, -tmp, dep+1 , cut-1, nextHash);//re-search
			if(dep==0 && m>alpha)BestMove=tmpBestMov;
		}
		if(m>=beta){//lower bound
			hashTable.insertHash(cut-1, 3, m, nowHash, tmpBestMov);
			return m;//cut-off
		} 

		
		n = Max(alpha, m) + 1;//set up a null window
	}
	// checkIsCorrectHash(nowHash,B);
	int cutNew;
	if(lst.num<=3)// Op is killed (not flip)
		cutNew = cut + 1;
	else
		cutNew = cut;
	for(int i=0;i<lst.num;i++) {

		BOARD N(B);
		unsigned long long int nextHash = nowHash;
		N.Move(lst.mov[i],nextHash);
		
		const SCORE tmp = -NegaScout(N, -n, -Max(alpha,m), dep+1 , cutNew-1, nextHash);
		if(tmp==WIN && dep==0){
			BestMove=lst.mov[i];
			hashTable.insertHash(cutNew-1, 1, tmp, nowHash, BestMove); 
			return WIN;
		}
		if(tmp>m && !TimesUp()){
			if(n==beta || cut<3 || tmp>=beta)
				m = tmp;
			else
				m = -NegaScout(N, -beta, -tmp, dep+1 , cutNew-1, nextHash);//re-search
			if(m>alpha){//prevent error of TimeUP()
				tmpBestMov = lst.mov[i];
				if(dep==0)BestMove=lst.mov[i];
			}
			
		}
		if(m>=beta){//lower bound
			hashTable.insertHash(cutNew-1, 3, m, nowHash, tmpBestMov);
			return m;//cut-off
		} 
		n = Max(alpha, m) + 1;//set up a null window
	}
	//<=alpha upper cuts
	if(m<=alpha)
		hashTable.insertHash(cut-1, 2, m, nowHash, tmpBestMov);
	else
		hashTable.insertHash(cut-1, 1, m, nowHash, tmpBestMov); 
	return m;
}

//Aspiration search
#define IDAS_THRESHOLD 1000
SCORE IDAS(const BOARD &B, const unsigned long long int & nowHash, const int IterDepthLimit){

	SCORE bestScore = NegaScout(B,-INF, INF, 0, 3,nowHash);// search dep:3
	int currentDepLimit = 4, totalSearch = 4;
	cout<<3<<",negaScore:"<<bestScore<<", useTime:"<<((clock() - Tick)/CLOCKS_PER_SEC)<<endl;
	SCORE tempScore;
	while(currentDepLimit <= IterDepthLimit && totalSearch<30){
		tempScore = NegaScout(B,bestScore-IDAS_THRESHOLD, bestScore+IDAS_THRESHOLD, 0, currentDepLimit,nowHash);
		cout<<currentDepLimit<<",negaScore:"<<tempScore<<", useTime:"<<((clock() - Tick)/CLOCKS_PER_SEC)<<endl;
		if(tempScore<=bestScore - IDAS_THRESHOLD){//failed-low
			cout<<"Failed-low"<<endl;
			tempScore = NegaScout(B,-INF, tempScore, 0, currentDepLimit,nowHash);
			cout<<currentDepLimit<<",negaScore:"<<tempScore<<", useTime:"<<((clock() - Tick)/CLOCKS_PER_SEC)<<endl;
		}else if(tempScore>=bestScore + IDAS_THRESHOLD){//failed-high
			cout<<"Failed-high"<<endl;
			tempScore = NegaScout(B, tempScore, INF, 0, currentDepLimit,nowHash);
			cout<<currentDepLimit<<",negaScore:"<<tempScore<<", useTime:"<<((clock() - Tick)/CLOCKS_PER_SEC)<<endl;
		}
		if(bestScore<tempScore+IDAS_THRESHOLD)//突然-1000分 要再多搜一層
			currentDepLimit++;
		bestScore = tempScore;
		totalSearch++;
		if (bestScore==WIN)
			break;
		if(TimesUp()){//If remaining time cannot do another deeper search than return best
			cout<<"TimesUp: "<< clock()<<", "<<Tick<<", "<<TimeOut<<", "<<((clock() - Tick))<<endl;
			break;
		}
		
	}
	return bestScore;
}

#define IterDepthLimit_flip 5
MOV flipPos;
SCORE IDAS_flip(const BOARD &B, const unsigned long long int & nowHash, const SCORE movScore){
#ifdef _WINDOWS
	Tick=GetTickCount();
	// TimeOut = (DEFAULTTIME-3)*1000;
	TimeOut = 4*1000;
	if(remain_time<50000)//只剩下50秒
	{
		TimeOut = 2*1000;//想2秒
	}else if(remain_time<20000)//只剩下20秒
	{
		TimeOut = 1*1000;//想1秒
	}
#else
	Tick=clock();
	// TimeOut = (DEFAULTTIME-3)*CLOCKS_PER_SEC;
	TimeOut = 4*CLOCKS_PER_SEC;
	if(remain_time<50000)//只剩下50秒
	{
		TimeOut = 2*CLOCKS_PER_SEC;//想2秒
	}else if(remain_time<20000)//只剩下20秒
	{
		TimeOut = 1*CLOCKS_PER_SEC;//想1秒
	}
#endif
	
	if(darkChessCnt==0)
		return 0;

	MOVLST lst;lst.num=0;
	for(POS p=0;p<BOARDPLACE;p++)
		if(B.fin[p]==FIN_X) lst.mov[lst.num++]=MOV(p,p); //search for all unflip position

	FIN finList[14];
	int finListCnt[14];
	int sizeOfFinList = 0;
	for(int i=0;i<14;i++){
		if(B.cnt[i]>0){
			finList[sizeOfFinList] = FIN(i);
			finListCnt[sizeOfFinList] = B.cnt[i];
			sizeOfFinList++;
		}
	}
	cout<<"lst.num:"<<lst.num<<", sizeOfFinList:"<<sizeOfFinList<<endl;
	SCORE tempScore;
	double bestExpectedValue, thisIExpectedValue;
	MOV bestExpectedMOV = lst.mov[0];
	for(int currentDepLimit=3; currentDepLimit <= IterDepthLimit_flip;currentDepLimit++){
		bestExpectedValue = -INF;
		for(int i=0;i<lst.num;i++) { //for each unflip position
			thisIExpectedValue = 0;
			for(int f=0;f<sizeOfFinList;f++){
				BOARD N(B);
				unsigned long long int nextHash = nowHash;
				N.Move(lst.mov[i],nextHash,finList[f]);
				tempScore = -NegaScout(N,-INF, INF, 1, currentDepLimit,nextHash);
				thisIExpectedValue += double (tempScore * finListCnt[f]) / darkChessCnt;
			}
			// cout<<"pos: "<<lst.mov[i].st<<",thisIExpectedValue:"<<thisIExpectedValue<<", useTime:"<<((clock() - Tick)/CLOCKS_PER_SEC)<<endl;
			if(thisIExpectedValue>bestExpectedValue){
				bestExpectedValue = thisIExpectedValue;
				bestExpectedMOV = lst.mov[i];
			}
			if(bestExpectedValue==WIN)
				break;
		}
		if(bestExpectedValue==WIN)
			break;

		if(TimesUp()){//If remaining time cannot do another deeper search than return best
			cout<<"TimesUp: "<< clock()<<", "<<Tick<<", "<<TimeOut<<", "<<((clock() - Tick))<<endl;
			break;
		}
		
	}
	flipPos = bestExpectedMOV;
	cout<<"pos: "<<bestExpectedMOV.st<<",thisIExpectedValue:"<<bestExpectedValue<<", useTime:"<<((clock() - Tick)/CLOCKS_PER_SEC)<<endl;
	if(bestExpectedValue>movScore)
		return (SCORE)bestExpectedValue;
	else
		return 0;
	// return 0;
}

int getMoreChessThenOpCnt(const BOARD &B){
	int cnt[PLAYERCNT] = {0,0};
	for(POS p=0;p<BOARDPLACE;p++){//盤面上
		const CLR c = GetColor(B.fin[p]);
		if(c!=-1){
			cnt[c] ++;
		}
	}
	for(int i=0;i<7;i++){//未翻開
		cnt[0] ++;
	}
	for(int i=7;i<TYPEOFFIN;i++){
		cnt[1] ++;
	}

	return cnt[B.who] - cnt[B.who^1];
}

void checkEndGame(const BOARD &B){
	if(isEndGame){
		return;
	}
	if(darkChessCnt==0 || darkChessCnt+brightChessCnt<=10 || getMoreChessThenOpCnt(B)>=7) {
		isEndGame=true;
		cout<<"Enter end game mode~~~"<<endl;
		return;
	}

}

MOV Play(const BOARD &B, const unsigned long long int & nowHash) {
#ifdef _WINDOWS
	Tick=GetTickCount();
	TimeOut = (remain_time/(totalPly-nowRound+1)) / 1000*1000;
	cout<<"Old timeout:"<<TimeOut<<endl;
	if(totalPly<=nowRound+5){
		totalPly += 5;
	}
	if(TimeOut>50000){//超過50秒
		TimeOut = 40*1000;
		totalPly += 5;
	}
	else if(TimeOut<=0)
		TimeOut = 3*1000;//想3秒
	if(remain_time<80000)//只剩下80秒
	{
		TimeOut = 3*1000;//想3秒
	}else if(remain_time<50000){
		TimeOut = 2*1000;//想2秒
	}
	else if(remain_time<20000)//只剩下20秒
	{
		TimeOut = 1*1000;//想1秒
	}
#else
	Tick=clock();
	// TimeOut = (DEFAULTTIME-3)*CLOCKS_PER_SEC;
	TimeOut = (remain_time/(totalPly-nowRound+1)) / 1000*CLOCKS_PER_SEC;
	if(totalPly<=nowRound+5){
		totalPly += 5;
	}
	if(TimeOut>50*CLOCKS_PER_SEC){//超過50秒
		TimeOut = 40*CLOCKS_PER_SEC;
		totalPly += 5;
	}
	else if(TimeOut<=0)
		TimeOut = 3*CLOCKS_PER_SEC;//想3秒
	if(remain_time<80000)//只剩下80秒
	{
		TimeOut = 3*CLOCKS_PER_SEC;//想3秒
	}else if(remain_time<50000)//只剩下50秒
	{
		TimeOut = 2*CLOCKS_PER_SEC;//想2秒
	}else if(remain_time<20000)//只剩下20秒
	{
		TimeOut = 1*CLOCKS_PER_SEC;//想1秒
	}
#endif
	//Time Control
	
	cout<<"TimeOut(s):"<<TimeOut/CLOCKS_PER_SEC<<endl;
	cout<<"darkChessCnt: "<<darkChessCnt<<", brightChessCnt:"<<brightChessCnt<<endl;
	cout<<"Not kill or flip:"<<B.visitcnt<<endl;
	POS p; int c=0;

	// 新遊戲？隨機翻子
	POS corner[4] = {0, 3, 28, 31};
	if(B.who==-1){p=corner[rand()%4];printf("%d\n",p);return MOV(p,p);}

	MOVLST lst;
	B.MoveGenWithCapFirst(lst);
	checkEndGame(B);
	cout<<"End game:"<<(isEndGame?"true":"false")<<endl;
		// initial best move
	BestMove = lst.mov[0];
	cout<<"Search Nega...len:"<<lst.num<<endl;
	if(lst.num>1 || lst.mov[0].st==-1){
		// Iterative Deepening
		int IterDepth;
			// 當合法步數小於10 深度減少
		if (lst.num < 5)
			IterDepth=17;
		else if (lst.num < 10)
			IterDepth=13;
		else
			IterDepth=11;

		SCORE movScore = IDAS(B,nowHash,IterDepth);
		MOV movMov = BestMove;
		cout<<"Mov:("<<movMov.st<<","<<movMov.ed<<")"<<endl;
		SCORE flipScore = IDAS_flip(B,nowHash,movScore);
		MOV flipMov = flipPos;
		cout<<"flipMov:("<<flipMov.st<<","<<flipMov.ed<<")"<<endl;
		if(flipScore>0 || movMov.st==-1){

			cout<<"FLip!"<<endl;
			return flipMov;
		}
		else
			return movMov;
	}
	return BestMove;
}

FIN type2fin(int type) {
    switch(type) {
	case  1: return FIN_K;
	case  2: return FIN_G;
	case  3: return FIN_M;
	case  4: return FIN_R;
	case  5: return FIN_N;
	case  6: return FIN_C;
	case  7: return FIN_P;
	case  9: return FIN_k;
	case 10: return FIN_g;
	case 11: return FIN_m;
	case 12: return FIN_r;
	case 13: return FIN_n;
	case 14: return FIN_c;
	case 15: return FIN_p;
	default: return FIN_E;
    }
}
FIN chess2fin(char chess) {
    switch (chess) {
	case 'K': return FIN_K;
	case 'G': return FIN_G;
	case 'M': return FIN_M;
	case 'R': return FIN_R;
	case 'N': return FIN_N;
	case 'C': return FIN_C;
	case 'P': return FIN_P;
	case 'k': return FIN_k;
	case 'g': return FIN_g;
	case 'm': return FIN_m;
	case 'r': return FIN_r;
	case 'n': return FIN_n;
	case 'c': return FIN_c;
	case 'p': return FIN_p;
	default: return FIN_E;
    }
}
void initialHash(unsigned long long int & startHash){
	for(int j=0;j<BOARDPLACE;j++){
		startHash ^= randFin[FIN_X][j];
	}
}

int main(int argc, char* argv[]) {

#ifdef _WINDOWS
	srand(Tick=GetTickCount());
#else
	srand(Tick=time(NULL));
#endif
	nowRound = 0;
	init_hashSeed();
	// hashTable.init();
	unsigned long long int nowHash = 0;
	initialHash(nowHash);

	BOARD B;
	if (argc<=1) {
	    TimeOut=(B.LoadGame("board.txt")-3)*1000;
	    if(!B.ChkLose())Output(Play(B,nowHash));
	    return 0;
	}
	Protocol *protocol;
	protocol = new Protocol();
	protocol->init_protocol(argv[argc-2],atoi(argv[argc-1]));
	int iPieceCount[14];
	char iCurrentPosition[32];
	int type;
	bool turn;
	PROTO_CLR color;

	char src[3], dst[3], mov[6];
	History moveRecord;
	protocol->init_board(iPieceCount, iCurrentPosition, moveRecord, remain_time);
	protocol->get_turn(turn,color);

	TimeOut = (DEFAULTTIME-3)*1000;

	B.Init(iCurrentPosition, iPieceCount, (color==2)?(-1):(int)color);

	MOV m;
	if(turn) // 我先
	{
		nowRound++;
	    m = Play(B,nowHash);
	    sprintf(src, "%c%c",(m.st%4)+'a', m.st/4+'1');
	    sprintf(dst, "%c%c",(m.ed%4)+'a', m.ed/4+'1');
	    protocol->send(src, dst);
	    protocol->recv(mov, remain_time);
	    if( color == 2)
		color = protocol->get_color(mov);
	    B.who = color;
	    nowHash ^= randTurn[B.who];
	    B.DoMove(m, chess2fin(mov[3]),nowHash,darkChessCnt,brightChessCnt);
	    hashTable.updateVisit(nowHash);
	    protocol->recv(mov, remain_time);
	    m.st = mov[0] - 'a' + (mov[1] - '1')*4;
	    m.ed = (mov[2]=='(')?m.st:(mov[3] - 'a' + (mov[4] - '1')*4);
	    B.DoMove(m, chess2fin(mov[3]),nowHash,darkChessCnt,brightChessCnt);
	    hashTable.updateVisit(nowHash);
	}
	else // 對方先
	{
	    protocol->recv(mov, remain_time);
	    if( color == 2)
	    {
		color = protocol->get_color(mov);
		B.who = color;
	    }
	    else {
		B.who = color;
		B.who^=1;
	    }
	    m.st = mov[0] - 'a' + (mov[1] - '1')*4;
	    m.ed = (mov[2]=='(')?m.st:(mov[3] - 'a' + (mov[4] - '1')*4);
	    nowHash ^= randTurn[B.who];
	    B.DoMove(m, chess2fin(mov[3]),nowHash,darkChessCnt,brightChessCnt);
	    hashTable.updateVisit(nowHash);
	}
	B.Display();
	while(1)
	{
		cout<<"Play searching..."<<endl;
		nowRound++;
	    m = Play(B,nowHash);
	    sprintf(src, "%c%c",(m.st%4)+'a', m.st/4+'1');
	    sprintf(dst, "%c%c",(m.ed%4)+'a', m.ed/4+'1');
	    protocol->send(src, dst);
	    cout<<"remain_time..."<<remain_time<<endl;
	    protocol->recv(mov, remain_time);
	    m.st = mov[0] - 'a' + (mov[1] - '1')*4;
	    m.ed = (mov[2]=='(')?m.st:(mov[3] - 'a' + (mov[4] - '1')*4);
	    B.DoMove(m, chess2fin(mov[3]),nowHash,darkChessCnt,brightChessCnt);
	    hashTable.updateVisit(nowHash);
	    B.Display();

	    protocol->recv(mov, remain_time);
	    m.st = mov[0] - 'a' + (mov[1] - '1')*4;
	    m.ed = (mov[2]=='(')?m.st:(mov[3] - 'a' + (mov[4] - '1')*4);
	    B.DoMove(m, chess2fin(mov[3]),nowHash,darkChessCnt,brightChessCnt);
	    hashTable.updateVisit(nowHash);
	    B.Display();
	}

	return 0;
}
