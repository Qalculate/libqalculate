#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <queue>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

#define SPACES " \t\n"

string& remove_blank_ends(string &str) {
	size_t i = str.find_first_not_of(SPACES);
	size_t i2 = str.find_last_not_of(SPACES);
	if(i != string::npos && i2 != string::npos) {
		if(i > 0 || i2 < str.length() - 1) {
			str = str.substr(i, i2 - i + 1);
		}
	} else {
		str.resize(0);
	}
	return str;
}

int main(int argc, char *argv[]) {
	if(argc < 2) return 1;
	string sfile = argv[1];
	ifstream cfile(sfile);
	if(argc > 2) sfile = argv[2];
	else sfile += ".fixed";
	ofstream ofile(sfile);
	int variant = 0;
	if(argc > 3) {
		variant = argv[3][0] - '0';
		if(variant < 0 || variant > 5) return 1;
	}
	if(cfile.is_open()) {
		char linebuffer[1000];
		string sbuffer, sbuffer2, msgid, msgstr, sout;
		while(!sbuffer2.empty() || (cfile.rdstate() & std::ifstream::eofbit) == 0) {
			if(!sbuffer2.empty()) {
				sbuffer = sbuffer2;
				sbuffer2 = "";
			} else {
				cfile.getline(linebuffer, 1000);
				sbuffer = linebuffer;
				sout = sbuffer;
				remove_blank_ends(sbuffer);
			}
			if(variant != 2 && variant != 5 && sbuffer.length() > 7 && sbuffer.substr(0, 7) == "msgid \"") {
				if(sbuffer[7] == '\"') {
					msgid = "";
					sout = sbuffer;
					while(true) {
						cfile.getline(linebuffer, 1000);
						sbuffer = linebuffer;
						sbuffer2 = sbuffer;
						remove_blank_ends(sbuffer2);
						if(sbuffer2.empty() || sbuffer2[0] != '\"') break;
						sout += "\n";
						sout += sbuffer;
						msgid += sbuffer2.substr(1, sbuffer2.length() - 2);
					}
				} else {
					msgid = sbuffer.substr(7, sbuffer.length() - 8);
				}
			} else if(sbuffer.length() > 8 && sbuffer.substr(0, 8) == "msgstr \"") {
				if(sbuffer[8] == '\"') {
					msgstr = "";
					while(true) {
						cfile.getline(linebuffer, 1000);
						sbuffer = linebuffer;
						sout += "\n";
						sout += sbuffer;
						remove_blank_ends(sbuffer);
						if(sbuffer.empty() || sbuffer[0] != '\"') break;
						msgstr += sbuffer.substr(1, sbuffer.length() - 2);
					}
				} else {
					msgstr = sbuffer.substr(8, sbuffer.length() - 9);
				}
				if(!msgid.empty() && (msgstr == msgid || msgstr == "-")) {
					if(variant == 3 || variant == 4) cout << msgstr << endl;
					sout = "msgstr \"\"";
				} else if(sout.find("\n", 10) == string::npos && variant != 1 && variant != 4) {
					size_t i = 0, i2 = 0;
					while(true) {
						i = msgstr.find(":", i);
						if(i == string::npos) break;
						i2 = msgstr.find_last_of("r,\"",i);
						if(i2 != string::npos && msgstr[i2] == 'r' && i != msgstr.length() - 1 && msgstr[i + 1] != ' ') {
							if(variant == 3 || variant == 5) cout << msgstr << " -> ";
							msgstr.erase(i2, 1);
							i--;
							if(i2 > 0 && msgstr[i2 - 1] == '-') {
								msgstr.erase(i2 - 1, 1);
								i--;
								i2--;
							}
							if(i == i2 && (i == 0 || msgstr.find_last_of("aciopsu", i) != i - 1)) {
								msgstr.erase(i, 1);
								i--;
							}
							if(variant == 3 || variant == 5) cout << msgstr << endl;
							sout = "msgstr \"";
							sout += msgstr;
							sout += "\"";
						}
						i++;
					}
				}
				msgid = "";
			}
			if(variant <= 2) ofile << sout << endl;
		}
		cfile.close();
	}
	return 0;
}
