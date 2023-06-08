#include <fstream>
#include <iostream>
using namespace std;
int main(int argc, char*argv[]){
	unsigned *buf = (unsigned*)malloc(sizeof(unsigned)*2*2);
	cout<<sizeof(unsigned)<<endl;
	buf[0] = 0;
	buf[1] = 1;
	buf[2] = 1;
	buf[3] = 2;
	cout<<argv[1]<<endl;
	ofstream file(argv[1], std::ios::binary | std::ios::app);

	file.write((char*)buf, sizeof(unsigned)*2*2);
	file.close();
	return 0;	
}
