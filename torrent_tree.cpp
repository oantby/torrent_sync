#include <iostream>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <getopt.h>
#include <set>
#include "bencode.hpp"
#include "sha1.hpp"

using namespace std;

void usage() {
	cout << "Usage: torrent_tree -[vquf] [--ignore file_or_dir ...] <source directory> <save directory> <announce URI>\n"
		"\tCreates a series of torrent files to enable full replication of the \n"
		"\thierarchy at \033[1msource directory\033[0m, with all files saved to \n"
		"\t\033[1msave directory\033[0m.  \033[1mannounce URI\033[0m is listed \n"
		"\ton the torrents as a valid tracker.\n\n"
		"Options:\n"
		"\t-v\n\t\tVerbose mode\n\n"
		"\t-q\n\t\tQuiet mode - anti-verbose mode\n\n"
		"\t-u\n\t\tUpdate mode - Existing .torrent files will be overwritten IF\n"
		"\t\tthe source file is newer than the existing .torrent\n\n"
		"\t-f\n\t\tForce overwrite - All existing .torrent files will be overwritten\n\n"
		"\t--ignore file_or_dir\n"
		"\t\tIf file_or_dir is a directory, do not recurse into it.  If file_or_dir\n"
		"\t\tis a file, do not create a .torrent entry for it\n";
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

#define OVERWRITE_NONE 0
#define OVERWRITE_NEWER 1
#define OVERWRITE_ALL 2
	
bool verbose = false;
short overwrite = OVERWRITE_NONE;

set<filesystem::path> ignored_dirs;
string start_path;
filesystem::path out_path;
string announce_url;
string torrent_file_name;

void process_file(filesystem::directory_entry entry) {
	if (verbose) {
		cout << "Processing " << entry.path() << endl;
	}
	
	filesystem::path file_path = out_path / entry.path().relative_path().replace_extension(".torrent");
	filesystem::file_status out_status = filesystem::status(file_path);
	// this will later be based on a command-line argument.
	if (filesystem::exists(out_status)) {
		bool skip = true;
		if (overwrite == OVERWRITE_ALL) {
			if (verbose) {
				cout << "Would skip " << file_path << ", but -f specified" << endl;
			}
			skip = false;
		} else if (overwrite == OVERWRITE_NEWER) {
			if (filesystem::last_write_time(file_path) < filesystem::last_write_time(entry.path())) {
				if (verbose) {
					cout << "Would skip " << file_path << ", but -u specified" << endl;
				}
				skip = false;
			}
		}
		
		if (skip) {
			if (verbose) {
				cout << "Skipping " << file_path << " - already exists" << endl;
			}
			return;
		} else {
			// time to get destructive.  In case this was a directory before (okay, satan),
			// we're going to delete recursively.
			filesystem::remove_all(file_path);
		}
	}
	
	bencode::BencodeVal torrent(bencode::bencode_type::dict);
	bencode::BencodeVal info(bencode::bencode_type::dict);
	torrent["announce"] = announce_url;
	info["name"] = torrent_file_name;
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
	info["private"] = 1;
	
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
		if (buff.size()) {
			info["pieces"] += sha1::hash(buff);
		}
	}
	torrent["info"] = info;
	
	if (verbose) {
		cout << "Creating " << file_path << endl;
	}
	filesystem::create_directories(file_path.parent_path());
	ofstream ofile(file_path, ios::out|ios::trunc);
	ofile << torrent.toString();
	ofile.close();
}

int main(int argc, char *argv[]) {
	struct option long_options[] = {
		{"ignore", required_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int c, option_index;
	while ((c = getopt_long(argc, argv, "vquf", long_options, &option_index)) != -1) {
		
		switch (c) {
			case 'v':
				verbose = true;
				break;
			case 'q':
				verbose = false;
				break;
			case 'u':
				overwrite = OVERWRITE_NEWER;
				break;
			case 'f':
				overwrite = OVERWRITE_ALL;
				break;
			case 0:
				{
					filesystem::path t = filesystem::absolute(optarg);
					if (!t.has_filename()) {
						t = t.parent_path();
					}
					
					if (verbose) {
						cout << "Adding ignored directory: " << t << endl;
					}
					
					ignored_dirs.insert(t);
				}
				break;
			default:
				usage();
				return 1;
				break;
		}
	}
	
	if (argc != optind + 3) {
		usage();
		return 1;
	}
	start_path = argv[optind];
	out_path = argv[optind + 1];
	announce_url = argv[optind + 2];
	
	if (!filesystem::is_directory(filesystem::status(start_path))) {
		cerr << "No such directory: " << start_path << endl;
		usage();
		return 2;
	}
	
	{
		filesystem::file_status saveStat = filesystem::status(out_path);
		if (filesystem::exists(saveStat)) {
			if (!filesystem::is_directory(saveStat)) {
				
				cerr << "Not a directory: " << out_path << endl;
				usage();
				return 3;
			}
			
		} else {
			// create the output directory (and any directories leading to it).
			cout << "Creating directory " << out_path << endl;
			filesystem::create_directories(out_path);
		}
	}
	
	// because we did no error checking above, getting here should mean all is well
	// (or exceptions would've occurred).  That's right, I just bragged about not checking for errors.
	if (start_path.back() == '/') start_path.pop_back();
	vector<filesystem::path> path_stack {start_path};
	torrent_file_name = path_stack.back().has_filename() ? path_stack.back().filename()
		: (path_stack.back().has_parent_path() ? path_stack.back().parent_path().filename() : ".");
	// there's a couple different ways for just dot as a name.
	if (torrent_file_name == ".") torrent_file_name = "files";
	
	for (size_t i = 0; i < path_stack.size(); i++) {
		for (auto &entry : filesystem::directory_iterator(path_stack[i])) {
			if (entry.is_regular_file()) {
				
				process_file(entry);
				
			} else if (entry.is_directory()) {
				filesystem::path t = filesystem::absolute(entry.path());
				if (ignored_dirs.count(t)) {
					if (verbose) {
						cout << "Skipping ignored directory " << t << endl;
					}
					continue;
				}
				path_stack.push_back(entry.path());
			}
		}
	}
	
	
	return 0;
}