/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MATH_STRUCTURE_H
#define MATH_STRUCTURE_H

#include <libqalculate/includes.h>
#include <libqalculate/Number.h>
#include <libqalculate/QalculateDateTime.h>
#include <sys/time.h>

class QalculateDate;

/** @file */

/// Types for MathStructure
typedef enum {
	STRUCT_MULTIPLICATION,
	STRUCT_INVERSE,
	STRUCT_DIVISION,
	STRUCT_ADDITION,
	STRUCT_NEGATE,
	STRUCT_POWER,
	STRUCT_NUMBER,
	STRUCT_UNIT,
	STRUCT_SYMBOLIC,
	STRUCT_FUNCTION,
	STRUCT_VARIABLE,
	STRUCT_VECTOR,
	STRUCT_BITWISE_AND,
	STRUCT_BITWISE_OR,
	STRUCT_BITWISE_XOR,
	STRUCT_BITWISE_NOT,
	STRUCT_LOGICAL_AND,
	STRUCT_LOGICAL_OR,
	STRUCT_LOGICAL_XOR,
	STRUCT_LOGICAL_NOT,
	STRUCT_COMPARISON,
	STRUCT_UNDEFINED,
	STRUCT_ABORTED,
	STRUCT_DATETIME
} StructureType;

enum {
	MULTIPLICATION_SIGN_NONE,
	MULTIPLICATION_SIGN_SPACE,
	MULTIPLICATION_SIGN_OPERATOR,
	MULTIPLICATION_SIGN_OPERATOR_SHORT
};

enum {
	TAG_TYPE_HTML,
	TAG_TYPE_TERMINAL
};

/// A structure representing a mathematical value/expression/result
/**
* A MathStructure can both be container representing an operation with an ordered list of children or simple value representing
* a number, , variable etc. The children of a container might be of any type, allowing a tree-like nested structure.
*
* These are the most common conatiner/operation types:
*	- \b Addition: contains two or more children, representing terms (x+y+...)
*	- \b Multiplication: contains two or more children, representing factors (x*y*...)
*	- \b Power: contains exactly two children, representing base and exponent (x^y)
*	- \b Function: contains a two or more children, representing arguments, and a reference to a MathFunction object ( f(x,y,...) )
*	- \b Comparison: an equality or inequality containing exactly two children, represening the expressions right and left of the sign, specified with a ComparisonType (x=y, x!=y, x&gt;y, ...)
*	- \b Vector: contains zero or more children, representing elements in a vector ( [x, y, z, ...] )
*
* Also available are containers representing logical and bitwise operations.
* Subtraction is represented by an addition structure with negated children and division by a multiplication structure with inverted children.
* Matrices is represented by a vector with vectors as children.
*
* For formatted structures, the following types is also available:
*	- \b Negation: contains exactly one child (-x)
*	- \b Invertion: contains exactly one child (1/x)
*	- \b Division: contains exactly two children representing numerator and denominator (x/y)
*
* The following value types are available:
*	- \b Number: has a Number object, representing a rational, floating point, complex or infinite numeric value
*	- \b Variable: has a reference to a Variable object, with a known or unknown value
*	- \b Symbolic: has an associated text string, with assumptions about the represented value controlled by the default assumptions
*	- \b Unit: has a reference to a Unit object, and might in a formatted structure also have a reference to a Prefix object
*	- \b Undefined: represents an undefined value
*
* To create a MathStructure, you can either create a simple structure using the constructors and then expanding it with structural operations,
* or use the parse or calculation functions of the global Calculator object to convert an expression string.
*
* The expression "(5x + 2) * 3" can be turned into a MathStructure either using
* \code
* MathStructure mstruct = CALCULATOR->parse("(5x + 2) * 3");
* \endcode
* or
* \code
* MathStructure mstruct(5);
* mstruct *= CALCULATOR->v_x;
* mstruct += 2;
* mstruct *= 3:
* \endcode
* The first variant is obviously simpler, but slower and allows less control.
*
* Then, to evaluate/calculate/simplify (whatever) a structure, eval() should normally be used. The EvaluationOptions passed to eval() allows much control over the process
* and the outcome.
* \code
* EvaluationOptions eo;
* mstruct.eval(eo);
* \endcode
*
* After that, to display the result, you should first format the structure using format() and then display it using print(), passing the PrintOptions to both.
* \code
* PrintOptions po;
* mstruct.format(po);
* std::cout << mstruct.print(po) << std::endl;
* \endcode
*
* Most low-level functions expect the structure to be unformatted och require that unformat() is called after an expression string has been parsed or format() has been called.
*
* To access a child structure either the [] operator or the safer getChild() can be used.
* Note however that the index passed to the operator start at zero and the index argument for getChild() starts at one.
* \code
* MathStructure mstruct(5);
* mstruct += 2;
* std::cout << mstruct.print() << std::endl; // output: "5 + 2"
* std::cout << mstruct.getChild(1)->print() << std::endl; // output: "5"
* std::cout << mstruct[1].print() << std::endl; // output: "2"
* \endcode
*
* MathStructure uses reference count for management of objects allocated with new.
* Call ref() when starting to use the object and unref() when done.
* Note that the reference count is initialized to 1 in the constructors, so ref() should not be called after the object creation.
* This system is used for all child objects, so the following is perfectly legal:
* \code
* MathStructure *mchild_p = mstruct->getChild(1);
* mchild_p->ref(); // mchild_p reference count = 2
* mstruct->unref(); //mstruct reference count = 0, mstruct deleted, mchild_p reference count = 1
* (...)
* mchild_p->unref(); // mchild_p reference count = 0, mchild_p deleted
* \endcode
*/

class MathStructure {

	protected:

		size_t i_ref;

		StructureType m_type;
		bool b_approx;
		int i_precision;

		std::vector<MathStructure*> v_subs;
		std::vector<size_t> v_order;
		std::string s_sym;
		Number o_number;
		Variable *o_variable;

		Unit *o_unit;
		Prefix *o_prefix;
		bool b_plural;

		MathFunction *o_function;
		MathStructure *function_value;
		QalculateDateTime *o_datetime;

		ComparisonType ct_comp;
		bool b_protected;

		bool b_parentheses;

		bool isolate_x_sub(const EvaluationOptions &eo, EvaluationOptions &eo2, const MathStructure &x_var, MathStructure *morig = NULL);
		bool isolate_x_sub(const EvaluationOptions &eo, EvaluationOptions &eo2, const MathStructure &x_var, MathStructure *morig, size_t depth);
		bool isolate_x(const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_var, bool check_result, size_t depth);

		void init();

		class MathStructure_p *priv;

	public:

		/** @name Constructors */
		//@{
		/** Create a new structure, initialized to zero. */
		MathStructure();
		/** Create a copy of a structure. Child structures are copied.
		*
		* @param o The structure to copy.
		*/
		MathStructure(const MathStructure &o);
		/** Create a new numeric structure (value=num/den*10^exp10). Equivalent to MathStructure(Number(num, den, exp10)).
		*
		* @param num The numerator of the numeric value.
		* @param den The denominator of the numeric value.
		* @param exp10 The base 10 exponent of the numeric value.
		*/
		MathStructure(int num, int den = 1, int exp10 = 0);
		MathStructure(long int num, long int den, long int exp10 = 0L);
		/** Create a new symbolic/text structure.
		*
		* @param sym Symbolic/text value.
		* @param force_symbol Do not check for undefined or date value.
		*/
		MathStructure(std::string sym, bool force_symbol = false);
		/** Create a new date and time structure.
		*
		* @param o_dt Date and time value.
		*/
		MathStructure(const QalculateDateTime &o_dt);
		/** Create a new numeric structure with floating point value. Uses Number::setFloat().
		*
		* @param float_value Numeric value.
		*/
		MathStructure(double float_value);
		/** Create a new vector.
		*
		* @param o The first element (copied) in the vector.
		* @param ... Elements (copied) in the vector. End with NULL.
		*/
		MathStructure(const MathStructure *o, ...);
		/** Create a new function structure.
		*
		* @param o Function value.
		* @param ... Arguments (copied) to the function. End with NULL.
		*/
		MathStructure(MathFunction *o, ...);
		/** Create a new unit structure.
		*
		* @param u The unit value.
		* @param p Prefix of the unit.
		*/
		MathStructure(Unit *u, Prefix *p = NULL);
		/** Create a new variable structure.
		*
		* @param o Variable value.
		*/
		MathStructure(Variable *o);
		/** Create a new numeric structure.
		*
		* @param o Numeric value.
		*/
		MathStructure(const Number &o);
		~MathStructure();
		//@}

		/** @name Functions/operators for setting type and content */
		//@{
		/** Set the structure to a copy of another structure. Child structures are copied.
		*
		* @param o The structure to copy.
		* @param merge_precision Preserve the current precision (unless the new value has a lower precision).
		*/
		void set(const MathStructure &o, bool merge_precision = false);
		/** Set the structure to a copy of another structure. Pointers to child structures are copied.
		*
		* @param o The structure to copy.
		* @param merge_precision Preserve the current precision (unless the new value has a lower precision).
		*/
		void set_nocopy(MathStructure &o, bool merge_precision = false);
		/** Set the structure to a number (num/den*10^exp10). Equivalent to set(Number(num, den, exp10), precerve_precision).
		*
		* @param num The numerator of the new numeric value.
		* @param den The denominator of the new numeric value.
		* @param exp10 The base 10 exponent of the new numeric value.
		* @param preserve_precision Preserve the current precision (unless the new value has a lower precision).
		*/
		void set(int num, int den = 1, int exp10 = 0, bool preserve_precision = false);
		void set(long int num, long int den, long int exp10 = 0L, bool preserve_precision = false);
		/** Set the structure to a symbolic/text value.
		*
		* @param sym The new symolic/text value.
		* @param preserve_precision Preserve the current precision.
		* @param force_symbol Do not check for undefined or date value.
		*/
		void set(std::string sym, bool preserve_precision = false, bool force_symbol = false);
		/** Set the structure to a date and time value.
		*
		* @param o_dt The new data and time value.
		* @param preserve_precision Preserve the current precision.
		*/
		void set(const QalculateDateTime &o_dt, bool preserve_precision = false);
		/** Set the structure to a number with a floating point value. Uses Number::setFloat().
		*
		* @param float_value The new numeric value.
		* @param preserve_precision Preserve the current precision (unless the new value has a lower precision).
		*/
		void set(double float_value, bool preserve_precision = false);
		/** Set the structure to a vector.
		*
		* @param o The first element (copied) in the new vector.
		* @param ... Elements (copied) in the new vector. End with NULL.
		*/
		void setVector(const MathStructure *o, ...);
		/** Set the structure to a mathematical function.
		*
		* @param o The new function value.
		* @param ... Arguments (copied) to the function. End with NULL.
		*/
		void set(MathFunction *o, ...);
		/** Set the structure to a unit.
		*
		* @param u The new unit value.
		* @param p Prefix of the unit.
		* @param preserve_precision Preserve the current precision (unless the new value has a lower precision).
		*/
		void set(Unit *u, Prefix *p = NULL, bool preserve_precision = false);
		/** Set the structure to a variable.
		*
		* @param o The new variable value.
		* @param preserve_precision Preserve the current precision.
		*/
		void set(Variable *o, bool preserve_precision = false);
		/** Set the structure to a number.
		*
		* @param o The new numeric value.
		* @param preserve_precision Preserve the current precision (unless the new value has a lower precision).
		*/
		void set(const Number &o, bool preserve_precision = false);
		/** Set the value of the structure to undefined.
		*
		* @param preserve_precision Preserve the current precision.
		*/
		void setUndefined(bool preserve_precision = false);
		/** Mark that calculation was aborted.
		*
		* @param preserve_precision Preserve the current precision.
		*/
		void setAborted(bool preserve_precision = false);
		/** Reset the value (to zero) and parameters of the structure.
		*
		* @param preserve_precision Preserve the current precision.
		*/
		void clear(bool preserve_precision = false);
		/** Set the structure to an empty vector.
		*
		* @param preserve_precision Preserve the current precision.
		*/
		void clearVector(bool preserve_precision = false);
		/** Set the structure to an empty matrix.
		*
		* @param preserve_precision Preserve the current precision.
		*/
		void clearMatrix(bool preserve_precision = false);

		/** Explicitely sets the type of the structure.
		* setType() is dangerous and might crash the program if used unwisely.
		*
		* @param mtype The new structure type
		*/
		void setType(StructureType mtype);

		void operator = (const MathStructure &o);
		void operator = (const Number &o);
		void operator = (int i);
		void operator = (Unit *u);
		void operator = (Variable *v);
		void operator = (std::string sym);
		//@}

		/** @name Functions to keep track of referrers */
		//@{
		void ref();
		void unref();
		size_t refcount() const;
		//@}

		/** @name Functions for numbers */
		//@{
		const Number &number() const;
		Number &number();
		void numberUpdated();
		//@}

		/** @name Functions for symbols */
		//@{
		const std::string &symbol() const;
		//@}

		/** @name Functions for date and time */
		//@{
		const QalculateDateTime *datetime() const;
		QalculateDateTime *datetime();
		//@}

		/** @name Functions for units */
		//@{
		Unit *unit() const;
		Unit *unit_exp_unit() const;
		Prefix *prefix() const;
		Prefix *unit_exp_prefix() const;
		void setPrefix(Prefix *p);
		bool isPlural() const;
		void setPlural(bool is_plural);
		void setUnit(Unit *u);
		//@}

		/** @name Functions for mathematical functions */
		//@{
		void setFunction(MathFunction *f);
		void setFunctionId(int id);
		MathFunction *function() const;
		const MathStructure *functionValue() const;
		//@}

		/** @name Functions for variables */
		//@{
		void setVariable(Variable *v);
		Variable *variable() const;
		//@}

		/** @name Functions for nested structures (power, muliplication, addition, vector, etc) */
		//@{
		/** Call this function when you have updated a child. Updates the precision.
		*
		* @param index Index (starting at 1) of the updated child.
		* @recursive If true, do the same for each child of the child.
		*/
		void childUpdated(size_t index, bool recursive = false);
		/** Call this function when you have updated children. Updates the precision.
		*
		* @recursive If true, do the same for each child of the children.
		*/
		void childrenUpdated(bool recursive = false);
		/** Returns a child. Does not check if a child exists at the index.
		*
		* @param index Index (starting at zero).
		*/
		MathStructure &operator [] (size_t index);
		/** Returns a child. Does not check if a child exists at the index.
		*
		* @param index Index (starting at zero).
		*/
		const MathStructure &operator [] (size_t index) const;
		MathStructure &last();
		const MathStructure last() const;
		void setToChild(size_t index, bool merge_precision = false, MathStructure *mparent = NULL, size_t index_this = 1);
		void swapChildren(size_t index1, size_t index2);
		void childToFront(size_t index);
		void addChild(const MathStructure &o);
		void addChild_nocopy(MathStructure *o);
		void delChild(size_t index, bool check_size = false);
		void insertChild(const MathStructure &o, size_t index);
		void insertChild_nocopy(MathStructure *o, size_t index);
		void setChild(const MathStructure &o, size_t index = 1, bool merge_precision = false);
		void setChild_nocopy(MathStructure *o, size_t index = 1, bool merge_precision = false);
		const MathStructure *getChild(size_t index) const;
		MathStructure *getChild(size_t index);
		size_t countChildren() const;
		size_t countTotalChildren(bool count_function_as_one = true) const;
		size_t size() const;
		//@}

		/** @name Functions for power */
		//@{
		const MathStructure *base() const;
		const MathStructure *exponent() const;
		MathStructure *base();
		MathStructure *exponent();
		//@}

		/** @name Functions for comparisons */
		//@{
		ComparisonType comparisonType() const;
		void setComparisonType(ComparisonType comparison_type);
		//@}

		/** @name Functions checking type and value */
		//@{
		StructureType type() const;

		bool isAddition() const;
		bool isMultiplication() const;
		bool isPower() const;
		bool isSymbolic() const;
		bool isDateTime() const;
		bool isAborted() const;
		bool isEmptySymbol() const;
		bool isVector() const;
		bool isMatrix() const;
		bool isFunction() const;
		bool isUnit() const;
		bool isUnit_exp() const;
		bool isUnknown() const;
		bool isUnknown_exp() const;
		bool isNumber_exp() const;
		bool isVariable() const;
		bool isComparison() const;
		bool isBitwiseAnd() const;
		bool isBitwiseOr() const;
		bool isBitwiseXor() const;
		bool isBitwiseNot() const;
		bool isLogicalAnd() const;
		bool isLogicalOr() const;
		bool isLogicalXor() const;
		bool isLogicalNot() const;
		bool isInverse() const;
		bool isDivision() const;
		bool isNegate() const;
		bool isInfinity() const;
		bool isUndefined() const;
		bool isInteger() const;
		bool isInfinite(bool ignore_imag = true) const;
		bool isNumber() const;
		bool isZero() const;
		bool isApproximatelyZero() const;
		bool isOne() const;
		bool isMinusOne() const;

		bool hasNegativeSign() const;

		bool representsBoolean() const;
		bool representsPositive(bool allow_units = false) const;
		bool representsNegative(bool allow_units = false) const;
		bool representsNonNegative(bool allow_units = false) const;
		bool representsNonPositive(bool allow_units = false) const;
		bool representsInteger(bool allow_units = false) const;
		bool representsNonInteger(bool allow_units = false) const;
		bool representsNumber(bool allow_units = false) const;
		bool representsRational(bool allow_units = false) const;
		bool representsFraction(bool allow_units = false) const;
		bool representsReal(bool allow_units = false) const;
		bool representsNonComplex(bool allow_units = false) const;
		bool representsComplex(bool allow_units = false) const;
		bool representsFinite(bool allow_units = false) const;
		bool representsNonZero(bool allow_units = false) const;
		bool representsZero(bool allow_units = false) const;
		bool representsApproximatelyZero(bool allow_units = false) const;
		bool representsEven(bool allow_units = false) const;
		bool representsOdd(bool allow_units = false) const;
		bool representsUndefined(bool include_children = false, bool include_infinite = false, bool be_strict = false) const;
		bool representsNonMatrix() const;
		bool representsScalar() const;
		//@}

		/** @name Functions for precision */
		//@{
		void setApproximate(bool is_approx = true, bool recursive = false);
		bool isApproximate() const;
		void setPrecision(int prec, bool recursive = false);
		int precision() const;
		void mergePrecision(const MathStructure &o);
		void mergePrecision(bool approx, int prec);

		//@}

		/** @name Operators for structural transformations and additions
		* These operators transforms or adds to the structure without doing any calculations
		*/
		//@{

		MathStructure operator - () const;
		MathStructure operator * (const MathStructure &o) const;
		MathStructure operator / (const MathStructure &o) const;
		MathStructure operator + (const MathStructure &o) const;
		MathStructure operator - (const MathStructure &o) const;
		MathStructure operator ^ (const MathStructure &o) const;
		MathStructure operator && (const MathStructure &o) const;
		MathStructure operator || (const MathStructure &o) const;
		MathStructure operator ! () const;

		void operator *= (const MathStructure &o);
		void operator /= (const MathStructure &o);
		void operator += (const MathStructure &o);
		void operator -= (const MathStructure &o);
		void operator ^= (const MathStructure &o);

		void operator *= (const Number &o);
		void operator /= (const Number &o);
		void operator += (const Number &o);
		void operator -= (const Number &o);
		void operator ^= (const Number &o);

		void operator *= (int i);
		void operator /= (int i);
		void operator += (int i);
		void operator -= (int i);
		void operator ^= (int i);

		void operator *= (Unit *u);
		void operator /= (Unit *u);
		void operator += (Unit *u);
		void operator -= (Unit *u);
		void operator ^= (Unit *u);

		void operator *= (Variable *v);
		void operator /= (Variable *v);
		void operator += (Variable *v);
		void operator -= (Variable *v);
		void operator ^= (Variable *v);

		void operator *= (std::string sym);
		void operator /= (std::string sym);
		void operator += (std::string sym);
		void operator -= (std::string sym);
		void operator ^= (std::string sym);

		//@}

		/** @name Functions for structural transformations and additions
		* These functions transforms or adds to the structure without doing any calculations
		*/
		//@{
		void add(const MathStructure &o, MathOperation op, bool append = false);
		void add(const MathStructure &o, bool append = false);
		void subtract(const MathStructure &o, bool append = false);
		void multiply(const MathStructure &o, bool append = false);
		void divide(const MathStructure &o, bool append = false);
		void raise(const MathStructure &o);
		void add(const Number &o, bool append = false);
		void subtract(const Number &o, bool append = false);
		void multiply(const Number &o, bool append = false);
		void divide(const Number &o, bool append = false);
		void raise(const Number &o);
		void add(int i, bool append = false);
		void subtract(int i, bool append = false);
		void multiply(int i, bool append = false);
		void divide(int i, bool append = false);
		void raise(int i);
		void add(Variable *v, bool append = false);
		void subtract(Variable *v, bool append = false);
		void multiply(Variable *v, bool append = false);
		void divide(Variable *v, bool append = false);
		void raise(Variable *v);
		void add(Unit *u, bool append = false);
		void subtract(Unit *u, bool append = false);
		void multiply(Unit *u, bool append = false);
		void divide(Unit *u, bool append = false);
		void raise(Unit *u);
		void add(std::string sym, bool append = false);
		void subtract(std::string sym, bool append = false);
		void multiply(std::string sym, bool append = false);
		void divide(std::string sym, bool append = false);
		void raise(std::string sym);
		void add_nocopy(MathStructure *o, MathOperation op, bool append = false);
		void add_nocopy(MathStructure *o, bool append = false);
		void subtract_nocopy(MathStructure *o, bool append = false);
		void multiply_nocopy(MathStructure *o, bool append = false);
		void divide_nocopy(MathStructure *o, bool append = false);
		void raise_nocopy(MathStructure *o);
		void inverse();
		void negate();
		void setLogicalNot();
		void setBitwiseNot();

		void transform(StructureType mtype, const MathStructure &o);
		void transform(StructureType mtype, const Number &o);
		void transform(StructureType mtype, int i);
		void transform(StructureType mtype, Unit *u);
		void transform(StructureType mtype, Variable *v);
		void transform(StructureType mtype, std::string sym);
		void transform_nocopy(StructureType mtype, MathStructure *o);
		void transform(StructureType mtype);
		void transform(MathFunction *o);
		void transformById(int id);
		void transform(ComparisonType ctype, const MathStructure &o);
		//@}

		/** @name Functions/operators for comparisons */
		//@{

		bool equals(const MathStructure &o, bool allow_interval = false, bool allow_infinity = false) const;
		bool equals(const Number &o, bool allow_interval = false, bool allow_infinity = false) const;
		bool equals(int i) const;
		bool equals(Unit *u) const;
		bool equals(Variable *v) const;
		bool equals(std::string sym) const;

		ComparisonResult compare(const MathStructure &o) const;
		ComparisonResult compareApproximately(const MathStructure &o, const EvaluationOptions &eo = default_evaluation_options) const;

		bool mergeInterval(const MathStructure &o, bool set_to_overlap = false);

		bool operator == (const MathStructure &o) const;
		bool operator == (const Number &o) const;
		bool operator == (int i) const;
		bool operator == (Unit *u) const;
		bool operator == (Variable *v) const;
		bool operator == (std::string sym) const;

		bool operator != (const MathStructure &o) const;

		//@}

		/** @name Functions for calculation/evaluation */
		//@{
		MathStructure &eval(const EvaluationOptions &eo = default_evaluation_options);
		bool calculateMergeIndex(size_t index, const EvaluationOptions &eo, const EvaluationOptions &feo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalOrLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalOrIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalXorLast(const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalAndLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalAndIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateLogicalNot(const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseNot(const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateInverse(const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateNegate(const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateRaiseExponent(const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateRaise(const MathStructure &mexp, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseOrLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseOrIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseXorLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseXorIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseAndLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseAndIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateBitwiseAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateMultiplyLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateMultiplyIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateMultiply(const MathStructure &mmul, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateDivide(const MathStructure &mdiv, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateAddLast(const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateAddIndex(size_t index, const EvaluationOptions &eo, bool check_size = true, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateAdd(const MathStructure &madd, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateSubtract(const MathStructure &msub, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1);
		bool calculateFunctions(const EvaluationOptions &eo, bool recursive = true, bool do_unformat = true);
		int merge_addition(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_multiplication(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false, bool do_append = true);
		int merge_power(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_logical_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_logical_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_logical_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_bitwise_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_bitwise_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		int merge_bitwise_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_this = 1, size_t index_that = 2, bool reversed = false);
		bool calculatesub(const EvaluationOptions &eo, const EvaluationOptions &feo, bool recursive = true, MathStructure *mparent = NULL, size_t index_this = 1);
		void evalSort(bool recursive = false, bool absolute = false);
		bool integerFactorize();
		//@}

		/** @name Functions for protection from changes when evaluating */
		//@{
		void setProtected(bool do_protect = true);
		bool isProtected() const;
		//@}

		/** @name Functions for format and display */
		//@{
		void sort(const PrintOptions &po = default_print_options, bool recursive = true);
		bool improve_division_multipliers(const PrintOptions &po = default_print_options, MathStructure *parent = NULL);
		void setPrefixes(const PrintOptions &po = default_print_options, MathStructure *parent = NULL, size_t pindex = 0);
		void prefixCurrencies(const PrintOptions &po = default_print_options);
		void format(const PrintOptions &po = default_print_options);
		void formatsub(const PrintOptions &po = default_print_options, MathStructure *parent = NULL, size_t pindex = 0, bool recursive = true, MathStructure *top_parent = NULL);
		void postFormatUnits(const PrintOptions &po = default_print_options, MathStructure *parent = NULL, size_t pindex = 0);
		bool factorizeUnits();
		void unformat(const EvaluationOptions &eo = default_evaluation_options);
		bool needsParenthesis(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool flat_division = true, bool flat_power = true) const;
		bool removeDefaultAngleUnit(const EvaluationOptions &eo = default_evaluation_options);

		int neededMultiplicationSign(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool par, bool par_prev, bool flat_division = true, bool flat_power = true) const;

		std::string print(const PrintOptions &po = default_print_options, const InternalPrintStruct &ips = top_ips) const;
		std::string print(const PrintOptions &po, bool format, int colorize = 0, int tagtype = TAG_TYPE_HTML, const InternalPrintStruct &ips = top_ips) const;
		//@}


		/** @name Functions for vectors */
		//@{

		MathStructure &flattenVector(MathStructure &mstruct) const;

		bool rankVector(bool ascending = true);
		bool sortVector(bool ascending = true);
		void flipVector();

		MathStructure &getRange(int start, int end, MathStructure &mstruct) const;

		void resizeVector(size_t i, const MathStructure &mfill);

		//@}


		/** @name Functions for matrices */
		//@{

		size_t rows() const;
		size_t columns() const;
		const MathStructure *getElement(size_t row, size_t column) const;
		MathStructure *getElement(size_t row, size_t column);
		MathStructure &getArea(size_t r1, size_t c1, size_t r2, size_t c2, MathStructure &mstruct) const;
		MathStructure &rowToVector(size_t r, MathStructure &mstruct) const;
		MathStructure &columnToVector(size_t c, MathStructure &mstruct) const;
		MathStructure &matrixToVector(MathStructure &mstruct) const;
		void setElement(const MathStructure &mstruct, size_t row, size_t column);
		void addRows(size_t r, const MathStructure &mfill);
		void addColumns(size_t c, const MathStructure &mfill);
		void addRow(const MathStructure &mfill);
		void addColumn(const MathStructure &mfill);
		void resizeMatrix(size_t r, size_t c, const MathStructure &mfill);
		bool matrixIsSquare() const;
		bool isNumericMatrix() const;
		int pivot(size_t ro, size_t co, bool symbolic = true);
		int gaussianElimination(const EvaluationOptions &eo = default_evaluation_options, bool det = false);
		MathStructure &determinant(MathStructure &mstruct, const EvaluationOptions &eo) const;
		MathStructure &permanent(MathStructure &mstruct, const EvaluationOptions &eo) const;
		void setToIdentityMatrix(size_t n);
		MathStructure &getIdentityMatrix(MathStructure &mstruct) const;
		bool invertMatrix(const EvaluationOptions &eo);
		bool adjointMatrix(const EvaluationOptions &eo);
		bool transposeMatrix();
		MathStructure &cofactor(size_t r, size_t c, MathStructure &mstruct, const EvaluationOptions &eo) const;
		//@}

		/** @name Functions for unit conversion */
		//@{
		int isUnitCompatible(const MathStructure &mstruct) const;
		bool syncUnits(bool sync_nonlinear_relations = false, bool *found_nonlinear_relations = NULL, bool calculate_new_functions = false, const EvaluationOptions &feo = default_evaluation_options);
		bool testDissolveCompositeUnit(Unit *u);
		bool testCompositeUnit(Unit *u);
		bool dissolveAllCompositeUnits();
		bool setPrefixForUnit(Unit *u, Prefix *new_prefix);
		bool convertToBaseUnits(bool convert_nonlinear_relations = false, bool *found_nonlinear_relations = NULL, bool calculate_new_functions = false, const EvaluationOptions &feo = default_evaluation_options, bool avoid_approximate_variables = false);
		bool convert(Unit *u, bool convert_nonlinear_relations = false, bool *found_nonlinear_relations = NULL, bool calculate_new_functions = false, const EvaluationOptions &feo = default_evaluation_options, Prefix *new_prefix = NULL);
		bool convert(const MathStructure unit_mstruct, bool convert_nonlinear_relations = false, bool *found_nonlinear_relations = NULL, bool calculate_new_functions = false, const EvaluationOptions &feo = default_evaluation_options);
		//@}

		/** @name Functions for recursive search and replace */
		//@{
		int contains(const MathStructure &mstruct, bool structural_only = true, bool check_variables = false, bool check_functions = false, bool loose_equals = false) const;
		size_t countOccurrences(const MathStructure &mstruct) const;
		size_t countOccurrences(const MathStructure &mstruct, bool check_variables) const;
		int containsRepresentativeOf(const MathStructure &mstruct, bool check_variables = false, bool check_functions = false) const;
		int containsType(StructureType mtype, bool structural_only = true, bool check_variables = false, bool check_functions = false) const;
		int containsRepresentativeOfType(StructureType mtype, bool check_variables = false, bool check_functions = false) const;
		int containsFunction(MathFunction *f, bool structural_only = true, bool check_variables = false, bool check_functions = false) const;
		int containsFunctionId(int id, bool structural_only = true, bool check_variables = false, bool check_functions = false) const;
		int containsInterval(bool structural_only = true, bool check_variables = false, bool check_functions = false, int ignore_high_precision_interval = 0, bool include_interval_function = false) const;
		int containsInfinity(bool structural_only = true, bool check_variables = false, bool check_functions = false) const;
		bool containsOpaqueContents() const;
		bool containsAdditionPower() const;
		bool containsUnknowns() const;
		bool containsDivision() const;
		size_t countFunctions(bool count_subfunctions = true) const;
		void findAllUnknowns(MathStructure &unknowns_vector);
		bool replace(const MathStructure &mfrom, const MathStructure &mto, bool once_only = false, bool exclude_function_arguments = false);
		bool replace(const MathStructure &mfrom, const MathStructure &mto, bool once_only, bool exclude_function_arguments, bool replace_in_variables);
		bool replace(Variable *v, const MathStructure &mto);
		bool calculateReplace(const MathStructure &mfrom, const MathStructure &mto, const EvaluationOptions &eo, bool exclude_function_arguments = false);
		bool replace(const MathStructure &mfrom1, const MathStructure &mto1, const MathStructure &mfrom2, const MathStructure &mto2);
		bool removeType(StructureType mtype);
		//@}

		/** @name Functions to generate vectors for plotting */
		//@{
		MathStructure generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector = NULL, const EvaluationOptions &eo = default_evaluation_options) const;
		MathStructure generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector = NULL, const EvaluationOptions &eo = default_evaluation_options) const;
		MathStructure generateVector(MathStructure x_mstruct, const MathStructure &x_vector, const EvaluationOptions &eo = default_evaluation_options) const;
		//@}

		/** @name Differentiation and integration */
		//@{
		bool differentiate(const MathStructure &x_var, const EvaluationOptions &eo);
		bool integrate(const MathStructure &lower_limit, const MathStructure &upper_limit, const MathStructure &x_var_pre, const EvaluationOptions &eo = default_evaluation_options, bool force_numerical = false, bool simplify_first = true);
		int integrate(const MathStructure &x_var, const EvaluationOptions &eo, bool simplify_first = true, int use_abs = 1, bool definite_integral = false, bool try_abs = true, int max_part_depth = 5, std::vector<MathStructure*> *parent_parts = NULL);
		//@}

		/** @name Functions for polynomials */
		//@{
		bool expand(const EvaluationOptions &eo = default_evaluation_options, bool unfactorize = true);
		bool simplify(const EvaluationOptions &eo = default_evaluation_options, bool unfactorize = true);
		bool factorize(const EvaluationOptions &eo = default_evaluation_options, bool unfactorize = true, int term_combination_levels = 0, int max_msecs = 1000, bool only_integers = true, int recursive = 1, struct timeval *endtime_p = NULL, const MathStructure &force_factorization = m_undefined, bool complete_square = false, bool only_sqrfree = false, int max_degree_factor = -1);
		bool expandPartialFractions(const EvaluationOptions &eo);
		bool structure(StructuringMode structuring, const EvaluationOptions &eo, bool restore_first = true);
		/** If the structure represents a rational polynomial.
		* This is true for
		*	- rational numbers;
		*	- functions, units, variables and symbols that do not represent a matrix or undefined;
		*	- a power with a positive integer exponent and any of the previous as base;
		*	- a multiplication with the previous as factors; or
		*	- an addition with the previous as terms.
		*
		* @returns true if structure represents a rational polynomial.
		*/
		bool isRationalPolynomial(bool allow_non_rational_coefficient = false, bool allow_interval_coefficient = false) const;
		const Number &overallCoefficient() const;
		const Number &degree(const MathStructure &xvar) const;
		const Number &ldegree(const MathStructure &xvar) const;
		void lcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const;
		void tcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const;
		void coefficient(const MathStructure &xvar, const Number &pownr, MathStructure &mcoeff) const;
		Number maxCoefficient();
		int polynomialUnit(const MathStructure &xvar) const;
		void polynomialContent(const MathStructure &xvar, MathStructure &mcontent, const EvaluationOptions &eo) const;
		void polynomialPrimpart(const MathStructure &xvar, MathStructure &mprim, const EvaluationOptions &eo) const;
		void polynomialPrimpart(const MathStructure &xvar, const MathStructure &c, MathStructure &mprim, const EvaluationOptions &eo) const;
		void polynomialUnitContentPrimpart(const MathStructure &xvar, int &munit, MathStructure &mcontent, MathStructure &mprim, const EvaluationOptions &eo) const;
		//@}

		/** @name Functions for conversion of complex numbers */
		//@{
		bool complexToExponentialForm(const EvaluationOptions &eo);
		bool complexToPolarForm(const EvaluationOptions &eo);
		bool complexToCisForm(const EvaluationOptions &eo);
		//@}

		bool calculateLimit(const MathStructure &x_var, const MathStructure &limit, const EvaluationOptions &eo_pre, int approach_direction = 0);

		bool decomposeFractions(const MathStructure &x_var, const EvaluationOptions &eo);

		static bool polynomialDivide(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args = true);
		static bool polynomialQuotient(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args = true);
		static bool lcm(const MathStructure &m1, const MathStructure &m2, MathStructure &mlcm, const EvaluationOptions &eo, bool check_args = true);
		static bool gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mresult, const EvaluationOptions &eo, MathStructure *ca = NULL, MathStructure *cb = NULL, bool check_args = true);

		/** @name Functions for equations */
		//@{
		const MathStructure &find_x_var() const;
		bool isolate_x(const EvaluationOptions &eo, const MathStructure &x_var = m_undefined, bool check_result = false);
		bool isolate_x(const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_var = m_undefined, bool check_result = false);
		//@}

		bool inParentheses() const;
		void setInParentheses(bool b = true);

};

std::ostream& operator << (std::ostream &os, const MathStructure&);

int has_information_unit(const MathStructure &m, bool top = true);
bool is_unit_multiexp(const MathStructure &mstruct);

#endif
