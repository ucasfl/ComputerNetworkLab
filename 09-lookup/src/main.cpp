#include "prefixTree.h"
int main(){
	MulPTree *Tree1 = new MulPTree();
	PTree *Tree2 = new PTree();
	CVTree *Tree3 = new CVTree();
	std::cout << "Constructing Tree...\n\n";
	if (!Tree1->ConstructTree()){
		std::cout << "Construct MulPTree failed.\n" ;
		return 0;
	}
	if (!Tree1->ConstructTree()){
		std::cout << "Construct MulPTree failed.\n" ;
		return 0;
	}
	if (!Tree2->ConstructTree()){
		std::cout << "Construct PTree failed.\n";
		return 0;
	}
	MulPTree *MNode = new MulPTree();
	// PTree *PNode = new PTree();
	// Tree2->Pleafpush(PNode);
	Tree1->MleafPush(MNode);
	if(!Tree3->ConstructCVTree(Tree1)){
		std::cout << "Construct CVTree failed.\n";
		return 0;
	}
	int num = 0;;
	std::string s;
	clock_t start;
	clock_t finish1, finish2, finish3;
	std::cout << "Input search IP: " ;
	while(getline(std::cin, s)){

		u32 ip = Tree2->StrToIp(s.c_str());
		start = clock();

		PTree *Result2 = Tree2->Search(ip);
		finish1 = clock();

		MulPTree *Result1 = Tree1->Search(ip);
		finish2 = clock();

		Leaf *RevLeaf = Tree3->Search(ip);
		finish3 = clock();

		printf("\nSearch Result:\n");
		if (Result1->Equal(Result2) && Result1->Equal(RevLeaf)){
			Result1->dumpNode();
			// RevLeaf->dumpLeaf();
			printf("Search Time of Single Bit Prefix Tree: %lf\n",double(finish1-start)/CLOCKS_PER_SEC);
			printf("Search Time of Double Bit Prefix Tree: %lf\n",double(finish2-finish1)/CLOCKS_PER_SEC);
			printf("Search Time of CV(Vector Compression) Prefix Tree: %lf\n",double(finish3-finish2)/CLOCKS_PER_SEC);
			std::cout<<'\n';
		}
		else{
			std::cout << "Search Results Are not Equal.\n" ;
			Result1->dumpNode();
			Result2->dumpNode();
			RevLeaf->dumpLeaf();
			std::cout << '\n';
			num ++;
		}
		std::cout << "Input search IP: ";
	}
	// Tree1->DestructTree();
	if (!num){
		printf ("\n\nTest End. All Test Cases Passed.\n");
	}
	else {
		printf ("%d Test Cases Failed.\n", num);
	}
	Tree2->DestructTree();
	Tree3->DestructCVTree();
	delete Tree3;
	return 0;
}

