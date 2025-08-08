/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

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
#include "BuiltinFunctions.h"

using std::string;
using std::vector;

Assumptions::Assumptions() : i_type(ASSUMPTION_TYPE_NUMBER), i_sign(ASSUMPTION_SIGN_UNKNOWN), fmin(NULL), fmax(NULL), b_incl_min(true), b_incl_max(true) {}
Assumptions::~Assumptions() {
	if(fmin) delete fmin;
	if(fmax) delete fmax;
}

bool Assumptions::isPositive() {return i_sign == ASSUMPTION_SIGN_POSITIVE || (fmin && (fmin->isPositive() || (!b_incl_min && fmin->isNonNegative())));}
bool Assumptions::isNegative() {return i_sign == ASSUMPTION_SIGN_NEGATIVE || (fmax && (fmax->isNegative() || (!b_incl_max && fmax->isNonPositive())));}
bool Assumptions::isNonNegative() {return i_type == ASSUMPTION_TYPE_BOOLEAN || i_sign == ASSUMPTION_SIGN_NONNEGATIVE || i_sign == ASSUMPTION_SIGN_POSITIVE || (fmin && fmin->isNonNegative());}
bool Assumptions::isNonPositive() {return i_sign == ASSUMPTION_SIGN_NONPOSITIVE || i_sign == ASSUMPTION_SIGN_NEGATIVE || (fmax && fmax->isNonPositive());}
bool Assumptions::isInteger() {return i_type >= ASSUMPTION_TYPE_INTEGER;}
bool Assumptions::isBoolean() {return i_type == ASSUMPTION_TYPE_BOOLEAN || (i_type == ASSUMPTION_TYPE_INTEGER && fmin && fmax && fmin->isZero() && fmax->isOne());}
bool Assumptions::isNumber() {return i_type >= ASSUMPTION_TYPE_NUMBER || fmin || fmax;}
bool Assumptions::isRational() {return i_type >= ASSUMPTION_TYPE_RATIONAL;}
bool Assumptions::isReal() {return i_type >= ASSUMPTION_TYPE_REAL || (fmin && !fmin->hasImaginaryPart()) || (fmax && !fmax->hasImaginaryPart());}
bool Assumptions::isComplex() {return i_type == ASSUMPTION_TYPE_COMPLEX;}
bool Assumptions::isNonZero() {return i_sign == ASSUMPTION_SIGN_NONZERO || isPositive() || isNegative();}
bool Assumptions::isNonMatrix() {return i_type >= ASSUMPTION_TYPE_NONMATRIX || fmin || fmax;}
bool Assumptions::isScalar() {return i_type >= ASSUMPTION_TYPE_NONMATRIX || fmin || fmax;}

AssumptionType Assumptions::type() {return i_type;}
AssumptionSign Assumptions::sign() {return i_sign;}
void Assumptions::setType(AssumptionType ant) {
	i_type = ant;
	if(i_type == ASSUMPTION_TYPE_BOOLEAN || (i_type <= ASSUMPTION_TYPE_COMPLEX && i_sign != ASSUMPTION_SIGN_NONZERO)) {
		i_sign = ASSUMPTION_SIGN_UNKNOWN;
	}
	if(i_type <= ASSUMPTION_TYPE_NONMATRIX || i_type == ASSUMPTION_TYPE_BOOLEAN) {
		if(fmax) delete fmax;
		if(fmin) delete fmin;
	}
}
void Assumptions::setSign(AssumptionSign as) {
	i_sign = as;
	if((i_type == ASSUMPTION_TYPE_BOOLEAN && i_sign != ASSUMPTION_SIGN_UNKNOWN) || (i_type <= ASSUMPTION_TYPE_COMPLEX && i_sign != ASSUMPTION_SIGN_NONZERO && i_sign != ASSUMPTION_SIGN_UNKNOWN)) {
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
	if(i_type <= ASSUMPTION_TYPE_COMPLEX || i_type == ASSUMPTION_TYPE_BOOLEAN) i_type = ASSUMPTION_TYPE_REAL;
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
	if(i_type <= ASSUMPTION_TYPE_COMPLEX || i_type == ASSUMPTION_TYPE_BOOLEAN) i_type = ASSUMPTION_TYPE_REAL;
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
	mstruct = NULL;
}
UnknownVariable::UnknownVariable() : Variable() {
	o_assumption = NULL;
	mstruct = NULL;
}
UnknownVariable::UnknownVariable(const UnknownVariable *variable) {
	mstruct = NULL;
	o_assumption = NULL;
	set(variable);
}
UnknownVariable::~UnknownVariable() {
	if(o_assumption) delete o_assumption;
	if(mstruct) mstruct->unref();
}
ExpressionItem *UnknownVariable::copy() const {
	return new UnknownVariable(this);
}
void UnknownVariable::set(const ExpressionItem *item) {
	if(item->type() == TYPE_VARIABLE && item->subtype() == SUBTYPE_UNKNOWN_VARIABLE) {
		if(o_assumption) delete o_assumption;
		o_assumption = ((UnknownVariable*) item)->assumptions();
		if(((UnknownVariable*) item)->interval().isUndefined()) {
			if(mstruct) mstruct->unref();
			mstruct = NULL;
		} else {
			if(mstruct) mstruct->set(((UnknownVariable*) item)->interval());
			else mstruct = new MathStructure(((UnknownVariable*) item)->interval());
		}
	}
	ExpressionItem::set(item);
}
void UnknownVariable::setAssumptions(Assumptions *ass) {
	if(o_assumption) delete o_assumption;
	o_assumption = ass;
}
void UnknownVariable::setAssumptions(const MathStructure &mvar) {
	Assumptions *ass = new Assumptions();
	if(mvar.representsInteger(true)) ass->setType(ASSUMPTION_TYPE_INTEGER);
	else if(mvar.representsRational(true)) ass->setType(ASSUMPTION_TYPE_RATIONAL);
	else if(mvar.representsReal(true)) ass->setType(ASSUMPTION_TYPE_REAL);
	else if(mvar.representsComplex(true)) ass->setType(ASSUMPTION_TYPE_COMPLEX);
	else if(mvar.representsNumber(true)) ass->setType(ASSUMPTION_TYPE_NUMBER);
	else if(mvar.representsNonMatrix()) ass->setType(ASSUMPTION_TYPE_NONMATRIX);
	if(mvar.representsPositive(true)) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
	else if(mvar.representsNegative(true)) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
	else if(mvar.representsNonPositive(true)) ass->setSign(ASSUMPTION_SIGN_NONPOSITIVE);
	else if(mvar.representsNonNegative(true)) ass->setSign(ASSUMPTION_SIGN_NONNEGATIVE);
	else if(mvar.representsNonZero(true)) ass->setSign(ASSUMPTION_SIGN_NONZERO);
	if(o_assumption) delete o_assumption;
	o_assumption = ass;
}
Assumptions *UnknownVariable::assumptions() {
	return o_assumption;
}
const MathStructure &UnknownVariable::interval() const {
	if(mstruct) return *mstruct;
	return m_undefined;
}
void UnknownVariable::setInterval(const MathStructure &o) {
	setAssumptions(o);
	if(o.isUndefined()) {
		if(mstruct) mstruct->unref();
		mstruct = NULL;
	} else {
		if(mstruct) mstruct->set(o);
		else mstruct = new MathStructure(o);
		if(!o_assumption->isReal() && (o.isNumber() && o.number().isInterval() && !o.number().lowerEndPoint().hasImaginaryPart() && !o.number().upperEndPoint().hasImaginaryPart())) o_assumption->setType(ASSUMPTION_TYPE_REAL);
		else if(!o_assumption->isNumber() && o.isNumber() && o.number().isInterval()) o_assumption->setType(ASSUMPTION_TYPE_NUMBER);
	}
}
bool UnknownVariable::representsPositive(bool b) {
	if(!b && mstruct) return mstruct->representsPositive(false);
	if(o_assumption) return o_assumption->isPositive();
	return CALCULATOR->defaultAssumptions()->isPositive();
}
bool UnknownVariable::representsNegative(bool b) {
	if(!b && mstruct) return mstruct->representsNegative(false);
	if(o_assumption) return o_assumption->isNegative();
	return CALCULATOR->defaultAssumptions()->isNegative();
}
bool UnknownVariable::representsNonNegative(bool b) {
	if(!b && mstruct) return mstruct->representsNonNegative(false);
	if(o_assumption) return o_assumption->isNonNegative();
	return CALCULATOR->defaultAssumptions()->isNonNegative();
}
bool UnknownVariable::representsNonPositive(bool b) {
	if(!b && mstruct) return mstruct->representsNonPositive(false);
	if(o_assumption) return o_assumption->isNonPositive();
	return CALCULATOR->defaultAssumptions()->isNonPositive();
}
bool UnknownVariable::representsInteger(bool b) {
	if(!b && mstruct) return mstruct->representsInteger(false);
	if(o_assumption) return o_assumption->isInteger();
	return CALCULATOR->defaultAssumptions()->isInteger();
}
bool UnknownVariable::representsNumber(bool b) {
	if(!b && mstruct) return mstruct->representsNumber(false);
	if(o_assumption) return o_assumption->isNumber();
	return CALCULATOR->defaultAssumptions()->isNumber();
}
bool UnknownVariable::representsRational(bool b) {
	if(!b && mstruct) return mstruct->representsRational(false);
	if(o_assumption) return o_assumption->isRational();
	return CALCULATOR->defaultAssumptions()->isRational();
}
bool UnknownVariable::representsReal(bool b) {
	if(!b && mstruct) return mstruct->representsReal(false);
	if(o_assumption) return o_assumption->isReal();
	return CALCULATOR->defaultAssumptions()->isReal();
}
bool UnknownVariable::representsNonComplex(bool b) {
	if(mstruct && (!b || (!o_assumption || (!o_assumption->isReal() && !o_assumption->isComplex())))) return mstruct->representsNonComplex(b);
	if(o_assumption) return o_assumption->isReal();
	return CALCULATOR->defaultAssumptions()->isReal();
}
bool UnknownVariable::representsComplex(bool b) {
	if(!b && mstruct) return mstruct->representsComplex(false);
	if(o_assumption) return o_assumption->isComplex();
	return CALCULATOR->defaultAssumptions()->isComplex();
}
bool UnknownVariable::representsNonZero(bool b) {
	if(!b && mstruct) return mstruct->representsNonZero(false);
	if(o_assumption) return o_assumption->isNonZero();
	return CALCULATOR->defaultAssumptions()->isNonZero();
}
bool UnknownVariable::representsBoolean() {
	if(mstruct) return mstruct->representsBoolean();
	if(o_assumption) return o_assumption->isBoolean();
	return CALCULATOR->defaultAssumptions()->isBoolean();
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
	mstruct = new MathStructure(o); mstruct_alt = NULL;
	setApproximate(mstruct->isApproximate());
	setPrecision(mstruct->precision());
	b_expression = false;
	sexpression = "";
	suncertainty = "";
	b_relative_uncertainty = false;
	sunit = "";
	calculated_precision = -1;
	setChanged(false);
}
KnownVariable::KnownVariable(string cat_, string name_, string expression_, string title_, bool is_local, bool is_builtin, bool is_active) : Variable(cat_, name_, title_, is_local, is_builtin, is_active) {
	mstruct = NULL; mstruct_alt = NULL;
	calculated_precision = -1;
	b_expression = true;
	sexpression = expression_;
	remove_blank_ends(sexpression);
	suncertainty = "";
	b_relative_uncertainty = false;
	sunit = "";
	setChanged(false);
}
KnownVariable::KnownVariable() : Variable() {
	mstruct = NULL; mstruct_alt = NULL;
	b_expression = true;
}
KnownVariable::KnownVariable(const KnownVariable *variable) {
	mstruct = NULL; mstruct_alt = NULL;
	set(variable);
}
KnownVariable::~KnownVariable() {
	if(mstruct) delete mstruct;
	if(mstruct_alt) delete mstruct_alt;
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
string KnownVariable::uncertainty(bool *is_relative) const {
	if(is_relative) *is_relative = b_relative_uncertainty;
	return suncertainty;
}
string KnownVariable::unit() const {
	return sunit;
}
void KnownVariable::set(const ExpressionItem *item) {
	if(item->type() == TYPE_VARIABLE && item->subtype() == SUBTYPE_KNOWN_VARIABLE) {
		calculated_precision = -1;
		sexpression = ((KnownVariable*) item)->expression();
		suncertainty = ((KnownVariable*) item)->uncertainty(&b_relative_uncertainty);
		sunit = ((KnownVariable*) item)->unit();
		b_expression = ((KnownVariable*) item)->isExpression();
		if(!b_expression) {
			set(((KnownVariable*) item)->get());
		} else {
			if(mstruct) delete mstruct;
			if(mstruct_alt) delete mstruct_alt;
			mstruct = NULL;
			mstruct_alt = NULL;
		}
	}
	ExpressionItem::set(item);
}
void KnownVariable::set(const MathStructure &o) {
	if(!mstruct) mstruct = new MathStructure(o);
	else mstruct->set(o);
	if(mstruct_alt) delete mstruct_alt;
	mstruct_alt = NULL;
	setApproximate(mstruct->isApproximate());
	setPrecision(mstruct->precision());
	calculated_precision = -1;
	b_expression = false;
	sexpression = "";
	setApproximate(o.isApproximate());
	setChanged(true);
}
void KnownVariable::set(string expression_) {
	if(b_expression && sexpression == expression_) return;
	if(mstruct) delete mstruct;
	if(mstruct_alt) delete mstruct_alt;
	mstruct = NULL;
	mstruct_alt = NULL;
	b_expression = true;
	sexpression = expression_;
	remove_blank_ends(sexpression);
	calculated_precision = -1;
	setChanged(true);
}
void KnownVariable::setUncertainty(string standard_uncertainty, bool is_relative) {
	if(mstruct) delete mstruct;
	if(mstruct_alt) delete mstruct_alt;
	mstruct = NULL;
	mstruct_alt = NULL;
	suncertainty = standard_uncertainty;
	b_relative_uncertainty = is_relative;
	remove_blank_ends(suncertainty);
	calculated_precision = -1;
	if(!suncertainty.empty()) setApproximate(true);
	setChanged(true);
}
void KnownVariable::setUnit(string unit_expression) {
	if(mstruct) delete mstruct;
	if(mstruct_alt) delete mstruct_alt;
	mstruct = NULL;
	mstruct_alt = NULL;
	sunit = unit_expression;
	remove_blank_ends(sunit);
	calculated_precision = -1;
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
extern bool set_uncertainty(MathStructure &mstruct, MathStructure &munc, const EvaluationOptions &eo = default_evaluation_options, bool do_eval = false);
extern bool create_interval(MathStructure &mstruct, const MathStructure &m1, const MathStructure &m2);
bool replace_f_interval(MathStructure &mstruct) {
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_INTERVAL && mstruct.size() == 2) {
		if(mstruct[0].isNumber() && mstruct[1].isNumber()) {
			Number nr;
			if(nr.setInterval(mstruct[0].number(), mstruct[1].number())) {
				mstruct.set(nr, true);
				return true;
			}
		} else {
			MathStructure m1(mstruct[0]);
			MathStructure m2(mstruct[1]);
			if(create_interval(mstruct, m1, m2)) return true;
		}
	} else if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_UNCERTAINTY && mstruct.size() == 3 && mstruct[2].isNumber()) {
		bool b_rel = mstruct[2].number().getBoolean();
		if(mstruct[0].isNumber() && mstruct[1].isNumber()) {
			Number nr(mstruct[0].number());
			if(b_rel) nr.setRelativeUncertainty(mstruct[1].number());
			else nr.setUncertainty(mstruct[1].number());
			mstruct.set(nr, true);
			return true;
		} else if(!b_rel) {
			MathStructure m1(mstruct[0]);
			MathStructure m2(mstruct[1]);
			if(set_uncertainty(m1, m2)) {mstruct = m1; return true;}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(replace_f_interval(mstruct[i])) {
				mstruct.childUpdated(i + 1);
				b = true;
			}
		}
		return b;
	}
	return false;
}
const MathStructure &KnownVariable::get() {
	MathStructure *m = mstruct;
	if(b_expression && !CALCULATOR->variableUnitsEnabled() && !sunit.empty()) m = mstruct_alt;
	if(b_expression && (!m || m->isAborted())) {
		if(m) m->unref();
		if(!CALCULATOR->variableUnitsEnabled() && !sunit.empty()) {
			mstruct_alt = new MathStructure();
			m = mstruct_alt;
		} else {
			mstruct = new MathStructure();
			m = mstruct;
		}
		ParseOptions po;
		if(isApproximate() && precision() == -1 && suncertainty.empty()) {
			po.read_precision = ALWAYS_READ_PRECISION;
		}
		bool b_number = false;
		if(!suncertainty.empty()) {
			b_number = true;
		} else {
			size_t i = sexpression.rfind(')');
			if(i != string::npos && i > 2 && (i == sexpression.length() - 1 || (i < sexpression.length() - 2 && (sexpression[i + 1] == 'E' || sexpression[i + 1] == 'e')))) {
				size_t i2 = sexpression.rfind('(');
				if(i2 != string::npos && i2 < i - 1) {
					if(sexpression.find_first_not_of(NUMBER_ELEMENTS SPACES, sexpression[0] == '-' || sexpression[0] == '+' ? 1 : 0) == i2 && sexpression.find_first_not_of(NUMBERS SPACES, i2 + 1) == i && (i == sexpression.length() - 1 || sexpression.find_first_not_of(NUMBER_ELEMENTS SPACES, sexpression[i + 2] == '-' || sexpression[i + 2] == '+' ? i + 3 : i + 2) == string::npos)) {
						b_number = true;
					}
				}
			}
		}
		if(b_number) {
			m->number().set(sexpression, po);
			m->numberUpdated();
		} else {
			m->setAborted();
			if(b_local && !CALCULATOR->conciseUncertaintyInputEnabled()) {
				CALCULATOR->setConciseUncertaintyInputEnabled(true);
				CALCULATOR->parse(m, sexpression, po);
				CALCULATOR->setConciseUncertaintyInputEnabled(false);
			} else {
				CALCULATOR->parse(m, sexpression, po);
			}
		}
		if(!sunit.empty() && (!CALCULATOR->variableUnitsEnabled() || sunit != "auto")) {
			m->removeType(STRUCT_UNIT);
			if(m->containsType(STRUCT_UNIT, false, true, true) != 0) m->transformById(FUNCTION_ID_STRIP_UNITS);
		}
		if(!suncertainty.empty()) {
			Number nr_u(suncertainty);
			if(m->isNumber()) {
				if(b_relative_uncertainty) m->number().setRelativeUncertainty(nr_u);
				else m->number().setUncertainty(nr_u);
				m->numberUpdated();
			} else if(m->isMultiplication() && m->size() > 0 && (*m)[0].isNumber()) {
				if(b_relative_uncertainty) (*m)[0].number().setRelativeUncertainty(nr_u);
				else (*m)[0].number().setUncertainty(nr_u);
				(*m)[0].numberUpdated();
				m->childUpdated(1);
			}
		} else if(precision() >= 0) {
			if(m->precision() < 0 || precision() < m->precision()) {
				if(!set_precision_of_numbers(*m, precision())) m->setPrecision(precision(), true);
			}
		} else if(isApproximate()) {
			if(!m->isApproximate()) {
				if(!set_precision_of_numbers(*m, precision())) m->setApproximate(true, true);
			}
		}
		if(!sunit.empty() && CALCULATOR->variableUnitsEnabled() && sunit != "auto") {
			MathStructure *m_unit = new MathStructure;
			m_unit->setAborted();
			CALCULATOR->parse(m_unit, sunit, po);
			m->multiply_nocopy(m_unit);
		}
		//m->unformat();
		replace_f_interval(*m);
	}
	if(m->contains(this, false, true, true) > 0) {
		CALCULATOR->error(true, _("Recursive variable: %s = %s"), name().c_str(), m->print().c_str(), NULL);
		return m_undefined;
	}
	return *m;
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
bool KnownVariable::representsNonComplex(bool allow_units) {return get().representsNonComplex(allow_units);}
bool KnownVariable::representsComplex(bool allow_units) {return get().representsComplex(allow_units);}
bool KnownVariable::representsNonZero(bool allow_units) {return get().representsNonZero(allow_units);}
bool KnownVariable::representsEven(bool allow_units) {return get().representsEven(allow_units);}
bool KnownVariable::representsOdd(bool allow_units) {return get().representsOdd(allow_units);}
bool KnownVariable::representsUndefined(bool include_childs, bool include_infinite, bool be_strict) {return get().representsUndefined(include_childs, include_infinite, be_strict);}
bool KnownVariable::representsBoolean() {return get().representsBoolean();}
bool KnownVariable::representsNonMatrix() {return get().representsNonMatrix();}
bool KnownVariable::representsScalar() {return get().representsScalar();}

DynamicVariable::DynamicVariable(string cat_, string name_, string title_, bool is_local, bool is_builtin, bool is_active) : KnownVariable(cat_, name_, "", title_, is_local, is_builtin, is_active) {
	b_expression = false;
	always_recalculate = false;
	setApproximate();
	setChanged(false);
}
DynamicVariable::DynamicVariable(const DynamicVariable *variable) {
	set(variable);
	setApproximate();
	setChanged(false);
	always_recalculate = false;
}
DynamicVariable::DynamicVariable() : KnownVariable() {
	calculated_precision = -1;
	setApproximate();
	setChanged(false);
	always_recalculate = false;
}
DynamicVariable::~DynamicVariable() {}
void DynamicVariable::set(const ExpressionItem *item) {
	ExpressionItem::set(item);
}
void DynamicVariable::set(const MathStructure&) {}
void DynamicVariable::set(string) {}
const MathStructure &DynamicVariable::get() {
	MathStructure *m = mstruct;
	if(!always_recalculate && !CALCULATOR->usesIntervalArithmetic()) m = mstruct_alt;
	if(always_recalculate || calculated_precision != CALCULATOR->getPrecision() || !m || m->isAborted()) {
		if(m) {
			if(mstruct) {mstruct->unref(); mstruct = NULL;}
			if(mstruct_alt) {mstruct_alt->unref(); mstruct_alt = NULL;}
		}
		if(!always_recalculate && !CALCULATOR->usesIntervalArithmetic()) {
			mstruct_alt = new MathStructure();
			mstruct_alt->setAborted();
			m = mstruct_alt;
		} else {
			mstruct = new MathStructure();
			mstruct->setAborted();
			m = mstruct;
		}
		calculated_precision = CALCULATOR->getPrecision();
		calculate(*m);
	}
	return *m;
}
int DynamicVariable::calculatedPrecision() const {
	return calculated_precision;
}

PiVariable::PiVariable() : DynamicVariable("Constants", "pi") {}
void PiVariable::calculate(MathStructure &m) const {
	Number nr; nr.pi(); m.set(nr);
}
EVariable::EVariable() : DynamicVariable("Constants", "e") {}
void EVariable::calculate(MathStructure &m) const {
	Number nr; nr.e(); m.set(nr);
}
EulerVariable::EulerVariable() : DynamicVariable("Constants", "euler") {}
void EulerVariable::calculate(MathStructure &m) const {
	Number nr; nr.euler(); m.set(nr);
}
CatalanVariable::CatalanVariable() : DynamicVariable("Constants", "catalan") {}
void CatalanVariable::calculate(MathStructure &m) const {
	Number nr; nr.catalan(); m.set(nr);
}
PrecisionVariable::PrecisionVariable() : DynamicVariable("", "precision") {
	setApproximate(false);
}
void PrecisionVariable::calculate(MathStructure &m) const {
	m.set(PRECISION, 1, 0);
}

TodayVariable::TodayVariable() : DynamicVariable("", "today") {
	setApproximate(false);
	always_recalculate = true;
}
void TodayVariable::calculate(MathStructure &m) const {
	QalculateDateTime dt;
	dt.setToCurrentDate();
	m.set(dt);
}
YesterdayVariable::YesterdayVariable() : DynamicVariable("", "yesterday") {
	setApproximate(false);
	always_recalculate = true;
}
void YesterdayVariable::calculate(MathStructure &m) const {
	QalculateDateTime dt;
	dt.setToCurrentDate();
	dt.addDays(-1);
	m.set(dt);
}
TomorrowVariable::TomorrowVariable() : DynamicVariable("", "tomorrow") {
	setApproximate(false);
	always_recalculate = true;
}
void TomorrowVariable::calculate(MathStructure &m) const {
	QalculateDateTime dt;
	dt.setToCurrentDate();
	dt.addDays(1);
	m.set(dt);
}
NowVariable::NowVariable() : DynamicVariable("", "now") {
	setApproximate(false);
	always_recalculate = true;
}
void NowVariable::calculate(MathStructure &m) const {
	QalculateDateTime dt;
	dt.setToCurrentTime();
	m.set(dt);
}

#ifdef __linux__
#	include <sys/sysinfo.h>
#endif

#if defined __linux__ || defined _WIN32

#include <fstream>

void UptimeVariable:: calculate(MathStructure &m) const {
#ifndef DISABLE_INSECURE
	Number nr;
#	ifdef __linux__
	std::ifstream proc_uptime("/proc/uptime", std::ios::in);
	if(proc_uptime.is_open()) {
		string s_uptime;
		getline(proc_uptime, s_uptime, ' ');
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
#endif
}
UptimeVariable::UptimeVariable() : DynamicVariable("", "uptime") {
	setApproximate(false);
	always_recalculate = true;
}
UptimeVariable::UptimeVariable(const UptimeVariable *variable) {set(variable);}
ExpressionItem *UptimeVariable::copy() const {return new UptimeVariable(this);}
#endif

