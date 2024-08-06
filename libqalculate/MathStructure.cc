/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "Calculator.h"
#include "MathStructure.h"
#include "BuiltinFunctions.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "Unit.h"
#include "Prefix.h"
#include <map>
#include <algorithm>
#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

#include "MathStructure_p.h"

void MathStructure::mergePrecision(const MathStructure &o) {MERGE_APPROX_AND_PREC(o)}
void MathStructure::mergePrecision(bool approx, int prec) {
	if(!b_approx && approx) setApproximate();
	if(prec >= 0 && (i_precision < 0 || prec < i_precision)) {
		setPrecision(prec);
	}
}

void MathStructure::ref() {
	i_ref++;
}
void MathStructure::unref() {
	i_ref--;
	if(i_ref == 0) {
		delete this;
	}
}
size_t MathStructure::refcount() const {
	return i_ref;
}


inline void MathStructure::init() {
	m_type = STRUCT_NUMBER;
	b_approx = false;
	b_plural = false;
	i_precision = -1;
	i_ref = 1;
	ct_comp = COMPARISON_EQUALS;
	function_value = NULL;
	b_protected = false;
	o_variable = NULL;
	o_function = NULL;
	o_unit = NULL;
	o_prefix = NULL;
	o_datetime = NULL;
	b_parentheses = false;
}

MathStructure::MathStructure() {
	init();
}
MathStructure::MathStructure(const MathStructure &o) {
	init();
	switch(o.type()) {
		case STRUCT_NUMBER: {
			o_number.set(o.number());
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			s_sym = o.symbol();
			break;
		}
		case STRUCT_DATETIME: {
			o_datetime = new QalculateDateTime(*o.datetime());
			break;
		}
		case STRUCT_FUNCTION: {
			o_function = o.function();
			if(o_function) o.function()->ref();
			if(o.functionValue()) function_value = new MathStructure(*o.functionValue());
			break;
		}
		case STRUCT_VARIABLE: {
			o_variable = o.variable();
			if(o_variable) o_variable->ref();
			break;
		}
		case STRUCT_UNIT: {
			o_unit = o.unit();
			if(o_unit) o_unit->ref();
			b_plural = o.isPlural();
			break;
		}
		case STRUCT_COMPARISON: {
			ct_comp = o.comparisonType();
			break;
		}
		default: {}
	}
	o_prefix = o.prefix();
	b_protected = o.isProtected();
	for(size_t i = 0; i < o.size(); i++) {
		APPEND_COPY((&o[i]))
	}
	b_approx = o.isApproximate();
	i_precision = o.precision();
	m_type = o.type();
	b_parentheses = o.inParentheses();
}
MathStructure::MathStructure(long int num, long int den, long int exp10) {
	init();
	o_number.set(num, den, exp10);
}
MathStructure::MathStructure(int num, int den, int exp10) {
	init();
	o_number.set(num, den, exp10);
}
MathStructure::MathStructure(string sym, bool force_symbol) {
	init();
	if(!force_symbol && sym.length() > 1) {
		if(sym == "undefined") {
			setUndefined(false);
			return;
		}
		o_datetime = new QalculateDateTime();
		if(o_datetime->set(sym)) {
			m_type = STRUCT_DATETIME;
			return;
		}
		delete o_datetime;
		o_datetime = NULL;
	}
	s_sym = sym;
	m_type = STRUCT_SYMBOLIC;
}
MathStructure::MathStructure(const QalculateDateTime &o_dt) {
	init();
	o_datetime = new QalculateDateTime(o_dt);
	m_type = STRUCT_DATETIME;
}
MathStructure::MathStructure(double float_value) {
	init();
	o_number.setFloat(float_value);
	b_approx = o_number.isApproximate();
	i_precision = o_number.precision();
}
MathStructure::MathStructure(const MathStructure *o, ...) {
	init();
	va_list ap;
	va_start(ap, o);
	if(o) {
		APPEND_COPY(o)
		while(true) {
			o = va_arg(ap, const MathStructure*);
			if(!o) break;
			APPEND_COPY(o)
		}
	}
	va_end(ap);
	m_type = STRUCT_VECTOR;
}
MathStructure::MathStructure(MathFunction *o, ...) {
	init();
	va_list ap;
	va_start(ap, o);
	o_function = o;
	if(o_function) o_function->ref();
	while(true) {
		const MathStructure *mstruct = va_arg(ap, const MathStructure*);
		if(!mstruct) break;
		APPEND_COPY(mstruct)
	}
	va_end(ap);
	m_type = STRUCT_FUNCTION;
}
MathStructure::MathStructure(Unit *u, Prefix *p) {
	init();
	o_unit = u;
	o_prefix = p;
	if(o_unit) o_unit->ref();
	m_type = STRUCT_UNIT;
}
MathStructure::MathStructure(Variable *o) {
	init();
	o_variable = o;
	if(o_variable) o_variable->ref();
	m_type = STRUCT_VARIABLE;
}
MathStructure::MathStructure(const Number &o) {
	init();
	o_number.set(o);
	b_approx = o_number.isApproximate();
	i_precision = o_number.precision();
}
MathStructure::~MathStructure() {
	if(function_value) function_value->unref();
	if(o_function) o_function->unref();
	if(o_variable) o_variable->unref();
	if(o_unit) o_unit->unref();
	if(o_datetime) delete o_datetime;
	for(size_t i = 0; i < v_subs.size(); i++) {v_subs[i]->unref();}
}

void MathStructure::set(const MathStructure &o, bool merge_precision) {
	Variable *var_bak = o_variable;
	if(var_bak) var_bak->ref();
	clear(merge_precision);
	switch(o.type()) {
		case STRUCT_NUMBER: {
			o_number.set(o.number());
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			s_sym = o.symbol();
			break;
		}
		case STRUCT_DATETIME: {
			o_datetime = new QalculateDateTime(*o.datetime());
			break;
		}
		case STRUCT_FUNCTION: {
			o_function = o.function();
			if(o_function) o.function()->ref();
			if(o.functionValue()) function_value = new MathStructure(*o.functionValue());
			break;
		}
		case STRUCT_VARIABLE: {
			o_variable = o.variable();
			if(o_variable) o_variable->ref();
			break;
		}
		case STRUCT_UNIT: {
			o_unit = o.unit();
			o_prefix = o.prefix();
			if(o_unit) o_unit->ref();
			b_plural = o.isPlural();
			break;
		}
		case STRUCT_COMPARISON: {
			ct_comp = o.comparisonType();
			break;
		}
		default: {}
	}
	b_protected = o.isProtected();
	for(size_t i = 0; i < o.size(); i++) {
		APPEND_COPY((&o[i]))
	}
	if(merge_precision) {
		MERGE_APPROX_AND_PREC(o);
	} else {
		b_approx = o.isApproximate();
		i_precision = o.precision();
	}
	b_parentheses = o.inParentheses();
	m_type = o.type();
	if(var_bak) var_bak->unref();
}
void MathStructure::set_nocopy(MathStructure &o, bool merge_precision) {
	Variable *var_bak = o_variable;
	if(var_bak) var_bak->ref();
	o.ref();
	clear(merge_precision);
	switch(o.type()) {
		case STRUCT_NUMBER: {
			o_number.set(o.number());
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			s_sym = o.symbol();
			break;
		}
		case STRUCT_DATETIME: {
			o_datetime = new QalculateDateTime(*o.datetime());
			break;
		}
		case STRUCT_FUNCTION: {
			o_function = o.function();
			if(o_function) o_function->ref();
			if(o.functionValue()) {
				function_value = (MathStructure*) o.functionValue();
				function_value->ref();
			}
			break;
		}
		case STRUCT_VARIABLE: {
			o_variable = o.variable();
			if(o_variable) o_variable->ref();
			break;
		}
		case STRUCT_UNIT: {
			o_unit = o.unit();
			if(o_unit) o_unit->ref();
			b_plural = o.isPlural();
			break;
		}
		case STRUCT_COMPARISON: {
			ct_comp = o.comparisonType();
			break;
		}
		default: {}
	}
	o_prefix = o.prefix();
	b_protected = o.isProtected();
	for(size_t i = 0; i < o.size(); i++) {
		APPEND_REF((&o[i]))
	}
	if(merge_precision) {
		MERGE_APPROX_AND_PREC(o);
	} else {
		b_approx = o.isApproximate();
		i_precision = o.precision();
	}
	b_parentheses = o.inParentheses();
	m_type = o.type();
	o.unref();
	if(var_bak) var_bak->unref();
}
void MathStructure::setToChild(size_t index, bool preserve_precision, MathStructure *mparent, size_t index_this) {
	if(index > 0 && index <= SIZE) {
		if(mparent) {
			CHILD(index - 1).ref();
			mparent->setChild_nocopy(&CHILD(index - 1), index_this, preserve_precision);
		} else {
			set_nocopy(CHILD(index - 1), preserve_precision);
		}
	}
}
void MathStructure::set(long int num, long int den, long int exp10, bool preserve_precision) {
	clear(preserve_precision);
	o_number.set(num, den, exp10);
	if(!preserve_precision) {
		b_approx = false;
		i_precision = -1;
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::set(int num, int den, int exp10, bool preserve_precision) {
	clear(preserve_precision);
	o_number.set(num, den, exp10);
	if(!preserve_precision) {
		b_approx = false;
		i_precision = -1;
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::set(double float_value, bool preserve_precision) {
	clear(preserve_precision);
	o_number.setFloat(float_value);
	if(preserve_precision) {
		MERGE_APPROX_AND_PREC(o_number);
	} else {
		b_approx = o_number.isApproximate();
		i_precision = o_number.precision();
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::set(string sym, bool preserve_precision, bool force_symbol) {
	clear(preserve_precision);
	if(!force_symbol && sym.length() > 1) {
		if(sym == "undefined") {
			setUndefined(true);
			return;
		}
		o_datetime = new QalculateDateTime();
		if(o_datetime->set(sym)) {
			m_type = STRUCT_DATETIME;
			return;
		}
		delete o_datetime;
		o_datetime = NULL;
	}
	s_sym = sym;
	m_type = STRUCT_SYMBOLIC;
}
void MathStructure::set(const QalculateDateTime &o_dt, bool preserve_precision) {
	clear(preserve_precision);
	o_datetime = new QalculateDateTime(o_dt);
	m_type = STRUCT_DATETIME;
}
void MathStructure::setVector(const MathStructure *o, ...) {
	clear();
	va_list ap;
	va_start(ap, o);
	if(o) {
		APPEND_COPY(o)
		while(true) {
			o = va_arg(ap, const MathStructure*);
			if(!o) break;
			APPEND_COPY(o)
		}
	}
	va_end(ap);
	m_type = STRUCT_VECTOR;
}
void MathStructure::set(MathFunction *o, ...) {
	clear();
	va_list ap;
	va_start(ap, o);
	o_function = o;
	if(o_function) o_function->ref();
	while(true) {
		const MathStructure *mstruct = va_arg(ap, const MathStructure*);
		if(!mstruct) break;
		APPEND_COPY(mstruct)
	}
	va_end(ap);
	m_type = STRUCT_FUNCTION;
}
void MathStructure::set(Unit *u, Prefix *p, bool preserve_precision) {
	clear(preserve_precision);
	o_unit = u;
	o_prefix = p;
	if(o_unit) o_unit->ref();
	m_type = STRUCT_UNIT;
}
void MathStructure::set(Variable *o, bool preserve_precision) {
	clear(preserve_precision);
	o_variable = o;
	if(o_variable) o_variable->ref();
	m_type = STRUCT_VARIABLE;
}
void MathStructure::set(const Number &o, bool preserve_precision) {
	clear(preserve_precision);
	o_number.set(o);
	if(preserve_precision) {
		MERGE_APPROX_AND_PREC(o_number);
	} else {
		b_approx = o_number.isApproximate();
		i_precision = o_number.precision();
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::setUndefined(bool preserve_precision) {
	clear(preserve_precision);
	m_type = STRUCT_UNDEFINED;
}
void MathStructure::setAborted(bool preserve_precision) {
	clear(preserve_precision);
	m_type = STRUCT_ABORTED;
	s_sym = _("aborted");
}

void MathStructure::setProtected(bool do_protect) {b_protected = do_protect;}
bool MathStructure::isProtected() const {return b_protected;}

void MathStructure::operator = (const MathStructure &o) {set(o);}
void MathStructure::operator = (const Number &o) {set(o);}
void MathStructure::operator = (int i) {set(i, 1, 0);}
void MathStructure::operator = (Unit *u) {set(u);}
void MathStructure::operator = (Variable *v) {set(v);}
void MathStructure::operator = (string sym) {set(sym);}
MathStructure MathStructure::operator - () const {
	MathStructure o2(*this);
	o2.negate();
	return o2;
}
MathStructure MathStructure::operator * (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.multiply(o);
	return o2;
}
MathStructure MathStructure::operator / (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.divide(o);
	return o2;
}
MathStructure MathStructure::operator + (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.add(o);
	return o;
}
MathStructure MathStructure::operator - (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.subtract(o);
	return o2;
}
MathStructure MathStructure::operator ^ (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.raise(o);
	return o2;
}
MathStructure MathStructure::operator && (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.add(o, OPERATION_LOGICAL_AND);
	return o2;
}
MathStructure MathStructure::operator || (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.add(o, OPERATION_LOGICAL_OR);
	return o2;
}
MathStructure MathStructure::operator ! () const {
	MathStructure o2(*this);
	o2.setLogicalNot();
	return o2;
}

void MathStructure::operator *= (const MathStructure &o) {multiply(o);}
void MathStructure::operator /= (const MathStructure &o) {divide(o);}
void MathStructure::operator += (const MathStructure &o) {add(o);}
void MathStructure::operator -= (const MathStructure &o) {subtract(o);}
void MathStructure::operator ^= (const MathStructure &o) {raise(o);}

void MathStructure::operator *= (const Number &o) {multiply(o);}
void MathStructure::operator /= (const Number &o) {divide(o);}
void MathStructure::operator += (const Number &o) {add(o);}
void MathStructure::operator -= (const Number &o) {subtract(o);}
void MathStructure::operator ^= (const Number &o) {raise(o);}

void MathStructure::operator *= (int i) {multiply(i);}
void MathStructure::operator /= (int i) {divide(i);}
void MathStructure::operator += (int i) {add(i);}
void MathStructure::operator -= (int i) {subtract(i);}
void MathStructure::operator ^= (int i) {raise(i);}

void MathStructure::operator *= (Unit *u) {multiply(u);}
void MathStructure::operator /= (Unit *u) {divide(u);}
void MathStructure::operator += (Unit *u) {add(u);}
void MathStructure::operator -= (Unit *u) {subtract(u);}
void MathStructure::operator ^= (Unit *u) {raise(u);}

void MathStructure::operator *= (Variable *v) {multiply(v);}
void MathStructure::operator /= (Variable *v) {divide(v);}
void MathStructure::operator += (Variable *v) {add(v);}
void MathStructure::operator -= (Variable *v) {subtract(v);}
void MathStructure::operator ^= (Variable *v) {raise(v);}

void MathStructure::operator *= (string sym) {multiply(sym);}
void MathStructure::operator /= (string sym) {divide(sym);}
void MathStructure::operator += (string sym) {add(sym);}
void MathStructure::operator -= (string sym) {subtract(sym);}
void MathStructure::operator ^= (string sym) {raise(sym);}

bool MathStructure::operator == (const MathStructure &o) const {return equals(o);}
bool MathStructure::operator == (const Number &o) const {return equals(o);}
bool MathStructure::operator == (int i) const {return equals(i);}
bool MathStructure::operator == (Unit *u) const {return equals(u);}
bool MathStructure::operator == (Variable *v) const {return equals(v);}
bool MathStructure::operator == (string sym) const {return equals(sym);}

bool MathStructure::operator != (const MathStructure &o) const {return !equals(o);}

const MathStructure &MathStructure::operator [] (size_t index) const {return CHILD(index);}
MathStructure &MathStructure::operator [] (size_t index) {return CHILD(index);}

MathStructure &MathStructure::last() {return LAST;}
const MathStructure MathStructure::last() const {return LAST;}

void MathStructure::clear(bool preserve_precision) {
	m_type = STRUCT_NUMBER;
	o_number.clear();
	if(function_value) {
		function_value->unref();
		function_value = NULL;
	}
	if(o_function) o_function->unref();
	o_function = NULL;
	if(o_variable) o_variable->unref();
	o_variable = NULL;
	if(o_unit) o_unit->unref();
	o_unit = NULL;
	if(o_datetime) delete o_datetime;
	o_datetime = NULL;
	o_prefix = NULL;
	b_plural = false;
	b_protected = false;
	b_parentheses = false;
	CLEAR;
	if(!preserve_precision) {
		i_precision = -1;
		b_approx = false;
	}
}
void MathStructure::clearVector(bool preserve_precision) {
	clear(preserve_precision);
	m_type = STRUCT_VECTOR;
}
void MathStructure::clearMatrix(bool preserve_precision) {
	clearVector(preserve_precision);
	MathStructure *mstruct = new MathStructure();
	mstruct->clearVector();
	APPEND_POINTER(mstruct);
}

const MathStructure *MathStructure::functionValue() const {
	return function_value;
}

const Number &MathStructure::number() const {
	return o_number;
}
Number &MathStructure::number() {
	return o_number;
}
void MathStructure::numberUpdated() {
	if(m_type != STRUCT_NUMBER) return;
	MERGE_APPROX_AND_PREC(o_number)
}
void MathStructure::childUpdated(size_t index, bool recursive) {
	if(index > SIZE || index < 1) return;
	if(recursive) CHILD(index - 1).childrenUpdated(true);
	MERGE_APPROX_AND_PREC(CHILD(index - 1))
}
void MathStructure::childrenUpdated(bool recursive) {
	for(size_t i = 0; i < SIZE; i++) {
		if(recursive) CHILD(i).childrenUpdated(true);
		MERGE_APPROX_AND_PREC(CHILD(i))
	}
}
const string &MathStructure::symbol() const {
	return s_sym;
}
const QalculateDateTime *MathStructure::datetime() const {
	return o_datetime;
}
QalculateDateTime *MathStructure::datetime() {
	return o_datetime;
}
ComparisonType MathStructure::comparisonType() const {
	return ct_comp;
}
void MathStructure::setComparisonType(ComparisonType comparison_type) {
	ct_comp = comparison_type;
}
void MathStructure::setType(StructureType mtype) {
	m_type = mtype;
	if(m_type != STRUCT_FUNCTION) {
		if(function_value) {
			function_value->unref();
			function_value = NULL;
		}
		if(o_function) o_function->unref();
		o_function = NULL;
	}
	if(m_type != STRUCT_VARIABLE && o_variable) {o_variable->unref(); o_variable = NULL;}
	if(m_type != STRUCT_UNIT && o_unit) {o_unit->unref(); o_unit = NULL; o_prefix = NULL;}
	if(m_type != STRUCT_DATETIME && o_datetime) {delete o_datetime; o_datetime = NULL;}
}
Unit *MathStructure::unit() const {
	return o_unit;
}
Unit *MathStructure::unit_exp_unit() const {
	if(isUnit()) return o_unit;
	if(isPower() && CHILD(0).isUnit()) return CHILD(0).unit();
	return NULL;
}
Prefix *MathStructure::prefix() const {
	return o_prefix;
}
Prefix *MathStructure::unit_exp_prefix() const {
	if(isUnit()) return o_prefix;
	if(isPower() && CHILD(0).isUnit()) return CHILD(0).prefix();
	return NULL;
}
void MathStructure::setPrefix(Prefix *p) {
	o_prefix = p;
}
bool MathStructure::isPlural() const {
	return b_plural;
}
void MathStructure::setPlural(bool is_plural) {
	if(isUnit()) b_plural = is_plural;
}
void MathStructure::setFunction(MathFunction *f) {
	if(f) f->ref();
	if(o_function) o_function->unref();
	o_function = f;
}
void MathStructure::setFunctionId(int id) {
	setFunction(CALCULATOR->getFunctionById(id));
}
void MathStructure::setUnit(Unit *u) {
	if(u) u->ref();
	if(o_unit) o_unit->unref();
	o_unit = u;
}
void MathStructure::setVariable(Variable *v) {
	if(v) v->ref();
	if(o_variable) o_variable->unref();
	o_variable = v;
}
MathFunction *MathStructure::function() const {
	return o_function;
}
Variable *MathStructure::variable() const {
	return o_variable;
}

bool MathStructure::isAddition() const {return m_type == STRUCT_ADDITION;}
bool MathStructure::isMultiplication() const {return m_type == STRUCT_MULTIPLICATION;}
bool MathStructure::isPower() const {return m_type == STRUCT_POWER;}
bool MathStructure::isSymbolic() const {return m_type == STRUCT_SYMBOLIC;}
bool MathStructure::isDateTime() const {return m_type == STRUCT_DATETIME;}
bool MathStructure::isAborted() const {return m_type == STRUCT_ABORTED;}
bool MathStructure::isEmptySymbol() const {return m_type == STRUCT_SYMBOLIC && s_sym.empty();}
bool MathStructure::isVector() const {return m_type == STRUCT_VECTOR;}
bool MathStructure::isMatrix() const {
	if(m_type != STRUCT_VECTOR || SIZE < 1) return false;
	for(size_t i = 0; i < SIZE; i++) {
		if(!CHILD(i).isVector() || (i > 0 && CHILD(i).size() != CHILD(i - 1).size())) return false;
	}
	return true;
}
bool MathStructure::isFunction() const {return m_type == STRUCT_FUNCTION;}
bool MathStructure::isUnit() const {return m_type == STRUCT_UNIT;}
bool MathStructure::isUnit_exp() const {return m_type == STRUCT_UNIT || (m_type == STRUCT_POWER && CHILD(0).isUnit());}
bool MathStructure::isUnknown() const {return m_type == STRUCT_SYMBOLIC || (m_type == STRUCT_VARIABLE && o_variable && !o_variable->isKnown());}
bool MathStructure::isUnknown_exp() const {return isUnknown() || (m_type == STRUCT_POWER && CHILD(0).isUnknown());}
bool MathStructure::isNumber_exp() const {return m_type == STRUCT_NUMBER || (m_type == STRUCT_POWER && CHILD(0).isNumber());}
bool MathStructure::isVariable() const {return m_type == STRUCT_VARIABLE;}
bool MathStructure::isComparison() const {return m_type == STRUCT_COMPARISON;}
bool MathStructure::isLogicalAnd() const {return m_type == STRUCT_LOGICAL_AND;}
bool MathStructure::isLogicalOr() const {return m_type == STRUCT_LOGICAL_OR;}
bool MathStructure::isLogicalXor() const {return m_type == STRUCT_LOGICAL_XOR;}
bool MathStructure::isLogicalNot() const {return m_type == STRUCT_LOGICAL_NOT;}
bool MathStructure::isBitwiseAnd() const {return m_type == STRUCT_BITWISE_AND;}
bool MathStructure::isBitwiseOr() const {return m_type == STRUCT_BITWISE_OR;}
bool MathStructure::isBitwiseXor() const {return m_type == STRUCT_BITWISE_XOR;}
bool MathStructure::isBitwiseNot() const {return m_type == STRUCT_BITWISE_NOT;}
bool MathStructure::isInverse() const {return m_type == STRUCT_INVERSE;}
bool MathStructure::isDivision() const {return m_type == STRUCT_DIVISION;}
bool MathStructure::isNegate() const {return m_type == STRUCT_NEGATE;}
bool MathStructure::isInfinity() const {return m_type == STRUCT_NUMBER && o_number.isInfinite(true);}
bool MathStructure::isUndefined() const {return m_type == STRUCT_UNDEFINED || (m_type == STRUCT_NUMBER && o_number.isUndefined()) || (m_type == STRUCT_VARIABLE && o_variable == CALCULATOR->getVariableById(VARIABLE_ID_UNDEFINED));}
bool MathStructure::isInteger() const {return m_type == STRUCT_NUMBER && o_number.isInteger();}
bool MathStructure::isInfinite(bool ignore_imag) const {return m_type == STRUCT_NUMBER && o_number.isInfinite(ignore_imag);}
bool MathStructure::isNumber() const {return m_type == STRUCT_NUMBER;}
bool MathStructure::isZero() const {return m_type == STRUCT_NUMBER && o_number.isZero();}
bool MathStructure::isApproximatelyZero() const {return m_type == STRUCT_NUMBER && !o_number.isNonZero();}
bool MathStructure::isOne() const {return m_type == STRUCT_NUMBER && o_number.isOne();}
bool MathStructure::isMinusOne() const {return m_type == STRUCT_NUMBER && o_number.isMinusOne();}

bool MathStructure::hasNegativeSign() const {
	return (m_type == STRUCT_NUMBER && o_number.hasNegativeSign()) || m_type == STRUCT_NEGATE || (m_type == STRUCT_MULTIPLICATION && SIZE > 0 && CHILD(0).hasNegativeSign());
}

bool MathStructure::representsBoolean() const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isOne() || o_number.isZero();}
		case STRUCT_VARIABLE: {return o_variable->representsBoolean();}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isBoolean();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsBoolean()) || o_function->representsBoolean(*this);}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsBoolean()) return false;
			}
			return true;
		}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_COMPARISON: {return true;}
		default: {return false;}
	}
}

bool MathStructure::representsNumber(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return !o_number.includesInfinity();}
		case STRUCT_VARIABLE: {return o_variable->representsNumber(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNumber();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNumber(allow_units)) || o_function->representsNumber(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_DATETIME: {return allow_units;}
		case STRUCT_POWER: {
			if(!CHILD(0).representsNonZero(allow_units) && !CHILD(1).representsPositive(allow_units)) return false;
		}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNumber(allow_units)) return false;
			}
			return true;
		}
		default: {return false;}
	}
}
bool MathStructure::representsInteger(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isInteger();}
		case STRUCT_VARIABLE: {return o_variable->representsInteger(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isInteger();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsInteger(allow_units)) || o_function->representsInteger(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsInteger(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsInteger(allow_units) && CHILD(1).representsInteger(false) && CHILD(1).representsPositive(false);
		}
		default: {return false;}
	}
}
bool MathStructure::representsNonInteger(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNonInteger();}
		case STRUCT_VARIABLE: {return o_variable->representsNonInteger(allow_units);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNonInteger(allow_units));}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			return false;
		}
		case STRUCT_POWER: {
			return false;
		}
		default: {return false;}
	}
}
bool MathStructure::representsFraction(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isFraction();}
		case STRUCT_VARIABLE: {return o_variable->representsFraction(allow_units);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsFraction(allow_units));}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			return false;
		}
		case STRUCT_POWER: {
			return false;
		}
		default: {return false;}
	}
}
bool MathStructure::representsPositive(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isPositive();}
		case STRUCT_VARIABLE: {return o_variable->representsPositive(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isPositive();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsPositive(true);
			return (function_value && function_value->representsPositive(allow_units)) || o_function->representsPositive(*this, allow_units);
		}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsPositive(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = true;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsPositive(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return (CHILD(0).representsPositive(allow_units) && CHILD(1).representsReal(false))
			|| (CHILD(0).representsNonZero(allow_units) && CHILD(0).representsReal(allow_units) && CHILD(1).representsEven(false) && CHILD(1).representsInteger(false));
		}
		default: {return false;}
	}
}
bool MathStructure::representsNegative(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNegative();}
		case STRUCT_VARIABLE: {return o_variable->representsNegative(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNegative();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsNegative(true);
			return (function_value && function_value->representsNegative(allow_units)) || o_function->representsNegative(*this, allow_units);
		}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNegative(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsPositive(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return CHILD(1).representsInteger(false) && CHILD(1).representsOdd(false) && CHILD(0).representsNegative(allow_units);
		}
		default: {return false;}
	}
}
bool MathStructure::representsNonNegative(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNonNegative();}
		case STRUCT_VARIABLE: {return o_variable->representsNonNegative(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonNegative();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsNonNegative(true);
			return (function_value && function_value->representsNonNegative(allow_units)) || o_function->representsNonNegative(*this, allow_units);
		}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonNegative(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = true;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsNonNegative(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return (CHILD(0).isZero() && CHILD(1).representsNonNegative(false))
			|| (CHILD(0).representsNonNegative(allow_units) && CHILD(1).representsReal(false))
			|| (CHILD(0).representsReal(allow_units) && CHILD(1).representsEven(false) && CHILD(1).representsInteger(false));
		}
		default: {return false;}
	}
}
bool MathStructure::representsNonPositive(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNonPositive();}
		case STRUCT_VARIABLE: {return o_variable->representsNonPositive(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonPositive();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsNonPositive(true);
			return (function_value && function_value->representsNonPositive(allow_units)) || o_function->representsNonPositive(*this, allow_units);
		}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonPositive(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsPositive(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return (CHILD(0).isZero() && CHILD(1).representsPositive(false)) || representsNegative(allow_units);
		}
		default: {return false;}
	}
}
bool MathStructure::representsRational(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isRational();}
		case STRUCT_VARIABLE: {return o_variable->representsRational(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isRational();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsRational(true);
			return (function_value && function_value->representsRational(allow_units)) || o_function->representsRational(*this, allow_units);
		}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsRational(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsRational(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(1).representsInteger(false) && CHILD(0).representsRational(allow_units) && (CHILD(0).representsPositive(allow_units) || (CHILD(0).representsNegative(allow_units) && CHILD(1).representsEven(false) && CHILD(1).representsPositive(false)));
		}
		default: {return false;}
	}
}
bool MathStructure::representsReal(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isReal();}
		case STRUCT_VARIABLE: {return o_variable->representsReal(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isReal();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsReal(true);
			return (function_value && function_value->representsReal(allow_units)) || o_function->representsReal(*this, allow_units);
		}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_DATETIME: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsReal(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsReal(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return (CHILD(0).representsPositive(allow_units) && CHILD(1).representsReal(false))
			|| (CHILD(0).representsReal(allow_units) && CHILD(1).representsInteger(false) && (CHILD(1).representsPositive(false) || CHILD(0).representsNonZero(allow_units)));
		}
		default: {return false;}
	}
}
bool MathStructure::representsFinite(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return !o_number.includesInfinity();}
		case STRUCT_VARIABLE: {
			if(o_variable->isKnown()) return ((KnownVariable*) o_variable)->get().representsFinite(allow_units);
			return o_variable->representsReal(allow_units);
		}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isReal();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsFinite(true);
			return (function_value && function_value->representsFinite(allow_units)) || o_function->representsReal(*this, allow_units);
		}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_DATETIME: {return allow_units;}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsFinite(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsFinite(allow_units) && CHILD(1).representsFinite(false) && (CHILD(1).representsPositive(false) || CHILD(0).representsNonZero(allow_units));
		}
		default: {return false;}
	}
}
bool MathStructure::representsNonComplex(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return !o_number.hasImaginaryPart();}
		case STRUCT_VARIABLE: {
			if(o_variable->isKnown()) return ((KnownVariable*) o_variable)->get().representsNonComplex(allow_units);
			return o_variable->representsNonComplex(allow_units);
		}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isReal();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsNonComplex(true);
			return (function_value && function_value->representsNonComplex(allow_units)) || o_function->representsNonComplex(*this, allow_units);
		}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_DATETIME: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonComplex(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonComplex(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return (CHILD(0).representsPositive(allow_units) && CHILD(1).representsReal(false))
			|| (CHILD(0).representsReal(allow_units) && CHILD(1).representsInteger(false));
		}
		default: {return false;}
	}
}
bool MathStructure::representsComplex(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.imaginaryPartIsNonZero();}
		case STRUCT_VARIABLE: {return o_variable->representsComplex(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isComplex();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsComplex(allow_units)) || o_function->representsComplex(*this, allow_units);}
		case STRUCT_ADDITION: {
			bool c = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsComplex(allow_units)) {
					if(c) return false;
					c = true;
				} else if(!CHILD(i).representsReal(allow_units)) {
					return false;
				}
			}
			return c;
		}
		case STRUCT_MULTIPLICATION: {
			bool c = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsComplex(allow_units)) {
					if(c) return false;
					c = true;
				} else if(!CHILD(i).representsReal(allow_units) || !CHILD(i).representsNonZero(allow_units)) {
					return false;
				}
			}
			return c;
		}
		case STRUCT_UNIT: {return false;}
		case STRUCT_POWER: {return CHILD(1).isNumber() && CHILD(1).number().isRational() && !CHILD(1).number().isInteger() && CHILD(0).representsNegative();}
		default: {return false;}
	}
}
bool MathStructure::representsNonZero(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNonZero();}
		case STRUCT_VARIABLE: {return o_variable->representsNonZero(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonZero();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNonZero(allow_units)) || o_function->representsNonZero(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_DATETIME: {return allow_units;}
		case STRUCT_ADDITION: {
			bool neg = false, started = false;
			for(size_t i = 0; i < SIZE; i++) {
				if((!started || neg) && CHILD(i).representsNegative(allow_units)) {
					neg = true;
				} else if(neg || !CHILD(i).representsPositive(allow_units)) {
					return false;
				}
				started = true;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonZero(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return (CHILD(0).representsNonZero(allow_units) && CHILD(1).representsNumber(true)) || (((!CHILD(0).isApproximatelyZero() && CHILD(1).representsNonPositive()) || CHILD(1).representsNegative()) && CHILD(0).representsNumber(allow_units) && CHILD(1).representsNumber(true));
		}
		default: {return false;}
	}
}
bool MathStructure::representsZero(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isZero();}
		case STRUCT_VARIABLE: {return o_variable->isKnown() && !o_variable->representsNonZero(allow_units) && ((KnownVariable*) o_variable)->get().representsZero();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsZero(allow_units));}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsZero(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsZero(allow_units)) {
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(i2 != i && CHILD(i2).representsUndefined(true, true, true)) return false;
					}
					return true;
				}
			}
			return false;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsZero(allow_units) && CHILD(1).representsPositive(allow_units);
		}
		default: {return false;}
	}
}
bool MathStructure::representsApproximatelyZero(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return !o_number.isNonZero();}
		case STRUCT_VARIABLE: {return o_variable->isKnown() && !o_variable->representsNonZero(allow_units) && ((KnownVariable*) o_variable)->get().representsApproximatelyZero();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsApproximatelyZero(allow_units));}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsApproximatelyZero(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsApproximatelyZero(allow_units)) {
					return true;
				}
			}
			return false;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsApproximatelyZero(allow_units) && CHILD(1).representsPositive(allow_units);
		}
		default: {return false;}
	}
}

bool MathStructure::representsEven(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isEven();}
		case STRUCT_VARIABLE: {return o_variable->representsEven(allow_units);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsEven(allow_units)) || o_function->representsEven(*this, allow_units);}
		default: {return false;}
	}
}
bool MathStructure::representsOdd(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isOdd();}
		case STRUCT_VARIABLE: {return o_variable->representsOdd(allow_units);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsOdd(allow_units)) || o_function->representsOdd(*this, allow_units);}
		default: {return false;}
	}
}
bool MathStructure::representsUndefined(bool include_childs, bool include_infinite, bool be_strict) const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			if(include_infinite) {
				return o_number.includesInfinity();
			}
			return false;
		}
		case STRUCT_UNDEFINED: {return true;}
		case STRUCT_VARIABLE: {return o_variable->representsUndefined(include_childs, include_infinite, be_strict);}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsUndefined(include_childs, include_infinite, be_strict);
			return (function_value && function_value->representsUndefined(include_childs, include_infinite, be_strict)) || o_function->representsUndefined(*this);
		}
		case STRUCT_POWER: {
			if(be_strict) {
				if((!CHILD(0).representsNonZero(true) && !CHILD(1).representsNonNegative(false)) || (CHILD(0).isInfinity() && !CHILD(1).representsNonZero(true))) {
					return true;
				}
			} else {
				if((CHILD(0).representsZero(true) && CHILD(1).representsNegative(false)) || (CHILD(0).isInfinity() && CHILD(1).representsZero(true))) {
					return true;
				}
			}
		}
		case STRUCT_MULTIPLICATION: {
			if(SIZE > 1 && CHILD(0).isZero() && CHILD(1).isInfinity()) return true;
		}
		default: {
			if(include_childs) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).representsUndefined(include_childs, include_infinite, be_strict)) return true;
				}
			}
			return false;
		}
	}
}
bool MathStructure::representsNonMatrix() const {
	switch(m_type) {
		case STRUCT_VECTOR: {
			if(SIZE == 0) return true;
			size_t n = 0;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isVariable() && CHILD(i).variable()->isKnown()) {
					if(((KnownVariable*) CHILD(i).variable())->get().isVector()) {
						if(n == 0) {
							n = ((KnownVariable*) CHILD(i).variable())->get().size();
							if(n == 0) return true;
						} else if(((KnownVariable*) CHILD(i).variable())->get().size() != n) {
							return true;
						}
					} else if(CHILD(i).variable()->representsScalar()) {
						return true;
					}
				} else if(CHILD(i).isVector()) {
					if(n == 0) {
						n = CHILD(i).size();
						if(n == 0) return true;
					} else if(CHILD(i).size() != n) {
						return true;
					}
				} else if(CHILD(i).representsScalar()) {
					return true;
				}
			}
			return false;
		}
		case STRUCT_POWER: {return CHILD(0).representsNonMatrix();}
		case STRUCT_VARIABLE: {return o_variable->representsNonMatrix();}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonMatrix();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsNonMatrix();
			return (function_value && function_value->representsNonMatrix()) || o_function->representsNonMatrix(*this);
		}
		case STRUCT_INVERSE: {}
		case STRUCT_NEGATE: {}
		case STRUCT_DIVISION: {}
		case STRUCT_MULTIPLICATION: {}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonMatrix()) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_ABORTED: {
			return false;
		}
		default: {}
	}
	return true;
}
bool MathStructure::representsScalar() const {
	switch(m_type) {
		case STRUCT_VECTOR: {return false;}
		case STRUCT_POWER: {return CHILD(0).representsScalar();}
		case STRUCT_VARIABLE: {return o_variable->representsScalar();}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonMatrix();}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) return CHILD(0).representsScalar();
			return (function_value && function_value->representsScalar()) || o_function->representsScalar(*this);
		}
		case STRUCT_INVERSE: {}
		case STRUCT_NEGATE: {}
		case STRUCT_DIVISION: {}
		case STRUCT_MULTIPLICATION: {}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsScalar()) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_ABORTED: {
			return false;
		}
		default: {}
	}
	return true;
}

void MathStructure::setApproximate(bool is_approx, bool recursive) {
	b_approx = is_approx;
	if(!b_approx) {
		i_precision = -1;
	}
	if(recursive) {
		if(m_type == STRUCT_NUMBER) {
			o_number.setApproximate(is_approx);
			if(i_precision < 0 || i_precision > o_number.precision()) i_precision = o_number.precision();
		}
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).setApproximate(is_approx, true);
		}
	}
}
bool MathStructure::isApproximate() const {
	return b_approx;
}

int MathStructure::precision() const {
	return i_precision;
}
void MathStructure::setPrecision(int prec, bool recursive) {
	i_precision = prec;
	if(i_precision > 0) b_approx = true;
	if(recursive) {
		if(m_type == STRUCT_NUMBER) {
			o_number.setPrecision(prec);
		}
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).setPrecision(prec, true);
		}
	}
}

void MathStructure::transform(StructureType mtype, const MathStructure &o) {
	MathStructure *struct_o = new MathStructure(o);
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(struct_o);
}
void MathStructure::transform(StructureType mtype, const Number &o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(o);
}
void MathStructure::transform(StructureType mtype, int i) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(new MathStructure(i, 1, 0));
}
void MathStructure::transform(StructureType mtype, Unit *u) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(u);
}
void MathStructure::transform(StructureType mtype, Variable *v) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(v);
}
void MathStructure::transform(StructureType mtype, string sym) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(sym);
}
void MathStructure::transform_nocopy(StructureType mtype, MathStructure *o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(o);
}
void MathStructure::transform(StructureType mtype) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
}
void MathStructure::transform(MathFunction *o) {
	transform(STRUCT_FUNCTION);
	setFunction(o);
}
void MathStructure::transformById(int id) {
	transform(STRUCT_FUNCTION);
	setFunctionId(id);
}
void MathStructure::transform(ComparisonType ctype, const MathStructure &o) {
	MathStructure *struct_o = new MathStructure(o);
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = STRUCT_COMPARISON;
	ct_comp = ctype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(struct_o);
}
void transform(ComparisonType ctype, const MathStructure &o);
void MathStructure::add(const MathStructure &o, MathOperation op, bool append) {
	switch(op) {
		case OPERATION_ADD: {
			add(o, append);
			break;
		}
		case OPERATION_SUBTRACT: {
			subtract(o, append);
			break;
		}
		case OPERATION_MULTIPLY: {
			multiply(o, append);
			break;
		}
		case OPERATION_DIVIDE: {
			divide(o, append);
			break;
		}
		case OPERATION_RAISE: {
			raise(o);
			break;
		}
		case OPERATION_EXP10: {
			MathStructure *mstruct = new MathStructure(10, 1, 0);
			mstruct->raise(o);
			multiply_nocopy(mstruct, append);
			break;
		}
		case OPERATION_LOGICAL_AND: {
			if(m_type == STRUCT_LOGICAL_AND && append) {
				APPEND(o);
			} else {
				transform(STRUCT_LOGICAL_AND, o);
			}
			break;
		}
		case OPERATION_LOGICAL_OR: {
			if(m_type == STRUCT_LOGICAL_OR && append) {
				APPEND(o);
			} else {
				transform(STRUCT_LOGICAL_OR, o);
			}
			break;
		}
		case OPERATION_LOGICAL_XOR: {
			transform(STRUCT_LOGICAL_XOR, o);
			break;
		}
		case OPERATION_BITWISE_AND: {
			if(m_type == STRUCT_BITWISE_AND && append) {
				APPEND(o);
			} else {
				transform(STRUCT_BITWISE_AND, o);
			}
			break;
		}
		case OPERATION_BITWISE_OR: {
			if(m_type == STRUCT_BITWISE_OR && append) {
				APPEND(o);
			} else {
				transform(STRUCT_BITWISE_OR, o);
			}
			break;
		}
		case OPERATION_BITWISE_XOR: {
			transform(STRUCT_BITWISE_XOR, o);
			break;
		}
		case OPERATION_EQUALS: {}
		case OPERATION_NOT_EQUALS: {}
		case OPERATION_GREATER: {}
		case OPERATION_LESS: {}
		case OPERATION_EQUALS_GREATER: {}
		case OPERATION_EQUALS_LESS: {
			if(append && m_type == STRUCT_COMPARISON) {
				MathStructure *o2 = new MathStructure(CHILD(1));
				o2->add(o, op);
				transform_nocopy(STRUCT_LOGICAL_AND, o2);
			} else if(append && m_type == STRUCT_LOGICAL_AND && LAST.type() == STRUCT_COMPARISON) {
				MathStructure *o2 = new MathStructure(LAST[1]);
				o2->add(o, op);
				APPEND_POINTER(o2);
			} else {
				transform(STRUCT_COMPARISON, o);
				switch(op) {
					case OPERATION_EQUALS: {ct_comp = COMPARISON_EQUALS; break;}
					case OPERATION_NOT_EQUALS: {ct_comp = COMPARISON_NOT_EQUALS; break;}
					case OPERATION_GREATER: {ct_comp = COMPARISON_GREATER; break;}
					case OPERATION_LESS: {ct_comp = COMPARISON_LESS; break;}
					case OPERATION_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
					case OPERATION_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_LESS; break;}
					default: {}
				}
			}
			break;
		}
		default: {
		}
	}
}
void MathStructure::add(const MathStructure &o, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND(o);
	} else {
		transform(STRUCT_ADDITION, o);
	}
}
void MathStructure::subtract(const MathStructure &o, bool append) {
	MathStructure *o2 = new MathStructure(o);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(const MathStructure &o, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND(o);
	} else {
		transform(STRUCT_MULTIPLICATION, o);
	}
}
void MathStructure::divide(const MathStructure &o, bool append) {
//	transform(STRUCT_DIVISION, o);
	MathStructure *o2 = new MathStructure(o);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(const MathStructure &o) {
	transform(STRUCT_POWER, o);
}
void MathStructure::add(const Number &o, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(o);
	} else {
		transform(STRUCT_ADDITION, o);
	}
}
void MathStructure::subtract(const Number &o, bool append) {
	MathStructure *o2 = new MathStructure(o);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(const Number &o, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(o);
	} else {
		transform(STRUCT_MULTIPLICATION, o);
	}
}
void MathStructure::divide(const Number &o, bool append) {
	MathStructure *o2 = new MathStructure(o);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(const Number &o) {
	transform(STRUCT_POWER, o);
}
void MathStructure::add(int i, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_POINTER(new MathStructure(i, 1, 0));
	} else {
		transform(STRUCT_ADDITION, i);
	}
}
void MathStructure::subtract(int i, bool append) {
	MathStructure *o2 = new MathStructure(i, 1, 0);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(int i, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_POINTER(new MathStructure(i, 1, 0));
	} else {
		transform(STRUCT_MULTIPLICATION, i);
	}
}
void MathStructure::divide(int i, bool append) {
	MathStructure *o2 = new MathStructure(i, 1, 0);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(int i) {
	transform(STRUCT_POWER, i);
}
void MathStructure::add(Variable *v, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(v);
	} else {
		transform(STRUCT_ADDITION, v);
	}
}
void MathStructure::subtract(Variable *v, bool append) {
	MathStructure *o2 = new MathStructure(v);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(Variable *v, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(v);
	} else {
		transform(STRUCT_MULTIPLICATION, v);
	}
}
void MathStructure::divide(Variable *v, bool append) {
	MathStructure *o2 = new MathStructure(v);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(Variable *v) {
	transform(STRUCT_POWER, v);
}
void MathStructure::add(Unit *u, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(u);
	} else {
		transform(STRUCT_ADDITION, u);
	}
}
void MathStructure::subtract(Unit *u, bool append) {
	MathStructure *o2 = new MathStructure(u);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(Unit *u, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(u);
	} else {
		transform(STRUCT_MULTIPLICATION, u);
	}
}
void MathStructure::divide(Unit *u, bool append) {
	MathStructure *o2 = new MathStructure(u);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(Unit *u) {
	transform(STRUCT_POWER, u);
}
void MathStructure::add(string sym, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(sym);
	} else {
		transform(STRUCT_ADDITION, sym);
	}
}
void MathStructure::subtract(string sym, bool append) {
	MathStructure *o2 = new MathStructure(sym);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(string sym, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(sym);
	} else {
		transform(STRUCT_MULTIPLICATION, sym);
	}
}
void MathStructure::divide(string sym, bool append) {
	MathStructure *o2 = new MathStructure(sym);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(string sym) {
	transform(STRUCT_POWER, sym);
}
void MathStructure::add_nocopy(MathStructure *o, MathOperation op, bool append) {
	switch(op) {
		case OPERATION_ADD: {
			add_nocopy(o, append);
			break;
		}
		case OPERATION_SUBTRACT: {
			subtract_nocopy(o, append);
			break;
		}
		case OPERATION_MULTIPLY: {
			multiply_nocopy(o, append);
			break;
		}
		case OPERATION_DIVIDE: {
			divide_nocopy(o, append);
			break;
		}
		case OPERATION_RAISE: {
			raise_nocopy(o);
			break;
		}
		case OPERATION_EXP10: {
			MathStructure *mstruct = new MathStructure(10, 1, 0);
			mstruct->raise_nocopy(o);
			multiply_nocopy(mstruct, append);
			break;
		}
		case OPERATION_LOGICAL_AND: {
			if(m_type == STRUCT_LOGICAL_AND && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_LOGICAL_AND, o);
			}
			break;
		}
		case OPERATION_LOGICAL_OR: {
			if(m_type == STRUCT_LOGICAL_OR && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_LOGICAL_OR, o);
			}
			break;
		}
		case OPERATION_LOGICAL_XOR: {
			transform_nocopy(STRUCT_LOGICAL_XOR, o);
			break;
		}
		case OPERATION_BITWISE_AND: {
			if(m_type == STRUCT_BITWISE_AND && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_BITWISE_AND, o);
			}
			break;
		}
		case OPERATION_BITWISE_OR: {
			if(m_type == STRUCT_BITWISE_OR && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_BITWISE_OR, o);
			}
			break;
		}
		case OPERATION_BITWISE_XOR: {
			transform_nocopy(STRUCT_BITWISE_XOR, o);
			break;
		}
		case OPERATION_EQUALS: {}
		case OPERATION_NOT_EQUALS: {}
		case OPERATION_GREATER: {}
		case OPERATION_LESS: {}
		case OPERATION_EQUALS_GREATER: {}
		case OPERATION_EQUALS_LESS: {
			if(append && m_type == STRUCT_COMPARISON) {
				MathStructure *o2 = new MathStructure(CHILD(1));
				o2->add_nocopy(o, op);
				transform_nocopy(STRUCT_LOGICAL_AND, o2);
			} else if(append && m_type == STRUCT_LOGICAL_AND && LAST.type() == STRUCT_COMPARISON) {
				MathStructure *o2 = new MathStructure(LAST[1]);
				o2->add_nocopy(o, op);
				APPEND_POINTER(o2);
			} else {
				transform_nocopy(STRUCT_COMPARISON, o);
				switch(op) {
					case OPERATION_EQUALS: {ct_comp = COMPARISON_EQUALS; break;}
					case OPERATION_NOT_EQUALS: {ct_comp = COMPARISON_NOT_EQUALS; break;}
					case OPERATION_GREATER: {ct_comp = COMPARISON_GREATER; break;}
					case OPERATION_LESS: {ct_comp = COMPARISON_LESS; break;}
					case OPERATION_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
					case OPERATION_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_LESS; break;}
					default: {}
				}
			}
			break;
		}
		default: {
		}
	}
}
void MathStructure::add_nocopy(MathStructure *o, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_POINTER(o);
	} else {
		transform_nocopy(STRUCT_ADDITION, o);
	}
}
void MathStructure::subtract_nocopy(MathStructure *o, bool append) {
	o->negate();
	add_nocopy(o, append);
}
void MathStructure::multiply_nocopy(MathStructure *o, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_POINTER(o);
	} else {
		transform_nocopy(STRUCT_MULTIPLICATION, o);
	}
}
void MathStructure::divide_nocopy(MathStructure *o, bool append) {
	o->inverse();
	multiply_nocopy(o, append);
}
void MathStructure::raise_nocopy(MathStructure *o) {
	transform_nocopy(STRUCT_POWER, o);
}
void MathStructure::negate() {
	//transform(STRUCT_NEGATE);
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = STRUCT_MULTIPLICATION;
	APPEND(m_minus_one);
	APPEND_POINTER(struct_this);
}
void MathStructure::inverse() {
	//transform(STRUCT_INVERSE);
	raise(m_minus_one);
}
void MathStructure::setLogicalNot() {
	transform(STRUCT_LOGICAL_NOT);
}
void MathStructure::setBitwiseNot() {
	transform(STRUCT_BITWISE_NOT);
}

bool MathStructure::equals(const MathStructure &o, bool allow_interval, bool allow_infinite) const {
	if(m_type != o.type()) return false;
	if(SIZE != o.size()) return false;
	switch(m_type) {
		case STRUCT_UNDEFINED: {return true;}
		case STRUCT_SYMBOLIC: {return s_sym == o.symbol();}
		case STRUCT_DATETIME: {return *o_datetime == *o.datetime();}
		case STRUCT_NUMBER: {return o_number.equals(o.number(), allow_interval, allow_infinite);}
		case STRUCT_VARIABLE: {return o_variable == o.variable();}
		case STRUCT_UNIT: {
			Prefix *p1 = (o_prefix == NULL || o_prefix == CALCULATOR->getDecimalNullPrefix() || o_prefix == CALCULATOR->getBinaryNullPrefix()) ? NULL : o_prefix;
			Prefix *p2 = (o.prefix() == NULL || o.prefix() == CALCULATOR->getDecimalNullPrefix() || o.prefix() == CALCULATOR->getBinaryNullPrefix()) ? NULL : o.prefix();
			return o_unit == o.unit() && p1 == p2;
		}
		case STRUCT_COMPARISON: {if(ct_comp != o.comparisonType()) return false; break;}
		case STRUCT_FUNCTION: {if(o_function != o.function() || o_function->args() == 0) return false; break;}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_LOGICAL_AND: {
			if(SIZE < 1) return false;
			if(SIZE == 2) {
				return (CHILD(0).equals(o[0], allow_interval, allow_infinite) && CHILD(1).equals(o[1], allow_interval, allow_infinite)) || (CHILD(0).equals(o[1], allow_interval, allow_infinite) && CHILD(1).equals(o[0], allow_interval, allow_infinite));
			}
			vector<size_t> i2taken;
			for(size_t i = 0; i < SIZE; i++) {
				bool b = false, b2 = false;
				for(size_t i2 = 0; i2 < o.size(); i2++) {
					if(CHILD(i).equals(o[i2], allow_interval, allow_infinite)) {
						b2 = true;
						for(size_t i3 = 0; i3 < i2taken.size(); i3++) {
							if(i2taken[i3] == i2) {
								b2 = false;
							}
						}
						if(b2) {
							b = true;
							i2taken.push_back(i2);
							break;
						}
					}
				}
				if(!b) return false;
			}
			return true;
		}
		default: {}
	}
	if(SIZE < 1) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(!CHILD(i).equals(o[i], allow_interval, allow_infinite)) return false;
	}
	return true;
}
bool MathStructure::equals(const Number &o, bool allow_interval, bool allow_infinite) const {
	if(m_type != STRUCT_NUMBER) return false;
	return o_number.equals(o, allow_interval, allow_infinite);
}
bool MathStructure::equals(int i) const {
	if(m_type != STRUCT_NUMBER) return false;
	return o_number.equals((long int) i);
}
bool MathStructure::equals(Unit *u) const {
	if(m_type != STRUCT_UNIT) return false;
	return o_unit == u;
}
bool MathStructure::equals(Variable *v) const {
	if(m_type != STRUCT_VARIABLE) return false;
	return o_variable == v;
}
bool MathStructure::equals(string sym) const {
	if(m_type != STRUCT_SYMBOLIC) return false;
	return s_sym == sym;
}

bool remove_rad_unit_cf(MathStructure &m) {
	if(m.isFunction() && m.containsType(STRUCT_UNIT, false, true, true) <= 0) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(!remove_rad_unit_cf(m[i])) return false;
	}
	return true;
}
bool remove_rad_unit(MathStructure &m, const EvaluationOptions &eo, bool top) {
	if(top && !remove_rad_unit_cf(m)) return false;
	if(m.isUnit()) {
		if(m.unit() == CALCULATOR->getRadUnit()) {
			m.set(1, 1, 0, true);
			return true;
		} else if(m.unit()->containsRelativeTo(CALCULATOR->getRadUnit())) {
			if(m.convert(CALCULATOR->getRadUnit())) {
				m.calculatesub(eo, eo, true);
				return remove_rad_unit(m, eo, false);
			}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < m.size(); i++) {
			if(remove_rad_unit(m[i], eo, false)) b = true;
		}
		if(b) {
			m.calculatesub(eo, eo, false);
			return true;
		}
	}
	return false;
}

bool contains_unknown_possibly_with_unit(MathStructure &m) {
	if(m.isUnknown()) return m.containsRepresentativeOfType(STRUCT_UNIT, true, true) != 0;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_unknown_possibly_with_unit(m[i])) return true;
	}
	return false;
}
int compare_check_incompability(MathStructure *mtest) {
	if(contains_unknown_possibly_with_unit(*mtest)) return -1;
	int incomp = 0;
	int unit_term_count = 0;
	int not_unit_term_count = 0;
	int compat_count = 0;
	bool b_not_number = false;
	for(size_t i = 0; i < mtest->size(); i++) {
		if((*mtest)[i].containsType(STRUCT_UNIT, false, true, true) > 0) {
			unit_term_count++;
			for(size_t i2 = i + 1; i2 < mtest->size(); i2++) {
				int b_test = (*mtest)[i].isUnitCompatible((*mtest)[i2]);
				if(b_test == 0) {
					incomp = 1;
				} else if(b_test > 0) {
					compat_count++;
				}
			}
			if(!b_not_number && !(*mtest)[i].representsNumber(true)) {
				b_not_number = true;
			}
		} else if((*mtest)[i].containsRepresentativeOfType(STRUCT_UNIT, true, true) == 0) {
			not_unit_term_count++;
		} else if(!b_not_number && !(*mtest)[i].representsNumber(true)) {
			b_not_number = true;
		}
	}
	if(b_not_number && unit_term_count > 0) {
		incomp = -1;
	} else if(unit_term_count > 0) {
		if((long int) mtest->size() - (unit_term_count + not_unit_term_count) >= unit_term_count - compat_count + (not_unit_term_count > 0)) {
			incomp = -1;
		} else if(not_unit_term_count > 0) {
			incomp = 1;
		}
	}
	return incomp;
}

bool replace_interval_unknowns(MathStructure &m, bool do_assumptions) {
	if(m.isVariable() && !m.variable()->isKnown()) {
		if(!((UnknownVariable*) m.variable())->interval().isUndefined()) {
			m = ((UnknownVariable*) m.variable())->interval();
			replace_interval_unknowns(m, do_assumptions);
			return true;
		} else if(do_assumptions) {
			Assumptions *ass = ((UnknownVariable*) m.variable())->assumptions();
			if(ass && ((ass->sign() != ASSUMPTION_SIGN_UNKNOWN && ass->sign() != ASSUMPTION_SIGN_NONZERO) || ass->min() || ass->max())) {
				Number nr_intval;
				if(ass->min()) {
					if(ass->max()) nr_intval.setInterval(*ass->min(), *ass->max());
					else nr_intval.setInterval(*ass->min(), nr_plus_inf);
				} else if(ass->max()) {
					nr_intval.setInterval(nr_minus_inf, *ass->max());
				} else if(ass->sign() == ASSUMPTION_SIGN_NEGATIVE || ass->sign() == ASSUMPTION_SIGN_NONPOSITIVE) {
					nr_intval.setInterval(nr_minus_inf, nr_zero);
				} else {
					nr_intval.setInterval(nr_zero, nr_plus_inf);
				}
				m = nr_intval;
				return true;
			}
		}
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_interval_unknowns(m[i], do_assumptions)) b = true;
	}
	return b;
}
int contains_ass_intval(const MathStructure &m) {
	if(m.isVariable() && !m.variable()->isKnown()) {
		Assumptions *ass = ((UnknownVariable*) m.variable())->assumptions();
		if(ass && (ass->min() || ass->max())) return 1;
		return 0;
	}
	int b = 0;
	for(size_t i = 0; i < m.size(); i++) {
		int i2 = contains_ass_intval(m[i]);
		if(i2 == 2 || (i2 == 1 && m.isFunction())) return 2;
		if(i2 == 1) b  = 1;
	}
	return b;
}


bool MathStructure::mergeInterval(const MathStructure &o, bool set_to_overlap) {
	if(isNumber() && o.isNumber()) {
		return o_number.mergeInterval(o.number(), set_to_overlap);
	}
	if(equals(o)) return true;
	if(isMultiplication() && SIZE > 1 && CHILD(0).isNumber()) {
		if(o.isMultiplication() && o.size() > 1) {
			if(SIZE == o.size() + (o[0].isNumber() ? 0 : 1)) {
				bool b = true;
				for(size_t i = 1; i < SIZE; i++) {
					if(!CHILD(i).equals(o[0].isNumber() ? o[i] : o[i - 1]) || !CHILD(i).representsNonNegative(true)) {
						b = false;
						break;
					}
				}
				if(b && o[0].isNumber()) {
					if(!CHILD(0).number().mergeInterval(o[0].number(), set_to_overlap)) return false;
				} else if(b) {
					if(!CHILD(0).number().mergeInterval(nr_one, set_to_overlap)) return false;
				}
				CHILD(0).numberUpdated();
				CHILD_UPDATED(0);
				return true;
			}
		} else if(SIZE == 2 && o.equals(CHILD(1)) && o.representsNonNegative(true)) {
			if(!CHILD(0).number().mergeInterval(nr_one, set_to_overlap)) return false;
			CHILD(0).numberUpdated();
			CHILD_UPDATED(0);
			return true;
		}
	} else if(o.isMultiplication() && o.size() == 2 && o[0].isNumber() && equals(o[1]) && representsNonNegative(true)) {
		Number nr(1, 1);
		if(!nr.mergeInterval(o[0].number(), set_to_overlap)) return false;
		transform(STRUCT_MULTIPLICATION);
		PREPEND(nr);
		return true;
	}
	return false;
}

ComparisonResult MathStructure::compare(const MathStructure &o) const {
	if(isNumber() && o.isNumber()) {
		return o_number.compare(o.number());
	}
	if(isDateTime() && o.isDateTime()) {
		if(*o_datetime == *o.datetime()) return COMPARISON_RESULT_EQUAL;
		if(*o_datetime > *o.datetime()) return COMPARISON_RESULT_LESS;
		return COMPARISON_RESULT_GREATER;
	}
	if(equals(o)) return COMPARISON_RESULT_EQUAL;
	if(o.isZero()) {
		if(representsPositive(true)) return COMPARISON_RESULT_LESS;
		if(representsNegative(true)) return COMPARISON_RESULT_GREATER;
	}
	if(isMultiplication() && SIZE > 1 && CHILD(0).isNumber()) {
		if(o.isMultiplication() && o.size() > 1) {
			if(SIZE == o.size() + (o[0].isNumber() ? 0 : 1)) {
				bool b = true;
				for(size_t i = 1; i < SIZE; i++) {
					if(!CHILD(i).equals(o[0].isNumber() ? o[i] : o[i - 1]) || !CHILD(i).representsPositive(true)) {
						b = false;
						break;
					}
				}
				if(b && o[0].isNumber()) return CHILD(0).number().compare(o[0].number());
				else if(b) return CHILD(0).number().compare(nr_one);
			}
		} else if(SIZE == 2 && o.equals(CHILD(1)) && o.representsPositive(true)) {
			return CHILD(0).number().compare(nr_one);
		}
	} else if(o.isMultiplication() && o.size() == 2 && o[0].isNumber() && equals(o[1]) && representsPositive(true)) {
		return nr_one.compare(o[0].number());
	}
	if(o.representsReal(true) && representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	if(representsReal(true) && o.representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	MathStructure mtest(*this);
	if(!o.isZero()) mtest -= o;
	EvaluationOptions eo = default_evaluation_options;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	eo.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
	eo.structuring = STRUCTURING_NONE;
	for(int i = 0; i < 2; i++) {
		int b_ass = (i == 0 ? 2 : contains_ass_intval(mtest));
		if(b_ass == 0) break;
		CALCULATOR->beginTemporaryEnableIntervalArithmetic();
		if(CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->beginTemporaryStopMessages();
			replace_interval_unknowns(mtest, i > 0);
			if(b_ass == 2) mtest.calculateFunctions(eo);
			mtest.calculatesub(eo, eo);
			remove_rad_unit(mtest, eo);
			CALCULATOR->endTemporaryStopMessages();
		}
		CALCULATOR->endTemporaryEnableIntervalArithmetic();
		int incomp = 0;
		if(mtest.isAddition()) {
			incomp = compare_check_incompability(&mtest);
		}
		if(incomp > 0) return COMPARISON_RESULT_NOT_EQUAL;
		if(incomp == 0) {
			if(mtest.representsZero(true)) return COMPARISON_RESULT_EQUAL;
			if(mtest.representsPositive(true)) return COMPARISON_RESULT_LESS;
			if(mtest.representsNegative(true)) return COMPARISON_RESULT_GREATER;
			if(mtest.representsNonZero(true)) return COMPARISON_RESULT_NOT_EQUAL;
			if(mtest.representsNonPositive(true)) return COMPARISON_RESULT_EQUAL_OR_GREATER;
			if(mtest.representsNonNegative(true)) return COMPARISON_RESULT_EQUAL_OR_LESS;
		} else if(i == 0) {
			bool a_pos = representsPositive(true);
			bool a_nneg = a_pos || representsNonNegative(true);
			bool a_neg = !a_nneg && representsNegative(true);
			bool a_npos = !a_pos && (a_neg || representsNonPositive(true));
			bool b_pos = o.representsPositive(true);
			bool b_nneg = b_pos || o.representsNonNegative(true);
			bool b_neg = !b_nneg && o.representsNegative(true);
			bool b_npos = !b_pos && (b_neg || o.representsNonPositive(true));
			if(a_pos && b_npos) return COMPARISON_RESULT_NOT_EQUAL;
			if(a_npos && b_pos) return COMPARISON_RESULT_NOT_EQUAL;
			if(a_nneg && b_neg) return COMPARISON_RESULT_NOT_EQUAL;
			if(a_neg && b_nneg) return COMPARISON_RESULT_NOT_EQUAL;
			break;
		}
	}
	return COMPARISON_RESULT_UNKNOWN;
}

ComparisonResult MathStructure::compareApproximately(const MathStructure &o, const EvaluationOptions &eo2) const {
	if(isNumber() && o.isNumber()) {
		return o_number.compareApproximately(o.number());
	}
	if(isDateTime() && o.isDateTime()) {
		if(*o_datetime == *o.datetime()) return COMPARISON_RESULT_EQUAL;
		if(*o_datetime < *o.datetime()) return COMPARISON_RESULT_LESS;
		return COMPARISON_RESULT_GREATER;
	}
	if(equals(o)) return COMPARISON_RESULT_EQUAL;
	if(isMultiplication() && SIZE > 1 && CHILD(0).isNumber()) {
		if(o.isMultiplication() && o.size() > 1) {
			if(SIZE == o.size() + (o[0].isNumber() ? 0 : 1)) {
				bool b = true;
				for(size_t i = 1; i < SIZE; i++) {
					if(!CHILD(i).equals(o[0].isNumber() ? o[i] : o[i - 1]) || !CHILD(i).representsPositive(true)) {
						b = false;
						break;
					}
				}
				if(b && o[0].isNumber()) return CHILD(0).number().compareApproximately(o[0].number());
				else if(b) return CHILD(0).number().compareApproximately(nr_one);
			}
		} else if(SIZE == 2 && o.equals(CHILD(1)) && o.representsPositive(true)) {
			return CHILD(0).number().compareApproximately(nr_one);
		}
	} else if(o.isMultiplication() && o.size() == 2 && o[0].isNumber() && equals(o[1]) && representsPositive(true)) {
		return nr_one.compareApproximately(o[0].number());
	}
	if(o.representsZero(true) && representsZero(true)) return COMPARISON_RESULT_EQUAL;
	if(o.representsReal(true) && representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	if(representsReal(true) && o.representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	EvaluationOptions eo = eo2;
	eo.expand = true;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	eo.structuring = STRUCTURING_NONE;
	for(int i = 0; i < 2; i++) {
		int b_ass1 = (i == 0 ? 2 : contains_ass_intval(*this));
		int b_ass2 = (i == 0 ? 2 : contains_ass_intval(o));
		if(b_ass1 == 0 && b_ass2 == 0) break;
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mtest(*this), mtest2(o);
		if(b_ass1 > 0) replace_interval_unknowns(mtest, i > 0);
		if(b_ass2 > 0) replace_interval_unknowns(mtest2, i > 0);
		if(b_ass1 == 2) mtest.calculateFunctions(eo);
		if(b_ass2 == 2) mtest2.calculateFunctions(eo);
		if(b_ass1 > 0) mtest.calculatesub(eo, eo);
		if(b_ass2 > 0) mtest2.calculatesub(eo, eo);
		remove_rad_unit(mtest, eo);
		remove_rad_unit(mtest2, eo);
		CALCULATOR->endTemporaryStopMessages();
		if(mtest.equals(mtest2)) return COMPARISON_RESULT_EQUAL;
		if(mtest.representsZero(true) && mtest2.representsZero(true)) return COMPARISON_RESULT_EQUAL;
		if(mtest.isNumber() && mtest2.isNumber()) {
			if(mtest2.isApproximate() && o.isAddition() && o.size() > 1 && mtest.isZero() && !mtest2.isZero()) {
				CALCULATOR->beginTemporaryStopMessages();
				mtest = *this;
				mtest.subtract(o[0]);
				mtest2 = o;
				mtest2.delChild(1, true);
				replace_interval_unknowns(mtest, i > 0);
				replace_interval_unknowns(mtest2, i > 0);
				mtest.calculateFunctions(eo);
				mtest2.calculateFunctions(eo);
				mtest.calculatesub(eo, eo);
				mtest2.calculatesub(eo, eo);
				remove_rad_unit(mtest, eo);
				remove_rad_unit(mtest2, eo);
				CALCULATOR->endTemporaryStopMessages();
				if(mtest.isNumber() && mtest2.isNumber()) return mtest.number().compareApproximately(mtest2.number());
			} else if(mtest.isApproximate() && isAddition() && SIZE > 1 && mtest2.isZero() && !mtest.isZero()) {
				CALCULATOR->beginTemporaryStopMessages();
				mtest2 = o;
				mtest2.subtract(CHILD(0));
				mtest = *this;
				mtest.delChild(1, true);
				replace_interval_unknowns(mtest, i > 0);
				replace_interval_unknowns(mtest2, i > 0);
				mtest.calculateFunctions(eo);
				mtest2.calculateFunctions(eo);
				mtest.calculatesub(eo, eo);
				mtest2.calculatesub(eo, eo);
				remove_rad_unit(mtest, eo);
				remove_rad_unit(mtest2, eo);
				CALCULATOR->endTemporaryStopMessages();
				if(mtest.isNumber() && mtest2.isNumber()) return mtest.number().compareApproximately(mtest2.number());
			} else {
				return mtest.number().compareApproximately(mtest2.number());
			}
		}
		if(mtest2.isZero() && mtest.isMultiplication() && mtest.size() > 0 && mtest[0].isNumber()) {
			bool b = true;
			for(size_t i = 1; i < SIZE; i++) {
				if(!CHILD(i).representsNonZero(true)) {
					b = false;
					break;
				}
			}
			if(b) return CHILD(0).number().compareApproximately(o.number());
		}
		if(!mtest2.isZero()) {
			CALCULATOR->beginTemporaryStopMessages();
			mtest.calculateSubtract(mtest2, eo);
			CALCULATOR->endTemporaryStopMessages();
		}
		if(mtest.representsZero(true)) return COMPARISON_RESULT_EQUAL;
		int incomp = 0;
		if(mtest.isAddition()) {
			incomp = compare_check_incompability(&mtest);
		}
		if(incomp > 0) return COMPARISON_RESULT_NOT_EQUAL;
		if(incomp == 0) {
			if(mtest.isZero()) return COMPARISON_RESULT_EQUAL;
			if(mtest.representsPositive(true)) return COMPARISON_RESULT_LESS;
			if(mtest.representsNegative(true)) return COMPARISON_RESULT_GREATER;
			if(mtest.representsNonZero(true)) return COMPARISON_RESULT_NOT_EQUAL;
			if(mtest.representsNonPositive(true)) return COMPARISON_RESULT_EQUAL_OR_GREATER;
			if(mtest.representsNonNegative(true)) return COMPARISON_RESULT_EQUAL_OR_LESS;
		} else if(i == 0) {
			bool a_pos = representsPositive(true);
			bool a_nneg = a_pos || representsNonNegative(true);
			bool a_neg = !a_nneg && representsNegative(true);
			bool a_npos = !a_pos && (a_neg || representsNonPositive(true));
			bool b_pos = o.representsPositive(true);
			bool b_nneg = b_pos || o.representsNonNegative(true);
			bool b_neg = !b_nneg && o.representsNegative(true);
			bool b_npos = !b_pos && (b_neg || o.representsNonPositive(true));
			if(a_pos && b_npos) return COMPARISON_RESULT_NOT_EQUAL;
			if(a_npos && b_pos) return COMPARISON_RESULT_NOT_EQUAL;
			if(a_nneg && b_neg) return COMPARISON_RESULT_NOT_EQUAL;
			if(a_neg && b_nneg) return COMPARISON_RESULT_NOT_EQUAL;
			break;
		}
	}
	return COMPARISON_RESULT_UNKNOWN;
}

bool MathStructure::containsOpaqueContents() const {
	if(isFunction()) return true;
	if(isUnit() && o_unit->subtype() != SUBTYPE_BASE_UNIT) return true;
	if(isVariable() && o_variable->isKnown()) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsOpaqueContents()) return true;
	}
	return false;
}
bool MathStructure::containsAdditionPower() const {
	if(m_type == STRUCT_POWER && CHILD(0).isAddition()) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsAdditionPower()) return true;
	}
	return false;
}

size_t MathStructure::countTotalChildren(bool count_function_as_one) const {
	if((m_type == STRUCT_FUNCTION && count_function_as_one) || SIZE == 0) return 1;
	size_t count = 0;
	for(size_t i = 0; i < SIZE; i++) {
		count += CHILD(i).countTotalChildren(count_function_as_one) + 1;
	}
	return count;
}

void MathStructure::swapChildren(size_t index1, size_t index2) {
	if(index1 > 0 && index2 > 0 && index1 <= SIZE && index2 <= SIZE) {
		SWAP_CHILDREN(index1 - 1, index2 - 1)
	}
}
void MathStructure::childToFront(size_t index) {
	if(index > 0 && index <= SIZE) {
		CHILD_TO_FRONT(index - 1)
	}
}
void MathStructure::addChild(const MathStructure &o) {
	APPEND(o);
}
void MathStructure::addChild_nocopy(MathStructure *o) {
	APPEND_POINTER(o);
}
void MathStructure::delChild(size_t index, bool check_size) {
	if(index > 0 && index <= SIZE) {
		ERASE(index - 1);
		if(check_size) {
			if(SIZE == 1) setToChild(1, true);
			else if(SIZE == 0) clear(true);
		}
	}
}
void MathStructure::insertChild(const MathStructure &o, size_t index) {
	if(index > 0 && index <= v_subs.size()) {
		v_order.insert(v_order.begin() + (index - 1), v_subs.size());
		v_subs.push_back(new MathStructure(o));
		CHILD_UPDATED(index - 1);
	} else {
		addChild(o);
	}
}
void MathStructure::insertChild_nocopy(MathStructure *o, size_t index) {
	if(index > 0 && index <= v_subs.size()) {
		v_order.insert(v_order.begin() + (index - 1), v_subs.size());
		v_subs.push_back(o);
		CHILD_UPDATED(index - 1);
	} else {
		addChild_nocopy(o);
	}
}
void MathStructure::setChild(const MathStructure &o, size_t index, bool merge_precision) {
	if(index > 0 && index <= SIZE) {
		CHILD(index - 1).set(o, merge_precision);
		CHILD_UPDATED(index - 1);
	}
}
void MathStructure::setChild_nocopy(MathStructure *o, size_t index, bool merge_precision) {
	if(index > 0 && index <= SIZE) {
		MathStructure *o_prev = v_subs[v_order[index - 1]];
		if(merge_precision) {
			if(!o->isApproximate() && o_prev->isApproximate()) o->setApproximate(true);
			if(o_prev->precision() >= 0 && (o->precision() < 0 || o_prev->precision() < o->precision())) o->setPrecision(o_prev->precision());
		}
		o_prev->unref();
		v_subs[v_order[index - 1]] = o;
		CHILD_UPDATED(index - 1);
	}
}
const MathStructure *MathStructure::getChild(size_t index) const {
	if(index > 0 && index <= v_order.size()) {
		return &CHILD(index - 1);
	}
	return NULL;
}
MathStructure *MathStructure::getChild(size_t index) {
	if(index > 0 && index <= v_order.size()) {
		return &CHILD(index - 1);
	}
	return NULL;
}
size_t MathStructure::countChildren() const {
	return SIZE;
}
size_t MathStructure::size() const {
	return SIZE;
}
const MathStructure *MathStructure::base() const {
	if(m_type == STRUCT_POWER && SIZE >= 1) {
		return &CHILD(0);
	}
	return NULL;
}
const MathStructure *MathStructure::exponent() const {
	if(m_type == STRUCT_POWER && SIZE >= 2) {
		return &CHILD(1);
	}
	return NULL;
}
MathStructure *MathStructure::base() {
	if(m_type == STRUCT_POWER && SIZE >= 1) {
		return &CHILD(0);
	}
	return NULL;
}
MathStructure *MathStructure::exponent() {
	if(m_type == STRUCT_POWER && SIZE >= 2) {
		return &CHILD(1);
	}
	return NULL;
}

StructureType MathStructure::type() const {
	return m_type;
}

int contains_angle_unit(const MathStructure &m, const ParseOptions &po, int check_functions) {
	if(m.isUnit() && m.unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit() && m.unit()->baseExponent() == 1) return 1;
	if(m.isVariable() && m.variable()->isKnown()) return contains_angle_unit(((KnownVariable*) m.variable())->get(), po, check_functions);
	if(m.isFunction()) {
		if(m.function()->id() == FUNCTION_ID_ASIN || m.function()->id() == FUNCTION_ID_ACOS || m.function()->id() == FUNCTION_ID_ATAN || m.function()->id() == FUNCTION_ID_ATAN2 || m.function()->id() == FUNCTION_ID_ARG) {
			if(NO_DEFAULT_ANGLE_UNIT(po.angle_unit)) return 1;
			return 0;
		}
		if(!check_functions) return 0;
		if(m.containsType(STRUCT_UNIT, false, true, true) == 0) return 0;
		if(check_functions > 1 && m.size() == 0) return -1;
	}
	int ret = 0;
	for(size_t i = 0; i < m.size(); i++) {
		if(!m.isFunction() || !m.function()->getArgumentDefinition(i + 1) || m.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
			int ret_i = contains_angle_unit(m[i], po, check_functions);
			if(ret_i != 0) ret = ret_i;
			if(ret_i > 0) break;
		}
	}
	return ret;
}

int MathStructure::contains(const MathStructure &mstruct, bool structural_only, bool check_variables, bool check_functions, bool loose_equals) const {
	if(mstruct.isUnit() && mstruct.prefix() == NULL && m_type == STRUCT_UNIT) return mstruct.unit() == o_unit;
	if(equals(mstruct, loose_equals, loose_equals)) return 1;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).contains(mstruct, structural_only, check_variables, check_functions, loose_equals)) return 1;
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().contains(mstruct, structural_only, check_variables, check_functions, loose_equals);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->contains(mstruct, structural_only, check_variables, check_functions, loose_equals);
			}
		}
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).contains(mstruct, structural_only, check_variables, check_functions, loose_equals);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().contains(mstruct, structural_only, check_variables, check_functions, loose_equals);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->contains(mstruct, structural_only, check_variables, check_functions, loose_equals);
			}
			return -1;
		} else if(isAborted()) {
			return -1;
		}
		return ret;
	}
	return 0;
}
size_t MathStructure::countOccurrences(const MathStructure &mstruct) const {
	return countOccurrences(mstruct, false);
}
size_t MathStructure::countOccurrences(const MathStructure &mstruct, bool check_variables) const {
	if(mstruct.isUnit() && mstruct.prefix() == NULL && m_type == STRUCT_UNIT && mstruct.unit() == o_unit) return 1;
	if(equals(mstruct, true, true)) return 1;
	if(check_variables && m_type == STRUCT_VARIABLE && o_variable->isKnown()) return ((KnownVariable*) o_variable)->get().countOccurrences(mstruct, true);
	size_t i_occ = 0;
	for(size_t i = 0; i < SIZE; i++) {
		i_occ += CHILD(i).countOccurrences(mstruct, check_variables);
	}
	return i_occ;
}
int MathStructure::containsFunction(MathFunction *f, bool structural_only, bool check_variables, bool check_functions) const {
	if(m_type == STRUCT_FUNCTION && o_function == f) return 1;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsFunction(f, structural_only, check_variables, check_functions)) return 1;
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsFunction(f, structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsFunction(f, structural_only, check_variables, check_functions);
			}
		}
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsFunction(f, structural_only, check_variables, check_functions);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsFunction(f, structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsFunction(f, structural_only, check_variables, check_functions);
			}
			return -1;
		} else if(isAborted()) {
			return -1;
		}
		return ret;
	}
	return 0;
}
int MathStructure::containsFunctionId(int id, bool structural_only, bool check_variables, bool check_functions) const {
	if(m_type == STRUCT_FUNCTION && o_function->id() == id) return 1;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsFunctionId(id, structural_only, check_variables, check_functions)) return 1;
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsFunctionId(id, structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsFunctionId(id, structural_only, check_variables, check_functions);
			}
		}
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsFunctionId(id, structural_only, check_variables, check_functions);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsFunctionId(id, structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsFunctionId(id, structural_only, check_variables, check_functions);
			}
			return -1;
		} else if(isAborted()) {
			return -1;
		}
		return ret;
	}
	return 0;
}
int contains_interval_var(const MathStructure &m, bool structural_only, bool check_variables, bool check_functions, int ignore_high_precision_interval, bool include_interval_function) {
	if(m.type() == STRUCT_NUMBER) {
		if(m.number().isInterval(false)) {
			if(ignore_high_precision_interval != 0) {
				if(m.number().precision(true) > (ignore_high_precision_interval < 0 ? (ignore_high_precision_interval == -1 ? PRECISION + 29 : PRECISION - ignore_high_precision_interval) : PRECISION + (10 * ignore_high_precision_interval))) return 0;
			}
			return 1;
		} else if(CALCULATOR->usesIntervalArithmetic() && m.number().precision() >= 0) {
			if(ignore_high_precision_interval != 0) {
				if(m.number().precision() > (ignore_high_precision_interval < 0 ? (ignore_high_precision_interval == -1 ? PRECISION + 29 : PRECISION - ignore_high_precision_interval) : PRECISION + (10 * ignore_high_precision_interval))) return 0;
			}
			return 1;
		}
	}
	if(m.type() == STRUCT_FUNCTION && (m.function()->id() == FUNCTION_ID_INTERVAL || m.function()->id() == FUNCTION_ID_UNCERTAINTY)) return include_interval_function;
	if(structural_only) {
		for(size_t i = 0; i < m.size(); i++) {
			if(contains_interval_var(m[i], structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function)) return 1;
		}
		if(m.type() == STRUCT_VARIABLE && check_variables && m.variable()->isKnown()) {
			return contains_interval_var(((KnownVariable*) m.variable())->get(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
		} else if(m.type() == STRUCT_FUNCTION && check_functions) {
			if(m.functionValue()) {
				return contains_interval_var(*m.functionValue(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
			}
		}
	} else {
		int ret = 0;
		if(m.type() != STRUCT_FUNCTION) {
			for(size_t i = 0; i < m.size(); i++) {
				int retval = contains_interval_var(m[i], structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m.type() == STRUCT_VARIABLE && check_variables && m.variable()->isKnown()) {
			return contains_interval_var(((KnownVariable*) m.variable())->get(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
		} else if(m.type() == STRUCT_FUNCTION && check_functions) {
			if(m.functionValue()) {
				return contains_interval_var(*m.functionValue(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
			}
			return -1;
		} else if(m.isAborted()) {
			return -1;
		}
		return ret;
	}
	return 0;
}
int MathStructure::containsInterval(bool structural_only, bool check_variables, bool check_functions, int ignore_high_precision_interval, bool include_interval_function) const {
	if(m_type == STRUCT_NUMBER && o_number.isInterval(false)) {
		if(ignore_high_precision_interval != 0) {
			if(o_number.precision(true) > (ignore_high_precision_interval < 0 ? (ignore_high_precision_interval == -1 ? PRECISION + 29 : PRECISION - ignore_high_precision_interval) : PRECISION + (10 * ignore_high_precision_interval))) return 0;
		}
		return 1;
	}
	if(m_type == STRUCT_FUNCTION && (o_function->id() == FUNCTION_ID_INTERVAL || o_function->id() == FUNCTION_ID_UNCERTAINTY)) return include_interval_function;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsInterval(structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function)) return 1;
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			if(ignore_high_precision_interval == -1 && o_variable->isBuiltin()) {
				return 0;
			}
			return contains_interval_var(((KnownVariable*) o_variable)->get(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsInterval(structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
			}
		}
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsInterval(structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			if(ignore_high_precision_interval == -1 && o_variable->isBuiltin()) {
				return 0;
			}
			return contains_interval_var(((KnownVariable*) o_variable)->get(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsInterval(structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
			}
			return -1;
		} else if(isAborted()) {
			return -1;
		}
		return ret;
	}
	return 0;
}
int MathStructure::containsInfinity(bool structural_only, bool check_variables, bool check_functions) const {
	if(m_type == STRUCT_NUMBER && o_number.includesInfinity(false)) {
		return 1;
	}
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsInfinity(structural_only, check_variables, check_functions)) return 1;
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsInfinity(structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsInfinity(structural_only, check_variables, check_functions);
			}
		}
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsInfinity(structural_only, check_variables, check_functions);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsInfinity(structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->containsInfinity(structural_only, check_variables, check_functions);
			}
			if(representsFinite(true)) return 0;
			return -1;
		} else if(isAborted()) {
			return -1;
		}
		return ret;
	}
	return 0;
}

int MathStructure::containsRepresentativeOf(const MathStructure &mstruct, bool check_variables, bool check_functions) const {
	if(equals(mstruct)) return 1;
	int ret = 0;
	if(m_type != STRUCT_FUNCTION) {
		for(size_t i = 0; i < SIZE; i++) {
			int retval = CHILD(i).containsRepresentativeOf(mstruct, check_variables, check_functions);
			if(retval == 1) return 1;
			else if(retval < 0) ret = retval;
		}
	}
	if(m_type == STRUCT_VARIABLE && check_variables) {
		if(o_variable->isKnown()) return ((KnownVariable*) o_variable)->get().containsRepresentativeOf(mstruct, check_variables, check_functions);
		else return ((UnknownVariable*) o_variable)->interval().containsRepresentativeOf(mstruct, check_variables, check_functions);
	} else if(m_type == STRUCT_FUNCTION && check_functions) {
		if(function_value) {
			return function_value->containsRepresentativeOf(mstruct, check_variables, check_functions);
		}
		if(!mstruct.isNumber() && (o_function->isBuiltin() || representsNumber())) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsRepresentativeOf(mstruct, check_variables, check_functions);
				if(retval != 0) return -1;
			}
			return 0;
		}
		return -1;
	} else if(isAborted()) {
		return -1;
	}
	return ret;
}

int MathStructure::containsType(StructureType mtype, bool structural_only, bool check_variables, bool check_functions) const {
	if(m_type == mtype) return 1;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsType(mtype, true, check_variables, check_functions)) return 1;
		}
		if(check_variables && m_type == STRUCT_VARIABLE && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsType(mtype, false, check_variables, check_functions);
		} else if(check_functions && m_type == STRUCT_FUNCTION) {
			if(function_value) {
				return function_value->containsType(mtype, false, check_variables, check_functions);
			}
		}
		return 0;
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsType(mtype, false, check_variables, check_functions);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(check_variables && m_type == STRUCT_VARIABLE) {
			if(o_variable->isKnown()) return ((KnownVariable*) o_variable)->get().containsType(mtype, false, check_variables, check_functions);
			else if(!((UnknownVariable*) o_variable)->interval().isUndefined()) return ((UnknownVariable*) o_variable)->interval().containsType(mtype, false, check_variables, check_functions);
			else if(mtype == STRUCT_UNIT) return -1;
		} else if(check_functions && m_type == STRUCT_FUNCTION) {
			if(function_value) {
				return function_value->containsType(mtype, false, check_variables, check_functions);
			}
			if(mtype == STRUCT_VECTOR) {
				if(o_function->id() == FUNCTION_ID_COLON || o_function->id() == FUNCTION_ID_GENERATE_VECTOR || o_function->id() == FUNCTION_ID_VERTCAT || o_function->id() == FUNCTION_ID_HORZCAT || o_function->id() == FUNCTION_ID_MATRIX || o_function->id() == FUNCTION_ID_VECTOR) return 1;
			} else if(mtype == STRUCT_UNIT) {
				if(o_function->id() == FUNCTION_ID_STRIP_UNITS) return 0;
				if(o_function->subtype() == SUBTYPE_USER_FUNCTION || o_function->subtype() == SUBTYPE_DATA_SET || o_function->id() == FUNCTION_ID_REGISTER || o_function->id() == FUNCTION_ID_STACK || o_function->id() == FUNCTION_ID_LOAD) return -1;
				// (NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit) && (o_function->id() == FUNCTION_ID_ASIN || o_function->id() == FUNCTION_ID_ACOS || o_function->id() == FUNCTION_ID_ATAN || o_function->id() == FUNCTION_ID_RADIANS_TO_DEFAULT_ANGLE_UNIT || o_function->id() == FUNCTION_ID_ARG || o_function->id() == FUNCTION_ID_ATAN2))
				if(o_function->id() == FUNCTION_ID_LOG || o_function->id() == FUNCTION_ID_LOGN || o_function->id() == FUNCTION_ID_GAMMA || o_function->id() == FUNCTION_ID_BETA || o_function->id() == FUNCTION_ID_FACTORIAL || o_function->id() == FUNCTION_ID_BESSELJ || o_function->id() == FUNCTION_ID_BESSELY || o_function->id() == FUNCTION_ID_ERF || o_function->id() == FUNCTION_ID_ERFI || o_function->id() == FUNCTION_ID_ERFC || o_function->id() == FUNCTION_ID_LOGINT || o_function->id() == FUNCTION_ID_POLYLOG || o_function->id() == FUNCTION_ID_EXPINT || o_function->id() == FUNCTION_ID_SININT || o_function->id() == FUNCTION_ID_COSINT || o_function->id() == FUNCTION_ID_SINHINT || o_function->id() == FUNCTION_ID_COSHINT || o_function->id() == FUNCTION_ID_FRESNEL_C || o_function->id() == FUNCTION_ID_FRESNEL_S || o_function->id() == FUNCTION_ID_SIGNUM || o_function->id() == FUNCTION_ID_HEAVISIDE || o_function->id() == FUNCTION_ID_LAMBERT_W || o_function->id() == FUNCTION_ID_SINC || o_function->id() == FUNCTION_ID_SIN || o_function->id() == FUNCTION_ID_COS || o_function->id() == FUNCTION_ID_TAN || o_function->id() == FUNCTION_ID_SINH || o_function->id() == FUNCTION_ID_COSH || o_function->id() == FUNCTION_ID_TANH || o_function->id() == FUNCTION_ID_ASINH || o_function->id() == FUNCTION_ID_ACOSH || o_function->id() == FUNCTION_ID_ATANH || o_function->id() == FUNCTION_ID_ASIN || o_function->id() == FUNCTION_ID_ACOS || o_function->id() == FUNCTION_ID_ATAN) return 0;
				int ret = 0;
				for(size_t i = 0; i < SIZE; i++) {
					int ret_i = CHILD(i).containsType(mtype, false, check_variables, check_functions);
					if(ret_i > 0) return ret_i;
					else if(ret_i < 0) ret = ret_i;
				}
				return ret;
			}
			return -1;
		} else if(isAborted()) {
			return -1;
		}
		return ret;
	}
}
int MathStructure::containsRepresentativeOfType(StructureType mtype, bool check_variables, bool check_functions) const {
	if(m_type == mtype) return 1;
	int ret = 0;
	if(m_type != STRUCT_FUNCTION) {
		for(size_t i = 0; i < SIZE; i++) {
			int retval = CHILD(i).containsRepresentativeOfType(mtype, check_variables, check_functions);
			if(retval == 1) return 1;
			else if(retval < 0) ret = retval;
		}
	}
	if(check_variables && m_type == STRUCT_VARIABLE && o_variable->isKnown()) {
		return ((KnownVariable*) o_variable)->get().containsRepresentativeOfType(mtype, check_variables, check_functions);
	} else if(check_functions && m_type == STRUCT_FUNCTION) {
		if(function_value) {
			return function_value->containsRepresentativeOfType(mtype, check_variables, check_functions);
		}
	}
	if(m_type == STRUCT_SYMBOLIC || m_type == STRUCT_VARIABLE || m_type == STRUCT_FUNCTION || m_type == STRUCT_ABORTED) {
		if(representsNumber(false)) {
			if(mtype == STRUCT_UNIT) return -1;
			return mtype == STRUCT_NUMBER;
		} else {
			return -1;
		}
	}
	return ret;
}
bool MathStructure::containsUnknowns() const {
	if(isUnknown()) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsUnknowns()) return true;
	}
	return false;
}
bool MathStructure::containsDivision() const {
	if(m_type == STRUCT_DIVISION || m_type == STRUCT_INVERSE || (m_type == STRUCT_POWER && CHILD(1).hasNegativeSign())) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsDivision()) return true;
	}
	return false;
}
size_t MathStructure::countFunctions(bool count_subfunctions) const {
	size_t c = 0;
	if(isFunction()) {
		if(!count_subfunctions) return 1;
		c = 1;
	}
	for(size_t i = 0; i < SIZE; i++) {
		c += CHILD(i).countFunctions();
	}
	return c;
}
void MathStructure::findAllUnknowns(MathStructure &unknowns_vector) {
	if(!unknowns_vector.isVector()) unknowns_vector.clearVector();
	switch(m_type) {
		case STRUCT_VARIABLE: {
			if(o_variable->isKnown()) {
				break;
			}
		}
		case STRUCT_SYMBOLIC: {
			bool b = false;
			for(size_t i = 0; i < unknowns_vector.size(); i++) {
				if(equals(unknowns_vector[i])) {
					b = true;
					break;
				}
			}
			if(!b) unknowns_vector.addChild(*this);
			break;
		}
		default: {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).findAllUnknowns(unknowns_vector);
			}
		}
	}
}
bool MathStructure::replace(const MathStructure &mfrom, const MathStructure &mto, bool once_only, bool exclude_function_arguments) {
	return replace(mfrom, mto, once_only, exclude_function_arguments, false);
}
bool MathStructure::replace(const MathStructure &mfrom, const MathStructure &mto, bool once_only, bool exclude_function_arguments, bool replace_in_variables) {
	if(b_protected) b_protected = false;
	if(equals(mfrom, true, true)) {
		set(mto);
		return true;
	}
	if(replace_in_variables && m_type == STRUCT_VARIABLE && o_variable->isKnown()) {
		if(((KnownVariable*) o_variable)->get().contains(mfrom, !exclude_function_arguments, true, false, true) > 0) {
			MathStructure m(((KnownVariable*) o_variable)->get());
			if(!m.isAborted() && m.replace(mfrom, mto, once_only, exclude_function_arguments, true)) {
				if(!o_variable->isRegistered()) {
					Variable *v = CALCULATOR->getActiveVariable(o_variable->referenceName());
					if(v->isKnown() && ((KnownVariable*) v)->get().equals(m, true, true)) {
						set(v);
						return true;
					}
				}
				KnownVariable *var = new KnownVariable("", o_variable->referenceName(), m);
				set(var);
				var->destroy();
				return true;
			}
		}
	}
	if(mfrom.size() > 0 && mfrom.type() == m_type && SIZE > mfrom.size() && (mfrom.isAddition() || mfrom.isMultiplication() || mfrom.isLogicalAnd() || mfrom.isLogicalOr())) {
		bool b = true;
		size_t i2 = 0;
		for(size_t i = 0; i < mfrom.size(); i++) {
			b = false;
			for(; i2 < SIZE; i2++) {
				if(CHILD(i2).equals(mfrom[i], true, true)) {b = true; break;}
			}
			if(!b) break;
		}
		if(b) {
			i2 = 0;
			for(size_t i = 0; i < mfrom.size(); i++) {
				for(; i2 < SIZE; i2++) {
					if(CHILD(i2).equals(mfrom[i], true, true)) {ERASE(i2); break;}
				}
			}
			if(SIZE == 1) setToChild(1);
			else if(SIZE == 0) clear();
			else if(!once_only) replace(mfrom, mto, once_only, exclude_function_arguments, replace_in_variables);
			if(mfrom.isAddition()) add(mto);
			else if(mfrom.isMultiplication()) multiply(mto);
			else if(mfrom.isLogicalAnd()) transform(STRUCT_LOGICAL_AND, mto);
			else if(mfrom.isLogicalOr()) transform(STRUCT_LOGICAL_OR, mto);
			return true;
		}
	}
	if(exclude_function_arguments && m_type == STRUCT_FUNCTION) return false;
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).replace(mfrom, mto, once_only, exclude_function_arguments, replace_in_variables)) {
			b = true;
			CHILD_UPDATED(i);
			if(once_only) return true;
		}
	}
	return b;
}
bool MathStructure::replace(Variable *v, const MathStructure &mto) {
	if(b_protected) b_protected = false;
	if(m_type == STRUCT_VARIABLE && o_variable == v) {
		set(mto);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).replace(v, mto)) {
			b = true;
			CHILD_UPDATED(i);
		}
	}
	return b;
}
bool MathStructure::calculateReplace(const MathStructure &mfrom, const MathStructure &mto, const EvaluationOptions &eo, bool exclude_function_arguments) {
	if(equals(mfrom, true, true)) {
		set(mto);
		return true;
	}
	if(mfrom.size() > 0 && mfrom.type() == m_type && SIZE > mfrom.size() && (mfrom.isAddition() || mfrom.isMultiplication() || mfrom.isLogicalAnd() || mfrom.isLogicalOr())) {
		bool b = true;
		size_t i2 = 0;
		for(size_t i = 0; i < mfrom.size(); i++) {
			b = false;
			for(; i2 < SIZE; i2++) {
				if(CHILD(i2).equals(mfrom[i], true, true)) {b = true; break;}
			}
			if(!b) break;
		}
		if(b) {
			i2 = 0;
			for(size_t i = 0; i < mfrom.size(); i++) {
				for(; i2 < SIZE; i2++) {
					if(CHILD(i2).equals(mfrom[i], true, true)) {ERASE(i2); break;}
				}
			}
			if(SIZE == 1) setToChild(1);
			else if(SIZE == 0) clear();
			else calculateReplace(mfrom, mto, eo, exclude_function_arguments);
			if(mfrom.isAddition()) add(mto);
			else if(mfrom.isMultiplication()) multiply(mto);
			else if(mfrom.isLogicalAnd()) transform(STRUCT_LOGICAL_AND, mto);
			else if(mfrom.isLogicalOr()) transform(STRUCT_LOGICAL_OR, mto);
			calculatesub(eo, eo, false);
			if(eo.calculate_functions && m_type == STRUCT_FUNCTION) calculateFunctions(eo, false);
			return true;
		}
	}
	if(exclude_function_arguments && m_type == STRUCT_FUNCTION) return false;
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).calculateReplace(mfrom, mto, eo, exclude_function_arguments)) {
			b = true;
			CHILD_UPDATED(i);
		}
	}
	if(b) {
		calculatesub(eo, eo, false);
		if(eo.calculate_functions && m_type == STRUCT_FUNCTION) calculateFunctions(eo, false);
	}
	return b;
}

bool MathStructure::replace(const MathStructure &mfrom1, const MathStructure &mto1, const MathStructure &mfrom2, const MathStructure &mto2) {
	if(equals(mfrom1, true, true)) {
		set(mto1);
		return true;
	}
	if(equals(mfrom2, true, true)) {
		set(mto2);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).replace(mfrom1, mto1, mfrom2, mto2)) {
			b = true;
			CHILD_UPDATED(i);
		}
	}
	return b;
}
bool MathStructure::removeType(StructureType mtype) {
	if(m_type == mtype || (m_type == STRUCT_POWER && CHILD(0).type() == mtype)) {
		set(1);
		return true;
	}
	bool b = false;
	if(m_type == STRUCT_MULTIPLICATION) {
		for(long int i = 0; i < (long int) SIZE; i++) {
			if(CHILD(i).removeType(mtype)) {
				if(CHILD(i).isOne()) {
					ERASE(i);
					i--;
				} else {
					CHILD_UPDATED(i);
				}
				b = true;
			}
		}
		if(SIZE == 0) {
			set(1);
		} else if(SIZE == 1) {
			setToChild(1, true);
		}
	} else {
		if(m_type == STRUCT_FUNCTION) {
			if(mtype != STRUCT_UNIT || (o_function->id() != FUNCTION_ID_SQRT && o_function->id() != FUNCTION_ID_ROOT && o_function->id() != FUNCTION_ID_CBRT)) return b;
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).removeType(mtype)) {
				b = true;
				CHILD_UPDATED(i);
			}
		}
	}
	return b;
}

const MathStructure &MathStructure::find_x_var() const {
	if(isSymbolic()) {
		return *this;
	} else if(isVariable()) {
		if(o_variable->isKnown()) return m_undefined;
		return *this;
	}
	const MathStructure *mstruct;
	const MathStructure *x_mstruct = &m_undefined;
	for(size_t i = 0; i < SIZE; i++) {
		mstruct = &CHILD(i).find_x_var();
		if(mstruct->isVariable()) {
			if(!((UnknownVariable*) mstruct->variable())->interval().isUndefined()) {
				if(x_mstruct->isUndefined()) x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->getVariableById(VARIABLE_ID_X)) {
				return *mstruct;
			} else if(!x_mstruct->isVariable() && (x_mstruct->isUndefined() || (mstruct->variable() != CALCULATOR->getVariableById(VARIABLE_ID_N) && mstruct->variable() != CALCULATOR->getVariableById(VARIABLE_ID_C)))) {
				x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->getVariableById(VARIABLE_ID_Y)) {
				x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->getVariableById(VARIABLE_ID_Z) && x_mstruct->variable() != CALCULATOR->getVariableById(VARIABLE_ID_Y)) {
				x_mstruct = mstruct;
			}
		} else if(mstruct->isSymbolic()) {
			if((!x_mstruct->isVariable() || x_mstruct->variable() == CALCULATOR->getVariableById(VARIABLE_ID_N) || x_mstruct->variable() == CALCULATOR->getVariableById(VARIABLE_ID_C)) && (m_type != STRUCT_FUNCTION || mstruct != &CHILD(i) || !o_function->getArgumentDefinition(i + 1) || o_function->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_TEXT) && (!x_mstruct->isSymbolic() || x_mstruct->symbol() > mstruct->symbol())) {
				x_mstruct = mstruct;
			}
		}
	}
	return *x_mstruct;
}

bool MathStructure::inParentheses() const {return b_parentheses;}
void MathStructure::setInParentheses(bool b) {b_parentheses = b;}

bool flattenMultiplication(MathStructure &mstruct, bool recursive) {
	bool retval = false;
	if(recursive) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(flattenMultiplication(mstruct[i], true)) retval = true;
		}
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size();) {
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					mstruct[i][i2].ref();
					mstruct.insertChild_nocopy(&mstruct[i][i2], i + i2 + 2);
				}
				mstruct.delChild(i + 1);
				retval = true;
			} else {
				i++;
			}
		}
	}
	return retval;
}

bool comparison_is_not_equal(ComparisonResult cr) {return COMPARISON_IS_NOT_EQUAL(cr);}
bool comparison_is_equal_or_less(ComparisonResult cr) {return COMPARISON_IS_EQUAL_OR_LESS(cr);}
bool comparison_is_equal_or_greater(ComparisonResult cr) {return COMPARISON_IS_EQUAL_OR_GREATER(cr);}
bool comparison_might_be_equal(ComparisonResult cr) {return COMPARISON_MIGHT_BE_EQUAL(cr);}

