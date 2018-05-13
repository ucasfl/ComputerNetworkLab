#ifndef _PREFIX_TREE_H
#define _PREFIX_TREE_H
#include<iostream>
#include<fstream>
#include<cstdio>
#include<string>
#include<cstdlib>
#include<ctime>

#define IP_MAXLEN 16
#define BIT_NUM 32
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct Leaf{
	u32 SubNet;
	u8 PreLen;
	u8 Port;
	void dumpLeaf(){
		u32 ip = SubNet;
		u8 ip1 = ip / (256*256*256);
		ip %= (256*256*256);
		u8 ip2 = ip / (256*256);
		ip %= (256*256);
		u8 ip3 = ip / 256;
		u8 ip4 = ip % 256;
		printf("Subnet: %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
		printf("Prefix Len: %d\n", PreLen);
		printf("Port: %d\n", Port);
	};
};

//Single bite prefix tree
class PTree{
	public :
		u32 SubNet;
		u8 PreLen;
		u8 Port;
		PTree(){
			SubNet = 0;
			PreLen = 0;
			Port = 0;
			Child[0] = Child[1] = NULL;
		};
		u32 StrToIp(const char *s);
		virtual bool ConstructTree();
		bool Insert(PTree *Node);
		bool Pleafpush(PTree *Node);
		PTree *Search(u32 IP);
		virtual bool DestructTree();
		virtual ~PTree(){
		};
		void dumpNode();
	private :
		PTree *Child[2];
};

// double bite prefix tree
class MulPTree : public PTree{
	public :
		MulPTree *mChild[4];
		MulPTree(){
			SubNet = 0;
			PreLen = 0;
			Port = 0;
			mChild[0] = mChild[1] = mChild[2] = mChild[3] = NULL;
		};
		virtual bool ConstructTree();
		bool Insert(MulPTree *Node);
		MulPTree *Search(u32 IP);
		bool MleafPush(MulPTree *Node);
		bool Equal(PTree *Node){
			return Node->SubNet == SubNet && Node->PreLen == PreLen && Node->Port == Port;
		}
		bool Equal(Leaf *leaf){
			return SubNet == leaf->SubNet && PreLen == leaf->PreLen && Port == leaf->Port;
		};
		virtual bool isLeaf(){
			return !mChild[0] && !mChild[1] && !mChild[2] && !mChild[3];
		}
		virtual bool DestructTree();
		virtual ~MulPTree(){
		};
};

class CVTree{
	public :
		bool NotLeaf[4];
		Leaf *LeafVec;
		CVTree *ChildVec;
		CVTree(){
			NotLeaf[0] = NotLeaf[1] = NotLeaf[2] = NotLeaf[3] = 0;
			LeafVec = NULL;
			ChildVec = NULL;
		}
		bool ConstructCVTree(MulPTree *Tree);
		// bool CompressVec();
		Leaf *Search(u32 IP);
		u8 Count(u8 loc);
		bool DestructCVTree();
		virtual ~CVTree(){
			if (LeafVec){
				delete[] LeafVec;
			}
		};
};

#endif
