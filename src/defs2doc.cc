/*
    Qalculate

    Copyright (C) 2004  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"
#include <libqalculate/qalculate.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::list;

KnownVariable *vans[5], *v_memory;
PrintOptions printops;
EvaluationOptions evalops;

char buffer[1000];

FILE *ffile, *vfile, *ufile;

bool is_answer_variable(Variable *v) {
	return v == vans[0] || v == vans[1] || v == vans[2] || v == vans[3] || v == vans[4];
}


string fix(string str, bool replace_signs = false, bool b2 = false) {
	if(printops.use_unicode_signs) {
		gsub(">=", SIGN_GREATER_OR_EQUAL, str);
		gsub("<=", SIGN_LESS_OR_EQUAL, str);
		gsub("!=", SIGN_NOT_EQUAL, str);
	}
	gsub("&", "&amp;", str);
	gsub("<", "&lt;", str);
	gsub(">", "&gt;", str);
	gsub("\n", "</para><para>", str);
	if(replace_signs) {
		gsub(SIGN_POWER_2, "<superscript>2</superscript>", str);
		gsub(SIGN_POWER_3, "<superscript>3</superscript>", str);
		gsub("-", SIGN_MINUS, str);
		size_t i = 0, i2 = 1;
		while(true) {
			i = str.find("^", i2);
			i2 = i + 1;
			if(i == string::npos || i2 == str.length()) break;
			bool b = is_in(NUMBERS, str[i2]);
			if(!b && (i2 + strlen(SIGN_MINUS) < str.length() && str.substr(i2, strlen(SIGN_MINUS)) == SIGN_MINUS && is_in(NUMBERS, str[i2 + strlen(SIGN_MINUS)]))) {
				i2 += strlen(SIGN_MINUS);
				b = true;
			}
			if(b) {
				b = (i2 == str.length() - 1);
				if(!b) b = is_in(OPERATORS COMMA SPACES PARENTHESISS, str[i2 + 1]) || (i2 + strlen(SIGN_MIDDLEDOT) + 1 < str.length() && str.substr(i2 + 1, strlen(SIGN_MIDDLEDOT)) == SIGN_MIDDLEDOT);
				if(b) {
					if(i2 == str.length() - 1) str += "</superscript>";
					else str.insert(i2 + 1, "</superscript>");
					str.erase(i, 1);
					str.insert(i, "<superscript>");
				}
			}
		}
		gsub("*", SIGN_MULTIPLICATION, str);
	}
	return str;
}
string fixcat(string str) {
	gsub(" ", "-", str);
	gsub(".", "", str);
	gsub("&", "", str);
	gsub("<", "", str);
	gsub(">", "", str);
	return str;
}

struct tree_struct {
	string item;
	list<tree_struct> items;
	list<tree_struct>::iterator it;
	list<tree_struct>::reverse_iterator rit;
	vector<void*> objects;
	tree_struct *parent;
	void sort() {
		items.sort();
		for(list<tree_struct>::iterator it = items.begin(); it != items.end(); ++it) {
			it->sort();
		}
	}
	bool operator < (tree_struct &s1) const {
		return item < s1.item;
	}
};

tree_struct function_cats, unit_cats, variable_cats;
vector<void*> ia_units, ia_variables, ia_functions;

void generate_units_tree_struct() {
	size_t cat_i, cat_i_prev;
	bool b;
	string str, cat, cat_sub;
	Unit *u = NULL;
	unit_cats.items.clear();
	unit_cats.objects.clear();
	unit_cats.parent = NULL;
	ia_units.clear();
	list<tree_struct>::iterator it;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->isActive() && CALCULATOR->units[i]->subtype() != SUBTYPE_COMPOSITE_UNIT) {
			tree_struct *item = &unit_cats;
			if(!CALCULATOR->units[i]->category().empty()) {
				cat = CALCULATOR->units[i]->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;
						cat.parent = item;
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			b = false;
			for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
				u = (Unit*) item->objects[i3];
				if(CALCULATOR->units[i]->title() < u->title()) {
					b = true;
					item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->units[i]);
					break;
				}
			}
			if(!b) item->objects.push_back((void*) CALCULATOR->units[i]);
		}
	}

	unit_cats.sort();

}
void generate_variables_tree_struct() {

	size_t cat_i, cat_i_prev;
	bool b;
	string str, cat, cat_sub;
	Variable *v = NULL;
	variable_cats.items.clear();
	variable_cats.objects.clear();
	variable_cats.parent = NULL;
	ia_variables.clear();
	list<tree_struct>::iterator it;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isActive() && !CALCULATOR->variables[i]->isHidden()) {
			tree_struct *item = &variable_cats;
			if(!CALCULATOR->variables[i]->category().empty()) {
				cat = CALCULATOR->variables[i]->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;
						cat.parent = item;
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			b = false;
			for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
				v = (Variable*) item->objects[i3];
				if(CALCULATOR->variables[i]->title() < v->title()) {
					b = true;
					item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->variables[i]);
					break;
				}
			}
			if(!b) item->objects.push_back((void*) CALCULATOR->variables[i]);
		}
	}

	variable_cats.sort();

}
void generate_functions_tree_struct() {

	size_t cat_i, cat_i_prev;
	bool b;
	string str, cat, cat_sub;
	MathFunction *f = NULL;
	function_cats.items.clear();
	function_cats.objects.clear();
	function_cats.parent = NULL;
	ia_functions.clear();
	list<tree_struct>::iterator it;

	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(CALCULATOR->functions[i]->isActive() && !CALCULATOR->functions[i]->isHidden()) {
			tree_struct *item = &function_cats;
			if(!CALCULATOR->functions[i]->category().empty()) {
				cat = CALCULATOR->functions[i]->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;
						cat.parent = item;
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			}
			b = false;
			for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
				f = (MathFunction*) item->objects[i3];
				if(CALCULATOR->functions[i]->title() < f->title()) {
					b = true;
					item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->functions[i]);
					break;
				}
			}
			if(!b) item->objects.push_back((void*) CALCULATOR->functions[i]);
		}
	}

	function_cats.sort();

}

string formatted_name(const ExpressionName *ename, int type) {
	return ename->formattedName(type, true);
}

string fix_supsub(string str) {
	gsub("<sup>", "<superscript>", str);
	gsub("<sub>", "<subscript>", str);
	gsub("</sup>", "</superscript>", str);
	gsub("</sub>", "</subscript>", str);
	return str;
}

void print_function(MathFunction *f) {

		string str;
		fputs("<varlistentry>\n", ffile);
		fprintf(ffile, "<term><emphasis>%s</emphasis></term>\n", f->title(false).c_str());
		fputs("<listitem>\n", ffile);
		Argument *arg;
		Argument default_arg;
		string str2;
		const ExpressionName *ename = &f->preferredName(false, printops.use_unicode_signs);
		str = formatted_name(ename, TYPE_FUNCTION);
		int iargs = f->maxargs();
		if(iargs < 0) {
			iargs = f->minargs() + 1;
			if((int) f->lastArgumentDefinitionIndex() > iargs) iargs = (int) f->lastArgumentDefinitionIndex();
		}
		str += "(";
		if(iargs != 0) {
			for(int i2 = 1; i2 <= iargs; i2++) {
				if(i2 > f->minargs()) {
					str += "[";
				}
				if(i2 > 1) {
					str += CALCULATOR->getComma();
					str += " ";
				}
				arg = f->getArgumentDefinition(i2);
				if(arg && !arg->name().empty()) {
					str2 = arg->name();
				} else {
					str2 = _("argument");
					if(i2 > 1 || f->maxargs() != 1) {
						str2 += " ";
						str2 += i2s(i2);
					}
				}
				str += str2;
				if(i2 > f->minargs()) {
					str += "]";
				}
			}
			if(f->maxargs() < 0) {
				str += CALCULATOR->getComma();
				str += " ...";
			}
		}
		str += ")";
		fprintf(ffile, "<para><command>%s</command></para>\n", str.c_str());
		for(size_t i2 = 1; i2 <= f->countNames(); i2++) {
			if(&f->getName(i2) != ename) {
				fprintf(ffile, "<para><command>%s</command></para>", formatted_name(&f->getName(i2), TYPE_FUNCTION).c_str());
			}
		}
		if(f->subtype() == SUBTYPE_DATA_SET) {
			fputs("<para>", ffile);
			fprintf(ffile, _("Retrieves data from the %s data set for a given object and property. If \"info\" is typed as property, all properties of the object will be listed."), f->title().c_str());
			fputs("</para>", ffile);
		}
		if(!f->description().empty()) {
			fprintf(ffile, "<para>%s</para>\n", fix(f->description()).c_str());
		}
		if(!f->example(true).empty()) {
			str = _("Example:"); str += " "; str += fix(f->example(false, formatted_name(ename, TYPE_FUNCTION)), printops.use_unicode_signs);
			fprintf(ffile, "<para>%s</para>\n", str.c_str());
		}
		if(f->subtype() == SUBTYPE_DATA_SET && !((DataSet*) f)->copyright().empty()) {
			fprintf(ffile, "<para>%s</para>\n", fix(((DataSet*) f)->copyright()).c_str());
		}
		if(iargs) {
			fputs("<formalpara>\n", ffile);
			fprintf(ffile, "<title>%s</title>", _("Arguments"));
			fputs("<para><itemizedlist spacing=\"compact\">\n", ffile);
			for(int i2 = 1; i2 <= iargs; i2++) {
				arg = f->getArgumentDefinition(i2);
				if(arg && !arg->name().empty()) {
					str = arg->name();
				} else {
					str = i2s(i2);
				}
				str += ": ";
				if(arg) {
					str2 = fix(arg->printlong());
				} else {
					str2 = fix(default_arg.printlong());
				}
				if(i2 > f->minargs()) {
					str2 += " (";
					str2 += _("optional");
					if(!f->getDefaultValue(i2).empty() && f->getDefaultValue(i2) != "\"\"") {
						str2 += ", ";
						str2 += _("default: ");
						str2 += f->getDefaultValue(i2);
					}
					str2 += ")";
				}
				str += str2;
				fprintf(ffile, "<listitem><para>%s</para></listitem>\n", str.c_str());
			}
			fputs("</itemizedlist></para>\n", ffile);
			fputs("</formalpara>\n", ffile);
		}
		if(!f->condition().empty()) {
			fputs("<formalpara>\n", ffile);
			fprintf(ffile, "<title>%s</title>", _("Requirement"));
			fputs("<para>\n", ffile);
			fputs(fix(f->printCondition(), printops.use_unicode_signs).c_str(), ffile); fputs("\n", ffile);
			fputs("</para>\n", ffile);
			fputs("</formalpara>\n", ffile);
		}
		if(f->subtype() == SUBTYPE_DATA_SET) {
			DataSet *ds = (DataSet*) f;
			fputs("<formalpara>\n", ffile);
			fprintf(ffile, "<title>%s</title>", _("Properties"));
			fputs("<para><itemizedlist spacing=\"compact\">\n", ffile);
			DataPropertyIter it;
			DataProperty *dp = ds->getFirstProperty(&it);
			while(dp) {
				if(!dp->isHidden()) {
					if(!dp->title(false).empty()) {
						str = dp->title();
						str += ": ";
					}
					for(size_t i = 1; i <= dp->countNames(); i++) {
						if(i > 1) str += ", ";
						str += dp->getName(i);
					}
					if(dp->isKey()) {
						str += " (";
						str += _("key");
						str += ")";
					}
					if(!dp->description().empty()) {
						str += "</para><para>";
						str += fix(dp->description());
					}
					fprintf(ffile, "<listitem><para>%s</para></listitem>\n", str.c_str());
				}
				dp = ds->getNextProperty(&it);
			}
			fputs("</itemizedlist></para>\n", ffile);
			fputs("</formalpara>\n", ffile);
		}
		fputs("</listitem>\n", ffile);
		fputs("</varlistentry>\n", ffile);
}

void print_variable(Variable *v) {
		string value, str;
		fputs("<row valign=\"top\">\n", vfile);
		fprintf(vfile, "<entry><para>%s</para></entry>\n", v->title().c_str());
		bool b_first = true;
		for(size_t i2 = 1; i2 <= v->countNames(); i2++) {
			if(!b_first) str += " / ";
			b_first = false;
			str += formatted_name(&v->getName(i2), STRUCT_VARIABLE);
		}
		fprintf(vfile, "<entry><para>%s</para></entry>\n", str.c_str());
		value = "";
		bool is_relative = false;
		if(is_answer_variable(v)) {
			value = _("a previous result");
		} else if(v == v_memory) {
			value = _("result of memory operations (MC, MS, M+, M−)");
		} else if(v->isKnown()) {
			if(v->id() == VARIABLE_ID_PRECISION) {
				value = _("current precision");
			} else if(v->id() == VARIABLE_ID_TODAY) {
				value = _("current date");
			} else if(v->id() == VARIABLE_ID_TOMORROW) {
				value = _("tomorrow's date");
			} else if(v->id() == VARIABLE_ID_YESTERDAY) {
				value = _("yesterday's date");
			} else if(v->id() == VARIABLE_ID_NOW) {
				value = _("current date and time");
			} else if(v->id() == VARIABLE_ID_UPTIME) {
				value = _("current computer uptime");
			} else if(((KnownVariable*) v)->isExpression()) {
				value = fix(CALCULATOR->localizeExpression(((KnownVariable*) v)->expression()), printops.use_unicode_signs);
				if(!((KnownVariable*) v)->uncertainty(&is_relative).empty()) {
					if(is_relative) {value += " ("; value += _("relative uncertainty"); value += ": ";}
					else value += SIGN_PLUSMINUS;
					value += fix(CALCULATOR->localizeExpression(((KnownVariable*) v)->uncertainty()), printops.use_unicode_signs);
					if(is_relative) {value += ")";}
				}
				if(((KnownVariable*) v)->expression().find_first_not_of(NUMBER_ELEMENTS EXPS) == string::npos && value.length() > 40) {
					value = value.substr(0, 30);
					value += "...";
				}
				if(!((KnownVariable*) v)->unit().empty() && ((KnownVariable*) v)->unit() != "auto") {
					value += " ";
					value += fix(((KnownVariable*) v)->unit());
				}
			} else {
				if(((KnownVariable*) v)->get().isMatrix()) {
					value = _("matrix");
				} else if(((KnownVariable*) v)->get().isVector()) {
					value = _("vector");
				} else {
					value = fix(CALCULATOR->print(((KnownVariable*) v)->get(), 30, printops), printops.use_unicode_signs);
				}
			}
		} else {
			if(((UnknownVariable*) v)->assumptions()) {
				switch(((UnknownVariable*) v)->assumptions()->sign()) {
					case ASSUMPTION_SIGN_POSITIVE: {value = _("positive"); break;}
					case ASSUMPTION_SIGN_NONPOSITIVE: {value = _("non-positive"); break;}
					case ASSUMPTION_SIGN_NEGATIVE: {value = _("negative"); break;}
					case ASSUMPTION_SIGN_NONNEGATIVE: {value = _("non-negative"); break;}
					case ASSUMPTION_SIGN_NONZERO: {value = _("non-zero"); break;}
					default: {}
				}
				if(!value.empty() && ((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_NONE) value += " ";
				switch(((UnknownVariable*) v)->assumptions()->type()) {
					case ASSUMPTION_TYPE_INTEGER: {value += _("integer"); break;}
					case ASSUMPTION_TYPE_RATIONAL: {value += _("rational"); break;}
					case ASSUMPTION_TYPE_REAL: {value += _("real"); break;}
					case ASSUMPTION_TYPE_COMPLEX: {value += _("complex"); break;}
					case ASSUMPTION_TYPE_NUMBER: {value += _("number"); break;}
					case ASSUMPTION_TYPE_NONMATRIX: {value += _("non-matrix"); break;}
					default: {}
				}
				if(value.empty()) value = _("unknown");
			} else {
				value = _("default assumptions");
			}
		}
		if(v->isApproximate() && !is_relative && value.find(SIGN_PLUSMINUS) == string::npos) {
			if(v->id() == VARIABLE_ID_PI || v->id() == VARIABLE_ID_E || v->id() == VARIABLE_ID_EULER || v->id() == VARIABLE_ID_CATALAN) {
				value += " (";
				value += _("variable precision");
				value += ")";
			} else {
				value += " (";
				value += _("approximate");
				value += ")";
			}
		}
		fprintf(vfile, "<entry><para>%s</para></entry>\n", value.c_str());
		fputs("</row>\n", vfile);
}

void print_unit(Unit *u) {
		if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) return;
		string str, base_unit, relation;
		fputs("<row valign=\"top\">\n", ufile);
		fprintf(ufile, "<entry><para>%s</para></entry>\n", u->title().c_str());
		bool b_first = true;
		for(size_t i2 = 1; i2 <= u->countNames(); i2++) {
			if(!b_first) str += " / ";
			b_first = false;
			str += formatted_name(&u->getName(i2), STRUCT_UNIT);
		}
		if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			fprintf(ufile, "<entry><para>(%s)</para></entry>\n", str.c_str());
		} else {
			fprintf(ufile, "<entry><para>%s</para></entry>\n", str.c_str());
		}
		switch(u->subtype()) {
			case SUBTYPE_BASE_UNIT: {
				base_unit = "";
				relation = "";
				break;
			}
			case SUBTYPE_ALIAS_UNIT: {
				AliasUnit *au = (AliasUnit*) u;
				base_unit = fix_supsub(au->firstBaseUnit()->print(printops, true, TAG_TYPE_HTML, false, false));
				if(au->firstBaseExponent() != 1) {
					if(au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {base_unit.insert(0, 1, '('); base_unit += ")";}
					base_unit += "<superscript>";
					base_unit += i2s(au->firstBaseExponent());
					base_unit += "</superscript>";
				}
				bool is_relative = false;
				if(au->baseUnit() == CALCULATOR->u_euro && au->isBuiltin()) {
					relation = "exchange rate";
				} else {
					relation = fix(CALCULATOR->localizeExpression(au->expression()).c_str(), printops.use_unicode_signs);
					if(!au->uncertainty(&is_relative).empty()) {
						if(is_relative) {relation += " ("; relation += _("relative uncertainty"); relation += ": ";}
						else relation += SIGN_PLUSMINUS;
						relation += fix(CALCULATOR->localizeExpression(au->uncertainty()));
						if(is_relative) {relation += ")";}
					}
					if(u->isApproximate() && !is_relative && relation.find(SIGN_PLUSMINUS) == string::npos) {
						relation += " (";
						relation += _("approximate");
						relation += ")";
					}
				}
				break;
			}
			case SUBTYPE_COMPOSITE_UNIT: {
				base_unit = fix_supsub(((CompositeUnit*) u)->print(printops, true, TAG_TYPE_HTML, false, false));
				relation = "";
				break;
			}
		}
		fprintf(ufile, "<entry><para>%s</para></entry>\n", base_unit.c_str());
		fprintf(ufile, "<entry><para>%s</para></entry>\n", relation.c_str());
		fputs("</row>\n", ufile);
}

int main(int, char *[]) {


#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	//create the almighty Calculator object
	new Calculator();

	ffile = fopen("appendixa.xml", "w+");
	vfile = fopen("appendixb.xml", "w+");
	ufile = fopen("appendixc.xml", "w+");

	string str;

	CALCULATOR->loadExchangeRates();

	string ans_str = _("ans");
	vans[0] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str, m_undefined, _("Last Answer"), false));
	vans[0]->addName(_("answer"));
	vans[0]->addName(ans_str + "1");
	vans[1] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "2", m_undefined, _("Answer 2"), false));
	vans[2] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "3", m_undefined, _("Answer 3"), false));
	vans[3] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "4", m_undefined, _("Answer 4"), false));
	vans[4] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "5", m_undefined, _("Answer 5"), false));
	v_memory = new KnownVariable(CALCULATOR->temporaryCategory(), "", m_zero, _("Memory"), true, true);
	ExpressionName ename;
	ename.name = "MR";
	ename.case_sensitive = true;
	ename.abbreviation = true;
	v_memory->addName(ename);
	ename.name = "MRC";
	v_memory->addName(ename);
	CALCULATOR->addVariable(v_memory);

	//load global definitions
	if(!CALCULATOR->loadGlobalDefinitions()) {
		printf(_("Failed to load global definitions!\n"));
	}
	printops.use_unicode_signs = true;
	printops.multiplication_sign = MULTIPLICATION_SIGN_X;
	printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;

	generate_functions_tree_struct();
	generate_variables_tree_struct();
	generate_units_tree_struct();

	fputs("<appendix id=\"qalculate-definitions-functions\">\n", ffile);
	fputs("<title>Function List</title>\n", ffile);
	tree_struct *item, *item2;
	function_cats.it = function_cats.items.begin();
	if(function_cats.it != function_cats.items.end()) {
		item = &*function_cats.it;
		++function_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	int level = 1;
	while(item) {
		fprintf(ffile, "<sect%i id=\"qalculate-definitions-functions-%i-%s\">\n", level, level, fixcat(item->item).c_str());
		fprintf(ffile, "<title>%s</title>\n", fix(item->item).c_str());
		if(item->objects.size() > 0) {
			fputs("<variablelist>\n", ffile);
			for(size_t i = 0; i < item->objects.size(); i++) {
				print_function((MathFunction*) item->objects[i]);
			}
			fputs("</variablelist>\n", ffile);
		}
		while(item && item->it == item->items.end()) {
			item = item->parent;
			if(item) {
				fprintf(ffile, "</sect%i>\n", level);
				level--;
			}

		}
		if(item) {
			item2 = &*item->it;
			++item->it;
			item = item2;
			item->it = item->items.begin();
			level++;
		}
	}
	if(!function_cats.objects.empty()) {
		fputs("<sect1 id=\"qalculate-definitions-functions-1-uncategorized\">\n", ffile);
		fprintf(ffile, "<title>%s</title>\n", _("Uncategorized"));
		fputs("<variablelist>\n", ffile);
		for(size_t i = 0; i < function_cats.objects.size(); i++) {
			print_function((MathFunction*) function_cats.objects[i]);
		}
		fputs("</variablelist>\n", ffile);
		fputs("</sect1>\n", ffile);
	}
	fputs("</appendix>\n", ffile);

	fclose(ffile);

	fputs("<appendix id=\"qalculate-definitions-variables\">\n", vfile);
	fputs("<title>Variable List</title>\n", vfile);
	variable_cats.it = variable_cats.items.begin();
	if(variable_cats.it != variable_cats.items.end()) {
		item = &*variable_cats.it;
		++variable_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	level = 1;
	while(item) {
		fprintf(vfile, "<sect%i id=\"qalculate-definitions-variables-%i-%s\">\n", level, level, fixcat(item->item).c_str());
		fprintf(vfile, "<title>%s</title>\n", fix(item->item).c_str());
		if(item->objects.size() > 0) {
			fprintf(vfile, "<table id=\"qalculate-TBL-variables-%s\" frame=\"topbot\" colsep=\"1\">\n", fixcat(item->item).c_str());
			fprintf(vfile, "<title>Variables: %s</title>\n", fix(item->item).c_str());
			fputs("<tgroup cols=\"3\" colsep=\"1\" rowsep=\"0\">\n", vfile);
			fputs("<colspec colname=\"COLSPEC0\"/>\n", vfile);
			fputs("<colspec colname=\"COLSPEC1\"/>\n", vfile);
			fputs("<colspec colname=\"COLSPEC2\"/>\n", vfile);
			fputs("<thead>\n", vfile);
			fputs("<row valign=\"top\">\n", vfile);
			fputs("<entry colname=\"COLSPEC0\"><para>Title</para></entry>\n", vfile);
			fputs("<entry colname=\"COLSPEC1\"><para>Names</para></entry>\n", vfile);
			fputs("<entry colname=\"COLSPEC2\"><para>Value</para></entry>\n", vfile);
			fputs("</row>\n", vfile);
			fputs("</thead>\n", vfile);
			fputs("<tbody>\n", vfile);
			for(size_t i = 0; i < item->objects.size(); i++) {
				print_variable((Variable*) item->objects[i]);
			}
			fputs("</tbody>\n", vfile);
			fputs("</tgroup>\n", vfile);
			fputs("</table>\n", vfile);
		}
		while(item && item->it == item->items.end()) {
			item = item->parent;
			if(item) {
				fprintf(vfile, "</sect%i>\n", level);
				level--;
			}

		}
		if(item) {
			item2 = &*item->it;
			++item->it;
			item = item2;
			item->it = item->items.begin();
			level++;
		}
	}
	if(!variable_cats.objects.empty()) {
		fputs("<sect1 id=\"qalculate-definitions-variables-1-uncategorized\">\n", vfile);
		fprintf(vfile, "<title>%s</title>\n", _("Uncategorized"));
		fprintf(vfile, "<table id=\"qalculate-TBL-variables-%s\" frame=\"topbot\" colsep=\"1\">\n", _("Uncategorized"));
		fprintf(vfile, "<title>Variables: %s</title>\n", _("Uncategorized"));
		fputs("<tgroup cols=\"3\" colsep=\"1\" rowsep=\"0\">\n", vfile);
		fputs("<colspec colname=\"COLSPEC0\"/>\n", vfile);
		fputs("<colspec colname=\"COLSPEC1\"/>\n", vfile);
		fputs("<colspec colname=\"COLSPEC2\"/>\n", vfile);
		fputs("<thead>\n", vfile);
		fputs("<row valign=\"top\">\n", vfile);
		fputs("<entry colname=\"COLSPEC0\"><para>Title</para></entry>\n", vfile);
		fputs("<entry colname=\"COLSPEC1\"><para>Names</para></entry>\n", vfile);
		fputs("<entry colname=\"COLSPEC2\"><para>Value</para></entry>\n", vfile);
		fputs("</row>\n", vfile);
		fputs("</thead>\n", vfile);
		fputs("<tbody>\n", vfile);
		for(size_t i = 0; i < variable_cats.objects.size(); i++) {
			print_variable((Variable*) function_cats.objects[i]);
		}
		fputs("</tbody>\n", vfile);
		fputs("</tgroup>\n", vfile);
		fputs("</table>\n", vfile);
		fputs("</sect1>\n", vfile);
	}
	fputs("</appendix>\n", vfile);

	fclose(vfile);

	fputs("<appendix id=\"qalculate-definitions-units\">\n", ufile);
	fputs("<title>Unit List</title>\n", ufile);
	unit_cats.it = unit_cats.items.begin();
	if(unit_cats.it != unit_cats.items.end()) {
		item = &*unit_cats.it;
		++unit_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	level = 1;
	while(item) {
		fprintf(ufile, "<sect%i id=\"qalculate-definitions-units-%i-%s\">\n", level, level, fixcat(item->item).c_str());
		fprintf(ufile, "<title>%s</title>\n", fix(item->item).c_str());
		if(item->objects.size() > 0) {
			fprintf(ufile, "<table id=\"qalculate-TBL-units-%s\" frame=\"topbot\" colsep=\"1\">\n", fixcat(item->item).c_str());
			fprintf(ufile, "<title>Units: %s</title>\n", fix(item->item).c_str());
			fputs("<tgroup cols=\"4\" colsep=\"1\" rowsep=\"0\">\n", ufile);
			fputs("<colspec colname=\"COLSPEC0\"/>\n", ufile);
			fputs("<colspec colname=\"COLSPEC1\"/>\n", ufile);
			fputs("<colspec colname=\"COLSPEC2\"/>\n", ufile);
			fputs("<colspec colname=\"COLSPEC3\"/>\n", ufile);
			fputs("<thead>\n", ufile);
			fputs("<row valign=\"top\">\n", ufile);
			fputs("<entry colname=\"COLSPEC0\"><para>Title</para></entry>\n", ufile);
			fputs("<entry colname=\"COLSPEC1\"><para>Names</para></entry>\n", ufile);
			fputs("<entry colname=\"COLSPEC2\"><para>Base Unit(s)</para></entry>\n", ufile);
			fputs("<entry colname=\"COLSPEC3\"><para>Relation</para></entry>\n", ufile);
			fputs("</row>\n", ufile);
			fputs("</thead>\n", ufile);
			fputs("<tbody>\n", ufile);
			for(size_t i = 0; i < item->objects.size(); i++) {
				print_unit((Unit*) item->objects[i]);
			}
			fputs("</tbody>\n", ufile);
			fputs("</tgroup>\n", ufile);
			fputs("</table>\n", ufile);
		}
		while(item && item->it == item->items.end()) {
			item = item->parent;
			if(item) {
				fprintf(ufile, "</sect%i>\n", level);
				level--;
			}

		}
		if(item) {
			item2 = &*item->it;
			++item->it;
			item = item2;
			item->it = item->items.begin();
			level++;
		}
	}
	if(!unit_cats.objects.empty()) {
		fputs("<sect1 id=\"qalculate-definitions-units-1-uncategorized\">\n", ufile);
		fprintf(ufile, "<title>%s</title>\n", _("Uncategorized"));
		fprintf(ufile, "<table id=\"qalculate-TBL-units-%s\" frame=\"topbot\" colsep=\"1\">\n", _("Uncategorized"));
		fprintf(ufile, "<title>Units: %s</title>\n", _("Uncategorized"));
		fputs("<tgroup cols=\"4\" colsep=\"1\" rowsep=\"0\">\n", ufile);
		fputs("<colspec colname=\"COLSPEC0\"/>\n", ufile);
		fputs("<colspec colname=\"COLSPEC1\"/>\n", ufile);
		fputs("<colspec colname=\"COLSPEC2\"/>\n", ufile);
		fputs("<colspec colname=\"COLSPEC3\"/>\n", ufile);
		fputs("<thead>\n", ufile);
		fputs("<row valign=\"top\">\n", ufile);
		fputs("<entry colname=\"COLSPEC0\"><para>Title</para></entry>\n", ufile);
		fputs("<entry colname=\"COLSPEC1\"><para>Names</para></entry>\n", ufile);
		fputs("<entry colname=\"COLSPEC2\"><para>Base Unit(s)</para></entry>\n", ufile);
		fputs("<entry colname=\"COLSPEC3\"><para>Relation</para></entry>\n", ufile);
		fputs("</row>\n", ufile);
		fputs("</thead>\n", ufile);
		fputs("<tbody>\n", ufile);
		for(size_t i = 0; i < unit_cats.objects.size(); i++) {
			print_unit((Unit*) function_cats.objects[i]);
		}
		fputs("</tbody>\n", ufile);
		fputs("</tgroup>\n", ufile);
		fputs("</table>\n", ufile);
		fputs("</sect1>\n", ufile);
	}
	fputs("</appendix>\n", ufile);

	fclose(ufile);

	return 0;

}



