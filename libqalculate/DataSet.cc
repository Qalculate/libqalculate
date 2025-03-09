/*
    Qalculate (library)

    Copyright (C) 2004-2006, 2016, 2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "DataSet.h"

#include "util.h"
#include "Calculator.h"
#include "MathStructure.h"
#include "MathStructure-support.h"
#include "Number.h"
#include "Unit.h"
#include "BuiltinFunctions.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef COMPILED_DEFINITIONS
#	include "definitions.h"
#endif

using std::string;
using std::vector;

#define XML_GET_STRING_FROM_PROP(node, name, str)	value = xmlGetProp(node, (xmlChar*) name); if(value) {str = (char*) value; remove_blank_ends(str); xmlFree(value);} else str = "";
#define XML_GET_STRING_FROM_TEXT(node, str)		value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value) {str = (char*) value; remove_blank_ends(str); xmlFree(value);} else str = "";

DataObject::DataObject(DataSet *parent_set) {
	parent = parent_set;
	b_uchanged = false;
}
DataObject::~DataObject() {
	for(size_t i = 0; i < m_properties.size(); i++) {
		if(m_properties[i]) {
			m_properties[i]->unref();
		}
	}
}

void DataObject::eraseProperty(DataProperty *property) {
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			s_properties.erase(s_properties.begin() + i);
			a_properties.erase(a_properties.begin() + i);
			if(m_properties[i]) {
				m_properties[i]->unref();
			}
			m_properties.erase(m_properties.begin() + i);
			s_nonlocalized_properties.erase(s_nonlocalized_properties.begin() + i);
		}
	}
}
void DataObject::setProperty(DataProperty *property, string s_value, int is_approximate) {
	if(s_value.empty()) eraseProperty(property);
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			s_properties[i] = s_value;
			a_properties[i] = is_approximate;
			if(m_properties[i]) {
				m_properties[i]->unref();
				m_properties[i] = NULL;
			}
			return;
		}
	}
	properties.push_back(property);
	s_properties.push_back(s_value);
	m_properties.push_back(NULL);
	a_properties.push_back(is_approximate);
	s_nonlocalized_properties.push_back("");
}
void DataObject::setNonlocalizedKeyProperty(DataProperty *property, string s_value) {
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			s_nonlocalized_properties[i] = s_value;
			return;
		}
	}
	properties.push_back(property);
	s_properties.push_back("");
	m_properties.push_back(NULL);
	a_properties.push_back(-1);
	s_nonlocalized_properties.push_back(s_value);
}

const string &DataObject::getProperty(DataProperty *property, int *is_approximate) {
	if(!property) return empty_string;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			if(is_approximate) *is_approximate = a_properties[i];
			return s_properties[i];
		}
	}
	return empty_string;
}
const string &DataObject::getNonlocalizedKeyProperty(DataProperty *property) {
	if(!property) return empty_string;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			return s_nonlocalized_properties[i];
		}
	}
	return empty_string;
}
string DataObject::getPropertyInputString(DataProperty *property) {
	if(!property) return empty_string;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			return property->getInputString(s_properties[i]);
		}
	}
	return empty_string;
}
string DataObject::getPropertyDisplayString(DataProperty *property) {
	if(!property) return empty_string;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			return property->getDisplayString(s_properties[i]);
		}
	}
	return empty_string;
}
const MathStructure *DataObject::getPropertyStruct(DataProperty *property) {
	if(!property) return NULL;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == property) {
			if(!m_properties[i]) m_properties[i] = property->generateStruct(s_properties[i], a_properties[i]);
			return m_properties[i];
		}
	}
	return NULL;
}

bool DataObject::isUserModified() const {
	return b_uchanged;
}
void DataObject::setUserModified(bool user_modified) {
	b_uchanged = user_modified;
}

DataSet *DataObject::parentSet() const {
	return parent;
}

DataProperty::DataProperty(DataSet *parent_set, string s_name, string s_title, string s_description) {
	if(!s_name.empty()) {
		names.push_back(s_name);
		name_is_ref.push_back(false);
	}
	stitle = s_title;
	sdescr = s_description;
	parent = parent_set;
	m_unit = NULL;
	ptype = PROPERTY_EXPRESSION;
	b_key = false; b_case = false; b_hide = false; b_brackets = false; b_approximate = false;
	b_uchanged = false;
}
DataProperty::DataProperty(const DataProperty &dp) {
	m_unit = NULL;
	set(dp);
}
DataProperty::~DataProperty() {
	if(m_unit) m_unit->unref();
}

void DataProperty::set(const DataProperty &dp) {
	stitle = dp.title(false);
	sdescr = dp.description();
	sunit = dp.getUnitString();
	parent = dp.parentSet();
	if(m_unit) m_unit->unref();
	m_unit = NULL;
	ptype = dp.propertyType();
	b_key = dp.isKey();
	b_case = dp.isCaseSensitive();
	b_hide = dp.isHidden();
	b_brackets = dp.usesBrackets();
	b_approximate = dp.isApproximate();
	b_uchanged = dp.isUserModified();
	clearNames();
	for(size_t i = 1; i <= dp.countNames(); i++) {
		names.push_back(dp.getName(i));
		name_is_ref.push_back(dp.nameIsReference(i));
	}
}

void DataProperty::setName(string s_name, bool is_ref) {
	if(s_name.empty()) return;
	names.clear();
	name_is_ref.clear();
	names.push_back(s_name);
	name_is_ref.push_back(is_ref);
}
void DataProperty::setNameIsReference(size_t index, bool is_ref) {
	if(index > 0 && index <= name_is_ref.size()) name_is_ref[index - 1] = is_ref;
}
bool DataProperty::nameIsReference(size_t index) const {
	if(index > 0 && index <= name_is_ref.size()) return name_is_ref[index - 1];
	return false;
}
void DataProperty::clearNames() {
	names.clear();
	name_is_ref.clear();
}
void DataProperty::addName(string s_name, bool is_ref, size_t index) {
	if(s_name.empty()) return;
	if(index < 1 || index > names.size()) {
		names.push_back(s_name);
		name_is_ref.push_back(is_ref);
	} else {
		names.insert(names.begin() + (index - 1), s_name);
		name_is_ref.insert(name_is_ref.begin() + (index - 1), is_ref);
	}
}
const string &DataProperty::getName(size_t index) const {
	if(index < 1 || index > names.size()) return empty_string;
	return names[index - 1];
}
const string &DataProperty::getReferenceName() const {
	for(size_t i = 0; i < name_is_ref.size(); i++) {
		if(name_is_ref[i]) return names[i];
	}
	return getName();
}
size_t DataProperty::hasName(const string &s_name) {
	for(size_t i = 0; i < names.size(); i++) {
		if(equalsIgnoreCase(s_name, names[i])) return i + 1;
	}
	return 0;
}
size_t DataProperty::countNames() const {
	return names.size();
}
const string &DataProperty::title(bool return_name_if_no_title) const {
	if(return_name_if_no_title && stitle.empty()) {
		return getName();
	}
	return stitle;
}
void DataProperty::setTitle(string s_title) {
	remove_blank_ends(s_title);
	stitle = s_title;
}
const string &DataProperty::description() const {
	return sdescr;
}
void DataProperty::setDescription(string s_description) {
	remove_blank_ends(s_description);
	sdescr = s_description;
}
void DataProperty::setUnit(string s_unit) {
	sunit = s_unit;
	if(m_unit) m_unit->unref();
	m_unit = NULL;
}
const string &DataProperty::getUnitString() const {
	return sunit;
}
const MathStructure *DataProperty::getUnitStruct() {
	if(!m_unit && !sunit.empty()) {
		m_unit = new MathStructure();
		CALCULATOR->parse(m_unit, sunit);
	}
	return m_unit;
}
string DataProperty::getInputString(const string &valuestr) {
	string str;
	if(b_brackets && valuestr.length() > 1 && valuestr[0] == '[' && valuestr[valuestr.length() - 1] == ']') {
		str = CALCULATOR->localizeExpression(valuestr.substr(1, valuestr.length() - 2));
	} else {
		str = CALCULATOR->localizeExpression(valuestr);
	}
	if(!sunit.empty()) {
		str += " ";
		CompositeUnit cu("", "", "", sunit);
		str += cu.print(false, true, CALCULATOR->messagePrintOptions().use_unicode_signs, CALCULATOR->messagePrintOptions().can_display_unicode_string_function, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg);
	}
	return str;
}
string DataProperty::getDisplayString(const string &valuestr) {
	if(!sunit.empty()) {
		string str = CALCULATOR->localizeExpression(valuestr);
		str += " ";
		CompositeUnit cu("", "", "", sunit);
		return str + cu.print(false, true, CALCULATOR->messagePrintOptions().use_unicode_signs, CALCULATOR->messagePrintOptions().can_display_unicode_string_function, CALCULATOR->messagePrintOptions().can_display_unicode_string_arg);
	}
	return CALCULATOR->localizeExpression(valuestr);
}
MathStructure *DataProperty::generateStruct(const string &valuestr, int is_approximate) {
	MathStructure *mstruct = NULL;
	switch(ptype) {
		case PROPERTY_EXPRESSION: {
			ParseOptions po;
			if((b_approximate && is_approximate < 0) || is_approximate > 0) po.read_precision = ALWAYS_READ_PRECISION;
			if(b_brackets && valuestr.length() > 1 && valuestr[0] == '[' && valuestr[valuestr.length() - 1] == ']') {
				mstruct = new MathStructure();
				CALCULATOR->parse(mstruct, valuestr.substr(1, valuestr.length() - 2), po);
			} else {
				mstruct = new MathStructure();
				CALCULATOR->parse(mstruct, valuestr, po);
			}
			break;
		}
		case PROPERTY_NUMBER: {
			if(valuestr.length() > 1 && valuestr[0] == '[' && valuestr[valuestr.length() - 1] == ']') {
				size_t i = valuestr.find(",");
				if(i != string::npos) {
					Number nr;
					nr.setInterval(Number(valuestr.substr(1, i - 1)), Number(valuestr.substr(i + 1, valuestr.length() - i - 2)));
					mstruct = new MathStructure(nr);
					break;
				} else if(b_brackets) {
					if(((b_approximate && is_approximate < 0) || is_approximate > 0) && valuestr.find(SIGN_PLUSMINUS) == string::npos && valuestr.find("+/-") == string::npos) {
						ParseOptions po; po.read_precision = ALWAYS_READ_PRECISION;
						mstruct = new MathStructure(Number(valuestr.substr(1, valuestr.length() - 2), po));
					} else {
						mstruct = new MathStructure(Number(valuestr.substr(1, valuestr.length() - 2)));
					}
					break;
				}
			}
			if(((b_approximate && is_approximate < 0) || is_approximate > 0) && valuestr.find(SIGN_PLUSMINUS) == string::npos && valuestr.find("+/-") == string::npos) {
				ParseOptions po; po.read_precision = ALWAYS_READ_PRECISION;
				mstruct = new MathStructure(Number(valuestr, po));
			} else {
				mstruct = new MathStructure(Number(valuestr));
			}
			break;
		}
		case PROPERTY_STRING: {
			if(b_brackets && valuestr.length() > 1 && valuestr[0] == '[' && valuestr[valuestr.length() - 1] == ']') mstruct = new MathStructure(valuestr.substr(1, valuestr.length() - 2));
			else mstruct = new MathStructure(valuestr);
			break;
		}
	}
	if(mstruct && getUnitStruct()) {
		mstruct->multiply(*getUnitStruct());
	}
	return mstruct;
}
void DataProperty::setKey(bool is_key) {b_key = is_key;}
bool DataProperty::isKey() const {return b_key;}
void DataProperty::setHidden(bool is_hidden) {b_hide = is_hidden;}
bool DataProperty::isHidden() const {return b_hide;}
void DataProperty::setCaseSensitive(bool is_case_sensitive) {b_case = is_case_sensitive;}
bool DataProperty::isCaseSensitive() const {return b_case;}
void DataProperty::setUsesBrackets(bool uses_brackets) {b_brackets = uses_brackets;}
bool DataProperty::usesBrackets() const {return b_brackets;}
void DataProperty::setApproximate(bool is_approximate) {b_approximate = is_approximate;}
bool DataProperty::isApproximate() const {return b_approximate;}
void DataProperty::setPropertyType(PropertyType property_type) {ptype = property_type;}
PropertyType DataProperty::propertyType() const {return ptype;}

bool DataProperty::isUserModified() const {
	return b_uchanged;
}
void DataProperty::setUserModified(bool user_modified) {
	b_uchanged = user_modified;
}

DataSet *DataProperty::parentSet() const {
	return parent;
}

DataSet::DataSet(string s_category, string s_name, string s_default_file, string s_title, string s_description, bool is_local) : MathFunction(s_name, 1, 2, s_category, s_title, s_description) {
	b_local = is_local;
	sfile = s_default_file;
	b_loaded = false;
	setArgumentDefinition(1, new DataObjectArgument(this, _("Object")));
	setArgumentDefinition(2, new DataPropertyArgument(this, _("Property")));
	setDefaultValue(2, _("info"));
	setChanged(false);
}
DataSet::DataSet(const DataSet *o) {
	b_loaded = false;
	set(o);
}
DataSet::~DataSet() {
	for(size_t i = 0; i < properties.size(); i++) delete properties[i];
	for(size_t i = 0; i < objects.size(); i++) delete objects[i];
}

ExpressionItem *DataSet::copy() const {return new DataSet(this);}
void DataSet::set(const ExpressionItem *item) {
	if(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_DATA_SET) {
		DataSet *dc = (DataSet*) item;
		sfile = dc->defaultDataFile();
		scopyright = dc->copyright();
	}
	MathFunction::set(item);
}
int DataSet::subtype() const {
	return SUBTYPE_DATA_SET;
}

int DataSet::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	DataObject *o = getObject(vargs[0]);
	if(!o) {
		CALCULATOR->error(true, _("Object %s not available in data set."), vargs[0].symbol().c_str(), NULL);
		return 0;
	}
	if(equalsIgnoreCase(vargs[1].symbol(), string("info")) || equalsIgnoreCase(vargs[1].symbol(), string(_("info")))) {
		string str = printProperties(o);
		CALCULATOR->message(MESSAGE_INFORMATION, str.c_str(), NULL);
		return 1;
	}
	DataProperty *dp = getProperty(vargs[1].symbol());
	if(!dp) {
		CALCULATOR->error(true, _("Property %s not available in data set."), vargs[1].symbol().c_str(), NULL);
		return 0;
	}
	const MathStructure *pmstruct = o->getPropertyStruct(dp);
	if(!pmstruct) {
		CALCULATOR->error(true, _("Property %s not defined for object %s."), vargs[1].symbol().c_str(), vargs[0].symbol().c_str(), NULL);
		return 0;
	}
	mstruct.set(*pmstruct);
	return 1;
}

void DataSet::setDefaultProperty(string property) {
	setDefaultValue(2, property);
	setChanged(true);
}
const string &DataSet::defaultProperty() const {
	return getDefaultValue(2);
}

void DataSet::setCopyright(string s_copyright) {
	scopyright = s_copyright;
	setChanged(true);
}
const string &DataSet::copyright() const {
	return scopyright;
}
void DataSet::setDefaultDataFile(string s_file) {
	sfile = s_file;
	setChanged(true);
}
const string &DataSet::defaultDataFile() const {
	return sfile;
}

#ifdef _WIN32
#	define FILE_SEPARATOR_CHAR '\\'
#else
#	define FILE_SEPARATOR_CHAR '/'
#endif
bool DataSet::loadObjects(const char *file_name, bool is_user_defs) {
	if(file_name) {
	} else if(sfile.empty()) {
		return false;
	} else if(sfile.find(FILE_SEPARATOR_CHAR) != string::npos) {
		bool b = !isLocal() && loadObjects(file_name, false);
		size_t i = sfile.find_last_of(FILE_SEPARATOR_CHAR);
		if(i != sfile.length() - 1) {
			string filepath = buildPath(getLocalDataDir(), "definitions", "datasets", sfile.substr(i + 1, sfile.length() - (i + 1)));
			if(loadObjects(filepath.c_str(), true)) {
				b = true;
			} else {
#ifndef _WIN32
				string filepath_old = buildPath(getOldLocalDir(), "definitions", "datasets", sfile.substr(i + 1, sfile.length() - (i + 1)));
				if(loadObjects(filepath_old.c_str(), true)) {
					b = true;
					recursiveMakeDir(getLocalDataDir());
					makeDir(buildPath(getLocalDataDir(), "definitions"));
					makeDir(buildPath(getLocalDataDir(), "definitions", "datasets"));
					move_file(filepath_old.c_str(), filepath.c_str());
					removeDir(buildPath(getOldLocalDir(), "definitions", "datasets"));
					removeDir(buildPath(getOldLocalDir(), "definitions"));
				}
#endif
			}
		}
		return b;
	} else {
		bool b = !isLocal() && loadObjects(buildPath(getGlobalDefinitionsDir(), sfile).c_str(), false);
		string filepath = buildPath(getLocalDataDir(), "definitions", "datasets", sfile);
		if(b && !fileExists(filepath)) return true;
		if(loadObjects(filepath.c_str(), true)) {
			b = true;
		} else {
#ifndef _WIN32
			string filepath_old = buildPath(getOldLocalDir(), "definitions", "datasets", sfile);
			if(loadObjects(filepath_old.c_str(), true)) {
				b = true;
				recursiveMakeDir(getLocalDataDir());
				makeDir(buildPath(getLocalDataDir(), "definitions"));
				makeDir(buildPath(getLocalDataDir(), "definitions", "datasets"));
				move_file(filepath_old.c_str(), filepath.c_str());
				removeDir(buildPath(getOldLocalDir(), "definitions", "datasets"));
				removeDir(buildPath(getOldLocalDir(), "definitions"));
			}
#endif
		}
		return b;
	}
	xmlDocPtr doc;
	xmlNodePtr cur, child;

	string locale, lang_tmp;
#ifdef _WIN32
	size_t n = 0;
	getenv_s(&n, NULL, 0, "LANG");
	if(n > 0) {
		char *c_lang = (char*) malloc(n * sizeof(char));
		getenv_s(&n, c_lang, n, "LANG");
		locale = c_lang;
		free(c_lang);
	} else {
		ULONG nlang = 0;
		DWORD n = 0;
		if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, NULL, &n)) {
			WCHAR* wlocale = new WCHAR[n];
			if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, wlocale, &n)) {
				locale = utf8_encode(wlocale);
			}
			delete[] wlocale;
		}
	}
	gsub("-", "_", locale);
#else
	char *clocale = setlocale(LC_MESSAGES, NULL);
	if(clocale) locale = clocale;
#endif
	if(CALCULATOR->getIgnoreLocale() || locale == "POSIX" || locale == "C") {
		locale = "";
	} else {
		size_t i = locale.find('.');
		if(i != string::npos) locale = locale.substr(0, i);
	}

	string localebase;
	if(locale.length() > 2) {
		localebase = locale.substr(0, 2);
	} else {
		localebase = locale;
	}
	while(localebase.length() < 2) localebase += " ";

#ifdef COMPILED_DEFINITIONS
	if(!is_user_defs) {
		if(strcmp(file_name, "/planets.xml") == 0) {
			doc = xmlParseMemory(planets_xml, strlen(planets_xml));
		} else if(strcmp(file_name, "/elements.xml") == 0) {
			doc = xmlParseMemory(elements_xml, strlen(elements_xml));
		} else {
			doc = xmlParseFile(file_name);
		}
	} else {
		doc = xmlParseFile(file_name);
	}
#else
	doc = xmlParseFile(file_name);
#endif

	if(doc == NULL) {
		if(!is_user_defs) {
			CALCULATOR->error(true, _("Unable to load data objects in %s."), file_name, NULL);
		}
		return false;
	}
	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		xmlFreeDoc(doc);
		if(!is_user_defs) {
			CALCULATOR->error(true, _("Unable to load data objects in %s."), file_name, NULL);
		}
		return false;
	}
	string version;
	xmlChar *value, *lang, *uncertainty;
	bool unc_rel;
	while(cur != NULL) {
		if(!xmlStrcmp(cur->name, (const xmlChar*) "QALCULATE")) {
			XML_GET_STRING_FROM_PROP(cur, "version", version)
			break;
		}
		cur = cur->next;
	}
	if(cur == NULL) {
		CALCULATOR->error(true, _("File not identified as Qalculate! definitions file: %s."), file_name, NULL);
		xmlFreeDoc(doc);
		return false;
	}
	DataObject *o = NULL;
	cur = cur->xmlChildrenNode;
	string str, str2;
	vector<DataProperty*> lang_status_p;
	vector<int> lang_status;
	int ils = 0;
	int i_approx = 0;
	bool cmp = false;
	bool b = false, old_object = false;
	size_t objects_before = objects.size();
	vector<DataProperty*> p_refs;
	vector<string> s_refs;
	while(cur) {
		b = false;
		if(!xmlStrcmp(cur->name, (const xmlChar*) "object")) {
			b = true;
		} else if(xmlStrcmp(cur->name, (const xmlChar*) "text")) {
			for(size_t i = 1; i <= countNames(); i++) {
				if(!xmlStrcmp(cur->name, (const xmlChar*) properties[i]->getName(i).c_str())) {
					b = true;
					break;
				}
			}
		}
		if(b) {
			lang_status_p.clear();
			lang_status.clear();
			if(is_user_defs && objects_before > 0) {
				s_refs.clear();
				p_refs.clear();
			} else {
				o = new DataObject(this);
			}
			for(size_t i = 0; i < properties.size(); i++) {
				if(properties[i]->isKey()) {
					for(size_t i2 = 1; i2 <= properties[i]->countNames(); i2++) {
						if(properties[i]->getName(i2).find(' ') != string::npos) {
							str2 = properties[i]->getName(i2);
							gsub(" ", "_", str2);
							XML_GET_STRING_FROM_PROP(cur, str2.c_str(), str)
						} else {
							XML_GET_STRING_FROM_PROP(cur, properties[i]->getName(i2).c_str(), str)
						}
						remove_blank_ends(str);
						if(!str.empty()) {
							if(is_user_defs && objects_before > 0) {
								s_refs.push_back(str);
								p_refs.push_back(properties[i]);
							} else {
								o->setProperty(properties[i], str);
							}
						}
					}
				}
			}
			old_object = false;
			if(is_user_defs && objects_before > 0) {
				for(size_t i = 0; i < p_refs.size(); i++) {
					for(size_t i2 = 0; i2 < objects_before; i2++) {
						if(s_refs[i] == objects[i2]->getProperty(p_refs[i]) || s_refs[i] == objects[i2]->getNonlocalizedKeyProperty(p_refs[i])) {
							o = objects[i2];
							old_object = true;
							break;
						}
					}
					if(old_object) break;
				}
				if(!old_object) o = new DataObject(this);
				for(size_t i = 0; i < p_refs.size(); i++) {
					o->setProperty(p_refs[i], s_refs[i]);
				}
			}
			child = cur->xmlChildrenNode;
			while(child) {
				b = false;
				for(size_t i = 0; i < properties.size(); i++) {
					for(size_t i2 = 1; i2 <= properties[i]->countNames(); i2++) {
						if(properties[i]->getName(i2).find(' ') != string::npos) {
							str2 = properties[i]->getName(i2);
							gsub(" ", "_", str2);
							cmp = !xmlStrcmp(child->name, (const xmlChar*) str2.c_str());
						} else {
							cmp = !xmlStrcmp(child->name, (const xmlChar*) properties[i]->getName(i2).c_str());
						}
						if(cmp) {
							value = xmlGetProp(child, (xmlChar*) "approximate");
							if(value) {
								if(!xmlStrcmp(value, (const xmlChar*) "false")) {
									i_approx = 0;
								} else if(value && !xmlStrcmp(value, (const xmlChar*) "true")) {
									i_approx = 1;
								}
								xmlFree(value);
							} else {
								i_approx = -1;
							}
							if(properties[i]->propertyType() == PROPERTY_STRING) {
								value = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
								lang = xmlNodeGetLang(child);
								ils = -1;
								for(int i3 = lang_status_p.size() - 1; i3 > 0; i3--) {
									if(lang_status_p[i3] == properties[i3]) {
										ils = i3;
										break;
									}
								}
								if(!lang && (ils < 0 || lang_status[ils] < 1 || properties[i]->isKey())) {
									if(value) {
										str = (char*) value;
										remove_blank_ends(str);
										if(!str.empty() && str[0] == '!') {
											size_t i4 = str.find('!', 1);
											if(i4 != string::npos) {
												if(i4 + 1 < str.length()) {
													str = str.substr(i4 + 1, str.length() - (i4 + 1));
												} else {
													str = "";
												}
											}
										}
									} else {
										str = "";
									}
									if(ils < 0 || lang_status[ils] < 1) {
										o->setProperty(properties[i], str, i_approx);
									} else {
										o->setNonlocalizedKeyProperty(properties[i], str);
									}
								} else if(!locale.empty() && (ils < 0 || lang_status[ils] < 2)) {
									if(locale == (char*) lang) {
										if(ils < 0 && properties[i]->isKey()) o->setNonlocalizedKeyProperty(properties[i], o->getProperty(properties[i]));
										if(ils < 0) {
											lang_status_p.push_back(properties[i]);
											lang_status.push_back(2);
										} else {
											lang_status[ils] = 2;
										}
										if(value) {
											str = (char*) value;
											remove_blank_ends(str);
										} else {
											str = "";
										}
										o->setProperty(properties[i], str, i_approx);
									} else if((ils < 0 || lang_status[ils] < 1) && strlen((char*) lang) >= 2 && lang[0] == localebase[0] && lang[1] == localebase[1]) {
										if(ils < 0 && properties[i]->isKey()) o->setNonlocalizedKeyProperty(properties[i], o->getProperty(properties[i]));
										if(ils < 0) {
											lang_status_p.push_back(properties[i]);
											lang_status.push_back(1);
										} else {
											lang_status[ils] = 1;
										}
										if(value) {
											str = (char*) value;
											remove_blank_ends(str);
										} else {
											str = "";
										}
										o->setProperty(properties[i], str, i_approx);
									}
								}
								if(value) xmlFree(value);
								if(lang) xmlFree(lang);
							} else {
								unc_rel = false;
								uncertainty = xmlGetProp(child, (xmlChar*) "relative_uncertainty");
								if(!uncertainty) uncertainty = xmlGetProp(child, (xmlChar*) "uncertainty");
								else unc_rel = true;
								XML_GET_STRING_FROM_TEXT(child, str)
								remove_blank_ends(str);
								if(uncertainty) {
									if(unc_rel) {
										str.insert(0, "(");
										str.insert(0, CALCULATOR->getFunctionById(FUNCTION_ID_UNCERTAINTY)->referenceName());
										str += ", ";
										str += (char*) uncertainty;
										str += ", 1)";
									} else {
										str += SIGN_PLUSMINUS; str += (char*) uncertainty;
									}
								}
								o->setProperty(properties[i], str, i_approx);
								if(uncertainty) xmlFree(uncertainty);
							}
							b = true;
							break;
						}
					}
					if(b) break;
				}
				child = child->next;
			}
			if(is_user_defs) o->setUserModified(true);
			if(!old_object) objects.push_back(o);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	b_loaded = true;
	/*if(!scopyright.empty()) {
		CALCULATOR->message(MESSAGE_INFORMATION, scopyright.c_str(), NULL);
	}*/
	return true;
}
int DataSet::saveObjects(const char *file_name, bool save_global) {
	if(!b_loaded) return 1;
	string str, filename;
	if(!save_global && !file_name) {
		recursiveMakeDir(getLocalDataDir());
		filename = buildPath(getLocalDataDir(), "definitions");
		makeDir(filename);
		filename = buildPath(filename, "datasets");
		makeDir(filename);
		filename = buildPath(filename, sfile);
	} else {
		filename = file_name;
	}
	const string *vstr;
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	xmlNodePtr cur, newnode, newnode2;
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	cur = doc->children;
	DataObject *o;
	int approx = false;
	bool do_save = save_global;
	for(size_t i = 0; i < objects.size(); i++) {
		if(save_global || objects[i]->isUserModified()) {
			do_save = true;
			o = objects[i];
			newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "object", NULL);
			if(!save_global) {
				for(size_t i2 = 0; i2 < properties.size(); i2++) {
					if(properties[i2]->isKey()) {
						vstr = &o->getProperty(properties[i2], &approx);
						if(approx < 0 && !vstr->empty()) {
							xmlNewProp(newnode, (xmlChar*) properties[i2]->getReferenceName().c_str(), (xmlChar*) vstr->c_str());
						}
					}
				}
			}
			for(size_t i2 = 0; i2 < properties.size(); i2++) {
				if(save_global && properties[i2]->isKey()) {
					vstr = &o->getNonlocalizedKeyProperty(properties[i2]);
					if(vstr->empty()) {
						vstr = &o->getProperty(properties[i2], &approx);
					} else {
						o->getProperty(properties[i2], &approx);
					}
				} else {
					vstr = &o->getProperty(properties[i2], &approx);
				}
				if(save_global || approx >= 0 || !properties[i2]->isKey()) {
					if(!vstr->empty()) {
						if(properties[i2]->getReferenceName().find(' ') != string::npos) {
							if(save_global && properties[i2]->propertyType() == PROPERTY_STRING) {
								str = "_";
							} else {
								str = "";
							}
							str += properties[i2]->getReferenceName();
							gsub(" ", "_", str);
							newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) str.c_str(), (xmlChar*) vstr->c_str());
						} else {
							if(save_global && properties[i2]->propertyType() == PROPERTY_STRING) {
								str = "_";
								str += properties[i2]->getReferenceName();
								newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) str.c_str(), (xmlChar*) vstr->c_str());
							} else {
								newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) properties[i2]->getReferenceName().c_str(), (xmlChar*) vstr->c_str());
							}
						}
						if(approx >= 0) {
							xmlNewProp(newnode2, (xmlChar*) "approximate", (xmlChar*) b2tf(approx > 0));
						}
					}
				}
			}
		}
	}
	int returnvalue = 1;
	if(do_save) {
		returnvalue = xmlSaveFormatFile(filename.c_str(), doc, 1);
	} else {
		if(fileExists(filename)) remove(filename.c_str());
	}
	xmlFreeDoc(doc);
	return returnvalue;
}
bool DataSet::objectsLoaded() const {
	return b_loaded || sfile.empty();
}
void DataSet::setObjectsLoaded(bool objects_loaded) {
	b_loaded = objects_loaded;
}

void DataSet::addProperty(DataProperty *dp) {
	properties.push_back(dp);
	setChanged(true);
}
void DataSet::delProperty(DataProperty *dp) {
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i] == dp) {
			delete properties[i];
			properties.erase(properties.begin() + i);
			setChanged(true);
			break;
		}
	}
}
void DataSet::delProperty(DataPropertyIter *it) {
	*it = properties.erase(*it);
	--(*it);
}
DataProperty *DataSet::getPrimaryKeyProperty() {
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i]->isKey()) return properties[i];
	}
	return NULL;
}
DataProperty *DataSet::getProperty(string property) {
	if(property.empty()) return NULL;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i]->hasName(property)) return properties[i];
	}
	return NULL;
}
DataProperty *DataSet::getFirstProperty(DataPropertyIter *it) {
	*it = properties.begin();
	if(*it != properties.end()) return **it;
	return NULL;
}
DataProperty *DataSet::getNextProperty(DataPropertyIter *it) {
	++(*it);
	if(*it != properties.end()) return **it;
	return NULL;
}
const string &DataSet::getFirstPropertyName(DataPropertyIter *it) {
	*it = properties.begin();
	if(*it != properties.end()) return (**it)->getName();
	return empty_string;
}
const string &DataSet::getNextPropertyName(DataPropertyIter *it) {
	++(*it);
	if(*it != properties.end()) return (**it)->getName();
	return empty_string;
}

void DataSet::addObject(DataObject *o) {
	if(!objectsLoaded()) loadObjects();
	objects.push_back(o);
	b_loaded = true;
}
void DataSet::delObject(DataObject *o) {
	for(size_t i = 0; i < objects.size(); i++) {
		if(objects[i] == o) {
			delete objects[i];
			objects.erase(objects.begin() + i);
			break;
		}
	}
}
void DataSet::delObject(DataObjectIter *it) {
	*it = objects.erase(*it);
	--(*it);
}
DataObject *DataSet::getObject(string object) {
	if(!objectsLoaded()) loadObjects();
	if(object.empty()) return NULL;
	DataProperty *dp;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i]->isKey()) {
			dp = properties[i];
			if(dp->isCaseSensitive()) {
				for(size_t i2 = 0; i2 < objects.size(); i2++) {
					if(object == objects[i2]->getProperty(dp) || object == objects[i2]->getNonlocalizedKeyProperty(dp)) {
						return objects[i2];
					}
				}
			} else {
				for(size_t i2 = 0; i2 < objects.size(); i2++) {
					if(equalsIgnoreCase(object, objects[i2]->getProperty(dp)) || equalsIgnoreCase(object, objects[i2]->getNonlocalizedKeyProperty(dp))) {
						return objects[i2];
					}
				}
			}
		}
	}
	return NULL;
}
DataObject *DataSet::getObject(const MathStructure &object) {
	if(object.isSymbolic()) return getObject(object.symbol());
	if(!objectsLoaded()) loadObjects();
	DataProperty *dp;
	for(size_t i = 0; i < properties.size(); i++) {
		if(properties[i]->isKey()) {
			dp = properties[i];
			if(dp->propertyType() != PROPERTY_STRING) {
				const MathStructure *mstruct;
				for(size_t i2 = 0; i2 < objects.size(); i2++) {
					mstruct = objects[i2]->getPropertyStruct(dp);
					if(mstruct && object.equals(*mstruct)) {
						return objects[i2];
					}
				}
			}
		}
	}
	return NULL;
}
DataObject *DataSet::getFirstObject(DataObjectIter *it) {
	if(!objectsLoaded()) loadObjects();
	*it = objects.begin();
	if(*it != objects.end()) return **it;
	return NULL;
}
DataObject *DataSet::getNextObject(DataObjectIter *it) {
	++(*it);
	if(*it != objects.end()) return **it;
	return NULL;
}

const MathStructure *DataSet::getObjectProperyStruct(string property, string object) {
	DataObject *o = getObject(object);
	DataProperty *dp = getProperty(property);
	if(o && dp) {
		return o->getPropertyStruct(dp);
	}
	return NULL;
}
const string &DataSet::getObjectProperty(string property, string object) {
	DataObject *o = getObject(object);
	DataProperty *dp = getProperty(property);
	if(o && dp) {
		return o->getProperty(dp);
	}
	return empty_string;
}
string DataSet::getObjectPropertyInputString(string property, string object) {
	DataObject *o = getObject(object);
	DataProperty *dp = getProperty(property);
	if(o && dp) {
		return o->getPropertyInputString(dp);
	}
	return empty_string;
}
string DataSet::getObjectPropertyDisplayString(string property, string object) {
	DataObject *o = getObject(object);
	DataProperty *dp = getProperty(property);
	if(o && dp) {
		return o->getPropertyDisplayString(dp);
	}
	return empty_string;
}

string DataSet::printProperties(string object) {
	return printProperties(getObject(object));
}
string DataSet::printProperties(DataObject *o) {
	if(o) {
		string str, stmp;
		str = "-------------------------------------\n";
		bool started = false;
		for(size_t i = 0; i < properties.size(); i++) {
			if(!properties[i]->isHidden() && properties[i]->isKey()) {
				stmp = o->getPropertyDisplayString(properties[i]);
				if(!stmp.empty()) {
					if(started) str += "\n\n";
					else started = true;
					str += properties[i]->title();
					str += ":\n";
					str += stmp;
				}
			}
		}
		for(size_t i = 0; i < properties.size(); i++) {
			if(!properties[i]->isHidden() && !properties[i]->isKey()) {
				stmp = o->getPropertyDisplayString(properties[i]);
				if(!stmp.empty()) {
					if(started) str += "\n\n";
					else started = true;
					str += properties[i]->title();
					str += ":\n";
					str += stmp;
				}
			}
		}
		str += "\n-------------------------------------";
		return str;
	}
	return empty_string;
}

DataPropertyArgument::DataPropertyArgument(DataSet *data_set, string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {
	b_text = true;
	o_data = data_set;
}
DataPropertyArgument::DataPropertyArgument(const DataPropertyArgument *arg) {set(arg); b_text = true; o_data = arg->dataSet();}
DataPropertyArgument::~DataPropertyArgument() {}
bool DataPropertyArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(!value.isSymbolic()) {
		value.eval(eo);
	}
	return value.isSymbolic() && o_data && (o_data->getProperty(value.symbol()) || equalsIgnoreCase(value.symbol(), string("info")) || equalsIgnoreCase(value.symbol(), string(_("info"))));
}
int DataPropertyArgument::type() const {return ARGUMENT_TYPE_DATA_PROPERTY;}
Argument *DataPropertyArgument::copy() const {return new DataPropertyArgument(this);}
string DataPropertyArgument::print() const {return _("data property");}
string DataPropertyArgument::subprintlong() const {
	string str = _("name of a data property");
	str += " (";
	DataPropertyIter it;
	DataProperty *o = NULL;
	if(o_data) {
		o = o_data->getFirstProperty(&it);
	}
	if(!o) {
		str += _("no properties available");
	} else {
		string stmp;
		size_t l_last = 0;
		while(true) {
			if(!o->isHidden()) {
				if(!stmp.empty()) {
					stmp += ", ";
					l_last = stmp.length();
				}
				stmp += o->getName();
			}
			o = o_data->getNextProperty(&it);
			if(!o) break;
		}
		if(stmp.empty()) {
			str += _("no properties available");
		} else {
			if(l_last > 0) {
				stmp.insert(l_last, " ");
				stmp.insert(l_last, _("or"));
			}
			str += stmp;
		}
	}
	str += ")";
	return str;
}
DataSet *DataPropertyArgument::dataSet() const {return o_data;}
void DataPropertyArgument::setDataSet(DataSet *data_set) {o_data = data_set;}

DataObjectArgument::DataObjectArgument(DataSet *data_set, string name_, bool does_test, bool does_error) : Argument(name_, does_test, does_error) {
	b_text = true;
	o_data = data_set;
}
DataObjectArgument::DataObjectArgument(const DataObjectArgument *arg) {set(arg); b_text = true; o_data = arg->dataSet();}
DataObjectArgument::~DataObjectArgument() {}
bool DataObjectArgument::subtest(MathStructure &value, const EvaluationOptions &eo) const {
	if(value.isSymbolic()) return true;
	value.eval(eo);
	if(value.isSymbolic()) return true;
	if(!o_data) return false;
	DataPropertyIter it;
	DataProperty *dp = o_data->getFirstProperty(&it);
	while(dp) {
		if(dp->isKey() && (dp->propertyType() == PROPERTY_EXPRESSION || (value.isNumber() && dp->propertyType() == PROPERTY_NUMBER))) {
			return true;
		}
		dp = o_data->getNextProperty(&it);
	}
	CALCULATOR->error(true, _("Data set \"%s\" has no object key that supports the provided argument type."), o_data->title().c_str(), NULL);
	return false;
}
int DataObjectArgument::type() const {return ARGUMENT_TYPE_DATA_OBJECT;}
Argument *DataObjectArgument::copy() const {return new DataObjectArgument(this);}
string DataObjectArgument::print() const {return _("data object");}
string DataObjectArgument::subprintlong() const {
	if(!o_data) return print();
	string str = _("an object from");
	str += " \"";
	str += o_data->title();
	str += "\"";
	DataPropertyIter it;
	DataProperty *o = NULL;
	o = o_data->getFirstProperty(&it);
	if(o) {
		string stmp;
		size_t l_last = 0;
		while(true) {
			if(o->isKey()) {
				if(!stmp.empty()) {
					stmp += ", ";
					l_last = stmp.length();
				}
				stmp += o->getName();
			}
			o = o_data->getNextProperty(&it);
			if(!o) break;
		}
		if(!stmp.empty()) {
			if(l_last > 0) {
				stmp.insert(l_last, " ");
				stmp.insert(l_last, _("or"));
			}
			str += " (";
			str += _("use");
			str += " ";
			str += stmp;
			str += ")";
		}
	}
	return str;
}
DataSet *DataObjectArgument::dataSet() const {return o_data;}
void DataObjectArgument::setDataSet(DataSet *data_set) {o_data = data_set;}

