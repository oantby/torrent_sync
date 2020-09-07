#include <iostream>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <set>
#include "sha1.hpp"
#include "bencode.hpp"

using namespace std;

void usage() {
	cout << "Usage: flatten_tree -[vquf] [--ignore file_or_dir ...] <source directory> <save directory>\n"
		"\tCreates a flat (no subdirectories) version of all the files within\n"
		"\t\033[1msource directory\033[0m at \033[1msave directory\033[0m,\n"
		"\tcreating it if necessary.  Files are named after their info_hash\n\n"
		"Options:\n"
		"\t-v\n\t\tVerbose mode\n\n"
		"\t-q\n\t\tQuiet mode - anti-verbose mode\n\n"
		"\t-u\n\t\tUpdate mode - Existing .torrent files will be overwritten IF\n"
		"\t\tthe source file is newer than the existing .torrent\n\n"
		"\t-f\n\t\tForce overwrite - All existing .torrent files will be overwritten\n\n"
		"\t--ignore dir\n"
		"\t\tIf dir is found, do not recurse into it.\n";
}

// this function from fredoverflow on stackoverflow.  short and simple, I definitely am not gonna make a better one.
string string_to_hex(const string& input)
{
	static const char hex_digits[] = "0123456789ABCDEF";
	
	string output;
	output.reserve(input.length() * 2);
	for (unsigned char c : input) {
		output.push_back(hex_digits[c >> 4]);
		output.push_back(hex_digits[c & 15]);
	}
	return output;
}

string info_hash(filesystem::path p) {
	ifstream f;
	f.exceptions(ifstream::failbit|ifstream::badbit);
	f.open(p, ios::in|ios::binary|ios::ate);
	
	auto size = f.tellg();
	f.seekg(0);
	string buff(size, '\0');
	f.read(&buff[0], size);
	f.close();
	bencode::BencodeVal b = bencode::BencodeVal::parse(buff);
	return sha1::hash(b["info"].toString());
	
}

#define OVERWRITE_NONE 0
#define OVERWRITE_NEWER 1
#define OVERWRITE_ALL 2

bool verbose = false;
short overwrite = OVERWRITE_NONE;

set<filesystem::path> ignored_dirs;
string start_path;
filesystem::path out_path;

void process_file(filesystem::directory_entry entry) {
	filesystem::path path = entry.path();
	if (verbose) {
		cout << "Processing " << path << endl;
	}
	string hash;
	try {
		hash = info_hash(path);
	} catch (...) {
		cerr << "Failed to calculate info_hash for " << path << endl;
		return;
	}
	filesystem::path file_path = out_path / (string_to_hex(hash) + ".torrent");
	filesystem::copy_options o = overwrite == OVERWRITE_NONE ? filesystem::copy_options::skip_existing :
		(overwrite == OVERWRITE_NEWER ? filesystem::copy_options::update_existing :
		filesystem::copy_options::overwrite_existing);
	filesystem::copy(path, file_path, o);
	if (verbose) {
		cout << "Copied " << path << " to " << file_path << endl;
	}
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
	
	if (argc != optind + 2) {
		usage();
		return 1;
	}
	start_path = argv[optind];
	out_path = argv[optind + 1];
	
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