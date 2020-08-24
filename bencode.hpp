#ifndef BENCODE_HPP
#define BENCODE_HPP

#include <string>
#include <vector>
#include <map>
#include <exception>

using namespace std;

// header-only bencode libarary
namespace bencode {
	// pretty ridiculously simple language.  Lists are vectors of value_type,
	// dictionaries are maps of string -> value_type
	// integers, we'll store as 64-bit to ensure there are no issues in ridiculous size.
	enum class bencode_type { bytes, integer, list, dict, none };
	
	class BencodeVal {
		bencode_type type;
		string bytes;
		vector<BencodeVal> list;
		long long integer;
		map<string, BencodeVal> dict;
		
		public:
		
		BencodeVal() : type(bencode_type::none) {}
		BencodeVal(bencode_type t) : type(t) {}
		BencodeVal(string s) : type(bencode_type::bytes), bytes(s) {}
		BencodeVal(vector<BencodeVal> v) : type(bencode_type::list), list(v) {}
		BencodeVal(map<string, BencodeVal> m) : type(bencode_type::dict), dict(m) {}
		BencodeVal(long long l) : type(bencode_type::integer), integer(l) {}
		
		BencodeVal &operator[](size_t idx) {
			if (type != bencode_type::list) {
				throw runtime_error("[int] operator invalid for BencodeVal of this type");
			}
			
			return list[idx];
		}
		
		BencodeVal operator+=(string s) {
			if (type != bencode_type::bytes) {
				throw runtime_error("+= (string) invalid for BencodeVal of this type");
			}
			bytes += s;
			return *this;
		}
		
		const BencodeVal operator[](size_t idx) const {
			if (type != bencode_type::list) {
				throw runtime_error("[int] operator invalid for BencodeVal of this type");
			}
			
			return list[idx];
		}
		
		BencodeVal &operator[](string idx) {
			if (type != bencode_type::dict) {
				throw runtime_error("[string] operator invalid for BencodeVal of this type");
			}
			
			return dict[idx];
		}
		
		const string toString() const {
			string r;
			switch (type) {
				case bencode_type::integer:
					r = 'i';
					r += to_string(integer);
					r += 'e';
					break;
				case bencode_type::bytes:
					r = to_string(bytes.size());
					r += ':';
					r += bytes;
					break;
				case bencode_type::list:
					r = 'l';
					for (const BencodeVal &x : list) {
						r += x.toString();
					}
					r += 'e';
					break;
				case bencode_type::dict:
					r = 'd';
					for (const pair<const string, const BencodeVal> &x : dict) {
						r += to_string(x.first.size());
						r += ':';
						r += x.first;
						r += x.second.toString();
					}
					r += 'e';
					break;
				case bencode_type::none:
					throw invalid_argument("Tried to print uninitialized node");
					break;
			}
			return r;
		}
		
		void push_back(BencodeVal v) {
			if (type != bencode_type::list) {
				throw runtime_error("push_back invalid for BencodeVal of this type");
			}
			
			list.push_back(v);
		}
	};
}

#endif