.PHONY : all

all : torrent_tree

torrent_tree : torrent_tree.cpp bencode.hpp sha1.hpp
	$(CXX) -std=c++17 torrent_tree.cpp -o torrent_tree
