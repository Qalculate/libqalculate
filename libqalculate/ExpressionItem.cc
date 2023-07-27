/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "ExpressionItem.h"
#include "Calculator.h"
#include "util.h"

using std::string;
using std::vector;

ExpressionName::ExpressionName(string sname) : suffix(false), unicode(false), plural(false), reference(false), avoid_input(false), completion_only(false) {
	name = sname;
	if(text_length_is_one(sname)) {
		abbreviation = true;
		case_sensitive = true;
	} else {
		abbreviation = false;
		case_sensitive = false;
	}
	for(size_t i = 0; i < sname.length(); i++) {
		if((unsigned char) sname[i] >= 0xC0) {
			unicode = true;
			break;
		}
	}
	if(sname.length() > 2) {
		size_t i = sname.find('_', 1);
		if(i != string::npos && i < sname.length() - 1 && sname.find('_', i + 1) == string::npos) {
			suffix = true;
			if(i == 1) {
				abbreviation = true;
				case_sensitive = true;
			}
		}
	}
	if(!case_sensitive && !suffix) {
		for(size_t i = 1; i < sname.length(); i++) {
			if((signed char) sname[i] > 0 || (unsigned char) sname[i] >= 0xC0) {
				if(sname.find_first_not_of(NUMBERS, i) == string::npos) {
					suffix = true;
					abbreviation = true;
					case_sensitive = true;
				}
				break;
			}
		}
	}
}
ExpressionName::ExpressionName() : abbreviation(false), suffix(false), unicode(false), plural(false), reference(false), avoid_input(false), case_sensitive(false), completion_only(false) {
}
void ExpressionName::operator = (const ExpressionName &ename) {
	name = ename.name;
	abbreviation = ename.abbreviation;
	case_sensitive = ename.case_sensitive;
	suffix = ename.suffix;
	unicode = ename.unicode;
	plural = ename.plural;
	reference = ename.reference;
	avoid_input = ename.avoid_input;
	completion_only = ename.completion_only;
}
bool ExpressionName::operator == (const ExpressionName &ename) const {
	return name == ename.name && abbreviation == ename.abbreviation && case_sensitive == ename.case_sensitive && suffix == ename.suffix && unicode == ename.unicode && plural == ename.plural && reference == ename.reference && avoid_input == ename.avoid_input && completion_only == ename.completion_only;
}
bool ExpressionName::operator != (const ExpressionName &ename) const {
	return name != ename.name || abbreviation != ename.abbreviation || case_sensitive != ename.case_sensitive || suffix != ename.suffix || unicode != ename.unicode || plural != ename.plural || reference != ename.reference || avoid_input != ename.avoid_input || completion_only != ename.completion_only;
}
int ExpressionName::underscoreRemovalAllowed() const {
	if(completion_only) return 0;
	size_t i = name.find('_', 1);
	size_t i_us = 0;
	while(true) {
		if(i == string::npos) {
			break;
		} else if(i == name.length() - 1 || name[i - 1] == '_' || (i == name.length() - 2 && (name[name.length() - 1] < '0' || name[name.length() - 1] > '9') && ((signed char) name[i - 1] >= 0 || CALCULATOR->getPrefix(name.substr(0, i))))) {
			i_us = 0;
			break;
		}
		i_us++;
		i = name.find('_', i + 1);
	}
	return i_us;
}
bool sub_suffix_unicode(string &name, int n) {
	if(n == 0) return false;
	size_t i = name.rfind('_');
	bool b = (i == string::npos || i == name.length() - 1 || i == 0);
	size_t i2 = 0;
	if(b) {
		if(is_in(NUMBERS, name[name.length() - 1])) {
			i2++;
			while(name.length() > i2 + 1 && is_in(NUMBERS, name[name.length() - 1 - i2])) {
				i2++;
			}
		}
	} else {
		if(name.find_first_not_of(NUMBERS, i + 1) != string::npos) return false;
		i2 = name.length() - i - 1;
	}
	if(i2 == 0 || (n > 0 && i2 > (size_t) n)) return false;
	if(!b) name.erase(i, 1);
	string ssub = "\xe2\x82\x80";
	while(i2 > 0) {
		ssub[2] = '\x80' + (name[name.length() - i2] - '0');
		name.replace(name.length() - i2, 1, ssub);
		i2--;
	}
	return true;
}
string ExpressionName::formattedName(int type, bool capitalize, bool html_suffix, int unicode_suffix, bool remove_typename, bool hide_underscore, bool *was_formatted, bool *was_capitalized) const {
	if(was_formatted) *was_formatted = false;
	if(was_capitalized) *was_capitalized = false;
	if(name.length() < 2) return name;
	string str = name;
	if(suffix && (unicode_suffix || html_suffix || remove_typename)) {
		size_t i = string::npos;
		if(remove_typename && (type == TYPE_VARIABLE || type == TYPE_UNIT)) i = name.rfind('_');
		size_t l = 4;
		if(type == TYPE_VARIABLE) l = 8;
		if(i != string::npos && i + l + 1 <= name.length() && ((type == TYPE_VARIABLE && name.substr(name.length() - l, l) == "constant" && CALCULATOR->getActiveUnit(name.substr(0, i + l + 1 == name.length() ? i : name.length() - l))) || (type == TYPE_UNIT && name.substr(name.length() - l, l) == "unit" && CALCULATOR->getActiveVariable(name.substr(0, i + l + 1 == name.length() ? i : name.length() - l))))) {
			if(i + l + 1 == name.length()) {
				str = name.substr(0, i);
			} else {
				str = str.substr(0, name.length() - l);
				if((!sub_suffix_unicode(str, unicode_suffix)) && html_suffix) str = sub_suffix_html(str);
				}
			if(was_formatted) *was_formatted = true;
		} else if(sub_suffix_unicode(str, unicode_suffix)) {
			if(was_formatted) *was_formatted = true;
		} else if(html_suffix) {
			str = sub_suffix_html(name);
			if(was_formatted) *was_formatted = true;
		}
	}
	if(type < 0 || !capitalize || completion_only || case_sensitive || str.length() <= 4) {
		if(hide_underscore && str.find('_') != string::npos) {if(was_formatted) *was_formatted = true; gsub("_", " ", str);}
		return str;
	}
	size_t i = str.find('_', 1);
	if(i == string::npos || unicode_length(str, i) < 3 || str[i - 1] == '_') {
		if(hide_underscore && i != string::npos) {if(was_formatted) *was_formatted = true; gsub("_", " ", str);}
		return str;
	}
	bool b_first = true;
	string str_bak = str;
	while(true) {
		if(i == string::npos) break;
		if(i == str.length() - 1 || str[i + 1] == '_' || (i == str.length() - 2 && (str[str.length() - 1] < '0' || str[str.length() - 1] > '9') && ((signed char) str[i - 1] >= 0 || CALCULATOR->getPrefix(str.substr(0, i))))) {
			if(hide_underscore) {if(was_formatted) *was_formatted = true; gsub("_", " ", str_bak);}
			return str_bak;
		}
		str.erase(i, 1);
		if(str[i] >= 'a' && str[i] <= 'z') {
			str[i] -= ('a' - 'A');
		} else if((unsigned char) str[i] >= 0xC0) {
			size_t l = 1;
			while(i + l < str.length() && (signed char) str[i + l] < 0 && (unsigned char) str[i + l] < 0xC0) l++;
			char *strup = utf8_strup(str.c_str() + (sizeof(char) * i), l);
			if(strup) {
				str.replace(i, l, strup);
				free(strup);
			} else {
				if(hide_underscore) {if(was_formatted) *was_formatted = true; gsub("_", " ", str_bak);}
				return str_bak;
			}
		}
		if(b_first && type != TYPE_FUNCTION) {
			if(str[0] >= 'a' && str[0] <= 'z') {
				str[0] -= ('a' - 'A');
			} else if((unsigned char) str[0] >= 0xC0) {
				size_t l = 1;
				while(l < str.length() && (signed char) str[l] < 0 && (unsigned char) str[l] < 0xC0) l++;
				char *strup = utf8_strup(str.c_str(), l);
				if(strup) {
					str.replace(0, l, strup);
					free(strup);
				} else {
					if(hide_underscore) {if(was_formatted) *was_formatted = true; gsub("_", " ", str_bak);}
					return str_bak;
				}
			}
		}
		b_first = false;
		i = str.find('_', i + 1);
	}
	if(hide_underscore && !suffix) gsub("_", " ", str);
	if(was_formatted) *was_formatted = true;
	if(was_capitalized) *was_capitalized = true;
	return str;
}

ExpressionItem::ExpressionItem(string cat_, string name_, string title_, string descr_, bool is_local, bool is_builtin, bool is_active) {

	b_local = is_local;
	b_builtin = is_builtin;
	remove_blank_ends(name_);
	remove_blank_ends(cat_);
	remove_blank_ends(title_);

	if(!name_.empty()) {
		names.push_back(ExpressionName(name_));
	}

	stitle = title_;
	scat = cat_;
	sdescr = descr_;
	b_changed = false;
	b_approx = false;
	i_precision = -1;
	b_active = is_active;
	b_registered = false;
	b_hidden = false;
	b_destroyed = false;
	i_ref = 0;

}
ExpressionItem::ExpressionItem() {
	b_changed = false;
	b_approx = false;
	i_precision = -1;
	b_active = true;
	b_local = true;
	b_builtin = false;
	b_registered = false;
	b_hidden = false;
	b_destroyed = false;
	i_ref = 0;
}
ExpressionItem::~ExpressionItem() {
}
void ExpressionItem::set(const ExpressionItem *item) {
	b_changed = item->hasChanged();
	b_approx = item->isApproximate();
	i_precision = item->precision();
	b_active = item->isActive();
	for(size_t i = 1; i <= item->countNames(); i++) {
		names.push_back(item->getName(1));
	}
	stitle = item->title(false);
	scat = item->category();
	sdescr = item->description();
	b_local = item->isLocal();
	b_builtin = item->isBuiltin();
	b_hidden = item->isHidden();
}
bool ExpressionItem::destroy() {
	if(b_registered) CALCULATOR->expressionItemDeleted(this);
	if(v_refs.size() > 0) {
		return false;
	} else if(i_ref > 0) {
		b_destroyed = true;
	} else {
		delete this;
	}
	return true;
}
bool ExpressionItem::isRegistered() const {
	return b_registered;
}
void ExpressionItem::setRegistered(bool is_registered) {
	b_registered = is_registered;
}
const string &ExpressionItem::title(bool return_name_if_no_title, bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	if(return_name_if_no_title && stitle.empty()) {
		return preferredName(false, use_unicode, false, false, can_display_unicode_string_function, can_display_unicode_string_arg).name;
	}
	return stitle;
}
void ExpressionItem::setTitle(string title_) {
	remove_blank_ends(title_);
	if(stitle != title_) {
		stitle = title_;
		b_changed = true;
	}
}
const string &ExpressionItem::description() const {
	return sdescr;
}
void ExpressionItem::setDescription(string descr_) {
	remove_blank_ends(descr_);
	if(sdescr != descr_) {
		sdescr = descr_;
		b_changed = true;
	}
}
const string &ExpressionItem::name(bool use_unicode, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	bool undisplayable_uni = false;
	for(size_t i = 0; i < names.size(); i++) {
		if(names[i].unicode == use_unicode && (!names[i].completion_only || i + 1 == names.size())) {
			if(use_unicode && can_display_unicode_string_function && !((*can_display_unicode_string_function) (names[i].name.c_str(), can_display_unicode_string_arg))) {
				undisplayable_uni = true;
			} else {
				return names[i].name;
			}
		}
	}
	if(undisplayable_uni) return name(false);
	if(names.size() > 0) return names[0].name;
	return empty_string;
}
const string &ExpressionItem::referenceName() const {
	for(size_t i = 0; i < names.size(); i++) {
		if(names[i].reference) {
			return names[i].name;
		}
	}
	if(names.size() > 0) return names[0].name;
	return empty_string;
}

const ExpressionName &ExpressionItem::preferredName(bool abbreviation, bool use_unicode, bool plural, bool reference, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
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
	if(use_unicode && index >= 0 && names[index].unicode && can_display_unicode_string_function && !((*can_display_unicode_string_function) (names[index].name.c_str(), can_display_unicode_string_arg))) {
		return preferredName(abbreviation, false, plural, reference, can_display_unicode_string_function, can_display_unicode_string_arg);
	}
	if(index >= 0) return names[index];
	return empty_expression_name;
}
const ExpressionName &ExpressionItem::preferredInputName(bool abbreviation, bool use_unicode, bool plural, bool reference, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	if(names.size() == 1) return names[0];
	int index = -1;
	for(size_t i = 0; i < names.size(); i++) {
		if((!reference || names[i].reference) && names[i].abbreviation == abbreviation && names[i].unicode == use_unicode && names[i].plural == plural && !names[i].avoid_input && !names[i].completion_only) return names[i];
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
	if(use_unicode && index >= 0 && names[index].unicode && can_display_unicode_string_function && !((*can_display_unicode_string_function) (names[index].name.c_str(), can_display_unicode_string_arg))) {
		return preferredInputName(abbreviation, false, plural, reference, can_display_unicode_string_function, can_display_unicode_string_arg);
	}
	if(index >= 0) return names[index];
	return empty_expression_name;
}
const ExpressionName &ExpressionItem::preferredDisplayName(bool abbreviation, bool use_unicode, bool plural, bool reference, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	return preferredName(abbreviation, use_unicode, plural, reference, can_display_unicode_string_function, can_display_unicode_string_arg);
}
const ExpressionName &ExpressionItem::getName(size_t index) const {
	if(index > 0 && index <= names.size()) return names[index - 1];
	return empty_expression_name;
}
void ExpressionItem::setName(const ExpressionName &ename, size_t index, bool force) {
	if(index < 1) addName(ename, 1);
	if(index > names.size()) addName(ename);
	if(b_registered && names[index - 1].name != ename.name) {
		names[index - 1] = ename;
		names[index - 1].name = CALCULATOR->getName(ename.name, this, force);
		b_changed = true;
		CALCULATOR->nameChanged(this);
	} else if(ename != names[index - 1]) {
		names[index - 1] = ename;
		b_changed = true;
	}
}
void ExpressionItem::setName(string sname, size_t index, bool force) {
	if(index < 1) addName(sname, 1);
	if(index > names.size()) addName(sname);
	if(b_registered && names[index - 1].name != sname) {
		names[index - 1].name = CALCULATOR->getName(sname, this, force);
		b_changed = true;
		CALCULATOR->nameChanged(this);
	} else if(sname != names[index - 1].name) {
		names[index - 1].name = sname;
		b_changed = true;
	}
}
void ExpressionItem::addName(const ExpressionName &ename, size_t index, bool force) {
	if(index < 1 || index > names.size()) {
		names.push_back(ename);
		index = names.size();
	} else {
		names.insert(names.begin() + (index - 1), ename);
	}
	if(b_registered) {
		names[index - 1].name = CALCULATOR->getName(names[index - 1].name, this, force);
		CALCULATOR->nameChanged(this);
	}
	b_changed = true;
}
void ExpressionItem::addName(string sname, size_t index, bool force) {
	if(index < 1 || index > names.size()) {
		names.push_back(ExpressionName(sname));
		index = names.size();
	} else {
		names.insert(names.begin() + (index - 1), ExpressionName(sname));
	}
	if(b_registered) {
		names[index - 1].name = CALCULATOR->getName(names[index - 1].name, this, force);
		CALCULATOR->nameChanged(this);
	}
	b_changed = true;
}
size_t ExpressionItem::countNames() const {
	return names.size();
}
void ExpressionItem::clearNames() {
	if(names.size() > 0) {
		names.clear();
		if(b_registered) {
			CALCULATOR->nameChanged(this);
		}
		b_changed = true;
	}
}
void ExpressionItem::clearNonReferenceNames() {
	bool b = false;
	for(vector<ExpressionName>::iterator it = names.begin(); it != names.end(); ++it) {
		if(!it->reference) {
			it = names.erase(it);
			--it;
			b = true;
		}
	}
	if(b) {
		if(b_registered) {
			CALCULATOR->nameChanged(this);
		}
		b_changed = true;
	}
}
void ExpressionItem::removeName(size_t index) {
	if(index > 0 && index <= names.size()) {
		names.erase(names.begin() + (index - 1));
		if(b_registered) {
			CALCULATOR->nameChanged(this);
		}
		b_changed = true;
	}
}
size_t ExpressionItem::hasName(const string &sname, bool case_sensitive) const {
	for(size_t i = 0; i < names.size(); i++) {
		if(case_sensitive && names[i].case_sensitive && sname == names[i].name) return i + 1;
		if((!case_sensitive || !names[i].case_sensitive) && equalsIgnoreCase(names[i].name, sname)) return i + 1;
	}
	return 0;
}
size_t ExpressionItem::hasNameCaseSensitive(const string &sname) const {
	for(size_t i = 0; i < names.size(); i++) {
		if(sname == names[i].name) return i + 1;
	}
	return 0;
}
const ExpressionName &ExpressionItem::findName(int abbreviation, int use_unicode, int plural, bool (*can_display_unicode_string_function) (const char*, void*), void *can_display_unicode_string_arg) const {
	for(size_t i = 0; i < names.size(); i++) {
		if((abbreviation < 0 || names[i].abbreviation == abbreviation) && (use_unicode < 0 || names[i].unicode == use_unicode) && (plural < 0 || names[i].plural == plural) && (!names[i].unicode || !can_display_unicode_string_function || ((*can_display_unicode_string_function) (names[i].name.c_str(), can_display_unicode_string_arg)))) return names[i];
	}
	return empty_expression_name;
}

const string &ExpressionItem::category() const {
	return scat;
}
void ExpressionItem::setCategory(string cat_) {
	remove_blank_ends(cat_);
	if(scat != cat_) {
		scat = cat_;
		b_changed = true;
	}
}
bool ExpressionItem::isLocal() const {
	return b_local;
}
bool ExpressionItem::setLocal(bool is_local, int will_be_active) {
	if(b_builtin) return false;
	if(is_local != b_local) {
		if(!b_local) {
			bool was_active = b_active;
			b_active = false;
			ExpressionItem *item = copy();
			b_local = is_local;
			b_active = was_active;
			if(will_be_active) {
				setActive(true);
			} else if(will_be_active == 0) {
				setActive(false);
			}
			CALCULATOR->addExpressionItem(item);
			if(was_active != item->isActive()) {
				item->setChanged(true);
			}
			if(was_active && will_be_active == 0) {
				item->setActive(true);
			}
		}
		b_local = is_local;
		if(type() == TYPE_UNIT) {
			for(vector<Unit*>::iterator it = CALCULATOR->units.begin(); it != CALCULATOR->units.end(); ++it) {
				if(*it == (Unit*) this) {
					CALCULATOR->units.erase(it);
					CALCULATOR->units.push_back((Unit*) this);
					break;
				}
			}
		}
	} else if(will_be_active >= 0) {
		setActive(will_be_active);
	}
	return true;
}
bool ExpressionItem::isBuiltin() const {
	return b_builtin;
}
bool ExpressionItem::hasChanged() const {
	return b_changed;
}
void ExpressionItem::setChanged(bool has_changed) {
	b_changed = has_changed;
}
bool ExpressionItem::isApproximate() const {
	return b_approx;
}
void ExpressionItem::setApproximate(bool is_approx) {
	if(is_approx != b_approx) {
		b_approx = is_approx;
		if(!b_approx) i_precision = -1;
		b_changed = true;
	}
}
int ExpressionItem::precision() const {
	return i_precision;
}
void ExpressionItem::setPrecision(int prec) {
	if(i_precision != prec) {
		i_precision = prec;
		if(i_precision >= 0) b_approx = true;
		b_changed = true;
	}
}
bool ExpressionItem::isActive() const {
	return b_active;
}
void ExpressionItem::setActive(bool is_active) {
	if(is_active != b_active) {
		b_active = is_active;
		if(b_registered) {
			if(is_active) {
				CALCULATOR->expressionItemActivated(this);
			} else {
				CALCULATOR->expressionItemDeactivated(this);
			}
		}
		b_changed = true;
	}
}
bool ExpressionItem::isHidden() const {
	return b_hidden;
}
void ExpressionItem::setHidden(bool is_hidden) {
	if(is_hidden != b_hidden) {
		b_hidden = is_hidden;
		b_changed = true;
	}
}

int ExpressionItem::refcount() const {
	return i_ref;
}
void ExpressionItem::ref() {
	i_ref++;
}
void ExpressionItem::unref() {
	i_ref--;
	if(b_destroyed && i_ref <= 0) {
		delete this;
	}
}
void ExpressionItem::ref(ExpressionItem *o) {
	i_ref++;
	v_refs.push_back(o);
}
void ExpressionItem::unref(ExpressionItem *o) {
	for(size_t i = 0; i < v_refs.size(); i++) {
		if(v_refs[i] == o) {
			i_ref--;
			v_refs.erase(v_refs.begin() + i);
			break;
		}
	}
}
ExpressionItem *ExpressionItem::getReferencer(size_t index) const {
	if(index > 0 && index <= v_refs.size()) {
		return v_refs[index - 1];
	}
	return NULL;
}
bool ExpressionItem::changeReference(ExpressionItem*, ExpressionItem*) {
	return false;
}
int ExpressionItem::id() const {
	return 0;
}
