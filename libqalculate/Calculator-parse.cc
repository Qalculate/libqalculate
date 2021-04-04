/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016-2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Calculator.h"
#include "BuiltinFunctions.h"
#include "util.h"
#include "MathStructure.h"
#include "Unit.h"
#include "Variable.h"
#include "Function.h"
#include "DataSet.h"
#include "ExpressionItem.h"
#include "Prefix.h"
#include "Number.h"

#include <locale.h>
#include <unistd.h>

using std::string;
using std::cout;
using std::vector;
using std::endl;

#include "Calculator_p.h"

// determine if character is not a numerical digit in a specific number base (-1=base using digits other than 0-9, a-z, A-Z; -12=duodecimal)
bool is_not_number(char c, int base) {
	// 0-9 is always treated as a digit
	if(c >= '0' && c <= '9') return false;
	// in non standard bases every character might be a digit
	if(base == -1) return false;
	// duodecimal bases uses 0-9, E, X
	if(base == -12) return c != 'E' && c != 'X' && c != 'A' && c != 'B' && c != 'a' && c != 'b';
	if(base <= 10) return true;
	// bases 11-36 is case insensitive
	if(base <= 36) {
		if(c >= 'a' && c < 'a' + (base - 10)) return false;
		if(c >= 'A' && c < 'A' + (base - 10)) return false;
		return true;
	}
	// bases 37-62 is case sensitive
	if(base <= 62) {
		if(c >= 'a' && c < 'a' + (base - 36)) return false;
		if(c >= 'A' && c < 'Z') return false;
		return true;
	}
	return false;
}

size_t Calculator::addId(MathStructure *mstruct, bool persistent) {
	size_t id = 0;
	if(priv->freed_ids.size() > 0) {
		id = priv->freed_ids.back();
		priv->freed_ids.pop_back();
	} else {
		priv->ids_i++;
		id = priv->ids_i;
	}
	priv->ids_p[id] = persistent;
	priv->id_structs[id] = mstruct;
	return id;
}
size_t Calculator::parseAddId(MathFunction *f, const string &str, const ParseOptions &po, bool persistent) {
	size_t id = 0;
	if(priv->freed_ids.size() > 0) {
		id = priv->freed_ids.back();
		priv->freed_ids.pop_back();
	} else {
		priv->ids_i++;
		id = priv->ids_i;
	}
	priv->ids_p[id] = persistent;
	priv->id_structs[id] = new MathStructure();
	f->parse(*priv->id_structs[id], str, po);
	return id;
}
size_t Calculator::parseAddIdAppend(MathFunction *f, const MathStructure &append_mstruct, const string &str, const ParseOptions &po, bool persistent) {
	size_t id = 0;
	if(priv->freed_ids.size() > 0) {
		id = priv->freed_ids.back();
		priv->freed_ids.pop_back();
	} else {
		priv->ids_i++;
		id = priv->ids_i;
	}
	priv->ids_p[id] = persistent;
	priv->id_structs[id] = new MathStructure();
	f->parse(*priv->id_structs[id], str, po);
	priv->id_structs[id]->addChild(append_mstruct);
	return id;
}
size_t Calculator::parseAddVectorId(const string &str, const ParseOptions &po, bool persistent) {
	size_t id = 0;
	if(priv->freed_ids.size() > 0) {
		id = priv->freed_ids.back();
		priv->freed_ids.pop_back();
	} else {
		priv->ids_i++;
		id = priv->ids_i;
	}
	priv->ids_p[id] = persistent;
	priv->id_structs[id] = new MathStructure();
	f_vector->args(str, *priv->id_structs[id], po);
	return id;
}
MathStructure *Calculator::getId(size_t id) {
	if(priv->id_structs.find(id) != priv->id_structs.end()) {
		if(priv->ids_p[id]) {
			return new MathStructure(*priv->id_structs[id]);
		} else {
			MathStructure *mstruct = priv->id_structs[id];
			priv->freed_ids.push_back(id);
			priv->id_structs.erase(id);
			priv->ids_p.erase(id);
			return mstruct;
		}
	}
	return NULL;
}

void Calculator::delId(size_t id) {
	if(priv->ids_p.find(id) != priv->ids_p.end()) {
		priv->freed_ids.push_back(id);
		priv->id_structs[id]->unref();
		priv->id_structs.erase(id);
		priv->ids_p.erase(id);
	}
}

// case sensitive string comparison; compares whole name with str from str_index to str_index + name_length
bool compare_name(const string &name, const string &str, const size_t &name_length, const size_t &str_index, int base) {
	if(name_length == 0) return false;
	if(name[0] != str[str_index]) return false;
	if(name_length == 1) {
		if(base < 2 || base > 10) return is_not_number(str[str_index], base);
		return true;
	}
	for(size_t i = 1; i < name_length; i++) {
		if(name[i] != str[str_index + i]) return false;
	}
	// number base uses digits other than 0-9, check that at least one non-digit is used
	if(base < 2 || base > 10) {
		for(size_t i = 0; i < name_length; i++) {
			if(is_not_number(str[str_index + i], base)) return true;
		}
		return false;
	}
	return true;
}

// case insensitive string comparison; compares whole name with str from str_index to str_index + name_length
size_t compare_name_no_case(const string &name, const string &str, const size_t &name_length, const size_t &str_index, int base) {
	if(name_length == 0) return 0;
	size_t is = str_index;
	for(size_t i = 0; i < name_length; i++, is++) {
		if(is >= str.length()) return 0;
		if((name[i] < 0 && i + 1 < name_length) || (str[is] < 0 && is + 1 < str.length())) {
			// assumed Unicode character found
			size_t i2 = 1, is2 = 1;
			// determine length of Unicode character(s)
			if(name[i] < 0) {
				while(i2 + i < name_length && name[i2 + i] < 0) {
					i2++;
				}
			}
			if(str[is] < 0) {
				while(is2 + is < str.length() && str[is2 + is] < 0) {
					is2++;
				}
			}
			// compare characters
			bool isequal = (i2 == is2);
			if(isequal) {
				for(size_t i3 = 0; i3 < i2; i3++) {
					if(str[is + i3] != name[i + i3]) {
						isequal = false;
						break;
					}
				}
			}
			// get lower case character and compare again
			if(!isequal) {
				char *gstr1 = utf8_strdown(name.c_str() + (sizeof(char) * i), i2);
				char *gstr2 = utf8_strdown(str.c_str() + (sizeof(char) * (is)), is2);
				if(!gstr1 || !gstr2) return 0;
				if(strcmp(gstr1, gstr2) != 0) {free(gstr1); free(gstr2); return 0;}
				free(gstr1); free(gstr2);
			}
			i += i2 - 1;
			is += is2 - 1;
		} else if(name[i] != str[is] && !((name[i] >= 'a' && name[i] <= 'z') && name[i] - 32 == str[is]) && !((name[i] <= 'Z' && name[i] >= 'A') && name[i] + 32 == str[is])) {
			return 0;
		}
	}
	// number base uses digits other than 0-9, check that at least one non-digit is used
	if(base < 2 || base > 10) {
		for(size_t i = str_index; i < is; i++) {
			if(is_not_number(str[i], base)) return is - str_index;
		}
		return 0;
	}
	return is - str_index;
}

const char *internal_signs[] = {SIGN_PLUSMINUS, "\b", "+/-", "\b", "⊻", "\a", "∠", "\x1c"};
#define INTERNAL_SIGNS_COUNT 8
#define INTERNAL_NUMBER_CHARS "\b"
#define INTERNAL_OPERATORS "\a\b%\x1c"
#define DUODECIMAL_CHARS "EXABab"

string Calculator::parseComments(string &str, const ParseOptions &po, bool *double_tag) {

	if(str.length() <= 1 || po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return "";

	if(double_tag) *double_tag = false;

	if(str[0] == '#') {
		string from_str = unlocalizeExpression(str, po);
		parseSigns(from_str);
		size_t i = from_str.find_first_of(NOT_IN_NAMES NUMBERS);
		if(from_str.length() == 1 || i == 0 || !getActiveExpressionItem(i == string::npos ? from_str : from_str.substr(0, i))) {
			i = from_str.find_first_of(NOT_IN_NAMES);
			if(from_str.length() == 1 || i == 0 || !getActiveExpressionItem(i == string::npos ? from_str : from_str.substr(0, i))) {
				string to_str = str.substr(1);
				str = "";
				if(to_str[0] == '#') {
					to_str.erase(0, 1);
					if(double_tag) *double_tag = true;
				}
				remove_blank_ends(to_str);
				return to_str;
			}
		}
	}

	size_t i = str.rfind("#");
	if(i == string::npos || i == 0) return "";

	// check if within quoted ranges
	size_t quote_index = 0;
	while(true) {
		quote_index = str.find_first_of("\"\'", quote_index);
		if(quote_index == string::npos || quote_index > i) {
			break;
		}
		quote_index = str.find(str[quote_index], quote_index + 1);
		if(quote_index == string::npos || quote_index > i) {
			return "";
		}
		quote_index++;
	}

	string from_str = CALCULATOR->unlocalizeExpression(str, po);
	parseSigns(from_str);
	size_t i4 = from_str.rfind("#");
	if(i4 != string::npos) {
		size_t i2 = from_str.find_first_of(NOT_IN_NAMES NUMBERS, i4);
		size_t i3 = from_str.find_last_of(NOT_IN_NAMES NUMBERS, i4);
		if(i3 == string::npos) i3 = 0;
		else i3++;
		if((i2 == i4 && i3 == i4)  || !getActiveExpressionItem(i2 == string::npos ? from_str.substr(i3) : from_str.substr(i3, i2 - i3 + 1))) {
			i2 = from_str.find_first_of(NOT_IN_NAMES, i4);
			i3 = from_str.find_last_of(NOT_IN_NAMES, i4);
			if(i3 == string::npos) i3 = 0;
			else i3++;
			i3 = from_str.find_first_not_of(NUMBERS, i3);
			if((i2 == i && i3 == i) || !getActiveExpressionItem(i2 == string::npos ? from_str.substr(i3) : from_str.substr(i3, i2 - i3 + 1))) {
				string to_str = str.substr(i + 1);
				str = str.substr(0, i);
				if(to_str.length() > 1 && to_str[1] == '#') to_str = to_str.substr(1);
				remove_blank_ends(to_str);
				return to_str;
			}
		}
	}
	return "";
}

void Calculator::parseSigns(string &str, bool convert_to_internal_representation) const {

	bool b_unicode = false;
	for(size_t i = 0; i < str.length(); i++) {
		if((unsigned char) str[i] >= 0xC0) {
			b_unicode = true;
			break;
		}
	}

	if(b_unicode && str.find_first_of("\"\'") == string::npos) {
		// replace unicode quotation marks
		size_t ui = str.find("\xe2\x80");
		bool b_double = false, b_single = false, b_angle = false;
		while(ui != string::npos && ui + 2 < str.length()) {
			if(str[ui + 2] >= -101 && str[ui + 2] <= -104) {
				// single quotation marks
				str.replace(ui, 3, "\'");
				b_single = true;
			} else if(str[ui + 2] >= -100 && str[ui + 2] <= -97) {
				// double quotation marks
				str.replace(ui, 3, "\"");
				b_double = true;
			} else if(str[ui + 2] == -70 || str[ui + 2] == -71) {
				// single angle quotation marks
				b_angle = true;
				ui += 2;
			} else {
				ui += 2;
			}
			ui = str.find("\xe2\x80", ui + 1);
		}
		if(!b_single && b_angle) {
			b_single = true;
			gsub("‹", "\'", str);
			gsub("›", "\'", str);
		}
		if(!b_double) {
			gsub("«", "\"", str);
			gsub("»", "\"", str);
		} else if(!b_single) {
			gsub("«", "\'", str);
			gsub("»", "\'", str);
		}
	}

	vector<size_t> q_begin;
	vector<size_t> q_end;
	// collect quoted ranges
	size_t quote_index = 0;
	while(true) {
		quote_index = str.find_first_of("\"\'", quote_index);
		if(quote_index == string::npos) {
			break;
		}
		q_begin.push_back(quote_index);
		quote_index = str.find(str[quote_index], quote_index + 1);
		if(quote_index == string::npos) {
			q_end.push_back(str.length() - 1);
			break;
		}
		q_end.push_back(quote_index);
		quote_index++;
	}

	// search and replace string alternatives
	for(size_t i = 0; i < signs.size(); i++) {
		if(b_unicode || signs[i][0] > 0) {
			size_t ui = str.find(signs[i]);
			size_t ui2 = 0;
			while(ui != string::npos) {
				// check that found index is outside quotes
				for(; ui2 < q_end.size(); ui2++) {
					if(ui >= q_begin[ui2]) {
						if(ui <= q_end[ui2]) {
							ui = str.find(signs[i], q_end[ui2] + 1);
							if(ui == string::npos) break;
						}
					} else {
						break;
					}
				}
				if(ui == string::npos) break;
				// adjust quotion mark indeces
				int index_shift = real_signs[i].length() - signs[i].length();
				for(size_t ui3 = ui2; ui3 < q_begin.size(); ui3++) {
					q_begin[ui3] += index_shift;
					q_end[ui3] += index_shift;
				}
				str.replace(ui, signs[i].length(), real_signs[i]);
				ui = str.find(signs[i], ui + real_signs[i].length());
			}
		}
	}

	if(b_unicode) {

		// replace Unicode exponents
		size_t prev_ui = string::npos, space_n = 0;
		while(true) {
			// Unicode powers 0 and 4-9 use three chars and begin with \xe2\x81
			size_t ui = str.find("\xe2\x81", prev_ui == string::npos ? 0 : prev_ui);
			while(ui != string::npos && (ui == str.length() - 2 || (str[ui + 2] != -80 && (str[ui + 2] < -76 || str[ui + 2] > -66 || str[ui + 2] == -68)))) ui = str.find("\xe2\x81", ui + 3);
			// Unicode powers 1-3 use two chars and begin with \xc2
			size_t ui2 = str.find('\xc2', prev_ui == string::npos ? 0 : prev_ui);
			while(ui2 != string::npos && (ui2 == str.length() - 1 || (str[ui2 + 1] != -71 && str[ui2 + 1] != -77 && str[ui2 + 1] != -78))) ui2 = str.find('\xc2', ui2 + 2);
			if(ui2 != string::npos && (ui == string::npos || ui2 < ui)) ui = ui2;
			if(ui != string::npos) {
				// check that found index is outside quotes
				for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
					if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
						ui = str.find("\xe2\x81", q_end[ui3] + 1);
						if(ui != string::npos && (ui == str.length() - 2 || (str[ui + 2] != -80 && (str[ui + 2] < -76 || str[ui + 2] > -66 || str[ui + 2] != -68)))) ui = string::npos;
						ui2 = str.find('\xc2', q_end[ui3] + 1);
						if(ui2 != string::npos && (ui2 == str.length() - 1 || (str[ui2 + 1] != -71 && str[ui2 + 1] != -77 && str[ui2 + 1] != -78))) ui2 = string::npos;
						if(ui2 != string::npos && (ui == string::npos || ui2 < ui)) ui = ui2;
						if(ui == string::npos) break;
					}
				}
			}
			if(ui == string::npos) break;
			int index_shift = (str[ui] == '\xc2' ? -2 : -3);
			if(ui == prev_ui) index_shift += 1;
			else index_shift += 4;
			// adjust quotion mark indeces
			for(size_t ui3 = 0; ui3 < q_begin.size(); ui3++) {
				if(q_begin[ui3] >= ui) {
					q_begin[ui3] += index_shift;
					q_end[ui3] += index_shift;
				}
			}
			// perform replacement; if next to previous Unicode power combine the powers
			if(str[ui] == '\xc2') {
				if(str[ui + 1] == -71) str.replace(ui, 2, ui == prev_ui ? "1)" : "^(1)");
				else if(str[ui + 1] == -78) str.replace(ui, 2, ui == prev_ui ? "2)" : "^(2)");
				else if(str[ui + 1] == -77) str.replace(ui, 2, ui == prev_ui ? "3)" : "^(3)");
			} else {
				if(str[ui + 2] == -80) str.replace(ui, 3, ui == prev_ui ? "0)" : "^(0)");
				else if(str[ui + 2] == -76) str.replace(ui, 3, ui == prev_ui ? "4)" : "^(4)");
				else if(str[ui + 2] == -75) str.replace(ui, 3, ui == prev_ui ? "5)" : "^(5)");
				else if(str[ui + 2] == -74) str.replace(ui, 3, ui == prev_ui ? "6)" : "^(6)");
				else if(str[ui + 2] == -73) str.replace(ui, 3, ui == prev_ui ? "7)" : "^(7)");
				else if(str[ui + 2] == -72) str.replace(ui, 3, ui == prev_ui ? "8)" : "^(8)");
				else if(str[ui + 2] == -71) str.replace(ui, 3, ui == prev_ui ? "9)" : "^(9)");
				else if(str[ui + 2] == -70) str.replace(ui, 3, ui == prev_ui ? "+)" : "^(+)");
				else if(str[ui + 2] == -69) str.replace(ui, 3, ui == prev_ui ? "-)" : "^(-)");
				else if(str[ui + 2] == -67) str.replace(ui, 3, ui == prev_ui ? "()" : "^(()");
				else if(str[ui + 2] == -66) str.replace(ui, 3, ui == prev_ui ? "))" : "^())");
			}
			if(ui == prev_ui) {
				str.erase(prev_ui - space_n - 1, 1);
				prev_ui = ui + 1;
			} else {
				prev_ui = ui + 4;
			}
			space_n = 0;
			while(prev_ui + 1 < str.length() && str[prev_ui] == SPACE_CH) {
				space_n++;
				prev_ui++;
			}
		}

		// replace Unicode fractions with three chars
		prev_ui = string::npos;
		while(true) {
			// three char Unicode fractions begin with \xe2\x85
			size_t ui = str.find("\xe2\x85", prev_ui == string::npos ? 0 : prev_ui);
			while(ui != string::npos && (ui == str.length() - 2 || str[ui + 2] < -112 || str[ui + 2] > -98)) ui = str.find("\xe2\x85", ui + 3);
			if(ui != string::npos) {
				// check that found index is outside quotes
				for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
					if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
						ui = str.find("\xe2\x85", q_end[ui3] + 1);
						if(ui != string::npos && (ui == str.length() - 2 || str[ui + 2] < -112 || str[ui + 2] > -98)) ui = string::npos;
						if(ui == string::npos) break;
					}
				}
			}
			if(ui == string::npos) break;
			// check if previous non-whitespace character is a numeral digit
			space_n = 0;
			while(ui > 0 && ui - 1 - space_n != 0 && str[ui - 1 - space_n] == SPACE_CH) space_n++;
			bool b_add = (ui > 0 && is_in(NUMBER_ELEMENTS, str[ui - 1 - space_n]));
			if(b_add) {
				size_t ui2 = str.find_last_not_of(NUMBER_ELEMENTS, ui - space_n - 1);
				if(ui2 == string::npos) ui2 = 0;
				else ui2++;
				str.insert(ui2, "(");
				ui += 1;
			}
			int index_shift = (b_add ? 7 : 5) - 3;
			if(str[ui + 2] == -110) index_shift++;
			// adjust quotion mark indeces
			for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
				if(q_begin[ui2] >= ui) {
					q_begin[ui2] += index_shift;
					q_end[ui2] += index_shift;
				}
			}
			// perform replacement; interpret as addition if previous character is a numeral digit
			if(str[ui + 2] == -98) str.replace(ui, 3, b_add ? "+(7/8))" : "(7/8)");
			else if(str[ui + 2] == -99) str.replace(ui, 3, b_add ? "+(5/8))" : "(5/8)");
			else if(str[ui + 2] == -100) str.replace(ui, 3, b_add ? "+(3/8))" : "(3/8)");
			else if(str[ui + 2] == -101) str.replace(ui, 3, b_add ? "+(1/8))" : "(1/8)");
			else if(str[ui + 2] == -102) str.replace(ui, 3, b_add ? "+(5/6))" : "(5/6)");
			else if(str[ui + 2] == -103) str.replace(ui, 3, b_add ? "+(1/6))" : "(1/6)");
			else if(str[ui + 2] == -104) str.replace(ui, 3, b_add ? "+(4/5))" : "(4/5)");
			else if(str[ui + 2] == -105) str.replace(ui, 3, b_add ? "+(3/5))" : "(3/5)");
			else if(str[ui + 2] == -106) str.replace(ui, 3, b_add ? "+(2/5))" : "(2/5)");
			else if(str[ui + 2] == -107) str.replace(ui, 3, b_add ? "+(1/5))" : "(1/5)");
			else if(str[ui + 2] == -108) str.replace(ui, 3, b_add ? "+(2/3))" : "(2/3)");
			else if(str[ui + 2] == -109) str.replace(ui, 3, b_add ? "+(1/3))" : "(1/3)");
			else if(str[ui + 2] == -110) {str.replace(ui, 3, b_add ? "+(1/10))" : "(1/10)"); ui++;}
			else if(str[ui + 2] == -111) str.replace(ui, 3, b_add ? "+(1/9))" : "(1/9)");
			else if(str[ui + 2] == -112) str.replace(ui, 3, b_add ? "+(1/7))" : "(1/7)");
			if(b_add) prev_ui = ui + 7;
			else prev_ui = ui + 5;
		}

		// replace Unicode fractions with two chars
		prev_ui = string::npos;
		while(true) {
			// two char Unicode fractions begin with \xc2
			size_t ui = str.find('\xc2', prev_ui == string::npos ? 0 : prev_ui);
			while(ui != string::npos && (ui == str.length() - 1 || (str[ui + 1] != -66 && str[ui + 1] != -67 && str[ui + 1] != -68))) ui = str.find('\xc2', ui + 2);
			if(ui != string::npos) {
				// check that found index is outside quotes
				for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
					if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
						ui = str.find('\xc2', q_end[ui3] + 1);
						if(ui != string::npos && (ui == str.length() - 1 || (str[ui + 1] != -66 && str[ui + 1] != -67 && str[ui + 1] != -68))) ui = string::npos;
						if(ui == string::npos) break;
					}
				}
			}
			if(ui == string::npos) break;
			// check if previous non-whitespace character is a numeral digit
			space_n = 0;
			while(ui > 0 && ui - 1 - space_n != 0 && str[ui - 1 - space_n] == SPACE_CH) space_n++;
			bool b_add = (ui > 0 && is_in(NUMBER_ELEMENTS, str[ui - 1 - space_n]));
			if(b_add) {
				size_t ui2 = str.find_last_not_of(NUMBER_ELEMENTS, ui - space_n - 1);
				if(ui2 == string::npos) ui2 = 0;
				else ui2++;
				str.insert(ui2, "(");
				ui += 1;
			}
			int index_shift = (b_add ? 7 : 5) - 2;
			// adjust quotion mark indeces
			for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
				if(q_begin[ui2] >= ui) {
					q_begin[ui2] += index_shift;
					q_end[ui2] += index_shift;
				}
			}
			// perform replacement; interpret as addition if previous character is a numeral digit
			if(str[ui + 1] == -66) str.replace(ui, 2, b_add ? "+(3/4))" : "(3/4)");
			else if(str[ui + 1] == -67) str.replace(ui, 2, b_add ? "+(1/2))" : "(1/2)");
			else if(str[ui + 1] == -68) str.replace(ui, 2, b_add ? "+(1/4))" : "(1/4)");
			if(b_add) prev_ui = ui + 7;
			else prev_ui = ui + 5;
		}

	}

	if(convert_to_internal_representation) {
		// remove superfluous whitespace
		remove_blank_ends(str);
		remove_duplicate_blanks(str);
		// replace operators with multiple chars with internal single character version
		for(size_t i = 0; i < INTERNAL_SIGNS_COUNT; i += 2) {
			if(b_unicode || internal_signs[i][0] > 0) {
				size_t ui = str.find(internal_signs[i]);
				size_t ui2 = 0;
				while(ui != string::npos) {
					// check that found index is outside quotes
					for(; ui2 < q_end.size(); ui2++) {
						if(ui >= q_begin[ui2]) {
							if(ui <= q_end[ui2]) {
								ui = str.find(internal_signs[i], q_end[ui2] + 1);
								if(ui == string::npos) break;
							}
						} else {
							break;
						}
					}
					if(ui == string::npos) break;
					// adjust quotion mark indeces
					int index_shift = strlen(internal_signs[i + 1]) - strlen(internal_signs[i]);
					for(size_t ui3 = ui2; ui3 < q_begin.size(); ui3++) {
						q_begin[ui3] += index_shift;
						q_end[ui3] += index_shift;
					}
					// perform replacement and search for next occurrence
					str.replace(ui, strlen(internal_signs[i]), internal_signs[i + 1]);
					ui = str.find(internal_signs[i], ui + strlen(internal_signs[i + 1]));
				}
			}
		}
	}
}


MathStructure Calculator::parse(string str, const ParseOptions &po) {

	MathStructure mstruct;
	parse(&mstruct, str, po);
	return mstruct;

}

void replace_internal_operators(string &str) {
	gsub("\a", " xor ", str);
	gsub("\b", SIGN_PLUSMINUS, str);
	gsub("\x1c", "∠", str);
	remove_blank_ends(str);
}

void Calculator::parse(MathStructure *mstruct, string str, const ParseOptions &parseoptions) {

	ParseOptions po = parseoptions;

	if(po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) {
		// Read whole expression as a number if the number base digits other than alphanumerical characters
		mstruct->set(Number(str, po));
		return;
	}

	if(po.base != BASE_DUODECIMAL && (str.find("↊") != string::npos || str.find("↋") != string::npos)) {
		po.base = BASE_DUODECIMAL;
		parse(mstruct, str, po);
		return;
	}

	if(po.rpn) {po.parsing_mode = PARSING_MODE_RPN; po.rpn = false;}

	MathStructure *unended_function = po.unended_function;
	po.unended_function = NULL;

	// use parse option number base to determine which characters are used as numerical digits and set base accordingly.
	// (-1=base using digits other than 0-9, a-z, A-Z; -12=duodecimal)
	int base = po.base;
	if(base == BASE_CUSTOM) {
		base = (int) priv->custom_input_base_i;
	} else if(base == BASE_GOLDEN_RATIO || base == BASE_SUPER_GOLDEN_RATIO || base == BASE_SQRT2) {
		base = 2;
	} else if(base == BASE_PI) {
		base = 4;
	} else if(base == BASE_E) {
		base = 3;
	} else if(base == BASE_DUODECIMAL) {
		base = -12;
	} else if(base < 2 || base > 36) {
		base = -1;
	}

	mstruct->clear();

	const string *name = NULL;
	string stmp, stmp2;

	// search for degree sign in epxressions (affects interpretation of ' and ")
	size_t i_degree = str.find(SIGN_DEGREE);
	if(i_degree != string::npos && i_degree + strlen(SIGN_DEGREE) < str.length() && is_in("CRF", str[i_degree + strlen(SIGN_DEGREE)])) {
		i_degree = string::npos;
	}

	if(base != -1 && base <= BASE_HEXADECIMAL) {
		// replace single ' and " with prime and double prime (for ft/in or minutes/seconds of arc)
		bool b_prime_quote = true;
		size_t i_quote = str.find('\'', 0);
		size_t i_dquote = str.find('\"', 0);
		if(i_degree == string::npos) {
			if(i_quote == 0 || i_dquote == 0) {
				b_prime_quote = false;
			} else if((i_quote != string::npos && i_quote < str.length() - 1 && str.find('\'', i_quote + 1) != string::npos) || (i_quote != string::npos && i_dquote == i_quote + 1) || (i_dquote != string::npos && i_dquote < str.length() - 1 && str.find('\"', i_dquote + 1) != string::npos)) {
				b_prime_quote = false;
				while(i_dquote != string::npos) {
					i_quote = str.rfind('\'', i_dquote - 1);
					if(i_quote != string::npos) {
						size_t i_prev = str.find_last_not_of(SPACES, i_quote - 1);
						if(i_prev != string::npos && is_in(NUMBER_ELEMENTS, str[i_prev])) {
							if(is_in(NUMBER_ELEMENTS, str[str.find_first_not_of(SPACES, i_quote + 1)]) && str.find_first_not_of(SPACES NUMBER_ELEMENTS, i_quote + 1) == i_dquote) {
								if(i_prev == 0) {
									b_prime_quote = true;
									break;
								} else {
									i_prev = str.find_last_not_of(NUMBER_ELEMENTS, i_prev - 1);
									if(i_prev == string::npos || (str[i_prev] != '\"' && str[i_prev] != '\'')) {
										b_prime_quote = true;
										break;
									}
								}
							}
						}
					}
					i_dquote = str.find('\"', i_dquote + 2);
				}
			}
		} else {
			for(size_t i = 0; i < 2 && b_prime_quote; i++) {
				if(i == 1) i_quote = i_dquote;
				while(i_quote != string::npos) {
					if(i_quote > 0 && (str[i_quote - 1] == LEFT_PARENTHESIS_CH || str[i_quote - 1] == COMMA_CH || is_in(SPACES, str[i_quote - 1]))) {
						size_t i_bspace = 0;
						if(str[i_quote - 1] == LEFT_PARENTHESIS_CH || str[i_quote - 1] == COMMA_CH || ((i_bspace = str.find_last_not_of(SPACES, i_quote - 1)) != string::npos && is_in(LEFT_PARENTHESIS COMMA, str[i_bspace]))) {
							i_quote = str.find(i == 0 ? '\'' : '\"', i_quote + 1);
							if(i_quote != string::npos) b_prime_quote = false;
							break;
						} else {
							i_quote = str.find(i == 0 ? '\'' : '\"', i_quote + 1);
						}
					} else {
						i_quote = str.find(i == 0 ? '\'' : '\"', i_quote + 1);
					}
				}
			}
		}
		if(b_prime_quote) {
			gsub("\'", "′", str);
			gsub("\"", "″", str);
		}
	}

	// replace alternative strings (primarily operators) with default ascii versions
	parseSigns(str, true);

	// parse quoted string as symbolic MathStructure
	for(size_t str_index = 0; str_index < str.length(); str_index++) {
		if(str[str_index] == '\"' || str[str_index] == '\'') {
			if(str_index == str.length() - 1) {
				str.erase(str_index, 1);
			} else {
				size_t i = str.find(str[str_index], str_index + 1);
				size_t name_length;
				if(i == string::npos) {
					i = str.length();
					name_length = i - str_index;
				} else {
					name_length = i - str_index + 1;
				}
				stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
				MathStructure *mstruct = new MathStructure(str.substr(str_index + 1, i - str_index - 1));
				stmp += i2s(addId(mstruct));
				stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				str.replace(str_index, name_length, stmp);
				str_index += stmp.length() - 1;
			}
		}
	}


	if(po.brackets_as_parentheses) {
		// replace [ and ] with ( and )
		gsub(LEFT_VECTOR_WRAP, LEFT_PARENTHESIS, str);
		gsub(RIGHT_VECTOR_WRAP, RIGHT_PARENTHESIS, str);
	}

	// Transform var:=a to save(save, a)
	size_t isave = 0;
	if((isave = str.find(":=", 1)) != string::npos) {
		string name = str.substr(0, isave);
		string value = str.substr(isave + 2, str.length() - (isave + 2));
		str = value;
		str += COMMA;
		str += name;
		f_save->parse(*mstruct, str, po);
		return;
	}

	if(po.default_dataset != NULL && str.length() > 1) {
		size_t str_index = str.find(DOT_CH, 1);
		while(str_index != string::npos) {
			if(str_index + 1 < str.length() && ((is_not_number(str[str_index + 1], base) && is_not_in(INTERNAL_OPERATORS NOT_IN_NAMES, str[str_index + 1]) && is_not_in(INTERNAL_OPERATORS NOT_IN_NAMES, str[str_index - 1])) || (is_not_in(INTERNAL_OPERATORS NOT_IN_NAMES, str[str_index + 1]) && is_not_number(str[str_index - 1], base) && is_not_in(INTERNAL_OPERATORS NOT_IN_NAMES, str[str_index - 1])))) {
				size_t dot_index = str.find_first_of(NOT_IN_NAMES INTERNAL_OPERATORS DOT, str_index + 1);
				if(dot_index != string::npos && str[dot_index] == DOT_CH) {
					str_index = dot_index;
				} else {
					size_t property_index = str.find_last_of(NOT_IN_NAMES INTERNAL_OPERATORS, str_index - 1);
					if(property_index == string::npos) {
						str.insert(0, 1, '.');
						str.insert(0, po.default_dataset->referenceName());
						str_index += po.default_dataset->referenceName().length() + 1;
					} else {
						str.insert(property_index + 1, 1, '.');
						str.insert(property_index + 1, po.default_dataset->referenceName());
						str_index += po.default_dataset->referenceName().length() + 1;
					}
				}
			}
			str_index = str.find(DOT_CH, str_index + 1);
		}
	}

	//remove spaces in numbers
	size_t space_i = 0;
	if(po.parsing_mode != PARSING_MODE_RPN) {
		space_i = str.find(SPACE_CH, 0);
		while(space_i != string::npos) {
			if(is_in(NUMBERS INTERNAL_NUMBER_CHARS DOT, str[space_i + 1]) && is_in(NUMBERS INTERNAL_NUMBER_CHARS DOT, str[space_i - 1])) {
				str.erase(space_i, 1);
				space_i--;
			}
			space_i = str.find(SPACE_CH, space_i + 1);
		}
	}

	if(base != -1 && base <= BASE_HEXADECIMAL) {
		// replace prime and double prime with feet and inches, or arcminues and arcseconds (if degree sign was previously found)
		bool b_degree = (i_degree != string::npos);
		size_t i_quote = str.find("′");
		size_t i_dquote = str.find("″");
		while(i_quote != string::npos || i_dquote != string::npos) {
			size_t i_op = 0;
			if(i_quote == string::npos || i_dquote < i_quote) {
				bool b = false;
				if(b_degree) {
					i_degree = str.rfind(SIGN_DEGREE, i_dquote - 1);
					if(i_degree != string::npos && i_degree > 0 && i_degree < i_dquote) {
						size_t i_op = str.find_first_not_of(SPACE, i_degree + strlen(SIGN_DEGREE));
						if(i_op != string::npos) {
							i_op = str.find_first_not_of(SPACE, i_degree + strlen(SIGN_DEGREE));
							if(is_in(NUMBER_ELEMENTS, str[i_op])) i_op = str.find_first_not_of(NUMBER_ELEMENTS SPACE, i_op);
							else i_op = 0;
						}
						size_t i_prev = string::npos;
						if(i_op == i_dquote) {
							i_prev = str.find_last_not_of(SPACE, i_degree - 1);
							if(i_prev != string::npos) {
								if(is_in(NUMBER_ELEMENTS, str[i_prev])) {
									i_prev = str.find_last_not_of(NUMBER_ELEMENTS SPACE, i_prev);
									if(i_prev == string::npos) i_prev = 0;
									else i_prev++;
								} else {
									i_prev = string::npos;
								}
							}
						}
						if(i_prev != string::npos) {
							str.insert(i_prev, LEFT_PARENTHESIS);
							i_degree++;
							i_op++;
							str.replace(i_op, strlen("″"), "arcsec" RIGHT_PARENTHESIS);
							str.replace(i_degree, strlen(SIGN_DEGREE), "deg" PLUS);
							b = true;
						}
					}
				}
				if(!b) {
					if(str.length() >= i_dquote + strlen("″") && is_in(NUMBERS, str[i_dquote + strlen("″")])) str.insert(i_dquote + strlen("″"), " ");
					str.replace(i_dquote, strlen("″"), b_degree ? "arcsec" : "in");
					i_op = i_dquote;
				}
			} else {
				bool b = false;
				if(b_degree) {
					i_degree = str.rfind(SIGN_DEGREE, i_quote - 1);
					if(i_degree != string::npos && i_degree > 0 && i_degree < i_quote) {
						size_t i_op = str.find_first_not_of(SPACE, i_degree + strlen(SIGN_DEGREE));
						if(i_op != string::npos) {
							i_op = str.find_first_not_of(SPACE, i_degree + strlen(SIGN_DEGREE));
							if(is_in(NUMBER_ELEMENTS, str[i_op])) i_op = str.find_first_not_of(NUMBER_ELEMENTS SPACE, i_op);
							else i_op = 0;
						}
						size_t i_prev = string::npos;
						if(i_op == i_quote) {
							i_prev = str.find_last_not_of(SPACE, i_degree - 1);
							if(i_prev != string::npos) {
								if(is_in(NUMBER_ELEMENTS, str[i_prev])) {
									i_prev = str.find_last_not_of(NUMBER_ELEMENTS SPACE, i_prev);
									if(i_prev == string::npos) i_prev = 0;
									else i_prev++;
								} else {
									i_prev = string::npos;
								}
							}
						}
						if(i_prev != string::npos) {
							str.insert(i_prev, LEFT_PARENTHESIS);
							i_degree++;
							i_quote++;
							i_op++;
							if(i_dquote != string::npos) {
								i_dquote++;
								size_t i_op2 = str.find_first_not_of(SPACE, i_quote + strlen("′"));
								if(i_op2 != string::npos && is_in(NUMBER_ELEMENTS, str[i_op2])) i_op2 = str.find_first_not_of(NUMBER_ELEMENTS SPACE, i_op2);
								else i_op2 = 0;
								if(i_op2 == i_dquote) {
									str.replace(i_dquote, strlen("″"), "arcsec" RIGHT_PARENTHESIS);
									i_op = i_op2;
								}
							}
							str.replace(i_quote, strlen("′"), i_op == i_quote ? "arcmin" RIGHT_PARENTHESIS : "arcmin" PLUS);
							str.replace(i_degree, strlen(SIGN_DEGREE), "deg" PLUS);
							b = true;
						}
					}
				}
				if(!b) {
					i_op = str.find_first_not_of(SPACE, i_quote + strlen("′"));
					if(i_op != string::npos && is_in(NUMBER_ELEMENTS, str[i_op])) i_op = str.find_first_not_of(NUMBER_ELEMENTS SPACE, i_op);
					else i_op = 0;
					size_t i_prev = string::npos;
					if(((!b_degree && i_op == string::npos) || i_op == i_dquote) && i_quote != 0) {
						i_prev = str.find_last_not_of(SPACE, i_quote - 1);
						if(i_prev != string::npos) {
							if(is_in(NUMBER_ELEMENTS, str[i_prev])) {
								i_prev = str.find_last_not_of(NUMBER_ELEMENTS SPACE, i_prev);
								if(i_prev == string::npos) i_prev = 0;
								else i_prev++;
							} else {
								i_prev = string::npos;
							}
						}
					}
					if(i_prev != string::npos) {
						str.insert(i_prev, LEFT_PARENTHESIS);
						i_quote++;
						if(i_op == string::npos) str += b_degree ? "arcsec" RIGHT_PARENTHESIS : "in" RIGHT_PARENTHESIS;
						else str.replace(i_op + 1, strlen("″"), b_degree ? "arcsec" RIGHT_PARENTHESIS : "in" RIGHT_PARENTHESIS);
						str.replace(i_quote, strlen("′"), b_degree ? "arcmin" PLUS : "ft" PLUS);
						if(i_op == string::npos) break;
						i_op++;
					} else {
						if(str.length() >= i_quote + strlen("′") && is_in(NUMBERS, str[i_quote + strlen("′")])) str.insert(i_quote + strlen("′"), " ");
						str.replace(i_quote, strlen("′"), b_degree ? "arcmin" : "ft");
						i_op = i_quote;
					}
				}
			}
			if(i_dquote != string::npos) i_dquote = str.find("″", i_op);
			if(i_quote != string::npos) i_quote = str.find("′", i_op);
		}
	}

	// Replace % with percent in case when it should not be interpreted as mod/rem
	size_t i_mod = str.find("%");
	if(i_mod != string::npos && !v_percent->hasName("%")) i_mod = string::npos;
	while(i_mod != string::npos) {
		if(po.parsing_mode == PARSING_MODE_RPN) {
			if(i_mod == 0 || is_not_in(OPERATORS "\\" INTERNAL_OPERATORS SPACE, str[i_mod - 1])) {
				str.replace(i_mod, 1, v_percent->referenceName());
				i_mod += v_percent->referenceName().length() - 1;
			}
		} else if(i_mod == 0 || i_mod == str.length() - 1 || (is_in(RIGHT_PARENTHESIS RIGHT_VECTOR_WRAP COMMA OPERATORS "%\a\b", str[i_mod + 1]) && str[i_mod + 1] != BITWISE_NOT_CH && str[i_mod + 1] != NOT_CH) || is_in(LEFT_PARENTHESIS LEFT_VECTOR_WRAP COMMA OPERATORS "\a\b", str[i_mod - 1])) {
			str.replace(i_mod, 1, v_percent->referenceName());
			i_mod += v_percent->referenceName().length() - 1;
		}
		i_mod = str.find("%", i_mod + 1);
	}

	size_t i_dx = str.find("dx", 4);
	while(i_dx != string::npos) {
		i_dx++;
		if(i_dx == str.length() - 1 || is_in(NUMBERS NOT_IN_NAMES, str[i_dx + 1])) {
			size_t l_dx = 2;
			if(i_dx > 4 && str[i_dx - 2] == SPACE_CH) l_dx++;
			if(str[i_dx - l_dx] == DIVISION_CH) {
				l_dx++;
				if(i_dx - l_dx > 0 && str[i_dx - l_dx] == SPACE_CH) l_dx++;
				if(i_dx - l_dx > 0 && str[i_dx - l_dx] == RIGHT_PARENTHESIS_CH) {
					size_t i_par = 1;
					size_t i_d = 0;
					for(i_d = i_dx - l_dx - 1; i_d > 0; i_d--) {
						if(str[i_d] == RIGHT_PARENTHESIS_CH) {
							i_par++;
						} else if(str[i_d] == LEFT_PARENTHESIS_CH) {
							i_par--;
							if(i_par == 0) break;
						}
					}
					if(i_par == 0) {
						i_d--;
						if(i_d > 0 && str[i_d] == SPACE_CH) i_d--;
						if(str[i_d] == 'd') {
							i_dx += (str[i_d + 1] == SPACE_CH ? 2 : 3);
							str.replace(i_d, str[i_d + 1] == SPACE_CH ? 2 : 1, "diff");
							str.replace(i_dx - l_dx, l_dx + 1, COMMA "x" RIGHT_PARENTHESIS);
							i_dx += 2;
						}
					}
				}
			}
		}
		i_dx = str.find("dx", i_dx + 1);
	}

	if(po.parsing_mode == PARSING_MODE_RPN) {
		// add space between double operators in rpn mode in order to ensure that they are interpreted as two single operators
		gsub("&&", "& &", str);
		gsub("||", "| |", str);
		gsub("\%\%", "\% \%", str);
	}

	for(size_t str_index = 0; str_index < str.length(); str_index++) {
		if(str[str_index] == LEFT_VECTOR_WRAP_CH) {
			// vector
			int i4 = 1;
			size_t i3 = str_index;
			while(true) {
				i3 = str.find_first_of(LEFT_VECTOR_WRAP RIGHT_VECTOR_WRAP, i3 + 1);
				if(i3 == string::npos) {
					for(; i4 > 0; i4--) {
						str += RIGHT_VECTOR_WRAP;
					}
					i3 = str.length() - 1;
				} else if(str[i3] == LEFT_VECTOR_WRAP_CH) {
					i4++;
				} else if(str[i3] == RIGHT_VECTOR_WRAP_CH) {
					i4--;
					if(i4 > 0) {
						size_t i5 = str.find_first_not_of(SPACE, i3 + 1);
						if(i5 != string::npos && str[i5] == LEFT_VECTOR_WRAP_CH) {
							str.insert(i5, COMMA);
						}
					}
				}
				if(i4 == 0) {
					stmp2 = str.substr(str_index + 1, i3 - str_index - 1);
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					stmp += i2s(parseAddVectorId(stmp2, po));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(str_index, i3 + 1 - str_index, stmp);
					str_index += stmp.length() - 1;
					break;
				}
			}
		} else if(str[str_index] == '\\' && str_index + 1 < str.length() && (is_not_in(NOT_IN_NAMES INTERNAL_OPERATORS NUMBERS, str[str_index + 1]) || (po.parsing_mode != PARSING_MODE_RPN && str_index > 0 && is_in(NUMBERS SPACE PLUS MINUS BITWISE_NOT NOT LEFT_PARENTHESIS, str[str_index + 1])))) {
			if(is_in(NUMBERS SPACE PLUS MINUS BITWISE_NOT NOT LEFT_PARENTHESIS, str[str_index + 1])) {
				// replace \ followed by number with // for integer division
				str.replace(str_index, 1, "//");
				str_index++;
			} else {
				// replaced \ followed by a character with symbolic MathStructure
				stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
				size_t l = 1;
				if(str[str_index + l] < 0) {
					do {
						l++;
					} while(str_index + l < str.length() && str[str_index + l] < 0 && (unsigned char) str[str_index + l] < 0xC0);
					l--;
				}
				MathStructure *mstruct = new MathStructure(str.substr(str_index + 1, l));
				stmp += i2s(addId(mstruct));
				stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				str.replace(str_index, l + 1, stmp);
				str_index += stmp.length() - l;
			}
		} else if(str[str_index] == '!' && po.functions_enabled) {
			// replace ! with factorial function when appropriate
			if(str_index > 0 && (str.length() - str_index == 1 || str[str_index + 1] != EQUALS_CH)) {
				stmp2 = "";
				size_t i5 = str.find_last_not_of(SPACE, str_index - 1);
				size_t i3;
				if(i5 == string::npos) {
				} else if(str[i5] == RIGHT_PARENTHESIS_CH) {
					if(i5 == 0) {
						stmp2 = str.substr(0, i5 + 1);
					} else {
						i3 = i5 - 1;
						size_t i4 = 1;
						while(true) {
							i3 = str.find_last_of(LEFT_PARENTHESIS RIGHT_PARENTHESIS, i3);
							if(i3 == string::npos) {
								stmp2 = str.substr(0, i5 + 1);
								break;
							}
							if(str[i3] == RIGHT_PARENTHESIS_CH) {
								i4++;
							} else {
								i4--;
								if(i4 == 0) {
									stmp2 = str.substr(i3, i5 + 1 - i3);
									break;
								}
							}
							if(i3 == 0) {
								stmp2 = str.substr(0, i5 + 1);
								break;
							}
							i3--;
						}
					}
				} else if(str[i5] == ID_WRAP_RIGHT_CH && (i3 = str.find_last_of(ID_WRAP_LEFT, i5 - 1)) != string::npos) {
					stmp2 = str.substr(i3, i5 + 1 - i3);
				} else if(is_not_in(RESERVED OPERATORS INTERNAL_OPERATORS SPACES VECTOR_WRAPS PARENTHESISS COMMAS, str[i5])) {
					i3 = str.find_last_of(RESERVED OPERATORS INTERNAL_OPERATORS SPACES VECTOR_WRAPS PARENTHESISS COMMAS, i5);
					if(i3 == string::npos) {
						stmp2 = str.substr(0, i5 + 1);
					} else {
						stmp2 = str.substr(i3 + 1, i5 - i3);
					}
				}
				if(!stmp2.empty()) {
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					int ifac = 1;
					i3 = str_index + 1;
					size_t i4 = i3;
					while((i3 = str.find_first_not_of(SPACE, i3)) != string::npos && str[i3] == '!') {
						ifac++;
						i3++;
						i4 = i3;
					}
					if(ifac == 2) stmp += i2s(parseAddId(f_factorial2, stmp2, po));
					else if(ifac == 1) stmp += i2s(parseAddId(f_factorial, stmp2, po));
					else stmp += i2s(parseAddIdAppend(f_multifactorial, MathStructure(ifac, 1, 0), stmp2, po));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(i5 - stmp2.length() + 1, stmp2.length() + i4 - i5 - 1, stmp);
					str_index = stmp.length() + i5 - stmp2.length();
				}
			}
		} else if(po.parsing_mode != PARSING_MODE_RPN && (str[str_index] == 'c' || str[str_index] == 'C') && str.length() > str_index + 6 && str[str_index + 5] == SPACE_CH && (str_index == 0 || is_in(OPERATORS INTERNAL_OPERATORS PARENTHESISS, str[str_index - 1])) && compare_name_no_case("compl", str, 5, str_index, base)) {
			// interprate "compl" followed by space as bitwise not
			str.replace(str_index, 6, BITWISE_NOT);
		} else if(str[str_index] == SPACE_CH) {
			size_t i = str.find(SPACE, str_index + 1);
			if(po.parsing_mode == PARSING_MODE_RPN && i == string::npos) i = str.length();
			if(i != string::npos) {
				// replace text operators, surrounded by space, with default operator characters
				i -= str_index + 1;
				size_t il = 0;
				if(i == per_str_len && (il = compare_name_no_case(per_str, str, per_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, DIVISION);
					str_index++;
				} else if(i == times_str_len && (il = compare_name_no_case(times_str, str, times_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, MULTIPLICATION);
					str_index++;
				} else if(i == plus_str_len && (il = compare_name_no_case(plus_str, str, plus_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, PLUS);
					str_index++;
				} else if(i == minus_str_len && (il = compare_name_no_case(minus_str, str, minus_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, MINUS);
					str_index++;
				} else if(and_str_len > 0 && i == and_str_len && (il = compare_name_no_case(and_str, str, and_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_AND);
					str_index += 2;
				} else if(i == AND_str_len && (il = compare_name_no_case(AND_str, str, AND_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_AND);
					str_index += 2;
				} else if(or_str_len > 0 && i == or_str_len && (il = compare_name_no_case(or_str, str, or_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_OR);
					str_index += 2;
				} else if(i == OR_str_len && (il = compare_name_no_case(OR_str, str, OR_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_OR);
					str_index += 2;
				} else if(i == XOR_str_len && (il = compare_name_no_case(XOR_str, str, XOR_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\a");
					str_index++;
				} else if(i == 5 && (il = compare_name_no_case("bitor", str, 5, str_index + 1, base))) {
					str.replace(str_index + 1, il, BITWISE_OR);
					str_index++;
				} else if(i == 6 && (il = compare_name_no_case("bitand", str, 6, str_index + 1, base))) {
					str.replace(str_index + 1, il, BITWISE_AND);
					str_index++;
				} else if(i == 3 && (il = compare_name_no_case("mod", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\%\%");
					str_index += 2;
				} else if(i == 3 && (il = compare_name_no_case("rem", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il, "%");
					str_index++;
				} else if(i == 3 && (il = compare_name_no_case("div", str, 3, str_index + 1, base))) {
					if(po.parsing_mode == PARSING_MODE_RPN) {
						str.replace(str_index + 1, il, "\\");
						str_index++;
					} else {
						str.replace(str_index + 1, il, "//");
						str_index += 2;
					}
				}
			}
		} else if(str_index > 0 && base >= 2 && base <= 10 && is_in(EXPS, str[str_index]) && str_index + 1 < str.length() && (is_in(NUMBER_ELEMENTS, str[str_index + 1]) || (is_in(PLUS MINUS, str[str_index + 1]) && str_index + 2 < str.length() && is_in(NUMBER_ELEMENTS, str[str_index + 2]))) && is_in(NUMBER_ELEMENTS, str[str_index - 1])) {
			//don't do anything when e is used instead of E for EXP
		} else if(base <= 33 && str[str_index] == '0' && (str_index == 0 || is_in(NOT_IN_NAMES INTERNAL_OPERATORS, str[str_index - 1]))) {
			if(str_index + 2 < str.length() && (str[str_index + 1] == 'x' || str[str_index + 1] == 'X') && is_in(NUMBER_ELEMENTS "abcdefABCDEF", str[str_index + 2])) {
				//hexadecimal number 0x...
				if(po.base == BASE_HEXADECIMAL) {
					str.erase(str_index, 2);
				} else {
					size_t i;
					if(po.parsing_mode == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS "abcdefABCDEF", str_index + 2);
					else i = str.find_first_not_of(SPACE NUMBER_ELEMENTS "abcdefABCDEF", str_index + 2);
					size_t name_length;
					if(i == string::npos) {
						i = str.length();
					} else if(po.parsing_mode != PARSING_MODE_RPN && is_not_in(ILLEGAL_IN_UNITNAMES, str[i]) && is_in("abcdefABCDEF", str[i - 1])) {
						size_t i2 = str.find_last_not_of("abcdefABCDEF", i - 1);
						if(i2 != string::npos && i2 > str_index + 2 && is_in(SPACE, str[i2])) {
							i = i2;
						}
					}
					while(str[i - 1] == SPACE_CH) i--;
					name_length = i - str_index;
					ParseOptions po_hex = po;
					po_hex.base = BASE_HEXADECIMAL;
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					MathStructure *mstruct = new MathStructure(Number(str.substr(str_index, i - str_index), po_hex));
					stmp += i2s(addId(mstruct));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(str_index, name_length, stmp);
					str_index += stmp.length() - 1;
				}

			} else if(base <= 12 && str_index + 2 < str.length() && (str[str_index + 1] == 'b' || str[str_index + 1] == 'B') && is_in("01", str[str_index + 2])) {
				//binary number 0b...
				if(po.base == BASE_BINARY) {
					str.erase(str_index, 2);
				} else {
					size_t i;
					if(po.parsing_mode == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS, str_index + 2);
					else i = str.find_first_not_of(SPACE NUMBER_ELEMENTS, str_index + 2);
					size_t name_length;
					if(i == string::npos) i = str.length();
					while(str[i - 1] == SPACE_CH) i--;
					name_length = i - str_index;
					ParseOptions po_bin = po;
					po_bin.base = BASE_BINARY;
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					MathStructure *mstruct = new MathStructure(Number(str.substr(str_index, i - str_index), po_bin));
					stmp += i2s(addId(mstruct));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(str_index, name_length, stmp);
					str_index += stmp.length() - 1;
				}
			} else if(base <= 14 && str_index + 2 < str.length() && (str[str_index + 1] == 'd' || str[str_index + 1] == 'D') && is_in(NUMBERS DUODECIMAL_CHARS, str[str_index + 2])) {
				//duodecimal number 0d...
				if(po.base == BASE_DUODECIMAL) {
					str.erase(str_index, 2);
				} else {
					size_t i;
					if(po.parsing_mode == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS DUODECIMAL_CHARS, str_index + 2);
					else i = str.find_first_not_of(SPACE NUMBER_ELEMENTS DUODECIMAL_CHARS, str_index + 2);
					size_t name_length;
					if(i == string::npos) i = str.length();
					while(str[i - 1] == SPACE_CH) i--;
					name_length = i - str_index;
					ParseOptions po_duo = po;
					po_duo.base = BASE_DUODECIMAL;
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					MathStructure *mstruct = new MathStructure(Number(str.substr(str_index, i - str_index), po_duo));
					stmp += i2s(addId(mstruct));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(str_index, name_length, stmp);
					str_index += stmp.length() - 1;
				}
			} else if(base <= 24 && str_index + 2 < str.length() && (str[str_index + 1] == 'o' || str[str_index + 1] == 'O') && is_in(NUMBERS, str[str_index + 2])) {
				//octal number 0o...
				if(po.base == BASE_OCTAL) {
					str.erase(str_index, 2);
				} else {
					size_t i;
					if(po.parsing_mode == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS, str_index + 2);
					else i = str.find_first_not_of(SPACE NUMBER_ELEMENTS, str_index + 2);
					size_t name_length;
					if(i == string::npos) i = str.length();
					while(str[i - 1] == SPACE_CH) i--;
					name_length = i - str_index;
					ParseOptions po_oct = po;
					po_oct.base = BASE_OCTAL;
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					MathStructure *mstruct = new MathStructure(Number(str.substr(str_index, i - str_index), po_oct));
					stmp += i2s(addId(mstruct));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(str_index, name_length, stmp);
					str_index += stmp.length() - 1;
				}
			}
		} else if(is_not_in(NUMBERS INTERNAL_OPERATORS NOT_IN_NAMES, str[str_index])) {
			// dx/dy derivative notation
			if((str[str_index] == 'd' && is_not_number('d', base)) || (str[str_index] == -50 && str_index + 1 < str.length() && str[str_index + 1] == -108) || (str[str_index] == -16 && str_index + 3 < str.length() && str[str_index + 1] == -99 && str[str_index + 2] == -102 && str[str_index + 3] == -85)) {
				size_t d_len = 1;
				if(str[str_index] == -50) d_len = 2;
				else if(str[str_index] == -16) d_len = 4;
				if(str_index + (d_len * 2) + 1 < str.length() && (str[str_index + d_len] == '/' || (str[str_index + d_len + 1] == '/' && is_in("xyz", str[str_index + d_len]) && str_index + (d_len * 2) + 2 < str.length()))) {
					size_t i_div = str_index + d_len;
					if(str[str_index + d_len] != '/') i_div++;
					bool b = true;
					for(size_t i_d = 0; i_d < d_len; i_d++) {
						if(str[i_div + 1 + i_d] != str[str_index + i_d]) {
							b = false;
							break;
						}
					}
					if(b && is_in("xyz", str[i_div + d_len + 1]) && (str.length() == i_div + d_len + 2 || str[i_div + d_len + 2] == SPACE_CH || str[i_div + d_len + 2] == LEFT_PARENTHESIS_CH)) {
						size_t i3 = i_div + d_len + 2;
						if(i3 < str.length()) i3++;
						int nr_of_p = 1;
						size_t i7 = i3;
						for(; i7 < str.length(); i7++) {
							if(str[i7] == LEFT_PARENTHESIS_CH) {
								nr_of_p++;
							} else if(str[i7] == RIGHT_PARENTHESIS_CH) {
								nr_of_p--;
								if(nr_of_p == 0) break;
							}
						}
						if(nr_of_p > 0) nr_of_p--;
						if(i3 == i7) stmp2 = "";
						else stmp2 = str.substr(i3, i7 - i3);
						while(nr_of_p > 0) {stmp2 += ')'; nr_of_p--;}
						stmp2 += COMMA_CH;
						stmp2 += str[i_div + d_len + 1];
						stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
						stmp += i2s(parseAddId(f_diff, stmp2, po));
						stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
						str.replace(str_index, i7 - str_index, stmp);
						str_index += stmp.length() - 1;
						continue;
					}
				}
			}
			// search for variable, function, unit, prefix names
			bool p_mode = false;
			void *best_p_object = NULL;
			const string *best_p_name = NULL;
			Prefix *best_p = NULL;
			size_t best_pl = 0;
			size_t best_pnl = 0;
			bool moved_forward = false;
			const string *found_function_name = NULL;
			bool case_sensitive = false;
			size_t found_function_name_length = 0;
			void *found_function = NULL, *object = NULL;
			int vt2 = -1;
			size_t ufv_index;
			size_t name_length;
			size_t vt3 = 0;
			char ufvt = 0;
			size_t last_name_char = str.find_first_of(NOT_IN_NAMES INTERNAL_OPERATORS, str_index + 1);
			if(last_name_char == string::npos) {
				last_name_char = str.length() - 1;
			} else {
				last_name_char--;
			}
			size_t last_unit_char = str.find_last_not_of(NUMBERS, last_name_char);
			size_t name_chars_left = last_name_char - str_index + 1;
			size_t unit_chars_left = last_unit_char - str_index + 1;
			if(name_chars_left <= UFV_LENGTHS) {
				ufv_index = name_chars_left - 1;
				vt2 = 0;
			} else {
				ufv_index = 0;
			}
			Prefix *p = NULL;
			while(vt2 < 4) {
				name = NULL;
				p = NULL;
				switch(vt2) {
					case -1: {
						if(ufv_index < ufvl.size()) {
							switch(ufvl_t[ufv_index]) {
								case 'v': {
									if(po.variables_enabled && !p_mode) {
										name = &((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).name;
										case_sensitive = ((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).case_sensitive;
										name_length = name->length();
										if(name_length < found_function_name_length) {
											name = NULL;
										} else if(po.limit_implicit_multiplication) {
											if(name_length != name_chars_left && name_length != unit_chars_left) name = NULL;
										} else if(name_length > name_chars_left) {
											name = NULL;
										}
									}
									break;
								}
								case 'f': {
									if(po.functions_enabled && !found_function_name && !p_mode) {
										name = &((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).name;
										case_sensitive = ((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).case_sensitive;
										name_length = name->length();
										if(po.limit_implicit_multiplication) {
											if(name_length != name_chars_left && name_length != unit_chars_left) name = NULL;
										} else if(name_length > name_chars_left || name_length < found_function_name_length) {
											name = NULL;
										}
									}
									break;
								}
								case 'u': {
									if(po.units_enabled && !p_mode) {
										name = &((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).name;
										case_sensitive = ((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).case_sensitive;
										name_length = name->length();
										if(name_length < found_function_name_length) {
											name = NULL;
										} else if(po.limit_implicit_multiplication || ((ExpressionItem*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).plural) {
											if(name_length != unit_chars_left) name = NULL;
										} else if(name_length > unit_chars_left) {
											name = NULL;
										}
									}
									break;
								}
								case 'p': {
									if(!p && po.units_enabled) {
										name = &((Prefix*) ufvl[ufv_index])->shortName();
										name_length = name->length();
										if(name_length >= unit_chars_left || name_length < found_function_name_length) {
											name = NULL;
										}
									}
									case_sensitive = true;
									break;
								}
								case 'P': {
									if(!p && po.units_enabled) {
										name = &((Prefix*) ufvl[ufv_index])->longName();
										name_length = name->length();
										if(name_length > unit_chars_left || name_length < found_function_name_length) {
											name = NULL;
										}
									}
									case_sensitive = false;
									break;
								}
								case 'q': {
									if(!p && po.units_enabled) {
										name = &((Prefix*) ufvl[ufv_index])->unicodeName();
										name_length = name->length();
										if(name_length >= unit_chars_left || name_length < found_function_name_length) {
											name = NULL;
										}
									}
									case_sensitive = true;
									break;
								}
							}
							ufvt = ufvl_t[ufv_index];
							object = ufvl[ufv_index];
							ufv_index++;
							break;
						} else {
							if(found_function_name) {
								vt2 = 4;
								break;
							}
							vt2 = 0;
							vt3 = 0;
							if(po.limit_implicit_multiplication && unit_chars_left <= UFV_LENGTHS) {
								ufv_index = unit_chars_left - 1;
							} else {
								ufv_index = UFV_LENGTHS - 1;
							}
						}
					}
					case 0: {
						if(po.units_enabled && vt3 < ufv[vt2][ufv_index].size()) {
							object = ufv[vt2][ufv_index][vt3];
							switch(ufv_i[vt2][ufv_index][vt3]) {
								case 1: {
									ufvt = 'P';
									name = &((Prefix*) object)->longName();
									name_length = name->length();
									case_sensitive = false;
									break;
								}
								case 2: {
									if(ufv_index >= unit_chars_left - 1) break;
									ufvt = 'p';
									name = &((Prefix*) object)->shortName();
									name_length = name->length();
									case_sensitive = true;
									break;
								}
								case 3: {
									if(ufv_index >= unit_chars_left - 1) break;
									ufvt = 'q';
									name = &((Prefix*) object)->unicodeName();
									name_length = name->length();
									case_sensitive = true;
									break;
								}
							}
							vt3++;
							break;
						}
						vt2 = 1;
						vt3 = 0;
					}
					case 1: {
						if(!found_function_name && po.functions_enabled && !p_mode && (!po.limit_implicit_multiplication || ufv_index + 1 == unit_chars_left || ufv_index + 1 == name_chars_left) && vt3 < ufv[vt2][ufv_index].size()) {
							object = ufv[vt2][ufv_index][vt3];
							ufvt = 'f';
							name = &((MathFunction*) object)->getName(ufv_i[vt2][ufv_index][vt3]).name;
							name_length = name->length();
							case_sensitive = ((MathFunction*) object)->getName(ufv_i[vt2][ufv_index][vt3]).case_sensitive;
							vt3++;
							break;
						}
						vt2 = 2;
						vt3 = 0;
					}
					case 2: {
						if(po.units_enabled && !p_mode && (!po.limit_implicit_multiplication || ufv_index + 1 == unit_chars_left) && ufv_index < unit_chars_left && vt3 < ufv[vt2][ufv_index].size()) {
							object = ufv[vt2][ufv_index][vt3];
							if(ufv_index + 1 == unit_chars_left || !((Unit*) object)->getName(ufv_i[vt2][ufv_index][vt3]).plural) {
								ufvt = 'u';
								name = &((Unit*) object)->getName(ufv_i[vt2][ufv_index][vt3]).name;
								name_length = name->length();
								case_sensitive = ((Unit*) object)->getName(ufv_i[vt2][ufv_index][vt3]).case_sensitive;
							}
							vt3++;
							break;
						}
						vt2 = 3;
						vt3 = 0;
					}
					case 3: {
						if(po.variables_enabled && !p_mode && (!po.limit_implicit_multiplication || ufv_index + 1 == unit_chars_left || ufv_index + 1 == name_chars_left) && vt3 < ufv[vt2][ufv_index].size()) {
							object = ufv[vt2][ufv_index][vt3];
							ufvt = 'v';
							name = &((Variable*) object)->getName(ufv_i[vt2][ufv_index][vt3]).name;
							name_length = name->length();
							case_sensitive = ((Variable*) object)->getName(ufv_i[vt2][ufv_index][vt3]).case_sensitive;
							vt3++;
							break;
						}
						if(ufv_index == 0 || found_function_name) {
							vt2 = 4;
						} else {
							ufv_index--;
							vt3 = 0;
							vt2 = 0;
						}
					}
				}
				if(name && name_length >= found_function_name_length && ((case_sensitive && compare_name(*name, str, name_length, str_index, base)) || (!case_sensitive && (name_length = compare_name_no_case(*name, str, name_length, str_index, base))))) {
					moved_forward = false;
					switch(ufvt) {
						case 'v': {
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure((Variable*) object)));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
							str.replace(str_index, name_length, stmp);
							str_index += stmp.length();
							moved_forward = true;
							break;
						}
						case 'f': {
							if(((ExpressionItem*) object)->subtype() == SUBTYPE_DATA_SET && str[str_index + name_length] == DOT_CH) {
								str[str_index + name_length] = LEFT_PARENTHESIS_CH;
								size_t dot2_index = str.find(DOT_CH, str_index + name_length + 1);
								str[dot2_index] = COMMA_CH;
								size_t end_index = str.find_first_of(NOT_IN_NAMES INTERNAL_OPERATORS, dot2_index + 1);
								if(end_index == string::npos) str += RIGHT_PARENTHESIS_CH;
								else str.insert(end_index, 1, RIGHT_PARENTHESIS_CH);
							}
							size_t not_space_index;
							if((not_space_index = str.find_first_not_of(SPACES, str_index + name_length)) == string::npos || str[not_space_index] != LEFT_PARENTHESIS_CH) {
								found_function = object;
								found_function_name = name;
								found_function_name_length = name_length;
								break;
							}
							set_function:
							MathFunction *f = (MathFunction*) object;
							if(str_index + name_length + 2 < str.length() && str[str_index + name_length] == POWER_CH && (f->id() == FUNCTION_ID_SIN || f->id() == FUNCTION_ID_COS || f->id() == FUNCTION_ID_TAN || f->id() == FUNCTION_ID_SINH || f->id() == FUNCTION_ID_COSH || f->id() == FUNCTION_ID_TANH) && str[str_index + name_length + 1] == MINUS_CH && str[str_index + name_length + 2] == '1' && (str_index + name_length + 3 == str.length() || is_not_in(NUMBER_ELEMENTS, str[str_index + name_length + 3]))) {
								name_length += 3;
								if(f->id() == FUNCTION_ID_SIN) f = f_asin;
								else if(f->id() == FUNCTION_ID_COS) f = f_acos;
								else if(f->id() == FUNCTION_ID_TAN) f = f_atan;
								else if(f->id() == FUNCTION_ID_SINH) f = f_asinh;
								else if(f->id() == FUNCTION_ID_COSH) f = f_acosh;
								else if(f->id() == FUNCTION_ID_TANH) f = f_atanh;
							}
							int i4 = -1;
							size_t i6;
							if(f->args() == 0) {
								size_t i7 = str.find_first_not_of(SPACES, str_index + name_length);
								if(i7 != string::npos && str[i7] == LEFT_PARENTHESIS_CH) {
									i7 = str.find_first_not_of(SPACES, i7 + 1);
									if(i7 != string::npos && str[i7] == RIGHT_PARENTHESIS_CH) {
										i4 = i7 - str_index + 1;
									}
								}
								stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
								stmp += i2s(parseAddId(f, empty_string, po));
								stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
								if(i4 < 0) i4 = name_length;
							} else if(po.parsing_mode == PARSING_MODE_CHAIN && f->minargs() == 1 && str_index > 0 && (i6 = str.find_last_not_of(SPACE, str_index - 1)) != string::npos && str[i6] != LEFT_PARENTHESIS_CH && is_not_in(OPERATORS INTERNAL_OPERATORS, str[i6]) && (str_index + name_length >= str.length() || (str.find_first_not_of(SPACE, str_index + name_length) == string::npos || is_in(OPERATORS INTERNAL_OPERATORS, str[str.find_first_not_of(SPACE, str_index + name_length)])))) {
								size_t i7 = i6;
								int nr_of_p = 0;
								while(true) {
									if(str[i7] == LEFT_PARENTHESIS_CH) {
										if(nr_of_p == 0) {i7++; break;}
										nr_of_p--;
										if(nr_of_p == 0) {break;}
									} else if(str[i7] == RIGHT_PARENTHESIS_CH) {
										if(nr_of_p == 0 && i7 != i6) {i7++; break;}
										nr_of_p++;
									} else if(nr_of_p == 0 && is_in(PLUS MINUS, str[i7])) {
										if(i7 != 0) i6 = str.find_last_not_of(SPACE, i7 - 1);
										if(i7 == 0 || is_not_in(OPERATORS INTERNAL_OPERATORS, str[i6])) {
											i7++;
											break;
										}
									} else if(nr_of_p == 0 && is_in("*/&|=><^%\x1c", str[i7])) {
										i7++;
										break;
									}
									if(i7 == 0) break;
									i7--;
								}
								stmp2 = str.substr(i7, str_index - i7);
								stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
								if(f->id() == FUNCTION_ID_VECTOR) stmp += i2s(parseAddVectorId(stmp2, po));
								else stmp += i2s(parseAddId(f, stmp2, po));
								stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
								str.replace(i7, str_index + name_length - i7, stmp);
								str_index += name_length;
								moved_forward = true;
							} else if(po.parsing_mode == PARSING_MODE_RPN && f->args() == 1 && str_index > 0 && str[str_index - 1] != LEFT_PARENTHESIS_CH && (str_index + name_length >= str.length() || str[str_index + name_length] != LEFT_PARENTHESIS_CH) && (i6 = str.find_last_not_of(SPACE, str_index - 1)) != string::npos) {
								size_t i7 = i6;
								int nr_of_p = 0, nr_of_op = 0;
								bool b_started = false;
								while(i7 != 0) {
									if(nr_of_p > 0) {
										if(str[i7] == LEFT_PARENTHESIS_CH) {
											nr_of_p--;
											if(nr_of_p == 0 && nr_of_op == 0) break;
										} else if(str[i7] == RIGHT_PARENTHESIS_CH) {
											nr_of_p++;
										}
									} else if(nr_of_p == 0 && is_in(OPERATORS INTERNAL_OPERATORS SPACE RIGHT_PARENTHESIS, str[i7])) {
										if(nr_of_op == 0 && b_started) {
											i7++;
											break;
										} else {
											if(is_in(OPERATORS INTERNAL_OPERATORS, str[i7])) {
												nr_of_op++;
												b_started = false;
											} else if(str[i7] == RIGHT_PARENTHESIS_CH) {
												nr_of_p++;
												b_started = true;
											} else if(b_started) {
												nr_of_op--;
												b_started = false;
											}
										}
									} else {
										b_started = true;
									}
									i7--;
								}
								stmp2 = str.substr(i7, i6 - i7 + 1);
								stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
								if(f->id() == FUNCTION_ID_VECTOR) stmp += i2s(parseAddVectorId(stmp2, po));
								else stmp += i2s(parseAddId(f, stmp2, po));
								stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
								str.replace(i7, str_index + name_length - i7, stmp);
								str_index += name_length;
								moved_forward = true;
							} else {
								bool b = false, b_unended_function = false, b_comma_before = false, b_power_before = false;
								//bool b_space_first = false;
								size_t i5 = 1;
								int arg_i = f->args();
								i6 = 0;
								while(!b) {
									if(i6 + str_index + name_length >= str.length()) {
										b = true;
										i5 = 2;
										i6++;
										b_unended_function = true;
										break;
									} else {
										char c = str[str_index + name_length + i6];
										if(c == LEFT_PARENTHESIS_CH) {
											if(i5 < 2) b = true;
											else if(i5 == 2 && po.parsing_mode >= PARSING_MODE_CONVENTIONAL && !b_power_before) b = true;
											else i5++;
										} else if(c == RIGHT_PARENTHESIS_CH) {
											if(i5 <= 2) b = true;
											else i5--;
										} else if(c == POWER_CH) {
											if(i5 < 2) i5 = 2;
											b_power_before = true;
										} else if(!b_comma_before && !b_power_before && c == ' ' && arg_i <= 1) {
											//if(i5 < 2) b_space_first = true;
											if(i5 == 2) b = true;
										} else if(!b_comma_before && i5 == 2 && arg_i <= 1 && is_in(OPERATORS INTERNAL_OPERATORS, c) && c != POWER_CH && (!b_power_before || (c != MINUS_CH && c != PLUS_CH))) {
											b = true;
										} else if(c == COMMA_CH) {
											if(i5 == 2) arg_i--;
											b_comma_before = true;
											if(i5 < 2) i5 = 2;
										} else if(i5 < 2) {
											i5 = 2;
										}
										if(c != COMMA_CH && c != ' ') b_comma_before = false;
										if(c != POWER_CH && c != ' ') b_power_before = false;
									}
									i6++;
								}
								if(b && i5 >= 2) {
									stmp2 = str.substr(str_index + name_length, i6 - 1);
									stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
									if(b_unended_function && unended_function) {
										po.unended_function = unended_function;
									}
									if(f->id() == FUNCTION_ID_VECTOR) {
										stmp += i2s(parseAddVectorId(stmp2, po));
									} else if((f->id() == FUNCTION_ID_INTERVAL || f->id() == FUNCTION_ID_UNCERTAINTY) && po.read_precision != DONT_READ_PRECISION) {
										ParseOptions po2 = po;
										po2.read_precision = DONT_READ_PRECISION;
										stmp += i2s(parseAddId(f, stmp2, po2));
									} else {
										stmp += i2s(parseAddId(f, stmp2, po));
									}
									po.unended_function = NULL;
									stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
									i4 = i6 + 1 + name_length - 2;
									b = false;
								}
								size_t i9 = i6;
								if(b) {
									b = false;
									i6 = i6 + 1 + str_index + name_length;
									size_t i7 = i6 - 1;
									size_t i8 = i7;
									while(true) {
										i5 = str.find(RIGHT_PARENTHESIS_CH, i7);
										if(i5 == string::npos) {
											b_unended_function = true;
											//str.append(1, RIGHT_PARENTHESIS_CH);
											//i5 = str.length() - 1;
											i5 = str.length();
										}
										if(i5 < (i6 = str.find(LEFT_PARENTHESIS_CH, i8)) || i6 == string::npos) {
											i6 = i5;
											b = true;
											break;
										}
										i7 = i5 + 1;
										i8 = i6 + 1;
									}
									if(!b) {
										b_unended_function = false;
									}
								}
								if(b) {
									stmp2 = str.substr(str_index + name_length + i9, i6 - (str_index + name_length + i9));
									stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
									if(b_unended_function && unended_function) {
										po.unended_function = unended_function;
									}
									if(f->id() == FUNCTION_ID_VECTOR) {
										stmp += i2s(parseAddVectorId(stmp2, po));
									} else if((f->id() == FUNCTION_ID_INTERVAL || f->id() == FUNCTION_ID_UNCERTAINTY) && po.read_precision != DONT_READ_PRECISION) {
										ParseOptions po2 = po;
										po2.read_precision = DONT_READ_PRECISION;
										stmp += i2s(parseAddId(f, stmp2, po2));
									} else {
										stmp += i2s(parseAddId(f, stmp2, po));
									}
									po.unended_function = NULL;
									stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
									i4 = i6 + 1 - str_index;
								}
							}
							if(i4 > 0) {
								str.replace(str_index, i4, stmp);
								str_index += stmp.length();
								moved_forward = true;
							}
							break;
						}
						case 'u': {
							replace_text_by_unit_place:
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure((Unit*) object, p)));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
							str.replace(str_index, name_length, stmp);
							if(str.length() > str_index + stmp.length() && is_in("23", str[str_index + stmp.length()]) && (str.length() == str_index + stmp.length() + 1 || is_not_in(NUMBER_ELEMENTS, str[str_index + stmp.length() + 1])) && (!name || *name != SIGN_DEGREE) && !((Unit*) object)->isCurrency()) {
								str.insert(str_index + stmp.length(), 1, POWER_CH);
								if(po.parsing_mode == PARSING_MODE_CHAIN) {
									str.insert(str_index + stmp.length() + 2, 1, RIGHT_PARENTHESIS_CH);
									str.insert(str_index, 1, LEFT_PARENTHESIS_CH);
									str_index++;
								}
							}
							str_index += stmp.length();
							moved_forward = true;
							p = NULL;
							break;
						}
						case 'p': {}
						case 'q': {}
						case 'P': {
							p = (Prefix*) object;
							if(str_index + name_length == str.length() || is_in(NOT_IN_NAMES INTERNAL_OPERATORS, str[str_index + name_length])) {
								if(ufvt == 'P') {
									stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
									switch(p->type()) {
										case PREFIX_DECIMAL: {
											MathStructure *m_prefix = new MathStructure(10, 1, 0);
											m_prefix->raise(Number(((DecimalPrefix*) p)->exponent(), 1));
											stmp += i2s(addId(m_prefix));
											break;
										}
										case PREFIX_BINARY: {
											MathStructure *m_prefix = new MathStructure(2, 1, 0);
											m_prefix->raise(Number(((BinaryPrefix*) p)->exponent(), 1));
											stmp += i2s(addId(m_prefix));
											break;
										}
										default: {
											stmp += i2s(addId(new MathStructure(p->value())));
										}
									}
									stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
									str.replace(str_index, name_length, stmp);
									str_index += stmp.length();
									moved_forward = true;
								}
								break;
							}
							str_index += name_length;
							unit_chars_left = last_unit_char - str_index + 1;
							size_t name_length_old = name_length;
							int index = 0;
							if(unit_chars_left > UFV_LENGTHS) {
								for(size_t ufv_index2 = 0; ufv_index2 < ufvl.size(); ufv_index2++) {
									name = NULL;
									switch(ufvl_t[ufv_index2]) {
										case 'u': {
											name = &((Unit*) ufvl[ufv_index2])->getName(ufvl_i[ufv_index2]).name;
											case_sensitive = ((Unit*) ufvl[ufv_index2])->getName(ufvl_i[ufv_index2]).case_sensitive;
											name_length = name->length();
											if(po.limit_implicit_multiplication || ((Unit*) ufvl[ufv_index2])->getName(ufvl_i[ufv_index2]).plural) {
												if(name_length != unit_chars_left) name = NULL;
											} else if(name_length > unit_chars_left) {
												name = NULL;
											}
											break;
										}
									}
									if(name && ((case_sensitive && compare_name(*name, str, name_length, str_index, base)) || (!case_sensitive && (name_length = compare_name_no_case(*name, str, name_length, str_index, base))))) {
										if((!p_mode && name_length_old > 1) || (p_mode && (name_length + name_length_old > best_pl || ((ufvt != 'P' || !((Unit*) ufvl[ufv_index2])->getName(ufvl_i[ufv_index2]).abbreviation) && name_length + name_length_old == best_pl)))) {
											p_mode = true;
											best_p = p;
											best_p_object = ufvl[ufv_index2];
											best_p_name = name;
											best_pl = name_length + name_length_old;
											best_pnl = name_length_old;
											index = -1;
											break;
										}
										if(!p_mode) {
											str.erase(str_index - name_length_old, name_length_old);
											str_index -= name_length_old;
											object = ufvl[ufv_index2];
											goto replace_text_by_unit_place;
										}
									}
								}
							}
							if(index < 0) {
							} else if(UFV_LENGTHS >= unit_chars_left) {
								index = unit_chars_left - 1;
							} else if(po.limit_implicit_multiplication) {
								index = -1;
							} else {
								index = UFV_LENGTHS - 1;
							}
							for(; index >= 0; index--) {
								for(size_t ufv_index2 = 0; ufv_index2 < ufv[2][index].size(); ufv_index2++) {
									name = &((Unit*) ufv[2][index][ufv_index2])->getName(ufv_i[2][index][ufv_index2]).name;
									case_sensitive = ((Unit*) ufv[2][index][ufv_index2])->getName(ufv_i[2][index][ufv_index2]).case_sensitive;
									name_length = name->length();
									if(index + 1 == (int) unit_chars_left || !((Unit*) ufv[2][index][ufv_index2])->getName(ufv_i[2][index][ufv_index2]).plural) {
										if(name_length <= unit_chars_left && ((case_sensitive && compare_name(*name, str, name_length, str_index, base)) || (!case_sensitive && (name_length = compare_name_no_case(*name, str, name_length, str_index, base))))) {
											if((!p_mode && name_length_old > 1) || (p_mode && (name_length + name_length_old > best_pl || ((ufvt != 'P' || !((Unit*) ufv[2][index][ufv_index2])->getName(ufv_i[2][index][ufv_index2]).abbreviation) && name_length + name_length_old == best_pl)))) {
												p_mode = true;
												best_p = p;
												best_p_object = ufv[2][index][ufv_index2];
												best_p_name = name;
												best_pl = name_length + name_length_old;
												best_pnl = name_length_old;
												index = -1;
											}
											if(!p_mode) {
												str.erase(str_index - name_length_old, name_length_old);
												str_index -= name_length_old;
												object = ufv[2][index][ufv_index2];
												goto replace_text_by_unit_place;
											}
										}
									}
								}
								if(po.limit_implicit_multiplication || (p_mode && index + 1 + name_length_old < best_pl)) {
									break;
								}
							}
							str_index -= name_length_old;
							unit_chars_left = last_unit_char - str_index + 1;
							break;
						}
					}
					if(moved_forward) {
						str_index--;
						break;
					}
				}
			}
			if(!moved_forward && p_mode) {
				object = best_p_object;
				name = best_p_name;
				p = best_p;
				str.erase(str_index, best_pnl);
				name_length = best_pl - best_pnl;
				goto replace_text_by_unit_place;
			} else if(!moved_forward && found_function) {
				object = found_function;
				name = found_function_name;
				name_length = found_function_name_length;
				goto set_function;
			}
			if(!moved_forward) {
				bool b = po.unknowns_enabled && is_not_number(str[str_index], base) && !(str_index > 0 && is_in(EXPS, str[str_index]) && str_index + 1 < str.length() && (is_in(NUMBER_ELEMENTS, str[str_index + 1]) || (is_in(PLUS MINUS, str[str_index + 1]) && str_index + 2 < str.length() && is_in(NUMBER_ELEMENTS, str[str_index + 2]))) && is_in(NUMBER_ELEMENTS, str[str_index - 1]));
				if(po.limit_implicit_multiplication) {
					if(b) {
						stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
						stmp += i2s(addId(new MathStructure(str.substr(str_index, unit_chars_left))));
						stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
						str.replace(str_index, unit_chars_left, stmp);
						str_index += stmp.length() - 1;
					} else {
						str_index += unit_chars_left - 1;
					}
				} else if(b) {
					size_t i = 1;
					if(str[str_index + 1] < 0) {
						i++;
						while(i <= unit_chars_left && (unsigned char) str[str_index + i] >= 0x80 && (unsigned char) str[str_index + i] <= 0xBF) {
							i++;
						}
					}
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					stmp += i2s(addId(new MathStructure(str.substr(str_index, i))));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(str_index, i, stmp);
					str_index += stmp.length() - 1;
				}
			}
		}
	}

	size_t comma_i = str.find(COMMA, 0);
	while(comma_i != string::npos) {
		int i3 = 1;
		size_t left_par_i = comma_i;
		while(left_par_i > 0) {
			left_par_i = str.find_last_of(LEFT_PARENTHESIS RIGHT_PARENTHESIS, left_par_i - 1);
			if(left_par_i == string::npos) break;
			if(str[left_par_i] == LEFT_PARENTHESIS_CH) {
				i3--;
				if(i3 == 0) break;
			} else if(str[left_par_i] == RIGHT_PARENTHESIS_CH) {
				i3++;
			}
		}
		if(i3 > 0) {
			str.insert(0, i3, LEFT_PARENTHESIS_CH);
			comma_i += i3;
			i3 = 0;
			left_par_i = 0;
		}
		if(i3 == 0) {
			i3 = 1;
			size_t right_par_i = comma_i;
			while(true) {
				right_par_i = str.find_first_of(LEFT_PARENTHESIS RIGHT_PARENTHESIS, right_par_i + 1);
				if(right_par_i == string::npos) {
					for(; i3 > 0; i3--) {
						str += RIGHT_PARENTHESIS;
					}
					right_par_i = str.length() - 1;
				} else if(str[right_par_i] == LEFT_PARENTHESIS_CH) {
					i3++;
				} else if(str[right_par_i] == RIGHT_PARENTHESIS_CH) {
					i3--;
				}
				if(i3 == 0) {
					stmp2 = str.substr(left_par_i + 1, right_par_i - left_par_i - 1);
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					stmp += i2s(parseAddVectorId(stmp2, po));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(left_par_i, right_par_i + 1 - left_par_i, stmp);
					comma_i = left_par_i + stmp.length() - 1;
					break;
				}
			}
		}
		comma_i = str.find(COMMA, comma_i + 1);
	}

	if(po.parsing_mode == PARSING_MODE_RPN) {
		size_t rpn_i = str.find(SPACE, 0);
		while(rpn_i != string::npos) {
			if(rpn_i == 0 || rpn_i + 1 == str.length() || is_in("~+-*/^\a\b\\\x1c", str[rpn_i - 1]) || (is_in("%&|", str[rpn_i - 1]) && str[rpn_i + 1] != str[rpn_i - 1]) || (is_in("!><=", str[rpn_i - 1]) && is_not_in("=<>", str[rpn_i + 1])) || (is_in(SPACE OPERATORS INTERNAL_OPERATORS, str[rpn_i + 1]) && (str[rpn_i - 1] == SPACE_CH || (str[rpn_i - 1] != str[rpn_i + 1] && is_not_in("!><=", str[rpn_i - 1]))))) {
				str.erase(rpn_i, 1);
			} else {
				rpn_i++;
			}
			rpn_i = str.find(SPACE, rpn_i);
		}
	} else if(po.parsing_mode != PARSING_MODE_ADAPTIVE) {
		remove_blanks(str);
	} else {
		//remove spaces between next to operators (except '/') and before/after parentheses
		space_i = str.find(SPACE_CH, 0);
		while(space_i != string::npos) {
			if((str[space_i + 1] != DIVISION_CH && is_in(OPERATORS INTERNAL_OPERATORS RIGHT_PARENTHESIS, str[space_i + 1])) || (str[space_i - 1] != DIVISION_CH && is_in(OPERATORS INTERNAL_OPERATORS LEFT_PARENTHESIS, str[space_i - 1]))) {
				str.erase(space_i, 1);
				space_i--;
			}
			space_i = str.find(SPACE_CH, space_i + 1);
		}
	}

	parseOperators(mstruct, str, po);

}

#define BASE_2_10 ((po.base >= 2 && po.base <= 10) || (po.base < BASE_CUSTOM && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26) || (po.base == BASE_CUSTOM && priv->custom_input_base_i <= 10))

bool Calculator::parseNumber(MathStructure *mstruct, string str, const ParseOptions &po) {

	mstruct->clear();
	if(str.empty()) return false;

	// check that string contains characters other than operators and whitespace
	if(str.find_first_not_of(OPERATORS "\a%\x1c" SPACE) == string::npos && (po.base != BASE_ROMAN_NUMERALS || str.find("|") == string::npos)) {
		replace_internal_operators(str);
		error(false, _("Misplaced operator(s) \"%s\" ignored"), str.c_str(), NULL);
		return false;
	}

	int minus_count = 0;
	bool has_sign = false, had_non_sign = false, b_dot = false, b_exp = false, after_sign_e = false;
	int i_colon = 0;
	size_t i = 0;

	while(i < str.length()) {
		if(!had_non_sign && str[i] == MINUS_CH) {
			// count minuses in front of the number
			has_sign = true;
			minus_count++;
			str.erase(i, 1);
		} else if(!had_non_sign && str[i] == PLUS_CH) {
			// + in front of the number is ignored
			has_sign = true;
			str.erase(i, 1);
		} else if(str[i] == SPACE_CH) {
			// ignore whitespace
			str.erase(i, 1);
		} else if(had_non_sign && !b_exp && BASE_2_10 && (str[i] == EXP_CH || str[i] == EXP2_CH)) {
			// scientific e-notation
			b_exp = true;
			had_non_sign = true;
			after_sign_e = true;
			i++;
		} else if(after_sign_e && (str[i] == MINUS_CH || str[i] == PLUS_CH)) {
			after_sign_e = false;
			i++;
		} else if(po.preserve_format && str[i] == DOT_CH) {
			b_dot = true;
			had_non_sign = true;
			after_sign_e = false;
			i++;
		} else if(po.preserve_format && (!b_dot || i_colon > 0) && str[i] == ':') {
			// sexagesimal colon; dots are not allowed in first part of a sexagesimal number
			i_colon++;
			had_non_sign = true;
			after_sign_e = false;
			i++;
		} else if(str[i] == COMMA_CH && DOT_S == ".") {
			// comma is ignored of decimal separator is dot
			str.erase(i, 1);
			after_sign_e = false;
			had_non_sign = true;
		} else if(is_in(OPERATORS, str[i]) && (po.base != BASE_ROMAN_NUMERALS || (str[i] != '(' && str[i] != ')' && str[i] != '|'))) {
			// ignore operators
			error(false, _("Misplaced '%c' ignored"), str[i], NULL);
			str.erase(i, 1);
		} else if(str[i] == '\a') {
			// ignore operators
			error(false, _("Misplaced operator(s) \"%s\" ignored"), "xor", NULL);
			str.erase(i, 1);
		} else if(str[i] == '\x1c') {
			// ignore operators
			error(false, _("Misplaced operator(s) \"%s\" ignored"), "∠", NULL);
			str.erase(i, 1);
		} else if(str[i] == '\b') {
			// +/-
			b_exp = false;
			had_non_sign = false;
			after_sign_e = false;
			i++;
		} else {
			had_non_sign = true;
			after_sign_e = false;
			i++;
		}
	}
	if(str.empty()) {
		if(minus_count % 2 == 1 && !po.preserve_format) {
			mstruct->set(-1, 1, 0);
		} else if(has_sign) {
			mstruct->set(1, 1, 0);
			if(po.preserve_format) {
				while(minus_count > 0) {
					mstruct->transform(STRUCT_NEGATE);
					minus_count--;
				}
			}
		}
		return false;
	}
	// numbers in brackets is an internal reference to a stored MathStructure object
	if(str[0] == ID_WRAP_LEFT_CH && str.length() > 2 && str[str.length() - 1] == ID_WRAP_RIGHT_CH) {
		int id = s2i(str.substr(1, str.length() - 2));
		MathStructure *m_temp = getId((size_t) id);
		if(!m_temp) {
			mstruct->setUndefined();
			error(true, _("Internal id %s does not exist."), i2s(id).c_str(), NULL);
			return true;
		}
		mstruct->set_nocopy(*m_temp);
		m_temp->unref();
		if(po.preserve_format) {
			while(minus_count > 0) {
				mstruct->transform(STRUCT_NEGATE);
				minus_count--;
			}
		} else if(minus_count % 2 == 1) {
			mstruct->negate();
		}
		return true;
	}

	// handle non-digits if number base is 2-10 or duodecimal
	size_t itmp;
	long int mulexp = 0;
	if((BASE_2_10 || po.base == BASE_DUODECIMAL) && (itmp = str.find_first_not_of(po.base == BASE_DUODECIMAL ? NUMBER_ELEMENTS INTERNAL_NUMBER_CHARS MINUS DUODECIMAL_CHARS : NUMBER_ELEMENTS INTERNAL_NUMBER_CHARS EXPS MINUS, 0)) != string::npos) {
		if(itmp == 0) {
			error(true, _("\"%s\" is not a valid variable/function/unit."), str.c_str(), NULL);
			if(minus_count % 2 == 1 && !po.preserve_format) {
				mstruct->set(-1, 1, 0);
			} else if(has_sign) {
				mstruct->set(1, 1, 0);
				if(po.preserve_format) {
					while(minus_count > 0) {
						mstruct->transform(STRUCT_NEGATE);
						minus_count--;
					}
				}
			}
			return false;
		} else if(po.base == BASE_DECIMAL && itmp == str.length() - 1 && (str[itmp] == 'k' || str[itmp] == 'K')) {
			mulexp = 3;
			str.erase(itmp, str.length() - itmp);
		} else {
			string stmp = str.substr(itmp, str.length() - itmp);
			error(true, _("Trailing characters \"%s\" (not a valid variable/function/unit) in number \"%s\" was ignored."), stmp.c_str(), str.c_str(), NULL);
			str.erase(itmp, str.length() - itmp);
		}
	}

	// replace internal +/- operator
	gsub("\b", "±", str);

	// parse number
	Number nr(str, po);

	// handle - in front of the number (even number of minuses equals plus, odd number equals a single minus)
	if(!po.preserve_format && minus_count % 2 == 1) {
		nr.negate();
	}

	if(i_colon && nr.isRational() && !nr.isInteger()) {
		// if po.preserve_format is true, parse sexagesimal number as division
		Number nr_num(nr.numerator()), nr_den(1, 1, 0);
		while(i_colon) {
			nr_den *= 60;
			i_colon--;
		}
		nr_num *= nr_den;
		nr_num /= nr.denominator();
		mstruct->set(nr_num);
		mstruct->transform(STRUCT_DIVISION, nr_den);
	} else {
		mstruct->set(nr);
	}
	if(mulexp != 0) mstruct->multiply(Number(1, 1, mulexp));
	if(po.preserve_format) {
		// handle multiple - in front of the number (treated as a single sign if po.preserve_format is false)
		while(minus_count > 0) {
			mstruct->transform(STRUCT_NEGATE);
			minus_count--;
		}
	}
	return true;

}

bool Calculator::parseAdd(string &str, MathStructure *mstruct, const ParseOptions &po) {
	if(str.length() > 0) {
		size_t i;
		if(BASE_2_10) {
			i = str.find_first_of(SPACE MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS EXPS ID_WRAP_LEFT, 1);
		} else {
			i = str.find_first_of(SPACE MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS ID_WRAP_LEFT, 1);
		}
		if(i == string::npos && str[0] != LOGICAL_NOT_CH && str[0] != BITWISE_NOT_CH && !(str[0] == ID_WRAP_LEFT_CH && str.find(ID_WRAP_RIGHT) < str.length() - 1) && (!BASE_2_10 || (str[0] != EXP_CH && str[0] != EXP2_CH))) {
			return parseNumber(mstruct, str, po);
		} else {
			return parseOperators(mstruct, str, po);
		}
	}
	return false;
}
bool Calculator::parseAdd(string &str, MathStructure *mstruct, const ParseOptions &po, MathOperation s, bool append) {
	if(str.length() > 0) {
		size_t i;
		if(BASE_2_10) {
			i = str.find_first_of(SPACE MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS EXPS ID_WRAP_LEFT, 1);
		} else {
			i = str.find_first_of(SPACE MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS ID_WRAP_LEFT, 1);
		}
		if(i == string::npos && str[0] != LOGICAL_NOT_CH && str[0] != BITWISE_NOT_CH && !(str[0] == ID_WRAP_LEFT_CH && str.find(ID_WRAP_RIGHT) < str.length() - 1) && (!BASE_2_10 || (str[0] != EXP_CH && str[0] != EXP2_CH))) {
			if(s == OPERATION_EXP10 && po.read_precision == ALWAYS_READ_PRECISION) {
				ParseOptions po2 = po;
				po2.read_precision = READ_PRECISION_WHEN_DECIMALS;
				MathStructure *mstruct2 = new MathStructure();
				if(!parseNumber(mstruct2, str, po2)) {
					mstruct2->unref();
					return false;
				}
				mstruct->add_nocopy(mstruct2, s, append);
			} else {
				MathStructure *mstruct2 = new MathStructure();
				if(!parseNumber(mstruct2, str, po)) {
					mstruct2->unref();
					return false;
				}
				if(s == OPERATION_EXP10 && !po.preserve_format && mstruct->isNumber() && mstruct2->isNumber()) {
					mstruct->number().exp10(mstruct2->number());
					mstruct->numberUpdated();
					mstruct->mergePrecision(*mstruct2);
				} else if(s == OPERATION_DIVIDE && po.preserve_format) {
					mstruct->transform_nocopy(STRUCT_DIVISION, mstruct2);
				} else if(s == OPERATION_SUBTRACT && po.preserve_format) {
					mstruct2->transform(STRUCT_NEGATE);
					mstruct->add_nocopy(mstruct2, OPERATION_ADD, append);
				} else {
					mstruct->add_nocopy(mstruct2, s, append);
				}
			}
		} else {
			MathStructure *mstruct2 = new MathStructure();
			if(!parseOperators(mstruct2, str, po)) {
				mstruct2->unref();
				return false;
			}
			if(s == OPERATION_DIVIDE && po.preserve_format) {
				mstruct->transform_nocopy(STRUCT_DIVISION, mstruct2);
			} else if(s == OPERATION_SUBTRACT && po.preserve_format) {
				mstruct2->transform(STRUCT_NEGATE);
				mstruct->add_nocopy(mstruct2, OPERATION_ADD, append);
			} else {
				mstruct->add_nocopy(mstruct2, s, append);
			}
		}
	}
	return true;
}

MathStructure *get_out_of_negate(MathStructure &mstruct, int *i_neg) {
	if(mstruct.isNegate() || (mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isMinusOne())) {
		if(i_neg) (*i_neg)++;
		return get_out_of_negate(mstruct.last(), i_neg);
	}
	return &mstruct;
}

bool Calculator::parseOperators(MathStructure *mstruct, string str, const ParseOptions &po) {
	string save_str = str;
	mstruct->clear();
	size_t i = 0, i2 = 0, i3 = 0;
	string str2, str3;
	bool extended_roman = (po.base == BASE_ROMAN_NUMERALS && (i = str.find("|")) != string::npos && i + 1 < str.length() && str[i + 1] == RIGHT_PARENTHESIS_CH);
	while(!extended_roman) {
		//find first right parenthesis and then the last left parenthesis before
		i2 = str.find(RIGHT_PARENTHESIS_CH);
		if(i2 == string::npos) {
			i = str.rfind(LEFT_PARENTHESIS_CH);
			if(i == string::npos) {
				//if no parenthesis break
				break;
			} else {
				//right parenthesis missing -- append
				str += RIGHT_PARENTHESIS_CH;
				i2 = str.length() - 1;
			}
		} else {
			if(i2 > 0) {
				i = str.rfind(LEFT_PARENTHESIS_CH, i2 - 1);
			} else {
				i = string::npos;
			}
			if(i == string::npos) {
				//left parenthesis missing -- prepend
				str.insert(str.begin(), 1, LEFT_PARENTHESIS_CH);
				i = 0;
				i2++;
			}
		}
		while(true) {
			//remove unnecessary double parenthesis and the found parenthesis
			if(i > 0 && i2 + 1 < str.length() && str[i - 1] == LEFT_PARENTHESIS_CH && str[i2 + 1] == RIGHT_PARENTHESIS_CH) {
				str.erase(str.begin() + (i - 1));
				i--; i2--;
				str.erase(str.begin() + (i2 + 1));
			} else {
				break;
			}
		}
		if(i > 0 && is_not_in(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS SPACE, str[i - 1]) && (!BASE_2_10 || (str[i - 1] != EXP_CH && str[i - 1] != EXP2_CH))) {
			if(po.parsing_mode == PARSING_MODE_RPN) {
				str.insert(i2 + 1, MULTIPLICATION);
				str.insert(i, SPACE);
				i++;
				i2++;
			}
		}
		if(i2 + 1 < str.length() && is_not_in(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS SPACE, str[i2 + 1]) && (!BASE_2_10 || (str[i2 + 1] != EXP_CH && str[i2 + 1] != EXP2_CH))) {
			if(po.parsing_mode == PARSING_MODE_RPN) {
				i3 = str.find(SPACE, i2 + 1);
				if(i3 == string::npos) {
					str += MULTIPLICATION;
				} else {
					str.replace(i3, 1, MULTIPLICATION);
				}
				str.insert(i2 + 1, SPACE);
			}
		}
		if(po.parsing_mode == PARSING_MODE_RPN && i > 0 && i2 + 1 == str.length() && is_not_in(PARENTHESISS SPACE, str[i - 1])) {
			str += MULTIPLICATION_CH;
		}
		str2 = str.substr(i + 1, i2 - (i + 1));
		MathStructure *mstruct2 = new MathStructure();
		if(str2.empty()) {
			error(false, _("Empty expression in parentheses interpreted as zero."), NULL);
		} else {
			parseOperators(mstruct2, str2, po);
		}
		mstruct2->setInParentheses(true);
		str2 = ID_WRAP_LEFT;
		str2 += i2s(addId(mstruct2));
		str2 += ID_WRAP_RIGHT;
		str.replace(i, i2 - i + 1, str2);
		mstruct->clear();
	}
	bool b_abs_or = false, b_bit_or = false;
	i = 0;
	if(po.parsing_mode != PARSING_MODE_RPN) {
		// determine if | is used for absolute value
		while(po.base != BASE_ROMAN_NUMERALS && (i = str.find('|', i)) != string::npos) {
			if(i == 0 || i == str.length() - 1 || is_in(OPERATORS INTERNAL_OPERATORS SPACE, str[i - 1])) {b_abs_or = true; break;}
			if(str[i + 1] == '|') {
				if(i == str.length() - 2) {b_abs_or = true; break;}
				if(b_bit_or) {
					b_abs_or = true;
					break;
				}
				i += 2;
			} else {
				b_bit_or = true;
				i++;
			}
		}
	}
	if(b_abs_or) {
		// |x|=abs(x)
		while((i = str.find('|', 0)) != string::npos && i + 1 != str.length()) {
			if(str[i + 1] == '|') {
				size_t depth = 1;
				i2 = i;
				while((i2 = str.find("||", i2 + 2)) != string::npos) {
					if(is_in(OPERATORS INTERNAL_OPERATORS, str[i2 - 1])) depth++;
					else depth--;
					if(depth == 0) break;
				}
				if(i2 == string::npos) str2 = str.substr(i + 2);
				else str2 = str.substr(i + 2, i2 - (i + 2));
				str3 = ID_WRAP_LEFT;
				str3 += i2s(parseAddId(f_magnitude, str2, po));
				str3 += ID_WRAP_RIGHT;
				if(i2 == string::npos) str.replace(i, str.length() - i, str3);
				else str.replace(i, i2 - i + 2, str3);
			} else {
				size_t depth = 1;
				i2 = i;
				while((i2 = str.find('|', i2 + 1)) != string::npos) {
					if(is_in(OPERATORS INTERNAL_OPERATORS, str[i2 - 1])) depth++;
					else depth--;
					if(depth == 0) break;
				}
				if(i2 == string::npos) str2 = str.substr(i + 1);
				else str2 = str.substr(i + 1, i2 - (i + 1));
				str3 = ID_WRAP_LEFT;
				str3 += i2s(parseAddId(f_abs, str2, po));
				str3 += ID_WRAP_RIGHT;
				if(i2 == string::npos) str.replace(i, str.length() - i, str3);
				else str.replace(i, i2 - i + 1, str3);
			}
		}
	}
	if(po.parsing_mode == PARSING_MODE_RPN) {
		// parse operators with RPN syntax
		i = 0;
		i3 = 0;
		ParseOptions po2 = po;
		po2.rpn = false;
		po2.parsing_mode = PARSING_MODE_CONVENTIONAL;
		vector<MathStructure*> mstack;
		bool b = false;
		char last_operator = 0;
		char last_operator2 = 0;
		while(true) {
			i = str.find_first_of(OPERATORS "\a%\x1c" SPACE "\\", i3 + 1);
			if(i == string::npos) {
				if(!b) {
					parseAdd(str, mstruct, po2);
					return true;
				}
				if(i3 != 0) {
					str2 = str.substr(i3 + 1, str.length() - i3 - 1);
				} else {
					str2 = str.substr(i3, str.length() - i3);
				}
				remove_blank_ends(str2);
				if(!str2.empty()) {
					error(false, _("RPN syntax error. Values left at the end of the RPN expression."), NULL);
				} else if(mstack.size() > 1) {
					if(last_operator == 0 && mstack.size() > 1) {
						error(false, _("Unused stack values."), NULL);
					} else {
						while(mstack.size() > 1) {
							switch(last_operator) {
								case PLUS_CH: {
									mstack[mstack.size() - 2]->add_nocopy(mstack.back());
									mstack.pop_back();
									break;
								}
								case MINUS_CH: {
									if(po.preserve_format) {
										mstack.back()->transform(STRUCT_NEGATE);
										mstack[mstack.size() - 2]->add_nocopy(mstack.back());
									} else {
										mstack[mstack.size() - 2]->subtract_nocopy(mstack.back());
									}
									mstack.pop_back();
									break;
								}
								case MULTIPLICATION_CH: {
									mstack[mstack.size() - 2]->multiply_nocopy(mstack.back());
									mstack.pop_back();
									break;
								}
								case DIVISION_CH: {
									if(po.preserve_format) {
										mstack[mstack.size() - 2]->transform_nocopy(STRUCT_DIVISION, mstack.back());
									} else {
										mstack[mstack.size() - 2]->divide_nocopy(mstack.back());
									}
									mstack.pop_back();
									break;
								}
								case POWER_CH: {
									mstack[mstack.size() - 2]->raise_nocopy(mstack.back());
									mstack.pop_back();
									break;
								}
								case AND_CH: {
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_BITWISE_AND, mstack.back());
									mstack.pop_back();
									break;
								}
								case OR_CH: {
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_BITWISE_OR, mstack.back());
									mstack.pop_back();
									break;
								}
								case GREATER_CH: {
									if(last_operator2 == GREATER_CH) {
										if(po.preserve_format) mstack.back()->transform(STRUCT_NEGATE);
										else mstack.back()->negate();
										mstack[mstack.size() - 2]->transform(f_shift);
										mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
										mstack[mstack.size() - 2]->addChild(m_one);
									} else if(last_operator2 == EQUALS_CH) {
										mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_EQUALS_GREATER);
									} else {
										mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_GREATER);
									}
									mstack.pop_back();
									break;
								}
								case LESS_CH: {
									if(last_operator2 == LESS_CH) {
										mstack[mstack.size() - 2]->transform(f_shift);
										mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
										mstack[mstack.size() - 2]->addChild(m_one);
									} else if(last_operator2 == EQUALS_CH) {
										mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_EQUALS_LESS);
									} else {
										mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_LESS);
									}
									mstack.pop_back();
									break;
								}
								case NOT_CH: {
									mstack.back()->transform(STRUCT_LOGICAL_NOT);
									break;
								}
								case EQUALS_CH: {
									if(last_operator2 == NOT_CH) {
										mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_NOT_EQUALS);
									} else {
										mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_EQUALS);
									}
									mstack.pop_back();
									break;
								}
								case BITWISE_NOT_CH: {
									mstack.back()->transform(STRUCT_BITWISE_NOT);
									error(false, _("Unused stack values."), NULL);
									break;
								}
								case '\x1c': {
									if(po.angle_unit != ANGLE_UNIT_NONE && po.angle_unit != ANGLE_UNIT_RADIANS && mstack.back()->contains(getRadUnit(), false, true, true) <= 0 && mstack.back()->contains(getGraUnit(), false, true, true) <= 0 && mstack.back()->contains(getDegUnit(), false, true, true) <= 0) {
										switch(po.angle_unit) {
											case ANGLE_UNIT_DEGREES: {mstack.back()->multiply(getDegUnit()); break;}
											case ANGLE_UNIT_GRADIANS: {mstack.back()->multiply(getGraUnit()); break;}
											default: {}
										}
									}
									mstack.back()->transform(priv->f_cis);
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_MULTIPLICATION, mstack.back());
									mstack.pop_back();
									break;
								}
								case '\a': {
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_BITWISE_XOR, mstack.back());
									mstack.pop_back();
									break;
								}
								case '%': {
									if(last_operator2 == '%') {
										mstack[mstack.size() - 2]->transform(f_mod);
									} else {
										mstack[mstack.size() - 2]->transform(f_rem);
									}
									mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
									mstack.pop_back();
									break;
								}
								case '\\': {
									if(po.preserve_format) {
										mstack[mstack.size() - 2]->transform_nocopy(STRUCT_DIVISION, mstack.back());
									} else {
										mstack[mstack.size() - 2]->divide_nocopy(mstack.back());
									}
									mstack[mstack.size() - 2]->transform(f_trunc);
									mstack.pop_back();
									break;
								}
								default: {
									error(true, _("RPN syntax error. Operator '%c' not supported."), last_operator, NULL);
									mstack.pop_back();
									break;
								}
							}
							if(last_operator == NOT_CH || last_operator == BITWISE_NOT_CH)  break;
						}
					}
				} else if(mstack.size() == 1) {
					if(last_operator == NOT_CH) {
						mstack.back()->transform(STRUCT_LOGICAL_NOT);
					} else if(last_operator == BITWISE_NOT_CH) {
						mstack.back()->transform(STRUCT_BITWISE_NOT);
					}
				}
				mstruct->set_nocopy(*mstack.back());
				while(!mstack.empty()) {
					mstack.back()->unref();
					mstack.pop_back();
				}
				return true;
			}
			b = true;
			if(i3 != 0) {
				str2 = str.substr(i3 + 1, i - i3 - 1);
			} else {
				str2 = str.substr(i3, i - i3);
			}
			remove_blank_ends(str2);
			if(!str2.empty()) {
				mstack.push_back(new MathStructure());
				if((str[i] == GREATER_CH || str[i] == LESS_CH) && po2.base < 10 && po2.base >= 2 && i + 1 < str.length() && str[i + 1] == str[i] && str2.find_first_not_of(NUMBERS SPACE PLUS MINUS) == string::npos) {
					for(i = 0; i < str2.size(); i++) {
						if(str2[i] >= '0' && str2[i] <= '9' && po.base <= str2[i] - '0') {
							po2.base = BASE_DECIMAL;
							break;
						}
					}
					parseAdd(str2, mstack.back(), po2);
					po2.base = po.base;
				} else {
					parseAdd(str2, mstack.back(), po2);
				}
			}
			if(str[i] != SPACE_CH) {
				if(mstack.size() < 1) {
					error(true, _("RPN syntax error. Stack is empty."), NULL);
				} else if(mstack.size() < 2) {
					if(str[i] == NOT_CH) {
						mstack.back()->transform(STRUCT_LOGICAL_NOT);
					} else if(str[i] == MINUS_CH) {
						if(po.preserve_format) mstack.back()->transform(STRUCT_NEGATE);
						else mstack.back()->negate();
					} else if(str[i] == BITWISE_NOT_CH) {
						mstack.back()->transform(STRUCT_BITWISE_NOT);
					} else if(str[i] == '\x1c') {
						if(po.angle_unit != ANGLE_UNIT_NONE && po.angle_unit != ANGLE_UNIT_RADIANS && mstack.back()->contains(getRadUnit(), false, true, true) <= 0 && mstack.back()->contains(getGraUnit(), false, true, true) <= 0 && mstack.back()->contains(getDegUnit(), false, true, true) <= 0) {
							switch(po.angle_unit) {
								case ANGLE_UNIT_DEGREES: {mstack.back()->multiply(getDegUnit()); break;}
								case ANGLE_UNIT_GRADIANS: {mstack.back()->multiply(getGraUnit()); break;}
								default: {}
							}
						}
						mstack.back()->transform(priv->f_cis);
						mstack.back()->multiply(m_one);
						if(po.preserve_format) mstack.back()->swapChildren(1, 2);
					} else {
						error(false, _("RPN syntax error. Operator ignored as there where only one stack value."), NULL);
					}
				} else {
					switch(str[i]) {
						case PLUS_CH: {
							mstack[mstack.size() - 2]->add_nocopy(mstack.back());
							mstack.pop_back();
							break;
						}
						case MINUS_CH: {
							if(po.preserve_format) {
								mstack.back()->transform(STRUCT_NEGATE);
								mstack[mstack.size() - 2]->add_nocopy(mstack.back());
							} else {
								mstack[mstack.size() - 2]->subtract_nocopy(mstack.back());
							}
							mstack.pop_back();
							break;
						}
						case MULTIPLICATION_CH: {
							mstack[mstack.size() - 2]->multiply_nocopy(mstack.back());
							mstack.pop_back();
							break;
						}
						case DIVISION_CH: {
							if(po.preserve_format) {
								mstack[mstack.size() - 2]->transform_nocopy(STRUCT_DIVISION, mstack.back());
							} else {
								mstack[mstack.size() - 2]->divide_nocopy(mstack.back());
							}
							mstack.pop_back();
							break;
						}
						case POWER_CH: {
							mstack[mstack.size() - 2]->raise_nocopy(mstack.back());
							mstack.pop_back();
							break;
						}
						case AND_CH: {
							if(i + 1 < str.length() && str[i + 1] == AND_CH) {
								mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_AND, mstack.back());
							} else {
								mstack[mstack.size() - 2]->transform_nocopy(STRUCT_BITWISE_AND, mstack.back());
							}
							mstack.pop_back();
							break;
						}
						case OR_CH: {
							if(i + 1 < str.length() && str[i + 1] == OR_CH) {
								mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_OR, mstack.back());
							} else {
								mstack[mstack.size() - 2]->transform_nocopy(STRUCT_BITWISE_OR, mstack.back());
							}
							mstack.pop_back();
							break;
						}
						case GREATER_CH: {
							if(i + 1 < str.length() && str[i + 1] == GREATER_CH) {
								if(po.preserve_format) mstack.back()->transform(STRUCT_NEGATE);
								else mstack.back()->negate();
								mstack[mstack.size() - 2]->transform(f_shift);
								mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
								mstack[mstack.size() - 2]->addChild(m_one);
							} else if(i + 1 < str.length() && str[i + 1] == EQUALS_CH) {
								mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_EQUALS_GREATER);
							} else {
								mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_GREATER);
							}
							mstack.pop_back();
							break;
						}
						case LESS_CH: {
							if(i + 1 < str.length() && str[i + 1] == LESS_CH) {
								mstack[mstack.size() - 2]->transform(f_shift);
								mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
								mstack[mstack.size() - 2]->addChild(m_one);
							} else if(i + 1 < str.length() && str[i + 1] == EQUALS_CH) {
								mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_EQUALS_LESS);
							} else {
								mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_LESS);
							}
							mstack.pop_back();
							break;
						}
						case NOT_CH: {
							mstack.back()->transform(STRUCT_LOGICAL_NOT);
							break;
						}
						case EQUALS_CH: {
							if(i + 1 < str.length() && str[i + 1] == NOT_CH) {
								mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_NOT_EQUALS);
								mstack.pop_back();
							} else {
								mstack[mstack.size() - 2]->add_nocopy(mstack.back(), OPERATION_EQUALS);
							}
							mstack.pop_back();
							break;
						}
						case BITWISE_NOT_CH: {
							mstack.back()->transform(STRUCT_BITWISE_NOT);
							break;
						}
						case '\x1c': {
							if(po.angle_unit != ANGLE_UNIT_NONE && po.angle_unit != ANGLE_UNIT_RADIANS && mstack.back()->contains(getRadUnit(), false, true, true) <= 0 && mstack.back()->contains(getGraUnit(), false, true, true) <= 0 && mstack.back()->contains(getDegUnit(), false, true, true) <= 0) {
								switch(po.angle_unit) {
									case ANGLE_UNIT_DEGREES: {mstack.back()->multiply(getDegUnit()); break;}
									case ANGLE_UNIT_GRADIANS: {mstack.back()->multiply(getGraUnit()); break;}
									default: {}
								}
							}
							mstack.back()->transform(priv->f_cis);
							mstack[mstack.size() - 2]->transform_nocopy(STRUCT_MULTIPLICATION, mstack.back());
							mstack.pop_back();
							break;
						}
						case '\a': {
							mstack[mstack.size() - 2]->transform_nocopy(STRUCT_BITWISE_XOR, mstack.back());
							mstack.pop_back();
							break;
						}
						case '%': {
							if(i + 1 < str.length() && str[i + 1] == '%') {
								mstack[mstack.size() - 2]->transform(f_mod);
							} else {
								mstack[mstack.size() - 2]->transform(f_rem);
							}
							mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
							mstack.pop_back();
							break;
						}
						case '\\': {
							if(po.preserve_format) {
								mstack[mstack.size() - 2]->transform_nocopy(STRUCT_DIVISION, mstack.back());
							} else {
								mstack[mstack.size() - 2]->divide_nocopy(mstack.back());
							}
							mstack[mstack.size() - 2]->transform(f_trunc);
							mstack.pop_back();
							break;
						}
						default: {
							error(true, _("RPN syntax error. Operator '%c' not supported."), str[i], NULL);
							mstack.pop_back();
							break;
						}
					}
					last_operator = str[i];
					if(i + 1 < str.length()) last_operator2 = str[i + 1];
					else last_operator2 = 0;
					if((last_operator2 == EQUALS_CH && (last_operator == GREATER_CH || last_operator == LESS_CH || last_operator == EQUALS_CH)) || (last_operator2 == NOT_CH && last_operator == EQUALS_CH) || (last_operator == last_operator2 && (last_operator == GREATER_CH || last_operator == LESS_CH || last_operator == '%' || last_operator == AND_CH || last_operator == OR_CH))) {
						i++;
					}
				}
			}
			i3 = i;
		}
	}
	if(po.parsing_mode == PARSING_MODE_RPN) remove_blanks(str);

	i = 0;
	i3 = 0;

	// Parse && as logical and
	if((i = str.find(LOGICAL_AND, 1)) != string::npos && i + 2 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 2 != str.length()) {
			str2 = str.substr(0, i);
			str = str.substr(i + 2, str.length() - (i + 2));
			if(b) {
				parseAdd(str2, mstruct, po, OPERATION_LOGICAL_AND, append);
				append = true;
			} else {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			i = str.find(LOGICAL_AND, 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_LOGICAL_AND, append);
		} else {
			parseAdd(str, mstruct, po);
		}
		return true;
	}
	// Parse || as logical or
	if(po.base != BASE_ROMAN_NUMERALS && (i = str.find(LOGICAL_OR, 1)) != string::npos && i + 2 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 2 != str.length()) {
			str2 = str.substr(0, i);
			str = str.substr(i + 2, str.length() - (i + 2));
			if(b) {
				parseAdd(str2, mstruct, po, OPERATION_LOGICAL_OR, append);
				append = true;
			} else {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			i = str.find(LOGICAL_OR, 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_LOGICAL_OR, append);
		} else {
			parseAdd(str, mstruct, po);
		}
		return true;
	}
	/*if((i = str.find(LOGICAL_XOR, 1)) != string::npos && i + strlen(LOGICAL_XOR) != str.length()) {
		str2 = str.substr(0, i);
		str = str.substr(i + strlen(LOGICAL_XOR), str.length() - (i + strlen(LOGICAL_XOR)));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_LOGICAL_XOR);
		return true;
	}*/
	// Parse | as bitwise or
	if(po.parsing_mode != PARSING_MODE_CHAIN && po.base != BASE_ROMAN_NUMERALS && (i = str.find(BITWISE_OR, 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 1 != str.length()) {
			str2 = str.substr(0, i);
			str = str.substr(i + 1, str.length() - (i + 1));
			if(b) {
				parseAdd(str2, mstruct, po, OPERATION_BITWISE_OR, append);
				append = true;
			} else {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			i = str.find(BITWISE_OR, 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_BITWISE_OR, append);
		} else {
			parseAdd(str, mstruct, po);
		}
		return true;
	}
	// Parse \a (internal single character substitution for xor operators) as bitwise xor
	if(po.parsing_mode != PARSING_MODE_CHAIN && (i = str.find('\a', 1)) != string::npos && i + 1 != str.length()) {
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_BITWISE_XOR);
		return true;
	}
	// Parse & as bitwise and
	if(po.parsing_mode != PARSING_MODE_CHAIN && (i = str.find(BITWISE_AND, 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 1 != str.length()) {
			str2 = str.substr(0, i);
			str = str.substr(i + 1, str.length() - (i + 1));
			if(b) {
				parseAdd(str2, mstruct, po, OPERATION_BITWISE_AND, append);
				append = true;
			} else {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			i = str.find(BITWISE_AND, 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_BITWISE_AND, append);
		} else {
			parseAdd(str, mstruct, po);
		}
		return true;
	}
	// Parse comparison operators (>, >=, <, <=, =, !=)
	if((i = str.find_first_of(LESS GREATER EQUALS NOT, 0)) != string::npos) {
		while(i != string::npos && ((str[i] == LOGICAL_NOT_CH && (i + 1 >= str.length() || str[i + 1] != EQUALS_CH)) || (str[i] == LESS_CH && i + 1 < str.length() && str[i + 1] == LESS_CH) || (str[i] == GREATER_CH && i + 1 < str.length() && str[i + 1] == GREATER_CH))) {
			i = str.find_first_of(LESS GREATER NOT EQUALS, i + 2);
		}
	}
	if(i != string::npos) {
		bool b = false;
		bool c = false;
		while(i != string::npos && str[i] == NOT_CH && str.length() > i + 1 && str[i + 1] == NOT_CH) {
			i++;
			if(i + 1 == str.length()) {
				c = true;
			}
		}
		MathOperation s = OPERATION_ADD;
		while(!c) {
			while(i != string::npos && ((str[i] == LOGICAL_NOT_CH && (i + 1 >= str.length() || str[i + 1] != EQUALS_CH)) || (str[i] == LESS_CH && i + 1 < str.length() && str[i + 1] == LESS_CH) || (str[i] == GREATER_CH && i + 1 < str.length() && str[i + 1] == GREATER_CH))) {
				i = str.find_first_of(LESS GREATER NOT EQUALS, i + 2);
				while(i != string::npos && str[i] == NOT_CH && str.length() > i + 1 && str[i + 1] == NOT_CH) {
					i++;
					if(i + 1 == str.length()) {
						i = string::npos;
					}
				}
			}
			if(i == string::npos) {
				str2 = str.substr(0, str.length());
			} else {
				str2 = str.substr(0, i);
			}
			if(b) {
				switch(i3) {
					case EQUALS_CH: {s = OPERATION_EQUALS; break;}
					case GREATER_CH: {s = OPERATION_GREATER; break;}
					case LESS_CH: {s = OPERATION_LESS; break;}
					case GREATER_CH * EQUALS_CH: {s = OPERATION_EQUALS_GREATER; break;}
					case LESS_CH * EQUALS_CH: {s = OPERATION_EQUALS_LESS; break;}
					case GREATER_CH * LESS_CH: {s = OPERATION_NOT_EQUALS; break;}
				}
				parseAdd(str2, mstruct, po, s);
			}
			if(i == string::npos) {
				return true;
			}
			if(!b) {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			if(str.length() > i + 1 && is_in(LESS GREATER NOT EQUALS, str[i + 1])) {
				if(str[i] == str[i + 1]) {
					i3 = str[i];
				} else {
					i3 = str[i] * str[i + 1];
					if(i3 == NOT_CH * EQUALS_CH) {
						i3 = GREATER_CH * LESS_CH;
					} else if(i3 == NOT_CH * LESS_CH) {
						i3 = GREATER_CH;
					} else if(i3 == NOT_CH * GREATER_CH) {
						i3 = LESS_CH;
					}
				}
				i++;
			} else {
				i3 = str[i];
			}
			str = str.substr(i + 1, str.length() - (i + 1));
			i = str.find_first_of(LESS GREATER NOT EQUALS, 0);
			while(i != string::npos && str[i] == NOT_CH && str.length() > i + 1 && str[i + 1] == NOT_CH) {
				i++;
				if(i + 1 == str.length()) {
					i = string::npos;
				}
			}
		}
	}

	if(po.parsing_mode == PARSING_MODE_CHAIN) {
		char c_operator = 0, prev_operator = 0;
		bool append = false;
		while(true) {
			i3 = str.find_first_not_of(OPERATORS INTERNAL_OPERATORS, 0);
			if(i3 == string::npos) i = string::npos;
			else i = str.find_first_of(PLUS MINUS MULTIPLICATION DIVISION POWER BITWISE_AND BITWISE_OR "<>%\x1c", i3);
			i3 = str.find_first_not_of(OPERATORS INTERNAL_OPERATORS, i);
			if(i3 == string::npos) i = string::npos;
			if(c_operator == 0) {
				if(i == string::npos) break;
				str2 = str.substr(0, i);
				parseAdd(str2, mstruct, po);
			} else {
				if(i == string::npos) str2 = str;
				else str2 = str.substr(0, i);
				append = c_operator == prev_operator || ((c_operator == DIVISION_CH || c_operator == MULTIPLICATION_CH || c_operator == '%') && (prev_operator == DIVISION_CH || prev_operator == MULTIPLICATION_CH || prev_operator == '%')) || ((c_operator == PLUS_CH || c_operator == MINUS_CH) && (prev_operator == PLUS_CH || prev_operator == MINUS_CH));
				switch(c_operator) {
					case MINUS_CH: {}
					case PLUS_CH: {
						if(parseAdd(str2, mstruct, po, c_operator == PLUS_CH ? OPERATION_ADD : OPERATION_SUBTRACT, append)) {
							int i_neg = 0;
							MathStructure *mstruct_a = get_out_of_negate(mstruct->last(), &i_neg);
							MathStructure *mstruct_b = mstruct_a;
							if(mstruct_a->isMultiplication() && mstruct_a->size() >= 2) mstruct_b = &mstruct_a->last();
							if(mstruct_b->isVariable() && (mstruct_b->variable() == v_percent || mstruct_b->variable() == v_permille || mstruct_b->variable() == v_permyriad)) {
								Variable *v = mstruct_b->variable();
								bool b_neg = (i_neg % 2 == 1);
								while(i_neg > 0) {
									mstruct->last().setToChild(mstruct->last().size());
									i_neg--;
								}
								if(mstruct->last().isVariable()) {
									mstruct->last().multiply(m_one);
									mstruct->last().swapChildren(1, 2);
								}
								if(mstruct->last().size() > 2) {
									mstruct->last().delChild(mstruct->last().size());
									mstruct->last().multiply(v);
								}
								if(mstruct->last()[0].isNumber()) {
									if(b_neg) mstruct->last()[0].number().negate();
									if(v == v_percent) mstruct->last()[0].number().add(100);
									else if(v == v_permille) mstruct->last()[0].number().add(1000);
									else mstruct->last()[0].number().add(10000);
								} else {
									if(b_neg && po.preserve_format) mstruct->last()[0].transform(STRUCT_NEGATE);
									else if(b_neg) mstruct->last()[0].negate();
									if(v == v_percent) mstruct->last()[0] += Number(100, 1);
									else if(v == v_permille) mstruct->last()[0] += Number(1000, 1);
									else mstruct->last()[0] += Number(10000, 1);
									mstruct->last()[0].swapChildren(1, 2);
								}
								if(mstruct->size() == 2) {
									mstruct->setType(STRUCT_MULTIPLICATION);
								} else {
									MathStructure *mpercent = &mstruct->last();
									mpercent->ref();
									mstruct->delChild(mstruct->size());
									mstruct->multiply_nocopy(mpercent);
								}
							}
						}
						break;
					}
					case MULTIPLICATION_CH: {
						parseAdd(str2, mstruct, po, OPERATION_MULTIPLY, append);
						break;
					}
					case DIVISION_CH: {
						if(str2[0] == DIVISION_CH) {
							str2.erase(0, 1);
							parseAdd(str2, mstruct, po, OPERATION_DIVIDE, append);
							mstruct->transform(f_trunc);
						} else {
							parseAdd(str2, mstruct, po, OPERATION_DIVIDE, append);
						}
						break;
					}
					case '%': {
						if(str2[0] == '%') {
							str2.erase(0, 1);
							MathStructure *mstruct2 = new MathStructure();
							parseAdd(str2, mstruct2, po);
							mstruct->transform(f_mod);
							mstruct->addChild_nocopy(mstruct2);
						} else {
							MathStructure *mstruct2 = new MathStructure();
							parseAdd(str2, mstruct2, po);
							mstruct->transform(f_rem);
							mstruct->addChild_nocopy(mstruct2);
						}
						break;
					}
					case POWER_CH: {
						parseAdd(str2, mstruct, po, OPERATION_RAISE);
						break;
					}
					case AND_CH: {
						parseAdd(str2, mstruct, po, OPERATION_BITWISE_AND, append);
						break;
					}
					case OR_CH: {
						parseAdd(str2, mstruct, po, OPERATION_BITWISE_OR, append);
						break;
					}
					case '<': {}
					case '>': {
						MathStructure mstruct2;
						bool b_neg = (c_operator == '>');
						str2.erase(0, 1);
						bool b = false;
						if(po.base < 10 && po.base >= 2 && str2.find_first_not_of(NUMBERS SPACE PLUS MINUS) == string::npos) {
							for(i = 0; i < str2.size(); i++) {
								if(str2[i] >= '0' && str2[i] <= '9' && po.base <= str2[i] - '0') {
									ParseOptions po2 = po;
									po2.base = BASE_DECIMAL;
									parseAdd(str2, &mstruct2, po2);
									if(b_neg) {
										if(po.preserve_format) mstruct2.transform(STRUCT_NEGATE);
										else mstruct2.negate();
									}
									mstruct->transform(f_shift);
									mstruct->addChild(mstruct2);
									mstruct->addChild(m_one);
									b = true;
									break;
								}
							}
						}
						if(!b) {
							parseAdd(str2, &mstruct2, po);
							if(b_neg) {
								if(po.preserve_format) mstruct2.transform(STRUCT_NEGATE);
								else mstruct2.negate();
							}
							mstruct->transform(f_shift);
							mstruct->addChild(mstruct2);
							mstruct->addChild(m_one);
						}
						break;
					}
					case '\x1c': {
						if(parseAdd(str2, mstruct, po, OPERATION_MULTIPLY)) {
							if(po.angle_unit != ANGLE_UNIT_NONE && po.angle_unit != ANGLE_UNIT_RADIANS && mstruct->last().contains(getRadUnit(), false, true, true) <= 0 && mstruct->last().contains(getGraUnit(), false, true, true) <= 0 && mstruct->last().contains(getDegUnit(), false, true, true) <= 0) {
								switch(po.angle_unit) {
									case ANGLE_UNIT_DEGREES: {mstruct->last().multiply(getDegUnit()); break;}
									case ANGLE_UNIT_GRADIANS: {mstruct->last().multiply(getGraUnit()); break;}
									default: {}
								}
							}
							mstruct->last().transform(priv->f_cis);
						}
						break;
					}
				}
			}
			if(i == string::npos) return true;
			prev_operator = c_operator;
			c_operator = str[i];
			str = str.substr(i + 1);
		}
	}

	if(po.parsing_mode != PARSING_MODE_CHAIN) {

		// Parse << and >> as bitwise shift
		i = str.find(SHIFT_LEFT, 1);
		i2 = str.find(SHIFT_RIGHT, 1);
		if(i2 != string::npos && (i == string::npos || i2 < i)) i = i2;
		if(i != string::npos && i + 2 != str.length()) {
			MathStructure mstruct1, mstruct2;
			bool b_neg = (str[i] == '>');
			str2 = str.substr(0, i);
			str = str.substr(i + 2, str.length() - (i + 2));
			parseAdd(str2, &mstruct1, po);
			if(po.base < 10 && po.base >= 2 && str.find_first_not_of(NUMBERS SPACE PLUS MINUS) == string::npos) {
				for(i = 0; i < str.size(); i++) {
					if(str[i] >= '0' && str[i] <= '9' && po.base <= str[i] - '0') {
						ParseOptions po2 = po;
						po2.base = BASE_DECIMAL;
						parseAdd(str, &mstruct2, po2);
						if(b_neg) {
							if(po.preserve_format) mstruct2.transform(STRUCT_NEGATE);
							else mstruct2.negate();
						}
						mstruct->set(f_shift, &mstruct1, &mstruct2, &m_one, NULL);
						return true;
					}
				}
			}
			parseAdd(str, &mstruct2, po);
			if(b_neg) {
				if(po.preserve_format) mstruct2.transform(STRUCT_NEGATE);
				else mstruct2.negate();
			}
			mstruct->set(f_shift, &mstruct1, &mstruct2, &m_one, NULL);
			return true;
		}

		// Parse addition and subtraction
		if((i = str.find_first_of(PLUS MINUS, 1)) != string::npos && i + 1 != str.length()) {
			bool b = false, c = false, append = false, do_percent = true;
			bool min = false;
			while(i != string::npos && i + 1 != str.length()) {
				if(is_not_in(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS EXPS, str[i - 1])) {
					str2 = str.substr(0, i);
					if(!c && b) {
						bool b_add;
						if(min) {
							b_add = parseAdd(str2, mstruct, po, OPERATION_SUBTRACT, append) && mstruct->isAddition();
						} else {
							b_add = parseAdd(str2, mstruct, po, OPERATION_ADD, append) && mstruct->isAddition();
						}
						append = true;
						if(b_add && do_percent) {
							int i_neg = 0;
							MathStructure *mstruct_a = get_out_of_negate(mstruct->last(), &i_neg);
							MathStructure *mstruct_b = mstruct_a;
							if(mstruct_a->isMultiplication() && mstruct_a->size() >= 2) mstruct_b = &mstruct_a->last();
							if(mstruct_b->isVariable() && (mstruct_b->variable() == v_percent || mstruct_b->variable() == v_permille || mstruct_b->variable() == v_permyriad)) {
								Variable *v = mstruct_b->variable();
								bool b_neg = (i_neg % 2 == 1);
								while(i_neg > 0) {
									mstruct->last().setToChild(mstruct->last().size());
									i_neg--;
								}
								if(mstruct->last().isVariable()) {
									mstruct->last().multiply(m_one);
									mstruct->last().swapChildren(1, 2);
								}
								if(mstruct->last().size() > 2) {
									mstruct->last().delChild(mstruct->last().size());
									mstruct->last().multiply(v);
								}
								if(mstruct->last()[0].isNumber()) {
									if(b_neg) mstruct->last()[0].number().negate();
									if(v == v_percent) mstruct->last()[0].number().add(100);
									else if(v == v_permille) mstruct->last()[0].number().add(1000);
									else mstruct->last()[0].number().add(10000);
								} else {
									if(b_neg && po.preserve_format) mstruct->last()[0].transform(STRUCT_NEGATE);
									else if(b_neg) mstruct->last()[0].negate();
									if(v == v_percent) mstruct->last()[0] += Number(100, 1);
									else if(v == v_permille) mstruct->last()[0] += Number(1000, 1);
									else mstruct->last()[0] += Number(10000, 1);
									mstruct->last()[0].swapChildren(1, 2);
								}
								if(mstruct->size() == 2) {
									mstruct->setType(STRUCT_MULTIPLICATION);
								} else {
									MathStructure *mpercent = &mstruct->last();
									mpercent->ref();
									mstruct->delChild(mstruct->size());
									mstruct->multiply_nocopy(mpercent);
								}
							}
						}
					} else {
						if(!b && str2.empty()) {
							c = true;
						} else {
							parseAdd(str2, mstruct, po);
							MathStructure *mstruct_a = get_out_of_negate(*mstruct, NULL);
							if((str2.length() < 3 || str2[0] != ID_WRAP_LEFT_CH || str2[str2.length() - 1] != ID_WRAP_RIGHT_CH || str.find(ID_WRAP_LEFT_CH, 1) != string::npos) && mstruct_a->isMultiplication()) mstruct_a = &mstruct_a->last();
							if(mstruct_a->isVariable() && (mstruct_a->variable() == v_percent || mstruct_a->variable() == v_permille || mstruct_a->variable() == v_permyriad)) do_percent = false;
							if(c && min) {
								if(po.preserve_format) mstruct->transform(STRUCT_NEGATE);
								else mstruct->negate();
							}
							c = false;
						}
						b = true;
					}
					min = str[i] == MINUS_CH;
					str = str.substr(i + 1, str.length() - (i + 1));
					i = str.find_first_of(PLUS MINUS, 1);
				} else {
					i = str.find_first_of(PLUS MINUS, i + 1);
				}
			}
			if(b) {
				if(c) {
					b = parseAdd(str, mstruct, po);
					if(min) {
						if(po.preserve_format) mstruct->transform(STRUCT_NEGATE);
						else mstruct->negate();
					}
					return b;
				} else {
					bool b_add;
					if(min) {
						b_add = parseAdd(str, mstruct, po, OPERATION_SUBTRACT, append) && mstruct->isAddition();
					} else {
						b_add = parseAdd(str, mstruct, po, OPERATION_ADD, append) && mstruct->isAddition();
					}
					if(b_add && do_percent) {
						int i_neg = 0;
						MathStructure *mstruct_a = get_out_of_negate(mstruct->last(), &i_neg);
						MathStructure *mstruct_b = mstruct_a;
						if(mstruct_a->isMultiplication() && mstruct_a->size() >= 2) mstruct_b = &mstruct_a->last();
						if(mstruct_b->isVariable() && (mstruct_b->variable() == v_percent || mstruct_b->variable() == v_permille || mstruct_b->variable() == v_permyriad)) {
							Variable *v = mstruct_b->variable();
							bool b_neg = (i_neg % 2 == 1);
							while(i_neg > 0) {
								mstruct->last().setToChild(mstruct->last().size());
								i_neg--;
							}
							if(mstruct->last().isVariable()) {
								mstruct->last().multiply(m_one);
								mstruct->last().swapChildren(1, 2);
							}
							if(mstruct->last().size() > 2) {
								mstruct->last().delChild(mstruct->last().size());
								mstruct->last().multiply(v);
							}
							if(mstruct->last()[0].isNumber()) {
								if(b_neg) mstruct->last()[0].number().negate();
								if(v == v_percent) mstruct->last()[0].number().add(100);
								else if(v == v_permille) mstruct->last()[0].number().add(1000);
								else mstruct->last()[0].number().add(10000);
							} else {
								if(b_neg && po.preserve_format) mstruct->last()[0].transform(STRUCT_NEGATE);
								else if(b_neg) mstruct->last()[0].negate();
								if(v == v_percent) mstruct->last()[0] += Number(100, 1);
								else if(v == v_permille) mstruct->last()[0] += Number(1000, 1);
								else mstruct->last()[0] += Number(10000, 1);
								mstruct->last()[0].swapChildren(1, 2);
							}
							if(mstruct->size() == 2) {
								mstruct->setType(STRUCT_MULTIPLICATION);
							} else {
								MathStructure *mpercent = &mstruct->last();
								mpercent->ref();
								mstruct->delChild(mstruct->size());
								mstruct->multiply_nocopy(mpercent);
							}
						}
					}
				}
				return true;
			}
		}

		// In adaptive parsing mode division might be handled differentiately depending on usage of whitespace characters, e.g. 5/2 m = (5/2)*m, 5/2m=5/(2m)
		if(po.parsing_mode == PARSING_MODE_ADAPTIVE && (i = str.find(DIVISION_CH, 1)) != string::npos && i + 1 != str.length()) {
			while(i != string::npos && i + 1 != str.length()) {
				bool b = false;
				if(i > 2 && i < str.length() - 3 && str[i + 1] == ID_WRAP_LEFT_CH) {
					i2 = i;
					b = true;
					bool had_unit = false, had_nonunit = false;
					MathStructure *m_temp = NULL, *m_temp2 = NULL;
					while(b) {
						b = false;
						size_t i4 = i2;
						if(i2 > 2 && str[i2 - 1] == ID_WRAP_RIGHT_CH) {
							b = true;
						} else if(i2 > 4 && str[i2 - 3] == ID_WRAP_RIGHT_CH && str[i2 - 2] == POWER_CH && is_in(NUMBERS, str[i2 - 1])) {
							b = true;
							i4 -= 2;
						}
						if(!b) {
							if((i2 > 1 && is_not_in(OPERATORS INTERNAL_OPERATORS MULTIPLICATION_2, str[i2 - 1])) || (i2 > 2 && str[i2 - 1] == MULTIPLICATION_2_CH && is_not_in(OPERATORS INTERNAL_OPERATORS, str[i2 - 2]))) had_nonunit = true;
							break;
						}
						i2 = str.rfind(ID_WRAP_LEFT_CH, i4 - 2);
						m_temp = NULL;
						if(i2 != string::npos) {
							int id = s2i(str.substr(i2 + 1, (i4 - 1) - (i2 + 1)));
							if(priv->id_structs.find(id) != priv->id_structs.end()) m_temp = priv->id_structs[id];
						}
						if(!m_temp || !m_temp->isUnit()) {
							had_nonunit = true;
							break;
						}
						had_unit = true;
					}
					i3 = i;
					b = had_unit && had_nonunit;
					had_unit = false;
					while(b) {
						size_t i4 = i3;
						i3 = str.find(ID_WRAP_RIGHT_CH, i4 + 2);
						m_temp2 = NULL;
						if(i3 != string::npos) {
							int id = s2i(str.substr(i4 + 2, (i3 - 1) - (i4 + 1)));
							if(priv->id_structs.find(id) != priv->id_structs.end()) m_temp2 = priv->id_structs[id];
						}
						if(!m_temp2 || !m_temp2->isUnit()) {
							b = false;
							break;
						}
						had_unit = true;
						b = false;
						if(i3 < str.length() - 3 && str[i3 + 1] == ID_WRAP_LEFT_CH) {
							b = true;
						} else if(i3 < str.length() - 5 && str[i3 + 3] == ID_WRAP_LEFT_CH && str[i3 + 1] == POWER_CH && is_in(NUMBERS, str[i3 + 2])) {
							b = true;
							i3 += 2;
						}
					}
					b = had_unit;
					if(b) {
						if(i3 < str.length() - 2 && str[i3 + 1] == POWER_CH && is_in(NUMBERS, str[i3 + 2])) {
							i3 += 2;
							while(i3 < str.length() - 1 && is_in(NUMBERS, str[i3 + 1])) i3++;
						}
						if(i3 == str.length() - 1 || (str[i3 + 1] != POWER_CH && str[i3 + 1] != DIVISION_CH)) {
							MathStructure *mstruct2 = new MathStructure();
							str2 = str.substr(i2, i - i2);
							parseAdd(str2, mstruct2, po);
							str2 = str.substr(i + 1, i3 - i);
							parseAdd(str2, mstruct2, po, OPERATION_DIVIDE);
							str2 = ID_WRAP_LEFT;
							str2 += i2s(addId(mstruct2));
							str2 += ID_WRAP_RIGHT;
							str.replace(i2, i3 - i2 + 1, str2);
						} else {
							b = false;
						}
					}
				}
				if(!b) {
					i2 = str.find_last_not_of(NUMBERS INTERNAL_NUMBER_CHARS PLUS MINUS EXPS, i - 1);
					if(i2 == string::npos || (i2 != i - 1 && str[i2] == MULTIPLICATION_2_CH)) b = true;
					i2 = str.rfind(MULTIPLICATION_2_CH, i - 1);
					if(i2 == string::npos) b = true;
					if(b) {
						i3 = str.find_first_of(MULTIPLICATION_2 "%" MULTIPLICATION DIVISION, i + 1);
						if(i3 == string::npos || i3 == i + 1 || str[i3] != MULTIPLICATION_2_CH) b = false;
						if(i3 < str.length() + 1 && (str[i3 + 1] == '%' || str[i3 + 1] == DIVISION_CH || str[i3 + 1] == MULTIPLICATION_CH || str[i3 + 1] == POWER_CH)) b = false;
					}
					if(b) {
						if(i3 != string::npos) str[i3] = MULTIPLICATION_CH;
						if(i2 != string::npos) str[i2] = MULTIPLICATION_CH;
					} else {
						if(str[i + 1] == MULTIPLICATION_2_CH) {
							str.erase(i + 1, 1);
						}
						if(str[i - 1] == MULTIPLICATION_2_CH) {
							str.erase(i - 1, 1);
							i--;
						}
					}
				}
				i = str.find(DIVISION_CH, i + 1);
			}
		}
		if(po.parsing_mode == PARSING_MODE_ADAPTIVE) remove_blanks(str);

		// In conventional parsing mode there is not difference between implicit and explicit multiplication
		if(po.parsing_mode >= PARSING_MODE_CONVENTIONAL) {
			if((i = str.find(ID_WRAP_RIGHT_CH, 1)) != string::npos && i + 1 != str.length()) {
				while(i != string::npos && i + 1 != str.length()) {
					if(is_in(NUMBERS ID_WRAP_LEFT, str[i + 1])) {
						str.insert(i + 1, 1, MULTIPLICATION_CH);
						i++;
					}
					i = str.find(ID_WRAP_RIGHT_CH, i + 1);
				}
			}
			if((i = str.find(ID_WRAP_LEFT_CH, 1)) != string::npos) {
				while(i != string::npos) {
					if(is_in(NUMBERS, str[i - 1])) {
						str.insert(i, 1, MULTIPLICATION_CH);
						i++;
					}
					i = str.find(ID_WRAP_LEFT_CH, i + 1);
				}
			}
		}

		// Parse explicit multiplication, division, and mod
		if((i = str.find_first_of(MULTIPLICATION DIVISION "%", 0)) != string::npos && i + 1 != str.length()) {
			bool b = false, append = false;
			int type = 0;
			while(i != string::npos && i + 1 != str.length()) {
				if(i < 1) {
					if(i < 1 && str.find_first_not_of(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS EXPS) == string::npos) {
						replace_internal_operators(str);
						error(false, _("Misplaced operator(s) \"%s\" ignored"), str.c_str(), NULL);
						return b;
					}
					i = 1;
					while(i < str.length() && is_in(MULTIPLICATION DIVISION "%", str[i])) {
						i++;
					}
					string errstr = str.substr(0, i);
					gsub("\a", str.find_first_of(OPERATORS "%") != string::npos ? " xor " : "xor", errstr);
					error(false, _("Misplaced operator(s) \"%s\" ignored"), errstr.c_str(), NULL);
					str = str.substr(i, str.length() - i);
					i = str.find_first_of(MULTIPLICATION DIVISION "%", 0);
				} else {
					str2 = str.substr(0, i);
					if(b) {
						switch(type) {
							case 1: {
								parseAdd(str2, mstruct, po, OPERATION_DIVIDE, append);
								if(po.parsing_mode == PARSING_MODE_ADAPTIVE && !str2.empty() && str2[0] != LEFT_PARENTHESIS_CH) {
									MathStructure *mden = NULL, *mnum = NULL;
									if(po.preserve_format && mstruct->isDivision()) {
										mden = &(*mstruct)[1];
										mnum = &(*mstruct)[0];
									} else if(!po.preserve_format && mstruct->isMultiplication() && mstruct->size() >= 2 && mstruct->last().isPower()) {
										mden = &mstruct->last()[0];
										mnum = &(*mstruct)[mstruct->size() - 2];
									}
									while(mnum && mnum->isMultiplication() && mnum->size() > 0) mnum = &mnum->last();
									if(mden && mden->isMultiplication() && (mden->size() != 2 || !(*mden)[0].isNumber() || !(*mden)[1].isUnit_exp() || !mnum->isUnit_exp())) {
										bool b_warn = str2[0] != ID_WRAP_LEFT_CH;
										if(!b_warn && str2.length() > 2) {
											size_t i3 = str2.find_first_not_of(NUMBERS, 1);
											b_warn = (i3 != string::npos && i3 != str2.length() - 1);
										}
										if(b_warn) error(false, _("The expression is ambiguous (be careful when combining implicit multiplication and division)."), NULL);
									}
								}
								break;
							}
							case 2: {
								MathStructure *mstruct2 = new MathStructure();
								parseAdd(str2, mstruct2, po);
								mstruct->transform(f_rem);
								mstruct->addChild_nocopy(mstruct2);
								break;
							}
							case 3: {
								parseAdd(str2, mstruct, po, OPERATION_DIVIDE, append);
								mstruct->transform(f_trunc);
								break;
							}
							case 4: {
								MathStructure *mstruct2 = new MathStructure();
								parseAdd(str2, mstruct2, po);
								mstruct->transform(f_mod);
								mstruct->addChild_nocopy(mstruct2);
								break;
							}
							default: {
								parseAdd(str2, mstruct, po, OPERATION_MULTIPLY, append);
							}
						}
						append = true;
					} else {
						parseAdd(str2, mstruct, po);
						b = true;
					}
					if(str[i] == DIVISION_CH) {
						if(str[i + 1] == DIVISION_CH) {type = 3; i++;}
						else type = 1;
					} else if(str[i] == '%') {
						if(str[i + 1] == '%') {type = 4; i++;}
						else type = 2;
					} else {
						type = 0;
					}
					if(is_in(MULTIPLICATION DIVISION "%", str[i + 1])) {
						i2 = 1;
						while(i2 + i + 1 != str.length() && is_in(MULTIPLICATION DIVISION "%", str[i2 + i + 1])) {
							i2++;
						}
						string errstr = str.substr(i, i2);
						gsub("\a", str.find_first_of(OPERATORS "%") != string::npos ? " xor " : "xor", errstr);
						error(false, _("Misplaced operator(s) \"%s\" ignored"), errstr.c_str(), NULL);
						i += i2;
					}
					str = str.substr(i + 1, str.length() - (i + 1));
					i = str.find_first_of(MULTIPLICATION DIVISION "%", 0);
				}
			}
			if(b) {
				switch(type) {
					case 1: {
						parseAdd(str, mstruct, po, OPERATION_DIVIDE, append);
						if(po.parsing_mode == PARSING_MODE_ADAPTIVE && !str.empty() && str[0] != LEFT_PARENTHESIS_CH) {
							MathStructure *mden = NULL, *mnum = NULL;
							if(po.preserve_format && mstruct->isDivision()) {
								mden = &(*mstruct)[1];
								mnum = &(*mstruct)[0];
							} else if(!po.preserve_format && mstruct->isMultiplication() && mstruct->size() >= 2 && mstruct->last().isPower()) {
								mden = &mstruct->last()[0];
								mnum = &(*mstruct)[mstruct->size() - 2];
							}
							while(mnum && mnum->isMultiplication() && mnum->size() > 0) mnum = &mnum->last();
							if(mden && mden->isMultiplication() && (mden->size() != 2 || !(*mden)[0].isNumber() || !(*mden)[1].isUnit_exp() || !mnum->isUnit_exp())) {
								bool b_warn = str[0] != ID_WRAP_LEFT_CH;
								if(!b_warn && str.length() > 2) {
									size_t i3 = str.find_first_not_of(NUMBERS, 1);
									b_warn = (i3 != string::npos && i3 != str.length() - 1);
								}
								if(b_warn) error(false, _("The expression is ambiguous (be careful when combining implicit multiplication and division)."), NULL);
							}
						}
						break;
					}
					case 2: {
						MathStructure *mstruct2 = new MathStructure();
						parseAdd(str, mstruct2, po);
						mstruct->transform(f_rem);
						mstruct->addChild_nocopy(mstruct2);
						break;
					}
					case 3: {
						parseAdd(str, mstruct, po, OPERATION_DIVIDE, append);
						mstruct->transform(f_trunc);
						break;
					}
					case 4: {
						MathStructure *mstruct2 = new MathStructure();
						parseAdd(str, mstruct2, po);
						mstruct->transform(f_mod);
						mstruct->addChild_nocopy(mstruct2);
						break;
					}
					default: {
						parseAdd(str, mstruct, po, OPERATION_MULTIPLY, append);
					}
				}
				return true;
			}
		}
	}

	// Parse \x1c (internal single substitution character for angle operator) for complex angle format
	if((i = str.find('\x1c', 0)) != string::npos && i + 1 != str.length() && (po.parsing_mode != PARSING_MODE_CHAIN || i == 0)) {
		if(i != 0) str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		if(i != 0) parseAdd(str2, mstruct, po);
		else mstruct->set(1, 1, 0);
		if(parseAdd(str, mstruct, po, OPERATION_MULTIPLY)) {
			if(po.angle_unit != ANGLE_UNIT_NONE && po.angle_unit != ANGLE_UNIT_RADIANS && mstruct->last().contains(getRadUnit(), false, true, true) <= 0 && mstruct->last().contains(getGraUnit(), false, true, true) <= 0 && mstruct->last().contains(getDegUnit(), false, true, true) <= 0) {
				switch(po.angle_unit) {
					case ANGLE_UNIT_DEGREES: {mstruct->last().multiply(getDegUnit()); break;}
					case ANGLE_UNIT_GRADIANS: {mstruct->last().multiply(getGraUnit()); break;}
					default: {}
				}
			}
			mstruct->last().transform(priv->f_cis);
		}
		return true;
	}

	if(str.empty()) return false;

	// Check if only operators are left
	if(str.find_first_not_of(OPERATORS INTERNAL_OPERATORS SPACE) == string::npos && (po.base != BASE_ROMAN_NUMERALS || str.find_first_of("(|)") == string::npos)) {
		replace_internal_operators(str);
		error(false, _("Misplaced operator(s) \"%s\" ignored"), str.c_str(), NULL);
		return false;
	}

	// Number signs (+ and - at the beginning of the string)
	i = 0;
	bool ret = true;
	bool has_sign = false;
	int minus_count = 0;
	while(i < str.length()) {
		if(str[i] == MINUS_CH) {
			has_sign = true;
			minus_count++;
			str.erase(i, 1);
		} else if(str[i] == PLUS_CH) {
			has_sign = true;
			str.erase(i, 1);
		} else if(str[i] == SPACE_CH) {
			str.erase(i, 1);
		} else if(str[i] == BITWISE_NOT_CH || str[i] == LOGICAL_NOT_CH) {
			break;
		} else if(is_in(OPERATORS INTERNAL_OPERATORS, str[i]) && (po.base != BASE_ROMAN_NUMERALS || (str[i] != '(' && str[i] != ')' && str[i] != '|'))) {
			if(str[i] == '\a') error(false, _("Misplaced operator(s) \"%s\" ignored"), "xor", NULL);
			else if(str[i] == '\b') error(false, _("Misplaced operator(s) \"%s\" ignored"), SIGN_PLUSMINUS, NULL);
			else if(str[i] == '\x1c') error(false, _("Misplaced operator(s) \"%s\" ignored"), "∠", NULL);
			else error(false, _("Misplaced '%c' ignored"), str[i], NULL);
			str.erase(i, 1);
		} else {
			break;
		}
	}

	// Parse ~ and ! at the beginning of the string as bitwise not and logical not
	if(!str.empty() && (str[0] == BITWISE_NOT_CH || str[0] == LOGICAL_NOT_CH)) {
		bool bit = (str[0] == BITWISE_NOT_CH);
		str.erase(0, 1);
		parseAdd(str, mstruct, po);
		if(bit) mstruct->setBitwiseNot();
		else mstruct->setLogicalNot();
		if(po.preserve_format) {
			while(minus_count > 0) {
				mstruct->transform(STRUCT_NEGATE);
				minus_count--;
			}
		} else if(minus_count % 2 == 1) {
			mstruct->negate();
		}
		return true;
	}

	if(str.empty()) {
		if(minus_count % 2 == 1 && !po.preserve_format) {
			mstruct->set(-1, 1, 0);
		} else if(has_sign) {
			mstruct->set(1, 1, 0);
			if(po.preserve_format) {
				while(minus_count > 0) {
					mstruct->transform(STRUCT_NEGATE);
					minus_count--;
				}
			}
		}
		return false;
	}

	// Implicit multiplication
	if((i = str.find(ID_WRAP_RIGHT_CH, 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 1 != str.length()) {
			if(str[i + 1] != POWER_CH && str[i + 1] != '\b') {
				str2 = str.substr(0, i + 1);
				str = str.substr(i + 1, str.length() - (i + 1));
				if(b) {
					parseAdd(str2, mstruct, po, OPERATION_MULTIPLY, append);
					append = true;
				} else {
					parseAdd(str2, mstruct, po);
					b = true;
				}
				i = str.find(ID_WRAP_RIGHT_CH, 1);
			} else {
				i = str.find(ID_WRAP_RIGHT_CH, i + 1);
			}
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_MULTIPLY, append);
			if(po.parsing_mode == PARSING_MODE_ADAPTIVE && mstruct->isMultiplication() && mstruct->size() >= 2 && !(*mstruct)[0].inParentheses()) {
				Unit *u1 = NULL; Prefix *p1 = NULL;
				bool b_plus = false;
				// In adaptive parsing mode, parse 5m 2cm as 5m+2cm, 5ft 2in as 5ft+2in, and similar
				if((*mstruct)[0].isMultiplication() && (*mstruct)[0].size() == 2 && (*mstruct)[0][0].isNumber() && (*mstruct)[0][1].isUnit()) {u1 = (*mstruct)[0][1].unit(); p1 = (*mstruct)[0][1].prefix();}
				if(u1 && u1->subtype() == SUBTYPE_BASE_UNIT && (u1->referenceName() == "m" || (!p1 && u1->referenceName() == "L")) && (!p1 || (p1->type() == PREFIX_DECIMAL && ((DecimalPrefix*) p1)->exponent() <= 3 && ((DecimalPrefix*) p1)->exponent() > -3))) {
					b_plus = true;
					for(size_t i2 = 1; i2 < mstruct->size(); i2++) {
						if(!(*mstruct)[i2].inParentheses() && (*mstruct)[i2].isMultiplication() && (*mstruct)[i2].size() == 2 && (*mstruct)[i2][0].isNumber() && (*mstruct)[i2][1].isUnit() && (*mstruct)[i2][1].unit() == u1) {
							Prefix *p2 = (*mstruct)[i2][1].prefix();
							if(p1 && p2) b_plus = p1->type() == PREFIX_DECIMAL && p2->type() == PREFIX_DECIMAL && ((DecimalPrefix*) p1)->exponent() > ((DecimalPrefix*) p2)->exponent() && ((DecimalPrefix*) p2)->exponent() >= -3;
							else if(p2) b_plus = p2->type() == PREFIX_DECIMAL && ((DecimalPrefix*) p2)->exponent() < 0 && ((DecimalPrefix*) p2)->exponent() >= -3;
							else if(p1) b_plus = p1->type() == PREFIX_DECIMAL && ((DecimalPrefix*) p1)->exponent() > 1;
							else b_plus = false;
							if(!b_plus) break;
							p1 = p2;
						} else {
							b_plus = false;
							break;
						}
					}
				} else if(u1 && !p1 && u1->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u1)->mixWithBase()) {
					b_plus = true;
					for(size_t i2 = 1; i2 < mstruct->size(); i2++) {
						if(!(*mstruct)[i2].inParentheses() && (*mstruct)[i2].isMultiplication() && (*mstruct)[i2].size() == 2 && (*mstruct)[i2][0].isNumber() && (*mstruct)[i2][1].isUnit() && u1->isChildOf((*mstruct)[i2][1].unit()) && !(*mstruct)[i2][1].prefix() && (i2 == mstruct->size() - 1 || ((*mstruct)[i2][1].unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) (*mstruct)[i2][1].unit())->mixWithBase()))) {
							while(((AliasUnit*) u1)->firstBaseUnit() != (*mstruct)[i2][1].unit()) {
								u1 = ((AliasUnit*) u1)->firstBaseUnit();
								if(u1->subtype() != SUBTYPE_ALIAS_UNIT || !((AliasUnit*) u1)->mixWithBase()) {
									b_plus = false;
									break;
								}
							}
							if(!b_plus) break;
							u1 = (*mstruct)[i2][1].unit();
						} else {
							b_plus = false;
							break;
						}
					}
				}
				if(b_plus) mstruct->setType(STRUCT_ADDITION);
			}
			if(po.preserve_format) {
				while(minus_count > 0) {
					mstruct->transform(STRUCT_NEGATE);
					minus_count--;
				}
			} else if(minus_count % 2 == 1) {
				mstruct->negate();
			}
			return true;
		}
	}
	// Implicit multiplication
	if((i = str.find(ID_WRAP_LEFT_CH, 1)) != string::npos) {
		bool b = false, append = false;
		while(i != string::npos) {
			if(str[i - 1] != POWER_CH && (i < 2 || str[i - 1] != MINUS_CH || str[i - 2] != POWER_CH) && str[i - 1] != '\b') {
				str2 = str.substr(0, i);
				str = str.substr(i, str.length() - i);
				if(b) {
					parseAdd(str2, mstruct, po, OPERATION_MULTIPLY, append);
					append = true;
				} else {
					parseAdd(str2, mstruct, po);
					b = true;
				}
				i = str.find(ID_WRAP_LEFT_CH, 1);
			} else {
				i = str.find(ID_WRAP_LEFT_CH, i + 1);
			}
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_MULTIPLY, append);
			if(po.preserve_format) {
				while(minus_count > 0) {
					mstruct->transform(STRUCT_NEGATE);
					minus_count--;
				}
			} else if(minus_count % 2 == 1) {
				mstruct->negate();
			}
			return true;
		}
	}
	if((i = str.find(POWER_CH, 1)) != string::npos && i + 1 != str.length()) {
		// Parse exponentiation (^)
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_RAISE);
	} else if((i = str.find("\b", 1)) != string::npos && i + 1 != str.length()) {
		// Parse uncertainty (using \b as internal single substitution character for +/-)
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		MathStructure *mstruct2 = new MathStructure;
		if(po.read_precision != DONT_READ_PRECISION) {
			ParseOptions po2 = po;
			po2.read_precision = DONT_READ_PRECISION;
			parseAdd(str2, mstruct, po2);
			parseAdd(str, mstruct2, po2);
		} else {
			parseAdd(str2, mstruct, po);
			parseAdd(str, mstruct2, po);
		}
		mstruct->transform(f_uncertainty);
		mstruct->addChild_nocopy(mstruct2);
		mstruct->addChild(m_zero);
	} else if(BASE_2_10 && (i = str.find_first_of(EXPS, 0)) != string::npos && i + 1 != str.length() && str.find("\b") == string::npos) {
		// Parse scientific e-notation
		if(i == 0) {
			mstruct->set(1, 1, 0);
			CALCULATOR->error(false, _("%s interpreted as 10^%s (1%s)"), str.c_str(), str.find_first_not_of(NUMBERS, 1) == string::npos ? str.substr(1, str.length() - 1).c_str() : (string("(") + str.substr(1, str.length() - 1) + string(")")).c_str(), str.c_str(), NULL);
		} else {
			str2 = str.substr(0, i);
			parseAdd(str2, mstruct, po);
		}
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str, mstruct, po, OPERATION_EXP10);
		if(i == 0 && mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[0].isOne()) mstruct->setToChild(2);
	} else if((i = str.find(ID_WRAP_LEFT_CH, 1)) != string::npos && i + 1 != str.length() && str.find(ID_WRAP_RIGHT_CH, i + 1) && str.find_first_not_of(PLUS MINUS, 0) != i) {
		// Implicit multiplication
		str2 = str.substr(0, i);
		str = str.substr(i, str.length() - i);
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_MULTIPLY);
	} else if(str.length() > 0 && str[0] == ID_WRAP_LEFT_CH && (i = str.find(ID_WRAP_RIGHT_CH, 1)) != string::npos && i + 1 != str.length()) {
		// Implicit multiplication
		str2 = str.substr(0, i + 1);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_MULTIPLY);
	} else {
		// Parse as number
		ret = parseNumber(mstruct, str, po);
	}
	if(po.preserve_format) {
		while(minus_count > 0) {
			mstruct->transform(STRUCT_NEGATE);
			minus_count--;
		}
	} else if(minus_count % 2 == 1) {
		mstruct->negate();
	}
	return ret;
}

