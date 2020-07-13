#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>

using std::string;

string& remove_blank_ends(string &str) {
	size_t i = str.find_first_not_of("\t\n ");
	size_t i2 = str.find_last_not_of("\t\n ");
	if(i != string::npos && i2 != string::npos) {
		if(i > 0 || i2 < str.length() - 1) {
			str = str.substr(i, i2 - i + 1);
		}
	} else {
		str.resize(0);
	}
	return str;
}

string& gsub(const char *pattern, const char *sub, string &str) {
	size_t i = str.find(pattern);
	while(i != string::npos) {
		str.replace(i, strlen(pattern), string(sub));
		i = str.find(pattern, i + strlen(sub));
	}
	return str;
}

int main(int, char *[]) {

	FILE *efile = fopen("examples.xml", "w+");
	if(!efile) return 1;
	FILE *rfile = fopen("README.md", "r");
	if(!rfile) {fclose(efile); return 1;}
	
	char line[10000];
	string stmp, expression, result;
	size_t n = 0;
	bool b = false;
	bool in_sect = false;
	bool in_list = false;
	bool b_readline = true;
	fputs("<appendix id=\"qalculate-examples\">\n<title>Example expressions</title>\n", efile);
	while(true) {
		if(b_readline) {
			if(fgets(line, 10000, rfile) == NULL) break;
			stmp = line;
			remove_blank_ends(stmp);
		}
		b_readline = true;
		if(!stmp.empty()) {
			if(!b && stmp[0] == '#' && stmp.substr(0, 11) == "## Examples") {
				b = true;
			} else if(b) {
				if(stmp[0] == '#') {
					if(in_sect) {
						fputs("</sect1>\n", efile);
					} else {
						in_sect = true;
					}
					n++;
					fprintf(efile, "<sect1 id=\"qalculate-examples-%i\">\n<title>%s</title>\n", n, stmp.substr(4).c_str());
					in_list = false;
				} else if(stmp[0] == '_') {
					if(!in_list) {
						stmp = stmp.substr(1, stmp.length() - 2);
						remove_blank_ends(stmp);
						fprintf(efile, "<para><emphasis>%s</emphasis></para>", stmp.c_str());
					}
				} else {
					size_t i = stmp.find(" _");
					if(i != string::npos) {
						expression = stmp.substr(0, i);
						remove_blank_ends(expression);
						result = stmp.substr(i + 2);
						remove_blank_ends(result);
						if(result.back() == '_') {result.pop_back(); remove_blank_ends(result);}
						gsub("&", "&amp;", result);
						gsub("<", "&lt;", result);
						gsub(">", "&gt;", result);
					} else {
						expression = stmp;
						remove_blank_ends(expression);
						result = "";
						while(fgets(line, 10000, rfile)) {
							stmp = line;
							remove_blank_ends(stmp);
							if(stmp.empty()) {
								break;
							} else if(stmp[0] == '_') {
								stmp = stmp.substr(1);
								if(stmp.back() == '_') {stmp.pop_back(); remove_blank_ends(stmp);}
								gsub("&", "&amp;", stmp);
								gsub("<", "&lt;", stmp);
								gsub(">", "&gt;", stmp);
								result += "<sbr/>";
								result += stmp;
							} else {
								b_readline = false;
								break;
							}
						}
					}
					gsub("&", "&amp;", expression);
					gsub("<", "&lt;", expression);
					gsub(">", "&gt;", expression);
					gsub("\\_", "_", expression);
					gsub("\\[", "[", expression);
					gsub("\\]", "]", expression);
					gsub("\\_", "_", result);
					gsub("\\[", "[", result);
					gsub("\\]", "]", result);
					if(!in_list) in_list = true;
					fputs("<blockquote><para><command>\n", efile);
					fputs(expression.c_str(), efile);
					fputs("</command> <emphasis>", efile);
					fputs(result.c_str(), efile);
					fputs("</emphasis></para></blockquote>\n", efile);
				}
			}
		}
	}
	if(in_sect) {
		fputs("</sect1>\n", efile);
	}
	fputs("</appendix>\n", efile);
	
	fclose(efile);
	fclose(rfile);

	return 0;

}



