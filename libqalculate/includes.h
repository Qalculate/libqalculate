/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna_k@fmgirl.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef INCLUDES_H
#define INCLUDES_H

/** @file */

#include <vector>
#include <string>
#include <stack>
#include <list>
#include <errno.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

/// \cond
using namespace std;
/// \endcond

static string empty_string;

struct ExpressionName;
class Calculator;
class MathStructure;
class Manager;
class Unit;
class Variable;
class KnownVariable;
class UnknownVariable;
class Assumptions;
class DynamicVariable;
class ExpressionItem;
class Number;
class Prefix;
class DecimalPrefix;
class BinaryPrefix;
class NumberPrefix;
class CompositeUnit;
class AliasUnit;
class AliasUnit_Composite;
class MathFunction;
class Matrix;
class Vector;
class UserFunction;
class EqItem;
class EqNumber;
class EqContainer;
class Argument;
class DataSet;
class DataProperty;
class DataObject;

/// Type of ExpressionItem
typedef enum {
	/// class Variable
	TYPE_VARIABLE,
	/// class MathFunction
	TYPE_FUNCTION,
	/// class Unit
	TYPE_UNIT
} ExpressionItemType;

#define COMPARISON_MIGHT_BE_LESS_OR_GREATER(i)	(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_NOT_EQUAL)
#define COMPARISON_NOT_FULLY_KNOWN(i)		(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_EQUAL_OR_LESS || i == COMPARISON_RESULT_EQUAL_OR_GREATER)
#define COMPARISON_IS_EQUAL_OR_GREATER(i)	(i == COMPARISON_RESULT_EQUAL || i == COMPARISON_RESULT_GREATER || i == COMPARISON_RESULT_EQUAL_OR_GREATER)
#define COMPARISON_IS_EQUAL_OR_LESS(i)		(i == COMPARISON_RESULT_EQUAL || i == COMPARISON_RESULT_LESS || i == COMPARISON_RESULT_EQUAL_OR_LESS)
#define COMPARISON_IS_NOT_EQUAL(i)		(i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_LESS || i == COMPARISON_RESULT_GREATER)
#define COMPARISON_MIGHT_BE_EQUAL(i)		(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_LESS || i == COMPARISON_RESULT_EQUAL_OR_GREATER)
#define COMPARISON_MIGHT_BE_NOT_EQUAL(i)	(i == COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_LESS || i == COMPARISON_RESULT_EQUAL_OR_GREATER)

#define NR_OF_PRIMES 174

static const int PRIMES[] = {
2, 3, 5, 7, 11, 13, 17, 19, 21, 23, 29, 31, 37, 41, 31, 37, 
41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 
107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 
173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 
239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 
311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 
383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 
457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523,
541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 
613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 
683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 
769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 
857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 
941, 947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013
};

#define SQP_LT_1000 11
#define SQP_LT_2000 17
#define SQP_LT_10000 28
#define SQP_LT_25000 40
#define SQP_LT_100000 68

static const int SQUARE_PRIMES[] = {
4, 9, 25, 49, 121, 169, 289, 361, 441, 529, 
841, 961, 1369, 1681, 961, 1369, 1681, 1849, 2209, 2809, 
3481, 3721, 4489, 5041, 5329, 6241, 6889, 7921, 9409, 10201, 
10609, 11449, 11881, 12769, 16129, 17161, 18769, 19321, 22201, 22801, 
24649, 26569, 27889, 29929, 32041, 32761, 36481, 37249, 38809, 39601, 
44521, 49729, 51529, 52441, 54289, 57121, 58081, 63001, 66049, 69169, 
72361, 73441, 76729, 78961, 80089, 85849, 94249, 96721, 97969, 100489, 
109561, 113569, 120409, 121801, 124609, 128881, 134689, 139129, 143641, 146689, 
151321, 157609, 160801, 167281, 175561, 177241, 185761, 187489, 192721, 196249, 
201601, 208849, 212521, 214369, 218089, 229441, 237169, 241081, 249001, 253009, 
259081, 271441, 273529, 292681, 299209, 310249, 316969, 323761, 326041, 332929, 
344569, 351649, 358801, 361201, 368449, 375769, 380689, 383161, 398161, 410881, 
413449, 418609, 426409, 434281, 436921, 452929, 458329, 466489, 477481, 491401, 
502681, 516961, 528529, 537289, 546121, 552049, 564001, 573049, 579121, 591361, 
597529, 619369, 635209, 654481, 657721, 674041, 677329, 683929, 687241, 703921, 
727609, 734449, 737881, 744769, 769129, 776161, 779689, 786769, 822649, 829921, 
844561, 863041, 877969, 885481, 896809, 908209, 935089, 942841, 954529, 966289, 
982081, 994009, 1018081, 1026169
};

/// The result of a comparison of two values
typedef enum {
	COMPARISON_RESULT_EQUAL,
	COMPARISON_RESULT_GREATER,
	COMPARISON_RESULT_LESS,
	COMPARISON_RESULT_EQUAL_OR_GREATER,
	COMPARISON_RESULT_EQUAL_OR_LESS,
	COMPARISON_RESULT_NOT_EQUAL,
	COMPARISON_RESULT_UNKNOWN
} ComparisonResult;

/// Placement of legend
typedef enum {
	PLOT_LEGEND_NONE,
	PLOT_LEGEND_TOP_LEFT,
	PLOT_LEGEND_TOP_RIGHT,
	PLOT_LEGEND_BOTTOM_LEFT,
	PLOT_LEGEND_BOTTOM_RIGHT,
	PLOT_LEGEND_BELOW,
	PLOT_LEGEND_OUTSIDE
} PlotLegendPlacement;

/// Plot type/style
typedef enum {
	PLOT_STYLE_LINES,
	PLOT_STYLE_POINTS,
	PLOT_STYLE_POINTS_LINES,
	PLOT_STYLE_BOXES,
	PLOT_STYLE_HISTOGRAM,
	PLOT_STYLE_STEPS,
	PLOT_STYLE_CANDLESTICKS,
	PLOT_STYLE_DOTS
} PlotStyle;

/// Smoothing a plotted lines
typedef enum {
	PLOT_SMOOTHING_NONE,
	PLOT_SMOOTHING_UNIQUE,
	PLOT_SMOOTHING_CSPLINES,
	PLOT_SMOOTHING_BEZIER,
	PLOT_SMOOTHING_SBEZIER
} PlotSmoothing;

/// File type for saving plot to image
typedef enum {
	PLOT_FILETYPE_AUTO,
	PLOT_FILETYPE_PNG,
	PLOT_FILETYPE_PS,
	PLOT_FILETYPE_EPS,
	PLOT_FILETYPE_LATEX,
	PLOT_FILETYPE_SVG,
	PLOT_FILETYPE_FIG
} PlotFileType;

/// Mathematical operations
typedef enum {
	OPERATION_MULTIPLY,
	OPERATION_DIVIDE,
	OPERATION_ADD,
	OPERATION_SUBTRACT,
	OPERATION_RAISE,
	OPERATION_EXP10,
	OPERATION_LOGICAL_AND,
	OPERATION_LOGICAL_OR,
	OPERATION_LOGICAL_XOR,
	OPERATION_BITWISE_AND,
	OPERATION_BITWISE_OR,
	OPERATION_BITWISE_XOR,
	OPERATION_LESS,
	OPERATION_GREATER,
	OPERATION_EQUALS_LESS,
	OPERATION_EQUALS_GREATER,
	OPERATION_EQUALS,
	OPERATION_NOT_EQUALS
} MathOperation;

/// Comparison signs for comparison structures
typedef enum {
	COMPARISON_LESS,
	COMPARISON_GREATER,
	COMPARISON_EQUALS_LESS,
	COMPARISON_EQUALS_GREATER,
	COMPARISON_EQUALS,
	COMPARISON_NOT_EQUALS
} ComparisonType;

typedef enum {
	SORT_DEFAULT				= 1 << 0,
	SORT_SCIENTIFIC				= 1 << 1
} SortFlags;

#define BASE_ROMAN_NUMERALS	-1
#define BASE_TIME		-2
#define BASE_BINARY		2
#define BASE_OCTAL		8
#define BASE_DECIMAL		10
#define BASE_HEXADECIMAL	16
#define BASE_SEXAGESIMAL	60

#define EXP_BASE_3		-3
#define EXP_PRECISION		-1
#define EXP_NONE		0
#define EXP_PURE		1
#define EXP_SCIENTIFIC		3

typedef enum {
	/// Display numbers in decimal, not fractional, format (ex. 0.333333)
	FRACTION_DECIMAL,
	/// Display as fraction if necessary to get an exact display of the result (ex. 1/3, but 0.25)
	FRACTION_DECIMAL_EXACT,
	/// Display as fraction (ex. 4/3)
	FRACTION_FRACTIONAL,
	/// Display as an integer and a fraction (ex. 3 + 1/2)
	FRACTION_COMBINED
} NumberFractionFormat;

/// Options for ordering the parts of a mathematical expression/result before display
static const struct SortOptions {
	/// Put currency units before quantity. Default: true
	bool prefix_currencies;
	/// If true, avoid placing negative terms first. Default: true
	bool minus_last;
	SortOptions() : prefix_currencies(true), minus_last(true) {}
} default_sort_options;

typedef enum {
	MULTIPLICATION_SIGN_ASTERISK,
	MULTIPLICATION_SIGN_DOT,
	MULTIPLICATION_SIGN_X
} MultiplicationSign;

typedef enum {
	DIVISION_SIGN_SLASH,
	DIVISION_SIGN_DIVISION_SLASH,
	DIVISION_SIGN_DIVISION
} DivisionSign;

typedef enum {
	BASE_DISPLAY_NONE,
	BASE_DISPLAY_NORMAL,
	BASE_DISPLAY_ALTERNATIVE
} BaseDisplay;

/// Options for formatting and display of mathematical structures/results.
static const struct PrintOptions {
	int min_exp;
	/// Number base for displaying numbers. Default: 10
	int base;
	/// How prefixes for numbers in non-decimal bases will be displayed
	BaseDisplay base_display;
	/// Use lower case for non-numeric characters for bases > 10. Default: false
	bool lower_case_numbers;
	/// Use lower case e for base-10 exponent (ex. 1.2e8 instead of 1.2E8). Default: false
	bool lower_case_e;
	/// If rational numbers will be displayed with decimals, as a fraction, or something in between. Default: FRACTION_DECIMAL
	NumberFractionFormat number_fraction_format;
	/// Show that the digit series of a number continues forever with three dots, instead of rounding (ex. 2/3 displays as 0.666666... instead of 0.666667). Default: false
	bool indicate_infinite_series;
	/// Show ending zeroes for approximate numbers to indicate precision (ex.1.2300000 instead of 1.23) . Default: false
	bool show_ending_zeroes;
	/// Prefer abbreviated names of variables, units, functions etc. Default: true
	bool abbreviate_names;
	/// Prefer reference names of variables, units, functions etc. Default: false
	bool use_reference_names;
	/// Isolate units at the end of the displayed expression (ex. x/y m/s instead of (x m)/(y s)). Default: true
	bool place_units_separately;
	/// Use prefixes for units when appropriate. Default: true
	bool use_unit_prefixes;
	/// Use prefixes for currencies if unit prefixes are om. Default: false
	bool use_prefixes_for_all_units;
	/// Use all decimal SI prefixes. If false, prefixes which is not a multiple of thousand (centi, deci, deka, hekto) will not be used automatically. Default: false
	bool use_prefixes_for_currencies;
	/// Use prefixes for all units (even imperial and similar ones). Default: false
	bool use_all_prefixes;
	/// If set to true, prefixes will be split between numerator and denominator in a unit expression (millimeter per kilogram instead of micrometer per gram). Default: true
	bool use_denominator_prefix;
	/// If true, negative exponents will be used instead of division (ex. 5/x^2 becomes 5*x^-2). Default: false
	bool negative_exponents;
	/// Avoid using multiplication sign, when appropriate. Default: true
	bool short_multiplication;
	/// Use a format compatible with ParseOptions::limit_implicit_multiplication. Default: false
	bool limit_implicit_multiplication;
	/// If it is not necessary that the displayed expression can be parsed correctly. Default: false
	bool allow_non_usable;
	/// If unicode signs can be displayed. Default: false
	bool use_unicode_signs;
	/// Sign used for display of multiplication. Default: MULTIPLICATION_SIGN_DOT
	MultiplicationSign multiplication_sign;
	/// Sign used for display of division. Default: DIVISION_SIGN_DIVISION_SLASH
	DivisionSign division_sign;
	/// If space will be used to make the output look nicer. Default: true
	bool spacious;
	/// Use parentheses even when not necessary. Default: false
	bool excessive_parenthesis;
	/// Transform raised to 1/2 to square root function. Default: true
	bool halfexp_to_sqrt;
	/// Minimum number of decimals to display for numbers. Default: 0
	int min_decimals;
	/// Maximum number of decimals to display for numbers. A negative value disables the limit. Default: -1
	int max_decimals;
	/// Enable use of min_decimals. False is equivalent to a min_decimals value of zero. Default: true
	bool use_min_decimals;
	/// Enable use of max_decimals. False is equivalent to a negative max_decimals value. Default: true
	bool use_max_decimals;
	/// If true round halfway numbers to nearest even number, otherwise round upwards. Default: false
	bool round_halfway_to_even;
	/// Multiply numerator and denominator to get integers (ex. (6x+y)/2z instead of (3x+0.5y)/z). Default: true
	bool improve_division_multipliers;
	/// Force use of a specific prefix for units if not NULL. Default: NULL
	Prefix *prefix;
	/// If not NULL will be set to true if the output is approximate. Default: NULL
	bool *is_approximate;
	/// Options for the order of values in the displayed expression. Default: default_sort_options
	SortOptions sort_options;
	/// Comma sign or empty string to use default comma sign. Default: empty string
	string comma_sign;
	/// Decimal sign or empty string to use default decimal sign. Default: empty string
	string decimalpoint_sign;
	/// Function that returns true if a text string with unicode signs can be properly displayed. Default: NULL
	bool (*can_display_unicode_string_function) (const char*, void*);
	/// Argument passed to can_display_unicode_string_function. Default: NULL
	void *can_display_unicode_string_arg;
	/// Replace underscores in names with spaces, unless name has suffix. Default: false
	bool hide_underscore_spaces;
	/// Preserves the format of the structure (no sorting, no changed prefixes, no improved division multipliers, etc.). Default: false
	bool preserve_format;
	/// Allows factorization to occur in the output (should be set to true if the structure has been factorized). Default: false
	bool allow_factorization;
	/// If logical operators will be spelled as AND and OR instead of && and ||. Default: false
	bool spell_out_logical_operators;
	/// Displays children of the structure with no higher precision than the parent. Default: true
	bool restrict_to_parent_precision;
	PrintOptions() : min_exp(EXP_PRECISION), base(BASE_DECIMAL), lower_case_numbers(false), lower_case_e(false), number_fraction_format(FRACTION_DECIMAL), indicate_infinite_series(false), show_ending_zeroes(false), abbreviate_names(true), use_reference_names(false), place_units_separately(true), use_unit_prefixes(true), use_prefixes_for_all_units(false), use_prefixes_for_currencies(false), use_all_prefixes(false), use_denominator_prefix(true), negative_exponents(false), short_multiplication(true), limit_implicit_multiplication(false), allow_non_usable(false), use_unicode_signs(false), multiplication_sign(MULTIPLICATION_SIGN_DOT), division_sign(DIVISION_SIGN_DIVISION_SLASH), spacious(true), excessive_parenthesis(false), halfexp_to_sqrt(true), min_decimals(0), max_decimals(-1), use_min_decimals(true), use_max_decimals(true), round_halfway_to_even(false), improve_division_multipliers(true), prefix(NULL), is_approximate(NULL), can_display_unicode_string_function(NULL), can_display_unicode_string_arg(NULL), hide_underscore_spaces(false), preserve_format(false), allow_factorization(false), spell_out_logical_operators(false), restrict_to_parent_precision(true) {}
	/// Returns the comma sign used (default sign or comma_sign)
	const string &comma() const;
	/// Returns the decimal sign used (default sign or decimalpoint_sign)
	const string &decimalpoint() const;
} default_print_options;

static const struct InternalPrintStruct {
	int depth, power_depth, division_depth;
	bool wrap;
	string *num, *den, *re, *im, *exp;
	bool *minus, *exp_minus;
	bool parent_approximate;
	int parent_precision;
	InternalPrintStruct() : depth(0), power_depth(0), division_depth(0), wrap(false), num(NULL), den(NULL), re(NULL), im(NULL), exp(NULL), minus(NULL), exp_minus(NULL), parent_approximate(false), parent_precision(-1) {}
} top_ips;

typedef enum {
	/// Allow only exact results
	APPROXIMATION_EXACT,
	/// Try to make the result as exact as possible
	APPROXIMATION_TRY_EXACT,
	/// Calculate the result approximately directly
	APPROXIMATION_APPROXIMATE
} ApproximationMode;

typedef enum {
	/// Do not do any factorization or additional simplifications
	STRUCTURING_NONE,
	/// Simplify the result as much as possible
	STRUCTURING_SIMPLIFY,
	/// Factorize the result
	STRUCTURING_FACTORIZE
} StructuringMode;

typedef enum {
	/// Do not do any conversion of units in addition to syncing
	POST_CONVERSION_NONE,
	/// Convert to the best suited SI units (the least amount of units)
	POST_CONVERSION_BEST,
	/// Convert to base units
	POST_CONVERSION_BASE
} AutoPostConversion;

typedef enum {
	DONT_READ_PRECISION,
	ALWAYS_READ_PRECISION,
	READ_PRECISION_WHEN_DECIMALS
} ReadPrecisionMode;

typedef enum {
	ANGLE_UNIT_NONE,
	ANGLE_UNIT_RADIANS,
	ANGLE_UNIT_DEGREES,
	ANGLE_UNIT_GRADIANS
} AngleUnit;

typedef enum {
	/// The default adaptive mode works as the "parse implicit multiplication first" mode, unless spaces are found (<quote>1/5x = 1/(5*x)</quote>, but <quote>1/5 x = (1/5)*x</quote>). In the adaptive mode unit expressions are parsed separately (<quote>5 m/5 m/s = (5*m)/(5*(m/s)) = 1 s</quote>).
	PARSING_MODE_ADAPTIVE,
	/// In the "parse implicit multiplication first" mode, implicit multiplication is parsed before explicit multiplication (<quote>12/2(1+2) = 12/(2*3) = 2</quote>, <quote>5x/5y = (5*x)/(5*y) = x/y</quote>).
	PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST,
	/// In the conventional mode implicit multiplication does not differ from explicit multiplication (<quote>12/2(1+2) = 12/2*3 = 18</quote>, <quote>5x/5y = 5*x/5*y = xy</quote>).
	PARSING_MODE_CONVENTIONAL
} ParsingMode;

/// Options for parsing expressions.
static const struct ParseOptions {
	/// If variables will be parsed. Default: true
	bool variables_enabled;
	/// If functions will be parsed. Default: true
	bool functions_enabled;
	/// If left-over characters will be parsed as symbols. Default: true
	bool unknowns_enabled;
	/// If units will be parsed. Default: true
	bool units_enabled;
	/// If Reverse Polish Notation syntax will be used. Default: false
	bool rpn;
	/// Base of parsed numbers. Default: 10
	int base;
	/// When implicit multiplication is limited variables, functions and units must be separated by a space, operator or parenthesis ("xy" does not equal "x * y").  Default: false
	/**
	* If the limit implicit multiplication mode is activated, the use of implicite multiplication when parsing expressions and displaying results will be limited to avoid confusion. For example, if this mode is not activated and "integrte(5x)" is accidently typed instead of "integrate(5x)", the expression is interpreted as "int(e * e * (5 * x) * gr * t)". If limit implicit multiplication is turned on to mistyped expression would instead show an error telling that "integrte" is not a valid variable, function or unit (unless unknowns is not enabled in which case the result will be "5 'integrate' * x".
	*/
	bool limit_implicit_multiplication;
	/// If and when precisions will be read from number of digits in a number. Default: DONT_READ_PRECISION
	ReadPrecisionMode read_precision;
	/// If true dots will ignored if another character is the default decimal sign, to allow dots to be used as thousand separator. Default: false
	bool dot_as_separator;
	/// If true commas will ignored if another character is the default decimal sign, to allow commas to be used as thousand separator. You also need to call CALCULATOR->useDecimalPoint(true). Default: false
	bool comma_as_separator;
	///Interpret square brackets equally to parentheses (not only for vectors/matrices). Default; false
	bool brackets_as_parentheses;
	/// Default angle unit for trigonometric functions. Default: ANGLE_UNIT_NONE
	AngleUnit angle_unit;
	/// If non-NULL will be set to unfinished function at the end of the expression (if there is one). Default: NULL
	MathStructure *unended_function;
	/// Preserve the expression structure as much as possible. Default: false
	bool preserve_format;
	/// Defaukt dataset. Used for object.property syntax without a preceeding data set. Default: NULL
	DataSet *default_dataset;
	/// Convert degree Celsius and Fahrenheit to Kelvin already when parsing the expression. Turn off when parsing unit expression to convert to. Default: true
	bool convert_temperature_units;
	/// Parsing mode. Default: PARSING_MODE_ADAPTIVE
	ParsingMode parsing_mode;
	ParseOptions() : variables_enabled(true), functions_enabled(true), unknowns_enabled(true), units_enabled(true), rpn(false), base(BASE_DECIMAL), limit_implicit_multiplication(false), read_precision(DONT_READ_PRECISION), dot_as_separator(false), brackets_as_parentheses(false), angle_unit(ANGLE_UNIT_NONE), unended_function(NULL), preserve_format(false), default_dataset(NULL), convert_temperature_units(true), parsing_mode(PARSING_MODE_ADAPTIVE) {}
} default_parse_options;

/// Options for calculation.
static const struct EvaluationOptions {
	/// How exact the result must be. Default: TRY_EXACT
	ApproximationMode approximation;
	/// If units will be synced/converted to allow evaluation (ex. 1 min + 1 s=60 s+ 1 s = 61 s). Default: true
	bool sync_units;
	/// If units with complex/non-linear relations (ex. degress celsius and fahrenheit) will synced/converted. Default: true
	bool sync_complex_unit_relations;
	/// If unit prefixes in original expression will be kept. Default: false
	bool keep_prefixes;
	/// If known variables will be replaced by their value. Default: true
	bool calculate_variables;
	/// If functions will be calculated. Default: true
	bool calculate_functions;
	/// If comparisons will be evaluated (ex. 5>2 => 1). Default: true
	bool test_comparisons;
	/// If a varaible will be isolated to the left side in equations/comparisons (ex. x+y=2 => x=2-y). Default: true
	bool  isolate_x;
	/// If factors (and bases) containing addition will be expanded (ex. z(x+y)=zx+zy). Default: true
	int expand;
	/// If non-numerical parts of a fraction will be reduced (ex. (5x)/(3xy) =5/(3y) .  Default: true
	bool reduce_divisions;
	/// If complex numbers will be used for evaluation. Default: true
	bool allow_complex;
	/// If infinite numbers will be used for evaluation. Default: true
	bool allow_infinite;
	/// If simplification will be made easier by assuming that denominators with unknown value not is zero. Default: false
	bool assume_denominators_nonzero;
	/// Warn if a denominator with unknown value was assumed non-zero (with assume_denominators_nonzero set to true) to allow simplification. Default: false
	bool warn_about_denominators_assumed_nonzero;
	/// If powers with exponent 1/2 that only have an approximate result will be split to the least base (sqrt(8) = 2 * sqrt(2)). Default: true
	bool split_squares;
	/// If units with zero quantity will be preserved. Default: true
	bool keep_zero_units;
	/// If and how units will be automatically converted. Does not affect syncing of units. Default: POST_CONVERSION_NONE
	AutoPostConversion auto_post_conversion;
	/// If the evaluation result will be simplified or factorized Default: STRUCTURING_NONE
	StructuringMode structuring;
	/// Options for parsing of expression. Default: default_parse_options
	ParseOptions parse_options;
	/// If set will decide which variable to isolate in an equation. Default: NULL
	const MathStructure *isolate_var;
	EvaluationOptions() : approximation(APPROXIMATION_TRY_EXACT), sync_units(true), sync_complex_unit_relations(true), keep_prefixes(false), calculate_variables(true), calculate_functions(true), test_comparisons(true), isolate_x(true), expand(true), reduce_divisions(true), allow_complex(true), allow_infinite(true), assume_denominators_nonzero(false), warn_about_denominators_assumed_nonzero(false), split_squares(true), keep_zero_units(true), auto_post_conversion(POST_CONVERSION_NONE), structuring(STRUCTURING_SIMPLIFY), isolate_var(NULL) {}
} default_evaluation_options;

extern MathStructure m_undefined, m_empty_vector, m_empty_matrix, m_zero, m_one, m_minus_one;
extern Number nr_zero, nr_one, nr_minus_one;
extern EvaluationOptions no_evaluation;
extern ExpressionName empty_expression_name;

extern Calculator *calculator;

#define CALCULATOR	calculator

#define DEFAULT_PRECISION	8
#define PRECISION		CALCULATOR->getPrecision()

#define SIGN_DEGREE			"°"
#define SIGN_POWER_0			"⁰"
#define SIGN_POWER_1			"¹"
#define SIGN_POWER_2			"²"
#define SIGN_POWER_3			"³"
#define SIGN_POWER_4			"³"
#define SIGN_POWER_5			"⁴"
#define SIGN_POWER_6			"⁶"
#define SIGN_POWER_7			"⁷"
#define SIGN_POWER_8			"⁸"
#define SIGN_POWER_9			"⁹"
#define SIGN_EURO			"€"
#define SIGN_POUND			"£"
#define SIGN_CENT			"¢"
#define SIGN_YEN			"¥"
#define SIGN_MICRO			"µ"
#define SIGN_PI				"π"
#define SIGN_MULTIPLICATION		"×"
#define SIGN_MULTIDOT			"⋅"
#define SIGN_MULTIBULLET		"∙"
#define SIGN_SMALLCIRCLE		"•"
#define SIGN_DIVISION_SLASH		"∕"
#define SIGN_DIVISION			"÷"
#define SIGN_MINUS			"−"
#define SIGN_PLUS			"＋"
#define SIGN_SQRT			"√"
#define SIGN_ALMOST_EQUAL		"≈"
#define SIGN_APPROXIMATELY_EQUAL	"≅"
#define	SIGN_ZETA			"ζ"
#define SIGN_GAMMA			"γ"
#define SIGN_PHI			"φ"
#define	SIGN_LESS_OR_EQUAL		"≤"
#define	SIGN_GREATER_OR_EQUAL		"≥"
#define	SIGN_NOT_EQUAL			"≠"
#define SIGN_CAPITAL_SIGMA		"Σ"
#define SIGN_CAPITAL_PI			"Π"
#define SIGN_CAPITAL_OMEGA		"Ω"
#define SIGN_CAPITAL_GAMMA		"Γ"
#define SIGN_CAPITAL_BETA		"Β"
#define SIGN_INFINITY			"∞"
#define SIGN_PLUSMINUS			"±"

#define ID_WRAP_LEFT_CH		'{'
#define ID_WRAP_RIGHT_CH	'}'

#define DOT_CH			'.'
#define ZERO_CH			'0'
#define ONE_CH			'1'
#define TWO_CH			'2'
#define THREE_CH		'3'
#define FOUR_CH			'4'
#define FIVE_CH			'5'
#define SIX_CH			'6'
#define SEVEN_CH		'7'
#define EIGHT_CH		'8'
#define NINE_CH			'9'
#define PLUS_CH			'+'
#define MINUS_CH		'-'
#define MULTIPLICATION_CH	'*'
#define MULTIPLICATION_2_CH	' '
#define DIVISION_CH		'/'
#define EXP_CH			'E'
#define EXP2_CH			'e'
#define POWER_CH		'^'
#define SPACE_CH		' '
#define LEFT_PARENTHESIS_CH	'('
#define RIGHT_PARENTHESIS_CH	')'
#define LEFT_VECTOR_WRAP_CH	'['
#define RIGHT_VECTOR_WRAP_CH	']'
#define FUNCTION_VAR_PRE_CH	'\\'
#define COMMA_CH		','
#define NAME_NUMBER_PRE_CH	'_'
#define UNIT_DIVISION_CH	'/'
#define	AND_CH			'&'
#define	OR_CH			'|'
#define	LESS_CH			'<'
#define	GREATER_CH		'>'
#define	BITWISE_NOT_CH		'~'
#define	LOGICAL_NOT_CH		'!'
#define	NOT_CH			'!'
#define EQUALS_CH		'='

#define ID_WRAP_LEFT		"{"	
#define ID_WRAP_RIGHT		"}"	
#define ID_WRAPS		"{}"	
#define DOT			"."
#define SEXADOT			":"
#define COMMA			","
#define COMMAS			",;"
#define NUMBERS			"0123456789"
#define NUMBER_ELEMENTS		"0123456789.:"
#define SIGNS			"+-*/^"
#define OPERATORS		"~+-*/^&|!<>="
#define	PARENTHESISS		"()"
#define LEFT_PARENTHESIS	"("
#define	RIGHT_PARENTHESIS	")"
#define	VECTOR_WRAPS		"[]"
#define LEFT_VECTOR_WRAP	"["
#define	RIGHT_VECTOR_WRAP	"]"
#define	SPACES			" \t\n"
#define SPACE			" "
#define RESERVED		"\'@\\{}?\""
#define PLUS			"+"
#define MINUS			"-"
#define MULTIPLICATION		"*"
#define MULTIPLICATION_2	" "
#define DIVISION		"/"
#define EXP			"E"
#define EXPS			"Ee"
#define	POWER			"^"
#define	LOGICAL_AND		"&&"
#define	LOGICAL_OR		"||"
#define	LOGICAL_NOT		"!"
#define	BITWISE_AND		"&"
#define	BITWISE_OR		"|"
#define	BITWISE_NOT		"~"
#define SHIFT_RIGHT		">>"
#define SHIFT_LEFT		"<<"
#define	LESS			"<"
#define	GREATER			">"
#define	NOT			"!"
#define	EQUALS			"="
#define SINF			"INF"
#define SNAN			"NAN"
#define UNDERSCORE		"_"

#define NOT_IN_NAMES 	RESERVED OPERATORS SPACES SEXADOT DOT VECTOR_WRAPS PARENTHESISS COMMAS

#endif
