/*
    Qalculate (CLI)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <list>
#ifdef HAVE_LIBREADLINE
#	include <readline/readline.h>
#	include <readline/history.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

class ViewThread : public Thread {
protected:
	virtual void run();
};

class CommandThread : public Thread {
protected:
	virtual void run();
};

MathStructure *mstruct, *parsed_mstruct;
KnownVariable *vans[5];
string result_text, parsed_text;
bool load_global_defs, fetch_exchange_rates_at_startup, first_time, save_mode_on_exit, save_defs_on_exit;
int auto_update_exchange_rates;
PrintOptions printops, saved_printops;
EvaluationOptions evalops, saved_evalops;
Number saved_custom_output_base, saved_custom_input_base;
AssumptionType saved_assumption_type;
AssumptionSign saved_assumption_sign;
int saved_precision;
bool saved_interval, saved_adaptive_interval_display, saved_variable_units_enabled;
bool adaptive_interval_display;
Thread *view_thread, *command_thread;
bool command_aborted = false;
volatile bool b_busy = false;
string expression_str;
bool expression_executed = false;
bool avoid_recalculation = false;
bool hide_parse_errors = false;
bool rpn_mode = false, saved_rpn_mode = false;
bool caret_as_xor = false, saved_caret_as_xor = false;
bool use_readline = true;
bool interactive_mode;
bool ask_questions;
bool canfetch = true;
bool programmers_mode = false;
int b_decimal_comma = -1;
long int i_maxtime = 0;
struct timeval t_end;

bool automatic_fraction = false;

bool ignore_locale = false;

bool result_only;

static char buffer[1000];

void setResult(Prefix *prefix = NULL, bool update_parse = false, bool goto_input = true, size_t stack_index = 0, bool register_moved = false, bool noprint = false);
void execute_expression(bool goto_input = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, bool check_exrates = true);
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
void expression_format_updated(bool reparse);
void expression_calculation_updated();
bool display_errors(bool goto_input = false, int cols = 0);
void replace_quotation_marks(string &result_text);

FILE *cfile;

enum {
	COMMAND_FACTORIZE,
	COMMAND_EXPAND,
	COMMAND_EXPAND_PARTIAL_FRACTIONS,
	COMMAND_EVAL
};

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z))

bool contains_unicode_char(const char *str) {
	for(int i = strlen(str) - 1; i >= 0; i--) {
		if(str[i] < 0) return true;
	}
	return false;
}

#define PUTS_UNICODE(x)				if(printops.use_unicode_signs || !contains_unicode_char(x)) {puts(x);} else {char *gstr = locale_from_utf8(x); if(gstr) {puts(gstr); free(gstr);} else {puts(x);}}
#define FPUTS_UNICODE(x, y)			if(printops.use_unicode_signs || !contains_unicode_char(x)) {fputs(x, y);} else {char *gstr = locale_from_utf8(x); if(gstr) {fputs(gstr, y); free(gstr);} else {fputs(x, y);}}

void update_message_print_options() {
	PrintOptions message_printoptions = printops;
	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.show_ending_zeroes = false;
	if(printops.min_exp < -10 || printops.min_exp > 10 || ((printops.min_exp == EXP_PRECISION || printops.min_exp == EXP_NONE) && PRECISION > 10)) message_printoptions.min_exp = 10;
	else if(printops.min_exp == EXP_NONE) message_printoptions.min_exp = EXP_PRECISION;
	if(PRECISION > 10) {
		message_printoptions.use_max_decimals = true;
		message_printoptions.max_decimals = 10;
	}
	CALCULATOR->setMessagePrintOptions(message_printoptions);
}

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
	if((str1[0] < 0 && str1.length() > 1) || (str2[0] < 0 && strlen(str2) > 1)) {
		size_t iu1 = 1, iu2 = 1;
		if(str1[0] < 0) {
			while(iu1 < str1.length() && str1[iu1] < 0) {
				iu1++;
			}
		}
		if(str2[0] < 0) {
			while(iu2 < strlen(str2) && str2[iu2] < 0) {
				iu2++;
			}
		}
		bool isequal = (iu1 == iu2);
		if(isequal) {
			for(size_t i = 0; i < iu1; i++) {
				if(str1[i] != str2[i]) {
					isequal = false;
					break;
				}
			}
		}
		if(!isequal) {
			char *gstr1 = utf8_strdown(str1.c_str(), iu1);
			char *gstr2 = utf8_strdown(str2, iu2);
			if(!gstr1 || !gstr2) return false;
			bool b = strcmp(gstr1, gstr2) == 0;
			free(gstr1);
			free(gstr2);
			return b;
		}
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
		if(!fgets(buffer, 1000, stdin)) return false;
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

void set_assumption(const string &str, bool last_of_two = false) {
	if(EQUALS_IGNORECASE_AND_LOCAL(str, "unknown", _("unknown")) || str == "0") {
		if(!last_of_two) {
			CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_UNKNOWN);
		} else {
			CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NUMBER);
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "real", _("real"))) {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_REAL);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "number", _("number")) || str == "num") {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_NUMBER);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rational", _("rational")) || str == "rat") {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_RATIONAL);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "integer", _("integer")) || str == "int") {
		CALCULATOR->defaultAssumptions()->setType(ASSUMPTION_TYPE_INTEGER);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-zero", _("non-zero")) || str == "nz") {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONZERO);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "positive", _("positive")) || str == "pos") {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_POSITIVE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-negative", _("non-negative")) || str == "nneg") {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "negative", _("negative")) || str == "neg") {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NEGATIVE);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "non-positive", _("non-positive")) || str == "npos") {
		CALCULATOR->defaultAssumptions()->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
	} else {
		PUTS_UNICODE(_("Unrecognized assumption."));
		return;
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
		bool b_match;
		size_t l = strlen(text);
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			if(CALCULATOR->functions[i]->isActive()) {
				ExpressionItem *item = CALCULATOR->functions[i];
				const ExpressionName *ename = NULL;
				b_match = false;
				for(size_t name_i = 1; name_i <= item->countNames() && !b_match; name_i++) {
					ename = &item->getName(name_i);
					if(ename && l <= ename->name.length()) {
						b_match = true;
						for(size_t i2 = 0; i2 < l; i2++) {
							if(ename->name[i2] != text[i2]) {
								b_match = false;
								break;
							}
						}
					}
				}
				if(b_match && ename) {
					if(ename->completion_only) ename = &item->preferredInputName(ename->abbreviation, printops.use_unicode_signs);
					matches.push_back(&ename->name);
				}
			}
		}
		for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			if(CALCULATOR->variables[i]->isActive()) {
				ExpressionItem *item = CALCULATOR->variables[i];
				const ExpressionName *ename = NULL;
				b_match = false;
				for(size_t name_i = 1; name_i <= item->countNames() && !b_match; name_i++) {
					ename = &item->getName(name_i);
					if(ename && l <= ename->name.length()) {
						b_match = true;
						for(size_t i2 = 0; i2 < l; i2++) {
							if(ename->name[i2] != text[i2]) {
								b_match = false;
								break;
							}
						}
					}
				}
				if(b_match && ename) {
					if(ename->completion_only) ename = &item->preferredInputName(ename->abbreviation, printops.use_unicode_signs);
					matches.push_back(&ename->name);
				}
			}
		}
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i]->isActive() && CALCULATOR->units[i]->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				ExpressionItem *item = CALCULATOR->units[i];
				const ExpressionName *ename = NULL;
				b_match = false;
				for(size_t name_i = 1; name_i <= item->countNames() && !b_match; name_i++) {
					ename = &item->getName(name_i);
					if(ename && l <= ename->name.length()) {
						b_match = true;
						for(size_t i2 = 0; i2 < l; i2++) {
							if(ename->name[i2] != text[i2]) {
								b_match = false;
								break;
							}
						}
					}
				}
				if(b_match && ename) {
					if(ename->completion_only) ename = &item->preferredInputName(ename->abbreviation, printops.use_unicode_signs);
					matches.push_back(&ename->name);
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
	CALCULATOR->abort();
	if(enable_unicode >= 0) {
		printops.use_unicode_signs = !enable_unicode;
	} 
	if(interactive_mode) {
		if(save_mode_on_exit) {
			save_mode();
		} else {
			save_preferences();
		}
		if(save_defs_on_exit) {
			save_defs();
		}
	}
	if(!view_thread->write(NULL)) view_thread->cancel();
	if(command_thread->running && (!command_thread->write(0) || !command_thread->write(NULL))) command_thread->cancel();
	CALCULATOR->terminateThreads();
}

#ifdef HAVE_LIBREADLINE
int rlcom_tab(int, int) {
	rl_complete_internal('!');
	return 0;	
}
#endif

int countRows(const char *str, int cols) {
	int l = strlen(str);
	if(l == 0) return 1;
	int r = 1, c = 0;
	for(int i = 0; i < l; i++) {
		if(str[i] == '\033') {
			do {
				i++;
			} while(i < l && str[i] != 'm');
			if(i >= l) break;
		}
		if(str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
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
	}
	return r;
}

int addLineBreaks(string &str, int cols, bool expr = false, size_t indent = 0, size_t result_start = 0) {
	if(cols <= 0) return 1;
	int r = 1;
	size_t c = 0;
	size_t lb_point = string::npos;
	size_t or_point = string::npos;
	int b_or = 0;
	if(expr && str.find("||") != string::npos) b_or = 2;
	else if(expr && str.find(_("or")) != string::npos) b_or = 1;
	for(size_t i = 0; i < str.length(); i++) {
		if(r != 1 && c == indent) {
			if(str[i] == ' ') {
				str.erase(i, 1);
				if(i < result_start) result_start--;
				if(i >= str.length()) break;
			} else if(expr && printops.use_unicode_signs && printops.digit_grouping != DIGIT_GROUPING_NONE && str[i] <= 0 && (unsigned char) str[i] >= 0xC0) {
				size_t l = 1;
				while(i + l < str.length() && str[i + l] <= 0 && (unsigned char) str[i + l] < 0xC0) l++;
				if(str.substr(i, l) == " ") {
					str.erase(i, l);
					if(i < result_start) result_start--;
					if(i >= str.length()) break;
				}
			}
		}
		if(str[i] == '\033') {
			do {
				i++;
			} while(i < str.length() && str[i] != 'm');
			if(i >= str.length()) break;
		}
		if(str[i] > 0 || (unsigned char) str[i] >= 0xC0) {
			if(str[i] == '\n') {
				r++;
				c = 0;
				lb_point = string::npos;
			} else {
				if(c > indent) {
					if(is_in(" \t", str[i])) {
						if(!expr || printops.digit_grouping == DIGIT_GROUPING_NONE || i + 1 == str.length() || is_not_in("0123456789", str[i + 1]) || is_not_in("0123456789", str[i - 1])) {
							if(expr || (c - indent) > (cols - indent) / 2) lb_point = i;
							if(i > result_start && b_or == 1 && str.length() > i + strlen("or") + 2 && str.substr(i + 1, strlen(_("or"))) == _("or") && str[i + strlen(_("or")) + 1] == ' ') or_point = i + strlen(_("or")) + 1;
							else if(i > result_start && b_or == 2 && str.length() > i + 2 + 2 && str.substr(i + 1, 2) == "||" && str[i + 2 + 1] == ' ') or_point = i + 2 + 1;
						}
					} else if(expr && !printops.spacious && (c - indent) > (cols - indent) / 2 && c < (size_t) cols && is_in("+-*", str[i]) && i + 1 != str.length() && str[i - 1] != '^' && str[i - 1] != ' ' && str[i - 1] != '=' && is_not_in(" \t.;,", str[i + 1])) {
						lb_point = i + 1;
					}
				}
				if(c == (size_t) cols || or_point != string::npos) {
					if(or_point != string::npos) lb_point = or_point;
					if(lb_point == string::npos) {
						if(expr && printops.digit_grouping != DIGIT_GROUPING_NONE) {
							if(i > 3 && str[i] <= '9' && str[i] >= '0' && str[i - 1] <= '9' && str[i - 1] >= '0') {
								if(str[i - 2] == ' ' && str[i - 3] <= '9' && str[i - 3] >= '0') i -= 2;
								else if(str[i - 3] == ' ' && str[i - 4] <= '9' && str[i - 4] >= '0') i -= 3;
								else if((str[i - 2] == '.' || str[i - 2] == ',') && str[i - 3] <= '9' && str[i - 3] >= '0') i--;
								else if((str[i - 3] == '.' || str[i - 3] == ',') && str[i - 4] <= '9' && str[i - 4] >= '0') i -= 2;
								else if(printops.use_unicode_signs && i > 6) {
									size_t i2 = str.find(" ", i - 6);
									if(i2 != string::npos && i2 > 0 && i2 < i && str[i2 - 1] <= '9' && str[i2 - 1] >= '0') {
										i = i2;
									}
								}
							} else if(i > 4 && (str[i] == '.' || str[i] == ',') && str[i - 1] <= '9' && str[i - 1] >= '0' && str[i - 4] == str[i] && str[i - 5] <= '9' && str[i - 5] >= '0') {
								i -= 3;
							}
						}
						str.insert(i, "\n");
						result_start++;
						for(size_t i2 = 0; i2 < indent; i2++) {
							i++;
							if(i < result_start) result_start++;
							str.insert(i, " ");
						}
						c = indent;
					} else if(str[lb_point] == ' ' || str[lb_point] == '\t') {
						str[lb_point] = '\n';
						for(size_t i2 = 0; i2 < indent; i2++) {
							lb_point++;
							if(i < result_start) result_start++;
							str.insert(lb_point, " ");
						}
						i = lb_point;
						c = indent;
					} else {
						str.insert(lb_point, "\n");
						result_start++;
						for(size_t i2 = 0; i2 < indent; i2++) {
							lb_point++;
							if(i < result_start) result_start++;
							str.insert(lb_point, " ");
						}
						i = lb_point;
						c = indent;
					}
					lb_point = string::npos;
					or_point = string::npos;
					r++;
				} else {
					if(str[i] == '\t') c += 8;
					else c++;
				}
			}
		} else if(expr && !printops.spacious && printops.use_unicode_signs && i + 1 < str.length() && (str[i + 1] > 0 || (unsigned char) str[i + 1] >= 0xC0)) {
			if(c > indent && (c - indent) > (cols - indent) / 2 && is_not_in(" \t.;,", str[i + 1])) {
				size_t index = i;
				while(index > 0 && str[index - 1] <= 0) {
					index--;
					if((unsigned char) str[index] >= 0xC0) break;
				}
				if(index > 0 && str[index - 1] != '^' && str[index - 1] != ' ' && str[index - 1] != '=') {
					string unichar = str.substr(index, i - index + 1);
					if(unichar == SIGN_MULTIPLICATION || unichar == SIGN_MULTIDOT || unichar == SIGN_MIDDLEDOT || unichar == SIGN_MULTIBULLET || unichar == SIGN_SMALLCIRCLE || unichar == SIGN_DIVISION_SLASH || unichar == SIGN_DIVISION || unichar == SIGN_MINUS || unichar == SIGN_PLUS) lb_point = i + 1;
				}
			}
		}
	}
	return r;
}

bool check_exchange_rates() {
	int i = CALCULATOR->exchangeRatesUsed();
	if(i == 0) return false;
	if(CALCULATOR->checkExchangeRatesDate(auto_update_exchange_rates > 0 ? auto_update_exchange_rates : 7, false, auto_update_exchange_rates == 0 || (auto_update_exchange_rates < 0 && !ask_questions), i)) return false;
	if(auto_update_exchange_rates == 0 || (auto_update_exchange_rates < 0 && !ask_questions)) return false;
	bool b = false;
	if(auto_update_exchange_rates < 0) {
		char buffer[1000];
		string ask_str;
		int cx = snprintf(buffer, 1000, _("It has been %s day(s) since the exchange rates last were updated."), i2s((int) floor(difftime(time(NULL), CALCULATOR->getExchangeRatesTime(i)) / 86400)).c_str());
		if(cx >= 0 && cx < 1000) {
			ask_str = buffer;
			ask_str += "\n";
		}
		ask_str += _("Do you wish to update the exchange rates now?");
		b = ask_question(ask_str.c_str());
	}
	if(b || auto_update_exchange_rates > 0) {
		if(auto_update_exchange_rates <= 0) i = -1;
		CALCULATOR->fetchExchangeRates(15, i);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}


#ifdef HAVE_LIBREADLINE
#	define CHECK_IF_SCREEN_FILLED if(check_sf) {rcount++; if(rcount + 2 >= rows) {FPUTS_UNICODE(_("\nPress Enter to continue."), stdout); fflush(stdout); sf_c = rl_read_key(); if(sf_c != '\n') {check_sf = false;} else {puts(""); rcount = 1;}}}
#	define CHECK_IF_SCREEN_FILLED_PUTS_RP(x, rplus) {str_lb = x; int cr = 0; if(!cfile) {cr = addLineBreaks(str_lb, cols);} if(check_sf) {if(rcount + cr + 1 + rplus >= rows) {rcount += 2; while(rcount < rows) {puts(""); rcount++;} FPUTS_UNICODE(_("\nPress Enter to continue."), stdout); fflush(stdout); sf_c = rl_read_key(); if(sf_c != '\n') {check_sf = false;} else {rcount = 0; if(str_lb.empty() || str_lb[0] != '\n') {puts(""); rcount++;}}} if(check_sf) {rcount += cr;}} PUTS_UNICODE(str_lb.c_str());}
#	define CHECK_IF_SCREEN_FILLED_PUTS(x) CHECK_IF_SCREEN_FILLED_PUTS_RP(x, 0)
#	define INIT_SCREEN_CHECK int rows = 0, cols = 0, rcount = 0; bool check_sf = (cfile == NULL); char sf_c; string str_lb; if(!cfile) rl_get_screen_size(&rows, &cols);
#	define INIT_COLS int rows = 0, cols = 0; if(!cfile) rl_get_screen_size(&rows, &cols);
#	define CHECK_IF_SCREEN_FILLED_HEADING_S(x) str = "\n"; BEGIN_UNDERLINED(str); str += x; END_UNDERLINED(str); CHECK_IF_SCREEN_FILLED_PUTS_RP(str.c_str(), 1);
#	define CHECK_IF_SCREEN_FILLED_HEADING(x) str = "\n"; BEGIN_UNDERLINED(str); BEGIN_BOLD(str); str += x; END_UNDERLINED(str); END_BOLD(str); CHECK_IF_SCREEN_FILLED_PUTS_RP(str.c_str(), 1);
#else
#	define CHECK_IF_SCREEN_FILLED
#	define CHECK_IF_SCREEN_FILLED_PUTS(x) str_lb = x; if(!cfile) {addLineBreaks(str_lb, cols);} PUTS_UNICODE(str_lb.c_str());
#	define INIT_SCREEN_CHECK string str_lb; int cols = 80;
#	define INIT_COLS int cols = (cfile ? 0 : 80);
#	define CHECK_IF_SCREEN_FILLED_HEADING_S(x) str = "\n"; BEGIN_UNDERLINED(str); str += x; END_UNDERLINED(str); PUTS_UNICODE(str.c_str());
#	define CHECK_IF_SCREEN_FILLED_HEADING(x) puts(""); str = "\n"; BEGIN_UNDERLINED(str); BEGIN_BOLD(str); str += x; END_UNDERLINED(str); END_BOLD(str); PUTS_UNICODE(str.c_str());
#endif

#define SET_BOOL(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else if(x != v) {x = v;}}
#define SET_BOOL_D(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else if(x != v) {x = v; result_display_updated();}}
#define SET_BOOL_E(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else if(x != v) {x = v; expression_calculation_updated();}}
#define SET_BOOL_PV(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else if(x != v) {x = v; expression_format_updated(v);}}
#define SET_BOOL_PT(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else if(x != v) {x = v; expression_format_updated(true);}}
#define SET_BOOL_PF(x)	{int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else if(x != v) {x = v; expression_format_updated(false);}}

void set_option(string str) {
	remove_blank_ends(str);
	string svalue, svar;
	bool empty_value = false;
	size_t i_underscore = str.find("_");
	size_t index;
	if(i_underscore != string::npos) {
		index = str.find_first_of(SPACES);
		if(index != string::npos && i_underscore > index) i_underscore = string::npos;
	}
	if(i_underscore == string::npos) index = str.find_last_of(SPACES);
	if(index != string::npos) {
		svar = str.substr(0, index);
		remove_blank_ends(svar);
		svalue = str.substr(index + 1);
		remove_blank_ends(svalue);
	} else {
		svar = str;
	}
	if(i_underscore != string::npos) gsub("_", " ", svar);
	if(svalue.empty()) {
		empty_value = true;
		svalue = "1";
	}

	set_option_place:
	if(EQUALS_IGNORECASE_AND_LOCAL(svar, "base", _("base")) || EQUALS_IGNORECASE_AND_LOCAL(svar, "input base", _("input base")) || svar == "inbase" || EQUALS_IGNORECASE_AND_LOCAL(svar, "output base", _("output base")) || svar == "outbase") {
		int v = 0;
		bool b_in = EQUALS_IGNORECASE_AND_LOCAL(svar, "input base", _("input base")) || svar == "inbase";
		bool b_out = EQUALS_IGNORECASE_AND_LOCAL(svar, "output base", _("output base")) || svar == "outbase";
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "roman", _("roman"))) v = BASE_ROMAN_NUMERALS;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "time", _("time"))) {if(b_in) v = 0; else v = BASE_TIME;}
		else if(equalsIgnoreCase(svalue, "hex") || EQUALS_IGNORECASE_AND_LOCAL(svalue, "hexadecimal", _("hexadecimal"))) v = BASE_HEXADECIMAL;
		else if(equalsIgnoreCase(svalue, "golden")) v = BASE_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "unicode")) v = BASE_UNICODE;
		else if(equalsIgnoreCase(svalue, "duo") || EQUALS_IGNORECASE_AND_LOCAL(svalue, "duodecimal", _("duodecimal"))) v = 12;
		else if(equalsIgnoreCase(svalue, "bin") || EQUALS_IGNORECASE_AND_LOCAL(svalue, "binary", _("binary"))) v = BASE_BINARY;
		else if(equalsIgnoreCase(svalue, "oct") || EQUALS_IGNORECASE_AND_LOCAL(svalue, "octal", _("octal"))) v = BASE_OCTAL;
		else if(equalsIgnoreCase(svalue, "dec") || EQUALS_IGNORECASE_AND_LOCAL(svalue, "decimal", _("decimal"))) v = BASE_DECIMAL;
		else if(equalsIgnoreCase(svalue, "sexa") || EQUALS_IGNORECASE_AND_LOCAL(svalue, "sexagesimal", _("sexagesimal"))) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL;}
		
		else if(!b_in && !b_out && (index = svalue.find_first_of(SPACES)) != string::npos) {
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
			if(expression_executed) {
				expression_executed = false;
				set_option(string("outbase ") + str);
				expression_executed = true;
			} else {
				set_option(string("outbase ") + str);
			}
			set_option(string("inbase ") + svalue);
			return;
		} else if(!empty_value) {
			MathStructure m;
			EvaluationOptions eo = evalops;
			eo.parse_options.base = 10;
			CALCULATOR->calculate(&m, svalue, 500, eo);
			if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
				v = m.number().intValue();
			} else if(m.isNumber() && (!m.number().isNegative() || m.number().isInteger()) && (m.number() > 1 || m.number() < -1) && m.number() >= -1114112L && m.number() <= 1114112L) {
				v = BASE_CUSTOM;
				if(b_in) CALCULATOR->setCustomInputBase(m.number());
				else CALCULATOR->setCustomOutputBase(m.number());
			}
		}
		if(v == 0) {
			PUTS_UNICODE(_("Illegal base."));
		} else if(b_in) {
			evalops.parse_options.base = v;
			expression_format_updated(false);
		} else {
			printops.base = v;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "assumptions", _("assumptions")) || svar == "ass") {
		size_t i = svalue.find_first_of(SPACES);
		if(i != string::npos) {
			set_assumption(svalue.substr(0, i), false);
			set_assumption(svalue.substr(i + 1, svalue.length() - (i + 1)), true);
		} else {
			set_assumption(svalue, false);
		}
		string value;
		switch(CALCULATOR->defaultAssumptions()->sign()) {
			case ASSUMPTION_SIGN_POSITIVE: {value = _("positive"); break;}
			case ASSUMPTION_SIGN_NONPOSITIVE: {value = _("non-positive"); break;}
			case ASSUMPTION_SIGN_NEGATIVE: {value = _("negative"); break;}
			case ASSUMPTION_SIGN_NONNEGATIVE: {value = _("non-negative"); break;}
			case ASSUMPTION_SIGN_NONZERO: {value = _("non-zero"); break;}
			default: {}
		}
		if(!value.empty() && CALCULATOR->defaultAssumptions()->type() != ASSUMPTION_TYPE_NONE) value += " ";
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
		FPUTS_UNICODE(_("assumptions"), stdout); fputs(": ", stdout); PUTS_UNICODE(value.c_str());
		expression_calculation_updated();
	}
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "all prefixes", _("all prefixes")) || svar == "allpref") SET_BOOL_D(printops.use_all_prefixes)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "complex numbers", _("complex numbers")) || svar == "cplx") SET_BOOL_E(evalops.allow_complex)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "excessive parentheses", _("excessive parentheses")) || svar == "expar") SET_BOOL_D(printops.excessive_parenthesis)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "functions", _("functions")) || svar == "func") SET_BOOL_PV(evalops.parse_options.functions_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "infinite numbers", _("infinite numbers")) || svar == "inf") SET_BOOL_E(evalops.allow_infinite)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "show negative exponents", _("show negative exponents")) || svar == "negexp") SET_BOOL_D(printops.negative_exponents)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "assume nonzero denominators", _("assume nonzero denominators")) || svar == "nzd") SET_BOOL_E(evalops.assume_denominators_nonzero)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "warn nonzero denominators", _("warn nonzero denominators")) || svar == "warnnzd") SET_BOOL_E(evalops.warn_about_denominators_assumed_nonzero)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "prefixes", _("prefixes")) || svar == "pref") SET_BOOL_D(printops.use_unit_prefixes)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "denominator prefixes", _("denominator prefixes")) || svar == "denpref") SET_BOOL_D(printops.use_denominator_prefix)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "place units separately", _("place units separately")) || svar == "unitsep") SET_BOOL_D(printops.place_units_separately)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "calculate variables", _("calculate variables")) || svar == "calcvar") SET_BOOL_E(evalops.calculate_variables)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "calculate functions", _("calculate functions")) || svar == "calcfunc") SET_BOOL_E(evalops.calculate_functions)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "sync units", _("sync units")) || svar == "sync") SET_BOOL_E(evalops.sync_units)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "round to even", _("round to even")) || svar == "rndeven") SET_BOOL_D(printops.round_halfway_to_even)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "rpn syntax", _("rpn syntax")) || svar == "rpnsyn") SET_BOOL_PF(evalops.parse_options.rpn)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "rpn", _("rpn")) && svalue.find(" ") == string::npos) {SET_BOOL(rpn_mode) if(!rpn_mode) CALCULATOR->clearRPNStack();} 
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "short multiplication", _("short multiplication")) || svar == "shortmul") SET_BOOL_D(printops.short_multiplication)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "lowercase e", _("lowercase e")) || svar == "lowe") SET_BOOL_D(printops.lower_case_e)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "lowercase numbers", _("lowercase numbers")) || svar == "lownum") SET_BOOL_D(printops.lower_case_numbers)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "base display", _("base display")) || svar == "basedisp") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = BASE_DISPLAY_NONE;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "normal", _("normal"))) v = BASE_DISPLAY_NORMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "alternative", _("alternative"))) v = BASE_DISPLAY_ALTERNATIVE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.base_display = (BaseDisplay) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "two's complement", _("two's complement")) || svar == "twos") SET_BOOL_D(printops.twos_complement)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "hexadecimal two's", _("hexadecimal two's")) || svar == "hextwos") SET_BOOL_D(printops.hexadecimal_twos_complement)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "digit grouping", _("digit grouping")) || svar =="group") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = DIGIT_GROUPING_NONE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = DIGIT_GROUPING_NONE;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "standard", _("standard")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "on", _("on"))) v = DIGIT_GROUPING_STANDARD;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "locale", _("locale"))) v = DIGIT_GROUPING_LOCALE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < DIGIT_GROUPING_NONE || v > DIGIT_GROUPING_LOCALE) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.digit_grouping = (DigitGrouping) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "spell out logical", _("spell out logical")) || svar == "spellout") SET_BOOL_D(printops.spell_out_logical_operators)
	else if((EQUALS_IGNORECASE_AND_LOCAL(svar, "ignore dot", _("ignore dot")) || svar == "nodot") && CALCULATOR->getDecimalPoint() != DOT) SET_BOOL_PF(evalops.parse_options.dot_as_separator)
	else if((EQUALS_IGNORECASE_AND_LOCAL(svar, "ignore comma", _("ignore comma")) || svar == "nocomma") && CALCULATOR->getDecimalPoint() != COMMA) {
		SET_BOOL(evalops.parse_options.comma_as_separator)
		CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
		expression_format_updated(false);
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "decimal comma", _("decimal comma"))) {
		int v = -2;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = 0;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "on", _("on"))) v = 1;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "locale", _("locale"))) v = -1;
		else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < -1 || v > 1) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			b_decimal_comma = v;
			if(b_decimal_comma > 0) CALCULATOR->useDecimalComma();
			else if(b_decimal_comma == 0) CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
			if(v >= 0) {
				expression_format_updated(false);
				result_display_updated();
			}
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "limit implicit multiplication", _("limit implicit multiplication")) || svar == "limimpl") {
		int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {printops.limit_implicit_multiplication = v; evalops.parse_options.limit_implicit_multiplication = v; expression_format_updated(true);}		
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "spacious", _("spacious")) || svar == "space") SET_BOOL_D(printops.spacious)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "unicode", _("unicode")) || svar == "uni") {
		int v = s2b(svalue); if(v < 0) {PUTS_UNICODE(_("Illegal value"));} else {printops.use_unicode_signs = v; result_display_updated();}
		enable_unicode = -1;
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "units", _("units")) || svar == "unit") SET_BOOL_PV(evalops.parse_options.units_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "unknowns", _("unknowns")) || svar == "unknown") SET_BOOL_PV(evalops.parse_options.unknowns_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "variables", _("variables")) || svar == "var") SET_BOOL_PV(evalops.parse_options.variables_enabled)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "abbreviations", _("abbreviations")) || svar == "abbr" || svar == "abbrev") SET_BOOL_D(printops.abbreviate_names)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "show ending zeroes", _("show ending zeroes")) || svar == "zeroes") SET_BOOL_D(printops.show_ending_zeroes)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "repeating decimals", _("repeating decimals")) || svar == "repdeci") SET_BOOL_D(printops.indicate_infinite_series)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "angle unit", _("angle unit")) || svar == "angle") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "rad", _("rad")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "radians", _("radians"))) v = ANGLE_UNIT_RADIANS;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "deg", _("deg")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "degrees", _("degrees"))) v = ANGLE_UNIT_DEGREES;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "gra", _("gra")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "gradians", _("gradians"))) v = ANGLE_UNIT_GRADIANS;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = ANGLE_UNIT_NONE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 3) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.parse_options.angle_unit = (AngleUnit) v;
			hide_parse_errors = true;
			expression_format_updated(true);
			hide_parse_errors = false;
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "caret as xor", _("caret as xor")) || equalsIgnoreCase(svar, "xor^")) SET_BOOL_PT(caret_as_xor)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "parsing mode", _("parsing mode")) || svar == "parse") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "adaptive", _("adaptive"))) v = PARSING_MODE_ADAPTIVE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "implicit first", _("implicit first"))) v = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "conventional", _("conventional"))) v = PARSING_MODE_CONVENTIONAL;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.parse_options.parsing_mode = (ParsingMode) v;
			expression_format_updated(true);
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "update exchange rates", _("update exchange rates")) || svar == "upxrates") {
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "never", _("never"))) {
			auto_update_exchange_rates = 0;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "ask", _("ask"))) {
			auto_update_exchange_rates = -1;
		} else {
			int v = s2i(svalue);
			if(empty_value) v = 7;
			if(v < 0) auto_update_exchange_rates = -1;
			else auto_update_exchange_rates = v;
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "multiplication sign", _("multiplication sign")) || svar == "mulsign") {
		int v = -1;
		if(svalue == SIGN_MULTIDOT || svalue == ".") v = MULTIPLICATION_SIGN_DOT;
		else if(svalue == SIGN_MULTIPLICATION || svalue == "x") v = MULTIPLICATION_SIGN_X;
		else if(svalue == "*") v = MULTIPLICATION_SIGN_ASTERISK;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.multiplication_sign = (MultiplicationSign) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "division sign", _("division sign")) || svar == "divsign") {
		int v = -1;
		if(svalue == SIGN_DIVISION_SLASH) v = DIVISION_SIGN_DIVISION_SLASH;
		else if(svalue == SIGN_DIVISION) v = DIVISION_SIGN_DIVISION;
		else if(svalue == "/") v = DIVISION_SIGN_SLASH;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.division_sign = (DivisionSign) v;
			result_display_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "approximation", _("approximation")) || svar == "appr" || svar == "approx") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "exact", _("exact"))) v = APPROXIMATION_EXACT;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "try exact", _("try exact")) || svalue == "try") v = APPROXIMATION_TRY_EXACT;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "approximate", _("approximate")) || svalue == "approx") v = APPROXIMATION_APPROXIMATE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			if(automatic_fraction && printops.number_fraction_format == FRACTION_DECIMAL_EXACT && v != APPROXIMATION_EXACT) {
				printops.number_fraction_format = FRACTION_DECIMAL;
				automatic_fraction = false;
			}
			evalops.approximation = (ApproximationMode) v;
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "interval calculation", _("interval calculation")) || svar == "ic" || EQUALS_IGNORECASE_AND_LOCAL(svar, "uncertainty propagation", _("uncertainty propagation")) || svar == "up") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "variance formula", _("variance formula")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "variance", _("variance"))) v = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "interval arithmetic", _("interval arithmetic")) || svalue == "iv") v = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < INTERVAL_CALCULATION_NONE || v > INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.interval_calculation = (IntervalCalculation) v;
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "autoconversion", _("autoconversion")) || svar == "conv") {
		int v = -1;
		MixedUnitsConversion muc = MIXED_UNITS_CONVERSION_DEFAULT;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) {v = POST_CONVERSION_NONE;  muc = MIXED_UNITS_CONVERSION_NONE;}
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "best", _("best"))) v = POST_CONVERSION_OPTIMAL_SI;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "optimalsi", _("optimalsi")) || svalue == "si") v = POST_CONVERSION_OPTIMAL_SI;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "optimal", _("optimal"))) v = POST_CONVERSION_OPTIMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "base", _("base"))) v = POST_CONVERSION_BASE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "mixed", _("mixed"))) v = POST_CONVERSION_NONE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v == POST_CONVERSION_OPTIMAL + 1) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_DEFAULT;
		} else if(v == 0) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_NONE;
		}
		if(v < 0 || v > POST_CONVERSION_OPTIMAL) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.auto_post_conversion = (AutoPostConversion) v;
			evalops.mixed_units_conversion = muc;
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "currency conversion", _("currency conversion")) || svar == "curconv") SET_BOOL_E(evalops.local_currency_conversion)
	else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "algebra mode", _("algebra mode")) || svar == "alg") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "none", _("none"))) v = STRUCTURING_NONE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "simplify", _("simplify")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "expand", _("expand"))) v = STRUCTURING_SIMPLIFY;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "factorize", _("factorize")) || svalue == "factor") v = STRUCTURING_FACTORIZE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > STRUCTURING_FACTORIZE) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.structuring = (StructuringMode) v;
			printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "exact", _("exact"))) {
		int v = s2b(svalue); 
		if(v < 0) {
			PUTS_UNICODE(_("Illegal value")); 
		} else if(v > 0) {
			evalops.approximation = APPROXIMATION_EXACT; 
			if(printops.number_fraction_format == FRACTION_DECIMAL) {
				automatic_fraction = true;
				printops.number_fraction_format = FRACTION_DECIMAL_EXACT;
			}
			expression_calculation_updated();
		} else {
			evalops.approximation = APPROXIMATION_TRY_EXACT;
			if(automatic_fraction && printops.number_fraction_format == FRACTION_DECIMAL_EXACT) {
				printops.number_fraction_format = FRACTION_DECIMAL;
				automatic_fraction = false;
			}
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "ignore locale", _("ignore locale"))) {
		int v = s2b(svalue); 
		if(v < 0) {
			PUTS_UNICODE(_("Illegal value")); 
		} else if(v != ignore_locale) {
			if(v > 0) {
				ignore_locale = true;
			} else {
				ignore_locale = false;
			}
			PUTS_UNICODE("Please restart the program for the change to take effect."); 
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
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "save definitions", _("save definitions")) || svar == "save defs") {
		int v = s2b(svalue); 
		if(v < 0) {
			PUTS_UNICODE(_("Illegal value")); 
		} else if(v > 0) {
			save_defs_on_exit = true;
		} else {
			save_defs_on_exit = false;
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "scientific notation", _("scientific notation")) || svar == "exp mode" || svar == "exp") {
		int v = -1;
		bool valid = true;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = EXP_NONE;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "auto", _("auto"))) v = EXP_PRECISION;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "pure", _("pure"))) v = EXP_PURE;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "scientific", _("scientific"))) v = EXP_SCIENTIFIC;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "engineering", _("engineering"))) v = EXP_BASE_3;
		else if(svalue.find_first_not_of(SPACES NUMBERS MINUS) == string::npos) v = s2i(svalue);
		else valid = false;
		if(valid) {
			printops.min_exp = v;
			result_format_updated();
		} else {
			PUTS_UNICODE(_("Illegal value."));
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "precision", _("precision")) || svar == "prec") {
		int v = 0;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 1) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			CALCULATOR->setPrecision(v);
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "interval display", _("interval display")) || svar == "ivdisp") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "adaptive", _("adaptive"))) v = 0;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "significant", _("significant"))) v = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS + 1;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "interval", _("interval"))) v = INTERVAL_DISPLAY_INTERVAL + 1;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "plusminus", _("plusminus"))) v = INTERVAL_DISPLAY_PLUSMINUS + 1;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "midpoint", _("midpoint"))) v = INTERVAL_DISPLAY_MIDPOINT + 1;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "upper", _("upper"))) v = INTERVAL_DISPLAY_UPPER + 1;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "lower", _("lower"))) v = INTERVAL_DISPLAY_LOWER + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v == 0) {
			adaptive_interval_display = true;
			printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
			result_format_updated();
		} else {
			v--;
			if(v < INTERVAL_DISPLAY_SIGNIFICANT_DIGITS || v > INTERVAL_DISPLAY_UPPER) {
				PUTS_UNICODE(_("Illegal value."));
			} else {
				adaptive_interval_display = false;
				printops.interval_display = (IntervalDisplay) v;
				result_format_updated();
			}
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "interval arithmetic", _("interval arithmetic")) || svar == "ia" || svar == "interval") {
		bool b = CALCULATOR->usesIntervalArithmetic();
		SET_BOOL(b)
		if(b != CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->useIntervalArithmetic(b);
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "variable units", _("variable units")) || svar == "varunits") {
		bool b = CALCULATOR->variableUnitsEnabled();
		SET_BOOL(b)
		if(b != CALCULATOR->variableUnitsEnabled()) {
			CALCULATOR->setVariableUnitsEnabled(b);
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "max decimals", _("max decimals")) || svar == "maxdeci") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 0) {
			printops.use_max_decimals = false;
			result_format_updated();
		} else {
			printops.max_decimals = v;
			printops.use_max_decimals = true;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "min decimals", _("min decimals")) || svar == "mindeci") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) v = s2i(svalue);
		if(v < 0) {
			printops.min_decimals = 0;
			printops.use_min_decimals = false;
			result_format_updated();
		} else {
			printops.min_decimals = v;
			printops.use_min_decimals = true;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "fractions", _("fractions")) || svar == "fr") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = FRACTION_DECIMAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "exact", _("exact"))) v = FRACTION_DECIMAL_EXACT;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "on", _("on"))) v = FRACTION_FRACTIONAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "combined", _("combined")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "mixed", _("mixed"))) v = FRACTION_COMBINED;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "long", _("long"))) v = FRACTION_COMBINED + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > FRACTION_COMBINED + 1) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			printops.restrict_fraction_length = (v == FRACTION_FRACTIONAL);
			if(v == 4) v = FRACTION_FRACTIONAL;
			printops.number_fraction_format = (NumberFractionFormat) v;
			automatic_fraction = false;
			result_format_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "complex form", _("complex form")) || svar == "cplxform") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "rectangular", _("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "cartesian", _("cartesian")) || svalue == "rect") v = COMPLEX_NUMBER_FORM_RECTANGULAR;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "exponential", _("exponential")) || svalue == "exp") v = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "polar", _("polar"))) v = COMPLEX_NUMBER_FORM_POLAR;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.complex_number_form = (ComplexNumberForm) v;
			expression_calculation_updated();
		}
	} else if(EQUALS_IGNORECASE_AND_LOCAL(svar, "read precision", _("read precision")) || svar == "readprec") {
		int v = -1;
		if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "off", _("off"))) v = DONT_READ_PRECISION;
		else if(EQUALS_IGNORECASE_AND_LOCAL(svalue, "always", _("always"))) v = ALWAYS_READ_PRECISION;
		else if(empty_value || EQUALS_IGNORECASE_AND_LOCAL(svalue, "when decimals", _("when decimals")) || EQUALS_IGNORECASE_AND_LOCAL(svalue, "on", _("on"))) v = READ_PRECISION_WHEN_DECIMALS;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			PUTS_UNICODE(_("Illegal value."));
		} else {
			evalops.parse_options.read_precision = (ReadPrecisionMode) v;
			expression_format_updated(true);
		}
	} else {
		if(i_underscore == string::npos) {
			if(index != string::npos) {
				if((index = svar.find_last_of(SPACES)) != string::npos) {
					svar = svar.substr(0, index);
					remove_blank_ends(svar);
					str = str.substr(index + 1);
					remove_blank_ends(str);
					svalue = str;
					gsub("_", " ", svar);
					goto set_option_place;
				}
			}
			if(!empty_value && !svalue.empty()) {
				svar += " ";
				svar += svalue;
				svalue = "1";
				empty_value = true;
				goto set_option_place;
			}
		}
		PUTS_UNICODE(_("Unrecognized option."));
	}
}

#define STR_AND_TABS(x) str = x; pctl = unicode_length(str); if(pctl >= 32) {str += "\t";} else if(pctl >= 24) {str += "\t\t";} else if(pctl >= 16) {str += "\t\t\t";} else if(pctl >= 8) {str += "\t\t\t\t";} else {str += "\t\t\t\t\t";}
#define STR_AND_TABS_T1(x) str = x; pctl = unicode_length(str); str += "\t";
#define STR_AND_TABS_T2(x) str = x; pctl = unicode_length(str); if(pctl >= 8) {str += "\t";} else {str += "\t\t";}
#define STR_AND_TABS_T3(x) str = x; pctl = unicode_length(str); if(pctl >= 16) {str += "\t";} else if(pctl >= 8) {str += "\t\t";} else {str += "\t\t\t";}
#define STR_AND_TABS_T4(x) str = x; pctl = unicode_length(str); if(pctl >= 24) {str += "\t";} else if(pctl >= 16) {str += "\t\t";} else if(pctl >= 8) {str += "\t\t\t";} else {str += "\t\t\t\t";}
#ifdef _WIN32
#	define BEGIN_BOLD(x)
#	define END_BOLD(x)
#	define BEGIN_UNDERLINED(x)
#	define END_UNDERLINED(x)
#	define BEGIN_ITALIC(x)
#	define END_ITALIC(x)
#	define PUTS_BOLD(x) PUTS_UNICODE(x);
#	define PUTS_UNDERLINED(x) PUTS_UNICODE(x);
#else
#	define BEGIN_BOLD(x) if(!cfile) {x += "\033[1m";}
#	define END_BOLD(x) if(!cfile) {x += "\033[0m";}
#	define BEGIN_UNDERLINED(x) if(!cfile) {x += "\033[4m";}
#	define END_UNDERLINED(x) if(!cfile) {x += "\033[0m";}
#	define BEGIN_ITALIC(x) if(!cfile) {x += "\033[3m";}
#	define END_ITALIC(x) if(!cfile) {x += "\033[23m";}
#	define PUTS_BOLD(x) if(cfile) {str = x;} else {str = "\033[1m"; str += x; str += "\033[0m";} PUTS_UNICODE(str.c_str());
#	define PUTS_UNDERLINED(x) if(cfile) {str = x;} else {str = "\033[4m"; str += x; str += "\033[0m";} PUTS_UNICODE(str.c_str());
#endif

bool equalsIgnoreCase(const string &str1, const string &str2, size_t i2, size_t i2_end, size_t minlength) {
	if(str1.empty() || str2.empty()) return false;
	size_t l = 0;
	if(i2_end == string::npos) i2_end = str2.length();
	for(size_t i1 = 0;; i1++, i2++) {
		if(i2 >= i2_end) {
			return i1 >= str1.length();
		}
		if(i1 >= str1.length()) break;
		if((str1[i1] < 0 && i1 + 1 < str1.length()) || (str2[i2] < 0 && i2 + 1 < str2.length())) {
			size_t iu1 = 1, iu2 = 1;
			if(str1[i1] < 0) {
				while(iu1 + i1 < str1.length() && str1[i1 + iu1] < 0) {
					iu1++;
				}
			}
			if(str2[i2] < 0) {
				while(iu2 + i2 < str2.length() && str2[i2 + iu2] < 0) {
					iu2++;
				}
			}
			bool isequal = (iu1 == iu2);
			if(isequal) {
				for(size_t i = 0; i < iu1; i++) {
					if(str1[i1 + i] != str2[i2 + i]) {
						isequal = false;
						break;
					}
				}
			}
			if(!isequal) {
				char *gstr1 = utf8_strdown(str1.c_str() + (sizeof(char) * i1), iu1);
				char *gstr2 = utf8_strdown(str2.c_str() + (sizeof(char) * i2), iu2);
				if(!gstr1 || !gstr2) return false;
				bool b = strcmp(gstr1, gstr2) == 0;
				free(gstr1);
				free(gstr2);
				if(!b) return false;
			}
			i1 += iu1 - 1;
			i2 += iu2 - 1;
		} else if(str1[i1] != str2[i2] && !((str1[i1] >= 'a' && str1[i1] <= 'z') && str1[i1] - 32 == str2[i2]) && !((str1[i1] <= 'Z' && str1[i1] >= 'A') && str1[i1] + 32 == str2[i2])) {
			return false;
		}
		l++;
	}
	return l >= minlength;
}

bool title_matches(ExpressionItem *item, const string &str, size_t minlength = 0) {
	const string &title = item->title(true);
	size_t i = 0;
	while(true) {
		while(true) {
			if(i >= title.length()) return false;
			if(title[i] != ' ') break;
			i++;
		}
		size_t i2 = title.find(' ', i);
		if(equalsIgnoreCase(str, title, i, i2, minlength)) {
			return true;
		}
		if(i2 == string::npos) break;
		i = i2 + 1;
	}
	return false;
}
bool name_matches(ExpressionItem *item, const string &str) {
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		if(item->getName(i2).case_sensitive) {
			if(str == item->getName(i2).name.substr(0, str.length())) {
				return true;
			}
		} else {
			if(equalsIgnoreCase(str, item->getName(i2).name, 0, str.length(), 0)) {
				return true;
			}
		}
	}
	return false;
}
bool country_matches(Unit *u, const string &str, size_t minlength = 0) {
	const string &countries = u->countries();
	size_t i = 0;
	while(true) {
		while(true) {
			if(i >= countries.length()) return false;
			if(countries[i] != ' ') break;
			i++;
		}
		size_t i2 = countries.find(',', i);
		if(equalsIgnoreCase(str, countries, i, i2, minlength)) {
			return true;
		}
		if(i2 == string::npos) break;
		i = i2 + 1;
	}
	return false;
}

void show_calendars(const QalculateDateTime &date, bool indentation = true) {
	string str, calstr;
	int pctl;
	bool b_fail;
	long int y, m, d;
	STR_AND_TABS((indentation ? string("  ") + _("Calendar") : _("Calendar"))); str += _("Day"); str += ", "; str += _("Month"); str += ", "; str += _("Year"); PUTS_UNICODE(str.c_str());
#define PUTS_CALENDAR(x, c) calstr = ""; BEGIN_BOLD(calstr); STR_AND_TABS((indentation ? string("  ") + x : x)); calstr += str; END_BOLD(calstr); b_fail = !dateToCalendar(date, y, m, d, c); if(b_fail) {calstr += _("failed");} else {calstr += i2s(d); calstr += " "; calstr += monthName(m, c, true); calstr += " "; calstr += i2s(y);} FPUTS_UNICODE(calstr.c_str(), stdout);
	PUTS_CALENDAR(string(_("Gregorian:")), CALENDAR_GREGORIAN); puts("");
	PUTS_CALENDAR(string(_("Hebrew:")), CALENDAR_HEBREW); puts("");
	PUTS_CALENDAR(string(_("Islamic:")), CALENDAR_ISLAMIC); puts("");
	PUTS_CALENDAR(string(_("Persian:")), CALENDAR_PERSIAN); puts("");
	PUTS_CALENDAR(string(_("Indian national:")), CALENDAR_INDIAN); puts("");
	PUTS_CALENDAR(string(_("Chinese:")), CALENDAR_CHINESE); 
	long int cy, yc, st, br;
	chineseYearInfo(y, cy, yc, st, br);
	if(!b_fail) {FPUTS_UNICODE((string(" (") + chineseStemName(st) + string(" ") + chineseBranchName(br) + ")").c_str(), stdout);}
	 puts("");
	PUTS_CALENDAR(string(_("Julian:")), CALENDAR_JULIAN); puts("");
	PUTS_CALENDAR(string(_("Revised julian:")), CALENDAR_MILANKOVIC); puts("");
	PUTS_CALENDAR(string(_("Coptic:")), CALENDAR_COPTIC); puts("");
	PUTS_CALENDAR(string(_("Ethiopian:")), CALENDAR_ETHIOPIAN); puts("");
	//PUTS_CALENDAR(string(_("Egyptian:")), CALENDAR_EGYPTIAN);
}


void list_defs(bool in_interactive, char list_type = 0, string search_str = "") {
#ifdef HAVE_LIBREADLINE
	int rows, cols, rcount = 0;
	bool check_sf = (cfile == NULL);
	char sf_c;
	if(in_interactive && !cfile) {
		rl_get_screen_size(&rows, &cols);
	} else {
		cols = 80;
	}
#else
	int cols = 80; 
#endif
	string str_lb;
	string str;
	if(!search_str.empty()) {
		int max_l = 0;
		list<string> name_list;
		int i_end = 0;
		size_t i2 = 0;
		if(list_type == 'v') i2 = 1;
		else if(list_type == 'u' || list_type == 'c') i2 = 2;
		for(; i2 <= 2; i2++) {
			if(i2 == 0) i_end = CALCULATOR->functions.size();
			else if(i2 == 1) i_end = CALCULATOR->variables.size();
			else if(i2 == 2) i_end = CALCULATOR->units.size();
			ExpressionItem *item = NULL;
			string name_str, name_str2;
			for(int i = 0; i < i_end; i++) {
				if(i2 == 0) item = CALCULATOR->functions[i];
				else if(i2 == 1) item = CALCULATOR->variables[i];
				else if(i2 == 2) item = CALCULATOR->units[i];
				if((!item->isHidden() || (i2 == 2 && ((Unit*) item)->isCurrency())) && item->isActive() && (i2 != 2 || (item->subtype() != SUBTYPE_COMPOSITE_UNIT)) && (list_type != 'c' || ((Unit*) item)->isCurrency())) {
					bool b_match = name_matches(item, search_str);
					if(!b_match && title_matches(item, search_str, list_type == 'c' ? 0 : 3)) b_match = true;
					if(!b_match && i2 == 2 && country_matches((Unit*) item, search_str, list_type == 'c' ? 0 : 3)) b_match = true;
					if(b_match) {
						const ExpressionName &ename1 = item->preferredInputName(false, false);
						name_str = ename1.name;
						size_t name_i = 1;
						while(true) {
							const ExpressionName &ename = item->getName(name_i);
							if(ename == empty_expression_name) break;
							if(ename != ename1 && !ename.avoid_input && !ename.plural && (!ename.unicode || printops.use_unicode_signs) && !ename.completion_only) {
								name_str += " / ";
								name_str += ename.name;
							}
							name_i++;
						}
						if(!item->title(false).empty()) {
							name_str += " (";
							name_str += item->title(false);
							name_str += ")";
						}
						if((int) name_str.length() > max_l) max_l = name_str.length();
						name_list.push_front(name_str);
					}
				}
			}
			if(list_type != 0) break;
		}
		if(name_list.empty()) {
			PUTS_UNICODE(_("No matching item found."));
			puts("");
		} else {
			name_list.sort();
			list<string>::iterator it = name_list.begin();
			list<string>::iterator it_e = name_list.end();
			int c = 0;
			int max_tabs = (max_l / 8) + 1;
			int max_c = cols / (max_tabs * 8);
			if(cfile) max_c = 0;
			while(it != it_e) {
				c++;
				if(c >= max_c) {
					c = 0;
					if(max_c == 1 && in_interactive) {CHECK_IF_SCREEN_FILLED}
					PUTS_UNICODE(it->c_str());
				} else {
					if(c == 1 && in_interactive) {CHECK_IF_SCREEN_FILLED}
					int l = unicode_length_check(it->c_str());
					int nr_of_tabs = max_tabs - (l / 8);
					for(int tab_nr = 0; tab_nr < nr_of_tabs; tab_nr++) {
						*it += "\t";
					}
					FPUTS_UNICODE(it->c_str(), stdout);
				}
				++it;
			}
			if(c > 0) puts("");
			if(in_interactive) {CHECK_IF_SCREEN_FILLED}
			puts("");
			if(in_interactive) {CHECK_IF_SCREEN_FILLED}
			if(in_interactive) {CHECK_IF_SCREEN_FILLED_PUTS(_("For more information about a specific function, variable or unit, please use the info command (in interactive mode)."));}
			else {PUTS_UNICODE(_("For more information about a specific function, variable or unit, please use the info command (in interactive mode)."));}
			puts("");
		}
	} else if(list_type == 0) {
		puts("");
		if(in_interactive) {CHECK_IF_SCREEN_FILLED;}
		bool b_variables = false, b_functions = false, b_units = false;
		for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			Variable *v = CALCULATOR->variables[i];
			if((v->isLocal() || v->hasChanged()) && v->isActive() && (!is_answer_variable(v) || !v->representsUndefined())) {
				int pctl;
				if(!b_variables) {
					b_variables = true;
					PUTS_BOLD(_("Variables:"));
					if(in_interactive) {CHECK_IF_SCREEN_FILLED}
					STR_AND_TABS(_("Name"))
					str += _("Value");
					PUTS_UNICODE(str.c_str());
					if(in_interactive) {CHECK_IF_SCREEN_FILLED}
				}
				STR_AND_TABS(v->preferredInputName(false, false).name.c_str())
				FPUTS_UNICODE(str.c_str(), stdout);
				string value;
				if(v->isKnown()) {
					bool is_relative = false;
					if(((KnownVariable*) v)->isExpression()) {
						value = CALCULATOR->localizeExpression(((KnownVariable*) v)->expression());
						if(!((KnownVariable*) v)->uncertainty(&is_relative).empty()) {
							if(is_relative) {value += " ("; value += _("relative uncertainty"); value += ": ";}
							else value += SIGN_PLUSMINUS;
							value += CALCULATOR->localizeExpression(((KnownVariable*) v)->uncertainty());
							if(is_relative) {value += ")";}
						}
						if(!((KnownVariable*) v)->unit().empty() && ((KnownVariable*) v)->unit() != "auto") {
							value += " ";
							value += CALCULATOR->localizeExpression(((KnownVariable*) v)->unit());
						}
						if(value.length() > 40) {
							value = value.substr(0, 30);
							value += "...";
						}
						FPUTS_UNICODE(value.c_str(), stdout);
						if(!is_relative && ((KnownVariable*) v)->uncertainty().empty() && v->isApproximate() && ((KnownVariable*) v)->expression().find(SIGN_PLUSMINUS) == string::npos && ((KnownVariable*) v)->expression().find(CALCULATOR->f_interval->referenceName()) == string::npos) {
							fputs(" (", stdout);
							FPUTS_UNICODE(_("approximate"), stdout);
							fputs(")", stdout);
							
						}
					} else {
						if(((KnownVariable*) v)->get().isMatrix()) {
							value = _("matrix");
						} else if(((KnownVariable*) v)->get().isVector()) {
							value = _("vector");
						} else {
							PrintOptions po;
							po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
							value = CALCULATOR->print(((KnownVariable*) v)->get(), 30, po);
						}
						FPUTS_UNICODE(value.c_str(), stdout);
						if(v->isApproximate() && ((KnownVariable*) v)->get().containsInterval(true, false, false, 0, true) <= 0) {
							fputs(" (", stdout);
							FPUTS_UNICODE(_("approximate"), stdout);
							fputs(")", stdout);
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
					FPUTS_UNICODE(value.c_str(), stdout);
				}
				puts("");
				if(in_interactive) {CHECK_IF_SCREEN_FILLED}
			}
		}
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			MathFunction *f = CALCULATOR->functions[i];
			if((f->isLocal() || f->hasChanged()) && f->isActive()) {
				if(!b_functions) {
					if(b_variables) puts("");
					if(in_interactive) {CHECK_IF_SCREEN_FILLED}
					if(in_interactive) {CHECK_IF_SCREEN_FILLED}
					b_functions = true;
					PUTS_BOLD(_("Functions:"));
				}
				puts(f->preferredInputName(false, false).name.c_str());
				if(in_interactive) {CHECK_IF_SCREEN_FILLED}
			}
		}
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			Unit *u = CALCULATOR->units[i];
			if((u->isLocal() || u->hasChanged()) && u->isActive()) {
				if(!b_units) {
					if(b_variables || b_functions) puts("");
					if(in_interactive) {CHECK_IF_SCREEN_FILLED}
					if(in_interactive) {CHECK_IF_SCREEN_FILLED}
					b_units = true;
					PUTS_BOLD(_("Units:"));
				}
				puts(u->preferredInputName(false, false).name.c_str());
				if(in_interactive) {CHECK_IF_SCREEN_FILLED}
			}
		}
		if(!b_variables && !b_functions && !b_units) {
			puts(_("No local variables, functions or units have been defined."));
			if(in_interactive) {CHECK_IF_SCREEN_FILLED}
		}
		puts("");
		if(in_interactive) {CHECK_IF_SCREEN_FILLED}
		if(in_interactive) {CHECK_IF_SCREEN_FILLED_PUTS(_("For more information about a specific function, variable or unit, please use the info command (in interactive mode)."));}
		else {PUTS_UNICODE(_("For more information about a specific function, variable or unit, please use the info command (in interactive mode)."));}
		puts("");
	} else {
		int max_l = 0;
		list<string> name_list;
		int i_end = 0;
		if(list_type == 'f') i_end = CALCULATOR->functions.size();
		if(list_type == 'v') i_end = CALCULATOR->variables.size();
		if(list_type == 'u') i_end = CALCULATOR->units.size();
		if(list_type == 'c') i_end = CALCULATOR->units.size();
		ExpressionItem *item = NULL;
		string name_str, name_str2;
		for(int i = 0; i < i_end; i++) {
			if(list_type == 'f') item = CALCULATOR->functions[i];
			if(list_type == 'v') item = CALCULATOR->variables[i];
			if(list_type == 'u') item = CALCULATOR->units[i];
			if(list_type == 'c') item = CALCULATOR->units[i];
			if((!item->isHidden() || list_type == 'c') && item->isActive() && (list_type != 'u' || (item->subtype() != SUBTYPE_COMPOSITE_UNIT && ((Unit*) item)->baseUnit() != CALCULATOR->u_euro)) && (list_type != 'c' || ((Unit*) item)->isCurrency())) {
				const ExpressionName &ename1 = item->preferredInputName(false, false);
				name_str = ename1.name;
				size_t name_i = 1;
				while(true) {
					const ExpressionName &ename = item->getName(name_i);
					if(ename == empty_expression_name) break;
					if(ename != ename1 && !ename.avoid_input && !ename.plural && (!ename.unicode || printops.use_unicode_signs) && !ename.completion_only) {
						name_str += " / ";
						name_str += ename.name;
					}
					name_i++;
				}
				if(list_type == 'c' && !item->title(false).empty()) {
					name_str += " (";
					name_str += item->title(false);
					name_str += ")";
				}
				if((int) name_str.length() > max_l) max_l = name_str.length();
				name_list.push_front(name_str);
			}
		}
		name_list.sort();
		list<string>::iterator it = name_list.begin();
		list<string>::iterator it_e = name_list.end();
		int c = 0;
		int max_tabs = (max_l / 8) + 1;
		int max_c = cols / (max_tabs * 8);
		if(cfile) max_c = 0;
		while(it != it_e) {
			c++;
			if(c >= max_c) {
				c = 0;
				if(max_c == 1 && in_interactive) {CHECK_IF_SCREEN_FILLED}
				PUTS_UNICODE(it->c_str());
			} else {
				if(c == 1 && in_interactive) {CHECK_IF_SCREEN_FILLED}
				int l = unicode_length_check(it->c_str());
				int nr_of_tabs = max_tabs - (l / 8);
				for(int tab_nr = 0; tab_nr < nr_of_tabs; tab_nr++) {
					*it += "\t";
				}
				FPUTS_UNICODE(it->c_str(), stdout);
			}
			++it;
		}
		if(c > 0) puts("");
		if(in_interactive) {CHECK_IF_SCREEN_FILLED}
		puts("");
		if(in_interactive) {CHECK_IF_SCREEN_FILLED}
		if(in_interactive) {CHECK_IF_SCREEN_FILLED_PUTS(_("For more information about a specific function, variable or unit, please use the info command (in interactive mode)."));}
		else {PUTS_UNICODE(_("For more information about a specific function, variable or unit, please use the info command (in interactive mode)."));}
		puts("");
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
	fetch_exchange_rates_at_startup = false;
	char list_type = 'n';
	string search_str;
	
#ifdef ENABLE_NLS
	string filename = buildPath(getLocalDir(), "qalc.cfg");
	FILE *file = fopen(filename.c_str(), "r");
	char line[10000];
	string stmp;
	if(file) {
		while(true) {
			if(fgets(line, 10000, file) == NULL) break;
			if(strcmp(line, "ignore_locale=1\n") == 0) {
				ignore_locale = true;
				break;
			} else if(strcmp(line, "ignore_locale=0\n") == 0) {
				break;
			}
		}
		fclose(file);
	}
	if(!ignore_locale) {
		bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
		textdomain(GETTEXT_PACKAGE);
	}
#endif
	
	if(!ignore_locale) setlocale(LC_ALL, "");
	
	for(int i = 1; i < argc; i++) {
		string svalue, svar;
		if(calc_arg_begun) {
			calc_arg += " ";
		} else {
			svar = argv[i];
			size_t i2 = svar.find_first_of(NUMBERS);
			if(i2 != string::npos && i2 != 0) {
				svalue = svar.substr(i2);
				svar = svar.substr(0, i2);
			}
		}
		if(!calc_arg_begun && (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0)) {
			PUTS_UNICODE(_("usage: qalc [options] [expression]"));
			printf("\n");
			PUTS_UNICODE(_("where options are:"));
			fputs("\n\t-b, -base", stdout); fputs(" ", stdout); FPUTS_UNICODE(_("BASE"), stdout); fputs("\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("set the number base for results and, optionally, expressions"));
#ifdef HAVE_LIBCURL
			fputs("\n\t-e, -exrates\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("update exchange rates"));
#endif
			fputs("\n\t-f, -file", stdout); fputs(" ", stdout); FPUTS_UNICODE(_("FILE"), stdout); fputs("\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("executes commands from a file first"));
			fputs("\n\t-i, -interactive\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("start in interactive mode"));
			fputs("\n\t-l, -list", stdout); fputs(" [", stdout); FPUTS_UNICODE(_("SEARCH TERM"), stdout); fputs("]\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("displays a list of all user-defined or matching variables, functions and units."));
			fputs("\n\t--list-functions", stdout); fputs(" [", stdout); FPUTS_UNICODE(_("SEARCH TERM"), stdout); fputs("]\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("displays a list of all or matching functions."));
			fputs("\n\t--list-units", stdout); fputs(" [", stdout); FPUTS_UNICODE(_("SEARCH TERM"), stdout); fputs("]\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("displays a list of all or matching units."));
			fputs("\n\t--list-variables", stdout); fputs(" [", stdout); FPUTS_UNICODE(_("SEARCH TERM"), stdout); fputs("]\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("displays a list of all or matching variables."));
			fputs("\n\t-m, -time", stdout); fputs(" ", stdout); FPUTS_UNICODE(_("MILLISECONDS"), stdout); fputs("\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("terminate calculation and display of result after specified amount of time"));
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
			fputs("\n\t-p", stdout); fputs(" [", stdout); FPUTS_UNICODE(_("BASE"), stdout); fputs("]\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("start in programming mode (same as -b \"BASE BASE\" -s \"xor^\", with base conversion)"));
			fputs("\n\t-s, -set", stdout); fputs(" \"", stdout); FPUTS_UNICODE(_("OPTION"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("VALUE"), stdout); fputs("\"\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("as set command in interactive program session (ex. -set \"base 16\")"));
			fputs("\n\t-t, -terse\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("reduces output to just the result of the input expression"));
			fputs("\n\t-/+u8\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("turn on/off unicode support"));
			fputs("\n\t-v, -version\n", stdout);
			fputs("\t", stdout); PUTS_UNICODE(_("show application version and exit"));
			puts("");
			PUTS_UNICODE(_("The program will start in interactive mode if no expression and no file is specified (or interactive mode is explicitly selected)."));
			puts("");
			return 0;
		} else if(!calc_arg_begun && strcmp(argv[i], "-u8") == 0) {
			enable_unicode = 1;
		} else if(!calc_arg_begun && strcmp(argv[i], "+u8") == 0) {
			enable_unicode = 0;
#ifdef HAVE_LIBCURL
		} else if(!calc_arg_begun && (strcmp(argv[i], "-exrates") == 0 || strcmp(argv[i], "--exrates") == 0 || strcmp(argv[i], "-e") == 0)) {
			fetch_exchange_rates_at_startup = true;
#endif
		} else if(!calc_arg_begun && (svar == "-base" || svar == "--base" || svar == "-b")) {
			string set_base_str = "base ";
			if(!svalue.empty()) {
				set_base_str += svalue;
			} else {
				i++;
				if(i < argc) {
					set_base_str += argv[i];
				}
			}
			set_option_strings.push_back(set_base_str);
		} else if(!calc_arg_begun && svar == "-p") {
			programmers_mode = true;
			string set_base_str = "base ";
			if(!svalue.empty()) {
				set_base_str += svalue;
				set_base_str += " ";
				set_base_str += svalue;
			} else {
				i++;
				if(i < argc) {
					set_base_str += argv[i];
					set_base_str += " ";
					set_base_str += argv[i];
				}
			}
			set_option_strings.push_back(set_base_str);
			set_option_strings.push_back("xor^ 1");
		} else if(!calc_arg_begun && (strcmp(argv[i], "+p") == 0)) {
			set_option_strings.push_back("base 10 10");
			set_option_strings.push_back("xor^ 0");
		} else if(!calc_arg_begun && (strcmp(argv[i], "-terse") == 0 || strcmp(argv[i], "--terse") == 0 || strcmp(argv[i], "-t") == 0)) {
			result_only = true;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)) {
			puts(VERSION);
			return 0;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-interactive") == 0 || strcmp(argv[i], "--interactive") == 0 || strcmp(argv[i], "-i") == 0)) {
			interactive_mode = true;
		} else if(!calc_arg_begun && (strcmp(argv[i], "-list") == 0 || strcmp(argv[i], "--list") == 0 || strcmp(argv[i], "-l") == 0)) {
			list_type = 0;
			if(i + 1 < argc && strlen(argv[i + 1]) > 0 && argv[i + 1][0] != '-' && argv[i + 1][0] != '+') {
				i++;
				search_str = argv[i];
				remove_blank_ends(search_str);
			}
		} else if(!calc_arg_begun && strcmp(argv[i], "--list-functions") == 0) {
			list_type = 'f';
			if(i + 1 < argc && strlen(argv[i + 1]) > 0 && argv[i + 1][0] != '-' && argv[i + 1][0] != '+') {
				i++;
				search_str = argv[i];
				remove_blank_ends(search_str);
			}
		} else if(!calc_arg_begun && strcmp(argv[i], "--list-units") == 0) {
			list_type = 'u';
			if(i + 1 < argc && strlen(argv[i + 1]) > 0 && argv[i + 1][0] != '-' && argv[i + 1][0] != '+') {
				i++;
				search_str = argv[i];
				remove_blank_ends(search_str);
			}
		} else if(!calc_arg_begun && strcmp(argv[i], "--list-variables") == 0) {
			list_type = 'v';
			if(i + 1 < argc && strlen(argv[i + 1]) > 0 && argv[i + 1][0] != '-' && argv[i + 1][0] != '+') {
				i++;
				search_str = argv[i];
				remove_blank_ends(search_str);
			}
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
		} else if(!calc_arg_begun && (strcmp(argv[i], "-time") == 0 || strcmp(argv[i], "--time") == 0 || strcmp(argv[i], "-m") == 0)) {
			i++;
			if(i < argc) {
				i_maxtime += strtol(argv[i], NULL, 10);
				if(i_maxtime < 0) i_maxtime = 0;
			}
		} else if(!calc_arg_begun && (strcmp(argv[i], "-set") == 0 || strcmp(argv[i], "--set") == 0 || strcmp(argv[i], "-s") == 0)) {
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
			calc_arg += argv[i];
			calc_arg_begun = true;
		}
	}

	b_busy = false;

	//create the almighty Calculator object
	new Calculator(ignore_locale);

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

	canfetch = CALCULATOR->canFetch();

	string str;
#ifdef HAVE_LIBREADLINE	
	static char* rlbuffer;
#endif

	ask_questions = (command_file.empty() || interactive_mode) && !result_only;
	
	//exchange rates
	if(fetch_exchange_rates_at_startup && canfetch) {
		CALCULATOR->fetchExchangeRates(15);
		int cols = 0;
		if(!command_file.empty()) {
#ifdef HAVE_LIBREADLINE
			int rows = 0;
			rl_get_screen_size(&rows, &cols);
#else
			cols = 80;
#endif
		}
		display_errors(false, cols);
	}
	if(load_global_defs && load_currencies && canfetch) {
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
		else if(!load_units && load_currencies && !CALCULATOR->loadGlobalCurrencies()) b = false;
		if(load_functions && !CALCULATOR->loadGlobalFunctions()) b = false;
		if(load_datasets && !CALCULATOR->loadGlobalDataSets()) b = false;
		if(load_variables && !CALCULATOR->loadGlobalVariables()) b = false;
		if(!b) {PUTS_UNICODE(_("Failed to load global definitions!"));}
	}

	//load local definitions
	CALCULATOR->loadLocalDefinitions();
	
	if(list_type != 'n') {
		CALCULATOR->terminateThreads();
		list_defs(false, list_type, search_str);
		return 0;
	}

	//reset
	result_text = "0";
	parsed_text = "0";
	
	view_thread = new ViewThread;
	command_thread = new CommandThread;

	if(!command_file.empty()) {
		if(command_file == "-") {
			cfile = stdin;
		} else {
			cfile = fopen(command_file.c_str(), "r");
			if(!cfile) {
				printf(_("Could not open \"%s\".\n"), command_file.c_str());
				if(!interactive_mode) {
					if(!view_thread->write(NULL)) view_thread->cancel();
					CALCULATOR->terminateThreads();
					return 0;
				}
			}
		}
	}

	if(i_maxtime > 0) {
#ifndef CLOCK_MONOTONIC
		gettimeofday(&t_end, NULL);
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		t_end.tv_sec = ts.tv_sec;
		t_end.tv_usec = ts.tv_nsec / 1000;
#endif
		long int usecs = t_end.tv_usec + (long int) i_maxtime * 1000;
		t_end.tv_usec = usecs % 1000000;
		t_end.tv_sec += usecs / 1000000;
	}
	if(!cfile && !calc_arg.empty()) {
		if(calc_arg.length() > 2 && ((calc_arg[0] == '\"' && calc_arg.find('\"', 1) == calc_arg.length() - 1) || (calc_arg[0] == '\'' && calc_arg.find('\'', 1) == calc_arg.length() - 1))) {
			calc_arg = calc_arg.substr(1, calc_arg.length() - 2);
		}
		if(!printops.use_unicode_signs && contains_unicode_char(calc_arg.c_str())) {
			char *gstr = locale_to_utf8(calc_arg.c_str());
			if(gstr) {
				expression_str = gstr;
				free(gstr);
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
			if(!view_thread->write(NULL)) view_thread->cancel();
			CALCULATOR->terminateThreads();
			return 0;
		}
		i_maxtime = 0;
		use_readline = true;
	} else if(!cfile) {
		ask_questions = !result_only;
		interactive_mode = true;
		i_maxtime = 0;
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
			if(i_maxtime < 0 || !fgets(buffer, 1000, cfile)) {
				if(cfile != stdin) {
					fclose(cfile);
				}
				cfile = NULL;
				if(!calc_arg.empty()) {
					if(!printops.use_unicode_signs && contains_unicode_char(calc_arg.c_str())) {
						char *gstr = locale_to_utf8(calc_arg.c_str());
						if(gstr) {
							expression_str = gstr;
							free(gstr);
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
				i_maxtime = 0;
				continue;
			}
			if(!printops.use_unicode_signs && contains_unicode_char(buffer)) {
				char *gstr = locale_to_utf8(buffer);
				if(gstr) {
					str = gstr;
					free(gstr);
				} else { 
					str = buffer;
				}
			} else {
				str = buffer;
			}
			remove_blank_ends(str);
			if(str.empty() || str[0] == '#' || (str.length() >= 2 && str[0] == '/' && str[1] == '/')) continue;
		} else {
#ifdef HAVE_LIBREADLINE
			rlbuffer = readline("> ");
			if(rlbuffer == NULL) break;
			if(!printops.use_unicode_signs && contains_unicode_char(rlbuffer)) {
				char *gstr = locale_to_utf8(rlbuffer);
				if(gstr) {
					str = gstr;
					free(gstr);
				} else {
					str = rlbuffer;
				}
			} else {
				str = rlbuffer;
			}
#else
			fputs("> ", stdout);
			if(!fgets(buffer, 1000, stdin)) {
				str = "";
			} else if(!printops.use_unicode_signs && contains_unicode_char(buffer)) {
				char *gstr = locale_to_utf8(buffer);
				if(gstr) {
					str = gstr;
					free(gstr);
				} else { 
					str = buffer;
				}
			} else {
				str = buffer;
			}
#endif	
		}
		bool explicit_command = (!str.empty() && str[0] == '/');
		if(explicit_command) str.erase(0, 1);
		remove_blank_ends(str);
		if(rpn_mode && explicit_command && str.empty()) {str = "/"; explicit_command = false;}
		slen = str.length();
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
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "variable", _("variable"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			string name = str, expr;
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
			if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
			expr = str;
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
					((KnownVariable*) v)->set(expr);
					if(v->countNames() == 0) {
						ExpressionName ename(name);
						ename.reference = true;
						v->setName(ename, 1);
					} else {
						v->setName(name, 1);
					}
				} else {
					CALCULATOR->addVariable(new KnownVariable("", name, expr));
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "function", _("function"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			string name = str, expr;
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
			if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
			expr = str;
			bool b = true;
			if(!CALCULATOR->functionNameIsValid(name)) {
				name = CALCULATOR->convertToValidFunctionName(name);
				size_t l = name.length() + strlen(_("Illegal name. Save as %s instead (default: no)?"));
				char *cstr = (char*) malloc(sizeof(char) * (l + 1));
				snprintf(cstr, l, _("Illegal name. Save as %s instead (default: no)?"), name.c_str());
				if(!ask_question(cstr)) {
					b = false;
				}
				free(cstr);
			}
			if(b && CALCULATOR->functionNameTaken(name)) {
				if(!ask_question(_("An function with the same name already exists.\nDo you want to overwrite it (default: no)?"))) {
					b = false;
				}
			}
			if(b) {
				MathFunction *f = CALCULATOR->getActiveFunction(name);
				if(f && f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) {
					((UserFunction*) f)->setFormula(expr);
					if(f->countNames() == 0) {
						ExpressionName ename(name);
						ename.reference = true;
						f->setName(ename, 1);
					} else {
						f->setName(name, 1);
					}
				} else {
					CALCULATOR->addFunction(new UserFunction("", name, expr));
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "delete", _("delete"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			Variable *v = CALCULATOR->getActiveVariable(str);
			if(v && v->isLocal()) {
				v->destroy();
			} else {
				MathFunction *f = CALCULATOR->getActiveFunction(str);
				if(f && f->isLocal()) {
					f->destroy();
				} else {
					PUTS_UNICODE(_("No user-defined variable or function with the specified name exist."));
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
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			if(EQUALS_IGNORECASE_AND_LOCAL(str, "syntax", _("syntax"))) {
				if(!evalops.parse_options.rpn) {
					evalops.parse_options.rpn = true;
					expression_format_updated(false);
				}
				rpn_mode = false;
				CALCULATOR->clearRPNStack();
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "stack", _("stack"))) {
				if(evalops.parse_options.rpn) {
					evalops.parse_options.rpn = false;
					expression_format_updated(false);
				}
				rpn_mode = true;
			} else {
				int v = s2b(str); 
				if(v < 0) {
					PUTS_UNICODE(_("Illegal value")); 
				} else {
					rpn_mode = v;
				}
				if(evalops.parse_options.rpn != rpn_mode) {
					evalops.parse_options.rpn = rpn_mode;
					expression_format_updated(false);
				}
				if(!rpn_mode) CALCULATOR->clearRPNStack();
			}
		//qalc command
		} else if(canfetch && EQUALS_IGNORECASE_AND_LOCAL(str, "exrates", _("exrates"))) {
			CALCULATOR->fetchExchangeRates(15);
			CALCULATOR->loadExchangeRates();
			int cols = 0;
			if(cfile) {
#ifdef HAVE_LIBREADLINE
				int rows = 0;
				rl_get_screen_size(&rows, &cols);
#else
				cols = 80;
#endif
			}
			display_errors(false, cols);

		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "stack", _("stack"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else {
				puts("");
				MathStructure m;
				for(size_t i = 1; i <= CALCULATOR->RPNStackSize(); i++) {
					m = *CALCULATOR->getRPNRegister(i);
					m.removeDefaultAngleUnit(evalops);
					m.format(printops);
					string regstr = m.print(printops);
					if(!cfile) replace_quotation_marks(regstr);
					printf("  %i:\t%s\n", (int) i, regstr.c_str());
				}
				puts("");
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "swap", _("swap"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else if(CALCULATOR->RPNStackSize() == 1) {
				PUTS_UNICODE(_("The RPN stack only contains one value."));
			} else {
				CALCULATOR->moveRPNRegisterUp(2);
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "swap", _("swap"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else if(CALCULATOR->RPNStackSize() == 1) {
				PUTS_UNICODE(_("The RPN stack only contains one value."));
			} else {
				int index1 = 0, index2 = 0;
				str = str.substr(ispace + 1, slen - (ispace + 1));
				string str2 = "";
				remove_blank_ends(str);
				ispace = str.find_first_of(SPACES);
				if(ispace != string::npos) {
					str2 = str.substr(ispace + 1, str.length() - (ispace + 1));
					str = str.substr(0, ispace);
					remove_blank_ends(str2);
					remove_blank_ends(str);
				}
				index1 = s2i(str);
				if(str2.empty()) index2 = 1;
				else index2 = s2i(str2);				
				if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
				if(index2 < 0) index2 = (int) CALCULATOR->RPNStackSize() + 1 + index2;
				if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize() || (!str2.empty() && (index2 <= 0 || index2 > (int) CALCULATOR->RPNStackSize()))) {
					PUTS_UNICODE(_("The specified RPN stack index does not exist."));
				} else if(index1 > index2) {					
					CALCULATOR->moveRPNRegister((size_t) index2, (size_t) index1);
					CALCULATOR->moveRPNRegister((size_t) index1 - 1, (size_t) index2);
				} else if(index1 != index2) {
					CALCULATOR->moveRPNRegister((size_t) index1, (size_t) index2);
					CALCULATOR->moveRPNRegister((size_t) index2 - 1, (size_t) index1);
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "move", _("move"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else if(CALCULATOR->RPNStackSize() == 1) {
				PUTS_UNICODE(_("The RPN stack only contains one value."));
			} else {
				int index1 = 0, index2 = 0;
				str = str.substr(ispace + 1, slen - (ispace + 1));
				string str2 = "";
				remove_blank_ends(str);
				ispace = str.find_first_of(SPACES);
				if(ispace != string::npos) {
					str2 = str.substr(ispace + 1, str.length() - (ispace + 1));
					str = str.substr(0, ispace);
					remove_blank_ends(str2);
					remove_blank_ends(str);
				}
				index1 = s2i(str);
				if(str2.empty()) index2 = 1;
				else index2 = s2i(str2);				
				if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
				if(index2 < 0) index2 = (int) CALCULATOR->RPNStackSize() + 1 + index2;
				if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize() || (!str2.empty() && (index2 <= 0 || index2 > (int) CALCULATOR->RPNStackSize()))) {
					PUTS_UNICODE(_("The specified RPN stack index does not exist."));
				} else {
					CALCULATOR->moveRPNRegister((size_t) index1, (size_t) index2);
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rotate", _("rotate"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else if(CALCULATOR->RPNStackSize() == 1) {
				PUTS_UNICODE(_("The RPN stack only contains one value."));
			} else {
				CALCULATOR->moveRPNRegister(1, CALCULATOR->RPNStackSize());
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "rotate", _("rotate"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else if(CALCULATOR->RPNStackSize() == 1) {
				PUTS_UNICODE(_("The RPN stack only contains one value."));
			} else {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(EQUALS_IGNORECASE_AND_LOCAL(str, "up", _("up"))) {
					CALCULATOR->moveRPNRegister(1, CALCULATOR->RPNStackSize());
				} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "down", _("down"))) {
					CALCULATOR->moveRPNRegister(CALCULATOR->RPNStackSize(), 1);
				} else {
					PUTS_UNICODE(_("Illegal value"));
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "copy", _("copy"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else {
				CALCULATOR->RPNStackEnter(new MathStructure(*CALCULATOR->getRPNRegister(1)));
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "copy", _("copy"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				int index1 = s2i(str);
				if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
				if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
					PUTS_UNICODE(_("The specified RPN stack index does not exist."));
				} else {
					CALCULATOR->RPNStackEnter(new MathStructure(*CALCULATOR->getRPNRegister((size_t) index1)));
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "clear stack", _("clear stack"))) {
			CALCULATOR->clearRPNStack();
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "pop", _("pop"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else {
				CALCULATOR->deleteRPNRegister(1);
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "pop", _("pop"))) {
			if(CALCULATOR->RPNStackSize() == 0) {
				PUTS_UNICODE(_("The RPN stack is empty."));
			} else {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				int index1 = s2i(str);
				if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
				if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
					PUTS_UNICODE(_("The specified RPN stack index does not exist."));
				} else {
					CALCULATOR->deleteRPNRegister((size_t) index1);
				}
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "exact", _("exact"))) {
			if(evalops.approximation != APPROXIMATION_EXACT) {
				if(printops.number_fraction_format == FRACTION_DECIMAL) {
					automatic_fraction = true;
					printops.number_fraction_format = FRACTION_DECIMAL_EXACT;
				}
				evalops.approximation = APPROXIMATION_EXACT;
				expression_calculation_updated();
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "approximate", _("approximate")) || str == "approx") {
			if(evalops.approximation != APPROXIMATION_TRY_EXACT) {
				if(automatic_fraction && printops.number_fraction_format == FRACTION_DECIMAL_EXACT) {
					printops.number_fraction_format = FRACTION_DECIMAL;
					automatic_fraction = false;
				}
				evalops.approximation = APPROXIMATION_TRY_EXACT;
				expression_calculation_updated();
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "convert", _("convert")) || EQUALS_IGNORECASE_AND_LOCAL(scom, "to", _("to"))) {
			str = str.substr(ispace + 1, slen - (ispace + 1));
			remove_blank_ends(str);
			string str1, str2;
			size_t ispace2 = str.find_first_of(SPACES);
			if(ispace2 != string::npos) {
				str1 = str.substr(0, ispace2);
				remove_blank_ends(str1);
				str2 = str.substr(ispace2 + 1);
				remove_blank_ends(str2);
			}
			remove_duplicate_blanks(str);
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
			} else if(equalsIgnoreCase(str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(str, "decimal", _("decimal"))) {
				int save_base = printops.base;
				printops.base = BASE_DECIMAL;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(str, "octal", _("octal"))) {
				int save_base = printops.base;
				printops.base = BASE_OCTAL;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(str, "duodecimal", _("duodecimal"))) {
				int save_base = printops.base;
				printops.base = BASE_DUODECIMAL;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "roman") || equalsIgnoreCase(str, _("roman"))) {
				int save_base = printops.base;
				printops.base = BASE_ROMAN_NUMERALS;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "sexa") || EQUALS_IGNORECASE_AND_LOCAL(str, "sexagesimal", _("sexagesimal"))) {
				int save_base = printops.base;
				printops.base = BASE_SEXAGESIMAL;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "time", _("time"))) {
				int save_base = printops.base;
				printops.base = BASE_TIME;
				setResult(NULL, false);
				printops.base = save_base;
			} else if(equalsIgnoreCase(str, "utc") || equalsIgnoreCase(str, "gmt")) {
				printops.time_zone = TIME_ZONE_UTC;
				setResult(NULL, false);
				printops.time_zone = TIME_ZONE_LOCAL;
			} else if(str.length() > 3 && (equalsIgnoreCase(str.substr(0, 3), "utc") || equalsIgnoreCase(str.substr(0, 3), "gmt"))) {
				string to_str = str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					CALCULATOR->error(true, _("Time zone parsing failed."), NULL);
				}
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = itz;
				setResult(NULL, false);
				printops.custom_time_zone = 0;
				printops.time_zone = TIME_ZONE_LOCAL;
			} else if(str == "CET") {
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = 60;
				setResult(NULL, false);
				printops.custom_time_zone = 0;
				printops.time_zone = TIME_ZONE_LOCAL;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rectangular", _("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(str, "cartesian", _("cartesian")) || str == "rect") {
				avoid_recalculation = false;
				ComplexNumberForm cnf_bak = evalops.complex_number_form;
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
				hide_parse_errors = true;
				if(rpn_mode) execute_command(COMMAND_EVAL);
				else execute_expression();
				hide_parse_errors = false;
				evalops.complex_number_form = cnf_bak;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "exponential", _("exponential")) || str == "exp") {
				avoid_recalculation = false;
				ComplexNumberForm cnf_bak = evalops.complex_number_form;
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
				hide_parse_errors = true;
				if(rpn_mode) execute_command(COMMAND_EVAL);
				else execute_expression();
				hide_parse_errors = false;
				evalops.complex_number_form = cnf_bak;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "polar", _("polar"))) {
				avoid_recalculation = false;
				ComplexNumberForm cnf_bak = evalops.complex_number_form;
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
				hide_parse_errors = true;
				if(rpn_mode) execute_command(COMMAND_EVAL);
				else execute_expression();
				hide_parse_errors = false;
				evalops.complex_number_form = cnf_bak;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "bases", _("bases"))) {
				int save_base = printops.base;
				string save_result_text = result_text;
				string base_str;
				int cols = 0;
				if(interactive_mode && !cfile) {
					base_str = "\n  ";
#ifdef HAVE_LIBREADLINE
					int rows = 0;
					rl_get_screen_size(&rows, &cols);
#else
					cols = 80;
#endif
				}
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
				if(interactive_mode && !cfile) {
					base_str += "\n";
					addLineBreaks(base_str, cols, true, 2, base_str.length());
					replace_quotation_marks(base_str);
				}
				PUTS_UNICODE(base_str.c_str());
				printops.base = save_base;
				result_text = save_result_text;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "calendar", _("calendars"))) {
				QalculateDateTime date;
				if(mstruct->isDateTime()) {
					date.set(*mstruct->datetime());
				} else {
					date.setToCurrentDate();
				}
				puts("");
				show_calendars(date);
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "fraction", _("fraction")) || str == "frac") {
				NumberFractionFormat save_format = printops.number_fraction_format;
				bool save_rfl = printops.restrict_fraction_length;
				printops.restrict_fraction_length = false;
				if(mstruct->isNumber()) printops.number_fraction_format = FRACTION_COMBINED;
				else printops.number_fraction_format = FRACTION_FRACTIONAL;
				setResult(NULL, false);
				printops.restrict_fraction_length = save_rfl;
				printops.number_fraction_format = save_format;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "factors", _("factors")) || str == "factor") {
				execute_command(COMMAND_FACTORIZE);
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "partial fraction", _("partial fraction")) || str == "partial") {
				execute_command(COMMAND_EXPAND_PARTIAL_FRACTIONS);
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "best", _("best")) || EQUALS_IGNORECASE_AND_LOCAL(str, "optimal", _("optimal"))) {
				CALCULATOR->resetExchangeRatesUsed();
				MathStructure mstruct_new(CALCULATOR->convertToOptimalUnit(*mstruct, evalops, true));
				if(check_exchange_rates()) mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, evalops, true));
				else mstruct->set(mstruct_new);
				result_action_executed();
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "base", _("base"))) {
				CALCULATOR->resetExchangeRatesUsed();
				MathStructure mstruct_new(CALCULATOR->convertToBaseUnits(*mstruct, evalops));
				if(check_exchange_rates()) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, evalops));
				else mstruct->set(mstruct_new);
				result_action_executed();
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str1, "base", _("base"))) {
				int save_base = printops.base;
				Number save_nr = CALCULATOR->customOutputBase();
				EvaluationOptions eo = evalops;
				eo.parse_options.base = 10;
				MathStructure m;
				CALCULATOR->calculate(&m, str2, 500, eo);
				if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
					printops.base = m.number().intValue();
				} else {
					printops.base = BASE_CUSTOM;
					CALCULATOR->setCustomOutputBase(m.number());
				}
				setResult(NULL, false);
				CALCULATOR->setCustomOutputBase(save_nr);
				printops.base = save_base;
			} else {
				CALCULATOR->resetExchangeRatesUsed();
				MathStructure mstruct_new(CALCULATOR->convert(*mstruct, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops));
				if(check_exchange_rates()) mstruct->set(CALCULATOR->convert(*mstruct, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), evalops));
				else mstruct->set(mstruct_new);
				result_action_executed();
			}
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "factor", _("factor"))) {
			execute_command(COMMAND_FACTORIZE);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "partial fraction", _("partial fraction"))) {
			execute_command(COMMAND_EXPAND_PARTIAL_FRACTIONS);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "simplify", _("simplify")) || EQUALS_IGNORECASE_AND_LOCAL(str, "expand", _("expand"))) {
			execute_command(COMMAND_EXPAND);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "mode", _("mode"))) {
			INIT_SCREEN_CHECK

			int pctl;
			
			CHECK_IF_SCREEN_FILLED_HEADING(_("Algebraic Mode"));

#define PRINT_AND_COLON_TABS(x, s) str = x; pctl = unicode_length_check(x); if(strlen(s) > 0) {str += " (" s ")"; pctl += unicode_length_check(" (" s ")");} do {str += '\t'; pctl += 8;} while(pctl < 48);

			PRINT_AND_COLON_TABS(_("algebra mode"), "alg");
			switch(evalops.structuring) {
				case STRUCTURING_NONE: {str += _("none"); break;}
				case STRUCTURING_FACTORIZE: {str += _("factorize"); break;}
				default: {str += _("expand"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("assume nonzero denominators"), "nzd"); str += b2oo(evalops.assume_denominators_nonzero, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("warn nonzero denominators"), "warnnzd"); str += b2oo(evalops.warn_about_denominators_assumed_nonzero, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			string value;
			switch(CALCULATOR->defaultAssumptions()->sign()) {
				case ASSUMPTION_SIGN_POSITIVE: {value = _("positive"); break;}
				case ASSUMPTION_SIGN_NONPOSITIVE: {value = _("non-positive"); break;}
				case ASSUMPTION_SIGN_NEGATIVE: {value = _("negative"); break;}
				case ASSUMPTION_SIGN_NONNEGATIVE: {value = _("non-negative"); break;}
				case ASSUMPTION_SIGN_NONZERO: {value = _("non-zero"); break;}
				default: {}
			}
			if(!value.empty() && CALCULATOR->defaultAssumptions()->type() != ASSUMPTION_TYPE_NONE) value += " ";
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
			PRINT_AND_COLON_TABS(_("assumptions"), "ass"); str += value; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Calculation"));

			PRINT_AND_COLON_TABS(_("angle unit"), "angle");
			switch(evalops.parse_options.angle_unit) {
				case ANGLE_UNIT_RADIANS: {str += _("rad"); break;}
				case ANGLE_UNIT_DEGREES: {str += _("rad"); break;}
				case ANGLE_UNIT_GRADIANS: {str += _("gra"); break;}
				default: {str += _("none"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("approximation"), "appr"); 
			switch(evalops.approximation) {
				case APPROXIMATION_EXACT: {str += _("exact"); break;}
				case APPROXIMATION_TRY_EXACT: {str += _("try exact"); break;}
				default: {str += _("approximate"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("interval arithmetic"), "ia"); str += b2oo(CALCULATOR->usesIntervalArithmetic(), false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("interval calculation"), "ic"); 
			switch(evalops.interval_calculation) {
				case INTERVAL_CALCULATION_NONE: {str += _("none"); break;}
				case INTERVAL_CALCULATION_VARIANCE_FORMULA: {str += _("variance formula"); break;}
				case INTERVAL_CALCULATION_INTERVAL_ARITHMETIC: {str += _("interval arithmetic"); break;}
				case INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC: {str += _("simplified"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("precision"), "prec") str += i2s(CALCULATOR->getPrecision()); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Enabled Objects"));

			PRINT_AND_COLON_TABS(_("calculate functions"), "calcfunc"); str += b2oo(evalops.calculate_functions, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("calculate variables"), "calcvar"); str += b2oo(evalops.calculate_variables, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("complex numbers"), "cplx"); str += b2oo(evalops.allow_complex, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("functions"), "func"); str += b2oo(evalops.parse_options.functions_enabled, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("infinite numbers"), "inf"); str += b2oo(evalops.allow_infinite, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("units"), ""); str += b2oo(evalops.parse_options.units_enabled, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("unknowns"), ""); str += b2oo(evalops.parse_options.unknowns_enabled, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("variables"), "var"); str += b2oo(evalops.parse_options.variables_enabled, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("variable units"), "varunit"); str += b2oo(CALCULATOR->variableUnitsEnabled(), false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Generic Display Options"));

			PRINT_AND_COLON_TABS(_("abbreviations"), "abbr"); str += b2oo(printops.abbreviate_names, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("division sign"), "divsign");
			switch(printops.division_sign) {
				case DIVISION_SIGN_DIVISION_SLASH: {str += SIGN_DIVISION_SLASH; break;}
				case DIVISION_SIGN_DIVISION: {str += SIGN_DIVISION; break;}
				default: {str += "/"; break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("excessive parentheses"), "expar"); str += b2oo(printops.excessive_parenthesis, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("multiplication sign"), "mulsign");
			switch(printops.multiplication_sign) {
				case MULTIPLICATION_SIGN_X: {str += SIGN_MULTIPLICATION; break;}
				case MULTIPLICATION_SIGN_DOT: {str += SIGN_MULTIDOT; break;}
				default: {str += "*"; break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("show negative exponents"), "negexp"); str += b2oo(printops.negative_exponents, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("short multiplication"), "shortmul"); str += b2oo(printops.short_multiplication, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("spacious"), "space"); str += b2oo(printops.spacious, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("spell out logical"), "spellout"); str += b2oo(printops.spell_out_logical_operators, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("unicode"), "uni"); str += b2oo(printops.use_unicode_signs, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Numerical Display"));

			PRINT_AND_COLON_TABS(_("base"), ""); 
			switch(printops.base) {
				case BASE_ROMAN_NUMERALS: {str += _("roman"); break;}
				case BASE_SEXAGESIMAL: {str += _("sexagesimal"); break;}
				case BASE_TIME: {str += _("time"); break;}
				case BASE_GOLDEN_RATIO: {str += "golden ratio"; break;}
				case BASE_UNICODE: {str += "Unicode"; break;}
				case BASE_CUSTOM: {str += CALCULATOR->customOutputBase().print(); break;}
				default: {str += i2s(printops.base);}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("base display"), "basedisp");
			switch(printops.base_display) {
				case BASE_DISPLAY_NONE: {str += _("none"); break;}
				case BASE_DISPLAY_NORMAL: {str += _("normal"); break;}
				case BASE_DISPLAY_ALTERNATIVE: {str += _("alternative"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("complex form"), "cplxform"); 
			switch(evalops.complex_number_form) {
				case COMPLEX_NUMBER_FORM_RECTANGULAR: {str += _("rectangular"); break;}
				case COMPLEX_NUMBER_FORM_EXPONENTIAL: {str += _("exponential"); break;}
				case COMPLEX_NUMBER_FORM_POLAR: {str += _("polar"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("decimal comma"), "");
			if(b_decimal_comma < 0) {str += _("locale");}
			else if(b_decimal_comma == 0) {str += _("off");}
			else {str += _("on");}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("digit grouping"), "group");
			switch(printops.digit_grouping) {
				case DIGIT_GROUPING_NONE: {str += _("off"); break;}
				case DIGIT_GROUPING_STANDARD: {str += _("standard"); break;}
				case DIGIT_GROUPING_LOCALE: {str += _("locale"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("scientific notation"), "exp");
			switch(printops.min_exp) {
				case EXP_NONE: {str += _("off"); break;}
				case EXP_PRECISION: {str += _("auto"); break;}
				case EXP_PURE: {str += _("pure"); break;}
				case EXP_SCIENTIFIC: {str += _("scientific"); break;}
				case EXP_BASE_3: {str += _("engineering"); break;}
				default: {str += i2s(printops.min_exp); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("fractions"), "fr"); 
			switch(printops.number_fraction_format) {
				case FRACTION_DECIMAL: {str += _("off"); break;}
				case FRACTION_DECIMAL_EXACT: {str += _("exact"); break;}
				case FRACTION_FRACTIONAL: {if(printops.restrict_fraction_length) {str += _("on");} else {str += _("long");} break;}
				case FRACTION_COMBINED: {str += _("mixed"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("interval display"), "ivdisp");
			if(adaptive_interval_display) {
				str += _("adaptive");
			} else {
				switch(printops.interval_display) {
					case INTERVAL_DISPLAY_SIGNIFICANT_DIGITS: {str += _("significant"); break;}
					case INTERVAL_DISPLAY_INTERVAL: {str += _("interval"); break;}
					case INTERVAL_DISPLAY_PLUSMINUS: {str += _("plusminus"); break;}
					case INTERVAL_DISPLAY_MIDPOINT: {str += _("midpoint"); break;}
					case INTERVAL_DISPLAY_LOWER: {str += _("lower"); break;}
					case INTERVAL_DISPLAY_UPPER: {str += _("upper"); break;}
					default: {str += i2s(printops.interval_display + 1); break;}
				}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("lowercase e"), "lowe"); str += b2oo(printops.lower_case_e, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("lowercase numbers"), "lownum"); str += b2oo(printops.lower_case_numbers, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("max decimals"), "maxdeci");
			if(printops.use_max_decimals && printops.max_decimals >= 0) {
				str += i2s(printops.max_decimals);
			} else {
				str += _("off");
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("min decimals"), "mindeci"); 
			if(printops.use_min_decimals && printops.min_decimals > 0) {
				str += i2s(printops.min_decimals);
			} else {
				str += _("off");
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("repeating decimals"), "repdeci"); str += b2oo(printops.indicate_infinite_series, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("round to even"), "rndeven"); str += b2oo(printops.round_halfway_to_even, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("show ending zeroes"), "zeroes"); str += b2oo(printops.show_ending_zeroes, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("two's complement"), "twos"); str += b2oo(printops.twos_complement, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("hexadecimal two's"), "hextwos"); str += b2oo(printops.hexadecimal_twos_complement, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Parsing"));

			PRINT_AND_COLON_TABS(_("caret as xor"), "xor^"); str += b2oo(caret_as_xor, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("decimal comma"), "");
			if(b_decimal_comma < 0) {str += _("locale");}
			else if(b_decimal_comma == 0) {str += _("off");}
			else {str += _("on");}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			if(CALCULATOR->getDecimalPoint() != COMMA) {
				PRINT_AND_COLON_TABS(_("ignore comma"), ""); str += b2oo(evalops.parse_options.comma_as_separator, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			}
			if(CALCULATOR->getDecimalPoint() != DOT) {
				PRINT_AND_COLON_TABS(_("ignore dot"), ""); str += b2oo(evalops.parse_options.dot_as_separator, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			}
			PRINT_AND_COLON_TABS(_("input base"), "inbase"); 
			switch(evalops.parse_options.base) {
				case BASE_ROMAN_NUMERALS: {str += _("roman"); break;}
				case BASE_GOLDEN_RATIO: {str += "golden ratio"; break;}
				case BASE_UNICODE: {str += "Unicode"; break;}
				case BASE_CUSTOM: {str += CALCULATOR->customOutputBase().print(); break;}
				default: {str += i2s(evalops.parse_options.base);}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("limit implicit multiplication"), "limimpl"); str += b2oo(evalops.parse_options.limit_implicit_multiplication, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("parsing mode"), "parse"); 
			switch(evalops.parse_options.parsing_mode) {
				case PARSING_MODE_ADAPTIVE: {str += _("adaptive"); break;}
				case PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST: {str += _("implicit first"); break;}
				case PARSING_MODE_CONVENTIONAL: {str += _("conventional"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("read precision"), "readprec"); 
			switch(evalops.parse_options.read_precision) {
				case DONT_READ_PRECISION: {str += _("off"); break;}
				case ALWAYS_READ_PRECISION: {str += _("always"); break;}
				case READ_PRECISION_WHEN_DECIMALS: {str += _("when decimals"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("rpn syntax"), "rpnsyn"); str += b2oo(evalops.parse_options.rpn, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Units"));

			PRINT_AND_COLON_TABS(_("all prefixes"), "allpref"); str += b2oo(printops.use_all_prefixes, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("autoconversion"), "conv");
			switch(evalops.auto_post_conversion) {
				case POST_CONVERSION_NONE: {
					if(evalops.mixed_units_conversion > MIXED_UNITS_CONVERSION_NONE) {str += _("mixed");}
					else {str += _("none");}
					break;
				}
				case POST_CONVERSION_OPTIMAL: {str += _("optimal"); break;}
				case POST_CONVERSION_BASE: {str += _("base"); break;}
				case POST_CONVERSION_OPTIMAL_SI: {str += _("optimalsi"); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("currency conversion"), "curconv"); str += b2oo(evalops.local_currency_conversion, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("denominator prefixes"), "denpref"); str += b2oo(printops.use_denominator_prefix, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("place units separately"), "unitsep"); str += b2oo(printops.place_units_separately, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("prefixes"), "pref"); str += b2oo(printops.use_unit_prefixes, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("sync units"), "sync"); str += b2oo(evalops.sync_units, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("update exchange rates"), "upxrates"); 
			switch(auto_update_exchange_rates) {
				case -1: {str += _("ask"); break;}
				case 0: {str += _("never"); break;}
				default: {str += i2s(auto_update_exchange_rates); break;}
			}
			CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())

			CHECK_IF_SCREEN_FILLED_HEADING(_("Other"));

			PRINT_AND_COLON_TABS(_("ignore locale"), ""); str += b2yn(ignore_locale, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("rpn"), ""); str += b2oo(rpn_mode, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("save definitions"), ""); str += b2yn(save_defs_on_exit, false); CHECK_IF_SCREEN_FILLED_PUTS(str.c_str())
			PRINT_AND_COLON_TABS(_("save mode"), ""); str += b2yn(save_mode_on_exit, false);
			puts("");
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "help", _("help")) || str == "?") {
			INIT_SCREEN_CHECK
			CHECK_IF_SCREEN_FILLED_PUTS("");
			CHECK_IF_SCREEN_FILLED_PUTS(_("Enter a mathematical expression or a command and press enter.")); 
			CHECK_IF_SCREEN_FILLED_PUTS(_("Complete functions, units and variables with the tabulator key."));
			CHECK_IF_SCREEN_FILLED_PUTS("");
			CHECK_IF_SCREEN_FILLED_PUTS(_("Available commands are:"));
			CHECK_IF_SCREEN_FILLED_PUTS("");
			PUTS_UNICODE(_("approximate")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("assume"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("ASSUMPTIONS")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("base"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("BASE")); CHECK_IF_SCREEN_FILLED			
			FPUTS_UNICODE(_("delete"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("NAME")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("exact")); CHECK_IF_SCREEN_FILLED
			if(canfetch) {
				FPUTS_UNICODE(_("exrates"), stdout);
			}
			puts(""); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("factor")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("find"), stdout); fputs("/", stdout); FPUTS_UNICODE(_("list"), stdout);  fputs(" [", stdout); FPUTS_UNICODE(_("NAME"), stdout); puts("]"); CHECK_IF_SCREEN_FILLED;
			FPUTS_UNICODE(_("function"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("NAME"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("EXPRESSION")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("info")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("mode")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("partial fraction")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("save"), stdout); fputs("/", stdout); FPUTS_UNICODE(_("store"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("NAME"), stdout); fputs(" [", stdout); FPUTS_UNICODE(_("CATEGORY"), stdout); fputs("] [", stdout); FPUTS_UNICODE(_("TITLE"), stdout); puts("]"); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("save definitions")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("save mode")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("set"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("OPTION"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("VALUE")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("expand")); CHECK_IF_SCREEN_FILLED			
			FPUTS_UNICODE(_("to"), stdout); fputs("/", stdout); FPUTS_UNICODE(_("convert"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("UNIT or \"TO\" COMMAND")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("variable"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("NAME"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("EXPRESSION"), stdout); CHECK_IF_SCREEN_FILLED_PUTS("");
			FPUTS_UNICODE(_("quit"), stdout); fputs("/", stdout); PUTS_UNICODE(_("exit")); CHECK_IF_SCREEN_FILLED_PUTS("");
			PUTS_UNICODE(_("Commands for RPN mode:")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("rpn"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("STATE")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("stack")); CHECK_IF_SCREEN_FILLED
			PUTS_UNICODE(_("clear stack")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("copy"), stdout); fputs(" [", stdout); FPUTS_UNICODE(_("INDEX"), stdout); puts("]"); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("move"), stdout); fputs(" ", stdout); FPUTS_UNICODE(_("INDEX 1"), stdout); fputs(" ", stdout); PUTS_UNICODE(_("INDEX 2")); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("pop"), stdout); fputs(" [", stdout); FPUTS_UNICODE(_("INDEX"), stdout); puts("]"); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("rotate"), stdout); fputs(" [", stdout); FPUTS_UNICODE(_("DIRECTION"), stdout); puts("]"); CHECK_IF_SCREEN_FILLED
			FPUTS_UNICODE(_("swap"), stdout); fputs(" [", stdout); FPUTS_UNICODE(_("INDEX 1"), stdout); fputs("] [", stdout); FPUTS_UNICODE(_("INDEX 2"), stdout); puts("]"); CHECK_IF_SCREEN_FILLED			
			CHECK_IF_SCREEN_FILLED_PUTS("");
			CHECK_IF_SCREEN_FILLED_PUTS(_("Type help COMMAND for more information (example: help save)."));
			CHECK_IF_SCREEN_FILLED_PUTS(_("Type info NAME for information about a function, variable or unit (example: info sin)."));
			CHECK_IF_SCREEN_FILLED_PUTS(_("When a line begins with '/', the following text is always interpreted as a command."));
			CHECK_IF_SCREEN_FILLED_PUTS("");
			CHECK_IF_SCREEN_FILLED_PUTS(_("For more information about mathematical expression, different options, and a complete list of functions, variables and units, see the relevant sections in the manual of the graphical user interface (available at http://qalculate.github.io/manual/index.html)."));
			puts("");
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "list", _("list"))) {
			list_defs(true);
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "list", _("list")) || EQUALS_IGNORECASE_AND_LOCAL(scom, "find", _("find"))) {
			str = str.substr(ispace + 1);
			remove_blank_ends(str);
			size_t i = str.find_first_of(SPACES);
			string str1, str2;
			if(i == string::npos) {
				str1 = str;
			} else {
				str1 = str.substr(0, i);
				str2 = str.substr(i + 1);
				remove_blank_ends(str2);
			}
			char list_type = 0;
			if(EQUALS_IGNORECASE_AND_LOCAL(str1, "currencies", _("currencies"))) list_type = 'c';
			else if(EQUALS_IGNORECASE_AND_LOCAL(str1, "functions", _("functions"))) list_type = 'f';
			else if(EQUALS_IGNORECASE_AND_LOCAL(str1, "variables", _("variables"))) list_type = 'v';
			else if(EQUALS_IGNORECASE_AND_LOCAL(str1, "units", _("units"))) list_type = 'u';
			else if(!str2.empty()) {
				if(equalsIgnoreCase(str, _("currencies"))) list_type = 'c';
				else if(equalsIgnoreCase(str, _("functions"))) list_type = 'f';
				else if(equalsIgnoreCase(str, _("variables"))) list_type = 'v';
				else if(equalsIgnoreCase(str, _("units"))) list_type = 'u';
				if(list_type != 0) str2 = "";
			} else str2 = str;
			list_defs(true, list_type, str2);
		//qalc command
		} else if(EQUALS_IGNORECASE_AND_LOCAL(scom, "info", _("info"))) {
			int pctl;
#define PRINT_AND_COLON_TABS_INFO(x) FPUTS_UNICODE(x, stdout); pctl = unicode_length_check(x); if(pctl >= 23) fputs(":\t", stdout); else if(pctl >= 15) fputs(":\t\t", stdout); else if(pctl >= 7) fputs(":\t\t\t", stdout); else fputs(":\t\t\t\t", stdout);
#define STR_AND_COLON_TABS_INFO(x) pctl = unicode_length(x); if(pctl >= 23) x += ":\t"; else if(pctl >= 15) x += ":\t\t"; else if(pctl >= 7) x += ":\t\t\t"; else x += ":\t\t\t\t";
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
						if(!f->example(true).empty()) {
							CHECK_IF_SCREEN_FILLED_PUTS(""); 
							str = _("Example:"); str += " "; str += f->example(false, ename->name);
							CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
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
						if(f->subtype() == SUBTYPE_USER_FUNCTION) {
							CHECK_IF_SCREEN_FILLED_PUTS("");
							str = _("Expression:"); str += " "; str += CALCULATOR->unlocalizeExpression(((UserFunction*) f)->formula());
							CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
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
								if(&item->getName(i2) != ename && !item->getName(i2).completion_only) {
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
								bool is_relative = false;
								if(!au->uncertainty(&is_relative).empty()) {
									puts("");
									if(is_relative) {PRINT_AND_COLON_TABS_INFO(_("Relative uncertainty"));}
									else {PRINT_AND_COLON_TABS_INFO(_("Uncertainty"));}
									PUTS_UNICODE(CALCULATOR->localizeExpression(au->uncertainty()).c_str())
								} else if(item->isApproximate()) {
									fputs(" (", stdout);
									FPUTS_UNICODE(_("approximate"), stdout);
									fputs(")", stdout);
									
								}
								if(!au->inverseExpression().empty()) {
									puts("");
									PRINT_AND_COLON_TABS_INFO(_("Inverse Relation"));
									FPUTS_UNICODE(CALCULATOR->localizeExpression(au->inverseExpression()).c_str(), stdout);
									if(au->uncertainty().empty() && item->isApproximate()) {
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
							if(&item->getName(i2) != ename && !item->getName(i2).completion_only) {
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
							} else {
								if(((KnownVariable*) v)->get().isMatrix()) {
									value = _("matrix");
								} else if(((KnownVariable*) v)->get().isVector()) {
									value = _("vector");
								} else {
									PrintOptions po;
									po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
									value = CALCULATOR->print(((KnownVariable*) v)->get(), 30, po);
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
						puts("");
						bool is_relative = false;
						if(v->isKnown() && ((KnownVariable*) v)->isExpression() && !((KnownVariable*) v)->uncertainty(&is_relative).empty()) {
							PRINT_AND_COLON_TABS_INFO(_("Value"));
							FPUTS_UNICODE(value.c_str(), stdout);
							puts("");
							if(is_relative) {PRINT_AND_COLON_TABS_INFO(_("Relative uncertainty"));}
							else {PRINT_AND_COLON_TABS_INFO(_("Uncertainty"));}
							PUTS_UNICODE(CALCULATOR->localizeExpression(((KnownVariable*) v)->uncertainty()).c_str())
						} else {
							string value_pre = _("Value");
							STR_AND_COLON_TABS_INFO(value_pre);
							value.insert(0, value_pre);
							bool b_approx = item->isApproximate();
							if(b_approx && v->isKnown()) {
								if(((KnownVariable*) v)->isExpression()) {
									b_approx = ((KnownVariable*) v)->expression().find(SIGN_PLUSMINUS) == string::npos && ((KnownVariable*) v)->expression().find(CALCULATOR->f_interval->referenceName()) == string::npos;
								} else {
									b_approx = ((KnownVariable*) v)->get().containsInterval(true, false, false, 0, true) <= 0;
								}
							}
							if(b_approx) {
								value += " (";
								value += _("approximate");
								value += ")";
							}
							int tabs = 0;
							for(size_t i = 0; i < value_pre.length(); i++) {
								if(value_pre[i] == '\t') {
									if(tabs == 0) tabs += (7 - ((i - 1) % 8));
									else tabs += 7;
								}
							}
							INIT_COLS
							addLineBreaks(value, cols, true, unicode_length(value_pre) + tabs, unicode_length(value_pre) + tabs);
							PUTS_UNICODE(value.c_str());
						}
						if(v->isKnown() && ((KnownVariable*) v)->isExpression() && !((KnownVariable*) v)->unit().empty()) {
							PRINT_AND_COLON_TABS_INFO(_("Unit"));
							PUTS_UNICODE(CALCULATOR->localizeExpression(((KnownVariable*) v)->unit()).c_str())
						}
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
			remove_duplicate_blanks(str);
			if(EQUALS_IGNORECASE_AND_LOCAL(str, "factor", _("factor"))) {
				puts("");
				PUTS_UNICODE(_("Factorizes the current result."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "partial fraction", _("partial fraction"))) {
				puts("");
				PUTS_UNICODE(_("Applies partial fraction decomposition to the current result."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "simplify", _("simplify")) || EQUALS_IGNORECASE_AND_LOCAL(str, "expand", _("expand"))) {
				puts("");
				PUTS_UNICODE(_("Expands the current result."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "set", _("set"))) {
				INIT_SCREEN_CHECK
#define STR_AND_TABS_SET(x, s) str = "- "; BEGIN_BOLD(str); str += x; END_BOLD(str); if(strlen(s) > 0) {str += " ("; str += s; str += ")";} str += "\n";
#define SET_DESCRIPTION(s) if(strlen(s) > 0) {BEGIN_ITALIC(str); str += s; END_ITALIC(str); str += "\n";}
#define STR_AND_TABS_BOOL(s, sh, d, v) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "("; str += _("on"); if(v) {str += "*";} str += ", "; str += _("off"); if(!v) {str += "*";} str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_YESNO(s, sh, d, v) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "("; str += _("yes"); if(v) {str += "*";} str += ", "; str += _("no"); if(!v) {str += "*";} str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_2(s, sh, d, v, s0, s1, s2) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "(0"; if(v == 0) {str += "*";} str += " = "; str += s0; str += ", 1"; if(v == 1) {str += "*";} str += " = "; str += s1; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s2; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_2b(s, sh, d, v, s0, s1) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "(1"; if(v == 1) {str += "*";} str += " = "; str += s0; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s1; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_3(s, sh, d, v, s0, s1, s2, s3) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "(0"; if(v == 0) {str += "*";} str += " = "; str += s0; str += ", 1"; if(v == 1) {str += "*";} str += " = "; str += s1; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s2; str += ", 3"; if(v == 3) {str += "*";} str += " = "; str += s3; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_4(s, sh, d, v, s0, s1, s2, s3, s4) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "(0"; if(v == 0) {str += "*";} str += " = "; str += s0; str += ", 1"; if(v == 1) {str += "*";} str += " = "; str += s1; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s2; str += ", 3"; if(v == 3) {str += "*";} str += " = "; str += s3; str += ", 4"; if(v == 4) {str += "*";} str += " = "; str += s4; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
#define STR_AND_TABS_7(s, sh, d, v, s0, s1, s2, s3, s4, s5, s6) STR_AND_TABS_SET(s, sh); SET_DESCRIPTION(d); str += "(0"; if(v == 0) {str += "*";} str += " = "; str += s0; str += ", 1"; if(v == 1) {str += "*";} str += " = "; str += s1; str += ", 2"; if(v == 2) {str += "*";} str += " = "; str += s2; str += ", 3"; if(v == 3) {str += "*";} str += " = "; str += s3; str += ", 4"; if(v == 4) {str += "*";} str += " = "; str += s4; str += ", 5"; if(v == 5) {str += "*";} str += " = "; str += s5; str += ", 6"; if(v == 6) {str += "*";} str += " = "; str += s6; str += ")"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Sets the value of an option."));
				CHECK_IF_SCREEN_FILLED_PUTS(_("Example: set base 16."));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Available options and accepted values are (the current value is marked with '*'):"));
				
				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Algebraic Mode"));

				STR_AND_TABS_2b(_("algebra mode"), "alg", _("Determines if the expression is factorized or not after calculation."), evalops.structuring, _("expand"), _("factorize"));
				STR_AND_TABS_BOOL(_("assume nonzero denominators"), "nzd", _("Determines if unknown values will be assumed non-zero (x/x=1)."), evalops.assume_denominators_nonzero);
				STR_AND_TABS_BOOL(_("warn nonzero denominators"), "warnnzd", _("Display a message after a value has been assumed non-zero."), evalops.warn_about_denominators_assumed_nonzero);
				Assumptions *ass = CALCULATOR->defaultAssumptions();
				STR_AND_TABS_SET(_("assumptions"), "ass"); 
				SET_DESCRIPTION(_("Default assumptions for unknown variables."));
			 	str += "(";
				str += _("unknown"); 
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
				str += " + "; str += _("number");
				if(ass->type() == ASSUMPTION_TYPE_NUMBER) str += "*";
				str += ", "; str += _("real");
				if(ass->type() == ASSUMPTION_TYPE_REAL) str += "*";
				str += ", "; str += _("rational");
				if(ass->type() == ASSUMPTION_TYPE_RATIONAL) str += "*";
				str += ", "; str += _("integer");
				if(ass->type() == ASSUMPTION_TYPE_INTEGER) str += "*";
				 str += ")";
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				
				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Calculation"));

				STR_AND_TABS_3(_("angle unit"), "angle", "Default angle unit for trigonometric functions.", evalops.parse_options.angle_unit, _("none"), _("radians"), _("degrees"), _("gradians"));
				STR_AND_TABS_2(_("approximation"), "appr", _("How approximate variables and calculations are handled. In exact mode approximate values will not be calculated."), evalops.approximation, _("exact"), _("try exact"), _("approximate"));
				STR_AND_TABS_BOOL(_("interval arithmetic"), "ia", _("If activated, interval arithmetic determines the final precision of calculations (avoids wrong results after loss of significance) with approximate functions and/or irrational numbers."), CALCULATOR->usesIntervalArithmetic());
				STR_AND_TABS_2b(_("interval calculation"), "ic", _("Determines the method used for interval calculation / uncertainty propagation."), evalops.interval_calculation, _("variance formula"), _("interval arithmetic"));
				STR_AND_TABS_SET(_("precision"), "prec");  
				SET_DESCRIPTION(_("Specifies the default number of significant digits displayed and determines the precision used for approximate calculations."));
				str += "(> 0) "; str += i2s(CALCULATOR->getPrecision()); str += "*"; CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				
				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Enabled Objects"));
				
				STR_AND_TABS_BOOL(_("calculate functions"), "calcfunc", "", evalops.calculate_functions);
				STR_AND_TABS_BOOL(_("calculate variables"), "calcvar", "", evalops.calculate_variables);
				STR_AND_TABS_BOOL(_("complex numbers"), "cplx", "", evalops.allow_complex);
				STR_AND_TABS_BOOL(_("functions"), "func", "", evalops.parse_options.functions_enabled);
				STR_AND_TABS_BOOL(_("infinite numbers"), "inf", "", evalops.allow_infinite);
				STR_AND_TABS_BOOL(_("units"), "", "", evalops.parse_options.units_enabled);
				STR_AND_TABS_BOOL(_("unknowns"), "", _("Interprete undefined symbols in expressions as unknown variables."), evalops.parse_options.unknowns_enabled);
				STR_AND_TABS_BOOL(_("variables"), "var", "", evalops.parse_options.variables_enabled);
				STR_AND_TABS_BOOL(_("variable units"), "varunit", _("If activated physical constants include units (e.g. c = 299 792 458 m∕s)."), CALCULATOR->variableUnitsEnabled());
				
				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Generic Display Options"));

				STR_AND_TABS_BOOL(_("abbreviations"), "abbr", _("Use abbreviated names for units and variables."), printops.abbreviate_names);
				STR_AND_TABS_2(_("division sign"), "divsign", "", printops.division_sign, "/", SIGN_DIVISION_SLASH, SIGN_DIVISION);
				STR_AND_TABS_BOOL(_("excessive parentheses"), "expar", "", printops.excessive_parenthesis);
				STR_AND_TABS_2(_("multiplication sign"), "mulsign", "", printops.multiplication_sign, "*", SIGN_MULTIDOT, SIGN_MULTIPLICATION);
				STR_AND_TABS_BOOL(_("show negative exponents"), "negexp", _("Use negative exponents instead of division in result (x/y = xy^-1)."), printops.negative_exponents);
				STR_AND_TABS_BOOL(_("short multiplication"), "shortmul", "", printops.short_multiplication);
				STR_AND_TABS_BOOL(_("spacious"), "space", _("Add extra space around operators."), printops.spacious);
				STR_AND_TABS_BOOL(_("spell out logical"), "spellout", "", printops.spell_out_logical_operators);
				STR_AND_TABS_BOOL(_("unicode"), "uni", _("Display Unicode characters."), printops.use_unicode_signs);

				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Numerical Display"));

				STR_AND_TABS_SET(_("base"), ""); str += "(-1114112 - 1114112"; str += ", "; str += _("bin");
				if(printops.base == BASE_BINARY) str += "*";
				str += ", "; str += _("oct");
				if(printops.base == BASE_OCTAL) str += "*";
				str += ", "; str += _("dec");
				if(printops.base == BASE_DECIMAL) str += "*";
				str += ", "; str += _("hex");
				if(printops.base == BASE_HEXADECIMAL) str += "*";
				str += ", "; str += _("sexa");
				if(printops.base == BASE_SEXAGESIMAL) str += "*";
				str += ", "; str += _("time");
				if(printops.base == BASE_TIME) str += "*";
				str += ", "; str += _("roman");
				if(printops.base == BASE_ROMAN_NUMERALS) str += "*";
				 str += ")";
				if(printops.base == BASE_CUSTOM) {str += " "; str += CALCULATOR->customOutputBase().print(); str += "*";}
				else if(printops.base == BASE_UNICODE) {str += " "; str += "Unicode"; str += "*";}
				else if(printops.base == BASE_GOLDEN_RATIO) {str += " "; str += "golden ratio"; str += "*";}
				else if(printops.base > 2 && printops.base <= 36 && printops.base != BASE_OCTAL && printops.base != BASE_DECIMAL && printops.base != BASE_HEXADECIMAL) {str += " "; str += i2s(printops.base); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_2(_("base display"), "basedisp", "", printops.base_display, _("none"), _("normal"), _("alternative"));
				STR_AND_TABS_2(_("complex form"), "cplxform", "", evalops.complex_number_form, _("rectangular"), _("exponential"), _("polar"));
				STR_AND_TABS_SET(_("decimal comma"), "");
				SET_DESCRIPTION(_("Determines the default decimal separator."));
				str += "(";
				str += _("locale"); 
				if(b_decimal_comma < 0) str += "*";
				str += ", "; str += _("off");
				if(b_decimal_comma == 0) str += "*";
				str += ", "; str += _("on");
				if(b_decimal_comma > 0) str += "*";
				str += ")";
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_2(_("digit grouping"), "group", "", printops.digit_grouping, _("off"), _("standard"), _("locale"));
				STR_AND_TABS_SET(_("scientific notation"), "exp");
				SET_DESCRIPTION(_("Determines how scientific notation are used (e.g. 5 543 000 = 5.543E6)."));
				str += "(";
				str += _("off"); 
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
				int nff = printops.number_fraction_format;
				if(!printops.restrict_fraction_length && printops.number_fraction_format == FRACTION_FRACTIONAL) nff = 4;
				STR_AND_TABS_4(_("fractions"), "fr", _("Determines how rational numbers are displayed (e.g. 5/4 = 1 + 1/4 = 1.25). 'long' removes limits on the size of the numerator and denonimator."), nff, _("off"), _("exact"), _("on"), _("mixed"), _("long"));
				STR_AND_TABS_7(_("interval display"), "ivdisp", "", (adaptive_interval_display ? 0 : printops.interval_display + 1), _("adaptive"), _("significant"), _("interval"), _("plusminus"), _("midpoint"), _("upper"), _("lower"))
				STR_AND_TABS_BOOL(_("lowercase e"), "lowe", _("Use lowercase e for E-notation (5e2 = 5 * 10^2)."), printops.lower_case_e);
				STR_AND_TABS_BOOL(_("lowercase numbers"), "lownum", _("Use lowercase letters for number bases > 10."), printops.lower_case_numbers);
				STR_AND_TABS_SET(_("max decimals"), "maxdeci"); 
				str += "(";
				str += _("off");
				if(printops.max_decimals < 0) str += "*";
				str += ", >= 0)";
				if(printops.max_decimals >= 0) {str += " "; str += i2s(printops.max_decimals); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_SET(_("min decimals"), "mindeci"); 
				str += "(";
				str += _("off");
				if(printops.min_decimals < 0) str += "*";
				str += ", >= 0)";
				if(printops.min_decimals >= 0) {str += " "; str += i2s(printops.min_decimals); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_BOOL(_("repeating decimals"), "repdec", _("If activated, 1/6 is displayed as '0.1 666...', otherwise as '0.166667'."), printops.indicate_infinite_series);
				STR_AND_TABS_BOOL(_("round to even"), "rndeven", _("Determines whether halfway numbers are rounded upwards or towards the nearest even integer."), printops.round_halfway_to_even);
				STR_AND_TABS_BOOL(_("show ending zeroes"), "zeroes", _("If actived, zeroes are kept at the end of approximate numbers."), printops.show_ending_zeroes);
				STR_AND_TABS_BOOL(_("two's complement"), "twos", _("Enables two's complement representation for display of negative binary numbers."), printops.twos_complement);
				
				STR_AND_TABS_BOOL(_("hexadecimal two's"), "hextwos", _("Enables two's complement representation for display of negative hexadecimal numbers."), printops.twos_complement);
				
				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Parsing"));

				STR_AND_TABS_BOOL(_("caret as xor"), "xor^", _("Use ^ as bitwise exclusive OR operator."), caret_as_xor);
				STR_AND_TABS_SET(_("decimal comma"), ""); 
				SET_DESCRIPTION(_("Determines the default decimal separator."));
				str += "(";
				str += _("locale"); 
				if(b_decimal_comma < 0) str += "*";
				str += ", "; str += _("off");
				if(b_decimal_comma == 0) str += "*";
				str += ", "; str += _("on");
				if(b_decimal_comma > 0) str += "*";
				str += ")";
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				if(CALCULATOR->getDecimalPoint() != COMMA) {
					STR_AND_TABS_BOOL(_("ignore comma"), "", _("Allows use of ',' as thousands separator."), evalops.parse_options.comma_as_separator);
				}
				if(CALCULATOR->getDecimalPoint() != DOT) {
					STR_AND_TABS_BOOL(_("ignore dot"), "", _("Allows use of '.' as thousands separator."), evalops.parse_options.dot_as_separator);
				}
				STR_AND_TABS_SET(_("input base"), "inbase"); str += "(-1114112 - 1114112"; str += ", "; str += _("bin");
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
				if(evalops.parse_options.base == BASE_CUSTOM) {str += " "; str += CALCULATOR->customInputBase().print(); str += "*";}
				else if(evalops.parse_options.base == BASE_UNICODE) {str += " "; str += "Unicode"; str += "*";}
				else if(evalops.parse_options.base == BASE_GOLDEN_RATIO) {str += " "; str += "golden ratio"; str += "*";}
				else if(evalops.parse_options.base > 2 && evalops.parse_options.base != BASE_OCTAL && evalops.parse_options.base != BASE_DECIMAL && evalops.parse_options.base != BASE_HEXADECIMAL) {str += " "; str += i2s(evalops.parse_options.base); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_BOOL(_("limit implicit multiplication"), "limimpl", "", evalops.parse_options.limit_implicit_multiplication);
				STR_AND_TABS_2(_("parsing mode"), "parse", _("See 'help parsing mode'."), evalops.parse_options.parsing_mode, _("adaptive"), _("implicit first"), _("conventional"));
				STR_AND_TABS_2(_("read precision"), "readprec", _("If activated, numbers be interpreted as approximate with precision equal to the number of significant digits (3.20 = 3.20+/-0.005)."), evalops.parse_options.read_precision, _("off"), _("always"), _("when decimals"))
				STR_AND_TABS_BOOL(_("rpn syntax"), "rpnsyn", "", evalops.parse_options.rpn);

				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Units"));

				STR_AND_TABS_BOOL(_("all prefixes"), "allpref", _("Enables automatic use of hecto, deca, deci, and centi."), printops.use_all_prefixes);
				STR_AND_TABS_SET(_("autoconversion"), "conv");
				SET_DESCRIPTION(_("Controls automatic unit conversion of the result. 'optimalsi' always converts non-SI units, while 'optimal' only converts to more optimal unit expressions, with less units and exponents."));
				str += "(";
				str += (_("none"));
				if(evalops.auto_post_conversion == POST_CONVERSION_NONE && evalops.mixed_units_conversion == MIXED_UNITS_CONVERSION_NONE) str += "*";
				str += ", "; str +=  _("optimal");
				if(evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL) str += "*";
				str += ", "; str += _("base");
				if(evalops.auto_post_conversion == POST_CONVERSION_BASE) str += "*";
				str += ", "; str += _("optimalsi");
				if(evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL_SI) str += "*";
				str += ", "; str += _("mixed");
				if(evalops.auto_post_conversion == POST_CONVERSION_NONE && evalops.mixed_units_conversion > MIXED_UNITS_CONVERSION_NONE) str += "*";
				str += ")";
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				STR_AND_TABS_BOOL(_("currency conversion"), "curconv", _("Enables automatic conversion to the local currency when optimal unit conversion is enabled."), evalops.local_currency_conversion);
				STR_AND_TABS_BOOL(_("denominator prefixes"), "denpref", _("Enables automatic use of prefixes in the denominator of unit expressions."), printops.use_denominator_prefix);
				STR_AND_TABS_BOOL(_("place units separately"), "unitsep", _("If activated, units are separated from variables at the end of the result."), printops.place_units_separately);
				STR_AND_TABS_BOOL(_("prefixes"), "pref", _("Enables automatic use of prefixes in the result."), printops.use_unit_prefixes);
				STR_AND_TABS_BOOL(_("sync units"), "sync", "", evalops.sync_units);
				STR_AND_TABS_SET(_("update exchange rates"), "upxrates"); 
				str += "(-1 = "; str += _("ask"); if(auto_update_exchange_rates < 0) str += "*";
				str += ", 0 = "; str += _("never"); if(auto_update_exchange_rates == 0) str += "*";
				str += ", > 0 = "; str += _("days");
				str += ")";
				if(auto_update_exchange_rates > 0) {str += " "; str += i2s(auto_update_exchange_rates); str += "*";}
				CHECK_IF_SCREEN_FILLED_PUTS(str.c_str());
				
				CHECK_IF_SCREEN_FILLED_HEADING_S(_("Other"));

				STR_AND_TABS_YESNO(_("ignore locale"), "", _("Ignore system language and use English (requires restart)."), ignore_locale);
				STR_AND_TABS_BOOL(_("rpn"), "", _("Activates the Reverse Polish Notation stack."), rpn_mode);
				STR_AND_TABS_YESNO(_("save definitions"), "", _("Save functions, units, and variables on exit."), save_defs_on_exit);
				STR_AND_TABS_YESNO(_("save mode"), "", _("Save settings on exit."), save_mode_on_exit);
				puts("");
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
				str += " +\n"; str += _("number");
				if(ass->type() == ASSUMPTION_TYPE_NUMBER) str += "*";
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
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "variable", _("variable"))) {
				puts("");
				PUTS_UNICODE(_("Create a variables with the specified name and expression."));
				puts("");
				PUTS_UNICODE(_("Example: variable var1 pi / 2."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "function", _("function"))) {
				puts("");
				PUTS_UNICODE(_("Creates a function with the specified name and expression."));
				PUTS_UNICODE(_("Use '\\x', '\\y', '\\z', '\\a', etc. for arguments in the expression."));
				puts("");
				PUTS_UNICODE(_("Example: function func1 5*\\x."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "delete", _("delete"))) {
				puts("");
				PUTS_UNICODE(_("Removes the user-defined variable or function with the specified name."));
				puts("");
				PUTS_UNICODE(_("Example: delete var1."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "mode", _("mode"))) {
				puts("");
				PUTS_UNICODE(_("Displays the current mode."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "list", _("list")) || EQUALS_IGNORECASE_AND_LOCAL(str, "find", _("find"))) {
				puts("");
				PUTS_UNICODE(_("Displays a list of variables, functions and units."));
				PUTS_UNICODE(_("Enter with argument 'currencies', 'functions', 'variables' or 'units' to show a list of all currencies, functions, variables or units. Enter a search term to find matching variables, functions, and/or units. If command is called with no argument all user-definied objects are listed."));
				puts("");
				PUTS_UNICODE(_("Example: list functions."));
				PUTS_UNICODE(_("Example: find dinar."));
				PUTS_UNICODE(_("Example: find variables planck."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "info", _("info"))) {
				puts("");
				PUTS_UNICODE(_("Displays information about a function, variable or unit."));
				puts("");
				PUTS_UNICODE(_("Example: info sin."));
				puts("");
			} else if(canfetch && EQUALS_IGNORECASE_AND_LOCAL(str, "exrates", _("exrates"))) {
				puts("");
				PUTS_UNICODE(_("Downloads current exchange rates from the Internet."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rpn", _("rpn"))) {
				puts("");
				PUTS_UNICODE(_("(De)activates the Reverse Polish Notation stack and syntax."));
				puts("");
				PUTS_UNICODE(_("\"syntax\" activates only the RPN syntax and \"stack\" enables the RPN stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "clear stack", _("clear stack"))) {
				puts("");
				PUTS_UNICODE(_("Clears the entire RPN stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "pop", _("pop"))) {
				puts("");
				PUTS_UNICODE(_("Removes the top of the RPN stack or the value at the specified index."));
				puts("");
				PUTS_UNICODE(_("Index 1 is the top of stack and negative index values counts from the bottom of the stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "stack", _("stack"))) {
				puts("");
				PUTS_UNICODE(_("Displays the RPN stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "swap", _("swap"))) {
				puts("");
				PUTS_UNICODE(_("Swaps position of values on the RPN stack."));
				puts("");
				FPUTS_UNICODE(_("If no index is specified, the values on the top of the stack (index 1 and index 2) will be swapped and if only one index is specified, the value at this index will be swapped with the top value."), stdout);
				fputs(" ", stdout);
				PUTS_UNICODE(_("Index 1 is the top of stack and negative index values counts from the bottom of the stack."));
				puts("");
				PUTS_UNICODE(_("Example: swap 2 4"));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "copy", _("copy"))) {
				puts("");
				PUTS_UNICODE(_("Duplicates a value on the RPN stack to the top of the stack."));
				puts("");
				FPUTS_UNICODE(_("If no index is specified, the top of the stack is duplicated."), stdout);
				fputs(" ", stdout);
				PUTS_UNICODE(_("Index 1 is the top of stack and negative index values counts from the bottom of the stack."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "rotate", _("rotate"))) {
				puts("");
				PUTS_UNICODE(_("Rotates the RPN stack up (default) or down."));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "move", _("move"))) {
				puts("");
				PUTS_UNICODE(_("Changes the position of a value on the RPN stack."));
				puts("");
				PUTS_UNICODE(_("Index 1 is the top of stack and negative index values counts from the bottom of the stack."));
				puts("");
				PUTS_UNICODE(_("Example: move 2 4"));
				puts("");
			} else if(EQUALS_IGNORECASE_AND_LOCAL(str, "base", _("base"))) {
				puts("");
				PUTS_UNICODE(_("Sets the result number base (equivalent to set base)."));
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
				INIT_SCREEN_CHECK
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Converts units or changes number base in current result."));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Possible values:"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("- a unit (e.g. meter)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("prepend with ? to request the optimal prefix"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("prepend with + or - to force/disable use of mixed units"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- a unit expression (e.g. km/h)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- a physical constant (e.g. c)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- base (convert to base units)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- optimal (convert to optimal unit)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- mixed (convert to mixed units, e.g. hours + minutes)"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("- bin / binary (show as binary number)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- oct / octal (show as octal number)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- duo / duodecimal (show as duodecimal number)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- hex / hexadecimal (show as hexadecimal number)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- sex / sexagesimal (show as sexagesimal number)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- roman (show as roman numerals)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- time (show in time format)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- bases (show as binary, octal, decimal and hexadecimal number)"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("- rectangular / cartesian (show complex numbers in rectangular form)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- exponential (show complex numbers in exponential form)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- polar (show complex numbers in polar form)"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("- fraction (show result as mixed fraction)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- factors (factorize result)"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("- UTC (show date and time in UTC time zone)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- UTC+/-hh[:mm] (show date and time in specified time zone)"));
				CHECK_IF_SCREEN_FILLED_PUTS(_("- calendars"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				CHECK_IF_SCREEN_FILLED_PUTS(_("Example: to ?g"));
				CHECK_IF_SCREEN_FILLED_PUTS("");
				if(EQUALS_IGNORECASE_AND_LOCAL(str, "to", _("to"))) {
					CHECK_IF_SCREEN_FILLED_PUTS(_("This command can also be typed directly at the end of the mathematical expression."));
					CHECK_IF_SCREEN_FILLED_PUTS("");
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
		} else if(explicit_command) {
			PUTS_UNICODE(_("Unknown command."));
			puts("");
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
			for(int i = history_length - 1; i >= 0; i--) {
				HIST_ENTRY *hist = history_get(i + history_base);
				if(hist && hist->line && strcmp(hist->line, rlbuffer) == 0) {
					hist = remove_history(i);
					if(hist) free_history_entry(hist);
					break;
				}
			}
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

bool display_errors(bool goto_input, int cols) {
	if(!CALCULATOR->message()) return false;
	while(true) {
		if(!hide_parse_errors || (CALCULATOR->message()->stage() != MESSAGE_STAGE_PARSING && CALCULATOR->message()->stage() != MESSAGE_STAGE_CONVERSION_PARSING)) {
			MessageType mtype = CALCULATOR->message()->type();
			string str;
			if(goto_input) str += "  ";
			if(mtype == MESSAGE_ERROR) {
				str += _("error"); str += ": ";
			} else if(mtype == MESSAGE_WARNING) {
				str += _("warning"); str += ": ";
			}
			str += CALCULATOR->message()->message();
			if(cols) addLineBreaks(str, cols, true, goto_input ? 2 : 0, str.length());
			PUTS_UNICODE(str.c_str())
		}
		if(!CALCULATOR->nextMessage()) break;
	}
	return true;
}

void on_abort_display() {
	CALCULATOR->abort();
}

void replace_quotation_marks(string &str) {
#ifndef _WIN32
	if(cfile) return;
	size_t i1 = 0, i2 = 0;
	size_t i_equals = str.find(_("approx.")) + strlen(_("approx."));
	while(i1 + 2 < str.length()) {
		i1 = str.find_first_of("\"\'", i1);
		if(i1 == string::npos) break;
		i2 = str.find(str[i1], i1 + 1);
		if(i2 == string::npos) break;
		if(i2 - i1 > 2) {
			if(!text_length_is_one(str.substr(i1 + 1, i2 - i1 - 1))) {
				i1 = i2 + 1;
				continue;
			}
		}
		if(i1 > 1 && str[i1 - 1] == ' ' && (i_equals == string::npos || i1 != i_equals + 1) && is_not_in(OPERATORS SPACES, str[i1 - 2])) {
			if(printops.use_unicode_signs && str[i1 - 2] < 0) {
				size_t i3 = i1 - 2;
				while(i3 > 0 && str[i3] < 0 && (unsigned char) str[i3] < 0xC0) i3--;
				string str2 = str.substr(i3, i1 - i3 - 1);
				if(str2 != SIGN_DIVISION && str2 != SIGN_DIVISION_SLASH && str2 != SIGN_MULTIPLICATION && str2 != SIGN_MULTIDOT && str2 != SIGN_SMALLCIRCLE && str2 != SIGN_MULTIBULLET && str2 != SIGN_MINUS && str2 != SIGN_PLUS && str2 != SIGN_NOT_EQUAL && str2 != SIGN_GREATER_OR_EQUAL && str2 != SIGN_LESS_OR_EQUAL && str2 != SIGN_ALMOST_EQUAL) {
					str.replace(i1 - 1, 2, "\033[3m");
					i2 += 2;
					if(i_equals != string::npos && i1 < i_equals) i_equals += 2;
				} else {
					str.replace(i1, 1, "\033[3m");
					i2 += 3;
					if(i_equals != string::npos && i1 < i_equals) i_equals += 3;
				}
			} else {
				str.replace(i1 - 1, 2, "\033[3m");
				i2 += 2;
				if(i_equals != string::npos && i1 < i_equals) i_equals += 2;
			}
		} else {
			str.replace(i1, 1, "\033[3m");
			i2 += 3;
			if(i_equals != string::npos && i1 < i_equals) i_equals += 3;
		}
		str.replace(i2, 1, "\033[23m");
		if(i_equals != string::npos && i1 < i_equals) i_equals += 4;
		i1 = i2 + 5;
	}
#endif
}

void ViewThread::run() {
	while(true) {

		void *x = NULL;
		if(!read(&x) || !x) break;
		MathStructure m(*((MathStructure*) x));
		x = NULL;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		if(x) {
			PrintOptions po;
			po.preserve_format = true;
			po.show_ending_zeroes = false;
			po.lower_case_e = printops.lower_case_e;
			po.lower_case_numbers = printops.lower_case_numbers;
			po.base_display = printops.base_display;
			po.twos_complement = printops.twos_complement;
			po.hexadecimal_twos_complement = printops.hexadecimal_twos_complement;
			po.base = evalops.parse_options.base;
			po.abbreviate_names = false;
			po.digit_grouping = printops.digit_grouping;
			po.use_unicode_signs = printops.use_unicode_signs;
			po.multiplication_sign = printops.multiplication_sign;
			po.division_sign = printops.division_sign;
			po.short_multiplication = false;
			po.excessive_parenthesis = true;
			po.improve_division_multipliers = false;
			po.restrict_to_parent_precision = false;
			po.spell_out_logical_operators = printops.spell_out_logical_operators;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			MathStructure mp(*((MathStructure*) x));
			read(&po.is_approximate);
			mp.format(po);
			parsed_text = mp.print(po);
		}
		printops.allow_non_usable = false;
		
		m.removeDefaultAngleUnit(evalops);
		m.format(printops);
		result_text = m.print(printops);
	
		if(result_text == _("aborted")) {
			*printops.is_approximate = false;
		}
		b_busy = false;
		CALCULATOR->stopControl();
	}
}

static bool wait_for_key_press(int timeout_ms) {
#ifdef _WIN32
	return WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), timeout_ms) == WAIT_OBJECT_0;
#else
	fd_set in_set;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = timeout_ms*1000;

	FD_ZERO(&in_set);
	FD_SET(STDIN_FILENO, &in_set);
	return select(FD_SETSIZE, &in_set, NULL, NULL, &timeout) > 0;
#endif
}

void setResult(Prefix *prefix, bool update_parse, bool goto_input, size_t stack_index, bool register_moved, bool noprint) {

	if(i_maxtime < 0) return;

	b_busy = true;
	
	if(!view_thread->running && !view_thread->start()) {b_busy = false; return;}
	
	if(!interactive_mode || cfile) goto_input = false;

	string prev_result_text = result_text;
	result_text = "?";

	if(update_parse) {
		parsed_text = _("aborted");
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

	bool parsed_approx = false;
	if(stack_index == 0) {
		if(!view_thread->write((void*) mstruct)) {b_busy = false; view_thread->cancel(); return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void*) mreg)) {b_busy = false; view_thread->cancel(); return;}
	}
	if(update_parse) {
		if(adaptive_interval_display) {
			if((parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_uncertainty)) || expression_str.find("+/-") != string::npos || expression_str.find("+/" SIGN_MINUS) != string::npos || expression_str.find("±") != string::npos) printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			else if(parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_interval)) printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
			else printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		}
		if(!view_thread->write((void *) parsed_mstruct)) {b_busy = false; view_thread->cancel(); return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write(parsed_approx_p)) {b_busy = false; view_thread->cancel(); return;}
	} else {
		if(!view_thread->write((void *) NULL)) {b_busy = false; view_thread->cancel(); return;}
	}
	
	bool has_printed = false;
	
	if(i_maxtime != 0) {
#ifndef CLOCK_MONOTONIC
		struct timeval tv;
		gettimeofday(&tv, NULL);
		long int i_timeleft = ((long int) t_end.tv_sec - tv.tv_sec) * 1000 + (t_end.tv_usec - tv.tv_usec) / 1000;
#else
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		long int i_timeleft = ((long int) t_end.tv_sec - tv.tv_sec) * 1000 + (t_end.tv_usec - tv.tv_nsec / 1000) / 1000;
#endif
		while(b_busy && view_thread->running && i_timeleft > 0) {
			sleep_ms(10);
			i_timeleft -= 10;
		}
		if(b_busy && view_thread->running) {
			on_abort_display();
			i_maxtime = -1;
		}
	} else {

		int i = 0;
		while(b_busy && view_thread->running && i < 75) {
			sleep_ms(10);
			i++;
		}
		i = 0;

		if(b_busy && view_thread->running && !cfile) {
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
		while(b_busy && view_thread->running) {
			if(cfile) {
				sleep_ms(100);
			} else {
				if(wait_for_key_press(100)) {
#ifdef HAVE_LIBREADLINE		
					if(use_readline) {
						c = rl_read_key();
					} else {
						if(read(STDIN_FILENO, &c, 1) == -1) c = 0;
					}
#else
					if(read(STDIN_FILENO, &c, 1) == -1) c = 0;
#endif			
					if(c == '\n') {
						on_abort_display();
						has_printed = false;
					}
				} else {
					if(!result_only) {
						printf(".");
						fflush(stdout);
					}
					sleep_ms(100);
				}
			}
		}
	}

	printops.prefix = NULL;

	if(noprint) {
		return;
	}

	b_busy = true;
	
	if(has_printed) printf("\n");
	if(goto_input) printf("\n");

	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}

	int cols = 0;

	if(goto_input) {
#ifdef HAVE_LIBREADLINE
		int rows = 0;
		rl_get_screen_size(&rows, &cols);
#else
		cols = 80;
#endif
	}
	
	if(!result_only) display_errors(goto_input, cols);

	if(stack_index != 0) {
		RPNRegisterChanged(result_text, stack_index);
	} else {
		string strout;
		if(goto_input) strout += "  ";
		size_t i_result = 0, i_result_u = 0;
		if(!result_only) {
			if(mstruct->isComparison()) strout += LEFT_PARENTHESIS;
			if(update_parse) {
				strout += parsed_text;
			} else {
				strout += prev_result_text;
			}
			if(mstruct->isComparison()) strout += RIGHT_PARENTHESIS;
			if(!(*printops.is_approximate) && !mstruct->isApproximate()) {
				strout += " = ";
				i_result_u = unicode_length(strout);
				i_result = strout.length();
			} else {
				if(printops.use_unicode_signs) {
					strout += " " SIGN_ALMOST_EQUAL " ";
					i_result_u = unicode_length(strout);
					i_result = strout.length();
				} else {
					strout += " = ";
					i_result_u = unicode_length(strout);
					i_result = strout.length();
					strout += _("approx.");
					strout += " ";
				}
			}
		}
		if(!result_only && mstruct->isComparison()) {
			strout += LEFT_PARENTHESIS;
			strout += result_text.c_str();
			strout += RIGHT_PARENTHESIS;
		} else {
			strout += result_text.c_str();
		}
		if(goto_input) {
			if(!result_only && i_result_u > (size_t) cols / 2 && unicode_length(strout) > (size_t) cols) {
				strout[i_result - 1] = '\n';
				strout.insert(i_result, "  ");
				i_result_u = 2;
			}
			addLineBreaks(strout, cols, true, result_only ? 2 : i_result_u, i_result);
			replace_quotation_marks(strout);
			strout += "\n";
		}
		PUTS_UNICODE(strout.c_str());
	}

	b_busy = false;
}

void viewresult(Prefix *prefix = NULL) {
	setResult(prefix);
}

void result_display_updated() {
	update_message_print_options();
	if(expression_executed) setResult(NULL, false);
}
void result_format_updated() {
	update_message_print_options();
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
void expression_calculation_updated() {
	if(expression_executed && !avoid_recalculation && !rpn_mode) {
		hide_parse_errors = true;
		execute_expression();
		hide_parse_errors = false;
	}
}
void expression_format_updated(bool reparse) {
	if(rpn_mode) reparse = false;
	if(!reparse && !rpn_mode) {
		avoid_recalculation = true;
	}
	if(expression_executed && reparse) {
		execute_expression();
	}
}

void on_abort_command() {
	CALCULATOR->abort();
	int msecs = 5000;
	while(b_busy && msecs > 0) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(b_busy) {
		command_thread->cancel();
		b_busy = false;
		CALCULATOR->stopControl();
		command_aborted = true;
	}
}

void CommandThread::run() {

	enableAsynchronousCancel();

	while(true) {
		int command_type = 0;
		if(!read(&command_type)) break;
		void *x = NULL;
		if(!read(&x) || !x) break;
		CALCULATOR->startControl();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->structure(STRUCTURING_FACTORIZE, evalops, true);
				}
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				((MathStructure*) x)->expandPartialFractions(evalops);
				break;
			}
			case COMMAND_EXPAND: {
				((MathStructure*) x)->expand(evalops);
				break;
			}
			case COMMAND_EVAL: {
				((MathStructure*) x)->eval(evalops);
				break;
			}
		}
		b_busy = false;
		CALCULATOR->stopControl();
		
	}
}

void execute_command(int command_type, bool show_result) {

	if(i_maxtime < 0) return;

	b_busy = true;	
	command_aborted = false;

	if(!command_thread->running && !command_thread->start()) {b_busy = false; return;}

	if(!command_thread->write(command_type)) {command_thread->cancel(); b_busy = false; return;}
	MathStructure *mfactor = new MathStructure(*mstruct);
	if(!command_thread->write((void *) mfactor)) {command_thread->cancel(); mfactor->unref(); b_busy = false; return;}

	if(i_maxtime != 0) {
#ifndef CLOCK_MONOTONIC
		struct timeval tv;
		gettimeofday(&tv, NULL);
		long int i_timeleft = ((long int) t_end.tv_sec - tv.tv_sec) * 1000 + (t_end.tv_usec - tv.tv_usec) / 1000;
#else
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		long int i_timeleft = ((long int) t_end.tv_sec - tv.tv_sec) * 1000 + (t_end.tv_usec - tv.tv_nsec / 1000) / 1000;
#endif
		while(b_busy && command_thread->running && i_timeleft > 0) {
			sleep_ms(10);
			i_timeleft -= 10;
		}
		if(b_busy && command_thread->running) {
			on_abort_command();
			i_maxtime = -1;
			printf(_("aborted"));
			printf("\n");
		}
	} else {

		int i = 0;
		bool has_printed = false;
		while(b_busy && command_thread->running && i < 75) {
			sleep_ms(10);
			i++;
		}
		i = 0;
	
		if(b_busy && command_thread->running && !cfile) {
			if(!result_only) {
				switch(command_type) {
					case COMMAND_FACTORIZE: {
						FPUTS_UNICODE(_("Factorizing (press Enter to abort)"), stdout);
						break;
					}
					case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
						FPUTS_UNICODE(_("Expanding partial fractions…"), stdout);
						break;
					}
					case COMMAND_EXPAND: {
						FPUTS_UNICODE(_("Expanding (press Enter to abort)"), stdout);
						break;
					}
					case COMMAND_EVAL: {
						FPUTS_UNICODE(_("Calculating (press Enter to abort)"), stdout);
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
		while(b_busy && command_thread->running) {
			if(cfile) {
				sleep_ms(100);
			} else {
				if(wait_for_key_press(100)) {
#ifdef HAVE_LIBREADLINE
					if(use_readline) {
						c = rl_read_key();
					} else {
						if(read(STDIN_FILENO, &c, 1) == -1) c = 0;
					}
#else
					if(read(STDIN_FILENO, &c, 1) == -1) c = 0;
#endif			
					if(c == '\n') {
						on_abort_command();
					}
				} else {
					if(!result_only) {
						printf(".");
						fflush(stdout);
					}
					sleep_ms(100);
				}
			}
		}
	
		if(has_printed) printf("\n");

	}

	b_busy = false;

	if(!command_aborted) {
		mstruct->unref();
		mstruct = mfactor;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				printops.allow_factorization = true;
				break;
			}
			case COMMAND_EXPAND: {
				printops.allow_factorization = false;
				break;
			}
			default: {
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
			}
		}
		if(show_result) setResult(NULL, false);
	}

}


void execute_expression(bool goto_input, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, bool check_exrates) {

	if(i_maxtime < 0) return;

	string str;
	bool do_bases = programmers_mode, do_factors = false, do_expand = false, do_fraction = false, do_pfe = false, do_calendars = false;
	avoid_recalculation = false;
	if(!interactive_mode) goto_input = false;
	if(do_stack) {
	} else {
		str = expression_str;
		string from_str = str, to_str;
		if(CALCULATOR->separateToExpression(from_str, to_str, evalops, true)) {
			remove_duplicate_blanks(to_str);
			string to_str1, to_str2;
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "hexadecimal", _("hexadecimal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_HEXADECIMAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", _("binary"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_BINARY;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimal", _("decimal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_DECIMAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", _("octal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_OCTAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "duodecimal", _("duodecimal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_DUODECIMAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_ROMAN_NUMERALS;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, _("sexagesimal"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_SEXAGESIMAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
				int save_base = printops.base;
				expression_str = from_str;
				printops.base = BASE_TIME;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				expression_str = from_str;
				printops.time_zone = TIME_ZONE_UTC;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.time_zone = TIME_ZONE_LOCAL;
				expression_str = str;
				return;
			} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
				to_str = to_str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					CALCULATOR->error(true, _("Time zone parsing failed."), NULL);
				}
				expression_str = from_str;
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = itz;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.custom_time_zone = 0;
				printops.time_zone = TIME_ZONE_LOCAL;
				expression_str = str;
				return;
			} else if(to_str == "CET") {
				expression_str = from_str;
				printops.time_zone = TIME_ZONE_CUSTOM;
				printops.custom_time_zone = 60;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				printops.custom_time_zone = 0;
				printops.time_zone = TIME_ZONE_LOCAL;
				expression_str = str;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "fraction", _("fraction")) || to_str == "frac") {
				str = from_str;
				do_fraction = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", _("factors")) || to_str == "factor") {
				str = from_str;
				do_factors = true;
			}  else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction")) || to_str == "partial") {
				str = from_str;
				do_pfe = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", _("bases"))) {
				do_bases = true;
				str = from_str;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "calendars", _("calendars"))) {
				do_calendars = true;
				str = from_str;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "rectangular", _("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "cartesian", _("cartesian")) || to_str == "rect") {
				expression_str = from_str;
				ComplexNumberForm save_complex_number_form = evalops.complex_number_form;
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.complex_number_form = save_complex_number_form;
				expression_str = str;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "exponential", _("exponential")) || to_str == "exp") {
				expression_str = from_str;
				ComplexNumberForm save_complex_number_form = evalops.complex_number_form;
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.complex_number_form = save_complex_number_form;
				expression_str = str;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "polar", _("polar"))) {
				expression_str = from_str;
				ComplexNumberForm save_complex_number_form = evalops.complex_number_form;
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.complex_number_form = save_complex_number_form;
				expression_str = str;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", _("optimal"))) {
				expression_str = from_str;
				bool b_units_saved = evalops.parse_options.units_enabled;
				evalops.parse_options.units_enabled = true;
				AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
				evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.auto_post_conversion = save_auto_post_conversion;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", _("base"))) {
				expression_str = from_str;
				bool b_units_saved = evalops.parse_options.units_enabled;
				evalops.parse_options.units_enabled = true;
				AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
				evalops.auto_post_conversion = POST_CONVERSION_BASE;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.auto_post_conversion = save_auto_post_conversion;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str1, "base", _("base"))) {
				int save_base = printops.base;
				expression_str = from_str;
				Number save_nr = CALCULATOR->customOutputBase();
				EvaluationOptions eo = evalops;
				eo.parse_options.base = 10;
				MathStructure m;
				CALCULATOR->calculate(&m, to_str2, 500, eo);
				if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
					printops.base = m.number().intValue();
				} else {
					printops.base = BASE_CUSTOM;
					CALCULATOR->setCustomOutputBase(m.number());
				}
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				CALCULATOR->setCustomOutputBase(save_nr);
				printops.base = save_base;
				expression_str = str;
				return;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "mixed", _("mixed"))) {
				expression_str = from_str;
				bool b_units_saved = evalops.parse_options.units_enabled;
				evalops.parse_options.units_enabled = true;
				AutoPostConversion save_auto_post_conversion = evalops.auto_post_conversion;
				MixedUnitsConversion save_mixed_units_conversion = evalops.mixed_units_conversion;
				evalops.auto_post_conversion = POST_CONVERSION_NONE;
				evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.auto_post_conversion = save_auto_post_conversion;
				evalops.mixed_units_conversion = save_mixed_units_conversion;
				expression_str = str;
				evalops.parse_options.units_enabled = b_units_saved;
				return;
			} else if(!evalops.parse_options.units_enabled) {
				evalops.parse_options.units_enabled = true;
				execute_expression(goto_input, do_mathoperation, op, f, do_stack, stack_index);
				evalops.parse_options.units_enabled = false;
				return;
			}
		} else {
			size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
			if(i != string::npos) {
				to_str = str.substr(0, i);
				if(to_str == "factor" || EQUALS_IGNORECASE_AND_LOCAL(to_str, "factorize", _("factorize"))) {
					str = str.substr(i + 1);
					do_factors = true;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "expand", _("expand"))) {
					str = str.substr(i + 1);
					do_expand = true;
				}
			}
		}
	}
	
	if(caret_as_xor) gsub("^", "⊻", str);
	
	expression_executed = true;

	b_busy = true;

	size_t stack_size = 0;

	CALCULATOR->resetExchangeRatesUsed();

	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 0, evalops, parsed_mstruct, NULL);
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
					case '\\': {
						MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
						if(fdiv) {
							CALCULATOR->calculateRPN(fdiv, 0, evalops, parsed_mstruct);
							break;
						}
					}
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
				} else if(str2 == "//") {
					MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
					if(fdiv) {
						CALCULATOR->calculateRPN(fdiv, 0, evalops, parsed_mstruct);
						do_mathoperation = true;
					}
				}
			} else if(str2.length() == 3) {
				if(str2 == "⊻") {
					CALCULATOR->calculateRPN(OPERATION_BITWISE_XOR, 0, evalops, parsed_mstruct);
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
				if(f && f->minargs() > 0) {
					do_mathoperation = true;
					CALCULATOR->calculateRPN(f, 0, evalops, parsed_mstruct);
				} else {
					CALCULATOR->RPNStackEnter(str2, 0, evalops, parsed_mstruct, NULL);
				}
			}
		}
	} else {
		CALCULATOR->calculate(mstruct, CALCULATOR->unlocalizeExpression(str, evalops.parse_options), 0, evalops, parsed_mstruct, NULL);
	}
	
	bool has_printed = false;

	if(i_maxtime != 0) {
#ifndef CLOCK_MONOTONIC
		struct timeval tv;
		gettimeofday(&tv, NULL);
		long int i_timeleft = ((long int) t_end.tv_sec - tv.tv_sec) * 1000 + (t_end.tv_usec - tv.tv_usec) / 1000;
#else
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		long int i_timeleft = ((long int) t_end.tv_sec - tv.tv_sec) * 1000 + (t_end.tv_usec - tv.tv_nsec / 1000) / 1000;
#endif
		while(CALCULATOR->busy() && i_timeleft > 0) {
			sleep_ms(10);
			i_timeleft -= 10;
		}
		if(CALCULATOR->busy()) {
			CALCULATOR->abort();
			i_maxtime = -1;
			printf(_("aborted"));
			printf("\n");
		}
	} else {
	
		int i = 0;
		while(CALCULATOR->busy() && i < 75) {
			sleep_ms(10);
			i++;
		}
		i = 0;
		
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
		while(CALCULATOR->busy()) {
			if(cfile) {
				sleep_ms(100);
			} else {
				if(wait_for_key_press(100)) {
#ifdef HAVE_LIBREADLINE		
					if(use_readline) {
						c = rl_read_key();
					} else {
						if(read(STDIN_FILENO, &c, 1) == -1) c = 0;
					}
#else
					if(read(STDIN_FILENO, &c, 1) == -1) c = 0;
#endif			
					if(c == '\n') {
						CALCULATOR->abort();
						avoid_recalculation = true;
						has_printed = false;
					}
				} else {
					if(!result_only) {
						printf(".");
						fflush(stdout);
					}
					sleep_ms(100);
				}
			}
		}
		if(has_printed) printf("\n");
	}
	
	b_busy = false;

	if(rpn_mode && (!do_stack || stack_index == 0)) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
	}
	
	if(!do_mathoperation && check_exrates && check_exchange_rates()) {
		execute_expression(goto_input, do_mathoperation, op, f, rpn_mode, do_stack ? stack_index : 0, false);
		return;
	}
	
	if(do_factors || do_expand || do_pfe) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			execute_command(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			mstruct = save_mstruct;
		} else {
			execute_command(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
		}
	}
	
	//update "ans" variables
	if(!do_stack || stack_index == 0) {
		MathStructure m4(vans[3]->get());
		m4.replace(vans[4], vans[4]->get());
		vans[4]->set(m4);
		MathStructure m3(vans[2]->get());
		m3.replace(vans[3], vans[4]);
		vans[3]->set(m3);
		MathStructure m2(vans[1]->get());
		m2.replace(vans[2], vans[3]);
		vans[2]->set(m2);
		MathStructure m1(vans[0]->get());
		m1.replace(vans[1], vans[2]);
		vans[1]->set(m1);
		mstruct->replace(vans[0], vans[1]);
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
	
	if(do_calendars && mstruct->isDateTime()) {
		setResult(NULL, (!do_stack || stack_index == 0), false, do_stack ? stack_index : 0, false, true);
		if(goto_input) printf("\n");
		show_calendars(*mstruct->datetime(), goto_input);
		if(goto_input) printf("\n");
	} else if(do_bases) {
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
		if(goto_input) printf("\n");
		if(!result_only) {
			string prestr = parsed_text;
			if(goto_input) prestr.insert(0, "  ");
			if(!(*printops.is_approximate) && !mstruct->isApproximate()) {
				prestr += " = ";
			} else {
				if(printops.use_unicode_signs) {
					prestr += " " SIGN_ALMOST_EQUAL " ";
				} else {
					prestr += " = ";
					prestr += _("approx.");
					prestr += " ";
				}
			}
			base_str = prestr + base_str;
		}
		result_text = base_str;
		if(goto_input) {
			int cols = 0;
#ifdef HAVE_LIBREADLINE
			int rows = 0;
			rl_get_screen_size(&rows, &cols);
#else
			cols = 80;
#endif
			addLineBreaks(base_str, cols, false, 2, base_str.length());
			replace_quotation_marks(base_str);
		}
		PUTS_UNICODE(base_str.c_str());
		printops.base = save_base;
		if(goto_input) printf("\n");
	} else if(do_fraction) {
		NumberFractionFormat save_format = printops.number_fraction_format;
		bool save_rfl = printops.restrict_fraction_length;
		printops.restrict_fraction_length = false;
		if(((!do_stack || stack_index == 0) && mstruct->isNumber()) || (do_stack && stack_index != 0 && CALCULATOR->getRPNRegister(stack_index + 1)->isNumber())) printops.number_fraction_format = FRACTION_COMBINED;
		else printops.number_fraction_format = FRACTION_FRACTIONAL;
		setResult(NULL, (!do_stack || stack_index == 0), goto_input, do_stack ? stack_index : 0);
		printops.restrict_fraction_length = save_rfl;
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
	saved_interval = CALCULATOR->usesIntervalArithmetic();
	saved_adaptive_interval_display = adaptive_interval_display;
	saved_variable_units_enabled = CALCULATOR->variableUnitsEnabled();
	saved_printops = printops;
	saved_printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	saved_evalops = evalops;
	saved_rpn_mode = rpn_mode;
	saved_caret_as_xor = caret_as_xor;
	saved_assumption_type = CALCULATOR->defaultAssumptions()->type();
	saved_assumption_sign = CALCULATOR->defaultAssumptions()->sign();
	saved_custom_output_base = CALCULATOR->customOutputBase();
	saved_custom_input_base = CALCULATOR->customInputBase();
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
	printops.show_ending_zeroes = true;
	printops.digit_grouping = DIGIT_GROUPING_NONE;
	printops.round_halfway_to_even = false;
	printops.number_fraction_format = FRACTION_DECIMAL;
	printops.restrict_fraction_length = false;
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
	printops.twos_complement = true;
	printops.hexadecimal_twos_complement = false;
	printops.division_sign = DIVISION_SIGN_SLASH;
	printops.multiplication_sign = MULTIPLICATION_SIGN_ASTERISK;
	printops.allow_factorization = false;
	printops.spell_out_logical_operators = true;
	printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	
	evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	evalops.approximation = APPROXIMATION_TRY_EXACT;
	evalops.sync_units = true;
	evalops.structuring = STRUCTURING_SIMPLIFY;
	evalops.parse_options.unknowns_enabled = false;
	evalops.parse_options.read_precision = DONT_READ_PRECISION;
	evalops.parse_options.base = BASE_DECIMAL;
	evalops.allow_complex = true;
	evalops.allow_infinite = true;
	evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL;
	evalops.assume_denominators_nonzero = true;
	evalops.warn_about_denominators_assumed_nonzero = true;
	evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	evalops.parse_options.comma_as_separator = false;
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	evalops.local_currency_conversion = true;
	evalops.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;
	b_decimal_comma = -1;
	
	ignore_locale = false;
	
	automatic_fraction = false;
	
	adaptive_interval_display = true;
	
	CALCULATOR->useIntervalArithmetic(true);

	rpn_mode = false;
	
	save_mode_on_exit = true;
	save_defs_on_exit = true;
	auto_update_exchange_rates = -1;
	first_time = false;
	
	FILE *file = NULL;
#ifdef HAVE_LIBREADLINE
	string historyfile = buildPath(getLocalDir(), "qalc.history");
	string oldhistoryfile;
#endif	
	string oldfilename;
	string filename = buildPath(getLocalDir(), "qalc.cfg");
	file = fopen(filename.c_str(), "r");
	if(!file) {
#ifndef _WIN32
		oldfilename = buildPath(getOldLocalDir(), "qalc.cfg");
		file = fopen(oldfilename.c_str(), "r");
#endif
		if(!file) {
			first_time = true;
			save_preferences(true);
			update_message_print_options();
			return;
		}
#ifdef HAVE_LIBREADLINE
#	ifndef _WIN32
		oldhistoryfile = buildPath(getOldLocalDir(), "qalc.history");
#	endif
#endif
		makeDir(getLocalDir());
	}
	
#ifdef HAVE_LIBREADLINE
	stifle_history(100);
	if(!oldhistoryfile.empty()) {
		read_history(oldhistoryfile.c_str());
		move_file(oldhistoryfile.c_str(), historyfile.c_str());
	} else {
		read_history(historyfile.c_str());
	}
#endif

	
	int version_numbers[] = {3, 2, 0};
	
	if(file) {
		char line[10000];
		string stmp, svalue, svar;
		size_t i;
		int v;
		while(true) {
			if(fgets(line, 10000, file) == NULL) break;
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
				} else if(svar == "ignore_locale") {
					ignore_locale = v;
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
				} else if(svar == "interval_arithmetic") {
					if(version_numbers[0] >= 3) CALCULATOR->useIntervalArithmetic(v);
				} else if(svar == "interval_display") {
					if(v == 0) {
						adaptive_interval_display = true;
						printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
					} else {
						v--;
						if(v >= INTERVAL_DISPLAY_SIGNIFICANT_DIGITS && v <= INTERVAL_DISPLAY_UPPER) {
							printops.interval_display = (IntervalDisplay) v;
							adaptive_interval_display = false;
						}
					}
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
				} else if(svar == "variable_units_enabled") {
					CALCULATOR->setVariableUnitsEnabled(v);
				} else if(svar == "use_prefixes") {
					printops.use_unit_prefixes = v;
				} else if(svar == "use_prefixes_for_all_units") {
					printops.use_prefixes_for_all_units = v;
				} else if(svar == "use_prefixes_for_currencies") {
					printops.use_prefixes_for_currencies = v;
				} else if(svar == "number_fraction_format") {
					if(v >= FRACTION_DECIMAL && v <= FRACTION_COMBINED) {
						printops.number_fraction_format = (NumberFractionFormat) v;
						printops.restrict_fraction_length = (v == FRACTION_FRACTIONAL);
					} else if(v == FRACTION_COMBINED + 1) {
						printops.number_fraction_format = FRACTION_FRACTIONAL;
						printops.restrict_fraction_length = false;
					}
				} else if(svar == "automatic_number_fraction_format") {
					automatic_fraction = v;
				} else if(svar == "complex_number_form") {
					if(v >= COMPLEX_NUMBER_FORM_RECTANGULAR && v <= COMPLEX_NUMBER_FORM_POLAR) {
						evalops.complex_number_form = (ComplexNumberForm) v;
					}
				} else if(svar == "number_base") {
					printops.base = v;
				} else if(svar == "custom_number_base") {
					CALCULATOR->setCustomOutputBase(Number(svalue));
				} else if(svar == "number_base_expression") {
					evalops.parse_options.base = v;	
				} else if(svar == "custom_number_base_expression") {
					CALCULATOR->setCustomInputBase(Number(svalue));
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
				} else if(svar == "caret_as_xor") {
					caret_as_xor = v;
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
					if(v >= POST_CONVERSION_NONE && v <= POST_CONVERSION_OPTIMAL) {
						evalops.auto_post_conversion = (AutoPostConversion) v;
					}
					if(v == POST_CONVERSION_NONE && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 12))) {
						evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL;
					}
				} else if(svar == "mixed_units_conversion") {
					if(v >= MIXED_UNITS_CONVERSION_NONE && v <= MIXED_UNITS_CONVERSION_FORCE_ALL) {
						evalops.mixed_units_conversion = (MixedUnitsConversion) v;
					}
				} else if(svar == "local_currency_conversion") {
					evalops.local_currency_conversion = v;
				} else if(svar == "use_unicode_signs") {
					printops.use_unicode_signs = v;	
				} else if(svar == "lower_case_numbers") {
					printops.lower_case_numbers = v;
				} else if(svar == "lower_case_e") {
					printops.lower_case_e = v;
				} else if(svar == "base_display") {
					if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_ALTERNATIVE) printops.base_display = (BaseDisplay) v;
				} else if(svar == "twos_complement") {
					printops.twos_complement = v;
				} else if(svar == "hexadecimal_twos_complement") {
					printops.hexadecimal_twos_complement = v;
				} else if(svar == "spell_out_logical_operators") {
						printops.spell_out_logical_operators = v;
				} else if(svar == "decimal_comma") {
					b_decimal_comma = v;
					if(v == 0) CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
					else if(v > 0) CALCULATOR->useDecimalComma();
				} else if(svar == "dot_as_separator") {
					evalops.parse_options.dot_as_separator = v;
				} else if(svar == "comma_as_separator") {
					evalops.parse_options.comma_as_separator = v;
					if(CALCULATOR->getDecimalPoint() != COMMA) {
						CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
					}
				} else if(svar == "multiplication_sign") {
					if(v >= MULTIPLICATION_SIGN_ASTERISK && v <= MULTIPLICATION_SIGN_X) printops.multiplication_sign = (MultiplicationSign) v;
				} else if(svar == "division_sign") {
					if(v >= DIVISION_SIGN_SLASH && v <= DIVISION_SIGN_DIVISION) printops.division_sign = (DivisionSign) v;
				} else if(svar == "indicate_infinite_series") {
					printops.indicate_infinite_series = v;
				} else if(svar == "show_ending_zeroes") {
					if(version_numbers[0] > 2 || (version_numbers[0] == 2 && version_numbers[1] >= 9)) printops.show_ending_zeroes = v;
				} else if(svar == "digit_grouping") {
					if(v >= DIGIT_GROUPING_NONE && v <= DIGIT_GROUPING_LOCALE) {
						printops.digit_grouping = (DigitGrouping) v;
					}
				} else if(svar == "round_halfway_to_even") {
					printops.round_halfway_to_even = v;
				} else if(svar == "approximation") {
					if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
						evalops.approximation = (ApproximationMode) v;
					}
				} else if(svar == "interval_calculation") {
					if(v >= INTERVAL_CALCULATION_NONE && v <= INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
						evalops.interval_calculation = (IntervalCalculation) v;
					}
				} else if(svar == "in_rpn_mode") {
					rpn_mode = v;
				} else if(svar == "rpn_syntax") {
					evalops.parse_options.rpn = v;
				} else if(svar == "default_assumption_type") {
					if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_INTEGER) {
						if(v < ASSUMPTION_TYPE_NUMBER && version_numbers[0] < 1) v = ASSUMPTION_TYPE_NUMBER;
						if(v == ASSUMPTION_TYPE_COMPLEX && version_numbers[0] < 2) v = ASSUMPTION_TYPE_NUMBER;
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
		if(!oldfilename.empty()) {
			move_file(oldfilename.c_str(), filename.c_str());
		}
		update_message_print_options();
	} else {
		first_time = true;
		save_preferences(true);
		update_message_print_options();
		return;
	}
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
	makeDir(getLocalDir());
#ifdef HAVE_LIBREADLINE	
	write_history(buildPath(getLocalDir(), "qalc.history").c_str());
#endif	
	string filename = buildPath(getLocalDir(), "qalc.cfg");
	file = fopen(filename.c_str(), "w+");
	if(file == NULL) {
		fprintf(stderr, _("Couldn't write preferences to\n%s"), filename.c_str());
		return false;
	}
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", VERSION);	
	fprintf(file, "save_mode_on_exit=%i\n", save_mode_on_exit);
	fprintf(file, "save_definitions_on_exit=%i\n", save_defs_on_exit);
	fprintf(file, "ignore_locale=%i\n", ignore_locale);
	fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
	fprintf(file, "spacious=%i\n", printops.spacious);
	fprintf(file, "excessive_parenthesis=%i\n", printops.excessive_parenthesis);
	fprintf(file, "short_multiplication=%i\n", printops.short_multiplication);
	fprintf(file, "use_unicode_signs=%i\n", printops.use_unicode_signs);
	fprintf(file, "lower_case_numbers=%i\n", printops.lower_case_numbers);
	fprintf(file, "lower_case_e=%i\n", printops.lower_case_e);
	fprintf(file, "base_display=%i\n", printops.base_display);
	fprintf(file, "twos_complement=%i\n", printops.twos_complement);
	fprintf(file, "hexadecimal_twos_complement=%i\n", printops.hexadecimal_twos_complement);
	fprintf(file, "spell_out_logical_operators=%i\n", printops.spell_out_logical_operators);
	fprintf(file, "digit_grouping=%i\n", printops.digit_grouping);
	fprintf(file, "decimal_comma=%i\n", b_decimal_comma);
	fprintf(file, "dot_as_separator=%i\n", evalops.parse_options.dot_as_separator);
	fprintf(file, "comma_as_separator=%i\n", evalops.parse_options.comma_as_separator);
	fprintf(file, "multiplication_sign=%i\n", printops.multiplication_sign);
	fprintf(file, "division_sign=%i\n", printops.division_sign);
	if(mode) {
		if(programmers_mode) {
			int saved_inbase = saved_evalops.parse_options.base;
			int saved_outbase = saved_printops.base;
			bool saved_caret = saved_caret_as_xor;
			set_saved_mode();
			saved_evalops.parse_options.base = saved_inbase;
			saved_printops.base = saved_outbase;
			saved_caret_as_xor = saved_caret;
		} else {
			set_saved_mode();
		}
	}
	fprintf(file, "\n[Mode]\n");
	fprintf(file, "min_deci=%i\n", saved_printops.min_decimals);
	fprintf(file, "use_min_deci=%i\n", saved_printops.use_min_decimals);
	fprintf(file, "max_deci=%i\n", saved_printops.max_decimals);
	fprintf(file, "use_max_deci=%i\n", saved_printops.use_max_decimals);	
	fprintf(file, "precision=%i\n", saved_precision);
	fprintf(file, "interval_arithmetic=%i\n", saved_interval);
	if(saved_adaptive_interval_display) fprintf(file, "interval_display=%i\n", 0);
	else fprintf(file, "interval_display=%i\n", saved_printops.interval_display + 1);
	fprintf(file, "min_exp=%i\n", saved_printops.min_exp);
	fprintf(file, "negative_exponents=%i\n", saved_printops.negative_exponents);
	fprintf(file, "sort_minus_last=%i\n", saved_printops.sort_options.minus_last);
	fprintf(file, "number_fraction_format=%i\n", !saved_printops.restrict_fraction_length && saved_printops.number_fraction_format == FRACTION_FRACTIONAL ? FRACTION_COMBINED + 1 : saved_printops.number_fraction_format);
	if(automatic_fraction) fprintf(file, "automatic_number_fraction_format=%i\n", automatic_fraction);
	fprintf(file, "complex_number_form=%i\n", saved_evalops.complex_number_form);
	fprintf(file, "use_prefixes=%i\n", saved_printops.use_unit_prefixes);
	fprintf(file, "use_prefixes_for_all_units=%i\n", saved_printops.use_prefixes_for_all_units);
	fprintf(file, "use_prefixes_for_currencies=%i\n", saved_printops.use_prefixes_for_currencies);
	fprintf(file, "abbreviate_names=%i\n", saved_printops.abbreviate_names);
	fprintf(file, "all_prefixes_enabled=%i\n", saved_printops.use_all_prefixes);
	fprintf(file, "denominator_prefix_enabled=%i\n", saved_printops.use_denominator_prefix);
	fprintf(file, "place_units_separately=%i\n", saved_printops.place_units_separately);
	fprintf(file, "auto_post_conversion=%i\n", saved_evalops.auto_post_conversion);
	fprintf(file, "mixed_units_conversion=%i\n", saved_evalops.mixed_units_conversion);
	fprintf(file, "local_currency_conversion=%i\n", saved_evalops.local_currency_conversion);
	fprintf(file, "number_base=%i\n", saved_printops.base);
	if(saved_printops.base == BASE_CUSTOM) fprintf(file, "custom_number_base=%s\n", saved_custom_output_base.print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "number_base_expression=%i\n", saved_evalops.parse_options.base);
	if(saved_evalops.parse_options.base == BASE_CUSTOM) fprintf(file, "custom_number_base_expression=%s\n", saved_custom_input_base.print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "read_precision=%i\n", saved_evalops.parse_options.read_precision);
	fprintf(file, "assume_denominators_nonzero=%i\n", saved_evalops.assume_denominators_nonzero);
	fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", saved_evalops.warn_about_denominators_assumed_nonzero);
	fprintf(file, "structuring=%i\n", saved_evalops.structuring);
	fprintf(file, "angle_unit=%i\n", saved_evalops.parse_options.angle_unit);
	fprintf(file, "caret_as_xor=%i\n", saved_caret_as_xor);
	fprintf(file, "functions_enabled=%i\n", saved_evalops.parse_options.functions_enabled);
	fprintf(file, "variables_enabled=%i\n", saved_evalops.parse_options.variables_enabled);
	fprintf(file, "calculate_variables=%i\n", saved_evalops.calculate_variables);
	fprintf(file, "calculate_functions=%i\n", saved_evalops.calculate_functions);
	fprintf(file, "variable_units_enabled=%i\n", saved_variable_units_enabled);
	fprintf(file, "sync_units=%i\n", saved_evalops.sync_units);
	fprintf(file, "unknownvariables_enabled=%i\n", saved_evalops.parse_options.unknowns_enabled);
	fprintf(file, "units_enabled=%i\n", saved_evalops.parse_options.units_enabled);
	fprintf(file, "allow_complex=%i\n", saved_evalops.allow_complex);
	fprintf(file, "allow_infinite=%i\n", saved_evalops.allow_infinite);
	fprintf(file, "indicate_infinite_series=%i\n", saved_printops.indicate_infinite_series);
	fprintf(file, "show_ending_zeroes=%i\n", saved_printops.show_ending_zeroes);
	fprintf(file, "round_halfway_to_even=%i\n", saved_printops.round_halfway_to_even);
	fprintf(file, "approximation=%i\n", saved_evalops.approximation);
	fprintf(file, "interval_calculation=%i\n", saved_evalops.interval_calculation);
	fprintf(file, "in_rpn_mode=%i\n", saved_rpn_mode);
	fprintf(file, "rpn_syntax=%i\n", saved_evalops.parse_options.rpn);
	fprintf(file, "limit_implicit_multiplication=%i\n", saved_evalops.parse_options.limit_implicit_multiplication);
	fprintf(file, "parsing_mode=%i\n", saved_evalops.parse_options.parsing_mode);
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



