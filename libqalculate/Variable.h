/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef VARIABLE_H
#define VARIABLE_H

#include <libqalculate/ExpressionItem.h>
#include <libqalculate/includes.h>

/** @file */

#define DECLARE_BUILTIN_VARIABLE(x, i)	class x : public DynamicVariable { \
					  private: \
						void calculate(MathStructure &m) const;	\
 					  public: \
						x(); \
						x(const x *variable) {set(variable);} \
						ExpressionItem *copy() const {return new x(this);} \
						int id() const {return i;} \
					};

/// Type assumption.
/**
* Each type is a subset of the type above.
*/
typedef enum {
	/// Multiplication is NOT commutative; do not use
	ASSUMPTION_TYPE_NONE = 0,
	ASSUMPTION_TYPE_NONMATRIX = 1,
	ASSUMPTION_TYPE_NUMBER = 2,
	/// im(x) != 0
	ASSUMPTION_TYPE_COMPLEX = 3,
	ASSUMPTION_TYPE_REAL = 4,
	ASSUMPTION_TYPE_RATIONAL = 5,
	ASSUMPTION_TYPE_INTEGER = 6,
	ASSUMPTION_TYPE_BOOLEAN = 7
} AssumptionType;

/// Signedness assumption.
typedef enum {
	/// x = ?
	ASSUMPTION_SIGN_UNKNOWN,
	/// x > 0
	ASSUMPTION_SIGN_POSITIVE,
	/// x >= 0
	ASSUMPTION_SIGN_NONNEGATIVE,
	/// x < 0
	ASSUMPTION_SIGN_NEGATIVE,
	/// x <= 0
	ASSUMPTION_SIGN_NONPOSITIVE,
	/// x != 0
	ASSUMPTION_SIGN_NONZERO
} AssumptionSign;

/// Type of variable
typedef enum {
	/// class Variable
	SUBTYPE_VARIABLE,
	/// class UnknownVariable
	SUBTYPE_UNKNOWN_VARIABLE,
	/// class KnownVariable
	SUBTYPE_KNOWN_VARIABLE
} VariableSubtype;

/// An assumption about an unknown mathematical value.
/** Assumptions have a type and a sign. The type describes the type of the value -- if it represents a number or something else, and what type of number is represented.
* The sign restricts the signedness of a number. The sign generally only applies the assumptions representing a number.
* The assumption class also includes max and min values, which however are not used anywhere yet.
*/
class Assumptions {

  protected:

	AssumptionType i_type;
	AssumptionSign i_sign;
	Number *fmin, *fmax;
	bool b_incl_min, b_incl_max;

  public:

	Assumptions();
	~Assumptions();

	bool isPositive();
	bool isNegative();
	bool isNonNegative();
	bool isNonPositive();
	bool isInteger();
	bool isBoolean();
	bool isNumber();
	bool isRational();
	bool isReal();
	bool isComplex();
	bool isNonZero();
	bool isNonMatrix();
	bool isScalar();

	AssumptionType type();
	AssumptionSign sign();
	void setType(AssumptionType ant);
	void setSign(AssumptionSign as);

	void setMin(const Number *nmin);
	void setIncludeEqualsMin(bool include_equals);
	bool includeEqualsMin() const;
	const Number *min() const;
	void setMax(const Number *nmax);
	void setIncludeEqualsMax(bool include_equals);
	bool includeEqualsMax() const;
	const Number *max() const;

};

/// Abstract base class for variables.
/** A variable is an alpha-numerical representation of a known or unknown value.
*/
class Variable : public ExpressionItem {

  public:

	Variable(std::string cat_, std::string name_, std::string title_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	Variable();
	Variable(const Variable *variable);
	virtual ~Variable();
	virtual ExpressionItem *copy() const = 0;
	virtual void set(const ExpressionItem *item);
	virtual int type() const {return TYPE_VARIABLE;}
	/** Returns the subtype of the variable, corresponding to which subsubclass the object belongs to.
	*
	* @returns ::VariableSubtype.
	*/
	virtual int subtype() const {return SUBTYPE_VARIABLE;}
	/** Returns if the variable has a known value (as opposed to assumptions).
	*
	* @returns true if the variable is of class KnownVariable, false if UnknownVariable.
	*/
	virtual bool isKnown() const = 0;

	/** Returns if the variable represents a positive value.
	*/
	virtual bool representsPositive(bool = false) {return false;}
	virtual bool representsNegative(bool = false) {return false;}
	virtual bool representsNonNegative(bool = false) {return false;}
	virtual bool representsNonPositive(bool = false) {return false;}
	virtual bool representsInteger(bool = false) {return false;}
	virtual bool representsNonInteger(bool = false) {return false;}
	virtual bool representsFraction(bool = false) {return false;}
	virtual bool representsNumber(bool = false) {return false;}
	virtual bool representsRational(bool = false) {return false;}
	virtual bool representsReal(bool = false) {return false;}
	virtual bool representsNonComplex(bool b = false) {return representsReal(b);}
	virtual bool representsComplex(bool = false) {return false;}
	virtual bool representsNonZero(bool = false) {return false;}
	virtual bool representsEven(bool = false) {return false;}
	virtual bool representsOdd(bool = false) {return false;}
	virtual bool representsUndefined(bool = false, bool = false, bool = false) {return false;}
	virtual bool representsBoolean() {return false;}
	virtual bool representsNonMatrix() {return false;}
	virtual bool representsScalar() {return false;}

	virtual int id() const {return 0;}

};

/// A variable with unknown value.
/** Unknown variables have an associated assumption object.
*/
class UnknownVariable : public Variable {

  protected:

  	Assumptions *o_assumption;
  	MathStructure *mstruct;

  public:

	/** Create an unknown.
	*
	* @param cat_ Category that the variable belongs to.
	* @param name_ Initial name of the variable.
	* @param title_ Descriptive name.
	* @param is_local If the variable is local/user-defined or global.
	* @param is_builtin If the variable is builtin and not modifiable.
	* @param is_active If the variable is active and can be used in expressions.
	*/
	UnknownVariable(std::string cat_, std::string name_, std::string title_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	/** Create an empty unknown variable.
	*/
	UnknownVariable();
	/** Create a copy of an unknown variable.
	*
	* @param variable Unknown variable to copy.
	*/
	UnknownVariable(const UnknownVariable *variable);
	virtual ~UnknownVariable();
	virtual ExpressionItem *copy() const;
	virtual void set(const ExpressionItem *item);
	bool isKnown() const {return false;}

	/** Sets the assumptions of the unknown variable.
	*
	* @param ass Assumptions.
	*/
	void setAssumptions(Assumptions *ass);
	void setAssumptions(const MathStructure &mvar);
	/** Returns the assumptions of the unknown variable.
	*
	* @returns Assumptions of the unknown variable.
	*/
	Assumptions *assumptions();

	const MathStructure &interval() const;
	void setInterval(const MathStructure &o);

	virtual int subtype() const {return SUBTYPE_UNKNOWN_VARIABLE;}

	virtual bool representsPositive(bool = false);
	virtual bool representsNegative(bool = false);
	virtual bool representsNonNegative(bool = false);
	virtual bool representsNonPositive(bool = false);
	virtual bool representsInteger(bool = false);
	virtual bool representsNumber(bool = false);
	virtual bool representsRational(bool = false);
	virtual bool representsReal(bool = false);
	virtual bool representsNonComplex(bool = false);
	virtual bool representsComplex(bool = false);
	virtual bool representsNonZero(bool = false);
	virtual bool representsBoolean();
	virtual bool representsNonMatrix();
	virtual bool representsScalar();

};

/// A variable with a known value.
/** Known variables have an associated value. The value can be a simple number or a full mathematical expression. The known variable class is used both for variable values and constants.
*
* The value can be provided as an expression in the form of a text string or as a mathematical value in the form of an object of the MathStructure class.
* The text string is parsed when needed, which saves time when loading many variable definitions which might not be used, at least not immediately.
*/
class KnownVariable : public Variable {

  protected:

	MathStructure *mstruct, *mstruct_alt;
	bool b_expression;
 	int calculated_precision;
	std::string sexpression, suncertainty, sunit;
	bool b_relative_uncertainty;

  public:

	/** Create a known variable with a value.
	*
	* @param cat_ Category that the variable belongs to.
	* @param name_ Initial name of the variable.
	* @param o Value.
	* @param title_ Descriptive name.
	* @param is_local If the variable is local/user-defined or global.
	* @param is_builtin If the variable is builtin and not modifiable.
	* @param is_active If the variable is active and can be used in expressions.
	*/
	KnownVariable(std::string cat_, std::string name_, const MathStructure &o, std::string title_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	/** Create a known variable with an text string expression.
	*
	* @param cat_ Category that the variable belongs to.
	* @param name_ Initial name of the variable.
	* @param expression_ Expression.
	* @param title_ Descriptive name.
	* @param is_local If the variable is local/user-defined or global.
	* @param is_builtin If the variable is builtin and not modifiable.
	* @param is_active If the variable is active and can be used in expressions.
	*/
	KnownVariable(std::string cat_, std::string name_, std::string expression_, std::string title_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	/** Create an empty known variable. Primarily for internal use.
	*/
	KnownVariable();
	/** Create a copy of a known variable.
	*
	* @param variable Known variable to copy.
	*/
	KnownVariable(const KnownVariable *variable);
	virtual ~KnownVariable();

	virtual ExpressionItem *copy() const;
	virtual void set(const ExpressionItem *item);
	bool isKnown() const {return true;}
	/** Returns if the variable has an text string expression instead of a value.
	*
	* @returns True if the variable has an expression instead of a value.
	*/
	virtual bool isExpression() const;
	/** Returns the variable's string expression or an empty string if it has not got an expression.
	*
	* @returns The variable's expression.
	*/
	virtual std::string expression() const;
	virtual std::string uncertainty(bool *is_relative = NULL) const;
	virtual std::string unit() const;

	int subtype() const {return SUBTYPE_KNOWN_VARIABLE;}

	/** Sets the value of the variable. If expression is set, it is cleared.
	*
	* @param o Value.
	*/
	virtual void set(const MathStructure &o);
	/** Sets the text string expression of the variable. The value is cleared.
	*
	* @param expression_ Expression.
	*/
	virtual void set(std::string expression_);
	virtual void setUncertainty(std::string standard_uncertainty, bool is_relative = false);
	virtual void setUnit(std::string unit_expression);

	/** Returns the value of the variable. If no value is set or parsed and an expression is set, the expression is parsed and resulting value returned.
	*
	* @returns The value of the variable..
	*/
	virtual const MathStructure &get();

	virtual bool representsPositive(bool = false);
	virtual bool representsNegative(bool = false);
	virtual bool representsNonNegative(bool = false);
	virtual bool representsNonPositive(bool = false);
	virtual bool representsInteger(bool = false);
	virtual bool representsNonInteger(bool = false);
	virtual bool representsFraction(bool = false);
	virtual bool representsNumber(bool = false);
	virtual bool representsRational(bool = false);
	virtual bool representsReal(bool = false);
	virtual bool representsNonComplex(bool = false);
	virtual bool representsComplex(bool = false);
	virtual bool representsNonZero(bool = false);
	virtual bool representsEven(bool = false);
	virtual bool representsOdd(bool = false);
	virtual bool representsUndefined(bool = false, bool = false, bool = false);
	virtual bool representsBoolean();
	virtual bool representsNonMatrix();
	virtual bool representsScalar();

	virtual int id() const {return 0;}

};

/// Abstract base class for variables with a value which is recalculated when the precision has changed.
/**
*/
class DynamicVariable : public KnownVariable {

  protected:

	virtual void calculate(MathStructure &m) const = 0;
	bool always_recalculate;

  public:

	DynamicVariable(std::string cat_, std::string name_, std::string title_ = "", bool is_local = false, bool is_builtin = true, bool is_active = true);
	DynamicVariable(const DynamicVariable *variable);
	DynamicVariable();
	virtual ~DynamicVariable();

	ExpressionItem *copy() const = 0;
	void set(const ExpressionItem *item);

	const MathStructure &get();

	void set(const MathStructure &o);
	void set(std::string expression_);

	/** Returns the precision of the calculated value.
	*
	* @returns Precision of the calculated value or zero if the value has not yet been calculated.
	*/
	int calculatedPrecision() const;

	virtual bool representsPositive(bool = false) {return true;}
	virtual bool representsNegative(bool = false) {return false;}
	virtual bool representsNonNegative(bool = false) {return true;}
	virtual bool representsNonPositive(bool = false) {return false;}
	virtual bool representsInteger(bool = false) {return false;}
	virtual bool representsNonInteger(bool = false) {return true;}
	virtual bool representsNumber(bool = false) {return true;}
	virtual bool representsRational(bool = false) {return false;}
	virtual bool representsReal(bool = false) {return true;}
	virtual bool representsComplex(bool = false) {return false;}
	virtual bool representsNonZero(bool = false) {return true;}
	virtual bool representsEven(bool = false) {return false;}
	virtual bool representsOdd(bool = false) {return false;}
	virtual bool representsUndefined(bool = false, bool = false, bool = false) {return false;}
	virtual bool representsBoolean() {return false;}
	virtual bool representsNonMatrix() {return true;}
	virtual bool representsScalar() {return true;}

};

enum {
	VARIABLE_ID_E = 100,
	VARIABLE_ID_PI = 101,
	VARIABLE_ID_EULER = 102,
	VARIABLE_ID_CATALAN = 103,
	VARIABLE_ID_PRECISION = 140,
	VARIABLE_ID_TODAY = 161,
	VARIABLE_ID_TOMORROW = 162,
	VARIABLE_ID_YESTERDAY = 163,
	VARIABLE_ID_NOW = 164,
	VARIABLE_ID_UPTIME = 201
};

/// Dynamic variable for Pi
DECLARE_BUILTIN_VARIABLE(PiVariable, VARIABLE_ID_PI)
/// Dynamic variable for e, the base of natural logarithms
DECLARE_BUILTIN_VARIABLE(EVariable, VARIABLE_ID_E)
/// Dynamic variable for Euler's constant
DECLARE_BUILTIN_VARIABLE(EulerVariable, VARIABLE_ID_EULER)
/// Dynamic variable for Catalan's constant
DECLARE_BUILTIN_VARIABLE(CatalanVariable, VARIABLE_ID_CATALAN)

/// Dynamic variable for current precision
class PrecisionVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const;
  public:
	PrecisionVariable();
	PrecisionVariable(const PrecisionVariable *variable) {set(variable);}
	ExpressionItem *copy() const {return new PrecisionVariable(this);}
	bool representsInteger(bool = false) {return true;}
	bool representsNonInteger(bool = false) {return false;}
	int id() const {return VARIABLE_ID_PRECISION;}
};

class TodayVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const;
  public:
	TodayVariable();
	TodayVariable(const TodayVariable *variable) {set(variable);}
	ExpressionItem *copy() const {return new TodayVariable(this);}
	virtual bool representsPositive(bool = false) {return false;}
	virtual bool representsNonNegative(bool = false) {return false;}
	virtual bool representsNonInteger(bool = false) {return false;}
	virtual bool representsNumber(bool b = false) {return b;}
	virtual bool representsReal(bool b = false) {return b;}
	virtual bool representsNonZero(bool b = false) {return b;}
	int id() const {return VARIABLE_ID_TODAY;}
};
class TomorrowVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const;
  public:
	TomorrowVariable();
	TomorrowVariable(const TomorrowVariable *variable) {set(variable);}
	ExpressionItem *copy() const {return new TomorrowVariable(this);}
	virtual bool representsPositive(bool = false) {return false;}
	virtual bool representsNonNegative(bool = false) {return false;}
	virtual bool representsNonInteger(bool = false) {return false;}
	virtual bool representsNumber(bool b = false) {return b;}
	virtual bool representsReal(bool b = false) {return b;}
	virtual bool representsNonZero(bool b = false) {return b;}
	int id() const {return VARIABLE_ID_TOMORROW;}
};
class YesterdayVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const;
  public:
	YesterdayVariable();
	YesterdayVariable(const YesterdayVariable *variable) {set(variable);}
	ExpressionItem *copy() const {return new YesterdayVariable(this);}
	virtual bool representsPositive(bool = false) {return false;}
	virtual bool representsNonNegative(bool = false) {return false;}
	virtual bool representsNonInteger(bool = false) {return false;}
	virtual bool representsNumber(bool b = false) {return b;}
	virtual bool representsReal(bool b = false) {return b;}
	virtual bool representsNonZero(bool b = false) {return b;}
	int id() const {return VARIABLE_ID_YESTERDAY;}
};
class NowVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const;
  public:
	NowVariable();
	NowVariable(const NowVariable *variable) {set(variable);}
	ExpressionItem *copy() const {return new NowVariable(this);}
	virtual bool representsPositive(bool = false) {return false;}
	virtual bool representsNonNegative(bool = false) {return false;}
	virtual bool representsNonInteger(bool = false) {return false;}
	virtual bool representsNumber(bool b = false) {return b;}
	virtual bool representsReal(bool b = false) {return b;}
	virtual bool representsNonZero(bool b = false) {return b;}
	int id() const {return VARIABLE_ID_NOW;}
};
class UptimeVariable : public DynamicVariable {
  private:
	void calculate(MathStructure &m) const;
  public:
	UptimeVariable();
	UptimeVariable(const UptimeVariable *variable);
	ExpressionItem *copy() const;
	int id() const {return VARIABLE_ID_UPTIME;}
};

#endif
