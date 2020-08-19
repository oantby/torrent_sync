#include <iostream>

using namespace std;

void usage() {
	cout << "Usage: torrent_tree <source directory> <save directory>\n"
		"\tCreates a series of torrent files to enable full replication of the "
		"hierarchy at \033[1msource directory\033[0m, with all files saved to "
		"\033[save directory\033[0m" << endl;
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		usage();
		return 1;
	}
}