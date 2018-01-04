/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Variable.h"
#include "util.h"
#include "Calculator.h"
#include "MathStructure.h"
#include "Number.h"

Assumptions::Assumptions() : i_type(ASSUMPTION_TYPE_NONE), i_sign(ASSUMPTION_SIGN_UNKNOWN), fmin(NULL), fmax(NULL), b_incl_min(true), b_incl_max(true) {}
Assumptions::~Assumptions() {}

bool Assumptions::isPositive() {return i_sign == ASSUMPTION_SIGN_POSITIVE || (fmin && (fmin->isPositive() || (!b_incl_min && fmin->isNonNegative())));}
bool Assumptions::isNegative() {return i_sign == ASSUMPTION_SIGN_NEGATIVE || (fmax && (fmax->isNegative() || (!b_incl_max && fmax->isNonPositive())));}
bool Assumptions::isNonNegative() {return i_sign == ASSUMPTION_SIGN_NONNEGATIVE || i_sign == ASSUMPTION_SIGN_POSITIVE || (fmin && fmin->isNonNegative());}
bool Assumptions::isNonPositive() {return i_sign == ASSUMPTION_SIGN_NONPOSITIVE || i_sign == ASSUMPTION_SIGN_NEGATIVE || (fmax && fmax->isNonPositive());}
bool Assumptions::isInteger() {return i_type >= ASSUMPTION_TYPE_INTEGER;}
bool Assumptions::isNumber() {return i_type >= ASSUMPTION_TYPE_NUMBER;}
bool Assumptions::isRational() {return i_type >= ASSUMPTION_TYPE_RATIONAL;}
bool Assumptions::isReal() {return i_type >= ASSUMPTION_TYPE_REAL;}
bool Assumptions::isComplex() {return i_type == ASSUMPTION_TYPE_COMPLEX;}
bool Assumptions::isNonZero() {return i_sign == ASSUMPTION_SIGN_NONZERO || isPositive() || isNegative();}
bool Assumptions::isNonMatrix() {return i_type >= ASSUMPTION_TYPE_NONMATRIX;}
bool Assumptions::isScalar() {return i_type >= ASSUMPTION_TYPE_NONMATRIX;}

AssumptionType Assumptions::type() {return i_type;}
AssumptionSign Assumptions::sign() {return i_sign;}
void Assumptions::setType(AssumptionType ant) {
	i_type = ant;
	if(i_type <= ASSUMPTION_TYPE_COMPLEX && i_sign != ASSUMPTION_SIGN_NONZERO) {
		i_sign = ASSUMPTION_SIGN_UNKNOWN;
	}
	if(i_type <= ASSUMPTION_TYPE_NONMATRIX) {
		if(fmax) delete fmax;
		if(fmin) delete fmin;
	}
}
void Assumptions::setSign(AssumptionSign as) {
	i_sign = as;
	if(i_type <= ASSUMPTION_TYPE_COMPLEX && i_sign != ASSUMPTION_SIGN_NONZERO && i_sign != ASSUMPTION_SIGN_UNKNOWN) {
		i_type = ASSUMPTION_TYPE_REAL;
	}
}
	
void Assumptions::setMin(const Number *nmin) {
	if(!nmin) {
		if(fmin) {
			delete fmin;
		}
		return;
	}
	if(i_type <= ASSUMPTION_TYPE_NONMATRIX) i_type = ASSUMPTION_TYPE_NUMBER;
	if(!fmin) {
		fmin = new Number(*nmin);
	} else {
		fmin->set(*nmin);
	}
}
void Assumptions::setIncludeEqualsMin(bool include_equals) {
	b_incl_min = include_equals;
}
bool Assumptions::includeEqualsMin() const {
	return b_incl_min;
}
const Number *Assumptions::min() const {
	return fmin;
}
void Assumptions::setMax(const Number *nmax) {
	if(!nmax) {
		if(fmax) {
			delete fmax;
		}
		return;
	}
	if(i_type <= ASSUMPTION_TYPE_NONMATRIX) i_type = ASSUMPTION_TYPE_NUMBER;
	if(!fmax) {
		fmax = new Number(*nmax);
	} else {
		fmax->set(*nmax);
	}
}
void Assumptions::setIncludeEqualsMax(bool include_equals) {
	b_incl_max = include_equals;
}
bool Assumptions::includeEqualsMax() const {
	return b_incl_max;
}
const Number *Assumptions::max() const {
	return fmax;
}


Variable::Variable(string cat_, string name_, string title_, bool is_local, bool is_builtin, bool is_active) : ExpressionItem(cat_, name_, title_, "", is_local, is_builtin, is_active) {
	setChanged(false);
}
Variable::Variable() : ExpressionItem() {}
Variable::Variable(const Variable *variable) {set(variable);}
Variable::~Variable() {}
void Variable::set(const ExpressionItem *item) {
	ExpressionItem::set(item);
}


UnknownVariable::UnknownVariable(string cat_, string name_, string title_, bool is_local, bool is_builtin, bool is_active) : Variable(cat_, name_, title_, is_local, is_builtin, is_active) {
	setChanged(false);
	o_assumption = NULL;
}
UnknownVariable::UnknownVariable() : Variable() {
	o_assumption = NULL;
}
UnknownVariable::UnknownVariable(const UnknownVariable *variable) {
	set(variable);
}
UnknownVariable::~UnknownVariable() {
	if(o_assumption) delete o_assumption;
}
ExpressionItem *UnknownVariable::copy() const {
	return new UnknownVariable(this);
}
void UnknownVariable::set(const ExpressionItem *item) {
	if(item->type() == TYPE_VARIABLE && item->subtype() == SUBTYPE_UNKNOWN_VARIABLE) {
		if(o_assumption) delete o_assumption;
		o_assumption = ((UnknownVariable*) item)->assumptions();
	}
	ExpressionItem::set(item);
}
void UnknownVariable::setAssumptions(Assumptions *ass) {
	if(o_assumption) delete o_assumption;
	o_assumption = ass;
}
Assumptions *UnknownVariable::assumptions() {
	return o_assumption;
}
bool UnknownVariable::representsPositive(bool) { 
	if(o_assumption) return o_assumption->isPositive();
	return CALCULATOR->defaultAssumptions()->isPositive();
}
bool UnknownVariable::representsNegative(bool) { 
	if(o_assumption) return o_assumption->isNegative();
	return CALCULATOR->defaultAssumptions()->isNegative();
}
bool UnknownVariable::representsNonNegative(bool) { 
	if(o_assumption) return o_assumption->isNonNegative();
	return CALCULATOR->defaultAssumptions()->isNonNegative();
}
bool UnknownVariable::representsNonPositive(bool) { 
	if(o_assumption) return o_assumption->isNonPositive();
	return CALCULATOR->defaultAssumptions()->isNonPositive();
}
bool UnknownVariable::representsInteger(bool) { 
	if(o_assumption) return o_assumption->isInteger();
	return CALCULATOR->defaultAssumptions()->isInteger();
}
bool UnknownVariable::representsNumber(bool) { 
	if(o_assumption) return o_assumption->isNumber();
	return CALCULATOR->defaultAssumptions()->isNumber();
}
bool UnknownVariable::representsRational(bool) { 
	if(o_assumption) return o_assumption->isRational();
	return CALCULATOR->defaultAssumptions()->isRational();
}
bool UnknownVariable::representsReal(bool) { 
	if(o_assumption) return o_assumption->isReal();
	return CALCULATOR->defaultAssumptions()->isReal();
}
bool UnknownVariable::representsComplex(bool) { 
	if(o_assumption) return o_assumption->isComplex();
	return CALCULATOR->defaultAssumptions()->isComplex();
}
bool UnknownVariable::representsNonZero(bool) { 
	if(o_assumption) return o_assumption->isNonZero();
	return CALCULATOR->defaultAssumptions()->isNonZero();
}
bool UnknownVariable::representsNonMatrix() { 
	if(o_assumption) return o_assumption->isNonMatrix();
	return CALCULATOR->defaultAssumptions()->isNonMatrix();
}
bool UnknownVariable::representsScalar() { 
	if(o_assumption) return o_assumption->isScalar();
	return CALCULATOR->defaultAssumptions()->isScalar();
}

KnownVariable::KnownVariable(string cat_, string name_, const MathStructure &o, string title_, bool is_local, bool is_builtin, bool is_active) : Variable(cat_, name_, title_, is_local, is_builtin, is_active) {
	mstruct = new MathStructure(o);
	setApproximate(mstruct->isApproximate());
	setPrecision(mstruct->precision());
	b_expression = false;
	sexpression = "";
	suncertainty = "";
	sunit = "";
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	setChanged(false);
}
KnownVariable::KnownVariable(string cat_, string name_, string expression_, string title_, bool is_local, bool is_builtin, bool is_active) : Variable(cat_, name_, title_, is_local, is_builtin, is_active) {
	mstruct = NULL;
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	suncertainty = "";
	sunit = "";
	set(expression_);
	setChanged(false);
}
KnownVariable::KnownVariable() : Variable() {
	mstruct = NULL;
}
KnownVariable::KnownVariable(const KnownVariable *variable) {
	mstruct = NULL;
	set(variable);
}
KnownVariable::~KnownVariable() {
	if(mstruct) delete mstruct;
}
ExpressionItem *KnownVariable::copy() const {
	return new KnownVariable(this);
}
bool KnownVariable::isExpression() const {
	return b_expression;
}
string KnownVariable::expression() const {
	return sexpression;
}
string KnownVariable::uncertainty() const {
	return suncertainty;
}
string KnownVariable::unit() const {
	return sunit;
}
void KnownVariable::set(const ExpressionItem *item) {
	if(item->type() == TYPE_VARIABLE && item->subtype() == SUBTYPE_KNOWN_VARIABLE) {
		calculated_precision = -1;
		calculated_with_interval = false;
		calculated_with_units = false;
		sexpression = ((KnownVariable*) item)->expression();
		suncertainty = ((KnownVariable*) item)->uncertainty();
		sunit = ((KnownVariable*) item)->unit();
		b_expression = ((KnownVariable*) item)->isExpression();
		if(!b_expression) {
			set(((KnownVariable*) item)->get());
		}
	}
	ExpressionItem::set(item);
}
void KnownVariable::set(const MathStructure &o) {
	if(!mstruct) mstruct = new MathStructure(o);
	else mstruct->set(o);
	setApproximate(mstruct->isApproximate());
	setPrecision(mstruct->precision());
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	b_expression = false;
	sexpression = "";
	setApproximate(o.isApproximate());
	setChanged(true);
}
void KnownVariable::set(string expression_) {
	if(mstruct) {
		delete mstruct;
	}	
	mstruct = NULL;
	b_expression = true;
	sexpression = expression_;
	remove_blank_ends(sexpression);
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	setChanged(true);
}
void KnownVariable::setUncertainty(string standard_uncertainty) {
	if(mstruct) {
		delete mstruct;
	}	
	mstruct = NULL;
	suncertainty = standard_uncertainty;
	remove_blank_ends(suncertainty);
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	if(!suncertainty.empty()) setApproximate(true);
	setChanged(true);
}
void KnownVariable::setUnit(string unit_expression) {
	if(mstruct) {
		delete mstruct;
	}	
	mstruct = NULL;
	sunit = unit_expression;
	remove_blank_ends(sunit);
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	setChanged(true);
}
bool set_precision_of_numbers(MathStructure &mstruct, int i_prec) {
	if(mstruct.isNumber()) {
		if(i_prec < 0) {
			if(!mstruct.number().isApproximate()) {
				mstruct.number().setApproximate();
				mstruct.numberUpdated();
			}
		} else if(mstruct.number().precision() < 0 || mstruct.number().precision() < i_prec) {
			mstruct.number().setPrecision(i_prec);
			mstruct.numberUpdated();
		}
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(set_precision_of_numbers(mstruct[i], i_prec)) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}
const MathStructure &KnownVariable::get() {
	if(b_expression && ((!mstruct || mstruct->isAborted()) || calculated_with_interval != CALCULATOR->usesIntervalArithmetic() || (!sunit.empty() && calculated_with_units != CALCULATOR->variableUnitsEnabled()))) {
		if(mstruct) mstruct->unref();
		mstruct = new MathStructure();
		ParseOptions po;
		if(isApproximate() && precision() == -1 && suncertainty.empty()) {
			po.read_precision = ALWAYS_READ_PRECISION;
		}
		if(suncertainty.empty()) {
			mstruct->setAborted();
			CALCULATOR->parse(mstruct, sexpression, po);
		} else {
			mstruct->number().set(sexpression, po);
			mstruct->numberUpdated();
		}
		if(!suncertainty.empty()) {
			Number nr_u(suncertainty);
			if(mstruct->isNumber()) {
				mstruct->number().setUncertainty(nr_u);
			} else if(mstruct->isMultiplication() && mstruct->size() > 0 && (*mstruct)[0].isNumber()) {
				(*mstruct)[0].number().setUncertainty(nr_u);
			}
		} else if(precision() >= 0) {
			if(mstruct->precision() < 0 || precision() < mstruct->precision()) {
				if(!set_precision_of_numbers(*mstruct, precision())) mstruct->setPrecision(precision(), true);
			}
		} else if(isApproximate()) {
			if(!mstruct->isApproximate()) {
				if(!set_precision_of_numbers(*mstruct, precision())) mstruct->setApproximate(true, true);
			}
		}
		if(!sunit.empty() && CALCULATOR->variableUnitsEnabled()) {
			MathStructure *mstruct_unit = new MathStructure;
			mstruct_unit->setAborted();
			CALCULATOR->parse(mstruct_unit, sunit, po);
			mstruct->multiply_nocopy(mstruct_unit);
		}
		calculated_with_interval = CALCULATOR->usesIntervalArithmetic();
		calculated_with_units = CALCULATOR->variableUnitsEnabled();
	}
	if(mstruct->contains(this, false, true, true) > 0) {
		CALCULATOR->error(true, _("Recursive variable: %s = %s"), name().c_str(), mstruct->print().c_str(), NULL);
		return m_undefined;
	}
	return *mstruct;
}
bool KnownVariable::representsPositive(bool allow_units) {return get().representsPositive(allow_units);}
bool KnownVariable::representsNegative(bool allow_units) {return get().representsNegative(allow_units);}
bool KnownVariable::representsNonNegative(bool allow_units) {return get().representsNonNegative(allow_units);}
bool KnownVariable::representsNonPositive(bool allow_units) {return get().representsNonPositive(allow_units);}
bool KnownVariable::representsInteger(bool allow_units) {return get().representsInteger(allow_units);}
bool KnownVariable::representsNonInteger(bool allow_units) {return get().representsNonInteger(allow_units);}
bool KnownVariable::representsFraction(bool allow_units) {return get().representsFraction(allow_units);}
bool KnownVariable::representsNumber(bool allow_units) {return get().representsNumber(allow_units);}
bool KnownVariable::representsRational(bool allow_units) {return get().representsRational(allow_units);}
bool KnownVariable::representsReal(bool allow_units) {return get().representsReal(allow_units);}
bool KnownVariable::representsComplex(bool allow_units) {return get().representsComplex(allow_units);}
bool KnownVariable::representsNonZero(bool allow_units) {return get().representsNonZero(allow_units);}
bool KnownVariable::representsEven(bool allow_units) {return get().representsEven(allow_units);}
bool KnownVariable::representsOdd(bool allow_units) {return get().representsOdd(allow_units);}
bool KnownVariable::representsUndefined(bool include_childs, bool include_infinite, bool be_strict) {return get().representsUndefined(include_childs, include_infinite, be_strict);}
bool KnownVariable::representsBoolean() {return get().representsBoolean();}
bool KnownVariable::representsNonMatrix() {return get().representsNonMatrix();}
bool KnownVariable::representsScalar() {return get().representsScalar();}

DynamicVariable::DynamicVariable(string cat_, string name_, string title_, bool is_local, bool is_builtin, bool is_active) : KnownVariable(cat_, name_, MathStructure(), title_, is_local, is_builtin, is_active) {
	mstruct = NULL;
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	setApproximate();
	setChanged(false);
}
DynamicVariable::DynamicVariable(const DynamicVariable *variable) {
	mstruct = NULL;
	set(variable);
	setApproximate();	
	setChanged(false);
}
DynamicVariable::DynamicVariable() : KnownVariable() {
	mstruct = NULL;
	calculated_precision = -1;
	calculated_with_interval = false;
	calculated_with_units = false;
	setApproximate();
	setChanged(false);
}
DynamicVariable::~DynamicVariable() {
	if(mstruct) delete mstruct;
}
void DynamicVariable::set(const ExpressionItem *item) {
	ExpressionItem::set(item);
}
void DynamicVariable::set(const MathStructure&) {}
void DynamicVariable::set(string) {}
const MathStructure &DynamicVariable::get() {
	if(calculated_with_interval != CALCULATOR->usesIntervalArithmetic() || calculated_precision != CALCULATOR->getPrecision() || !mstruct || mstruct->isAborted()) {
		if(mstruct) mstruct->unref();
		mstruct = new MathStructure();
		mstruct->setAborted();
		calculated_precision = CALCULATOR->getPrecision();
		calculated_with_interval = CALCULATOR->usesIntervalArithmetic();
		calculate();
	}
	return *mstruct;
}
int DynamicVariable::calculatedPrecision() const {
	return calculated_precision;
}
bool DynamicVariable::calculatedWithInterval() const {
	return calculated_with_interval;
}

PiVariable::PiVariable() : DynamicVariable("Constants", "pi") {}
void PiVariable::calculate() const {
	Number nr; nr.pi(); mstruct->set(nr);
}
EVariable::EVariable() : DynamicVariable("Constants", "e") {}
void EVariable::calculate() const {
	Number nr; nr.e(); mstruct->set(nr);
}
EulerVariable::EulerVariable() : DynamicVariable("Constants", "euler") {}
void EulerVariable::calculate() const {
	Number nr; nr.euler(); mstruct->set(nr);
}
CatalanVariable::CatalanVariable() : DynamicVariable("Constants", "catalan") {}
void CatalanVariable::calculate() const {
	Number nr; nr.catalan(); mstruct->set(nr);
}
PrecisionVariable::PrecisionVariable() : DynamicVariable("", "precision") {
	setApproximate(false);
}
void PrecisionVariable::calculate() const {
	mstruct->set(PRECISION, 1, 0);
}

