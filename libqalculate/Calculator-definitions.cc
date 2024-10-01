/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include "QalculateDateTime.h"

#include <locale.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#ifdef COMPILED_DEFINITIONS
#	include "definitions.h"
#endif
#ifdef _MSC_VER
#	include <sys/utime.h>
#else
#	include <unistd.h>
#	include <utime.h>
#endif
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#	include <dirent.h>
#endif
#include <queue>
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits.h>

#include "MathStructure-support.h"

#ifdef HAVE_LIBCURL
#	include <curl/curl.h>
#endif

using std::string;
using std::cout;
using std::vector;
using std::ostream;
using std::ofstream;
using std::endl;
using std::ios;
using std::ifstream;
using std::iterator;
using std::list;
using std::queue;

#include "Calculator_p.h"

#define XML_GET_PREC_FROM_PROP(node, i)			value = xmlGetProp(node, (xmlChar*) "precision"); if(value) {i = s2i((char*) value); xmlFree(value);} else {i = -1;}
#define XML_GET_APPROX_FROM_PROP(node, b)		value = xmlGetProp(node, (xmlChar*) "approximate"); if(value) {b = !xmlStrcmp(value, (const xmlChar*) "true");} else {value = xmlGetProp(node, (xmlChar*) "precise"); if(value) {b = xmlStrcmp(value, (const xmlChar*) "true");} else {b = false;}} if(value) xmlFree(value);
#define XML_GET_FALSE_FROM_PROP(node, name, b)		value = xmlGetProp(node, (xmlChar*) name); if(value && !xmlStrcmp(value, (const xmlChar*) "false")) {b = false;} else {b = true;} if(value) xmlFree(value);
#define XML_GET_TRUE_FROM_PROP(node, name, b)		value = xmlGetProp(node, (xmlChar*) name); if(value && !xmlStrcmp(value, (const xmlChar*) "true")) {b = true;} else {b = false;} if(value) xmlFree(value);
#define XML_GET_BOOL_FROM_PROP(node, name, b)		value = xmlGetProp(node, (xmlChar*) name); if(value && !xmlStrcmp(value, (const xmlChar*) "false")) {b = false;} else if(value && !xmlStrcmp(value, (const xmlChar*) "true")) {b = true;} if(value) xmlFree(value);
#define XML_GET_FALSE_FROM_TEXT(node, b)		value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value && !xmlStrcmp(value, (const xmlChar*) "false")) {b = false;} else {b = true;} if(value) xmlFree(value);
#define XML_GET_TRUE_FROM_TEXT(node, b)			value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value && !xmlStrcmp(value, (const xmlChar*) "true")) {b = true;} else {b = false;} if(value) xmlFree(value);
#define XML_GET_BOOL_FROM_TEXT(node, b)			value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value && !xmlStrcmp(value, (const xmlChar*) "false")) {b = false;} else if(value && !xmlStrcmp(value, (const xmlChar*) "true")) {b = true;} if(value) xmlFree(value);
#define XML_GET_STRING_FROM_PROP(node, name, str)	value = xmlGetProp(node, (xmlChar*) name); if(value) {str = (char*) value; remove_blank_ends(str); xmlFree(value);} else str = "";
#define XML_GET_STRING_FROM_TEXT(node, str)		value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value) {str = (char*) value; remove_blank_ends(str); xmlFree(value);} else str = "";
#define XML_DO_FROM_PROP(node, name, action)		value = xmlGetProp(node, (xmlChar*) name); if(value) action((char*) value); else action(""); if(value) xmlFree(value);
#define XML_DO_FROM_TEXT(node, action)			value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value) {action((char*) value); xmlFree(value);} else action("");
#define XML_GET_INT_FROM_PROP(node, name, i)		value = xmlGetProp(node, (xmlChar*) name); if(value) {i = s2i((char*) value); xmlFree(value);}
#define XML_GET_INT_FROM_TEXT(node, i)			value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); if(value) {i = s2i((char*) value); xmlFree(value);}
#define XML_GET_LOCALE_STRING_FROM_TEXT(node, str, best, next_best)		value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); lang = xmlNodeGetLang(node); if(!best) {if(!lang) {if(!next_best) {if(value) {str = (char*) value; remove_blank_ends(str);} else str = ""; if(locale.empty()) {best = true;}}} else {if(locale == (char*) lang) {best = true; if(value) {str = (char*) value; remove_blank_ends(str);} else str = "";} else if(!next_best && strlen((char*) lang) >= 2 && fulfilled_translation == 0 && lang[0] == localebase[0] && lang[1] == localebase[1]) {next_best = true; if(value) {str = (char*) value; remove_blank_ends(str);} else str = "";} else if(!next_best && str.empty() && value) {str = (char*) value; remove_blank_ends(str);}}} if(value) xmlFree(value); if(lang) xmlFree(lang);
#define XML_GET_LOCALE_STRING_FROM_TEXT_REQ(node, str, best, next_best)		value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1); lang = xmlNodeGetLang(node); if(!best) {if(!lang) {if(!next_best) {if(value) {str = (char*) value; remove_blank_ends(str);} else str = ""; if(locale.empty()) {best = true;}}} else {if(locale == (char*) lang) {best = true; if(value) {str = (char*) value; remove_blank_ends(str);} else str = "";} else if(!next_best && strlen((char*) lang) >= 2 && fulfilled_translation == 0 && lang[0] == localebase[0] && lang[1] == localebase[1]) {next_best = true; if(value) {str = (char*) value; remove_blank_ends(str);} else str = "";} else if(!next_best && str.empty() && value && !require_translation) {str = (char*) value; remove_blank_ends(str);}}} if(value) xmlFree(value); if(lang) xmlFree(lang);

#define VERSION_BEFORE(i1, i2, i3) (version_numbers[0] < i1 || (version_numbers[0] == i1 && (version_numbers[1] < i2 || (version_numbers[1] == i2 && version_numbers[2] < i3))))

#ifdef COMPILED_DEFINITIONS

bool Calculator::loadGlobalDefinitions() {
	bool b = true;
	if(!loadDefinitions(prefixes_xml, false)) b = false;
	if(!loadDefinitions(currencies_xml, false)) b = false;
	if(!loadDefinitions(units_xml, false)) b = false;
	if(!loadDefinitions(functions_xml, false)) b = false;
	if(!loadDefinitions(datasets_xml, false)) b = false;
	if(!loadDefinitions(variables_xml, false)) b = false;
	return b;
}
bool Calculator::loadGlobalDefinitions(string filename) {
	return loadDefinitions(buildPath(getGlobalDefinitionsDir(), filename).c_str(), false);
}
bool Calculator::loadGlobalPrefixes() {
	return loadDefinitions(prefixes_xml, false);
}
bool Calculator::loadGlobalCurrencies() {
	return loadDefinitions(currencies_xml, false);
}
bool Calculator::loadGlobalUnits() {
	bool b = loadDefinitions(currencies_xml, false);
	return loadDefinitions(units_xml, false) && b;
}
bool Calculator::loadGlobalVariables() {
	return loadDefinitions(variables_xml, false);
}
bool Calculator::loadGlobalFunctions() {
	return loadDefinitions(functions_xml, false);
}
bool Calculator::loadGlobalDataSets() {
	return loadDefinitions(datasets_xml, false);
}
#else
bool Calculator::loadGlobalDefinitions() {
	bool b = true;
	if(!loadDefinitions(buildPath(getGlobalDefinitionsDir(), "prefixes.xml").c_str(), false)) b = false;
	if(!loadDefinitions(buildPath(getGlobalDefinitionsDir(), "currencies.xml").c_str(), false)) b = false;
	if(!loadDefinitions(buildPath(getGlobalDefinitionsDir(), "units.xml").c_str(), false)) b = false;
	if(!loadDefinitions(buildPath(getGlobalDefinitionsDir(), "functions.xml").c_str(), false)) b = false;
	if(!loadDefinitions(buildPath(getGlobalDefinitionsDir(), "datasets.xml").c_str(), false)) b = false;
	if(!loadDefinitions(buildPath(getGlobalDefinitionsDir(), "variables.xml").c_str(), false)) b = false;
	return b;
}
bool Calculator::loadGlobalDefinitions(string filename) {
	return loadDefinitions(buildPath(getGlobalDefinitionsDir(), filename).c_str(), false);
}
bool Calculator::loadGlobalPrefixes() {
	return loadGlobalDefinitions("prefixes.xml");
}
bool Calculator::loadGlobalCurrencies() {
	return loadGlobalDefinitions("currencies.xml");
}
bool Calculator::loadGlobalUnits() {
	bool b = loadGlobalDefinitions("currencies.xml");
	return loadGlobalDefinitions("units.xml") && b;
}
bool Calculator::loadGlobalVariables() {
	return loadGlobalDefinitions("variables.xml");
}
bool Calculator::loadGlobalFunctions() {
	return loadGlobalDefinitions("functions.xml");
}
bool Calculator::loadGlobalDataSets() {
	return loadGlobalDefinitions("datasets.xml");
}
#endif
bool Calculator::loadLocalDefinitions() {
	string homedir = buildPath(getLocalDataDir(), "definitions");
	if(!dirExists(homedir)) {
		string homedir_old = buildPath(getOldLocalDir(), "definitions");
		if(dirExists(homedir)) {
			if(!dirExists(getLocalDataDir())) {
				recursiveMakeDir(getLocalDataDir());
			}
			if(makeDir(homedir)) {
#ifndef _MSC_VER
				list<string> eps_old;
				struct dirent *ep_old;
				DIR *dp_old = opendir(homedir_old.c_str());
				if(dp_old) {
					while((ep_old = readdir(dp_old))) {
#	ifdef _DIRENT_HAVE_D_TYPE
						if(ep_old->d_type != DT_DIR) {
#	endif
							if(strcmp(ep_old->d_name, "..") != 0 && strcmp(ep_old->d_name, ".") != 0 && strcmp(ep_old->d_name, "datasets") != 0) {
								eps_old.push_back(ep_old->d_name);
							}
#	ifdef _DIRENT_HAVE_D_TYPE
						}
#	endif
					}
					closedir(dp_old);
				}
				for(list<string>::iterator it = eps_old.begin(); it != eps_old.end(); ++it) {
					move_file(buildPath(homedir_old, *it).c_str(), buildPath(homedir, *it).c_str());
				}
				if(removeDir(homedir_old)) {
					removeDir(getOldLocalDir());
				}
#endif
			}
		}
	}
	list<string> eps;
#ifdef _MSC_VER
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	if((hFind = FindFirstFile(buildPath(homedir, "*").c_str(), &FindFileData)) != INVALID_HANDLE_VALUE) {
		do {
			if(!(FineFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) eps.push_back(FindFileData.cFileName);
		} while(FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}
#else
	struct dirent *ep;
	DIR *dp = opendir(homedir.c_str());
	if(dp) {
		while((ep = readdir(dp))) {
#	ifdef _DIRENT_HAVE_D_TYPE
			if(ep->d_type != DT_DIR) {
#	endif
				if(strcmp(ep->d_name, "..") != 0 && strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "datasets") != 0) {
					eps.push_back(ep->d_name);
				}
#	ifdef _DIRENT_HAVE_D_TYPE
			}
#	endif
		}
		closedir(dp);
	}
#endif
	eps.sort();
	for(list<string>::iterator it = eps.begin(); it != eps.end(); ++it) {
		loadDefinitions(buildPath(homedir, *it).c_str(), (*it) == "functions.xml" || (*it) == "variables.xml" || (*it) == "units.xml" || (*it) == "datasets.xml", true);
	}
	for(size_t i = 0; i < variables.size(); i++) {
		if(!variables[i]->isLocal() && !variables[i]->isActive() && !getActiveExpressionItem(variables[i])) variables[i]->setActive(true);
	}
	for(size_t i = 0; i < units.size(); i++) {
		if(!units[i]->isLocal() && !units[i]->isActive() && !getActiveExpressionItem(units[i])) units[i]->setActive(true);
	}
	for(size_t i = 0; i < functions.size(); i++) {
		if(!functions[i]->isLocal() && !functions[i]->isActive() && !getActiveExpressionItem(functions[i])) functions[i]->setActive(true);
	}
	return true;
}

#define ITEM_SAVE_BUILTIN_NAMES\
	if(!is_user_defs) {item->setRegistered(false);} \
	for(size_t i = 1; i <= item->countNames(); i++) { \
		if(item->getName(i).reference) { \
			for(size_t i2 = 0; i2 < 10; i2++) { \
				if(ref_names[i2].name.empty()) { \
					ref_names[i2] = item->getName(i); \
					break; \
				} \
			} \
		} \
	} \
	item->clearNames();

#define X_SET_BEST_NAMES(validation, x) \
	size_t names_i = 0, i2 = 0; \
	string *str_names = NULL; \
	if(best_names == "-") {best_names = ""; nextbest_names = "";} \
	if(!best_names.empty()) {str_names = &best_names;} \
	else if(!nextbest_names.empty()) {str_names = &nextbest_names;} \
	else if(!require_translation || best_title || next_best_title) {str_names = &default_names;} \
	if(str_names && !str_names->empty() && (*str_names)[0] == '!') { \
		names_i = str_names->find('!', 1) + 1; \
	} \
	while(str_names != NULL) { \
		size_t i3 = names_i; \
		names_i = str_names->find(",", i3); \
		if(i2 == 0) { \
			i2 = str_names->find(":", i3); \
		} \
		bool case_set = false; \
		ename.unicode = false; \
		ename.abbreviation = false; \
		ename.case_sensitive = false; \
		ename.suffix = false; \
		ename.avoid_input = false; \
		ename.completion_only = false; \
		ename.reference = false; \
		ename.plural = false; \
		if(i2 < names_i) { \
			bool b = true; \
			for(; i3 < i2; i3++) { \
				switch((*str_names)[i3]) { \
					case '-': {b = false; break;} \
					case 'a': {ename.abbreviation = b; b = true; break;} \
					case 'c': {ename.case_sensitive = b; b = true; case_set = true; break;} \
					case 'i': {ename.avoid_input = b; b = true; break;} \
					case 'p': {ename.plural = b; b = true; break;} \
					case 'r': {ename.reference = b; b = true; break;} \
					case 's': {ename.suffix = b; b = true; break;} \
					case 'u': {ename.unicode = b; b = true; break;} \
					case 'o': {ename.completion_only = b; b = true; break;} \
				} \
			} \
			i3++; \
			i2 = 0; \
		} \
		if(names_i == string::npos) {ename.name = str_names->substr(i3, str_names->length() - i3);} \
		else {ename.name = str_names->substr(i3, names_i - i3);} \
		remove_blank_ends(ename.name); \
		if(!ename.name.empty() && validation(ename.name, version_numbers, is_user_defs)) { \
			if(!case_set) { \
				ename.case_sensitive = ename.abbreviation || text_length_is_one(ename.name); \
			} \
			x->addName(ename); \
		} \
		if(names_i == string::npos) {break;} \
		names_i++; \
	}

#define PREFIX_SET_BEST_NAMES X_SET_BEST_NAMES(unitNameIsValid, p)
#define ITEM_SET_BEST_NAMES(validation) X_SET_BEST_NAMES(validation, item)

#define ITEM_SET_BUILTIN_NAMES \
	for(size_t i = 0; i < 10; i++) { \
		if(ref_names[i].name.empty()) { \
			break; \
		} else { \
			size_t i4 = item->hasName(ref_names[i].name, ref_names[i].case_sensitive); \
			if(i4 > 0) { \
				const ExpressionName *enameptr = &item->getName(i4); \
				ref_names[i].case_sensitive = enameptr->case_sensitive; \
				ref_names[i].abbreviation = enameptr->abbreviation; \
				ref_names[i].avoid_input = enameptr->avoid_input; \
				ref_names[i].completion_only = enameptr->completion_only; \
				ref_names[i].plural = enameptr->plural; \
				ref_names[i].suffix = enameptr->suffix; \
				item->setName(ref_names[i], i4); \
			} else { \
				item->addName(ref_names[i]); \
			} \
			ref_names[i].name = ""; \
		} \
	} \
	if(!is_user_defs) { \
		item->setRegistered(true); \
		nameChanged(item); \
	}

#define X_SET_REFERENCE_NAMES(validation, x) \
	if(str_names != &default_names && !default_names.empty()) { \
		if(default_names[0] == '!') { \
			names_i = default_names.find('!', 1) + 1; \
		} else { \
			names_i = 0; \
		} \
		i2 = 0; \
		while(true) { \
			size_t i3 = names_i; \
			names_i = default_names.find(",", i3); \
			if(i2 == 0) { \
				i2 = default_names.find(":", i3); \
			} \
			bool case_set = false; \
			ename.unicode = false; \
			ename.abbreviation = false; \
			ename.case_sensitive = false; \
			ename.suffix = false; \
			ename.avoid_input = false; \
			ename.completion_only = false; \
			ename.reference = false; \
			ename.plural = false; \
			if(i2 < names_i) { \
				bool b = true; \
				for(; i3 < i2; i3++) { \
					switch(default_names[i3]) { \
						case '-': {b = false; break;} \
						case 'a': {ename.abbreviation = b; b = true; break;} \
						case 'c': {ename.case_sensitive = b; b = true; case_set = true; break;} \
						case 'i': {ename.avoid_input = b; b = true; break;} \
						case 'p': {ename.plural = b; b = true; break;} \
						case 'r': {ename.reference = b; b = true; break;} \
						case 's': {ename.suffix = b; b = true; break;} \
						case 'u': {ename.unicode = b; b = true; break;} \
						case 'o': {ename.completion_only = b; b = true; break;} \
					} \
				} \
				i3++; \
				i2 = 0; \
			} \
			if(ename.reference) { \
				if(names_i == string::npos) {ename.name = default_names.substr(i3, default_names.length() - i3);} \
				else {ename.name = default_names.substr(i3, names_i - i3);} \
				if(!case_set) { \
					ename.case_sensitive = ename.abbreviation || text_length_is_one(ename.name); \
				} \
				remove_blank_ends(ename.name); \
				size_t i4 = x->hasName(ename.name, ename.case_sensitive); \
				if(i4 > 0) { \
					const ExpressionName *enameptr = &x->getName(i4); \
					ename.suffix = enameptr->suffix; \
					ename.abbreviation = enameptr->abbreviation; \
					ename.avoid_input = enameptr->avoid_input; \
					ename.completion_only = enameptr->completion_only; \
					ename.plural = enameptr->plural; \
					ename.case_sensitive = enameptr->case_sensitive; \
					x->setName(ename, i4); \
				} else if(!ename.name.empty() && validation(ename.name, version_numbers, is_user_defs)) { \
					if(!case_set) { \
						ename.case_sensitive = ename.abbreviation || text_length_is_one(ename.name); \
					} \
					x->addName(ename); \
				} \
			} \
			if(names_i == string::npos) {break;} \
			names_i++; \
		} \
	}

#define PREFIX_SET_REFERENCE_NAMES X_SET_REFERENCE_NAMES(unitNameIsValid, p)
#define ITEM_SET_REFERENCE_NAMES(validation) X_SET_REFERENCE_NAMES(validation, item)

#define ITEM_READ_NAME(validation)\
					if(!new_names && (!xmlStrcmp(child->name, (const xmlChar*) "name") || !xmlStrcmp(child->name, (const xmlChar*) "abbreviation") || !xmlStrcmp(child->name, (const xmlChar*) "plural"))) {\
						name_index = 1;\
						XML_GET_INT_FROM_PROP(child, "index", name_index)\
						if(name_index > 0 && name_index <= 10) {\
							name_index--;\
							names[name_index] = empty_expression_name;\
							ref_names[name_index] = empty_expression_name;\
							value2 = NULL;\
							bool case_set = false;\
							if(child->name[0] == 'a') {\
								names[name_index].abbreviation = true;\
								ref_names[name_index].abbreviation = true;\
							} else if(child->name[0] == 'p') {\
								names[name_index].plural = true;\
								ref_names[name_index].plural = true;\
							}\
							child2 = child->xmlChildrenNode;\
							while(child2 != NULL) {\
								if((!best_name[name_index] || (ref_names[name_index].name.empty() && !locale.empty())) && !xmlStrcmp(child2->name, (const xmlChar*) "name")) {\
									lang = xmlNodeGetLang(child2);\
									if(!lang) {\
										value2 = xmlNodeListGetString(doc, child2->xmlChildrenNode, 1);\
										if(!value2 || validation((char*) value2, version_numbers, is_user_defs)) {\
											if(locale.empty()) {\
												best_name[name_index] = true;\
												if(value2) names[name_index].name = (char*) value2;\
												else names[name_index].name = "";\
											} else if(!require_translation) {\
												if(!best_name[name_index] && !nextbest_name[name_index]) {\
													if(value2) names[name_index].name = (char*) value2;\
													else names[name_index].name = "";\
												}\
												if(value2) ref_names[name_index].name = (char*) value2;\
												else ref_names[name_index].name = "";\
											}\
										}\
									} else if(!best_name[name_index] && !locale.empty()) {\
										if(locale == (char*) lang) {\
											value2 = xmlNodeListGetString(doc, child2->xmlChildrenNode, 1);\
											if(!value2 || validation((char*) value2, version_numbers, is_user_defs)) {\
												best_name[name_index] = true;\
												if(value2) names[name_index].name = (char*) value2;\
												else names[name_index].name = "";\
											}\
										} else if(!nextbest_name[name_index] && strlen((char*) lang) >= 2 && fulfilled_translation == 0 && lang[0] == localebase[0] && lang[1] == localebase[1]) {\
											value2 = xmlNodeListGetString(doc, child2->xmlChildrenNode, 1);\
											if(!value2 || validation((char*) value2, version_numbers, is_user_defs)) {\
												nextbest_name[name_index] = true; \
												if(value2) names[name_index].name = (char*) value2;\
												else names[name_index].name = "";\
											}\
										}\
									}\
									if(value2) xmlFree(value2);\
									if(lang) xmlFree(lang);\
									value2 = NULL; lang = NULL;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "unicode")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].unicode)\
									ref_names[name_index].unicode = names[name_index].unicode;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "reference")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].reference)\
									ref_names[name_index].reference = names[name_index].reference;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "suffix")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].suffix)\
									ref_names[name_index].suffix = names[name_index].suffix;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "avoid_input")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].avoid_input)\
									ref_names[name_index].avoid_input = names[name_index].avoid_input;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "completion_only")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].completion_only)\
									ref_names[name_index].completion_only = names[name_index].completion_only;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "plural")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].plural)\
									ref_names[name_index].plural = names[name_index].plural;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "abbreviation")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].abbreviation)\
									ref_names[name_index].abbreviation = names[name_index].abbreviation;\
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "case_sensitive")) {\
									XML_GET_BOOL_FROM_TEXT(child2, names[name_index].case_sensitive)\
									ref_names[name_index].case_sensitive = names[name_index].case_sensitive;\
									case_set = true;\
								}\
								child2 = child2->next;\
							}\
							if(!case_set) {\
								ref_names[name_index].case_sensitive = ref_names[name_index].abbreviation || text_length_is_one(ref_names[name_index].name);\
								names[name_index].case_sensitive = names[name_index].abbreviation || text_length_is_one(names[name_index].name);\
							}\
							if(names[name_index].reference) {\
								if(!ref_names[name_index].name.empty()) {\
									if(ref_names[name_index].name == names[name_index].name) {\
										ref_names[name_index].name = "";\
									} else {\
										names[name_index].reference = false;\
									}\
								}\
							} else if(!ref_names[name_index].name.empty()) {\
								ref_names[name_index].name = "";\
							}\
						}\
					}

#define ITEM_READ_DTH \
					if(!xmlStrcmp(child->name, (const xmlChar*) "description")) {\
						XML_GET_LOCALE_STRING_FROM_TEXT(child, description, best_description, next_best_description)\
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "title")) {\
						XML_GET_LOCALE_STRING_FROM_TEXT_REQ(child, title, best_title, next_best_title)\
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "hidden")) {\
						XML_GET_TRUE_FROM_TEXT(child, hidden);\
					}

#define ITEM_READ_NAMES \
					if(new_names && ((best_names.empty() && fulfilled_translation != 2) || default_names.empty()) && !xmlStrcmp(child->name, (const xmlChar*) "names")) {\
						value = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);\
 						lang = xmlNodeGetLang(child);\
						if(!lang) {\
							if(default_names.empty()) {\
								if(value) {\
									default_names = (char*) value;\
									remove_blank_ends(default_names);\
								} else {\
									default_names = "";\
								}\
							}\
						} else if(best_names.empty()) {\
							if(locale == (char*) lang) {\
								if(value) {\
									best_names = (char*) value;\
									remove_blank_ends(best_names);\
								} else {\
									best_names = " ";\
								}\
							} else if(nextbest_names.empty() && strlen((char*) lang) >= 2 && fulfilled_translation == 0 && lang[0] == localebase[0] && lang[1] == localebase[1]) {\
								if(value) {\
									nextbest_names = (char*) value;\
									remove_blank_ends(nextbest_names);\
								} else {\
									nextbest_names = " ";\
								}\
							} else if(nextbest_names.empty() && default_names.empty() && value && !require_translation) {\
								nextbest_names = (char*) value;\
								remove_blank_ends(nextbest_names);\
							}\
						}\
						if(value) xmlFree(value);\
						if(lang) xmlFree(lang);\
					}

#define ITEM_INIT_DTH \
					hidden = -1;\
					title = ""; best_title = false; next_best_title = false;\
					description = ""; best_description = false; next_best_description = false;\
					if(fulfilled_translation > 0) require_translation = false; \
					else {XML_GET_TRUE_FROM_PROP(cur, "require_translation", require_translation)}

#define ITEM_INIT_NAME \
					if(new_names) {\
						best_names = "";\
						nextbest_names = "";\
						default_names = "";\
					} else {\
						for(size_t i = 0; i < 10; i++) {\
							best_name[i] = false;\
							nextbest_name[i] = false;\
						}\
					}


#define ITEM_SET_NAME_1(validation)\
					if(!name.empty() && validation(name, version_numbers, is_user_defs)) {\
						ename.name = name;\
						ename.unicode = false;\
						ename.abbreviation = false;\
						ename.case_sensitive = text_length_is_one(ename.name);\
						ename.suffix = false;\
						ename.avoid_input = false;\
						ename.completion_only = false;\
						ename.reference = true;\
						ename.plural = false;\
						item->addName(ename);\
					}

#define ITEM_SET_NAME_2\
					for(size_t i = 0; i < 10; i++) {\
						if(!names[i].name.empty()) {\
							item->addName(names[i], i + 1);\
							names[i].name = "";\
						} else if(!ref_names[i].name.empty()) {\
							item->addName(ref_names[i], i + 1);\
							ref_names[i].name = "";\
						}\
					}

#define ITEM_SET_NAME_3\
					for(size_t i = 0; i < 10; i++) {\
						if(!ref_names[i].name.empty()) {\
							item->addName(ref_names[i]);\
							ref_names[i].name = "";\
						}\
					}

#define ITEM_SET_DTH\
					if(!description.empty()) item->setDescription(description);\
					if(!title.empty() && title[0] == '!') {\
						size_t i = title.find('!', 1);\
						if(i == string::npos) {\
							item->setTitle(title);\
						} else if(i + 1 == title.length()) {\
							item->setTitle("");\
						} else {\
							item->setTitle(title.substr(i + 1, title.length() - (i + 1)));\
						}\
					} else {\
						item->setTitle(title);\
					}\
					if(hidden >= 0) item->setHidden(hidden);

#define ITEM_SET_SHORT_NAME\
					if(!name.empty() && unitNameIsValid(name, version_numbers, is_user_defs)) {\
						ename.name = name;\
						ename.unicode = false;\
						ename.abbreviation = true;\
						ename.case_sensitive = true;\
						ename.suffix = false;\
						ename.avoid_input = false;\
						ename.completion_only = false;\
						ename.reference = true;\
						ename.plural = false;\
						item->addName(ename);\
					}

#define ITEM_SET_SINGULAR\
					if(!singular.empty()) {\
						ename.name = singular;\
						ename.unicode = false;\
						ename.abbreviation = false;\
						ename.case_sensitive = text_length_is_one(ename.name);\
						ename.suffix = false;\
						ename.avoid_input = false;\
						ename.completion_only = false;\
						ename.reference = false;\
						ename.plural = false;\
						item->addName(ename);\
					}

#define ITEM_SET_PLURAL\
					if(!plural.empty()) {\
						ename.name = plural;\
						ename.unicode = false;\
						ename.abbreviation = false;\
						ename.case_sensitive = text_length_is_one(ename.name);\
						ename.suffix = false;\
						ename.avoid_input = false;\
						ename.completion_only = false;\
						ename.reference = false;\
						ename.plural = true;\
						item->addName(ename);\
					}

#define BUILTIN_NAMES_1\
				if(!is_user_defs) item->setRegistered(false);\
					bool has_ref_name;\
					for(size_t i = 1; i <= item->countNames(); i++) {\
						if(item->getName(i).reference) {\
							has_ref_name = false;\
							for(size_t i2 = 0; i2 < 10; i2++) {\
								if(names[i2].name == item->getName(i).name || ref_names[i2].name == item->getName(i).name) {\
									has_ref_name = true;\
									break;\
								}\
							}\
							if(!has_ref_name) {\
								for(int i2 = 9; i2 >= 0; i2--) {\
									if(ref_names[i2].name.empty()) {\
										ref_names[i2] = item->getName(i);\
										break;\
									}\
								}\
							}\
						}\
					}\
					item->clearNames();

#define BUILTIN_UNIT_NAMES_1\
				if(!is_user_defs) item->setRegistered(false);\
					bool has_ref_name;\
					for(size_t i = 1; i <= item->countNames(); i++) {\
						if(item->getName(i).reference) {\
							has_ref_name = item->getName(i).name == singular || item->getName(i).name == plural;\
							for(size_t i2 = 0; !has_ref_name && i2 < 10; i2++) {\
								if(names[i2].name == item->getName(i).name || ref_names[i2].name == item->getName(i).name) {\
									has_ref_name = true;\
									break;\
								}\
							}\
							if(!has_ref_name) {\
								for(int i2 = 9; i2 >= 0; i2--) {\
									if(ref_names[i2].name.empty()) {\
										ref_names[i2] = item->getName(i);\
										break;\
									}\
								}\
							}\
						}\
					}\
					item->clearNames();

#define BUILTIN_NAMES_2\
				if(!is_user_defs) {\
					item->setRegistered(true);\
					nameChanged(item);\
				}

#define ITEM_CLEAR_NAMES\
					for(size_t i = 0; i < 10; i++) {\
						if(!names[i].name.empty()) {\
							names[i].name = "";\
						}\
						if(!ref_names[i].name.empty()) {\
							ref_names[i].name = "";\
						}\
					}

int Calculator::loadDefinitions(const char* file_name, bool is_user_defs, bool check_duplicates) {

	xmlDocPtr doc;
	xmlNodePtr cur, child, child2, child3;
	string version, stmp, name, uname, type, svalue, sexp, plural, countries, singular, category_title, category, description, title, inverse, suncertainty, base, argname, usystem;
	bool unc_rel;
	bool best_title = false, next_best_title = false, best_category_title, next_best_category_title, best_description, next_best_description;
	bool best_plural, next_best_plural, best_singular, next_best_singular, best_argname, next_best_argname, best_countries, next_best_countries;
	bool best_proptitle, next_best_proptitle, best_propdescr, next_best_propdescr;
	string proptitle, propdescr;
	ExpressionName names[10];
	ExpressionName ref_names[10];
	string prop_names[10];
	string ref_prop_names[10];
	bool best_name[10];
	bool nextbest_name[10];
	string best_names, nextbest_names, default_names;
	string best_prop_names, nextbest_prop_names, default_prop_names;
	int name_index, prec;
	ExpressionName ename;

	string locale;
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

	if(b_ignore_locale || locale == "POSIX" || locale == "C") {
		locale = "";
	} else {
		size_t i = locale.find('.');
		if(i != string::npos) locale = locale.substr(0, i);
	}

	int fulfilled_translation = 0;
	string localebase;
	if(locale.length() > 2) {
		localebase = locale.substr(0, 2);
		if(locale == "en_US") {
			fulfilled_translation = 2;
		} else if(localebase == "en") {
			fulfilled_translation = 1;
		}
	} else {
		localebase = locale;
		if(locale == "en") {
			fulfilled_translation = 2;
		}
	}
	while(localebase.length() < 2) {
		localebase += " ";
		fulfilled_translation = 2;
	}

	int exponent = 1, litmp = 0, mix_priority = 0, mix_min = 0;
	bool active = false, b = false, require_translation = false, use_with_prefixes = false, use_with_prefixes_set = false;
	bool b_currency = false;
	int max_prefix = INT_MAX, min_prefix = INT_MIN, default_prefix = 0;
	int hidden = -1;
	Number nr;
	ExpressionItem *item;
	MathFunction *f;
	Variable *v;
	Unit *u;
	AliasUnit *au;
	CompositeUnit *cu;
	Prefix *p;
	Argument *arg;
	DataSet *dc;
	DataProperty *dp;
	int itmp;
	IntegerArgument *iarg;
	NumberArgument *farg;
	xmlChar *value, *lang, *value2;
	int in_unfinished = 0;
	bool done_something = false;
	if(strlen(file_name) > 1 && file_name[0] == '<') {
		doc = xmlParseMemory(file_name, strlen(file_name));
	} else {
		doc = xmlParseFile(file_name);
	}
	if(doc == NULL) {
		return false;
	}
	cur = xmlDocGetRootElement(doc);
	if(cur == NULL) {
		xmlFreeDoc(doc);
		return false;
	}
	while(cur != NULL) {
		if(!xmlStrcmp(cur->name, (const xmlChar*) "QALCULATE")) {
			XML_GET_STRING_FROM_PROP(cur, "version", version)
			break;
		}
		cur = cur->next;
	}
	if(cur == NULL) {
		error(true, _("File not identified as Qalculate! definitions file: %s."), file_name, NULL);
		xmlFreeDoc(doc);
		return false;
	}
	int version_numbers[] = {5, 3, 0};
	parse_qalculate_version(version, version_numbers);

	bool new_names = version_numbers[0] > 0 || version_numbers[1] > 9 || (version_numbers[1] == 9 && version_numbers[2] >= 4);
	bool new_prefix_names = version_numbers[0] > 3 || (version_numbers[0] == 3 && version_numbers[1] > 18);
	bool old_names = VERSION_BEFORE(0, 6, 3);

	ParseOptions po;

	vector<xmlNodePtr> unfinished_nodes;
	vector<string> unfinished_cats;
	queue<xmlNodePtr> sub_items;
	vector<queue<xmlNodePtr> > nodes;

	category = "";
	nodes.resize(1);

	Unit *u_usd = getUnit("USD");
	Unit *u_gbp = getUnit("GBP");
	bool b_remove_cent = false;

	while(true) {
		if(!in_unfinished) {
			category_title = ""; best_category_title = false; next_best_category_title = false;
			child = cur->xmlChildrenNode;
			while(child != NULL) {
				if(!xmlStrcmp(child->name, (const xmlChar*) "title")) {
					XML_GET_LOCALE_STRING_FROM_TEXT(child, category_title, best_category_title, next_best_category_title)
				} else if(!xmlStrcmp(child->name, (const xmlChar*) "category")) {
					nodes.back().push(child);
				} else {
					sub_items.push(child);
				}
				child = child->next;
			}
			if(!category.empty()) {
				category += "/";
			} else if(category_title == "Temporary") {
				category_title = temporaryCategory();
			}
			if(!category_title.empty() && category_title[0] == '!') {\
				size_t i = category_title.find('!', 1);
				if(i == string::npos) {
					category += category_title;
				} else if(i + 1 < category_title.length()) {
					category += category_title.substr(i + 1, category_title.length() - (i + 1));
				}
			} else {
				category += category_title;
			}
		}
		while(!sub_items.empty() || (in_unfinished && cur)) {
			if(!in_unfinished) {
				cur = sub_items.front();
				sub_items.pop();
			}
			if(!xmlStrcmp(cur->name, (const xmlChar*) "activate")) {
				XML_GET_STRING_FROM_TEXT(cur, name)
				ExpressionItem *item = getInactiveExpressionItem(name);
				if(item && !item->isLocal()) {
					item->setActive(true);
					done_something = true;
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "deactivate")) {
				XML_GET_STRING_FROM_TEXT(cur, name)
				ExpressionItem *item = getActiveExpressionItem(name);
				if(item && !item->isLocal()) {
					item->setActive(false);
					done_something = true;
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "function")) {
				if(old_names) {
					XML_GET_STRING_FROM_PROP(cur, "name", name)
				} else {
					name = "";
				}
				XML_GET_FALSE_FROM_PROP(cur, "active", active)
				f = new UserFunction(category, "", "", is_user_defs, 0, "", "", 0, active);
				item = f;
				done_something = true;
				child = cur->xmlChildrenNode;
				ITEM_INIT_DTH
				ITEM_INIT_NAME
				while(child != NULL) {
					if(!xmlStrcmp(child->name, (const xmlChar*) "expression")) {
						XML_DO_FROM_TEXT(child, ((UserFunction*) f)->setFormula);
						XML_GET_PREC_FROM_PROP(child, prec)
						f->setPrecision(prec);
						XML_GET_APPROX_FROM_PROP(child, b)
						f->setApproximate(b);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "condition")) {
						XML_DO_FROM_TEXT(child, f->setCondition);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "subfunction")) {
						XML_GET_FALSE_FROM_PROP(child, "precalculate", b);
						value = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
						if(value) ((UserFunction*) f)->addSubfunction((char*) value, b);
						else ((UserFunction*) f)->addSubfunction("", true);
						if(value) xmlFree(value);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "argument")) {
						farg = NULL; iarg = NULL;
						XML_GET_STRING_FROM_PROP(child, "type", type);
						if(type == "text") {
							arg = new TextArgument();
						} else if(type == "symbol") {
							arg = new SymbolicArgument();
						} else if(type == "date") {
							arg = new DateArgument();
						} else if(type == "integer") {
							iarg = new IntegerArgument();
							arg = iarg;
						} else if(type == "number") {
							farg = new NumberArgument();
							arg = farg;
						} else if(type == "vector") {
							arg = new VectorArgument();
						} else if(type == "matrix") {
							arg = new MatrixArgument();
						} else if(type == "boolean") {
							arg = new BooleanArgument();
						} else if(type == "function") {
							arg = new FunctionArgument();
						} else if(type == "unit") {
							arg = new UnitArgument();
						} else if(type == "variable") {
							arg = new VariableArgument();
						} else if(type == "object") {
							arg = new ExpressionItemArgument();
						} else if(type == "angle") {
							arg = new AngleArgument();
						} else if(type == "data-object") {
							arg = new DataObjectArgument(NULL, "");
						} else if(type == "data-property") {
							arg = new DataPropertyArgument(NULL, "");
						} else {
							arg = new Argument();
							if(!is_user_defs) arg->setHandleVector(true);
						}
						child2 = child->xmlChildrenNode;
						argname = ""; best_argname = false; next_best_argname = false;
						while(child2 != NULL) {
							if(!xmlStrcmp(child2->name, (const xmlChar*) "title")) {
								XML_GET_LOCALE_STRING_FROM_TEXT(child2, argname, best_argname, next_best_argname)
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "min")) {
								if(farg) {
									XML_DO_FROM_TEXT(child2, nr.set);
									farg->setMin(&nr);
									XML_GET_FALSE_FROM_PROP(child2, "include_equals", b)
									farg->setIncludeEqualsMin(b);
								} else if(iarg) {
									XML_GET_STRING_FROM_TEXT(child2, stmp);
									Number integ(stmp);
									iarg->setMin(&integ);
								}
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "max")) {
								if(farg) {
									XML_DO_FROM_TEXT(child2, nr.set);
									farg->setMax(&nr);
									XML_GET_FALSE_FROM_PROP(child2, "include_equals", b)
									farg->setIncludeEqualsMax(b);
								} else if(iarg) {
									XML_GET_STRING_FROM_TEXT(child2, stmp);
									Number integ(stmp);
									iarg->setMax(&integ);
								}
							} else if(farg && !xmlStrcmp(child2->name, (const xmlChar*) "complex_allowed")) {
								XML_GET_FALSE_FROM_TEXT(child2, b);
								farg->setComplexAllowed(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "condition")) {
								XML_DO_FROM_TEXT(child2, arg->setCustomCondition);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "matrix_allowed")) {
								XML_GET_TRUE_FROM_TEXT(child2, b);
								arg->setMatrixAllowed(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "zero_forbidden")) {
								XML_GET_TRUE_FROM_TEXT(child2, b);
								arg->setZeroForbidden(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "test")) {
								XML_GET_FALSE_FROM_TEXT(child2, b);
								arg->setTests(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "handle_vector")) {
								XML_GET_FALSE_FROM_TEXT(child2, b);
								arg->setHandleVector(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "alert")) {
								XML_GET_FALSE_FROM_TEXT(child2, b);
								arg->setAlerts(b);
							}
							child2 = child2->next;
						}
						if(!argname.empty() && argname[0] == '!') {
							size_t i = argname.find('!', 1);
							if(i == string::npos) {
								arg->setName(argname);
							} else if(i + 1 < argname.length()) {
								arg->setName(argname.substr(i + 1, argname.length() - (i + 1)));
							}
						} else {
							arg->setName(argname);
						}
						itmp = 1;
						XML_GET_INT_FROM_PROP(child, "index", itmp);
						f->setArgumentDefinition(itmp, arg);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "example")) {
						XML_DO_FROM_TEXT(child, f->setExample);
					} else ITEM_READ_NAME(functionNameIsValid)
					 else ITEM_READ_DTH
					 else {
						ITEM_READ_NAMES
					}
					child = child->next;
				}
				if(new_names) {
					ITEM_SET_BEST_NAMES(functionNameIsValid)
					ITEM_SET_REFERENCE_NAMES(functionNameIsValid)
				} else {
					ITEM_SET_NAME_1(functionNameIsValid)
					ITEM_SET_NAME_2
					ITEM_SET_NAME_3
				}
				ITEM_SET_DTH
				if(check_duplicates && !is_user_defs) {
					for(size_t i = 1; i <= f->countNames();) {
						if(getActiveFunction(f->getName(i).name)) f->removeName(i);
						else i++;
					}
				}
				if(f->countNames() == 0) {
					f->destroy();
					f = NULL;
				} else {
					f->setChanged(false);
					addFunction(f, true, is_user_defs);
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "dataset") || !xmlStrcmp(cur->name, (const xmlChar*) "builtin_dataset")) {
				bool builtin = !xmlStrcmp(cur->name, (const xmlChar*) "builtin_dataset");
				XML_GET_FALSE_FROM_PROP(cur, "active", active)
				if(builtin) {
					XML_GET_STRING_FROM_PROP(cur, "name", name)
					dc = getDataSet(name);
					if(!dc) {
						goto after_load_object;
					}
					dc->setCategory(category);
				} else {
					dc = new DataSet(category, "", "", "", "", is_user_defs);
				}
				item = dc;
				done_something = true;
				child = cur->xmlChildrenNode;
				ITEM_INIT_DTH
				ITEM_INIT_NAME
				while(child != NULL) {
					if(!xmlStrcmp(child->name, (const xmlChar*) "property")) {
						dp = new DataProperty(dc);
						child2 = child->xmlChildrenNode;
						if(new_names) {
							default_prop_names = ""; best_prop_names = ""; nextbest_prop_names = "";
						} else {
							for(size_t i = 0; i < 10; i++) {
								best_name[i] = false;
								nextbest_name[i] = false;
							}
						}
						proptitle = ""; best_proptitle = false; next_best_proptitle = false;
						propdescr = ""; best_propdescr = false; next_best_propdescr = false;
						while(child2 != NULL) {
							if(!xmlStrcmp(child2->name, (const xmlChar*) "title")) {
								XML_GET_LOCALE_STRING_FROM_TEXT(child2, proptitle, best_proptitle, next_best_proptitle)
							} else if(!new_names && !xmlStrcmp(child2->name, (const xmlChar*) "name")) {
								name_index = 1;
								XML_GET_INT_FROM_PROP(child2, "index", name_index)
								if(name_index > 0 && name_index <= 10) {
									name_index--;
									prop_names[name_index] = "";
									ref_prop_names[name_index] = "";
									value2 = NULL;
									child3 = child2->xmlChildrenNode;
									while(child3 != NULL) {
										if((!best_name[name_index] || (ref_prop_names[name_index].empty() && !locale.empty())) && !xmlStrcmp(child3->name, (const xmlChar*) "name")) {
											lang = xmlNodeGetLang(child3);
											if(!lang) {
												value2 = xmlNodeListGetString(doc, child3->xmlChildrenNode, 1);
												if(locale.empty()) {
													best_name[name_index] = true;
													if(value2) prop_names[name_index] = (char*) value2;
													else prop_names[name_index] = "";
												} else {
													if(!best_name[name_index] && !nextbest_name[name_index]) {
														if(value2) prop_names[name_index] = (char*) value2;
														else prop_names[name_index] = "";
													}
													if(value2) ref_prop_names[name_index] = (char*) value2;
													else ref_prop_names[name_index] = "";
												}
											} else if(!best_name[name_index] && !locale.empty()) {
												if(locale == (char*) lang) {
													value2 = xmlNodeListGetString(doc, child3->xmlChildrenNode, 1);
													best_name[name_index] = true;
													if(value2) prop_names[name_index] = (char*) value2;
													else prop_names[name_index] = "";
												} else if(!nextbest_name[name_index] && strlen((char*) lang) >= 2 && fulfilled_translation == 0 && lang[0] == localebase[0] && lang[1] == localebase[1]) {
													value2 = xmlNodeListGetString(doc, child3->xmlChildrenNode, 1);
													nextbest_name[name_index] = true;
													if(value2) prop_names[name_index] = (char*) value2;
													else prop_names[name_index] = "";
												}
											}
											if(value2) xmlFree(value2);
											if(lang) xmlFree(lang);
											value2 = NULL; lang = NULL;
										}
										child3 = child3->next;
									}
									if(!ref_prop_names[name_index].empty() && ref_prop_names[name_index] == prop_names[name_index]) {
										ref_prop_names[name_index] = "";
									}
								}
							} else if(new_names && !xmlStrcmp(child2->name, (const xmlChar*) "names") && ((best_prop_names.empty() && fulfilled_translation != 2) || default_prop_names.empty())) {
									value2 = xmlNodeListGetString(doc, child2->xmlChildrenNode, 1);
 									lang = xmlNodeGetLang(child2);
									if(!lang) {
										if(default_prop_names.empty()) {
											if(value2) {
												default_prop_names = (char*) value2;
												remove_blank_ends(default_prop_names);
											} else {
												default_prop_names = "";
											}
										}
									} else {
										if(locale == (char*) lang) {
											if(value2) {
												best_prop_names = (char*) value2;
												remove_blank_ends(best_prop_names);
											} else {
												best_prop_names = " ";
											}
									} else if(nextbest_prop_names.empty() && strlen((char*) lang) >= 2 && fulfilled_translation == 0 && lang[0] == localebase[0] && lang[1] == localebase[1]) {
										if(value2) {
											nextbest_prop_names = (char*) value2;
											remove_blank_ends(nextbest_prop_names);
										} else {
											nextbest_prop_names = " ";
										}
									} else if(nextbest_prop_names.empty() && default_prop_names.empty() && value2 && !require_translation) {
										nextbest_prop_names = (char*) value2;
										remove_blank_ends(nextbest_prop_names);
									}
								}
								if(value2) xmlFree(value2);
								if(lang) xmlFree(lang);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "description")) {
								XML_GET_LOCALE_STRING_FROM_TEXT(child2, propdescr, best_propdescr, next_best_propdescr)
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "unit")) {
								XML_DO_FROM_TEXT(child2, dp->setUnit)
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "key")) {
								XML_GET_TRUE_FROM_TEXT(child2, b)
								dp->setKey(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "hidden")) {
								XML_GET_TRUE_FROM_TEXT(child2, b)
								dp->setHidden(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "brackets")) {
								XML_GET_TRUE_FROM_TEXT(child2, b)
								dp->setUsesBrackets(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "approximate")) {
								XML_GET_TRUE_FROM_TEXT(child2, b)
								dp->setApproximate(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "case_sensitive")) {
								XML_GET_TRUE_FROM_TEXT(child2, b)
								dp->setCaseSensitive(b);
							} else if(!xmlStrcmp(child2->name, (const xmlChar*) "type")) {
								XML_GET_STRING_FROM_TEXT(child2, stmp)
								if(stmp == "text") {
									dp->setPropertyType(PROPERTY_STRING);
								} else if(stmp == "number") {
									dp->setPropertyType(PROPERTY_NUMBER);
								} else if(stmp == "expression") {
									dp->setPropertyType(PROPERTY_EXPRESSION);
								}
							}
							child2 = child2->next;
						}
						if(!proptitle.empty() && proptitle[0] == '!') {\
							size_t i = proptitle.find('!', 1);
							if(i == string::npos) {
								dp->setTitle(proptitle);
							} else if(i + 1 < proptitle.length()) {
								dp->setTitle(proptitle.substr(i + 1, proptitle.length() - (i + 1)));
							}
						} else {
							dp->setTitle(proptitle);
						}
						dp->setDescription(propdescr);
						if(new_names) {
							size_t names_i = 0, i2 = 0;
							string *str_names;
							bool had_ref = false;
							if(best_prop_names == "-") {best_prop_names = ""; nextbest_prop_names = "";}
							if(!best_prop_names.empty()) {str_names = &best_prop_names;}
							else if(!nextbest_prop_names.empty()) {str_names = &nextbest_prop_names;}
							else {str_names = &default_prop_names;}
							if(!str_names->empty() && (*str_names)[0] == '!') {
								names_i = str_names->find('!', 1) + 1;
							}
							while(true) {
								size_t i3 = names_i;
								names_i = str_names->find(",", i3);
								if(i2 == 0) {
									i2 = str_names->find(":", i3);
								}
								bool b_prop_ref = false;
								if(i2 < names_i) {
									bool b = true;
									for(; i3 < i2; i3++) {
										switch((*str_names)[i3]) {
											case '-': {b = false; break;}
											case 'r': {b_prop_ref = b; b = true; break;}
										}
									}
									i3++;
									i2 = 0;
								}
								if(names_i == string::npos) {stmp = str_names->substr(i3, str_names->length() - i3);}
								else {stmp = str_names->substr(i3, names_i - i3);}
								remove_blank_ends(stmp);
								if(!stmp.empty()) {
									if(b_prop_ref) had_ref = true;
									dp->addName(stmp, b_prop_ref);
								}
								if(names_i == string::npos) {break;}
								names_i++;
							}
							if(str_names != &default_prop_names && !default_prop_names.empty()) {
								if(default_prop_names[0] == '!') {
									names_i = default_prop_names.find('!', 1) + 1;
								} else {
									names_i = 0;
								}
								i2 = 0;
								while(true) {
									size_t i3 = names_i;
									names_i = default_prop_names.find(",", i3);
									if(i2 == 0) {
										i2 = default_prop_names.find(":", i3);
									}
									bool b_prop_ref = false;
									if(i2 < names_i) {
										bool b = true;
										for(; i3 < i2; i3++) {
											switch(default_prop_names[i3]) {
												case '-': {b = false; break;}
												case 'r': {b_prop_ref = b; b = true; break;}
											}
										}
										i3++;
										i2 = 0;
									}
									if(b_prop_ref || (!had_ref && names_i == string::npos)) {
										had_ref = true;
										if(names_i == string::npos) {stmp = default_prop_names.substr(i3, default_prop_names.length() - i3);}
										else {stmp = default_prop_names.substr(i3, names_i - i3);}
										remove_blank_ends(stmp);
										size_t i4 = dp->hasName(stmp);
										if(i4 > 0) {
											dp->setNameIsReference(i4, true);
										} else if(!stmp.empty()) {
											dp->addName(stmp, true);
										}
									}
									if(names_i == string::npos) {break;}
									names_i++;
								}
							}
							if(!had_ref && dp->countNames() > 0) dp->setNameIsReference(1, true);
						} else {
							bool b = false;
							for(size_t i = 0; i < 10; i++) {
								if(!prop_names[i].empty()) {
									if(!b && ref_prop_names[i].empty()) {
										dp->addName(prop_names[i], true, i + 1);
										b = true;
									} else {
										dp->addName(prop_names[i], false, i + 1);
									}
									prop_names[i] = "";
								}
							}
							for(size_t i = 0; i < 10; i++) {
								if(!ref_prop_names[i].empty()) {
									if(!b) {
										dp->addName(ref_prop_names[i], true);
										b = true;
									} else {
										dp->addName(ref_prop_names[i], false);
									}
									ref_prop_names[i] = "";
								}
							}
						}
						dp->setUserModified(is_user_defs);
						dc->addProperty(dp);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "argument")) {
						child2 = child->xmlChildrenNode;
						argname = ""; best_argname = false; next_best_argname = false;
						while(child2 != NULL) {
							if(!xmlStrcmp(child2->name, (const xmlChar*) "title")) {
								XML_GET_LOCALE_STRING_FROM_TEXT(child2, argname, best_argname, next_best_argname)
							}
							child2 = child2->next;
						}
						itmp = 1;
						XML_GET_INT_FROM_PROP(child, "index", itmp);
						if(dc->getArgumentDefinition(itmp)) {
							dc->getArgumentDefinition(itmp)->setName(argname);
						}
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "object_argument")) {
						child2 = child->xmlChildrenNode;
						argname = ""; best_argname = false; next_best_argname = false;
						while(child2 != NULL) {
							if(!xmlStrcmp(child2->name, (const xmlChar*) "title")) {
								XML_GET_LOCALE_STRING_FROM_TEXT(child2, argname, best_argname, next_best_argname)
							}
							child2 = child2->next;
						}
						itmp = 1;
						if(dc->getArgumentDefinition(itmp)) {
							if(!argname.empty() && argname[0] == '!') {
								size_t i = argname.find('!', 1);
								if(i == string::npos) {
									dc->getArgumentDefinition(itmp)->setName(argname);
								} else if(i + 1 < argname.length()) {
									dc->getArgumentDefinition(itmp)->setName(argname.substr(i + 1, argname.length() - (i + 1)));
								}
							} else {
								dc->getArgumentDefinition(itmp)->setName(argname);
							}
						}
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "property_argument")) {
						child2 = child->xmlChildrenNode;
						argname = ""; best_argname = false; next_best_argname = false;
						while(child2 != NULL) {
							if(!xmlStrcmp(child2->name, (const xmlChar*) "title")) {
								XML_GET_LOCALE_STRING_FROM_TEXT(child2, argname, best_argname, next_best_argname)
							}
							child2 = child2->next;
						}
						itmp = 2;
						if(dc->getArgumentDefinition(itmp)) {
							if(!argname.empty() && argname[0] == '!') {
								size_t i = argname.find('!', 1);
								if(i == string::npos) {
									dc->getArgumentDefinition(itmp)->setName(argname);
								} else if(i + 1 < argname.length()) {
									dc->getArgumentDefinition(itmp)->setName(argname.substr(i + 1, argname.length() - (i + 1)));
								}
							} else {
								dc->getArgumentDefinition(itmp)->setName(argname);
							}
						}
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "default_property")) {
						XML_DO_FROM_TEXT(child, dc->setDefaultProperty)
					} else if(!builtin && !xmlStrcmp(child->name, (const xmlChar*) "copyright")) {
						XML_DO_FROM_TEXT(child, dc->setCopyright)
					} else if(!builtin && !xmlStrcmp(child->name, (const xmlChar*) "datafile")) {
						XML_DO_FROM_TEXT(child, dc->setDefaultDataFile)
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "example")) {
						XML_DO_FROM_TEXT(child, dc->setExample);
					} else ITEM_READ_NAME(functionNameIsValid)
					 else ITEM_READ_DTH
					 else {
						ITEM_READ_NAMES
					}
					child = child->next;
				}
				if(new_names) {
					if(builtin) {
						ITEM_SAVE_BUILTIN_NAMES
					}
					ITEM_SET_BEST_NAMES(functionNameIsValid)
					ITEM_SET_REFERENCE_NAMES(functionNameIsValid)
					if(builtin) {
						ITEM_SET_BUILTIN_NAMES
					}
				} else {
					if(builtin) {
						BUILTIN_NAMES_1
					}
					ITEM_SET_NAME_2
					ITEM_SET_NAME_3
					if(builtin) {
						BUILTIN_NAMES_2
					}
				}
				ITEM_SET_DTH
				if(check_duplicates && !is_user_defs) {
					for(size_t i = 1; i <= dc->countNames();) {
						if(getActiveFunction(dc->getName(i).name)) dc->removeName(i);
						else i++;
					}
				}
				if(!builtin && dc->countNames() == 0) {
					dc->destroy();
					dc = NULL;
				} else {
					dc->setChanged(builtin && is_user_defs);
					if(!builtin) addDataSet(dc, true, is_user_defs);
				}
				done_something = true;
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "builtin_function")) {
				XML_GET_STRING_FROM_PROP(cur, "name", name)
				f = getFunction(name);
				if(f) {
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					f->setLocal(is_user_defs, active);
					f->setCategory(category);
					item = f;
					child = cur->xmlChildrenNode;
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*) "argument")) {
							child2 = child->xmlChildrenNode;
							argname = ""; best_argname = false; next_best_argname = false;
							while(child2 != NULL) {
								if(!xmlStrcmp(child2->name, (const xmlChar*) "title")) {
									XML_GET_LOCALE_STRING_FROM_TEXT(child2, argname, best_argname, next_best_argname)
								}
								child2 = child2->next;
							}
							itmp = 1;
							XML_GET_INT_FROM_PROP(child, "index", itmp);
							if(f->getArgumentDefinition(itmp)) {
								if(!argname.empty() && argname[0] == '!') {
									size_t i = argname.find('!', 1);
									if(i == string::npos) {
										f->getArgumentDefinition(itmp)->setName(argname);
									} else if(i + 1 < argname.length()) {
										f->getArgumentDefinition(itmp)->setName(argname.substr(i + 1, argname.length() - (i + 1)));
									}
								} else {
									f->getArgumentDefinition(itmp)->setName(argname);
								}
							} else if(f->maxargs() < 0 || itmp <= f->maxargs()) {
								if(!argname.empty() && argname[0] == '!') {
									size_t i = argname.find('!', 1);
									if(i == string::npos) {
										f->setArgumentDefinition(itmp, new Argument(argname, false));
									} else if(i + 1 < argname.length()) {
										f->setArgumentDefinition(itmp, new Argument(argname.substr(i + 1, argname.length() - (i + 1)), false));
									}
								} else {
									f->setArgumentDefinition(itmp, new Argument(argname, false));
								}
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "example")) {
							XML_DO_FROM_TEXT(child, f->setExample);
						} else ITEM_READ_NAME(functionNameIsValid)
						 else ITEM_READ_DTH
						 else {
							ITEM_READ_NAMES
						}
						child = child->next;
					}
					if(new_names) {
						ITEM_SAVE_BUILTIN_NAMES
						ITEM_SET_BEST_NAMES(functionNameIsValid)
						ITEM_SET_REFERENCE_NAMES(functionNameIsValid)
						ITEM_SET_BUILTIN_NAMES
					} else {
						BUILTIN_NAMES_1
						ITEM_SET_NAME_2
						ITEM_SET_NAME_3
						BUILTIN_NAMES_2
					}
					ITEM_SET_DTH
					f->setChanged(false);
					done_something = true;
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "unknown")) {
				if(old_names) {
					XML_GET_STRING_FROM_PROP(cur, "name", name)
				} else {
					name = "";
				}
				XML_GET_FALSE_FROM_PROP(cur, "active", active)
				svalue = "";
				v = new UnknownVariable(category, "", "", is_user_defs, false, active);
				item = v;
				done_something = true;
				child = cur->xmlChildrenNode;
				b = true;
				ITEM_INIT_DTH
				ITEM_INIT_NAME
				while(child != NULL) {
					if(!xmlStrcmp(child->name, (const xmlChar*) "type")) {
						XML_GET_STRING_FROM_TEXT(child, stmp);
						if(!((UnknownVariable*) v)->assumptions()) ((UnknownVariable*) v)->setAssumptions(new Assumptions());
						if(stmp == "integer") ((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_INTEGER);
						else if(stmp == "boolean") ((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_BOOLEAN);
						else if(stmp == "rational") ((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_RATIONAL);
						else if(stmp == "real") ((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_REAL);
						else if(stmp == "complex") ((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_COMPLEX);
						else if(stmp == "number") ((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_NUMBER);
						else if(stmp == "non-matrix") {
							if(VERSION_BEFORE(0, 9, 13)) {
								((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_NUMBER);
							} else {
								((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_NONMATRIX);
							}
						} else if(stmp == "none") {
							if(VERSION_BEFORE(0, 9, 13)) {
								((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_NUMBER);
							} else {
								((UnknownVariable*) v)->assumptions()->setType(ASSUMPTION_TYPE_NONE);
							}
						}
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "sign")) {
						XML_GET_STRING_FROM_TEXT(child, stmp);
						if(!((UnknownVariable*) v)->assumptions()) ((UnknownVariable*) v)->setAssumptions(new Assumptions());
						if(stmp == "non-zero") ((UnknownVariable*) v)->assumptions()->setSign(ASSUMPTION_SIGN_NONZERO);
						else if(stmp == "non-positive") ((UnknownVariable*) v)->assumptions()->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
						else if(stmp == "negative") ((UnknownVariable*) v)->assumptions()->setSign(ASSUMPTION_SIGN_NEGATIVE);
						else if(stmp == "non-negative") ((UnknownVariable*) v)->assumptions()->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
						else if(stmp == "positive") ((UnknownVariable*) v)->assumptions()->setSign(ASSUMPTION_SIGN_POSITIVE);
						else if(stmp == "unknown") ((UnknownVariable*) v)->assumptions()->setSign(ASSUMPTION_SIGN_UNKNOWN);
					} else ITEM_READ_NAME(variableNameIsValid)
					 else ITEM_READ_DTH
					 else {
						ITEM_READ_NAMES
					}
					child = child->next;
				}
				if(new_names) {
					ITEM_SET_BEST_NAMES(variableNameIsValid)
					ITEM_SET_REFERENCE_NAMES(variableNameIsValid)
				} else {
					ITEM_SET_NAME_1(variableNameIsValid)
					ITEM_SET_NAME_2
					ITEM_SET_NAME_3
				}
				ITEM_SET_DTH
				if(check_duplicates && !is_user_defs) {
					for(size_t i = 1; i <= v->countNames();) {
						if(getActiveVariable(v->getName(i).name) || getActiveUnit(v->getName(i).name) || getCompositeUnit(v->getName(i).name)) v->removeName(i);
						else i++;
					}
				}
				for(size_t i = 1; i <= v->countNames(); i++) {
					if(v->getName(i).name == "x") {v_x->destroy(); v_x = (UnknownVariable*) v; break;}
					if(v->getName(i).name == "y") {v_y->destroy(); v_y = (UnknownVariable*) v; break;}
					if(v->getName(i).name == "z") {v_z->destroy(); v_z = (UnknownVariable*) v; break;}
				}
				if(v->countNames() == 0) {
					v->destroy();
					v = NULL;
				} else {
					addVariable(v, true, is_user_defs);
					v->setChanged(false);
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "variable")) {
				if(old_names) {
					XML_GET_STRING_FROM_PROP(cur, "name", name)
				} else {
					name = "";
				}
				XML_GET_FALSE_FROM_PROP(cur, "active", active)
				svalue = "";
				v = new KnownVariable(category, "", "", "", is_user_defs, false, active);
				item = v;
				done_something = true;
				child = cur->xmlChildrenNode;
				b = true;
				ITEM_INIT_DTH
				ITEM_INIT_NAME
				while(child != NULL) {
					if(!xmlStrcmp(child->name, (const xmlChar*) "value")) {
						XML_DO_FROM_TEXT(child, ((KnownVariable*) v)->set);
						XML_GET_STRING_FROM_PROP(child, "relative_uncertainty", suncertainty)
						unc_rel = false;
						if(suncertainty.empty()) {XML_GET_STRING_FROM_PROP(child, "uncertainty", suncertainty)}
						else unc_rel = true;
						((KnownVariable*) v)->setUncertainty(suncertainty, unc_rel);
						XML_DO_FROM_PROP(child, "unit", ((KnownVariable*) v)->setUnit)
						XML_GET_PREC_FROM_PROP(child, prec)
						v->setPrecision(prec);
						XML_GET_APPROX_FROM_PROP(child, b);
						if(b) v->setApproximate(true);
					} else ITEM_READ_NAME(variableNameIsValid)
					 else ITEM_READ_DTH
					 else {
						ITEM_READ_NAMES
					}
					child = child->next;
				}
				if(new_names) {
					ITEM_SET_BEST_NAMES(variableNameIsValid)
					ITEM_SET_REFERENCE_NAMES(variableNameIsValid)
				} else {
					ITEM_SET_NAME_1(variableNameIsValid)
					ITEM_SET_NAME_2
					ITEM_SET_NAME_3
				}
				ITEM_SET_DTH
				if(check_duplicates && !is_user_defs) {
					for(size_t i = 1; i <= v->countNames();) {
						if(getActiveVariable(v->getName(i).name) || getActiveUnit(v->getName(i).name) || getCompositeUnit(v->getName(i).name)) v->removeName(i);
						else i++;
					}
				}
				if(v->countNames() == 0) {
					v->destroy();
					v = NULL;
				} else {
					addVariable(v, true, is_user_defs);
					item->setChanged(false);
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "builtin_variable")) {
				XML_GET_STRING_FROM_PROP(cur, "name", name)
				v = getVariable(name);
				if(v) {
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					v->setLocal(is_user_defs, active);
					v->setCategory(category);
					item = v;
					child = cur->xmlChildrenNode;
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					while(child != NULL) {
						ITEM_READ_NAME(variableNameIsValid)
						 else ITEM_READ_DTH
						 else {
							ITEM_READ_NAMES
						}
						child = child->next;
					}
					if(new_names) {
						ITEM_SAVE_BUILTIN_NAMES
						ITEM_SET_BEST_NAMES(variableNameIsValid)
						ITEM_SET_REFERENCE_NAMES(variableNameIsValid)
						ITEM_SET_BUILTIN_NAMES
					} else {
						BUILTIN_NAMES_1
						ITEM_SET_NAME_2
						ITEM_SET_NAME_3
						BUILTIN_NAMES_2
					}
					ITEM_SET_DTH
					v->setChanged(false);
					done_something = true;
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "unit")) {
				XML_GET_STRING_FROM_PROP(cur, "type", type)
				if(type == "base") {
					if(old_names) {
						XML_GET_STRING_FROM_PROP(cur, "name", name)
					} else {
						name = "";
					}
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					u = new Unit(category, "", "", "", "", is_user_defs, false, active);
					item = u;
					child = cur->xmlChildrenNode;
					singular = ""; best_singular = false; next_best_singular = false;
					plural = ""; best_plural = false; next_best_plural = false;
					countries = "", best_countries = false, next_best_countries = false;
					use_with_prefixes_set = false;
					max_prefix = INT_MAX, min_prefix = INT_MIN, default_prefix = 0;
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*) "system")) {
							XML_DO_FROM_TEXT(child, u->setSystem)
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "use_with_prefixes")) {
							XML_GET_TRUE_FROM_TEXT(child, use_with_prefixes)
							XML_GET_INT_FROM_PROP(child, "max", max_prefix)
							XML_GET_INT_FROM_PROP(child, "min", min_prefix)
							XML_GET_INT_FROM_PROP(child, "default", default_prefix)
							use_with_prefixes_set = true;
						} else if(old_names && !xmlStrcmp(child->name, (const xmlChar*) "singular")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, singular, best_singular, next_best_singular)
							if(!unitNameIsValid(singular, version_numbers, is_user_defs)) {
								singular = "";
							}
						} else if(old_names && !xmlStrcmp(child->name, (const xmlChar*) "plural") && !xmlHasProp(child, (xmlChar*) "index")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, plural, best_plural, next_best_plural)
							if(!unitNameIsValid(plural, version_numbers, is_user_defs)) {
								plural = "";
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "countries")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, countries, best_countries, next_best_countries)
						} else ITEM_READ_NAME(unitNameIsValid)
						 else ITEM_READ_DTH
						 else {
							ITEM_READ_NAMES
						}
						child = child->next;
					}
					u->setCountries(countries);
					if(new_names) {
						ITEM_SET_BEST_NAMES(unitNameIsValid)
						ITEM_SET_REFERENCE_NAMES(unitNameIsValid)
					} else {
						ITEM_SET_SHORT_NAME
						ITEM_SET_SINGULAR
						ITEM_SET_PLURAL
						ITEM_SET_NAME_2
						ITEM_SET_NAME_3
					}
					ITEM_SET_DTH
					if(use_with_prefixes_set) {
						u->setUseWithPrefixesByDefault(use_with_prefixes);
						u->setMaxPreferredPrefix(max_prefix);
						u->setMinPreferredPrefix(min_prefix);
						u->setDefaultPrefix(default_prefix);
					}
					if(check_duplicates && !is_user_defs) {
						for(size_t i = 1; i <= u->countNames();) {
							if(getActiveVariable(u->getName(i).name) || getActiveUnit(u->getName(i).name) || getCompositeUnit(u->getName(i).name)) u->removeName(i);
							else i++;
						}
					}
					if(u->countNames() == 0) {
						u->destroy();
						u = NULL;
					} else {
						if(!is_user_defs && u->referenceName() == "s") u_second = u;
						else if(!is_user_defs && u->referenceName() == "K") priv->u_kelvin = u;
						addUnit(u, true, is_user_defs);
						u->setChanged(false);
					}
					done_something = true;
				} else if(type == "alias") {
					if(old_names) {
						XML_GET_STRING_FROM_PROP(cur, "name", name)
					} else {
						name = "";
					}
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					u = NULL;
					child = cur->xmlChildrenNode;
					singular = ""; best_singular = false; next_best_singular = false;
					plural = ""; best_plural = false; next_best_plural = false;
					countries = "", best_countries = false, next_best_countries = false;
					b_currency = false;
					use_with_prefixes_set = false;
					max_prefix = INT_MAX, min_prefix = INT_MIN, default_prefix = 0;
					usystem = "";
					prec = -1;
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					unc_rel = false;
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*) "base")) {
							child2 = child->xmlChildrenNode;
							exponent = 1;
							mix_priority = 0;
							mix_min = 0;
							svalue = "";
							inverse = "";
							suncertainty = "";
							b = true;
							while(child2 != NULL) {
								if(!xmlStrcmp(child2->name, (const xmlChar*) "unit")) {
									XML_GET_STRING_FROM_TEXT(child2, base);
									u = getActiveUnit(base);
									if(!u) u = getCompositeUnit(base);
									if(!u) getUnit(base);
									b_currency = (!is_user_defs && u && u == u_euro);
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "relation")) {
									XML_GET_STRING_FROM_TEXT(child2, svalue);
									XML_GET_APPROX_FROM_PROP(child2, b)
									XML_GET_PREC_FROM_PROP(child2, prec)
									XML_GET_STRING_FROM_PROP(child2, "relative_uncertainty", suncertainty)
									if(suncertainty.empty()) {XML_GET_STRING_FROM_PROP(child2, "uncertainty", suncertainty)}
									else unc_rel = true;
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "reverse_relation")) {
									XML_GET_STRING_FROM_TEXT(child2, inverse);
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "inverse_relation")) {
									XML_GET_STRING_FROM_TEXT(child2, inverse);
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "exponent")) {
									XML_GET_STRING_FROM_TEXT(child2, stmp);
									if(stmp.empty()) {
										exponent = 1;
									} else {
										exponent = s2i(stmp);
									}
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "mix")) {
									XML_GET_INT_FROM_PROP(child2, "min", mix_min);
									XML_GET_STRING_FROM_TEXT(child2, stmp);
									if(stmp.empty()) {
										mix_priority = 0;
									} else {
										mix_priority = s2i(stmp);
									}
								}
								child2 = child2->next;
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "system")) {
							XML_GET_STRING_FROM_TEXT(child, usystem);
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "use_with_prefixes")) {
							XML_GET_TRUE_FROM_TEXT(child, use_with_prefixes)
							XML_GET_INT_FROM_PROP(child, "max", max_prefix)
							XML_GET_INT_FROM_PROP(child, "min", min_prefix)
							XML_GET_INT_FROM_PROP(child, "default", default_prefix)
							use_with_prefixes_set = true;
						} else if(old_names && !xmlStrcmp(child->name, (const xmlChar*) "singular")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, singular, best_singular, next_best_singular)
							if(!unitNameIsValid(singular, version_numbers, is_user_defs)) {
								singular = "";
							}
						} else if(old_names && !xmlStrcmp(child->name, (const xmlChar*) "plural") && !xmlHasProp(child, (xmlChar*) "index")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, plural, best_plural, next_best_plural)
							if(!unitNameIsValid(plural, version_numbers, is_user_defs)) {
								plural = "";
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "countries")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, countries, best_countries, next_best_countries)
						} else ITEM_READ_NAME(unitNameIsValid)
						 else ITEM_READ_DTH
						 else {
							ITEM_READ_NAMES
						}
						child = child->next;
					}
					if(!u) {
						ITEM_CLEAR_NAMES
						if(!in_unfinished) {
							unfinished_nodes.push_back(cur);
							unfinished_cats.push_back(category);
						}
					} else {
						au = new AliasUnit(category, name, plural, singular, title, u, svalue, exponent, inverse, is_user_defs, false, active);
						au->setCountries(countries);
						if(mix_priority > 0) {
							au->setMixWithBase(mix_priority);
							au->setMixWithBaseMinimum(mix_min);
						}
						au->setDescription(description);
						au->setPrecision(prec);
						if(b) au->setApproximate(true);
						au->setUncertainty(suncertainty, unc_rel);
						if(hidden >= 0) au->setHidden(hidden);
						au->setSystem(usystem);
						if(use_with_prefixes_set) {
							au->setUseWithPrefixesByDefault(use_with_prefixes);
							au->setMaxPreferredPrefix(max_prefix);
							au->setMinPreferredPrefix(min_prefix);
							au->setDefaultPrefix(default_prefix);
						}
						item = au;
						if(new_names) {
							ITEM_SET_BEST_NAMES(unitNameIsValid)
							ITEM_SET_REFERENCE_NAMES(unitNameIsValid)
						} else {
							ITEM_SET_NAME_2
							ITEM_SET_NAME_3
						}
						if(b_currency && !au->referenceName().empty()) {
							u = getUnit(au->referenceName());
							if(u && u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->baseUnit() == u_euro) u->destroy();
						}
						if(check_duplicates && !is_user_defs) {
							for(size_t i = 1; i <= au->countNames();) {
								if(getActiveVariable(au->getName(i).name) || getActiveUnit(au->getName(i).name) || getCompositeUnit(au->getName(i).name)) au->removeName(i);
								else i++;
							}
						}
						if(au->countNames() == 0 || (b_remove_cent && au->firstBaseUnit() == u_usd && au->hasName("cent"))) {
							au->destroy();
							au = NULL;
						} else {
							if(!is_user_defs && au->baseUnit() == u_second) {
								if(au->referenceName() == "d" || au->referenceName() == "day") u_day = au;
								else if(au->referenceName() == "year") u_year = au;
								else if(au->referenceName() == "month") u_month = au;
								else if(au->referenceName() == "min") u_minute = au;
								else if(au->referenceName() == "h") u_hour = au;
							} else if(!is_user_defs && au->baseUnit() == priv->u_kelvin) {
								if(au->referenceName() == "oR") priv->u_rankine = au;
								else if(au->referenceName() == "oC") priv->u_celsius = au;
								else if(au->referenceName() == "oF") priv->u_fahrenheit = au;
							}
							addUnit(au, true, is_user_defs);
							au->setChanged(false);
						}
						done_something = true;
					}
				} else if(type == "composite") {
					if(old_names) {
						XML_GET_STRING_FROM_PROP(cur, "name", name)
					} else {
						name = "";
					}
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					child = cur->xmlChildrenNode;
					usystem = "";
					use_with_prefixes_set = false;
					max_prefix = INT_MAX, min_prefix = INT_MIN, default_prefix = 0;
					cu = NULL;
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					b = true;
					while(child != NULL) {
						u = NULL;
						if(!xmlStrcmp(child->name, (const xmlChar*) "part")) {
							child2 = child->xmlChildrenNode;
							p = NULL;
							exponent = 1;
							while(child2 != NULL) {
								if(!xmlStrcmp(child2->name, (const xmlChar*) "unit")) {
									XML_GET_STRING_FROM_TEXT(child2, base);
									u = getUnit(base);
									if(!u) {
										u = getCompositeUnit(base);
									}
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "prefix")) {
									XML_GET_STRING_FROM_PROP(child2, "type", stmp)
									XML_GET_STRING_FROM_TEXT(child2, svalue);
									p = NULL;
									if(stmp == "binary") {
										litmp = s2i(svalue);
										if(litmp != 0) {
											p = getExactBinaryPrefix(litmp);
											if(!p) b = false;
										}
									} else if(stmp == "number") {
										nr.set(stmp);
										if(!nr.isZero()) {
											p = getExactPrefix(stmp);
											if(!p) b = false;
										}
									} else {
										litmp = s2i(svalue);
										if(litmp != 0) {
											p = getExactDecimalPrefix(litmp);
											if(!p) b = false;
										}
									}
									if(!b) {
										if(cu) {
											delete cu;
										}
										cu = NULL;
										break;
									}
								} else if(!xmlStrcmp(child2->name, (const xmlChar*) "exponent")) {
									XML_GET_STRING_FROM_TEXT(child2, stmp);
									if(stmp.empty()) {
										exponent = 1;
									} else {
										exponent = s2i(stmp);
									}
								}
								child2 = child2->next;
							}
							if(!b) break;
							if(u) {
								if(!cu) {
									cu = new CompositeUnit("", "", "", "", is_user_defs, false, active);
								}
								cu->add(u, exponent, p);
							} else {
								if(cu) delete cu;
								cu = NULL;
								if(!in_unfinished) {
									unfinished_nodes.push_back(cur);
									unfinished_cats.push_back(category);
								}
								break;
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "system")) {
							XML_GET_STRING_FROM_TEXT(child, usystem);
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "use_with_prefixes")) {
							XML_GET_TRUE_FROM_TEXT(child, use_with_prefixes)
							XML_GET_INT_FROM_PROP(child, "max", max_prefix)
							XML_GET_INT_FROM_PROP(child, "min", min_prefix)
							XML_GET_INT_FROM_PROP(child, "default", default_prefix)
							use_with_prefixes_set = true;
						} else ITEM_READ_NAME(unitNameIsValid)
						 else ITEM_READ_DTH
						 else {
							ITEM_READ_NAMES
						}
						child = child->next;
					}
					if(cu) {
						item = cu;
						cu->setCategory(category);
						cu->setSystem(usystem);
						if(use_with_prefixes_set) {
							cu->setUseWithPrefixesByDefault(use_with_prefixes);
							cu->setMaxPreferredPrefix(max_prefix);
							cu->setMinPreferredPrefix(min_prefix);
							cu->setDefaultPrefix(default_prefix);
						}
						if(new_names) {
							ITEM_SET_BEST_NAMES(unitNameIsValid)
							ITEM_SET_REFERENCE_NAMES(unitNameIsValid)
						} else {
							ITEM_SET_NAME_1(unitNameIsValid)
							ITEM_SET_NAME_2
							ITEM_SET_NAME_3
						}
						ITEM_SET_DTH
						if(check_duplicates && !is_user_defs) {
							for(size_t i = 1; i <= cu->countNames();) {
								if(getActiveVariable(cu->getName(i).name) || getActiveUnit(cu->getName(i).name) || getCompositeUnit(cu->getName(i).name)) cu->removeName(i);
								else i++;
							}
						}
						if(cu->countNames() == 0) {
							cu->destroy();
							cu = NULL;
						} else {
							addUnit(cu, true, is_user_defs);
							cu->setChanged(false);
						}
						done_something = true;
					} else {
						ITEM_CLEAR_NAMES
					}
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "builtin_unit")) {
				XML_GET_STRING_FROM_PROP(cur, "name", name)
				u = getUnit(name);
				if(!u) {
					u = getCompositeUnit(name);
				}
				if(u) {
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					u->setLocal(is_user_defs, active);
					u->setCategory(category);
					item = u;
					child = cur->xmlChildrenNode;
					singular = ""; best_singular = false; next_best_singular = false;
					plural = ""; best_plural = false; next_best_plural = false;
					countries = "", best_countries = false, next_best_countries = false;
					use_with_prefixes_set = false;
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*) "singular")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, singular, best_singular, next_best_singular)
							if(!unitNameIsValid(singular, version_numbers, is_user_defs)) {
								singular = "";
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "plural") && !xmlHasProp(child, (xmlChar*) "index")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, plural, best_plural, next_best_plural)
							if(!unitNameIsValid(plural, version_numbers, is_user_defs)) {
								plural = "";
							}
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "system")) {
							XML_DO_FROM_TEXT(child, u->setSystem)
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "use_with_prefixes")) {
							XML_GET_TRUE_FROM_TEXT(child, use_with_prefixes)
							use_with_prefixes_set = true;
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "countries")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, countries, best_countries, next_best_countries)
						} else ITEM_READ_NAME(unitNameIsValid)
						 else ITEM_READ_DTH
						 else {
							ITEM_READ_NAMES
						}
						child = child->next;
					}
					if(use_with_prefixes_set) {
						u->setUseWithPrefixesByDefault(use_with_prefixes);
					}
					u->setCountries(countries);
					if(new_names) {
						ITEM_SAVE_BUILTIN_NAMES
						ITEM_SET_BEST_NAMES(unitNameIsValid)
						ITEM_SET_REFERENCE_NAMES(unitNameIsValid)
						ITEM_SET_BUILTIN_NAMES
					} else {
						BUILTIN_UNIT_NAMES_1
						ITEM_SET_SINGULAR
						ITEM_SET_PLURAL
						ITEM_SET_NAME_2
						ITEM_SET_NAME_3
						BUILTIN_NAMES_2
					}
					ITEM_SET_DTH
					if((u == u_usd || u == u_gbp) && !is_user_defs && !b_ignore_locale) {
						struct lconv *lc = localeconv();
						if(lc) {
							string local_currency = lc->int_curr_symbol;
							remove_blank_ends(local_currency);
							if(local_currency.length() > 3) local_currency = local_currency.substr(0, 3);
							if(!u->hasName(local_currency)) {
								local_currency = lc->currency_symbol;
								remove_blank_ends(local_currency);
								if(u == u_usd && local_currency == "$") {
									b_remove_cent = true;
									size_t index = u->hasName("$");
									if(index > 0) u->removeName(index);
									index = u->hasName("dollar");
									if(index > 0) u->removeName(index);
									index = u->hasName("dollars");
									if(index > 0) u->removeName(index);
								} else if(u == u_gbp && local_currency == "£") {
									size_t index = u->hasName("£");
									if(index > 0) u->removeName(index);
								}
							}
						}
					}
					if(u != u_btc && u_usd && u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit() == u_usd) u->setHidden(true);
					u->setChanged(false);
					done_something = true;
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "prefix")) {
				child = cur->xmlChildrenNode;
				XML_GET_STRING_FROM_PROP(cur, "type", type)
				uname = ""; sexp = ""; svalue = ""; name = "";
				bool b_best = false;
				ITEM_INIT_NAME
				while(child != NULL) {
					if(!new_prefix_names && !xmlStrcmp(child->name, (const xmlChar*) "name")) {
						lang = xmlNodeGetLang(child);
						if(!lang) {
							if(name.empty()) {
								XML_GET_STRING_FROM_TEXT(child, name);
							}
						} else {
							if(!b_best && !locale.empty()) {
								if(locale == (char*) lang) {
									XML_GET_STRING_FROM_TEXT(child, name);
									b_best = true;
								} else if(strlen((char*) lang) >= 2 && lang[0] == localebase[0] && lang[1] == localebase[1]) {
									XML_GET_STRING_FROM_TEXT(child, name);
								}
							}
							xmlFree(lang);
						}
					} else if(!new_prefix_names && !xmlStrcmp(child->name, (const xmlChar*) "abbreviation")) {
						XML_GET_STRING_FROM_TEXT(child, stmp);
					} else if(!new_prefix_names && !xmlStrcmp(child->name, (const xmlChar*) "unicode")) {
						XML_GET_STRING_FROM_TEXT(child, uname);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "exponent")) {
						XML_GET_STRING_FROM_TEXT(child, sexp);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "value")) {
						XML_GET_STRING_FROM_TEXT(child, svalue);
					} else if(new_prefix_names) {
						ITEM_READ_NAMES
					}
					child = child->next;
				}
				p = NULL;
				if(type == "decimal") {
					p = new DecimalPrefix(s2i(sexp), name, stmp, uname);
				} else if(type == "number") {
					p = new NumberPrefix(svalue, name, stmp, uname);
				} else if(type == "binary") {
					p = new BinaryPrefix(s2i(sexp), name, stmp, uname);
				} else {
					if(svalue.empty()) {
						p = new DecimalPrefix(s2i(sexp), name, stmp, uname);
					} else {
						p = new NumberPrefix(svalue, name, stmp, uname);
					}
				}
				if(p) {
					if(new_prefix_names) {
						PREFIX_SET_BEST_NAMES
						PREFIX_SET_REFERENCE_NAMES
					}
					addPrefix(p);
				} else {
					ITEM_CLEAR_NAMES
				}
				done_something = true;
			}
			after_load_object:
			cur = NULL;
			if(in_unfinished) {
				if(done_something) {
					in_unfinished--;
					unfinished_nodes.erase(unfinished_nodes.begin() + in_unfinished);
					unfinished_cats.erase(unfinished_cats.begin() + in_unfinished);
				}
				if((int) unfinished_nodes.size() > in_unfinished) {
					cur = unfinished_nodes[in_unfinished];
					category = unfinished_cats[in_unfinished];
				} else if(done_something && unfinished_nodes.size() > 0) {
					cur = unfinished_nodes[0];
					category = unfinished_cats[0];
					in_unfinished = 0;
					done_something = false;
				}
				in_unfinished++;
				done_something = false;
			}
		}
		if(in_unfinished) {
			break;
		}
		while(!nodes.empty() && nodes.back().empty()) {
			size_t cat_i = category.rfind("/");
			if(cat_i == string::npos) {
				category = "";
			} else {
				category = category.substr(0, cat_i);
			}
			nodes.pop_back();
		}
		if(!nodes.empty()) {
			cur = nodes.back().front();
			nodes.back().pop();
			nodes.resize(nodes.size() + 1);
		} else {
			if(unfinished_nodes.size() > 0) {
				cur = unfinished_nodes[0];
				category = unfinished_cats[0];
				in_unfinished = 1;
				done_something = false;
			} else {
				cur = NULL;
			}
		}
		if(cur == NULL) {
			break;
		}
	}
	xmlFreeDoc(doc);
	return true;
}
bool Calculator::saveDefinitions() {

	recursiveMakeDir(getLocalDataDir());
	string homedir = buildPath(getLocalDataDir(), "definitions");
	makeDir(homedir);
	bool b = true;
	if(!saveFunctions(buildPath(homedir, "functions.xml").c_str())) b = false;
	if(!saveUnits(buildPath(homedir, "units.xml").c_str())) b = false;
	if(!saveVariables(buildPath(homedir, "variables.xml").c_str())) b = false;
	if(!saveDataSets(buildPath(homedir, "datasets.xml").c_str())) b = false;
	if(!saveDataObjects()) b = false;
	return b;
}

struct node_tree_item {
	xmlNodePtr node;
	string category;
	vector<node_tree_item> items;
};

int Calculator::saveDataObjects() {
	int returnvalue = 1;
	for(size_t i = 0; i < data_sets.size(); i++) {
		int rv = data_sets[i]->saveObjects(NULL, false);
		if(rv <= 0) returnvalue = rv;
	}
	return returnvalue;
}

#define SAVE_NAMES(o)\
				str = "";\
				for(size_t i2 = 1;;)  {\
					ename = &o->getName(i2);\
					if(ename->abbreviation) {str += 'a';}\
					bool b_cs = (ename->abbreviation || text_length_is_one(ename->name));\
					if(ename->case_sensitive && !b_cs) {str += 'c';}\
					if(!ename->case_sensitive && b_cs) {str += "-c";}\
					if(ename->avoid_input) {str += 'i';}\
					if(ename->completion_only) {str += 'o';}\
					if(ename->plural) {str += 'p';}\
					if(ename->reference) {str += 'r';}\
					if(ename->suffix) {str += 's';}\
					if(ename->unicode) {str += 'u';}\
					if(str.empty() || str[str.length() - 1] == ',') {\
						if(i2 == 1 && o->countNames() == 1) {\
							if(save_global) {\
								xmlNewTextChild(newnode, NULL, (xmlChar*) "_names", (xmlChar*) ename->name.c_str());\
							} else {\
								xmlNewTextChild(newnode, NULL, (xmlChar*) "names", (xmlChar*) ename->name.c_str());\
							}\
							break;\
						}\
					} else {\
						str += ':';\
					}\
					str += ename->name;\
					i2++;\
					if(i2 > o->countNames()) {\
						if(save_global) {\
							xmlNewTextChild(newnode, NULL, (xmlChar*) "_names", (xmlChar*) str.c_str());\
						} else {\
							xmlNewTextChild(newnode, NULL, (xmlChar*) "names", (xmlChar*) str.c_str());\
						}\
						break;\
					}\
					str += ',';\
				}

int Calculator::savePrefixes(const char* file_name, bool save_global) {
	if(!save_global) {
		return true;
	}
	string str;
	const ExpressionName *ename;
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	xmlNodePtr cur, newnode;
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	cur = doc->children;
	for(size_t i = 0; i < prefixes.size(); i++) {
		newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "prefix", NULL);
		SAVE_NAMES(prefixes[i])
		switch(prefixes[i]->type()) {
			case PREFIX_DECIMAL: {
				xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "decimal");
				xmlNewTextChild(newnode, NULL, (xmlChar*) "exponent", (xmlChar*) i2s(((DecimalPrefix*) prefixes[i])->exponent()).c_str());
				break;
			}
			case PREFIX_BINARY: {
				xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "binary");
				xmlNewTextChild(newnode, NULL, (xmlChar*) "exponent", (xmlChar*) i2s(((BinaryPrefix*) prefixes[i])->exponent()).c_str());
				break;
			}
			case PREFIX_NUMBER: {
				xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "number");
				xmlNewTextChild(newnode, NULL, (xmlChar*) "value", (xmlChar*) prefixes[i]->value().print(save_printoptions).c_str());
				break;
			}
		}
	}
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}

string Calculator::temporaryCategory() const {
	return _("Temporary");
}

string Calculator::saveTemporaryDefinitions() {
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	saveVariables(doc, false, true);
	saveFunctions(doc, false, true);
	saveUnits(doc, false, true);
	int len = 0;
	xmlChar *s = NULL;
	xmlDocDumpMemory(doc, &s, &len);
	string str = (char*) s;
	xmlFree(s);
	xmlFreeDoc(doc);
	return str;
}
int Calculator::saveVariables(const char* file_name, bool save_global) {
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	saveVariables(doc, save_global, false);
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}
void Calculator::saveVariables(void *xmldoc, bool save_global, bool save_only_temp) {
	xmlDocPtr doc = (xmlDocPtr) xmldoc;
	xmlNodePtr cur, newnode, newnode2;
	string str;
	const ExpressionName *ename;
	node_tree_item top;
	top.category = "";
	top.node = doc->children;
	node_tree_item *item;
	string cat, cat_sub;
	bool matlab_matrices_bak = priv->matlab_matrices;
	priv->matlab_matrices = false;
	for(size_t i = 0; i < variables.size(); i++) {
		if((save_global || variables[i]->isLocal() || (!save_only_temp && variables[i]->hasChanged())) && (variables[i]->category() != _("Temporary") && variables[i]->category() != "Temporary") == !save_only_temp && (!save_only_temp || !variables[i]->isBuiltin())) {
			item = &top;
			if(!variables[i]->category().empty()) {
				if(save_only_temp) cat = "Temporary";
				else cat = variables[i]->category();
				size_t cat_i = cat.find("/"); size_t cat_i_prev = 0;
				bool b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(size_t i2 = 0; i2 < item->items.size(); i2++) {
						if(cat_sub == item->items[i2].category) {
							item = &item->items[i2];
							b = true;
							break;
						}
					}
					if(!b) {
						item->items.resize(item->items.size() + 1);
						item->items[item->items.size() - 1].node = xmlNewTextChild(item->node, NULL, (xmlChar*) "category", NULL);
						item = &item->items[item->items.size() - 1];
						item->category = cat_sub;
						if(save_global) {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "_title", (xmlChar*) item->category.c_str());
						} else {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "title", (xmlChar*) item->category.c_str());
						}
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			cur = item->node;
			if(!save_global && !variables[i]->isLocal() && variables[i]->hasChanged()) {
				if(variables[i]->isActive()) {
					xmlNewTextChild(cur, NULL, (xmlChar*) "activate", (xmlChar*) variables[i]->referenceName().c_str());
				} else {
					xmlNewTextChild(cur, NULL, (xmlChar*) "deactivate", (xmlChar*) variables[i]->referenceName().c_str());
				}
			} else if(save_global || variables[i]->isLocal()) {
				if(variables[i]->isBuiltin()) {
					if(variables[i]->isKnown()) {
						newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "builtin_variable", NULL);
					} else {
						newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "builtin_unknown", NULL);
					}
					xmlNewProp(newnode, (xmlChar*) "name", (xmlChar*) variables[i]->referenceName().c_str());
				} else {
					if(variables[i]->isKnown()) {
						newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "variable", NULL);
					} else {
						newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "unknown", NULL);
					}
				}
				if(!variables[i]->isActive()) xmlNewProp(newnode, (xmlChar*) "active", (xmlChar*) "false");
				if(variables[i]->isHidden()) xmlNewTextChild(newnode, NULL, (xmlChar*) "hidden", (xmlChar*) "true");
				if(!variables[i]->title(false).empty()) {
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_title", (xmlChar*) variables[i]->title(false).c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "title", (xmlChar*) variables[i]->title(false).c_str());
					}
				}
				SAVE_NAMES(variables[i])
				if(!variables[i]->description().empty()) {
					str = variables[i]->description();
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_description", (xmlChar*) str.c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "description", (xmlChar*) str.c_str());
					}
				}
				if(!variables[i]->isBuiltin()) {
					if(variables[i]->isKnown()) {
						bool is_approx = false;
						save_printoptions.is_approximate = &is_approx;
						if(((KnownVariable*) variables[i])->isExpression()) {
							newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "value", (xmlChar*) ((KnownVariable*) variables[i])->expression().c_str());
							bool unc_rel = false;
							if(!((KnownVariable*) variables[i])->uncertainty(&unc_rel).empty()) xmlNewProp(newnode2, (xmlChar*) (unc_rel ? "relative_uncertainty" : "uncertainty"), (xmlChar*) ((KnownVariable*) variables[i])->uncertainty().c_str());
							if(!((KnownVariable*) variables[i])->unit().empty()) xmlNewProp(newnode2, (xmlChar*) "unit", (xmlChar*) ((KnownVariable*) variables[i])->unit().c_str());
						} else {
							newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "value", (xmlChar*) ((KnownVariable*) variables[i])->get().print(save_printoptions).c_str());
						}
						save_printoptions.is_approximate = NULL;
						if(variables[i]->isApproximate() || is_approx) xmlNewProp(newnode2, (xmlChar*) "approximate", (xmlChar*) "true");
						if(variables[i]->precision() >= 0) xmlNewProp(newnode2, (xmlChar*) "precision", (xmlChar*) i2s(variables[i]->precision()).c_str());
					} else {
						if(((UnknownVariable*) variables[i])->assumptions()) {
							switch(((UnknownVariable*) variables[i])->assumptions()->type()) {
								case ASSUMPTION_TYPE_INTEGER: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "integer");
									break;
								}
								case ASSUMPTION_TYPE_BOOLEAN: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "boolean");
									break;
								}
								case ASSUMPTION_TYPE_RATIONAL: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "rational");
									break;
								}
								case ASSUMPTION_TYPE_REAL: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "real");
									break;
								}
								case ASSUMPTION_TYPE_COMPLEX: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "complex");
									break;
								}
								case ASSUMPTION_TYPE_NUMBER: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "number");
									break;
								}
								case ASSUMPTION_TYPE_NONMATRIX: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "non-matrix");
									break;
								}
								case ASSUMPTION_TYPE_NONE: {
									newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "type", (xmlChar*) "none");
									break;
								}
							}
							if(((UnknownVariable*) variables[i])->assumptions()->type() != ASSUMPTION_TYPE_BOOLEAN) {
								switch(((UnknownVariable*) variables[i])->assumptions()->sign()) {
									case ASSUMPTION_SIGN_NONZERO: {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "sign", (xmlChar*) "non-zero");
										break;
									}
									case ASSUMPTION_SIGN_NONPOSITIVE: {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "sign", (xmlChar*) "non-positive");
										break;
									}
									case ASSUMPTION_SIGN_NEGATIVE: {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "sign", (xmlChar*) "negative");
										break;
									}
									case ASSUMPTION_SIGN_NONNEGATIVE: {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "sign", (xmlChar*) "non-negative");
										break;
									}
									case ASSUMPTION_SIGN_POSITIVE: {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "sign", (xmlChar*) "positive");
										break;
									}
									case ASSUMPTION_SIGN_UNKNOWN: {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "sign", (xmlChar*) "unknown");
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	priv->matlab_matrices = matlab_matrices_bak;
}

int Calculator::saveUnits(const char* file_name, bool save_global) {
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	saveUnits(doc, save_global, false);
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}
void Calculator::saveUnits(void *xmldoc, bool save_global, bool save_only_temp) {
	xmlDocPtr doc = (xmlDocPtr) xmldoc;
	string str;
	xmlNodePtr cur, newnode, newnode2, newnode3;
	const ExpressionName *ename;
	CompositeUnit *cu = NULL;
	AliasUnit *au = NULL;
	Unit *u;
	node_tree_item top;
	top.category = "";
	top.node = doc->children;
	node_tree_item *item;
	string cat, cat_sub;
	for(size_t i = 0; i < units.size(); i++) {
		u = units[i];
		if((save_global || u->isLocal() || (!save_only_temp && u->hasChanged())) && (u->category() != _("Temporary") && u->category() != "Temporary") == !save_only_temp && (!save_only_temp || !units[i]->isBuiltin())) {
			item = &top;
			if(!u->category().empty()) {
				if(save_only_temp) cat = "Temporary";
				else cat = u->category();
				size_t cat_i = cat.find("/"); size_t cat_i_prev = 0;
				bool b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(size_t i2 = 0; i2 < item->items.size(); i2++) {
						if(cat_sub == item->items[i2].category) {
							item = &item->items[i2];
							b = true;
							break;
						}
					}
					if(!b) {
						item->items.resize(item->items.size() + 1);
						item->items[item->items.size() - 1].node = xmlNewTextChild(item->node, NULL, (xmlChar*) "category", NULL);
						item = &item->items[item->items.size() - 1];
						item->category = cat_sub;
						if(save_global) {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "_title", (xmlChar*) item->category.c_str());
						} else {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "title", (xmlChar*) item->category.c_str());
						}
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			cur = item->node;
			if(!save_global && !u->isLocal() && u->hasChanged()) {
				if(u->isActive()) {
					xmlNewTextChild(cur, NULL, (xmlChar*) "activate", (xmlChar*) u->referenceName().c_str());
				} else {
					xmlNewTextChild(cur, NULL, (xmlChar*) "deactivate", (xmlChar*) u->referenceName().c_str());
				}
			} else if(save_global || u->isLocal()) {
				if(u->isBuiltin()) {
					newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "builtin_unit", NULL);
					xmlNewProp(newnode, (xmlChar*) "name", (xmlChar*) u->referenceName().c_str());
				} else {
					newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "unit", NULL);
					switch(u->subtype()) {
						case SUBTYPE_BASE_UNIT: {
							xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "base");
							break;
						}
						case SUBTYPE_ALIAS_UNIT: {
							au = (AliasUnit*) u;
							xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "alias");
							break;
						}
						case SUBTYPE_COMPOSITE_UNIT: {
							cu = (CompositeUnit*) u;
							xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "composite");
							break;
						}
					}
				}
				if(!u->isActive()) xmlNewProp(newnode, (xmlChar*) "active", (xmlChar*) "false");
				if(u->isHidden()) xmlNewTextChild(newnode, NULL, (xmlChar*) "hidden", (xmlChar*) "true");
				if(!u->system().empty()) {
					xmlNewTextChild(newnode, NULL, (xmlChar*) "system", (xmlChar*) u->system().c_str());
				}
				if((u->isSIUnit() && (!u->useWithPrefixesByDefault() || u->maxPreferredPrefix() != INT_MAX || u->minPreferredPrefix() != INT_MIN)) || (u->useWithPrefixesByDefault() && !u->isSIUnit()) || u->defaultPrefix() != 0) {
					newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "use_with_prefixes", u->useWithPrefixesByDefault() ? (xmlChar*) "true" : (xmlChar*) "false");
					if(u->minPreferredPrefix() != INT_MIN) xmlNewProp(newnode2, (xmlChar*) "min", (xmlChar*) i2s(u->minPreferredPrefix()).c_str());
					if(u->maxPreferredPrefix() != INT_MAX) xmlNewProp(newnode2, (xmlChar*) "max", (xmlChar*) i2s(u->maxPreferredPrefix()).c_str());
					if(u->defaultPrefix() != 0) xmlNewProp(newnode2, (xmlChar*) "default", (xmlChar*) i2s(u->defaultPrefix()).c_str());
				}
				if(!u->title(false).empty()) {
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_title", (xmlChar*) u->title(false).c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "title", (xmlChar*) u->title(false).c_str());
					}
				}
				if(save_global && u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					save_global = false;
					SAVE_NAMES(u)
					save_global = true;
				} else {
					SAVE_NAMES(u)
				}
				if(!u->description().empty()) {
					str = u->description();
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_description", (xmlChar*) str.c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "description", (xmlChar*) str.c_str());
					}
				}
				if(!u->isBuiltin()) {
					if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
							newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "part", NULL);
							int exp = 1;
							Prefix *p = NULL;
							Unit *u = cu->get(i2, &exp, &p);
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "unit", (xmlChar*) u->referenceName().c_str());
							if(p) {
								switch(p->type()) {
									case PREFIX_DECIMAL: {
										xmlNewTextChild(newnode2, NULL, (xmlChar*) "prefix", (xmlChar*) i2s(((DecimalPrefix*) p)->exponent()).c_str());
										break;
									}
									case PREFIX_BINARY: {
										newnode3 = xmlNewTextChild(newnode2, NULL, (xmlChar*) "prefix", (xmlChar*) i2s(((BinaryPrefix*) p)->exponent()).c_str());
										xmlNewProp(newnode3, (xmlChar*) "type", (xmlChar*) "binary");
										break;
									}
									case PREFIX_NUMBER: {
										newnode3 = xmlNewTextChild(newnode2, NULL, (xmlChar*) "prefix", (xmlChar*) p->value().print(save_printoptions).c_str());
										xmlNewProp(newnode3, (xmlChar*) "type", (xmlChar*) "number");
										break;
									}
								}
							}
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "exponent", (xmlChar*) i2s(exp).c_str());
						}
					}
					if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
						newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "base", NULL);
						xmlNewTextChild(newnode2, NULL, (xmlChar*) "unit", (xmlChar*) au->firstBaseUnit()->referenceName().c_str());
						newnode3 = xmlNewTextChild(newnode2, NULL, (xmlChar*) "relation", (xmlChar*) au->expression().c_str());
						if(au->isApproximate()) xmlNewProp(newnode3, (xmlChar*) "approximate", (xmlChar*) "true");
						if(au->precision() >= 0) xmlNewProp(newnode3, (xmlChar*) "precision", (xmlChar*) i2s(u->precision()).c_str());
						bool unc_rel = false;
						if(!au->uncertainty(&unc_rel).empty()) xmlNewProp(newnode3, (xmlChar*) (unc_rel ? "relative_uncertainty" : "uncertainty"), (xmlChar*) au->uncertainty().c_str());
						if(!au->inverseExpression().empty()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "inverse_relation", (xmlChar*) au->inverseExpression().c_str());
						}
						xmlNewTextChild(newnode2, NULL, (xmlChar*) "exponent", (xmlChar*) i2s(au->firstBaseExponent()).c_str());
						if(au->mixWithBase() > 0) {
							newnode3 = xmlNewTextChild(newnode2, NULL, (xmlChar*) "mix", (xmlChar*) i2s(au->mixWithBase()).c_str());
							if(au->mixWithBaseMinimum() > 1) xmlNewProp(newnode3, (xmlChar*) "min", (xmlChar*) i2s(au->mixWithBaseMinimum()).c_str());
						}
					}
				}
			}
		}
	}
}

int Calculator::saveFunctions(const char* file_name, bool save_global) {
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	saveFunctions(doc, save_global, false);
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}
void Calculator::saveFunctions(void *xmldoc, bool save_global, bool save_only_temp) {
	xmlDocPtr doc = (xmlDocPtr) xmldoc;
	xmlNodePtr cur, newnode, newnode2;
	const ExpressionName *ename;
	node_tree_item top;
	top.category = "";
	top.node = doc->children;
	node_tree_item *item;
	string cat, cat_sub;
	Argument *arg;
	IntegerArgument *iarg;
	NumberArgument *farg;
	string str;
	for(size_t i = 0; i < functions.size(); i++) {
		if(functions[i]->subtype() != SUBTYPE_DATA_SET && (save_global || functions[i]->isLocal() || (!save_only_temp && functions[i]->hasChanged())) && (functions[i]->category() != _("Temporary") && functions[i]->category() != "Temporary") == !save_only_temp && (!save_only_temp || !functions[i]->isBuiltin())) {
			item = &top;
			if(!functions[i]->category().empty()) {
				if(save_only_temp) cat = "Temporary";
				else cat = functions[i]->category();
				size_t cat_i = cat.find("/"); size_t cat_i_prev = 0;
				bool b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(size_t i2 = 0; i2 < item->items.size(); i2++) {
						if(cat_sub == item->items[i2].category) {
							item = &item->items[i2];
							b = true;
							break;
						}
					}
					if(!b) {
						item->items.resize(item->items.size() + 1);
						item->items[item->items.size() - 1].node = xmlNewTextChild(item->node, NULL, (xmlChar*) "category", NULL);
						item = &item->items[item->items.size() - 1];
						item->category = cat_sub;
						if(save_global) {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "_title", (xmlChar*) item->category.c_str());
						} else {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "title", (xmlChar*) item->category.c_str());
						}
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			cur = item->node;
			if(!save_global && !functions[i]->isLocal() && functions[i]->hasChanged()) {
				if(functions[i]->isActive()) {
					xmlNewTextChild(cur, NULL, (xmlChar*) "activate", (xmlChar*) functions[i]->referenceName().c_str());
				} else {
					xmlNewTextChild(cur, NULL, (xmlChar*) "deactivate", (xmlChar*) functions[i]->referenceName().c_str());
				}
			} else if(save_global || functions[i]->isLocal()) {
				if(functions[i]->isBuiltin()) {
					newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "builtin_function", NULL);
					xmlNewProp(newnode, (xmlChar*) "name", (xmlChar*) functions[i]->referenceName().c_str());
				} else {
					newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "function", NULL);
				}
				if(!functions[i]->isActive()) xmlNewProp(newnode, (xmlChar*) "active", (xmlChar*) "false");
				if(functions[i]->isHidden()) xmlNewTextChild(newnode, NULL, (xmlChar*) "hidden", (xmlChar*) "true");
				if(!functions[i]->title(false).empty()) {
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_title", (xmlChar*) functions[i]->title(false).c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "title", (xmlChar*) functions[i]->title(false).c_str());
					}
				}
				SAVE_NAMES(functions[i])
				if(!functions[i]->description().empty()) {
					str = functions[i]->description();
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_description", (xmlChar*) str.c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "description", (xmlChar*) str.c_str());
					}
				}
				if(!functions[i]->example(true).empty()) newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "example", (xmlChar*) functions[i]->example(true).c_str());
				if(functions[i]->isBuiltin()) {
					cur = newnode;
					for(size_t i2 = 1; i2 <= functions[i]->lastArgumentDefinitionIndex(); i2++) {
						arg = functions[i]->getArgumentDefinition(i2);
						if(arg && !arg->name().empty()) {
							newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "argument", NULL);
							if(save_global) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "_title", (xmlChar*) arg->name().c_str());
							} else {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "title", (xmlChar*) arg->name().c_str());
							}
							xmlNewProp(newnode, (xmlChar*) "index", (xmlChar*) i2s(i2).c_str());
						}
					}
				} else {
					for(size_t i2 = 1; i2 <= ((UserFunction*) functions[i])->countSubfunctions(); i2++) {
						newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "subfunction", (xmlChar*) ((UserFunction*) functions[i])->getSubfunction(i2).c_str());
						if(((UserFunction*) functions[i])->subfunctionPrecalculated(i2)) xmlNewProp(newnode2, (xmlChar*) "precalculate", (xmlChar*) "true");
						else xmlNewProp(newnode2, (xmlChar*) "precalculate", (xmlChar*) "false");

					}
					newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "expression", (xmlChar*) ((UserFunction*) functions[i])->formula().c_str());
					if(functions[i]->isApproximate()) xmlNewProp(newnode2, (xmlChar*) "approximate", (xmlChar*) "true");
					if(functions[i]->precision() >= 0) xmlNewProp(newnode2, (xmlChar*) "precision", (xmlChar*) i2s(functions[i]->precision()).c_str());
					if(!functions[i]->condition().empty()) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "condition", (xmlChar*) functions[i]->condition().c_str());
					}
					cur = newnode;
					for(size_t i2 = 1; i2 <= functions[i]->lastArgumentDefinitionIndex(); i2++) {
						arg = functions[i]->getArgumentDefinition(i2);
						if(arg) {
							newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "argument", NULL);
							if(!arg->name().empty()) {
								if(save_global) {
									xmlNewTextChild(newnode, NULL, (xmlChar*) "_title", (xmlChar*) arg->name().c_str());
								} else {
									xmlNewTextChild(newnode, NULL, (xmlChar*) "title", (xmlChar*) arg->name().c_str());
								}
							}
							switch(arg->type()) {
								case ARGUMENT_TYPE_TEXT: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "text"); break;}
								case ARGUMENT_TYPE_SYMBOLIC: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "symbol"); break;}
								case ARGUMENT_TYPE_DATE: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "date"); break;}
								case ARGUMENT_TYPE_INTEGER: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "integer"); break;}
								case ARGUMENT_TYPE_NUMBER: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "number"); break;}
								case ARGUMENT_TYPE_VECTOR: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "vector"); break;}
								case ARGUMENT_TYPE_MATRIX: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "matrix"); break;}
								case ARGUMENT_TYPE_BOOLEAN: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "boolean"); break;}
								case ARGUMENT_TYPE_FUNCTION: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "function"); break;}
								case ARGUMENT_TYPE_UNIT: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "unit"); break;}
								case ARGUMENT_TYPE_VARIABLE: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "variable"); break;}
								case ARGUMENT_TYPE_EXPRESSION_ITEM: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "object"); break;}
								case ARGUMENT_TYPE_ANGLE: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "angle"); break;}
								case ARGUMENT_TYPE_DATA_OBJECT: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "data-object"); break;}
								case ARGUMENT_TYPE_DATA_PROPERTY: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "data-property"); break;}
								default: {xmlNewProp(newnode, (xmlChar*) "type", (xmlChar*) "free");}
							}
							xmlNewProp(newnode, (xmlChar*) "index", (xmlChar*) i2s(i2).c_str());
							bool default_hv = arg->tests() && (arg->type() == ARGUMENT_TYPE_NUMBER || arg->type() == ARGUMENT_TYPE_INTEGER);
							if(!default_hv && arg->handlesVector()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "handle_vector", (xmlChar*) "true");
							} else if(default_hv && !arg->handlesVector()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "handle_vector", (xmlChar*) "false");
							}
							if(!arg->tests()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "test", (xmlChar*) "false");
							}
							if(!arg->alerts()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "alert", (xmlChar*) "false");
							}
							if(arg->zeroForbidden()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "zero_forbidden", (xmlChar*) "true");
							}
							if(arg->matrixAllowed()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "matrix_allowed", (xmlChar*) "true");
							}
							switch(arg->type()) {
								case ARGUMENT_TYPE_INTEGER: {
									iarg = (IntegerArgument*) arg;
									if(iarg->min()) {
										xmlNewTextChild(newnode, NULL, (xmlChar*) "min", (xmlChar*) iarg->min()->print(save_printoptions).c_str());
									}
									if(iarg->max()) {
										xmlNewTextChild(newnode, NULL, (xmlChar*) "max", (xmlChar*) iarg->max()->print(save_printoptions).c_str());
									}
									break;
								}
								case ARGUMENT_TYPE_NUMBER: {
									farg = (NumberArgument*) arg;
									if(farg->min()) {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "min", (xmlChar*) farg->min()->print(save_printoptions).c_str());
										if(farg->includeEqualsMin()) {
											xmlNewProp(newnode2, (xmlChar*) "include_equals", (xmlChar*) "true");
										} else {
											xmlNewProp(newnode2, (xmlChar*) "include_equals", (xmlChar*) "false");
										}
									}
									if(farg->max()) {
										newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "max", (xmlChar*) farg->max()->print(save_printoptions).c_str());
										if(farg->includeEqualsMax()) {
											xmlNewProp(newnode2, (xmlChar*) "include_equals", (xmlChar*) "true");
										} else {
											xmlNewProp(newnode2, (xmlChar*) "include_equals", (xmlChar*) "false");
										}
									}
									if(!farg->complexAllowed()) {
										xmlNewTextChild(newnode, NULL, (xmlChar*) "complex_allowed", (xmlChar*) "false");
									}
									break;
								}
							}
							if(!arg->getCustomCondition().empty()) {
								xmlNewTextChild(newnode, NULL, (xmlChar*) "condition", (xmlChar*) arg->getCustomCondition().c_str());
							}
						}
					}
				}
			}
		}
	}
}
int Calculator::saveDataSets(const char* file_name, bool save_global) {
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
	xmlNodePtr cur, newnode, newnode2;
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	const ExpressionName *ename;
	node_tree_item top;
	top.category = "";
	top.node = doc->children;
	node_tree_item *item;
	string cat, cat_sub;
	Argument *arg;
	DataSet *ds;
	DataProperty *dp;
	string str;
	for(size_t i = 0; i < functions.size(); i++) {
		if(functions[i]->subtype() == SUBTYPE_DATA_SET && (save_global || functions[i]->isLocal() || functions[i]->hasChanged())) {
			item = &top;
			ds = (DataSet*) functions[i];
			if(!ds->category().empty()) {
				cat = ds->category();
				size_t cat_i = cat.find("/"); size_t cat_i_prev = 0;
				bool b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(size_t i2 = 0; i2 < item->items.size(); i2++) {
						if(cat_sub == item->items[i2].category) {
							item = &item->items[i2];
							b = true;
							break;
						}
					}
					if(!b) {
						item->items.resize(item->items.size() + 1);
						item->items[item->items.size() - 1].node = xmlNewTextChild(item->node, NULL, (xmlChar*) "category", NULL);
						item = &item->items[item->items.size() - 1];
						item->category = cat_sub;
						if(save_global) {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "_title", (xmlChar*) item->category.c_str());
						} else {
							xmlNewTextChild(item->node, NULL, (xmlChar*) "title", (xmlChar*) item->category.c_str());
						}
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			cur = item->node;
			if(save_global || ds->isLocal() || ds->hasChanged()) {
				if(save_global || ds->isLocal()) {
					newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "dataset", NULL);
				} else {
					newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "builtin_dataset", NULL);
					xmlNewProp(newnode, (xmlChar*) "name", (xmlChar*) ds->referenceName().c_str());
				}
				if(!ds->isActive()) xmlNewProp(newnode, (xmlChar*) "active", (xmlChar*) "false");
				if(ds->isHidden()) xmlNewTextChild(newnode, NULL, (xmlChar*) "hidden", (xmlChar*) "true");
				if(!ds->title(false).empty()) {
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_title", (xmlChar*) ds->title(false).c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "title", (xmlChar*) ds->title(false).c_str());
					}
				}
				if((save_global || ds->isLocal()) && !ds->defaultDataFile().empty()) {
					xmlNewTextChild(newnode, NULL, (xmlChar*) "datafile", (xmlChar*) ds->defaultDataFile().c_str());
				}
				SAVE_NAMES(ds)
				if(!ds->description().empty()) {
					str = ds->description();
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_description", (xmlChar*) str.c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "description", (xmlChar*) str.c_str());
					}
				}
				if((save_global || ds->isLocal()) && !ds->copyright().empty()) {
					if(save_global) {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "_copyright", (xmlChar*) ds->copyright().c_str());
					} else {
						xmlNewTextChild(newnode, NULL, (xmlChar*) "copyright", (xmlChar*) ds->copyright().c_str());
					}
				}
				arg = ds->getArgumentDefinition(1);
				if(arg && ((!save_global && !ds->isLocal()) || arg->name() != _("Object"))) {
					newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "object_argument", NULL);
					if(save_global) {
						xmlNewTextChild(newnode2, NULL, (xmlChar*) "_title", (xmlChar*) arg->name().c_str());
					} else {
						xmlNewTextChild(newnode2, NULL, (xmlChar*) "title", (xmlChar*) arg->name().c_str());
					}
				}
				arg = ds->getArgumentDefinition(2);
				if(arg && ((!save_global && !ds->isLocal()) || arg->name() != _("Property"))) {
					newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "property_argument", NULL);
					if(save_global) {
						xmlNewTextChild(newnode2, NULL, (xmlChar*) "_title", (xmlChar*) arg->name().c_str());
					} else {
						xmlNewTextChild(newnode2, NULL, (xmlChar*) "title", (xmlChar*) arg->name().c_str());
					}
				}
				if((!save_global && !ds->isLocal()) || ds->getDefaultValue(2) != _("info")) {
					xmlNewTextChild(newnode, NULL, (xmlChar*) "default_property", (xmlChar*) ds->getDefaultValue(2).c_str());
				}
				DataPropertyIter it;
				dp = ds->getFirstProperty(&it);
				while(dp) {
					if(save_global || ds->isLocal() || dp->isUserModified()) {
						newnode2 = xmlNewTextChild(newnode, NULL, (xmlChar*) "property", NULL);
						if(!dp->title(false).empty()) {
							if(save_global) {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "_title", (xmlChar*) dp->title().c_str());
							} else {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "title", (xmlChar*) dp->title().c_str());
							}
						}
						switch(dp->propertyType()) {
							case PROPERTY_STRING: {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "type", (xmlChar*) "text");
								break;
							}
							case PROPERTY_NUMBER: {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "type", (xmlChar*) "number");
								break;
							}
							case PROPERTY_EXPRESSION: {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "type", (xmlChar*) "expression");
								break;
							}
						}
						if(dp->isHidden()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "hidden", (xmlChar*) "true");
						}
						if(dp->isKey()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "key", (xmlChar*) "true");
						}
						if(dp->isApproximate()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "approximate", (xmlChar*) "true");
						}
						if(dp->usesBrackets()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "brackets", (xmlChar*) "true");
						}
						if(dp->isCaseSensitive()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "case_sensitive", (xmlChar*) "true");
						}
						if(!dp->getUnitString().empty()) {
							xmlNewTextChild(newnode2, NULL, (xmlChar*) "unit", (xmlChar*) dp->getUnitString().c_str());
						}
						str = "";
						for(size_t i2 = 1;;)  {
							if(dp->nameIsReference(i2)) {str += 'r';}
							if(str.empty() || str[str.length() - 1] == ',') {
								if(i2 == 1 && dp->countNames() == 1) {
									if(save_global) {
										xmlNewTextChild(newnode2, NULL, (xmlChar*) "_names", (xmlChar*) dp->getName(i2).c_str());
									} else {
										xmlNewTextChild(newnode2, NULL, (xmlChar*) "names", (xmlChar*) dp->getName(i2).c_str());
									}
									break;
								}
							} else {
								str += ':';
							}
							str += dp->getName(i2);
							i2++;
							if(i2 > dp->countNames()) {
								if(save_global) {
									xmlNewTextChild(newnode2, NULL, (xmlChar*) "_names", (xmlChar*) str.c_str());
								} else {
									xmlNewTextChild(newnode2, NULL, (xmlChar*) "names", (xmlChar*) str.c_str());
								}
								break;
							}
							str += ',';
						}
						if(!dp->description().empty()) {
							str = dp->description();
							if(save_global) {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "_description", (xmlChar*) str.c_str());
							} else {
								xmlNewTextChild(newnode2, NULL, (xmlChar*) "description", (xmlChar*) str.c_str());
							}
						}
					}
					dp = ds->getNextProperty(&it);
				}
			}
		}
	}
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}

bool Calculator::importCSV(MathStructure &mstruct, const char *file_name, int first_row, string delimiter, vector<string> *headers) {
	FILE *file = fopen(file_name, "r");
	if(file == NULL) {
		return false;
	}
	if(first_row < 1) {
		first_row = 1;
	}
	char line[10000];
	string stmp, str1, str2;
	long int row = 0, rows = 1;
	int columns = 1;
	int column;
	mstruct = m_empty_matrix;
	size_t is, is_n;
	bool v_added = false;
	while(fgets(line, 10000, file)) {
		row++;
		if(row >= first_row) {
			stmp = line;
			remove_blank_ends(stmp);
			if(row == first_row) {
				if(stmp.empty()) {
					row--;
				} else {
					is = 0;
					while((is_n = stmp.find(delimiter, is)) != string::npos) {
						columns++;
						if(headers) {
							str1 = stmp.substr(is, is_n - is);
							remove_blank_ends(str1);
							headers->push_back(str1);
						}
						is = is_n + delimiter.length();
					}
					if(headers) {
						str1 = stmp.substr(is, stmp.length() - is);
						remove_blank_ends(str1);
						headers->push_back(str1);
					}
					mstruct.resizeMatrix(1, columns, m_undefined);
				}
			}
			if((!headers || row > first_row) && !stmp.empty()) {
				is = 0;
				column = 1;
				if(v_added) {
					mstruct.addRow(m_undefined);
					rows++;
				}
				while(column <= columns) {
					is_n = stmp.find(delimiter, is);
					if(is_n == string::npos) {
						str1 = stmp.substr(is, stmp.length() - is);
					} else {
						str1 = stmp.substr(is, is_n - is);
						is = is_n + delimiter.length();
					}
					parse(&mstruct[rows - 1][column - 1], str1);
					column++;
					if(is_n == string::npos) {
						break;
					}
				}
				v_added = true;
			}
		}
	}
	return true;
}

bool Calculator::importCSV(const char *file_name, int first_row, bool headers, string delimiter, bool to_matrix, string name, string title, string category) {
	FILE *file = fopen(file_name, "r");
	if(file == NULL) {
		return false;
	}
	if(first_row < 1) {
		first_row = 1;
	}
	string filestr = file_name;
	remove_blank_ends(filestr);
	size_t i = filestr.find_last_of("/");
	if(i != string::npos) {
		filestr = filestr.substr(i + 1, filestr.length() - (i + 1));
	}
	remove_blank_ends(name);
	if(name.empty()) {
		name = filestr;
		i = name.find_last_of("/");
		if(i != string::npos) name = name.substr(i + 1, name.length() - i);
		i = name.find_last_of(".");
		if(i != string::npos) name = name.substr(0, i);
	}

	char line[10000];
	string stmp, str1, str2;
	int row = 0;
	int columns = 1, rows = 1;
	int column;
	vector<string> header;
	vector<MathStructure> vectors;
	MathStructure mstruct = m_empty_matrix;
	size_t is, is_n;
	bool v_added = false;
	while(fgets(line, 10000, file)) {
		row++;
		if(row >= first_row) {
			stmp = line;
			remove_blank_ends(stmp);
			if(row == first_row) {
				if(stmp.empty()) {
					row--;
				} else {
					is = 0;
					while((is_n = stmp.find(delimiter, is)) != string::npos) {
						columns++;
						if(headers) {
							str1 = stmp.substr(is, is_n - is);
							remove_blank_ends(str1);
							header.push_back(str1);
						}
						if(!to_matrix) {
							vectors.push_back(m_empty_vector);
						}
						is = is_n + delimiter.length();
					}
					if(headers) {
						str1 = stmp.substr(is, stmp.length() - is);
						remove_blank_ends(str1);
						header.push_back(str1);
					}
					if(to_matrix) {
						mstruct.resizeMatrix(1, columns, m_undefined);
					} else {
						vectors.push_back(m_empty_vector);
					}
				}
			}
			if((!headers || row > first_row) && !stmp.empty()) {
				if(to_matrix && v_added) {
					mstruct.addRow(m_undefined);
					rows++;
				}
				is = 0;
				column = 1;
				while(column <= columns) {
					is_n = stmp.find(delimiter, is);
					if(is_n == string::npos) {
						str1 = stmp.substr(is, stmp.length() - is);
					} else {
						str1 = stmp.substr(is, is_n - is);
						is = is_n + delimiter.length();
					}
					if(to_matrix) {
						parse(&mstruct[rows - 1][column - 1], str1);
					} else {
						vectors[column - 1].addChild(parse(str1));
					}
					column++;
					if(is_n == string::npos) {
						break;
					}
				}
				for(; column <= columns; column++) {
					if(!to_matrix) {
						vectors[column - 1].addChild(m_undefined);
					}
				}
				v_added = true;
			}
		}
	}
	if(to_matrix) {
		addVariable(new KnownVariable(category, name, mstruct, title));
	} else {
		if(vectors.size() > 1) {
			if(!category.empty()) {
				category += "/";
			}
			category += name;
		}
		for(size_t i = 0; i < vectors.size(); i++) {
			str1 = "";
			str2 = "";
			if(vectors.size() > 1) {
				str1 += name;
				str1 += "_";
				if(title.empty()) {
					str2 += name;
					str2 += " ";
				} else {
					str2 += title;
					str2 += " ";
				}
				if(i < header.size()) {
					str1 += header[i];
					str2 += header[i];
				} else {
					str1 += _("column");
					str1 += "_";
					str1 += i2s(i + 1);
					str2 += _("Column ");
					str2 += i2s(i + 1);
				}
				gsub(" ", "_", str1);
			} else {
				str1 = name;
				str2 = title;
				if(i < header.size()) {
					str2 += " (";
					str2 += header[i];
					str2 += ")";
				}
			}
			addVariable(new KnownVariable(category, str1, vectors[i], str2));
		}
	}
	return true;
}
bool Calculator::exportCSV(const MathStructure &mstruct, const char *file_name, string delimiter) {
	FILE *file = fopen(file_name, "w+");
	if(file == NULL) {
		return false;
	}
	MathStructure mcsv(mstruct);
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	po.decimalpoint_sign = ".";
	po.comma_sign = ",";
	if(mcsv.isMatrix()) {
		for(size_t i = 0; i < mcsv.size(); i++) {
			for(size_t i2 = 0; i2 < mcsv[i].size(); i2++) {
				if(i2 > 0) fputs(delimiter.c_str(), file);
				mcsv[i][i2].format(po);
				fputs(mcsv[i][i2].print(po).c_str(), file);
			}
			fputs("\n", file);
		}
	} else if(mcsv.isVector()) {
		for(size_t i = 0; i < mcsv.size(); i++) {
			mcsv[i].format(po);
			fputs(mcsv[i].print(po).c_str(), file);
			fputs("\n", file);
		}
	} else {
		mcsv.format(po);
		fputs(mcsv.print(po).c_str(), file);
		fputs("\n", file);
	}
	fclose(file);
	return true;
}


bool Calculator::loadExchangeRates() {

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *value;
	bool global_file = false;
	string currency, rate, sdate;

	unordered_map<Unit*, bool> cunits;

	string filename = buildPath(getLocalDataDir(), "custom_exchange_rates");
	ifstream cfile(filename.c_str());
	if(cfile.is_open()) {
		char linebuffer[1000];
		string sbuffer, s1, s2;
		string nr1, nr2;
		Unit *u1, *u2;
		while((cfile.rdstate() & std::ifstream::eofbit) == 0) {
			cfile.getline(linebuffer, 1000);
			if((cfile.rdstate() & std::ifstream::failbit) != 0) break;
			sbuffer = linebuffer;
			size_t i = sbuffer.find("=");
			if(i != string::npos && i > 0 && i < sbuffer.length() - 1) {
				s1 = sbuffer.substr(0, i);
				s2 = sbuffer.substr(i + 1, sbuffer.length() - (i + 1));
				remove_blanks(s1);
				remove_blanks(s2);
				if(!s1.empty() && !s2.empty()) {
					i = s1.find_first_not_of(ILLEGAL_IN_UNITNAMES);
					if(i == 0) {
						i = s1.find_first_of(ILLEGAL_IN_UNITNAMES);
						if(i == string::npos) {
							nr1 = "1";
						} else {
							nr1 = s1.substr(i, s1.length() - i);
							if(i == 0) s1 = "";
							else s1 = s1.substr(0, i);
						}
					} else {
						if(i == s1.length() - 1) nr1 = "1";
						else nr1 = s1.substr(0, i);
						s1 = s1.substr(i, s1.length() - i);
					}
					i = s2.find_first_not_of(ILLEGAL_IN_UNITNAMES);
					if(i == 0) {
						i = s2.find_first_of(ILLEGAL_IN_UNITNAMES);
						if(i == string::npos) {
							nr2 = "1";
						} else {
							nr2 = s2.substr(i, s2.length() - i);
							if(i == 0) s2 = "";
							else s2 = s2.substr(0, i);
						}
					} else {
						if(i == s2.length() - 1) nr2 = "1";
						else nr2 = s2.substr(0, i);
						s2 = s2.substr(i, s2.length() - i);
					}
				}
				if(!s1.empty() && !s2.empty()) {
					u2 = CALCULATOR->getActiveUnit(s2);
					if(!u2) {
						u2 = addUnit(new AliasUnit(_("Currency"), s2, "", "", "", u_euro, "1", 1, "", false, true));
					}
					if(u2 && u2->isCurrency()) {
						u1 = CALCULATOR->getActiveUnit(s1);
						if(!u1) {
							u1 = addUnit(new AliasUnit(_("Currency"), s1, "", "", "", u2, nr1 == "1" ? nr2 : nr2 + string("/") + nr1, 1, "", false, true));
							u1->setApproximate();
							u1->setPrecision(-2);
							u1->setChanged(false);
							cunits[u1] = true;
						} else if(u1->isCurrency()) {
							if(u1 == u_euro) {
								((AliasUnit*) u2)->setBaseUnit(u1);
								((AliasUnit*) u2)->setExpression(nr2 == "1" ? nr1 : nr1 + string("/") + nr2);
								u2->setApproximate();
								u2->setPrecision(-2);
								u2->setChanged(false);
								cunits[u2] = true;
							} else {
								((AliasUnit*) u1)->setBaseUnit(u2);
								((AliasUnit*) u1)->setExpression(nr1 == "1" ? nr2 : nr2 + string("/") + nr1);
								u1->setApproximate();
								u1->setPrecision(-2);
								u1->setChanged(false);
								cunits[u1] = true;
							}
						}
					}
				}
			}
		}
		cfile.close();
	}

	filename = buildPath(getLocalDataDir(), "eurofxref-daily.xml");
	if(fileExists(filename)) {
		doc = xmlParseFile(filename.c_str());
	} else {
#ifndef _WIN32
		string filename_old = buildPath(getOldLocalDir(), "eurofxref-daily.xml");
		if(fileExists(filename)) {
			doc = xmlParseFile(filename_old.c_str());
			if(doc) {
				recursiveMakeDir(getLocalDataDir());
				move_file(filename_old.c_str(), filename.c_str());
				removeDir(getOldLocalDir());
			}
		}
#endif
	}
	if(doc) cur = xmlDocGetRootElement(doc);
	if(!cur) {
		if(doc) xmlFreeDoc(doc);
#ifdef COMPILED_DEFINITIONS
		doc = xmlParseMemory(eurofxref_daily_xml, strlen(eurofxref_daily_xml));
#else
		filename = buildPath(getGlobalDefinitionsDir(), "eurofxref-daily.xml");
		doc = xmlParseFile(filename.c_str());
#endif
		if(!doc) return false;
		cur = xmlDocGetRootElement(doc);
		if(!cur) return false;
		global_file = true;
	}
	Unit *u;
	while(cur) {
		if(!xmlStrcmp(cur->name, (const xmlChar*) "Cube")) {
			if(global_file && sdate.empty()) {
				XML_GET_STRING_FROM_PROP(cur, "time", sdate);
				QalculateDateTime qdate;
				if(qdate.set(sdate)) {
					exchange_rates_time[0] = (time_t) qdate.timestamp().ulintValue();
					if(exchange_rates_time[0] > exchange_rates_check_time[0]) exchange_rates_check_time[0] = exchange_rates_time[0];
				} else {
					sdate.clear();
				}
			}
			XML_GET_STRING_FROM_PROP(cur, "currency", currency);
			if(!currency.empty()) {
				XML_GET_STRING_FROM_PROP(cur, "rate", rate);
				if(!rate.empty()) {
					rate = "1/" + rate;
					u = getUnit(currency);
					if(!u) {
						u = addUnit(new AliasUnit(_("Currency"), currency, "", "", "", u_euro, rate, 1, "", false, true));
					} else if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
						if(cunits.find(u) != cunits.end()) {
							u = NULL;
						} else {
							((AliasUnit*) u)->setBaseUnit(u_euro);
							((AliasUnit*) u)->setExpression(rate);
						}
					}
					if(u) {
						u->setApproximate();
						u->setPrecision(-2);
						u->setChanged(false);
					}
				}
			}
		}
		if(cur->children) {
			cur = cur->children;
		} else if(cur->next) {
			cur = cur->next;
		} else {
			cur = cur->parent;
			if(cur) {
				cur = cur->next;
			}
		}
	}
	xmlFreeDoc(doc);
	if(sdate.empty()) {
		struct stat stats;
		if(stat(filename.c_str(), &stats) == 0) {
			if(exchange_rates_time[0] >= stats.st_mtime) {
#ifdef _WIN32
				struct _utimbuf new_times;
#else
				struct utimbuf new_times;
#endif
				struct tm *temptm = localtime(&exchange_rates_time[0]);
				if(temptm) {
					struct tm extm = *temptm;
					time_t time_now = time(NULL);
					struct tm *newtm = localtime(&time_now);
					if(newtm && newtm->tm_mday != extm.tm_mday) {
						newtm->tm_hour = extm.tm_hour;
						newtm->tm_min = extm.tm_min;
						newtm->tm_sec = extm.tm_sec;
						exchange_rates_time[0] = mktime(newtm);
					} else {
						time(&exchange_rates_time[0]);
					}
				} else {
					time(&exchange_rates_time[0]);
				}
				new_times.modtime = exchange_rates_time[0];
				new_times.actime = exchange_rates_time[0];
#ifdef _WIN32
				_utime(filename.c_str(), &new_times);
#else
				utime(filename.c_str(), &new_times);
#endif
			} else {
				exchange_rates_time[0] = stats.st_mtime;
				if(exchange_rates_time[0] > exchange_rates_check_time[0]) exchange_rates_check_time[0] = exchange_rates_time[0];
			}
		}
	}

	if(cunits.find(u_btc) == cunits.end()) {
		filename = getExchangeRatesFileName(2);
		ifstream file2(filename.c_str());
		if(file2.is_open()) {
			std::stringstream ssbuffer2;
			ssbuffer2 << file2.rdbuf();
			string sbuffer = ssbuffer2.str();
			size_t i = sbuffer.find("\"amount\":");
			size_t i3 = sbuffer.find("\"currency\":");
			if(i != string::npos && i3 != string::npos) {
				i = sbuffer.find("\"", i + 9);
				i3 = sbuffer.find("\"", i3 + 11);
				if(i != string::npos && i3 != string::npos) {
					size_t i2 = sbuffer.find("\"", i + 1);
					size_t i4 = sbuffer.find("\"", i3 + 1);
					u = getActiveUnit(sbuffer.substr(i3 + 1, i4 - (i3 + 1)));
					if(u && u->isCurrency()) {
						((AliasUnit*) u_btc)->setBaseUnit(u);
						((AliasUnit*) u_btc)->setExpression(sbuffer.substr(i + 1, i2 - (i + 1)));
						u_btc->setApproximate();
						u_btc->setPrecision(-2);
						u_btc->setChanged(false);
					}
				}
			}
			file2.close();
			struct stat stats;
			if(stat(filename.c_str(), &stats) == 0) {
				exchange_rates_time[1] = stats.st_mtime;
				if(exchange_rates_time[1] > exchange_rates_check_time[1]) exchange_rates_check_time[1] = exchange_rates_time[1];
			}
		}
	} else {
		exchange_rates_time[1] = exchange_rates_time[0];
		if(exchange_rates_time[1] > exchange_rates_check_time[1]) exchange_rates_check_time[1] = exchange_rates_time[1];
	}

	string sbuffer;
	filename = getExchangeRatesFileName(3);
	ifstream file(filename.c_str());
	global_file = false;
	if(file.is_open()) {
		std::stringstream ssbuffer;
		ssbuffer << file.rdbuf();
		sbuffer = ssbuffer.str();
	}
	if(sbuffer.empty()) {
		if(file.is_open()) file.close();
		file.clear();
#ifdef COMPILED_DEFINITIONS
		sbuffer = rates_json;
#else
		filename = buildPath(getGlobalDefinitionsDir(), "rates.json");
		file.open(filename.c_str());
		if(file.is_open()) {
			std::stringstream ssbuffer;
			ssbuffer << file.rdbuf();
			sbuffer = ssbuffer.str();
		}
#endif
		size_t i = sbuffer.find("\"date\":");
		if(i != string::npos) {
			i = sbuffer.find("\"", i + 7);
			if(i != string::npos) {
				size_t i2 = sbuffer.find("\"", i + 1);
				if(i2 != string::npos) {
					QalculateDateTime qdate;
					if(qdate.set(sbuffer.substr(i + 1, i2 - (i + 1)))) {
						exchange_rates_time[2] = (time_t) qdate.timestamp().ulintValue();
						if(exchange_rates_time[2] > exchange_rates_check_time[2]) exchange_rates_check_time[2] = exchange_rates_time[2];
						global_file = true;
					}
				}
			}
		}
	}
	int json_variant = 0;
	size_t i = sbuffer.find("\"currency_code\":");
	if(i != string::npos) {
		json_variant = 1;
	} else {
		i = sbuffer.find("\"alphaCode\":");
		if(i != string::npos) {
			json_variant = 2;
		} else if(sbuffer.find("exchangerate.host") != string::npos) {
			i = sbuffer.find("rates");
		} else {
			i = sbuffer.find("\"eur\": {");
			if(i == string::npos) i = sbuffer.find("\"eur\":{");
			if(i == string::npos) {
				i = sbuffer.find("\": {");
				if(i == string::npos) i = sbuffer.find("\":{");
				if(i != string::npos) i = sbuffer.find("\"", i + 1);
			} else {
				i = sbuffer.find("\"", i + 5);
			}
		}

	}
	priv->exchange_rates_url3 = json_variant;
	bool byn_found = false;
	string sname;
	string currency_defs;
	if(i != string::npos) {
#ifdef COMPILED_DEFINITIONS
		currency_defs = currencies_xml;
#else
		ifstream cfile(buildPath(getGlobalDefinitionsDir(), "currencies.xml").c_str());
		if(cfile.is_open()) {
			std::stringstream ssbuffer;
			ssbuffer << cfile.rdbuf();
			currency_defs = ssbuffer.str();
			cfile.close();
		}
#endif
	}
	string builtin_str = "<builtin_unit name=\"";
	while(i != string::npos) {
		currency = ""; sname = ""; rate = "";
		size_t i2 = 0, i3 = 0;
		if(json_variant == 1 || json_variant == 2) {
			if(json_variant == 1) i += 16;
			else i += 12;
			i2 = sbuffer.find("\"", i);
			if(i2 == string::npos) break;
			i3 = sbuffer.find("\"", i2 + 1);
			if(i3 == string::npos) break;
			currency = sbuffer.substr(i2 + 1, i3 - (i2 + 1));
			if(json_variant == 1) i = sbuffer.find("\"currency_code\":", i);
			else if(json_variant == 2) i = sbuffer.find("\"alphaCode\":", i);
		} else {
			i2 = sbuffer.find("\"", i + 1);
			if(i2 == string::npos) break;
			currency = sbuffer.substr(i + 1, i2 - (i + 1));
			remove_blank_ends(currency);
			for(i = 0; i < currency.size(); i++) {
				if(currency[i] >= 'a' && currency[i] <= 'z') currency[i] -= 32;
			}
			i = sbuffer.find("\"", i2 + 1);
		}
		if(currency.length() == 3 && (currency_defs.empty() || currency_defs.find(builtin_str + currency) != string::npos) && currency != "BYR") {
			if(!byn_found && currency == "BYN") byn_found = true;
			u = getUnit(currency);
			if(!u || (u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->isHidden() && u->isBuiltin() && !u->isLocal())) {
				if(json_variant == 1 || json_variant == 2) {
					i2 = sbuffer.find("\"rate\":", i3 + 1);
					size_t i4 = sbuffer.find("}", i3 + 1);
					if(i2 != string::npos && i2 < i4) {
						i3 = sbuffer.find(",", i2 + 7);
						rate = "1/" + sbuffer.substr(i2 + 7, i3 - (i2 + 7));
						if(!u) {
							i2 = sbuffer.find("\"name\":\"", i3 + 1);
							if(i2 != string::npos && i2 < i4) {
								i3 = sbuffer.find("\"", i2 + 8);
								if(i3 != string::npos) {
									sname = sbuffer.substr(i2 + 8, i3 - (i2 + 8));
									remove_blank_ends(sname);
								}
							}
						}
					}
				} else {
					i2 = sbuffer.find(":", i2 + 1);
					if(i2 != string::npos) i2 = sbuffer.find_first_not_of(SPACES, i2 + 1);
					if(i2 != string::npos) {
						size_t i3 = sbuffer.find_first_not_of(NUMBERS ".", i2);
						if(i3 != string::npos && i3 != i2 + 1) {
							rate = "1/" + sbuffer.substr(i2, i3 - i2);
						}
					}
				}
				if(!rate.empty()) {
					if(!u) {
						u = addUnit(new AliasUnit(_("Currency"), currency, "", "", sname, u_euro, rate, 1, "", false, true), false, true);
						if(u) u->setHidden(true);
					} else {
						if(cunits.find(u) != cunits.end()) {
							u = NULL;
						} else {
							((AliasUnit*) u)->setBaseUnit(u_euro);
							((AliasUnit*) u)->setExpression(rate);
						}
					}
					if(u) {
						u->setApproximate();
						u->setPrecision(-2);
						u->setChanged(false);
					}
				}
			}
		}
	}
	if(file.is_open()) file.close();
	if(!global_file) {
		struct stat stats;
		if(stat(filename.c_str(), &stats) == 0) {
			exchange_rates_time[2] = stats.st_mtime;
			if(exchange_rates_time[2] > exchange_rates_check_time[2]) exchange_rates_check_time[2] = exchange_rates_time[2];
		}
	}

	if(byn_found && !global_file) {
		priv->exchange_rates_time2[0] = exchange_rates_time[2];
		priv->exchange_rates_check_time2[0] = exchange_rates_check_time[2];
	} else if(cunits.find(priv->u_byn) == cunits.end()) {
		filename = getExchangeRatesFileName(4);
		ifstream file3(filename.c_str());
		if(file3.is_open()) {
			std::stringstream ssbuffer3;
			ssbuffer3 << file3.rdbuf();
			string sbuffer = ssbuffer3.str();
			size_t i = sbuffer.find("\"Cur_OfficialRate\":");
			if(i != string::npos) {
				i = sbuffer.find_first_of(NUMBER_ELEMENTS, i + 19);
				if(i != string::npos) {
					size_t i2 = sbuffer.find_first_not_of(NUMBER_ELEMENTS, i);
					if(i2 == string::npos) i2 = sbuffer.length();
					((AliasUnit*) priv->u_byn)->setBaseUnit(u_euro);
					((AliasUnit*) priv->u_byn)->setExpression(string("1/") + sbuffer.substr(i, i2 - i));
					priv->u_byn->setApproximate();
					priv->u_byn->setPrecision(-2);
					priv->u_byn->setChanged(false);
				}
			}
			i = sbuffer.find("\"Date\":");
			if(i != string::npos) {
				i = sbuffer.find("\"", i + 7);
				if(i != string::npos) {
					size_t i2 = sbuffer.find("\"", i + 1);
					QalculateDateTime qdate;
					if(qdate.set(sbuffer.substr(i + 1, i2 - (i + 1)))) {
						priv->exchange_rates_time2[0] = (time_t) qdate.timestamp().ulintValue();
						if(priv->exchange_rates_time2[0] > priv->exchange_rates_check_time2[0]) priv->exchange_rates_check_time2[0] = priv->exchange_rates_time2[0];
					}
				}
			}
			file3.close();
		}
	} else {
		priv->exchange_rates_time2[0] = exchange_rates_time[0];
		if(priv->exchange_rates_time2[0] > priv->exchange_rates_check_time2[0]) priv->exchange_rates_check_time2[0] = priv->exchange_rates_time2[0];
	}

	return true;

}
bool Calculator::hasGVFS() {
	return false;
}
bool Calculator::hasGnomeVFS() {
	return hasGVFS();
}
bool Calculator::canFetch() {
#ifdef HAVE_LIBCURL
	return true;
#else
	return false;
#endif
}
string Calculator::getExchangeRatesFileName(int index) {
	switch(index) {
		case 1: {return buildPath(getLocalDataDir(), "eurofxref-daily.xml");}
		case 2: {return buildPath(getLocalDataDir(), "btc.json");}
		case 3: {return buildPath(getLocalDataDir(), "rates.json");}
		case 4: {return buildPath(getLocalDataDir(), "nrby.json");}
		default: {}
	}
	return "";
}
time_t Calculator::getExchangeRatesTime(int index) {
	if(index > 5) index = 5;
	if(index < 1) {
		time_t extime = exchange_rates_time[0];
		for(int i = 1; i < 4; i++) {
			if(i > 2 && priv->exchange_rates_time2[i - 3] < extime) extime = priv->exchange_rates_time2[i - 3];
			else if(i <= 2 && exchange_rates_time[i] < extime) extime = exchange_rates_time[i];
		}
		return extime;
	}
	index--;
	if(index == 5) {
		if(exchange_rates_time[2] < priv->exchange_rates_time2[0]) return exchange_rates_time[2];
		return priv->exchange_rates_time2[0];
	}
	if(index > 2) return priv->exchange_rates_time2[index - 3];
	return exchange_rates_time[index];
}

string Calculator::getExchangeRatesUrl(int index) {
	switch(index) {
		case 1: {return "https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml";}
		case 2: {return "https://api.coinbase.com/v2/prices/spot?currency=EUR";}
		case 3: {
			if(priv->exchange_rates_url3 == 1) {
				return "https://www.mycurrency.net/FR.json";
			} else if(priv->exchange_rates_url3 == 2) {
				return "https://www.floatrates.com/daily/eur.json";
			} else {
				return "https://cdn.jsdelivr.net/npm/@fawazahmed0/currency-api@latest/v1/currencies/eur.json";
                        }

		}
		case 4: {return "https://www.nbrb.by/api/exrates/rates/eur?parammode=2";}
		default: {}
	}
	return "";
}
bool Calculator::fetchExchangeRates(int timeout, string) {return fetchExchangeRates(timeout);}
size_t write_data(void *ptr, size_t size, size_t nmemb, string *sbuffer) {
	sbuffer->append((char*) ptr, size * nmemb);
	return size * nmemb;
}

#define FER_ERROR(x, y, z, u) \
	if(n <= 0 && strlen(z) > 0) {\
		char buffer[10000];\
		int n1 = 0;\
		if(strlen(u) > 0) n1 = snprintf(buffer, 10000, _("Failed to download exchange rates (%s) from %s: %s."), u, x, y);\
		else n1 = snprintf(buffer, 10000, _("Failed to download exchange rates from %s: %s."), x, y);\
		if(n1 > 0 && n1 < 9000) {\
			buffer[n1] = ' '; n1++;\
			int n2 = snprintf(buffer + (sizeof(char) * n1), 10000 - n1, _("Exchange rates were successfully downloaded from %s."), z);\
			if(n2 > 0 && n2 + n1 < 10000) {\
				error(true, buffer, NULL);\
			}\
		}\
	} else {\
		if(strlen(u) > 0) error(true, _("Failed to download exchange rates (%s) from %s: %s."), u, x, y, NULL);\
		else error(true, _("Failed to download exchange rates from %s: %s."), x, y, NULL);\
	}

#define FETCH_FAIL_CLEANUP curl_easy_cleanup(curl); curl_global_cleanup(); time(&exchange_rates_check_time[0]); time(&exchange_rates_check_time[1]); time(&exchange_rates_check_time[2]); time(&priv->exchange_rates_check_time2[0]);

bool Calculator::fetchExchangeRates(int timeout, int n) {
#ifdef HAVE_LIBCURL
	recursiveMakeDir(getLocalDataDir());
	string sbuffer;
	char error_buffer[CURL_ERROR_SIZE];
	CURL *curl;
	CURLcode res;
	long int file_time = 0;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(!curl) {return false;}
	curl_easy_setopt(curl, CURLOPT_URL, getExchangeRatesUrl(1).c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
	error_buffer[0] = 0;
	curl_easy_setopt(curl, CURLOPT_FILETIME, &file_time);
#ifdef _WIN32
	char exepath[MAX_PATH];
	GetModuleFileName(NULL, exepath, MAX_PATH);
	string datadir(exepath);
	datadir.resize(datadir.find_last_of('\\'));
	if(datadir.substr(datadir.length() - 4) != "\\bin" && datadir.substr(datadir.length() - 6) != "\\.libs") {
		string cainfo = buildPath(datadir, "ssl", "certs", "ca-bundle.crt");
		gsub("\\", "/", cainfo);
		curl_easy_setopt(curl, CURLOPT_CAINFO, cainfo.c_str());
	}
#endif
	res = curl_easy_perform(curl);

	if(res != CURLE_OK) {
		if(strlen(error_buffer)) {FER_ERROR("ecb.europa.eu", error_buffer, "", "");}
		else {FER_ERROR("ecb.europa.eu", curl_easy_strerror(res), "", "");}
		FETCH_FAIL_CLEANUP;
		return false;
	}
	if(sbuffer.find("Cube") == string::npos) {FER_ERROR("ecb.europa.eu", "Document empty", "", ""); FETCH_FAIL_CLEANUP; return false;}
	ofstream file(getExchangeRatesFileName(1).c_str(), ios::out | ios::trunc | ios::binary);
	if(!file.is_open()) {
		FER_ERROR("ecb.europa.eu", strerror(errno), "", "");
		FETCH_FAIL_CLEANUP
		return false;
	}
	file << sbuffer;
	file.close();
	if(file_time > 0) {
#ifdef _WIN32
		struct _utimbuf new_times;
#else
		struct utimbuf new_times;
#endif
		new_times.modtime = (time_t) file_time;
		new_times.actime = (time_t) file_time;
#ifdef _WIN32
		_utime(getExchangeRatesFileName(1).c_str(), &new_times);
#else
		utime(getExchangeRatesFileName(1).c_str(), &new_times);
#endif
	}

	if(n <= 0 || n >= 2) {

		sbuffer = "";
		curl_easy_setopt(curl, CURLOPT_URL, getExchangeRatesUrl(2).c_str());
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

		res = curl_easy_perform(curl);

		if(res != CURLE_OK) {
			if(strlen(error_buffer)) {FER_ERROR("coinbase.com", error_buffer, "ecb.europa.eu", u_btc->title().c_str());}
			else {FER_ERROR("coinbase.com", curl_easy_strerror(res), "ecb.europa.eu", u_btc->title().c_str());}
			FETCH_FAIL_CLEANUP;
			return false;
		}
		if(sbuffer.find("\"amount\":") == string::npos || sbuffer.find("\"currency\":") == string::npos) {FER_ERROR("coinbase.com", "Document empty", "ecb.europa.eu", u_btc->title().c_str()); FETCH_FAIL_CLEANUP; return false;}
		ofstream file3(getExchangeRatesFileName(2).c_str(), ios::out | ios::trunc | ios::binary);
		if(!file3.is_open()) {
			FER_ERROR("coinbase.com", strerror(errno), "ECB", u_btc->title().c_str());
			FETCH_FAIL_CLEANUP
			return false;
		}
		file3 << sbuffer;
		file3.close();

	}

	bool mycurrency_net = false;

	if(n <= 0 || n >= 3) {
		bool b = false;
		bool bad_gateway = false;
		string first_error;
		for(size_t i = 0; i <= 2 || (bad_gateway && i == 3); i++) {
			sbuffer = "";
			if(i == 0) curl_easy_setopt(curl, CURLOPT_URL, "https://cdn.jsdelivr.net/npm/@fawazahmed0/currency-api@latest/v1/currencies/eur.json");
			else if(i == 2) curl_easy_setopt(curl, CURLOPT_URL, "https://www.floatrates.com/daily/eur.json");
			else curl_easy_setopt(curl, CURLOPT_URL, "https://www.mycurrency.net/FR.json");
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
			if(i == 3) curl_easy_setopt(curl, CURLOPT_USERAGENT, (string("libqalculate/") + VERSION).c_str());

			res = curl_easy_perform(curl);

			if(res == CURLE_OK) {
				if((i == 0 && (sbuffer.find(": {") != string::npos || sbuffer.find(":{") != string::npos) && (sbuffer.find("\"eur\": 1,") != string::npos || sbuffer.find("\"eur\":1,") != string::npos || sbuffer.find("\"EUR\": 1,") != string::npos || sbuffer.find("\"EUR\":1,") != string::npos)) || ((i == 1 || i == 3) && sbuffer.find("\"currency_code\":") != string::npos && sbuffer.find("\"baseCurrency\":\"EUR\"") != string::npos) || (i == 2 && sbuffer.find("\"alphaCode\":") != string::npos)) {
					if(i > 2) priv->exchange_rates_url3 = 1;
					else priv->exchange_rates_url3 = i;
					b = true;
					mycurrency_net = (i == 1 || i == 3);
					break;
				}
				if(i == 1 && sbuffer.find("Bad Gateway") != string::npos) bad_gateway = true;
				if(i == 0) first_error = "Document empty";
			} else if(i == 0) {
				if(strlen(error_buffer)) first_error = error_buffer;
				else first_error = curl_easy_strerror(res);
			}
		}
		if(!b) {
			FER_ERROR("floatrates.com", first_error.c_str(), "ecb.europa.eu, coinbase.com", "");
			FETCH_FAIL_CLEANUP;
			return false;
		}
		ofstream file2(getExchangeRatesFileName(3).c_str(), ios::out | ios::trunc | ios::binary);
		if(!file2.is_open()) {
			FER_ERROR("floatrates.com", strerror(errno), "ecb.europa.eu, coinbase.com", "");
			FETCH_FAIL_CLEANUP
			return false;
		}
		file2 << sbuffer;
		file2.close();

	}

	if(n >= 4 && mycurrency_net) {

		sbuffer = "";
		curl_easy_setopt(curl, CURLOPT_URL, getExchangeRatesUrl(4).c_str());
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, (timeout > 1 && n <= 0) ? 1 : timeout);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

		res = curl_easy_perform(curl);

		if(res != CURLE_OK) {
			if(strlen(error_buffer)) {FER_ERROR("nbrb.by", error_buffer, "ecb.europa.eu, coinbase.com, mycurrency.net", priv->u_byn->title().c_str());}
			else {FER_ERROR("nbrb.by", curl_easy_strerror(res), "ecb.europa.eu, coinbase.com, mycurrency.net", priv->u_byn->title().c_str());}
			FETCH_FAIL_CLEANUP;
			return false;
		}
		if(sbuffer.find("\"Cur_OfficialRate\":") == string::npos) {
			FER_ERROR("nbrb.by", "Document empty", "ecb.europa.eu, coinbase.com, mycurrency.net", priv->u_byn->title().c_str());
			FETCH_FAIL_CLEANUP;
			return false;
		}
		ofstream file4(getExchangeRatesFileName(4).c_str(), ios::out | ios::trunc | ios::binary);
		if(!file4.is_open()) {
			FER_ERROR("nbrb.by", strerror(errno), "ecb.europa.eu, coinbase.com, mycurrency.net", priv->u_byn->title().c_str());
			FETCH_FAIL_CLEANUP
			return false;
		}
		file4 << sbuffer;
		file4.close();

	}

	curl_easy_cleanup(curl); curl_global_cleanup();

	return true;
#else
	return false;
#endif
}
bool Calculator::checkExchangeRatesDate(unsigned int n_days, bool force_check, bool send_warning, int n) {
	if(n <= 0) n = 5;
	time_t extime = exchange_rates_time[0];
	for(int i = 1; i < (n > 4 ? 4 : n); i++) {
		if(i > 2 && priv->exchange_rates_time2[i - 3] < extime) extime = priv->exchange_rates_time2[i - 3];
		else if(i <= 2 && (i != 2 || n != 4) && exchange_rates_time[i] < extime) extime = exchange_rates_time[i];
	}
	time_t cextime = exchange_rates_check_time[0];
	for(int i = 1; i < (n > 4 ? 4 : n); i++) {
		if(i > 2 && priv->exchange_rates_check_time2[i - 3] < cextime) cextime = priv->exchange_rates_check_time2[i - 3];
		else if(i <= 2 && (i != 2 || n != 4) && exchange_rates_check_time[i] < cextime) cextime = exchange_rates_check_time[i];
	}
	if(extime > 0 && ((!force_check && cextime > 0 && difftime(time(NULL), cextime) < 86400 * n_days) || difftime(time(NULL), extime) < (86400 * n_days) + 3600)) return true;
	for(int i = 0; i < (n > 4 ? 4 : n); i++) {
		if(i <= 2 && (i != 2 || n != 4)) time(&exchange_rates_check_time[i]);
		else if(i > 2) time(&priv->exchange_rates_check_time2[i - 3]);
	}
	int days = (int) floor(difftime(time(NULL), extime) / 86400);
	if(send_warning) error(false, _n("It has been %s day since the exchange rates last were updated.", "It has been %s days since the exchange rates last were updated.", days), i2s(days).c_str(), NULL);
	return false;
}
void Calculator::setExchangeRatesWarningEnabled(bool enable) {
	b_exchange_rates_warning_enabled = enable;
}
bool Calculator::exchangeRatesWarningEnabled() const {
	return b_exchange_rates_warning_enabled;
}
int Calculator::exchangeRatesUsed() const {
	if(b_exchange_rates_used > 100) return b_exchange_rates_used - 100;
	if(b_exchange_rates_used & 0b1000) {
		if(b_exchange_rates_used & 0b0100) return 5;
		return 4;
	} else if(b_exchange_rates_used & 0b0100) return 3;
	else if(b_exchange_rates_used & 0b0010) return 2;
	else if(b_exchange_rates_used & 0b0001) return 1;
	return 0;
}
void Calculator::resetExchangeRatesUsed() {
	b_exchange_rates_used = 0;
}
void Calculator::setExchangeRatesUsed(int index) {
	if(index == -100) {
		if(b_exchange_rates_used == 0) return;
		if(b_exchange_rates_used > 100) b_exchange_rates_used -= 100;
		else b_exchange_rates_used += 100;
		return;
	}
	b_exchange_rates_used = b_exchange_rates_used | index;
	if(b_exchange_rates_warning_enabled) checkExchangeRatesDate(7, false, true, exchangeRatesUsed());
}

