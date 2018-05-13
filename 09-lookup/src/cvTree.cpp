#include "prefixTree.h"

bool CVTree::ConstructCVTree(MulPTree *Tree){
	u8 haveChild = 0;
	u8 haveLeaf = 0;
	for(u8 i = 0; i != 4; ++i){
		if (Tree->mChild[i] && !Tree->mChild[i]->isLeaf()){
			NotLeaf[i] = true;
			++haveChild;
		}
		if (Tree->mChild[i] && Tree->mChild[i]->isLeaf()){
			++haveLeaf;
		}
	}
	if (haveLeaf + haveChild != 4){
		return false;
	}
	if (haveLeaf){
		LeafVec = new Leaf[haveLeaf];
		for (u8 i = 0, j = 0; i != 4 && j != haveLeaf; ++i){
			if (Tree->mChild[i]&& Tree->mChild[i]->isLeaf()){
				LeafVec[j].SubNet = Tree->mChild[i]->SubNet;
				LeafVec[j].PreLen = Tree->mChild[i]->PreLen;
				LeafVec[j].Port = Tree->mChild[i]->Port;
				++j;
			}
		}
	}
	if (haveChild){
		ChildVec = new CVTree[haveChild];
		for (u8 i = 0, j = 0; i != 4 && j != haveChild; ++i){
			if (Tree->mChild[i] && !Tree->mChild[i]->isLeaf()){
				ChildVec[j].ConstructCVTree(Tree->mChild[i]);
				++j;
			}
		}
	}
	return true;
}

#if 0
bool CVTree::CompressVec(){

}
#endif

Leaf *CVTree::Search(u32 IP){
	CVTree *pos = this;
	u32 tmpIP = IP;
	u8 loc = (tmpIP &0xc0000000) >> 30;
	while (pos->NotLeaf[loc]){
		pos = &pos->ChildVec[pos->Count(loc)];
		tmpIP <<= 2;
		loc = (tmpIP & 0xc0000000) >> 30;
	}
	u32 subnet = pos->LeafVec[loc-pos->Count(loc)].SubNet;
	u8 prelen = pos->LeafVec[loc-pos->Count(loc)].PreLen;
	for(u8 i = 1; i != prelen + 1; ++i){
		if (subnet >> (32 -i) != IP >> (32 -i)){
			Leaf *leaf = new Leaf();
			return leaf;
		}
	}
#if 0
	if (pos->LeafVec[loc-pos->Count(loc)].SubNet != IP){
	}
#endif
	return &pos->LeafVec[loc - pos->Count(loc) ];
}

u8 CVTree::Count(u8 loc){
	u8 num = 0;
	for (u8 i = 0; i != loc && i != 4; ++i){
		num += NotLeaf[i];
	}
	return num;
}

bool CVTree::DestructCVTree(){
	if (ChildVec){
		for (u8 i = 0; i != this->Count(4); ++i){
			ChildVec[i].DestructCVTree();
		}
		delete[] ChildVec;
	}
	// delete this;
	return true;
}

