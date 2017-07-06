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

Prefix::Prefix(string long_name, string short_name, string unicode_name) {
	l_name = long_name;
	s_name = short_name;
	u_name = unicode_name;
}
Prefix::~Prefix() {
}
const string &Prefix::shortName(bool return_long_if_no_short, bool use_unicode) const {
	if(use_unicode && !u_name.empty()) return u_name;
	if(return_long_if_no_short && s_name.empty()) {
		return l_name;
	}
	return s_name;
}
const string &Prefix::longName(bool return_short_if_no_long, bool use_unicode) const {
	if(return_short_if_no_long && l_name.empty()) {		
		if(use_unicode && !u_name.empty()) return u_name;
		return s_name;
	}
	return l_name;
}
const string &Prefix::unicodeName(bool return_short_if_no_unicode) const {
	if(return_short_if_no_unicode && u_name.empty()) {
		return s_name;
	}
	return u_name;
}
void Prefix::setShortName(string short_name) {
	s_name = short_name;
	CALCULATOR->prefixNameChanged(this);	
}
void Prefix::setLongName(string long_name) {
	l_name = long_name;
	CALCULATOR->prefixNameChanged(this);
}
void Prefix::setUnicodeName(string unicode_name) {
	u_name = unicode_name;
	CALCULATOR->prefixNameChanged(this);
}
const string &Prefix::name(bool short_default, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	if(short_default) {
		if(use_unicode && !u_name.empty() && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (u_name.c_str(), can_display_unicode_string_arg))) return u_name;
		if(s_name.empty()) {
			return l_name;
		}
		return s_name;
	}
	if(l_name.empty()) {
		if(use_unicode && !u_name.empty() && (!can_display_unicode_string_function || (*can_display_unicode_string_function) (u_name.c_str(), can_display_unicode_string_arg))) return u_name;
		return s_name;
	}
	return l_name;
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
