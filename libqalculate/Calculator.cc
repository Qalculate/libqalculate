/*
    Qalculate    

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <queue>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#ifdef HAVE_LIBCURL
#	include <curl/curl.h>
#endif
#ifdef HAVE_ICU
#	include <unicode/ucasemap.h>
#endif

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
#elif 	defined(__GNUC__)

#	ifndef __has_include
#	define __has_include(x) 0
#	endif

#	if (defined(__clang__) && __has_include(<tr1/unordered_map>)) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#		include <tr1/unordered_map>
		namespace Sgi = std;
#		define unordered_map std::tr1::unordered_map
#	else
#		if __GNUC__ < 3
#			include <hash_map.h>
			namespace Sgi { using ::hash_map; }; // inherit globals
#		else
#			include <ext/hash_map>
#			if __GNUC__ == 3 && __GNUC_MINOR__ == 0
				namespace Sgi = std;               // GCC 3.0
#			else
				namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
#			endif
#		endif
#		define unordered_map Sgi::hash_map
#	endif
#else      // ...  there are other compilers, right?
	namespace Sgi = std;
#	define unordered_map Sgi::hash_map
#endif

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

PrintOptions::PrintOptions() : min_exp(EXP_PRECISION), base(BASE_DECIMAL), lower_case_numbers(false), lower_case_e(false), number_fraction_format(FRACTION_DECIMAL), indicate_infinite_series(false), show_ending_zeroes(true), abbreviate_names(true), use_reference_names(false), place_units_separately(true), use_unit_prefixes(true), use_prefixes_for_all_units(false), use_prefixes_for_currencies(false), use_all_prefixes(false), use_denominator_prefix(true), negative_exponents(false), short_multiplication(true), limit_implicit_multiplication(false), allow_non_usable(false), use_unicode_signs(false), multiplication_sign(MULTIPLICATION_SIGN_DOT), division_sign(DIVISION_SIGN_DIVISION_SLASH), spacious(true), excessive_parenthesis(false), halfexp_to_sqrt(true), min_decimals(0), max_decimals(-1), use_min_decimals(true), use_max_decimals(true), round_halfway_to_even(false), improve_division_multipliers(true), prefix(NULL), is_approximate(NULL), can_display_unicode_string_function(NULL), can_display_unicode_string_arg(NULL), hide_underscore_spaces(false), preserve_format(false), allow_factorization(false), spell_out_logical_operators(false), restrict_to_parent_precision(true), restrict_fraction_length(false), exp_to_root(false), preserve_precision(false), interval_display(INTERVAL_DISPLAY_INTERVAL), digit_grouping(DIGIT_GROUPING_NONE), date_time_format(DATE_TIME_FORMAT_ISO), time_zone(TIME_ZONE_LOCAL), custom_time_zone(0), twos_complement(true), hexadecimal_twos_complement(false), binary_bits(0) {}

const string &PrintOptions::comma() const {if(comma_sign.empty()) return CALCULATOR->getComma(); return comma_sign;}
const string &PrintOptions::decimalpoint() const {if(decimalpoint_sign.empty()) return CALCULATOR->getDecimalPoint(); return decimalpoint_sign;}

InternalPrintStruct::InternalPrintStruct() : depth(0), power_depth(0), division_depth(0), wrap(false), num(NULL), den(NULL), re(NULL), im(NULL), exp(NULL), minus(NULL), exp_minus(NULL), parent_approximate(false), parent_precision(-1), iexp(NULL) {}

ParseOptions::ParseOptions() : variables_enabled(true), functions_enabled(true), unknowns_enabled(true), units_enabled(true), rpn(false), base(BASE_DECIMAL), limit_implicit_multiplication(false), read_precision(DONT_READ_PRECISION), dot_as_separator(false), brackets_as_parentheses(false), angle_unit(ANGLE_UNIT_NONE), unended_function(NULL), preserve_format(false), default_dataset(NULL), parsing_mode(PARSING_MODE_ADAPTIVE), twos_complement(false), hexadecimal_twos_complement(false) {}

EvaluationOptions::EvaluationOptions() : approximation(APPROXIMATION_TRY_EXACT), sync_units(true), sync_nonlinear_unit_relations(true), keep_prefixes(false), calculate_variables(true), calculate_functions(true), test_comparisons(true), isolate_x(true), expand(true), combine_divisions(false), reduce_divisions(true), allow_complex(true), allow_infinite(true), assume_denominators_nonzero(true), warn_about_denominators_assumed_nonzero(false), split_squares(true), keep_zero_units(true), auto_post_conversion(POST_CONVERSION_OPTIMAL), mixed_units_conversion(MIXED_UNITS_CONVERSION_DEFAULT), structuring(STRUCTURING_SIMPLIFY), isolate_var(NULL), do_polynomial_division(true), protected_function(NULL), complex_number_form(COMPLEX_NUMBER_FORM_RECTANGULAR), local_currency_conversion(true), transform_trigonometric_functions(true), interval_calculation(INTERVAL_CALCULATION_VARIANCE_FORMULA) {}

/*#include <time.h>
#include <sys/time.h>

struct timeval tvtime;
long int usecs, secs, usecs2, usecs3;

#define PRINT_TIME(x) gettimeofday(&tvtime, NULL); usecs2 = tvtime.tv_usec - usecs + (tvtime.tv_sec - secs) * 1000000; printf("%s %li\n", x, usecs2);
#define PRINT_TIMEDIFF(x) gettimeofday(&tvtime, NULL); printf("%s %li\n", x, tvtime.tv_usec - usecs + (tvtime.tv_sec - secs) * 1000000 - usecs2); usecs2 = tvtime.tv_usec - usecs + (tvtime.tv_sec - secs) * 1000000; 
#define ADD_TIME1 gettimeofday(&tvtime, NULL); usecs2 = tvtime.tv_usec - usecs + (tvtime.tv_sec - secs) * 1000000; 
#define ADD_TIME2 gettimeofday(&tvtime, NULL); usecs3 += tvtime.tv_usec - usecs + (tvtime.tv_sec - secs) * 1000000 - usecs2; */

typedef void (*CREATEPLUG_PROC)();

PlotParameters::PlotParameters() {
	auto_y_min = true;
	auto_x_min = true;
	auto_y_max = true;
	auto_x_max = true;
	y_log = false;
	x_log = false;
	y_log_base = 10;
	x_log_base = 10;
	grid = false;
	color = true;
	linewidth = -1;
	show_all_borders = false;
	legend_placement = PLOT_LEGEND_TOP_RIGHT;	
}
PlotDataParameters::PlotDataParameters() {
	yaxis2 = false;
	xaxis2 = false;
	style = PLOT_STYLE_LINES;
	smoothing = PLOT_SMOOTHING_NONE;
	test_continuous = false;
}

CalculatorMessage::CalculatorMessage(string message_, MessageType type_, int cat_, int stage_) {
	mtype = type_;
	i_stage = stage_;
	i_cat = cat_;
	smessage = message_;
}
CalculatorMessage::CalculatorMessage(const CalculatorMessage &e) {
	mtype = e.type();
	i_stage = e.stage();
	i_cat = e.category();
	smessage = e.message();
}
string CalculatorMessage::message() const {
	return smessage;
}
const char* CalculatorMessage::c_message() const {
	return smessage.c_str();
}
MessageType CalculatorMessage::type() const {
	return mtype;
}
int CalculatorMessage::stage() const {
	return i_stage;
}
int CalculatorMessage::category() const {
	return i_cat;
}

void Calculator::addStringAlternative(string replacement, string standard) {
	signs.push_back(replacement);
	real_signs.push_back(standard);
}
bool Calculator::delStringAlternative(string replacement, string standard) {
	for(size_t i = 0; i < signs.size(); i++) {
		if(signs[i] == replacement && real_signs[i] == standard) {
			signs.erase(signs.begin() + i);
			real_signs.erase(real_signs.begin() + i);
			return true;
		}
	}
	return false;
}
void Calculator::addDefaultStringAlternative(string replacement, string standard) {
	default_signs.push_back(replacement);
	default_real_signs.push_back(standard);
}
bool Calculator::delDefaultStringAlternative(string replacement, string standard) {
	for(size_t i = 0; i < default_signs.size(); i++) {
		if(default_signs[i] == replacement && default_real_signs[i] == standard) {
			default_signs.erase(default_signs.begin() + i);
			default_real_signs.erase(default_real_signs.begin() + i);
			return true;
		}
	}
	return false;
}

Calculator *calculator = NULL;

MathStructure m_undefined, m_empty_vector, m_empty_matrix, m_zero, m_one, m_minus_one, m_one_i;
Number nr_zero, nr_one, nr_two, nr_three, nr_minus_one, nr_one_i, nr_minus_i, nr_half, nr_minus_half, nr_plus_inf, nr_minus_inf;
EvaluationOptions no_evaluation;
ExpressionName empty_expression_name;
extern gmp_randstate_t randstate;
#ifdef HAVE_ICU
	extern UCaseMap *ucm;
#endif


enum {
	PROC_RPN_ADD,
	PROC_RPN_SET,
	PROC_RPN_OPERATION_1,
	PROC_RPN_OPERATION_2,
	PROC_RPN_OPERATION_F,
	PROC_NO_COMMAND
};

class CalculateThread : public Thread {
  protected:
	virtual void run();
};



void autoConvert(const MathStructure &morig, MathStructure &mconv, const EvaluationOptions &eo) {
	if(!morig.containsType(STRUCT_UNIT, true)) {
		if(&mconv != &morig) mconv.set(morig);
		return;
	}
	switch(eo.auto_post_conversion) {
		case POST_CONVERSION_OPTIMAL: {
			mconv.set(CALCULATOR->convertToOptimalUnit(morig, eo, false));
			break;
		}
		case POST_CONVERSION_BASE: {
			mconv.set(CALCULATOR->convertToBaseUnits(morig, eo));
			break;
		}
		case POST_CONVERSION_OPTIMAL_SI: {
			mconv.set(CALCULATOR->convertToOptimalUnit(morig, eo, true));
			break;
		}
		default: {
			if(&mconv != &morig) mconv.set(morig);
		}
	}
	if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mconv.set(CALCULATOR->convertToMixedUnits(mconv, eo));
}

void CalculateThread::run() {
	enableAsynchronousCancel();
	while(true) {
		bool b_parse = true;
		if(!read<bool>(&b_parse)) break;
		void *x = NULL;
		if(!read<void *>(&x) || !x) break;
		MathStructure *mstruct = (MathStructure*) x;
		CALCULATOR->startControl();
		if(b_parse) {
			mstruct->setAborted();
			if(CALCULATOR->tmp_parsedstruct) CALCULATOR->tmp_parsedstruct->setAborted();
			//if(CALCULATOR->tmp_tostruct) CALCULATOR->tmp_tostruct->setUndefined();
			mstruct->set(CALCULATOR->calculate(CALCULATOR->expression_to_calculate, CALCULATOR->tmp_evaluationoptions, CALCULATOR->tmp_parsedstruct, CALCULATOR->tmp_tostruct, CALCULATOR->tmp_maketodivision));
		} else {
			MathStructure meval(*mstruct);
			mstruct->setAborted();
			mstruct->set(CALCULATOR->calculate(meval, CALCULATOR->tmp_evaluationoptions));
		}
		switch(CALCULATOR->tmp_proc_command) {
			case PROC_RPN_ADD: {
				CALCULATOR->RPNStackEnter(mstruct, false);
				break;
			}
			case PROC_RPN_SET: {
				CALCULATOR->setRPNRegister(CALCULATOR->tmp_rpnindex, mstruct, false);
				break;
			}
			case PROC_RPN_OPERATION_1: {
				if(CALCULATOR->RPNStackSize() > 0) {
					CALCULATOR->setRPNRegister(1, mstruct, false);
				} else {
					CALCULATOR->RPNStackEnter(mstruct, false);
				}
				break;
			}
			case PROC_RPN_OPERATION_2: {
				if(CALCULATOR->RPNStackSize() > 1) {
					CALCULATOR->deleteRPNRegister(1);
				}
				if(CALCULATOR->RPNStackSize() > 0) {
					CALCULATOR->setRPNRegister(1, mstruct, false);
				} else {
					CALCULATOR->RPNStackEnter(mstruct, false);
				}
				break;
			}
			case PROC_RPN_OPERATION_F: {
				for(size_t i = 0; (CALCULATOR->tmp_proc_registers < 0 || (int) i < CALCULATOR->tmp_proc_registers - 1) && CALCULATOR->RPNStackSize() > 1; i++) {
					CALCULATOR->deleteRPNRegister(1);
				}
				if(CALCULATOR->RPNStackSize() > 0 && CALCULATOR->tmp_proc_registers != 0) {
					CALCULATOR->setRPNRegister(1, mstruct, false);
				} else {
					CALCULATOR->RPNStackEnter(mstruct, false);
				}
				break;
			}
			case PROC_NO_COMMAND: {}
		}
		CALCULATOR->stopControl();
		CALCULATOR->b_busy = false;
	}
}

class Calculator_p {
	public:
		unordered_map<size_t, MathStructure*> id_structs;
		unordered_map<size_t, bool> ids_p;
		vector<size_t> freed_ids;
		size_t ids_i;
		Number custom_input_base, custom_output_base;
		long int custom_input_base_i;
		Unit *local_currency;
};

bool is_not_number(char c, int base) {
	if(c >= '0' && c <= '9') return false;
	if(base == -1) return false;
	if(base == -12) return c != 'E' && c != 'X';
	if(base <= 10) return true;
	if(base <= 36) {
		if(c >= 'a' && c < 'a' + (base - 10)) return false;
		if(c >= 'A' && c < 'A' + (base - 10)) return false;
		return true;
	}
	if(base <= 62) {
		if(c >= 'a' && c < 'a' + (base - 36)) return false;
		if(c >= 'A' && c < 'Z') return false;
		return true;
	}
	return false;
}

#define BITWISE_XOR "⊻"

Calculator::Calculator() {
	b_ignore_locale = false;

#ifdef ENABLE_NLS
	if(!b_ignore_locale) {
		bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	}
#endif

	if(b_ignore_locale) {
		char *current_lc_monetary = setlocale(LC_MONETARY, "");
		if(current_lc_monetary) saved_locale = strdup(current_lc_monetary);
		else saved_locale = NULL;
		setlocale(LC_ALL, "C");
		if(saved_locale) {
			setlocale(LC_MONETARY, saved_locale);
			free(saved_locale);
			saved_locale = NULL;
		}
	} else {
		setlocale(LC_ALL, "");
	}


	gmp_randinit_default(randstate);
	gmp_randseed_ui(randstate, (unsigned long int) time(NULL));

	priv = new Calculator_p;
	priv->custom_input_base_i = 0;
	priv->ids_i = 0;
	priv->local_currency = NULL;

#ifdef HAVE_ICU
	UErrorCode err = U_ZERO_ERROR;
	ucm = ucasemap_open(NULL, 0, &err);
#endif

	srand(time(NULL));
	
	exchange_rates_time[0] = 0;
	exchange_rates_time[1] = 0;
	exchange_rates_time[2] = 0;
	exchange_rates_check_time[0] = 0;
	exchange_rates_check_time[1] = 0;
	exchange_rates_check_time[2] = 0;
	b_exchange_rates_warning_enabled = true;
	b_exchange_rates_used = 0;
	
	i_aborted = 0;
	b_controlled = false;
	i_timeout = 0;
	
	setPrecision(DEFAULT_PRECISION);
	b_interval = true;
	i_stop_interval = 0;
	i_start_interval = 0;
	
	b_var_units = true;

	addStringAlternative(SIGN_DIVISION, DIVISION);
	addStringAlternative(SIGN_DIVISION_SLASH, DIVISION);
	addStringAlternative(SIGN_MULTIPLICATION, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MIDDLEDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIBULLET, MULTIPLICATION);
	addStringAlternative(SIGN_SMALLCIRCLE, MULTIPLICATION);
	addStringAlternative(SIGN_MINUS, MINUS);
	addStringAlternative("–", MINUS);
	addStringAlternative(SIGN_PLUS, PLUS);
	addStringAlternative(SIGN_NOT_EQUAL, " " NOT EQUALS);
	addStringAlternative(SIGN_GREATER_OR_EQUAL, GREATER EQUALS);
	addStringAlternative(SIGN_LESS_OR_EQUAL, LESS EQUALS);
	addStringAlternative(";", COMMA);
	addStringAlternative("\t", SPACE);
	addStringAlternative("\n", SPACE);
	addStringAlternative(" ", SPACE);
	addStringAlternative("**", POWER);
	addStringAlternative("↊", "X");
	addStringAlternative("↋", "E");
	addStringAlternative("∧", BITWISE_AND);
	addStringAlternative("∨", BITWISE_OR);
	addStringAlternative("¬", BITWISE_NOT);
	addStringAlternative(SIGN_MICRO, "μ");
	
	per_str = _("per");
	per_str_len = per_str.length();
	times_str = _("times");
	times_str_len = times_str.length();
	plus_str = _("plus");
	plus_str_len = plus_str.length();
	minus_str = _("minus");
	minus_str_len = minus_str.length();
	and_str = _("and");
	if(and_str == "and") and_str = "";
	and_str_len = and_str.length();
	AND_str = "AND";
	AND_str_len = AND_str.length();
	or_str = _("or");
	if(or_str == "or") or_str = "";
	or_str_len = or_str.length();
	OR_str = "OR";
	OR_str_len = OR_str.length();
	XOR_str = "XOR";
	XOR_str_len = XOR_str.length();
	
	char *current_lc_numeric = setlocale(LC_NUMERIC, NULL);
	if(current_lc_numeric) saved_locale = strdup(current_lc_numeric);
	else saved_locale = NULL;
	struct lconv *lc = localeconv();
	if(!lc) {
		setlocale(LC_NUMERIC, "C");
		lc = localeconv();
	}
	place_currency_sign_before = lc->p_cs_precedes;
	place_currency_sign_before_negative = lc->n_cs_precedes;
#ifdef HAVE_STRUCT_LCONV_INT_P_CS_PRECEDES
 	place_currency_code_before = lc->int_p_cs_precedes;
#else
	place_currency_code_before = place_currency_sign_before;
#endif
#ifdef HAVE_STRUCT_LCONV_INT_N_CS_PRECEDES
	place_currency_code_before_negative = lc->int_n_cs_precedes;
#else
	place_currency_code_before_negative = place_currency_sign_before_negative;
#endif	
	local_digit_group_separator = lc->thousands_sep;
	if((local_digit_group_separator.length() == 1 && local_digit_group_separator[0] < 0) || local_digit_group_separator == " ") local_digit_group_separator = " ";
	local_digit_group_format = lc->grouping;
	remove_blank_ends(local_digit_group_format);
	default_dot_as_separator = (local_digit_group_separator == ".");
	if(strcmp(lc->decimal_point, ",") == 0) {
		DOT_STR = ",";
		DOT_S = ".,";
		COMMA_STR = ";";
		COMMA_S = ";";
	} else {
		DOT_STR = ".";
		DOT_S = ".";
		COMMA_STR = ",";
		COMMA_S = ",;";
	}
	setlocale(LC_NUMERIC, "C");

	NAME_NUMBER_PRE_S = "_#";
	NAME_NUMBER_PRE_STR = "_";
	
	string str = _(" to ");
	local_to = (str != " to ");
	
	decimal_null_prefix = new DecimalPrefix(0, "", "");
	binary_null_prefix = new BinaryPrefix(0, "", "");
	m_undefined.setUndefined();
	m_empty_vector.clearVector();
	m_empty_matrix.clearMatrix();
	m_zero.clear();
	m_one.set(1, 1, 0);
	m_minus_one.set(-1, 1, 0);
	nr_zero.clear();
	nr_one.set(1, 1, 0);
	nr_two.set(2, 1, 0);
	nr_three.set(3, 1, 0);
	nr_half.set(1, 2, 0);
	nr_minus_half.set(-1, 2, 0);
	nr_one_i.setImaginaryPart(1, 1, 0);
	nr_minus_i.setImaginaryPart(-1, 1, 0);
	m_one_i.set(nr_one_i);
	nr_minus_one.set(-1, 1, 0);
	nr_plus_inf.setPlusInfinity();
	nr_minus_inf.setMinusInfinity();
	no_evaluation.approximation = APPROXIMATION_EXACT;
	no_evaluation.structuring = STRUCTURING_NONE;
	no_evaluation.sync_units = false;
	
	save_printoptions.decimalpoint_sign = ".";
	save_printoptions.comma_sign = ",";
	save_printoptions.use_reference_names = true;
	save_printoptions.preserve_precision = true;
	save_printoptions.interval_display = INTERVAL_DISPLAY_INTERVAL;
	save_printoptions.use_reference_names = true;
	save_printoptions.limit_implicit_multiplication = true;
	save_printoptions.spacious = false;
	save_printoptions.number_fraction_format = FRACTION_FRACTIONAL;
	save_printoptions.short_multiplication = false;
	save_printoptions.show_ending_zeroes = false;
	
	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.spell_out_logical_operators = true;
	message_printoptions.number_fraction_format = FRACTION_FRACTIONAL;
	
	default_user_evaluation_options.structuring = STRUCTURING_SIMPLIFY;
	
	default_assumptions = new Assumptions;
	default_assumptions->setType(ASSUMPTION_TYPE_REAL);
	default_assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);
	
	u_rad = NULL; u_gra = NULL; u_deg = NULL;
	
	b_save_called = false;

	ILLEGAL_IN_NAMES = "\a\b" + DOT_S + RESERVED OPERATORS SPACES PARENTHESISS VECTOR_WRAPS COMMAS;
	ILLEGAL_IN_NAMES_MINUS_SPACE_STR = "\a\b" + DOT_S + RESERVED OPERATORS PARENTHESISS VECTOR_WRAPS COMMAS;
	ILLEGAL_IN_UNITNAMES = ILLEGAL_IN_NAMES + NUMBERS;
	b_argument_errors = true;
	current_stage = MESSAGE_STAGE_UNSET;
	calculator = this;
	srand48(time(0));
	
	addBuiltinVariables();
	addBuiltinFunctions();
	addBuiltinUnits();

	disable_errors_ref = 0;
	b_busy = false;
	b_gnuplot_open = false;
	gnuplot_pipe = NULL;

	calculate_thread = new CalculateThread;
}
Calculator::Calculator(bool ignore_locale) {

	b_ignore_locale = ignore_locale;

#ifdef ENABLE_NLS
	if(!b_ignore_locale) {
		bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	}
#endif

	if(b_ignore_locale) {
		char *current_lc_monetary = setlocale(LC_MONETARY, "");
		if(current_lc_monetary) saved_locale = strdup(current_lc_monetary);
		else saved_locale = NULL;
		setlocale(LC_ALL, "C");
		if(saved_locale) {
			setlocale(LC_MONETARY, saved_locale);
			free(saved_locale);
			saved_locale = NULL;
		}
	} else {
		setlocale(LC_ALL, "");
	}


	gmp_randinit_default(randstate);
	gmp_randseed_ui(randstate, (unsigned long int) time(NULL));

	priv = new Calculator_p;
	priv->custom_input_base_i = 0;
	priv->ids_i = 0;
	priv->local_currency = NULL;

#ifdef HAVE_ICU
	UErrorCode err = U_ZERO_ERROR;
	ucm = ucasemap_open(NULL, 0, &err);
#endif

	srand(time(NULL));
	
	exchange_rates_time[0] = 0;
	exchange_rates_time[1] = 0;
	exchange_rates_time[2] = 0;
	exchange_rates_check_time[0] = 0;
	exchange_rates_check_time[1] = 0;
	exchange_rates_check_time[2] = 0;
	b_exchange_rates_warning_enabled = true;
	b_exchange_rates_used = 0;
	
	i_aborted = 0;
	b_controlled = false;
	i_timeout = 0;
	
	setPrecision(DEFAULT_PRECISION);
	b_interval = true;
	i_stop_interval = 0;
	i_start_interval = 0;
	
	b_var_units = true;

	addStringAlternative(SIGN_DIVISION, DIVISION);
	addStringAlternative(SIGN_DIVISION_SLASH, DIVISION);
	addStringAlternative(SIGN_MULTIPLICATION, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MIDDLEDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIBULLET, MULTIPLICATION);
	addStringAlternative(SIGN_SMALLCIRCLE, MULTIPLICATION);
	addStringAlternative(SIGN_MINUS, MINUS);
	addStringAlternative("–", MINUS);
	addStringAlternative(SIGN_PLUS, PLUS);
	addStringAlternative(SIGN_NOT_EQUAL, " " NOT EQUALS);
	addStringAlternative(SIGN_GREATER_OR_EQUAL, GREATER EQUALS);
	addStringAlternative(SIGN_LESS_OR_EQUAL, LESS EQUALS);
	addStringAlternative(";", COMMA);
	addStringAlternative("\t", SPACE);
	addStringAlternative("\n", SPACE);
	addStringAlternative(" ", SPACE);
	addStringAlternative("**", POWER);
	addStringAlternative("↊", "X");
	addStringAlternative("↋", "E");
	addStringAlternative("∧", BITWISE_AND);
	addStringAlternative("∨", BITWISE_OR);
	addStringAlternative("¬", BITWISE_NOT);
	addStringAlternative(SIGN_MICRO, "μ");
	
	per_str = _("per");
	per_str_len = per_str.length();
	times_str = _("times");
	times_str_len = times_str.length();
	plus_str = _("plus");
	plus_str_len = plus_str.length();
	minus_str = _("minus");
	minus_str_len = minus_str.length();
	and_str = _("and");
	if(and_str == "and") and_str = "";
	and_str_len = and_str.length();
	AND_str = "AND";
	AND_str_len = AND_str.length();
	or_str = _("or");
	if(or_str == "or") or_str = "";
	or_str_len = or_str.length();
	OR_str = "OR";
	OR_str_len = OR_str.length();
	XOR_str = "XOR";
	XOR_str_len = XOR_str.length();
	
	char *current_lc_numeric = setlocale(LC_NUMERIC, NULL);
	if(current_lc_numeric) saved_locale = strdup(current_lc_numeric);
	else saved_locale = NULL;
	struct lconv *lc = localeconv();
	if(!lc) {
		setlocale(LC_NUMERIC, "C");
		lc = localeconv();
	}
	place_currency_sign_before = lc->p_cs_precedes;
	place_currency_sign_before_negative = lc->n_cs_precedes;
#ifdef HAVE_STRUCT_LCONV_INT_P_CS_PRECEDES
 	place_currency_code_before = lc->int_p_cs_precedes;
#else
	place_currency_code_before = place_currency_sign_before;
#endif
#ifdef HAVE_STRUCT_LCONV_INT_N_CS_PRECEDES
	place_currency_code_before_negative = lc->int_n_cs_precedes;
#else
	place_currency_code_before_negative = place_currency_sign_before_negative;
#endif	
	local_digit_group_separator = lc->thousands_sep;
	if((local_digit_group_separator.length() == 1 && local_digit_group_separator[0] < 0) || local_digit_group_separator == " ") local_digit_group_separator = " ";
	local_digit_group_format = lc->grouping;
	remove_blank_ends(local_digit_group_format);
	default_dot_as_separator = (local_digit_group_separator == ".");
	if(strcmp(lc->decimal_point, ",") == 0) {
		DOT_STR = ",";
		DOT_S = ".,";
		COMMA_STR = ";";
		COMMA_S = ";";
	} else {
		DOT_STR = ".";
		DOT_S = ".";
		COMMA_STR = ",";
		COMMA_S = ",;";
	}
	setlocale(LC_NUMERIC, "C");

	NAME_NUMBER_PRE_S = "_#";
	NAME_NUMBER_PRE_STR = "_";
	
	string str = _(" to ");
	local_to = (str != " to ");

	decimal_null_prefix = new DecimalPrefix(0, "", "");
	binary_null_prefix = new BinaryPrefix(0, "", "");
	m_undefined.setUndefined();
	m_empty_vector.clearVector();
	m_empty_matrix.clearMatrix();
	m_zero.clear();
	m_one.set(1, 1, 0);
	m_minus_one.set(-1, 1, 0);
	nr_zero.clear();
	nr_one.set(1, 1, 0);
	nr_two.set(2, 1, 0);
	nr_three.set(3, 1, 0);
	nr_half.set(1, 2, 0);
	nr_minus_half.set(-1, 2, 0);
	nr_one_i.setImaginaryPart(1, 1, 0);
	nr_minus_i.setImaginaryPart(-1, 1, 0);
	m_one_i.set(nr_one_i);
	nr_minus_one.set(-1, 1, 0);
	nr_plus_inf.setPlusInfinity();
	nr_minus_inf.setMinusInfinity();
	no_evaluation.approximation = APPROXIMATION_EXACT;
	no_evaluation.structuring = STRUCTURING_NONE;
	no_evaluation.sync_units = false;
	
	save_printoptions.decimalpoint_sign = ".";
	save_printoptions.comma_sign = ",";
	save_printoptions.use_reference_names = true;
	save_printoptions.preserve_precision = true;
	save_printoptions.interval_display = INTERVAL_DISPLAY_INTERVAL;
	save_printoptions.use_reference_names = true;
	save_printoptions.limit_implicit_multiplication = true;
	save_printoptions.spacious = false;
	save_printoptions.number_fraction_format = FRACTION_FRACTIONAL;
	save_printoptions.short_multiplication = false;
	save_printoptions.show_ending_zeroes = false;
	
	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.spell_out_logical_operators = true;
	message_printoptions.number_fraction_format = FRACTION_FRACTIONAL;
	
	default_user_evaluation_options.structuring = STRUCTURING_SIMPLIFY;
	
	default_assumptions = new Assumptions;
	default_assumptions->setType(ASSUMPTION_TYPE_REAL);
	default_assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);
	
	u_rad = NULL; u_gra = NULL; u_deg = NULL;
	
	b_save_called = false;

	ILLEGAL_IN_NAMES = "\a\b" + DOT_S + RESERVED OPERATORS SPACES PARENTHESISS VECTOR_WRAPS COMMAS;
	ILLEGAL_IN_NAMES_MINUS_SPACE_STR = "\a\b" + DOT_S + RESERVED OPERATORS PARENTHESISS VECTOR_WRAPS COMMAS;
	ILLEGAL_IN_UNITNAMES = ILLEGAL_IN_NAMES + NUMBERS;
	b_argument_errors = true;
	current_stage = MESSAGE_STAGE_UNSET;
	calculator = this;
	srand48(time(0));
	
	addBuiltinVariables();
	addBuiltinFunctions();
	addBuiltinUnits();

	disable_errors_ref = 0;
	b_busy = false;
	b_gnuplot_open = false;
	gnuplot_pipe = NULL;

	calculate_thread = new CalculateThread;
}
Calculator::~Calculator() {
	closeGnuplot();
	abort();
	terminateThreads();
	delete priv;
	delete calculate_thread;
	gmp_randclear(randstate);
#ifdef HAVE_ICU
	if(ucm) ucasemap_close(ucm);
#endif
}

Unit *Calculator::getGraUnit() {
	if(!u_gra) u_gra = getUnit("gra");
	if(!u_gra) {
		error(true, _("Gradians unit is missing. Creating one for this session."), NULL);
		u_gra = addUnit(new AliasUnit(_("Angle/Plane Angle"), "gra", "gradians", "gradian", "Gradian", getRadUnit(), "pi/200", 1, "", false, true, true));	
	}
	return u_gra;
}
Unit *Calculator::getRadUnit() {
	if(!u_rad) u_rad = getUnit("rad");
	if(!u_rad) {
		error(true, _("Radians unit is missing. Creating one for this session."), NULL);
		u_rad = addUnit(new Unit(_("Angle/Plane Angle"), "rad", "radians", "radian", "Radian", false, true, true));
	}
	return u_rad;
}
Unit *Calculator::getDegUnit() {
	if(!u_deg) u_deg = getUnit("deg");
	if(!u_deg) {
		error(true, _("Degrees unit is missing. Creating one for this session."), NULL);
		u_deg = addUnit(new AliasUnit(_("Angle/Plane Angle"), "deg", "degrees", "degree", "Degree", getRadUnit(), "pi/180", 1, "", false, true, true));
	}
	return u_deg;
}

bool Calculator::utf8_pos_is_valid_in_name(char *pos) {
	if(is_in(ILLEGAL_IN_NAMES, pos[0])) {
		return false;
	}
	if((unsigned char) pos[0] >= 0xC0) {
		string str;
		str += pos[0];
		while((unsigned char) pos[1] >= 0x80 && (unsigned char) pos[1] < 0xC0) {
			str += pos[1];
			pos++;
		}
		return str != SIGN_DIVISION && str != SIGN_DIVISION_SLASH && str != SIGN_MULTIPLICATION && str != SIGN_MULTIDOT && str != SIGN_SMALLCIRCLE && str != SIGN_MULTIBULLET && str != SIGN_MINUS && str != SIGN_PLUS && str != SIGN_NOT_EQUAL && str != SIGN_GREATER_OR_EQUAL && str != SIGN_LESS_OR_EQUAL;
	}
	return true;
}

bool Calculator::showArgumentErrors() const {
	return b_argument_errors;
}
void Calculator::beginTemporaryStopMessages() {
	disable_errors_ref++;
	stopped_errors_count.push_back(0);
	stopped_warnings_count.push_back(0);
	stopped_messages_count.push_back(0);
	vector<CalculatorMessage> vcm;
	stopped_messages.push_back(vcm);
}
int Calculator::endTemporaryStopMessages(int *message_count, int *warning_count, int release_messages_if_no_equal_or_greater_than_message_type) {
	if(disable_errors_ref <= 0) return -1;
	disable_errors_ref--;
	int ret = stopped_errors_count[disable_errors_ref];
	bool release_messages = false;
	if(release_messages_if_no_equal_or_greater_than_message_type >= MESSAGE_INFORMATION) {
		if(ret > 0) release_messages = true;
		if(release_messages_if_no_equal_or_greater_than_message_type == MESSAGE_WARNING && stopped_warnings_count[disable_errors_ref] > 0) release_messages = true;
		else if(release_messages_if_no_equal_or_greater_than_message_type == MESSAGE_INFORMATION && stopped_messages_count[disable_errors_ref] > 0) release_messages = true;
	}
	if(message_count) *message_count = stopped_messages_count[disable_errors_ref];
	if(warning_count) *warning_count = stopped_warnings_count[disable_errors_ref];
	stopped_errors_count.pop_back();
	stopped_warnings_count.pop_back();
	stopped_messages_count.pop_back();
	if(release_messages) addMessages(&stopped_messages[disable_errors_ref]);
	stopped_messages.pop_back();
	return ret;
}
void Calculator::endTemporaryStopMessages(bool release_messages, vector<CalculatorMessage> *blocked_messages) {
	if(disable_errors_ref <= 0) return;
	disable_errors_ref--;
	stopped_errors_count.pop_back();
	stopped_warnings_count.pop_back();
	stopped_messages_count.pop_back();
	if(blocked_messages) *blocked_messages = stopped_messages[disable_errors_ref];
	if(release_messages) addMessages(&stopped_messages[disable_errors_ref]);
	stopped_messages.pop_back();
}
void Calculator::addMessages(vector<CalculatorMessage> *message_vector) {
	for(size_t i3 = 0; i3 < message_vector->size(); i3++) {
		string error_str = (*message_vector)[i3].message();
		bool dup_error = false;
		for(size_t i = 0; i < messages.size(); i++) {
			if(error_str == messages[i].message()) {
				dup_error = true;
				break;
			}
		}
		if(!dup_error) {
			if(disable_errors_ref > 0) {
				for(size_t i2 = 0; !dup_error && i2 < (size_t) disable_errors_ref; i2++) {
					for(size_t i = 0; i < stopped_messages[i2].size(); i++) {
						if(error_str == stopped_messages[i2][i].message()) {
							dup_error = true;
							break;
						}
					}
				}
				if(!dup_error) stopped_messages[disable_errors_ref - 1].push_back((*message_vector)[i3]);
			} else {
				messages.push_back((*message_vector)[i3]);
			}
		}
	}
}
const PrintOptions &Calculator::messagePrintOptions() const {return message_printoptions;}
void Calculator::setMessagePrintOptions(const PrintOptions &po) {message_printoptions = po;}

Variable *Calculator::getVariable(size_t index) const {
	if(index < variables.size()) {
		return variables[index];
	}
	return NULL;
}
bool Calculator::hasVariable(Variable *v) {
	for(size_t i = 0; i < variables.size(); i++) {
		if(variables[i] == v) return true;
	}
	return false;
}
bool Calculator::hasUnit(Unit *u) {
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i] == u) return true;
	}
	return false;
}
bool Calculator::hasFunction(MathFunction *f) {
	for(size_t i = 0; i < functions.size(); i++) {
		if(functions[i] == f) return true;
	}
	return false;
}
bool Calculator::stillHasVariable(Variable *v) {
	for(vector<Variable*>::iterator it = deleted_variables.begin(); it != deleted_variables.end(); ++it) {
		if(*it == v) return false;
	}
	return true;
}
bool Calculator::stillHasUnit(Unit *u) {
	for(vector<Unit*>::iterator it = deleted_units.begin(); it != deleted_units.end(); ++it) {
		if(*it == u) return false;
	}
	return true;
}
bool Calculator::stillHasFunction(MathFunction *f) {
	for(vector<MathFunction*>::iterator it = deleted_functions.begin(); it != deleted_functions.end(); ++it) {
		if(*it == f) return false;
	}
	return true;
}
void Calculator::saveFunctionCalled() {
	b_save_called = true;
}
bool Calculator::checkSaveFunctionCalled() {
	if(b_save_called) {
		b_save_called = false;
		return true;
	}
	return false;
}
ExpressionItem *Calculator::getActiveExpressionItem(ExpressionItem *item) {
	if(!item) return NULL;
	for(size_t i = 1; i <= item->countNames(); i++) {
		ExpressionItem *item2 = getActiveExpressionItem(item->getName(i).name, item);
		if(item2) {
			return item2;
		}
	}
	return NULL;
}
ExpressionItem *Calculator::getActiveExpressionItem(string name, ExpressionItem *item) {
	if(name.empty()) return NULL;
	for(size_t index = 0; index < variables.size(); index++) {
		if(variables[index] != item && variables[index]->isActive() && variables[index]->hasName(name)) {
			return variables[index];
		}
	}
	for(size_t index = 0; index < functions.size(); index++) {
		if(functions[index] != item && functions[index]->isActive() && functions[index]->hasName(name)) {
			return functions[index];
		}
	}
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i] != item && units[i]->isActive() && units[i]->hasName(name)) {
			return units[i];
		}
	}
	return NULL;
}
ExpressionItem *Calculator::getInactiveExpressionItem(string name, ExpressionItem *item) {
	if(name.empty()) return NULL;
	for(size_t index = 0; index < variables.size(); index++) {
		if(variables[index] != item && !variables[index]->isActive() && variables[index]->hasName(name)) {
			return variables[index];
		}
	}
	for(size_t index = 0; index < functions.size(); index++) {
		if(functions[index] != item && !functions[index]->isActive() && functions[index]->hasName(name)) {
			return functions[index];
		}
	}
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i] != item && !units[i]->isActive() && units[i]->hasName(name)) {
			return units[i];
		}
	}
	return NULL;
}
ExpressionItem *Calculator::getExpressionItem(string name, ExpressionItem *item) {
	if(name.empty()) return NULL;
	Variable *v = getVariable(name);
	if(v && v != item) return v;
	MathFunction *f = getFunction(name);
	if(f && f != item) return f;
	Unit *u = getUnit(name);
	if(u && u != item) return u;
	u = getCompositeUnit(name);
	if(u && u != item) return u;
	return NULL;
}
Unit *Calculator::getUnit(size_t index) const {
	if(index < units.size()) {
		return units[index];
	}
	return NULL;
}
MathFunction *Calculator::getFunction(size_t index) const {
	if(index < functions.size()) {
		return functions[index];
	}
	return NULL;
}

void Calculator::setDefaultAssumptions(Assumptions *ass) {
	if(default_assumptions) delete default_assumptions;
	default_assumptions = ass;
}
Assumptions *Calculator::defaultAssumptions() {
	return default_assumptions;
}

Prefix *Calculator::getPrefix(size_t index) const {
	if(index < prefixes.size()) {
		return prefixes[index];
	}
	return NULL;
}
Prefix *Calculator::getPrefix(string name_) const {
	for(size_t i = 0; i < prefixes.size(); i++) {
		if(prefixes[i]->shortName(false) == name_ || prefixes[i]->longName(false) == name_ || prefixes[i]->unicodeName(false) == name_) {
			return prefixes[i];
		}
	}
	return NULL;
}
DecimalPrefix *Calculator::getExactDecimalPrefix(int exp10, int exp) const {
	for(size_t i = 0; i < decimal_prefixes.size(); i++) {
		if(decimal_prefixes[i]->exponent(exp) == exp10) {
			return decimal_prefixes[i];
		} else if(decimal_prefixes[i]->exponent(exp) > exp10) {
			break;
		}
	}
	return NULL;
}
BinaryPrefix *Calculator::getExactBinaryPrefix(int exp2, int exp) const {
	for(size_t i = 0; i < binary_prefixes.size(); i++) {
		if(binary_prefixes[i]->exponent(exp) == exp2) {
			return binary_prefixes[i];
		} else if(binary_prefixes[i]->exponent(exp) > exp2) {
			break;
		}
	}
	return NULL;
}
Prefix *Calculator::getExactPrefix(const Number &o, int exp) const {
	ComparisonResult c;
	for(size_t i = 0; i < prefixes.size(); i++) {
		c = o.compare(prefixes[i]->value(exp));
		if(c == COMPARISON_RESULT_EQUAL) {
			return prefixes[i];
		} else if(c == COMPARISON_RESULT_GREATER) {
			break;
		}
	}
	return NULL;
}
DecimalPrefix *Calculator::getNearestDecimalPrefix(int exp10, int exp) const {
	if(decimal_prefixes.size() <= 0) return NULL;
	int i = 0;
	if(exp < 0) {
		i = decimal_prefixes.size() - 1;
	}
	while((exp < 0 && i >= 0) || (exp >= 0 && i < (int) decimal_prefixes.size())) {	
		if(decimal_prefixes[i]->exponent(exp) == exp10) {
			return decimal_prefixes[i];
		} else if(decimal_prefixes[i]->exponent(exp) > exp10) {
			if(i == 0) {
				return decimal_prefixes[i];
			} else if(exp10 - decimal_prefixes[i - 1]->exponent(exp) < decimal_prefixes[i]->exponent(exp) - exp10) {
				return decimal_prefixes[i - 1];
			} else {
				return decimal_prefixes[i];
			}
		}
		if(exp < 0) {
			i--;
		} else {
			i++;
		}
	}
	return decimal_prefixes[decimal_prefixes.size() - 1];
}
DecimalPrefix *Calculator::getOptimalDecimalPrefix(int exp10, int exp, bool all_prefixes) const {
	if(decimal_prefixes.size() <= 0 || exp10 == 0) return NULL;
	int i = 0;
	if(exp < 0) {
		i = decimal_prefixes.size() - 1;
	}
	DecimalPrefix *p = NULL, *p_prev = NULL;
	int exp10_1, exp10_2;
	while((exp < 0 && i >= 0) || (exp >= 0 && i < (int) decimal_prefixes.size())) {
		if(all_prefixes || decimal_prefixes[i]->exponent() % 3 == 0) {
			p = decimal_prefixes[i];
			if(p_prev && (p_prev->exponent() >= 0) != (p->exponent() >= 0) && p_prev->exponent() != 0) {
				if(exp < 0) {
					i++;
				} else {
					i--;
				}
				p = decimal_null_prefix;
			}
			if(p->exponent(exp) == exp10) {
				if(p == decimal_null_prefix) return NULL;
				return p;
			} else if(p->exponent(exp) > exp10) {
				if(i == 0) {
					if(p == decimal_null_prefix) return NULL;
					return p;
				}
				exp10_1 = exp10;
				if(p_prev) {
					exp10_1 -= p_prev->exponent(exp);
				}
				exp10_2 = p->exponent(exp);
				exp10_2 -= exp10;
				exp10_2 *= 2;
				exp10_2 += 2;
				if(exp10_1 < exp10_2) {
					if(p_prev == decimal_null_prefix) return NULL;
					return p_prev;
				} else {
					return p;
				}
			}
			p_prev = p;
		}
		if(exp < 0) {
			i--;
		} else {
			i++;
		}
	}
	return p_prev;
}
DecimalPrefix *Calculator::getOptimalDecimalPrefix(const Number &exp10, const Number &exp, bool all_prefixes) const {
	if(decimal_prefixes.size() <= 0 || exp10.isZero()) return NULL;
	int i = 0;
	ComparisonResult c;
	if(exp.isNegative()) {
		i = decimal_prefixes.size() - 1;
	}
	DecimalPrefix *p = NULL, *p_prev = NULL;
	Number exp10_1, exp10_2;
	while((exp.isNegative() && i >= 0) || (!exp.isNegative() && i < (int) decimal_prefixes.size())) {
		if(all_prefixes || decimal_prefixes[i]->exponent() % 3 == 0) {
			p = decimal_prefixes[i];
			if(p_prev && (p_prev->exponent() >= 0) != (p->exponent() >= 0) && p_prev->exponent() != 0) {
				if(exp.isNegative()) {
					i++;
				} else {
					i--;
				}
				p = decimal_null_prefix;
			}
			c = exp10.compare(p->exponent(exp));
			if(c == COMPARISON_RESULT_EQUAL) {
				if(p == decimal_null_prefix) return NULL;
				return p;
			} else if(c == COMPARISON_RESULT_GREATER) {
				if(i == 0) {
					if(p == decimal_null_prefix) return NULL;
					return p;
				}
				exp10_1 = exp10;
				if(p_prev) {
					exp10_1 -= p_prev->exponent(exp);
				}
				exp10_2 = p->exponent(exp);
				exp10_2 -= exp10;
				exp10_2 *= 2;
				exp10_2 += 2;
				if(exp10_1.isLessThan(exp10_2)) {
					if(p_prev == decimal_null_prefix) return NULL;
					return p_prev;
				} else {
					return p;
				}
			}
			p_prev = p;
		}
		if(exp.isNegative()) {
			i--;
		} else {
			i++;
		}
	}
	return p_prev;
}
BinaryPrefix *Calculator::getNearestBinaryPrefix(int exp2, int exp) const {
	if(binary_prefixes.size() <= 0) return NULL;
	int i = 0;
	if(exp < 0) {
		i = binary_prefixes.size() - 1;
	}
	while((exp < 0 && i >= 0) || (exp >= 0 && i < (int) binary_prefixes.size())) {	
		if(binary_prefixes[i]->exponent(exp) == exp2) {
			return binary_prefixes[i];
		} else if(binary_prefixes[i]->exponent(exp) > exp2) {
			if(i == 0) {
				return binary_prefixes[i];
			} else if(exp2 - binary_prefixes[i - 1]->exponent(exp) < binary_prefixes[i]->exponent(exp) - exp2) {
				return binary_prefixes[i - 1];
			} else {
				return binary_prefixes[i];
			}
		}
		if(exp < 0) {
			i--;
		} else {
			i++;
		}
	}
	return binary_prefixes[binary_prefixes.size() - 1];
}
BinaryPrefix *Calculator::getOptimalBinaryPrefix(int exp2, int exp) const {
	if(binary_prefixes.size() <= 0 || exp2 == 0) return NULL;
	int i = 0;
	if(exp < 0) {
		i = binary_prefixes.size() - 1;
	}
	BinaryPrefix *p = NULL, *p_prev = NULL;
	int exp2_1, exp2_2;
	while((exp < 0 && i >= 0) || (exp >= 0 && i < (int) binary_prefixes.size())) {
		p = binary_prefixes[i];
		if(p_prev && (p_prev->exponent() >= 0) != (p->exponent() >= 0) && p_prev->exponent() != 0) {
			if(exp < 0) {
				i++;
			} else {
				i--;
			}
			p = binary_null_prefix;
		}
		if(p->exponent(exp) == exp2) {
			if(p == binary_null_prefix) return NULL;
			return p;
		} else if(p->exponent(exp) > exp2) {
			if(i == 0) {
				if(p == binary_null_prefix) return NULL;
				return p;
			}
			exp2_1 = exp2;
			if(p_prev) {
				exp2_1 -= p_prev->exponent(exp);
			}
			exp2_2 = p->exponent(exp);
			exp2_2 -= exp2;
			exp2_2 *= 2;
			exp2_2 += 2;
			if(exp2_1 < exp2_2) {
				if(p_prev == binary_null_prefix) return NULL;
				return p_prev;
			} else {
				return p;
			}
		}
		p_prev = p;
		if(exp < 0) {
			i--;
		} else {
			i++;
		}
	}
	return p_prev;
}
BinaryPrefix *Calculator::getOptimalBinaryPrefix(const Number &exp2, const Number &exp) const {
	if(binary_prefixes.size() <= 0 || exp2.isZero()) return NULL;
	int i = 0;
	ComparisonResult c;
	if(exp.isNegative()) {
		i = binary_prefixes.size() - 1;
	}
	BinaryPrefix *p = NULL, *p_prev = NULL;
	Number exp2_1, exp2_2;
	while((exp.isNegative() && i >= 0) || (!exp.isNegative() && i < (int) binary_prefixes.size())) {
		p = binary_prefixes[i];
		if(p_prev && (p_prev->exponent() >= 0) != (p->exponent() >= 0) && p_prev->exponent() != 0) {
			if(exp.isNegative()) {
				i++;
			} else {
				i--;
			}
			p = binary_null_prefix;
		}
		c = exp2.compare(p->exponent(exp));
		if(c == COMPARISON_RESULT_EQUAL) {
			if(p == binary_null_prefix) return NULL;
			return p;
		} else if(c == COMPARISON_RESULT_GREATER) {
			if(i == 0) {
				if(p == binary_null_prefix) return NULL;
				return p;
			}
			exp2_1 = exp2;
			if(p_prev) {
				exp2_1 -= p_prev->exponent(exp);
			}
			exp2_2 = p->exponent(exp);
			exp2_2 -= exp2;
			exp2_2 *= 2;
			exp2_2 += 2;
			if(exp2_1.isLessThan(exp2_2)) {
				if(p_prev == binary_null_prefix) return NULL;
				return p_prev;
			} else {
				return p;
			}
		}
		p_prev = p;
		if(exp.isNegative()) {
			i--;
		} else {
			i++;
		}
	}
	return p_prev;
}
Prefix *Calculator::addPrefix(Prefix *p) {
	if(p->type() == PREFIX_DECIMAL) {
		decimal_prefixes.push_back((DecimalPrefix*) p);
	} else if(p->type() == PREFIX_BINARY) {
		binary_prefixes.push_back((BinaryPrefix*) p);
	}
	prefixes.push_back(p);
	prefixNameChanged(p, true);
	return p;	
}
void Calculator::prefixNameChanged(Prefix *p, bool new_item) {
	size_t l2;
	if(!new_item) delPrefixUFV(p);
	if(!p->longName(false).empty()) {
		l2 = p->longName(false).length();
		if(l2 > UFV_LENGTHS) {
			size_t i = 0, l;
			for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
				l = 0;
				if(it != ufvl.end()) {
					if(ufvl_t[i] == 'v')
						l = ((Variable*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'f')
						l = ((MathFunction*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'u')
						l = ((Unit*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'p')
						l = ((Prefix*) (*it))->shortName(false).length();
					else if(ufvl_t[i] == 'P')
						l = ((Prefix*) (*it))->longName(false).length();
					else if(ufvl_t[i] == 'q')
						l = ((Prefix*) (*it))->unicodeName(false).length();
				}
				if(it == ufvl.end()) {
					ufvl.push_back((void*) p);
					ufvl_t.push_back('P');
					ufvl_i.push_back(1);
					break;
				} else if(l <= l2) {			
					ufvl.insert(it, (void*) p);
					ufvl_t.insert(ufvl_t.begin() + i, 'P');
					ufvl_i.insert(ufvl_i.begin() + i, 1);
					break;
				}
				i++;
			}
		} else if(l2 > 0) {
			l2--;
			ufv[0][l2].push_back((void*) p);
			ufv_i[0][l2].push_back(1);
		}
	}
	if(!p->shortName(false).empty()) {
		l2 = p->shortName(false).length();
		if(l2 > UFV_LENGTHS) {
			size_t i = 0, l;
			for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
				l = 0;
				if(it != ufvl.end()) {
					if(ufvl_t[i] == 'v')
						l = ((Variable*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'f')
						l = ((MathFunction*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'u')
						l = ((Unit*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'p')
						l = ((Prefix*) (*it))->shortName(false).length();
					else if(ufvl_t[i] == 'P')
						l = ((Prefix*) (*it))->longName(false).length();
					else if(ufvl_t[i] == 'q')
						l = ((Prefix*) (*it))->unicodeName(false).length();
				}
				if(it == ufvl.end()) {
					ufvl.push_back((void*) p);
					ufvl_t.push_back('p');
					ufvl_i.push_back(1);
					break;
				} else if(l <= l2) {			
					ufvl.insert(it, (void*) p);
					ufvl_t.insert(ufvl_t.begin() + i, 'p');
					ufvl_i.insert(ufvl_i.begin() + i, 1);
					break;
				}
				i++;
			}
		} else if(l2 > 0) {
			l2--;
			ufv[0][l2].push_back((void*) p);
			ufv_i[0][l2].push_back(2);
		}
	}
	if(!p->unicodeName(false).empty()) {
		l2 = p->unicodeName(false).length();
		if(l2 > UFV_LENGTHS) {
			size_t i = 0, l;
			for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
				l = 0;
				if(it != ufvl.end()) {
					if(ufvl_t[i] == 'v')
						l = ((Variable*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'f')
						l = ((MathFunction*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'u')
						l = ((Unit*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'p')
						l = ((Prefix*) (*it))->shortName(false).length();
					else if(ufvl_t[i] == 'P')
						l = ((Prefix*) (*it))->longName(false).length();
					else if(ufvl_t[i] == 'q')
						l = ((Prefix*) (*it))->unicodeName(false).length();
				}
				if(it == ufvl.end()) {
					ufvl.push_back((void*) p);
					ufvl_t.push_back('q');
					ufvl_i.push_back(1);
					break;
				} else if(l <= l2) {			
					ufvl.insert(it, (void*) p);
					ufvl_t.insert(ufvl_t.begin() + i, 'q');
					ufvl_i.insert(ufvl_i.begin() + i, 1);
					break;
				}
				i++;
			}
		} else if(l2 > 0) {
			l2--;
			ufv[0][l2].push_back((void*) p);
			ufv_i[0][l2].push_back(3);
		}
	}
}
#define PRECISION_TO_BITS(p) (((p) * 3.322) + 100)
void Calculator::setPrecision(int precision) {
	if(precision <= 0) precision = DEFAULT_PRECISION;
	i_precision = precision;
	mpfr_set_default_prec(PRECISION_TO_BITS(i_precision));
}
int Calculator::getPrecision() const {
	return i_precision;
}
void Calculator::useIntervalArithmetic(bool use_interval_arithmetic) {b_interval = use_interval_arithmetic;}
bool Calculator::usesIntervalArithmetic() const {return i_start_interval > 0 || (b_interval && i_stop_interval <= 0);}
void Calculator::beginTemporaryStopIntervalArithmetic() {
	i_stop_interval++;
}
void Calculator::endTemporaryStopIntervalArithmetic() {
	i_stop_interval--;
}
void Calculator::beginTemporaryEnableIntervalArithmetic() {
	i_start_interval++;
}
void Calculator::endTemporaryEnableIntervalArithmetic() {
	i_start_interval--;
}

void Calculator::setCustomInputBase(Number nr) {
	priv->custom_input_base = nr;
	if(!nr.isReal()) {
		priv->custom_input_base_i = LONG_MAX;
	} else {
		nr.abs(); nr.ceil();
		priv->custom_input_base_i = nr.lintValue();
		if(priv->custom_input_base_i < 2) priv->custom_input_base_i = 2;
	}
}
void Calculator::setCustomOutputBase(Number nr) {priv->custom_output_base = nr;}
const Number &Calculator::customInputBase() const {return priv->custom_input_base;}
const Number &Calculator::customOutputBase() const {return priv->custom_output_base;}

const string &Calculator::getDecimalPoint() const {return DOT_STR;}
const string &Calculator::getComma() const {return COMMA_STR;}
string Calculator::localToString(bool include_spaces) const {
	if(include_spaces) return _(" to ");
	else return _("to");
}
void Calculator::setLocale() {
	if(b_ignore_locale) return;
	if(saved_locale) setlocale(LC_NUMERIC, saved_locale);
	lconv *locale = localeconv();
	if(strcmp(locale->decimal_point, ",") == 0) {
		DOT_STR = ",";
		DOT_S = ".,";
		COMMA_STR = ";";
		COMMA_S = ";";
	} else {
		DOT_STR = ".";
		DOT_S = ".";
		COMMA_STR = ",";
		COMMA_S = ",;";
	}
	setlocale(LC_NUMERIC, "C");
}
void Calculator::setIgnoreLocale() {
	if(saved_locale) {
		free(saved_locale);
		saved_locale = NULL;
	}
	char *current_lc_monetary = setlocale(LC_MONETARY, NULL);
	if(current_lc_monetary) saved_locale = strdup(current_lc_monetary);
	else saved_locale = NULL;
	setlocale(LC_ALL, "C");
#ifdef ENABLE_NLS
#	ifdef _WIN32
	bindtextdomain(GETTEXT_PACKAGE, "NULL");
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#	endif
#endif
	if(saved_locale) {
		setlocale(LC_MONETARY, saved_locale);
		free(saved_locale);
		saved_locale = NULL;
	}
	b_ignore_locale = true;
	per_str = "per";
	per_str_len = per_str.length();
	times_str = "times";
	times_str_len = times_str.length();
	plus_str = "plus";
	plus_str_len = plus_str.length();
	minus_str = "minus";
	minus_str_len = minus_str.length();
	and_str = "";
	and_str_len = 0;
	or_str = "";
	or_str_len = 0;
	local_to = false;
	unsetLocale();
}
bool Calculator::getIgnoreLocale() {
	return b_ignore_locale;
}
void Calculator::useDecimalComma() {
	DOT_STR = ",";
	DOT_S = ".,";
	COMMA_STR = ";";
	COMMA_S = ";";
}
void Calculator::useDecimalPoint(bool use_comma_as_separator) {
	DOT_STR = ".";
	DOT_S = ".";
	if(use_comma_as_separator) {
		COMMA_STR = ";";
		COMMA_S = ";";
	} else {
		COMMA_STR = ",";
		COMMA_S = ",;";
	}
}
void Calculator::unsetLocale() {
	COMMA_STR = ",";
	COMMA_S = ",;";
	DOT_STR = ".";
	DOT_S = ".";
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

void Calculator::resetVariables() {
	variables.clear();
	addBuiltinVariables();
}
void Calculator::resetFunctions() {
	functions.clear();
	addBuiltinFunctions();
}
void Calculator::resetUnits() {
	units.clear();
	addBuiltinUnits();
}
void Calculator::reset() {
	resetVariables();
	resetFunctions();
	resetUnits();
}

#ifdef __linux__
#	include <sys/sysinfo.h>
#endif

#if defined __linux__ || defined _WIN32
class UptimeVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const {
		Number nr;
#	ifdef __linux__
		std::ifstream proc_uptime("/proc/uptime", std::ios::in);
		if(proc_uptime.is_open()) {
			char s_uptime[100];
			proc_uptime.getline(s_uptime, 100);
			nr.set(s_uptime);
		} else {
			struct sysinfo sf;
			if(!sysinfo(&sf)) nr = (long int) sf.uptime;
		}
#	elif _WIN32
		ULONGLONG i_uptime = GetTickCount64();
		nr.set((long int) (i_uptime % 1000), 1000);
		nr += (long int) (i_uptime / 1000);
#	endif
		m = nr;
		Unit *u = CALCULATOR->getUnit("s");
		if(u) m *= u;
	}
  public:
	UptimeVariable() : DynamicVariable("", "uptime") {
		setApproximate(false);
		always_recalculate = true;
	}
	UptimeVariable(const UptimeVariable *variable) {set(variable);}
	ExpressionItem *copy() const {return new UptimeVariable(this);}
};
#endif

void Calculator::addBuiltinVariables() {

	v_e = (KnownVariable*) addVariable(new EVariable());
	v_pi = (KnownVariable*) addVariable(new PiVariable());	
	Number nr(1, 1);
	MathStructure mstruct;
	mstruct.number().setImaginaryPart(nr);
	v_i = (KnownVariable*) addVariable(new KnownVariable("", "i", mstruct, "Imaginary i (sqrt(-1))", false, true));
	mstruct.number().setPlusInfinity();
	v_pinf = (KnownVariable*) addVariable(new KnownVariable("", "plus_infinity", mstruct, "+Infinity", false, true));
	mstruct.number().setMinusInfinity();
	v_minf = (KnownVariable*) addVariable(new KnownVariable("", "minus_infinity", mstruct, "-Infinity", false, true));
	mstruct.setUndefined();
	v_undef = (KnownVariable*) addVariable(new KnownVariable("", "undefined", mstruct, "Undefined", false, true));
	v_euler = (KnownVariable*) addVariable(new EulerVariable());
	v_catalan = (KnownVariable*) addVariable(new CatalanVariable());
	v_precision = (KnownVariable*) addVariable(new PrecisionVariable());
	v_percent = (KnownVariable*) addVariable(new KnownVariable("", "percent", MathStructure(1, 1, -2), "Percent", false, true));
	v_percent->addName("%");
	v_permille = (KnownVariable*) addVariable(new KnownVariable("", "permille", MathStructure(1, 1, -3), "Per Mille", false, true));
	v_permyriad = (KnownVariable*) addVariable(new KnownVariable("", "permyriad", MathStructure(1, 1, -4), "Per Myriad", false, true));
	v_x = (UnknownVariable*) addVariable(new UnknownVariable("", "x", "", true, false));
	v_y = (UnknownVariable*) addVariable(new UnknownVariable("", "y", "", true, false));
	v_z = (UnknownVariable*) addVariable(new UnknownVariable("", "z", "", true, false));
	v_C = new UnknownVariable("", "C", "", false, true);
	v_C->setAssumptions(new Assumptions());
	v_n = (UnknownVariable*) addVariable(new UnknownVariable("", "n", "", false, true));
	v_n->setAssumptions(new Assumptions());
	v_n->assumptions()->setType(ASSUMPTION_TYPE_INTEGER);
	v_today = (KnownVariable*) addVariable(new TodayVariable());
	v_yesterday = (KnownVariable*) addVariable(new YesterdayVariable());
	v_tomorrow = (KnownVariable*) addVariable(new TomorrowVariable());
	v_now = (KnownVariable*) addVariable(new NowVariable());
#if defined __linux__ || defined _WIN32
	addVariable(new UptimeVariable());
#endif
	
}

DECLARE_BUILTIN_FUNCTION(CircularShiftFunction)

CircularShiftFunction::CircularShiftFunction() : MathFunction("bitrot", 2, 4) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument());
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(3, "0");
	setDefaultValue(4, "1");
}
int CircularShiftFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isZero()) {
		mstruct.clear();
		return 1;
	}
	Number nr(vargs[0].number());
	unsigned int bits = vargs[2].number().uintValue();
	if(bits == 0) {
		bits = nr.integerLength();
		if(bits <= 8) bits = 8;
		else if(bits <= 16) bits = 16;
		else if(bits <= 32) bits = 32;
		else if(bits <= 64) bits = 64;
		else if(bits <= 128) bits = 128;
		else {
			bits = (unsigned int) ::ceil(::log2(bits));
			bits = ::pow(2, bits);
		}
	}
	Number nr_n(vargs[1].number());
	nr_n.rem(bits);
	if(nr_n.isZero()) {
		mstruct = nr;
		return 1;
	}
	PrintOptions po;
	po.base = BASE_BINARY;
	po.base_display = BASE_DISPLAY_NORMAL;
	po.binary_bits = bits;
	string str = nr.print(po);
	remove_blanks(str);
	if(str.length() < bits) return 0;
	if(nr_n.isNegative()) {
		nr_n.negate();
		std::rotate(str.rbegin(), str.rbegin() + nr_n.uintValue(), str.rend());
	} else {
		std::rotate(str.begin(), str.begin() + nr_n.uintValue(), str.end());
	}
	ParseOptions pao;
	pao.base = BASE_BINARY;
	pao.twos_complement = vargs[3].number().getBoolean();
	mstruct = Number(str, pao);
	return 1;
}


void Calculator::addBuiltinFunctions() {

	f_vector = addFunction(new VectorFunction());
	f_sort = addFunction(new SortFunction());
	f_rank = addFunction(new RankFunction());
	f_limits = addFunction(new LimitsFunction());
	//f_component = addFunction(new ComponentFunction());
	f_dimension = addFunction(new DimensionFunction());
	f_merge_vectors = addFunction(new MergeVectorsFunction());
	f_matrix = addFunction(new MatrixFunction());
	f_matrix_to_vector = addFunction(new MatrixToVectorFunction());
	f_area = addFunction(new AreaFunction());
	f_rows = addFunction(new RowsFunction());
	f_columns = addFunction(new ColumnsFunction());
	f_row = addFunction(new RowFunction());
	f_column = addFunction(new ColumnFunction());
	f_elements = addFunction(new ElementsFunction());
	f_element = addFunction(new ElementFunction());
	f_transpose = addFunction(new TransposeFunction());
	f_identity = addFunction(new IdentityFunction());
	f_determinant = addFunction(new DeterminantFunction());
	f_permanent = addFunction(new PermanentFunction());
	f_adjoint = addFunction(new AdjointFunction());
	f_cofactor = addFunction(new CofactorFunction());
	f_inverse = addFunction(new InverseFunction());
	f_magnitude = addFunction(new MagnitudeFunction());
	f_hadamard = addFunction(new HadamardFunction());
	f_entrywise = addFunction(new EntrywiseFunction());

	f_factorial = addFunction(new FactorialFunction());
	f_factorial2 = addFunction(new DoubleFactorialFunction());
	f_multifactorial = addFunction(new MultiFactorialFunction());
	f_binomial = addFunction(new BinomialFunction());
	
	f_xor = addFunction(new XorFunction());
	f_bitxor = addFunction(new BitXorFunction());
	f_even = addFunction(new EvenFunction());
	f_odd = addFunction(new OddFunction());
	f_shift = addFunction(new ShiftFunction());
	f_bitcmp = addFunction(new BitCmpFunction());
	addFunction(new CircularShiftFunction());
	
	f_abs = addFunction(new AbsFunction());
	f_signum = addFunction(new SignumFunction());
	f_heaviside = addFunction(new HeavisideFunction());
	f_dirac = addFunction(new DiracFunction());
	f_gcd = addFunction(new GcdFunction());
	f_lcm = addFunction(new LcmFunction());
	f_round = addFunction(new RoundFunction());
	f_floor = addFunction(new FloorFunction());
	f_ceil = addFunction(new CeilFunction());
	f_trunc = addFunction(new TruncFunction());
	f_int = addFunction(new IntFunction());
	f_frac = addFunction(new FracFunction());
	f_rem = addFunction(new RemFunction());
	f_mod = addFunction(new ModFunction());
	
	f_polynomial_unit = addFunction(new PolynomialUnitFunction());
	f_polynomial_primpart = addFunction(new PolynomialPrimpartFunction());
	f_polynomial_content = addFunction(new PolynomialContentFunction());
	f_coeff = addFunction(new CoeffFunction());
	f_lcoeff = addFunction(new LCoeffFunction());
	f_tcoeff = addFunction(new TCoeffFunction());
	f_degree = addFunction(new DegreeFunction());
	f_ldegree = addFunction(new LDegreeFunction());

	f_re = addFunction(new ReFunction());
	f_im = addFunction(new ImFunction());
	f_arg = addFunction(new ArgFunction());
	f_numerator = addFunction(new NumeratorFunction());
	f_denominator = addFunction(new DenominatorFunction());
	
	f_interval = addFunction(new IntervalFunction());
	f_uncertainty = addFunction(new UncertaintyFunction());

	f_sqrt = addFunction(new SqrtFunction());
	f_cbrt = addFunction(new CbrtFunction());
	f_root = addFunction(new RootFunction());
	f_sq = addFunction(new SquareFunction());

	f_exp = addFunction(new ExpFunction());

	f_ln = addFunction(new LogFunction());
	f_logn = addFunction(new LognFunction());

	f_lambert_w = addFunction(new LambertWFunction());

	f_sin = addFunction(new SinFunction());
	f_cos = addFunction(new CosFunction());
	f_tan = addFunction(new TanFunction());
	f_asin = addFunction(new AsinFunction());
	f_acos = addFunction(new AcosFunction());
	f_atan = addFunction(new AtanFunction());
	f_sinh = addFunction(new SinhFunction());
	f_cosh = addFunction(new CoshFunction());
	f_tanh = addFunction(new TanhFunction());
	f_asinh = addFunction(new AsinhFunction());
	f_acosh = addFunction(new AcoshFunction());
	f_atanh = addFunction(new AtanhFunction());
	f_atan2 = addFunction(new Atan2Function());
	f_sinc = addFunction(new SincFunction());
	f_radians_to_default_angle_unit = addFunction(new RadiansToDefaultAngleUnitFunction());

	f_zeta = addFunction(new ZetaFunction());
	f_gamma = addFunction(new GammaFunction());
	f_digamma = addFunction(new DigammaFunction());
	f_beta = addFunction(new BetaFunction());
	f_airy = addFunction(new AiryFunction());
	f_besselj = addFunction(new BesseljFunction());
	f_bessely = addFunction(new BesselyFunction());
	f_erf = addFunction(new ErfFunction());
	f_erfc = addFunction(new ErfcFunction());

	f_total = addFunction(new TotalFunction());
	f_percentile = addFunction(new PercentileFunction());
	f_min = addFunction(new MinFunction());
	f_max = addFunction(new MaxFunction());
	f_mode = addFunction(new ModeFunction());
	f_rand = addFunction(new RandFunction());

	f_date = addFunction(new DateFunction());
	f_datetime = addFunction(new DateTimeFunction());
	f_timevalue = addFunction(new TimeValueFunction());
	f_timestamp = addFunction(new TimestampFunction());
	f_stamptodate = addFunction(new TimestampToDateFunction());
	f_days = addFunction(new DaysFunction());
	f_yearfrac = addFunction(new YearFracFunction());
	f_week = addFunction(new WeekFunction());
	f_weekday = addFunction(new WeekdayFunction());
	f_month = addFunction(new MonthFunction());
	f_day = addFunction(new DayFunction());
	f_year = addFunction(new YearFunction());
	f_yearday = addFunction(new YeardayFunction());
	f_time = addFunction(new TimeFunction());
	f_add_days = addFunction(new AddDaysFunction());
	f_add_months = addFunction(new AddMonthsFunction());
	f_add_years = addFunction(new AddYearsFunction());
	
	f_lunarphase = addFunction(new LunarPhaseFunction());
	f_nextlunarphase = addFunction(new NextLunarPhaseFunction());

	f_base = addFunction(new BaseFunction());
	f_bin = addFunction(new BinFunction());
	f_oct = addFunction(new OctFunction());
	addFunction(new DecFunction());
	f_hex = addFunction(new HexFunction());
	f_roman = addFunction(new RomanFunction());

	f_ascii = addFunction(new AsciiFunction());
	f_char = addFunction(new CharFunction());

	f_length = addFunction(new LengthFunction());
	f_concatenate = addFunction(new ConcatenateFunction());
		
	f_replace = addFunction(new ReplaceFunction());
	f_stripunits = addFunction(new StripUnitsFunction());

	f_genvector = addFunction(new GenerateVectorFunction());
	f_for = addFunction(new ForFunction());
	f_sum = addFunction(new SumFunction());
	f_product = addFunction(new ProductFunction());
	f_process = addFunction(new ProcessFunction());
	f_process_matrix = addFunction(new ProcessMatrixFunction());
	f_csum = addFunction(new CustomSumFunction());
	f_function = addFunction(new FunctionFunction());
	f_select = addFunction(new SelectFunction());
	f_title = addFunction(new TitleFunction());
	f_if = addFunction(new IFFunction());
	f_is_number = addFunction(new IsNumberFunction());
	f_is_real = addFunction(new IsRealFunction());
	f_is_rational = addFunction(new IsRationalFunction());
	f_is_integer = addFunction(new IsIntegerFunction());
	f_represents_number = addFunction(new RepresentsNumberFunction());
	f_represents_real = addFunction(new RepresentsRealFunction());
	f_represents_rational = addFunction(new RepresentsRationalFunction());
	f_represents_integer = addFunction(new RepresentsIntegerFunction());
	f_error = addFunction(new ErrorFunction());
	f_warning = addFunction(new WarningFunction());
	f_message = addFunction(new MessageFunction());
	
	f_save = addFunction(new SaveFunction());
	f_load = addFunction(new LoadFunction());
	f_export = addFunction(new ExportFunction());

	f_register = addFunction(new RegisterFunction());
	f_stack = addFunction(new StackFunction());

	f_diff = addFunction(new DeriveFunction());
	f_integrate = addFunction(new IntegrateFunction());
	f_solve = addFunction(new SolveFunction());
	f_multisolve = addFunction(new SolveMultipleFunction());
	f_dsolve = addFunction(new DSolveFunction());
	f_limit = addFunction(new LimitFunction());
	
	f_li = addFunction(new liFunction());
	f_Li = addFunction(new LiFunction());
	f_Ei = addFunction(new EiFunction());
	f_Si = addFunction(new SiFunction());
	f_Ci = addFunction(new CiFunction());
	f_Shi = addFunction(new ShiFunction());
	f_Chi = addFunction(new ChiFunction());
	f_igamma = addFunction(new IGammaFunction());
	
	if(canPlot()) f_plot = addFunction(new PlotFunction());
	
	/*void *plugin = dlopen("", RTLD_NOW);
	if(plugin) {
		CREATEPLUG_PROC createproc = (CREATEPLUG_PROC) dlsym(plugin, "createPlugin");
		if (dlerror() != NULL) {
			dlclose(plugin);
			printf( "dlsym error\n");
		} else {
			createproc();
		}
	} else {
		printf( "dlopen error\n");
	}*/

}
void Calculator::addBuiltinUnits() {
	u_euro = addUnit(new Unit(_("Currency"), "EUR", "euros", "euro", "European Euros", false, true, true));
	u_btc = addUnit(new AliasUnit(_("Currency"), "BTC", "bitcoins", "bitcoin", "Bitcoins", u_euro, "8794.46", 1, "", false, true, true));
	u_btc->setApproximate();
	u_btc->setPrecision(-2);
	u_btc->setChanged(false);
	u_second = NULL;
	u_minute = NULL;
	u_hour = NULL;
	u_day = NULL;
	u_month = NULL;
	u_year = NULL;
}

void Calculator::setVariableUnitsEnabled(bool enable_variable_units) {
	b_var_units = enable_variable_units;
}
bool Calculator::variableUnitsEnabled() const {
	return b_var_units;
}

void Calculator::error(bool critical, int message_category, const char *TEMPLATE, ...) {
	va_list ap;
	va_start(ap, TEMPLATE);
	message(critical ? MESSAGE_ERROR : MESSAGE_WARNING, message_category, TEMPLATE, ap);
	va_end(ap);
}
void Calculator::error(bool critical, const char *TEMPLATE, ...) {
	va_list ap;
	va_start(ap, TEMPLATE);
	message(critical ? MESSAGE_ERROR : MESSAGE_WARNING, MESSAGE_CATEGORY_NONE, TEMPLATE, ap);
	va_end(ap);
}
void Calculator::message(MessageType mtype, int message_category, const char *TEMPLATE, ...) {
	va_list ap;
	va_start(ap, TEMPLATE);
	message(mtype, message_category, TEMPLATE, ap);
	va_end(ap);
}
void Calculator::message(MessageType mtype, const char *TEMPLATE, ...) {
	va_list ap;
	va_start(ap, TEMPLATE);
	message(mtype, MESSAGE_CATEGORY_NONE, TEMPLATE, ap);
	va_end(ap);
}
void Calculator::message(MessageType mtype, int message_category, const char *TEMPLATE, va_list ap) {
	if(disable_errors_ref > 0) {
		stopped_messages_count[disable_errors_ref - 1]++;
		if(mtype == MESSAGE_ERROR) {
			stopped_errors_count[disable_errors_ref - 1]++;
		} else if(mtype == MESSAGE_WARNING) {
			stopped_warnings_count[disable_errors_ref - 1]++;
		}
	}
	string error_str = TEMPLATE;
	size_t i = 0;
	while(true) {
		i = error_str.find("%", i);
		if(i == string::npos || i + 1 == error_str.length()) break;
		switch(error_str[i + 1]) {
			case 's': {
				const char *str = va_arg(ap, const char*);
				if(!str) {
					i++;
				} else {
					error_str.replace(i, 2, str);
					i += strlen(str);
				}
				break;
			}
			case 'c': {
				char c = (char) va_arg(ap, int);
				if(c > 0) {
					error_str.replace(i, 2, 1, c);
				}
				i++;
				break;
			}
			default: {
				i++;
				break;
			}
		}
	}
	bool dup_error = false;
	for(i = 0; i < messages.size(); i++) {
		if(error_str == messages[i].message()) {
			dup_error = true;
			break;
		}
	}
	if(disable_errors_ref > 0) {
		for(size_t i2 = 0; !dup_error && i2 < (size_t) disable_errors_ref; i2++) {
			for(i = 0; i < stopped_messages[i2].size(); i++) {
				if(error_str == stopped_messages[i2][i].message()) {
					dup_error = true;
					break;
				}
			}
		}
	}
	if(!dup_error) {
		if(disable_errors_ref > 0) stopped_messages[disable_errors_ref - 1].push_back(CalculatorMessage(error_str, mtype, message_category, current_stage));
		else messages.push_back(CalculatorMessage(error_str, mtype, message_category, current_stage));
	}
}
CalculatorMessage* Calculator::message() {
	if(!messages.empty()) {
		return &messages[0];
	}
	return NULL;
}
CalculatorMessage* Calculator::nextMessage() {
	if(!messages.empty()) {
		messages.erase(messages.begin());
		if(!messages.empty()) {
			return &messages[0];
		}
	}
	return NULL;
}
void Calculator::cleanMessages(const MathStructure &mstruct, size_t first_message) {
	if(first_message > 0) first_message--;
	if(messages.size() <= first_message) return;
	if(mstruct.containsInterval(true, false, false, -2, true) <= 0) {
		for(size_t i = messages.size() - 1; ; i--) {
			if(messages[i].category() == MESSAGE_CATEGORY_WIDE_INTERVAL) {
				messages.erase(messages.begin() + i);
			}
			if(i == first_message) break;
		}
	}
}
void Calculator::deleteName(string name_, ExpressionItem *object) {
	Variable *v2 = getVariable(name_);
	if(v2 == object) {
		return;
	}
	if(v2 != NULL) {
		v2->destroy();
	} else {
		MathFunction *f2 = getFunction(name_);
		if(f2 == object)
			return;
		if(f2 != NULL) {
			f2->destroy();
		}
	}
	deleteName(name_, object);
}
void Calculator::deleteUnitName(string name_, Unit *object) {
	Unit *u2 = getUnit(name_);
	if(u2) {
		if(u2 != object) {
			u2->destroy();
		}
		return;
	} 
	u2 = getCompositeUnit(name_);	
	if(u2) {
		if(u2 != object) {
			u2->destroy();
		}
	}
	deleteUnitName(name_, object);
}
void Calculator::saveState() {
}
void Calculator::restoreState() {
}
void Calculator::clearBuffers() {
	for(unordered_map<size_t, bool>::iterator it = priv->ids_p.begin(); it != priv->ids_p.end(); ++it) {
		if(!it->second) {
			priv->freed_ids.push_back(it->first);
			priv->id_structs.erase(it->first);
			priv->ids_p.erase(it);
		}
	}
}
bool Calculator::abort() {
	i_aborted = 1;
	if(!b_busy) return true;
	if(!calculate_thread->running) {
		b_busy = false;
	} else {
		int msecs = 5000;
		while(b_busy && msecs > 0) {
			sleep_ms(10);
			msecs -= 10;
		}
		if(b_busy) {
			calculate_thread->cancel();
			stopControl();
			stopped_messages_count.clear();
			stopped_warnings_count.clear();
			stopped_errors_count.clear();
			stopped_messages.clear();
			disable_errors_ref = 0;
			if(tmp_rpn_mstruct) tmp_rpn_mstruct->unref();
			tmp_rpn_mstruct = NULL;
			error(true, _("The calculation has been forcibly terminated. Please restart the application and report this as a bug."), NULL);
			b_busy = false;
			calculate_thread->start();
			return false;
		}
	}
	return true;
}
bool Calculator::busy() {
	return b_busy;
}
void Calculator::terminateThreads() {
	if(calculate_thread->running) {
		if(!calculate_thread->write(false) || !calculate_thread->write(NULL)) calculate_thread->cancel();
		for(size_t i = 0; i < 10 && calculate_thread->running; i++) {
			sleep_ms(1);
		}
		if(calculate_thread->running) calculate_thread->cancel();
	}
}

string Calculator::localizeExpression(string str, const ParseOptions &po) const {
	if((DOT_STR == DOT && COMMA_STR == COMMA && !po.comma_as_separator) || po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return str;
	vector<size_t> q_begin;
	vector<size_t> q_end;
	size_t i3 = 0;
	while(true) {
		i3 = str.find_first_of("\"\'", i3);
		if(i3 == string::npos) {
			break;
		}
		q_begin.push_back(i3);
		i3 = str.find(str[i3], i3 + 1);
		if(i3 == string::npos) {
			q_end.push_back(str.length() - 1);
			break;
		}
		q_end.push_back(i3);
		i3++;
	}
	if(COMMA_STR != COMMA || po.comma_as_separator) {
		bool b_alt_comma = po.comma_as_separator && COMMA_STR == COMMA;
		size_t ui = str.find(COMMA);
		while(ui != string::npos) {
			bool b = false;
			for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
				if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
					ui = str.find(COMMA, q_end[ui2] + 1);
					b = true;
					break;
				}
			}
			if(!b) {
				str.replace(ui, strlen(COMMA), b_alt_comma ? ";" : COMMA_STR);
				ui = str.find(COMMA, ui + (b_alt_comma ? 1 : COMMA_STR.length()));
			}
		}
	}
	if(DOT_STR != DOT) {
		size_t ui = str.find(DOT);
		while(ui != string::npos) {
			bool b = false;
			for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
				if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
					ui = str.find(DOT, q_end[ui2] + 1);
					b = true;
					break;
				}
			}
			if(!b) {
				str.replace(ui, strlen(DOT), DOT_STR);
				ui = str.find(DOT, ui + DOT_STR.length());
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
	vector<size_t> q_begin;
	vector<size_t> q_end;
	size_t i3 = 0;
	while(true) {
		i3 = str.find_first_of("\"\'", i3);
		if(i3 == string::npos) {
			break;
		}
		q_begin.push_back(i3);
		i3 = str.find(str[i3], i3 + 1);
		if(i3 == string::npos) {
			q_end.push_back(str.length() - 1);
			break;
		}
		q_end.push_back(i3);
		i3++;
	}
	if(DOT_STR != DOT) {
		if(DOT_STR == COMMA && str.find(COMMA_STR) == string::npos && base > 0 && base <= 10) {
			size_t ui = str.find(DOT_STR);
			while(ui != string::npos) {
				bool b = false;
				for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
					if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
						ui = str.find(DOT_STR, q_end[ui2] + 1);
						b = true;
						break;
					}
				}
				if(!b && ui > 0) {
					size_t ui2 = str.find_last_not_of(SPACES, ui - 1);
					if(ui2 != string::npos && ((str[ui2] > 'a' && str[ui2] < 'z') || (str[ui2] > 'A' && str[ui2] < 'Z')) && is_not_number(str[ui2], base)) return str;
				}
				if(!b && ui != str.length() - 1) {
					size_t ui2 = str.find_last_not_of(SPACES, ui + 1);
					if(ui2 != string::npos && is_not_number(str[ui2], base)) return str;
					ui2 = str.find_first_not_of(SPACES NUMBERS, ui2 + 1);
					if(ui2 != string::npos && str[ui2] == COMMA_CH) return str;
				}
				if(!b) {
					ui = str.find(DOT_STR, ui + 1);
				}
			}
		}
		if(po.dot_as_separator) {
			size_t ui = str.find(DOT);
			while(ui != string::npos) {
				bool b = false;
				for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
					if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
						ui = str.find(DOT, q_end[ui2] + 1);
						b = true;
						break;
					}
				}
				if(!b) {
					str.replace(ui, strlen(DOT), SPACE);
					ui = str.find(DOT, ui + strlen(SPACE));
				}
			}
		}
		size_t ui = str.find(DOT_STR);
		while(ui != string::npos) {
			bool b = false;
			for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
				if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
					ui = str.find(DOT_STR, q_end[ui2] + 1);
					b = true;
					break;
				}
			}
			if(!b) {
				str.replace(ui, DOT_STR.length(), DOT);
				ui = str.find(DOT_STR, ui + strlen(DOT));
			}
		}
	}
	if(COMMA_STR != COMMA || po.comma_as_separator) {
		bool b_alt_comma = po.comma_as_separator && COMMA_STR == COMMA;
		if(po.comma_as_separator) {
			size_t ui = str.find(COMMA);
			while(ui != string::npos) {
				bool b = false;
				for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
					if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
						ui = str.find(COMMA, q_end[ui2] + 1);
						b = true;
						break;
					}
				}
				if(!b) {
					str.erase(ui, strlen(COMMA));
					ui = str.find(COMMA, ui);
				}
			}	
		}
		size_t ui = str.find(b_alt_comma ? ";" : COMMA_STR);
		while(ui != string::npos) {
			bool b = false;
			for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
				if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
					ui = str.find(b_alt_comma ? ";" : COMMA_STR, q_end[ui2] + 1);
					b = true;
					break;
				}
			}
			if(!b) {
				str.replace(ui, b_alt_comma ? 1 : COMMA_STR.length(), COMMA);
				ui = str.find(b_alt_comma ? ";" : COMMA_STR, ui + strlen(COMMA));
			}
		}
	}
	return str;
}

bool Calculator::calculateRPNRegister(size_t index, int msecs, const EvaluationOptions &eo) {
	if(index <= 0 || index > rpn_stack.size()) return false;
	return calculateRPN(new MathStructure(*rpn_stack[rpn_stack.size() - index]), PROC_RPN_SET, index, msecs, eo);
}

bool Calculator::calculateRPN(MathStructure *mstruct, int command, size_t index, int msecs, const EvaluationOptions &eo, int function_arguments) {
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;
	tmp_evaluationoptions = eo;
	tmp_proc_command = command;
	tmp_rpnindex = index;
	tmp_rpn_mstruct = mstruct;
	tmp_proc_registers = function_arguments;
	if(!calculate_thread->write(false)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}	
	if(had_msecs && b_busy) {
		abort();
		return false;
	}
	return true;
}
bool Calculator::calculateRPN(string str, int command, size_t index, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division, int function_arguments) {
	MathStructure *mstruct = new MathStructure();
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;
	expression_to_calculate = str;
	tmp_evaluationoptions = eo;
	tmp_proc_command = command;
	tmp_rpnindex = index;
	tmp_rpn_mstruct = mstruct;
	tmp_parsedstruct = parsed_struct;
	tmp_tostruct = to_struct;
	tmp_maketodivision = make_to_division;
	tmp_proc_registers = function_arguments;
	if(!calculate_thread->write(true)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}	
	if(had_msecs && b_busy) {
		abort();
		return false;
	}
	return true;
}

bool Calculator::calculateRPN(MathOperation op, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->add(m_zero, op);
		if(parsed_struct) parsed_struct->clear();
	} else if(rpn_stack.size() == 1) {
		if(parsed_struct) {
			parsed_struct->set(*rpn_stack.back());
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		if(op == OPERATION_SUBTRACT) {
			mstruct = new MathStructure();
		} else {
			mstruct = new MathStructure(*rpn_stack.back());
		}
		mstruct->add(*rpn_stack.back(), op);
	} else {
		if(parsed_struct) {
			parsed_struct->set(*rpn_stack[rpn_stack.size() - 2]);
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_ADDITION, *rpn_stack.back());
				(*parsed_struct)[1].transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		mstruct = new MathStructure(*rpn_stack[rpn_stack.size() - 2]);
		mstruct->add(*rpn_stack.back(), op);
	}
	return calculateRPN(mstruct, PROC_RPN_OPERATION_2, 0, msecs, eo);
}
bool Calculator::calculateRPN(MathFunction *f, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct = new MathStructure(f, NULL);
	int iregs = 0;
	if(f->args() != 0) {
		size_t i = f->minargs();
		bool fill_vector = (i > 0 && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_VECTOR);
		if(fill_vector && rpn_stack.size() < i) fill_vector = false;
		if(fill_vector && rpn_stack.size() > 0 && rpn_stack.back()->isVector()) fill_vector = false;
		if(fill_vector) {
			i = rpn_stack.size();
		} else if(i < 1) {
			i = 1;
		}
		for(; i > 0; i--) {
			if(i > rpn_stack.size()) {
				error(false, _("Stack is empty. Filling remaining function arguments with zeroes."), NULL);
				mstruct->addChild(m_zero);
			} else {
				if(fill_vector && rpn_stack.size() - i == (size_t) f->minargs() - 1) mstruct->addChild(m_empty_vector);
				if(fill_vector && rpn_stack.size() - i >= (size_t) f->minargs() - 1) mstruct->getChild(f->minargs())->addChild(*rpn_stack[rpn_stack.size() - i]);
				else mstruct->addChild(*rpn_stack[rpn_stack.size() - i]);
				iregs++;
			}
			if(!fill_vector && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_ANGLE) {
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {
						(*mstruct)[i - 1].multiply(getDegUnit());
						break;
					}
					case ANGLE_UNIT_GRADIANS: {
						(*mstruct)[i - 1].multiply(getGraUnit());
						break;
					}
					case ANGLE_UNIT_RADIANS: {
						(*mstruct)[i - 1].multiply(getRadUnit());
						break;
					}
					default: {}
				}
			}
		}
		if(fill_vector) mstruct->childrenUpdated();
		f->appendDefaultValues(*mstruct);
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	return calculateRPN(mstruct, PROC_RPN_OPERATION_F, 0, msecs, eo, iregs);
}
bool Calculator::calculateRPNBitwiseNot(int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setBitwiseNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setBitwiseNot();
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	return calculateRPN(mstruct, PROC_RPN_OPERATION_1, 0, msecs, eo);
}
bool Calculator::calculateRPNLogicalNot(int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setLogicalNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setLogicalNot();
	}
	if(parsed_struct) parsed_struct->set(*rpn_stack.back());
	return calculateRPN(mstruct, PROC_RPN_OPERATION_1, 0, msecs, eo);
}
MathStructure *Calculator::calculateRPN(MathOperation op, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->add(m_zero, op);
		if(parsed_struct) parsed_struct->clear();
	} else if(rpn_stack.size() == 1) {
		if(parsed_struct) {
			parsed_struct->clear();
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_ADDITION, *rpn_stack.back());
				(*parsed_struct)[1].transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		mstruct = new MathStructure();
		mstruct->add(*rpn_stack.back(), op);
	} else {
		if(parsed_struct) {
			parsed_struct->set(*rpn_stack[rpn_stack.size() - 2]);
			if(op == OPERATION_SUBTRACT) {
				parsed_struct->transform(STRUCT_ADDITION, *rpn_stack.back());
				(*parsed_struct)[1].transform(STRUCT_NEGATE);
			} else if(op == OPERATION_DIVIDE) {
				parsed_struct->transform(STRUCT_DIVISION, *rpn_stack.back());
			} else {
				parsed_struct->add(*rpn_stack.back(), op);
			}
		}
		mstruct = new MathStructure(*rpn_stack[rpn_stack.size() - 2]);
		mstruct->add(*rpn_stack.back(), op);
	}
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(rpn_stack.size() > 1) {
		rpn_stack.back()->unref();
		rpn_stack.erase(rpn_stack.begin() + (rpn_stack.size() - 1));
	}
	if(rpn_stack.size() > 0) {
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	} else {
		rpn_stack.push_back(mstruct);
	}
	return rpn_stack.back();
}
MathStructure *Calculator::calculateRPN(MathFunction *f, const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct = new MathStructure(f, NULL);
	size_t iregs = 0;
	if(f->args() != 0) {
		size_t i = f->minargs();
		bool fill_vector = (i > 0 && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_VECTOR);
		if(fill_vector && rpn_stack.size() < i) fill_vector = false;
		if(fill_vector && rpn_stack.size() > 0 && rpn_stack.back()->isVector()) fill_vector = false;
		if(fill_vector) {
			i = rpn_stack.size();
		} else if(i < 1) {
			i = 1;
		}
		for(; i > 0; i--) {
			if(i > rpn_stack.size()) {
				error(false, _("Stack is empty. Filling remaining function arguments with zeroes."), NULL);
				mstruct->addChild(m_zero);
			} else {
				if(fill_vector && rpn_stack.size() - i == (size_t) f->minargs() - 1) mstruct->addChild(m_empty_vector);
				if(fill_vector && rpn_stack.size() - i >= (size_t) f->minargs() - 1) mstruct->getChild(f->minargs())->addChild(*rpn_stack[rpn_stack.size() - i]);
				else mstruct->addChild(*rpn_stack[rpn_stack.size() - i]);
				iregs++;
			}
			if(!fill_vector && f->getArgumentDefinition(i) && f->getArgumentDefinition(i)->type() == ARGUMENT_TYPE_ANGLE) {
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {
						(*mstruct)[i - 1].multiply(getDegUnit());
						break;
					}
					case ANGLE_UNIT_GRADIANS: {
						(*mstruct)[i - 1].multiply(getGraUnit());
						break;
					}
					case ANGLE_UNIT_RADIANS: {
						(*mstruct)[i - 1].multiply(getRadUnit());
						break;
					}
					default: {}
				}
			}
		}
		if(fill_vector) mstruct->childrenUpdated();
		f->appendDefaultValues(*mstruct);
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(iregs == 0) {
		rpn_stack.push_back(mstruct);
	} else {
		for(size_t i = 0; i < iregs - 1 && rpn_stack.size() > 1; i++) {
			rpn_stack.back()->unref();
			rpn_stack.pop_back();
			deleteRPNRegister(1);
		}
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	}
	return rpn_stack.back();
}
MathStructure *Calculator::calculateRPNBitwiseNot(const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setBitwiseNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setBitwiseNot();
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(rpn_stack.size() == 0) {
		rpn_stack.push_back(mstruct);
	} else {
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	}
	return rpn_stack.back();
}
MathStructure *Calculator::calculateRPNLogicalNot(const EvaluationOptions &eo, MathStructure *parsed_struct) {
	current_stage = MESSAGE_STAGE_PARSING;
	MathStructure *mstruct;
	if(rpn_stack.size() == 0) {
		mstruct = new MathStructure();
		mstruct->setLogicalNot();
	} else {
		mstruct = new MathStructure(*rpn_stack.back());
		mstruct->setLogicalNot();
	}
	if(parsed_struct) parsed_struct->set(*mstruct);
	current_stage = MESSAGE_STAGE_CALCULATION;
	mstruct->eval(eo);
	current_stage = MESSAGE_STAGE_CONVERSION;
	autoConvert(*mstruct, *mstruct, eo);
	current_stage = MESSAGE_STAGE_UNSET;
	if(rpn_stack.size() == 0) {
		rpn_stack.push_back(mstruct);
	} else {
		rpn_stack.back()->unref();
		rpn_stack.back() = mstruct;
	}
	return rpn_stack.back();
}
bool Calculator::RPNStackEnter(MathStructure *mstruct, int msecs, const EvaluationOptions &eo) {
	return calculateRPN(mstruct, PROC_RPN_ADD, 0, msecs, eo);
}
bool Calculator::RPNStackEnter(string str, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	remove_blank_ends(str);
	if(str.empty() && rpn_stack.size() > 0) {
		rpn_stack.push_back(new MathStructure(*rpn_stack.back()));
		return true;
	}
	return calculateRPN(str, PROC_RPN_ADD, 0, msecs, eo, parsed_struct, to_struct, make_to_division);
}
void Calculator::RPNStackEnter(MathStructure *mstruct, bool eval, const EvaluationOptions &eo) {
	if(eval) {
		current_stage = MESSAGE_STAGE_CALCULATION;
		mstruct->eval(eo);
		current_stage = MESSAGE_STAGE_CONVERSION;
		autoConvert(*mstruct, *mstruct, eo);
		current_stage = MESSAGE_STAGE_UNSET;
	}
	rpn_stack.push_back(mstruct);
}
void Calculator::RPNStackEnter(string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	remove_blank_ends(str);
	if(str.empty() && rpn_stack.size() > 0) rpn_stack.push_back(new MathStructure(*rpn_stack.back()));
	else rpn_stack.push_back(new MathStructure(calculate(str, eo, parsed_struct, to_struct, make_to_division)));
}
bool Calculator::setRPNRegister(size_t index, MathStructure *mstruct, int msecs, const EvaluationOptions &eo) {
	if(mstruct == NULL) {
		deleteRPNRegister(index);
		return true;
	}
	if(index <= 0 || index > rpn_stack.size()) return false;
	return calculateRPN(mstruct, PROC_RPN_SET, index, msecs, eo);
}
bool Calculator::setRPNRegister(size_t index, string str, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	if(index <= 0 || index > rpn_stack.size()) return false;
	return calculateRPN(str, PROC_RPN_SET, index, msecs, eo, parsed_struct, to_struct, make_to_division);
}
void Calculator::setRPNRegister(size_t index, MathStructure *mstruct, bool eval, const EvaluationOptions &eo) {
	if(mstruct == NULL) {
		deleteRPNRegister(index);
		return;
	}
	if(eval) {
		current_stage = MESSAGE_STAGE_CALCULATION;
		mstruct->eval(eo);
		current_stage = MESSAGE_STAGE_CONVERSION;
		autoConvert(*mstruct, *mstruct, eo);
		current_stage = MESSAGE_STAGE_UNSET;
	}
	if(index <= 0 || index > rpn_stack.size()) return;
	index = rpn_stack.size() - index;
	rpn_stack[index]->unref();
	rpn_stack[index] = mstruct;
}
void Calculator::setRPNRegister(size_t index, string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	if(index <= 0 || index > rpn_stack.size()) return;
	index = rpn_stack.size() - index;
	MathStructure *mstruct = new MathStructure(calculate(str, eo, parsed_struct, to_struct, make_to_division));
	rpn_stack[index]->unref();
	rpn_stack[index] = mstruct;
}
void Calculator::deleteRPNRegister(size_t index) {
	if(index <= 0 || index > rpn_stack.size()) return;
	index = rpn_stack.size() - index;
	rpn_stack[index]->unref();
	rpn_stack.erase(rpn_stack.begin() + index);
}
MathStructure *Calculator::getRPNRegister(size_t index) const {
	if(index > 0 && index <= rpn_stack.size()) {
		index = rpn_stack.size() - index;
		return rpn_stack[index];
	}
	return NULL;
}
size_t Calculator::RPNStackSize() const {
	return rpn_stack.size();
}
void Calculator::clearRPNStack() {
	for(size_t i = 0; i < rpn_stack.size(); i++) {
		rpn_stack[i]->unref();
	}
	rpn_stack.clear();
}
void Calculator::moveRPNRegister(size_t old_index, size_t new_index) {
	if(old_index == new_index) return;
	if(old_index > 0 && old_index <= rpn_stack.size()) {
		old_index = rpn_stack.size() - old_index;
		MathStructure *mstruct = rpn_stack[old_index];
		if(new_index > rpn_stack.size()) {
			new_index = 0;
		} else if(new_index <= 1) {
			rpn_stack.push_back(mstruct);
			rpn_stack.erase(rpn_stack.begin() + old_index);
			return;
		} else {
			new_index = rpn_stack.size() - new_index;
		}
		if(new_index > old_index) {
			rpn_stack.erase(rpn_stack.begin() + old_index);
			rpn_stack.insert(rpn_stack.begin() + new_index, mstruct);
		} else if(new_index < old_index) {
			rpn_stack.insert(rpn_stack.begin() + new_index, mstruct);
			rpn_stack.erase(rpn_stack.begin() + (old_index + 1));
		}
	}
}
void Calculator::moveRPNRegisterUp(size_t index) {
	if(index > 1 && index <= rpn_stack.size()) {
		index = rpn_stack.size() - index;
		MathStructure *mstruct = rpn_stack[index];
		rpn_stack.erase(rpn_stack.begin() + index);
		index++;
		if(index == rpn_stack.size()) rpn_stack.push_back(mstruct);
		else rpn_stack.insert(rpn_stack.begin() + index, mstruct);
	}
}
void Calculator::moveRPNRegisterDown(size_t index) {
	if(index > 0 && index < rpn_stack.size()) {
		index = rpn_stack.size() - index;
		MathStructure *mstruct = rpn_stack[index];
		rpn_stack.erase(rpn_stack.begin() + index);
		index--;
		rpn_stack.insert(rpn_stack.begin() + index, mstruct);
	}
}

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z))
string Calculator::calculateAndPrint(string str, int msecs, const EvaluationOptions &eo, const PrintOptions &po) {
	if(msecs > 0) startControl(msecs);
	PrintOptions printops = po;
	EvaluationOptions evalops = eo;
	MathStructure mstruct;
	bool do_bases = false, do_factors = false, do_fraction = false, do_pfe = false, do_calendars = false, do_expand = false;
	string from_str = str, to_str;
	Number base_save;
	if(printops.base == BASE_CUSTOM) base_save = customOutputBase();
	if(separateToExpression(from_str, to_str, evalops, true)) {
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
			str = from_str;
			printops.base = BASE_HEXADECIMAL;
		} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", _("binary"))) {
			str = from_str;
			printops.base = BASE_BINARY;
		} else if(equalsIgnoreCase(to_str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimal", _("decimal"))) {
			str = from_str;
			printops.base = BASE_DECIMAL;
		} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", _("octal"))) {
			str = from_str;
			printops.base = BASE_OCTAL;
		} else if(equalsIgnoreCase(to_str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "duodecimal", _("duodecimal"))) {
			str = from_str;
			printops.base = BASE_DUODECIMAL;
		} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, _("roman"))) {
			str = from_str;
			printops.base = BASE_ROMAN_NUMERALS;
		} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, _("sexagesimal"))) {
			str = from_str;
			printops.base = BASE_SEXAGESIMAL;
		} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, _("time"))) {
			str = from_str;
			printops.base = BASE_TIME;
		} else if(equalsIgnoreCase(to_str, "unicode")) {
			str = from_str;
			printops.base = BASE_UNICODE;
		} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
			str = from_str;
			printops.time_zone = TIME_ZONE_UTC;
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
				error(true, _("Time zone parsing failed."), NULL);
			}
			printops.time_zone = TIME_ZONE_CUSTOM;
			printops.custom_time_zone = itz;
			str = from_str;
		} else if(to_str == "CET") {
			printops.time_zone = TIME_ZONE_CUSTOM;
			printops.custom_time_zone = 60;
			str = from_str;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "fraction", _("fraction"))) {
			str = from_str;
			do_fraction = true;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", _("factors")) || equalsIgnoreCase(to_str, "factor")) {
			str = from_str;
			evalops.structuring = STRUCTURING_FACTORIZE;
			do_factors = true;
		}  else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, _("partial fraction"))) {
			str = from_str;
			do_pfe = true;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", _("bases"))) {
			do_bases = true;
			str = from_str;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "calendars", _("calendars"))) {
			do_calendars = true;
			str = from_str;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", _("optimal"))) {
			str = from_str;
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", _("base"))) {
			str = from_str;
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_BASE;
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str1, "base", _("base"))) {
			str = from_str;
			if(equalsIgnoreCase(to_str2, "golden") || equalsIgnoreCase(to_str2, "golden ratio") || to_str2 == "φ") printops.base = BASE_GOLDEN_RATIO;
			else if(equalsIgnoreCase(to_str2, "unicode")) printops.base = BASE_UNICODE;
			else if(equalsIgnoreCase(to_str2, "supergolden") || equalsIgnoreCase(to_str2, "supergolden ratio") || to_str2 == "ψ") printops.base = BASE_SUPER_GOLDEN_RATIO;
			else if(equalsIgnoreCase(to_str2, "pi") || to_str2 == "π") printops.base = BASE_PI;
			else if(to_str2 == "e") printops.base = BASE_E;
			else if(to_str2 == "sqrt(2)" || to_str2 == "sqrt 2" || to_str2 == "sqrt2" || to_str2 == "√2") printops.base = BASE_SQRT2;
			else {
				EvaluationOptions eo = evalops;
				eo.parse_options.base = 10;
				MathStructure m = calculate(to_str2, eo);
				if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
					printops.base = m.number().intValue();
				} else {
					printops.base = BASE_CUSTOM;
					base_save = customOutputBase();
					setCustomOutputBase(m.number());
				}
			}
		} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "mixed", _("mixed"))) {
			str = from_str;
			evalops.parse_options.units_enabled = true;
			evalops.auto_post_conversion = POST_CONVERSION_NONE;
			evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
		} else {
			evalops.parse_options.units_enabled = true;
		}
	} else {
		size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
		if(i != string::npos) {
			to_str = str.substr(0, i);
			if(to_str == "factor" || EQUALS_IGNORECASE_AND_LOCAL(to_str, "factorize", _("factorize"))) {
				str = str.substr(i + 1);
				do_factors = true;
				evalops.structuring = STRUCTURING_FACTORIZE;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "expand", _("expand"))) {
				str = str.substr(i + 1);
				evalops.structuring = STRUCTURING_SIMPLIFY;
				do_expand = true;
			}
		}
	}

	mstruct = calculate(str, evalops);
	
	if(do_factors) {
		mstruct.integerFactorize();
	} else if(do_expand) {
		mstruct.expand(evalops, false);
	}
	if(do_pfe) mstruct.expandPartialFractions(evalops);

	printops.allow_factorization = printops.allow_factorization || evalops.structuring == STRUCTURING_FACTORIZE || do_factors;
	
	if(do_calendars && mstruct.isDateTime()) {
		str = "";
		bool b_fail;
		long int y, m, d;
#define PRINT_CALENDAR(x, c) if(!str.empty()) {str += "\n";} str += x; str += " "; b_fail = !dateToCalendar(*mstruct.datetime(), y, m, d, c); if(b_fail) {str += _("failed");} else {str += i2s(d); str += " "; str += monthName(m, c, true); str += " "; str += i2s(y);}
		PRINT_CALENDAR(string(_("Gregorian:")), CALENDAR_GREGORIAN);
		PRINT_CALENDAR(string(_("Hebrew:")), CALENDAR_HEBREW);
		PRINT_CALENDAR(string(_("Islamic:")), CALENDAR_ISLAMIC);
		PRINT_CALENDAR(string(_("Persian:")), CALENDAR_PERSIAN);
		PRINT_CALENDAR(string(_("Indian national:")), CALENDAR_INDIAN);
		PRINT_CALENDAR(string(_("Chinese:")), CALENDAR_CHINESE); 
		long int cy, yc, st, br;
		chineseYearInfo(y, cy, yc, st, br);
		if(!b_fail) {str += " ("; str += chineseStemName(st); str += string(" "); str += chineseBranchName(br); str += ")";}
		PRINT_CALENDAR(string(_("Julian:")), CALENDAR_JULIAN);
		PRINT_CALENDAR(string(_("Revised julian:")), CALENDAR_MILANKOVIC);
		PRINT_CALENDAR(string(_("Coptic:")), CALENDAR_COPTIC);
		PRINT_CALENDAR(string(_("Ethiopian:")), CALENDAR_ETHIOPIAN);
		stopControl();
		return str;
	} else if(do_bases) {
		printops.base = BASE_BINARY;
		str = print(mstruct, 0, printops);
		str += " = ";
		printops.base = BASE_OCTAL;
		str += print(mstruct, 0, printops);
		str += " = ";
		printops.base = BASE_DECIMAL;
		str += print(mstruct, 0, printops);
		str += " = ";
		printops.base = BASE_HEXADECIMAL;
		str += print(mstruct, 0, printops);
		stopControl();
		return str;
	} else if(do_fraction) {
		if(mstruct.isNumber()) printops.number_fraction_format = FRACTION_COMBINED;
		else printops.number_fraction_format = FRACTION_FRACTIONAL;
	}
	mstruct.removeDefaultAngleUnit(evalops);
	mstruct.format(printops);
	str = mstruct.print(printops);
	stopControl();
	if(printops.base == BASE_CUSTOM) setCustomOutputBase(base_save);
	return str;
}
bool Calculator::calculate(MathStructure *mstruct, string str, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	mstruct->set(string(_("calculating...")), false, true);
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;
	expression_to_calculate = str;
	tmp_evaluationoptions = eo;
	tmp_proc_command = PROC_NO_COMMAND;
	tmp_rpn_mstruct = NULL;
	tmp_parsedstruct = parsed_struct;
	tmp_tostruct = to_struct;
	tmp_maketodivision = make_to_division;
	if(!calculate_thread->write(true)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}	
	if(had_msecs && b_busy) {
		if(!abort()) mstruct->setAborted();
		return false;
	}
	return true;
}
bool Calculator::calculate(MathStructure *mstruct, int msecs, const EvaluationOptions &eo, string to_str) {
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) {mstruct->setAborted(); return false;}
	bool had_msecs = msecs > 0;
	expression_to_calculate = "";
	tmp_evaluationoptions = eo;
	tmp_proc_command = PROC_NO_COMMAND;
	tmp_rpn_mstruct = NULL;
	tmp_parsedstruct = NULL;
	if(!to_str.empty()) tmp_tostruct = new MathStructure(to_str);
	else tmp_tostruct = NULL;
	tmp_tostruct = NULL;
	tmp_maketodivision = false;
	if(!calculate_thread->write(false)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	if(!calculate_thread->write((void*) mstruct)) {calculate_thread->cancel(); mstruct->setAborted(); return false;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}	
	if(had_msecs && b_busy) {
		if(!abort()) mstruct->setAborted();
		return false;
	}
	return true;
}
bool Calculator::hasToExpression(const string &str, bool allow_empty_from) const {
	if(str.rfind(_(" to ")) != string::npos || str.rfind(" to ") != string::npos) return true;
	if(allow_empty_from && (str.find("to ") == 0 || (str.find(_("to")) == 0 && str.length() > strlen(_("to")) && str[strlen(_("to"))] == ' '))) return true;
	return false;
}
bool Calculator::hasToExpression(const string &str, bool allow_empty_from, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	if(str.rfind(_(" to ")) != string::npos || str.rfind(" to ") != string::npos) return true;
	if(allow_empty_from && (str.find("to ") == 0 || (str.find(_("to")) == 0 && str.length() > strlen(_("to")) && str[strlen(_("to"))] == ' '))) return true;
	return false;
}
bool Calculator::separateToExpression(string &str, string &to_str, const EvaluationOptions &eo, bool keep_modifiers, bool allow_empty_from) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	to_str = "";
	size_t i = 0;
	if((i = str.find(_(" to "))) != string::npos) {
		size_t l = strlen(_(" to "));
		to_str = str.substr(i + l, str.length() - i - l);
	} else if((i = str.find(" to ")) != string::npos) {
		size_t l = strlen(" to ");
		to_str = str.substr(i + l, str.length() - i - l);
	} else if(allow_empty_from && str.find("to ") == 0) {
		to_str = str.substr(3);
		i = 0;
	} else if(allow_empty_from && (str.find(_("to")) == 0 && str.length() > strlen(_("to")) && str[strlen(_("to"))] == ' ')) {
		to_str = str.substr(strlen(_("to")));
		i = 0;
	} else {
		return false;
	}
	if(!to_str.empty()) {
		remove_blank_ends(to_str);
		if(to_str.rfind(SIGN_MINUS, 0) == 0) {
			to_str.replace(0, strlen(SIGN_MINUS), MINUS);
		}
		if(!keep_modifiers && (to_str[0] == '0' || to_str[0] == '?' || to_str[0] == '+' || to_str[0] == '-')) {
			to_str = to_str.substr(1, str.length() - 1);
			remove_blank_ends(to_str);
		}
		str = str.substr(0, i);
		return true;
	}
	return false;
}
bool Calculator::hasWhereExpression(const string &str, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	if(str.rfind(_(" where ")) != string::npos || str.rfind(" where ") != string::npos) return true;
	size_t i = 0;
	if((i = str.find(_("/."))) != string::npos && i != str.length() - 2 && eo.parse_options.base >= 2 && eo.parse_options.base <= 10 && (str[i + 2] < '0' || str[i + 2] > '9')) return true;
	return false;
}
bool Calculator::separateWhereExpression(string &str, string &to_str, const EvaluationOptions &eo) const {
	if(eo.parse_options.base == BASE_UNICODE || (eo.parse_options.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) return false;
	to_str = "";
	size_t i = 0;
	if((i = str.find(_(" where "))) != string::npos) {
		size_t l = strlen(_(" where "));
		to_str = str.substr(i + l, str.length() - i - l);
	} else if((i = str.find(" where ")) != string::npos) {
		to_str = str.substr(i + 7, str.length() - i - 7);
	} else if((i = str.find(_("/."))) != string::npos && i != str.length() - 2 && eo.parse_options.base >= 2 && eo.parse_options.base <= 10 && (str[i + 2] < '0' || str[i + 2] > '9')) {
		to_str = str.substr(i + 2, str.length() - i - 2);
	} else {
		return false;
	}
	if(!to_str.empty()) {
		remove_blank_ends(to_str);
		str = str.substr(0, i);
		return true;
	}
	return false;
}
extern string format_and_print(const MathStructure &mstruct);
bool handle_where_expression(MathStructure &m, MathStructure &mstruct, const EvaluationOptions &eo, vector<UnknownVariable*>& vars, vector<MathStructure>& varms) {
	if(m.isComparison()) {
		if(m.comparisonType() == COMPARISON_EQUALS) {
			mstruct.replace(m[0], m[1]);
			return true;
		} else if(m[0].isSymbolic() || (m[0].isVariable() && !m[0].variable()->isKnown())) {
			if(!m[1].isNumber()) m[1].eval(eo);
			if(m[1].isNumber() || m[1].number().hasImaginaryPart()) {
				Assumptions *ass = NULL;
				for(size_t i = 0; i < varms.size(); i++) {
					if(varms[i] == m[0]) {
						ass = vars[0]->assumptions();
						break;
					}
				}
				if((m.comparisonType() != COMPARISON_NOT_EQUALS || (!ass && m[1].isZero()))) {
					if(ass) {
						if(m.comparisonType() == COMPARISON_EQUALS_GREATER) {
							if(!ass->min() || (*ass->min() < m[1].number())) {
								ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(true);
								return true;
							} else if(*ass->min() >= m[1].number()) {
								return true;
							}
						} else if(m.comparisonType() == COMPARISON_EQUALS_LESS) {
							if(!ass->max() || (*ass->max() > m[1].number())) {
								ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(true);
								return true;
							} else if(*ass->max() <= m[1].number()) {
								return true;
							}
						} else if(m.comparisonType() == COMPARISON_GREATER) {
							if(!ass->min() || (ass->includeEqualsMin() && *ass->min() <= m[1].number()) || (!ass->includeEqualsMin() && *ass->min() < m[1].number())) {
								ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(false);
								return true;
							} else if((ass->includeEqualsMin() && *ass->min() > m[1].number()) || (!ass->includeEqualsMin() && *ass->min() >= m[1].number())) {
								return true;
							}
						} else if(m.comparisonType() == COMPARISON_LESS) {
							if(!ass->max() || (ass->includeEqualsMax() && *ass->max() >= m[1].number()) || (!ass->includeEqualsMax() && *ass->max() > m[1].number())) {
								ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(false);
								return true;
							} else if((ass->includeEqualsMax() && *ass->max() < m[1].number()) || (!ass->includeEqualsMax() && *ass->max() <= m[1].number())) {
								return true;
							}
						}
					} else {
						UnknownVariable *var = new UnknownVariable("", format_and_print(m[0]));
						ass = new Assumptions();
						if(m[1].isZero()) {
							if(m.comparisonType() == COMPARISON_EQUALS_GREATER) ass->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
							else if(m.comparisonType() == COMPARISON_EQUALS_LESS) ass->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
							else if(m.comparisonType() == COMPARISON_GREATER) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
							else if(m.comparisonType() == COMPARISON_LESS) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
							else if(m.comparisonType() == COMPARISON_NOT_EQUALS) ass->setSign(ASSUMPTION_SIGN_NONZERO);
						} else {
							if(m.comparisonType() == COMPARISON_EQUALS_GREATER) {ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(true);}
							else if(m.comparisonType() == COMPARISON_EQUALS_LESS) {ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(true);}
							else if(m.comparisonType() == COMPARISON_GREATER) {ass->setMin(&m[1].number()); ass->setIncludeEqualsMin(false);}
							else if(m.comparisonType() == COMPARISON_LESS) {ass->setMax(&m[1].number()); ass->setIncludeEqualsMax(false);}
						}
						var->setAssumptions(ass);
						var->ref();
						vars.push_back(var);
						varms.push_back(m[0]);
						MathStructure u_var(var);
						mstruct.replace(m[0], u_var);
						return true;
					}
				}
			}
		}
	} else if(m.isLogicalAnd()) {
		bool ret = true;
		for(size_t i = 0; i < m.size(); i++) {
			if(!handle_where_expression(m[i], mstruct, eo, vars, varms)) ret = false;
		}
		return ret;
	}
	CALCULATOR->error(true, _("Unhandled \"where\" expression: %s"), format_and_print(m).c_str(), NULL);
	return false;
}
MathStructure Calculator::calculate(string str, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division) {
	
	string str2, str_where;
	
	separateWhereExpression(str, str_where, eo);
	if(make_to_division) separateToExpression(str, str2, eo, true);
	Unit *u = NULL;
	if(to_struct) {
		if(str2.empty()) {
			if(to_struct->isSymbolic() && !to_struct->symbol().empty()) {
				str2 = to_struct->symbol();
				remove_blank_ends(str2);
			} else if(to_struct->isUnit()) {
				u = to_struct->unit();
			}
		}
		to_struct->setUndefined();
	}
	
	MathStructure mstruct;
	current_stage = MESSAGE_STAGE_PARSING;
	size_t n_messages = messages.size();
	parse(&mstruct, str, eo.parse_options);
	if(parsed_struct) {
		beginTemporaryStopMessages();
		ParseOptions po = eo.parse_options;
		po.preserve_format = true;
		parse(parsed_struct, str, po);
		endTemporaryStopMessages();
	}
	
	vector<UnknownVariable*> vars;
	vector<MathStructure> varms;
	if(!str_where.empty()) {
		parseSigns(str_where);
		if(str_where.find("&&") == string::npos) {
			int par = 0;
			int bra = 0;
			for(size_t i = 0; i < str_where.length(); i++) {
				switch(str_where[i]) {
					case '(': {par++; break;}
					case ')': {if(par > 0) par--; break;}
					case '[': {bra++; break;}
					case ']': {if(bra > 0) bra--; break;}
					case COMMA_CH: {
						if(par == 0 && bra == 0) {
							str_where.replace(i, 1, LOGICAL_AND);
							i++;
						}
						break;
					}
					default: {}
				}
			}
		}
		MathStructure where_struct;
		parse(&where_struct, str_where, eo.parse_options);
		if(mstruct.isComparison() || (mstruct.isFunction() && mstruct.function() == CALCULATOR->f_solve && mstruct.size() >= 1 && mstruct[0].isComparison())) {
			beginTemporaryStopMessages();
			MathStructure mbak(mstruct);
			if(handle_where_expression(where_struct, mstruct, eo, vars, varms)) {
				endTemporaryStopMessages(true);
			} else {
				endTemporaryStopMessages();
				mstruct = mbak;
				if(mstruct.isComparison()) mstruct.transform(STRUCT_LOGICAL_AND, where_struct);
				else {mstruct[0].transform(STRUCT_LOGICAL_AND, where_struct); mstruct.childUpdated(1);}
			}
		} else {
			if(eo.approximation == APPROXIMATION_EXACT) {
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_TRY_EXACT;
				handle_where_expression(where_struct, mstruct, eo2, vars, varms);
			} else {
				handle_where_expression(where_struct, mstruct, eo, vars, varms);
			}
		}
	}
	
	current_stage = MESSAGE_STAGE_CALCULATION;

	mstruct.eval(eo);
	
	current_stage = MESSAGE_STAGE_UNSET;

	if(!aborted()) {
		bool b_units = mstruct.containsType(STRUCT_UNIT, true);
		if(b_units && u) {
			current_stage = MESSAGE_STAGE_CONVERSION;
			if(to_struct) to_struct->set(u);
			mstruct.set(convert(mstruct, u, eo, false, false));
			if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
		} else if(!str2.empty()) {
			mstruct.set(convert(mstruct, str2, eo));
		} else if(b_units) {
			current_stage = MESSAGE_STAGE_CONVERSION;
			switch(eo.auto_post_conversion) {
				case POST_CONVERSION_OPTIMAL: {
					mstruct.set(convertToOptimalUnit(mstruct, eo, false));
					break;
				}
				case POST_CONVERSION_BASE: {
					mstruct.set(convertToBaseUnits(mstruct, eo));
					break;
				}
				case POST_CONVERSION_OPTIMAL_SI: {
					mstruct.set(convertToOptimalUnit(mstruct, eo, true));
					break;
				}
				default: {}
			}
			if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
		}
	}
	
	cleanMessages(mstruct, n_messages + 1);

	current_stage = MESSAGE_STAGE_UNSET;
	
	for(size_t i = 0; i < vars.size(); i++) {
		mstruct.replace(vars[i], varms[i]);
		vars[i]->destroy();
	}

	return mstruct;

}
MathStructure Calculator::calculate(const MathStructure &mstruct_to_calculate, const EvaluationOptions &eo, string to_str) {

	remove_blank_ends(to_str);
	MathStructure mstruct(mstruct_to_calculate);
	current_stage = MESSAGE_STAGE_CALCULATION;
	size_t n_messages = messages.size();
	mstruct.eval(eo);

	current_stage = MESSAGE_STAGE_CONVERSION;
	if(!to_str.empty()) {
		mstruct.set(convert(mstruct, to_str, eo));
	} else {
		switch(eo.auto_post_conversion) {
			case POST_CONVERSION_OPTIMAL: {
				mstruct.set(convertToOptimalUnit(mstruct, eo, false));
				break;
			}
			case POST_CONVERSION_BASE: {
				mstruct.set(convertToBaseUnits(mstruct, eo));
				break;
			}
			case POST_CONVERSION_OPTIMAL_SI: {
				mstruct.set(convertToOptimalUnit(mstruct, eo, true));
				break;
			}
			default: {}
		}
		if(eo.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo));
	}

	cleanMessages(mstruct, n_messages + 1);
	
	current_stage = MESSAGE_STAGE_UNSET;
	return mstruct;
}

string Calculator::print(const MathStructure &mstruct, int msecs, const PrintOptions &po) {
	startControl(msecs);
	MathStructure mstruct2(mstruct);
	mstruct2.format(po);
	string print_result = mstruct2.print(po);
	stopControl();
	return print_result;
}
string Calculator::printMathStructureTimeOut(const MathStructure &mstruct, int msecs, const PrintOptions &po) {
	return print(mstruct, msecs, po);
}

MathStructure Calculator::convertToMixedUnits(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_NONE) return mstruct;
	if(!mstruct.isMultiplication()) return mstruct;
	if(mstruct.size() != 2) return mstruct;
	size_t n_messages = messages.size();
	if(mstruct[1].isUnit() && (!mstruct[1].prefix() || mstruct[1].prefix() == decimal_null_prefix) && mstruct[0].isNumber()) {
		Prefix *p = mstruct[1].prefix();
		MathStructure mstruct_new(mstruct);
		Unit *u = mstruct[1].unit();
		Number nr = mstruct[0].number();
		if(!nr.isReal()) return mstruct;
		if(nr.isOne()) return mstruct;
		if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) return mstruct;
		bool negated = false;
		if(nr.isNegative()) {
			nr.negate();
			negated = true;
		}
		bool accept_obsolete = (u->subtype() == SUBTYPE_ALIAS_UNIT && abs(((AliasUnit*) u)->mixWithBase()) > 1);
		Unit *original_u = u;
		Unit *last_nonobsolete_u = u;
		Number last_nonobsolete_nr = nr;
		Number nr_one(1, 1);
		Number nr_ten(10, 1);
		while(eo.mixed_units_conversion > MIXED_UNITS_CONVERSION_DOWNWARDS && nr.isGreaterThan(nr_one)) {
			Unit *best_u = NULL;
			Number best_nr;
			int best_priority = 0;
			for(size_t i = 0; i < units.size(); i++) {
				Unit *ui = units[i];				
				if(ui->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) ui)->firstBaseUnit() == u  && ((AliasUnit*) ui)->firstBaseExponent() == 1) {
					AliasUnit *aui = (AliasUnit*) ui;
					int priority_i = aui->mixWithBase();
					if(((priority_i > 0 && (!best_u || priority_i <= best_priority)) || (best_priority == 0 && priority_i == 0 && ((eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_INTEGER && aui->expression().find_first_not_of(NUMBERS) == string::npos) || eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL))) && (aui->mixWithBaseMinimum() <= 1 || nr.isGreaterThanOrEqualTo(aui->mixWithBaseMinimum()))) {
						MathStructure mstruct_nr(nr);
						MathStructure m_exp(m_one);
						aui->convertFromFirstBaseUnit(mstruct_nr, m_exp);
						mstruct_nr.eval(eo);
						if(mstruct_nr.isNumber() && m_exp.isOne() && mstruct_nr.number().isLessThan(nr) && mstruct_nr.number().isGreaterThanOrEqualTo(nr_one) && (!best_u || mstruct_nr.number().isLessThan(best_nr))) {
							best_u = ui;
							best_nr = mstruct_nr.number();
							best_priority = priority_i;
						}
					}
				}
			}
			if(!best_u) break;
			u = best_u;
			nr = best_nr;
			if(accept_obsolete || best_priority <= 1) {
				last_nonobsolete_u = u;
				last_nonobsolete_nr = nr;
			}
		}
		u = last_nonobsolete_u;
		nr = last_nonobsolete_nr;
		if(u != original_u) {
			if(negated) last_nonobsolete_nr.negate();
			mstruct_new[0].set(last_nonobsolete_nr);
			mstruct_new[1].set(u, p);
		}
		while(u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT && ((AliasUnit*) u)->firstBaseExponent() == 1 && (((AliasUnit*) u)->mixWithBase() != 0 || eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL || (eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_INTEGER && ((AliasUnit*) u)->expression().find_first_not_of(NUMBERS) == string::npos)) && !nr.isInteger() && !nr.isZero()) {
			Number int_nr(nr);
			int_nr.trunc();
			if(eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP && int_nr.isZero()) break;
			nr -= int_nr;
			MathStructure mstruct_nr(nr);
			MathStructure m_exp(m_one);
			((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
			mstruct_nr.eval(eo);
			while(!accept_obsolete && ((AliasUnit*) u)->firstBaseUnit()->subtype() == SUBTYPE_ALIAS_UNIT && abs(((AliasUnit*) ((AliasUnit*) u)->firstBaseUnit())->mixWithBase()) > 1) {
				u = ((AliasUnit*) u)->firstBaseUnit();
				if(((AliasUnit*) u)->firstBaseExponent() == 1 && (((AliasUnit*) u)->mixWithBase() != 0 || eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL || (eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_INTEGER && ((AliasUnit*) u)->expression().find_first_not_of(NUMBERS) == string::npos))) {
					((AliasUnit*) u)->convertToFirstBaseUnit(mstruct_nr, m_exp);
					mstruct_nr.eval(eo);
					if(!mstruct_nr.isNumber() || !m_exp.isOne()) break;
				} else {
					mstruct_nr.setUndefined();
					break;
				}
			}
			if(!mstruct_nr.isNumber() || !m_exp.isOne()) break;
			if(eo.mixed_units_conversion == MIXED_UNITS_CONVERSION_FORCE_ALL && mstruct_nr.number().isLessThanOrEqualTo(nr)) break;
			u = ((AliasUnit*) u)->firstBaseUnit();
			nr = mstruct_nr.number();
			MathStructure mstruct_term;
			if(negated) {
				Number pos_nr(nr);
				pos_nr.negate();
				mstruct_term.set(pos_nr);
			} else {
				mstruct_term.set(nr);
			}
			mstruct_term *= MathStructure(u, p);
			if(int_nr.isZero()) {
				if(mstruct_new.isAddition()) mstruct_new[mstruct_new.size() - 1].set(mstruct_term);
				else mstruct_new.set(mstruct_term);
			} else {
				if(negated) int_nr.negate();
				if(mstruct_new.isAddition()) mstruct_new[mstruct_new.size() - 1][0].set(int_nr);
				else mstruct_new[0].set(int_nr);
				mstruct_new.add(mstruct_term, true);
			}
		}
		cleanMessages(mstruct_new, n_messages + 1);
		return mstruct_new;
	}
	return mstruct;
}

MathStructure Calculator::convert(double value, Unit *from_unit, Unit *to_unit, const EvaluationOptions &eo) {
	size_t n_messages = messages.size();
	MathStructure mstruct(value);
	mstruct *= from_unit;
	mstruct.eval(eo);
	if(eo.approximation == APPROXIMATION_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_TRY_EXACT;
		mstruct.convert(to_unit, true, NULL, false, eo2);
	} else {
		mstruct.convert(to_unit, true, NULL, false, eo);
	}
	mstruct.divide(to_unit, true);
	mstruct.eval(eo);
	cleanMessages(mstruct, n_messages + 1);
	return mstruct;

}
MathStructure Calculator::convert(string str, Unit *from_unit, Unit *to_unit, int msecs, const EvaluationOptions &eo) {
	return convertTimeOut(str, from_unit, to_unit, msecs, eo);
}
MathStructure Calculator::convertTimeOut(string str, Unit *from_unit, Unit *to_unit, int msecs, const EvaluationOptions &eo) {
	MathStructure mstruct;
	parse(&mstruct, str, eo.parse_options);
	mstruct *= from_unit;
	b_busy = true;
	if(!calculate_thread->running && !calculate_thread->start()) return mstruct;
	bool had_msecs = msecs > 0;
	tmp_evaluationoptions = eo;
	tmp_proc_command = PROC_NO_COMMAND;
	bool b_parse = false;
	if(!calculate_thread->write(b_parse)) {calculate_thread->cancel(); return mstruct;}
	void *x = (void*) &mstruct;
	if(!calculate_thread->write(x)) {calculate_thread->cancel(); return mstruct;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}	
	if(had_msecs && b_busy) {
		abort();
		mstruct.setAborted();
		return mstruct;
	}
	if(eo.approximation == APPROXIMATION_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_TRY_EXACT;
		mstruct.convert(to_unit, true, NULL, false, eo2);
	} else {
		mstruct.convert(to_unit, true, NULL, false, eo);
	}
	mstruct.divide(to_unit, true);
	b_busy = true;
	if(!calculate_thread->write(b_parse)) {calculate_thread->cancel(); return mstruct;}
	x = (void*) &mstruct;
	if(!calculate_thread->write(x)) {calculate_thread->cancel(); return mstruct;}
	while(msecs > 0 && b_busy) {
		sleep_ms(10);
		msecs -= 10;
	}	
	if(had_msecs && b_busy) {
		abort();
		mstruct.setAborted();
	}
	return mstruct;
}
MathStructure Calculator::convert(string str, Unit *from_unit, Unit *to_unit, const EvaluationOptions &eo) {
	size_t n_messages = messages.size();
	MathStructure mstruct;
	parse(&mstruct, str, eo.parse_options);
	mstruct *= from_unit;
	mstruct.eval(eo);
	if(eo.approximation == APPROXIMATION_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_TRY_EXACT;
		mstruct.convert(to_unit, true, NULL, false, eo2);
	} else {
		mstruct.convert(to_unit, true, NULL, false, eo);
	}
	mstruct.divide(to_unit, true);
	mstruct.eval(eo);
	cleanMessages(mstruct, n_messages + 1);
	return mstruct;
}
MathStructure Calculator::convert(const MathStructure &mstruct, KnownVariable *to_var, const EvaluationOptions &eo) {
	if(mstruct.contains(to_var, true) > 0) return mstruct;
	size_t n_messages = messages.size();
	if(b_var_units && !to_var->unit().empty() && to_var->isExpression()) {
		CompositeUnit cu("", "temporary_composite_convert", "", to_var->unit());
		if(cu.countUnits() > 0) {
			AliasUnit au("", "temporary_alias_convert", "", "", "", &cu, to_var->expression());
			bool unc_rel = false;
			if(!to_var->uncertainty(&unc_rel).empty()) au.setUncertainty(to_var->uncertainty(), unc_rel);
			au.setApproximate(to_var->isApproximate());
			au.setPrecision(to_var->precision());
			MathStructure mstruct_new(convert(mstruct, &au, eo, false, false));
			if(mstruct_new.contains(&au)) {
				mstruct_new.replace(&au, to_var);
				return mstruct_new;
			}
		}
	}
	MathStructure mstruct_new(mstruct);
	mstruct_new /= to_var->get();
	mstruct_new.eval(eo);
	mstruct_new *= to_var;
	cleanMessages(mstruct, n_messages + 1);
	return mstruct_new;
}
MathStructure Calculator::convert(const MathStructure &mstruct, Unit *to_unit, const EvaluationOptions &eo, bool always_convert, bool convert_to_mixed_units) {
	if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
	CompositeUnit *cu = NULL;
	if(to_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) cu = (CompositeUnit*) to_unit;
	if(cu && cu->countUnits() == 0) return mstruct;
	MathStructure mstruct_new(mstruct);
	size_t n_messages = messages.size();
	if(to_unit->hasNonlinearRelationTo(to_unit->baseUnit()) && to_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		mstruct_new = convert(mstruct, to_unit->baseUnit(), eo, always_convert, convert_to_mixed_units);
		mstruct_new.calculateDivide(((CompositeUnit*) to_unit->baseUnit())->generateMathStructure(false, eo.keep_prefixes), eo);
		to_unit->convertFromBaseUnit(mstruct_new);
		mstruct_new.eval(eo);
		mstruct_new.multiply(MathStructure(to_unit, eo.keep_prefixes ? decimal_null_prefix : NULL));
		EvaluationOptions eo2 = eo;
		eo2.sync_units = false;
		eo2.keep_prefixes = true;
		mstruct_new.eval(eo2);
		cleanMessages(mstruct, n_messages + 1);
		return mstruct_new;
	}
	//bool b_simple = !cu && (to_unit->subtype() != SUBTYPE_ALIAS_UNIT || (((AliasUnit*) to_unit)->baseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT && ((AliasUnit*) to_unit)->baseExponent() == 1));

	bool b_changed = false;
	if(mstruct_new.isAddition()) {
		if(mstruct_new.size() > 100 && aborted()) return mstruct;
		mstruct_new.factorizeUnits();
		if(!b_changed && !mstruct_new.equals(mstruct, true, true)) b_changed = true;
	}

	if(!mstruct_new.isPower() && !mstruct_new.isUnit() && !mstruct_new.isMultiplication()) {
		if(mstruct_new.size() > 0) {
			for(size_t i = 0; i < mstruct_new.size(); i++) {
				if(mstruct_new.size() > 100 && aborted()) return mstruct;
				if(!mstruct_new.isFunction() || !mstruct_new.function()->getArgumentDefinition(i + 1) || mstruct_new.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) { 
					mstruct_new[i] = convert(mstruct_new[i], to_unit, eo, false, convert_to_mixed_units);
					if(!b_changed && !mstruct_new.equals(mstruct[i], true, true)) b_changed = true;
				}
			}
			if(b_changed) {
				mstruct_new.childrenUpdated();
				EvaluationOptions eo2 = eo;
				//eo2.calculate_functions = false;
				eo2.sync_units = false;
				eo2.keep_prefixes = true;
				mstruct_new.eval(eo2);
				cleanMessages(mstruct, n_messages + 1);
			}
			return mstruct_new;
		}
	} else {
		EvaluationOptions eo2 = eo;
		eo2.keep_prefixes = true;
		bool b = false;
		if(eo.approximation == APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_TRY_EXACT;
		if(mstruct_new.convert(to_unit, true, NULL, false, eo2, eo.keep_prefixes ? decimal_null_prefix : NULL) || always_convert) {
			b = true;
		} else {
			CompositeUnit *cu2 = cu;
			if(to_unit->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) to_unit)->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				cu2 = (CompositeUnit*) ((AliasUnit*) to_unit)->baseUnit();
			}
			if(cu2) {
				switch(mstruct_new.type()) {
					case STRUCT_UNIT: {
						if(cu2->containsRelativeTo(mstruct_new.unit())) {
							b = true;
						}
						break;
					} 
					case STRUCT_MULTIPLICATION: {
						for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
							if(mstruct_new.countChildren() > 100 && aborted()) return mstruct;
							if(mstruct_new.getChild(i)->isUnit() && cu2->containsRelativeTo(mstruct_new.getChild(i)->unit())) {
								b = true;
							}
							if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && cu2->containsRelativeTo(mstruct_new.getChild(i)->base()->unit())) {
								b = true;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mstruct_new.base()->isUnit() && cu2->containsRelativeTo(mstruct_new.base()->unit())) {
							b = true;
						}
						break;				
					}
					default: {}
				}
			}
		}
		if(b) {
			eo2.approximation = eo.approximation;
			eo2.sync_units = true;
			eo2.keep_prefixes = false;
			MathStructure mbak(mstruct_new);
			mstruct_new.divide(MathStructure(to_unit, NULL));
			mstruct_new.eval(eo2);
			if(mstruct_new.containsType(STRUCT_UNIT)) {
				mbak.inverse();
				mbak.divide(MathStructure(to_unit, NULL));
				mbak.eval(eo2);
				if(!mbak.containsType(STRUCT_UNIT)) mstruct_new = mbak;
			}

			if(cu) {
				MathStructure mstruct_cu(cu->generateMathStructure(false, eo.keep_prefixes));
				Prefix *p = NULL;
				size_t i = 1;
				Unit *u = cu->get(i, NULL, &p);
				while(u) {
					mstruct_new.setPrefixForUnit(u, p);
					i++;
					u = cu->get(i, NULL, &p);
				}
				mstruct_new.multiply(mstruct_cu);
			} else {
				mstruct_new.multiply(MathStructure(to_unit, eo.keep_prefixes ? decimal_null_prefix : NULL));
			}

			eo2.sync_units = false;
			eo2.keep_prefixes = true;
			mstruct_new.eval(eo2);
			
			cleanMessages(mstruct, n_messages + 1);

			if(convert_to_mixed_units && eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) {
				eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP;
				return convertToMixedUnits(mstruct_new, eo2);
			} else {
				return mstruct_new;
			}
		}
	}

	return mstruct;

}
MathStructure Calculator::convertToBaseUnits(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
	size_t n_messages = messages.size();
	MathStructure mstruct_new(mstruct);
	mstruct_new.convertToBaseUnits(true, NULL, true, eo);
	if(!mstruct_new.equals(mstruct, true, true)) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = eo.approximation;
		eo2.keep_prefixes = false;
		eo2.isolate_x = false;
		eo2.test_comparisons = false;
		//eo2.calculate_functions = false;
		mstruct_new.eval(eo2);
		cleanMessages(mstruct, n_messages + 1);
	}
	return mstruct_new;
}
Unit *Calculator::findMatchingUnit(const MathStructure &mstruct) {
	switch(mstruct.type()) {
		case STRUCT_POWER: {
			if(mstruct.base()->isUnit() && mstruct.exponent()->isNumber() && mstruct.exponent()->number().isInteger() && mstruct.exponent()->number() < 10 && mstruct.exponent()->number() > -10) {
				Unit *u_base = mstruct.base()->unit();
				int exp = mstruct.exponent()->number().intValue();
				if(u_base->subtype() == SUBTYPE_ALIAS_UNIT) {
					u_base = u_base->baseUnit();
					exp *= ((AliasUnit*) u_base)->baseExponent();
				}
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->baseUnit() == u_base && ((AliasUnit*) u)->baseExponent() == exp) {
						return u;
					}
				}
				CompositeUnit *cu = new CompositeUnit("", "temporary_find_matching_unit");
				cu->add(u_base, exp);
				Unit *u = getOptimalUnit(cu);
				if(u != cu && !u->isRegistered()) {
					delete u;
				} else if(u != cu) {
					MathStructure mtest(mstruct);
					mtest.divide(u);
					mtest.eval();
					if(mtest.isNumber()) {
						delete cu;
						return u;
					}
				}
				delete cu;
			}
			return findMatchingUnit(mstruct[0]);
		}
		case STRUCT_UNIT: {
			return mstruct.unit();
		}
		case STRUCT_MULTIPLICATION: {
			if(mstruct.size() == 2 && !mstruct[0].isUnit_exp()) {
				return findMatchingUnit(mstruct[1]);
			}
			CompositeUnit *cu = new CompositeUnit("", "temporary_find_matching_unit");
			for(size_t i = 1; i <= mstruct.countChildren(); i++) {
				if(mstruct.getChild(i)->isUnit()) {
					cu->add(mstruct.getChild(i)->unit()->baseUnit());
				} else if(mstruct.getChild(i)->isPower() && mstruct.getChild(i)->base()->isUnit() && mstruct.getChild(i)->exponent()->isNumber() && mstruct.getChild(i)->exponent()->number().isInteger()) {
					cu->add(mstruct.getChild(i)->base()->unit()->baseUnit(), mstruct.getChild(i)->exponent()->number().intValue());
				}
			}
			if(cu->countUnits() == 1) {
				int exp = 1;
				Unit *u_base = cu->get(1, &exp);
				if(exp == 1) return u_base;
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_ALIAS_UNIT && u->baseUnit() == u_base && ((AliasUnit*) u)->baseExponent() == exp) {
						return u;
					}
				}
			}
			if(cu->countUnits() > 1) {
				for(size_t i = 0; i < units.size(); i++) {
					Unit *u = units[i];
					if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						if(((CompositeUnit*) u)->countUnits() == cu->countUnits()) {
							bool b = true;
							for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
								int exp1 = 1, exp2 = 1;
								Unit *ui1 = cu->get(i2, &exp1);
								b = false;
								for(size_t i3 = 1; i3 <= cu->countUnits(); i3++) {
									Unit *ui2 = ((CompositeUnit*) u)->get(i3, &exp2);
									if(ui1 == ui2->baseUnit()) {
										b = (exp1 == exp2);
										break;
									}
								}
								if(!b) break;
							}
							if(b) {
								delete cu;
								return u;
							}
						}
					}
				}
			}
			Unit *u = getOptimalUnit(cu);
			if(u != cu && !u->isRegistered()) {
				if(cu->countUnits() > 1 && u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					MathStructure m_u = ((CompositeUnit*) u)->generateMathStructure();
					if(m_u != cu->generateMathStructure()) {
						Unit *u2 = findMatchingUnit(m_u);
						if(u2) {
							MathStructure mtest(mstruct);
							mtest.divide(u2);
							mtest.eval();
							if(mtest.isNumber()) {
								delete cu;
								delete u;
								return u2;
							}
						}
					}
				}
				delete u;
			} else if(u != cu) {
				MathStructure mtest(mstruct);
				mtest.divide(u);
				mtest.eval();
				if(mtest.isNumber()) {
					delete cu;
					return u;
				}
			}
			delete cu;
			break;
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct.size() > 100 && aborted()) return NULL;
				if(!mstruct.isFunction() || !mstruct.function()->getArgumentDefinition(i + 1) || mstruct.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) { 
					Unit *u = findMatchingUnit(mstruct[i]);
					if(u) return u;
				}
			}
			break;
		}
	}
	return NULL;
}
Unit *Calculator::getBestUnit(Unit *u, bool allow_only_div, bool convert_to_local_currency) {return getOptimalUnit(u, allow_only_div, convert_to_local_currency);}
Unit *Calculator::getOptimalUnit(Unit *u, bool allow_only_div, bool convert_to_local_currency) {
	switch(u->subtype()) {
		case SUBTYPE_BASE_UNIT: {
			if(convert_to_local_currency && u->isCurrency()) {
				Unit *u_local_currency = getLocalCurrency();
				if(u_local_currency) return u_local_currency;
			}
			return u;
		}
		case SUBTYPE_ALIAS_UNIT: {
			AliasUnit *au = (AliasUnit*) u;
			if(au->baseExponent() == 1 && au->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
				if(au->isCurrency()) {
					if(!convert_to_local_currency) return u;
					Unit *u_local_currency = getLocalCurrency();
					if(u_local_currency) return u_local_currency;
				}
				return (Unit*) au->baseUnit();
			} else if(au->isSIUnit() && (au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT || au->firstBaseExponent() != 1)) {
				return u;
			} else {
				return getOptimalUnit((Unit*) au->firstBaseUnit());
			}
		}
		case SUBTYPE_COMPOSITE_UNIT: {
			CompositeUnit *cu = (CompositeUnit*) u;
			int exp, b_exp;
			int points = 0;
			bool minus = false;
			bool has_positive = false;
			int new_points;
			int new_points_m;
			int max_points = 0;
			for(size_t i = 1; i <= cu->countUnits(); i++) {
				cu->get(i, &exp);
				if(exp < 0) {
					max_points -= exp;
				} else {
					max_points += exp;
					has_positive = true;
				}
			}
			for(size_t i = 0; i < units.size(); i++) {
				if(units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					CompositeUnit *cu2 = (CompositeUnit*) units[i];
					if(cu == cu2 && !cu2->isHidden()) {
						points = max_points - 1;
					} else if(!cu2->isHidden() && cu2->isSIUnit() && cu2->countUnits() == cu->countUnits()) {
						bool b_match = true;
						for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
							int exp2;
							if(cu->get(i2, &exp) != cu2->get(i2, &exp2) || exp != exp2) {
								b_match = false;
								break;
							}
						}
						if(b_match) {
							points = max_points - 1;
							break;
						}
					}
				}
			}
			Unit *best_u = NULL;
			Unit *bu, *u2;
			AliasUnit *au;
			for(size_t i = 0; i < units.size(); i++) {
				u2 = units[i];
				if(u2->subtype() == SUBTYPE_BASE_UNIT && (points == 0 || (points == 1 && minus))) {
					for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
						if(cu->get(i2, &exp)->baseUnit() == u2 && !cu->get(i2)->hasNonlinearRelationTo(u2)) {
							points = 1;
							best_u = u2;
							minus = !has_positive && (exp < 0);
							break;
						}
					}
				} else if(!u2->isSIUnit()) {
				} else if(u2->subtype() == SUBTYPE_ALIAS_UNIT) {
					au = (AliasUnit*) u2;
					bu = (Unit*) au->baseUnit();
					b_exp = au->baseExponent();
					new_points = 0;
					new_points_m = 0;
					if((b_exp != 1 || bu->subtype() == SUBTYPE_COMPOSITE_UNIT) && !au->hasNonlinearRelationTo(bu)) {
						if(bu->subtype() == SUBTYPE_BASE_UNIT) {
							for(size_t i2 = 1; i2 <= cu->countUnits(); i2++) {
								if(cu->get(i2, &exp) == bu) {
									bool m = false;
									if(b_exp < 0 && exp < 0) {
										b_exp = -b_exp;
										exp = -exp;
									} else if(b_exp < 0) {
										b_exp = -b_exp;	
										m = true;
									} else if(exp < 0) {
										exp = -exp;
										m = true;
									}
									new_points = exp - b_exp;
									if(new_points < 0) {
										new_points = -new_points;
									}
									new_points = exp - new_points;
									if(!allow_only_div && m && new_points >= max_points) {
										new_points = -1;
									}
									if(new_points > points || (!m && minus && new_points == points)) {
										points = new_points;
										minus = m;
										best_u = au;
									}
									break;
								}
							}
						} else if(au->firstBaseExponent() != 1 || au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
							MathStructure cu_mstruct = ((CompositeUnit*) bu)->generateMathStructure();
							if(b_exp != 1) {
								if(cu_mstruct.isMultiplication()) {
									for(size_t i2 = 0; i2 < cu_mstruct.size(); i2++) {
										if(cu_mstruct[i2].isPower()) cu_mstruct[i2][1].number() *= b_exp;
										else cu_mstruct[i2].raise(b_exp);
									}
								} else if(cu_mstruct.isPower()) {
									cu_mstruct[1].number() *= b_exp;
								} else {
									cu_mstruct.raise(b_exp);
								}
							}
							cu_mstruct = convertToBaseUnits(cu_mstruct);
							if(cu_mstruct.isMultiplication()) {
								for(size_t i2 = 1; i2 <= cu_mstruct.countChildren(); i2++) {
									bu = NULL;
									if(cu_mstruct.getChild(i2)->isUnit()) {
										bu = cu_mstruct.getChild(i2)->unit();
										b_exp = 1;
									} else if(cu_mstruct.getChild(i2)->isPower() && cu_mstruct.getChild(i2)->base()->isUnit() && cu_mstruct.getChild(i2)->exponent()->isNumber() && cu_mstruct.getChild(i2)->exponent()->number().isInteger()) {
										bu = cu_mstruct.getChild(i2)->base()->unit();
										b_exp = cu_mstruct.getChild(i2)->exponent()->number().intValue();
									}
									if(bu) {
										bool b = false;
										for(size_t i3 = 1; i3 <= cu->countUnits(); i3++) {
											if(cu->get(i3, &exp) == bu) {
												b = true;
												bool m = false;
												if(exp < 0 && b_exp > 0) {
													new_points -= b_exp;
													exp = -exp;
													m = true;
												} else if(exp > 0 && b_exp < 0) {
													new_points += b_exp;
													b_exp = -b_exp;
													m = true;
												} else {
													if(b_exp < 0) new_points_m += b_exp;
													else new_points_m -= b_exp;
												}
												if(exp < 0) {
													exp = -exp;
													b_exp = -b_exp;
												}
												if(exp >= b_exp) {
													if(m) new_points_m += exp - (exp - b_exp);
													else new_points += exp - (exp - b_exp);
												} else {
													if(m) new_points_m += exp - (b_exp - exp);
													else new_points += exp - (b_exp - exp);
												}
												break;
											}
										}
										if(!b) {
											if(b_exp < 0) b_exp = -b_exp;
											new_points -= b_exp;	
											new_points_m -= b_exp;
										}
									}
								}
								if(!allow_only_div && new_points_m >= max_points) {
									new_points_m = -1;
								}
								if(new_points > points && new_points >= new_points_m) {
									minus = false;
									points = new_points;
									best_u = au;
								} else if(new_points_m > points || (new_points_m == points && minus)) {
									minus = true;
									points = new_points_m;
									best_u = au;
								}
							}
						}
					}
				}
				if(points >= max_points && !minus) break;
			}
			if(!best_u) return u;
			best_u = getOptimalUnit(best_u, false, convert_to_local_currency);
			if(points > 1 && points < max_points - 1) {
				CompositeUnit *cu_new = new CompositeUnit("", "temporary_composite_convert");
				bool return_cu = minus;
				if(minus) {
					cu_new->add(best_u, -1);
				} else {
					cu_new->add(best_u);
				}
				MathStructure cu_mstruct = ((CompositeUnit*) u)->generateMathStructure();
				if(minus) cu_mstruct *= best_u;
				else cu_mstruct /= best_u;
				cu_mstruct = convertToBaseUnits(cu_mstruct);
				CompositeUnit *cu2 = new CompositeUnit("", "temporary_composite_convert_to_optimal_unit");
				bool b = false;
				for(size_t i = 1; i <= cu_mstruct.countChildren(); i++) {
					if(cu_mstruct.getChild(i)->isUnit()) {
						b = true;
						cu2->add(cu_mstruct.getChild(i)->unit());
					} else if(cu_mstruct.getChild(i)->isPower() && cu_mstruct.getChild(i)->base()->isUnit() && cu_mstruct.getChild(i)->exponent()->isNumber() && cu_mstruct.getChild(i)->exponent()->number().isInteger()) {
						if(cu_mstruct.getChild(i)->exponent()->number().isGreaterThan(10) || cu_mstruct.getChild(i)->exponent()->number().isLessThan(-10)) {
							if(aborted() || cu_mstruct.getChild(i)->exponent()->number().isGreaterThan(1000) || cu_mstruct.getChild(i)->exponent()->number().isLessThan(-1000)) {
								b = false;
								break;
							}
						}
						b = true;
						cu2->add(cu_mstruct.getChild(i)->base()->unit(), cu_mstruct.getChild(i)->exponent()->number().intValue());
					}
				}
				if(b) {
					Unit *u2 = getOptimalUnit(cu2, true, convert_to_local_currency);
					b = false;
					if(u2->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						for(size_t i3 = 1; i3 <= ((CompositeUnit*) u2)->countUnits(); i3++) {
							Unit *cu_unit = ((CompositeUnit*) u2)->get(i3, &exp);
							for(size_t i4 = 1; i4 <= cu_new->countUnits(); i4++) {
								if(cu_new->get(i4, &b_exp) == cu_unit) {
									b = true;
									cu_new->setExponent(i4, b_exp + exp);
									break;
								}
							}
							if(!b) cu_new->add(cu_unit, exp);
						}
						return_cu = true;						
					} else if(u2->subtype() == SUBTYPE_ALIAS_UNIT) {
						return_cu = true;
						for(size_t i3 = 1; i3 <= cu_new->countUnits(); i3++) {
							if(cu_new->get(i3, &exp) == u2) {
								b = true;
								cu_new->setExponent(i3, exp + 1);
								break;
							}
						}
						if(!b) cu_new->add(u2);
					}
					if(!u2->isRegistered() && u2 != cu2) delete u2;
				}
				delete cu2;
				if(return_cu) {
					return cu_new;
				} else {
					delete cu_new;
					return best_u;
				}
			}
			if(minus) {
				CompositeUnit *cu_new = new CompositeUnit("", "temporary_composite_convert");
				cu_new->add(best_u, -1);
				return cu_new;
			} else {
				return best_u;
			}
		}
	}
	return u;
}
MathStructure Calculator::convertToBestUnit(const MathStructure &mstruct, const EvaluationOptions &eo, bool convert_to_si_units) {return convertToOptimalUnit(mstruct, eo, convert_to_si_units);}
MathStructure Calculator::convertToOptimalUnit(const MathStructure &mstruct, const EvaluationOptions &eo, bool convert_to_si_units) {
	EvaluationOptions eo2 = eo;
	//eo2.calculate_functions = false;
	eo2.sync_units = false;
	eo2.isolate_x = false;
	eo2.test_comparisons = false;
	switch(mstruct.type()) {
		case STRUCT_POWER: {
			if(mstruct.base()->isUnit() && mstruct.exponent()->isNumber() && mstruct.exponent()->number().isRational() && !mstruct.exponent()->number().isZero()) {
				MathStructure mstruct_new(mstruct);
				int old_points = 0;
				bool overflow = false;
				if(mstruct_new.exponent()->isInteger()) old_points = mstruct_new.exponent()->number().intValue(&overflow);
				else old_points = mstruct_new.exponent()->number().numerator().intValue(&overflow) + mstruct_new.exponent()->number().denominator().intValue() * (mstruct_new.exponent()->number().isNegative() ? -1 : 1);
				if(overflow) return mstruct_new;
				bool old_minus = false;
				if(old_points < 0) {
					old_points = -old_points;
					old_minus = true;
				}
				bool is_si_units = mstruct_new.base()->unit()->isSIUnit();
				if(mstruct_new.base()->unit()->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					mstruct_new.convertToBaseUnits(true, NULL, true, eo2, true);
					if(mstruct_new.equals(mstruct, true, true)) {
						return mstruct_new;
					} else {
						mstruct_new.eval(eo2);
					}
					mstruct_new = convertToOptimalUnit(mstruct_new, eo, convert_to_si_units);
					if(mstruct_new.equals(mstruct, true, true)) return mstruct_new;
				} else {
					CompositeUnit *cu = new CompositeUnit("", "temporary_composite_convert_to_optimal_unit");
					cu->add(mstruct_new.base()->unit(), mstruct_new.exponent()->number().numerator().intValue());
					Unit *u = getOptimalUnit(cu, false, eo.local_currency_conversion);
					if(u == cu) {
						delete cu;
						return mstruct_new;
					}
					if(eo.approximation == APPROXIMATION_EXACT && cu->hasApproximateRelationTo(u, true)) {
						if(!u->isRegistered()) delete u;
						delete cu;
						return mstruct_new;
					}
					delete cu;
					mstruct_new = convert(mstruct_new, u, eo, true);
					if(!u->isRegistered()) delete u;
				}
				int new_points = 0;
				bool new_is_si_units = true;
				bool new_minus = true;
				bool is_currency = false;
				if(mstruct_new.isMultiplication()) {
					for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
						if(mstruct_new.getChild(i)->isUnit()) {
							if(new_is_si_units && !mstruct_new.getChild(i)->unit()->isSIUnit()) new_is_si_units = false;
							is_currency = mstruct_new.getChild(i)->unit()->isCurrency();
							new_points++;
							new_minus = false;
						} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isRational()) {
							int points = 0;
							if(mstruct_new.getChild(i)->exponent()->isInteger()) points = mstruct_new.getChild(i)->exponent()->number().intValue();
							else points = mstruct_new.getChild(i)->exponent()->number().numerator().intValue() + mstruct_new.getChild(i)->exponent()->number().denominator().intValue() * (mstruct_new.getChild(i)->exponent()->number().isNegative() ? -1 : 1);
							if(new_is_si_units && !mstruct_new.getChild(i)->base()->unit()->isSIUnit()) new_is_si_units = false;
							is_currency = mstruct_new.getChild(i)->base()->unit()->isCurrency();
							if(points < 0) {
								new_points -= points;
							} else {
								new_points += points;
								new_minus = false;
							}

						}
					}
				} else if(mstruct_new.isPower() && mstruct_new.base()->isUnit() && mstruct_new.exponent()->isNumber() && mstruct_new.exponent()->number().isRational()) {
					int points = 0;
					if(mstruct_new.exponent()->isInteger()) points = mstruct_new.exponent()->number().intValue();
					else points = mstruct_new.exponent()->number().numerator().intValue() + mstruct_new.exponent()->number().denominator().intValue() * (mstruct_new.exponent()->number().isNegative() ? -1 : 1);
					if(new_is_si_units && !mstruct_new.base()->unit()->isSIUnit()) new_is_si_units = false;
					is_currency = mstruct_new.base()->unit()->isCurrency();
					if(points < 0) {
						new_points = -points;
					} else {
						new_points = points;
						new_minus = false;
					}
				} else if(mstruct_new.isUnit()) {
					if(!mstruct_new.unit()->isSIUnit()) new_is_si_units = false;
					is_currency = mstruct_new.unit()->isCurrency();
					new_points = 1;
					new_minus = false;
				}
				if(new_points == 0) return mstruct;
				if((new_points > old_points && (!convert_to_si_units || is_si_units || !new_is_si_units)) || (new_points == old_points && (new_minus || !old_minus) && (!is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || !new_is_si_units))) return mstruct;
				return mstruct_new;
			}
		}
		case STRUCT_BITWISE_XOR: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_NOT: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_COMPARISON: {}
		case STRUCT_FUNCTION: {}
		case STRUCT_VECTOR: {}
		case STRUCT_ADDITION: {
			if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
			MathStructure mstruct_new(mstruct);
			bool b = false;
			for(size_t i = 0; i < mstruct_new.size(); i++) {
				if(aborted()) return mstruct;
				if(!mstruct_new.isFunction() || !mstruct_new.function()->getArgumentDefinition(i + 1) || mstruct_new.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) { 
					mstruct_new[i] = convertToOptimalUnit(mstruct_new[i], eo, convert_to_si_units);
					if(!b && !mstruct_new[i].equals(mstruct[i], true, true)) b = true;
				}
			}
			if(b) {
				mstruct_new.childrenUpdated();
				if(mstruct.isAddition()) mstruct_new.eval(eo2);
			}
			return mstruct_new;
		}
		case STRUCT_UNIT: {
			if((!mstruct.unit()->isCurrency() || !eo.local_currency_conversion) && (!convert_to_si_units || mstruct.unit()->isSIUnit())) return mstruct;
			Unit *u = getOptimalUnit(mstruct.unit(), false, eo.local_currency_conversion);
			if(u != mstruct.unit()) {
				if((u->isSIUnit() || (u->isCurrency() && eo.local_currency_conversion)) && (eo.approximation != APPROXIMATION_EXACT || !mstruct.unit()->hasApproximateRelationTo(u, true))) {
					MathStructure mstruct_new = convert(mstruct, u, eo, true);
					if(!u->isRegistered()) delete u;
					return mstruct_new;
				}
				if(!u->isRegistered()) delete u;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(!mstruct.containsType(STRUCT_UNIT, true)) return mstruct;
			int old_points = 0;
			bool old_minus = true;
			bool is_si_units = true;
			bool is_currency = false;
			bool child_updated = false;
			MathStructure mstruct_old(mstruct);
			for(size_t i = 1; i <= mstruct_old.countChildren(); i++) {
				if(mstruct_old.countChildren() > 100 && aborted()) return mstruct_old;
				if(mstruct_old.getChild(i)->isUnit()) {
					if(is_si_units && !mstruct_old.getChild(i)->unit()->isSIUnit()) is_si_units = false;
					is_currency = mstruct_old.getChild(i)->unit()->isCurrency();
					old_points++;
					old_minus = false;
				} else if(mstruct_old.getChild(i)->isPower() && mstruct_old.getChild(i)->base()->isUnit() && mstruct_old.getChild(i)->exponent()->isNumber() && mstruct_old.getChild(i)->exponent()->number().isRational()) {
					int points = 0;
					if(mstruct_old.getChild(i)->exponent()->number().isInteger()) points = mstruct_old.getChild(i)->exponent()->number().intValue();
					else points = mstruct_old.getChild(i)->exponent()->number().numerator().intValue() + mstruct_old.getChild(i)->exponent()->number().denominator().intValue() * (mstruct_old.getChild(i)->exponent()->number().isNegative() ? -1 : 1);;
					if(is_si_units && !mstruct_old.getChild(i)->base()->unit()->isSIUnit()) is_si_units = false;
						is_currency = mstruct_old.getChild(i)->base()->unit()->isCurrency();
					if(points < 0) {
						old_points -= points;
					} else {
						old_points += points;
						old_minus = false;
					}
				} else if(mstruct_old.getChild(i)->size() > 0 && !aborted()) {
					mstruct_old[i - 1] = convertToOptimalUnit(mstruct_old[i - 1], eo, convert_to_si_units);
					mstruct_old.childUpdated(i);
					if(!mstruct_old[i - 1].equals(mstruct[i - 1], true, true)) child_updated = true;
				}
			}
			if(child_updated) mstruct_old.eval(eo2);
			if((!is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || is_si_units) && old_points <= 1 && !old_minus) {
				return mstruct_old;
			}
			MathStructure mstruct_new(mstruct_old);
			mstruct_new.convertToBaseUnits(true, NULL, true, eo2, true);
			if(!mstruct_new.equals(mstruct, true, true)) {
				mstruct_new.eval(eo2);
			}
			if(mstruct_new.type() != STRUCT_MULTIPLICATION) {
				if(!mstruct_new.containsInterval(true, true, false, 1, true) && !aborted()) mstruct_new = convertToOptimalUnit(mstruct_new, eo, convert_to_si_units);
			} else {
				CompositeUnit *cu = new CompositeUnit("", "temporary_composite_convert_to_optimal_unit");
				bool b = false;
				child_updated = false;
				for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
					if(mstruct_new.countChildren() > 100 && aborted()) return mstruct_old;
					if(mstruct_new.getChild(i)->isUnit()) {
						b = true;
						cu->add(mstruct_new.getChild(i)->unit());
					} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isInteger()) {
						b = true;
						cu->add(mstruct_new.getChild(i)->base()->unit(), mstruct_new.getChild(i)->exponent()->number().intValue());
					} else if(mstruct_new.getChild(i)->size() > 0 && !mstruct_new.getChild(i)->containsInterval(true, true, false, 1, true) && !aborted()) {
						MathStructure m_i_old(mstruct_new[i - 1]);
						mstruct_new[i - 1] = convertToOptimalUnit(mstruct_new[i - 1], eo, convert_to_si_units);
						mstruct_new.childUpdated(i);
						if(!mstruct_new[i - 1].equals(m_i_old, true, true)) child_updated = true;
					}
				}
				bool is_converted = false;
				if(b) {
					Unit *u = getOptimalUnit(cu, false, eo.local_currency_conversion);
					if(u != cu) {
						if(eo.approximation != APPROXIMATION_EXACT || !cu->hasApproximateRelationTo(u, true)) {
							mstruct_new = convert(mstruct_new, u, eo, true);
							is_converted = true;
						}
						if(!u->isRegistered()) delete u;
					}
				}
				delete cu;
				if((!b || !is_converted) && (!convert_to_si_units || is_si_units)) {
					return mstruct_old;
				}
				if(child_updated) mstruct_new.eval(eo2);
			}
			if((eo.approximation == APPROXIMATION_EXACT && !mstruct_old.isApproximate()) && (mstruct_new.isApproximate() || (mstruct_old.containsInterval(true, true, false, 0, true) <= 0 && mstruct_new.containsInterval(true, true, false, 0, true) > 0))) return mstruct_old;
			if(mstruct_new.equals(mstruct_old, true, true)) return mstruct_old;
			int new_points = 0;
			bool new_minus = true;
			bool new_is_si_units = true;
			bool new_is_currency = false;
			if(mstruct_new.isMultiplication()) {
				for(size_t i = 1; i <= mstruct_new.countChildren(); i++) {
					if(mstruct_new.countChildren() > 100 && aborted()) return mstruct_old;
					if(mstruct_new.getChild(i)->isUnit()) {
						if(new_is_si_units && !mstruct_new.getChild(i)->unit()->isSIUnit()) new_is_si_units = false;
						new_is_currency = mstruct_new.getChild(i)->unit()->isCurrency();
						new_points++;
						new_minus = false;
					} else if(mstruct_new.getChild(i)->isPower() && mstruct_new.getChild(i)->base()->isUnit() && mstruct_new.getChild(i)->exponent()->isNumber() && mstruct_new.getChild(i)->exponent()->number().isRational()) {
						int points = 0;
						if(mstruct_new.getChild(i)->exponent()->number().isInteger()) points = mstruct_new.getChild(i)->exponent()->number().intValue();
						else points = mstruct_new.getChild(i)->exponent()->number().numerator().intValue() + mstruct_new.getChild(i)->exponent()->number().denominator().intValue() * (mstruct_new.getChild(i)->exponent()->number().isNegative() ? -1 : 1);
						if(new_is_si_units && !mstruct_new.getChild(i)->base()->unit()->isSIUnit()) new_is_si_units = false;
						new_is_currency = mstruct_new.getChild(i)->base()->unit()->isCurrency();
						if(points < 0) {
							new_points -= points;
						} else {
							new_points += points;
							new_minus = false;
						}
					}
				}
			} else if(mstruct_new.isPower() && mstruct_new.base()->isUnit() && mstruct_new.exponent()->isNumber() && mstruct_new.exponent()->number().isRational()) {
				int points = 0;
				if(mstruct_new.exponent()->number().isInteger()) points = mstruct_new.exponent()->number().intValue();
				else points = mstruct_new.exponent()->number().numerator().intValue() + mstruct_new.exponent()->number().denominator().intValue() * (mstruct_new.exponent()->number().isNegative() ? -1 : 1);
				if(new_is_si_units && !mstruct_new.base()->unit()->isSIUnit()) new_is_si_units = false;
				new_is_currency = mstruct_new.base()->unit()->isCurrency();
				if(points < 0) {
					new_points = -points;
				} else {
					new_points = points;
					new_minus = false;
				}
			} else if(mstruct_new.isUnit()) {
				if(!mstruct_new.unit()->isSIUnit()) new_is_si_units = false;
				new_is_currency = mstruct_new.unit()->isCurrency();
				new_points = 1;
				new_minus = false;
			}
			if(new_points == 0) return mstruct_old;
			if((new_points > old_points && (!convert_to_si_units || is_si_units || !new_is_si_units)) || (new_points == old_points && (new_minus || !old_minus) && (!new_is_currency || !eo.local_currency_conversion) && (!convert_to_si_units || !new_is_si_units))) return mstruct_old;
			return mstruct_new;
		}
		default: {}
	}
	return mstruct;
}
MathStructure Calculator::convertToCompositeUnit(const MathStructure &mstruct, CompositeUnit *cu, const EvaluationOptions &eo, bool always_convert) {
	return convert(mstruct, cu, eo, always_convert);
}
MathStructure Calculator::convert(const MathStructure &mstruct_to_convert, string str2, const EvaluationOptions &eo, MathStructure *to_struct) {
	if(to_struct) to_struct->setUndefined();
	remove_blank_ends(str2);
	if(str2.empty()) return mstruct_to_convert;
	current_stage = MESSAGE_STAGE_CONVERSION;
	EvaluationOptions eo2 = eo;
	eo2.keep_prefixes = (str2[0] != '?');
	if(str2[0] == '-') eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
	else if(str2[0] == '+') eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
	else if(eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) eo2.mixed_units_conversion = MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP;
	if(str2[0] == '?' || str2[0] == '0' || str2[0] == '+' || str2[0] == '-') {
		str2 = str2.substr(1, str2.length() - 1);
		remove_blank_ends(str2);
		if(str2.empty()) {
			current_stage = MESSAGE_STAGE_UNSET;
			return convertToMixedUnits(mstruct_to_convert, eo2);
		}
	}
	MathStructure mstruct;
	bool b = false;
	Unit *u = getUnit(str2);
	if(!u) u = getCompositeUnit(str2);
	Variable *v = NULL;
	if(!u) v = getVariable(str2);
	if(!u && !v) {
		for(size_t i = 0; i < signs.size(); i++) {
			if(str2 == signs[i]) {
				u = getUnit(real_signs[i]);
				if(!u) v = getVariable(real_signs[i]);
				break;
			}
		}
	}
	if(v && !v->isKnown()) v = NULL;
	if(u) {
		if(to_struct) to_struct->set(u);
		mstruct.set(convert(mstruct_to_convert, u, eo2, false, false));
		b = true;
	} else if(v) {
		if(to_struct) to_struct->set(v);
		mstruct.set(convert(mstruct_to_convert, (KnownVariable*) v, eo2));
		b = true;
	} else {
		current_stage = MESSAGE_STAGE_CONVERSION_PARSING;
		CompositeUnit cu("", "temporary_composite_convert", "", str2);
		current_stage = MESSAGE_STAGE_CONVERSION;
		if(to_struct) to_struct->set(cu.generateMathStructure());
		if(cu.countUnits() > 0) {
			mstruct.set(convert(mstruct_to_convert, &cu, eo2, false, false));
			b = true;
		}
	}
	if(!b) return mstruct_to_convert;
	if(!v && eo2.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE) mstruct.set(convertToMixedUnits(mstruct, eo2));
	current_stage = MESSAGE_STAGE_UNSET;
	return mstruct;
}
Unit* Calculator::addUnit(Unit *u, bool force, bool check_names) {
	if(check_names) {
		for(size_t i = 1; i <= u->countNames(); i++) {
			u->setName(getName(u->getName(i).name, u, force), i);
		}
	}
	if(!u->isLocal() && units.size() > 0 && units[units.size() - 1]->isLocal()) {
		units.insert(units.begin(), u);
	} else {	
		units.push_back(u);
	}
	unitNameChanged(u, true);
	for(vector<Unit*>::iterator it = deleted_units.begin(); it != deleted_units.end(); ++it) {
		if(*it == u) {
			deleted_units.erase(it);
			break;
		}
	}
	u->setRegistered(true);
	u->setChanged(false);
	return u;
}
void Calculator::delPrefixUFV(Prefix *object) {
	int i = 0;
	for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
		del_ufvl:
		if(it == ufvl.end()) {
			break;
		}
		if(*it == object) {
			it = ufvl.erase(it);
			ufvl_t.erase(ufvl_t.begin() + i);
			ufvl_i.erase(ufvl_i.begin() + i);
			if(it == ufvl.end()) break;
			goto del_ufvl;
		}
		i++;
	}
	for(size_t i2 = 0; i2 < UFV_LENGTHS; i2++) {
		i = 0;
		for(vector<void*>::iterator it = ufv[0][i2].begin(); ; ++it) {
			del_ufv:
			if(it == ufv[0][i2].end()) {
				break;
			}
			if(*it == object) {
				it = ufv[0][i2].erase(it);
				ufv_i[0][i2].erase(ufv_i[0][i2].begin() + i);
				if(it == ufv[0][i2].end()) break;
				goto del_ufv;
			}
			i++;
		}
	}
}
void Calculator::delUFV(ExpressionItem *object) {
	int i = 0;
	for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
		del_ufvl:
		if(it == ufvl.end()) {
			break;
		}
		if(*it == object) {
			it = ufvl.erase(it);
			ufvl_t.erase(ufvl_t.begin() + i);
			ufvl_i.erase(ufvl_i.begin() + i);
			if(it == ufvl.end()) break;
			goto del_ufvl;
		}
		i++;
	}
	int i3 = 0;
	switch(object->type()) {
		case TYPE_FUNCTION: {i3 = 1; break;}
		case TYPE_UNIT: {i3 = 2; break;}
		case TYPE_VARIABLE: {i3 = 3; break;}
	}
	for(size_t i2 = 0; i2 < UFV_LENGTHS; i2++) {
		i = 0;
		for(vector<void*>::iterator it = ufv[i3][i2].begin(); ; ++it) {
			del_ufv:
			if(it == ufv[i3][i2].end()) {
				break;
			}
			if(*it == object) {
				it = ufv[i3][i2].erase(it);
				ufv_i[i3][i2].erase(ufv_i[i3][i2].begin() + i);
				if(it == ufv[i3][i2].end()) break;
				goto del_ufv;
			}
			i++;
		}
	}
}
Unit* Calculator::getUnit(string name_) {
	if(name_.empty()) return NULL;
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->subtype() != SUBTYPE_COMPOSITE_UNIT && (units[i]->hasName(name_))) {
			return units[i];
		}
	}
	return NULL;
}
Unit* Calculator::getActiveUnit(string name_) {
	if(name_.empty()) return NULL;
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->isActive() && units[i]->subtype() != SUBTYPE_COMPOSITE_UNIT && units[i]->hasName(name_)) {
			return units[i];
		}
	}
	return NULL;
}
Unit* Calculator::getLocalCurrency() {
	if(priv->local_currency) return priv->local_currency;
	struct lconv *lc = localeconv();
	if(lc) {
		string local_currency = lc->int_curr_symbol;
		remove_blank_ends(local_currency);
		if(!local_currency.empty()) {
			if(local_currency.length() > 3) local_currency = local_currency.substr(0, 3);
			return getActiveUnit(local_currency);
		}
	}
	return NULL;
}
void Calculator::setLocalCurrency(Unit *u) {
	priv->local_currency = u;
}
Unit* Calculator::getCompositeUnit(string internal_name_) {
	if(internal_name_.empty()) return NULL;
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT && units[i]->hasName(internal_name_)) {
			return units[i];
		}
	}
	return NULL;
}

Variable* Calculator::addVariable(Variable *v, bool force, bool check_names) {
	if(check_names) {
		for(size_t i = 1; i <= v->countNames(); i++) {
			v->setName(getName(v->getName(i).name, v, force), i);
		}
	}
	if(!v->isLocal() && variables.size() > 0 && variables[variables.size() - 1]->isLocal()) {
		variables.insert(variables.begin(), v);
	} else {
		variables.push_back(v);
	}
	variableNameChanged(v, true);
	for(vector<Variable*>::iterator it = deleted_variables.begin(); it != deleted_variables.end(); ++it) {
		if(*it == v) {
			deleted_variables.erase(it);
			break;
		}
	}
	v->setRegistered(true);
	v->setChanged(false);
	return v;
}
void Calculator::expressionItemDeactivated(ExpressionItem *item) {
	delUFV(item);
}
void Calculator::expressionItemActivated(ExpressionItem *item) {
	ExpressionItem *item2 = getActiveExpressionItem(item);
	if(item2) {
		item2->setActive(false);
	}
	nameChanged(item);
}
void Calculator::expressionItemDeleted(ExpressionItem *item) {
	switch(item->type()) {
		case TYPE_VARIABLE: {
			for(vector<Variable*>::iterator it = variables.begin(); it != variables.end(); ++it) {
				if(*it == item) {
					variables.erase(it);
					deleted_variables.push_back((Variable*) item);
					break;
				}
			}
			break;
		}
		case TYPE_FUNCTION: {
			for(vector<MathFunction*>::iterator it = functions.begin(); it != functions.end(); ++it) {
				if(*it == item) {
					functions.erase(it);
					deleted_functions.push_back((MathFunction*) item);
					break;
				}
			}
			if(item->subtype() == SUBTYPE_DATA_SET) {
				for(vector<DataSet*>::iterator it = data_sets.begin(); it != data_sets.end(); ++it) {
					if(*it == item) {
						data_sets.erase(it);
						break;
					}
				}	
			}
			break;
		}		
		case TYPE_UNIT: {
			for(vector<Unit*>::iterator it = units.begin(); it != units.end(); ++it) {
				if(*it == item) {
					units.erase(it);
					deleted_units.push_back((Unit*) item);
					break;
				}
			}
			break;
		}
	}
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		if(item->type() == TYPE_VARIABLE || item->type() == TYPE_UNIT) {
			for(size_t i = 0; i < variables.size(); i++) {
				if(!variables[i]->isLocal() && !variables[i]->isActive() && variables[i]->hasName(item->getName(i2).name, item->getName(i2).case_sensitive) && !getActiveExpressionItem(variables[i])) {variables[i]->setActive(true);}
			}
			for(size_t i = 0; i < units.size(); i++) {
				if(!units[i]->isLocal() && !units[i]->isActive() && units[i]->hasName(item->getName(i2).name, item->getName(i2).case_sensitive) && !getActiveExpressionItem(units[i])) units[i]->setActive(true);
			}
		} else {
			for(size_t i = 0; i < functions.size(); i++) {
				if(!functions[i]->isLocal() && !functions[i]->isActive() && functions[i]->hasName(item->getName(i2).name, item->getName(i2).case_sensitive) && !getActiveExpressionItem(functions[i])) functions[i]->setActive(true);
			}
		}
	}
	delUFV(item);
}
void Calculator::nameChanged(ExpressionItem *item, bool new_item) {
	if(!item->isActive() || item->countNames() == 0) return;
	if(item->type() == TYPE_UNIT && ((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		return;
	}
	size_t l2;
	if(!new_item) delUFV(item);
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		l2 = item->getName(i2).name.length();
		if(l2 > UFV_LENGTHS) {
			size_t i = 0, l = 0;
			for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
				if(it != ufvl.end()) {
					if(ufvl_t[i] == 'v')
						l = ((Variable*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'f')
						l = ((MathFunction*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'u')
						l = ((Unit*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'p')
						l = ((Prefix*) (*it))->shortName(false).length();
					else if(ufvl_t[i] == 'P')
						l = ((Prefix*) (*it))->longName(false).length();				
					else if(ufvl_t[i] == 'q')
						l = ((Prefix*) (*it))->unicodeName(false).length();
				}
				if(it == ufvl.end()) {
					ufvl.push_back((void*) item);
					switch(item->type()) {
						case TYPE_VARIABLE: {ufvl_t.push_back('v'); break;}
						case TYPE_FUNCTION: {ufvl_t.push_back('f'); break;}		
						case TYPE_UNIT: {ufvl_t.push_back('u'); break;}		
					}
					ufvl_i.push_back(i2);
					break;
				} else {
					if(l < l2 
					|| (item->type() == TYPE_VARIABLE && l == l2 && ufvl_t[i] == 'v')
					|| (item->type() == TYPE_FUNCTION && l == l2 && (ufvl_t[i] != 'p' && ufvl_t[i] != 'P' && ufvl_t[i] != 'q'))
					|| (item->type() == TYPE_UNIT && l == l2 && (ufvl_t[i] != 'p' && ufvl_t[i] != 'P' && ufvl_t[i] != 'q' && ufvl_t[i] != 'f'))
					) {
						ufvl.insert(it, (void*) item);
						switch(item->type()) {
							case TYPE_VARIABLE: {ufvl_t.insert(ufvl_t.begin() + i, 'v'); break;}
							case TYPE_FUNCTION: {ufvl_t.insert(ufvl_t.begin() + i, 'f'); break;}		
							case TYPE_UNIT: {ufvl_t.insert(ufvl_t.begin() + i, 'u'); break;}		
						}
						ufvl_i.insert(ufvl_i.begin() + i, i2);
						break;
					}
				}
				i++;
			}
		} else if(l2 > 0) {
			l2--;
			switch(item->type()) {			
				case TYPE_VARIABLE: {
					ufv[3][l2].push_back((void*) item);
					ufv_i[3][l2].push_back(i2);
					break;
				}
				case TYPE_FUNCTION:  {
					ufv[1][l2].push_back((void*) item);
					ufv_i[1][l2].push_back(i2);
					break;
				}
				case TYPE_UNIT:  {
					ufv[2][l2].push_back((void*) item);
					ufv_i[2][l2].push_back(i2);
					break;
				}
			}			
		}
	}
}
void Calculator::variableNameChanged(Variable *v, bool new_item) {
	nameChanged(v, new_item);
}
void Calculator::functionNameChanged(MathFunction *f, bool new_item) {
	nameChanged(f, new_item);
}
void Calculator::unitNameChanged(Unit *u, bool new_item) {
	nameChanged(u, new_item);
}

Variable* Calculator::getVariable(string name_) {
	if(name_.empty()) return NULL;
	for(size_t i = 0; i < variables.size(); i++) {
		if(variables[i]->hasName(name_)) {
			return variables[i];
		}
	}
	return NULL;
}
Variable* Calculator::getActiveVariable(string name_) {
	if(name_.empty()) return NULL;
	for(size_t i = 0; i < variables.size(); i++) {
		if(variables[i]->isActive() && variables[i]->hasName(name_)) {
			return variables[i];
		}
	}
	return NULL;
}
ExpressionItem* Calculator::addExpressionItem(ExpressionItem *item, bool force) {
	switch(item->type()) {
		case TYPE_VARIABLE: {
			return addVariable((Variable*) item, force);
		}
		case TYPE_FUNCTION: {
			if(item->subtype() == SUBTYPE_DATA_SET) return addDataSet((DataSet*) item, force);
			else return addFunction((MathFunction*) item, force);
		}		
		case TYPE_UNIT: {
			return addUnit((Unit*) item, force);
		}		
	}
	return NULL;
}
MathFunction* Calculator::addFunction(MathFunction *f, bool force, bool check_names) {
	if(check_names) {
		for(size_t i = 1; i <= f->countNames(); i++) {
			f->setName(getName(f->getName(i).name, f, force), i);
		}
	}
	if(!f->isLocal() && functions.size() > 0 && functions[functions.size() - 1]->isLocal()) {
		functions.insert(functions.begin(), f);
	} else {
		functions.push_back(f);
	}
	functionNameChanged(f, true);
	for(vector<MathFunction*>::iterator it = deleted_functions.begin(); it != deleted_functions.end(); ++it) {
		if(*it == f) {
			deleted_functions.erase(it);
			break;
		}
	}
	f->setRegistered(true);
	f->setChanged(false);
	return f;
}
DataSet* Calculator::addDataSet(DataSet *dc, bool force, bool check_names) {
	addFunction(dc, force, check_names);
	data_sets.push_back(dc);
	return dc;
}
DataSet* Calculator::getDataSet(size_t index) {
	if(index > 0 && index <= data_sets.size()) {
		return data_sets[index - 1];
	}
	return 0;
}
DataSet* Calculator::getDataSet(string name) {
	if(name.empty()) return NULL;
	for(size_t i = 0; i < data_sets.size(); i++) {
		if(data_sets[i]->hasName(name)) {
			return data_sets[i];
		}
	}
	return NULL;
}
MathFunction* Calculator::getFunction(string name_) {
	if(name_.empty()) return NULL;
	for(size_t i = 0; i < functions.size(); i++) {
		if(functions[i]->hasName(name_)) {
			return functions[i];
		}
	}
	return NULL;
}
MathFunction* Calculator::getActiveFunction(string name_) {
	if(name_.empty()) return NULL;
	for(size_t i = 0; i < functions.size(); i++) {
		if(functions[i]->isActive() && functions[i]->hasName(name_)) {
			return functions[i];
		}
	}
	return NULL;
}
bool Calculator::variableNameIsValid(const string &name_) {
	return !name_.empty() && name_.find_first_of(ILLEGAL_IN_NAMES) == string::npos && is_not_in(NUMBERS, name_[0]);
}
bool Calculator::functionNameIsValid(const string &name_) {
	return !name_.empty() && name_.find_first_of(ILLEGAL_IN_NAMES) == string::npos && is_not_in(NUMBERS, name_[0]);
}
bool Calculator::unitNameIsValid(const string &name_) {
	return !name_.empty() && name_.find_first_of(ILLEGAL_IN_UNITNAMES) == string::npos;
}
bool Calculator::variableNameIsValid(const char *name_) {
	if(strlen(name_) == 0) return false;
	if(is_in(NUMBERS, name_[0])) return false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_NAMES, name_[i])) return false;
	}
	return true;
}
bool Calculator::functionNameIsValid(const char *name_) {
	if(strlen(name_) == 0) return false;
	if(is_in(NUMBERS, name_[0])) return false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_NAMES, name_[i])) return false;
	}
	return true;
}
bool Calculator::unitNameIsValid(const char *name_) {
	if(strlen(name_) == 0) return false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_UNITNAMES, name_[i])) return false;
	}
	return true;
}
#define VERSION_BEFORE(i1, i2, i3) (version_numbers[0] < i1 || (version_numbers[0] == i1 && (version_numbers[1] < i2 || (version_numbers[1] == i2 && version_numbers[2] < i3))))
bool Calculator::variableNameIsValid(const string &name_, int version_numbers[3], bool is_user_defs) {
	return variableNameIsValid(name_.c_str(), version_numbers, is_user_defs);
}
bool Calculator::functionNameIsValid(const string &name_, int version_numbers[3], bool is_user_defs) {
	return functionNameIsValid(name_.c_str(), version_numbers, is_user_defs);
}
bool Calculator::unitNameIsValid(const string &name_, int version_numbers[3], bool is_user_defs) {
	return unitNameIsValid(name_.c_str(), version_numbers, is_user_defs);
}
bool Calculator::variableNameIsValid(const char *name_, int version_numbers[3], bool is_user_defs) {
	if(strlen(name_) == 0) return false;
	if(is_in(NUMBERS, name_[0])) return false;
	bool b = false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_NAMES, name_[i])) {
			if(is_user_defs && VERSION_BEFORE(0, 8, 1) && name_[i] == BITWISE_NOT_CH) {
				b = true;	
			} else {
				return false;
			}
		}
	}
	if(b) {
		error(true, _("\"%s\" is not allowed in names anymore. Please change the name of \"%s\", or the variable will be lost."), BITWISE_NOT, name_, NULL);
	}
	return true;
}
bool Calculator::functionNameIsValid(const char *name_, int version_numbers[3], bool is_user_defs) {
	if(strlen(name_) == 0) return false;
	if(is_in(NUMBERS, name_[0])) return false;
	bool b = false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_NAMES, name_[i])) {
			if(is_user_defs && VERSION_BEFORE(0, 8, 1) && name_[i] == BITWISE_NOT_CH) {
				b = true;	
			} else {
				return false;
			}
		}
	}
	if(b) {
		error(true, _("\"%s\" is not allowed in names anymore. Please change the name \"%s\", or the function will be lost."), BITWISE_NOT, name_, NULL);
	}
	return true;
}
bool Calculator::unitNameIsValid(const char *name_, int version_numbers[3], bool is_user_defs) {
	if(strlen(name_) == 0) return false;
	bool b = false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_UNITNAMES, name_[i])) {
			if(is_user_defs && VERSION_BEFORE(0, 8, 1) && name_[i] == BITWISE_NOT_CH) {
				b = true;	
			} else {
				return false;
			}
		}
	}
	if(b) {
		error(true, _("\"%s\" is not allowed in names anymore. Please change the name \"%s\", or the unit will be lost."), BITWISE_NOT, name_, NULL);
	}
	return true;
}
string Calculator::convertToValidVariableName(string name_) {
	if(name_.empty()) return "var_1";
	size_t i = 0;
	while(true) {
		i = name_.find_first_of(ILLEGAL_IN_NAMES_MINUS_SPACE_STR, i);
		if(i == string::npos)
			break;
		name_.erase(name_.begin() + i);
	}
	gsub(SPACE, UNDERSCORE, name_);
	while(is_in(NUMBERS, name_[0])) {
		name_.erase(name_.begin());
	}
	return name_;
}
string Calculator::convertToValidFunctionName(string name_) {
	if(name_.empty()) return "func_1";
	return convertToValidVariableName(name_);
}
string Calculator::convertToValidUnitName(string name_) {
	if(name_.empty()) return "new_unit";
	size_t i = 0;
	string stmp = ILLEGAL_IN_NAMES_MINUS_SPACE_STR + NUMBERS;
	while(true) {
		i = name_.find_first_of(stmp, i);
		if(i == string::npos)
			break;
		name_.erase(name_.begin() + i);
	}
	gsub(SPACE, UNDERSCORE, name_);
	return name_;
}
bool Calculator::nameTaken(string name, ExpressionItem *object) {
	if(name.empty()) return false;
	if(object) {
		switch(object->type()) {
			case TYPE_VARIABLE: {}
			case TYPE_UNIT: {
				for(size_t index = 0; index < variables.size(); index++) {
					if(variables[index]->isActive() && variables[index]->hasName(name)) {
						return variables[index] != object;
					}
				}
				for(size_t i = 0; i < units.size(); i++) {
					if(units[i]->isActive() && units[i]->hasName(name)) {
						return units[i] != object;
					}
				}
				break;
			}
			case TYPE_FUNCTION: {
				for(size_t index = 0; index < functions.size(); index++) {
					if(functions[index]->isActive() && functions[index]->hasName(name)) {
						return functions[index] != object;
					}
				}
				break;
			}
		}
	} else {
		return getActiveExpressionItem(name) != NULL;
	}
	return false;
}
bool Calculator::variableNameTaken(string name, Variable *object) {
	if(name.empty()) return false;
	for(size_t index = 0; index < variables.size(); index++) {
		if(variables[index]->isActive() && variables[index]->hasName(name)) {
			return variables[index] != object;
		}
	}
	
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->isActive() && units[i]->hasName(name)) {
			return true;
		}
	}
	return false;
}
bool Calculator::unitNameTaken(string name, Unit *object) {
	if(name.empty()) return false;
	for(size_t index = 0; index < variables.size(); index++) {
		if(variables[index]->isActive() && variables[index]->hasName(name)) {
			return true;
		}
	}
	
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i]->isActive() && units[i]->hasName(name)) {
			return units[i] == object;
		}
	}
	return false;
}
bool Calculator::functionNameTaken(string name, MathFunction *object) {
	if(name.empty()) return false;
	for(size_t index = 0; index < functions.size(); index++) {
		if(functions[index]->isActive() && functions[index]->hasName(name)) {
			return functions[index] != object;
		}
	}
	return false;
}
bool Calculator::unitIsUsedByOtherUnits(const Unit *u) const {
	const Unit *u2;
	for(size_t i = 0; i < units.size(); i++) {
		if(units[i] != u) {
			u2 = units[i];
			while(u2->subtype() == SUBTYPE_ALIAS_UNIT) {
				u2 = ((AliasUnit*) u2)->firstBaseUnit();
				if(u2 == u) {
					return true;
				}
			}
		}
	}
	return false;
}

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
	if(base < 2 || base > 10) {
		for(size_t i = 0; i < name_length; i++) {
			if(is_not_number(str[str_index + i], base)) return true;
		}
		return false;
	}
	return true;
}
size_t compare_name_no_case(const string &name, const string &str, const size_t &name_length, const size_t &str_index, int base) {
	if(name_length == 0) return 0;
	size_t is = str_index;
	for(size_t i = 0; i < name_length; i++, is++) {
		if(is >= str.length()) return 0;
		if((name[i] < 0 && i + 1 < name_length) || (str[is] < 0 && is + 1 < str.length())) {
			size_t i2 = 1, is2 = 1;
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
			bool isequal = (i2 == is2);
			if(isequal) {
				for(size_t i3 = 0; i3 < i2; i3++) {
					if(str[is + i3] != name[i + i3]) {
						isequal = false;
						break;
					}
				}
			}
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
	if(base < 2 || base > 10) {
		for(size_t i = str_index; i < is; i++) {
			if(is_not_number(str[i], base)) return is - str_index;
		}
		return 0;
	}
	return is - str_index;
}

const char *internal_signs[] = {SIGN_PLUSMINUS, "\b", "+/-", "\b", "⊻", "\a"};
#define INTERNAL_SIGNS_COUNT 6
#define INTERNAL_NUMBER_CHARS "\b"
#define INTERNAL_OPERATORS "\a\b%"
#define DUODECIMAL_CHARS "EX"

void Calculator::parseSigns(string &str, bool convert_to_internal_representation) const {
	vector<size_t> q_begin;
	vector<size_t> q_end;
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
	for(size_t i = 0; i < signs.size(); i++) {
		size_t ui = str.find(signs[i]);
		while(ui != string::npos) {
			bool b = false;
			for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
				if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
					ui = str.find(signs[i], q_end[ui2] + 1);
					b = true;
					break;
				}
			}
			if(!b) {
				int index_shift = real_signs[i].length() - signs[i].length();
				for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
					if(q_begin[ui2] >= ui) {
						q_begin[ui2] += index_shift;
						q_end[ui2] += index_shift;
					}
				}
				str.replace(ui, signs[i].length(), real_signs[i]);
				ui = str.find(signs[i], ui + real_signs[i].length());
			}
		}
	}

	size_t prev_ui = string::npos, space_n = 0;
	while(true) {
		size_t ui = str.find("\xe2\x81", prev_ui == string::npos ? 0 : prev_ui);
		if(ui != string::npos && (ui == str.length() - 2 || (str[ui + 2] != -80 && (str[ui + 2] < -76 || str[ui + 2] > -71)))) ui = string::npos;
		size_t ui2 = str.find('\xc2', prev_ui == string::npos ? 0 : prev_ui);
		if(ui2 != string::npos && (ui2 == str.length() - 1 || (str[ui2 + 1] != -71 && str[ui2 + 1] != -77 && str[ui2 + 1] != -78))) ui2 = string::npos;
		if(ui2 != string::npos && (ui == string::npos || ui2 < ui)) ui = ui2;
		if(ui != string::npos) {
			for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
				if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
					ui = str.find("\xe2\x81", q_end[ui3] + 1);
					if(ui != string::npos && (ui == str.length() - 2 || (str[ui + 2] != -80 && (str[ui + 2] < -76 || str[ui + 2] > -71)))) ui = string::npos;
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
		for(size_t ui3 = 0; ui3 < q_begin.size(); ui3++) {
			if(q_begin[ui3] >= ui) {
				q_begin[ui3] += index_shift;
				q_end[ui3] += index_shift;
			}
		}
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
	prev_ui = string::npos;
	while(true) {
		size_t ui = str.find("\xe2\x85", prev_ui == string::npos ? 0 : prev_ui);
		if(ui != string::npos && (ui == str.length() - 2 || str[ui + 2] < -112 || str[ui + 2] > -98)) ui = string::npos;
		if(ui != string::npos) {
			for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
				if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
					ui = str.find("\xe2\x85", q_end[ui3] + 1);
					if(ui != string::npos && (ui == str.length() - 2 || str[ui + 2] < -112 || str[ui + 2] > -98)) ui = string::npos;
					if(ui == string::npos) break;
				}
			}
		}
		if(ui == string::npos) break;
		space_n = 0;
		while(ui > 0 && ui - 1 - space_n != 0 && str[ui - 1 - space_n] == SPACE_CH) space_n++;
		bool b_add = (ui > 0 && is_in(NUMBER_ELEMENTS, str[ui - 1 - space_n]));
		int index_shift = (b_add ? 6 : 5) - 3;
		if(str[ui + 2] == -110) index_shift++;
		for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
			if(q_begin[ui2] >= ui) {
				q_begin[ui2] += index_shift;
				q_end[ui2] += index_shift;
			}
		}
		if(str[ui + 2] == -98) str.replace(ui, 3, b_add ? "+(7/8)" : "(7/8)");
		else if(str[ui + 2] == -99) str.replace(ui, 3, b_add ? "+(5/8)" : "(5/8)");
		else if(str[ui + 2] == -100) str.replace(ui, 3, b_add ? "+(3/8)" : "(3/8)");
		else if(str[ui + 2] == -101) str.replace(ui, 3, b_add ? "+(1/8)" : "(1/8)");
		else if(str[ui + 2] == -102) str.replace(ui, 3, b_add ? "+(5/6)" : "(5/6)");
		else if(str[ui + 2] == -103) str.replace(ui, 3, b_add ? "+(1/6)" : "(1/6)");
		else if(str[ui + 2] == -104) str.replace(ui, 3, b_add ? "+(4/5)" : "(4/5)");
		else if(str[ui + 2] == -105) str.replace(ui, 3, b_add ? "+(3/5)" : "(3/5)");
		else if(str[ui + 2] == -106) str.replace(ui, 3, b_add ? "+(2/5)" : "(2/5)");
		else if(str[ui + 2] == -107) str.replace(ui, 3, b_add ? "+(1/5)" : "(1/5)");
		else if(str[ui + 2] == -108) str.replace(ui, 3, b_add ? "+(2/3)" : "(2/3)");
		else if(str[ui + 2] == -109) str.replace(ui, 3, b_add ? "+(1/3)" : "(1/3)");
		else if(str[ui + 2] == -110) {str.replace(ui, 3, b_add ? "+(1/10)" : "(1/10)"); ui++;}
		else if(str[ui + 2] == -111) str.replace(ui, 3, b_add ? "+(1/9)" : "(1/9)");
		else if(str[ui + 2] == -112) str.replace(ui, 3, b_add ? "+(1/7)" : "(1/7)");
		if(b_add) prev_ui = ui + 6;
		else prev_ui = ui + 5;
	}
	prev_ui = string::npos;
	while(true) {
		size_t ui = str.find('\xc2', prev_ui == string::npos ? 0 : prev_ui);
		if(ui != string::npos && (ui == str.length() - 1 || (str[ui + 1] != -66 && str[ui + 1] != -67 && str[ui + 1] != -68))) ui = string::npos;
		if(ui != string::npos) {
			for(size_t ui3 = 0; ui3 < q_end.size(); ui3++) {
				if(ui <= q_end[ui3] && ui >= q_begin[ui3]) {
					ui = str.find('\xc2', q_end[ui3] + 1);
					if(ui != string::npos && (ui == str.length() - 1 || (str[ui + 1] != -66 && str[ui + 1] != -67 && str[ui + 1] != -68))) ui = string::npos;
					if(ui == string::npos) break;
				}
			}
		}
		if(ui == string::npos) break;
		space_n = 0;
		while(ui > 0 && ui - 1 - space_n != 0 && str[ui - 1 - space_n] == SPACE_CH) space_n++;
		bool b_add = (ui > 0 && is_in(NUMBER_ELEMENTS, str[ui - 1 - space_n]));
		int index_shift = (b_add ? 6 : 5) - 2;
		for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
			if(q_begin[ui2] >= ui) {
				q_begin[ui2] += index_shift;
				q_end[ui2] += index_shift;
			}
		}
		if(str[ui + 1] == -66) str.replace(ui, 2, b_add ? "+(3/4)" : "(3/4)");
		else if(str[ui + 1] == -67) str.replace(ui, 2, b_add ? "+(1/2)" : "(1/2)");
		else if(str[ui + 1] == -68) str.replace(ui, 2, b_add ? "+(1/4)" : "(1/4)");
		if(b_add) prev_ui = ui + 6;
		else prev_ui = ui + 5;
	}
	if(convert_to_internal_representation) {
		remove_blank_ends(str);
		remove_duplicate_blanks(str);
		for(size_t i = 0; i < INTERNAL_SIGNS_COUNT; i += 2) {
			size_t ui = str.find(internal_signs[i]);
			while(ui != string::npos) {
				bool b = false;
				for(size_t ui2 = 0; ui2 < q_end.size(); ui2++) {
					if(ui <= q_end[ui2] && ui >= q_begin[ui2]) {
						ui = str.find(internal_signs[i], q_end[ui2] + 1);
						b = true;
						break;
					}
				}
				if(!b) {
					int index_shift = strlen(internal_signs[i + 1]) - strlen(internal_signs[i]);
					for(size_t ui2 = 0; ui2 < q_begin.size(); ui2++) {
						if(q_begin[ui2] >= ui) {
							q_begin[ui2] += index_shift;
							q_end[ui2] += index_shift;
						}
					}
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

void Calculator::parse(MathStructure *mstruct, string str, const ParseOptions &parseoptions) {

	ParseOptions po = parseoptions;
	MathStructure *unended_function = po.unended_function;
	po.unended_function = NULL;
	
	if(po.base == BASE_UNICODE || (po.base == BASE_CUSTOM && priv->custom_input_base_i > 62)) {
		mstruct->set(Number(str, po));
		return;
	}
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
	
	bool b_prime_quote = true;
	
	size_t i_degree = str.find(SIGN_DEGREE);
	if(i_degree != string::npos && i_degree < str.length() - strlen(SIGN_DEGREE) && is_not_in(NOT_IN_NAMES INTERNAL_OPERATORS NUMBER_ELEMENTS, str[i_degree + strlen(SIGN_DEGREE)])) i_degree = string::npos;
	
	if(i_degree == string::npos) {
		size_t i_quote = str.find('\'', 0);
		size_t i_dquote = str.find('\"', 0);
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
	}
	if(b_prime_quote) {
		gsub("\'", "′", str);
		gsub("\"", "″", str);
	}

	parseSigns(str, true);

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
		gsub(LEFT_VECTOR_WRAP, LEFT_PARENTHESIS, str);
		gsub(RIGHT_VECTOR_WRAP, RIGHT_PARENTHESIS, str);
	}
	
	
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
	if(!po.rpn) {
		space_i = str.find(SPACE_CH, 0);
		while(space_i != string::npos) {
			if(is_in(NUMBERS INTERNAL_NUMBER_CHARS DOT, str[space_i + 1]) && is_in(NUMBERS INTERNAL_NUMBER_CHARS DOT, str[space_i - 1])) {
				str.erase(space_i, 1);
				space_i--;
			}		
			space_i = str.find(SPACE_CH, space_i + 1);
		}
	}

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

	size_t i_mod = str.find("%");
	if(i_mod != string::npos && !v_percent->hasName("%")) i_mod = string::npos;
	while(i_mod != string::npos) {
		if(po.rpn) {
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
	
	if(po.rpn) {
		gsub("&&", "& &", str);
		gsub("||", "| |", str);
		gsub("\%\%", "\% \%", str);
	}

	for(size_t str_index = 0; str_index < str.length(); str_index++) {
		if(str[str_index] == LEFT_VECTOR_WRAP_CH) {
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
		} else if(str[str_index] == '\\' && str_index + 1 < str.length() && (is_not_in(NOT_IN_NAMES INTERNAL_OPERATORS NUMBERS, str[str_index + 1]) || (!po.rpn && str_index > 0 && is_in(NUMBERS SPACE PLUS MINUS BITWISE_NOT NOT LEFT_PARENTHESIS, str[str_index + 1])))) {
			if(is_in(NUMBERS SPACE PLUS MINUS BITWISE_NOT NOT LEFT_PARENTHESIS, str[str_index + 1])) {
				str.replace(str_index, 1, "//");
				str_index++;
			} else {
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
		} else if(!po.rpn && (str[str_index] == 'c' || str[str_index] == 'C') && str.length() > str_index + 6 && str[str_index + 5] == SPACE_CH && (str_index == 0 || is_in(OPERATORS INTERNAL_OPERATORS PARENTHESISS, str[str_index - 1])) && compare_name_no_case("compl", str, 5, str_index, base)) {
			str.replace(str_index, 6, BITWISE_NOT);
		} else if(str[str_index] == SPACE_CH) {
			size_t i = str.find(SPACE, str_index + 1);
			if(po.rpn && i == string::npos) i = str.length();
			if(i != string::npos) {
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
					if(po.rpn) {
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
					if(po.rpn) i = str.find_first_not_of(NUMBER_ELEMENTS "abcdefABCDEF", str_index + 2);
					else i = str.find_first_not_of(SPACE NUMBER_ELEMENTS "abcdefABCDEF", str_index + 2);
					size_t name_length;
					if(i == string::npos) i = str.length();
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
					if(po.rpn) i = str.find_first_not_of(NUMBER_ELEMENTS, str_index + 2);
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
			} else if(base <= 24 && str_index + 2 < str.length() && (str[str_index + 1] == 'o' || str[str_index + 1] == 'O') && is_in(NUMBERS, str[str_index + 2])) {
				//octal number 0o...
				if(po.base == BASE_OCTAL) {
					str.erase(str_index, 2);
				} else {
					size_t i;
					if(po.rpn) i = str.find_first_not_of(NUMBER_ELEMENTS, str_index + 2);
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
			bool p_mode = false;
			void *best_p_object = NULL;
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
										if(name_length >= unit_chars_left || name_length < found_function_name_length) {
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
						if(po.units_enabled && ufv_index < unit_chars_left - 1 && vt3 < ufv[vt2][ufv_index].size()) {
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
									ufvt = 'p';
									name = &((Prefix*) object)->shortName();
									name_length = name->length();
									case_sensitive = true;
									break;
								}
								case 3: {
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
							} else if(po.rpn && f->args() == 1 && str_index > 0 && str[str_index - 1] != LEFT_PARENTHESIS_CH && (str_index + name_length >= str.length() || str[str_index + name_length] != LEFT_PARENTHESIS_CH) && (i6 = str.find_last_not_of(SPACE, str_index - 1)) != string::npos) {
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
								if(f == f_vector) stmp += i2s(parseAddVectorId(stmp2, po));
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
											else if(i5 == 2 && po.parsing_mode == PARSING_MODE_CONVENTIONAL && !b_power_before) b = true;
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
										} else if(!b_comma_before && i5 == 2 && arg_i <= 1 && is_in(OPERATORS INTERNAL_OPERATORS, c) && c != POWER_CH) {
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
									if(f == f_vector) {
										stmp += i2s(parseAddVectorId(stmp2, po));
									} else if((f == f_interval || f == f_uncertainty) && po.read_precision != DONT_READ_PRECISION) {
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
									if(f == f_vector) {
										stmp += i2s(parseAddVectorId(stmp2, po));
									} else if((f == f_interval || f == f_uncertainty) && po.read_precision != DONT_READ_PRECISION) {
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
							if(str.length() > str_index + name_length && is_in("23", str[str_index + name_length]) && (str.length() == str_index + name_length + 1 || is_not_in(NUMBER_ELEMENTS, str[str_index + name_length + 1])) && *name != SIGN_DEGREE && !((Unit*) object)->isCurrency()) {
								str.insert(str_index + name_length, 1, POWER_CH);
							}
							stmp = LEFT_PARENTHESIS ID_WRAP_LEFT;
							stmp += i2s(addId(new MathStructure((Unit*) object, p)));
							stmp += ID_WRAP_RIGHT RIGHT_PARENTHESIS;
							str.replace(str_index, name_length, stmp);
							str_index += stmp.length();
							moved_forward = true;
							p = NULL;
							break;
						}
						case 'p': {}
						case 'q': {}
						case 'P': {
							if(str_index + name_length == str.length() || is_in(NOT_IN_NAMES INTERNAL_OPERATORS, str[str_index + name_length])) {
								break;
							}
							p = (Prefix*) object;
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

	if(po.rpn) {
		size_t rpn_i = str.find(SPACE, 0);
		while(rpn_i != string::npos) {
			if(rpn_i == 0 || rpn_i + 1 == str.length() || is_in("~+-*/^\a\b\\", str[rpn_i - 1]) || (is_in("%&|", str[rpn_i - 1]) && str[rpn_i + 1] != str[rpn_i - 1]) || (is_in("!><=", str[rpn_i - 1]) && is_not_in("=<>", str[rpn_i + 1])) || (is_in(SPACE OPERATORS INTERNAL_OPERATORS, str[rpn_i + 1]) && (str[rpn_i - 1] == SPACE_CH || (str[rpn_i - 1] != str[rpn_i + 1] && is_not_in("!><=", str[rpn_i - 1]))))) {
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

#define BASE_2_10 ((po.base >= 2 && po.base <= 10) || (po.base < BASE_CUSTOM && po.base != BASE_UNICODE) || (po.base == BASE_CUSTOM && priv->custom_input_base_i <= 10))

bool Calculator::parseNumber(MathStructure *mstruct, string str, const ParseOptions &po) {
	mstruct->clear();
	if(str.empty()) return false;
	if(str.find_first_not_of(OPERATORS "\a%" SPACE) == string::npos && (po.base != BASE_ROMAN_NUMERALS || str.find("|") == string::npos)) {
		gsub("\a", str.find_first_of("%" OPERATORS) != string::npos ? " xor " : "xor", str);
		error(false, _("Misplaced operator(s) \"%s\" ignored"), str.c_str(), NULL);
		return false;
	}
	int minus_count = 0;
	bool has_sign = false, had_non_sign = false, b_dot = false, b_exp = false, after_sign_e = false;
	int i_colon = 0;
	size_t i = 0;

	while(i < str.length()) {
		if(!had_non_sign && str[i] == MINUS_CH) {
			has_sign = true;
			minus_count++;
			str.erase(i, 1);
		} else if(!had_non_sign && str[i] == PLUS_CH) {
			has_sign = true;
			str.erase(i, 1);
		} else if(str[i] == SPACE_CH) {
			str.erase(i, 1);
		} else if(!b_exp && BASE_2_10 && (str[i] == EXP_CH || str[i] == EXP2_CH)) {
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
			i_colon++;
			had_non_sign = true;
			after_sign_e = false;
			i++;
		} else if(str[i] == COMMA_CH && DOT_S == ".") {
			str.erase(i, 1);
			after_sign_e = false;
			had_non_sign = true;
		} else if(is_in(OPERATORS, str[i]) && (po.base != BASE_ROMAN_NUMERALS || (str[i] != '(' && str[i] != ')' && str[i] != '|'))) {
			error(false, _("Misplaced '%c' ignored"), str[i], NULL);
			str.erase(i, 1);
		} else if(str[i] == '\a') {
			error(false, _("Misplaced operator(s) \"%s\" ignored"), "xor", NULL);
			str.erase(i, 1);
		} else if(str[i] == '\b') {
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
	size_t itmp;
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
		} else {
			string stmp = str.substr(itmp, str.length() - itmp);
			error(true, _("Trailing characters \"%s\" (not a valid variable/function/unit) in number \"%s\" was ignored."), stmp.c_str(), str.c_str(), NULL);
			str.erase(itmp, str.length() - itmp);
		}
	}
	gsub("\b", "±", str);
	Number nr(str, po);
	if(!po.preserve_format && minus_count % 2 == 1) {
		nr.negate();
	}
	if(i_colon && nr.isRational() && !nr.isInteger()) {
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
	if(po.preserve_format) {
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
		if(i == string::npos && str[0] != LOGICAL_NOT_CH && str[0] != BITWISE_NOT_CH && !(str[0] == ID_WRAP_LEFT_CH && str.find(ID_WRAP_RIGHT) < str.length() - 1)) {
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
		if(i == string::npos && str[0] != LOGICAL_NOT_CH && str[0] != BITWISE_NOT_CH && !(str[0] == ID_WRAP_LEFT_CH && str.find(ID_WRAP_RIGHT) < str.length() - 1)) {
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
		(*i_neg)++;
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
			if(po.rpn) {
				str.insert(i2 + 1, MULTIPLICATION);	
				str.insert(i, SPACE);
				i++;
				i2++;		
			}
		}
		if(i2 + 1 < str.length() && is_not_in(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS PARENTHESISS SPACE, str[i2 + 1]) && (!BASE_2_10 || (str[i2 + 1] != EXP_CH && str[i2 + 1] != EXP2_CH))) {
			if(po.rpn) {
				i3 = str.find(SPACE, i2 + 1);
				if(i3 == string::npos) {
					str += MULTIPLICATION;
				} else {
					str.replace(i3, 1, MULTIPLICATION);
				}
				str.insert(i2 + 1, SPACE);
			}
		}
		if(po.rpn && i > 0 && i2 + 1 == str.length() && is_not_in(PARENTHESISS SPACE, str[i - 1])) {
			str += MULTIPLICATION_CH;
		}
		str2 = str.substr(i + 1, i2 - (i + 1));
		MathStructure *mstruct2 = new MathStructure();
		if(str2.empty()) {
			error(false, "Empty expression in parentheses interpreted as zero.", NULL);
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
	if(!po.rpn) {
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
	if(po.rpn) {
		i = 0;
		i3 = 0;
		ParseOptions po2 = po;
		po2.rpn = false;
		vector<MathStructure*> mstack;
		bool b = false;
		char last_operator = 0;
		char last_operator2 = 0;
		while(true) {
			i = str.find_first_of(OPERATORS "\a%" SPACE "\\", i3 + 1);
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
										mstack[mstack.size() - 2]->transform(CALCULATOR->f_shift);
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
										mstack[mstack.size() - 2]->transform(CALCULATOR->f_shift);
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
					} else if(str[i] == BITWISE_NOT_CH) {
						mstack.back()->transform(STRUCT_BITWISE_NOT); 
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
								mstack[mstack.size() - 2]->transform(CALCULATOR->f_shift);
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
								mstack[mstack.size() - 2]->transform(CALCULATOR->f_shift);
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
	if(po.rpn) remove_blanks(str);
	i = 0;
	i3 = 0;
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
	if(po.base != BASE_ROMAN_NUMERALS && (i = str.find(BITWISE_OR, 1)) != string::npos && i + 1 != str.length()) {
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
	if((i = str.find('\a', 1)) != string::npos && i + 1 != str.length()) {
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_BITWISE_XOR);
		return true;
	}
	if((i = str.find(BITWISE_AND, 1)) != string::npos && i + 1 != str.length()) {
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

	if((i = str.find_first_of(PLUS MINUS, 1)) != string::npos && i + 1 != str.length()) {
		bool b = false, c = false, append = false;
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
					if(b_add) {
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
				if(b_add) {
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
	if(!po.rpn && po.parsing_mode == PARSING_MODE_ADAPTIVE && (i = str.find(DIVISION_CH, 1)) != string::npos && i + 1 != str.length()) {
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
	if(po.parsing_mode == PARSING_MODE_ADAPTIVE && !po.rpn) remove_blanks(str);
	if(po.parsing_mode == PARSING_MODE_CONVENTIONAL) {
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
	if((i = str.find_first_of(MULTIPLICATION DIVISION "%", 0)) != string::npos && i + 1 != str.length()) {
		bool b = false, append = false;
		int type = 0;
		while(i != string::npos && i + 1 != str.length()) {
			if(i < 1) {
				if(i < 1 && str.find_first_not_of(MULTIPLICATION_2 OPERATORS INTERNAL_OPERATORS EXPS) == string::npos) {
					gsub("\a", str.find_first_of(OPERATORS "%") != string::npos ? " xor " : "xor", str);
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

	if(str.empty()) return false;
	if(str.find_first_not_of(OPERATORS INTERNAL_OPERATORS SPACE) == string::npos && (po.base != BASE_ROMAN_NUMERALS || str.find_first_of("(|)") == string::npos)) {
		gsub("\a", str.find_first_of(OPERATORS "%") != string::npos ? " xor " : "xor", str);
		error(false, _("Misplaced operator(s) \"%s\" ignored"), str.c_str(), NULL);
		return false;
	}
	
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
			else error(false, _("Misplaced '%c' ignored"), str[i], NULL);
			str.erase(i, 1);
		} else {
			break;
		}
	}
	
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
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_RAISE);
	} else if((i = str.find("\b", 1)) != string::npos && i + 1 != str.length()) {
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
	} else if(BASE_2_10 && (i = str.find_first_of(EXPS, 1)) != string::npos && i + 1 != str.length() && str.find("\b") == string::npos) {
		str2 = str.substr(0, i);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_EXP10);
	} else if((i = str.find(ID_WRAP_LEFT_CH, 1)) != string::npos && i + 1 != str.length() && str.find(ID_WRAP_RIGHT_CH, i + 1) && str.find_first_not_of(PLUS MINUS, 0) != i) {
		str2 = str.substr(0, i);
		str = str.substr(i, str.length() - i);
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_MULTIPLY);
	} else if(str.length() > 0 && str[0] == ID_WRAP_LEFT_CH && (i = str.find(ID_WRAP_RIGHT_CH, 1)) != string::npos && i + 1 != str.length()) {
		str2 = str.substr(0, i + 1);
		str = str.substr(i + 1, str.length() - (i + 1));
		parseAdd(str2, mstruct, po);
		parseAdd(str, mstruct, po, OPERATION_MULTIPLY);
	} else {
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

string Calculator::getName(string name, ExpressionItem *object, bool force, bool always_append) {
	ExpressionItem *item = NULL;
	if(!object) {
	} else if(object->type() == TYPE_FUNCTION) {
		item = getActiveFunction(name);
	} else {
		item = getActiveVariable(name);
		if(!item) {
			item = getActiveUnit(name);
		}
		if(!item) {
			item = getCompositeUnit(name);
		}
	}
	if(item && force && !name.empty() && item != object && object) {
		if(!item->isLocal()) {
			bool b = item->hasChanged();
			if(object->isActive()) {
				item->setActive(false);
			}
			if(!object->isLocal()) {
				item->setChanged(b);
			}
		} else {
			if(object->isActive()) {
				item->destroy();
			}
		}
		return name;
	}
	int i2 = 1;
	bool changed = false;
	if(name.empty()) {
		name = "var";
		always_append = true;
		item = NULL;
		changed = true;
	}
	string stmp = name;
	if(always_append) {
		stmp += NAME_NUMBER_PRE_STR;
		stmp += "1";
	}
	if(changed || (item && item != object)) {
		if(item) {
			i2++;
			stmp = name;
			stmp += NAME_NUMBER_PRE_STR;
			stmp += i2s(i2);
		}
		while(true) {
			if(!object) {
				item = getActiveFunction(stmp);
				if(!item) {
					item = getActiveVariable(stmp);
				}
				if(!item) {
					item = getActiveUnit(stmp);
				}
				if(!item) {
					item = getCompositeUnit(stmp);
				}
			} else if(object->type() == TYPE_FUNCTION) {
				item = getActiveFunction(stmp);
			} else {
				item = getActiveVariable(stmp);
				if(!item) {
					item = getActiveUnit(stmp);
				}
				if(!item) {
					item = getCompositeUnit(stmp);
				}
			}
			if(item && item != object) {
				i2++;
				stmp = name;
				stmp += NAME_NUMBER_PRE_STR;
				stmp += i2s(i2);
			} else {
				break;
			}
		}
	}
	if(i2 > 1 && !always_append) {
		error(false, _("Name \"%s\" is in use. Replacing with \"%s\"."), name.c_str(), stmp.c_str(), NULL);
	}
	return stmp;
}

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
bool Calculator::loadLocalDefinitions() {
	string homedir = buildPath(getLocalDataDir(), "definitions");
	if(!dirExists(homedir)) {
		string homedir_old = buildPath(getOldLocalDir(), "definitions");
		if(dirExists(homedir)) {
			if(!dirExists(getLocalDataDir())) {
				recursiveMakeDir(getLocalDataDir());
			}
			if(makeDir(homedir)) {
				list<string> eps_old;
				struct dirent *ep_old;	
				DIR *dp_old = opendir(homedir_old.c_str());
				if(dp_old) {
					while((ep_old = readdir(dp_old))) {
#ifdef _DIRENT_HAVE_D_TYPE
						if(ep_old->d_type != DT_DIR) {
#endif
							if(strcmp(ep_old->d_name, "..") != 0 && strcmp(ep_old->d_name, ".") != 0 && strcmp(ep_old->d_name, "datasets") != 0) {
								eps_old.push_back(ep_old->d_name);
							}
#ifdef _DIRENT_HAVE_D_TYPE			
						}
#endif
					}
					closedir(dp_old);
				}
				for(list<string>::iterator it = eps_old.begin(); it != eps_old.end(); ++it) {	
					move_file(buildPath(homedir_old, *it).c_str(), buildPath(homedir, *it).c_str());
				}
				if(removeDir(homedir_old)) {
					removeDir(getOldLocalDir());
				}
			}
		}
	}
	list<string> eps;
	struct dirent *ep;	
	DIR *dp = opendir(homedir.c_str());
	if(dp) {
		while((ep = readdir(dp))) {
#ifdef _DIRENT_HAVE_D_TYPE
			if(ep->d_type != DT_DIR) {
#endif
				if(strcmp(ep->d_name, "..") != 0 && strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "datasets") != 0) {
					eps.push_back(ep->d_name);
				}
#ifdef _DIRENT_HAVE_D_TYPE			
			}
#endif
		}
		closedir(dp);
	}
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

#define ITEM_SET_BEST_NAMES(validation) \
	size_t names_i = 0, i2 = 0; \
	string *str_names; \
	if(best_names == "-") {best_names = ""; nextbest_names = "";} \
	if(!best_names.empty()) {str_names = &best_names;} \
	else if(!nextbest_names.empty()) {str_names = &nextbest_names;} \
	else {str_names = &default_names;} \
	if(!str_names->empty() && (*str_names)[0] == '!') { \
		names_i = str_names->find('!', 1) + 1; \
	} \
	while(true) { \
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
			item->addName(ename); \
		} \
		if(names_i == string::npos) {break;} \
		names_i++; \
	}

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

#define ITEM_SET_REFERENCE_NAMES(validation) \
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
				remove_blank_ends(ename.name); \
				size_t i4 = item->hasName(ename.name, ename.case_sensitive); \
				if(i4 > 0) { \
					const ExpressionName *enameptr = &item->getName(i4); \
					ename.suffix = enameptr->suffix; \
					ename.abbreviation = enameptr->abbreviation; \
					ename.avoid_input = enameptr->avoid_input; \
					ename.completion_only = enameptr->completion_only; \
					ename.plural = enameptr->plural; \
					ename.case_sensitive = enameptr->case_sensitive; \
					item->setName(ename, i4); \
				} else if(!ename.name.empty() && validation(ename.name, version_numbers, is_user_defs)) { \
					if(!case_set) { \
						ename.case_sensitive = ename.abbreviation || text_length_is_one(ename.name); \
					} \
					item->addName(ename); \
				} \
			} \
			if(names_i == string::npos) {break;} \
			names_i++; \
		} \
	}


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
					hidden = false;\
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
					item->setDescription(description);\
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
					item->setHidden(hidden);

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
	bool best_title, next_best_title, best_category_title, next_best_category_title, best_description, next_best_description;
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
	WCHAR wlocale[LOCALE_NAME_MAX_LENGTH];
	if(LCIDToLocaleName(LOCALE_USER_DEFAULT, wlocale, LOCALE_NAME_MAX_LENGTH, 0) != 0) locale = utf8_encode(wlocale);
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
	bool active = false, hidden = false, b = false, require_translation = false, use_with_prefixes = false, use_with_prefixes_set = false;
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
	doc = xmlParseFile(file_name);
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
	int version_numbers[] = {3, 3, 0};
	parse_qalculate_version(version, version_numbers);

	bool new_names = version_numbers[0] > 0 || version_numbers[1] > 9 || (version_numbers[1] == 9 && version_numbers[2] >= 4);
	
	ParseOptions po;

	vector<xmlNodePtr> unfinished_nodes;
	vector<string> unfinished_cats;
	queue<xmlNodePtr> sub_items;
	vector<queue<xmlNodePtr> > nodes;

	category = "";
	nodes.resize(1);
	
	Unit *u_usd = getUnit("USD");

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
				if(VERSION_BEFORE(0, 6, 3)) {
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
									XML_GET_FALSE_FROM_PROP(child, "include_equals", b)
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
									XML_GET_FALSE_FROM_PROP(child, "include_equals", b)
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
							} else if(itmp <= f->maxargs() || itmp <= f->minargs()) {
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
				if(VERSION_BEFORE(0, 6, 3)) {
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
				if(VERSION_BEFORE(0, 6, 3)) {
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
					if(VERSION_BEFORE(0, 6, 3)) {
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
					ITEM_INIT_DTH
					ITEM_INIT_NAME
					while(child != NULL) {
						if(!xmlStrcmp(child->name, (const xmlChar*) "system")) {
							XML_DO_FROM_TEXT(child, u->setSystem)
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "use_with_prefixes")) {
							XML_GET_TRUE_FROM_TEXT(child, use_with_prefixes)
							use_with_prefixes_set = true;
						} else if((VERSION_BEFORE(0, 6, 3)) && !xmlStrcmp(child->name, (const xmlChar*) "singular")) {
							XML_GET_LOCALE_STRING_FROM_TEXT(child, singular, best_singular, next_best_singular)
							if(!unitNameIsValid(singular, version_numbers, is_user_defs)) {
								singular = "";
							}
						} else if((VERSION_BEFORE(0, 6, 3)) && !xmlStrcmp(child->name, (const xmlChar*) "plural") && !xmlGetProp(child, (xmlChar*) "index")) {
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
						addUnit(u, true, is_user_defs);
						u->setChanged(false);
					}
					done_something = true;
				} else if(type == "alias") {
					if(VERSION_BEFORE(0, 6, 3)) {
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
					bool b_currency = false;
					use_with_prefixes_set = false;
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
									u = getUnit(base);
									b_currency = (!is_user_defs && u && u == u_euro);
									if(!u) {
										u = getCompositeUnit(base);
									}
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
							use_with_prefixes_set = true;
						} else if((VERSION_BEFORE(0, 6, 3)) && !xmlStrcmp(child->name, (const xmlChar*) "singular")) {	
							XML_GET_LOCALE_STRING_FROM_TEXT(child, singular, best_singular, next_best_singular)
							if(!unitNameIsValid(singular, version_numbers, is_user_defs)) {
								singular = "";
							}
						} else if((VERSION_BEFORE(0, 6, 3)) && !xmlStrcmp(child->name, (const xmlChar*) "plural") && !xmlGetProp(child, (xmlChar*) "index")) {	
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
						au->setHidden(hidden);
						au->setSystem(usystem);
						if(use_with_prefixes_set) {
							au->setUseWithPrefixesByDefault(use_with_prefixes);
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
						if(au->countNames() == 0) {
							au->destroy();
							au = NULL;
						} else {
							if(!is_user_defs && au->baseUnit() == u_second) {
								if(au->referenceName() == "d" || au->referenceName() == "day") u_day = au;
								else if(au->referenceName() == "year") u_year = au;
								else if(au->referenceName() == "month") u_month = au;
								else if(au->referenceName() == "min") u_minute = au;
								else if(au->referenceName() == "h") u_hour = au;
							}
							addUnit(au, true, is_user_defs);
							au->setChanged(false);
						}
						done_something = true;	
					}
				} else if(type == "composite") {
					if(VERSION_BEFORE(0, 6, 3)) {
						XML_GET_STRING_FROM_PROP(cur, "name", name)
					} else {
						name = "";
					}
					XML_GET_FALSE_FROM_PROP(cur, "active", active)
					child = cur->xmlChildrenNode;
					usystem = "";
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
						/*if(use_with_prefixes_set) {
							cu->setUseWithPrefixesByDefault(use_with_prefixes);
						}*/
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
						} else if(!xmlStrcmp(child->name, (const xmlChar*) "plural") && !xmlGetProp(child, (xmlChar*) "index")) {	
							XML_GET_LOCALE_STRING_FROM_TEXT(child, plural, best_plural, next_best_plural)
							if(!unitNameIsValid(plural, version_numbers, is_user_defs)) {
								plural = "";
							}
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
					if(u_usd && u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit() == u_usd) u->setHidden(true);
					u->setChanged(false);
					done_something = true;
				}
			} else if(!xmlStrcmp(cur->name, (const xmlChar*) "prefix")) {
				child = cur->xmlChildrenNode;
				XML_GET_STRING_FROM_PROP(cur, "type", type)
				uname = ""; sexp = ""; svalue = ""; name = "";
				bool b_best = false;
				while(child != NULL) {
					if(!xmlStrcmp(child->name, (const xmlChar*) "name")) {
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
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "abbreviation")) {
						XML_GET_STRING_FROM_TEXT(child, stmp);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "unicode")) {
						XML_GET_STRING_FROM_TEXT(child, uname);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "exponent")) {
						XML_GET_STRING_FROM_TEXT(child, sexp);
					} else if(!xmlStrcmp(child->name, (const xmlChar*) "value")) {
						XML_GET_STRING_FROM_TEXT(child, svalue);
					}
					child = child->next;
				}
				if(type == "decimal") {
					addPrefix(new DecimalPrefix(s2i(sexp), name, stmp, uname));
				} else if(type == "number") {
					addPrefix(new NumberPrefix(svalue, name, stmp, uname));
				} else if(type == "binary") {
					addPrefix(new BinaryPrefix(s2i(sexp), name, stmp, uname));
				} else {
					if(svalue.empty()) {
						addPrefix(new DecimalPrefix(s2i(sexp), name, stmp, uname));
					} else {
						addPrefix(new NumberPrefix(svalue, name, stmp, uname));
					}
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

int Calculator::savePrefixes(const char* file_name, bool save_global) {
	if(!save_global) {
		return true;
	}
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");	
	xmlNodePtr cur, newnode;	
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);	
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	cur = doc->children;
	for(size_t i = 0; i < prefixes.size(); i++) {
		newnode = xmlNewTextChild(cur, NULL, (xmlChar*) "prefix", NULL);
		if(!prefixes[i]->longName(false).empty()) xmlNewTextChild(newnode, NULL, (xmlChar*) "name", (xmlChar*) prefixes[i]->longName(false).c_str());
		if(!prefixes[i]->shortName(false).empty()) xmlNewTextChild(newnode, NULL, (xmlChar*) "abbreviation", (xmlChar*) prefixes[i]->shortName(false).c_str());
		if(!prefixes[i]->unicodeName(false).empty()) xmlNewTextChild(newnode, NULL, (xmlChar*) "unicode", (xmlChar*) prefixes[i]->unicodeName(false).c_str());
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

string Calculator::temporaryCategory() const {
	return _("Temporary");
}

int Calculator::saveVariables(const char* file_name, bool save_global) {
	string str;
	const ExpressionName *ename;
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");	
	xmlNodePtr cur, newnode, newnode2;	
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);	
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
	node_tree_item top;
	top.category = "";
	top.node = doc->children;
	node_tree_item *item;
	string cat, cat_sub;
	for(size_t i = 0; i < variables.size(); i++) {
		if((save_global || variables[i]->isLocal() || variables[i]->hasChanged()) && variables[i]->category() != _("Temporary") && variables[i]->category() != "Temporary") {
			item = &top;
			if(!variables[i]->category().empty()) {
				cat = variables[i]->category();
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
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}

int Calculator::saveUnits(const char* file_name, bool save_global) {
	string str;
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");	
	xmlNodePtr cur, newnode, newnode2, newnode3;	
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*) "QALCULATE", NULL);	
	xmlNewProp(doc->children, (xmlChar*) "version", (xmlChar*) VERSION);
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
		if(save_global || u->isLocal() || u->hasChanged()) {
			item = &top;
			if(!u->category().empty()) {
				cat = u->category();
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
				if(!u->isSIUnit() || !u->useWithPrefixesByDefault()) {
					xmlNewTextChild(newnode, NULL, (xmlChar*) "use_with_prefixes", u->useWithPrefixesByDefault() ? (xmlChar*) "true" : (xmlChar*) "false");
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
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
}

int Calculator::saveFunctions(const char* file_name, bool save_global) {
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
	IntegerArgument *iarg;
	NumberArgument *farg;
	string str;
	for(size_t i = 0; i < functions.size(); i++) {
		if(functions[i]->subtype() != SUBTYPE_DATA_SET && (save_global || functions[i]->isLocal() || functions[i]->hasChanged())) {
			item = &top;
			if(!functions[i]->category().empty()) {
				cat = functions[i]->category();
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
	int returnvalue = xmlSaveFormatFile(file_name, doc, 1);
	xmlFreeDoc(doc);
	return returnvalue;
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
int Calculator::testCondition(string expression) {
	MathStructure mstruct = calculate(expression);
	if(mstruct.isNumber()) {
		if(mstruct.number().isPositive()) {
			return 1;
		} else {
			return 0;
		}
	}
	return -1;
}

void Calculator::startPrintControl(int milli_timeout) {
	startControl(milli_timeout);
}
void Calculator::abortPrint() {
	abort();
}
bool Calculator::printingAborted() {
	return aborted();
}
string Calculator::printingAbortedMessage() const {
	return abortedMessage();
}
string Calculator::timedOutString() const {
	return _("timed out");
}
bool Calculator::printingControlled() const {
	return isControlled();
}
void Calculator::stopPrintControl() {
	stopControl();
}

void Calculator::startControl(int milli_timeout) {
	b_controlled = true;
	i_aborted = 0;
	i_timeout = milli_timeout;
	if(i_timeout > 0) {
#ifndef CLOCK_MONOTONIC
		gettimeofday(&t_end, NULL);
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		t_end.tv_sec = ts.tv_sec;
		t_end.tv_usec = ts.tv_nsec / 1000;
#endif
		long int usecs = t_end.tv_usec + (long int) milli_timeout * 1000;
		t_end.tv_usec = usecs % 1000000;
		t_end.tv_sec += usecs / 1000000;
	}
}
bool Calculator::aborted() {
	if(!b_controlled) return false;
	if(i_aborted > 0) return true;
	if(i_timeout > 0) {
#ifndef CLOCK_MONOTONIC
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if(tv.tv_sec > t_end.tv_sec || (tv.tv_sec == t_end.tv_sec && tv.tv_usec > t_end.tv_usec)) {
#else
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		if(tv.tv_sec > t_end.tv_sec || (tv.tv_sec == t_end.tv_sec && tv.tv_nsec / 1000 > t_end.tv_usec)) {
#endif
			i_aborted = 2;
			return true;
		}
	}
	return false;
}
string Calculator::abortedMessage() const {
	if(i_aborted == 2) return _("timed out");
	return _("aborted");
}
bool Calculator::isControlled() const {
	return b_controlled;
}
void Calculator::stopControl() {
	b_controlled = false;
	i_aborted = 0;
	i_timeout = 0;
}


bool Calculator::loadExchangeRates() {
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *value;
	bool global_file = false;
	string currency, rate, sdate;
	string filename = buildPath(getLocalDataDir(), "eurofxref-daily.xml");
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
		filename = buildPath(getGlobalDefinitionsDir(), "eurofxref-daily.xml");
		doc = xmlParseFile(filename.c_str());
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
						((AliasUnit*) u)->setExpression(rate);
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
	
	filename = buildPath(getLocalDataDir(), "btc.json");
	ifstream file2(filename.c_str());
	if(file2.is_open()) {
		std::stringstream ssbuffer2;
		ssbuffer2 << file2.rdbuf();
		string sbuffer = ssbuffer2.str();
		size_t i = sbuffer.find("\"amount\":");
		if(i != string::npos) {
			i = sbuffer.find("\"", i + 9);
			if(i != string::npos) {
				size_t i2 = sbuffer.find("\"", i + 1);
				((AliasUnit*) u_btc)->setExpression(sbuffer.substr(i + 1, i2 - (i + 1)));
			}
		}
		file2.close();
		struct stat stats;
		if(stat(filename.c_str(), &stats) == 0) {
			exchange_rates_time[1] = stats.st_mtime;
			if(exchange_rates_time[1] > exchange_rates_check_time[1]) exchange_rates_check_time[1] = exchange_rates_time[1];
		}
	} else {
		exchange_rates_time[1] = ((time_t) 1531087L) * 1000;
		if(exchange_rates_time[1] > exchange_rates_check_time[1]) exchange_rates_check_time[1] = exchange_rates_time[1];
	}
	
	Unit *u_usd = getUnit("USD");
	if(!u_usd) return true;
	
	string sbuffer;
	filename = buildPath(getLocalDataDir(), "rates.html");
	ifstream file(filename.c_str());
	if(file.is_open()) {
		std::stringstream ssbuffer;
		ssbuffer << file.rdbuf();
		sbuffer = ssbuffer.str();
	}
	if(sbuffer.empty()) {
		if(file.is_open()) file.close();
		file.clear();
		filename = buildPath(getGlobalDefinitionsDir(), "rates.json");
		file.open(filename.c_str());
		if(!file.is_open()) return true;
		std::stringstream ssbuffer;
		ssbuffer << file.rdbuf();
		sbuffer = ssbuffer.str();
		string sname;
		size_t i = sbuffer.find("\"currency_code\":");
		while(i != string::npos) {
			i += 16;
			size_t i2 = sbuffer.find("\"", i);
			if(i2 == string::npos) break;
			size_t i3 = sbuffer.find("\"", i2 + 1);
			if(i3 != string::npos && i3 - (i2 + 1) == 3) {
				currency = sbuffer.substr(i2 + 1, i3 - (i2 + 1));
				if(currency.length() == 3 && currency[0] >= 'A' && currency[0] <= 'Z') {
					u = getUnit(currency);
					if(!u || (u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit() == u_usd)) {
						i2 = sbuffer.find("\"rate\":", i3 + 1);
						size_t i4 = sbuffer.find("}", i3 + 1);
						if(i2 != string::npos && i2 < i4) {
							i3 = sbuffer.find(",", i2 + 7);
							rate = sbuffer.substr(i2 + 7, i3 - (i2 + 7));
							rate = "1/" + rate;
							if(!u) {
								i2 = sbuffer.find("\"name\":\"", i3 + 1);
								if(i2 != string::npos && i2 < i4) {
									i3 = sbuffer.find("\"", i2 + 8);
									if(i3 != string::npos) {
										sname = sbuffer.substr(i2 + 8, i3 - (i2 + 8));
										remove_blank_ends(sname);
									}
								} else {
									sname = "";
								}
								u = addUnit(new AliasUnit(_("Currency"), currency, "", "", sname, u_usd, rate, 1, "", false, true), false, true);
								if(u) u->setHidden(true);
							} else {
								((AliasUnit*) u)->setBaseUnit(u_usd);
								((AliasUnit*) u)->setExpression(rate);
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
			i = sbuffer.find("\"currency_code\":", i);
		}
		file.close();
		exchange_rates_time[2] = ((time_t) 1527199L) * 1000;
		if(exchange_rates_time[2] > exchange_rates_check_time[2]) exchange_rates_check_time[2] = exchange_rates_time[2];
	} else {
		string sname;
		size_t i = sbuffer.find("class=\'country\'");
		while(i != string::npos) {
			currency = ""; sname = ""; rate = "";
			i += 15;
			size_t i2 = sbuffer.find("data-currency-code=\"", i);
			if(i2 != string::npos) {
				i2 += 19;
				size_t i3 = sbuffer.find("\"", i2 + 1);
				if(i3 != string::npos) {
					currency = sbuffer.substr(i2 + 1, i3 - (i2 + 1));
					remove_blank_ends(currency);
				}
			}
			i2 = sbuffer.find("data-currency-name=\'", i);
			if(i2 != string::npos) {
				i2 += 19;
				size_t i3 = sbuffer.find("|", i2 + 1);
				if(i3 != string::npos) {
					sname = sbuffer.substr(i2 + 1, i3 - (i2 + 1));
					remove_blank_ends(sname);
				}
			}
			i2 = sbuffer.find("data-rate=\'", i);
			if(i2 != string::npos) {
				i2 += 10;
				size_t i3 = sbuffer.find("'", i2 + 1);
				if(i3 != string::npos) {
					rate = sbuffer.substr(i2 + 1, i3 - (i2 + 1));
					remove_blank_ends(rate);
				}
			}
			if(currency.length() == 3 && currency[0] >= 'A' && currency[0] <= 'Z' && !rate.empty()) {
				u = getUnit(currency);
				if(!u || (u->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) u)->firstBaseUnit() == u_usd)) {
					rate = "1/" + rate;
					if(!u) {
						u = addUnit(new AliasUnit(_("Currency"), currency, "", "", sname, u_usd, rate, 1, "", false, true), false, true);
						if(u) u->setHidden(true);
					} else {
						((AliasUnit*) u)->setBaseUnit(u_usd);
						((AliasUnit*) u)->setExpression(rate);
					}
					if(u) {
						u->setApproximate();
						u->setPrecision(-2);
						u->setChanged(false);
					}
				}
			}
			i = sbuffer.find("class=\'country\'", i);
		}
		file.close();
		struct stat stats;
		if(stat(filename.c_str(), &stats) == 0) {
			exchange_rates_time[2] = stats.st_mtime;
			if(exchange_rates_time[2] > exchange_rates_check_time[2]) exchange_rates_check_time[2] = exchange_rates_time[2];
		}
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
		//case 3: {return buildPath(getLocalDataDir(), "rates.json");}
		case 3: {return buildPath(getLocalDataDir(), "rates.html");}
		default: {}
	}
	return "";
}
time_t Calculator::getExchangeRatesTime(int index) {
	if(index > 3) return 0;
	if(index < 1) {
		if(exchange_rates_time[1] < exchange_rates_time[0]) {
			if(exchange_rates_time[2] < exchange_rates_time[1]) return exchange_rates_time[2];
			return exchange_rates_time[1];
		}
		if(exchange_rates_time[2] < exchange_rates_time[0]) return exchange_rates_time[2];
		return exchange_rates_time[0];
	}
	index--;
	return exchange_rates_time[index];
}
string Calculator::getExchangeRatesUrl(int index) {
	switch(index) {
		case 1: {return "https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml";}
		case 2: {return "https://api.coinbase.com/v2/prices/spot?currency=EUR";}
		//case 2: {return "http://www.mycurrency.net/service/rates";}
		case 3: {return "https://www.mycurrency.net/=US";}
		default: {}
	}
	return "";
}
bool Calculator::fetchExchangeRates(int timeout, string) {return fetchExchangeRates(timeout);}
size_t write_data(void *ptr, size_t size, size_t nmemb, string *sbuffer) {
	sbuffer->append((char*) ptr, size * nmemb);
	return size * nmemb;
}
#define FETCH_FAIL_CLEANUP curl_easy_cleanup(curl); curl_global_cleanup(); time(&exchange_rates_check_time[0]); time(&exchange_rates_check_time[1]); time(&exchange_rates_check_time[2]);
bool Calculator::fetchExchangeRates(int timeout, int n) {
#ifdef HAVE_LIBCURL
	if(n <= 0) n = 3;
	
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
		if(strlen(error_buffer)) error(true, _("Failed to download exchange rates from %s: %s."), "ECB", error_buffer, NULL);
		else error(true, _("Failed to download exchange rates from %s: %s."), "ECB", curl_easy_strerror(res), NULL);
		FETCH_FAIL_CLEANUP;
		return false;
	}
	if(sbuffer.empty()) {error(true, _("Failed to download exchange rates from %s: %s."), "ECB", "Document empty", NULL); FETCH_FAIL_CLEANUP; return false;}
	ofstream file(getExchangeRatesFileName(1).c_str(), ios::out | ios::trunc | ios::binary);
	if(!file.is_open()) {
		error(true, _("Failed to download exchange rates from %s: %s."), "ECB", strerror(errno), NULL);
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

	if(n >= 2) {

		sbuffer = "";
		curl_easy_setopt(curl, CURLOPT_URL, getExchangeRatesUrl(2).c_str());
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK) {error(true, _("Failed to download exchange rates from %s: %s."), "coinbase.com", error_buffer, NULL); FETCH_FAIL_CLEANUP; return false;}
		if(sbuffer.empty()) {error(true, _("Failed to download exchange rates from %s: %s."), "coinbase.com", "Document empty", NULL); FETCH_FAIL_CLEANUP; return false;}
		ofstream file3(getExchangeRatesFileName(2).c_str(), ios::out | ios::trunc | ios::binary);
		if(!file3.is_open()) {
			error(true, _("Failed to download exchange rates from %s: %s."), "coinbase.com", strerror(errno), NULL);
			FETCH_FAIL_CLEANUP
			return false;
		}
		file3 << sbuffer;
		file3.close();

	}

	if(n >= 3) {

		sbuffer = "";
		curl_easy_setopt(curl, CURLOPT_URL, getExchangeRatesUrl(3).c_str());
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sbuffer);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
		res = curl_easy_perform(curl);
		
		if(res != CURLE_OK) {error(true, _("Failed to download exchange rates from %s: %s."), "mycurrency.net", error_buffer, NULL); FETCH_FAIL_CLEANUP; return false;}
		if(sbuffer.empty() || sbuffer.find("Internal Server Error") != string::npos) {error(true, _("Failed to download exchange rates from %s: %s."), "mycurrency.net", "Document empty", NULL); FETCH_FAIL_CLEANUP; return false;}
		ofstream file2(getExchangeRatesFileName(3).c_str(), ios::out | ios::trunc | ios::binary);
		if(!file2.is_open()) {
			error(true, _("Failed to download exchange rates from %s: %s."), "mycurrency.net", strerror(errno), NULL);
			FETCH_FAIL_CLEANUP
			return false;
		}
		file2 << sbuffer;
		file2.close();
		
	}
	
	curl_easy_cleanup(curl); curl_global_cleanup(); 
	
	return true;
#else
	return false;
#endif
}
bool Calculator::checkExchangeRatesDate(unsigned int n_days, bool force_check, bool send_warning, int n) {
	if(n <= 0) n = 3;
	time_t extime = exchange_rates_time[0];
	if(n > 1 && exchange_rates_time[1] < extime) extime = exchange_rates_time[1];
	if(n > 2 && exchange_rates_time[2] < extime) extime = exchange_rates_time[2];
	time_t cextime = exchange_rates_check_time[0];
	if(n > 1 && exchange_rates_check_time[1] < cextime) cextime = exchange_rates_check_time[1];
	if(n > 2 && exchange_rates_check_time[2] < cextime) cextime = exchange_rates_check_time[2];
	if(extime > 0 && ((!force_check && cextime > 0 && difftime(time(NULL), cextime) < 86400 * n_days) || difftime(time(NULL), extime) < (86400 * n_days) + 3600)) return true;
	time(&exchange_rates_check_time[0]);
	if(n > 1) time(&exchange_rates_check_time[1]);
	if(n > 2) time(&exchange_rates_check_time[2]);
	if(send_warning) error(false, _("It has been %s day(s) since the exchange rates last were updated."), i2s((int) floor(difftime(time(NULL), extime) / 86400)).c_str(), NULL);
	return false;
}
void Calculator::setExchangeRatesWarningEnabled(bool enable) {
	b_exchange_rates_warning_enabled = enable;
}
bool Calculator::exchangeRatesWarningEnabled() const {
	return b_exchange_rates_warning_enabled;
}
int Calculator::exchangeRatesUsed() const {
	return b_exchange_rates_used;
}
void Calculator::resetExchangeRatesUsed() {
	b_exchange_rates_used = 0;
}
void Calculator::setExchangeRatesUsed(int index) {
	if(index > b_exchange_rates_used) b_exchange_rates_used = index;
	if(b_exchange_rates_warning_enabled) checkExchangeRatesDate(7, false, true);
}

bool Calculator::canPlot() {
#ifdef _WIN32
	LPSTR lpFilePart;
	char filename[MAX_PATH]; 
	return SearchPath(NULL, "gnuplot", ".exe", MAX_PATH, filename, &lpFilePart);
#else
	FILE *pipe = popen("gnuplot - 2>/dev/null", "w");
	if(!pipe) return false;
	return pclose(pipe) == 0;
#endif
}

extern bool fix_intervals(MathStructure &mstruct, const EvaluationOptions &eo, bool *failed = NULL, long int min_precision = 2, bool function_middle = false);
void parse_and_precalculate_plot(string &expression, MathStructure &mstruct, const ParseOptions &po, EvaluationOptions &eo) {
	eo.approximation = APPROXIMATION_APPROXIMATE;
	ParseOptions po2 = po;
	po2.read_precision = DONT_READ_PRECISION;
	eo.parse_options = po2;
	eo.interval_calculation = INTERVAL_CALCULATION_NONE;
	mstruct = CALCULATOR->parse(expression, po2);
	MathStructure mbak(mstruct);
	eo.calculate_functions = false;
	eo.expand = false;
	CALCULATOR->beginTemporaryStopMessages();
	mstruct.eval(eo);
	int im = 0;
	if(CALCULATOR->endTemporaryStopMessages(NULL, &im) > 0 || im > 0) mstruct = mbak;
	eo.calculate_functions = true;
	eo.expand = true;
}

MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure y_vector(mparse.generateVector(x_mstruct, min, max, steps, x_vector, eo));
	endTemporaryStopMessages();
	endTemporaryStopIntervalArithmetic();
	if(msecs > 0) {
		if(aborted()) error(true, _("It took too long to generate the plot data."), NULL);
		stopControl();
	}
	if(y_vector.size() == 0) {
		error(true, _("Unable to generate plot data with current min, max and sampling rate."), NULL);
	}
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, float min, float max, int steps, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	MathStructure min_mstruct(min), max_mstruct(max);
	ParseOptions po2 = po;
	po2.read_precision = DONT_READ_PRECISION;
	MathStructure y_vector(expressionToPlotVector(expression, min_mstruct, max_mstruct, steps, x_vector, x_var, po2, msecs));
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure y_vector(mparse.generateVector(x_mstruct, min, max, step, x_vector, eo));
	endTemporaryStopMessages();
	endTemporaryStopIntervalArithmetic();
	if(msecs > 0) {
		if(aborted()) error(true, _("It took too long to generate the plot data."), NULL);
		stopControl();
	}
	if(y_vector.size() == 0) {
		error(true, _("Unable to generate plot data with current min, max and step size."), NULL);
	}
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, float min, float max, float step, MathStructure *x_vector, string x_var, const ParseOptions &po, int msecs) {
	MathStructure min_mstruct(min), max_mstruct(max), step_mstruct(step);
	ParseOptions po2 = po;
	po2.read_precision = DONT_READ_PRECISION;
	MathStructure y_vector(expressionToPlotVector(expression, min_mstruct, max_mstruct, step_mstruct, x_vector, x_var, po2, msecs));
	return y_vector;
}
MathStructure Calculator::expressionToPlotVector(string expression, const MathStructure &x_vector, string x_var, const ParseOptions &po, int msecs) {
	Variable *v = getActiveVariable(x_var);
	MathStructure x_mstruct;
	if(v) x_mstruct = v;
	else x_mstruct = x_var;
	EvaluationOptions eo;
	MathStructure mparse;
	if(msecs > 0) startControl(msecs);
	beginTemporaryStopIntervalArithmetic();
	parse_and_precalculate_plot(expression, mparse, po, eo);
	beginTemporaryStopMessages();
	MathStructure y_vector(mparse.generateVector(x_mstruct, x_vector, eo).eval(eo));
	endTemporaryStopMessages();
	endTemporaryStopIntervalArithmetic();
	if(msecs > 0) {
		if(aborted()) error(true, _("It took too long to generate the plot data."), NULL);
		stopControl();
	}
	return y_vector;
}

extern bool testComplexZero(const Number *this_nr, const Number *i_nr);

bool Calculator::plotVectors(PlotParameters *param, const vector<MathStructure> &y_vectors, const vector<MathStructure> &x_vectors, vector<PlotDataParameters*> &pdps, bool persistent, int msecs) {

	string homedir = getLocalTmpDir();
	recursiveMakeDir(homedir);

	string commandline_extra;
	string title;

	if(!param) {
		PlotParameters pp;
		param = &pp;
	}
	
	string plot;
	
	if(param->filename.empty()) {
		if(!param->color) {
			commandline_extra += " -mono";
		}
		plot += "set terminal pop\n";
	} else {
		persistent = true;
		if(param->filetype == PLOT_FILETYPE_AUTO) {
			size_t i = param->filename.rfind(".");
			if(i == string::npos) {
				param->filetype = PLOT_FILETYPE_PNG;
				error(false, _("No extension in file name. Saving as PNG image."), NULL);
			} else {
				string ext = param->filename.substr(i + 1, param->filename.length() - (i + 1));
				if(ext == "png") {
					param->filetype = PLOT_FILETYPE_PNG;
				} else if(ext == "ps") {
					param->filetype = PLOT_FILETYPE_PS;
				} else if(ext == "pdf") {
					param->filetype = PLOT_FILETYPE_PDF;
				} else if(ext == "eps") {
					param->filetype = PLOT_FILETYPE_EPS;
				} else if(ext == "svg") {
					param->filetype = PLOT_FILETYPE_SVG;
				} else if(ext == "fig") {
					param->filetype = PLOT_FILETYPE_FIG;
				} else if(ext == "tex") {
					param->filetype = PLOT_FILETYPE_LATEX;
				} else {
					param->filetype = PLOT_FILETYPE_PNG;
					error(false, _("Unknown extension in file name. Saving as PNG image."), NULL);
				}
			}
		}
		plot += "set terminal ";
		switch(param->filetype) {
			case PLOT_FILETYPE_FIG: {
				plot += "fig ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				break;
			}
			case PLOT_FILETYPE_SVG: {
				plot += "svg";
				break;
			}
			case PLOT_FILETYPE_LATEX: {
				plot += "latex ";
				break;
			}
			case PLOT_FILETYPE_PS: {
				plot += "postscript ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				plot += " \"Times\"";
				break;
			}
			case PLOT_FILETYPE_PDF: {
				plot += "pdf ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				break;
			}
			case PLOT_FILETYPE_EPS: {
				plot += "postscript eps ";
				if(param->color) {
					plot += "color";
				} else {
					plot += "monochrome";
				}
				plot += " \"Times\"";
				break;
			}
			default: {
				plot += "png ";
				break;
			}

		}
		plot += "\nset output \"";
		plot += param->filename;
		plot += "\"\n";
	}

	switch(param->legend_placement) {
		case PLOT_LEGEND_NONE: {plot += "set nokey\n"; break;}
		case PLOT_LEGEND_TOP_LEFT: {plot += "set key top left\n"; break;}
		case PLOT_LEGEND_TOP_RIGHT: {plot += "set key top right\n"; break;}
		case PLOT_LEGEND_BOTTOM_LEFT: {plot += "set key bottom left\n"; break;}
		case PLOT_LEGEND_BOTTOM_RIGHT: {plot += "set key bottom right\n"; break;}
		case PLOT_LEGEND_BELOW: {plot += "set key below\n"; break;}
		case PLOT_LEGEND_OUTSIDE: {plot += "set key outside\n"; break;}
	}
	if(!param->x_label.empty()) {
		title = param->x_label;
		gsub("\"", "\\\"", title);
		plot += "set xlabel \"";
		plot += title;
		plot += "\"\n";	
	}
	if(!param->y_label.empty()) {
		string title = param->y_label;
		gsub("\"", "\\\"", title);
		plot += "set ylabel \"";
		plot += title;
		plot += "\"\n";	
	}
	if(!param->title.empty()) {
		title = param->title;
		gsub("\"", "\\\"", title);
		plot += "set title \"";
		plot += title;
		plot += "\"\n";	
	}
	if(param->grid) {
		plot += "set grid\n";
	
	}
	if(!param->auto_y_min || !param->auto_y_max) {
		plot += "set yrange [";
		if(!param->auto_y_min) plot += d2s(param->y_min);
		plot += ":";
		if(!param->auto_y_max) plot += d2s(param->y_max);
		plot += "]";
		plot += "\n";
	}
	if(param->x_log) {
		plot += "set logscale x ";
		plot += i2s(param->x_log_base);
		plot += "\n";
	}
	if(param->show_all_borders) {
		plot += "set border 15\n";
	} else {
		bool xaxis2 = false, yaxis2 = false;
		for(size_t i = 0; i < pdps.size(); i++) {
			if(pdps[i] && pdps[i]->xaxis2) {
				xaxis2 = true;
			}
			if(pdps[i] && pdps[i]->yaxis2) {
				yaxis2 = true;
			}
		}
		if(xaxis2 && yaxis2) {
			plot += "set border 15\nset x2tics\nset y2tics\n";
		} else if(xaxis2) {
			plot += "set border 7\nset x2tics\n";
		} else if(yaxis2) {
			plot += "set border 11\nset y2tics\n";
		} else {
			plot += "set border 3\n";
		}
		plot += "set xtics nomirror\nset ytics nomirror\n";
	}
	size_t samples = 1000;
	for(size_t i = 0; i < y_vectors.size(); i++) {
		if(!y_vectors[i].isUndefined()) {
			if(y_vectors[i].size() > 3000) {
				samples = 6000;
				break;
			}
			if(y_vectors[i].size() * 2 > samples) samples = y_vectors[i].size() * 2;
		}
	}
	plot += "set samples ";
	plot += i2s(samples);
	plot += "\n";
	plot += "plot ";
	for(size_t i = 0; i < y_vectors.size(); i++) {
		if(!y_vectors[i].isUndefined()) {
			if(i != 0) {
				plot += ",";
			}
			string filename = "gnuplot_data";
			filename += i2s(i + 1);
			filename = buildPath(homedir, filename);
#ifdef _WIN32
			gsub("\\", "\\\\", filename);
#endif
			plot += "\"";
			plot += filename;
			plot += "\"";
			if(i < pdps.size()) {
				switch(pdps[i]->smoothing) {
					case PLOT_SMOOTHING_UNIQUE: {plot += " smooth unique"; break;}
					case PLOT_SMOOTHING_CSPLINES: {plot += " smooth csplines"; break;}
					case PLOT_SMOOTHING_BEZIER: {plot += " smooth bezier"; break;}
					case PLOT_SMOOTHING_SBEZIER: {plot += " smooth sbezier"; break;}
					default: {}
				}
				if(pdps[i]->xaxis2 && pdps[i]->yaxis2) {
					plot += " axis x2y2";
				} else if(pdps[i]->xaxis2) {
					plot += " axis x2y1";
				} else if(pdps[i]->yaxis2) {
					plot += " axis x1y2";
				}
				if(!pdps[i]->title.empty()) {
					title = pdps[i]->title;
					gsub("\"", "\\\"", title);
					plot += " title \"";
					plot += title;
					plot += "\"";
				}
				switch(pdps[i]->style) {
					case PLOT_STYLE_LINES: {plot += " with lines"; break;}
					case PLOT_STYLE_POINTS: {plot += " with points"; break;}
					case PLOT_STYLE_POINTS_LINES: {plot += " with linespoints"; break;}
					case PLOT_STYLE_BOXES: {plot += " with boxes"; break;}
					case PLOT_STYLE_HISTOGRAM: {plot += " with histeps"; break;}
					case PLOT_STYLE_STEPS: {plot += " with steps"; break;}
					case PLOT_STYLE_CANDLESTICKS: {plot += " with candlesticks"; break;}
					case PLOT_STYLE_DOTS: {plot += " with dots"; break;}
				}
				if(param->linewidth < 1) {
					plot += " lw 2";
				} else {
					plot += " lw ";
					plot += i2s(param->linewidth);
				}
			}
		}
	}
	plot += "\n";
	
	string plot_data;
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_MIDPOINT;
	po.decimalpoint_sign = ".";
	po.comma_sign = ",";
	for(size_t serie = 0; serie < y_vectors.size(); serie++) {
		if(!y_vectors[serie].isUndefined()) {
			string filename = "gnuplot_data";
			filename += i2s(serie + 1);
			string filepath = buildPath(homedir, filename);
			FILE *fdata = fopen(filepath.c_str(), "w+");
			if(!fdata) {
				error(true, _("Could not create temporary file %s"), filepath.c_str(), NULL);
				return false;
			}
			plot_data = "";
			int non_numerical = 0, non_real = 0;
			//string str = "";
			if(msecs > 0) startControl(msecs);
			ComparisonResult ct1 = COMPARISON_RESULT_EQUAL, ct2 = COMPARISON_RESULT_EQUAL;
			size_t last_index = string::npos, last_index2 = string::npos;
			bool check_continuous = pdps[serie]->test_continuous && (pdps[serie]->style == PLOT_STYLE_LINES || pdps[serie]->style == PLOT_STYLE_POINTS_LINES);
			bool prev_failed = false;
			for(size_t i = 1; i <= y_vectors[serie].countChildren(); i++) {
				ComparisonResult ct = COMPARISON_RESULT_UNKNOWN;
				bool invalid_nr = false, b_imagzero_x = false, b_imagzero_y = false;
				if(!y_vectors[serie].getChild(i)->isNumber()) {
					invalid_nr = true;
					non_numerical++;
					//if(non_numerical == 1) str = y_vectors[serie].getChild(i)->print(po);
				} else if(!y_vectors[serie].getChild(i)->number().isReal()) {
					b_imagzero_y = testComplexZero(&y_vectors[serie].getChild(i)->number(), y_vectors[serie].getChild(i)->number().internalImaginary());
					if(!b_imagzero_y) {
						invalid_nr = true;
						non_real++;
						//if(non_numerical + non_real == 1) str = y_vectors[serie].getChild(i)->print(po);
					}
				}
				if(serie < x_vectors.size() && !x_vectors[serie].isUndefined() && x_vectors[serie].countChildren() == y_vectors[serie].countChildren()) {
					if(!x_vectors[serie].getChild(i)->isNumber()) {
						invalid_nr = true;
						non_numerical++;
						//if(non_numerical == 1) str = x_vectors[serie].getChild(i)->print(po);
					} else if(!x_vectors[serie].getChild(i)->number().isReal()) {
						b_imagzero_x = testComplexZero(&x_vectors[serie].getChild(i)->number(), x_vectors[serie].getChild(i)->number().internalImaginary());
						if(!b_imagzero_x) {
							invalid_nr = true;
							non_real++;
							//if(non_numerical + non_real == 1) str = x_vectors[serie].getChild(i)->print(po);
						}
					}
					if(!invalid_nr) {
						if(b_imagzero_y) plot_data += x_vectors[serie].getChild(i)->number().realPart().print(po);
						else plot_data += x_vectors[serie].getChild(i)->print(po);
						plot_data += " ";
					}
				}
				if(!invalid_nr) {
					if(check_continuous && !prev_failed) {
						if(i == 1 || ct2 == COMPARISON_RESULT_UNKNOWN) ct = COMPARISON_RESULT_EQUAL;
						else ct = y_vectors[serie].getChild(i - 1)->number().compare(y_vectors[serie].getChild(i)->number());
						if((ct == COMPARISON_RESULT_GREATER || ct == COMPARISON_RESULT_LESS) && (ct1 == COMPARISON_RESULT_GREATER || ct1 == COMPARISON_RESULT_LESS) && (ct2 == COMPARISON_RESULT_GREATER || ct2 == COMPARISON_RESULT_LESS) && ct1 != ct2 && ct != ct2) {
							if(last_index2 != string::npos) plot_data.insert(last_index2 + 1, "  \n");
						}
					}
					if(b_imagzero_x) plot_data += y_vectors[serie].getChild(i)->number().realPart().print(po);
					else plot_data += y_vectors[serie].getChild(i)->print(po);
					plot_data += "\n";
					prev_failed = false;
				} else if(!prev_failed) {
					ct = COMPARISON_RESULT_UNKNOWN;
					plot_data += "  \n";
					prev_failed = true;
				}
				last_index2 = last_index;
				last_index = plot_data.length() - 1;
				ct1 = ct2;
				ct2 = ct;
				if(aborted()) {
					fclose(fdata);
					if(msecs > 0) {
						error(true, _("It took too long to generate the plot data."), NULL);
						stopControl();
					}
					return false;
				}
			}
			if(msecs > 0) stopControl();
			/*if(non_numerical > 0 || non_real > 0) {
				string stitle;
				if(serie < pdps.size() && !pdps[serie]->title.empty()) {
					stitle = pdps[serie]->title.c_str();
				} else {
					stitle = i2s(serie).c_str();
				}
				if(non_numerical > 0) {
					error(true, _("Series %s contains non-numerical data (\"%s\" first of %s) which can not be properly plotted."), stitle.c_str(), str.c_str(), i2s(non_numerical).c_str(), NULL);
				} else {
					error(true, _("Series %s contains non-real data (\"%s\" first of %s) which can not be properly plotted."), stitle.c_str(), str.c_str(), i2s(non_real).c_str(), NULL);
				}
			}*/
			fputs(plot_data.c_str(), fdata);
			fflush(fdata);
			fclose(fdata);
		}
	}
	
	return invokeGnuplot(plot, commandline_extra, persistent);
}
bool Calculator::invokeGnuplot(string commands, string commandline_extra, bool persistent) {
	FILE *pipe = NULL;
	if(!b_gnuplot_open || !gnuplot_pipe || persistent || commandline_extra != gnuplot_cmdline) {
		if(!persistent) {
			closeGnuplot();
		}
		string commandline = "gnuplot";
		if(persistent) {
			commandline += " -persist";
		}
		commandline += commandline_extra;
#ifdef _WIN32
		commandline += " - 2>nul";
		pipe = _popen(commandline.c_str(), "w");
#else
		commandline += " - 2>/dev/null";
		pipe = popen(commandline.c_str(), "w");
#endif
		if(!pipe) {
			error(true, _("Failed to invoke gnuplot. Make sure that you have gnuplot installed in your path."), NULL);
			return false;
		}
		if(!persistent && pipe) {
			gnuplot_pipe = pipe;
			b_gnuplot_open = true;
			gnuplot_cmdline = commandline_extra;
		}
	} else {
		pipe = gnuplot_pipe;
	}
	if(!pipe) {
		return false;
	}
	if(!persistent) {
		fputs("clear\n", pipe);
		fputs("reset\n", pipe);
	}
	fputs(commands.c_str(), pipe);
	fflush(pipe);
	if(persistent) {
		return pclose(pipe) == 0;
	}
	return true;
}
bool Calculator::closeGnuplot() {
	if(gnuplot_pipe) {
#ifdef _WIN32
		int rv = _pclose(gnuplot_pipe);
#else
		int rv = pclose(gnuplot_pipe);
#endif
		gnuplot_pipe = NULL;
		b_gnuplot_open = false;
		return rv == 0;
	}
	gnuplot_pipe = NULL;
	b_gnuplot_open = false;
	return true;
}
bool Calculator::gnuplotOpen() {
	return b_gnuplot_open && gnuplot_pipe;
}

