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
#ifdef _MSC_VER
#	include <sys/utime.h>
#else
#	include <unistd.h>
#	include <utime.h>
#endif
#include <time.h>
#include <limits.h>
#include <sys/types.h>

#ifdef HAVE_ICU
#	include <unicode/ucasemap.h>
#endif

#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;
using std::iterator;

#include "Calculator_p.h"

PrintOptions::PrintOptions() : min_exp(EXP_PRECISION), base(BASE_DECIMAL), base_display(BASE_DISPLAY_NONE), lower_case_numbers(false), lower_case_e(false), number_fraction_format(FRACTION_DECIMAL), indicate_infinite_series(false), show_ending_zeroes(true), abbreviate_names(true), use_reference_names(false), place_units_separately(true), use_unit_prefixes(true), use_prefixes_for_all_units(false), use_prefixes_for_currencies(false), use_all_prefixes(false), use_denominator_prefix(true), negative_exponents(false), short_multiplication(true), limit_implicit_multiplication(false), allow_non_usable(false), use_unicode_signs(false), multiplication_sign(MULTIPLICATION_SIGN_DOT), division_sign(DIVISION_SIGN_DIVISION_SLASH), spacious(true), excessive_parenthesis(false), halfexp_to_sqrt(true), min_decimals(0), max_decimals(-1), use_min_decimals(true), use_max_decimals(true), round_halfway_to_even(false), improve_division_multipliers(true), prefix(NULL), is_approximate(NULL), can_display_unicode_string_function(NULL), can_display_unicode_string_arg(NULL), hide_underscore_spaces(false), preserve_format(false), allow_factorization(false), spell_out_logical_operators(false), restrict_to_parent_precision(true), restrict_fraction_length(false), exp_to_root(false), preserve_precision(false), interval_display(INTERVAL_DISPLAY_INTERVAL), digit_grouping(DIGIT_GROUPING_NONE), date_time_format(DATE_TIME_FORMAT_ISO), time_zone(TIME_ZONE_LOCAL), custom_time_zone(0), twos_complement(true), hexadecimal_twos_complement(false), binary_bits(0), exp_display(EXP_DEFAULT), duodecimal_symbols(false), rounding(ROUNDING_HALF_AWAY_FROM_ZERO) {}

const string &PrintOptions::comma() const {if(comma_sign.empty()) return CALCULATOR->getComma(); return comma_sign;}
const string &PrintOptions::decimalpoint() const {if(decimalpoint_sign.empty()) return CALCULATOR->getDecimalPoint(); return decimalpoint_sign;}

InternalPrintStruct::InternalPrintStruct() : depth(0), power_depth(0), division_depth(0), wrap(false), num(NULL), den(NULL), re(NULL), im(NULL), exp(NULL), minus(NULL), exp_minus(NULL), parent_approximate(false), parent_precision(-1), iexp(NULL) {}

ParseOptions::ParseOptions() : variables_enabled(true), functions_enabled(true), unknowns_enabled(true), units_enabled(true), rpn(false), base(BASE_DECIMAL), limit_implicit_multiplication(false), read_precision(DONT_READ_PRECISION), dot_as_separator(false), brackets_as_parentheses(false), angle_unit(ANGLE_UNIT_NONE), unended_function(NULL), preserve_format(false), default_dataset(NULL), parsing_mode(PARSING_MODE_ADAPTIVE), twos_complement(false), hexadecimal_twos_complement(false), binary_bits(0) {}

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

#define BITWISE_XOR "⊻"

#ifdef _MSC_VER
#	define strdup _strdup
#endif

Calculator::Calculator() {
	b_ignore_locale = false;

#ifdef _WIN32
	size_t n = 0;
	getenv_s(&n, NULL, 0, "LANG");
	if(n == 0) {
		ULONG nlang = 0;
		DWORD n = 0;
		if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, NULL, &n)) {
			WCHAR* wlocale = new WCHAR[n];
			if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, wlocale, &n)) {
				string lang = utf8_encode(wlocale);
				gsub("-", "_", lang);
				if(lang.length() > 5) lang = lang.substr(0, 5);
				if(!lang.empty()) _putenv_s("LANG", lang.c_str());
			}
			delete[] wlocale;
		}
	}
#endif

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif

	setlocale(LC_ALL, "");

	gmp_randinit_default(randstate);
	gmp_randseed_ui(randstate, (unsigned long int) time(NULL));

	priv = new Calculator_p;
	priv->custom_input_base_i = 0;
	priv->ids_i = 0;
	priv->local_currency = NULL;
	priv->use_binary_prefixes = 0;
	priv->temperature_calculation = TEMPERATURE_CALCULATION_HYBRID;
	priv->matlab_matrices = true;
	priv->persistent_plot = false;
	priv->concise_uncertainty_input = false;
	priv->fixed_denominator = 2;

#ifdef HAVE_ICU
	UErrorCode err = U_ZERO_ERROR;
	ucm = ucasemap_open(NULL, 0, &err);
#endif

	srand(time(NULL));

	exchange_rates_time[0] = 0;
	exchange_rates_time[1] = (time_t) 479928L * (time_t) 3600;
	exchange_rates_time[2] = 0;
	priv->exchange_rates_time2[0] = (time_t) 479928L * (time_t) 3600;
	exchange_rates_check_time[0] = 0;
	exchange_rates_check_time[1] = (time_t) 479928L * (time_t) 3600;
	exchange_rates_check_time[2] = 0;
	priv->exchange_rates_check_time2[0] = (time_t) 479928L * (time_t) 3600;
	b_exchange_rates_warning_enabled = true;
	b_exchange_rates_used = 0;
	priv->exchange_rates_url3 = 0;

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
	addStringAlternative("⁄", DIVISION);
	addStringAlternative(SIGN_MULTIPLICATION, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MIDDLEDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIBULLET, MULTIPLICATION);
	addStringAlternative(SIGN_SMALLCIRCLE, MULTIPLICATION);
	addStringAlternative(SIGN_MINUS, MINUS);
	addStringAlternative("–", MINUS);
	addStringAlternative(SIGN_PLUS, PLUS);
	addStringAlternative(SIGN_NOT_EQUAL, NOT EQUALS);
	addStringAlternative("~=", NOT EQUALS);
	addStringAlternative(SIGN_GREATER_OR_EQUAL, GREATER EQUALS);
	addStringAlternative(SIGN_LESS_OR_EQUAL, LESS EQUALS);
	addStringAlternative("\t", SPACE);
	addStringAlternative("\n", SPACE);
	addStringAlternative(NBSP, SPACE);
	addStringAlternative(NNBSP, THIN_SPACE);
	addStringAlternative("**", POWER);
	addStringAlternative("^^", "⊻");
	addStringAlternative("↊", "X");
	addStringAlternative("↋", "E");
	addStringAlternative("∧", BITWISE_AND);
	addStringAlternative("∨", BITWISE_OR);
	addStringAlternative("¬", BITWISE_NOT);
	addStringAlternative("…", "...");

	//division operator
	per_str = _("per");
	per_str_len = per_str.length();
	//multiplication operator
	times_str = _("times");
	times_str_len = times_str.length();
	//addition operator
	plus_str = _("plus");
	plus_str_len = plus_str.length();
	//subtraction operator
	minus_str = _("minus");
	minus_str_len = minus_str.length();
	//logical operator
	and_str = _("and");
	if(and_str == "and") and_str = "";
	and_str_len = and_str.length();
	AND_str = "AND";
	AND_str_len = AND_str.length();
	//logical operator
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
	if((local_digit_group_separator.length() == 1 && (signed char) local_digit_group_separator[0] < 0) || local_digit_group_separator == NBSP) local_digit_group_separator = SPACE;
	else if(local_digit_group_separator == NNBSP) local_digit_group_separator = THIN_SPACE;
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

	//"to"-operator
	string str = _("to");
	local_to = (str != "to");

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
	save_printoptions.limit_implicit_multiplication = true;
	save_printoptions.spacious = false;
	save_printoptions.number_fraction_format = FRACTION_FRACTIONAL;
	save_printoptions.short_multiplication = false;
	save_printoptions.show_ending_zeroes = false;
	save_printoptions.use_unit_prefixes = false;

	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.spell_out_logical_operators = true;
	message_printoptions.number_fraction_format = FRACTION_FRACTIONAL;

	default_user_evaluation_options.structuring = STRUCTURING_SIMPLIFY;

	default_assumptions = new Assumptions;
	default_assumptions->setType(ASSUMPTION_TYPE_REAL);
	default_assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);

	u_rad = NULL; u_gra = NULL; u_deg = NULL;
	priv->custom_angle_unit = NULL;

	priv->simplified_percentage_used = false;
	b_save_called = false;

	ILLEGAL_IN_NAMES = "\a\b" + DOT_S + RESERVED OPERATORS SEXADOT SPACES PARENTHESISS VECTOR_WRAPS COMMAS;
	ILLEGAL_IN_NAMES_MINUS_SPACE_STR = "\a\b" + DOT_S + RESERVED OPERATORS SEXADOT PARENTHESISS VECTOR_WRAPS COMMAS;
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

	if(b_ignore_locale) {
		char *current_lc_monetary = setlocale(LC_MONETARY, "");
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
	} else {
#ifdef _WIN32
		size_t n = 0;
		getenv_s(&n, NULL, 0, "LANG");
		if(n == 0) {
			ULONG nlang = 0;
			DWORD n = 0;
			if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, NULL, &n)) {
				WCHAR* wlocale = new WCHAR[n];
				if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &nlang, wlocale, &n)) {
					string lang = utf8_encode(wlocale);
					gsub("-", "_", lang);
					if(lang.length() > 5) lang = lang.substr(0, 5);
					if(!lang.empty()) _putenv_s("LANG", lang.c_str());
				}
				delete[] wlocale;
			}
		}
#endif
#ifdef ENABLE_NLS
		bindtextdomain(GETTEXT_PACKAGE, getPackageLocaleDir().c_str());
		bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
		setlocale(LC_ALL, "");
	}


	gmp_randinit_default(randstate);
	gmp_randseed_ui(randstate, (unsigned long int) time(NULL));

	priv = new Calculator_p;
	priv->custom_input_base_i = 0;
	priv->ids_i = 0;
	priv->local_currency = NULL;
	priv->use_binary_prefixes = 0;
	priv->temperature_calculation = TEMPERATURE_CALCULATION_HYBRID;
	priv->matlab_matrices = true;
	priv->persistent_plot = false;
	priv->concise_uncertainty_input = false;
	priv->fixed_denominator = 2;

#ifdef HAVE_ICU
	UErrorCode err = U_ZERO_ERROR;
	ucm = ucasemap_open(NULL, 0, &err);
#endif

	srand(time(NULL));

	exchange_rates_time[0] = 0;
	exchange_rates_time[1] = (time_t) 479928L * (time_t) 3600;
	exchange_rates_time[2] = 0;
	priv->exchange_rates_time2[0] = (time_t) 479928L * (time_t) 3600;
	exchange_rates_check_time[0] = 0;
	exchange_rates_check_time[1] = (time_t) 479928L * (time_t) 3600;
	exchange_rates_check_time[2] = 0;
	priv->exchange_rates_check_time2[0] = (time_t) 479928L * (time_t) 3600;
	b_exchange_rates_warning_enabled = true;
	b_exchange_rates_used = 0;
	priv->exchange_rates_url3 = 0;

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
	addStringAlternative("⁄", DIVISION);
	addStringAlternative(SIGN_MULTIPLICATION, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MIDDLEDOT, MULTIPLICATION);
	addStringAlternative(SIGN_MULTIBULLET, MULTIPLICATION);
	addStringAlternative(SIGN_SMALLCIRCLE, MULTIPLICATION);
	addStringAlternative(SIGN_MINUS, MINUS);
	addStringAlternative("–", MINUS);
	addStringAlternative(SIGN_PLUS, PLUS);
	addStringAlternative(SIGN_NOT_EQUAL, NOT EQUALS);
	addStringAlternative("~=", NOT EQUALS);
	addStringAlternative(SIGN_GREATER_OR_EQUAL, GREATER EQUALS);
	addStringAlternative(SIGN_LESS_OR_EQUAL, LESS EQUALS);
	addStringAlternative("\t", SPACE);
	addStringAlternative("\n", SPACE);
	addStringAlternative(NBSP, SPACE);
	addStringAlternative(NNBSP, THIN_SPACE);
	addStringAlternative("**", POWER);
	addStringAlternative("^^", "⊻");
	addStringAlternative("↊", "X");
	addStringAlternative("↋", "E");
	addStringAlternative("∧", BITWISE_AND);
	addStringAlternative("∨", BITWISE_OR);
	addStringAlternative("¬", BITWISE_NOT);
	addStringAlternative("…", "...");

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
	if((local_digit_group_separator.length() == 1 && (signed char) local_digit_group_separator[0] < 0) || local_digit_group_separator == " ") local_digit_group_separator = " ";
	else if(local_digit_group_separator == " ") local_digit_group_separator = THIN_SPACE;
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

	//"to"-operator
	string str = _("to");
	local_to = (str != "to");

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
	save_printoptions.limit_implicit_multiplication = true;
	save_printoptions.spacious = false;
	save_printoptions.number_fraction_format = FRACTION_FRACTIONAL;
	save_printoptions.short_multiplication = false;
	save_printoptions.show_ending_zeroes = false;
	save_printoptions.use_unit_prefixes = false;

	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.spell_out_logical_operators = true;
	message_printoptions.number_fraction_format = FRACTION_FRACTIONAL;

	default_user_evaluation_options.structuring = STRUCTURING_SIMPLIFY;

	default_assumptions = new Assumptions;
	default_assumptions->setType(ASSUMPTION_TYPE_REAL);
	default_assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);

	u_rad = NULL; u_gra = NULL; u_deg = NULL;
	priv->custom_angle_unit = NULL;

	priv->simplified_percentage_used = false;
	b_save_called = false;

	ILLEGAL_IN_NAMES = "\a\b" + DOT_S + RESERVED OPERATORS SEXADOT SPACES PARENTHESISS VECTOR_WRAPS COMMAS;
	ILLEGAL_IN_NAMES_MINUS_SPACE_STR = "\a\b" + DOT_S + RESERVED OPERATORS SEXADOT PARENTHESISS VECTOR_WRAPS COMMAS;
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
	clearRPNStack();
	for(unordered_map<Unit*, MathStructure*>::iterator it = priv->composite_unit_base.begin(); it != priv->composite_unit_base.end(); ++it) it->second->unref();
	for(unordered_map<size_t, MathStructure*>::iterator it = priv->id_structs.begin(); it != priv->id_structs.end(); ++it) it->second->unref();
#define REMOVE_EXPRESSION_ITEM(o) o->setRegistered(false); o->destroy();
	for(size_t i = 0; i < functions.size(); i++) {REMOVE_EXPRESSION_ITEM(functions[i])}
	for(size_t i = 0; i < variables.size(); i++) {REMOVE_EXPRESSION_ITEM(variables[i])}
	for(size_t i = 0; i < units.size(); i++) {REMOVE_EXPRESSION_ITEM(units[i])}
	for(size_t i = 0; i < prefixes.size(); i++) delete prefixes[i];
	if(v_C) {REMOVE_EXPRESSION_ITEM(v_C)}
	if(decimal_null_prefix) delete decimal_null_prefix;
	if(binary_null_prefix) delete binary_null_prefix;
	if(default_assumptions) delete default_assumptions;
	if(saved_locale) free(saved_locale);
	delete priv;
	delete calculate_thread;
	calculator = NULL;
	gmp_randclear(randstate);
	mpfr_free_cache();
#ifdef HAVE_ICU
	if(ucm) ucasemap_close(ucm);
#endif
}

string Calculator::logicalORString(void) const {return _("or");}
string Calculator::logicalANDString(void) const {return _("and");}

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

bool Calculator::utf8_pos_is_valid_in_name(char *pos) const {
	if(is_in(ILLEGAL_IN_NAMES, pos[0])) {
		return false;
	}
	if((unsigned char) pos[0] >= 0xC0) {
		size_t l = 1;
		while((unsigned char) pos[l] >= 0x80 && (unsigned char) pos[l] < 0xC0) {
			l++;
		}
		if(l == 3 && pos[0] == '\xe2') {
			if(pos[1] == '\x80') {
				// thin space
				if(pos[2] == '\x89') return false;
				// quotation marks
				if(((signed char) pos[2] >= -104 && (signed char) pos[2] <= -97) || (signed char) pos[2] == -70 || (signed char) pos[2] == -71) return false;
				// operator
				if(pos[2] == '\xa2') return false;
			} else if(pos[1] == '\x81') {
				// exponents
				if((signed char) pos[2] == -80 || ((signed char) pos[2] >= -76 && (signed char) pos[2] <= -69) || (signed char) pos[2] == -67 || (signed char) pos[2] == -66) return false;
			} else if(pos[1] == '\x85') {
				// fractions
				if((signed char) pos[2] >= -112 && (signed char) pos[2] <= -98) return false;
			} else if(pos[1] == '\x88') {
				// operators
				if(pos[2] == '\x95' || pos[2] == '\x99' || pos[2] == '\x92') return false;
			} else if(pos[1] == '\x89') {
				// inequalities
				if(pos[2] == '\xa0' || pos[2] == '\xa5' || pos[2] == '\xa4') return false;
			} else if(pos[1] == '\x8b') {
				// operator
				if(pos[2] == '\x85') return false;
			}
		} else if(l == 3 && pos[0] == '\xef') {
			if(pos[1] == '\xbc' && pos[2] == '\x8b') return false;
		} else if(l == 2 && pos[0] == '\xc2') {
			// exponents
			if((signed char) pos[1] == -71 || (signed char) pos[1] == -77 || (signed char) pos[1] == -78) return false;
			// fractions
			if((signed char) pos[1] == -66 || (signed char) pos[1] == -67 || (signed char) pos[1] == -68) return false;
			// operators
			if(pos[1] == '\xb1' || pos[1] == '\xb7') return false;
		} else if(l == 2 && pos[0] == '\xc3') {
			// operators
			if(pos[1] == '\x97' || pos[1] == '\xb7') return false;
		}
		pos += (l - 1);
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
bool equals_ignore_us(const string &str1, const string &str2, int ignore_us) {
	if(ignore_us == 0) return str1 == str2;
	if(str1.length() != str2.length() - ignore_us) return false;
	size_t iu = 0;
	for(size_t i = 0; i < str1.length(); i++) {
		if(ignore_us > 0 && str2[i + iu] == '_') {ignore_us--; iu++;}
		if(str1[i] != str2[i + iu]) return false;
	}
	return true;
}
bool equalsIgnoreCase(const string &str1, const string &str2, int ignore_us) {
	if(str1.empty() || str2.empty()) return false;
	for(size_t i1 = 0, i2 = 0; i1 < str1.length() || i2 < str2.length(); i1++, i2++) {
		if(ignore_us > 0 && str2[i2] == '_') {i2++; ignore_us--;}
		if(i1 >= str1.length() || i2 >= str2.length()) return false;
		if(((signed char) str1[i1] < 0 && i1 + 1 < str1.length()) || ((signed char) str2[i2] < 0 && i2 + 1 < str2.length())) {
			size_t iu1 = 1, iu2 = 1;
			if((signed char) str1[i1] < 0) {
				while(iu1 + i1 < str1.length() && (signed char) str1[i1 + iu1] < 0) {
					iu1++;
				}
			}
			if((signed char) str2[i2] < 0) {
				while(iu2 + i2 < str2.length() && (signed char) str2[i2 + iu2] < 0) {
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
				char *gstr1 = utf8_strdown(str1.c_str() + (sizeof(char) * i1));
				if(!gstr1) return false;
				char *gstr2 = utf8_strdown(str2.c_str() + (sizeof(char) * i2));
				if(!gstr2) {
					free(gstr1);
					return false;
				}
				bool b = strcmp(gstr1, gstr2) == 0;
				free(gstr1);
				free(gstr2);
				return b;
			}
			i1 += iu1 - 1;
			i2 += iu2 - 1;
		} else if(str1[i1] != str2[i2] && !((str1[i1] >= 'a' && str1[i1] <= 'z') && str1[i1] - 32 == str2[i2]) && !((str1[i1] <= 'Z' && str1[i1] >= 'A') && str1[i1] + 32 == str2[i2])) {
			return false;
		}
	}
	return true;
}
ExpressionItem *Calculator::getActiveExpressionItem(ExpressionItem *item) {
	if(!item) return NULL;
	for(size_t i = 1; i <= item->countNames(); i++) {
		ExpressionItem *item2 = getActiveExpressionItem(item->getName(i).name, item, !item->getName(i).completion_only);
		if(item2) {
			return item2;
		}
	}
	return NULL;
}
bool name_allows_underscore_removal(const string &name) {
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
ExpressionItem *Calculator::getActiveExpressionItem(string name_, ExpressionItem *item, bool ignore_us) {
	ExpressionItem *o = getActiveExpressionItem(name_, item);
	if(o || !ignore_us || !name_allows_underscore_removal(name_)) return o;
	gsub("_", "", name_);
	return getActiveExpressionItem(name_, o);
}
ExpressionItem *Calculator::getActiveExpressionItem(string name_, ExpressionItem *item) {
	if(name_.empty()) return NULL;
	size_t l = name_.length();
	if(l > UFV_LENGTHS) {
		for(size_t i = 0; i < ufvl.size(); i++) {
			if(ufvl_t[i] == 'f' || ufvl_t[i] == 'v' || ufvl_t[i] == 'u') {
				if(ufvl[i] != item) {
					const ExpressionName &ename = ((ExpressionItem*) ufvl[i])->getName(ufvl_i[i]);
					if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufvl_us[i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufvl_us[i]))) {
						return (ExpressionItem*) ufvl[i];
					}
				}
			}
		}
	} else {
		l--;
		for(size_t i2 = 1; i2 <= 3; i2++) {
			for(size_t i = 0; i < ufv[i2][l].size(); i++) {
				if(ufv[i2][l][i] != item) {
					const ExpressionName &ename = ((ExpressionItem*) ufv[i2][l][i])->getName(ufv_i[i2][l][i]);
					if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufv_us[i2][l][i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufv_us[i2][l][i]))) {
						return (ExpressionItem*) ufv[i2][l][i];
					}
				}
			}
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
	if(!default_assumptions) {
		default_assumptions = new Assumptions;
		default_assumptions->setType(ASSUMPTION_TYPE_REAL);
		default_assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);
	}
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
		if(prefixes[i]->hasName(name_, true)) {
			return prefixes[i];
		}
	}
	return NULL;
}
Prefix *Calculator::getDecimalNullPrefix() const {return decimal_null_prefix;}
Prefix *Calculator::getBinaryNullPrefix() const {return binary_null_prefix;}
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
		p = decimal_prefixes[i];
		if(all_prefixes || (p->exponent() % 3 == 0 && p->exponent() >= -24 && p->exponent() <= 24)) {
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
				if((exp < 0 && (i == (int) decimal_prefixes.size() - 1 || (!all_prefixes && p->exponent() == 24))) || (exp >= 0 && (i == 0 || (!all_prefixes && p->exponent() == -24)))) {
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
		p = decimal_prefixes[i];
		if(all_prefixes || (p->exponent() % 3 == 0 && p->exponent() >= -24 && p->exponent() <= 24)) {
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
				if((exp.isNegative() && (i == (int) decimal_prefixes.size() - 1 || (!all_prefixes && p->exponent() == 24))) || (!exp.isNegative() && (i == 0 || (!all_prefixes && p->exponent() == -24)))) {
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
void Calculator::setTemperatureCalculationMode(TemperatureCalculationMode temperature_calculation_mode) {
	priv->temperature_calculation = temperature_calculation_mode;
}
TemperatureCalculationMode Calculator::getTemperatureCalculationMode() const {
	return priv->temperature_calculation;
}
int Calculator::usesBinaryPrefixes() const {
	return priv->use_binary_prefixes;
}
void Calculator::useBinaryPrefixes(int use_binary_prefixes) {
	priv->use_binary_prefixes = use_binary_prefixes;
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
	int i = -1;
	if(exp < 0) {
		i = binary_prefixes.size() - 1;
	}
	BinaryPrefix *p = NULL, *p_prev = NULL;
	int exp2_1, exp2_2;
	while((exp < 0 && i >= -1) || (exp >= 0 && i < (int) binary_prefixes.size())) {
		if(i >= 0) p = binary_prefixes[i];
		else p = binary_null_prefix;
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
			if((exp >= 0 && i == 0) || (exp < 0 && i == (int) binary_prefixes.size())) {
				if(p == binary_null_prefix) return NULL;
				return p;
			}
			exp2_1 = exp2;
			if(p_prev) {
				exp2_1 -= p_prev->exponent(exp);
			}
			exp2_2 = p->exponent(exp);
			exp2_2 -= exp2;
			exp2_2 += 9;
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
	int i = -1;
	ComparisonResult c;
	if(exp.isNegative()) {
		i = binary_prefixes.size() - 1;
	}
	BinaryPrefix *p = NULL, *p_prev = NULL;
	Number exp2_1, exp2_2;
	while((exp.isNegative() && i >= -1) || (!exp.isNegative() && i < (int) binary_prefixes.size())) {
		if(i >= 0) p = binary_prefixes[i];
		else p = binary_null_prefix;
		c = exp2.compare(p->exponent(exp));
		if(c == COMPARISON_RESULT_EQUAL) {
			if(p == binary_null_prefix) return NULL;
			return p;
		} else if(c == COMPARISON_RESULT_GREATER) {
			if((exp.isNegative() && i == (int) binary_prefixes.size() - 1) || (!exp.isNegative() && i == 0)) {
				if(p == binary_null_prefix) return NULL;
				return p;
			}
			exp2_1 = exp2;
			if(p_prev) {
				exp2_1 -= p_prev->exponent(exp);
			}
			exp2_2 = p->exponent(exp);
			exp2_2 -= exp2;
			exp2_2 += 9;
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
		if(decimal_prefixes.empty() || ((DecimalPrefix*) p)->exponent() > decimal_prefixes[decimal_prefixes.size() - 1]->exponent()) {
			decimal_prefixes.push_back((DecimalPrefix*) p);
		} else {
			size_t i = decimal_prefixes.size() - 1;
			while(i > 0 && ((DecimalPrefix*) p)->exponent() < decimal_prefixes[i - 1]->exponent()) i--;
			decimal_prefixes.insert(decimal_prefixes.begin() + i, (DecimalPrefix*) p);
		}
	} else if(p->type() == PREFIX_BINARY) {
		if(binary_prefixes.empty() || ((BinaryPrefix*) p)->exponent() > binary_prefixes[binary_prefixes.size() - 1]->exponent()) {
			binary_prefixes.push_back((BinaryPrefix*) p);
		} else {
			size_t i = binary_prefixes.size() - 1;
			while(i > 0 && ((BinaryPrefix*) p)->exponent() < binary_prefixes[i - 1]->exponent()) i--;
			binary_prefixes.insert(binary_prefixes.begin() + i, (BinaryPrefix*) p);
		}
	}
	prefixes.push_back(p);
	prefixNameChanged(p, true);
	return p;
}
void Calculator::prefixNameChanged(Prefix *p, bool new_item) {
	size_t l2;
	if(!new_item) delPrefixUFV(p);
	for(size_t i2 = 1; i2 <= p->countNames(); i2++) {
		const ExpressionName &ename = p->getName(i2);
		l2 = ename.name.length();
		if(l2 > UFV_LENGTHS) {
			size_t i = 0, l;
			for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
				l = 0;
				if(it != ufvl.end()) {
					if(ufvl_t[i] == 'v' || ufvl_t[i] == 'f' || ufvl_t[i] == 'u') l = ((ExpressionItem*) (*it))->getName(ufvl_i[i]).name.length();
					else if(ufvl_t[i] == 'p' || ufvl_t[i] == 'P') l = ((Prefix*) (*it))->getName(ufvl_i[i]).name.length();
					l -= priv->ufvl_us[i];
				}
				if(it == ufvl.end()) {
					ufvl.push_back((void*) p);
					if(ename.abbreviation) ufvl_t.push_back('p');
					else ufvl_t.push_back('P');
					ufvl_i.push_back(i2);
					priv->ufvl_us.push_back(0);
					break;
				} else if(l <= l2) {
					ufvl.insert(it, (void*) p);
					if(ename.abbreviation) ufvl_t.insert(ufvl_t.begin() + i, 'p');
					else ufvl_t.insert(ufvl_t.begin() + i, 'P');
					ufvl_i.insert(ufvl_i.begin() + i, i2);
					priv->ufvl_us.insert(priv->ufvl_us.begin() + i, 0);
					break;
				}
				i++;
			}
		} else if(l2 > 0) {
			l2--;
			ufv[0][l2].push_back((void*) p);
			ufv_i[0][l2].push_back(i2);
			priv->ufv_us[0][l2].push_back(0);
		}
	}
}
#define PRECISION_TO_BITS(p) ((((double) p) * 3.3219281) + 100)
#define BITS_TO_PRECISION(p) (::ceil((((double) p) - 100) / 3.3219281))
void Calculator::setPrecision(int precision) {
	if(precision <= 0) precision = DEFAULT_PRECISION;
	if(PRECISION_TO_BITS(precision) > (double) MPFR_PREC_MAX - 1000L) {
		if(BITS_TO_PRECISION(MPFR_PREC_MAX) > INT_MAX) i_precision = INT_MAX;
		else i_precision = (int) BITS_TO_PRECISION(MPFR_PREC_MAX - 1000L);
		mpfr_set_default_prec(MPFR_PREC_MAX - 1000L);
	} else {
		i_precision = precision;
		mpfr_set_default_prec(PRECISION_TO_BITS(i_precision));
	}
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

bool Calculator::usesMatlabStyleMatrices() const {return priv->matlab_matrices;}
void Calculator::useMatlabStyleMatrices(bool use_matlab_style_matrices) {priv->matlab_matrices = use_matlab_style_matrices;}

bool Calculator::conciseUncertaintyInputEnabled() const {return priv->concise_uncertainty_input;}
void Calculator::setConciseUncertaintyInputEnabled(bool enable_concise_uncertainty_input) {priv->concise_uncertainty_input = enable_concise_uncertainty_input;}

long int Calculator::fixedDenominator() const {return priv->fixed_denominator;}
void Calculator::setFixedDenominator(long int fixed_denominator) {
	if(fixed_denominator > 1) priv->fixed_denominator = fixed_denominator;
}

void Calculator::setCustomAngleUnit(Unit *u) {
	if(u) u->ref();
	if(priv->custom_angle_unit) priv->custom_angle_unit->unref();
	priv->custom_angle_unit = u;
}
Unit *Calculator::customAngleUnit() {
	return priv->custom_angle_unit;
}

bool Calculator::simplifiedPercentageUsed() const {
	return priv->simplified_percentage_used;
}
void Calculator::setSimplifiedPercentageUsed(bool percentage_used) {
	priv->simplified_percentage_used = percentage_used;
}

void Calculator::setCustomInputBase(Number nr) {
	priv->custom_input_base = nr;
	if(!nr.isReal()) {
		priv->custom_input_base_i = LONG_MAX;
	} else {
		nr.abs(); nr.intervalToMidValue(); nr.ceil();
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
	if(include_spaces) return string(SPACE) + string(_("to")) + SPACE;
	else return _("to");
}
string Calculator::localWhereString() const {
	return string(SPACE) + string(_("where")) + SPACE;
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

void Calculator::resetVariables() {
	for(size_t i = 0; i < variables.size();) {
		size_t n = variables.size();
		variables[i]->destroy();
		if(n == variables.size()) i++;
	}
	if(v_C) v_C->destroy();
	addBuiltinVariables();
}
void Calculator::resetFunctions() {
	for(size_t i = 0; i < functions.size();) {
		size_t n = functions.size();
		functions[i]->destroy();
		if(n == functions.size()) i++;
	}
	addBuiltinFunctions();
}
void Calculator::resetUnits() {
	for(unordered_map<Unit*, MathStructure*>::iterator it = priv->composite_unit_base.begin(); it != priv->composite_unit_base.end(); ++it) it->second->unref();
	for(size_t i = 0; i < units.size();) {
		size_t n = units.size();
		units[i]->destroy();
		if(n == units.size()) i++;
	}
	for(size_t i = 0; i < prefixes.size(); i++) {
		delPrefixUFV(prefixes[i]);
		delete prefixes[i];
	}
	priv->composite_unit_base.clear();
	prefixes.clear();
	addBuiltinUnits();
}
void Calculator::reset() {
	resetVariables();
	resetFunctions();
	resetUnits();
}

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
	v_x = (UnknownVariable*) addVariable(new UnknownVariable("", "x", "", false, false));
	v_y = (UnknownVariable*) addVariable(new UnknownVariable("", "y", "", false, false));
	v_z = (UnknownVariable*) addVariable(new UnknownVariable("", "z", "", false, false));
	v_C = new UnknownVariable("", "C", "", false, true);
	v_C->setAssumptions(new Assumptions());
	v_n = (UnknownVariable*) addVariable(new UnknownVariable("", "n", "", false, true));
	v_n->setAssumptions(new Assumptions());
	v_n->assumptions()->setType(ASSUMPTION_TYPE_INTEGER);
	v_today = (KnownVariable*) addVariable(new TodayVariable());
	v_yesterday = (KnownVariable*) addVariable(new YesterdayVariable());
	v_tomorrow = (KnownVariable*) addVariable(new TomorrowVariable());
	v_now = (KnownVariable*) addVariable(new NowVariable());
#ifndef DISABLE_INSECURE
#if 	defined __linux__ || defined _WIN32
	addVariable(new UptimeVariable());
#	endif
#endif

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
	f_identity = addFunction(new IdentityMatrixFunction());
	f_determinant = addFunction(new DeterminantFunction());
	f_permanent = addFunction(new PermanentFunction());
	f_adjoint = addFunction(new AdjointFunction());
	f_cofactor = addFunction(new CofactorFunction());
	f_inverse = addFunction(new InverseFunction());
	f_magnitude = addFunction(new MagnitudeFunction());
	f_entrywise = addFunction(new EntrywiseFunction());
	addFunction(new RRefFunction());
	addFunction(new MatrixRankFunction());
	priv->f_dot = addFunction(new DotProductFunction());
	priv->f_times = addFunction(new EntrywiseMultiplicationFunction());
	f_hadamard = priv->f_times;
	priv->f_rdivide = addFunction(new EntrywiseDivisionFunction());
	priv->f_power = addFunction(new EntrywisePowerFunction());
	addFunction(new NormFunction());
	priv->f_vertcat = addFunction(new VertCatFunction());
	priv->f_horzcat = addFunction(new HorzCatFunction());
	addFunction(new KroneckerProductFunction());
	addFunction(new FlipFunction());

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
	addFunction(new BitSetFunction());
	addFunction(new BitGetFunction());
	addFunction(new SetBitsFunction());
	addFunction(new CircularShiftFunction());

	f_abs = addFunction(new AbsFunction());
	f_signum = addFunction(new SignumFunction());
	f_heaviside = addFunction(new HeavisideFunction());
	f_dirac = addFunction(new DiracFunction());
	f_gcd = addFunction(new GcdFunction());
	addFunction(new DivisorsFunction());
	addFunction(new PrimeCountFunction());
	addFunction(new PrimesFunction());
	addFunction(new IsPrimeFunction());
	addFunction(new PrevPrimeFunction());
	addFunction(new NextPrimeFunction());
	addFunction(new NthPrimeFunction());
	f_lcm = addFunction(new LcmFunction());
	f_round = addFunction(new RoundFunction());
	f_floor = addFunction(new FloorFunction());
	f_ceil = addFunction(new CeilFunction());
	f_trunc = addFunction(new TruncFunction());
	f_int = addFunction(new IntFunction());
	f_frac = addFunction(new FracFunction());
	f_rem = addFunction(new RemFunction());
	f_mod = addFunction(new ModFunction());
	addFunction(new PowerModFunction());
	addFunction(new BernoulliFunction());
	addFunction(new TotientFunction());
	priv->f_parallel = addFunction(new ParallelFunction());

	addFunction(new IntegerDigitsFunction());
	addFunction(new DigitGetFunction());
	addFunction(new DigitSetFunction());

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
	addFunction(new LowerEndPointFunction());
	addFunction(new UpperEndPointFunction());
	addFunction(new MidPointFunction());
	addFunction(new GetUncertaintyFunction());

	f_sqrt = addFunction(new SqrtFunction());
	f_cbrt = addFunction(new CbrtFunction());
	f_root = addFunction(new RootFunction());
	addFunction(new AllRootsFunction());
	f_sq = addFunction(new SquareFunction());

	f_exp = addFunction(new ExpFunction());

	f_ln = addFunction(new LogFunction());
	f_logn = addFunction(new LognFunction());

	addFunction(new PowerTowerFunction());

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
	priv->f_cis = addFunction(new CisFunction());
	f_radians_to_default_angle_unit = addFunction(new RadiansToDefaultAngleUnitFunction());

	f_zeta = addFunction(new ZetaFunction());
	f_gamma = addFunction(new GammaFunction());
	f_digamma = addFunction(new DigammaFunction());
	f_beta = addFunction(new BetaFunction());
	f_airy = addFunction(new AiryFunction());
	f_besselj = addFunction(new BesseljFunction());
	f_bessely = addFunction(new BesselyFunction());
	f_erf = addFunction(new ErfFunction());
	priv->f_erfi = addFunction(new ErfiFunction());
	f_erfc = addFunction(new ErfcFunction());
	addFunction(new ErfinvFunction());

	f_total = addFunction(new TotalFunction());
	f_percentile = addFunction(new PercentileFunction());
	f_min = addFunction(new MinFunction());
	f_max = addFunction(new MaxFunction());
	f_mode = addFunction(new ModeFunction());
	f_rand = addFunction(new RandFunction());
	addFunction(new RandnFunction());
	addFunction(new RandPoissonFunction());

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
	addFunction(new BijectiveFunction());
	addFunction(new BinaryDecimalFunction());

	addFunction(new IEEE754FloatFunction());
	addFunction(new IEEE754FloatBitsFunction());
	addFunction(new IEEE754FloatComponentsFunction());
	addFunction(new IEEE754FloatValueFunction());
	addFunction(new IEEE754FloatErrorFunction());

	f_ascii = addFunction(new AsciiFunction());
	f_char = addFunction(new CharFunction());

	f_length = addFunction(new LengthFunction());
	f_concatenate = addFunction(new ConcatenateFunction());

	f_replace = addFunction(new ReplaceFunction());
	f_stripunits = addFunction(new StripUnitsFunction());

	f_genvector = addFunction(new GenerateVectorFunction());
	priv->f_colon = addFunction(new ColonFunction());
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
	addFunction(new ForEachFunction());

	f_save = addFunction(new SaveFunction());
#ifndef DISABLE_INSECURE
	f_load = addFunction(new LoadFunction());
	f_export = addFunction(new ExportFunction());
#else
	f_load = NULL;
	f_export = NULL;
#endif

	f_register = addFunction(new RegisterFunction());
	f_stack = addFunction(new StackFunction());

#ifndef DISABLE_INSECURE
	addFunction(new CommandFunction());
#endif

	f_diff = addFunction(new DeriveFunction());
	f_integrate = addFunction(new IntegrateFunction());
	addFunction(new RombergFunction());
	addFunction(new MonteCarloFunction());
	f_solve = addFunction(new SolveFunction());
	f_multisolve = addFunction(new SolveMultipleFunction());
	f_dsolve = addFunction(new DSolveFunction());
	f_limit = addFunction(new LimitFunction());
	priv->f_newton = addFunction(new NewtonRaphsonFunction());
	priv->f_secant = addFunction(new SecantMethodFunction());

	f_li = addFunction(new liFunction());
	f_Li = addFunction(new LiFunction());
	f_Ei = addFunction(new EiFunction());
	f_Si = addFunction(new SiFunction());
	f_Ci = addFunction(new CiFunction());
	f_Shi = addFunction(new ShiFunction());
	f_Chi = addFunction(new ChiFunction());
	priv->f_fresnels = addFunction(new FresnelSFunction());
	priv->f_fresnelc = addFunction(new FresnelCFunction());
	f_igamma = addFunction(new IGammaFunction());
	addFunction(new IncompleteBetaFunction());
	addFunction(new InverseIncompleteBetaFunction());

	addFunction(new GeographicDistanceFunction());

	if(canPlot()) f_plot = addFunction(new PlotFunction());
	else f_plot = NULL;

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
	u_btc = addUnit(new AliasUnit(_("Currency"), "BTC", "bitcoins", "bitcoin", "Bitcoins", u_euro, "57055.5", 1, "", false, true, true));
	u_btc->setApproximate();
	u_btc->setPrecision(-2);
	u_btc->setChanged(false);
	priv->u_byn = addUnit(new AliasUnit(_("Currency"), "BYN", "", "", "Belarusian Ruble", u_euro, "1/3.64171", 1, "", false, true, true));
	priv->u_byn->setHidden(true);
	priv->u_byn->setApproximate();
	priv->u_byn->setPrecision(-2);
	priv->u_byn->setChanged(false);
	Unit *u = addUnit(new AliasUnit(_("Currency"), "BYR", "", "", "Belarusian Ruble p. (obsolete)", priv->u_byn, "0.0001", 1, "", false, true, true));
	u->setHidden(true);
	u->setChanged(false);
	priv->u_kelvin = NULL;
	priv->u_rankine = NULL;
	priv->u_celsius = NULL;
	priv->u_fahrenheit = NULL;
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
void Calculator::clearMessages() {
	messages.clear();
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
	if(u->id() != 0) priv->id_units[u->id()] = u;
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
			priv->ufvl_us.erase(priv->ufvl_us.begin() + i);
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
				priv->ufv_us[0][i2].erase(priv->ufv_us[0][i2].begin() + i);
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
			priv->ufvl_us.erase(priv->ufvl_us.begin() + i);
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
				priv->ufv_us[i3][i2].erase(priv->ufv_us[i3][i2].begin() + i);
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
Unit* Calculator::getUnitById(int id) const {
	switch(id) {
		case UNIT_ID_EURO: {return u_euro;}
		case UNIT_ID_KELVIN: {return priv->u_kelvin;}
		case UNIT_ID_RANKINE: {return priv->u_rankine;}
		case UNIT_ID_CELSIUS: {return priv->u_celsius;}
		case UNIT_ID_FAHRENHEIT: {return priv->u_fahrenheit;}
		case UNIT_ID_BYN: {return priv->u_byn;}
		case UNIT_ID_BTC: {return u_btc;}
		case UNIT_ID_SECOND: {return u_second;}
		case UNIT_ID_MINUTE: {return u_minute;}
		case UNIT_ID_HOUR: {return u_hour;}
		case UNIT_ID_DAY: {return u_day;}
		case UNIT_ID_MONTH: {return u_month;}
		case UNIT_ID_YEAR: {return u_year;}
	}
	unordered_map<int, Unit*>::iterator it = priv->id_units.find(id);
	if(it == priv->id_units.end()) return NULL;
	return it->second;
}
Unit *Calculator::getActiveUnit(string name_, bool ignore_us) {
	Unit *o = getActiveUnit(name_);
	if(o || !ignore_us || !name_allows_underscore_removal(name_)) return o;
	gsub("_", "", name_);
	return getActiveUnit(name_);
}
Unit* Calculator::getActiveUnit(string name_) {
	if(name_.empty()) return NULL;
	size_t l = name_.length();
	if(l > UFV_LENGTHS) {
		for(size_t i = 0; i < ufvl.size(); i++) {
			if(ufvl_t[i] == 'u') {
				const ExpressionName &ename = ((ExpressionItem*) ufvl[i])->getName(ufvl_i[i]);
				if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufvl_us[i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufvl_us[i]))) {
					return (Unit*) ufvl[i];
				}
			}
		}
	} else {
		l--;
		for(size_t i = 0; i < ufv[2][l].size(); i++) {
			const ExpressionName &ename = ((ExpressionItem*) ufv[2][l][i])->getName(ufv_i[2][l][i]);
			if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufv_us[2][l][i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufv_us[2][l][i]))) {
				return (Unit*) ufv[2][l][i];
			}
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
	if(v->id() != 0) priv->id_variables[v->id()] = v;
	return v;
}
void Calculator::expressionItemDeactivated(ExpressionItem *item) {
	delUFV(item);
}
void Calculator::expressionItemActivated(ExpressionItem *item) {
	if(item->type() == TYPE_FUNCTION) {
		for(size_t i = 1; i <= item->countNames(); i++) {
			ExpressionItem *item2 = getActiveFunction(item->getName(i).name, !item->getName(i).completion_only);
			if(item2) item2->setActive(false);
		}
	} else {
		for(size_t i = 1; i <= item->countNames(); i++) {
			ExpressionItem *item2 = getActiveVariable(item->getName(i).name, !item->getName(i).completion_only);
			if(item2) item2->setActive(false);
			item2 = getActiveUnit(item->getName(i).name, !item->getName(i).completion_only);
			if(item2) item2->setActive(false);
		}
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
	delUFV(item);

	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		if(item->type() == TYPE_VARIABLE || item->type() == TYPE_UNIT) {
			for(size_t i = 0; i < variables.size(); i++) {
				if(!variables[i]->isLocal() && !variables[i]->isActive() && variables[i]->hasName(item->getName(i2).name, item->getName(i2).case_sensitive)) {
					bool b = true;
					for(size_t i3 = 1; i3 <= variables[i]->countNames(); i3++) {
						if(getActiveVariable(variables[i]->getName(i3).name, !variables[i]->getName(i3).completion_only) || getActiveUnit(variables[i]->getName(i3).name, !variables[i]->getName(i3).completion_only)) {
							b = false;
							break;
						}
					}
					if(b) variables[i]->setActive(true);
				}
			}
			for(size_t i = 0; i < units.size(); i++) {
				if(!units[i]->isLocal() && !units[i]->isActive() && units[i]->hasName(item->getName(i2).name, item->getName(i2).case_sensitive)) {
					bool b = true;
					for(size_t i3 = 1; i3 <= units[i]->countNames(); i3++) {
						if(getActiveVariable(units[i]->getName(i3).name, !units[i]->getName(i3).completion_only) || getActiveUnit(units[i]->getName(i3).name, !units[i]->getName(i3).completion_only)) {
							b = false;
							break;
						}
					}
					if(b) units[i]->setActive(true);
				}
			}
		} else {
			for(size_t i = 0; i < functions.size(); i++) {
				if(!functions[i]->isLocal() && !functions[i]->isActive() && functions[i]->hasName(item->getName(i2).name, item->getName(i2).case_sensitive)) {
					bool b = true;
					for(size_t i3 = 1; i3 <= functions[i]->countNames(); i3++) {
						if(getActiveFunction(functions[i]->getName(i3).name, !functions[i]->getName(i3).completion_only)) {
							b = false;
							break;
						}
					}
					if(b) functions[i]->setActive(true);
				}
			}
		}
	}
}
void Calculator::nameChanged(ExpressionItem *item, bool new_item) {
	if(!item->isActive() || item->countNames() == 0) return;
	if(item->type() == TYPE_UNIT && ((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		return;
	}
	size_t l2;
	if(!new_item) delUFV(item);
	size_t itype = 1;
	if(item->type() == TYPE_VARIABLE) itype = 3;
	else if(item->type() == TYPE_UNIT) itype = 2;
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		l2 = item->getName(i2).name.length();
		size_t i_us = 0;
		while(true) {
			if(l2 > UFV_LENGTHS) {
				size_t i = 0, l = 0;
				for(vector<void*>::iterator it = ufvl.begin(); ; ++it) {
					if(it != ufvl.end()) {
						if(ufvl_t[i] == 'f' || ufvl_t[i] == 'u' || ufvl_t[i] == 'v') l = ((ExpressionItem*) (*it))->getName(ufvl_i[i]).name.length();
						else if(ufvl_t[i] == 'p' || ufvl_t[i] == 'P') l = ((Prefix*) (*it))->getName(ufvl_i[i]).name.length();
						l -= priv->ufvl_us[i];
					}
					if(it == ufvl.end()) {
						ufvl.push_back((void*) item);
						switch(item->type()) {
							case TYPE_VARIABLE: {ufvl_t.push_back('v'); break;}
							case TYPE_FUNCTION: {ufvl_t.push_back('f'); break;}
							case TYPE_UNIT: {ufvl_t.push_back('u'); break;}
						}
						priv->ufvl_us.push_back(i_us);
						ufvl_i.push_back(i2);
						break;
					} else {
						if(l < l2 || (i_us == 0 && priv->ufvl_us[i] > 0) || ((i_us == 0 || priv->ufvl_us[i] > 0) && (
						(item->type() == TYPE_VARIABLE && l == l2 && ufvl_t[i] == 'v')
						|| (item->type() == TYPE_FUNCTION && l == l2 && (ufvl_t[i] != 'p' && ufvl_t[i] != 'P'))
						|| (item->type() == TYPE_UNIT && l == l2 && (ufvl_t[i] != 'p' && ufvl_t[i] != 'P' && ufvl_t[i] != 'f'))
						))) {
							ufvl.insert(it, (void*) item);
							switch(item->type()) {
								case TYPE_VARIABLE: {ufvl_t.insert(ufvl_t.begin() + i, 'v'); break;}
								case TYPE_FUNCTION: {ufvl_t.insert(ufvl_t.begin() + i, 'f'); break;}
								case TYPE_UNIT: {ufvl_t.insert(ufvl_t.begin() + i, 'u'); break;}
							}
							priv->ufvl_us.insert(priv->ufvl_us.begin() + i, i_us);
							ufvl_i.insert(ufvl_i.begin() + i, i2);
							break;
						}
					}
					i++;
				}
			} else if(l2 > 0) {
				l2--;
				if(i_us == 0) {
					size_t i = 0;
					for(vector<size_t>::reverse_iterator it = priv->ufv_us[itype][l2].rbegin(); ; ++it) {
						i++;
						if(it == priv->ufv_us[itype][l2].rend()) {
							ufv[itype][l2].insert(ufv[itype][l2].begin(), (void*) item);
							ufv_i[itype][l2].insert(ufv_i[itype][l2].begin(), i2);
							priv->ufv_us[itype][l2].insert(priv->ufv_us[itype][l2].begin(), 0);
							break;
						}
						if((*it) == 0) {
							if(it == priv->ufv_us[itype][l2].rbegin()) {
								ufv[itype][l2].push_back((void*) item);
								ufv_i[itype][l2].push_back(i2);
								priv->ufv_us[itype][l2].push_back(0);
							} else {
								ufv[itype][l2].insert(ufv[itype][l2].end() - i, (void*) item);
								ufv_i[itype][l2].insert(ufv_i[itype][l2].end() - i, i2);
								priv->ufv_us[itype][l2].insert(priv->ufv_us[itype][l2].end() - i, 0);
							}
							break;
						}
					}
				} else {
					ufv[itype][l2].push_back((void*) item);
					ufv_i[itype][l2].push_back(i2);
					priv->ufv_us[itype][l2].push_back(i_us);
				}
				l2++;
			}
			if(i_us > 0 || l2 < 3) break;
			i_us = item->getName(i2).underscoreRemovalAllowed();
			if(i_us == 0) break;
			l2 -= i_us;
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
Variable* Calculator::getVariableById(int id) const {
	switch(id) {
		case VARIABLE_ID_E: {return v_e;}
		case VARIABLE_ID_PI: {return v_pi;}
		case VARIABLE_ID_EULER: {return v_euler;}
		case VARIABLE_ID_CATALAN: {return v_catalan;}
		case VARIABLE_ID_I: {return v_i;}
		case VARIABLE_ID_PLUS_INFINITY: {return v_pinf;}
		case VARIABLE_ID_MINUS_INFINITY: {return v_minf;}
		case VARIABLE_ID_X: {return v_x;}
		case VARIABLE_ID_Y: {return v_y;}
		case VARIABLE_ID_Z: {return v_z;}
		case VARIABLE_ID_N: {return v_n;}
		case VARIABLE_ID_C: {return v_C;}
		case VARIABLE_ID_PERCENT: {return v_percent;}
		case VARIABLE_ID_PERMILLE: {return v_permille;}
		case VARIABLE_ID_PERMYRIAD: {return v_permyriad;}
		case VARIABLE_ID_UNDEFINED: {return v_undef;}
	}
	unordered_map<int, Variable*>::iterator it = priv->id_variables.find(id);
	if(it == priv->id_variables.end()) return NULL;
	return it->second;
}
Variable *Calculator::getActiveVariable(string name_, bool ignore_us) {
	Variable *o = getActiveVariable(name_);
	if(o || !ignore_us || !name_allows_underscore_removal(name_)) return o;
	gsub("_", "", name_);
	return getActiveVariable(name_);
}
Variable* Calculator::getActiveVariable(string name_) {
	if(name_.empty()) return NULL;
	size_t l = name_.length();
	if(l > UFV_LENGTHS) {
		for(size_t i = 0; i < ufvl.size(); i++) {
			if(ufvl_t[i] == 'v') {
				const ExpressionName &ename = ((ExpressionItem*) ufvl[i])->getName(ufvl_i[i]);
				if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufvl_us[i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufvl_us[i]))) {
					return (Variable*) ufvl[i];
				}
			}
		}
	} else {
		l--;
		for(size_t i = 0; i < ufv[3][l].size(); i++) {
			const ExpressionName &ename = ((ExpressionItem*) ufv[3][l][i])->getName(ufv_i[3][l][i]);
			if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufv_us[3][l][i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufv_us[3][l][i]))) {
				return (Variable*) ufv[3][l][i];
			}
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
	if(f->id() != 0) priv->id_functions[f->id()] = f;
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
MathFunction* Calculator::getFunctionById(int id) const {
	switch(id) {
		case FUNCTION_ID_SIN: {return f_sin;}
		case FUNCTION_ID_COS: {return f_cos;}
		case FUNCTION_ID_TAN: {return f_tan;}
		case FUNCTION_ID_ASIN: {return f_asin;}
		case FUNCTION_ID_ACOS: {return f_acos;}
		case FUNCTION_ID_ATAN: {return f_atan;}
		case FUNCTION_ID_SINH: {return f_sinh;}
		case FUNCTION_ID_COSH: {return f_cosh;}
		case FUNCTION_ID_TANH: {return f_tanh;}
		case FUNCTION_ID_ASINH: {return f_asinh;}
		case FUNCTION_ID_ACOSH: {return f_acosh;}
		case FUNCTION_ID_ATANH: {return f_atanh;}
		case FUNCTION_ID_SINC: {return f_sinc;}
		case FUNCTION_ID_LOG: {return f_ln;}
		case FUNCTION_ID_LOGN: {return f_logn;}
		case FUNCTION_ID_SQRT: {return f_sqrt;}
		case FUNCTION_ID_CBRT: {return f_cbrt;}
		case FUNCTION_ID_ROOT: {return f_root;}
		case FUNCTION_ID_CIS: {return priv->f_cis;}
		case FUNCTION_ID_ABS: {return f_abs;}
		case FUNCTION_ID_SIGNUM: {return f_signum;}
		case FUNCTION_ID_POLYLOG: {return f_Li;}
		case FUNCTION_ID_GAMMA: {return f_gamma;}
		case FUNCTION_ID_ERF: {return f_erf;}
		case FUNCTION_ID_ERFC: {return f_erfc;}
		case FUNCTION_ID_ERFI: {return priv->f_erfi;}
		case FUNCTION_ID_STRIP_UNITS: {return f_stripunits;}
		case FUNCTION_ID_UNCERTAINTY: {return f_uncertainty;}
		case FUNCTION_ID_INTERVAL: {return f_interval;}
		case FUNCTION_ID_FACTORIAL: {return f_factorial;}
		case FUNCTION_ID_DOUBLE_FACTORIAL: {return f_factorial2;}
		case FUNCTION_ID_I_GAMMA: {return f_igamma;}
		case FUNCTION_ID_SININT: {return f_Si;}
		case FUNCTION_ID_COSINT: {return f_Ci;}
		case FUNCTION_ID_SINHINT: {return f_Shi;}
		case FUNCTION_ID_COSHINT: {return f_Chi;}
		case FUNCTION_ID_LOGINT: {return f_li;}
		case FUNCTION_ID_EXPINT: {return f_Ei;}
		case FUNCTION_ID_FRESNEL_S: {return priv->f_fresnels;}
		case FUNCTION_ID_FRESNEL_C: {return priv->f_fresnelc;}
		case FUNCTION_ID_RE: {return f_re;}
		case FUNCTION_ID_IM: {return f_im;}
		case FUNCTION_ID_ARG: {return f_arg;}
		case FUNCTION_ID_INTEGRATE: {return f_integrate;}
		case FUNCTION_ID_DIFFERENTIATE: {return f_diff;}
		case FUNCTION_ID_BETA: {return f_beta;}
		case FUNCTION_ID_BESSELY: {return f_bessely;}
		case FUNCTION_ID_BESSELJ: {return f_besselj;}
		case FUNCTION_ID_HEAVISIDE: {return f_heaviside;}
		case FUNCTION_ID_DIRAC: {return f_dirac;}
		case FUNCTION_ID_DIGAMMA: {return f_digamma;}
		case FUNCTION_ID_AIRY: {return f_airy;}
		case FUNCTION_ID_ZETA: {return f_zeta;}
		case FUNCTION_ID_LAMBERT_W: {return f_lambert_w;}
		case FUNCTION_ID_IF: {return f_if;}
		case FUNCTION_ID_SHIFT: {return f_shift;}
		case FUNCTION_ID_XOR: {return f_xor;}
		case FUNCTION_ID_WARNING: {return f_warning;}
		case FUNCTION_ID_MESSAGE: {return f_message;}
		case FUNCTION_ID_PLOT: {return f_plot;}
		case FUNCTION_ID_SAVE: {return f_save;}
		case FUNCTION_ID_CONCATENATE: {return f_concatenate;}
		case FUNCTION_ID_SECANT_METHOD: {return priv->f_secant;}
		case FUNCTION_ID_NEWTON_RAPHSON: {return priv->f_newton;}
		case FUNCTION_ID_RAND: {return f_rand;}
	}
	unordered_map<int, MathFunction*>::iterator it = priv->id_functions.find(id);
	if(it == priv->id_functions.end()) return NULL;
	return it->second;
}
MathFunction *Calculator::getActiveFunction(string name_, bool ignore_us) {
	MathFunction *o = getActiveFunction(name_);
	if(o || !ignore_us || !name_allows_underscore_removal(name_)) return o;
	gsub("_", "", name_);
	return getActiveFunction(name_);
}
MathFunction* Calculator::getActiveFunction(string name_) {
	if(name_.empty()) return NULL;
	size_t l = name_.length();
	if(l > UFV_LENGTHS) {
		for(size_t i = 0; i < ufvl.size(); i++) {
			if(ufvl_t[i] == 'f') {
				const ExpressionName &ename = ((ExpressionItem*) ufvl[i])->getName(ufvl_i[i]);
				if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufvl_us[i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufvl_us[i]))) {
					return (MathFunction*) ufvl[i];
				}
			}
		}
	} else {
		l--;
		for(size_t i = 0; i < ufv[1][l].size(); i++) {
			const ExpressionName &ename = ((ExpressionItem*) ufv[1][l][i])->getName(ufv_i[1][l][i]);
			if((ename.case_sensitive && equals_ignore_us(name_, ename.name, priv->ufv_us[1][l][i])) || (!ename.case_sensitive && equalsIgnoreCase(name_, ename.name, priv->ufv_us[1][l][i]))) {
				return (MathFunction*) ufv[1][l][i];
			}
		}
	}
	return NULL;
}
bool Calculator::variableNameIsValid(const string &name_) const {
	return !name_.empty() && name_.find_first_of(ILLEGAL_IN_NAMES) == string::npos && is_not_in(NUMBERS, name_[0]);
}
bool Calculator::functionNameIsValid(const string &name_) const {
	return !name_.empty() && name_.find_first_of(ILLEGAL_IN_NAMES) == string::npos && is_not_in(NUMBERS, name_[0]);
}
bool Calculator::unitNameIsValid(const string &name_) const {
	return !name_.empty() && name_.find_first_of(ILLEGAL_IN_UNITNAMES) == string::npos;
}
bool Calculator::variableNameIsValid(const char *name_) const {
	if(strlen(name_) == 0) return false;
	if(is_in(NUMBERS, name_[0])) return false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_NAMES, name_[i])) return false;
	}
	return true;
}
bool Calculator::functionNameIsValid(const char *name_) const {
	if(strlen(name_) == 0) return false;
	if(is_in(NUMBERS, name_[0])) return false;
	for(size_t i = 0; name_[i] != '\0'; i++) {
		if(is_in(ILLEGAL_IN_NAMES, name_[i])) return false;
	}
	return true;
}
bool Calculator::unitNameIsValid(const char *name_) const {
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
string Calculator::convertToValidVariableName(string name_) const {
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
string Calculator::convertToValidFunctionName(string name_) const {
	if(name_.empty()) return "func_1";
	return convertToValidVariableName(name_);
}
string Calculator::convertToValidUnitName(string name_) const {
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
			case TYPE_VARIABLE: {return variableNameTaken(name, (Variable*) object);}
			case TYPE_UNIT: {return unitNameTaken(name, (Unit*) object);}
			case TYPE_FUNCTION: {return functionNameTaken(name, (MathFunction*) object);}
		}
	} else {
		return getActiveExpressionItem(name) != NULL;
	}
	return false;
}
bool Calculator::variableNameTaken(string name, Variable *object) {
	if(name.empty()) return false;
	Variable *v = getActiveVariable(name, true);
	return (v && v != object) || getActiveUnit(name, true);
}
bool Calculator::unitNameTaken(string name, Unit *object) {
	if(name.empty()) return false;
	bool ignore_us = !object || object->subtype() != SUBTYPE_COMPOSITE_UNIT;
	Unit *u = getActiveUnit(name, ignore_us);
	return (u && u != object) || getActiveVariable(name, ignore_us);
}
bool Calculator::functionNameTaken(string name, MathFunction *object) {
	if(name.empty()) return false;
	MathFunction *f = getActiveFunction(name, true);
	return f && f != object;
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

