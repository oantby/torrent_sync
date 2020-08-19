#include <iostream>
#include <filesystem>

using namespace std;

void usage() {
	cout << "Usage: torrent_tree <source directory> <save directory>\n"
		"\tCreates a series of torrent files to enable full replication of the "
		"hierarchy at \033[1msource directory\033[0m, with all files saved to "
		"\033[1msave directory\033[0m" << endl;
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		usage();
		return 1;
	}
	
	if (!filesystem::is_directory(filesystem::status(argv[1]))) {
		cerr << "No such directory: " << argv[1] << endl;
		usage();
		return 2;
	}
	
	{
		filesystem::file_status saveStat = filesystem::status(argv[2]);
		if (filesystem::exists(saveStat)) {
			if (!filesystem::is_directory(saveStat)) {
				
				cerr << "Not a directory: " << argv[2] << endl;
				usage();
				return 3;
			}
			
		} else {
			// create the output directory (and any directories leading to it).
			cout << "Creating directory " << argv[2] << endl;
			filesystem::create_directories(argv[2]);
		}
	}
	
	// because we did no error checking above, getting here should mean all is well
	// (or exceptions would've occurred)
	
}