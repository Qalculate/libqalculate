/*
    Qalculate (library)

    Copyright (C) 2003-2006, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Prefix.h"
#include "Calculator.h"
#include "Number.h"

using std::string;
using std::vector;

Prefix::Prefix(string long_name, string short_name, string unicode_name) {
	if(!unicode_name.empty()) {
		names.push_back(unicode_name);
		names[names.size() - 1].abbreviation = true;
		names[names.size() - 1].unicode = true;
		names[names.size() - 1].case_sensitive = true;
	}
	if(!short_name.empty()) {
		names.push_back(short_name);
		names[names.size() - 1].abbreviation = true;
		names[names.size() - 1].case_sensitive = true;
	}
	if(!long_name.empty()) {
		names.push_back(long_name);
		names[names.size() - 1].abbreviation = false;
		names[names.size() - 1].case_sensitive = false;
	}
}
Prefix::~Prefix() {
}
const string &Prefix::shortName(bool return_long_if_no_short, bool use_unicode) const {
	const ExpressionName &ename = preferredName(true, use_unicode);
	if(!return_long_if_no_short && !ename.abbreviation) return empty_string;
	return ename.name;
}
const string &Prefix::longName(bool return_short_if_no_long, bool use_unicode) const {
	const ExpressionName &ename = preferredName(false, use_unicode);
	if(!return_short_if_no_long && ename.abbreviation) return empty_string;
	return ename.name;
}
const string &Prefix::unicodeName(bool return_short_if_no_unicode) const {
	const ExpressionName &ename = preferredName(true, true);
	if(!return_short_if_no_unicode && !ename.unicode) return empty_string;
	return ename.name;
}
void Prefix::setShortName(string short_name) {
	for(size_t i = 0; i < names.size(); i++) {
		if(names[i].abbreviation && !names[i].unicode) {
			if(short_name.empty()) {
				removeName(i + 1);
			} else {
				names[i].name = short_name;
				names[i].case_sensitive = true;
				CALCULATOR->prefixNameChanged(this);
			}
			return;
		}
	}
	if(!short_name.empty()) {
		ExpressionName ename(short_name);
		ename.abbreviation = true;
		ename.case_sensitive = true;
		addName(ename);
	}
}
void Prefix::setLongName(string long_name) {
	for(size_t i = 0; i < names.size(); i++) {
		if(!names[i].abbreviation) {
			if(long_name.empty()) {
				removeName(i + 1);
			} else {
				names[i].name = long_name;
				names[i].case_sensitive = false;
				CALCULATOR->prefixNameChanged(this);
			}
			return;
		}
	}
	if(!long_name.empty()) {
		ExpressionName ename(long_name);
		ename.abbreviation = false;
		ename.case_sensitive = false;
		addName(ename);
	}
}
void Prefix::setUnicodeName(string unicode_name) {
	for(size_t i = 0; i < names.size(); i++) {
		if(names[i].abbreviation && names[i].unicode) {
			if(unicode_name.empty()) {
				removeName(i + 1);
			} else {
				names[i].name = unicode_name;
				names[i].case_sensitive = true;
				CALCULATOR->prefixNameChanged(this);
			}
			return;
		}
	}
	if(!unicode_name.empty()) {
		ExpressionName ename(unicode_name);
		ename.abbreviation = true;
		ename.unicode = true;
		ename.case_sensitive = true;
		addName(ename);
	}
}
const string &Prefix::name(bool short_default, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	return preferredName(short_default, use_unicode, false, false, can_display_unicode_string_function, can_display_unicode_string_arg).name;
}
const string &Prefix::referenceName() const {
	for(size_t i = 0; i < names.size(); i++) {
		if(names[i].reference) {
			return names[i].name;
		}
	}
	if(names.size() > 0) return names[0].name;
	return empty_string;
}

const ExpressionName &Prefix::preferredName(bool abbreviation, bool use_unicode, bool plural, bool reference, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	if(names.size() == 1) return names[0];
	int index = -1;
	for(size_t i = 0; i < names.size(); i++) {
		if((!reference || names[i].reference) && names[i].abbreviation == abbreviation && names[i].unicode == use_unicode && names[i].plural == plural && !names[i].completion_only && (!use_unicode || !can_display_unicode_string_function || (*can_display_unicode_string_function) (names[i].name.c_str(), can_display_unicode_string_arg))) return names[i];
		if(index < 0) {
			index = i;
		} else if(names[i].completion_only != names[index].completion_only) {
			if(!names[i].completion_only) index = i;
		} else if(reference && names[i].reference != names[index].reference) {
			if(names[i].reference) index = i;
		} else if(!use_unicode && names[i].unicode != names[index].unicode) {
			if(!names[i].unicode) index = i;
		} else if(names[i].abbreviation != names[index].abbreviation) {
			if(names[i].abbreviation == abbreviation) index = i;
		} else if(names[i].plural != names[index].plural) {
			if(names[i].plural == plural) index = i;
		} else if(use_unicode && names[i].unicode != names[index].unicode) {
			if(names[i].unicode) index = i;
		}
	}
	if(use_unicode && names[index].unicode && can_display_unicode_string_function && !((*can_display_unicode_string_function) (names[index].name.c_str(), can_display_unicode_string_arg))) {
		return preferredName(abbreviation, false, plural, reference, can_display_unicode_string_function, can_display_unicode_string_arg);
	}
	if(index >= 0) return names[index];
	return empty_expression_name;
}
const ExpressionName &Prefix::preferredInputName(bool abbreviation, bool use_unicode, bool plural, bool reference, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	if(names.size() == 1) return names[0];
	int index = -1;
	for(size_t i = 0; i < names.size(); i++) {
		if((!reference || names[i].reference) && names[i].abbreviation == abbreviation && names[i].unicode == use_unicode && !names[i].avoid_input && !names[i].completion_only) return names[i];
		if(index < 0) {
			index = i;
		} else if(names[i].completion_only != names[index].completion_only) {
			if(!names[i].completion_only) index = i;
		} else if(reference && names[i].reference != names[index].reference) {
			if(names[i].reference) index = i;
		} else if(!use_unicode && names[i].unicode != names[index].unicode) {
			if(!names[i].unicode) index = i;
		} else if(names[i].avoid_input != names[index].avoid_input) {
			if(!names[i].avoid_input) index = i;
		} else if(abbreviation && names[i].abbreviation != names[index].abbreviation) {
			if(names[i].abbreviation) index = i;
		} else if(plural && names[i].plural != names[index].plural) {
			if(names[i].plural) index = i;
		} else if(!abbreviation && names[i].abbreviation != names[index].abbreviation) {
			if(!names[i].abbreviation) index = i;
		} else if(!plural && names[i].plural != names[index].plural) {
			if(!names[i].plural) index = i;
		} else if(use_unicode && names[i].unicode != names[index].unicode) {
			if(names[i].unicode) index = i;
		}
	}
	if(use_unicode && names[index].unicode && can_display_unicode_string_function && !((*can_display_unicode_string_function) (names[index].name.c_str(), can_display_unicode_string_arg))) {
		return preferredInputName(abbreviation, false, plural, reference, can_display_unicode_string_function, can_display_unicode_string_arg);
	}
	if(index >= 0) return names[index];
	return empty_expression_name;
}
const ExpressionName &Prefix::preferredDisplayName(bool abbreviation, bool use_unicode, bool plural, bool reference, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	return preferredName(abbreviation, use_unicode, plural, reference, can_display_unicode_string_function, can_display_unicode_string_arg);
}
const ExpressionName &Prefix::getName(size_t index) const {
	if(index > 0 && index <= names.size()) return names[index - 1];
	return empty_expression_name;
}
void Prefix::setName(const ExpressionName &ename, size_t index) {
	if(index < 1) addName(ename, 1);
	else if(index > names.size()) addName(ename);
	else if(names[index - 1].name != ename.name) {
		names[index - 1] = ename;
		CALCULATOR->prefixNameChanged(this);
	}
}
void Prefix::setName(string sname, size_t index) {
	if(index < 1) addName(sname, 1);
	else if(index > names.size()) addName(sname);
	else if(names[index - 1].name != sname) {
		names[index - 1].name = sname;
		CALCULATOR->prefixNameChanged(this);
	}
}
void Prefix::addName(const ExpressionName &ename, size_t index) {
	if(index < 1 || index > names.size()) {
		names.push_back(ename);
		index = names.size();
	} else {
		names.insert(names.begin() + (index - 1), ename);
	}
	CALCULATOR->prefixNameChanged(this);
}
void Prefix::addName(string sname, size_t index) {
	if(index < 1 || index > names.size()) {
		names.push_back(ExpressionName(sname));
		index = names.size();
	} else {
		names.insert(names.begin() + (index - 1), ExpressionName(sname));
	}
	CALCULATOR->prefixNameChanged(this);
}
size_t Prefix::countNames() const {
	return names.size();
}
void Prefix::clearNames() {
	if(names.size() > 0) {
		names.clear();
		CALCULATOR->prefixNameChanged(this);
	}
}
void Prefix::clearNonReferenceNames() {
	bool b = false;
	for(vector<ExpressionName>::iterator it = names.begin(); it != names.end(); ++it) {
		if(!it->reference) {
			it = names.erase(it);
			--it;
			b = true;
		}
	}
	if(b) {
		CALCULATOR->prefixNameChanged(this);
	}
}
void Prefix::removeName(size_t index) {
	if(index > 0 && index <= names.size()) {
		names.erase(names.begin() + (index - 1));
		CALCULATOR->prefixNameChanged(this);
	}
}
size_t Prefix::hasName(const string &sname, bool case_sensitive) const {
	for(size_t i = 0; i < names.size(); i++) {
		if(case_sensitive && names[i].case_sensitive && sname == names[i].name) return i + 1;
		if((!case_sensitive || !names[i].case_sensitive) && equalsIgnoreCase(names[i].name, sname)) return i + 1;
	}
	return 0;
}
size_t Prefix::hasNameCaseSensitive(const string &sname) const {
	for(size_t i = 0; i < names.size(); i++) {
		if(sname == names[i].name) return i + 1;
	}
	return 0;
}
const ExpressionName &Prefix::findName(int abbreviation, int use_unicode, int plural, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	for(size_t i = 0; i < names.size(); i++) {
		if((abbreviation < 0 || names[i].abbreviation == abbreviation) && (use_unicode < 0 || names[i].unicode == use_unicode) && (plural < 0 || names[i].plural == plural) && (!names[i].unicode || !can_display_unicode_string_function || ((*can_display_unicode_string_function) (names[i].name.c_str(), can_display_unicode_string_arg)))) return names[i];
	}
	return empty_expression_name;
}

DecimalPrefix::DecimalPrefix(int exp10, string long_name, string short_name, string unicode_name) : Prefix(long_name, short_name, unicode_name) {
	exp = exp10;
}
DecimalPrefix::~DecimalPrefix() {
}
int DecimalPrefix::exponent(int iexp) const {
	return exp * iexp;
}
Number DecimalPrefix::exponent(const Number &nexp) const {
	return nexp * exp;
}
void DecimalPrefix::setExponent(int iexp) {
	exp = iexp;
}
Number DecimalPrefix::value(const Number &nexp) const {
	Number nr(exponent(nexp));
	nr.exp10();
	return nr;
}
Number DecimalPrefix::value(int iexp) const {
	Number nr(exponent(iexp));
	nr.exp10();
	return nr;
}
Number DecimalPrefix::value() const {
	Number nr(exp);
	nr.exp10();
	return nr;
}
int DecimalPrefix::type() const {
	return PREFIX_DECIMAL;
}

BinaryPrefix::BinaryPrefix(int exp2, string long_name, string short_name, string unicode_name) : Prefix(long_name, short_name, unicode_name) {
	exp = exp2;
}
BinaryPrefix::~BinaryPrefix() {
}
int BinaryPrefix::exponent(int iexp) const {
	return exp * iexp;
}
Number BinaryPrefix::exponent(const Number &nexp) const {
	return nexp * exp;
}
void BinaryPrefix::setExponent(int iexp) {
	exp = iexp;
}
Number BinaryPrefix::value(const Number &nexp) const {
	Number nr(exponent(nexp));
	nr.exp2();
	return nr;
}
Number BinaryPrefix::value(int iexp) const {
	Number nr(exponent(iexp));
	nr.exp2();
	return nr;
}
Number BinaryPrefix::value() const {
	Number nr(exp);
	nr.exp2();
	return nr;
}
int BinaryPrefix::type() const {
	return PREFIX_BINARY;
}

NumberPrefix::NumberPrefix(const Number &nr, string long_name, string short_name, string unicode_name) : Prefix(long_name, short_name, unicode_name) {
	o_number = nr;
}
NumberPrefix::~NumberPrefix() {
}
void NumberPrefix::setValue(const Number &nr) {
	o_number = nr;
}
Number NumberPrefix::value(const Number &nexp) const {
	Number nr(o_number);
	nr.raise(nexp);
	return nr;
}
Number NumberPrefix::value(int iexp) const {
	Number nr(o_number);
	nr.raise(iexp);
	return nr;
}
Number NumberPrefix::value() const {
	return o_number;
}
int NumberPrefix::type() const {
	return PREFIX_NUMBER;
}
