/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <stdint.h>

#define QALCULATE_MAJOR_VERSION (5)
#define QALCULATE_MINOR_VERSION (2)
#define QALCULATE_MICRO_VERSION (0)

static std::string empty_string;

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

#define COMPARISON_MIGHT_BE_LESS_OR_GREATER(i)	(i >= COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_NOT_EQUAL)
#define COMPARISON_NOT_FULLY_KNOWN(i)		(i >= COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_EQUAL_OR_LESS || i == COMPARISON_RESULT_EQUAL_OR_GREATER)
#define COMPARISON_IS_EQUAL_OR_GREATER(i)	(i == COMPARISON_RESULT_EQUAL || i == COMPARISON_RESULT_GREATER || i == COMPARISON_RESULT_EQUAL_OR_GREATER)
#define COMPARISON_IS_EQUAL_OR_LESS(i)		(i == COMPARISON_RESULT_EQUAL || i == COMPARISON_RESULT_LESS || i == COMPARISON_RESULT_EQUAL_OR_LESS)
#define COMPARISON_IS_NOT_EQUAL(i)		(i == COMPARISON_RESULT_NOT_EQUAL || i == COMPARISON_RESULT_LESS || i == COMPARISON_RESULT_GREATER)
#define COMPARISON_MIGHT_BE_EQUAL(i)		(i >= COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_LESS || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_EQUAL)
#define COMPARISON_MIGHT_BE_NOT_EQUAL(i)	(i >= COMPARISON_RESULT_UNKNOWN || i == COMPARISON_RESULT_EQUAL_OR_LESS || i == COMPARISON_RESULT_EQUAL_OR_GREATER || i == COMPARISON_RESULT_NOT_EQUAL)

#define NR_OF_PRIMES 600

static const long int PRIMES[] = {
2L, 3L, 5L, 7L, 11L, 13L, 17L, 19L, 23L, 29L,
31L, 37L, 41L, 43L, 47L, 53L, 59L, 61L, 67L, 71L,
73L, 79L, 83L, 89L, 97L, 101L, 103L, 107L, 109L, 113L,
127L, 131L, 137L, 139L, 149L, 151L, 157L, 163L, 167L, 173L,
179L, 181L, 191L, 193L, 197L, 199L, 211L, 223L, 227L, 229L,
233L, 239L, 241L, 251L, 257L, 263L, 269L, 271L, 277L, 281L,
283L, 293L, 307L, 311L, 313L, 317L, 331L, 337L, 347L, 349L,
353L, 359L, 367L, 373L, 379L, 383L, 389L, 397L, 401L, 409L,
419L, 421L, 431L, 433L, 439L, 443L, 449L, 457L, 461L, 463L,
467L, 479L, 487L, 491L, 499L, 503L, 509L, 521L, 523L, 541L,
547L, 557L, 563L, 569L, 571L, 577L, 587L, 593L, 599L, 601L,
607L, 613L, 617L, 619L, 631L, 641L, 643L, 647L, 653L, 659L,
661L, 673L, 677L, 683L, 691L, 701L, 709L, 719L, 727L, 733L,
739L, 743L, 751L, 757L, 761L, 769L, 773L, 787L, 797L, 809L,
811L, 821L, 823L, 827L, 829L, 839L, 853L, 857L, 859L, 863L,
877L, 881L, 883L, 887L, 907L, 911L, 919L, 929L, 937L, 941L,
947L, 953L, 967L, 971L, 977L, 983L, 991L, 997L, 1009L, 1013L,
1019L, 1021L, 1031L, 1033L, 1039L, 1049L, 1051L, 1061L, 1063L, 1069L,
1087L, 1091L, 1093L, 1097L, 1103L, 1109L, 1117L, 1123L, 1129L, 1151L,
1153L, 1163L, 1171L, 1181L, 1187L, 1193L, 1201L, 1213L, 1217L, 1223L,
1229L, 1231L, 1237L, 1249L, 1259L, 1277L, 1279L, 1283L, 1289L, 1291L,
1297L, 1301L, 1303L, 1307L, 1319L, 1321L, 1327L, 1361L, 1367L, 1373L,
1381L, 1399L, 1409L, 1423L, 1427L, 1429L, 1433L, 1439L, 1447L, 1451L,
1453L, 1459L, 1471L, 1481L, 1483L, 1487L, 1489L, 1493L, 1499L, 1511L,
1523L, 1531L, 1543L, 1549L, 1553L, 1559L, 1567L, 1571L, 1579L, 1583L,
1597L, 1601L, 1607L, 1609L, 1613L, 1619L, 1621L, 1627L, 1637L, 1657L,
1663L, 1667L, 1669L, 1693L, 1697L, 1699L, 1709L, 1721L, 1723L, 1733L,
1741L, 1747L, 1753L, 1759L, 1777L, 1783L, 1787L, 1789L, 1801L, 1811L,
1823L, 1831L, 1847L, 1861L, 1867L, 1871L, 1873L, 1877L, 1879L, 1889L,
1901L, 1907L, 1913L, 1931L, 1933L, 1949L, 1951L, 1973L, 1979L, 1987L,
1993L, 1997L, 1999L, 2003L, 2011L, 2017L, 2027L, 2029L, 2039L, 2053L,
2063L, 2069L, 2081L, 2083L, 2087L, 2089L, 2099L, 2111L, 2113L, 2129L,
2131L, 2137L, 2141L, 2143L, 2153L, 2161L, 2179L, 2203L, 2207L, 2213L,
2221L, 2237L, 2239L, 2243L, 2251L, 2267L, 2269L, 2273L, 2281L, 2287L,
2293L, 2297L, 2309L, 2311L, 2333L, 2339L, 2341L, 2347L, 2351L, 2357L,
2371L, 2377L, 2381L, 2383L, 2389L, 2393L, 2399L, 2411L, 2417L, 2423L,
2437L, 2441L, 2447L, 2459L, 2467L, 2473L, 2477L, 2503L, 2521L, 2531L,
2539L, 2543L, 2549L, 2551L, 2557L, 2579L, 2591L, 2593L, 2609L, 2617L,
2621L, 2633L, 2647L, 2657L, 2659L, 2663L, 2671L, 2677L, 2683L, 2687L,
2689L, 2693L, 2699L, 2707L, 2711L, 2713L, 2719L, 2729L, 2731L, 2741L,
2749L, 2753L, 2767L, 2777L, 2789L, 2791L, 2797L, 2801L, 2803L, 2819L,
2833L, 2837L, 2843L, 2851L, 2857L, 2861L, 2879L, 2887L, 2897L, 2903L,
2909L, 2917L, 2927L, 2939L, 2953L, 2957L, 2963L, 2969L, 2971L, 2999L,
3001L, 3011L, 3019L, 3023L, 3037L, 3041L, 3049L, 3061L, 3067L, 3079L,
3083L, 3089L, 3109L, 3119L, 3121L, 3137L, 3163L, 3167L, 3169L, 3181L,
3187L, 3191L, 3203L, 3209L, 3217L, 3221L, 3229L, 3251L, 3253L, 3257L,
3259L, 3271L, 3299L, 3301L, 3307L, 3313L, 3319L, 3323L, 3329L, 3331L,
3343L, 3347L, 3359L, 3361L, 3371L, 3373L, 3389L, 3391L, 3407L, 3413L,
3433L, 3449L, 3457L, 3461L, 3463L, 3467L, 3469L, 3491L, 3499L, 3511L,
3517L, 3527L, 3529L, 3533L, 3539L, 3541L, 3547L, 3557L, 3559L, 3571L,
3581L, 3583L, 3593L, 3607L, 3613L, 3617L, 3623L, 3631L, 3637L, 3643L,
3659L, 3671L, 3673L, 3677L, 3691L, 3697L, 3701L, 3709L, 3719L, 3727L,
3733L, 3739L, 3761L, 3767L, 3769L, 3779L, 3793L, 3797L, 3803L, 3821L,
3823L, 3833L, 3847L, 3851L, 3853L, 3863L, 3877L, 3881L, 3889L, 3907L,
3911L, 3917L, 3919L, 3923L, 3929L, 3931L, 3943L, 3947L, 3967L, 3989L,
4001L, 4003L, 4007L, 4013L, 4019L, 4021L, 4027L, 4049L, 4051L, 4057L,
4073L, 4079L, 4091L, 4093L, 4099L, 4111L, 4127L, 4129L, 4133L, 4139L,
4153L, 4157L, 4159L, 4177L, 4201L, 4211L, 4217L, 4219L, 4229L, 4231L,
4241L, 4243L, 4253L, 4259L, 4261L, 4271L, 4273L, 4283L, 4289L, 4297L,
4327L, 4337L, 4339L, 4349L, 4357L, 4363L, 4373L, 4391L, 4397L, 4409L
};

#define SQP_LT_1000 11
#define SQP_LT_2000 17
#define SQP_LT_10000 28
#define SQP_LT_25000 40
#define SQP_LT_100000 68

#define NR_OF_SQUARE_PRIMES 170

static const long int SQUARE_PRIMES[] = {
4L, 9L, 25L, 49L, 121L, 169L, 289L, 361L, 529L, 841L,
961L, 1369L, 1681L, 1849L, 2209L, 2809L, 3481L, 3721L, 4489L, 5041L,
5329L, 6241L, 6889L, 7921L, 9409L, 10201L, 10609L, 11449L, 11881L, 12769L,
16129L, 17161L, 18769L, 19321L, 22201L, 22801L, 24649L, 26569L, 27889L, 29929L,
32041L, 32761L, 36481L, 37249L, 38809L, 39601L, 44521L, 49729L, 51529L, 52441L,
54289L, 57121L, 58081L, 63001L, 66049L, 69169L, 72361L, 73441L, 76729L, 78961L,
80089L, 85849L, 94249L, 96721L, 97969L, 100489L, 109561L, 113569L, 120409L, 121801L,
124609L, 128881L, 134689L, 139129L, 143641L, 146689L, 151321L, 157609L, 160801L, 167281L,
175561L, 177241L, 185761L, 187489L, 192721L, 196249L, 201601L, 208849L, 212521L, 214369L,
218089L, 229441L, 237169L, 241081L, 249001L, 253009L, 259081L, 271441L, 273529L, 292681L,
299209L, 310249L, 316969L, 323761L, 326041L, 332929L, 344569L, 351649L, 358801L, 361201L,
368449L, 375769L, 380689L, 383161L, 398161L, 410881L, 413449L, 418609L, 426409L, 434281L,
436921L, 452929L, 458329L, 466489L, 477481L, 491401L, 502681L, 516961L, 528529L, 537289L,
546121L, 552049L, 564001L, 573049L, 579121L, 591361L, 597529L, 619369L, 635209L, 654481L,
657721L, 674041L, 677329L, 683929L, 687241L, 703921L, 727609L, 734449L, 737881L, 744769L,
769129L, 776161L, 779689L, 786769L, 822649L, 829921L, 844561L, 863041L, 877969L, 885481L,
896809L, 908209L, 935089L, 942841L, 954529L, 966289L, 982081L, 994009L, 1018081L, 1026169L
};

#define LARGEST_RAISED_PRIME_EXPONENT 10

static const long int RAISED_PRIMES[][49] = {
{8L, 27L, 125L, 343L, 1331L, 2197, 4913, 6859, 12167L, 24389L,
29791L, 50653L, 68921L, 79507L, 103823L, 148877L, 205379L, 226981L, 300763L, 357911L,
389017L, 493039L, 571787L, 704969L, 912673L, 1030301L, 1092727L, 1225043L, 1295029, 1442897,
2048383L, 2248091L, 2571353L, 2685619L, 3307949L, 3442951L, 3869893L, 4330747L, 4657463L, 5177717L,
5735339L, 5929741L, 6967871L, 7189057L, 7645373L, 7880599L, 9393931L, 11089567L, 0},
{16L, 81L, 625L, 2401L, 14641L, 28561L, 83521L, 130321L, 279841L, 707281L,
923521L, 1874161L, 2825761L, 3418801L, 4879681L, 7890481L, 12117361L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{32L, 243L, 3125L, 16807L, 161051L, 371293L, 1419857L, 2476099L, 6436343L, 20511149L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{64L, 729L, 15625L, 117649L, 1771561L, 4826809L, 24137569L, 47045881L, 148035889L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{128L, 2187L, 78125L, 823543L, 19487171L, 62748517L, 410338673L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{256L, 6561L, 390625L, 5764801L, 214358881L, 815730721L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{512L, 19683L, 1953125L, 40353607L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{1024L, 59049L, 9765625L, 282475249L,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/// The result of a comparison of two values
typedef enum {
	COMPARISON_RESULT_EQUAL,
	COMPARISON_RESULT_GREATER,
	COMPARISON_RESULT_LESS,
	COMPARISON_RESULT_EQUAL_OR_GREATER,
	COMPARISON_RESULT_EQUAL_OR_LESS,
	COMPARISON_RESULT_NOT_EQUAL,
	COMPARISON_RESULT_UNKNOWN,
	COMPARISON_RESULT_EQUAL_LIMITS,
	COMPARISON_RESULT_CONTAINS,
	COMPARISON_RESULT_CONTAINED,
	COMPARISON_RESULT_OVERLAPPING_LESS,
	COMPARISON_RESULT_OVERLAPPING_GREATER
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
	PLOT_STYLE_DOTS,
	PLOT_STYLE_POLAR
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
	PLOT_FILETYPE_FIG,
	PLOT_FILETYPE_PDF
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

/// Special values for PrintOptions::base and ParseOptions::base
#define BASE_ROMAN_NUMERALS	-1
#define BASE_TIME		-2
#define BASE_BINARY		2
#define BASE_OCTAL		8
#define BASE_DECIMAL		10
#define BASE_DUODECIMAL		12
#define BASE_HEXADECIMAL	16
#define BASE_SEXAGESIMAL	60
#define BASE_SEXAGESIMAL_2	62
#define BASE_SEXAGESIMAL_3	63
#define BASE_LATITUDE		70
#define BASE_LATITUDE_2		71
#define BASE_LONGITUDE		72
#define BASE_LONGITUDE_2	73
/// Use Calculate::setCustomOutputBase() or Calculate::setCustomInputBase() to specify a number base which greater than 36, negative, or a non-integer
#define BASE_CUSTOM		-3
#define BASE_UNICODE		-4
#define BASE_GOLDEN_RATIO	-5
#define BASE_SUPER_GOLDEN_RATIO	-6
#define BASE_PI			-7
#define BASE_E			-8
#define BASE_SQRT2		-9
#define BASE_BINARY_DECIMAL	-20
#define BASE_BIJECTIVE_26	-26
#define BASE_FP16		-30
#define BASE_FP32		-31
#define BASE_FP64		-32
#define BASE_FP128		-33
#define BASE_FP80		-34

#define BASE_IS_SEXAGESIMAL(x) ((x >= BASE_SEXAGESIMAL && x <= BASE_SEXAGESIMAL_3) || (x >= BASE_LATITUDE && x <= BASE_LONGITUDE_2))

/// Primary values for PrintOptions::min_exp
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
	FRACTION_COMBINED,
	/// Display as fraction with denominator specified using Calculator::setFixedDenominator(). Both rational and non-rational numbers are rounded to match the selected denominator.
	FRACTION_FRACTIONAL_FIXED_DENOMINATOR,
	/// Display as an integer and a fraction with denominator specified using Calculator::setFixedDenominator(). Both rational and non-rational numbers are rounded to match the selected denominator.
	FRACTION_COMBINED_FIXED_DENOMINATOR,
	/// Display numbers in decimal format multiplied by percent
	FRACTION_PERCENT,
	/// Display numbers in decimal format multiplied by permille
	FRACTION_PERMILLE,
	/// Display numbers in decimal format multiplied by permyriad
	FRACTION_PERMYRIAD
} NumberFractionFormat;

/// Options for ordering the parts of a mathematical expression/result before display
struct SortOptions {
	/// Put currency units before quantity. Default: true
	bool prefix_currencies;
	/// If true, avoid placing negative terms first. Default: true
	bool minus_last;
	SortOptions() : prefix_currencies(true), minus_last(true) {}
};

static const SortOptions default_sort_options;

typedef enum {
	MULTIPLICATION_SIGN_ASTERISK,
	MULTIPLICATION_SIGN_DOT,
	MULTIPLICATION_SIGN_X,
	MULTIPLICATION_SIGN_ALTDOT
} MultiplicationSign;

typedef enum {
	DIVISION_SIGN_SLASH,
	DIVISION_SIGN_DIVISION_SLASH,
	DIVISION_SIGN_DIVISION
} DivisionSign;

typedef enum {
	BASE_DISPLAY_NONE,
	BASE_DISPLAY_NORMAL,
	BASE_DISPLAY_ALTERNATIVE,
	BASE_DISPLAY_SUFFIX
} BaseDisplay;

typedef enum {
	INTERVAL_DISPLAY_SIGNIFICANT_DIGITS,
	INTERVAL_DISPLAY_INTERVAL,
	INTERVAL_DISPLAY_PLUSMINUS,
	INTERVAL_DISPLAY_MIDPOINT,
	INTERVAL_DISPLAY_LOWER,
	INTERVAL_DISPLAY_UPPER,
	INTERVAL_DISPLAY_CONCISE,
	INTERVAL_DISPLAY_RELATIVE
} IntervalDisplay;

typedef enum {
	DIGIT_GROUPING_NONE,
	DIGIT_GROUPING_STANDARD,
	DIGIT_GROUPING_LOCALE
} DigitGrouping;

typedef enum {
	DATE_TIME_FORMAT_ISO,
	DATE_TIME_FORMAT_LOCALE
} DateTimeFormat;

typedef enum {
	TIME_ZONE_UTC,
	TIME_ZONE_LOCAL,
	TIME_ZONE_CUSTOM
} TimeZone;

typedef enum {
	EXP_DEFAULT,
	EXP_UPPERCASE_E,
	EXP_LOWERCASE_E,
	EXP_POWER_OF_10
} ExpDisplay;

typedef enum {
	ROUNDING_HALF_AWAY_FROM_ZERO,
	ROUNDING_HALF_TO_EVEN,
	ROUNDING_HALF_TO_ODD,
	ROUNDING_HALF_TOWARD_ZERO,
	ROUNDING_HALF_UP,
	ROUNDING_HALF_DOWN,
	ROUNDING_HALF_RANDOM,
	ROUNDING_TOWARD_ZERO,
	ROUNDING_AWAY_FROM_ZERO,
	ROUNDING_UP,
	ROUNDING_DOWN
} RoundingMode;

enum {
	UNICODE_SIGNS_OFF,
	UNICODE_SIGNS_ON,
	UNICODE_SIGNS_ONLY_UNIT_EXPONENTS,
	UNICODE_SIGNS_WITHOUT_EXPONENTS
};

// temporary custom time zone value for truncation rounding in output
#define TZ_TRUNCATE -21586
// temporary custom time zone value for special duodecimal symbols in output
#define TZ_DOZENAL -53172

/// Options for formatting and display of mathematical structures/results.
struct PrintOptions {
	/// Determines the minimum exponent with scientific notation. Values < -1 restricts the exponent to multiples of -min_exp, e.g. 20 000 is output as 20e3 (engineering notation) if min_exp is -3. The special value EXP_PRECISION (-1) uses the current precision to determine the minimum exponent. Default: EXP_PRECISION
	int min_exp;
	/// Number base for displaying numbers. Specify a value between 2 and 36 or use one of the special values defined in includes.h (BASE_*). Default: 10
	int base;
	/// How prefixes for numbers in non-decimal bases will be displayed. Default: BASE_DISPLAY_NONE
	BaseDisplay base_display;
	/// Use lower case for non-numeric characters for bases > 10. Default: false
	bool lower_case_numbers;
	/// Deprecated: use exp_mode instead
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
	int use_unicode_signs;
	/// Sign used for display of multiplication. Default: MULTIPLICATION_SIGN_DOT
	MultiplicationSign multiplication_sign;
	/// Sign used for display of division. Default: DIVISION_SIGN_DIVISION_SLASH
	DivisionSign division_sign;
	/// If space will be used to make the output look nicer. Default: true
	bool spacious;
	/// Use parentheses even when not necessary. Default: false
	bool excessive_parenthesis;
	/// Transform raised to 1/2 to square root and raised to 1/3 to cbrt function. Default: true
	bool halfexp_to_sqrt;
	/// Minimum number of decimals to display for numbers. Default: 0
	int min_decimals;
	/// Maximum number of decimals to display for numbers. A negative value disables the limit. Default: -1
	int max_decimals;
	/// Enable use of min_decimals. False is equivalent to a min_decimals value of zero. Default: true
	bool use_min_decimals;
	/// Enable use of max_decimals. False is equivalent to a negative max_decimals value. Default: true
	bool use_max_decimals;
	/// Deprecated: use rounding
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
	std::string comma_sign;
	/// Decimal sign or empty string to use default decimal sign. Default: empty string
	std::string decimalpoint_sign;
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
	/// Restrict the length of numerators and denominator as integers in decimal mode for fractional display of numbers. Default: false
	bool restrict_fraction_length;
	/// Transform exponentiation positive base and unit fraction exponent (if denominator < 10) to root function. Default: false
	bool exp_to_root;
	/// Use the internal precision of each number instead of global precision. Default: false
	bool preserve_precision;
	/// How number intervals will be displayed. Default: INTERVAL_DISPLAY_INTERVAL
	IntervalDisplay interval_display;
	/// Digit grouping separator. Default: DIGIT_GROUPING_NONE
	DigitGrouping digit_grouping;
	/// Format for time and date. Default: DATE_TIME_FORMAT_ISO
	DateTimeFormat date_time_format;
	/// Time zone for time and date. Default: TIME_ZONE_LOCAL
	TimeZone time_zone;
	/// Offset in minute for custom time zone. Default: 0
	int custom_time_zone;
	/// Negative binary numbers uses two's complement representation. All binary numbers starting with 1 are negative. Default: true
	bool twos_complement;
	/// Negative hexadecimal numbers uses two's complement representation. All hexadecimal numbers starting with 8 or higher are negative. Default: false
	bool hexadecimal_twos_complement;
	/// Number of bits used for binary and hexadecimal numbers. Set to 0 for automatic. Default: 0
	unsigned int binary_bits;
	/// Specifies how numbers using scientific notation are displayed. Default: EXP_DEFAULT
	ExpDisplay exp_display;
	/// Show special duodecimal symbols in output, instead of A and B. Default: false
	bool duodecimal_symbols;
	/// Specifies rounding method used in output. Default: ROUNDING_HALF_AWAY_FROM_ZERO
	RoundingMode rounding;

	PrintOptions();
	/// Returns the comma sign used (default sign or comma_sign)
	const std::string &comma() const;
	/// Returns the decimal sign used (default sign or decimalpoint_sign)
	const std::string &decimalpoint() const;
	/// Returns the digit grouping separator used
};

static const PrintOptions default_print_options;

struct InternalPrintStruct {
	int depth, power_depth, division_depth;
	bool wrap;
	std::string *num, *den, *re, *im, *exp;
	bool *minus, *exp_minus;
	bool parent_approximate;
	int parent_precision;
	long int *iexp;
	InternalPrintStruct();
};

static const InternalPrintStruct top_ips;

typedef enum {
	/// Allow only exact results
	APPROXIMATION_EXACT,
	/// Try to make the result as exact as possible
	APPROXIMATION_TRY_EXACT,
	/// Calculate the result approximately directly
	APPROXIMATION_APPROXIMATE,
	/// Used internally
	APPROXIMATION_EXACT_VARIABLES
} ApproximationMode;

#define STRUCTURING_SIMPLIFY STRUCTURING_EXPAND

typedef enum {
	/// Do not do any factorization or additional simplifications
	STRUCTURING_NONE,
	/// Simplify the result as much as possible and expand (minimal factorization, normally the same as STRUCTURING_NONE)
	STRUCTURING_EXPAND,
	/// Factorize the result
	STRUCTURING_FACTORIZE,
	/// Deprecated: use STRUCTURING_SIMPLIFY instead
	STRUCTURING_HYBRID
} StructuringMode;

typedef enum {
	/// Do not do any conversion of units in addition to syncing
	POST_CONVERSION_NONE,
	/// Convert to the least amount of units. Non-SI units are converted to SI units.
	POST_CONVERSION_OPTIMAL_SI,
	/// Convert to base units
	POST_CONVERSION_BASE,
	/// Convert to the least amount of units. Non-SI units is kept (if optimal), but for conversion only SI units are used.
	POST_CONVERSION_OPTIMAL
} AutoPostConversion;

#define POST_CONVERSION_BEST POST_CONVERSION_OPTIMAL_SI

typedef enum {
	MIXED_UNITS_CONVERSION_NONE,
	MIXED_UNITS_CONVERSION_DOWNWARDS_KEEP,
	MIXED_UNITS_CONVERSION_DOWNWARDS,
	MIXED_UNITS_CONVERSION_DEFAULT,
	MIXED_UNITS_CONVERSION_FORCE_INTEGER,
	MIXED_UNITS_CONVERSION_FORCE_ALL
} MixedUnitsConversion;

typedef enum {
	DONT_READ_PRECISION,
	ALWAYS_READ_PRECISION,
	READ_PRECISION_WHEN_DECIMALS
} ReadPrecisionMode;

typedef enum {
	ANGLE_UNIT_NONE,
	ANGLE_UNIT_RADIANS,
	ANGLE_UNIT_DEGREES,
	ANGLE_UNIT_GRADIANS,
	ANGLE_UNIT_CUSTOM,
} AngleUnit;

typedef enum {
	COMPLEX_NUMBER_FORM_RECTANGULAR,
	COMPLEX_NUMBER_FORM_EXPONENTIAL,
	COMPLEX_NUMBER_FORM_POLAR,
	COMPLEX_NUMBER_FORM_CIS
} ComplexNumberForm;

typedef enum {
	/// The default adaptive mode works as the "parse implicit multiplication first" mode, unless spaces are found (<quote>1/5x = 1/(5*x)</quote>, but <quote>1/5 x = (1/5)*x</quote>). In the adaptive mode unit expressions are parsed separately (<quote>5 m/5 m/s = (5*m)/(5*(m/s)) = 1 s</quote>).
	PARSING_MODE_ADAPTIVE,
	/// In the "parse implicit multiplication first" mode, implicit multiplication is parsed before explicit multiplication (<quote>12/2(1+2) = 12/(2*3) = 2</quote>, <quote>5x/5y = (5*x)/(5*y) = x/y</quote>).
	PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST,
	/// In the conventional mode implicit multiplication does not differ from explicit multiplication (<quote>12/2(1+2) = 12/2*3 = 18</quote>, <quote>5x/5y = 5*x/5*y = xy</quote>).
	PARSING_MODE_CONVENTIONAL,
	// as immediate execution mode in simple traditional calculators (e.g. 5+2*3=(5+2)*3=21)
	PARSING_MODE_CHAIN,
	PARSING_MODE_RPN
} ParsingMode;

#define PARSE_PERCENT_AS_ORDINARY_CONSTANT 0x10

typedef enum {
	/// Ignores uncertainties and uses the middle value of intervals
	INTERVAL_CALCULATION_NONE,
	INTERVAL_CALCULATION_VARIANCE_FORMULA,
	INTERVAL_CALCULATION_INTERVAL_ARITHMETIC,
	/// Treats all intervals as uncorrelated
	INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC
} IntervalCalculation;

/// Options for parsing expressions.
struct ParseOptions {
	/// If variables will be parsed. Default: true
	bool variables_enabled;
	/// If functions will be parsed. Default: true
	bool functions_enabled;
	/// If left-over characters will be parsed as symbols. Default: true
	bool unknowns_enabled;
	/// If units will be parsed. Default: true
	bool units_enabled;
	/// If Reverse Polish Notation syntax will be used. Default: false (deprecated, use parsing_mode = PARSING_MODE_RPN instead)
	bool rpn;
	/// Base of parsed numbers. Specify a value between 2 and 36 or use one of the special values defined in includes.h (BASE_*). Default: 10
	int base;
	/// When implicit multiplication is limited variables, functions and units must be separated by a space, operator or parenthesis ("xy" does not equal "x * y").  Default: false
	/**
	* If the limit implicit multiplication mode is activated, the use of implicit multiplication when parsing expressions and displaying results will be limited to avoid confusion. For example, if this mode is not activated and "integrte(5x)" is accidently typed instead of "integrate(5x)", the expression is interpreted as "int(e * e * (5 * x) * gr * t)". If limit implicit multiplication is turned on to mistyped expression would instead show an error telling that "integrte" is not a valid variable, function or unit (unless unknowns is not enabled in which case the result will be "5 'integrate' * x".
	*/
	bool limit_implicit_multiplication;
	/// If and when precisions will be read from number of digits in a number. Default: DONT_READ_PRECISION
	ReadPrecisionMode read_precision;
	/// If true dots will ignored if another character is the default decimal sign, to allow dots to be used as thousand separator. Default: false
	bool dot_as_separator;
	/// If true commas will ignored if another character is the default decimal sign, to allow commas to be used as thousand separator. You should also call CALCULATOR->useDecimalPoint(true). Default: false
	bool comma_as_separator;
	///Interpret square brackets equally to parentheses (not only for vectors/matrices). Default; false
	bool brackets_as_parentheses;
	/// Default angle unit for trigonometric functions. Default: ANGLE_UNIT_NONE
	AngleUnit angle_unit;
	/// If non-NULL will be set to unfinished function at the end of the expression (if there is one). Default: NULL
	MathStructure *unended_function;
	/// Preserve the expression structure as much as possible. Default: false
	bool preserve_format;
	/// Default dataset. Used for object.property syntax without a preceding data set. Default: NULL
	DataSet *default_dataset;
	/// Parsing mode. Default: PARSING_MODE_ADAPTIVE
	ParsingMode parsing_mode;
	/// Negative binary numbers uses two's complement representation. All binary numbers starting with 1 are assumed to be negative, unless binary_bits is set. Default: false
	bool twos_complement;
	/// Negative hexadecimal numbers uses two's complement representation. All hexadecimal numbers starting with 8 or higher are assumed to be negative, unless binary_bits is set. Default: false
	bool hexadecimal_twos_complement;
	/// Number of bits used for binary and hexadecimal numbers. Set to 0 for automatic. Default: 0
	unsigned int binary_bits;

	ParseOptions();

};

static const ParseOptions default_parse_options;

/// Options for calculation.
struct EvaluationOptions {
	/// How exact the result must be. Default: TRY_EXACT
	ApproximationMode approximation;
	/// If units will be synced/converted to allow evaluation (ex. 1 min + 1 s=60 s+ 1 s = 61 s). Default: true
	bool sync_units;
	/// If units with complex/non-linear relations (ex. degrees Celsius and Fahrenheit) will synced/converted. Default: true
	bool sync_nonlinear_unit_relations;
	/// If unit prefixes in original expression will be kept. Default: false
	bool keep_prefixes;
	/// If known variables will be replaced by their value. Default: true
	bool calculate_variables;
	/// If functions will be calculated. Default: true
	bool calculate_functions;
	/// If comparisons will be evaluated (ex. 5>2 => 1). Default: true
	int test_comparisons;
	/// If a variable will be isolated to the left side in equations/comparisons (ex. x+y=2 => x=2-y). Default: true
	bool isolate_x;
	/// If factors (and bases) containing addition will be expanded (ex. z(x+y)=zx+zy). Default: true
	int expand;
	/// Use behaviour from version <= 0.9.12 which returns (x+y)/z instead of x/y+y/z if expand = true
	bool combine_divisions;
	/// If non-numerical parts of a fraction will be reduced (ex. (5x)/(3xy) =5/(3y) .  Default: true
	bool reduce_divisions;
	/// If complex numbers will be used for evaluation. Default: true
	bool allow_complex;
	/// If infinite numbers will be used for evaluation. Default: true
	bool allow_infinite;
	/// If simplification will be made easier by assuming that denominators with unknown value not is zero. Default: false
	int assume_denominators_nonzero;
	/// Warn if a denominator with unknown value was assumed non-zero (with assume_denominators_nonzero set to true) to allow simplification. Default: true
	bool warn_about_denominators_assumed_nonzero;
	/// If powers with exponent 1/2 that only have an approximate result will be split to the least base (sqrt(8) = 2 * sqrt(2)). Default: true
	bool split_squares;
	/// If units with zero quantity will be preserved. Default: true
	bool keep_zero_units;
	/// If and how units will be automatically converted. Does not affect syncing of units. Default: POST_CONVERSION_OPTIMAL
	AutoPostConversion auto_post_conversion;
	/// Shows time as h + min + s, imperial length as ft + in, etc. Default: MIXED_UNITS_CONVERSION_DEFAULT
	MixedUnitsConversion mixed_units_conversion;
	/// If the evaluation result will be expanded or factorized Default: STRUCTURING_NONE
	StructuringMode structuring;
	/// Options for parsing of expression. Default: default_parse_options
	ParseOptions parse_options;
	/// If set will decide which variable to isolate in an equation. Default: NULL
	const MathStructure *isolate_var;
	/// Use polynomial division to simplify the result. Default: true
	bool do_polynomial_division;
	/// Do not calculate the specified function. Default: NULL
	MathFunction *protected_function;
	/// Complex number form. Default: COMPLEX_NUMBER_FORM_RECTANGULAR
	ComplexNumberForm complex_number_form;
	/// Convert to local currency when optimal conversion is enabled
	bool local_currency_conversion;
	/// Mainly for internal use. Default: true
	bool transform_trigonometric_functions;
	/// Algorithm used for calculation of uncertainty propagation / intervals. This does not affect calculation of the high precision intervals produced by approximate functions or irrational numbers. Default: INTERVAL_CALCULATION_VARIANCE_FORMULA
	IntervalCalculation interval_calculation;

	EvaluationOptions();
};

static const EvaluationOptions default_evaluation_options;
static EvaluationOptions default_user_evaluation_options;

extern MathStructure m_undefined, m_empty_vector, m_empty_matrix, m_zero, m_one, m_minus_one, m_one_i;
extern Number nr_zero, nr_one, nr_two, nr_three, nr_minus_one, nr_one_i, nr_minus_i, nr_half, nr_minus_half, nr_plus_inf, nr_minus_inf;
extern EvaluationOptions no_evaluation;
extern ExpressionName empty_expression_name;

extern Calculator *calculator;

#define CALCULATOR	calculator

#define DEFAULT_PRECISION	8
#define PRECISION		(CALCULATOR ? CALCULATOR->getPrecision() : DEFAULT_PRECISION)

#define SIGN_DEGREE			"°"
#define SIGN_POWER_0			"⁰"
#define SIGN_POWER_1			"¹"
#define SIGN_POWER_2			"²"
#define SIGN_POWER_3			"³"
#define SIGN_POWER_4			"⁴"
#define SIGN_POWER_5			"⁵"
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
#define SIGN_MIDDLEDOT			"·"
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

#define THIN_SPACE " "
#define NNBSP " "
#define NBSP " "

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
//#define SNAN			"NAN"
#define UNDERSCORE		"_"

#define NOT_IN_NAMES 	RESERVED OPERATORS SPACES SEXADOT DOT VECTOR_WRAPS PARENTHESISS COMMAS

#endif
