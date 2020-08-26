#include <iostream>
#include <filesystem>
#include <fstream>
#include <cmath>
#include "bencode.hpp"
#include "sha1.hpp"

using namespace std;

void usage() {
	cout << "Usage: torrent_tree <source directory> <save directory> <announce URI>\n"
		"\tCreates a series of torrent files to enable full replication of the "
		"hierarchy at \033[1msource directory\033[0m, with all files saved to "
		"\033[1msave directory\033[0m.  \033[1mannounce URI\033[0m is listed "
		"on the torrents as a valid tracker." << endl;
}

// helper function for path conversions.
bencode::BencodeVal path_to_list(const string &path, const char sep = '/') {
	bencode::BencodeVal r(bencode::bencode_type::list);
	
	string cur_path;
	for (size_t i = 0; i < path.size(); i++) {
		if (path[i] == sep) {
			if (cur_path.size()) {// leading slashes shouldn't result in an empty list item.
				r.push_back(cur_path);
				cur_path.clear();
			}
		} else {
			cur_path += path[i];
		}
	}
	r.push_back(cur_path);
	return r;
}

int main(int argc, char *argv[]) {
	if (argc < 4) {
		usage();
		return 1;
	}
	
	bool verbose = argc > 4;
	
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
	// (or exceptions would've occurred).  That's right, I just bragged about not checking for errors.
	string announce_url = argv[3];
	string start_path = argv[1];
	filesystem::path out_path(argv[2]);
	if (start_path.back() == '/') start_path.pop_back();
	vector<filesystem::path> path_stack {start_path};
	for (size_t i = 0; i < path_stack.size(); i++) {
		for (auto &entry : filesystem::directory_iterator(path_stack[i])) {
			if (entry.is_regular_file()) {
				if (verbose) {
					cout << "Processing " << entry << endl;
				}
				bencode::BencodeVal torrent(bencode::bencode_type::dict);
				bencode::BencodeVal info(bencode::bencode_type::dict);
				torrent["announce"] = announce_url;
				info["name"] = string("files");
				uint64_t file_size = filesystem::file_size(entry.path());
				// 5120=102400/20, looks to keep .torrent files <100K with minimum
				// possible piece length, holding a power of 2.
				uint64_t piece_length = max(
					min(
						(uint64_t)1048576,
						(uint64_t)exp2((int)log2(file_size / 5120) + 1)),
					(uint64_t)4096);
				info["piece length"] = piece_length;
				bencode::BencodeVal file(bencode::bencode_type::dict);
				file["path"] = path_to_list(entry.path().string().erase(0, start_path.size() + 1));
				file["length"] = file_size;
				info["files"] = vector<bencode::BencodeVal> {file};
				info["pieces"] = string();
				
				ifstream ifile(entry.path(), ios::in|ios::binary);
				if (!ifile) {
					perror("failed to open file");
				}
				string buff(piece_length, '\0');
				while (ifile.read(&buff[0], piece_length)) {
					info["pieces"] += sha1::hash(buff);
				}
				if (ifile.eof()) {
					// loop finished with data left.  add that data.
					buff.resize(ifile.gcount());
					info["pieces"] += sha1::hash(buff);
				}
				torrent["info"] = info;
				filesystem::path file_path = out_path / entry.path().relative_path();
				if (verbose) {
					cout << "Creating " << file_path << endl;
				}
				filesystem::create_directories(file_path.parent_path());
				ofstream ofile(file_path.replace_extension(".torrent"), ios::out|ios::trunc);
				ofile << torrent.toString();
				ofile.close();
				
				//cout << torrent.toString() << endl;
			} else if (entry.is_directory()) {
				path_stack.push_back(entry.path());
			}
		}
	}
	
	
	return 0;
}