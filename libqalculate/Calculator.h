/*
    Qalculate

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <libqalculate/includes.h>
#include <libqalculate/util.h>
#include <sys/time.h>

/** @file */

/** \mainpage Index
*
* \section intro_sec Introduction
*
* libqalculate is a math library for expression evaluation with units, variables and functions support and CAS functionality.
*
* The main parts of the library is the almighty Calculator class, the MathStructure class for mathematical expressions and classes for objects in an expression,
* mostly of the class Numbers and sub classes of ExpressionItem.
*
* A simple application using libqalculate need only create a calculator object, perhaps load definitions (functions, variables, units, etc.), and calculate (and output) an expression as follows:
* \code
* new Calculator();
* CALCULATOR->loadExchangeRates();
* CALCULATOR->loadGlobalDefinitions();
* CALCULATOR->loadLocalDefinitions();
* cout << CALCULATOR->calculateAndPrint("1 + 1", 2000) << endl;\endcode
* In the above example, the calculation is terminated after two seconds (2000 ms), if it is not finished before then.
* Applications using localized numbers should first call Calculator::unlocalizeExpression() on the expression.
*
* A less simple application might calculate and output the expression separately.
* \code
* EvaluationOptions eo;
* MathStructure result;
* CALCULATOR->calculate(&mstruct, unlocalizeExpression("1 + 1"), 2000, eo);\endcode
*
* More complex usage mainly involves manipulating objects of the MathStructure class directly.
*
* To display the resulting expression use Calculator::print() as follows:
* \code
* PrintOptions po;
* string result_str = CALCULATOR->print(result, 2000, po);\endcode
* Alternatively MathStructure::format() followed by MathStructure::print() can be used, without the possibility to specify a time limit.
*
* Central to the flexibility of libqalculate is the many options passed to evaluating and display functions with EvaluationOptions and PrintOptions.
*
* \section usage_sec Using the library
*
* libqalculate uses pkg-config.
*
* For a simple program use pkg-config on the command line:
* \code c++ `pkg-config --cflags --libs libqalculate` hello.c -o hello \endcode
*
* If the program uses autoconf, put the following in configure.ac:
* \code PKG_CHECK_MODULES(QALCULATE, [
*	libqalculate >= 1.0.0
*	])
* AC_SUBST(QALCULATE_CFLAGS)
* AC_SUBST(QALCULATE_LIBS) \endcode
*/

typedef std::vector<Prefix*> p_type;

/// Parameters passed to plotting functions.
struct PlotParameters {
	/// Title label.
	std::string title;
	/// Y-axis label.
	std::string y_label;
	/// X-axis label.
	std::string x_label;
	/// Image to save plot to. If empty shows plot in a window on the screen.
	std::string filename;
	/// The image type to save as. Set to PLOT_FILETYPE_AUTO to guess from file extension.
	PlotFileType filetype;
	/// Font used for text
	std::string font;
	/// Set to true for colored plot, false for monochrome. Default: true
	bool color;
	/// If the minimum y-axis value shall be set automatically (otherwise uses y_min). Default: true
	bool auto_y_min;
	/// If the minimum x-axis value shall be set automatically (otherwise uses x_min). Default: true
	bool auto_x_min;
	/// If the maximum y-axis value shall be set automatically (otherwise uses y_max). Default: true
	bool auto_y_max;
	/// If the maximum x-axis value shall be set automatically (otherwise uses x_max). Default: true
	bool auto_x_max;
	/// Minimum y-axis value.
	float y_min;
	/// Minimum x-axis value.
	float x_min;
	/// Maximum y-axis value.
	float y_max;
	/// Maximum x-axis value.
	float x_max;
	/// If a logarithmic scale shall be used for the y-axis. Default: false
	bool y_log;
	/// If a logarithmic scale shall be used for the x-axis. Default: false
	bool x_log;
	/// Logarithmic base for the y-axis. Default: 10
	int y_log_base;
	/// Logarithmic base for the x-axis. Default: 10
	int x_log_base;
	/// If  a grid shall be shown in the plot. Default: false
	unsigned int grid;
	/// Width of lines. Default: 2
	int linewidth;
	/// If the plot shall be surrounded by borders on all sides (not just axis). Default: false
	bool show_all_borders;
	/// Where the plot legend shall be placed. Default: PLOT_LEGEND_TOP_RIGHT
	PlotLegendPlacement legend_placement;
	PlotParameters();
};

/// Parameters for plot data series.
struct PlotDataParameters {
	/// Title label.
	std::string title;
	/// Plot smoothing.
	PlotSmoothing smoothing;
	/// Plot style
	PlotStyle style;
	/// Use scale on second y-axis
	bool yaxis2;
	/// Use scale on second x-axis
	bool xaxis2;
	/// Check if data is continuous
	bool test_continuous;
	PlotDataParameters();
};

///Message types
typedef enum {
	MESSAGE_INFORMATION,
	MESSAGE_WARNING,
	MESSAGE_ERROR
} MessageType;

typedef enum {
	AUTOMATIC_FRACTION_OFF,
	AUTOMATIC_FRACTION_SINGLE,
	AUTOMATIC_FRACTION_AUTO,
	AUTOMATIC_FRACTION_DUAL
} AutomaticFractionFormat;

typedef enum {
	AUTOMATIC_APPROXIMATION_OFF,
	AUTOMATIC_APPROXIMATION_SINGLE,
	AUTOMATIC_APPROXIMATION_AUTO,
	AUTOMATIC_APPROXIMATION_DUAL
} AutomaticApproximation;

///Message stages
#define MESSAGE_STAGE_CONVERSION		-4
#define MESSAGE_STAGE_CONVERSION_PARSING	-3
#define MESSAGE_STAGE_CALCULATION		-2
#define MESSAGE_STAGE_PARSING			-1
#define MESSAGE_STAGE_UNSET			0

///Message categories
#define MESSAGE_CATEGORY_NONE				0
#define MESSAGE_CATEGORY_PARSING			1
#define MESSAGE_CATEGORY_WIDE_INTERVAL			10
#define MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION	11
#define MESSAGE_CATEGORY_NO_PROPER_INTERVAL_SUPPORT	12


/// A message with information to the user. Primarily used for errors and warnings.
class CalculatorMessage {
  protected:
	std::string smessage;
	MessageType mtype;
	int i_stage, i_cat;
  public:
	CalculatorMessage(std::string message_, MessageType type_ = MESSAGE_WARNING, int cat_ = MESSAGE_CATEGORY_NONE, int stage_ = MESSAGE_STAGE_UNSET);
	CalculatorMessage(const CalculatorMessage &e);
	std::string message() const;
	const char* c_message() const;
	MessageType type() const;
	int stage() const;
	int category() const;
};

#include <libqalculate/MathStructure.h>

enum {
	VARIABLE_ID_I = 200,
	VARIABLE_ID_PLUS_INFINITY = 201,
	VARIABLE_ID_MINUS_INFINITY = 202,
	VARIABLE_ID_UNDEFINED = 203,
	VARIABLE_ID_X = 300,
	VARIABLE_ID_Y = 301,
	VARIABLE_ID_Z = 302,
	VARIABLE_ID_N = 303,
	VARIABLE_ID_C = 304,
	VARIABLE_ID_PERCENT = 350,
	VARIABLE_ID_PERMILLE = 351,
	VARIABLE_ID_PERMYRIAD = 352
};

enum {
	UNIT_ID_EURO = 510,
	UNIT_ID_BYN = 515,
	UNIT_ID_BTC = 520,
	UNIT_ID_SECOND = 550,
	UNIT_ID_MINUTE = 551,
	UNIT_ID_HOUR = 552,
	UNIT_ID_DAY = 553,
	UNIT_ID_MONTH = 554,
	UNIT_ID_YEAR = 555,
	UNIT_ID_KELVIN = 560,
	UNIT_ID_RANKINE = 561,
	UNIT_ID_CELSIUS = 562,
	UNIT_ID_FAHRENHEIT = 563
};

enum {
	ELEMENT_CLASS_NOT_DEFINED,
	ALKALI_METALS,
	ALKALI_EARTH_METALS,
	LANTHANIDES,
	ACTINIDES,
	TRANSITION_METALS,
	METALS,
	METALLOIDS,
	NONMETALS,
	HALOGENS,
	NOBLE_GASES,
	TRANSACTINIDES
};

typedef enum {
	TEMPERATURE_CALCULATION_HYBRID,
	TEMPERATURE_CALCULATION_ABSOLUTE,
	TEMPERATURE_CALCULATION_RELATIVE
} TemperatureCalculationMode;

struct Element {
	std::string symbol, name;
	int number, group;
	std::string weight;
	int x_pos, y_pos;
};

#define UFV_LENGTHS	20

class Calculate_p;

/// The almighty calculator class.
/** The calculator class is responsible for loading functions, variables and units, and keeping track of them, as well as parsing expressions and much more. A calculator object must be created before any other Qalculate! class is used. There should never be more than one calculator object, accessed with CALCULATOR.
*
* A simple application using libqalculate need only create a calculator object, perhaps load definitions (functions, variables, units, etc.) and use the calculate function as follows:
* \code new Calculator();
* CALCULATOR->loadGlobalDefinitions();
* CALCULATOR->loadLocalDefinitions();
* MathStructure result = CALCULATOR->calculate("1 + 1");\endcode
*/
class Calculator {

  protected:

	std::vector<CalculatorMessage> messages;

	int ianglemode;
	int i_precision;
	bool b_interval;
	int i_stop_interval;
	int i_start_interval;
	char vbuffer[200];
	std::vector<void*> ufvl;
	std::vector<char> ufvl_t;
	std::vector<size_t> ufvl_i;
	std::vector<void*> ufv[4][UFV_LENGTHS];
	std::vector<size_t> ufv_i[4][UFV_LENGTHS];

	std::vector<DataSet*> data_sets;

	class Calculator_p *priv;

	std::vector<std::string> signs;
	std::vector<std::string> real_signs;
	std::vector<std::string> default_signs;
	std::vector<std::string> default_real_signs;
	bool b_ignore_locale;
	char *saved_locale;
	int disable_errors_ref;
	std::vector<int> stopped_errors_count;
	std::vector<int> stopped_warnings_count;
	std::vector<int> stopped_messages_count;
	std::vector<std::vector<CalculatorMessage> > stopped_messages;

	Thread *calculate_thread;

	std::string NAME_NUMBER_PRE_S, NAME_NUMBER_PRE_STR, DOT_STR, DOT_S, COMMA_S, COMMA_STR, ILLEGAL_IN_NAMES, ILLEGAL_IN_UNITNAMES, ILLEGAL_IN_NAMES_MINUS_SPACE_STR;

	bool b_argument_errors;
	int current_stage;

	time_t exchange_rates_time[3], exchange_rates_check_time[3];
	int b_exchange_rates_used;
	bool b_exchange_rates_warning_enabled;

	bool b_gnuplot_open;
	std::string gnuplot_cmdline;
	FILE *gnuplot_pipe;

	bool local_to;

	Assumptions *default_assumptions;

	std::vector<Variable*> deleted_variables;
	std::vector<MathFunction*> deleted_functions;
	std::vector<Unit*> deleted_units;

	bool b_var_units;

	bool b_save_called;

	int i_timeout;
	struct timeval t_end;
	int i_aborted;
	bool b_controlled;

	std::string per_str, times_str, plus_str, minus_str, and_str, AND_str, or_str, OR_str, XOR_str;
	size_t per_str_len, times_str_len, plus_str_len, minus_str_len, and_str_len, AND_str_len, or_str_len, OR_str_len, XOR_str_len;

	std::vector<MathStructure*> rpn_stack;

	bool calculateRPN(MathStructure *mstruct, int command, size_t index, int msecs, const EvaluationOptions &eo, int function_arguments = 0);
	bool calculateRPN(std::string str, int command, size_t index, int msecs, const EvaluationOptions &eo, MathStructure *parsed_struct, MathStructure *to_struct, bool make_to_division, int function_arguments = 0);

  public:

	KnownVariable *v_pi, *v_e, *v_euler, *v_catalan, *v_i, *v_pinf, *v_minf, *v_undef, *v_precision;
	KnownVariable *v_percent, *v_permille, *v_permyriad;
	KnownVariable *v_today, *v_yesterday, *v_tomorrow, *v_now;
	UnknownVariable *v_x, *v_y, *v_z;
	UnknownVariable *v_C, *v_n;
	MathFunction *f_vector, *f_sort, *f_rank, *f_limits, *f_component, *f_dimension, *f_merge_vectors;
	MathFunction *f_matrix, *f_matrix_to_vector, *f_area, *f_rows, *f_columns, *f_row, *f_column, *f_elements, *f_element, *f_transpose, *f_identity, *f_determinant, *f_permanent, *f_adjoint, *f_cofactor, *f_inverse, *f_magnitude, *f_hadamard, *f_entrywise;
	MathFunction *f_factorial, *f_factorial2, *f_multifactorial, *f_binomial;
	MathFunction *f_xor, *f_bitxor, *f_even, *f_odd, *f_shift, *f_bitcmp;
	MathFunction *f_abs, *f_gcd, *f_lcm, *f_signum, *f_heaviside, *f_dirac, *f_round, *f_floor, *f_ceil, *f_trunc, *f_int, *f_frac, *f_rem, *f_mod;
	MathFunction *f_polynomial_unit, *f_polynomial_primpart, *f_polynomial_content, *f_coeff, *f_lcoeff, *f_tcoeff, *f_degree, *f_ldegree;
	MathFunction *f_re, *f_im, *f_arg, *f_numerator, *f_denominator;
	MathFunction *f_interval, *f_uncertainty;
  	MathFunction *f_sqrt, *f_cbrt, *f_root, *f_sq;
	MathFunction *f_exp;
	MathFunction *f_ln, *f_logn;
	MathFunction *f_lambert_w;
	MathFunction *f_sin, *f_cos, *f_tan, *f_asin, *f_acos, *f_atan, *f_sinh, *f_cosh, *f_tanh, *f_asinh, *f_acosh, *f_atanh, *f_atan2, *f_sinc, *f_radians_to_default_angle_unit;
	MathFunction *f_zeta, *f_gamma, *f_digamma, *f_beta, *f_airy, *f_besselj, *f_bessely, *f_erf, *f_erfc;
	MathFunction *f_total, *f_percentile, *f_min, *f_max, *f_mode, *f_rand;
	MathFunction *f_date, *f_datetime, *f_timevalue, *f_timestamp, *f_stamptodate, *f_days, *f_yearfrac, *f_week, *f_weekday, *f_month, *f_day, *f_year, *f_yearday, *f_time, *f_add_days, *f_add_months, *f_add_years;
	MathFunction *f_lunarphase, *f_nextlunarphase;
	MathFunction *f_bin, *f_oct, *f_hex, *f_base, *f_roman;
	MathFunction *f_ascii, *f_char;
	MathFunction *f_length, *f_concatenate;
	MathFunction *f_replace, *f_stripunits;
	MathFunction *f_genvector, *f_for, *f_sum, *f_product, *f_process, *f_process_matrix, *f_csum, *f_if, *f_is_number, *f_is_real, *f_is_rational, *f_is_integer, *f_represents_number, *f_represents_real, *f_represents_rational, *f_represents_integer, *f_function, *f_select;
	MathFunction *f_diff, *f_integrate, *f_solve, *f_multisolve, *f_dsolve, *f_limit;
	MathFunction *f_li, *f_Li, *f_Ei, *f_Si, *f_Ci, *f_Shi, *f_Chi, *f_igamma;
	MathFunction *f_error, *f_warning, *f_message, *f_save, *f_load, *f_export, *f_title;
	MathFunction *f_register, *f_stack;
	MathFunction *f_plot;

	Unit *u_rad, *u_gra, *u_deg, *u_euro, *u_btc, *u_second, *u_minute, *u_hour, *u_year, *u_month, *u_day;
	DecimalPrefix *decimal_null_prefix;
	BinaryPrefix *binary_null_prefix;

	bool place_currency_code_before, place_currency_sign_before;
	bool place_currency_code_before_negative, place_currency_sign_before_negative;
	bool default_dot_as_separator;
	std::string local_digit_group_separator, local_digit_group_format;

	bool b_busy;
	std::string expression_to_calculate;
	EvaluationOptions tmp_evaluationoptions;
	MathStructure *tmp_parsedstruct;
	MathStructure *tmp_tostruct;
	MathStructure *tmp_rpn_mstruct;
	bool tmp_maketodivision;
	int tmp_proc_command;
	int tmp_proc_registers;
	size_t tmp_rpnindex;

	PrintOptions save_printoptions, message_printoptions;

	std::vector<Variable*> variables;
	std::vector<MathFunction*> functions;
	std::vector<Unit*> units;
	std::vector<Prefix*> prefixes;
	std::vector<DecimalPrefix*> decimal_prefixes;
	std::vector<BinaryPrefix*> binary_prefixes;

	/** @name Constructor */
	//@{
	Calculator();
	Calculator(bool ignore_locale);
	virtual ~Calculator();
	//@}

	/** @name Functions for calculating expressions. */
	//@{
	/** Calculates an expression. The expression should be unlocalized first with unlocalizeExpression().
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param[out] mstruct Math structure to fill with the result.
	* @param str Expression.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the result of the parsing of the expression.
	* @param[out] to_struct NULL or a math structure to fill with unit expression parsed after "to". If expression does not contain a "to" string, and to_struct is a unit or a symbol (a unit expression string), to_struct will be used instead.
	* @param make_to_division If true, the expression after "to" will be interpreted as a unit expression to convert the result to.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool calculate(MathStructure *mstruct, std::string str, int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL, MathStructure *to_struct = NULL, bool make_to_division = true);
	/** Calculates an expression. The expression should be unlocalized first with unlocalizeExpression().
	*
	* @param str Expression.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the result of the parsing of the expression.
	* @param[out] to_struct NULL or a math structure to fill with unit expression parsed after "to". If expression does not contain a "to" string, and to_struct is a unit or a symbol (a unit expression string), to_struct will be used instead.
	* @param make_to_division If true, the expression after "to" will be interpreted as a unit expression to convert the result to.
	* @returns The result of the calculation.
	*/
	MathStructure calculate(std::string str, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL, MathStructure *to_struct = NULL, bool make_to_division = true);
	/** Calculates a parsed value.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param[out] mstruct Parsed value to evaluate and fill with the result.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation of the expression.
	* @param to_str "to" expression for conversion.
	* @returns The result of the calculation.
	*/
	bool calculate(MathStructure *mstruct, int msecs, const EvaluationOptions &eo = default_user_evaluation_options, std::string to_str = "");
	/** Calculates a parsed value.
	*
	* @param mstruct Parsed value to evaluate.
	* @param eo Options for the evaluation of the expression.
	* @param to_str "to" expression for conversion.
	* @returns The result of the calculation.
	*/
	MathStructure calculate(const MathStructure &mstruct, const EvaluationOptions &eo = default_user_evaluation_options, std::string to_str = "");
	/** Calculates an expression.and outputs the result to a text string. The expression should be unlocalized first with unlocalizeExpression().
	*
	* Unlike other functions for expression evaluation this function handles ending "to"-commands, in addition to unit conversion, such "to hexadecimal" or to "fractions", similar to the qalc application.
	*
	*
	* @param str Expression.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param po Result formatting options.
	* @returns The result of the calculation.
	* \since 2.6.0
	*/
	std::string calculateAndPrint(std::string str, int msecs = 10000, const EvaluationOptions &eo = default_user_evaluation_options, const PrintOptions &po = default_print_options, std::string *parsed_expression = NULL);
	/** Calculates an expression.and outputs the result to a text string. The expression should be unlocalized first with unlocalizeExpression().
	*
	* Unlike other functions for expression evaluation this function handles ending "to"-commands, in addition to unit conversion, such "to hexadecimal" or to "fractions", similar to the qalc application.
	*
	*
	* @param str Expression.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param po Result formatting options.
	* @returns The result of the calculation.
	* \since 4.0.0
	*/
	std::string calculateAndPrint(std::string str, int msecs, const EvaluationOptions &eo, const PrintOptions &po, AutomaticFractionFormat auto_fraction, AutomaticApproximation auto_approx = AUTOMATIC_APPROXIMATION_OFF, std::string *parsed_expression = NULL, int max_length = -1, bool *result_is_comparison = NULL, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML);
	int testCondition(std::string expression);
	//@}

	/** @name Functions for printing expressions with the option to set maximum time or abort. */
	//@{
	/** Calls MathStructure::format(po) and MathStructure::print(po). The process is aborted after msecs milliseconds.
	*/
	std::string print(const MathStructure &mstruct, int milliseconds = 100000, const PrintOptions &po = default_print_options);
	/** @name Functions for printing expressions with the option to set maximum time or abort. */
	//@{
	/** Calls MathStructure::format(po) and MathStructure::print(po). The process is aborted after msecs milliseconds.
	 * \since 4.0.0
	*/
	std::string print(const MathStructure &mstruct, int milliseconds, const PrintOptions &po, bool format, int colorize = 0, int tagtype = TAG_TYPE_HTML);
	///Deprecated: use print() instead
	std::string printMathStructureTimeOut(const MathStructure &mstruct, int milliseconds = 100000, const PrintOptions &op = default_print_options);

	/** Called before calculation, formatting or printing of a MathStructure (Calculator::calculate(), without maximum time, MathStructure::eval(), MathStructure::format() and MathStructure::print(), etc.) or printing of a Number (using Number::print) to be able to abort the process. Always use Calculator::stopControl() after finishing.
	*
	* @param milliseconds The maximum time for the process in milliseconds. If msecs <= 0 the time will be unlimited (stop with abort()).
	*/
	void startControl(int milliseconds = 0);
	/** Always call this function after Calculator::startControl() after formatting, printing or calculation has finished.
	*/
	void stopControl(void);
	/** Abort formatting, printing or evaluation (after startControl() has been called).
	* This function will normally be called from a thread that checks for user input or other conditions.
	*
	* @returns false if the calculation thread was forcibly stopped
	*/
	bool abort();
	bool aborted(void);
	bool isControlled(void) const;
	std::string abortedMessage(void) const;
	std::string timedOutString(void) const;

	std::string logicalORString(void) const;
	std::string logicalANDString(void) const;

	///Deprecated: use startControl()
	void startPrintControl(int milliseconds = 0);
	///Deprecated: use abort()
	void abortPrint(void);
	///Deprecated: use stopControl()
	void stopPrintControl(void);
	/// Deprecated: use aborted()
	bool printingAborted(void);
	/// Deprecated: use isControlled()
	bool printingControlled(void) const;
	/// Deprecated: use abortedMessage()
	std::string printingAbortedMessage(void) const;
	//@}

	/** @name Functions for handling of threaded calculations */
	//@{
	/** Returns true if the calculate or print thread is busy. */
	bool busy();
	/// Deprecated: does nothing.
	void saveState();
	/// Deprecated: does nothing.
	void restoreState();
	/** Clears all stored values. Used internally after aborted calculation. */
	void clearBuffers();
	/** Terminate calculation and print threads if started. Do not use to terminate calculation. */
	void terminateThreads();
	//@}

	/** @name Functions for manipulation of the RPN stack. */
	//@{
	/** Evaluates a value on the RPN stack.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param index Index, starting at 1, on the RPN stack.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool calculateRPNRegister(size_t index, int msecs, const EvaluationOptions &eo = default_user_evaluation_options);
	/** Applies a mathematical operation to the first and second value on the RPN stack. The the second value is changed with input from the first value.
	* For example, with OPERATION_SUBTRACT the first value is subtracted from the second. The first value on the stack is removed.
	* If not enough registers is available, then zeros are added.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param op Operation.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool calculateRPN(MathOperation op, int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies a mathematical operation to the first value on the RPN stack. The value is set as the first argument of the function.
	* If no register is available, then zero is added.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param f Mathematical function.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool calculateRPN(MathFunction *f, int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies bitwise not to the first value on the RPN stack.
	* If no register is available, then zero is added.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool calculateRPNBitwiseNot(int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies logical not to the first value on the RPN stack.
	* If no register is available, then zero is added.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool calculateRPNLogicalNot(int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies a mathematical operation to the first and second value on the RPN stack. The the second value is changed with input from the first value.
	* For example, with OPERATION_SUBTRACT the first value is subtracted from the second. The first value on the stack is removed.
	* If not enough registers is available, then zeros are added.
	*
	* @param op Operation.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns The first value on the stack.
	*/
	MathStructure *calculateRPN(MathOperation op, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies a mathematical operation to the first value on the RPN stack. The value is set as the first argument of the function.
	* If no register is available, then zero is added.
	*
	* @param f Mathematical function.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns The first value on the stack.
	*/
	MathStructure *calculateRPN(MathFunction *f, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies bitwise not to the first value on the RPN stack.
	* If no register is available, then zero is added.
	*
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns The first value on the stack.
	*/
	MathStructure *calculateRPNBitwiseNot(const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Applies logical not to the first value on the RPN stack.
	* If no register is available, then zero is added.
	*
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the unevaluated result.
	* @returns The first value on the stack.
	*/
	MathStructure *calculateRPNLogicalNot(const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL);
	/** Evaluates a value and adds the result first on the RPN stack.
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param mstruct Value.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation of the expression.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool RPNStackEnter(MathStructure *mstruct, int msecs, const EvaluationOptions &eo = default_user_evaluation_options);
	/** Calculates an expression and adds the result first on the RPN stack. The expression should be unlocalized first with unlocalizeExpression().
	* This function starts the calculation in a separate thread and will return when the calculation has started unless a maximum time has been specified.
	* The calculation can then be stopped with abort().
	*
	* @param str Expression.
	* @param msecs The maximum time for the calculation in milliseconds. If msecs <= 0 the time will be unlimited.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the result of the parsing of the expression.
	* @param[out] to_struct NULL or a math structure to fill with unit expression parsed after "to".
	* @param make_to_division If true, the expression after "to" will be interpreted as a unit expression to convert the result to.
	* @returns true if the calculation was successfully started (and finished if msecs > 0).
	*/
	bool RPNStackEnter(std::string str, int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL, MathStructure *to_struct = NULL, bool make_to_division = true);
	/** Adds a value first on the RPN stack.
	*
	* @param mstruct Value.
	* @param eval If true, the the mathematical structure will be evaluated first.
	*/
	void RPNStackEnter(MathStructure *mstruct, bool eval = false, const EvaluationOptions &eo = default_user_evaluation_options);
	/** Calculates an expression adds the result first on the RPN stack. The expression should be unlocalized first with unlocalizeExpression().
	*
	* @param str Expression.
	* @param eo Options for the evaluation and parsing of the expression.
	* @param[out] parsed_struct NULL or a math structure to fill with the result of the parsing of the expression.
	* @param[out] to_struct NULL or a math structure to fill with unit expression parsed after "to".
	* @param make_to_division If true, the expression after "to" will be interpreted as a unit expression to convert the result to.
	*/
	void RPNStackEnter(std::string str, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL, MathStructure *to_struct = NULL, bool make_to_division = true);
	bool setRPNRegister(size_t index, MathStructure *mstruct, int msecs, const EvaluationOptions &eo = default_user_evaluation_options);
	bool setRPNRegister(size_t index, std::string str, int msecs, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL, MathStructure *to_struct = NULL, bool make_to_division = true);
	void setRPNRegister(size_t index, MathStructure *mstruct, bool eval = false, const EvaluationOptions &eo = default_user_evaluation_options);
	void setRPNRegister(size_t index, std::string str, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *parsed_struct = NULL, MathStructure *to_struct = NULL, bool make_to_division = true);
	void deleteRPNRegister(size_t index);
	MathStructure *getRPNRegister(size_t index = 1) const;
	size_t RPNStackSize() const;
	void clearRPNStack();
	void moveRPNRegister(size_t old_index, size_t new_index);
	void moveRPNRegisterUp(size_t index);
	void moveRPNRegisterDown(size_t index);
	//@}

	/** @name Functions for expression parsing. */
	//@{
	/** Returns a localized expressions. Affects decimal signs and argument separators.
	*
	* @param str The expression to localize.
	* @returns A localized expression.
	*/
	std::string localizeExpression(std::string str, const ParseOptions &po = default_parse_options) const;
	/** Returns an unlocalized expressions. Affects decimal signs and argument separators.
	*
	* @param str The expression to unlocalize.
	* @returns An unlocalized expression.
	*/
	std::string unlocalizeExpression(std::string str, const ParseOptions &po = default_parse_options) const;
	/** Split an expression string after and before " to ".
	*
	* @param[out] str The expression. Will be set to the string before " to ".
	* @param[out] to_str Will be set to the string after " to ".
	* @param eo Options for the evaluation and parsing of the expression (nothing will be done if units are not enabled).
	* @returns true if " to " was found and the expression split.
	*/
	bool separateToExpression(std::string &str, std::string &to_str, const EvaluationOptions &eo, bool keep_modifiers = false, bool allow_empty_from = false) const;
	bool hasToExpression(const std::string &str, bool allow_empty_from = false) const;
	bool hasToExpression(const std::string &str, bool allow_empty_from, const EvaluationOptions &eo) const;

	/// Split an expression string after and before " where ".
	bool separateWhereExpression(std::string &str, std::string &where_str, const EvaluationOptions &eo) const;
	bool hasWhereExpression(const std::string &str, const EvaluationOptions &eo) const;

	std::string parseComments(std::string &str, const ParseOptions &po = default_parse_options, bool *double_tag = NULL);
	void parseSigns(std::string &str, bool convert_to_internal_representation = false) const;
	/** Parse an expression and place in a MathStructure object.
	*
	* @param str Expression
	* @param po Parse options.
	* @returns MathStructure with result of parse.
	*/
	MathStructure parse(std::string str, const ParseOptions &po = default_parse_options);
	void parse(MathStructure *mstruct, std::string str, const ParseOptions &po = default_parse_options);
	bool parseNumber(MathStructure *mstruct, std::string str, const ParseOptions &po = default_parse_options);
	bool parseOperators(MathStructure *mstruct, std::string str, const ParseOptions &po = default_parse_options);
	bool parseAdd(std::string &str, MathStructure *mstruct, const ParseOptions &po, MathOperation s, bool append = true);
	bool parseAdd(std::string &str, MathStructure *mstruct, const ParseOptions &po);
	void parseExpressionAndWhere(MathStructure *mstruct, MathStructure *mwhere, std::string str, std::string str_where, const ParseOptions &po);
	//@}

	/** @name Functions converting expressions between units. */
	//@{
	/** Converts to a unit expression.
	* The converted value is evaluated.
	*
	* @param mstruct The value to convert.
	* @param composite_ Unit expression.
	* @param eo Evaluation options.
	* @param[out] units NULL or a math structure to fill with the parsed unit expression (or set to undefined if no units were found).
	* @returns Converted value.
	*/
	MathStructure convert(const MathStructure &mstruct, std::string composite_, const EvaluationOptions &eo, MathStructure *units, bool transform_orig, MathStructure *parsed_struct = NULL);
	MathStructure convert(const MathStructure &mstruct, std::string composite_, const EvaluationOptions &eo = default_user_evaluation_options, MathStructure *units = NULL);
	/** Converts to a unit.
	* The converted value is evaluated.
	*
	* @param mstruct The value to convert.
	* @param to_unit Unit to convert to.
	* @param eo Evaluation options.
	* @param always_convert ...
	* @returns Converted value.
	*/
	MathStructure convert(const MathStructure &mstruct, Unit *to_unit, const EvaluationOptions &eo = default_user_evaluation_options, bool always_convert = true, bool convert_to_mixed_units = true, bool transform_orig = false, MathStructure *parsed_struct = NULL);
	MathStructure convert(const MathStructure &mstruct, KnownVariable *to_var, const EvaluationOptions &eo = default_user_evaluation_options);
	MathStructure convert(double value, Unit *from_unit, Unit *to_unit, const EvaluationOptions &eo = default_user_evaluation_options);
	MathStructure convert(std::string str, Unit *from_unit, Unit *to_unit, int milliseconds, const EvaluationOptions &eo = default_user_evaluation_options);
	/** Deprecated: use convert() */
	MathStructure convertTimeOut(std::string str, Unit *from_unit, Unit *to_unit, int milliseconds, const EvaluationOptions &eo = default_user_evaluation_options);
	MathStructure convert(std::string str, Unit *from_unit, Unit *to_unit, const EvaluationOptions &eo = default_user_evaluation_options);
	MathStructure convertToBaseUnits(const MathStructure &mstruct, const EvaluationOptions &eo = default_user_evaluation_options);
	/** Deprecated: use getOptimalUnit() */
	Unit *getBestUnit(Unit *u, bool allow_only_div = false, bool convert_to_local_currency = true);
	Unit *getOptimalUnit(Unit *u, bool allow_only_div = false, bool convert_to_local_currency = true);
	/** Deprecated: use convertToOptimalUnit() */
	MathStructure convertToBestUnit(const MathStructure &mstruct, const EvaluationOptions &eo = default_user_evaluation_options, bool convert_to_si_units = true);
	MathStructure convertToOptimalUnit(const MathStructure &mstruct, const EvaluationOptions &eo = default_user_evaluation_options, bool convert_to_si_units = true);
	MathStructure convertToCompositeUnit(const MathStructure &mstruct, CompositeUnit *cu, const EvaluationOptions &eo = default_user_evaluation_options, bool always_convert = true);
	MathStructure convertToMixedUnits(const MathStructure &mstruct, const EvaluationOptions &eo = default_user_evaluation_options);
	//@}

	void setTemperatureCalculationMode(TemperatureCalculationMode temperature_calculation_mode);
	TemperatureCalculationMode getTemperatureCalculationMode() const;

	/** Used by the UI to find unit category for a mathematical expression.*/
	Unit *findMatchingUnit(const MathStructure &mstruct);

	/** @name Functions for default assumptions for unknown variables and symbols */
	//@{
	/** Set assumptions for objects without own assumptions (unknown variables and symbols).
	*/
	void setDefaultAssumptions(Assumptions *ass);
	/** Returns the default assumptions for objects without own assumptions (unknown variables and symbols).
	*/
	Assumptions *defaultAssumptions();
	//@}

	/** @name Functions for retrieval of angle units */
	//@{
	/** Returns the gradians unit.
	*/
	Unit *getGraUnit();
	/** Returns the radians unit.
	*/
	Unit *getRadUnit();
	/** Returns the degrees unit.
	*/
	Unit *getDegUnit();
	//@}

	/** @name Functions for finding a suitable prefix. */
	//@{
		/** Returns a decimal prefix with exactly the provided value, that fulfils the condition prefix->exponent(exp) == exp10.
	*
	* @param exp10 Base-10 exponent of the requested prefix.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if not found.
	*/
	DecimalPrefix *getExactDecimalPrefix(int exp10, int exp = 1) const;
	/** Returns a binary prefix with exactly the provided value, that fulfils the condition prefix->exponent(exp) == exp2.
	*
	* @param exp2 Base-2 exponent of the requested prefix.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if not found.
	*/
	BinaryPrefix *getExactBinaryPrefix(int exp2, int exp = 1) const;
	/** Returns a prefix with exactly the provided value, that fulfils the condition prefix->value(exp) == o.
	*
	* @param o Value of the requested prefix.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if not found.
	*/
	Prefix *getExactPrefix(const Number &o, int exp = 1) const;
	/** Returns the nearest decimal prefix for a value.
	*
	* @param exp10 Base-10 exponent of the value.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if no decimal prefix is available.
	*/
	DecimalPrefix *getNearestDecimalPrefix(int exp10, int exp = 1) const;
	/** Returns the best suited decimal prefix for a value.
	*
	* @param exp10 Base-10 exponent of the value.
	* @param exp The exponent of the unit.
	* @param all_prefixes If false, prefixes which is not a multiple of thousand (centi, deci, deca, hecto) will be skipped.
	* @returns A prefix or NULL if the unit should be left without prefix.
	*/
	DecimalPrefix *getOptimalDecimalPrefix(int exp10, int exp = 1, bool all_prefixes = true) const;
	/** Returns the best suited decimal prefix for a value.
	*
	* @param exp10 Base-10 exponent of the value.
	* @param exp The exponent of the unit.
	* @param all_prefixes If false, prefixes which is not a multiple of thousand (centi, deci, deca, hecto) will be skipped.
	* @returns A prefix or NULL if the unit should be left without prefix.
	*/
	DecimalPrefix *getOptimalDecimalPrefix(const Number &exp10, const Number &exp, bool all_prefixes = true) const;
	/** Returns the nearest binary prefix for a value.
	*
	* @param exp2 Base-2 exponent of the value.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if no binary prefix is available.
	*/
	BinaryPrefix *getNearestBinaryPrefix(int exp2, int exp = 1) const;
	/** Returns the best suited binary prefix for a value.
	*
	* @param exp2 Base-2 exponent of the value.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if the unit should be left without prefix.
	*/
	BinaryPrefix *getOptimalBinaryPrefix(int exp2, int exp = 1) const;
	/** Returns the best suited binary prefix for a value.
	*
	* @param exp2 Base-2 exponent of the value.
	* @param exp The exponent of the unit.
	* @returns A prefix or NULL if the unit should be left without prefix.
	*/
	BinaryPrefix *getOptimalBinaryPrefix(const Number &exp2, const Number &exp) const;
	/** Controls if binary, instead of decimal, prefixes will be used by default.
	* 1 = use binary prefixes for information units, 2 = use binary prefixes for all units.
	*/
	int usesBinaryPrefixes() const;
	void useBinaryPrefixes(int use_binary_prefixes);
	/** Add a new prefix to the calculator. */
	Prefix *addPrefix(Prefix *p);
	/** Used internally. */
	void prefixNameChanged(Prefix *p, bool new_item = false);
	//@}

	/** @name Functions for managing functions, variables, units, prefixes and data sets. */
	//@{
	void expressionItemActivated(ExpressionItem *item);
	void expressionItemDeactivated(ExpressionItem *item);
	void expressionItemDeleted(ExpressionItem *item);
	void nameChanged(ExpressionItem *item, bool new_item = false);
	void deleteName(std::string name_, ExpressionItem *object = NULL);
	void deleteUnitName(std::string name_, Unit *object = NULL);
	Unit* addUnit(Unit *u, bool force = true, bool check_names = true);
	void delPrefixUFV(Prefix *object);
	void delUFV(ExpressionItem *object);
	/** Checks if a variable exists/is registered in the calculator. */
	bool hasVariable(Variable *v);
	/** Checks if a unit exists/is registered in the calculator. */
	bool hasUnit(Unit *u);
	/** Checks if a function exists/is registered in the calculator. */
	bool hasFunction(MathFunction *f);
	/** Checks if a pointer points to a variable that still exists in the calculator.
	* As opposed to hasFunction(), this function only checks if the mathematical function has been deleted.
	*/
	bool stillHasVariable(Variable *v);
	/** Checks if a pointer points to a unit that still exists in the calculator.
	* As opposed to hasUnit(), this function only checks if the unit has been deleted.
	*/
	bool stillHasUnit(Unit *u);
	/** Checks if a pointer points to a mathematical function that still exists in the calculator.
	* As opposed to hasFunction(), this function only checks if the mathematical function has been deleted.
	*/
	bool stillHasFunction(MathFunction *f);
	void saveFunctionCalled();
	bool checkSaveFunctionCalled();
	ExpressionItem *getActiveExpressionItem(std::string name, ExpressionItem *item = NULL);
	ExpressionItem *getActiveExpressionItem(std::string name, ExpressionItem *item, bool ignore_us);
	ExpressionItem *getInactiveExpressionItem(std::string name, ExpressionItem *item = NULL);
	ExpressionItem *getActiveExpressionItem(ExpressionItem *item);
	ExpressionItem *getExpressionItem(std::string name, ExpressionItem *item = NULL);
	Unit* getUnit(std::string name_);
	Unit* getUnitById(int id) const;
	Unit* getActiveUnit(std::string name_);
	Unit* getActiveUnit(std::string name_, bool ignore_us);
	Unit* getCompositeUnit(std::string internal_name_);
	Unit* getLocalCurrency();
	void setLocalCurrency(Unit *u);
	/** Returns prefix for an index (starting at zero). All prefixes can be traversed by starting at index zero and increasing the index until NULL is returned.
	*
	* @param index Index of prefix.
	* @returns Prefix for index or NULL if not found.
	*/
	Prefix *getPrefix(size_t index) const;
	/** Returns prefix with provided name.
	*
	* @param name_ Name of prefix to retrieve.
	* @returns Prefix with provided name or NULL if not found.
	*/
	Prefix *getPrefix(std::string name_) const;
	Prefix *getDecimalNullPrefix() const;
	Prefix *getBinaryNullPrefix() const;
	Variable* addVariable(Variable *v, bool force = true, bool check_names = true);
	void variableNameChanged(Variable *v, bool new_item = false);
	void functionNameChanged(MathFunction *f, bool new_item = false);
	void unitNameChanged(Unit *u, bool new_item = false);
	Variable* getVariable(std::string name_);
	Variable* getVariableById(int id) const;
	Variable* getActiveVariable(std::string name_);
	Variable* getActiveVariable(std::string name_, bool ignore_us);
	ExpressionItem *addExpressionItem(ExpressionItem *item, bool force = true);
	MathFunction* addFunction(MathFunction *f, bool force = true, bool check_names = true);
	DataSet* addDataSet(DataSet *dc, bool force = true, bool check_names = true);
	DataSet* getDataSet(size_t index);
	DataSet* getDataSet(std::string name);
	MathFunction* getFunction(std::string name_);
	MathFunction* getFunctionById(int id) const;
	MathFunction* getActiveFunction(std::string name_);
	MathFunction* getActiveFunction(std::string name_, bool ignore_us);
	/** Returns variable for an index (starting at zero). All variables can be traversed by starting at index zero and increasing the index until NULL is returned.
	*
	* @param index Index of variable.
	* @returns Variable for index or NULL if not found.
	*/
	Variable *getVariable(size_t index) const;
	/** Returns unit for an index (starting at zero). All units can be traversed by starting at index zero and increasing the index until NULL is returned.
	*
	* @param index Index of unit.
	* @returns Unit for index or NULL if not found.
	*/
	Unit *getUnit(size_t index) const;
	/** Returns function for an index (starting at zero). All functions can be traversed by starting at index zero and increasing the index until NULL is returned.
	*
	* @param index Index of function.
	* @returns Function for index or NULL if not found.
	*/
	MathFunction *getFunction(size_t index) const;
	bool unitIsUsedByOtherUnits(const Unit *u) const;
	//@}

	/** @name Functions for handling of builtin expression items */
	//@{
	/** Unloads all non-builtin variables. */
	void resetVariables();
	/** Unloads all non-builtin functions. */
	void resetFunctions();
	/** Unloads all non-builtin units. */
	void resetUnits();
	/** Unloads all non-builtin variables, functions and units. */
	void reset();
	/** Adds builtin variables. Called automatically when the calculator is created. */
	void addBuiltinVariables();
	/** Adds builtin functions. Called automatically when the calculator is created. */
	void addBuiltinFunctions();
	/** Adds builtin units. Called automatically when the calculator is created. */
	void addBuiltinUnits();
	//@}

	void setVariableUnitsEnabled(bool enable_variable_units = true);
	bool variableUnitsEnabled() const;

	/** @name Functions for testing validity of functions, variable and unit names. */
	//@{
	/** Tests if a name is valid for a variable.
	*
	* @param name_ Variable name.
	* @returns true if the name is valid for a variable.
	*/
	bool variableNameIsValid(const std::string &name_) const;
	/** Tests if a name is valid for a variable.
	*
	* @param name_ Variable name.
	* @returns true if the name is valid for a variable.
	*/
	bool variableNameIsValid(const char *name_) const;
	bool variableNameIsValid(const char *name_, int version_numbers[3], bool is_user_defs);
	bool variableNameIsValid(const std::string &name_, int version_numbers[3], bool is_user_defs);
	std::string convertToValidVariableName(std::string name_) const;
	bool functionNameIsValid(const std::string &name_) const;
	bool functionNameIsValid(const char *name_) const;
	bool functionNameIsValid(const char *name_, int version_numbers[3], bool is_user_defs);
	bool functionNameIsValid(const std::string &name_, int version_numbers[3], bool is_user_defs);
	std::string convertToValidFunctionName(std::string name_) const;
	bool unitNameIsValid(const std::string &name_) const;
	bool unitNameIsValid(const char *name_) const;
	bool unitNameIsValid(const char *name_, int version_numbers[3], bool is_user_defs);
	bool unitNameIsValid(const std::string &name_, int version_numbers[3], bool is_user_defs);
	bool utf8_pos_is_valid_in_name(char *pos) const;
	std::string convertToValidUnitName(std::string name_) const;
	/** Checks if a name is used by another object which is not allowed to have the same name.
	*
	* @param name Name.
	* @param object Object to exclude from check.
	* @returns true if the name is used.
	*/
	bool nameTaken(std::string name, ExpressionItem *object = NULL);
	bool variableNameTaken(std::string name, Variable *object = NULL);
	bool unitNameTaken(std::string name, Unit *object = NULL);
	bool functionNameTaken(std::string name, MathFunction *object = NULL);
	std::string getName(std::string name = "", ExpressionItem *object = NULL, bool force = false, bool always_append = false);
	//@}

	/** @name Functions for message handling. */
	//@{
	void error(bool critical, int message_category, const char *TEMPLATE,...);
	void error(bool critical, const char *TEMPLATE,...);
	/** Put a message in the message queue.
	*/
	void message(MessageType mtype, int message_category, const char *TEMPLATE,...);
	void message(MessageType mtype, const char *TEMPLATE,...);
	void message(MessageType mtype, int message_category, const char *TEMPLATE, va_list ap);
	/** Returns the first message in queue.
	*/
	CalculatorMessage *message();
	/** Removes the first message in queue and returns the next.
	*/
	CalculatorMessage *nextMessage();
	/** Clear the message queue.
	*/
	void clearMessages();
	bool showArgumentErrors() const;
	void beginTemporaryStopMessages();
	int endTemporaryStopMessages(int *message_count = NULL, int *warning_count = NULL, int release_messages_if_no_equal_or_greater_than_message_type = -1);
	void endTemporaryStopMessages(bool release_messages, std::vector<CalculatorMessage> *blocked_messages = NULL);
	void addMessages(std::vector<CalculatorMessage> *message_vector);
	const PrintOptions &messagePrintOptions() const;
	void setMessagePrintOptions(const PrintOptions &po);
	void cleanMessages(const MathStructure &mstruct, size_t first_message = 1);
	//@}

	/** @name Functions for loading and saving definitions (variables, functions, units, etc.). */
	//@{
	/** Load all standard global (system wide) definitions from the global data directory ($PREFIX/share/qalculate).
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalDefinitions();
	/** Load global (system wide) definitions from a file in the global data directory ($PREFIX/share/qalculate).
	*
	* @param filename Name of the file in the global data directory.
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalDefinitions(std::string filename);
	/** Load prefixes.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalPrefixes();
	/** Load currencies.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalCurrencies();
	/** Load units.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalUnits();
	/** Load variables.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalVariables();
	/** Load functions.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalFunctions();
	/** Load data sets.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadGlobalDataSets();
	/** Load local, user specific, definitions from the local definitions directory (~/.qalculate/definitions).
	* All files in the directory and in the datasets subdirectory are loaded.
	*
	* @returns true if the definitions were successfully loaded.
	*/
	bool loadLocalDefinitions();
	/** Load definitions from a file.
	*
	* @param file_name The path to the file to load.
	* @param is_user_defs true if the definitions are local, false if they are global.
	* @returns true if the definitions were successfully loaded.
	*/
	int loadDefinitions(const char *file_name, bool is_user_defs = true, bool check_duplicates_of_global = false);
	/** Save local definitions to ~/.qalculate/definitions/
	*
	* @returns true if definitions was successfully saved.
	*/
	bool saveDefinitions();
	int saveDataObjects();
	int savePrefixes(const char *file_name, bool save_global = false);
	std::string temporaryCategory(void) const;
	int saveVariables(const char *file_name, bool save_global = false);
	int saveUnits(const char *file_name, bool save_global = false);
	int saveFunctions(const char *file_name, bool save_global = false);
	int saveDataSets(const char *file_name, bool save_global = false);
	std::string saveTemporaryDefinitions();
	void saveVariables(void *xmldoc, bool save_global = false, bool save_only_temp = false);
	void saveUnits(void *xmldoc, bool save_global = false, bool save_only_temp = false);
	void saveFunctions(void *xmldoc, bool save_global = false, bool save_only_temp = false);
	//@}

	/** @name Functions for CSV file import/export. */
	//@{
	bool importCSV(MathStructure &mstruct, const char *file_name, int first_row = 1, std::string delimiter = ",", std::vector<std::string> *headers = NULL);
	bool importCSV(const char *file_name, int first_row = 1, bool headers = true, std::string delimiter = ",", bool to_matrix = false, std::string name = "", std::string title = "", std::string category = "");
	bool exportCSV(const MathStructure &mstruct, const char *file_name, std::string delimiter = ",");
	//@}

	/** @name Functions for exchange rates. */
	/** Checks if able to downloading exchange rates from the Internet (using libcurl).
	*
	* @returns true if exchange rates can downloaded (if libcurl is available).
	*/
	bool canFetch();
	///Deprecated: gvfs is not needed anymore.
	bool hasGVFS();
	///Deprecated: gvfs is not needed anymore.
	bool hasGnomeVFS();
	/** Load exchange rates. Use before loadGlobalCurrencies() or loadGlobalDefinitions().
	*
	* @returns true if operation successful.
	*/
	bool loadExchangeRates();
	/** Name of the exchange rates file on local disc.
	* Multiple exchange rates sources might be used. Iterate over these, using the index parameter, until an empty string is returned.
	*
	* @param index The index (starting at one) of the exchange rate source
	* @returns name of local exchange rates file.
	*/
	std::string getExchangeRatesFileName(int index = 1);
	/** Url of the exchange rates file on the Internet.
	* Multiple exchange rates sources might be used. Iterate over these, using the index parameter, until an empty string is returned.
	*
	* @param index The index (starting at one) of the exchange rate source
	* @returns Url of exchange rates file.
	*/
	std::string getExchangeRatesUrl(int index = 1);
	/** Modification time of the exchange rates file.
	*
	* @returns Returns exchange rates modification time.
	*/
	time_t getExchangeRatesTime(int index = -1);
	///Deprecated: wget arguments are not used
	bool fetchExchangeRates(int seconds, std::string wget_args);
	/** Download current exchange rates from the Internet to local disc with default wget arguments.
	*
	* @param seconds Maximum time for download try
	* @returns true if operation was successful.
	*/
	bool fetchExchangeRates(int seconds = 15, int n = -1);
	/** Check age of exchange rates on local disc.
	*
	* @param n_days How old in days exchange rates may be before exchange rates need updating
	* @param force_check If exchange rates date should be checked again even if found outdated within n_days before
	* @param send_warning If the standard exchange rates warning should be sent.
	* @returns false if exchange.rates need updating
	*/
	bool checkExchangeRatesDate(unsigned int n_days = 7, bool force_check = false, bool send_warning = false, int n = -1);
	/// Enable or disable old exchange rates warning (initial state is true).
	void setExchangeRatesWarningEnabled(bool enable);
	bool exchangeRatesWarningEnabled() const;
	/// Check if exchange rates has been used since resetExchangeRatesUsed() was last called
	int exchangeRatesUsed() const;
	void resetExchangeRatesUsed();
	/// For internal use, called by currency units
	void setExchangeRatesUsed(int index);
	//@}

	/** @name Functions for plotting */
	//@{
	/** Checks if gnuplot is available.
	*
	* @returns true if gnuplot was found.
	*/
	bool canPlot();
	MathStructure expressionToPlotVector(std::string expression, const MathStructure &min, const MathStructure &max, int steps, bool separate_complex_part, MathStructure *x_vector = NULL, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	MathStructure expressionToPlotVector(std::string expression, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector = NULL, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	MathStructure expressionToPlotVector(std::string expression, float min, float max, int steps, MathStructure *x_vector = NULL, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	MathStructure expressionToPlotVector(std::string expression, const MathStructure &min, const MathStructure &max, const MathStructure &step, bool separate_complex_part, MathStructure *x_vector = NULL, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	MathStructure expressionToPlotVector(std::string expression, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector = NULL, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	MathStructure expressionToPlotVector(std::string expression, float min, float max, float step, MathStructure *x_vector = NULL, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	MathStructure expressionToPlotVector(std::string expression, const MathStructure &x_vector, std::string x_var = "\\x", const ParseOptions &po = default_parse_options, int msecs = 5000);
	bool plotVectors(PlotParameters *param, const std::vector<MathStructure> &y_vectors, const std::vector<MathStructure> &x_vectors, std::vector<PlotDataParameters*> &pdps, bool persistent = false, int msecs = 5000);
	void forcePersistentPlot(bool persistent = true);
	bool invokeGnuplot(std::string commands, std::string commandline_extra = "", bool persistent = false);
	bool closeGnuplot();
	bool gnuplotOpen();
	//@}

	/** @name Functions for global precision */
	//@{
	/** Set default precision for approximate calculations.
	*
	* @param precision Precision.
	*/
	void setPrecision(int precision = DEFAULT_PRECISION);
	/** Returns default precision for approximate calculations.
	*/
	int getPrecision() const;
	/** Set if interval should be produced for approximate functions and irrational numbers.
	* This does not affect calculation of lower precision explicit intervals (uncertainty propagation).
	*
	* @param use_interval_arithmetic Set true to activate, or false to deactivate, interval arithmetic.
	*/
	void useIntervalArithmetic(bool use_interval_arithmetic = true);
	/** Returns true if interval arithmetic are activated.
	*/
	bool usesIntervalArithmetic() const;
	void beginTemporaryStopIntervalArithmetic();
	void endTemporaryStopIntervalArithmetic();
	void beginTemporaryEnableIntervalArithmetic();
	void endTemporaryEnableIntervalArithmetic();
	//@}

	bool usesMatlabStyleMatrices() const;
	void useMatlabStyleMatrices(bool use_matlab_style_matrices);

	bool conciseUncertaintyInputEnabled() const;
	void setConciseUncertaintyInputEnabled(bool enable_concise_uncertainty_input);

	long int fixedDenominator() const;
	void setFixedDenominator(long int fixed_denominator);

	void setCustomInputBase(Number nr);
	void setCustomOutputBase(Number nr);
	const Number &customInputBase() const;
	const Number &customOutputBase() const;

	Unit *customAngleUnit();
	void setCustomAngleUnit(Unit *u);

	bool simplifiedPercentageUsed() const;
	void setSimplifiedPercentageUsed(bool percentage_used = true);

	/** @name Functions for localization */
	//@{
	/** Returns the preferred decimal point character.
	*/
	const std::string &getDecimalPoint() const;
	/** Returns the preferred comma character for separating arguments.*/
	const std::string &getComma() const;
	/** Sets argument separator and decimal sign from the current locale. Mainly for internal use. */
	void setLocale();
	///Deprecated: use pass true to constructor instead
	void setIgnoreLocale();
	bool getIgnoreLocale();
	void useDecimalComma();
	/** Use point as decimal separator.
	* To use comma as an ignored separator in numbers, must be invoked with comma_as_separator = true, to change the default function argument separator to semicolon, in addition to using ParseOptions::comma_as_separator.
	*/
	void useDecimalPoint(bool comma_as_separator = false);
	/** Resets argument separator and decimal sign. Mainly for internal use. */
	void unsetLocale();
	/** Returns the translated text string used in expressions for converting to a specific unit expression (ex "5 meters to feet.*/
	std::string localToString(bool include_spaces = true) const;
	std::string localWhereString() const;
	//@}

	/** @name Functions adding alternative symbols for operators and such */
	//@{
	void addStringAlternative(std::string replacement, std::string standard);
	bool delStringAlternative(std::string replacement, std::string standard);
	void addDefaultStringAlternative(std::string replacement, std::string standard);
	bool delDefaultStringAlternative(std::string replacement, std::string standard);
	//@}

	/** @name Functions for storing values with associated identifiers */
	//@{
	/** Stores a value with an associated id. Mainly for internal use.
	*
	* @param mstruct The value to store.
	* @param persistent If false the values will be removed from storage when retrieved with getId().
	* @returns Storage id.
	*/
	size_t addId(MathStructure *mstruct, bool persistent = false);
	/** Stores a function value with arguments parsed from a text string using Function::parse(), with an associated id. Mainly for internal use.
	*
	* @param f Mathematical function.
	* @param str Arguments.
	* @param po Parse options.
	* @param persistent If false the values will be removed from storage when retrieved with getId().
	* @returns Storage id.
	*/
	size_t parseAddId(MathFunction *f, const std::string &str, const ParseOptions &po, bool persistent = false);
	size_t parseAddIdAppend(MathFunction *f, const MathStructure &append_mstruct, const std::string &str, const ParseOptions &po, bool persistent = false);
	size_t parseAddVectorId(const std::string &str, const ParseOptions &po, bool persistent = false);
	/** Returns a stored value. Mainly for internal use.
	*
	* @param id Storage id.
	* @returns A stored value.
	*/
	MathStructure *getId(size_t id);
	/** Removes and unreferences (value->unref() will be called) a value from storage. Mainly for internal use.
	*
	* @param id Storage id.
	*/
	void delId(size_t id);
	//@}

};

void print_dual(const MathStructure &mresult, const std::string &original_expression, const MathStructure &mparse, MathStructure &mexact, std::string &result_str, std::vector<std::string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle = false, bool *exact_cmp = NULL, bool b_parsed = true, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1, bool converted = false);
void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const std::string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs = 0, int max_size = 10);
bool transform_expression_for_equals_save(std::string&, const ParseOptions&);
MathStructure get_units_for_parsed_expression(const MathStructure *parsed_struct, Unit *to_unit, const EvaluationOptions &eo, const MathStructure *mstruct = NULL);
bool expression_contains_save_function(const std::string&, const ParseOptions&, bool = false);

#endif
