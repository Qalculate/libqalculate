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
#include "MathStructure-support.h"
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
	priv->ids_ref[id] = 1;
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
	priv->ids_ref[id] = 1;
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
	priv->ids_ref[id] = 1;
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
	priv->ids_ref[id] = 1;
	priv->id_structs[id] = new MathStructure();
	f_vector->args(str, *priv->id_structs[id], po);
	return id;
}
MathStructure *Calculator::getId(size_t id) {
	if(priv->id_structs.find(id) != priv->id_structs.end()) {
		if(priv->ids_p[id] || priv->ids_ref[id] > 1) {
			if(!priv->ids_p[id]) priv->ids_ref[id]--;
			return new MathStructure(*priv->id_structs[id]);
		} else {
			MathStructure *mstruct = priv->id_structs[id];
			priv->freed_ids.push_back(id);
			priv->id_structs.erase(id);
			priv->ids_p.erase(id);
			priv->ids_ref.erase(id);
			return mstruct;
		}
	}
	return NULL;
}

void Calculator::delId(size_t id) {
	unordered_map<size_t, size_t>::iterator it = priv->ids_ref.find(id);
	if(it != priv->ids_ref.end()) {
		if(it->second > 1) {
			it->second--;
		} else {
			priv->freed_ids.push_back(id);
			priv->id_structs[id]->unref();
			priv->id_structs.erase(id);
			priv->ids_p.erase(id);
			priv->ids_ref.erase(it);
		}
	}
}

// case sensitive string comparison; compares whole name with str from str_index to str_index + name_length
size_t compare_name(const string &name, const string &str, const size_t &name_length, const size_t &str_index, int base, size_t ignore_us = 0) {
	if(name_length == 0) return 0;
	if(name[0] != str[str_index]) return 0;
	if(name_length == 1) {
		if((base < 2 || base > 10) && !is_not_number(str[str_index], base)) return 0;
		return name_length;
	}
	size_t ip = 0;
	for(size_t i = 1; i < name_length; i++) {
		if(ignore_us > 0 && name[i + ip] == '_') {ip++; ignore_us--;}
		if(name[i + ip] != str[str_index + i]) return 0;
	}
	// number base uses digits other than 0-9, check that at least one non-digit is used
	if(base < 2 || base > 10) {
		for(size_t i = 0; i < name_length; i++) {
			if(is_not_number(str[str_index + i], base)) return name_length;
		}
		return false;
	}
	return name_length;
}

// case insensitive string comparison; compares whole name with str from str_index to str_index + name_length
size_t compare_name_no_case(const string &name, const string &str, const size_t &name_length, const size_t &str_index, int base, size_t ignore_us = 0) {
	if(name_length == 0) return 0;
	size_t is = str_index;
	size_t ip = 0;
	for(size_t i = 0; i < name_length; i++, is++) {
		if(ignore_us > 0 && name[i + ip] == '_') {ip++; ignore_us--;}
		if(is >= str.length()) return 0;
		if(((signed char) name[i + ip] < 0 && i + 1 < name_length) || ((signed char) str[is] < 0 && is + 1 < str.length())) {
			// assumed Unicode character found
			size_t i2 = 1, is2 = 1;
			// determine length of Unicode character(s)
			if((signed char) name[i + ip] < 0) {
				while(i2 + i < name_length && (signed char) name[i2 + i + ip] < 0) {
					i2++;
				}
			}
			if((signed char) str[is] < 0) {
				while(is2 + is < str.length() && (signed char) str[is2 + is] < 0) {
					is2++;
				}
			}
			// compare characters
			bool isequal = (i2 == is2);
			if(isequal) {
				for(size_t i3 = 0; i3 < i2; i3++) {
					if(str[is + i3] != name[i + i3 + ip]) {
						isequal = false;
						break;
					}
				}
			}
			// get lower case character and compare again
			if(!isequal) {
				char *gstr1 = utf8_strdown(name.c_str() + (sizeof(char) * (i + ip)), i2);
				char *gstr2 = utf8_strdown(str.c_str() + (sizeof(char) * (is)), is2);
				if(!gstr1 || !gstr2) return 0;
				if(strcmp(gstr1, gstr2) != 0) {free(gstr1); free(gstr2); return 0;}
				free(gstr1); free(gstr2);
			}
			i += i2 - 1;
			is += is2 - 1;
		} else if(name[i + ip] != str[is] && !((name[i + ip] >= 'a' && name[i + ip] <= 'z') && name[i + ip] - 32 == str[is]) && !((name[i + ip] <= 'Z' && name[i + ip] >= 'A') && name[i + ip] + 32 == str[is])) {
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

const char *internal_signs[] = {SIGN_PLUSMINUS, "\b", "+/-", "\b", "⊻", "\a", "∠", "\x1c", "⊼", "\x1d", "⊽", "\x1e", "⊕", "\x1f", "⨯", "\x15", "∥", "\x14"};
#define INTERNAL_UPOW "\x13"
#define INTERNAL_UPOW_CH '\x13'
#define INTERNAL_SIGNS_COUNT 18
#define INTERNAL_NUMBER_CHARS "\b"
#define INTERNAL_OPERATORS "\a\b%\x1c\x1d\x1e\x1f\x14\x15\x16\x17\x18\x19\x1a\x13"
#define INTERNAL_OPERATORS_TWO "\a\b%\x1c\x1d\x1e\x1f\x14\x15\x16\x17\x18\x19\x13"
#define INTERNAL_OPERATORS_NOPM "\a%\x1c\x1d\x1e\x1f\x14\x15\x16\x17\x18\x19\x1a\x13"
#define INTERNAL_OPERATORS_RPN "\a%\x1c\x1d\x1e\x1f\x14\x15\x16\x17\x18\x19\x1a"
#define INTERNAL_OPERATORS_NOMOD "\a\b\x1c\x1d\x1e\x1f\x14\x15\x16\x17\x18\x19\x1a\x13"
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
			if((signed char) str[ui + 2] >= -104 && (signed char) str[ui + 2] <= -101) {
				// single quotation marks
				str.replace(ui, 3, "\'");
				b_single = true;
			} else if((signed char) str[ui + 2] >= -100 && (signed char) str[ui + 2] <= -97) {
				// double quotation marks
				str.replace(ui, 3, "\"");
				b_double = true;
			} else if((signed char) str[ui + 2] == -70 || (signed char) str[ui + 2] == -71) {
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
		if(quote_index > 1 && str[quote_index] == '\'' && str[quote_index - 1] == '.' && is_not_in(INTERNAL_OPERATORS OPERATORS "\\" NUMBERS, str[quote_index - 2])) {
			if(convert_to_internal_representation) {
				str.replace(quote_index - 1, 2, "\x1a");
			} else {
				quote_index++;
			}
		} else {
			q_begin.push_back(quote_index);
			quote_index = str.find(str[quote_index], quote_index + 1);
			if(quote_index == string::npos) {
				q_end.push_back(str.length() - 1);
				break;
			} else {
				q_end.push_back(quote_index);
			}
			quote_index++;
		}
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
				// adjust quotation mark indices
				int index_shift = real_signs[i].length() - signs[i].length();
				for(size_t ui3 = ui2; ui3 < q_begin.size(); ui3++) {
					q_begin[ui3] += index_shift;
					q_end[ui3] += index_shift;
				}
				str.replace(ui, signs[i].length(), real_signs[i]);
				ui = str.find(signs[i], ui + real_signs[i].length());
				if(!b_unicode && (signed char) real_signs[i][0] < 0) b_unicode = true;
			}
		}
	}

	if(b_unicode) {

		// replace thin space
		size_t ui = str.find(THIN_SPACE);
		size_t ui2 = 0;
		while(ui != string::npos) {
			// check that found index is outside quotes
			for(; ui2 < q_end.size(); ui2++) {
				if(ui >= q_begin[ui2]) {
					if(ui <= q_end[ui2]) {
						ui = str.find(THIN_SPACE, q_end[ui2] + 1);
						if(ui == string::npos) break;
					}
				} else {
					break;
				}
			}
			if(ui == string::npos) break;
			bool b_erase = (ui > 0 && str[ui - 1] >= '0' && str[ui - 1] <= '9' && ui < str.length() - 1 && str[ui + 1] >= '0' && str[ui + 1] <= '9');
			// adjust quotation mark indices
			int index_shift = (b_erase ? 3 : 2);
			for(size_t ui3 = ui2; ui3 < q_begin.size(); ui3++) {
				q_begin[ui3] += index_shift;
				q_end[ui3] += index_shift;
			}
			if(b_erase) str.erase(ui, 3);
			else str.replace(ui, 3, SPACE);
			ui = str.find(THIN_SPACE, ui + (b_erase ? 0 : 1));
		}

		// replace Unicode exponents
		size_t prev_ui = string::npos, space_n = 0;
		while(true) {
			// Unicode powers 0 and 4-9 use three chars and begin with \xe2\x81
			size_t ui = str.find("\xe2\x81", prev_ui == string::npos ? 0 : prev_ui);
			while(ui != string::npos && (ui == str.length() - 2 || ((signed char) str[ui + 2] != -80 && ((signed char) str[ui + 2] < -76 || (signed char) str[ui + 2] > -66 || (signed char) str[ui + 2] == -68)))) ui = str.find("\xe2\x81", ui + 3);
			// Unicode powers 1-3 use two chars and begin with \xc2
			size_t ui2 = str.find('\xc2', prev_ui == string::npos ? 0 : prev_ui);
			while(ui2 != string::npos && (ui2 == str.length() - 1 || ((signed char) str[ui2 + 1] != -71 && (signed char) str[ui2 + 1] != -77 && (signed char) str[ui2 + 1] != -78))) ui2 = str.find('\xc2', ui2 + 2);
			if(ui2 != string::npos && (ui == string::npos || ui2 < ui)) ui = ui2;
			if(ui != string::npos) {
				// check that found index is outside quotes
				for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
					if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
						ui = str.find("\xe2\x81", q_end[ui3] + 1);
						if(ui != string::npos && (ui == str.length() - 2 || ((signed char) str[ui + 2] != -80 && ((signed char) str[ui + 2] < -76 || (signed char) str[ui + 2] > -66 || (signed char) str[ui + 2] != -68)))) ui = string::npos;
						ui2 = str.find('\xc2', q_end[ui3] + 1);
						if(ui2 != string::npos && (ui2 == str.length() - 1 || ((signed char) str[ui2 + 1] != -71 && (signed char) str[ui2 + 1] != -77 && (signed char) str[ui2 + 1] != -78))) ui2 = string::npos;
						if(ui2 != string::npos && (ui == string::npos || ui2 < ui)) ui = ui2;
						if(ui == string::npos) break;
					}
				}
			}
			if(ui == string::npos) break;
			int index_shift = (str[ui] == '\xc2' ? -2 : -3);
			if(ui == prev_ui) index_shift += 1;
			else index_shift += 4;
			// adjust quotation mark indices
			for(size_t ui3 = 0; ui3 < q_begin.size(); ui3++) {
				if(q_begin[ui3] >= ui) {
					q_begin[ui3] += index_shift;
					q_end[ui3] += index_shift;
				}
			}
#define PS_UPOW(x) (convert_to_internal_representation ? INTERNAL_UPOW x : POWER x)
			// perform replacement; if next to previous Unicode power combine the powers
			if(str[ui] == '\xc2') {
				if((signed char) str[ui + 1] == -71) str.replace(ui, 2, ui == prev_ui ? "1)" : PS_UPOW("(1)"));
				else if((signed char) str[ui + 1] == -78) str.replace(ui, 2, ui == prev_ui ? "2)" : PS_UPOW("(2)"));
				else if((signed char) str[ui + 1] == -77) str.replace(ui, 2, ui == prev_ui ? "3)" : PS_UPOW("(3)"));
			} else {
				if((signed char) str[ui + 2] == -80) str.replace(ui, 3, ui == prev_ui ? "0)" : PS_UPOW("(0)"));
				else if((signed char) str[ui + 2] == -76) str.replace(ui, 3, ui == prev_ui ? "4)" : PS_UPOW("(4)"));
				else if((signed char) str[ui + 2] == -75) str.replace(ui, 3, ui == prev_ui ? "5)" : PS_UPOW("(5)"));
				else if((signed char) str[ui + 2] == -74) str.replace(ui, 3, ui == prev_ui ? "6)" : PS_UPOW("(6)"));
				else if((signed char) str[ui + 2] == -73) str.replace(ui, 3, ui == prev_ui ? "7)" : PS_UPOW("(7)"));
				else if((signed char) str[ui + 2] == -72) str.replace(ui, 3, ui == prev_ui ? "8)" : PS_UPOW("(8)"));
				else if((signed char) str[ui + 2] == -71) str.replace(ui, 3, ui == prev_ui ? "9)" : PS_UPOW("(9)"));
				else if((signed char) str[ui + 2] == -70) str.replace(ui, 3, ui == prev_ui ? "+)" : PS_UPOW("(+)"));
				else if((signed char) str[ui + 2] == -69) str.replace(ui, 3, ui == prev_ui ? "-)" : PS_UPOW("(-)"));
				else if((signed char) str[ui + 2] == -67) str.replace(ui, 3, ui == prev_ui ? "()" : PS_UPOW("(()"));
				else if((signed char) str[ui + 2] == -66) str.replace(ui, 3, ui == prev_ui ? "))" : PS_UPOW("())"));
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
			while(ui != string::npos && (ui == str.length() - 2 || (signed char) str[ui + 2] < -112 || (signed char) str[ui + 2] > -98)) ui = str.find("\xe2\x85", ui + 3);
			if(ui != string::npos) {
				// check that found index is outside quotes
				for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
					if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
						ui = str.find("\xe2\x85", q_end[ui3] + 1);
						if(ui != string::npos && (ui == str.length() - 2 || (signed char) str[ui + 2] < -112 || (signed char) str[ui + 2] > -98)) ui = string::npos;
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
			if((signed char) str[ui + 2] == -110) index_shift++;
			// adjust quotation mark indices
			for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
				if(q_begin[ui2] >= ui) {
					q_begin[ui2] += index_shift;
					q_end[ui2] += index_shift;
				}
			}
			// perform replacement; interpret as addition if previous character is a numeral digit
			if((signed char) str[ui + 2] == -98) str.replace(ui, 3, b_add ? "+(7/8))" : "(7/8)");
			else if((signed char) str[ui + 2] == -99) str.replace(ui, 3, b_add ? "+(5/8))" : "(5/8)");
			else if((signed char) str[ui + 2] == -100) str.replace(ui, 3, b_add ? "+(3/8))" : "(3/8)");
			else if((signed char) str[ui + 2] == -101) str.replace(ui, 3, b_add ? "+(1/8))" : "(1/8)");
			else if((signed char) str[ui + 2] == -102) str.replace(ui, 3, b_add ? "+(5/6))" : "(5/6)");
			else if((signed char) str[ui + 2] == -103) str.replace(ui, 3, b_add ? "+(1/6))" : "(1/6)");
			else if((signed char) str[ui + 2] == -104) str.replace(ui, 3, b_add ? "+(4/5))" : "(4/5)");
			else if((signed char) str[ui + 2] == -105) str.replace(ui, 3, b_add ? "+(3/5))" : "(3/5)");
			else if((signed char) str[ui + 2] == -106) str.replace(ui, 3, b_add ? "+(2/5))" : "(2/5)");
			else if((signed char) str[ui + 2] == -107) str.replace(ui, 3, b_add ? "+(1/5))" : "(1/5)");
			else if((signed char) str[ui + 2] == -108) str.replace(ui, 3, b_add ? "+(2/3))" : "(2/3)");
			else if((signed char) str[ui + 2] == -109) str.replace(ui, 3, b_add ? "+(1/3))" : "(1/3)");
			else if((signed char) str[ui + 2] == -110) {str.replace(ui, 3, b_add ? "+(1/10))" : "(1/10)"); ui++;}
			else if((signed char) str[ui + 2] == -111) str.replace(ui, 3, b_add ? "+(1/9))" : "(1/9)");
			else if((signed char) str[ui + 2] == -112) str.replace(ui, 3, b_add ? "+(1/7))" : "(1/7)");
			if(b_add) prev_ui = ui + 7;
			else prev_ui = ui + 5;
		}

		// replace Unicode fractions with two chars
		prev_ui = string::npos;
		while(true) {
			// two char Unicode fractions begin with \xc2
			size_t ui = str.find('\xc2', prev_ui == string::npos ? 0 : prev_ui);
			while(ui != string::npos && (ui == str.length() - 1 || ((signed char) str[ui + 1] != -66 && (signed char) str[ui + 1] != -67 && (signed char) str[ui + 1] != -68))) ui = str.find('\xc2', ui + 2);
			if(ui != string::npos) {
				// check that found index is outside quotes
				for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
					if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
						ui = str.find('\xc2', q_end[ui3] + 1);
						if(ui != string::npos && (ui == str.length() - 1 || ((signed char) str[ui + 1] != -66 && (signed char) str[ui + 1] != -67 && (signed char) str[ui + 1] != -68))) ui = string::npos;
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
			// adjust quotation mark indices
			for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
				if(q_begin[ui2] >= ui) {
					q_begin[ui2] += index_shift;
					q_end[ui2] += index_shift;
				}
			}
			// perform replacement; interpret as addition if previous character is a numeral digit
			if((signed char) str[ui + 1] == -66) str.replace(ui, 2, b_add ? "+(3/4))" : "(3/4)");
			else if((signed char) str[ui + 1] == -67) str.replace(ui, 2, b_add ? "+(1/2))" : "(1/2)");
			else if((signed char) str[ui + 1] == -68) str.replace(ui, 2, b_add ? "+(1/4))" : "(1/4)");
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
					// adjust quotation mark indices
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

	// replace semicolon outside matrices
	if(str.find(";") != string::npos) {
		bool in_cit1 = false, in_cit2 = false;
		vector<int> pars;
		int brackets = 0;
		for(size_t i = 0; i < str.length(); i++) {
			switch(str[i]) {
				case LEFT_VECTOR_WRAP_CH: {
					if(!in_cit1 && !in_cit2) {
						brackets++;
						pars.push_back(0);
					}
					break;
				}
				case LEFT_PARENTHESIS_CH: {
					if(brackets > 0 && !in_cit1 && !in_cit2) pars[brackets - 1]++;
					break;
				}
				case RIGHT_PARENTHESIS_CH: {
					if(brackets > 0 && !in_cit1 && !in_cit2 && pars[brackets - 1] > 0) pars[brackets - 1]--;
					break;
				}
				case '\"': {
					if(in_cit1) in_cit1 = false;
					else if(!in_cit2) in_cit1 = true;
					break;
				}
				case '\'': {
					if(in_cit2) in_cit2 = false;
					else if(!in_cit1) in_cit1 = true;
					break;
				}
				case RIGHT_VECTOR_WRAP_CH: {
					if(!in_cit1 && !in_cit2 && brackets > 0) {
						brackets--;
						pars.pop_back();
					}
					break;
				}
				case ';': {
					if((brackets == 0 || pars[brackets - 1] > 0) && !in_cit1 && !in_cit2) {
						str[i] = COMMA_CH;
					}
					break;
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

string internal_operator_replacement(char c) {
	switch(c) {
		case '\a': return "xor";
		case '\b': return SIGN_PLUSMINUS;
		case '\x1c': return "∠";
		case '\x1d': return "nand";
		case '\x1e': return "nor";
		case '\x1f': return "xor";
		case '\x13': return POWER;
		case '\x14': return "∥";
		case '\x15': return "cross";
		case '\x16': return DOT;
		case '\x17': return string(DOT) + internal_operator_replacement(MULTIPLICATION_CH);
		case '\x18': return string(DOT) + internal_operator_replacement(DIVISION_CH);
		case '\x19': return DOT POWER;
		case '\x1a': return DOT "\'";
		case MULTIPLICATION_CH: {
			if(CALCULATOR->messagePrintOptions().use_unicode_signs && CALCULATOR->messagePrintOptions().multiplication_sign == MULTIPLICATION_SIGN_DOT && (!CALCULATOR->messagePrintOptions().can_display_unicode_string_function || (*CALCULATOR->messagePrintOptions().can_display_unicode_string_function) (SIGN_MULTIDOT, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg))) return SIGN_MULTIDOT;
			else if(CALCULATOR->messagePrintOptions().use_unicode_signs && (CALCULATOR->messagePrintOptions().multiplication_sign == MULTIPLICATION_SIGN_DOT || CALCULATOR->messagePrintOptions().multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!CALCULATOR->messagePrintOptions().can_display_unicode_string_function || (*CALCULATOR->messagePrintOptions().can_display_unicode_string_function) (SIGN_MIDDLEDOT, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg))) return SIGN_MIDDLEDOT;
			else if(CALCULATOR->messagePrintOptions().use_unicode_signs && CALCULATOR->messagePrintOptions().multiplication_sign == MULTIPLICATION_SIGN_X && (!CALCULATOR->messagePrintOptions().can_display_unicode_string_function || (*CALCULATOR->messagePrintOptions().can_display_unicode_string_function) (SIGN_MULTIPLICATION, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg))) return SIGN_MULTIPLICATION;
			break;
		}
		case DIVISION_CH: {
			if(CALCULATOR->messagePrintOptions().use_unicode_signs && CALCULATOR->messagePrintOptions().division_sign == DIVISION_SIGN_DIVISION && (!CALCULATOR->messagePrintOptions().can_display_unicode_string_function || (*CALCULATOR->messagePrintOptions().can_display_unicode_string_function) (SIGN_DIVISION, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg))) return SIGN_DIVISION;
			break;
		}
		case MINUS_CH: {
			if(CALCULATOR->messagePrintOptions().use_unicode_signs && (!CALCULATOR->messagePrintOptions().can_display_unicode_string_function || (*CALCULATOR->messagePrintOptions().can_display_unicode_string_function) (SIGN_MINUS, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg))) return SIGN_MINUS;
			break;
		}
	}
	string str; str += c;
	return str;
}
void replace_internal_operators(string &str) {
	bool prev_s = true;
	for(size_t i = 0; i < str.length(); i++) {
		if(str[i] == '\a' || str[i] == '\x1d' || str[i] == '\x1e' || str[i] == '\x1f' || str[i] == '\x15') {
			if(prev_s && i + 1 == str.length()) str.replace(i, 1, internal_operator_replacement(str[i]));
			else if(prev_s) str.replace(i, 1, internal_operator_replacement(str[i]) + SPACE);
			else if(i + 1 == str.length()) str.replace(i, 1, string(SPACE) + internal_operator_replacement(str[i]));
			else str.replace(i, 1, string(SPACE) + internal_operator_replacement(str[i]) + SPACE);
			prev_s = true;
		} else if(str[i] == '\b' || str[i] == '\x13' || str[i] == '\x14' || str[i] == '\x1c' || str[i] >= '\x16' || str[i] <= '\x1a' || str[i] == MULTIPLICATION_CH || str[i] == MINUS_CH || str[i] == DIVISION_CH) {
			str.replace(i, 1, internal_operator_replacement(str[i]));
			prev_s = false;
		} else {
			prev_s = (str[i] == SPACE_CH);
		}
	}
}

bool has_boolean_variable(const MathStructure &m) {
	if(m.isVariable()) return !m.variable()->isKnown() && ((UnknownVariable*) m.variable())->representsBoolean();
	else if(m.isSymbolic()) return m.representsBoolean();
	for(size_t i = 0; i < m.size(); i++) {
		if(has_boolean_variable(m[i])) return true;
	}
	return false;
}
bool is_boolean_algebra_expression2(const MathStructure &m, bool *bitfound = NULL) {
	if(!bitfound) {
		bool bitf = false;
		return is_boolean_algebra_expression2(m, &bitf) && bitf;
	}
	if(!(*bitfound) && (m.type() == STRUCT_BITWISE_AND || m.type() == STRUCT_BITWISE_OR)) *bitfound = true;
	if(m.isUnknown()) return true;
	if(m.size() > 0) {
		if(m.type() < STRUCT_BITWISE_AND || m.type() >= STRUCT_COMPARISON) return false;
		for(size_t i = 0; i < m.size(); i++) {
			if(!is_boolean_algebra_expression2(m[i], bitfound)) return false;
		}
	} else if(!m.representsBoolean()) {
		return false;
	}
	return true;
}
bool is_boolean_algebra_expression3(const MathStructure &m, bool *bitfound = NULL) {
	if(!bitfound) {
		bool bitf = false;
		return is_boolean_algebra_expression3(m, &bitf) && bitf;
	}
	if(m.isUnknown()) return true;
	if(m.size() > 0) {
		if(m.type() == STRUCT_LOGICAL_AND || m.type() == STRUCT_LOGICAL_OR || m.type() == STRUCT_LOGICAL_XOR) *bitfound = true;
		else if(m.type() != STRUCT_LOGICAL_NOT && m.type() != STRUCT_BITWISE_NOT && m.type() != STRUCT_BITWISE_XOR) return false;
		for(size_t i = 0; i < m.size(); i++) {
			if(!is_boolean_algebra_expression3(m[i], bitfound)) return false;
		}
	} else if(!m.representsBoolean()) {
		return false;
	}
	return true;
}
bool is_boolean_algebra_expression(const MathStructure &m, int ascii_bitwise, bool top = true) {
	if(top && !has_boolean_variable(m)) {
		if(!ascii_bitwise && is_boolean_algebra_expression2(m)) return true;
		if(ascii_bitwise != 1 && is_boolean_algebra_expression3(m)) return true;
		return false;
	}
	if(m.size() == 0 && !m.representsBoolean()) return false;
	if(m.size() > 0 && (m.type() < STRUCT_BITWISE_AND || m.type() > STRUCT_COMPARISON)) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(!is_boolean_algebra_expression(m[i], false, false)) return false;
	}
	return true;
}
void bitwise_to_logical(MathStructure &m) {
	if(m.isBitwiseOr()) m.setType(STRUCT_LOGICAL_OR);
	else if(m.isBitwiseXor()) m.setType(STRUCT_LOGICAL_XOR);
	else if(m.isBitwiseAnd()) m.setType(STRUCT_LOGICAL_AND);
	else if(m.isBitwiseNot()) m.setType(STRUCT_LOGICAL_NOT);
	for(size_t i = 0; i < m.size(); i++) {
		bitwise_to_logical(m[i]);
	}
}

string Calculator::localizeExpression(string str, const ParseOptions &po) const {
	if((DOT_STR == DOT && COMMA_STR == COMMA && !po.comma_as_separator) || po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return str;
	int base = po.base;
	if(base == BASE_CUSTOM) {
		base = (int) priv->custom_input_base_i;
	} else if(base == BASE_BIJECTIVE_26) {
		base = 36;
	} else if(base == BASE_GOLDEN_RATIO || base == BASE_SUPER_GOLDEN_RATIO || base == BASE_SQRT2 || base == BASE_BINARY_DECIMAL) {
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
	bool in_cit1 = false, in_cit2 = false;
	vector<int> pars;
	vector<size_t> espace;
	vector<size_t> epos;
	int brackets = 0;
	int comma_type = 0;
	if(COMMA_STR == ";" || (po.comma_as_separator && COMMA_STR == COMMA)) comma_type = 1;
	else if(COMMA_STR != COMMA) comma_type = 2;
	int dot_type = 0;
	if(DOT_STR == COMMA) dot_type = 1;
	else if(DOT_STR != DOT) comma_type = 2;
	for(size_t i = 0; i < str.length(); i++) {
		switch(str[i]) {
			case LEFT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2) {
					brackets++;
					pars.push_back(0);
					epos.push_back(0);
					espace.push_back(false);
				}
				break;
			}
			case RIGHT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0) {
					if(epos[brackets - 1] > 0 && espace[brackets - 1] > 0) {
						str.insert(epos[brackets - 1], LEFT_PARENTHESIS); i++;
						str.insert(i, RIGHT_PARENTHESIS); i++;
					}
					brackets--;
					pars.pop_back();
					epos.pop_back();
					espace.pop_back();
				}
				break;
			}
			case LEFT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0) pars[brackets - 1]++;
				break;
			}
			case RIGHT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0 && pars[brackets - 1] > 0) pars[brackets - 1]--;
				break;
			}
			case '\"': {
				if(in_cit1) in_cit1 = false;
				else if(!in_cit2) in_cit1 = true;
				break;
			}
			case '\'': {
				if(in_cit2) in_cit2 = false;
				else if(!in_cit1) in_cit1 = true;
				break;
			}
			case '\n': {}
			case '\t': {}
			case SPACE_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0 && pars[brackets - 1] == 0 && epos[brackets - 1] > 0 && espace[brackets - 1] == 0) {
					espace[brackets - 1] = i;
				}
				break;
			}
			case COMMA_CH: {
				if(!in_cit1 && !in_cit2 && comma_type != 0) {
					if(priv->matlab_matrices && brackets > 0 && pars[brackets - 1] == 0) {
						if(epos[brackets - 1] > 0 && espace[brackets - 1] > 0) {
							str.insert(epos[brackets - 1], LEFT_PARENTHESIS); i++;
							size_t i2 = str.find_last_not_of(SPACES, i - 1);
							str.insert(i2 + 1, RIGHT_PARENTHESIS); i++;
						}
						if(dot_type == 1 && str[i - 1] != RIGHT_VECTOR_WRAP_CH) {
							if(i < str.length() - 1 && is_in(SPACES, str[i + 1])) {
								str.erase(i, 1); i--;
							} else {
								str[i] = SPACE_CH;
							}
							i = str.find_first_not_of(SPACES, i + 1);
							if(i == string::npos) return str;
							epos[brackets - 1] = i;
							espace[brackets - 1] = 0;
							i--;
						}
					} else {
						if(comma_type == 1) {
							str[i] = ';';
						} else {
							str.replace(i, 1, COMMA_STR);
							i += COMMA_STR.length() - 1;
						}
					}
				}
				break;
			}
			case DOT_CH: {
				if(!in_cit1 && !in_cit2 && dot_type != 0) {
					if(po.rpn || i == 0 || i == str.length() - 1 || is_not_number(str[i - 1], base) || is_not_number(str[i + 1], base) || is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i - 1]) || (str[i + 1] != '\'' && str[i + 1] != POWER_CH && str[i + 1] != MULTIPLICATION_CH && str[i + 1] != DIVISION_CH && is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i + 1]))) {
						if(dot_type == 1) {
							str[i] = COMMA_CH;
						} else {
							str.replace(i, 1, DOT_STR);
							i += DOT_STR.length() - 1;
						}
					}
				}
				break;
			}
		}
	}
	return str;
}
string Calculator::unlocalizeExpression(string str, const ParseOptions &po) const {
	if((DOT_STR == DOT && COMMA_STR == COMMA && !po.comma_as_separator) || po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return str;
	int base = po.base;
	if(base == BASE_CUSTOM) {
		base = (int) priv->custom_input_base_i;
	} else if(base == BASE_BIJECTIVE_26) {
		base = 36;
	} else if(base == BASE_GOLDEN_RATIO || base == BASE_SUPER_GOLDEN_RATIO || base == BASE_SQRT2 || base == BASE_BINARY_DECIMAL) {
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
	bool in_cit1 = false, in_cit2 = false;
	vector<int> pars;
	int brackets = 0;
	int comma_type = 0;
	if(COMMA_STR == ";") comma_type = 1;
	else if(COMMA_STR != COMMA) comma_type = 2;
	int dot_type = 0;
	if(DOT_STR == COMMA) dot_type = 1;
	else if(DOT_STR != DOT) comma_type = 2;
	bool b_matrix_comma = (dot_type != 1);
	pars.push_back(0);
	for(size_t i = 0; dot_type == 1 && i < str.length(); i++) {
		switch(str[i]) {
			case LEFT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2) {
					brackets++;
					pars.push_back(0);
				}
				break;
			}
			case LEFT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2) pars[brackets]++;
				break;
			}
			case RIGHT_PARENTHESIS_CH: {
				if(!in_cit1 && !in_cit2 && pars[brackets] > 0) pars[brackets]--;
				break;
			}
			case '\"': {
				if(in_cit1) in_cit1 = false;
				else if(!in_cit2) in_cit1 = true;
				break;
			}
			case '\'': {
				if(in_cit2) in_cit2 = false;
				else if(!in_cit1) in_cit1 = true;
				break;
			}
			case RIGHT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0) {
					brackets--;
					pars.pop_back();
				}
				break;
			}
			/*case SPACE_CH: {
				if(!in_cit1 && !in_cit2 && po.comma_as_separator && b_matrix_comma && brackets > 0 && pars[brackets] == 0 && i != str.length() - 1) {
					size_t i2 = str.find_last_not_of(SPACES, i - 1);
					size_t i3 = str.find_first_not_of(SPACES, i + 1);
					if(i2 != string::npos && i3 != string::npos && !is_not_number(str[i2], base) && !is_not_number(str[i3], base)) {
						b_matrix_comma = false;
					}
					break;
				}
			}*/
			case COMMA_CH: {
				if(!in_cit1 && !in_cit2 && ((priv->matlab_matrices && brackets > 0 && !b_matrix_comma) || pars[brackets] > 0)) {
					if(i > 0) {
						size_t i2 = str.find_last_not_of(SPACES, i - 1);
						if(i2 != string::npos && (str[i2] == RIGHT_PARENTHESIS_CH || str[i2] == RIGHT_VECTOR_WRAP_CH || (((str[i2] > 'a' && str[i2] < 'z') || (str[i2] > 'A' && str[i2] < 'Z')) && is_not_number(str[i2], base)))) {
							b_matrix_comma = true;
							if(pars[brackets] > 0) {dot_type = 0; break;}
						}
					}
					if(i != str.length() - 1) {
						size_t i2 = str.find_first_not_of(SPACES, i + 1);
						if(i2 != string::npos) {
							if(is_not_number(str[i2], base)) {
								b_matrix_comma = true;
								if(pars[brackets] > 0) {dot_type = 0; break;}
							} else {
								for(i2 = i2 + 1; i2 < str.length(); i2++) {
									if(str[i2] == COMMA_CH) {
										b_matrix_comma = true;
										if(pars[brackets] > 0) dot_type = 0;
										break;
									} else if(is_not_number(str[i2], base) && (is_not_in(SPACES, str[i2]) || (brackets > 0 && pars[brackets] == 0))) {
										break;
									}
								}
							}
						}
					}
				}
				break;
			}
			case DOT_CH: {
				if(!po.dot_as_separator && brackets > 0 && !b_matrix_comma) {
					if(po.rpn || i == 0 || i == str.length() - 1 || is_not_number(str[i - 1], base) || is_not_number(str[i + 1], base) || is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i - 1]) || (str[i + 1] != '\'' && str[i + 1] != POWER_CH && str[i + 1] != MULTIPLICATION_CH && str[i + 1] != DIVISION_CH && is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i + 1]))) {
						b_matrix_comma = true;
						break;
					}
				}
				break;
			}
		}
	}
	if(!priv->matlab_matrices) b_matrix_comma = false;
	pars.clear();
	in_cit1 = false; in_cit2 = false; brackets = 0;
	for(size_t i = 0; i < str.length();) {
		switch(str[i]) {
			case LEFT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2) {
					brackets++;
					pars.push_back(0);
				}
				break;
			}
			case LEFT_PARENTHESIS_CH: {
				if(brackets > 0 && !in_cit1 && !in_cit2) pars[brackets - 1]++;
				break;
			}
			case RIGHT_PARENTHESIS_CH: {
				if(brackets > 0 && !in_cit1 && !in_cit2 && pars[brackets - 1] > 0) pars[brackets - 1]--;
				break;
			}
			case '\"': {
				if(in_cit1) in_cit1 = false;
				else if(!in_cit2) in_cit1 = true;
				break;
			}
			case '\'': {
				if(in_cit2) in_cit2 = false;
				else if(!in_cit1) in_cit1 = true;
				break;
			}
			case RIGHT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0) {
					brackets--;
					pars.pop_back();
				}
				break;
			}
			case COMMA_CH: {
				if(!in_cit1 && !in_cit2 && (dot_type == 1 || po.comma_as_separator) && (!b_matrix_comma || brackets == 0 || pars[brackets - 1] > 0)) {
					if(dot_type == 1) str[i] = DOT_CH;
					else {str.erase(i, 1); continue;}
				}
				break;
			}
			case ';': {
				if((comma_type == 1 || po.comma_as_separator) && (!priv->matlab_matrices || brackets == 0 || pars[brackets - 1] > 0) && !in_cit1 && !in_cit2) {
					str[i] = COMMA_CH;
				}
				break;
			}
			case DOT_CH: {
				if((dot_type != 0 && po.dot_as_separator) && !in_cit1 && !in_cit2) {
					if(po.rpn || i == 0 || i == str.length() - 1 || is_not_number(str[i - 1], base) || is_not_number(str[i + 1], base) || is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i - 1]) || (str[i + 1] != '\'' && str[i + 1] != POWER_CH && str[i + 1] != MULTIPLICATION_CH && str[i + 1] != DIVISION_CH && is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i + 1]))) {
						str.erase(i, 1);
						continue;
					}
				}
				break;
			}
			default: {
				if(dot_type == 2 && !in_cit1 && !in_cit2 && str[i] == DOT_STR[0] && i + DOT_STR.length() <= str.length()) {
					bool b = true;
					for(size_t i2 = 1; i2 < DOT_STR.length(); i2++) {
						if(str[i + i2] != DOT_STR[i2]) {b = false; break;}
					}
					if(b) {str.replace(i, DOT_STR.length(), DOT); i += DOT_STR.length() - 1;}
				}
				if(comma_type == 2 && !in_cit1 && !in_cit2 && str[i] == COMMA_STR[0] && i + COMMA_STR.length() <= str.length()) {
					bool b = true;
					for(size_t i2 = 1; i2 < COMMA_STR.length(); i2++) {
						if(str[i + i2] != COMMA_STR[i2]) {b = false; break;}
					}
					if(b) {str.replace(i, COMMA_STR.length(), COMMA); i += COMMA_STR.length() - 1;}
				}
				break;
			}
		}
		i++;
	}
	return str;
}

bool contains_parallel(const MathStructure &m) {
	if(m.isLogicalOr()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].containsType(STRUCT_UNIT, false, true, true) <= 0) return false;
			if(m[i].representsBoolean() && (!m[i].isLogicalOr() || !contains_parallel(m[i]))) return false;
		}
		return true;
	}
	if(m.representsBoolean()) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_parallel(m[i])) return true;
	}
	return false;
}

bool first_is_unit(const MathStructure &m) {
	if(m.isUnit()) return true;
	if(m.size() == 0 || m.isNegate()) return false;
	return first_is_unit(m[0]);
}
MathStructure *last_is_function(MathStructure &m) {
	if(m.isFunction() && m.size() == 0 && m.function()->minargs() == 1) return &m;
	if(m.size() == 0) return NULL;
	return last_is_function(m.last());
}

#define PARSING_MODE (po.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT)
#define BASE_2_10 ((po.base >= 2 && po.base <= 10) || (po.base < BASE_CUSTOM && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26) || (po.base == BASE_CUSTOM && priv->custom_input_base_i <= 10))

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

	if(po.rpn) {
		if(po.parsing_mode & PARSE_PERCENT_AS_ORDINARY_CONSTANT) po.parsing_mode = (ParsingMode) (PARSING_MODE_RPN | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
		else po.parsing_mode = PARSING_MODE_RPN;
		po.rpn = false;
	}

	MathStructure *unended_function = po.unended_function;
	po.unended_function = NULL;

	// use parse option number base to determine which characters are used as numerical digits and set base accordingly.
	// (-1=base using digits other than 0-9, a-z, A-Z; -12=duodecimal)
	int base = po.base;
	if(base == BASE_CUSTOM) {
		base = (int) priv->custom_input_base_i;
	} else if(base == BASE_GOLDEN_RATIO || base == BASE_SUPER_GOLDEN_RATIO || base == BASE_SQRT2 || base == BASE_BINARY_DECIMAL) {
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

	// search for degree sign in expressions (affects interpretation of ' and ")
	size_t i_degree = str.find(SIGN_DEGREE);
	if(i_degree != string::npos && i_degree + strlen(SIGN_DEGREE) < str.length() && is_in("CRF", str[i_degree + strlen(SIGN_DEGREE)])) {
		i_degree = string::npos;
	}

	if(base != -1 && base <= BASE_HEXADECIMAL && po.units_enabled) {
		// replace single ' and " with prime and double prime (for ft/in or minutes/seconds of arc)
		size_t i_quote2 = str.find('\'', 0);
		size_t i_dquote = str.find('\"', 0);
		size_t i_quote = string::npos;
		bool b_prime_quote = true;
		while(i_quote2 != string::npos) {
			if(i_quote2 > 1 && i_degree == string::npos && str[i_quote2 - 1] == DOT_CH && is_not_number(str[i_quote2 - 2], base) && is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[i_quote2 - 2])) {
				b_prime_quote = false;
				break;
			} else if(b_prime_quote && i_degree == string::npos && i_quote2 > 0 && str[i_quote2 - 1] == '(') {
				b_prime_quote = false;
				break;
			} else if(i_quote == string::npos) {
				i_quote = i_quote2;
			}
			i_quote2 = str.find('\'', i_quote2 + 1);
		}
		if(b_prime_quote && i_degree == string::npos) {
			i_quote2 = i_dquote;
			while(i_quote2 != string::npos) {
				if(i_quote2 > 0 && str[i_quote2 - 1] == '(') {
					b_prime_quote = false;
					break;
				}
				i_quote2 = str.find('\"', i_quote2 + 1);
			}
		}
		b_prime_quote = b_prime_quote && (i_quote != string::npos || i_dquote != string::npos);
		if(b_prime_quote && i_degree == string::npos) {
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

	int ascii_bitwise = 0;
	if(str.find_first_of(BITWISE_AND BITWISE_OR LOGICAL_NOT) != string::npos) ascii_bitwise = 1;

	bool test_or_parallel = (str.find("||") != string::npos);

	// replace alternative strings (primarily operators) with default ascii versions
	parseSigns(str, true);

	if(test_or_parallel) test_or_parallel = (str.find("&&") == string::npos && str.find('\x14') == string::npos);

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
	if((isave = str.find(":=", 1)) != string::npos || (isave = str.find("=:", 1)) != string::npos) {
		string name = str.substr(0, isave);
		remove_blank_ends(name);
		replace_internal_operators(name);
		string value = str.substr(isave + 2, str.length() - (isave + 2));
		remove_blank_ends(value);
		MathStructure mvalue;
		MathStructure mtmp(CALCULATOR->temporaryCategory(), true);
		MathStructure mempty(string(""), true);
		MathStructure mname;
		po.unended_function = unended_function;
		parse(&mvalue, value, po);
		if(f_save->getArgumentDefinition(2)) f_save->getArgumentDefinition(2)->parse(&mname, name, po);
		else mname.set(name, true);
		mstruct->set(f_save, &mvalue, &mname, &mtmp, &mempty, str[isave] == '=' ? &m_one : &m_zero, NULL);
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
	if(PARSING_MODE != PARSING_MODE_RPN && !str.empty()) {
		bool in_cit1 = false, in_cit2 = false;
		int brackets = 0;
		for(size_t i = 0; i < str.length() - 1; i++) {
			switch(str[i]) {
				case LEFT_VECTOR_WRAP_CH: {
					if(!in_cit1 && !in_cit2) brackets++;
					break;
				}
				case RIGHT_VECTOR_WRAP_CH: {
					if(!in_cit1 && !in_cit2 && brackets > 0) brackets--;
					break;
				}
				case '\"': {
					if(in_cit1) in_cit1 = false;
					else if(!in_cit2) in_cit1 = true;
					break;
				}
				case '\'': {
					if(in_cit2) in_cit2 = false;
					else if(!in_cit1) in_cit1 = true;
					break;
				}
				case SPACE_CH: {
					if(brackets == 0 && !in_cit1 && !in_cit2 && i > 0) {
						if(is_in(NUMBERS INTERNAL_NUMBER_CHARS DOT, str[i + 1]) && is_in(NUMBERS INTERNAL_NUMBER_CHARS DOT, str[i - 1])) {
							str.erase(i, 1);
							i--;
						}
					}
					break;
				}
			}
		}
	}

	if(base != -1 && base <= BASE_HEXADECIMAL && po.units_enabled) {
		// replace prime and double prime with feet and inches, or arcminutes and arcseconds (if degree sign was previously found)
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
							Unit *u = getActiveUnit("arcsec");
							if(!u) break;
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure(u)));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS RIGHT_PARENTHESIS;
							str.replace(i_op, strlen("″"), stmp);
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure(getDegUnit())));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS PLUS;
							str.replace(i_degree, strlen(SIGN_DEGREE), stmp);
							b = true;
						}
					}
				}
				if(!b) {
					if(str.length() >= i_dquote + strlen("″") && is_in(NUMBERS, str[i_dquote + strlen("″")])) str.insert(i_dquote + strlen("″"), " ");
					Unit *u = getActiveUnit(b_degree ? "arcsec" : "in");
					if(!u) break;
					stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
					stmp += i2s(addId(new MathStructure(u)));
					stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
					str.replace(i_dquote, strlen("″"), stmp);
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
									Unit *u = getActiveUnit("arcsec");
									if(!u) break;
									stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
									stmp += i2s(addId(new MathStructure(u)));
									stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS RIGHT_PARENTHESIS;
									str.replace(i_dquote, strlen("″"), stmp);
									i_op = i_op2;
								}
							}
							Unit *u = getActiveUnit("arcmin");
							if(!u) break;
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure(u)));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
							if(i_op == i_quote) str += RIGHT_PARENTHESIS;
							else str += PLUS;
							str.replace(i_quote, strlen("′"), stmp);
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure(getDegUnit())));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS PLUS;
							str.replace(i_degree, strlen(SIGN_DEGREE), stmp);
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
						Unit *u = getActiveUnit(b_degree ? "arcsec" : "in");
						if(!u) break;
						stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
						stmp += i2s(addId(new MathStructure(u)));
						stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS RIGHT_PARENTHESIS;
						if(i_op == string::npos) str += stmp;
						else str.replace(i_op + 1, strlen("″"), stmp);
						u = getActiveUnit(b_degree ? "arcmin" : "ft");
						if(!u) break;
						stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
						stmp += i2s(addId(new MathStructure(u)));
						stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS PLUS;
						str.replace(i_quote, strlen("′"), stmp);
						if(i_op == string::npos) break;
						i_op++;
					} else {
						if(str.length() >= i_quote + strlen("′") && is_in(NUMBERS, str[i_quote + strlen("′")])) str.insert(i_quote + strlen("′"), " ");
						Unit *u = getActiveUnit(b_degree ? "arcmin" : "ft");
						if(!u) break;
						stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
						stmp += i2s(addId(new MathStructure(u)));
						stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
						str.replace(i_quote, strlen("′"), stmp);
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
		if(PARSING_MODE == PARSING_MODE_RPN) {
			if(i_mod == 0 || is_not_in(OPERATORS "\\" INTERNAL_OPERATORS SPACE, str[i_mod - 1])) {
				stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
				stmp += i2s(addId(new MathStructure(v_percent)));
				stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				str.replace(i_mod, 1, stmp);
				i_mod += v_percent->referenceName().length() - 1;
			}
		} else {
			size_t i_nonspace = string::npos;
			if(i_mod < str.length() - 1) i_nonspace = str.find_first_not_of(SPACE, i_mod + 1);
			if(i_mod == 0 || i_mod == str.length() - 1 || (str[i_mod - 1] != '%' && str[i_mod + 1] != '%' && ((i_nonspace != string::npos && is_in(RIGHT_PARENTHESIS RIGHT_VECTOR_WRAP COMMAS OPERATORS INTERNAL_OPERATORS, str[i_nonspace]) && str[i_nonspace] != BITWISE_NOT_CH && str[i_nonspace] != NOT_CH && str[i_nonspace] != '%') || is_in(LEFT_PARENTHESIS LEFT_VECTOR_WRAP COMMAS OPERATORS INTERNAL_OPERATORS, str[i_mod - 1])))) {
				stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
				stmp += i2s(addId(new MathStructure(v_percent)));
				stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				str.replace(i_mod, 1, stmp);
				i_mod += v_percent->referenceName().length() - 1;
			}
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

	if(PARSING_MODE == PARSING_MODE_RPN) {
		// add space between double operators in rpn mode in order to ensure that they are interpreted as two single operators
		gsub("&&", "& &", str);
		gsub("||", "| |", str);
		gsub("\%\%", "\% \%", str);
	}

	for(size_t str_index = 0; str_index < str.length(); str_index++) {
		if(str[str_index] == LEFT_VECTOR_WRAP_CH) {
			// vector
			int b_old_matrix = 2;
			bool b_comma = false;
			if(priv->matlab_matrices) {
				b_old_matrix = -1;
				bool in_cit1 = false, in_cit2 = false;
				int pars = 0;
				int brackets = 1;
				for(size_t i = str_index + 1; i < str.length() && brackets > 0 && (b_old_matrix != 0 || !b_comma); i++) {
					switch(str[i]) {
						case LEFT_VECTOR_WRAP_CH: {
							if(!in_cit1 && !in_cit2) brackets++;
							break;
						}
						case RIGHT_VECTOR_WRAP_CH: {
							if(!in_cit1 && !in_cit2 && brackets > 0) {
								if(b_old_matrix != 0 && brackets == 2 && i < str.length() - 1) {
									size_t i2 = str.find_first_not_of(SPACE, i + 1);
									if(i2 != string::npos && (str[i2] == COMMA_CH || str[i2] == ';')) i2 = str.find_first_not_of(SPACE, i2 + 1);
									if(i2 != string::npos) {
										if(str[i2] == LEFT_VECTOR_WRAP_CH) b_old_matrix = 1;
										else if(str[i2] != RIGHT_VECTOR_WRAP_CH) b_old_matrix = 0;
									}
								}
								brackets--;
							}
							break;
						}
						case LEFT_PARENTHESIS_CH: {
							if(brackets == 1 && !in_cit1 && !in_cit2) pars++;
							break;
						}
						case RIGHT_PARENTHESIS_CH: {
							if(brackets == 1 && !in_cit1 && !in_cit2 && pars > 0) pars--;
							break;
						}
						case '\"': {
							if(in_cit1) in_cit1 = false;
							else if(!in_cit2) in_cit1 = true;
							break;
						}
						case '\'': {
							if(in_cit2) in_cit2 = false;
							else if(!in_cit1) in_cit1 = true;
							break;
						}
						case ' ': {
							if(pars == 0 && !in_cit1 && !in_cit2 && i < str.length() - 1) {
								if(b_old_matrix != 0 && ((brackets == 1 && i != str_index + 1) || (brackets == 2 && !is_not_number(str[i - 1], base) && !is_not_number(str[i + 1], base))) && str[i - 1] != ',' && str[i - 1] != ';' && str[i + 1] != ',' && str[i + 1] != ';') {
									b_old_matrix = 0;
								}
							}
							break;
						}
						case ',': {
							if(brackets == 1 && pars == 0 && !in_cit1 && !in_cit2) {
								b_comma = true;
								size_t i2 = str.find_last_not_of(SPACE, i - 1);
								if(i2 != string::npos && str[i2] != RIGHT_VECTOR_WRAP_CH) {
									b_old_matrix = 0;
								}
							}
							break;
						}
						case ';': {
							if(brackets == 1 && pars == 0 && !in_cit1 && !in_cit2) {
								size_t i2 = str.find_last_not_of(SPACE, i - 1);
								if(i2 != string::npos && str[i2] != RIGHT_VECTOR_WRAP_CH) {
									b_old_matrix = 0;
								}
							}
							break;
						}
					}
				}
			}
			if(b_old_matrix <= 0) {
				MathStructure *mstruct2 = new MathStructure();
				mstruct2->clearVector();
				MathStructure *mrow = NULL;
				MathStructure unended_test;
				if(unended_function) po.unended_function = &unended_test;
				string prev_func;
				bool in_cit1 = false, in_cit2 = false, first_not_unit = false;
				int pars = 0;
				int brackets = 1;
				size_t i = str_index + 1;
				size_t col_index = i;
				for(; i < str.length() && brackets > 0; i++) {
					bool b_row = false, b_col = false;
					switch(str[i]) {
						case LEFT_VECTOR_WRAP_CH: {
							if(!in_cit1 && !in_cit2) brackets++;
							break;
						}
						case RIGHT_VECTOR_WRAP_CH: {
							if(!in_cit1 && !in_cit2 && brackets > 0) brackets--;
							break;
						}
						case LEFT_PARENTHESIS_CH: {
							if(brackets == 1 && !in_cit1 && !in_cit2) pars++;
							break;
						}
						case RIGHT_PARENTHESIS_CH: {
							if(brackets == 1 && !in_cit1 && !in_cit2 && pars > 0) pars--;
							break;
						}
						case '\"': {
							if(in_cit1) in_cit1 = false;
							else if(!in_cit2) in_cit1 = true;
							break;
						}
						case '\'': {
							if(in_cit2) in_cit2 = false;
							else if(!in_cit1) in_cit1 = true;
							break;
						}
						case ';': {
							if(brackets == 1 && pars == 0 && !in_cit1 && !in_cit2) b_row = true;
							break;
						}
						case SPACE_CH: {
							if(b_comma) break;
						}
						case ',': {
							if(brackets == 1 && pars == 0 && !in_cit1 && !in_cit2) {
								if(!b_comma && (str[i - 1] == ';' || (is_in(OPERATORS INTERNAL_OPERATORS, str[i - 1]) && str[i - 1] != NOT_CH))) break;
								if(!b_comma && i + 1 < str.length()) {
									if(is_in(OPERATORS INTERNAL_OPERATORS, str[i + 1])) {
										if(str[i + 1] == PLUS_CH || str[i + 1] == MINUS_CH) {
											if(i + 2 == str.length() || str[i + 2] == SPACE_CH) {
												break;
											}
										} else if(str[i + 1] != BITWISE_NOT_CH && str[i + 1] != LOGICAL_NOT_CH) {
											break;
										}
									}
								}
								b_col = true;
							}
							break;
						}
						default: {}
					}
					if(brackets == 0) {
						b_row = true;
					} else if(i == str.length() - 1) {
						i++;
						b_row = true;
					}
					if(b_row) b_col = true;
					if(b_col) {
						stmp2 = str.substr(col_index, i - col_index);
						remove_blank_ends(stmp2);
						MathStructure *mcol = NULL;
						if(b_comma || (b_row && ((mrow && mrow->size() == 0) || (mstruct2->size() == 0 && brackets > 0 && i < str.length() - 1))) || !stmp2.empty()) {
							bool b_unit = false;
							if(!b_comma && !prev_func.empty()) {
								prev_func += stmp2;
								prev_func += SPACE_CH;
								parse(mrow ? &mrow->last() : &mstruct2->last(), prev_func, po);
							} else {
								mcol = new MathStructure();
								parse(mcol, stmp2, po);
								b_unit = !b_comma && first_is_unit(*mcol) && stmp2.size() > 0 && is_not_in(ILLEGAL_IN_UNITNAMES, stmp2[0]) && is_unit_multiexp(*mcol);
								if(b_unit) {
									if(!first_not_unit) b_unit = false;
									else if(mrow) mrow->last().multiply_nocopy(mcol);
									else mstruct2->last().multiply_nocopy(mcol);
									if(b_unit) mcol = NULL;
								} else if((mrow && mrow->size() == 0) || (!mrow && mstruct2->size() == 0)) {
									first_not_unit = true;
								}
							}
						}
						col_index = i + 1;
						if(mrow) {
							if(mcol) {
								if(mrow->isVector() && (!b_row || mrow->size() > 0) && !mcol->representsScalar()) {
									mrow->setType(STRUCT_FUNCTION);
									mrow->setFunction(priv->f_horzcat);
								}
								mrow->addChild_nocopy(mcol);
							}
							if(b_row) mstruct2->addChild_nocopy(mrow);
						} else if(mcol) {
							if(mstruct2->isVector() && (!b_row || mstruct2->size() > 0) && !mcol->representsScalar()) {
								mstruct2->setType(STRUCT_FUNCTION);
								mstruct2->setFunction(priv->f_horzcat);
							}
							mstruct2->addChild_nocopy(mcol);
						}
						if(i < str.length() - 1 && brackets > 0) {
							if(b_row) {
								if(!mrow) mstruct2->transform(STRUCT_VECTOR);
								mrow = new MathStructure();
								mrow->clearVector();
								first_not_unit = false;
								prev_func = "";
							} else if(mcol && !b_comma) {
								if(last_is_function(*mcol) && is_not_in(ILLEGAL_IN_NAMES, stmp2[stmp2.size() - 1])) {
									prev_func = stmp2;
								}
							}
						}
						if(b_row && mrow && !mstruct2->last().representsNonMatrix()) {
							mstruct2->setType(STRUCT_FUNCTION);
							mstruct2->setFunction(priv->f_vertcat);
						}
					}
				}
				i--;
				if(brackets != 0 && unended_function && !unended_test.isZero()) {
					unended_function->set(unended_test);
				}
				if(mstruct2->type() == STRUCT_FUNCTION) {
					for(size_t i = 1; i <= mstruct2->size(); i++) {
						while(mstruct2->getChild(i)->isVector() && mstruct2->getChild(i)->size() == 1) mstruct2->getChild(i)->setToChild(1);
					}
				}
				while(mstruct2->isVector() && mstruct2->size() == 1) mstruct2->setToChild(1);
				po.unended_function = NULL;
				stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
				stmp += i2s(addId(mstruct2));
				stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
				str.replace(str_index, i + 1 - str_index, stmp);
				str_index += stmp.length() - 1;
			} else {
				if(b_old_matrix == 1) {
					priv->matlab_matrices = false;
					bool in_cit1 = false, in_cit2 = false;
					int brackets = 1;
					for(size_t i = str_index + 1; i < str.length() && brackets > 0 && (b_old_matrix != 0 || !b_comma); i++) {
						switch(str[i]) {
							case LEFT_VECTOR_WRAP_CH: {
								if(!in_cit1 && !in_cit2) brackets++;
								break;
							}
							case RIGHT_VECTOR_WRAP_CH: {
								if(!in_cit1 && !in_cit2 && brackets > 0) brackets--;
								break;
							}
							case '\"': {
								if(in_cit1) in_cit1 = false;
								else if(!in_cit2) in_cit1 = true;
								break;
							}
							case '\'': {
								if(in_cit2) in_cit2 = false;
								else if(!in_cit1) in_cit1 = true;
								break;
							}
							case ';': {
								if(!in_cit1 && !in_cit2) str[i] = COMMA_CH;
								break;
							}
						}
					}
				}
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
				if(b_old_matrix == 1) priv->matlab_matrices = true;
			}
		} else if(str[str_index] == '\\' && str_index + 1 < str.length() && (is_not_in(NOT_IN_NAMES INTERNAL_OPERATORS NUMBERS, str[str_index + 1]) || (PARSING_MODE != PARSING_MODE_RPN && str_index > 0 && is_in(NUMBERS SPACE PLUS MINUS BITWISE_NOT NOT LEFT_PARENTHESIS, str[str_index + 1])))) {
			if(is_in(NUMBERS SPACE PLUS MINUS BITWISE_NOT NOT LEFT_PARENTHESIS, str[str_index + 1])) {
				// replace \ followed by number with // for integer division
				str.replace(str_index, 1, "//");
				str_index++;
			} else {
				// replaced \ followed by a character with symbolic MathStructure
				stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
				size_t l = 1;
				if((signed char) str[str_index + l] < 0) {
					do {
						l++;
					} while(str_index + l < str.length() && (signed char) str[str_index + l] < 0 && (unsigned char) str[str_index + l] < 0xC0);
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
				} else if(i5 > 0 && str[i5] == ID_WRAP_RIGHT_CH && (i3 = str.find_last_of(ID_WRAP_LEFT, i5 - 1)) != string::npos) {
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
		} else if(PARSING_MODE != PARSING_MODE_RPN && (str[str_index] == 'c' || str[str_index] == 'C') && str.length() > str_index + 6 && str[str_index + 5] == SPACE_CH && (str_index == 0 || is_in(OPERATORS INTERNAL_OPERATORS PARENTHESISS, str[str_index - 1])) && compare_name_no_case("compl", str, 5, str_index, base)) {
			// interpret "compl" followed by space as bitwise not
			str.replace(str_index, 6, BITWISE_NOT);
			ascii_bitwise = 1;
		} else if(PARSING_MODE != PARSING_MODE_RPN && (str[str_index] == 'n' || str[str_index] == 'N') && str.length() > str_index + 4 && str[str_index + 3] == SPACE_CH && (str_index == 0 || is_in(OPERATORS INTERNAL_OPERATORS PARENTHESISS, str[str_index - 1])) && compare_name_no_case("not", str, 3, str_index, base)) {
			// interpret "NOT" followed by space as logical not
			str.replace(str_index, 4, LOGICAL_NOT);
			ascii_bitwise = 1;
		} else if(str[str_index] == SPACE_CH) {
			size_t i = str.find(SPACE, str_index + 1);
			if(PARSING_MODE == PARSING_MODE_RPN && i == string::npos) i = str.length();
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
					if(!ascii_bitwise) ascii_bitwise = 2;
					str_index += 2;
				} else if(i == AND_str_len && (il = compare_name_no_case(AND_str, str, AND_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_AND);
					if(!ascii_bitwise) ascii_bitwise = 2;
					str_index += 2;
				} else if(or_str_len > 0 && i == or_str_len && (il = compare_name_no_case(or_str, str, or_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_OR);
					if(!ascii_bitwise) ascii_bitwise = 2;
					test_or_parallel = false;
					str_index += 2;
				} else if(i == OR_str_len && (il = compare_name_no_case(OR_str, str, OR_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, LOGICAL_OR);
					if(!ascii_bitwise) ascii_bitwise = 2;
					test_or_parallel = false;
					str_index += 2;
				} else if(i == XOR_str_len && (il = compare_name_no_case(XOR_str, str, XOR_str_len, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\a");
					if(!ascii_bitwise) ascii_bitwise = 2;
					str_index++;
				} else if(i == 5 && (il = compare_name_no_case("bitor", str, 5, str_index + 1, base))) {
					str.replace(str_index + 1, il, BITWISE_OR);
					ascii_bitwise = 1;
					str_index++;
				} else if(i == 6 && (il = compare_name_no_case("bitand", str, 6, str_index + 1, base))) {
					str.replace(str_index + 1, il, BITWISE_AND);
					ascii_bitwise = 1;
					str_index++;
				} else if(i == 4 && (il = compare_name_no_case("nand", str, 4, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\x1d");
					if(!ascii_bitwise) ascii_bitwise = 2;
					str_index++;
				} else if(i == 4 && po.functions_enabled && ((il = compare_name_no_case("perm", str, 4, str_index + 1, base)) || (il = compare_name_no_case("comb", str, 4, str_index + 1, base)))) {
					MathFunction *f = NULL;
					if(str[str_index + 1] == 'p' || str[str_index + 1] == 'P') f = CALCULATOR->getActiveFunction("perm");
					else f = CALCULATOR->getActiveFunction("comb");
					if(f) {
						int i_par = 0;
						size_t i2 = str_index + 2 + il;
						bool b = true;
						for(; i2 < str.length(); i2++) {
							if(str[i2] == LEFT_PARENTHESIS_CH) {
								i_par++;
							} else if(str[i2] == RIGHT_PARENTHESIS_CH) {
								if(i_par == 0) break;
								i_par--;
							} else if(i_par == 0 && str[i2] == COMMA_CH) {
								b = false;
								break;
							}
						}
						if(b) {
							i_par = 0;
							size_t i3 = str_index;
							while(true) {
								if(str[i3] == LEFT_PARENTHESIS_CH) {
									if(i_par == 0) {i3++; break;}
									i_par--;
								} else if(str[i3] == RIGHT_PARENTHESIS_CH) {
									i_par++;
								}
								if(i3 == 0) break;
								i3--;
							}
							stmp2 = str.substr(i3, str_index - i3);
							stmp2 += ",";
							stmp2 += str.substr(str_index + 1 + il, i2 - (str_index + 1 + il));
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(parseAddId(f, stmp2, po));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
							str.replace(i3, i2 - i3, stmp);
							str_index = i3 + stmp.length() + 1;
						}
					}
				} else if(i == 3 && (il = compare_name_no_case("nor", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\x1e");
					if(!ascii_bitwise) ascii_bitwise = 2;
					str_index++;
				} else if(i == 3 && (il = compare_name_no_case("not", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il + 1, LOGICAL_NOT);
					ascii_bitwise = 1;
					str_index++;
				} else if(i == 3 && (il = compare_name_no_case("mod", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\%\%");
					str_index += 2;
				} else if(i == 3 && (il = compare_name_no_case("rem", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il, "%");
					str_index++;
				} else if(i == 3 && (il = compare_name_no_case("dot", str, 3, str_index + 1, base))) {
					str.replace(str_index + 1, il, ".");
					str_index++;
				} else if(i == 5 && (il = compare_name_no_case("cross", str, 5, str_index + 1, base))) {
					str.replace(str_index + 1, il, "\x15");
					str_index++;
				} else if(i == 3 && (il = compare_name_no_case("div", str, 3, str_index + 1, base))) {
					if(PARSING_MODE == PARSING_MODE_RPN) {
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
					if(PARSING_MODE == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS "abcdefABCDEF", str_index + 2);
					else i = str.find_first_not_of(SPACE NUMBER_ELEMENTS "abcdefABCDEF", str_index + 2);
					size_t name_length;
					if(i == string::npos) {
						i = str.length();
					} else if(PARSING_MODE != PARSING_MODE_RPN && is_not_in(ILLEGAL_IN_UNITNAMES, str[i]) && is_in("abcdefABCDEF", str[i - 1])) {
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
					if(PARSING_MODE == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS, str_index + 2);
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
					if(PARSING_MODE == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS DUODECIMAL_CHARS, str_index + 2);
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
					if(PARSING_MODE == PARSING_MODE_RPN) i = str.find_first_not_of(NUMBER_ELEMENTS, str_index + 2);
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
		} else if(str[str_index] == DOT_CH && !po.rpn && str_index > 0 && str_index < str.length() - 1 && is_not_number(str[str_index - 1], base) && is_not_number(str[str_index + 1], base) && is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[str_index - 1]) && (str[str_index + 1] == POWER_CH || str[str_index + 1] == MULTIPLICATION_CH || str[str_index + 1] == DIVISION_CH || is_not_in(INTERNAL_OPERATORS OPERATORS "\\", str[str_index + 1])) && (str[str_index - 1] != DOT_CH || str[str_index + 1] != DOT_CH)) {
			if(str[str_index + 1] == MULTIPLICATION_CH) str.replace(str_index, 2, "\x17");
			else if(str[str_index + 1] == DIVISION_CH) str.replace(str_index, 2, "\x18");
			else if(str[str_index + 1] == POWER_CH) str.replace(str_index, 2, "\x19");
			else str[str_index] = '\x16';
		} else if(is_not_in(NUMBERS INTERNAL_OPERATORS NOT_IN_NAMES, str[str_index])) {
			// dx/dy derivative notation
			if((str[str_index] == 'd' && is_not_number('d', base)) || ((signed char) str[str_index] == -50 && str_index + 1 < str.length() && (signed char) str[str_index + 1] == -108) || ((signed char) str[str_index] == -16 && str_index + 3 < str.length() && (signed char) str[str_index + 1] == -99 && (signed char) str[str_index + 2] == -102 && (signed char) str[str_index + 3] == -85)) {
				size_t d_len = 1;
				if((signed char) str[str_index] == -50) d_len = 2;
				else if((signed char) str[str_index] == -16) d_len = 4;
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
			size_t underscore = false;
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
										underscore = priv->ufvl_us[ufv_index]; name_length -= underscore;
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
										underscore = priv->ufvl_us[ufv_index]; name_length -= underscore;
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
										underscore = priv->ufvl_us[ufv_index]; name_length -= underscore;
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
										name = &((Prefix*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).name;
										name_length = name->length();
										underscore = priv->ufvl_us[ufv_index]; name_length -= underscore;
										if(name_length >= unit_chars_left || name_length < found_function_name_length) {
											name = NULL;
										}
										case_sensitive = ((Prefix*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).case_sensitive;
									}
									break;
								}
								case 'P': {
									if(!p && po.units_enabled) {
										name = &((Prefix*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).name;
										name_length = name->length();
										underscore = priv->ufvl_us[ufv_index]; name_length -= underscore;
										if(name_length > unit_chars_left || name_length < found_function_name_length) {
											name = NULL;
										}
										case_sensitive = ((Prefix*) ufvl[ufv_index])->getName(ufvl_i[ufv_index]).case_sensitive;
									}
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
							if(((Prefix*) object)->getName(ufv_i[vt2][ufv_index][vt3]).abbreviation) {
								if(ufv_index < unit_chars_left - 1) {
									ufvt = 'p';
									name = &((Prefix*) object)->getName(ufv_i[vt2][ufv_index][vt3]).name;
									name_length = name->length();
									underscore = priv->ufv_us[vt2][ufv_index][vt3]; name_length -= underscore;
									case_sensitive = ((Prefix*) object)->getName(ufv_i[vt2][ufv_index][vt3]).case_sensitive;
								}
							} else {
								ufvt = 'P';
								name = &((Prefix*) object)->getName(ufv_i[vt2][ufv_index][vt3]).name;
								name_length = name->length();
								underscore = priv->ufv_us[vt2][ufv_index][vt3]; name_length -= underscore;
								case_sensitive = ((Prefix*) object)->getName(ufv_i[vt2][ufv_index][vt3]).case_sensitive;
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
							underscore = priv->ufv_us[vt2][ufv_index][vt3]; name_length -= underscore;
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
								underscore = priv->ufv_us[vt2][ufv_index][vt3]; name_length -= underscore;
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
							underscore = priv->ufv_us[vt2][ufv_index][vt3]; name_length -= underscore;
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
				if(name && name_length >= found_function_name_length && ((case_sensitive && (name_length = compare_name(*name, str, name_length, str_index, base, underscore))) || (!case_sensitive && (name_length = compare_name_no_case(*name, str, name_length, str_index, base, underscore))))) {
					if(ufvt != 'p' && po.units_enabled && name_length < name_chars_left && name_chars_left > 2 && name_chars_left - 1 <= UFV_LENGTHS) {
						for(size_t i7 = 0; i7 < ufv[0][0].size(); i7++) {
							if(((Prefix*) ufv[0][0][i7])->getName(ufv_i[0][0][i7]).name[0] == str[str_index]) {
								for(size_t i8 = 0; i8 < ufv[2][name_chars_left - 2].size(); i8++) {
									const ExpressionName *name_u = &((ExpressionItem*) ufv[2][name_chars_left - 2][i8])->getName(ufv_i[2][name_chars_left - 2][i8]);
									size_t name_length_u = name_u->name.length();
									bool underscore_u = priv->ufv_us[2][name_chars_left - 2][i8];
									name_length_u -= underscore_u;
									if(((name_u->case_sensitive && (name_length_u = compare_name(name_u->name, str, name_length_u, str_index + 1, base, underscore_u))) || (!name_u->case_sensitive && (name_length_u = compare_name_no_case(name_u->name, str, name_length_u, str_index + 1, base, underscore_u)))) && name_length_u + 1 == name_chars_left) {
										ufvt = 'u';
										p = (Prefix*) ufv[0][0][i7];
										object = (ExpressionItem*) ufv[2][name_chars_left - 2][i8];
										name_length = name_length_u + 1;
										break;
									}
								}
								break;
							}
						}
					}
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
							if(str_index + name_length + 2 < str.length() && (str[str_index + name_length] == POWER_CH || str[str_index + name_length] == INTERNAL_UPOW_CH) && (f->id() == FUNCTION_ID_SIN || f->id() == FUNCTION_ID_COS || f->id() == FUNCTION_ID_TAN || f->id() == FUNCTION_ID_SINH || f->id() == FUNCTION_ID_COSH || f->id() == FUNCTION_ID_TANH) && ((str[str_index + name_length + 1] == MINUS_CH && str[str_index + name_length + 2] == '1' && (str_index + name_length + 3 == str.length() || is_not_in(NUMBER_ELEMENTS, str[str_index + name_length + 3]))) || (str[str_index + name_length + 1] == LEFT_PARENTHESIS_CH && str[str_index + name_length + 2] == MINUS_CH && str_index + name_length + 4 < str.length() && str[str_index + name_length + 3] == '1' && str[str_index + name_length + 4] == RIGHT_PARENTHESIS_CH))) {
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
							} else if(PARSING_MODE == PARSING_MODE_CHAIN && f->minargs() == 1 && str_index > 0 && (i6 = str.find_last_not_of(SPACE, str_index - 1)) != string::npos && str[i6] != LEFT_PARENTHESIS_CH && is_not_in(OPERATORS INTERNAL_OPERATORS, str[i6]) && (str_index + name_length >= str.length() || (str.find_first_not_of(SPACE, str_index + name_length) == string::npos || is_in(OPERATORS INTERNAL_OPERATORS, str[str.find_first_not_of(SPACE, str_index + name_length)])))) {
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
							} else if(PARSING_MODE == PARSING_MODE_RPN && f->args() == 1 && str_index > 0 && str[str_index - 1] != LEFT_PARENTHESIS_CH && (str_index + name_length >= str.length() || str[str_index + name_length] != LEFT_PARENTHESIS_CH) && (i6 = str.find_last_not_of(SPACE, str_index - 1)) != string::npos) {
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
								size_t icand = 0;
								int arg_i = 1;
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
											else if(i5 == 2 && PARSING_MODE >= PARSING_MODE_CONVENTIONAL && !b_power_before) b = true;
											else i5++;
										} else if(c == RIGHT_PARENTHESIS_CH) {
											if(i5 <= 2) b = true;
											else i5--;
										} else if(c == POWER_CH || c == INTERNAL_UPOW_CH) {
											if(i5 < 2) i5 = 2;
											b_power_before = true;
										} else if(!b_comma_before && !b_power_before && c == ' ' && ((arg_i >= f->args() && f->args() >= 0) || arg_i >= f->minargs())) {
											//if(i5 < 2) b_space_first = true;
											if(i5 == 2) {
												if(arg_i >= f->args() && f->args() >= 0) b = true;
												else icand = i6 + 1;
											}
										} else if(!b_comma_before && i5 == 2 && ((arg_i >= f->args() && f->args() >= 0) || arg_i >= f->minargs()) && is_in(OPERATORS INTERNAL_OPERATORS, c) && c != POWER_CH && c != '\b' && c != INTERNAL_UPOW_CH && ((c != MINUS_CH && c != PLUS_CH) || (!b_power_before && (i6 < 3 || !BASE_2_10 || is_not_in(EXPS, str[str_index + name_length + i6 - 1]) || is_not_in(NUMBERS, str[str_index + name_length + i6 - 2]) || i6 + str_index + name_length == str.length() - 1 || is_not_in(NUMBERS, str[str_index + name_length + i6 + 1]))))) {
											if(arg_i >= f->args() && f->args() >= 0) b = true;
											else icand = i6 + 1;
										} else if(c == COMMA_CH) {
											if(i5 == 2) {
												if(f->args() == 1 && (!f->getArgumentDefinition(1) || f->getArgumentDefinition(1)->type() != ARGUMENT_TYPE_VECTOR)) b = true;
												icand = 0;
												arg_i++;
											}
											b_comma_before = true;
											if(i5 < 2) i5 = 2;
										} else if(i5 < 2) {
											i5 = 2;
										}
										if(c != COMMA_CH && c != ' ') b_comma_before = false;
										if(c != POWER_CH && c != INTERNAL_UPOW_CH && c != ' ') b_power_before = false;
									}
									i6++;
								}
								if(icand > 0) i6 = icand;
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
								if(PARSING_MODE == PARSING_MODE_CHAIN) {
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
											underscore = priv->ufvl_us[ufv_index2]; name_length -= underscore;
											if(po.limit_implicit_multiplication || ((Unit*) ufvl[ufv_index2])->getName(ufvl_i[ufv_index2]).plural) {
												if(name_length != unit_chars_left) name = NULL;
											} else if(name_length > unit_chars_left) {
												name = NULL;
											}
											break;
										}
									}
									if(name && ((case_sensitive && (name_length = compare_name(*name, str, name_length, str_index, base, underscore))) || (!case_sensitive && (name_length = compare_name_no_case(*name, str, name_length, str_index, base, underscore))))) {
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
									underscore = priv->ufv_us[2][index][ufv_index2]; name_length -= underscore;
									if(index + 1 == (int) unit_chars_left || !((Unit*) ufv[2][index][ufv_index2])->getName(ufv_i[2][index][ufv_index2]).plural) {
										if(name_length <= unit_chars_left && ((case_sensitive && (name_length = compare_name(*name, str, name_length, str_index, base, underscore))) || (!case_sensitive && (name_length = compare_name_no_case(*name, str, name_length, str_index, base, underscore))))) {
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
					if((signed char) str[str_index + 1] < 0) {
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

	if(PARSING_MODE == PARSING_MODE_RPN) {
		size_t rpn_i = str.find(SPACE, 0);
		while(rpn_i != string::npos) {
			if(rpn_i == 0 || rpn_i + 1 == str.length() || is_in("~+-*/^\\" INTERNAL_OPERATORS_NOMOD, str[rpn_i - 1]) || (is_in("%&|", str[rpn_i - 1]) && str[rpn_i + 1] != str[rpn_i - 1]) || (is_in("!><=", str[rpn_i - 1]) && is_not_in("=<>", str[rpn_i + 1])) || (is_in(SPACE OPERATORS INTERNAL_OPERATORS, str[rpn_i + 1]) && (str[rpn_i - 1] == SPACE_CH || (str[rpn_i - 1] != str[rpn_i + 1] && is_not_in("!><=", str[rpn_i - 1]))))) {
				str.erase(rpn_i, 1);
			} else {
				rpn_i++;
			}
			rpn_i = str.find(SPACE, rpn_i);
		}
	} else if(PARSING_MODE != PARSING_MODE_ADAPTIVE) {
		remove_blanks(str);
	} else {
		//remove spaces between next to operators (except '/') and before/after parentheses
		size_t space_i = str.find(SPACE_CH, 0);
		while(space_i != string::npos) {
			if((str[space_i + 1] != DIVISION_CH && is_in(OPERATORS INTERNAL_OPERATORS RIGHT_PARENTHESIS, str[space_i + 1])) || (str[space_i - 1] != DIVISION_CH && is_in(OPERATORS INTERNAL_OPERATORS LEFT_PARENTHESIS, str[space_i - 1]))) {
				str.erase(space_i, 1);
				space_i--;
			}
			space_i = str.find(SPACE_CH, space_i + 1);
		}
	}

	if(test_or_parallel) {
		beginTemporaryStopMessages();
		unordered_map<size_t, size_t> ids_ref_bak = priv->ids_ref;
		for(unordered_map<size_t, size_t>::iterator it = priv->ids_ref.begin(); it != priv->ids_ref.end(); ++it) {
			it->second++;
		}
		parseOperators(mstruct, str, po);
		if(contains_parallel(*mstruct)) {
			endTemporaryStopMessages();
			gsub("||", "\x14", str);
			parseOperators(mstruct, str, po);
		} else {
			endTemporaryStopMessages(true);
		}
		for(unordered_map<size_t, size_t>::iterator it = ids_ref_bak.begin(); it != ids_ref_bak.end(); ++it) {
			delId(it->first);
		}
	} else {
		parseOperators(mstruct, str, po);
	}

	if(is_boolean_algebra_expression(*mstruct, ascii_bitwise)) {
		bitwise_to_logical(*mstruct);
	}

}

bool Calculator::parseNumber(MathStructure *mstruct, string str, const ParseOptions &po) {

	mstruct->clear();
	if(str.empty()) return false;

	// check that string contains characters other than operators and whitespace
	if(str.find_first_not_of(OPERATORS INTERNAL_OPERATORS_NOPM SPACE) == string::npos && (po.base != BASE_ROMAN_NUMERALS || str.find("|") == string::npos)) {
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
			error(false, _("Misplaced operator(s) \"%s\" ignored"), internal_operator_replacement(str[i]).c_str(), NULL);
			str.erase(i, 1);
		} else if(str[i] == '\a' || (str[i] <= '\x1f' && str[i] >= '\x1c') || (str[i] <= '\x1a' && str[i] >= '\x14')) {
			// ignore operators
			error(false, _("Misplaced operator(s) \"%s\" ignored"), internal_operator_replacement(str[i]).c_str(), NULL);
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
		} else if(po.base == BASE_DECIMAL && itmp == str.length() - 1 && (str[itmp] == 'm' || str[itmp] == 'M')) {
			mulexp = 6;
			str.erase(itmp, str.length() - itmp);
		} else {
			string stmp = str.substr(itmp, str.length() - itmp);
			error(true, _("Trailing characters \"%s\" (not a valid variable/function/unit) in number \"%s\" were ignored."), stmp.c_str(), str.c_str(), NULL);
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
					if(s == OPERATION_MULTIPLY && (mstruct->isVector() && mstruct2->isVector() && mstruct->size() == mstruct2->size() && !mstruct->isMatrix() && !mstruct2->isMatrix())) error(true, _("Please use the cross(), dot(), and hadamard() functions for vector multiplication."), NULL);
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
				if(s == OPERATION_MULTIPLY && mstruct->isVector() && mstruct2->isVector() && mstruct->size() == mstruct2->size() && !mstruct->isMatrix() && !mstruct2->isMatrix()) error(true, _("Please use the cross(), dot(), and hadamard() functions for vector multiplication."), NULL);
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
			if(PARSING_MODE == PARSING_MODE_RPN) {
				str.insert(i2 + 1, MULTIPLICATION);
				str.insert(i, SPACE);
				i++;
				i2++;
			}
		}
		if(i2 + 1 < str.length() && is_not_in(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS SPACE, str[i2 + 1]) && (!BASE_2_10 || (str[i2 + 1] != EXP_CH && str[i2 + 1] != EXP2_CH))) {
			if(PARSING_MODE == PARSING_MODE_RPN) {
				i3 = str.find(SPACE, i2 + 1);
				if(i3 == string::npos) {
					str += MULTIPLICATION;
				} else {
					str.replace(i3, 1, MULTIPLICATION);
				}
				str.insert(i2 + 1, SPACE);
			}
		}
		if(PARSING_MODE == PARSING_MODE_RPN && i > 0 && i2 + 1 == str.length() && is_not_in(PARENTHESISS SPACE, str[i - 1])) {
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
	if(PARSING_MODE != PARSING_MODE_RPN) {
		// determine if | is used for absolute value
		while(po.base != BASE_ROMAN_NUMERALS && (i = str.find('|', i)) != string::npos) {
			if(i == 0 || i == str.length() - 1 || is_in(OPERATORS INTERNAL_OPERATORS SPACE, str[i - 1]) || is_in("*/^<>=", str[i + 1])) {b_abs_or = true; break;}
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
	if(PARSING_MODE == PARSING_MODE_RPN) {
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
			i = str.find_first_of(OPERATORS INTERNAL_OPERATORS_RPN SPACE "\\", i3 + 1);
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
								case '\x14': {
									mstack[mstack.size() - 2]->transform(priv->f_parallel);
									mstack[mstack.size() - 2]->addChild_nocopy(mstack.back());
									mstack.pop_back();
									break;
								}
								case '\x1d': {
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_AND, mstack.back());
									mstack[mstack.size() - 2]->transform(STRUCT_LOGICAL_NOT);
									mstack.pop_back();
									break;
								}
								case '\x1e': {
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_OR, mstack.back());
									mstack[mstack.size() - 2]->transform(STRUCT_LOGICAL_NOT);
									mstack.pop_back();
									break;
								}
								case '\x1f': {
									mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_XOR, mstack.back());
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
						error(false, _("RPN syntax error. Operator ignored as there was only one stack value."), NULL);
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
						case '\x14': {
							mstack[mstack.size() - 2]->transform(priv->f_parallel);
							mstack[mstack.size() - 2]->add_nocopy(mstack.back());
							mstack.pop_back();
							break;
						}
						case '\x1d': {
							mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_AND, mstack.back());
							mstack[mstack.size() - 2]->transform(STRUCT_LOGICAL_NOT);
							mstack.pop_back();
							break;
						}
						case '\x1e': {
							mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_OR, mstack.back());
							mstack[mstack.size() - 2]->transform(STRUCT_LOGICAL_NOT);
							mstack.pop_back();
							break;
						}
						case '\x1f': {
							mstack[mstack.size() - 2]->transform_nocopy(STRUCT_LOGICAL_XOR, mstack.back());
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
	if(PARSING_MODE == PARSING_MODE_RPN) remove_blanks(str);

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
	if((i = str.find('\x1d', 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 1 != str.length()) {
			str2 = str.substr(0, i);
			str = str.substr(i + 1, str.length() - (i + 1));
			if(b) {
				parseAdd(str2, mstruct, po, OPERATION_LOGICAL_AND, append);
				append = true;
			} else {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			i = str.find('\x1d', 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_LOGICAL_AND, append);
		} else {
			parseAdd(str, mstruct, po);
		}
		mstruct->transform(STRUCT_LOGICAL_NOT);
		return true;
	}
	if((i = str.find('\x1e', 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 1 != str.length()) {
			str2 = str.substr(0, i);
			str = str.substr(i + 1, str.length() - (i + 1));
			if(b) {
				parseAdd(str2, mstruct, po, OPERATION_LOGICAL_OR, append);
				append = true;
			} else {
				parseAdd(str2, mstruct, po);
				b = true;
			}
			i = str.find('\x1e', 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_LOGICAL_OR, append);
		} else {
			parseAdd(str, mstruct, po);
		}
		mstruct->transform(STRUCT_LOGICAL_NOT);
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
	if((i = str.find('\x1f', 1)) != string::npos && i + 1 != str.length()) {
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_LOGICAL_XOR);
		return true;
	}
	// Parse | as bitwise or
	if(PARSING_MODE != PARSING_MODE_CHAIN && po.base != BASE_ROMAN_NUMERALS && (i = str.find(BITWISE_OR, 1)) != string::npos && i + 1 != str.length()) {
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
	if(PARSING_MODE != PARSING_MODE_CHAIN && (i = str.find('\a', 1)) != string::npos && i + 1 != str.length()) {
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_BITWISE_XOR);
		return true;
	}
	// Parse & as bitwise and
	if(PARSING_MODE != PARSING_MODE_CHAIN && (i = str.find(BITWISE_AND, 1)) != string::npos && i + 1 != str.length()) {
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

	if(PARSING_MODE == PARSING_MODE_CHAIN) {
		char c_operator = 0, prev_operator = 0;
		bool append = false;
		while(true) {
			i3 = str.find_first_not_of(OPERATORS INTERNAL_OPERATORS, 0);
			if(i3 == string::npos) i = string::npos;
			else i = str.find_first_of(PLUS MINUS MULTIPLICATION DIVISION POWER INTERNAL_UPOW BITWISE_AND BITWISE_OR "<>%\x1c\x14", i3);
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
							if(!(po.parsing_mode & PARSE_PERCENT_AS_ORDINARY_CONSTANT) && mstruct_b->isVariable() && (mstruct_b->variable() == v_percent || mstruct_b->variable() == v_permille || mstruct_b->variable() == v_permyriad)) {
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
					case INTERNAL_UPOW_CH: {}
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
					case '\x14': {
						MathStructure *mstruct2 = new MathStructure();
						parseAdd(str2, mstruct2, po);
						if(!append) mstruct->transform(priv->f_parallel);
						mstruct->addChild_nocopy(mstruct2);
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

	if(PARSING_MODE != PARSING_MODE_CHAIN) {

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
			bool b = false, c = false, append = false, do_percent = !(po.parsing_mode & PARSE_PERCENT_AS_ORDINARY_CONSTANT);
			bool min = false;
			while(i != string::npos && i + 1 != str.length()) {
				if(is_not_in(BASE_2_10 ? MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS_TWO EXPS : MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS_TWO, str[i - 1])) {
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

		// In adaptive parsing mode division might be handled differently depending on usage of whitespace characters, e.g. 5/2 m = (5/2)*m, 5/2m=5/(2m)
		if(PARSING_MODE == PARSING_MODE_ADAPTIVE && (i = str.find(DIVISION_CH, 1)) != string::npos && i + 1 != str.length()) {
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
						} else if(i2 > 4 && str[i2 - 3] == ID_WRAP_RIGHT_CH && (str[i2 - 2] == POWER_CH || str[i2 - 2] == INTERNAL_UPOW_CH) && is_in(NUMBERS, str[i2 - 1])) {
							b = true;
							i4 -= 2;
						}
						if(!b) {
							if((i2 > 0 && is_not_in(OPERATORS INTERNAL_OPERATORS MULTIPLICATION_2, str[i2 - 1])) || (i2 > 1 && str[i2 - 1] == MULTIPLICATION_2_CH && is_not_in(OPERATORS INTERNAL_OPERATORS, str[i2 - 2]))) had_nonunit = true;
							break;
						}
						i2 = str.rfind(ID_WRAP_LEFT_CH, i4 - 2);
						m_temp = NULL;
						if(i2 != string::npos) {
							int id = s2i(str.substr(i2 + 1, (i4 - 1) - (i2 + 1)));
							if(priv->id_structs.find(id) != priv->id_structs.end()) m_temp = priv->id_structs[id];
						}
						if(m_temp && m_temp->isInteger() && i2 > 3 && (str[i2 - 1] == POWER_CH || str[i2 - 1] == INTERNAL_UPOW_CH) && str[i2 - 2] == ID_WRAP_RIGHT_CH) {
							i4 = i2 - 1;
							i2 = str.rfind(ID_WRAP_LEFT_CH, i4 - 2);
							m_temp = NULL;
							if(i2 != string::npos) {
								int id = s2i(str.substr(i2 + 1, (i4 - 1) - (i2 + 1)));
								if(priv->id_structs.find(id) != priv->id_structs.end()) m_temp = priv->id_structs[id];
							}
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
						} else if(i3 < str.length() - 5 && str[i3 + 3] == ID_WRAP_LEFT_CH && (str[i3 + 1] == POWER_CH || str[i3 + 1] == INTERNAL_UPOW_CH) && is_in(NUMBERS, str[i3 + 2])) {
							b = true;
							i3 += 2;
						} else if(i3 < str.length() - 7 && (str[i3 + 1] == POWER_CH || str[i3 + 1] == INTERNAL_UPOW_CH) && str[i3 + 2] == ID_WRAP_LEFT_CH) {
							size_t i4 = str.find(ID_WRAP_RIGHT, i3 + 3);
							m_temp2 = NULL;
							if(i4 != string::npos && i4 < str.length() - 3 && str[i4 + 1] == ID_WRAP_LEFT_CH) {
								int id = s2i(str.substr(i3 + 3, i4 - (i3 + 3)));
								if(priv->id_structs.find(id) != priv->id_structs.end()) m_temp2 = priv->id_structs[id];
							}
							if(m_temp2 && m_temp2->isInteger()) {
								b = true;
								i3 = i4;
							}
						}
					}
					b = had_unit;
					if(b) {
						if(i3 < str.length() - 2 && (str[i3 + 1] == POWER_CH || str[i3 + 1] == INTERNAL_UPOW_CH) && is_in(NUMBERS, str[i3 + 2])) {
							i3 += 2;
							while(i3 < str.length() - 1 && is_in(NUMBERS, str[i3 + 1])) i3++;
						} else if(i3 < str.length() - 4 && (str[i3 + 1] == POWER_CH || str[i3 + 1] == INTERNAL_UPOW_CH) && str[i3 + 2] == ID_WRAP_LEFT_CH) {
							size_t i4 = str.find(ID_WRAP_RIGHT, i3 + 3);
							m_temp2 = NULL;
							if(i4 != string::npos) {
								int id = s2i(str.substr(i3 + 3, i4 - (i3 + 3)));
								if(priv->id_structs.find(id) != priv->id_structs.end()) m_temp2 = priv->id_structs[id];
							}
							if(m_temp2 && m_temp2->isInteger()) {
								i3 = i4;
							}
						}
						if(i3 == str.length() - 1 || (str[i3 + 1] != POWER_CH && str[i3 + 1] != INTERNAL_UPOW_CH && str[i3 + 1] != DIVISION_CH)) {
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
					i2 = str.find_last_not_of(BASE_2_10 ? NUMBERS INTERNAL_NUMBER_CHARS PLUS MINUS EXPS : NUMBERS INTERNAL_NUMBER_CHARS PLUS MINUS, i - 1);
					if(i2 == string::npos || (i2 != i - 1 && str[i2] == MULTIPLICATION_2_CH)) b = true;
					i2 = str.rfind(MULTIPLICATION_2_CH, i - 1);
					if(i2 == string::npos) b = true;
					if(b) {
						i3 = str.find_first_of(MULTIPLICATION_2 "%" MULTIPLICATION DIVISION, i + 1);
						if(i3 == string::npos || i3 == i + 1 || str[i3] != MULTIPLICATION_2_CH) b = false;
						if(i3 < str.length() + 1 && (str[i3 + 1] == '%' || str[i3 + 1] == DIVISION_CH || str[i3 + 1] == MULTIPLICATION_CH || str[i3 + 1] == POWER_CH || str[i3 + 1] == INTERNAL_UPOW_CH)) b = false;
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
		if(PARSING_MODE == PARSING_MODE_ADAPTIVE) remove_blanks(str);

		// In conventional parsing mode there is not difference between implicit and explicit multiplication
		if(PARSING_MODE >= PARSING_MODE_CONVENTIONAL) {
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

		// Parse parallel operator
		if((i = str.find('\x14', 1)) != string::npos && i + 1 != str.length()) {
			bool b = false;
			while(i != string::npos && i + 1 != str.length()) {
				str2 = str.substr(0, i);
				str = str.substr(i + 1, str.length() - (i + 1));
				if(b) {
					MathStructure *mstruct2 = new MathStructure();
					parseAdd(str2, mstruct2, po);
					mstruct->addChild_nocopy(mstruct2);
				} else {
					parseAdd(str2, mstruct, po);
					mstruct->transform(priv->f_parallel);
					b = true;
				}
				i = str.find('\x14', 1);
			}
			if(b) {
				MathStructure *mstruct2 = new MathStructure();
				parseAdd(str, mstruct2, po);
				mstruct->addChild_nocopy(mstruct2);
			} else {
				parseAdd(str, mstruct, po);
				mstruct->transform(priv->f_parallel);
			}
			return true;
		}

		// Parse explicit multiplication, division, and mod
		if((i = str.find_first_of(MULTIPLICATION DIVISION "%", 0)) != string::npos && i + 1 != str.length()) {
			bool b = false, append = false;
			int type = 0;
			while(i != string::npos && i + 1 != str.length()) {
				if(i < 1) {
					if(str.find_first_not_of(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS EXPS) == string::npos) {
						replace_internal_operators(str);
						error(false, _("Misplaced operator(s) \"%s\" ignored"), str.c_str(), NULL);
						return b;
					}
					i = 1;
					while(i < str.length() && is_in(MULTIPLICATION DIVISION "%", str[i])) {
						i++;
					}
					string errstr = str.substr(0, i);
					replace_internal_operators(errstr);
					error(false, _("Misplaced operator(s) \"%s\" ignored"), errstr.c_str(), NULL);
					str = str.substr(i, str.length() - i);
					i = str.find_first_of(MULTIPLICATION DIVISION "%", 0);
				} else {
					str2 = str.substr(0, i);
					if(b) {
						switch(type) {
							case 1: {
								parseAdd(str2, mstruct, po, OPERATION_DIVIDE, append);
								if(PARSING_MODE == PARSING_MODE_ADAPTIVE && !str2.empty() && str2[0] != LEFT_PARENTHESIS_CH) {
									MathStructure *mden = NULL, *mnum = NULL;
									if(po.preserve_format && mstruct->isDivision()) {
										mden = &(*mstruct)[1];
										mnum = &(*mstruct)[0];
									} else if(!po.preserve_format && mstruct->isMultiplication() && mstruct->size() >= 2 && mstruct->last().isPower()) {
										mden = &mstruct->last()[0];
										mnum = &(*mstruct)[mstruct->size() - 2];
									}
									while(mnum && (mnum->isNegate() || mnum->isAddition() || mnum->isMultiplication()) && mnum->size() > 0) mnum = &mnum->last();
									if(mden && mden->isMultiplication() && (mden->size() != 2 || !(*mden)[0].isNumber() || !(*mden)[1].isUnit_exp() || !mnum->isUnit_exp())) {
										bool b_warn = str2[0] != ID_WRAP_LEFT_CH;
										if(!b_warn && str2.length() > 2) {
											size_t i3 = str2.find_first_not_of(NUMBERS, 1);
											b_warn = (i3 != string::npos && i3 != str2.length() - 1);
										}
										if(b_warn) error(false, MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION, _("The expression is ambiguous (be careful when combining implicit multiplication and division)."), NULL);
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
						string errstr = str.substr(i + 1, i2);
						replace_internal_operators(errstr);
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
						if(PARSING_MODE == PARSING_MODE_ADAPTIVE && !str.empty() && str[0] != LEFT_PARENTHESIS_CH) {
							MathStructure *mden = NULL, *mnum = NULL;
							if(po.preserve_format && mstruct->isDivision()) {
								mden = &(*mstruct)[1];
								mnum = &(*mstruct)[0];
							} else if(!po.preserve_format && mstruct->isMultiplication() && mstruct->size() >= 2 && mstruct->last().isPower()) {
								mden = &mstruct->last()[0];
								mnum = &(*mstruct)[mstruct->size() - 2];
							}
							while(mnum && (mnum->isNegate() || mnum->isAddition() || mnum->isMultiplication()) && mnum->size() > 0) mnum = &mnum->last();
							if(mden && mden->isMultiplication() && (mden->size() != 2 || !(*mden)[0].isNumber() || !(*mden)[1].isUnit_exp() || !mnum->isUnit_exp())) {
								bool b_warn = str[0] != ID_WRAP_LEFT_CH;
								if(!b_warn && str.length() > 2) {
									size_t i3 = str.find_first_not_of(NUMBERS, 1);
									b_warn = (i3 != string::npos && i3 != str.length() - 1);
								}
								if(b_warn) error(false, MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION, _("The expression is ambiguous (be careful when combining implicit multiplication and division)."), NULL);
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

	// Parse internal operators dot product and element-wise functions
	if((i = str.find_first_of("\x15\x16\x17\x18", 1)) != string::npos && i + 1 != str.length()) {
		str2 = str.substr(0, i);
		char op = str[i];
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		MathStructure *mstruct2 = new MathStructure();
		parseAdd(str, mstruct2, po);
		if(op == '\x15') {
			MathFunction *f = getActiveFunction("cross");
			if(f) mstruct->transform(f);
			else mstruct->transform(STRUCT_MULTIPLICATION);
			mstruct->addChild_nocopy(mstruct2);
		} else if(op == '\x17') {
			mstruct->transform_nocopy(STRUCT_VECTOR, mstruct2);
			mstruct->transform(priv->f_times);
		} else {
			if(op == '\x18') mstruct->transform(priv->f_rdivide);
			else mstruct->transform(priv->f_dot);
			mstruct->addChild_nocopy(mstruct2);
		}
		return true;
	}

	// Parse \x1c (internal single substitution character for angle operator) for complex angle format
	if((i = str.find('\x1c', 0)) != string::npos && i + 1 != str.length() && (PARSING_MODE != PARSING_MODE_CHAIN || i == 0)) {
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
		} else if(is_in(OPERATORS INTERNAL_OPERATORS_TWO, str[i]) && str[i] != '\x19' && (po.base != BASE_ROMAN_NUMERALS || (str[i] != '(' && str[i] != ')' && str[i] != '|'))) {
			error(false, _("Misplaced operator(s) \"%s\" ignored"), internal_operator_replacement(str[i]).c_str(), NULL);
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
	if((i = str.find_first_of(ID_WRAPS, 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		while(i != string::npos && i + 1 != str.length()) {
			if(str[i] == ID_WRAP_RIGHT_CH && str[i + 1] != POWER_CH && str[i + 1] != INTERNAL_UPOW_CH && str[i + 1] != '\x19' && str[i + 1] != '\x1a' && str[i + 1] != '\b') {
				str2 = str.substr(0, i + 1);
				str = str.substr(i + 1, str.length() - (i + 1));
				if(b) {
					parseAdd(str2, mstruct, po, OPERATION_MULTIPLY, append);
					append = true;
				} else {
					parseAdd(str2, mstruct, po);
					b = true;
				}
				i = 0;
			} else if(str[i] == ID_WRAP_LEFT_CH && str[i - 1] != POWER_CH && str[i - 1] != INTERNAL_UPOW_CH && str[i - 1] != '\x19' && str[i - 1] != '\x1a' && (i < 2 || str[i - 1] != MINUS_CH || (str[i - 2] != POWER_CH && str[i - 2] != '\x19' && str[i - 2] != '\x1a')) && str[i - 1] != '\b') {
				str2 = str.substr(0, i);
				str = str.substr(i, str.length() - i);
				if(b) {
					parseAdd(str2, mstruct, po, OPERATION_MULTIPLY, append);
					append = true;
				} else {
					parseAdd(str2, mstruct, po);
					b = true;
				}
				i = 0;
			}
			i = str.find_first_of(ID_WRAPS, i + 1);
		}
		if(b) {
			parseAdd(str, mstruct, po, OPERATION_MULTIPLY, append);
			if(mstruct->isMultiplication() && mstruct->size() >= 4 && mstruct->size() % 2 == 0 && !(*mstruct)[0].isUnit_exp() && (*mstruct)[1].isUnit()) {
				bool b_plus = false;
				// Parse 5m 2cm as 5m+2cm, 5ft 2in as 5ft+2in, and similar
				Unit *u1 = (*mstruct)[1].unit(); Prefix *p1 = (*mstruct)[1].prefix();
				if(u1 && u1->subtype() == SUBTYPE_BASE_UNIT && (u1->referenceName() == "m" || (!p1 && u1->referenceName() == "L")) && (!p1 || (p1->type() == PREFIX_DECIMAL && ((DecimalPrefix*) p1)->exponent() <= 3 && ((DecimalPrefix*) p1)->exponent() > -3))) {
					b_plus = true;
					for(size_t i2 = 3; i2 < mstruct->size(); i2 += 2) {
						if(!(*mstruct)[i2 - 1].isUnit_exp() && (*mstruct)[i2].isUnit() && (*mstruct)[i2].unit() == u1) {
							Prefix *p2 = (*mstruct)[i2].prefix();
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
					for(size_t i2 = 3; i2 < mstruct->size(); i2 += 2) {
						if(!(*mstruct)[i2 - 1].isUnit_exp() && (*mstruct)[i2].isUnit() && u1->isChildOf((*mstruct)[i2].unit()) && !(*mstruct)[i2].prefix() && (i2 == mstruct->size() - 1 || ((*mstruct)[i2].unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) (*mstruct)[i2].unit())->mixWithBase()))) {
							while(((AliasUnit*) u1)->firstBaseUnit() != (*mstruct)[i2].unit()) {
								u1 = ((AliasUnit*) u1)->firstBaseUnit();
								if(u1->subtype() != SUBTYPE_ALIAS_UNIT || !((AliasUnit*) u1)->mixWithBase()) {
									b_plus = false;
									break;
								}
							}
							if(!b_plus) break;
							u1 = (*mstruct)[i2].unit();
						} else {
							b_plus = false;
							break;
						}
					}
				}
				if(b_plus) {
					for(size_t i = 1; i < mstruct->size(); i++) {
						(*mstruct)[i].ref();
						(*mstruct)[i - 1].transform_nocopy(STRUCT_MULTIPLICATION, &(*mstruct)[i]);
						mstruct->delChild(i + 1);
					}
					mstruct->setType(STRUCT_ADDITION);
				}
			}
			if(po.preserve_format) {
				if(minus_count > 0 && mstruct->isMultiplication() && mstruct->size() > 0 && !(*mstruct)[0].isUnit_exp()) {
					while(minus_count > 0) {
						(*mstruct)[0].transform(STRUCT_NEGATE);
						minus_count--;
					}
				} else {
					while(minus_count > 0) {
						mstruct->transform(STRUCT_NEGATE);
						minus_count--;
					}
				}
			} else if(minus_count % 2 == 1) {
				if(mstruct->isMultiplication() && mstruct->size() > 0 && !(*mstruct)[0].isUnit_exp()) {
					(*mstruct)[0].negate();
				} else {
					mstruct->negate();
				}
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
	} else if((i = str.find('\x19', 1)) != string::npos && i + 1 != str.length()) {
		// Parse element-wise exponentiation
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		MathStructure *mstruct2 = new MathStructure();
		parseAdd(str, mstruct2, po);
		mstruct->transform(priv->f_power);
		mstruct->addChild_nocopy(mstruct2);
	} else if(!str.empty() && str[str.length() - 1] == '\x1a') {
		str.erase(str.length() - 1, 1);
		parseAdd(str, mstruct, po);
		mstruct->transform(f_transpose);
	} else if((i = str.find("\b", 1)) != string::npos) {
		// Parse uncertainty (using \b as internal single substitution character for +/-)
		str2 = str.substr(0, i);
		MathStructure *mstruct2 = new MathStructure;
		if(po.read_precision != DONT_READ_PRECISION) {
			ParseOptions po2 = po;
			po2.read_precision = DONT_READ_PRECISION;
			parseAdd(str2, mstruct, po2);
			if(i + 1 != str.length()) {
				str = str.substr(i + 1, str.length() - (i + 1));
				parseAdd(str, mstruct2, po2);
			}
		} else {
			parseAdd(str2, mstruct, po);
			if(i + 1 != str.length()) {
				str = str.substr(i + 1, str.length() - (i + 1));
				parseAdd(str, mstruct2, po);
			}
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
	} else if((i = str.find(INTERNAL_UPOW_CH, 1)) != string::npos && i + 1 != str.length()) {
		// Parse exponentiation (^)
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_RAISE);
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

