.PHONY : all

all : torrent_tree flatten_tree

torrent_tree : torrent_tree.cpp bencode.hpp sha1.hpp
	$(CXX) -static -std=c++17 -Ofast -Wall torrent_tree.cpp -o torrent_tree

flatten_tree : flatten_tree.cpp bencode.hpp sha1.hpp
	$(CXX) -static -std=c++17 -Ofast -Wall flatten_tree.cpp -o flatten_tree
