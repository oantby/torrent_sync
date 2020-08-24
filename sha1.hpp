// my own header-only SHA-1 implementation, for no good reason other than to verify that I can.
// yes, I know sha1 isn't overly complex.  But, the various memory handling is mine, and I'll work with it.
// the record may show that, using a random 1GB file and no compiler optimization, this takes about 20x as long as GNU sha1sum.
// there are A LOT of known places for optimization here.
#ifndef SHA_1_HPP
#define SHA_1_HPP

#include <iostream>
using namespace std;

namespace sha1 {
	typedef uint32_t sha1_word;
	
	// for t in [0,79], use t/20
	const sha1_word starters[] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};
	
	sha1_word ch(sha1_word x, sha1_word y, sha1_word z) {
		return (x & y) ^ ((~x) & z);
	}
	
	sha1_word par(sha1_word x, sha1_word y, sha1_word z) {
		return x ^ y ^ z;
	}
	
	sha1_word maj(sha1_word x, sha1_word y, sha1_word z) {
		return (x & y) ^ (x & z) ^ (y & z);
	}
	
	string hash(string message) {
		// message is assumed to contain a number of octets.  No partial bytes.
		uint64_t start_length = message.size() * 8;
		// actual message must be a multiple of 512 bits long, including
		// a 1, some number of 0s, and 64 bits indicating start_length.
		uint64_t final_length = (uint64_t)((start_length + 65) / 512)*512 + 512;
		// because we've assumed full bytes, we're guaranteed to start with
		// a byte that is 10000000
		message += (unsigned char)0x80;
		uint64_t z_byte_count = final_length/8 - start_length/8 - 9;
		// append 0 bytes until we're at 512k-64 bits (k in Z).
		message += string(z_byte_count, 0);
		
		// append the big-endian length.
		message += (unsigned char)((start_length << 0) >> 56);
		message += (unsigned char)((start_length << 8) >> 56);
		message += (unsigned char)((start_length << 16) >> 56);
		message += (unsigned char)((start_length << 24) >> 56);
		message += (unsigned char)((start_length << 32) >> 56);
		message += (unsigned char)((start_length << 40) >> 56);
		message += (unsigned char)((start_length << 48) >> 56);
		message += (unsigned char)((start_length << 56) >> 56);
		
		uint64_t block_count = final_length/512;
		
		sha1_word hash_blocks[] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
		
		// the rules of the SHA.  Loop over each block.
		for (uint64_t i = 1; i <= block_count; i++) {
			sha1_word a = hash_blocks[0], b = hash_blocks[1], c = hash_blocks[2],
				d = hash_blocks[3], e = hash_blocks[4];
			sha1_word W[80];
			// each block is subjected to 80 operations
			for (short t = 0; t < 80; t++) {
				// initialize the message scheduler.
				if (t < 16) {
					uint64_t start_point = (i-1)*64 + 4*t;
					W[t] = ((unsigned char)message[start_point]    << 24)
						+ ((unsigned char)message[start_point + 1] << 16)
						+ ((unsigned char)message[start_point + 2] <<  8)
						+ ((unsigned char)message[start_point + 3]);
				} else {
					W[t] = W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16];
					W[t] = (sha1_word)(W[t] << 1) + (sha1_word)(W[t] >> 31);
				}
				sha1_word temp = (sha1_word)(a << 5) + (sha1_word)(a >> 27);
				// f_t(b, c, d), from the SHA1 description, broken out here:
				if (t < 20) {
					temp += ch(b, c, d);
				} else if (t < 40) {
					temp += par(b, c, d);
				} else if (t < 60) {
					temp += maj(b, c, d);
				} else {
					temp += par(b, c, d);
				}
				temp += e + starters[t/20] + W[t];
				
				e = d;
				d = c;
				c = (sha1_word)(b << 30) + (sha1_word)(b >> 2);
				b = a;
				a = temp;
				#ifdef DEBUG
				cout << "  [t = " << t << "] A=" << a << ", B=" << b << ", C=" << c << ", D=" << d << ", E=" << e << endl;
				#endif
			}
			hash_blocks[0] += a;
			hash_blocks[1] += b;
			hash_blocks[2] += c;
			hash_blocks[3] += d;
			hash_blocks[4] += e;
		}
		
		string hash;
		hash += (unsigned char)((hash_blocks[0] <<  0) >> 24);
		hash += (unsigned char)((hash_blocks[0] <<  8) >> 24);
		hash += (unsigned char)((hash_blocks[0] << 16) >> 24);
		hash += (unsigned char)((hash_blocks[0] << 24) >> 24);
		hash += (unsigned char)((hash_blocks[1] <<  0) >> 24);
		hash += (unsigned char)((hash_blocks[1] <<  8) >> 24);
		hash += (unsigned char)((hash_blocks[1] << 16) >> 24);
		hash += (unsigned char)((hash_blocks[1] << 24) >> 24);
		hash += (unsigned char)((hash_blocks[2] <<  0) >> 24);
		hash += (unsigned char)((hash_blocks[2] <<  8) >> 24);
		hash += (unsigned char)((hash_blocks[2] << 16) >> 24);
		hash += (unsigned char)((hash_blocks[2] << 24) >> 24);
		hash += (unsigned char)((hash_blocks[3] <<  0) >> 24);
		hash += (unsigned char)((hash_blocks[3] <<  8) >> 24);
		hash += (unsigned char)((hash_blocks[3] << 16) >> 24);
		hash += (unsigned char)((hash_blocks[3] << 24) >> 24);
		hash += (unsigned char)((hash_blocks[4] <<  0) >> 24);
		hash += (unsigned char)((hash_blocks[4] <<  8) >> 24);
		hash += (unsigned char)((hash_blocks[4] << 16) >> 24);
		hash += (unsigned char)((hash_blocks[4] << 24) >> 24);
		return hash;
	}
}

# endif