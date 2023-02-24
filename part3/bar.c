#include <time.h>

struct bar {
	char seats[4][8];
	time_t arrivalTimes[4][8];
	int open;
};


int cleanTable(bar* b, int table) {
        printf("cleaning table...\n");
        //if entire table is dirty, clean and return true
        //else return false
        return 0;
}

bar* new_bar() {
	bar* temp;
        temp->seats = { [0 ... 31] = 'c' };
        temp->arrivalTimes { [0 ... 31] = 0 };
        temp->open = 0;
        return temp;
}


int main(int argc, char *argv[]) {
	bar b = new_bar();
	cleanTable(b, 4);

}
