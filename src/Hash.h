#ifndef HASH_H
#define HASH_H

#include<iostream>
#include"anqi.hh"

using namespace std;

#define HASHSIZE 0xFFFFFFF//28 bit
#define HASHEACHSIZE 4

class HashNode{
public:
	int depth[HASHEACHSIZE];
	int exact[HASHEACHSIZE];//0:empty; 1:exact;2: upper bound,3:lower bound
	int value[HASHEACHSIZE];
	int visit[HASHEACHSIZE];
	MOV bestMov[HASHEACHSIZE];
	unsigned long long int hashKey[HASHEACHSIZE];

};


class Hash{
public:
	
	// unsigned long long int hashSize;
	HashNode *hashTable;
	Hash(){
		// hashSize = HASHSIZE;
		// hashTable = (HashNode *) malloc(HASHSIZE* sizeof(HashNode));
		hashTable = new HashNode[HASHSIZE+1];
		
	}
	~Hash(){
		delete [] hashTable;
		// free(hashTable);
	}
	// void clearCollisionHash(const unsigned long long int hashIndex){
	// 	hashTable[hashIndex].depth[0] = 0;
	// 	hashTable[hashIndex].exact[0] = 0;
	// }
	int getIsAvailable(const unsigned long long int nowHash, const unsigned long long int hashIndex, int &index){
		// cout<<"getIsAvailable"<<endl;
		// cout<<"[getIsAvailable] "<<hashIndex<<", "<<hashTable[hashIndex].hashKey[0]<<", "<<hashTable[hashIndex].hashKey[1]<<", "<<hashTable[hashIndex].hashKey[2]<<", "<<hashTable[hashIndex].hashKey[3]<<", "<<nowHash<<endl;
		for(int i=0;i<HASHEACHSIZE;i++){
			if(hashTable[hashIndex].exact[i]==0)//no record
				return 0;
			if(hashTable[hashIndex].hashKey[i]==nowHash){//find record
				index = i;
				// cout<<"Find the record! "<<hashTable[hashIndex].exact[i]<<endl;
				return hashTable[hashIndex].exact[i];
			}
		}
		// if(hashTable[hashIndex].exact[2]!=0){
			cout<<"**********Hash Crash with same key! "<<hashIndex<<", "<<hashTable[hashIndex].hashKey[0]<<", "<<hashTable[hashIndex].hashKey[1]<<", "<<hashTable[hashIndex].hashKey[2]<<", "<<nowHash<<endl;
		// }
		return 0;
	}
	MOV getBestMove(const unsigned long long int hashIndex,const int index){
		return hashTable[hashIndex].bestMov[index];
	}
	int getDepth(const unsigned long long int hashIndex,const int index){
		return hashTable[hashIndex].depth[index];
	}
	int getValue(const unsigned long long int hashIndex,const int index){
		return hashTable[hashIndex].value[index];
	}
	void updateVisit(const unsigned long long int newhash){
		unsigned long long int hashIndex = newhash & HASHSIZE;
		for(int i=0;i<HASHEACHSIZE;i++){
			if(hashTable[hashIndex].hashKey[i]==newhash){//find record
				hashTable[hashIndex].visit[i] ++;
				return;
			}
		}
		cout<<"[updateVisit] Not found the hash record! "<<hashIndex<<", "<<hashTable[hashIndex].hashKey[0]<<", "<<hashTable[hashIndex].hashKey[1]<<", "<<hashTable[hashIndex].hashKey[2]<<", "<<newhash<<endl;
	}
	int checkVisit(const unsigned long long int newhash){
		unsigned long long int hashIndex = newhash & HASHSIZE;
		for(int i=0;i<HASHEACHSIZE;i++){
			if(hashTable[hashIndex].hashKey[i]==newhash){//find record
				return hashTable[hashIndex].visit[i];
			}
		}
		cout<<"[checkVisit] Not found the hash record! "<<hashIndex<<", "<<hashTable[hashIndex].hashKey[0]<<", "<<hashTable[hashIndex].hashKey[1]<<", "<<hashTable[hashIndex].hashKey[2]<<", "<<newhash<<endl;
	}
	void insertHash(const int cut, const int Exact, const int v, const unsigned long long int newhash, const MOV bestMV ){
		unsigned long long int hashIndex = newhash & HASHSIZE;
		int index =-1,firstNoRecord = -1;
		for(int i=0;i<HASHEACHSIZE;i++){
			if(hashTable[hashIndex].hashKey[i]==newhash){//find record
				index = i;
				break;
			}
			if(hashTable[hashIndex].exact[i]==0){//no record
				firstNoRecord = i;
				break;
			}
		}
		// cout<<"[insertHash] "<<hashIndex<<", "<<hashTable[hashIndex].hashKey[0]<<", "<<hashTable[hashIndex].hashKey[1]<<", "<<hashTable[hashIndex].hashKey[2]<<", "<<hashTable[hashIndex].hashKey[3]<<", "<<newhash<<endl;
		if(index!=-1){
			if(hashTable[hashIndex].depth[index] <= cut){//Find old record and update
				// cout<<"**********Update the hash "<<hashIndex<<", "<<index<<", "<<newhash<<endl;
				hashTable[hashIndex].depth[index] = cut;
				hashTable[hashIndex].exact[index] = Exact;
				hashTable[hashIndex].value[index] = v;
				hashTable[hashIndex].hashKey[index] = newhash;
				hashTable[hashIndex].bestMov[index] = bestMV;
			}
		}else if(firstNoRecord==-1){// (&& index==-1) Insert without empty space
			cout<<"**********Insert Crash with same key! "<<hashIndex<<", "<<hashTable[hashIndex].hashKey[0]<<", "<<hashTable[hashIndex].hashKey[1]<<", "<<hashTable[hashIndex].hashKey[2]<<", "<<newhash<<endl;
			index = 0;
			hashTable[hashIndex].depth[index] = cut;
			hashTable[hashIndex].exact[index] = Exact;
			hashTable[hashIndex].value[index] = v;
			hashTable[hashIndex].hashKey[index] = newhash;
			hashTable[hashIndex].bestMov[index] = bestMV; 
		}else{//No record and insert in empty space
			index = firstNoRecord;
			hashTable[hashIndex].depth[index] = cut;
			hashTable[hashIndex].exact[index] = Exact;
			hashTable[hashIndex].value[index] = v;
			hashTable[hashIndex].hashKey[index] = newhash;
			hashTable[hashIndex].bestMov[index] = bestMV;
			// if(index>0)
			// 	cout<<"**********Insert First time of "<<hashIndex<<", "<<index<<", "<<newhash<<", "<<hashTable[hashIndex].exact[index]<<endl;
		}
		
	}
};

#endif