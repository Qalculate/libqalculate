/*
    Qalculate (CLI)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna_k@fmgirl.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"
#include <libqalculate/qalculate.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <glib.h>
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

MathStructure *mstruct, *parsed_mstruct;
KnownVariable *vans[5];
string result_text, parsed_text;
bool load_global_defs, fetch_exchange_rates_at_startup, first_time, first_qalculate_run, save_mode_on_exit, save_defs_on_exit;
int auto_update_exchange_rates;
PrintOptions printops, saved_printops;
EvaluationOptions evalops, saved_evalops;
AssumptionType saved_assumption_type;
AssumptionSign saved_assumption_sign;
int saved_precision;
FILE *view_pipe_r, *view_pipe_w, *command_pipe_r, *command_pipe_w;
pthread_t view_thread, command_thread;
pthread_attr_t view_thread_attr, command_thread_attr;
bool command_aborted = false, command_thread_started;
bool b_busy = false;
string expression_str;
bool expression_executed = false;
bool rpn_mode;
bool use_readline = true;
bool interactive_mode;
bool ask_questions;

bool result_only;

fd_set in_set;
struct timeval timeout;

char buffer[1000];

void *view_proc(void *pipe);
void setResult(Prefix *prefix = NULL, bool update_parse = false, bool goto_input = true, size_t stack_index = 0, bool register_moved = false, bool noprint = false);
void *command_proc(void *pipe);
void execute_expression(bool goto_input = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0);
void execute_command(int command_type, bool show_result = true);
void load_preferences();
bool save_preferences(bool mode = false);
bool save_mode();
void set_saved_mode();
bool save_defs();
void result_display_updated();
void result_format_updated();
void result_action_executed();
void result_prefix_changed(Prefix *prefix = NULL);
void expression_format_updated();

FILE *cfile;

enum {
	COMMAND_FACTORIZE,
	COMMAND_SIMPLIFY
};

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z))

bool contains_unicode_char(const char *str) {
	for(int i = strlen(str) - 1; i >= 0; i--) {
		if(str[i] < 0) return true;
	}
	return false;
}

#define PUTS_UNICODE(x)				if(printops.use_unicode_signs || !contains_unicode_char(x)) {puts(x);} else {gchar *gstr = g_locale_from_utf8(x, -1, NULL, NULL, NULL); if(gstr) {puts(gstr); g_free(gstr);} else {puts(x);}}
#define FPUTS_UNICODE(x, y)			if(printops.use_unicode_signs || !contains_unicode_char(x)) {fputs(x, y);} else {gchar *gstr = g_locale_from_utf8(x, -1, NULL, NULL, NULL); if(gstr) {fputs(gstr, y); g_free(gstr);} else {fputs(x, y);}}

size_t unicode_length_check(const char *str) {
	/*if(printops.use_unicode_signs) return unicode_length(str);
	return strlen(str);*/
	return unicode_length(str);
}

int s2b(const string &str) {
	if(str.empty()) return -1;
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "yes", _("yes"))) return 1;
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "no", _("no"))) return 0;
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "true", _("true"))) return 1;
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "false", _("false"))) return 0;
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "on", _("on"))) return 1;
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "off", _("off"))) return 0;
	if(str.find_first_not_of(SPACES NUMBERS) != string::npos) return -1;
	int i = s2i(str);
	if(i > 0) return 1;
	return 0;
}

bool is_answer_variable(Variable *v) {
	return v == vans[0] || v == vans[1] || v == vans[2] || v == vans[3] || v == vans[4];
}

bool equalsIgnoreCaseFirst(const string &str1, const char *str2) {
	if(str1.length() < 1 || strlen(str2) < 1) return false;
	if(str1[0] < 0 && str1.length() > 1) {
		if(str2[0] >= 0) return false;
		size_t i2 = 1;
		while(i2 < str1.length() && str1[i2] < 0) {
			if(i2 >= strlen(str2) || str2[i2] >= 0) return false;
			i2++;
		}
		if(i2 != str1.length()) return false;
		gchar *gstr1 = g_utf8_strdown(str1.c_str(), i2);
		gchar *gstr2 = g_utf8_strdown(str2, i2);
		if(strcmp(gstr1, gstr2) != 0) return false;
		g_free(gstr1);
		g_free(gstr2);
	} else if(str1.length() != 1 || (str1[0] != str2[0] && !((str1[0] >= 'a' && str1[0] <= 'z') && str1[0] - 32 == str2[0]) && !((str1[0] <= 'Z' && str1[0] >= 'A') && str1[0] + 32 == str2[0]))) {
		return false;
	}
	return true;
}

bool ask_question(const char *question, bool default_answer = false) {
	FPUTS_UNICODE(question, stdout);
	while(true) {
#ifdef HAVE_LIBREADLINE
		char *rlbuffer = readline(" ");
		string str = rlbuffer;
		free(rlbuffer);
#else
		fputs(" ", stdout);
		fgets(buffer, 1000, stdin);
		string str = buffer;
#endif		
		remove_blank_ends(str);
		if(equalsIgnoreCaseFirst(str, "yes") || equalsIgnoreCaseFirst(str, _("yes")) || EQUALS_IGNORECASE_AND_LOCAL(str, "yes", _("yes"))) {
			return true;
		} else if(equalsIgnoreCaseFirst(str, "no") || equalsIgnoreCaseFirst(str, _("no")) || EQUALS_IGNORECASE_AND_LOCAL(str, "no", _("no"))) {	
			return false;
		} else if(str.empty()) {
			return default_answer;
		} else {
			FPUTS_UNICODE(_("Please answer yes or no"), stdout);
			FPUTS_UNICODE(":", stdout);
		}
	}
}

void set_assumption(const string &str, bool first_of_two = false) {
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "unknown", _("unknown"))) {
		if(first_of_two) {
			CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_UNKNOWN);
		} else {
			CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NONE);
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "none", _("none"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NONE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-matrix", _("non-matrix"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NONMATRIX);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "complex", _("complex"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_COMPLEX);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "real", _("real"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_REAL);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "number", _("number"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NUMBER);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rational", _("rational"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_RATIONAL);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "integer", _("integer"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_INTEGER);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-zero", _("non-zero"))) {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONZERO);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "positive", _("positive"))) {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_POSITIVE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-negative", _("non-negative"))) {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "negative", _("negatve"))) {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NEGATIVE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-positive", _("non-positive"))) {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
	} else {
		PUTS_UNICODE(_("Unrecognized assumption."));
	}
}

vector<const string*> matches;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_LIBREADLINE

char *qalc_completion(const char *text, int index) {
	if(index == 0) {
		if(strlen(text) < 1) return NULL;
		matches.clear();
		const string *str;
		bool b_match;
		size_t l = strlen(text);
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			if(CALCULATOR->functions[i]->isActive()) {
				str = &CALCULATOR->functions[i]->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs).name;
				if(l <= str->length()) {
					b_match = true;
					for(size_t i2 = 0; i2 < l; i2++) {
						if((*str)[i2] != text[i2]) {
							b_match = false;
							break;
						}
					}
					if(b_match) {
						matches.push_back(str);
					}
				}
			}
		}
		for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			if(CALCULATOR->variables[i]->isActive()) {
				str = &CALCULATOR->variables[i]->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs).name;
				if(l <= str->length()) {
					b_match = true;
					for(size_t i2 = 0; i2 < l; i2++) {
						if((*str)[i2] != text[i2]) {
							b_match = false;
							break;
						}
					}
					if(b_match) {
						matches.push_back(str);
					}
				}
			}
		}
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i]->isActive() && CALCULATOR->units[i]->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				str = &CALCULATOR->units[i]->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs).name;
				if(l <= str->length()) {
					b_match = true;
					for(size_t i2 = 0; i2 < l; i2++) {
						if((*str)[i2] != text[i2]) {
							b_match = false;
							break;
						}
					}
					if(b_match) {
						matches.push_back(str);
					}
				}
			}
		}
	}
	if(index >= 0 && index < (int) matches.size()) {
		char *cstr = (char*) malloc(sizeof(char) *matches[index]->length() + 1);
		strcpy(cstr, matches[index]->c_str());
		return cstr;
	}
	return NULL;
}

#endif

#ifdef __cplusplus
}
#endif

int enable_unicode = -1;

void handle_exit() {
	if(enable_unicode >= 0) {
		printops.use_unicode_signs = enable_unicode;
	} 
	if(!cfile) {
		if(save_mode_on_exit) {
			save_mode();
		} else {
			save_preferences();
		}
		if(save_defs_on_exit) {
			save_defs();
		}
	}
	pthread_cancel(view_thread);
	CALCULATOR->terminateThreads();
}

#ifdef HAVE_LIBREADLINE
int rlcom_tab(int, int) {
	rl_complete_internal('!');
	return 0;	
}
#endif

int countRows(const char *str, int cols) {
	int l = unicode_length_check(str);
	if(l == 0) return 1;
	int r = 1, c = 0;
	for(int i = 0; i < l; i++) {
		if(str[i] == '\n') {
			r++;
			c = 0;
		} else {
			if(c == cols) {
				r++;
				c = 0;
			}
			c++;
		}
	}
	return r;
}

bool check_exchange_rates() {
	if(!CALCULATOR->exchangeRatesUsed()) return false;
	if(CALCULATOR->checkExchangeRatesDate(auto_update_exchange_rates > 0 ? auto_update_exchange_rates : 7, false, auto_update_exchange_rates == 0 || (auto_update_exchange_rates < 0 && !ask_questions))) return false;
	if(auto_update_exchange_rates == 0 || (auto_update_exchange_rates < 0 && !ask_questions)) return false;
	bool b = false;
	if(auto_update_exchange_rates < 0) {
		string ask_str = _("It has been more than one week since the exchange rates last were updated.");
		ask_str += "\n";
		ask_str += _("Do you wish to update the exchange rates now?");
		b = ask_question(ask_str.c_str());
	}
	if(b || auto_update_exchange_rates > 0) {
		CALCULATOR->fetchExchangeRates(15);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}


#ifdef HAVE_LIBREADLINE
#	define CHECK_IF_SCREEN_FILLED if(!cfile) {rcount++; if(rcount + 3 >= rows) {FPUTS_UNICODE(_("\nPress Enter to continue."), stdout); fflush(stdout); rl_read_key(); puts(""); rcount = 1;}}
#	define CHECK_IF_SCREEN_FILLED_PUTS(x) if(!cfile) {rcount += countRows(x, cols); if(rcount + 2 >= rows) {FPUTS_UNICODE(_("\nPress Enter to continue."), stdout); fflush(stdout); rl_read_key(); puts(""); rcount = 1;}} PUTS_UNICODE(x);
#	define INIT_SCREEN_CHECK int rows, cols, rcount = 0; if(!cfile) rl_get_screen_size(&rows, &cols);
#else
#	define CHECK_IF_SCREEN_FILLED
#	define CHECK_IF_SCREEN_FILLED_PUTS(x) PUTS_UNICODE(x);
#	define INIT_SCREEN_CHECK
#endif

#define SET_BOOL(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {x = v;}}
#define SET_BOOL_D(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {x = v; result_display_updated();}}
#define SET_BOOL_E(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {x = v; expression_format_updated();}}

void set_option(string str) {
	remove_blank_ends(str);
	string svalue, svar;
	size_t index;
	if((index = str.find_first_of(SPACES)) != string::npos) {
		svalue = str.substr(index + 1, str.length() - (index + 1));
		remove_blank_ends(svalue);
	}
	svar = str.substr(0, index);
	remove_blank_ends(svar);
	gsub("_", " ", svar);
	set_option_place:
	if(EQUALS_IGNORECASE_AND_LOCAL(svar, "base", _("base")) || EQUALS_IGNORECASE_AND_LOCAL(svar, "input base", _("input base")) || EQUALS_IGNORECASE_AND_LOCAL(svar, "output base", _("output base"))) {
		int v = 0;
		bool b_in = EQUALS_IGNORECASE_AND_LOCAL(svar, "input base", _("input base"));
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "roman", _("roman"))) v = BASE_ROMAN_NUMERALS;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "time", _("time"))) {if(b_in) v = 0; else v = BASE_TIME;}
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "hex", _("hex")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "hexadecimal", _("hexadecimal"))) v = BASE_HEXADECIMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "bin", _("bin")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "binary", _("binary"))) v = BASE_BINARY;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "oct", _("oct")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "octal", _("octal"))) v = BASE_HEXADECIMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "dec", _("dec")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "decimal", _("decimal"))) v = BASE_DECIMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "sex", _("sex")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "sexagesimal", _("sexagesimal"))) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL;}
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
			if((v < 2 || v > 36) && (b_in || v != 60)) {
				v = 0;
			}
		}
		if(v == 0) {
			if((index = svalue.find_first_of(SPACES)) != string::npos) {
				str = svalue;
				svalue = str.substr(index + 1, str.length() - (index + 1));
				remove_blank_ends(svalue);
				svar += " ";
				str = str.substr(0, index);
				remove_blank_ends(str);
				svar += str;
				gsub("_", " ", svar);
				if(EQUALS_IGNORECASE_AND_LOCAL(svar, "base display", _("base display"))) {
					goto set_option_place;
				}
			}
			
			PUTS_UNICODE(_("Illegal base."));
		} else if(b_in) {
			evalops.parse_options.base = v;
			expression_format_updated();
		} else {
			printops.base = v;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "assumptions", _("assumptions"))) {
		size_t i = svalue.find_first_of(SPACES);
		if(i != string::npos) {			
			set_assumption(svalue.substr(0, i), true);
			set_assumption(svalue.substr(i + 1, svalue.length() - (i + 1)), false);
		} else {
			set_assumption(svalue, false);
		}
		expression_format_updated();
	}
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "all prefixes", _("all prefixes"))) SET_BOOL_D(printops.use_all_prefixes)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "complex numbers", _("complex numbers"))) SET_BOOL_E(evalops.allow_complex)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "excessive parentheses", _("excessive parentheses"))) SET_BOOL_D(printops.excessive_parenthesis)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "functions", _("functions"))) SET_BOOL_E(evalops.parse_options.functions_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "infinite numbers", _("infinite numbers"))) SET_BOOL_E(evalops.allow_infinite)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "show negative exponents", _("show negative exponents"))) SET_BOOL_D(printops.negative_exponents)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "assume nonzero denominators", _("assume nonzero denominators"))) SET_BOOL_E(evalops.assume_denominators_nonzero)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "warn nonzero denominators", _("warn nonzero denominators"))) SET_BOOL_E(evalops.warn_about_denominators_assumed_nonzero)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "prefixes", _("prefixes"))) SET_BOOL_D(printops.use_unit_prefixes)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "denominator prefixes", _("denominator prefixes"))) SET_BOOL_D(printops.use_denominator_prefix)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "place units separately", _("place units separately"))) SET_BOOL_D(printops.place_units_separately)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "calculate variables", _("calculate variables"))) SET_BOOL_E(evalops.calculate_variables)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "calculate functions", _("calculate functions"))) SET_BOOL_E(evalops.calculate_functions)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "sync units", _("sync units"))) SET_BOOL_E(evalops.sync_units)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "round to even", _("round to even"))) SET_BOOL_D(printops.round_halfway_to_even)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "rpn", _("rpn"))) {SET_BOOL(rpn_mode) if(!rpn_mode) CALCULATOR->clearRPNStack();}
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "rpn syntax", _("rpn syntax"))) SET_BOOL_E(evalops.parse_options.rpn)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "short multiplication", _("short multiplication"))) SET_BOOL_D(printops.short_multiplication)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "lowercase e", _("lowercase e"))) SET_BOOL_D(printops.lower_case_e)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "lowercase numbers", _("lowercase numbers"))) SET_BOOL_D(printops.lower_case_numbers)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "base display", _("base display"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = BASE_DISPLAY_NONE;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "normal", _("normal"))) v = BASE_DISPLAY_NORMAL;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "alternative", _("alternative"))) v = BASE_DISPLAY_ALTERNATIVE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.base_display = (BaseDisplay) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "spell out logical", _("spell out logical"))) SET_BOOL_D(printops.spell_out_logical_operators)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "dot as separator", _("dot as separator"))) SET_BOOL_E(evalops.parse_options.dot_as_separator)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "limit implicit multiplication", _("limit implicit multiplication"))) {
		int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {printops.limit_implicit_multiplication = v; evalops.parse_options.limit_implicit_multiplication = v; expression_format_updated();}		
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "spacious", _("spacious"))) SET_BOOL_D(printops.spacious)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "unicode", _("unicode"))) {
		int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {printops.use_unicode_signs = v; result_display_updated();}
		enable_unicode = -1;
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "units", _("units"))) SET_BOOL_E(evalops.parse_options.units_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "unknowns", _("unknowns"))) SET_BOOL_E(evalops.parse_options.unknowns_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "variables", _("variables"))) SET_BOOL_E(evalops.parse_options.variables_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "abbreviations", _("abbreviations"))) SET_BOOL_D(printops.abbreviate_names)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "show ending zeroes", _("show ending zeroes"))) SET_BOOL_D(printops.show_ending_zeroes)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "indicate infinite series", _("indicate infinite series"))) SET_BOOL_D(printops.indicate_infinite_series)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "angle unit", _("angle unit"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "rad", _("rad")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "radians", _("radians"))) v = ANGLE_UNIT_RADIANS;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "deg", _("deg")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "degrees", _("degrees"))) v = ANGLE_UNIT_DEGREES;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "gra", _("gra")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "gradians", _("gradians"))) v = ANGLE_UNIT_GRADIANS;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = ANGLE_UNIT_NONE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.parse_options.angle_unit = (AngleUnit) v;
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "parsing mode", _("parsing mode"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "adaptive", _("adaptive"))) v = PARSING_MODE_ADAPTIVE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "implicit first", _("implicit first"))) v = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "conventional", _("conventional"))) v = PARSING_MODE_CONVENTIONAL;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.parse_options.parsing_mode = (ParsingMode) v;
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "update exchange rates", _("update exchange rates"))) {
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "never", _("never"))) {
			auto_update_exchange_rates = 0;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "ask", _("ask"))) {
			auto_update_exchange_rates = -1;
		} else {
			int v = s2i(svalue);
			if(v < 0) auto_update_exchange_rates = -1;
			else auto_update_exchange_rates = v;
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "multiplication sign", _("multiplication sign"))) {
		int v = -1;
		if(svalue == SIGN_MULTIDOT || svalue == ".") v = MULTIPLICATION_SIGN_DOT;
		else if(svalue == SIGN_MULTIPLICATION || svalue == "x") v = MULTIPLICATION_SIGN_X;
		else if(svalue == "*") v = MULTIPLICATION_SIGN_ASTERISK;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.multiplication_sign = (MultiplicationSign) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "division sign", _("division sign"))) {
		int v = -1;
		if(svalue == SIGN_DIVISION_SLASH) v = DIVISION_SIGN_DIVISION_SLASH;
		else if(svalue == SIGN_DIVISION) v = DIVISION_SIGN_DIVISION;
		else if(svalue == "/") v = DIVISION_SIGN_SLASH;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.division_sign = (DivisionSign) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "approximation", _("approximation"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "exact", _("exact"))) v = APPROXIMATION_EXACT;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "try exact", _("try exact"))) v = APPROXIMATION_TRY_EXACT;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "approximate", _("approximate"))) v = APPROXIMATION_APPROXIMATE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.approximation = (ApproximationMode) v;
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "autoconversion", _("autoconversion"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = POST_CONVERSION_NONE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "best", _("best"))) v = POST_CONVERSION_BEST;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "optimal", _("optimal"))) v = POST_CONVERSION_BEST;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "base", _("base"))) v = POST_CONVERSION_BASE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.auto_post_conversion = (AutoPostConversion) v;
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "algebra mode", _("algebra mode"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = STRUCTURING_NONE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "simplify", _("simplify"))) v = STRUCTURING_SIMPLIFY;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "factorize", _("factorize"))) v = STRUCTURING_FACTORIZE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.structuring = (StructuringMode) v;
			printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "exact", _("exact"))) {
		int v = s2b(svalue); 
		if(v < 0) {
			PUTS_UNICODE(_("Illegal value")); 
		} else if(v > 0) {
			evalops.approximation = APPROXIMATION_EXACT; 
			expression_format_updated();
		} else {
			evalops.approximation = APPROXIMATION_TRY_EXACT; 
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "save mode", _("save mode"))) {
		int v = s2b(svalue); 
		if(v < 0) {
			PUTS_UNICODE(_("Illegal value")); 
		} else if(v > 0) {
			save_mode_on_exit = true;
		} else {
			save_mode_on_exit = false;
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "save definitions", _("save definitions"))) {
		int v = s2b(svalue); 
		if(v < 0) {
			PUTS_UNICODE(_("Illegal value")); 
		} else if(v > 0) {
			save_defs_on_exit = true;
		} else {
			save_defs_on_exit = false;
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "exp mode", _("exp mode"))) {
		int v = -2;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = EXP_NONE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "auto", _("auto"))) v = EXP_PRECISION;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "pure", _("pure"))) v = EXP_PURE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "scientific", _("scientific"))) v = EXP_SCIENTIFIC;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "engineering", _("engineering"))) v = EXP_BASE_3;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
			if(v < 0) v = -2;
		}
		if(v <= -2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.min_exp = v;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "precision", _("precision"))) {
		int v = 0;
		if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 1) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			CALCULATOR->setPrecision(v);
			expression_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "max decimals", _("max decimals"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = -1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 0) {
			printops.use_max_decimals = false;
			result_format_updated();
		} else {
			printops.max_decimals = v;
			printops.use_max_decimals = true;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "min decimals", _("min decimals"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = -1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 0) {
			printops.min_decimals = 0;
			printops.use_min_decimals = false;
			result_format_updated();
		} else {
			printops.min_decimals = v;
			printops.use_min_decimals = true;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "fractions", _("fractions"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = FRACTION_DECIMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "exact", _("exact"))) v = FRACTION_DECIMAL_EXACT;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "on", _("on"))) v = FRACTION_FRACTIONAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "combined", _("combined"))) v = FRACTION_COMBINED;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 3) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.number_fraction_format = (NumberFractionFormat) v;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "read precision", _("read precision"))) {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = DONT_READ_PRECISION;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "always", _("always"))) v = ALWAYS_READ_PRECISION;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "when decimals", _("when decimals")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "on", _("on"))) v = READ_PRECISION_WHEN_DECIMALS;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.parse_options.read_precision = (ReadPrecisionMode) v;
			expression_format_updated();
		}
	} else {
		if(index != string::npos) {
			if((index = svalue.find_first_of(SPACES)) != string::npos) {
				str = svalue;
				svalue = str.substr(index + 1, str.length() - (index + 1));
				remove_blank_ends(svalue);
				svar += " ";
				str = str.substr(0, index);
				remove_blank_ends(str);
				svar += str;
				gsub("_", " ", svar);
				goto set_option_place;
			}
		}
		PUTS_UNICODE(_("Unrecognized option."));
	}
}

int main(int argc, char *argv[]) {

	string calc_arg;
	vector<string> set_option_strings;
	bool calc_arg_begun = false;
	string command_file;
	cfile = NULL;
	interactive_mode = false;
	result_only = false;
	bool load_units = true, load_functions = true, load_variables = true, load_currencies = true, load_datasets = true;
	load_global_defs = true;
	printops.use_unicode_signs = false;
	
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	setlocale(LC_ALL, "");
	
	for(int i = 1; i < argc; i++) {
		if(calc_arg_begun) {
			calc_arg += " ";
		}
		if(!calc_arg_begun && (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0)) {
			PUTS_UNICODE(_("usage: qalc [options] [expression]"));
			printf("\n");
			PUTS_UNICODE(_("where options are:"));
			fputs("\n\t-/+u8\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("turn on/off unicode support"));
			fputs("\n\t-set", stdout); fputs(" \"", stdout); FPUTS_UNICODE(_("OPTION"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("VALUE"), stdout); fputs("\"\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("as set command in interactive program session (ex. -set \"base 16\")"));
			fputs("\n\t-f, -file", stdout); fputs(" ", stdout); FPUTS_UNICODE(_("FILE"), stdout); fputs("\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("executes commands from a file first"));
			fputs("\n\t-t, -terse\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("reduces output to just the result of the input expression"));
			fputs("\n\t-i, -interactive\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("start in interactive mode"));
			fputs("\n\t-n, -nodefs\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("do not load any functions, units, or variables from file"));
			fputs("\n\t-nocurrencies\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("do not load any global currencies from file"));
			fputs("\n\t-nodatasets\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("do not load any global data sets from file"));
			fputs("\n\t-nofunctions\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("do not load any global functions from file"));
			fputs("\n\t-nounits\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("do not load any global units from file"));
			fputs("\n\t-novariables\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("do not load any global variables from file"));
			puts("");
			PUTS_UNICODE(_("The program will start in interactive mode if no expression and no file is specified (or interactive mode is explicitly selected)."));
			puts("");
			return 0;
		} else if(!calc_arg_begun && strcmp(argv[i], "-u8") == 0) {
			enable_unicode = 1;
		} else if(!calc_arg_begun && strcmp(argv[i], "+u8") == 0) {
			enable_unicode = 0;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-terse") == 0 || strcmp(argv[i], "--terse") == 0 || strcmp(argv[i], "-t") == 0)) {
			result_only = true;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-interactive") == 0 || strcmp(argv[i], "--interactive") == 0 || strcmp(argv[i], "-i") == 0)) {
			interactive_mode = true;
		} else if(!calc_arg_begun && strcmp(argv[i], "-nounits") == 0) {
			load_units = false;
		} else if(!calc_arg_begun && strcmp(argv[i], "-nocurrencies") == 0) {
			load_currencies = false;
		} else if(!calc_arg_begun && strcmp(argv[i], "-nofunctions") == 0) {
			load_functions = false;
		} else if(!calc_arg_begun && strcmp(argv[i], "-novariables") == 0) {
			load_variables = false;
		} else if(!calc_arg_begun && strcmp(argv[i], "-nodatasets") == 0) {
			load_datasets = false;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-nodefs") == 0 || strcmp(argv[i], "-n") == 0)) {
			load_global_defs = false;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-set") == 0 || strcmp(argv[i], "--set") == 0)) {
			i++;
			if(i < argc) {
				set_option_strings.push_back(argv[i]);
			} else {
				puts(_("No option and value specified for set command."));
			}
		} else if(!calc_arg_begun && (strcmp(argv[i], "-file") == 0 || strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0)) {
			i++;
			if(i < argc) {
				command_file = argv[i];
				remove_blank_ends(command_file);
			} else {
				puts(_("No file specified."));
			}
		} else {
			if(strlen(argv[i]) >= 2 && ((argv[i][0] == '\"' && argv[i][strlen(argv[i] - 1)] == '\"') || (argv[i][0] == '\'' && argv[i][strlen(argv[i] - 1)] == '\''))) {
				calc_arg += argv[i] + 1;
				calc_arg.erase(calc_arg.length() - 1);
			} else {
				calc_arg += argv[i];
			}
			calc_arg_begun = true;
		}
	}

	b_busy = false;

	//create the almighty Calculator object
	new Calculator();

	//load application specific preferences
	load_preferences();

	for(size_t i = 0; i < set_option_strings.size(); i++) {
		set_option(set_option_strings[i]);
	}

	if(enable_unicode >= 0) {
		if(printops.use_unicode_signs == enable_unicode) {
			enable_unicode = -1;
		} else {
			printops.use_unicode_signs = enable_unicode;
		}
	} 

	mstruct = new MathStructure();
	parsed_mstruct = new MathStructure();

	bool canfetch = CALCULATOR->canFetch();

	string str;
#ifdef HAVE_LIBREADLINE	
	static char* rlbuffer;
#endif

	ask_questions = (command_file.empty() || interactive_mode) && !result_only;
	
	//exchange rates
	if(load_global_defs && load_currencies) {
		if(first_qalculate_run && canfetch && ask_questions) {
			if(ask_question(_("You need the download exchange rates to be able to convert between different currencies.\nYou can later get current exchange rates with the \"exchange rates\" command.\nDo you want to fetch exchange rates now from the Internet (default: yes)?"), true)) {
				CALCULATOR->fetchExchangeRates(5);
			}
		} else if(fetch_exchange_rates_at_startup && canfetch) {
			CALCULATOR->fetchExchangeRates(5);
		}
		CALCULATOR->setExchangeRatesWarningEnabled(!interactive_mode && (!command_file.empty() || (result_only && !calc_arg.empty())));
		CALCULATOR->loadExchangeRates();
	}

	string ans_str = _("ans");
	vans[0] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str, m_undefined, _("Last Answer"), false));
	vans[0]->addName(_("answer"));
	vans[0]->addName(ans_str + "1");
	vans[1] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "2", m_undefined, _("Answer 2"), false));
	vans[2] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "3", m_undefined, _("Answer 3"), false));
	vans[3] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "4", m_undefined, _("Answer 4"), false));
	vans[4] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(_("Temporary"), ans_str + "5", m_undefined, _("Answer 5"), false));

	//load global definitions
	if(load_global_defs) {
		bool b = true;
		if(load_units && !CALCULATOR->loadGlobalPrefixes()) b = false;
		if(load_units && !CALCULATOR->loadGlobalUnits()) b = false;
		else if(load_currencies && !CALCULATOR->loadGlobalCurrencies()) b = false;
		if(load_functions && !CALCULATOR->loadGlobalFunctions()) b = false;
		if(load_datasets && !CALCULATOR->loadGlobalDataSets()) b = false;
		if(load_variables && !CALCULATOR->loadGlobalVariables()) b = false;
		if(!b) {PUTS_UNICODE(_("Failed to load global definitions!"))};
	}

	//load local definitions
	if(load_global_defs) CALCULATOR->loadLocalDefinitions();

	//reset
	result_text = "0";
	parsed_text = "0";
	
	int pipe_wr[] = {0, 0};
	pipe(pipe_wr);
	view_pipe_r = fdopen(pipe_wr[0], "r");
	view_pipe_w = fdopen(pipe_wr[1], "w");
	pthread_attr_init(&view_thread_attr);
	pthread_create(&view_thread, &view_thread_attr, view_proc, view_pipe_r);
	
	int pipe_wr2[] = {0, 0};
	pipe(pipe_wr2);
	command_pipe_r = fdopen(pipe_wr2[0], "r");
	command_pipe_w = fdopen(pipe_wr2[1], "w");
	pthread_attr_init(&command_thread_attr);
	command_thread_started = false;
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	if(!command_file.empty()) {
		if(command_file == "-") {
			cfile = stdin;
		} else {
			cfile = fopen(command_file.c_str(), "r");
			if(!cfile) {
				printf(_("Could not open \"%s\".\n"), command_file.c_str());
				if(!interactive_mode) {
					pthread_cancel(view_thread);
					CALCULATOR->terminateThreads();
					return 0;
				}
			}
		}
	}

	if(!cfile && !calc_arg.empty()) {
		if(!printops.use_unicode_signs && contains_unicode_char(calc_arg.c_str())) {
			gchar *gstr = g_locale_to_utf8(calc_arg.c_str(), -1, NULL, NULL, NULL);
			if(gstr) {
				expression_str = gstr;
				g_free(gstr);
			} else {
				expression_str = calc_arg;
			}
		} else {
			expression_str = calc_arg;
		}
		size_t index = expression_str.find_first_of(ID_WRAPS);
		if(index != string::npos) {
			printf(_("Illegal character, \'%c\', in expression."), expression_str[index]);
			puts("");
		} else {
			use_readline = false;
			execute_expression(interactive_mode);
		}
		if(!interactive_mode) {
			pthread_cancel(view_thread);
			CALCULATOR->terminateThreads();
			return 0;
		}
		use_readline = true;
	} else if(!cfile) {
		ask_questions = !result_only;
		interactive_mode = true;
	}
	
#ifdef HAVE_LIBREADLINE
	rl_catch_signals = 1;
	rl_catch_sigwinch = rl_readline_version >= 0x0500;
	rl_readline_name = "qalc";
	rl_basic_word_break_characters = NOT_IN_NAMES NUMBERS;
	rl_completion_entry_function = qalc_completion;
	rl_bind_key('\t', rlcom_tab);
#endif
	
	string scom;
	size_t slen, ispace;
	
	while(true) {
		if(cfile) {
			if(!fgets(buffer, 1000, cfile)) {
				if(cfile != stdin) {
					fclose(cfile);
				}
				cfile = NULL;
				if(!calc_arg.empty()) {
					if(!printops.use_unicode_signs && contains_unicode_char(calc_arg.c_str())) {
						gchar *gstr = g_locale_to_utf8(calc_arg.c_str(), -1, NULL, NULL, NULL);
						if(gstr) {
							expression_str = gstr;
							g_free(gstr);
						} else {
							expression_str = calc_arg;
						}
					} else {
						expression_str = calc_arg;
					}
					size_t index = expression_str.find_first_of(ID_WRAPS);
					if(index != string::npos) {
						printf(_("Illegal character, \'%c\', in expression."), expression_str[index]);
						puts("");
					} else {
						execute_expression(interactive_mode);
					}
				}
				if(!interactive_mode) break;
				continue;
			}
			if(!printops.use_unicode_signs && contains_unicode_char(buffer)) {
				gchar *gstr = g_locale_to_utf8(buffer, -1, NULL, NULL, NULL);
				if(gstr) {
					str = gstr;
					g_free(gstr);
				} else { 
					str = buffer;
				}
			} else {
				str = buffer;
			}
		} else {
#ifdef HAVE_LIBREADLINE
			rlbuffer = readline("> ");
			if(rlbuffer == NULL) break;
			if(!printops.use_unicode_signs && contains_unicode_char(rlbuffer)) {
				gchar *gstr = g_locale_to_utf8(rlbuffer, -1, NULL, NULL, NULL);
				if(gstr) {
					str = gstr;
					g_free(gstr);
				} else {
					str = rlbuffer;
				}
			} else {
				str = rlbuffer;
			}
#else
			fputs("> ", stdout);
			fgets(buffer, 1000, stdin);
			if(!printops.use_unicode_signs && contains_unicode_char(buffer)) {
				gchar *gstr = g_locale_to_utf8(buffer, -1, NULL, NULL, NULL);
				if(gstr) {
					str = gstr;
					g_free(gstr);
				} else { 
					str = buffer;
				}
			} else {
				str = buffer;
			}
#endif	
		}	
		slen = str.length();
		remove_blank_ends(str);
		ispace = str.find_first_of(SPACES);
		if(ispace == string::npos) {
			scom = "";
		} else {
			scom = str.substr(0, ispace);
		}
		//The qalc command "set" as in "set precision 10". The original text string for commands is kept in addition to the translation.
		if(EQUALS_IGNORECASE_AND_LOCAL(scom, "set", _("set"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			set_option(str);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "save", _("save")) || EQUALS_IGNORECASE_AND_LOCAL(scom, "store", _("store"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			if(EQUALS_IGNORECASE_AND_LOCAL(str, "mode", _("mode"))) {
				if(save_mode()) {
					PUTS_UNICODE(_("mode saved"));
				}
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "definitions", _("definitions"))) {
				if(save_defs()) {
					PUTS_UNICODE(_("definitions saved"));
				}
			} else {
				string name = str, cat, title;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				bool catset = false;
				if(str.empty()) {
					cat = _("Temporary");
				} else {
					if(str[0] == '\"') {
						size_t i = str.find('\"', 1);
						if(i != string::npos) {
							cat = str.substr(1, i - 1);
							title = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(title);
						}
					} else {
						size_t i = str.find_first_of(SPACES, 1);
						if(i != string::npos) {
							cat = str.substr(0, i);
							title = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(title);
						}
					}
					catset = true;
				}
				bool b = true;
				if(!CALCULATOR->variableNameIsValid(name)) {
					name = CALCULATOR->convertToValidVariableName(name);
					size_t l = name.length() + strlen(_("Illegal name. Save as %s instead (default: no)?"));
					char *cstr = (char*) malloc(sizeof(char) * (l + 1));
					snprintf(cstr, l, _("Illegal name. Save as %s instead (default: no)?"), name.c_str());
					if(!ask_question(cstr)) {
						b = false;
					}
					free(cstr);
				}
				if(b && CALCULATOR->variableNameTaken(name)) {
					if(!ask_question(_("An unit or variable with the same name already exists.\nDo you want to overwrite it (default: no)?"))) {
						b = false;
					}
				}
				if(b) {
					Variable *v = CALCULATOR->getActiveVariable(name);
					if(v && v->isLocal() && v->isKnown()) {
						if(catset) v->setCategory(cat);
						if(!title.empty()) v->setTitle(title);
						((KnownVariable*) v)->set(*mstruct);
						if(v->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							v->setName(ename, 1);
						} else {
							v->setName(name, 1);
						}
					} else {
						CALCULATOR->addVariable(new KnownVariable(cat, name, *mstruct, title));
					}
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "assume", _("assume"))) {
			string str2 = "assumptions ";
			set_option(str2 + str.substr(ispace + 1, slen - (ispace + 1)));
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "base", _("base"))) {
			set_option(str);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "rpn", _("rpn"))) {
			set_option(str);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "exrates", _("exrates"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			CALCULATOR->fetchExchangeRates(15, str);
			CALCULATOR->loadExchangeRates();
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "exrates", _("exrates"))) {
			CALCULATOR->fetchExchangeRates(15);
			CALCULATOR->loadExchangeRates();
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "stack", _("stack"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else {
				puts("");
				MathStructure m;
				for(size_t i = 1; i <= CALCULATOR->RPNStackSize(); i++) {
					m = *CALCULATOR->getRPNRegister(i);
					m.format(printops);
					printf("  %i:\t%s\n", (int) i, m.print(printops).c_str());
				}
				puts("");
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "clear stack", _("clear stack"))) {
			CALCULATOR->clearRPNStack();
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "exact", _("exact"))) {
			if(evalops.approximation != APPROXIMATION_EXACT) {
				evalops.approximation = APPROXIMATION_EXACT;
				expression_format_updated();
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "approximate", _("approximate"))) {
			if(evalops.approximation != APPROXIMATION_TRY_EXACT) {
				evalops.approximation = APPROXIMATION_TRY_EXACT;
				expression_format_updated();
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "convert", _("convert")) || EQUALS_IGNORECASE_AND_LOCAL(scom, "to", _("to"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			if(equalsIgnoreCase(str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(str, "hexadecimal", _("hexadecimal"))) {
				int save_base = printops.base;
				printops.base = BASE_HEXADECIMAL;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(str, "binary", _("binary"))) {
				int save_base = printops.base;
				printops.base = BASE_BINARY;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(str, "octal", _("octal"))) {
				int save_base = printops.base;
				printops.base = BASE_OCTAL;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "bases", _("bases"))) {
				int save_base = printops.base;
				string save_result_text = result_text;
				string base_str = "\n  ";
				base_str += result_text;
				if(save_base != BASE_BINARY) {
					base_str += " = ";
					printops.base = BASE_BINARY;
					setResult(NULL, false, true, 0, false, true);
					base_str += result_text;
				}
				if(save_base != BASE_OCTAL) {
					base_str += " = ";
					printops.base = BASE_OCTAL;
					setResult(NULL, false, true, 0, false, true);
					base_str += result_text;
				}
				if(save_base != BASE_DECIMAL) {
					base_str += " = ";
					printops.base = BASE_DECIMAL;
					setResult(NULL, false, true, 0, false, true);
					base_str += result_text;					
				}
				if(save_base != BASE_HEXADECIMAL) {
					base_str += " = ";
					printops.base = BASE_HEXADECIMAL;
					setResult(NULL, false, true, 0, false, true);
					base_str += result_text;
				}
				base_str += "\n";
				PUTS_UNICODE(base_str.c_str());
				printops.base = save_base;
				result_text = save_result_text;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "fraction", _("fraction"))) {
				NumberFractionFormat save_format = printops.number_fraction_format;
				if(mstruct->isNumber()) printops.number_fraction_format = FRACTION_COMBINED;
				else printops.number_fraction_format = FRACTION_FRACTIONAL;
				setResult(NULL, false);
				printops.number_fraction_format = save_format;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "factors", _("factors"))) {
				execute_command(COMMAND_FACTORIZE);
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "best", _("best")) || EQUALS_IGNORECASE_AND_LOCAL(str, "optimal", _("optimal"))) {
				CALCULATOR->resetExchangeRatesUsed();
				MathStructure mstruct_new(CALCULATOR->convertToBestUnit(*mstruct, evalops));
				if(check_exchange_rates()) mstruct->set(CALCULATOR->convertToBestUnit(*mstruct, evalops));
				else mstruct->set(mstruct_new);
				result_action_executed();
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "base", _("base"))) {
				CALCULATOR->resetExchangeRatesUsed();
				MathStructure mstruct_new(CALCULATOR->convertToBaseUnits(*mstruct, evalops));
				if(check_exchange_rates()) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, evalops));
				else mstruct->set(mstruct_new);
				result_action_executed();
			} else {
				CALCULATOR->resetExchangeRatesUsed();
				MathStructure mstruct_new(CALCULATOR->convert(*mstruct, str, evalops));
				if(check_exchange_rates()) mstruct->set(CALCULATOR->convert(*mstruct, str, evalops));
				else mstruct->set(mstruct_new);
				result_action_executed();
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "factor", _("factor"))) {
			execute_command(COMMAND_FACTORIZE);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "simplify", _("simplify"))) {
			execute_command(COMMAND_SIMPLIFY);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "mode", _("mode"))) {
			INIT_SCREEN_CHECK
			puts(""); CHECK_IF_SCREEN_FILLED
			int pctl;
#define PRINT_AND_COLON_TABS(x) FPUTS_UNICODE(x, stdout); pctl = unicode_length_check(x); if(pctl >= 32) fputs("\t", stdout); else if(pctl >= 24) fputs("\t\t", stdout); else if(pctl >= 16) fputs("\t\t\t", stdout); else if(pctl >= 8) fputs("\t\t\t\t", stdout); else fputs("\t\t\t\t\t", stdout);
#define PUTS_BOLD(x) str = "\033[1m"; str += x; str += "\033[0m"; puts(str.c_str());
			PRINT_AND_COLON_TABS(_("abbreviations")); PUTS_UNICODE(b2oo(printops.abbreviate_names, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("algebra mode"));
			switch(evalops.structuring) {
				case STRUCTURING_NONE: {PUTS_UNICODE(_("none")); break;}
				case STRUCTURING_SIMPLIFY: {PUTS_UNICODE(_("simplify")); break;}
				case STRUCTURING_FACTORIZE: {PUTS_UNICODE(_("factorize")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("all prefixes")); PUTS_UNICODE(b2oo(printops.use_all_prefixes, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("angle unit"));
			switch(evalops.parse_options.angle_unit) {
				case ANGLE_UNIT_RADIANS: {PUTS_UNICODE(_("rad")); break;}
				case ANGLE_UNIT_DEGREES: {PUTS_UNICODE(_("rad")); break;}
				case ANGLE_UNIT_GRADIANS: {PUTS_UNICODE(_("gra")); break;}
				default: {PUTS_UNICODE(_("none")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("approximation")); 
			switch(evalops.approximation) {
				case APPROXIMATION_EXACT: {PUTS_UNICODE(_("exact")); break;}
				case APPROXIMATION_TRY_EXACT: {PUTS_UNICODE(_("try exact")); break;}
				case APPROXIMATION_APPROXIMATE: {PUTS_UNICODE(_("approximate")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("assume nonzero denominators")); PUTS_UNICODE(b2oo(evalops.assume_denominators_nonzero, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("warn nonzero denominators")); PUTS_UNICODE(b2oo(evalops.warn_about_denominators_assumed_nonzero, false)); CHECK_IF_SCREEN_FILLED
			string value;
			switch(CALCULATOR->defaultAssumptions()->sign()) {
				case ASSUMPTION_SIGN_POSITIVE: {value = _("positive"); break;}
				case ASSUMPTION_SIGN_NONPOSITIVE: {value = _("non-positive"); break;}
				case ASSUMPTION_SIGN_NEGATIVE: {value = _("negative"); break;}
				case ASSUMPTION_SIGN_NONNEGATIVE: {value = _("non-negative"); break;}
				case ASSUMPTION_SIGN_NONZERO: {value = _("non-zero"); break;}
				default: {}
			}
			if(!value.empty() && !CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_NONE) value += " ";
			switch(CALCULATOR->defaultAssumptions()->type()) {
				case ASSUMPTION_TYPE_INTEGER: {value += _("integer"); break;}
				case ASSUMPTION_TYPE_RATIONAL: {value += _("rational"); break;}
				case ASSUMPTION_TYPE_REAL: {value += _("real"); break;}
				case ASSUMPTION_TYPE_COMPLEX: {value += _("complex"); break;}
				case ASSUMPTION_TYPE_NUMBER: {value += _("number"); break;}
				case ASSUMPTION_TYPE_NONMATRIX: {value += _("non-matrix"); break;}
				default: {}
			}
			if(value.empty()) value = _("unknown");
			PRINT_AND_COLON_TABS(_("assumptions")); PUTS_UNICODE(value.c_str()); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("autoconversion"));
			switch(evalops.auto_post_conversion) {
				case POST_CONVERSION_NONE: {PUTS_UNICODE(_("none")); break;}
				case POST_CONVERSION_BEST: {PUTS_UNICODE(_("optimal")); break;}
				case POST_CONVERSION_BASE: {PUTS_UNICODE(_("base")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("base")); 
			switch(printops.base) {
				case BASE_ROMAN_NUMERALS: {PUTS_UNICODE(_("roman")); break;}
				case BASE_SEXAGESIMAL: {PUTS_UNICODE(_("sexagesimal")); break;}
				case BASE_TIME: {PUTS_UNICODE(_("time")); break;}
				default: {printf("%i\n", printops.base);}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("base display"));
			switch(printops.base_display) {
				case BASE_DISPLAY_NONE: {PUTS_UNICODE(_("none")); break;}
				case BASE_DISPLAY_NORMAL: {PUTS_UNICODE(_("normal")); break;}
				case BASE_DISPLAY_ALTERNATIVE: {PUTS_UNICODE(_("alternative")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("calculate functions")); PUTS_UNICODE(b2oo(evalops.calculate_functions, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("calculate variables")); PUTS_UNICODE(b2oo(evalops.calculate_variables, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("complex numbers")); PUTS_UNICODE(b2oo(evalops.allow_complex, false)); CHECK_IF_SCREEN_FILLED			
			PRINT_AND_COLON_TABS(_("denominator prefixes")); PUTS_UNICODE(b2oo(printops.use_denominator_prefix, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("division sign"));
			switch(printops.division_sign) {
				case DIVISION_SIGN_DIVISION_SLASH: {puts(SIGN_DIVISION_SLASH); break;}
				case DIVISION_SIGN_DIVISION: {puts(SIGN_DIVISION); break;}
				default: {puts("/"); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("dot as separator")); PUTS_UNICODE(b2oo(evalops.parse_options.dot_as_separator, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("excessive parentheses")); PUTS_UNICODE(b2oo(printops.excessive_parenthesis, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("exp mode"));
			switch(printops.min_exp) {
				case EXP_NONE: {PUTS_UNICODE(_("off")); break;}
				case EXP_PRECISION: {PUTS_UNICODE(_("auto")); break;}
				case EXP_PURE: {PUTS_UNICODE(_("pure")); break;}
				case EXP_SCIENTIFIC: {PUTS_UNICODE(_("scientific")); break;}
				case EXP_BASE_3: {PUTS_UNICODE(_("engineering")); break;}
				default: {printf("%i\n", printops.min_exp); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("fractions")); 
			switch(printops.number_fraction_format) {
				case FRACTION_DECIMAL: {PUTS_UNICODE(_("off")); break;}
				case FRACTION_DECIMAL_EXACT: {PUTS_UNICODE(_("exact")); break;}
				case FRACTION_FRACTIONAL: {PUTS_UNICODE(_("on")); break;}
				case FRACTION_COMBINED: {PUTS_UNICODE(_("combined")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("functions")); PUTS_UNICODE(b2oo(evalops.parse_options.functions_enabled, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("input base")); 
			switch(evalops.parse_options.base) {
				case BASE_ROMAN_NUMERALS: {PUTS_UNICODE(_("roman")); break;}
				default: {printf("%i\n", evalops.parse_options.base);}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("indicate infinite series")); PUTS_UNICODE(b2oo(printops.indicate_infinite_series, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("infinite numbers")); PUTS_UNICODE(b2oo(evalops.allow_infinite, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("limit implicit multiplication")); PUTS_UNICODE(b2oo(evalops.parse_options.limit_implicit_multiplication, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("lowercase e")); PUTS_UNICODE(b2oo(printops.lower_case_e, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("lowercase numbers")); PUTS_UNICODE(b2oo(printops.lower_case_numbers, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("max decimals"));
			if(printops.use_max_decimals && printops.max_decimals >= 0) {
				printf("%i\n", printops.max_decimals);
			} else {
				PUTS_UNICODE(_("off"));
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("min decimals")); 
			if(printops.use_min_decimals && printops.min_decimals > 0) {
				printf("%i\n", printops.min_decimals);
			} else {
				PUTS_UNICODE(_("off"));
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("multiplication sign"));
			switch(printops.multiplication_sign) {
				case MULTIPLICATION_SIGN_X: {puts(SIGN_MULTIPLICATION); break;}
				case MULTIPLICATION_SIGN_DOT: {puts(SIGN_MULTIDOT); break;}
				default: {puts("*"); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("parsing mode")); 
			switch(evalops.parse_options.parsing_mode) {
				case PARSING_MODE_ADAPTIVE: {PUTS_UNICODE(_("adaptive")); break;}
				case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {PUTS_UNICODE(_("implicit first")); break;}
				case PARSING_MODE_CONVENTIONAL: {PUTS_UNICODE(_("conventional")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("place units separately")); PUTS_UNICODE(b2oo(printops.place_units_separately, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("precision")) printf("%i\n", CALCULATOR->getPrecision()); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("prefixes")); PUTS_UNICODE(b2oo(printops.use_unit_prefixes, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("read precision")); 
			switch(evalops.parse_options.read_precision) {
				case DONT_READ_PRECISION: {PUTS_UNICODE(_("off")); break;}
				case ALWAYS_READ_PRECISION: {PUTS_UNICODE(_("always")); break;}
				case READ_PRECISION_WHEN_DECIMALS: {PUTS_UNICODE(_("when decimals")); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("round to even")); PUTS_UNICODE(b2oo(printops.round_halfway_to_even, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("rpn")); PUTS_UNICODE(b2oo(rpn_mode, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("rpn syntax")); PUTS_UNICODE(b2oo(evalops.parse_options.rpn, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("save definitions")); PUTS_UNICODE(b2yn(save_defs_on_exit, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("save mode")); PUTS_UNICODE(b2yn(save_mode_on_exit, false)); CHECK_IF_SCREEN_FILLED			
			PRINT_AND_COLON_TABS(_("show ending zeroes")); PUTS_UNICODE(b2oo(printops.show_ending_zeroes, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("show negative exponents")); PUTS_UNICODE(b2oo(printops.negative_exponents, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("short multiplication")); PUTS_UNICODE(b2oo(printops.short_multiplication, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("spacious")); PUTS_UNICODE(b2oo(printops.spacious, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("spell out logical")); PUTS_UNICODE(b2oo(printops.spell_out_logical_operators, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("sync units")); PUTS_UNICODE(b2oo(evalops.sync_units, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("unicode")); PUTS_UNICODE(b2oo(printops.use_unicode_signs, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("units")); PUTS_UNICODE(b2oo(evalops.parse_options.units_enabled, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("unknowns")); PUTS_UNICODE(b2oo(evalops.parse_options.unknowns_enabled, false)); CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("update exchange rates")); 
			switch(auto_update_exchange_rates) {
				case -1: {PUTS_UNICODE(_("ask")); break;}
				case 0: {PUTS_UNICODE(_("never")); break;}
				default: {printf("%i\n", auto_update_exchange_rates); break;}
			}
			CHECK_IF_SCREEN_FILLED
			PRINT_AND_COLON_TABS(_("variables")); PUTS_UNICODE(b2oo(evalops.parse_options.variables_enabled, false)); CHECK_IF_SCREEN_FILLED
			puts("");
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "help", _("help")) || str == "?") {
			INIT_SCREEN_CHECK
			puts(""); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("Enter a mathematical expression or a command and press enter.")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("Complete functions, units and variables with the tabulator key.")); CHECK_IF_SCREEN_FILLED
			puts(""); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("Available commands are:")); CHECK_IF_SCREEN_FILLED
			puts(""); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("approximate")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("assume"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("ASSUMPTIONS")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("base"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("BASE")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("clear stack")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("to"), stdout); fputs("/", stdout); FPUTS_UNICODE(_("convert"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("UNIT or \"TO\" COMMAND")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("exact")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("exrates"), stdout);			
			if(!CALCULATOR->hasGVFS()) {
				fputs(" ", stdout); FPUTS_UNICODE("[", stdout); fputs(_("WGET ARGUMENTS"), stdout); FPUTS_UNICODE("]", stdout);
			}			
			puts(""); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("factor")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("info")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("mode")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("rpn"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("ON/OFF")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("save"), stdout); fputs("/", stdout); FPUTS_UNICODE(_("store"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("NAME"), stdout); fputs(" [", stdout); FPUTS_UNICODE(_("CATEGORY"), stdout); fputs("] [", stdout); FPUTS_UNICODE("[", stdout); fputs(_("TITLE"), stdout); PUTS_UNICODE("]"); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("save definitions")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("save mode")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("set"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("OPTION"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("VALUE")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("simplify")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("stack")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("quit"), stdout); fputs("/", stdout); PUTS_UNICODE(_("exit")); CHECK_IF_SCREEN_FILLED_PUTS("");
			PUTS_UNICODE(_("Type help COMMAND for more information (example: help save).")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("Type info NAME for information about a function, variable or unit (example: info sin).")); CHECK_IF_SCREEN_FILLED_PUTS("");
			PUTS_UNICODE(_("For more information about mathematical expression, different options, and a complete list of functions, variables and units, see the relevant sections in the manual of the graphical user interface (available at http://qalculate.github.io/manual/index.html)"));
			puts("");
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "info", _("info"))) {
			int pctl;
#define PRINT_AND_COLON_TABS_INFO(x) FPUTS_UNICODE(x, stdout); pctl = unicode_length_check(x); if(pctl >= 23) fputs(":\t", stdout); else if(pctl >= 15) fputs(":\t\t", stdout); else if(pctl >= 7) fputs(":\t\t\t", stdout); else fputs(":\t\t\t\t", stdout);
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			show_info:
			ExpressionItem *item = CALCULATOR->getActiveExpressionItem(str);
			if(!item) {
				PUTS_UNICODE(_("No function, variable or unit with specified name exist."));
			} else {
				switch(item->type()) {
					case TYPE_FUNCTION: {
						INIT_SCREEN_CHECK
						CHECK_IF_SCREEN_FILLED_PUTS("");
						MathFunction *f = (MathFunction*) item;
						Argument *arg;
						Argument default_arg;
						string str2;
						str = _("Function");
						if(!f->title(false).empty()) {
							str += ": ";
							str += f->title();
						}
						CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
						CHECK_IF_SCREEN_FILLED_PUTS("");
						const ExpressionName *ename = &f->preferredName(false, printops.use_unicode_signs);
						str = ename->name;
						int iargs = f->maxargs();
						if(iargs < 0) {
							iargs = f->minargs() + 1;
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
									str2 += " ";
									str2 += i2s(i2);
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
						CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
						for(size_t i2 = 1; i2 <= f->countNames(); i2++) {
							if(&f->getName(i2) != ename) {
								CHECK_IF_SCREEN_FILLED_PUTS(f->getName(i2).name.c_str());
							}
						}
						
						if(f->subtype() == SUBTYPE_DATA_SET) {
							CHECK_IF_SCREEN_FILLED_PUTS("");
							snprintf(buffer, 1000, _("Retrieves data from the %s data set for a given object and property. If \"info\" is typed as property, all properties of the object will be listed."), f->title().c_str());
							CHECK_IF_SCREEN_FILLED_PUTS(buffer);
						}
						if(!f->description().empty()) {
							CHECK_IF_SCREEN_FILLED_PUTS(""); 
							CHECK_IF_SCREEN_FILLED_PUTS(f->description().c_str());
						}
						if(f->subtype() == SUBTYPE_DATA_SET && !((DataSet*) f)->copyright().empty()) {
							CHECK_IF_SCREEN_FILLED_PUTS(""); 
							CHECK_IF_SCREEN_FILLED_PUTS(((DataSet*) f)->copyright().c_str());
						}
						if(iargs) {
							CHECK_IF_SCREEN_FILLED_PUTS("");
							CHECK_IF_SCREEN_FILLED_PUTS(_("Arguments"));
							for(int i2 = 1; i2 <= iargs; i2++) {	
								arg = f->getArgumentDefinition(i2);
								if(arg && !arg->name().empty()) {
									str = arg->name();
								} else {
									str = i2s(i2);	
								}
								str += ": ";
								if(arg) {
									str2 = arg->printlong();
								} else {
									str2 = default_arg.printlong();
								}
								if(i2 > f->minargs()) {
									str2 += " (";
									//optional argument, in description
									str2 += _("optional");
									if(!f->getDefaultValue(i2).empty()) {
										str2 += ", ";
										//argument default, in description
										str2 += _("default: ");
										str2 += f->getDefaultValue(i2);
									}
									str2 += ")";
								}
								str += str2;
								CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
							}
						}
						if(!f->condition().empty()) {
							CHECK_IF_SCREEN_FILLED_PUTS(""); 
							str = _("Requirement");
							str += ": ";
							str += f->printCondition();
							CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
						}
						if(f->subtype() == SUBTYPE_DATA_SET) {
							DataSet *ds = (DataSet*) f;
							CHECK_IF_SCREEN_FILLED_PUTS("");
							CHECK_IF_SCREEN_FILLED_PUTS(_("Properties"));
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
										str += "\n";
										str += dp->description();										
									}
									CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
								}
								dp = ds->getNextProperty(&it);
							}
						}
						CHECK_IF_SCREEN_FILLED_PUTS("");
						break;
					}
					case TYPE_UNIT: {
						puts("");						
						if(!item->title(false).empty()) {
							PRINT_AND_COLON_TABS_INFO(_("Unit"));
							FPUTS_UNICODE(item->title().c_str(), stdout);
						} else {
							FPUTS_UNICODE(_("Unit"), stdout);
						}
						puts("");
						PRINT_AND_COLON_TABS_INFO(_("Names"));
						if(item->subtype() != SUBTYPE_COMPOSITE_UNIT) {
							const ExpressionName *ename = &item->preferredName(true, printops.use_unicode_signs);
							FPUTS_UNICODE(ename->name.c_str(), stdout);
							for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
								if(&item->getName(i2) != ename) {
									fputs(" / ", stdout);
									FPUTS_UNICODE(item->getName(i2).name.c_str(), stdout);
								}
							}
						}
						puts("");
						switch(item->subtype()) {
							case SUBTYPE_BASE_UNIT: {
								break;
							}
							case SUBTYPE_ALIAS_UNIT: {
								AliasUnit *au = (AliasUnit*) item;
								PRINT_AND_COLON_TABS_INFO(_("Base Unit"));
								FPUTS_UNICODE(au->firstBaseUnit()->preferredDisplayName(printops.abbreviate_names, printops.use_unicode_signs).name.c_str(), stdout);
								if(au->firstBaseExponent() != 1) {
									fputs(POWER, stdout);
									printf("%i", au->firstBaseExponent());
								}
								puts("");
								PRINT_AND_COLON_TABS_INFO(_("Relation"));
								FPUTS_UNICODE(CALCULATOR->localizeExpression(au->expression()).c_str(), stdout);
								if(item->isApproximate()) {
									fputs(" (", stdout);
									FPUTS_UNICODE(_("approximate"), stdout);
									fputs(")", stdout);
									
								}
								if(!au->inverseExpression().empty()) {
									puts("");
									PRINT_AND_COLON_TABS_INFO(_("Inverse Relation"));
									FPUTS_UNICODE(CALCULATOR->localizeExpression(au->inverseExpression()).c_str(), stdout);
									if(item->isApproximate()) {
										fputs(" (", stdout);
										FPUTS_UNICODE(_("approximate"), stdout);
										fputs(")", stdout);
									}									
								}
								puts("");
								break;
							}
							case SUBTYPE_COMPOSITE_UNIT: {
								PRINT_AND_COLON_TABS_INFO(_("Base Units"));
								PUTS_UNICODE(((CompositeUnit*) item)->print(false, true, printops.use_unicode_signs).c_str());
								break;
							}
						}					
						if(!item->description().empty()) {
							puts("");
							PUTS_UNICODE(item->description().c_str());
						}
						puts("");
						break;
					}
					case TYPE_VARIABLE: {
						puts("");						
						if(!item->title(false).empty()) {
							PRINT_AND_COLON_TABS_INFO(_("Variable"));
							FPUTS_UNICODE(item->title().c_str(), stdout);
						} else {
							FPUTS_UNICODE(_("Variable"), stdout);
						}
						puts("");
						PRINT_AND_COLON_TABS_INFO(_("Names"));
						const ExpressionName *ename = &item->preferredName(false, printops.use_unicode_signs);
						FPUTS_UNICODE(ename->name.c_str(), stdout);
						for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
							if(&item->getName(i2) != ename) {
								fputs(" / ", stdout);
								FPUTS_UNICODE(item->getName(i2).name.c_str(), stdout);
							}
						}
						Variable *v = (Variable*) item;
						string value;
						if(is_answer_variable(v)) {
							value = _("a previous result");
						} else if(v->isKnown()) {
							if(((KnownVariable*) v)->isExpression()) {
								value = CALCULATOR->localizeExpression(((KnownVariable*) v)->expression());
								if(value.length() > 40) {
									value = value.substr(0, 30);
									value += "...";
								}
							} else {
								if(((KnownVariable*) v)->get().isMatrix()) {
									value = _("matrix");
								} else if(((KnownVariable*) v)->get().isVector()) {
									value = _("vector");
								} else {
									value = CALCULATOR->printMathStructureTimeOut(((KnownVariable*) v)->get(), 30);
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
								if(!value.empty() && !((UnknownVariable*) v)->assumptions()->type() == ASSUMPTION_TYPE_NONE) value += " ";
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
						puts("");
						PRINT_AND_COLON_TABS_INFO(_("Value"));
						FPUTS_UNICODE(value.c_str(), stdout);
						if(item->isApproximate()) {
							fputs(" (", stdout);
							FPUTS_UNICODE(_("approximate"), stdout);
							fputs(")", stdout);
						}
						puts("");
						if(!item->description().empty()) {
							fputs("\n", stdout);
							FPUTS_UNICODE(item->description().c_str(), stdout);
							fputs("\n", stdout);
						}
						puts("");
						break;
					}
				}
			}
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "help", _("help"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			if(EQUALS_IGNORECASE_AND_LOCAL(str, "factor", _("factor"))) {
				puts("");
				PUTS_UNICODE(_("Factorizes the current result."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "simplify", _("simplify"))) {
				puts("");
				PUTS_UNICODE(_("Simplifies the current result."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "set", _("set"))) {
				INIT_SCREEN_CHECK
				int pctl;

#define STR_AND_TABS(x) str = x; pctl = unicode_length_check(x); if(pctl >= 32) {str += "\t";} else if(pctl >= 24) {str += "\t\t";} else if(pctl >= 16) {str += "\t\t\t";} else if(pctl >= 8) {str += "\t\t\t\t";} else {str += "\t\t\t\t\t";}
#define STR_AND_TABS_BOOL(s, v) STR_AND_TABS(s); str += "("; str += _("on"); if(v) {str += "*";} str += ", "; str += _("off"); if(!v) {str += "*";} str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_YESNO(s, v) STR_AND_TABS(s); str += "("; str += _("yes"); if(v) {str += "*";} str += ", "; str += _("no"); if(!v) {str += "*";} str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_2(s, v, s0, s1, s2) STR_AND_TABS(s); str += "(0"; if(v == 0) {str += "*";} str += " = "; str += s0; str += ", 1"; if(v == 1) {str += "*";} str += " = "; str += s1; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s2; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_3(s, v, s0, s1, s2, s3) STR_AND_TABS(s); str += "(0"; if(v == 0) {str += "*";} str += " = "; str += s0; str += ", 1"; if(v == 1) {str += "*";} str += " = "; str += s1; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s2; str += ", 2"; if(v == 3) {str += "*";} str += " = "; str += s3; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Sets the value of an option."));				
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Available options and accepted values are:"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				STR_AND_TABS_BOOL(_("abbreviations"), printops.abbreviate_names);
				STR_AND_TABS_2(_("algebra mode"), evalops.structuring, _("none"), _("simplify"), _("factorize"));
				STR_AND_TABS_BOOL(_("all prefixes"), printops.use_all_prefixes);
				STR_AND_TABS_3(_("angle unit"), evalops.parse_options.angle_unit, _("none"), _("radians"), _("degrees"), _("gradians"));
				STR_AND_TABS_2(_("approximation"), evalops.approximation, _("exact"), _("try exact"), _("approximate"));
				STR_AND_TABS_BOOL(_("assume nonzero denominators"), evalops.assume_denominators_nonzero);
				STR_AND_TABS_BOOL(_("warn nonzero denominators"), evalops.warn_about_denominators_assumed_nonzero);
				Assumptions *ass = CALCULATOR->defaultAssumptions();
				STR_AND_TABS(_("assumptions")); 
				str += "("; str += _("unknown"); 
				if(ass->sign() == ASSUMPTION_SIGN_UNKNOWN) str += "*";
				str += ", "; str += _("non-zero");
				if(ass->sign() == ASSUMPTION_SIGN_NONZERO) str += "*";
				str += ", "; str += _("positive");
				if(ass->sign() == ASSUMPTION_SIGN_POSITIVE) str += "*";
				str += ", "; str += _("negative");
				if(ass->sign() == ASSUMPTION_SIGN_NEGATIVE) str += "*";
				str += ", "; str += _("non-positive");
				if(ass->sign() == ASSUMPTION_SIGN_NONPOSITIVE) str += "*";
				str += ", "; str += _("non-negative");
				if(ass->sign() == ASSUMPTION_SIGN_NONNEGATIVE) str += "*";
				str += " +\n"; str += "\t\t\t\t\t "; str += _("unknown");
				if(ass->type() == ASSUMPTION_TYPE_NONE) str += "*";
				str += ", "; str += _("non-matrix");
				if(ass->type() == ASSUMPTION_TYPE_NONMATRIX) str += "*";
				str += ", "; str += _("number");
				if(ass->type() == ASSUMPTION_TYPE_NUMBER) str += "*";
				str += ", "; str += _("complex");
				if(ass->type() == ASSUMPTION_TYPE_COMPLEX) str += "*";
				str += ", "; str += _("real");
				if(ass->type() == ASSUMPTION_TYPE_REAL) str += "*";
				str += ", "; str += _("rational");
				if(ass->type() == ASSUMPTION_TYPE_RATIONAL) str += "*";
				str += ", "; str += _("integer");
				if(ass->type() == ASSUMPTION_TYPE_INTEGER) str += "*";
				str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_2(_("autoconversion"), evalops.auto_post_conversion, _("none"), _("optimal"), _("base"));
				STR_AND_TABS(_("base")); str += "(2 - 36"; str += ", "; str += _("bin");
				if(printops.base == BASE_BINARY) str += "*";
				str += ", "; str += _("oct");
				if(printops.base == BASE_OCTAL) str += "*";
				str += ", "; str += _("dec");
				if(printops.base == BASE_DECIMAL) str += "*";
				str += ", "; str += _("hex");
				if(printops.base == BASE_HEXADECIMAL) str += "*";
				str += ", "; str += _("sex");
				if(printops.base == BASE_SEXAGESIMAL) str += "*";
				str += ", "; str += _("time");
				if(printops.base == BASE_TIME) str += "*";
				str += ", "; str += _("roman");
				if(printops.base == BASE_ROMAN_NUMERALS) str += "*";
				str += ")";
				if(printops.base > 2 && printops.base <= 36 && printops.base != BASE_OCTAL && printops.base != BASE_DECIMAL && printops.base != BASE_HEXADECIMAL) {str += " "; str += i2s(printops.base); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_2(_("base display"), printops.base_display, _("none"), _("normal"), _("alternative"));
				STR_AND_TABS_BOOL(_("calculate functions"), evalops.calculate_functions);
				STR_AND_TABS_BOOL(_("calculate variables"), evalops.calculate_variables);
				STR_AND_TABS_BOOL(_("complex numbers"), evalops.allow_complex);
				STR_AND_TABS_BOOL(_("denominator prefixes"), printops.use_denominator_prefix);
				STR_AND_TABS_2(_("division sign"), printops.division_sign, "/", SIGN_DIVISION_SLASH, SIGN_DIVISION);
				STR_AND_TABS_BOOL(_("dot as separator"), evalops.parse_options.dot_as_separator);
				STR_AND_TABS_BOOL(_("exact"), (evalops.approximation == APPROXIMATION_EXACT));
				STR_AND_TABS_BOOL(_("excessive parentheses"), printops.excessive_parenthesis);
				STR_AND_TABS(_("exp mode")); str += "("; str += _("off"); 
				if(printops.min_exp == EXP_NONE) str += "*";
				str += ", "; str += _("auto");
				if(printops.min_exp == EXP_PRECISION) str += "*";
				str += ", "; str += _("engineering");
				if(printops.min_exp == EXP_BASE_3) str += "*";
				str += ", "; str += _("pure");
				if(printops.min_exp == EXP_PURE) str += "*";
				str += ", "; str += _("scientific");
				if(printops.min_exp == EXP_SCIENTIFIC) str += "*";
				str += ", >= 0)";
				if(printops.min_exp != EXP_NONE && printops.min_exp != EXP_NONE && printops.min_exp != EXP_PRECISION && printops.min_exp != EXP_BASE_3 && printops.min_exp != EXP_PURE && printops.min_exp != EXP_SCIENTIFIC) {str += " "; str += i2s(printops.min_exp); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_3(_("fractions"), printops.number_fraction_format, _("off"), _("exact"), _("on"), _("combined"));
				STR_AND_TABS_BOOL(_("functions"), evalops.parse_options.functions_enabled);
				STR_AND_TABS(_("input base")); str += "(2 - 36"; str += ", "; str += _("bin");
				if(evalops.parse_options.base == BASE_BINARY) str += "*";
				str += ", "; str += _("oct");
				if(evalops.parse_options.base == BASE_OCTAL) str += "*";
				str += ", "; str += _("dec");
				if(evalops.parse_options.base == BASE_DECIMAL) str += "*";
				str += ", "; str += _("hex");
				if(evalops.parse_options.base == BASE_HEXADECIMAL) str += "*";
				str += ", "; str += _("roman");
				if(evalops.parse_options.base == BASE_ROMAN_NUMERALS) str += "*";
				str += ")";
				if(evalops.parse_options.base > 2 && evalops.parse_options.base != BASE_OCTAL && evalops.parse_options.base != BASE_DECIMAL && evalops.parse_options.base != BASE_HEXADECIMAL) {str += " "; str += i2s(evalops.parse_options.base); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_BOOL(_("infinite numbers"), evalops.allow_infinite);
				STR_AND_TABS_BOOL(_("indicate infinite series"), printops.indicate_infinite_series);
				STR_AND_TABS_BOOL(_("limit implicit multiplication"), evalops.parse_options.limit_implicit_multiplication);
				STR_AND_TABS_BOOL(_("lowercase e"), printops.lower_case_e);
				STR_AND_TABS_BOOL(_("lowercase numbers"), printops.lower_case_numbers);
				STR_AND_TABS(_("max decimals")); str += "("; str += _("off");
				if(printops.max_decimals < 0) str += "*";
				str += ", >= 0)";
				if(printops.max_decimals >= 0) {str += " "; str += i2s(printops.max_decimals); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS(_("min decimals")); str += "("; str += _("off");
				if(printops.min_decimals < 0) str += "*";
				str += ", >= 0)";
				if(printops.min_decimals >= 0) {str += " "; str += i2s(printops.min_decimals); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_2(_("multiplication sign"), printops.multiplication_sign, "*", SIGN_MULTIDOT, SIGN_MULTIPLICATION);
				STR_AND_TABS_2(_("parsing mode"), evalops.parse_options.parsing_mode, _("adaptive"), _("implicit first"), _("conventional"));
				STR_AND_TABS_BOOL(_("place units separately"), printops.place_units_separately);
				STR_AND_TABS(_("precision"));  str += "(> 0) "; str += i2s(CALCULATOR->getPrecision()); str += "*"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_BOOL(_("prefixes"), printops.use_unit_prefixes);
				STR_AND_TABS_2(_("read precision"), evalops.parse_options.read_precision, _("off"), _("always"), _("when decimals"))
				STR_AND_TABS_BOOL(_("round to even"), printops.round_halfway_to_even);
				STR_AND_TABS_BOOL(_("rpn"), rpn_mode);
				STR_AND_TABS_BOOL(_("rpn syntax"), evalops.parse_options.rpn);
				STR_AND_TABS_YESNO(_("save definitions"), save_defs_on_exit);
				STR_AND_TABS_YESNO(_("save mode"), save_mode_on_exit);
				STR_AND_TABS_BOOL(_("show ending zeroes"), printops.show_ending_zeroes);
				STR_AND_TABS_BOOL(_("show negative exponents"), printops.negative_exponents);
				STR_AND_TABS_BOOL(_("short multiplication"), printops.short_multiplication);
				STR_AND_TABS_BOOL(_("spacious"), printops.spacious);
				STR_AND_TABS_BOOL(_("spell out logical"), printops.spell_out_logical_operators);
				STR_AND_TABS_BOOL(_("sync units"), evalops.sync_units);
				STR_AND_TABS_BOOL(_("unicode"), printops.use_unicode_signs);
				STR_AND_TABS_BOOL(_("units"), evalops.parse_options.units_enabled);
				STR_AND_TABS_BOOL(_("unknowns"), evalops.parse_options.unknowns_enabled);
				STR_AND_TABS(_("update exchange rates")); 
				str += "(-1 = "; str += _("ask"); if(auto_update_exchange_rates < 0) str += "*";
				str += ", 0 = "; str += _("never"); if(auto_update_exchange_rates == 0) str += "*";
				str += ", > 0 = "; str += _("days"); str += ")";
				if(auto_update_exchange_rates > 0) {str += " "; str += i2s(auto_update_exchange_rates); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_BOOL(_("variables"), evalops.parse_options.variables_enabled);
				CHECK_IF_SCREEN_FILLED_PUTS(_("The current value is marked with '*'."));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Example: set base 16."));				
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Some options have additional help text (e.g. help parsing mode)."));
				CHECK_IF_SCREEN_FILLED_PUTS("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "assume", _("assume"))) {
				puts("");
				PUTS_UNICODE(_("Set default assumptions for unknown variables."));
				Assumptions *ass = CALCULATOR->defaultAssumptions();
				str = "("; str += _("unknown"); 
				if(ass->sign() == ASSUMPTION_SIGN_UNKNOWN) str += "*";
				str += ", "; str += _("non-zero");
				if(ass->sign() == ASSUMPTION_SIGN_NONZERO) str += "*";
				str += ", "; str += _("positive");
				if(ass->sign() == ASSUMPTION_SIGN_POSITIVE) str += "*";
				str += ", "; str += _("negative");
				if(ass->sign() == ASSUMPTION_SIGN_NEGATIVE) str += "*";
				str += ", "; str += _("non-positive");
				if(ass->sign() == ASSUMPTION_SIGN_NONPOSITIVE) str += "*";
				str += ", "; str += _("non-negative");
				if(ass->sign() == ASSUMPTION_SIGN_NONNEGATIVE) str += "*";
				str += " +\n"; str += _("unknown");
				if(ass->type() == ASSUMPTION_TYPE_NONE) str += "*";
				str += ", "; str += _("non-matrix");
				if(ass->type() == ASSUMPTION_TYPE_NONMATRIX) str += "*";
				str += ", "; str += _("number");
				if(ass->type() == ASSUMPTION_TYPE_NUMBER) str += "*";
				str += ", "; str += _("complex");
				if(ass->type() == ASSUMPTION_TYPE_COMPLEX) str += "*";
				str += ", "; str += _("real");
				if(ass->type() == ASSUMPTION_TYPE_REAL) str += "*";
				str += ", "; str += _("rational");
				if(ass->type() == ASSUMPTION_TYPE_RATIONAL) str += "*";
				str += ", "; str += _("integer");
				if(ass->type() == ASSUMPTION_TYPE_INTEGER) str += "*";
				str += ")";
				PUTS_UNICODE(str.c_str());
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "save", _("save")) || EQUALS_IGNORECASE_AND_LOCAL(str, "store", _("store"))) {
				puts("");
				PUTS_UNICODE(_("Saves the current result in a variable with the specified name. You may optionally also provide a category (default \"Temporary\") and a title."));
				PUTS_UNICODE(_("If name equals \"mode\" or \"definitions\", the current mode and definitions, respectively, will be saved."));
				puts("");
				PUTS_UNICODE(_("Example: store var1."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "mode", _("mode"))) {
				puts("");
				PUTS_UNICODE(_("Displays the current mode."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "info", _("info"))) {
				puts("");
				PUTS_UNICODE(_("Displays information about a function, variable or unit."));
				puts("");
				PUTS_UNICODE(_("Example: info sin."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "exrates", _("exrates"))) {
				puts("");
				PUTS_UNICODE(_("Downloads current exchange rates from the Internet."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rpn", _("rpn"))) {
				puts("");
				PUTS_UNICODE(_("(De)activates the Reverse Polish Notation mode."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "clear stack", _("clear stack"))) {
				puts("");
				PUTS_UNICODE(_("Clears the RPN stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "stack", _("stack"))) {
				puts("");
				PUTS_UNICODE(_("Displays the RPN stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "base", _("base"))) {
				puts("");
				PUTS_UNICODE(_("Sets the result base (equivalent to set base)."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "exact", _("exact"))) {
				puts("");
				PUTS_UNICODE(_("Equivalent to set approximation exact."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "approximate", _("approximate"))) {
				puts("");
				PUTS_UNICODE(_("Equivalent to set approximation try exact."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "convert", _("convert")) || EQUALS_IGNORECASE_AND_LOCAL(str, "to", _("to"))) {
				puts("");
				PUTS_UNICODE(_("Converts units or changes number base in current result."));
				puts("");
				PUTS_UNICODE(_("Possible values:"));
				puts("");
				PUTS_UNICODE(_("a unit (e.g. meter, prepend with ? to request the optimal prefix)"));
				PUTS_UNICODE(_("a unit expression (e.g. km/h)"));
				PUTS_UNICODE(_("base (convert to base units)"));
				PUTS_UNICODE(_("optimal (convert to optimal unit)"));
				puts("");
				PUTS_UNICODE(_("bin / binary (show as binary number)"));
				PUTS_UNICODE(_("oct / octal (show as octal number)"));
				PUTS_UNICODE(_("hex / hexadecimal (show as hexadecimal number)"));
				PUTS_UNICODE(_("bases (show as binary, octal, decimal and hexadecimal number)"));
				puts("");
				PUTS_UNICODE(_("fraction (show result in combined fractional format)"));
				PUTS_UNICODE(_("factors (factorize result)"));
				puts("");
				PUTS_UNICODE(_("Example: to ?g"));
				puts("");
				if(EQUALS_IGNORECASE_AND_LOCAL(str, "to", _("to"))) {
					PUTS_UNICODE(_("This command can also be typed directly at the end of the mathematical expression."));
					puts("");
				}
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "quit", _("quit")) || EQUALS_IGNORECASE_AND_LOCAL(str, "exit", _("exit"))) {
				puts("");
				PUTS_UNICODE(_("Terminates this program."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "parsing mode", _("parsing mode"))) {
				puts("");
				PUTS_BOLD(_("conventional"));
				puts(_("Implicit multiplication does not differ from explicit multiplication (\"12/2(1+2) = 12/2*3 = 18\", \"5x/5y = 5*x/5*y = xy\")."));
				puts("");
				PUTS_BOLD(_("implicit first")); 
				puts(_("Implicit multiplication is parsed before explicit multiplication (\"12/2(1+2) = 12/(2*3) = 2\", \"5x/5y = (5*x)/(5*y) = x/y\")."));
				puts("");
				PUTS_BOLD(_("adaptive")); 
				puts(_("The default adaptive mode works as the \"implicit first\" mode, unless spaces are found (\"1/5x = 1/(5*x)\", but \"1/5 x = (1/5)*x\"). In the adaptive mode unit expressions are parsed separately (\"5 m/5 m/s = (5*m)/(5*(m/s)) = 1 s\")."));
				puts("");
				puts(_("Function arguments without parentheses are an exception, where implicit multiplication in front of variables and units is parsed first regardless of mode (\"sqrt 2x = sqrt(2x)\")."));
				puts("");
			} else {
				goto show_info;
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "quit", _("quit")) || EQUALS_IGNORECASE_AND_LOCAL(str, "exit", _("exit"))) {
#ifdef HAVE_LIBREADLINE
			if(!cfile) {
				free(rlbuffer);
			}
#endif			
			break;
		} else {
			size_t index = str.find_first_of(ID_WRAPS);
			if(index != string::npos) {
				printf(_("Illegal character, \'%c\', in expression."), str[index]);
				puts("");
			} else {
				expression_str = str;
				execute_expression();
			}
		}
#ifdef HAVE_LIBREADLINE
		if(!cfile) {		
			add_history(rlbuffer);		
			free(rlbuffer);
		}
#endif
    	}
	if(cfile && cfile != stdin) {
		fclose(cfile);
	}
	
	handle_exit();
	
	return 0;
	
}

void RPNRegisterAdded(string, int = 0) {}
void RPNRegisterRemoved(int) {}
void RPNRegisterChanged(string, int) {}

bool display_errors(bool goto_input) {
	if(!CALCULATOR->message()) return false;
	bool b = false;
	while(true) {
		MessageType mtype = CALCULATOR->message()->type();
		if(b && goto_input) fputs("  ", stdout);
		if(mtype == MESSAGE_ERROR) {
			FPUTS_UNICODE(_("error"), stdout); fputs(": ", stdout); PUTS_UNICODE(CALCULATOR->message()->message().c_str());
		} else if(mtype == MESSAGE_WARNING) {
			FPUTS_UNICODE(_("warning"), stdout); fputs(": ", stdout); PUTS_UNICODE(CALCULATOR->message()->message().c_str());
		} else {
			PUTS_UNICODE(CALCULATOR->message()->message().c_str());
		}
		if(!CALCULATOR->nextMessage()) break;
		b = true;
	}
	return true;
}

void on_abort_display() {
	CALCULATOR->abortPrint();
}


void *view_proc(void *pipe) {

	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	FILE *view_pipe = (FILE*) pipe;
	
	while(true) {
	
		//pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		void *x = NULL;
		fread(&x, sizeof(void*), 1, view_pipe);
		MathStructure m(*((MathStructure*) x));
		bool b_stack = false;
		fread(&b_stack, sizeof(bool), 1, view_pipe);
		fread(&x, sizeof(void*), 1, view_pipe);
		if(x) {
			PrintOptions po;
			po.preserve_format = true;
			po.show_ending_zeroes = true;
			po.lower_case_e = printops.lower_case_e;
			po.lower_case_numbers = printops.lower_case_numbers;
			po.base_display = printops.base_display;
			po.abbreviate_names = false;
			po.use_unicode_signs = printops.use_unicode_signs;
			po.multiplication_sign = printops.multiplication_sign;
			po.division_sign = printops.division_sign;
			po.short_multiplication = false;
			po.excessive_parenthesis = true;
			po.improve_division_multipliers = false;
			po.restrict_to_parent_precision = false;
			po.spell_out_logical_operators = printops.spell_out_logical_operators;
			MathStructure mp(*((MathStructure*) x));
			fread(&po.is_approximate, sizeof(bool*), 1, view_pipe);
			mp.format(po);
			parsed_text = mp.print(po);
		}
		printops.allow_non_usable = false;
		
		m.format(printops);
		result_text = m.print(printops);
	
		if(result_text == _("aborted")) {
			*printops.is_approximate = false;
		}
		b_busy = false;
	}
	return NULL;
}


void setResult(Prefix *prefix, bool update_parse, bool goto_input, size_t stack_index, bool register_moved, bool noprint) {

	b_busy = true;
	
	if(!interactive_mode) goto_input = false;

	string prev_result_text = result_text;
	result_text = "?";

	if(update_parse) {
		parsed_text = "aborted";
	}

	if(!rpn_mode) stack_index = 0;
	if(stack_index != 0) {
		update_parse = false;
	}
	if(register_moved) {
		update_parse = false;
	}

	if(register_moved) {
		result_text = _("RPN Register Moved");
	}
	
	printops.prefix = prefix;
	
	//CALCULATOR->saveState();
	CALCULATOR->startPrintControl();

	bool parsed_approx = false;
	if(stack_index == 0) {
		fwrite(&mstruct, sizeof(void*), 1, view_pipe_w);
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		fwrite(&mreg, sizeof(void*), 1, view_pipe_w);
	}
	bool b_stack = stack_index != 0;
	fwrite(&b_stack, sizeof(bool), 1, view_pipe_w);
	if(update_parse) {
		fwrite(&parsed_mstruct, sizeof(void*), 1, view_pipe_w);
		bool *parsed_approx_p = &parsed_approx;
		fwrite(&parsed_approx_p, sizeof(void*), 1, view_pipe_w);
	} else {
		void *x = NULL;
		fwrite(&x, sizeof(void*), 1, view_pipe_w);
	}
	fflush(view_pipe_w);

	struct timespec rtime;
	rtime.tv_sec = 0;
	rtime.tv_nsec = 10000000;
	int i = 0;
	bool has_printed = false;
	while(b_busy && i < 75) {
		nanosleep(&rtime, NULL);
		i++;
	}
	i = 0;
	
	if(b_busy && !cfile) {
		if(!result_only) { 
			FPUTS_UNICODE(_("Processing (press Enter to abort)"), stdout);
			has_printed = true;
			fflush(stdout);
		}
	}
#ifdef HAVE_LIBREADLINE	
	int c = 0;
#else
	char c = 0;
#endif
	rtime.tv_nsec = 100000000;
	while(b_busy) {
		if(cfile) {
			nanosleep(&rtime, NULL);
		} else {
			FD_ZERO(&in_set);
			FD_SET(STDIN_FILENO, &in_set);
			if(select(FD_SETSIZE, &in_set, NULL, NULL, &timeout) > 0) {
#ifdef HAVE_LIBREADLINE		
				if(use_readline) c = rl_read_key();
				else read(STDIN_FILENO, &c, 1);
#else
				read(STDIN_FILENO, &c, 1);
#endif			
				if(c == '\n') {
					on_abort_display();
				}
			} else {
				if(!result_only) {
					printf(".");
					fflush(stdout);
				}
				nanosleep(&rtime, NULL);
			}
		}
	}
	CALCULATOR->stopPrintControl();

	printops.prefix = NULL;

	if(noprint) {
		return;
	}

	b_busy = true;
	
	if(has_printed) printf("\n");
	if(goto_input) printf("\n  ");

	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}
	
	if(!result_only && display_errors(goto_input) && goto_input) printf("  ");

	if(stack_index != 0) {
		RPNRegisterChanged(result_text, stack_index);
	} else {
		if(!result_only) {
			if(mstruct->isComparison()) fputs(LEFT_PARENTHESIS, stdout);
			if(update_parse) {
				FPUTS_UNICODE(parsed_text.c_str(), stdout);
			} else {
				FPUTS_UNICODE(prev_result_text.c_str(), stdout);
			}
			if(mstruct->isComparison()) fputs(RIGHT_PARENTHESIS, stdout);
			if(!(*printops.is_approximate) && !mstruct->isApproximate()) {
				printf(" = ");	
			} else {
				if(printops.use_unicode_signs) {
					printf(" " SIGN_ALMOST_EQUAL " ");
				} else {
					printf(" = %s ", _("approx."));
				}
			}
		}
		if(!result_only && mstruct->isComparison()) {
			fputs(LEFT_PARENTHESIS, stdout);
			FPUTS_UNICODE(result_text.c_str(), stdout);
			puts(RIGHT_PARENTHESIS);
		} else {
			PUTS_UNICODE(result_text.c_str());
		}
		if(goto_input) printf("\n");
	}

	b_busy = false;
}

void viewresult(Prefix *prefix = NULL) {
	setResult(prefix);
}

void result_display_updated() {
	if(expression_executed) setResult(NULL, false);
}
void result_format_updated() {
	if(expression_executed) setResult(NULL, false);
}
void result_action_executed() {
	if(expression_executed) {
		printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
		setResult(NULL, false);
	}
}
void result_prefix_changed(Prefix *prefix) {
	if(expression_executed) setResult(prefix, false);
}
void expression_format_updated() {
	if(expression_executed && !rpn_mode) execute_expression();
}

void on_abort_command() {
	pthread_cancel(command_thread);
	CALCULATOR->restoreState();
	CALCULATOR->clearBuffers();
	b_busy = false;
	command_aborted = true;
	command_thread_started = false;
}

void *command_proc(void *pipe) {

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);	
	FILE *command_pipe = (FILE*) pipe;
	
	while(true) {
	
		void *x = NULL;
		int command_type;
		fread(&command_type, sizeof(int), 1, command_pipe);
		fread(&x, sizeof(void*), 1, command_pipe);
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->factorize(evalops);
				}
				break;
			}
			case COMMAND_SIMPLIFY: {
				((MathStructure*) x)->simplify(evalops);
				break;
			}
		}
		b_busy = false;
		
	}
	return NULL;
}

void execute_command(int command_type, bool show_result) {

	b_busy = true;	
	command_aborted = false;
	CALCULATOR->saveState();

	if(!command_thread_started) {
		pthread_create(&command_thread, &command_thread_attr, command_proc, command_pipe_r);
		command_thread_started = true;
	}


	fwrite(&command_type, sizeof(int), 1, command_pipe_w);
	MathStructure *mfactor = new MathStructure(*mstruct);
	fwrite(&mfactor, sizeof(void*), 1, command_pipe_w);
	
	fflush(command_pipe_w);

	struct timespec rtime;
	rtime.tv_sec = 0;
	rtime.tv_nsec = 10000000;
	int i = 0;
	bool has_printed = false;
	while(b_busy && i < 75) {
		nanosleep(&rtime, NULL);
		i++;
	}
	i = 0;
	
	if(b_busy && !cfile) {
		if(!result_only) {
			switch(command_type) {
				case COMMAND_FACTORIZE: {				
					FPUTS_UNICODE(_("Factorizing (press Enter to abort)"), stdout);
					break;
				}
				case COMMAND_SIMPLIFY: {
					FPUTS_UNICODE(_("Simplifying (press Enter to abort)"), stdout);
					break;
				}
			}
			has_printed = true;
			fflush(stdout);
		}
	}
#ifdef HAVE_LIBREADLINE	
	int c = 0;
#else
	char c = 0;
#endif
	rtime.tv_nsec = 100000000;
	while(b_busy) {
		if(cfile) {
			nanosleep(&rtime, NULL);
		} else {
			FD_ZERO(&in_set);
			FD_SET(STDIN_FILENO, &in_set);
			if(select(FD_SETSIZE, &in_set, NULL, NULL, &timeout) > 0) {
#ifdef HAVE_LIBREADLINE
				if(use_readline) c = rl_read_key();
				else read(STDIN_FILENO, &c, 1);
#else
				read(STDIN_FILENO, &c, 1);
#endif			
				if(c == '\n') {
					on_abort_command();
				}
			} else {
				if(!result_only) {
					printf(".");
					fflush(stdout);
				}
				nanosleep(&rtime, NULL);
			}
		}
	}
	
	if(has_printed) printf("\n");

	b_busy = false;
	
	if(!command_aborted) {
		mstruct->unref();
		mstruct = mfactor;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				printops.allow_factorization = true;
				break;
			}
			case COMMAND_SIMPLIFY: {
				printops.allow_factorization = false;
				break;
			}
		}
		if(show_result) setResult(NULL, false);
	}

}


void execute_expression(bool goto_input, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index) {

	string str;
	bool do_bases = false, do_factors = false, do_fraction = false;
	if(!interactive_mode) goto_input = false;
	if(do_stack) {
	} else {
		str = expression_str;
		string from_str = str, to_str;
		bool b_units_saved = evalops.parse_options.units_enabled;
		evalops.parse_options.units_enabled = true;
		if(CALCULATOR->separateToExpression(from_str, to_str, evalops, true)) {
			if(equalsIgnoreCase(to_str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "hexadecimal", _("hexadecimal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_HEXADECIMAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", _("binary"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_BINARY;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", _("octal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_OCTAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "fraction", _("fraction"))) {
				str = from_str;
				do_fraction = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", _("factors"))) {
				str = from_str;
				do_factors = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", _("bases"))) {
				do_bases = true;
				str = from_str;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", _("optimal"))) {
				expression_str = from_str;
				AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
				evalops.auto_post_conversion = POST_CONVERSION_BEST;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.auto_post_conversion = save_auto_post_conversion;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", _("base"))) {
				expression_str = from_str;
				AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
				evalops.auto_post_conversion = POST_CONVERSION_BEST;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.auto_post_conversion = save_auto_post_conversion;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			}
		}		
		evalops.parse_options.units_enabled = b_units_saved;
	}
	
	expression_executed = true;

	b_busy = true;

	size_t stack_size = 0;
	
	CALCULATOR->resetExchangeRatesUsed();

	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 0, evalops, parsed_mstruct, NULL, !printops.negative_exponents);
	} else if(rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			if(f) CALCULATOR->calculateRPN(f, 0, evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, evalops, parsed_mstruct);
		} else {
			string str2 = CALCULATOR->unlocalizeExpression(str, evalops.parse_options);
			CALCULATOR->parseSigns(str2);
			remove_blank_ends(str2);
			if(str2.length() == 1) {
				do_mathoperation = true;
				switch(str2[0]) {
					case '^': {CALCULATOR->calculateRPN(OPERATION_RAISE, 0, evalops, parsed_mstruct); break;}
					case '+': {CALCULATOR->calculateRPN(OPERATION_ADD, 0, evalops, parsed_mstruct); break;}
					case '-': {CALCULATOR->calculateRPN(OPERATION_SUBTRACT, 0, evalops, parsed_mstruct); break;}
					case '*': {CALCULATOR->calculateRPN(OPERATION_MULTIPLY, 0, evalops, parsed_mstruct); break;}
					case '/': {CALCULATOR->calculateRPN(OPERATION_DIVIDE, 0, evalops, parsed_mstruct); break;}
					case '&': {CALCULATOR->calculateRPN(OPERATION_BITWISE_AND, 0, evalops, parsed_mstruct); break;}
					case '|': {CALCULATOR->calculateRPN(OPERATION_BITWISE_OR, 0, evalops, parsed_mstruct); break;}
					case '~': {CALCULATOR->calculateRPNBitwiseNot(0, evalops, parsed_mstruct); break;}
					case '!': {CALCULATOR->calculateRPN(CALCULATOR->f_factorial, 0, evalops, parsed_mstruct); break;}
					case '>': {CALCULATOR->calculateRPN(OPERATION_GREATER, 0, evalops, parsed_mstruct); break;}
					case '<': {CALCULATOR->calculateRPN(OPERATION_LESS, 0, evalops, parsed_mstruct); break;}
					case '=': {CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, evalops, parsed_mstruct); break;}
					default: {do_mathoperation = false;}
				}
			} else if(str2.length() == 2) {
				if(str2 == "**") {
					CALCULATOR->calculateRPN(OPERATION_RAISE, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!!") {
					CALCULATOR->calculateRPN(CALCULATOR->f_factorial2, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!=" || str == "=!" || str == "<>") {
					CALCULATOR->calculateRPN(OPERATION_NOT_EQUALS, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "<=" || str == "=<") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_LESS, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == ">=" || str == "=>") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_GREATER, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "==") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, evalops, parsed_mstruct);
					do_mathoperation = true;
				}
			}
			if(!do_mathoperation) {
				bool had_nonnum = false, test_function = true;
				int in_par = 0;
				for(size_t i = 0; i < str2.length(); i++) {
					if(is_in(NUMBERS, str2[i])) {
						if(!had_nonnum || in_par) {
							test_function = false;
							break;
						}
					} else if(str2[i] == '(') {
						if(in_par || !had_nonnum) {
							test_function = false;
							break;
						}
						in_par = i;
					} else if(str2[i] == ')') {
						if(i != str2.length() - 1) {
							test_function = false;
							break;
						}
					} else if(str2[i] == ' ') {
						if(!in_par) {
							test_function = false;
							break;
						}
					} else if(is_in(NOT_IN_NAMES, str2[i])) {
						test_function = false;
						break;
					} else {
						if(in_par) {
							test_function = false;
							break;
						}
						had_nonnum = true;
					}
				}
				f = NULL;
				if(test_function) {
					if(in_par) f = CALCULATOR->getActiveFunction(str2.substr(0, in_par));
					else f = CALCULATOR->getActiveFunction(str2);
				}
				if(f && f->minargs() > 1) {
					printf("Can only apply functions wich requires one argument on RPN stack.\n");
					f = NULL;
					return;
				}
				if(f && f->minargs() > 0) {
					do_mathoperation = true;
					CALCULATOR->calculateRPN(f, 0, evalops, parsed_mstruct);
				} else {
					CALCULATOR->RPNStackEnter(str2, 0, evalops, parsed_mstruct, NULL, !printops.negative_exponents);
				}
			}
		}
	} else {
		CALCULATOR->calculate(mstruct, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 0, evalops, parsed_mstruct, NULL, !printops.negative_exponents);
	}
	struct timespec rtime;
	rtime.tv_sec = 0;
	rtime.tv_nsec = 10000000;
	int i = 0;
	while(CALCULATOR->busy() && i < 75) {
		nanosleep(&rtime, NULL);
		i++;
	}
	i = 0;
	bool has_printed = false;
	if(CALCULATOR->busy() && !cfile) {
		if(!result_only) {
			FPUTS_UNICODE(_("Calculating (press Enter to abort)"), stdout);
			fflush(stdout);
			has_printed = true;
		}
	}
#ifdef HAVE_LIBREADLINE		
	int c = 0;
#else
	char c = 0;
#endif
	rtime.tv_nsec = 100000000;
	while(CALCULATOR->busy()) {
		if(cfile) {
			nanosleep(&rtime, NULL);
		} else {
			FD_ZERO(&in_set);
			FD_SET(STDIN_FILENO, &in_set);
			if(select(FD_SETSIZE, &in_set, NULL, NULL, &timeout) > 0) {
#ifdef HAVE_LIBREADLINE		
				if(use_readline) c = rl_read_key();
				else read(STDIN_FILENO, &c, 1);
#else
				read(STDIN_FILENO, &c, 1);
#endif			
				if(c == '\n') {
					CALCULATOR->abort();
				}
			} else {
				if(!result_only) {
					printf(".");
					fflush(stdout);
				}
				nanosleep(&rtime, NULL);
			}
		}
	}
	if(has_printed) printf("\n");
	b_busy = false;	

	if(rpn_mode && (!do_stack || stack_index == 0)) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
	}
	
	if(!do_mathoperation && check_exchange_rates()) {
		execute_expression(goto_input, do_mathoperation, op, f, rpn_mode, do_stack ? stack_index : 0);
		return;
	}
	
	if(do_factors) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			execute_command(COMMAND_FACTORIZE, false);
			mstruct = save_mstruct;
		} else {
			execute_command(COMMAND_FACTORIZE, false);
		}
	}
	
	//update "ans" variables
	if(!do_stack || stack_index == 0) {
		vans[4]->set(vans[3]->get());
		vans[3]->set(vans[2]->get());
		vans[2]->set(vans[1]->get());
		vans[1]->set(vans[0]->get());
		vans[0]->set(*mstruct);
	}

	if(do_stack && stack_index > 0) {
	} else if(rpn_mode && do_mathoperation) {
		result_text = _("RPN Operation");
	} else {
		result_text = str;
	}
	printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	if(rpn_mode && (!do_stack || stack_index == 0)) {
		if(CALCULATOR->RPNStackSize() < stack_size) {
			RPNRegisterRemoved(1);
		} else if(CALCULATOR->RPNStackSize() > stack_size) {
			RPNRegisterAdded("");
		}
	}
	
	if(do_bases) {
		int save_base = printops.base;
		printops.base = BASE_BINARY;
		setResult(NULL, (!do_stack || stack_index == 0), false, do_stack ? stack_index : 0, false, true);
		string base_str = result_text;
		base_str += " = ";
		printops.base = BASE_OCTAL;
		setResult(NULL, false, false, do_stack ? stack_index : 0, false, true);
		base_str += result_text;
		base_str += " = ";
		printops.base = BASE_DECIMAL;
		setResult(NULL, false, false, do_stack ? stack_index : 0, false, true);
		base_str += result_text;					
		base_str += " = ";
		printops.base = BASE_HEXADECIMAL;
		setResult(NULL, false, false, do_stack ? stack_index : 0, false, true);
		base_str += result_text;
		if(has_printed) printf("\n");
		if(goto_input) printf("\n  ");
		if(!result_only) {
			FPUTS_UNICODE(parsed_text.c_str(), stdout);
			if(!(*printops.is_approximate) && !mstruct->isApproximate()) {
				printf(" = ");
			} else {
				if(printops.use_unicode_signs) {
					printf(" " SIGN_ALMOST_EQUAL " ");
				} else {
					printf(" = %s ", _("approx."));
				}
			}
		}
		PUTS_UNICODE(base_str.c_str());
		printops.base = save_base;
		result_text = base_str;
		if(goto_input) printf("\n");
	} else if(do_fraction) {
		NumberFractionFormat save_format = printops.number_fraction_format;
		if(((!do_stack || stack_index == 0) && mstruct->isNumber()) || (do_stack && stack_index != 0 && CALCULATOR->getRPNRegister(stack_index + 1)->isNumber())) printops.number_fraction_format = FRACTION_COMBINED;
		else printops.number_fraction_format = FRACTION_FRACTIONAL;
		setResult(NULL, (!do_stack || stack_index == 0), goto_input, do_stack ? stack_index : 0);
		printops.number_fraction_format = save_format;
	} else {
		setResult(NULL, (!do_stack || stack_index == 0), goto_input, do_stack ? stack_index : 0);
	}

}

/*
	save mode to file
*/
bool save_mode() {
	return save_preferences(true);
}

/*
	remember current mode
*/
void set_saved_mode() {
	saved_precision = CALCULATOR->getPrecision();
	saved_printops = printops;
	saved_printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	saved_evalops = evalops;
	saved_assumption_type = CALCULATOR->defaultAssumptions()->type();
	saved_assumption_sign = CALCULATOR->defaultAssumptions()->sign();
}


void load_preferences() {

	printops.is_approximate = new bool(false);
	printops.prefix = NULL;
	printops.use_min_decimals = false;
	printops.use_denominator_prefix = true;
	printops.min_decimals = 0;
	printops.use_max_decimals = false;
	printops.max_decimals = 2;
	printops.base = 10;
	printops.min_exp = EXP_PRECISION;
	printops.negative_exponents = false;
	printops.sort_options.minus_last = true;
	printops.indicate_infinite_series = false;
	printops.show_ending_zeroes = false;
	printops.round_halfway_to_even = false;
	printops.number_fraction_format = FRACTION_DECIMAL;
	printops.abbreviate_names = true;
	printops.use_unicode_signs = false;
	printops.use_unit_prefixes = true;
	printops.spacious = true;
	printops.short_multiplication = true;
	printops.limit_implicit_multiplication = false;
	printops.place_units_separately = true;
	printops.use_all_prefixes = false;
	printops.excessive_parenthesis = false;
	printops.allow_non_usable = false;
	printops.lower_case_numbers = false;
	printops.lower_case_e = false;
	printops.base_display = BASE_DISPLAY_NORMAL;
	printops.division_sign = DIVISION_SIGN_SLASH;
	printops.multiplication_sign = MULTIPLICATION_SIGN_ASTERISK;
	printops.allow_factorization = false;
	printops.spell_out_logical_operators = true;
	
	evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	evalops.approximation = APPROXIMATION_TRY_EXACT;
	evalops.sync_units = true;
	evalops.structuring = STRUCTURING_SIMPLIFY;
	evalops.parse_options.unknowns_enabled = false;
	evalops.parse_options.read_precision = DONT_READ_PRECISION;
	evalops.parse_options.base = BASE_DECIMAL;
	evalops.allow_complex = true;
	evalops.allow_infinite = true;
	evalops.auto_post_conversion = POST_CONVERSION_NONE;
	evalops.assume_denominators_nonzero = true;
	evalops.warn_about_denominators_assumed_nonzero = true;
	evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;

	rpn_mode = false;
	
	save_mode_on_exit = true;
	save_defs_on_exit = true;	
	fetch_exchange_rates_at_startup = false;
	auto_update_exchange_rates = -1;
	first_time = false;

	FILE *file = NULL;
#ifdef HAVE_LIBREADLINE	
	gchar *historyfile = g_build_filename(getLocalDir().c_str(), "qalc.history", NULL);
	gchar *oldhistoryfile = NULL;
#endif	
	gchar *gstr_oldfile = NULL;	
	gchar *gstr_file = g_build_filename(getLocalDir().c_str(), "qalc.cfg", NULL);
	file = fopen(gstr_file, "r");
	if(!file) {
		gstr_oldfile = g_build_filename(getOldLocalDir().c_str(), "qalc.cfg", NULL);
		file = fopen(gstr_oldfile, "r");
		if(!file) {
			g_free(gstr_file);
			g_free(gstr_oldfile);
			g_free(historyfile);
			first_time = true;
			set_saved_mode();
			return;
		}
#ifdef HAVE_LIBREADLINE
		oldhistoryfile = g_build_filename(getOldLocalDir().c_str(), "qalc.history", NULL);
#endif
		mkdir(getLocalDir().c_str(), S_IRWXU);
	}
	
#ifdef HAVE_LIBREADLINE
	stifle_history(100);
	if(oldhistoryfile) {
		read_history(oldhistoryfile);
		move_file(oldhistoryfile, historyfile);
		g_free(oldhistoryfile);
	} else {
		read_history(historyfile);		
	}
	g_free(historyfile);
#endif

	
	int version_numbers[] = {0, 9, 8};
	
	if(file) {
		char line[10000];
		string stmp, svalue, svar;
		size_t i;
		int v;
		while(true) {
			if(fgets(line, 10000, file) == NULL)
				break;
			stmp = line;
			remove_blank_ends(stmp);
			if((i = stmp.find_first_of("=")) != string::npos) {
				svar = stmp.substr(0, i);
				remove_blank_ends(svar);
				svalue = stmp.substr(i + 1, stmp.length() - (i + 1));
				remove_blank_ends(svalue);
				v = s2i(svalue);
				if(svar == "version") {
					parse_qalculate_version(svalue, version_numbers);
				} else if(svar == "save_mode_on_exit") {
					save_mode_on_exit = v;
				} else if(svar == "save_definitions_on_exit") {
					save_defs_on_exit = v;
				} else if(svar == "fetch_exchange_rates_at_startup") {
					if(auto_update_exchange_rates < 0 && v) auto_update_exchange_rates = 1;
				} else if(svar == "auto_update_exchange_rates") {
					auto_update_exchange_rates = v;
				} else if(svar == "min_deci") {
					printops.min_decimals = v;
				} else if(svar == "use_min_deci") {
					printops.use_min_decimals = v;
				} else if(svar == "max_deci") {
					printops.max_decimals = v;
				} else if(svar == "use_max_deci") {
					printops.use_max_decimals = v;					
				} else if(svar == "precision") {
					CALCULATOR->setPrecision(v);
				} else if(svar == "min_exp") {
					printops.min_exp = v;
				} else if(svar == "negative_exponents") {
					printops.negative_exponents = v;
				} else if(svar == "sort_minus_last") {
					printops.sort_options.minus_last = v;
				} else if(svar == "spacious") {
					printops.spacious = v;	
				} else if(svar == "excessive_parenthesis") {
					printops.excessive_parenthesis = v;
				} else if(svar == "short_multiplication") {
					printops.short_multiplication = v;
				} else if(svar == "limit_implicit_multiplication") {
					evalops.parse_options.limit_implicit_multiplication = v;
					printops.limit_implicit_multiplication = v;
				} else if(svar == "parsing_mode") {
					if(v >= PARSING_MODE_ADAPTIVE && v <= PARSING_MODE_CONVENTIONAL) {
						evalops.parse_options.parsing_mode = (ParsingMode) v;
					}
				} else if(svar == "place_units_separately") {
					printops.place_units_separately = v;
				} else if(svar == "use_prefixes") {
					printops.use_unit_prefixes = v;
				} else if(svar == "use_prefixes_for_all_units") {
					printops.use_prefixes_for_all_units = v;
				} else if(svar == "use_prefixes_for_currencies") {
					printops.use_prefixes_for_currencies = v;
				} else if(svar == "number_fraction_format") {
					if(v >= FRACTION_DECIMAL && v <= FRACTION_COMBINED) {
						printops.number_fraction_format = (NumberFractionFormat) v;
					}
				} else if(svar == "number_base") {
					printops.base = v;
				} else if(svar == "number_base_expression") {
					evalops.parse_options.base = v;	
				} else if(svar == "read_precision") {
					if(v >= DONT_READ_PRECISION && v <= READ_PRECISION_WHEN_DECIMALS) {
						evalops.parse_options.read_precision = (ReadPrecisionMode) v;
					}
				} else if(svar == "assume_denominators_nonzero") {
					if(version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
						v = true;
					}
					evalops.assume_denominators_nonzero = v;
				} else if(svar == "warn_about_denominators_assumed_nonzero") {
					evalops.warn_about_denominators_assumed_nonzero = v;
				} else if(svar == "structuring") {
					if(v >= STRUCTURING_NONE && v <= STRUCTURING_FACTORIZE) {
						evalops.structuring = (StructuringMode) v;
						printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
					}
				} else if(svar == "angle_unit") {
					if(version_numbers[0] == 0 && (version_numbers[1] < 7 || (version_numbers[1] == 7 && version_numbers[2] == 0))) {
						v++;
					}
					if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_GRADIANS) {
						evalops.parse_options.angle_unit = (AngleUnit) v;
					}
				} else if(svar == "functions_enabled") {
					evalops.parse_options.functions_enabled = v;
				} else if(svar == "variables_enabled") {
					evalops.parse_options.variables_enabled = v;
				} else if(svar == "calculate_variables") {
					evalops.calculate_variables = v;
				} else if(svar == "calculate_functions") {
					evalops.calculate_functions = v;
				} else if(svar == "sync_units") {
					evalops.sync_units = v;
				} else if(svar == "unknownvariables_enabled") {
					evalops.parse_options.unknowns_enabled = v;
				} else if(svar == "units_enabled") {
					evalops.parse_options.units_enabled = v;
				} else if(svar == "allow_complex") {
					evalops.allow_complex = v;
				} else if(svar == "allow_infinite") {
					evalops.allow_infinite = v;
				} else if(svar == "use_short_units") {
					printops.abbreviate_names = v;
				} else if(svar == "abbreviate_names") {
					printops.abbreviate_names = v;	
				} else if(svar == "all_prefixes_enabled") {
				 	printops.use_all_prefixes = v;
				} else if(svar == "denominator_prefix_enabled") {
					printops.use_denominator_prefix = v;
				} else if(svar == "auto_post_conversion") {
					if(v >= POST_CONVERSION_NONE && v <= POST_CONVERSION_BASE) {
						evalops.auto_post_conversion = (AutoPostConversion) v;
					}
				} else if(svar == "use_unicode_signs") {
					printops.use_unicode_signs = v;	
				} else if(svar == "lower_case_numbers") {
					printops.lower_case_numbers = v;
				} else if(svar == "lower_case_e") {
					printops.lower_case_e = v;
				} else if(svar == "base_display") {
					if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_ALTERNATIVE) printops.base_display = (BaseDisplay) v;
				} else if(svar == "spell_out_logical_operators") {
						printops.spell_out_logical_operators = v;
				} else if(svar == "dot_as_separator") {
					evalops.parse_options.dot_as_separator = v;
				} else if(svar == "comma_as_separator") {
					evalops.parse_options.comma_as_separator = v;
					if(CALCULATOR->getDecimalPoint() != ",") {						
						CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
					}
				} else if(svar == "multiplication_sign") {
					if(v >= MULTIPLICATION_SIGN_ASTERISK && v <= MULTIPLICATION_SIGN_X) printops.multiplication_sign = (MultiplicationSign) v;
				} else if(svar == "division_sign") {
					if(v >= DIVISION_SIGN_SLASH && v <= DIVISION_SIGN_DIVISION) printops.division_sign = (DivisionSign) v;
				} else if(svar == "indicate_infinite_series") {
					printops.indicate_infinite_series = v;
				} else if(svar == "show_ending_zeroes") {
					printops.show_ending_zeroes = v;
				} else if(svar == "round_halfway_to_even") {
					printops.round_halfway_to_even = v;	
				} else if(svar == "approximation") {
					if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {					
						evalops.approximation = (ApproximationMode) v;
					}
				} else if(svar == "in_rpn_mode") {
					rpn_mode = v;
				} else if(svar == "rpn_syntax") {
					evalops.parse_options.rpn = v;
				} else if(svar == "default_assumption_type") {
					if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_INTEGER) {
						if(v == ASSUMPTION_TYPE_NONE && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
							v = ASSUMPTION_TYPE_NONMATRIX;
						}
						CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
					}
				} else if(svar == "default_assumption_sign") {
					if(v >= ASSUMPTION_SIGN_UNKNOWN && v <= ASSUMPTION_SIGN_NONZERO) {
						if(v == ASSUMPTION_SIGN_NONZERO && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
							v = ASSUMPTION_SIGN_UNKNOWN;
						}
						CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
					}
				}
			}
		}
		fclose(file);
		if(gstr_oldfile) {			
			move_file(gstr_oldfile, gstr_file);
			g_free(gstr_oldfile);
		}
	} else {
		first_time = true;
	}
	g_free(gstr_file);
	//remember start mode for when we save preferences
	set_saved_mode();
}

/*
	save preferences to ~/.config/qalculate/qalc.cfg
	set mode to true to save current calculator mode
*/
bool save_preferences(bool mode)
{
	FILE *file = NULL;
	mkdir(getLocalDir().c_str(), S_IRWXU);
#ifdef HAVE_LIBREADLINE	
	gchar *historyfile = g_build_filename(getLocalDir().c_str(), "qalc.history", NULL);
	write_history(historyfile);
	g_free(historyfile);
#endif	
	gchar *filename = g_build_filename(getLocalDir().c_str(), "qalc.cfg", NULL);
	file = fopen(filename, "w+");
	g_free(filename);
	if(file == NULL) {
		fprintf(stderr, _("Couldn't write preferences to\n%s"), filename);
		return false;
	}
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", VERSION);	
	fprintf(file, "save_mode_on_exit=%i\n", save_mode_on_exit);
	fprintf(file, "save_definitions_on_exit=%i\n", save_defs_on_exit);
	fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
	fprintf(file, "spacious=%i\n", printops.spacious);
	fprintf(file, "excessive_parenthesis=%i\n", printops.excessive_parenthesis);
	fprintf(file, "short_multiplication=%i\n", printops.short_multiplication);
	fprintf(file, "use_unicode_signs=%i\n", printops.use_unicode_signs);
	fprintf(file, "lower_case_numbers=%i\n", printops.lower_case_numbers);
	fprintf(file, "lower_case_e=%i\n", printops.lower_case_e);
	fprintf(file, "base_display=%i\n", printops.base_display);
	fprintf(file, "spell_out_logical_operators=%i\n", printops.spell_out_logical_operators);
	fprintf(file, "dot_as_separator=%i\n", evalops.parse_options.dot_as_separator);
	fprintf(file, "comma_as_separator=%i\n", evalops.parse_options.comma_as_separator);
	fprintf(file, "multiplication_sign=%i\n", printops.multiplication_sign);
	fprintf(file, "division_sign=%i\n", printops.division_sign);
	if(mode)
		set_saved_mode();
	fprintf(file, "\n[Mode]\n");
	fprintf(file, "min_deci=%i\n", saved_printops.min_decimals);
	fprintf(file, "use_min_deci=%i\n", saved_printops.use_min_decimals);
	fprintf(file, "max_deci=%i\n", saved_printops.max_decimals);
	fprintf(file, "use_max_deci=%i\n", saved_printops.use_max_decimals);	
	fprintf(file, "precision=%i\n", saved_precision);
	fprintf(file, "min_exp=%i\n", saved_printops.min_exp);
	fprintf(file, "negative_exponents=%i\n", saved_printops.negative_exponents);
	fprintf(file, "sort_minus_last=%i\n", saved_printops.sort_options.minus_last);
	fprintf(file, "number_fraction_format=%i\n", saved_printops.number_fraction_format);	
	fprintf(file, "use_prefixes=%i\n", saved_printops.use_unit_prefixes);	
	fprintf(file, "use_prefixes_for_all_units=%i\n", saved_printops.use_prefixes_for_all_units);
	fprintf(file, "use_prefixes_for_currencies=%i\n", saved_printops.use_prefixes_for_currencies);
	fprintf(file, "abbreviate_names=%i\n", saved_printops.abbreviate_names);
	fprintf(file, "all_prefixes_enabled=%i\n", saved_printops.use_all_prefixes);
	fprintf(file, "denominator_prefix_enabled=%i\n", saved_printops.use_denominator_prefix);
	fprintf(file, "place_units_separately=%i\n", saved_printops.place_units_separately);
	fprintf(file, "auto_post_conversion=%i\n", saved_evalops.auto_post_conversion);	
	fprintf(file, "number_base=%i\n", saved_printops.base);
	fprintf(file, "number_base_expression=%i\n", saved_evalops.parse_options.base);
	fprintf(file, "read_precision=%i\n", saved_evalops.parse_options.read_precision);
	fprintf(file, "assume_denominators_nonzero=%i\n", saved_evalops.assume_denominators_nonzero);
	fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", saved_evalops.warn_about_denominators_assumed_nonzero);
	fprintf(file, "structuring=%i\n", saved_evalops.structuring);
	fprintf(file, "angle_unit=%i\n", saved_evalops.parse_options.angle_unit);
	fprintf(file, "functions_enabled=%i\n", saved_evalops.parse_options.functions_enabled);
	fprintf(file, "variables_enabled=%i\n", saved_evalops.parse_options.variables_enabled);
	fprintf(file, "calculate_variables=%i\n", saved_evalops.calculate_variables);	
	fprintf(file, "calculate_functions=%i\n", saved_evalops.calculate_functions);	
	fprintf(file, "sync_units=%i\n", saved_evalops.sync_units);
	fprintf(file, "unknownvariables_enabled=%i\n", saved_evalops.parse_options.unknowns_enabled);
	fprintf(file, "units_enabled=%i\n", saved_evalops.parse_options.units_enabled);
	fprintf(file, "allow_complex=%i\n", saved_evalops.allow_complex);
	fprintf(file, "allow_infinite=%i\n", saved_evalops.allow_infinite);
	fprintf(file, "indicate_infinite_series=%i\n", saved_printops.indicate_infinite_series);
	fprintf(file, "show_ending_zeroes=%i\n", saved_printops.show_ending_zeroes);
	fprintf(file, "round_halfway_to_even=%i\n", saved_printops.round_halfway_to_even);
	fprintf(file, "approximation=%i\n", saved_evalops.approximation);	
	fprintf(file, "in_rpn_mode=%i\n", rpn_mode);
	fprintf(file, "rpn_syntax=%i\n", saved_evalops.parse_options.rpn);
	fprintf(file, "limit_implicit_multiplication=%i\n", evalops.parse_options.limit_implicit_multiplication);
	fprintf(file, "parsing_mode=%i\n", evalops.parse_options.parsing_mode);
	fprintf(file, "default_assumption_type=%i\n", CALCULATOR->defaultAssumptions()->type());
	fprintf(file, "default_assumption_sign=%i\n", CALCULATOR->defaultAssumptions()->sign());
	
	fclose(file);
	return true;
	
}

/*
	save definitions to ~/.qalculate/qalculate.cfg
	the hard work is done in the Calculator class
*/
bool save_defs() {
	if(!CALCULATOR->saveDefinitions()) {
		PUTS_UNICODE(_("Couldn't write definitions"));
		return false;
	}
	return true;
}



