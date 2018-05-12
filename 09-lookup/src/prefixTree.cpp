#include "prefixTree.h"

u32 PTree::StrToIp(const char *s){
	u8 loc[4];
	u32 num = 0;
	loc[0] = 0;
	const char *tmp = s;
	while (*tmp){
		if (*tmp == '.'){
			loc[++num] = tmp - s;
		}
		++tmp;
	}
	std::string tmps(s);
	u8 ip4[4];
	ip4[0] = atoi(tmps.substr(0, loc[1]).c_str());
	for (int i = 1; i != 4; ++i){
		ip4[i] = atoi(tmps.substr(loc[i]+1, loc[i+1] - loc[i] -1).c_str());
	}
	u32 ret = 0;
	u64 base = 1;
	for (int i = 3; i >= 0; --i){
		ret += ip4[i] * base;
		base *= 256;
	}
	return ret;
}

bool PTree::ConstructTree(){
	std::ifstream infile;
	infile.open("./forwarding-table.txt");
	if (!infile){
		std::cout << "Open File Error.\n";
		return false;
	}
	std::string s;
	while(getline(infile, s)){
		u16 fb = s.find(' ');
		u16 lb = s.find_last_of(' ');
		PTree *Node = new PTree();
		Node->SubNet = Node->StrToIp(s.substr(0, fb).c_str());
		Node->PreLen = atoi(s.substr(fb+1, lb).c_str());
		Node->Port = atoi(s.substr(lb+1).c_str());
		if(!Insert(Node)){
			infile.close();
			return false;
		}
	}
	infile.close();
	return true;
}
bool PTree::Insert(PTree *Node){
	PTree *pos = this, *q, *p;
	q = p = NULL;
	u32 nodeIp = Node->SubNet;
	u8 preLen = Node->PreLen;
	bool loc;
	while (pos && preLen){
		loc = nodeIp & 0x80000000;
		q = pos;
		pos = pos->Child[loc];
		nodeIp <<= 1;
		--preLen;
	}
	if (!preLen){
		if (!pos){
			q->Child[loc] = Node;
		}
		else {
			pos->SubNet = Node->SubNet;
			pos->PreLen = Node->PreLen;
			pos->Port = Node->Port;
			delete Node;
		}
	}
	else{
		while(preLen--){
			p = new PTree();
			q->Child[loc] = p;
			q = q->Child[loc];
			loc = nodeIp & 0x80000000;
			nodeIp <<= 1;
		}
		p->Child[loc] = Node;
	}
#if 0
	else {
		pos->SubNet = Node->SubNet;
		pos->PreLen = Node->PreLen;
		pos->Port = Node->Port;
		// delete Node;
		return true;
	}
#endif
	return true;
}

PTree *PTree::Search(u32 IP){
	PTree *pos = this, *q = NULL;
	u32 tmp = IP;
	bool loc;
	while (pos){
		loc = tmp & 0x80000000;
		q = pos ;
		pos = pos->Child[loc];
		tmp <<= 1;
	}
	int bite = 0;
	for(int i = 0; i <= q->PreLen ; ++i){
		if (q->SubNet>>(31 -i) == IP >> (31 -i))
		  ++bite;
		else{
			break;
		}
	}
	if (bite < q->PreLen){
		return this;
	}
	return q;
}
bool PTree::DestructTree(){
	u8 i;
	for(i = 0; i != 2; ++i){
		if (Child[i]){
			Child[i]->DestructTree();
		}
	}
	delete this;
	return true;
}

void PTree::dumpNode(){
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
}

#if 0
void PTree::dumpTree(){
	if (Child[0] == NULL && Child[1] == NULL){
		dumpNode();
		return;
	}
	else if (Child[0] != NULL && Child[1] == NULL){
		dumpNode();
		Child[0]->dumpTree();
	}
	else if (Child[1] != NULL && Child[0] == NULL){
		dumpNode();
		Child[1]->dumpTree();
	}
	else {
		dumpNode();
		Child[0]->dumpTree();
		Child[1]->dumpTree();
	}
}
#endif

bool MulPTree::ConstructTree(){
	std::ifstream infile;
	infile.open("./forwarding-table.txt");
	if (!infile){
		std::cout << "Open File Error.\n";
		return false;
	}
	std::string s;
	while(getline(infile, s)){
		u16 fb = s.find(' ');
		u16 lb = s.find_last_of(' ');
		MulPTree *Node = new MulPTree();
		Node->SubNet = Node->StrToIp(s.substr(0, fb).c_str());
		Node->PreLen = atoi(s.substr(fb+1, lb).c_str());
		Node->Port = atoi(s.substr(lb+1).c_str());
		if( !Insert(Node) ){
			infile.close();
			return false;
		}
	}
	infile.close();
	return true;
}

bool MulPTree::Insert(MulPTree *Node){
	MulPTree *pos = this, *p, *q;
	p = q = NULL;
	u32 nodeIp = Node->SubNet;
	int preLen = Node->PreLen;
	u8 loc;
	while (pos && preLen > 1){
		loc = (nodeIp & 0xc0000000) >> 30;
		q = pos;
		pos = pos->mChild[loc];
		nodeIp <<= 2;
		preLen -= 2;
	}
	if (preLen == 0){
		if (pos){
			pos->SubNet = Node->SubNet;
			pos->PreLen = Node->PreLen;
			pos->Port = Node->Port;
			delete Node;
		}
		else {
			q->mChild[loc] = Node;
		}
	}
#if 0
	else if (preLen < 0 && !pos){
		if (loc & 0x2){
			q->mChild[2] = q->mChild[3] = Node;
		}
		else {
			q->mChild[0] = q->mChild[1] = Node;
		}
	}
#endif
	else if ( preLen > 1){
		while(preLen > 0){
			p = new MulPTree();
			q->mChild[loc] = p;
			q = q->mChild[loc];
			loc = (nodeIp & 0xc0000000) >> 30;
			nodeIp <<= 2;
			preLen -= 2;
		}
		if (preLen == 0){
			p->mChild[loc] = Node;
		}
		else {
			MulPTree *NewNode = new MulPTree();
			NewNode->SubNet = Node->SubNet;
			NewNode->PreLen = Node->PreLen;
			NewNode->Port = Node->Port;
			if (loc & 0x2){
				p->mChild[2] = Node;
				p->mChild[3] = NewNode;
			}
			else{
				p->mChild[0]  = Node;
				p->mChild[1]  = NewNode;
			}
		}
	}
	else {
		if (!pos){
			q->mChild[loc] = new MulPTree();
			pos = q->mChild[loc];
		}
		MulPTree *NewNode = new MulPTree();
		NewNode->SubNet = Node->SubNet;
		NewNode->PreLen = Node->PreLen;
		NewNode->Port = Node->Port;
		loc = (nodeIp & 0xc0000000) >> 30;
		if (loc &0x2){
			if (!pos->mChild[2]){
				pos->mChild[2] = Node;
			}
			if (!pos->mChild[3]){
				pos->mChild[3] = NewNode;
			}
			// pos->mChild[2] = pos->mChild[3] = Node;
		}
		else {
			if (!pos->mChild[0]){
				pos->mChild[0] = Node;
			}
			if (!pos->mChild[1]){
				pos->mChild[1] = NewNode;
			}
			// pos->mChild[0] = pos->mChild[1] = Node;
		}
		// delete Node;
	}
	return true;
}

MulPTree *MulPTree::Search(u32 IP){
	MulPTree *pos = this, *q = NULL;
	u32 tmp = IP;
	u8 loc;
	while(pos){
		loc = (tmp & 0xc0000000) >> 30;
		q = pos;
		pos = pos->mChild[loc];
		tmp <<= 2;
	}
#if 0
	if (loc == 3 && q->mChild[2] && q->mChild[2]->SubNet != 0){
		return q->mChild[2];
	}
	if ((loc == 2 ||loc == 1 || loc == 3) &&q->mChild[0]&& q->mChild[0]->SubNet != 0){
		return q->mChild[0];
	}
#endif
	int bite = 0;
	for(int i = 0; i <= q->PreLen ; ++i){
		if (q->SubNet>>(31 -i) == IP >> (31 -i))
		  ++bite;
		else{
			break;
		}
	}
	if (bite < q->PreLen){
		return this;
	}
	return q;
}
// #if 0
bool MulPTree::DestructTree(){
	u8 loc;
	for (loc = 0; loc != 4; ++loc){
		if (mChild[loc]){
			mChild[loc]->DestructTree();
		}
	}
	delete this;
	return true;
}
// #endif

bool PTree::Pleafpush(PTree *Node){
	if ( Child[0] == NULL && Child[1] == NULL){
		return true;
	}
	if (SubNet != 0){
		Node->SubNet = SubNet;
		Node->Port = Port;
		Node->PreLen = PreLen;
	}
	// else {
	//     SubNet = Node->SubNet;
	//     Port = Node->Port;
	//     PreLen = Node->PreLen;
	// }
	if (Child[0] && Child[1]){
		Child[0]->Pleafpush(Node);
		Child[1]->Pleafpush(Node);
	}
	else {
		PTree *NewNode = new PTree();
		NewNode->SubNet = Node->SubNet;
		NewNode->Port = Node->Port;
		NewNode->PreLen = Node->PreLen;
		if (Child[0]){
			Child[0]->Pleafpush(NewNode);
			Child[1] = Node;
		}
		else {
			Child[0] = Node;
			Child[1]->Pleafpush(NewNode);
		}
	}
	return true;
}

bool MulPTree::MleafPush(MulPTree *Node){
	if (mChild[0] == NULL && mChild[1] == NULL && mChild[2] == NULL && mChild[3] == NULL){
		return true;
	}
	if (SubNet != 0){
		Node->SubNet = SubNet;
		Node->Port = Port;
		Node->PreLen = PreLen;
	}
	// else{
	//     SubNet = Node->SubNet;
	//     Port = Node->Port;
	//     PreLen = Node->PreLen;
	// }
	if (mChild[0]&&mChild[1]&&mChild[2]&&mChild[3]){
		for (u8 i = 0; i != 4; ++i){
			mChild[i]->MleafPush(Node);
		}
	}
	else {
		MulPTree *NewNode = new MulPTree();
		NewNode->SubNet = Node->SubNet;
		NewNode->PreLen = Node->PreLen;
		NewNode->Port = Node->Port;
		for (u8 i = 0; i != 4; ++i){
			if (mChild[i] == NULL)
			  mChild[i] = Node;
			else 
			  mChild[i]->MleafPush(NewNode);
		}
	}
	return true;
}

