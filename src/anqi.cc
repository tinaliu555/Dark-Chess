/*****************************************************************************\
 * Theory of Computer Games: Fall 2013
 * Chinese Dark Chess Library by You-cheng Syu
 *
 * This file may not be used out of the class unless asking
 * for permission first.
 *
 * Modify by Hung-Jui Chang, December 2013
\*****************************************************************************/
#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<iostream>
#include"anqi.hh"
#ifdef _WINDOWS
#include<windows.h>
#endif
using namespace std;

static const char *tbl="KGMRNCPkgmrncpX-";

static const char *nam[16]={
	"帥","仕","相","硨","傌","炮","兵",
	"將","士","象","車","馬","砲","卒",
	"Ｏ","　"
};

// static const char *nam[16]={
// 	"\033[1;35m帥\033[m","\033[1;35m仕\033[m","\033[1;35m相\033[m","\033[1;35m硨\033[m","\033[1;35m傌\033[m","\033[1;35m炮\033[m","\033[1;35m兵\033[m",
// 	"\033[1;36m將\033[m","\033[1;36m士\033[m","\033[1;36m象\033[m","\033[1;36m車\033[m","\033[1;36m馬\033[m","\033[1;36m砲\033[m","\033[1;36m卒\033[m",
// 	"Ｏ","　"
// };

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

// int sortScoreCmp(const void* a, const void* b) {
// 	MOV lhs = *(MOV*)a, rhs=*(MOV*)b;
// 	if(lhs.score==rhs.score) return 0;
// 	return lhs.score>rhs.score?-1:1;
// }

CLR GetColor(FIN f) {
	return f<FIN_X?f/7:-1;
}

LVL GetLevel(FIN f) {
	assert(f<FIN_X);
	return LVL(f%7);
}

bool ChkEats(FIN fa,FIN fb) {
	if(fa>=FIN_X)return false;
	if(fb==FIN_X)return false;
	if(fb==FIN_E)return true ;
	if(GetColor(fb)==GetColor(fa))return false;

	const LVL la=GetLevel(fa);
	if(la==LVL_C)return true ;

	const LVL lb=GetLevel(fb);
	if(la==LVL_K)return lb!=LVL_P;
	if(la==LVL_P)return lb==LVL_P||lb==LVL_K;

	return la<=lb;
}

static void Output(FILE *fp,POS p) {
	fprintf(fp,"%c%d\n",'a'+p%4,8-(p/4));
}

void Output(MOV m) {
	FILE *fp=fopen("move.txt","w");
	assert(fp!=NULL);
	if(m.ed!=m.st) {
		fputs("0\n",fp);
		Output(fp,m.st);
		Output(fp,m.ed);
		fputs("0\n",fp);
	} else {
		fputs("1\n",fp);
		Output(fp,m.st);
		fputs("0\n",fp);
		fputs("0\n",fp);
	}
	fclose(fp);
}

void BOARD::NewGame() {
	static const int tbl[]={1,2,2,2,2,2,5};
	who=-1;
	for(POS p=0;p<32;p++)fin[p]=FIN_X;
	for(int i=0;i<14;i++)cnt[i]=tbl[GetLevel(FIN(i))];
}

static FIN find(char c) {
	return FIN(strchr(tbl,c)-tbl);
}

static POS LoadGameConv(const char *s) {
	return (8-(s[1]-'0'))*4+(s[0]-'a');
}

static void LoadGameReplay(BOARD &brd,const char *cmd) {
	if(cmd[2]!='-')brd.Flip(LoadGameConv(cmd),find(cmd[3]));
	else{
		unsigned long long int temp = 0;
		brd.Move(MOV(LoadGameConv(cmd),LoadGameConv(cmd+3)),temp);//hash = 0
	} 
}

static POS mkpos(int x,int y) {
	return x*4+y;
}

// void BOARD::Init(int Board[32], int Piece[14], int Color) {
//     for (int i = 0 ; i < 14; ++i) {
// 	cnt[i] = Piece[i];
//     }
//     for (int i = 0 ; i < 32; ++i) {
// 	switch(Board[i]) {
// 	    case  0: fin[i] = FIN_E;break;
// 	    case  1: fin[i] = FIN_K;cnt[FIN_K]--;break;
// 	    case  2: fin[i] = FIN_G;cnt[FIN_G]--;break;
// 	    case  3: fin[i] = FIN_M;cnt[FIN_M]--;break;
// 	    case  4: fin[i] = FIN_R;cnt[FIN_R]--;break;
// 	    case  5: fin[i] = FIN_N;cnt[FIN_N]--;break;
// 	    case  6: fin[i] = FIN_C;cnt[FIN_C]--;break;
// 	    case  7: fin[i] = FIN_P;cnt[FIN_P]--;break;
// 	    case  8: fin[i] = FIN_X;break;
// 	    case  9: fin[i] = FIN_k;cnt[FIN_k]--;break;
// 	    case 10: fin[i] = FIN_g;cnt[FIN_g]--;break;
// 	    case 11: fin[i] = FIN_m;cnt[FIN_m]--;break;
// 	    case 12: fin[i] = FIN_r;cnt[FIN_r]--;break;
// 	    case 13: fin[i] = FIN_n;cnt[FIN_n]--;break;
// 	    case 14: fin[i] = FIN_c;cnt[FIN_c]--;break;
// 	    case 15: fin[i] = FIN_p;cnt[FIN_p]--;break;
// 	}
//     }
//     who = Color;
// }

void BOARD::Init(char Board[32], int Piece[14], int Color) {

	visitcnt = 0;
    for (int i = 0 ; i < 14; ++i) {
	cnt[i] = Piece[i];
    }
    for (int i = 0 ; i < 32; ++i) {
		int p = (7-i/4)*4+i%4;
	switch(Board[i]) {
	    case '-': fin[p] = FIN_E;break;
	    case 'K': fin[p] = FIN_K;cnt[FIN_K]--;break;
	    case 'G': fin[p] = FIN_G;cnt[FIN_G]--;break;
	    case 'M': fin[p] = FIN_M;cnt[FIN_M]--;break;
	    case 'R': fin[p] = FIN_R;cnt[FIN_R]--;break;
	    case 'N': fin[p] = FIN_N;cnt[FIN_N]--;break;
	    case 'C': fin[p] = FIN_C;cnt[FIN_C]--;break;
	    case 'P': fin[p] = FIN_P;cnt[FIN_P]--;break;
	    case 'X': fin[p] = FIN_X;break;
	    case 'k': fin[p] = FIN_k;cnt[FIN_k]--;break;
	    case 'g': fin[p] = FIN_g;cnt[FIN_g]--;break;
	    case 'm': fin[p] = FIN_m;cnt[FIN_m]--;break;
	    case 'r': fin[p] = FIN_r;cnt[FIN_r]--;break;
	    case 'n': fin[p] = FIN_n;cnt[FIN_n]--;break;
	    case 'c': fin[p] = FIN_c;cnt[FIN_c]--;break;
	    case 'p': fin[p] = FIN_p;cnt[FIN_p]--;break;
	}
    }
    who = Color;
}

int BOARD::LoadGame(const char *fn) {
	FILE *fp=fopen(fn,"r");
	assert(fp!=NULL);

	while(fgetc(fp)!='\n');

	while(fgetc(fp)!='\n');

	fscanf(fp," %*c");
	for(int i=0;i<14;i++)fscanf(fp,"%d",cnt+i);

	for(int i=0;i<8;i++) {
		fscanf(fp," %*c");
		for(int j=0;j<4;j++) {
			char c;
			fscanf(fp," %c",&c);
			fin[mkpos(i,j)]=find(c);
		}
	}

	int r;
	fscanf(fp," %*c%*s%d" ,&r);
	who=(r==0||r==1?r:-1);
	fscanf(fp," %*c%*s%d ",&r);

	for(char buf[64];fgets(buf,sizeof(buf),fp);) {
		if(buf[2]<'0'||buf[2]>'9')break;
		char xxx[16],yyy[16];
		const int n=sscanf(buf+2,"%*s%s%s",xxx,yyy);
		if(n>=1)LoadGameReplay(*this,xxx);
		if(n>=2)LoadGameReplay(*this,yyy);
	}

	fclose(fp);
	return r;
}

void BOARD::Display() const {
#ifdef _WINDOWS
	HANDLE hErr=GetStdHandle(STD_ERROR_HANDLE);
#endif
	for(int i=0;i<8;i++) {
#ifdef _WINDOWS
		SetConsoleTextAttribute(hErr,8);
#endif
		for(int j=0;j<4;j++)fprintf(stderr,"[%02d]",mkpos(i,j));
		if(i==2) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,12);
#endif
			fputs("  ",stderr);
			for(int j=0;j<7;j++)for(int k=0;k<cnt[j];k++)fputs(nam[j],stderr);
		}
		fputc('\n',stderr);
		for(int j=0;j<4;j++) {
			const FIN f=fin[mkpos(i,j)];
			const CLR c=GetColor(f);
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,(c!=-1?12-c*2:7));
#endif
			fprintf(stderr," %s ",nam[fin[mkpos(i,j)]]);
		}
		if(i==0) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,7);
#endif
			fputs("  輪到 ",stderr);
			if(who==0) {
#ifdef _WINDOWS
				SetConsoleTextAttribute(hErr,12);
#endif
				fputs("紅方",stderr);
			} else if(who==1) {
#ifdef _WINDOWS
				SetConsoleTextAttribute(hErr,10);
#endif
				fputs("黑方",stderr);
			} else {
				fputs("？？",stderr);
			}
		} else if(i==1) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,7);
#endif
			fputs("  尚未翻出：",stderr);
		} else if(i==2) {
#ifdef _WINDOWS
			SetConsoleTextAttribute(hErr,10);
#endif
			fputs("  ",stderr);
			for(int j=7;j<14;j++)for(int k=0;k<cnt[j];k++)fputs(nam[j],stderr);
		}
		fputc('\n',stderr);
	}
#ifdef _WINDOWS
	SetConsoleTextAttribute(hErr,7);
#endif
}

int BOARD::MoveGen(MOVLST &lst) const {
	if(who==-1)return false;
	lst.num=0;
	for(POS p=0;p<32;p++) {
		const FIN pf=fin[p];
		if(GetColor(pf)!=who)continue;
		const LVL pl=GetLevel(pf);
		for(int z=0;z<4;z++) {
			const POS q=ADJ[p][z];
			if(q==-1)continue;
			const FIN qf=fin[q];
			if(pl!=LVL_C){if(!ChkEats(pf,qf))continue;}
			else if(qf!=FIN_E)continue;
			lst.mov[lst.num++]=MOV(p,q);
		}
		if(pl!=LVL_C)continue;
		for(int z=0;z<4;z++) {
			int c=0;
			for(POS q=p;(q=ADJ[q][z])!=-1;) {
				const FIN qf=fin[q];
				if(qf==FIN_E||++c!=2)continue;
				if(qf!=FIN_X&&GetColor(qf)!=who)lst.mov[lst.num++]=MOV(p,q);
				break;
			}
		}
	}
	return lst.num;
}

int BOARD::CapMoveGen(MOVLST &lst) const {
	if(who==-1)return false;
	lst.num=0;
	for(POS p=0;p<32;p++) {
		const FIN pf=fin[p];
		if(GetColor(pf)!=who)continue;
		const LVL pl=GetLevel(pf);
		for(int z=0;z<4;z++) {
			const POS q=ADJ[p][z];
			if(q==-1)continue;
			const FIN qf=fin[q];
			if(pl!=LVL_C){if(!ChkEats(pf,qf))continue;}
			else if(qf!=FIN_E)continue;//是砲但是目的地有子
			if(qf!=FIN_E)
				lst.mov[lst.num++]=MOV(p,q);
		}
		if(pl!=LVL_C)continue;
		for(int z=0;z<4;z++) {
			int c=0;
			for(POS q=p;(q=ADJ[q][z])!=-1;) {
				const FIN qf=fin[q];
				if(qf==FIN_E||++c!=2)continue;
				if(qf!=FIN_X&&GetColor(qf)!=who){//砲吃子
					lst.mov[lst.num++]=MOV(p,q);
				}
				break;
			}
		}
	}
	return lst.num;
}


int BOARD::MoveGenWithCapFirst(MOVLST &lst) const {
	if(who==-1)return false;
	MOVLST tmpList;
	tmpList.num = 0;
	lst.num=0;
	for(POS p=0;p<32;p++) {
		const FIN pf=fin[p];
		if(GetColor(pf)!=who)continue;
		const LVL pl=GetLevel(pf);
		for(int z=0;z<4;z++) {
			const POS q=ADJ[p][z];
			if(q==-1)continue;
			const FIN qf=fin[q];
			if(pl!=LVL_C){if(!ChkEats(pf,qf))continue;}
			else if(qf!=FIN_E)continue;//是砲但是目的地有子
			if(qf!=FIN_E)
				lst.mov[lst.num++]=MOV(p,q);
			else
				tmpList.mov[tmpList.num++]=MOV(p,q);
		}
		if(pl!=LVL_C)continue;
		for(int z=0;z<4;z++) {
			int c=0;
			for(POS q=p;(q=ADJ[q][z])!=-1;) {
				const FIN qf=fin[q];
				if(qf==FIN_E||++c!=2)continue;
				if(qf!=FIN_X&&GetColor(qf)!=who){//砲吃子
					lst.mov[lst.num++]=MOV(p,q);
				}
				break;
			}
		}
	}
	int c = 0;
	for(int f=0;f<14;f++){
		c += cnt[f];
	}
	if(c>0)
		lst.mov[lst.num++] = MOV(-1,-1);
	for(int i=0;i<tmpList.num;i++){
		lst.mov[lst.num++] = tmpList.mov[i];
	}
	return lst.num;
}

int BOARD::MoveGenWithCapFirstAndFlip(MOVLST &lst, const int & darkChessCnt) const {
	if(who==-1)return false;
	MoveGenWithCapFirst(lst);
	// if(darkChessCnt>10)//不用chance search
	// 	return lst.num;
	if(darkChessCnt>0){
		for(POS p=0;p<32;p++){
			if(fin[p]==FIN_X)
				lst.mov[lst.num++]=MOV(p,p);
		}
	}
	return lst.num;
}

bool BOARD::ChkLose() const {
	if(who==-1)return false;

	bool fDark=false;
	for(int i=0;i<14;i++) {
		if(cnt[i]==0)continue;
		if(GetColor(FIN(i))==who)return false;
		fDark=true;
	}

	bool fLive=false;
	for(POS p=0;p<32;p++)if(GetColor(fin[p])==who){fLive=true;break;}
	if(!fLive)return true;

	MOVLST lst;
	return !fDark&&MoveGen(lst)==0;
}

bool BOARD::ChkValid(MOV m) const {
	if(m.ed!=m.st) {
		MOVLST lst;
		MoveGen(lst);
		for(int i=0;i<lst.num;i++)if(m==lst.mov[i])return true;
	} else {
		if(m.st<0||m.st>=32)return false;
		if(fin[m.st]!=FIN_X)return false;
		for(int i=0;i<14;i++)if(cnt[i]!=0)return true;
	}
	return false;
}

void BOARD::Flip(POS p,FIN f) {
	if(f==FIN_X) {
		int i,sum=0;
		for(i=0;i<14;i++)    sum+=cnt[i];
		sum=rand()%sum;
		for(i=0;i<14;i++)if((sum-=cnt[i])<0)break;
		f=FIN(i);
	}
	fin[p]=f;
	cnt[f]--;
	if(who==-1)who=GetColor(f);
	who^=1;
}


void BOARD::Move(MOV m, unsigned long long int & nowHash, FIN f) {
	visitcnt++;
	if(m.ed!=m.st) {
		if(fin[m.ed]!=FIN_E)
			visitcnt = 0;
		nowHash ^= randTurn[who];//remove "who" do move
    	nowHash ^= randFin[fin[m.ed]][m.ed];//remove the end place
		fin[m.ed]=fin[m.st];
		nowHash ^= randFin[fin[m.ed]][m.ed];//add the end place
		nowHash ^= randFin[fin[m.st]][m.st];//remove the start place
		fin[m.st]=FIN_E;
		nowHash ^= randFin[fin[m.st]][m.st];//add the start place
		who^=1;
		nowHash ^= randTurn[who];//add next "who"
	} else {
		visitcnt = 0;
		if(m.st==-1){
			nowHash ^= randTurn[who];//remove "who" do move
			who^=1;
			// Flip(m.st, f);
			// nowHash ^= randFin[fin[m.st]][m.st];//add the flip place
			nowHash ^= randTurn[who];//add next "who"
		}else{
			nowHash ^= randTurn[who];//remove "who" do move
			nowHash ^= randFin[fin[m.st]][m.st];//remove the flip place
			Flip(m.st, f);
			nowHash ^= randFin[fin[m.st]][m.st];//add the flip place
			nowHash ^= randTurn[who];//add next "who"
		}
		
	}
	
}

void BOARD::DoMove(MOV m, FIN f, unsigned long long int & nowHash, int & darkChessCnt, int & brightChessCnt) {
	visitcnt++;
    if (m.ed!=m.st) {
    	nowHash ^= randTurn[who];//remove "who" do move
    	if(fin[m.ed]<FIN_X){
    		brightChessCnt--;
    	}
    	if(fin[m.ed]!=FIN_E)
			visitcnt = 0;
    	nowHash ^= randFin[fin[m.ed]][m.ed];//remove the end place
		fin[m.ed]=fin[m.st];
		nowHash ^= randFin[fin[m.ed]][m.ed];//add the end place
		nowHash ^= randFin[fin[m.st]][m.st];//remove the start place
		fin[m.st]=FIN_E;
		nowHash ^= randFin[fin[m.st]][m.st];//add the start place
		who^=1;
		nowHash ^= randTurn[who];//add next "who"
    }
    else {
    	visitcnt = 0;
    	nowHash ^= randTurn[who];//remove "who" do move
    	nowHash ^= randFin[fin[m.st]][m.st];//remove the flip place
		Flip(m.st, f);
		darkChessCnt --;
		brightChessCnt++;
		nowHash ^= randFin[fin[m.st]][m.st];//add the flip place
		nowHash ^= randTurn[who];//add next "who"
    }
    
}
