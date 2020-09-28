#ifndef BENCODE_HPP
#define BENCODE_HPP

#include <string>
#include <vector>
#include <map>
#include <exception>
#include <fstream>

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
		
		static BencodeVal parseInt(string data, size_t &endPos) {
			if (data.empty() || data[0] != 'i') {
				throw runtime_error("Invalid integer representation");
			}
			data = data.substr(1);
			if ((data[0] < 48 || data[0] > 57) && data[0] != '-') {
				throw runtime_error("Invalid integer representation");
			}
			BencodeVal r(stol(data, &endPos));
			if (endPos > data.size() - 1 || data[endPos] != 'e') {
				throw runtime_error("Invalid integer representation");
			}
			endPos += 2; // add the e and the i.
			return r;
		}
		
		static BencodeVal parseBytes(string data, size_t &endPos) {
			// must start with an integer.
			if (data.empty() || data[0] < 48 || data[0] > 57) {
				throw runtime_error("Invalid bytes representation - byte count invalid");
			}
			size_t len = stoul(data, &endPos);
			// endPos should now be pointing at a colon.
			if (endPos > data.size() - 2 || data[endPos] != ':') {
				throw runtime_error("Invalid bytes representation");
			}
			
			if (len + endPos + 1 > data.size()) {
				throw runtime_error("Invalid bytes representation");
			}
			BencodeVal r(data.substr(endPos + 1, len));
			endPos += 1 + len;
			return r;
		}
		static BencodeVal parseDict(string data, size_t &endPos) {
			if (data.empty() || data[0] != 'd') {
				throw runtime_error("Invalid dict representation");
			}
			endPos = 1;
			BencodeVal r(bencode_type::dict);
			
			try {
				size_t s_endPos = 0;
				while (true) {
					BencodeVal k = parseBytes(data.substr(endPos), s_endPos);
					endPos += s_endPos;
					r[k.bytes] = parse(data.substr(endPos), &s_endPos);
					endPos += s_endPos;
				}
			} catch (runtime_error &e) {
				// reached an invalid string.  hopefully it's the end of the dict.
				if (data[endPos] != 'e') {
					throw;
				}
			}
			endPos++;
			return r;
		}
		static BencodeVal parseList(string data, size_t &endPos) {
			if (data.empty() || data[0] != 'l') {
				throw runtime_error("Invalid list representation");
			}
			endPos = 1;
			BencodeVal r(bencode_type::list);
			
			try {
				size_t s_endPos = 0;
				while (true) {
					BencodeVal k = parse(data.substr(endPos), &s_endPos);
					endPos += s_endPos;
					r.list.push_back(k);
				}
			} catch (runtime_error &e) {
				if (data[endPos] != 'e') {
					throw;
				}
			}
			endPos++;
			return r;
		}
		
		public:
		
		static void testThings(string *path = NULL) {
			size_t idx = 0;
			string s("11:hello world");
			BencodeVal b;
			b = parseBytes(s, idx);
			
			cout << "parseBytes(s).toString() == s: " << (b.toString() == s) << endl;
			cout << "endPos == s.size(): " << (idx == s.size()) << endl;
			
			s = "i7144911e";
			b = parseInt(s, idx);
			cout << "parseInt(s).toString() == s: " << (b.toString() == s) << endl;
			cout << "endPos == s.size(): " << (idx == s.size()) << endl;
			cout << "parseInt(s).integer is correct: " << (b.integer == 7144911) << endl;
			
			// those are the easy ones.  here we go.
			s = "d1:ai0e5:helloi413e3:hey5:helloe";
			b = parseDict(s, idx);
			cout << "parseDict(s).toString() == s: " << (b.toString() == s) << endl;
			cout << "endPos == s.size(): " << (idx == s.size()) << endl;
			cout << "parseDict(s) contains expected keys with expected values: "
				<< (b.dict["hey"].bytes == "hello" && b.dict["hello"].integer == 413 && b.dict["a"].integer == 0) << endl;
			
			s = "li71e81:A41tiAS0GLSWxCeRmc0H3dMDEttNFacbLVsJzM97jW5DNFdnbKRjGuM11J8plZ8ncd3qopLC68HCQMlrP"
				"d1:a4:bruhee";
			b = parseList(s, idx);
			cout << "parseList(s).toString() == s: " << (b.toString() == s) << endl;
			cout << "endPos == s.size(): " << (idx == s.size()) << endl;
			
			if (path) {
				// verify a file matches once undone and redone.
				ifstream ifile(*path, ios::in|ios::binary|ios::ate);
				size_t size = ifile.tellg();
				string buff(size, '\0');
				ifile.seekg(0);
				if (!ifile.read(&buff[0], size)) {
					cerr << "Failed to read data from test file" << endl;
				} else {
					cout << "Testing given file" << endl;
					b = parse(buff);
					cout << "parse(s).toString() == s: " << (b.toString() == buff) << endl;
				}
				ifile.close();
			}
		}
		
		static BencodeVal parse(string data, size_t *endPos = 0) {
			BencodeVal r;
			size_t idx = 0;
			if (data.empty()) return r;
			switch (data[0]) {
				case 'i':
					r = parseInt(data, idx);
					break;
				case 'l':
					r = parseList(data, idx);
					break;
				case 'd':
					r = parseDict(data, idx);
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					r = parseBytes(data, idx);
					break;
				default:
					throw runtime_error("Invalid bencode data");
					break;
			}
			if (idx != data.size() && !endPos) {
				throw runtime_error("bencode data had more than one root element");
			} else if (endPos) {
				*endPos = idx;
			}
			
			return r;
		}
		
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
					for (const pair<const string, BencodeVal> &x : dict) {
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