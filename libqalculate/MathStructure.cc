/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "MathStructure.h"
#include "mathstructure-support.h"
#include "Calculator.h"
#include "BuiltinFunctions.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "Unit.h"
#include "Prefix.h"
#include <map>
#include <algorithm>

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
	using std::unordered_map;
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

using std::string;
using std::cout;
using std::vector;
using std::ostream;
using std::endl;

void printRecursive(const MathStructure &mstruct) {
	std::cout << "RECURSIVE " << mstruct.print() << std::endl;
	for(size_t i = 0; i < mstruct.size(); i++) {
		std::cout << i << ": " << mstruct[i].print() << std::endl;
		for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
			std::cout << i << "-" << i2 << ": " << mstruct[i][i2].print() << std::endl;
			for(size_t i3 = 0; i3 < mstruct[i][i2].size(); i3++) {
				std::cout << i << "-" << i2 << "-" << i3 << ": " << mstruct[i][i2][i3].print() << std::endl;
				for(size_t i4 = 0; i4 < mstruct[i][i2][i3].size(); i4++) {
					std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << ": " << mstruct[i][i2][i3][i4].print() << std::endl;
					for(size_t i5 = 0; i5 < mstruct[i][i2][i3][i4].size(); i5++) {
						std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << ": " << mstruct[i][i2][i3][i4][i5].print() << std::endl;
						for(size_t i6 = 0; i6 < mstruct[i][i2][i3][i4][i5].size(); i6++) {
							std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << ": " << mstruct[i][i2][i3][i4][i5][i6].print() << std::endl;
							for(size_t i7 = 0; i7 < mstruct[i][i2][i3][i4][i5][i6].size(); i7++) {
								std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << "-" << i7 << ": " << mstruct[i][i2][i3][i4][i5][i6][i7].print() << std::endl;
								for(size_t i8 = 0; i8 < mstruct[i][i2][i3][i4][i5][i6][i7].size(); i8++) {
									std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << "-" << i7 << "-" << i8 << ": " << mstruct[i][i2][i3][i4][i5][i6][i7][i8].print() << std::endl;
									for(size_t i9 = 0; i9 < mstruct[i][i2][i3][i4][i5][i6][i7][i8].size(); i9++) {
										std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << "-" << i7 << "-" << i8 << "-" << i9 << ": " << mstruct[i][i2][i3][i4][i5][i6][i7][i8][i9].print() << std::endl;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

string format_and_print(const MathStructure &mstruct) {
	MathStructure m_print(mstruct);
	if(CALCULATOR) {
		m_print.sort(CALCULATOR->messagePrintOptions());
		m_print.formatsub(CALCULATOR->messagePrintOptions(), NULL, 0, true, &m_print);
		return m_print.print(CALCULATOR->messagePrintOptions());
	} else {
		PrintOptions po;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		po.spell_out_logical_operators = true;
		po.number_fraction_format = FRACTION_FRACTIONAL;
		m_print.sort(po);
		m_print.formatsub(po, NULL, 0, true, &m_print);
		return m_print.print(po);
	}
}

bool sym_desc::operator<(const sym_desc &x) const {
	if (max_deg == x.max_deg) return max_lcnops < x.max_lcnops;
	else return max_deg.isLessThan(x.max_deg);
}

bool flattenMultiplication(MathStructure &mstruct) {
	bool retval = false;
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
	return retval;
}
bool flattenAddition(MathStructure &mstruct) {
	bool retval = false;
	for(size_t i = 0; i < mstruct.size();) {
		if(mstruct[i].isAddition()) {
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
	return retval;
}
bool warn_test_interval(MathStructure &mnonzero, const EvaluationOptions &eo2) {
	if(mnonzero.isComparison() && mnonzero[0].isVariable() && !mnonzero[0].variable()->isKnown() && !((UnknownVariable*) mnonzero[0].variable())->interval().isUndefined()) {
		/*if(((UnknownVariable*) mnonzero[0].variable())->interval().isNumber() && mnonzero[1].isNumber()) {
			MathStructure mbak(mnonzero);
			mnonzero[0] = ((UnknownVariable*) mnonzero[0].variable())->interval();
			mnonzero.eval(eo2);
			if(!mnonzero.isNumber()) mnonzero.clear();
			return true;
		} else*/ if(((UnknownVariable*) mnonzero[0].variable())->interval().containsInterval(true)) {
			MathStructure mbak(mnonzero);
			mnonzero[0] = ((UnknownVariable*) mnonzero[0].variable())->interval();
			mnonzero.eval(eo2);
			if(mnonzero.isComparison()) mnonzero = mbak;
			else return true;
		} else {
			mnonzero[0] = ((UnknownVariable*) mnonzero[0].variable())->interval();
			mnonzero.eval(eo2);
			return true;
		}
	} else if(mnonzero.isLogicalAnd() || mnonzero.isLogicalOr()) {
		bool b_ret = false;
		for(size_t i = 0; i < mnonzero.size(); i++) {
			if(warn_test_interval(mnonzero[i], eo2)) b_ret = true;
		}
		if(b_ret) {
			mnonzero.calculatesub(eo2, eo2, false);
			return true;
		}
	}
	return false;
}
bool warn_about_assumed_not_value(const MathStructure &mstruct, const MathStructure &mvalue, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(mvalue, OPERATION_NOT_EQUALS);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mvalue.isZero() && mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("Required assumption: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero(const MathStructure &mstruct, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero_or_positive(const MathStructure &mstruct, const MathStructure &mstruct2, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	MathStructure *mpos = new MathStructure(mstruct2);
	mpos->add(m_zero, OPERATION_EQUALS_GREATER);
	mnonzero.add_nocopy(mpos, OPERATION_LOGICAL_OR);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero_llgg(const MathStructure &mstruct, const MathStructure &mstruct2, const MathStructure &mstruct3, const EvaluationOptions &eo) {
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	MathStructure *mpos = new MathStructure(mstruct2);
	mpos->add(m_zero, OPERATION_EQUALS_GREATER);
	MathStructure *mpos2 = new MathStructure(mstruct3);
	mpos2->add(m_zero, OPERATION_EQUALS_GREATER);
	mpos->add_nocopy(mpos2, OPERATION_LOGICAL_AND);
	mnonzero.add_nocopy(mpos, OPERATION_LOGICAL_OR);
	MathStructure *mneg = new MathStructure(mstruct2);
	mneg->add(m_zero, OPERATION_LESS);
	MathStructure *mneg2 = new MathStructure(mstruct3);
	mneg2->add(m_zero, OPERATION_LESS);
	mneg->add_nocopy(mneg2, OPERATION_LOGICAL_AND);
	mnonzero.add_nocopy(mneg, OPERATION_LOGICAL_OR);
	mnonzero.eval(eo2);
	warn_test_interval(mnonzero, eo2);
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	if(mnonzero.isComparison() && mnonzero.comparisonType() == COMPARISON_NOT_EQUALS && mnonzero[1].isZero() && mnonzero[0].representsApproximatelyZero(true)) return false;
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mnonzero).c_str(), NULL);
	return true;
}

void unnegate_sign(MathStructure &mstruct) {
	if(mstruct.isNumber()) {
		mstruct.number().negate();
	} else if(mstruct.isMultiplication() && mstruct.size() > 0) {
		if(mstruct[0].isMinusOne()) {
			if(mstruct.size() > 2) {
				mstruct.delChild(1);
			} else if(mstruct.size() == 2) {
				mstruct.setToChild(2);
			} else {
				mstruct.set(1, 1, 0);
			}
		} else {
			unnegate_sign(mstruct[0]);
		}
	}
}

void MathStructure::mergePrecision(const MathStructure &o) {MERGE_APPROX_AND_PREC(o)}
void MathStructure::mergePrecision(bool approx, int prec) {
	if(!b_approx && approx) setApproximate();
	if(prec >= 0 && (i_precision < 0 || prec < i_precision)) {
		setPrecision(prec);
	}
}

void idm1(const MathStructure &mnum, bool &bfrac, bool &bint);
void idm2(const MathStructure &mnum, bool &bfrac, bool &bint, Number &nr);
int idm3(MathStructure &mnum, Number &nr, bool expand);

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
	i_precision = -1;
	i_ref = 1;
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
	clear();
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

/*MathStructure& MathStructure::CHILD(size_t v_index) const {
	if(v_index < v_order.size() && v_order[v_index] < v_subs.size()) return *v_subs[v_order[v_index]];
	MathStructure* m = new MathStructure;//(new UnknownVariable("x","x"));
	m->setUndefined(true);
	return *m;
}*/

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
	if(isUnit()) {
		o_prefix = p;
	}
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
	if(m_type != STRUCT_VECTOR || SIZE < 1 || CHILD(0).size() == 0) return false;
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
bool MathStructure::isUndefined() const {return m_type == STRUCT_UNDEFINED || (m_type == STRUCT_NUMBER && o_number.isUndefined()) || (m_type == STRUCT_VARIABLE && o_variable == CALCULATOR->v_undef);}
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
		case STRUCT_NUMBER: {return !o_number.isInteger();}
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsPositive(true);
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsNegative(true);
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsNonNegative(true);
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsNonPositive(true);
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsRational(true);
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsReal(true);
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
bool MathStructure::representsNonComplex(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return !o_number.hasImaginaryPart();}
		case STRUCT_VARIABLE: {
			if(o_variable->isKnown()) return ((KnownVariable*) o_variable)->get().representsNonComplex(allow_units);
			return o_variable->representsNonComplex(allow_units);
		}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isReal();}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsNonComplex(true);
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
			return CHILD(0).representsNonZero(allow_units) || (!CHILD(0).isApproximatelyZero() && CHILD(1).representsNonPositive());
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsUndefined(include_childs, include_infinite, be_strict);
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
		case STRUCT_VECTOR: {return !isMatrix();}
		case STRUCT_POWER: {return CHILD(0).representsNonMatrix();}
		case STRUCT_VARIABLE: {return o_variable->representsNonMatrix();}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonMatrix();}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsNonMatrix();
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
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) return CHILD(0).representsScalar();
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
	b_parentheses = false;
}
void MathStructure::transform(StructureType mtype, const Number &o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(o);
	b_parentheses = false;
}
void MathStructure::transform(StructureType mtype, int i) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(new MathStructure(i, 1, 0));
	b_parentheses = false;
}
void MathStructure::transform(StructureType mtype, Unit *u) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(u);
	b_parentheses = false;
}
void MathStructure::transform(StructureType mtype, Variable *v) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(v);
	b_parentheses = false;
}
void MathStructure::transform(StructureType mtype, string sym) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(sym);
	b_parentheses = false;
}
void MathStructure::transform_nocopy(StructureType mtype, MathStructure *o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(o);
	b_parentheses = false;
}
void MathStructure::transform(StructureType mtype) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	b_parentheses = false;
}
void MathStructure::transform(MathFunction *o) {
	transform(STRUCT_FUNCTION);
	setFunction(o);
	b_parentheses = false;
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
	b_parentheses = false;
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
			Prefix *p1 = (o_prefix == NULL || o_prefix == CALCULATOR->decimal_null_prefix || o_prefix == CALCULATOR->binary_null_prefix) ? NULL : o_prefix;
			Prefix *p2 = (o.prefix() == NULL || o.prefix() == CALCULATOR->decimal_null_prefix || o.prefix() == CALCULATOR->binary_null_prefix) ? NULL : o.prefix();
			return o_unit == o.unit() && p1 == p2;
		}
		case STRUCT_COMPARISON: {if(ct_comp != o.comparisonType()) return false; break;}
		case STRUCT_FUNCTION: {if(o_function != o.function()) return false; break;}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_LOGICAL_AND: {
			if(SIZE < 1) return false;
			if(SIZE == 2) {
				return (CHILD(0) == o[0] && CHILD(1) == o[1]) || (CHILD(0) == o[1] && CHILD(1) == o[0]);
			}
			vector<size_t> i2taken;
			for(size_t i = 0; i < SIZE; i++) {
				bool b = false, b2 = false;
				for(size_t i2 = 0; i2 < o.size(); i2++) {
					if(CHILD(i).equals(o[i2], allow_interval)) {
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
	if(SIZE < 1) return false;
	for(size_t i = 0; i < SIZE; i++) {
		if(!CHILD(i).equals(o[i], allow_interval)) return false;
	}
	return true;
}
bool MathStructure::equals(const Number &o, bool allow_interval, bool allow_infinite) const {
	if(m_type != STRUCT_NUMBER) return false;
	return o_number.equals(o, allow_interval, allow_infinite);
}
bool MathStructure::equals(int i) const {
	if(m_type != STRUCT_NUMBER) return false;
	return o_number.equals(Number(i, 1, 0));
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
bool remove_rad_unit(MathStructure &m, const EvaluationOptions &eo, bool top = true) {
	if(top && !remove_rad_unit_cf(m)) return false;
	if(m.isUnit()) {
		if(m.unit() == CALCULATOR->getRadUnit()) {
			m.set(1, 1, 0, true);
			return true;
		} else if(m.unit()->containsRelativeTo(CALCULATOR->getRadUnit())) {
			if(m.convert(CALCULATOR->getRadUnit())) {
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

int compare_check_incompability(MathStructure *mtest) {
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

bool replace_interval_unknowns(MathStructure &m, bool do_assumptions = false) {
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
		if(ass && ((ass->sign() != ASSUMPTION_SIGN_UNKNOWN && ass->sign() != ASSUMPTION_SIGN_NONZERO) || ass->min() || ass->max())) return 1;
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

int MathStructure::merge_addition(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool reversed) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.add(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	if(isZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(mstruct.isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(mstruct.representsReal(false)) {
			MERGE_APPROX_AND_PREC(mstruct)
			return 2;
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(representsReal(false)) {
			if(mparent) {
				mparent->swapChildren(index_this + 1, index_mstruct + 1);
			} else {
				clear(true);
				o_number = mstruct.number();
				MERGE_APPROX_AND_PREC(mstruct)
			}
			return 3;
		}
	}
	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {return 0;}
				case STRUCT_VECTOR: {
					if(SIZE == mstruct.size()) {
						for(size_t i = 0; i < SIZE; i++) {
							CHILD(i).calculateAdd(mstruct[i], eo, this, i);
						}
						MERGE_APPROX_AND_PREC(mstruct)
						CHILDREN_UPDATED
						return 1;
					}
				}
				default: {
					if(mstruct.representsScalar()) {
						for(size_t i = 0; i < SIZE; i++) {
							CHILD(i).calculateAdd(mstruct, eo, this, i);
						}
						CHILDREN_UPDATED
						return 1;
					}
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_ADDITION: {
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(reversed) {
							INSERT_REF(&mstruct[i], i)
							calculateAddIndex(i, eo, false);
						} else {
							APPEND_REF(&mstruct[i]);
							calculateAddLast(eo, false);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
				default: {
					MERGE_APPROX_AND_PREC(mstruct)
					if(reversed) {
						PREPEND_REF(&mstruct);
						calculateAddIndex(0, eo, true, mparent, index_this);
					} else {
						APPEND_REF(&mstruct);
						calculateAddLast(eo, true, mparent, index_this);
					}
					return 1;
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {return -1;}
				case STRUCT_ADDITION: {
					return 0;
				}
				case STRUCT_MULTIPLICATION: {

					size_t i1 = 0, i2 = 0;
					bool b = true;
					if(CHILD(0).isNumber()) i1 = 1;
					if(mstruct[0].isNumber()) i2 = 1;
					if(SIZE - i1 == mstruct.size() - i2) {
						for(size_t i = i1; i < SIZE; i++) {
							if(CHILD(i) != mstruct[i + i2 - i1]) {
								b = false;
								break;
							}
						}
						if(b) {
							if(i1 == 0) {
								PREPEND(m_one);
							}
							if(i2 == 0) {
								CHILD(0).number()++;
							} else {
								CHILD(0).number() += mstruct[0].number();
							}
							MERGE_APPROX_AND_PREC(mstruct)
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
							return 1;
						}
					}

					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(eo.transform_trigonometric_functions && CHILD(i2).isPower() && CHILD(i2)[0].isFunction() && (CHILD(i2)[0].function() == CALCULATOR->f_cos || CHILD(i2)[0].function() == CALCULATOR->f_sin) && CHILD(i2)[0].size() == 1 && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isTwo() && mstruct.size() > 0) {
							if(eo.protected_function != (CHILD(i2)[0].function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin)) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(mstruct[i].isPower() && mstruct[i][0].isFunction() && mstruct[i][0].function() == (CHILD(i2)[0].function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin) && mstruct[i][0].size() == 1 && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && CHILD(i2)[0][0] == mstruct[i][0][0]) {
										MathStructure madd1(*this);
										MathStructure madd2(mstruct);
										MathStructure madd;
										madd1.delChild(i2 + 1, true);
										madd2.delChild(i + 1, true);
										if(CHILD(i2)[0].function() == CALCULATOR->f_sin) {madd = madd2; madd2.calculateNegate(eo);}
										else {madd = madd1; madd1.calculateNegate(eo);}
										if(madd1.calculateAdd(madd2, eo)) {
											SET_CHILD_MAP(i2);
											CHILD(0).setFunction(CALCULATOR->f_sin);
											calculateMultiply(madd1, eo);
											EvaluationOptions eo2 = eo;
											eo2.transform_trigonometric_functions = false;
											calculateAdd(madd, eo2);
											return 1;
										}
									}
								}
							}
							if(eo.protected_function != CHILD(i2)[0].function()) {
								bool b = false;
								if(mstruct[0].isNumber()) {
									if(CHILD(0).isNumber()) {
										if(mstruct.size() == SIZE - 1 && CHILD(0).number() == -mstruct[0].number()) {
											b = true;
											for(size_t i = 1; i < mstruct.size(); i++) {
												if(!mstruct[i].equals(CHILD(i2 > i ? i : i + 1))) {b = false; break;}
											}
										}
									} else if(mstruct.size() == SIZE && mstruct[0].isMinusOne()) {
										b = true;
										for(size_t i = 1; i < mstruct.size(); i++) {
											if(!mstruct[i].equals(CHILD(i2 >= i ? i - 1 : i))) {b = false; break;}
										}
									}
								} else if(mstruct.size() == SIZE - 2 && CHILD(0).isMinusOne()) {
									b = true;
									for(size_t i = 0; i < mstruct.size(); i++) {
										if(!mstruct[i].equals(CHILD(i2 - 1 >= i ? i + 1 : i + 2))) {b = false; break;}
									}
								}
								if(b) {
									CHILD(i2)[0].setFunction(CHILD(i2)[0].function() == CALCULATOR->f_cos ? CALCULATOR->f_sin : CALCULATOR->f_cos);
									MERGE_APPROX_AND_PREC(mstruct)
									calculateNegate(eo, mparent, index_this);
									return 1;
								}
							}
						} else if(eo.transform_trigonometric_functions && CHILD(i2).isPower() && CHILD(i2)[0].isFunction() && (CHILD(i2)[0].function() == CALCULATOR->f_cosh || CHILD(i2)[0].function() == CALCULATOR->f_sinh) && CHILD(i2)[0].size() == 1 && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isTwo() && mstruct.size() > 0) {
							if(eo.protected_function != (CHILD(i2)[0].function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh)) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(mstruct[i].isPower() && mstruct[i][0].isFunction() && mstruct[i][0].function() == (CHILD(i2)[0].function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh) && mstruct[i][0].size() == 1 && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && CHILD(i2)[0][0] == mstruct[i][0][0]) {
										MathStructure madd1(*this);
										MathStructure madd2(mstruct);
										MathStructure madd;
										madd1.delChild(i2 + 1, true);
										madd2.delChild(i + 1, true);
										if(mstruct[i][0].function() == CALCULATOR->f_sinh) madd = madd1;
										else madd = madd2;
										if(madd1.calculateAdd(madd2, eo)) {
											SET_CHILD_MAP(i2);
											CHILD(0).setFunction(CALCULATOR->f_sinh);
											calculateMultiply(madd1, eo);
											EvaluationOptions eo2 = eo;
											eo2.transform_trigonometric_functions = false;
											calculateAdd(madd, eo2);
											return 1;
										}
									}
								}
							}
							if(eo.protected_function != CHILD(i2)[0].function()) {
								if(CHILD(i2)[0].function() == CALCULATOR->f_sinh && mstruct.size() == SIZE - 1) {
									for(size_t i = 0; i < mstruct.size(); i++) {
										if(!mstruct[i].equals(CHILD(i2 > i ? i : i + 1))) break;
										if(i == mstruct.size() - 1) {
											CHILD(i2)[0].setFunction(CALCULATOR->f_cosh);
											MERGE_APPROX_AND_PREC(mstruct)
											return 1;
										}
									}
								} else if(CHILD(i2)[0].function() == CALCULATOR->f_cosh) {
									bool b = false;
									if(mstruct[0].isNumber()) {
										if(CHILD(0).isNumber()) {
											if(mstruct.size() == SIZE - 1 && CHILD(0).number() == -mstruct[0].number()) {
												b = true;
												for(size_t i = 1; i < mstruct.size(); i++) {
													if(!mstruct[i].equals(CHILD(i2 > i ? i : i + 1))) {b = false; break;}
												}
											}
										} else if(mstruct.size() == SIZE && mstruct[0].isMinusOne()) {
											b = true;
											for(size_t i = 1; i < mstruct.size(); i++) {
												if(!mstruct[i].equals(CHILD(i2 >= i ? i - 1 : i))) {b = false; break;}
											}
										}
									} else if(mstruct.size() == SIZE - 2 && CHILD(0).isMinusOne()) {
										b = true;
										for(size_t i = 1; i < mstruct.size(); i++) {
											if(!mstruct[i].equals(CHILD(i2 - 1 >= i ? i + 1 : i + 2))) {b = false; break;}
										}
									}
									if(b) {
										CHILD(i2)[0].setFunction(CALCULATOR->f_sinh);
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									}
								}
							}
						} else if(CHILD(i2).isFunction()) {
							if(CHILD(i2).function() == CALCULATOR->f_signum && eo.protected_function != CALCULATOR->f_signum && CHILD(i2).size() == 2 && CHILD(i2)[0].isAddition() && CHILD(i2)[0].size() == 2 && CHILD(i2)[0].representsReal(true)) {
								for(size_t im = 0; im < mstruct.size(); im++) {
									if(mstruct[im] == CHILD(i2)) {
										MathStructure m1(*this), m2(mstruct);
										m1.delChild(i2 + 1, true);
										m2.delChild(im + 1, true);
										if((m1 == CHILD(i2)[0][0] && m2 == CHILD(i2)[0][1]) || (m2 == CHILD(i2)[0][0] && m1 == CHILD(i2)[0][1])) {
											SET_CHILD_MAP(i2)
											setFunction(CALCULATOR->f_abs);
											ERASE(1)
											MERGE_APPROX_AND_PREC(mstruct)
											return 1;
										}
									}
								}
							} else if(CHILD(i2).function() == CALCULATOR->f_asin || CHILD(i2).function() == CALCULATOR->f_acos) {
								CHILD(i2).setFunction(CHILD(i2).function() == CALCULATOR->f_asin ? CALCULATOR->f_acos : CALCULATOR->f_asin);
								if(equals(mstruct)) {
									//asin(x)+acos(x)=pi/2
									delChild(i2 + 1, true);
									switch(eo.parse_options.angle_unit) {
										case ANGLE_UNIT_DEGREES: {calculateMultiply(Number(90, 1, 0), eo); break;}
										case ANGLE_UNIT_GRADIANS: {calculateMultiply(Number(100, 1, 0), eo); break;}
										case ANGLE_UNIT_RADIANS: {calculateMultiply(CALCULATOR->v_pi, eo); calculateMultiply(nr_half, eo); break;}
										default: {calculateMultiply(CALCULATOR->v_pi, eo); calculateMultiply(nr_half, eo); if(CALCULATOR->getRadUnit()) {calculateMultiply(CALCULATOR->getRadUnit(), eo);} break;}
									}
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								CHILD(i2).setFunction(CHILD(i2).function() == CALCULATOR->f_asin ? CALCULATOR->f_acos : CALCULATOR->f_asin);
							} else if(CHILD(i2).function() == CALCULATOR->f_sinh || CHILD(i2).function() == CALCULATOR->f_cosh) {
								CHILD(i2).setFunction(CHILD(i2).function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh);
								if(equals(mstruct)) {
									//sinh(x)+cosh(x)=e^x
									MathStructure *mexp = &CHILD(i2)[0];
									mexp->ref();
									delChild(i2 + 1, true);
									MathStructure *mmul = new MathStructure(CALCULATOR->v_e);
									mmul->raise_nocopy(mexp);
									mmul->calculateRaiseExponent(eo);
									MERGE_APPROX_AND_PREC(mstruct)
									multiply_nocopy(mmul);
									calculateMultiplyLast(eo, true, mparent, index_this);
									return 1;
								}
								CHILD(i2).setFunction(CHILD(i2).function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh);
							}
						}
					}

					if(eo.transform_trigonometric_functions) {
						for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
							if(mstruct[i2].isPower() && mstruct[i2][0].isFunction() && (mstruct[i2][0].function() == CALCULATOR->f_cos || mstruct[i2][0].function() == CALCULATOR->f_sin) && mstruct[i2][0].size() == 1 && mstruct[i2][1].isNumber() && mstruct[i2][1].number().isTwo() && SIZE > 0) {
								if(eo.protected_function != mstruct[i2][0].function()) {
									bool b = false;
									if(CHILD(0).isNumber()) {
										if(mstruct[0].isNumber()) {
											if(mstruct.size() - 1 == SIZE && CHILD(0).number() == -mstruct[0].number()) {
												b = true;
												for(size_t i = 1; i < SIZE; i++) {
													if(!CHILD(i).equals(mstruct[i2 > i ? i : i + 1])) {b = false; break;}
												}
											}
										} else if(mstruct.size() == SIZE && CHILD(0).isMinusOne()) {
											b = true;
											for(size_t i = 1; i < SIZE; i++) {
												if(!CHILD(i).equals(mstruct[i2 >= i ? i - 1 : i])) {b = false; break;}
											}
										}
									} else if(mstruct.size() - 2 == SIZE && mstruct[0].isMinusOne()) {
										b = true;
										for(size_t i = 0; i < SIZE; i++) {
											if(!CHILD(i).equals(mstruct[i2 - 1 >= i ? i + 1 : i + 2])) {b = false; break;}
										}
									}
									if(b) {
										mstruct[i2][0].setFunction(mstruct[i2][0].function() == CALCULATOR->f_cos ? CALCULATOR->f_sin : CALCULATOR->f_cos);
										mstruct.calculateNegate(eo);
										if(mparent) mparent->swapChildren(index_this + 1, index_mstruct + 1);
										else set_nocopy(mstruct, true);
										return 1;
									}
								}
							} else if(mstruct[i2].isPower() && mstruct[i2][0].isFunction() && (mstruct[i2][0].function() == CALCULATOR->f_cosh || mstruct[i2][0].function() == CALCULATOR->f_sinh) && mstruct[i2][0].size() == 1 && mstruct[i2][1].isNumber() && mstruct[i2][1].number().isTwo() && SIZE > 0) {
								if(eo.protected_function != mstruct[i2][0].function()) {
									if(mstruct[i2][0].function() == CALCULATOR->f_sinh && mstruct.size() - 1 == SIZE) {
										for(size_t i = 0; i < SIZE; i++) {
											if(!CHILD(i).equals(mstruct[i2 > i ? i : i + 1])) break;
											if(i == SIZE - 1) {
												mstruct[i2][0].setFunction(CALCULATOR->f_cosh);
												if(mparent) mparent->swapChildren(index_this + 1, index_mstruct + 1);
												else set_nocopy(mstruct, true);
												return 1;
											}
										}
									} else if(mstruct[i2][0].function() == CALCULATOR->f_cosh) {
										bool b = false;
										if(CHILD(0).isNumber()) {
											if(mstruct[0].isNumber()) {
												if(mstruct.size() - 1 == SIZE && CHILD(0).number() == -mstruct[0].number()) {
													b = true;
													for(size_t i = 1; i < SIZE; i++) {
														if(!CHILD(i).equals(mstruct[i2 > i ? i : i + 1])) {b = false; break;}
													}
												}
											} else if(mstruct.size() == SIZE && CHILD(0).isMinusOne()) {
												b = true;
												for(size_t i = 1; i < SIZE; i++) {
													if(!CHILD(i).equals(mstruct[i2 >= i ? i - 1 : i])) {b = false; break;}
												}
											}
										} else if(mstruct.size() - 2 == SIZE && mstruct[0].isMinusOne()) {
											b = true;
											for(size_t i = 1; i < SIZE; i++) {
												if(!CHILD(i).equals(mstruct[i2 - 1 >= i ? i + 1 : i + 2])) {b = false; break;}
											}
										}
										if(b) {
											mstruct[i2][0].setFunction(CALCULATOR->f_sinh);
											if(mparent) mparent->swapChildren(index_this + 1, index_mstruct + 1);
											else set_nocopy(mstruct, true);
											return 1;
										}
									}
								}
							}
						}
					}

					if(!eo.combine_divisions) break;
					b = true; size_t divs = 0;
					for(; b && i1 < SIZE; i1++) {
						if(CHILD(i1).isPower() && CHILD(i1)[1].hasNegativeSign()) {
							divs++;
							b = false;
							for(; i2 < mstruct.size(); i2++) {
								if(mstruct[i2].isPower() && mstruct[i2][1].hasNegativeSign()) {
									if(mstruct[i2] == CHILD(i1)) {
										b = true;
									}
									i2++;
									break;
								}
							}
						}
					}
					if(b && divs > 0) {
						for(; i2 < mstruct.size(); i2++) {
							if(mstruct[i2].isPower() && mstruct[i2][1].hasNegativeSign()) {
								b = false;
								break;
							}
						}
					}
					if(b && divs > 0) {
						if(SIZE - divs == 0) {
							if(mstruct.size() - divs == 0) {
								calculateMultiply(nr_two, eo);
							} else if(mstruct.size() - divs == 1) {
								PREPEND(m_one);
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										CHILD(0).add_nocopy(&mstruct[i], true);
										CHILD(0).calculateAddLast(eo, true, this, 0);
										break;
									}
								}
								calculateMultiplyIndex(0, eo, true, mparent, index_this);
							} else {
								for(size_t i = 0; i < mstruct.size();) {
									if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
										mstruct.delChild(i + 1);
									} else {
										i++;
									}
								}
								PREPEND(m_one);
								mstruct.ref();
								CHILD(0).add_nocopy(&mstruct, true);
								CHILD(0).calculateAddLast(eo, true, this, 0);
								calculateMultiplyIndex(0, eo, true, mparent, index_this);
							}
						} else if(SIZE - divs == 1) {
							size_t index = 0;
							for(; index < SIZE; index++) {
								if(!CHILD(index).isPower() || !CHILD(index)[1].hasNegativeSign()) {
									break;
								}
							}
							if(mstruct.size() - divs == 0) {
								if(IS_REAL(CHILD(index))) {
									CHILD(index).number()++;
								} else {
									CHILD(index).calculateAdd(m_one, eo, this, index);
								}
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							} else if(mstruct.size() - divs == 1) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										CHILD(index).add_nocopy(&mstruct[i], true);
										CHILD(index).calculateAddLast(eo, true, this, index);
										break;
									}
								}
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							} else {
								for(size_t i = 0; i < mstruct.size();) {
									if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
										mstruct.delChild(i + 1);
									} else {
										i++;
									}
								}
								mstruct.ref();
								CHILD(index).add_nocopy(&mstruct, true);
								CHILD(index).calculateAddLast(eo, true, this, index);
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							}
						} else {
							for(size_t i = 0; i < SIZE;) {
								if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
									ERASE(i);
								} else {
									i++;
								}
							}
							if(mstruct.size() - divs == 0) {
								calculateAdd(m_one, eo);
							} else if(mstruct.size() - divs == 1) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										add_nocopy(&mstruct[i], true);
										calculateAddLast(eo);
										break;
									}
								}
							} else {
								MathStructure *mstruct2 = new MathStructure();
								mstruct2->setType(STRUCT_MULTIPLICATION);
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										mstruct2->addChild_nocopy(&mstruct[i]);
									}
								}
								add_nocopy(mstruct2, true);
								calculateAddLast(eo, true, mparent, index_this);
							}
							for(size_t i = 0; i < mstruct.size(); i++) {
								if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
									mstruct[i].ref();
									multiply_nocopy(&mstruct[i], true);
									calculateMultiplyLast(eo);
								}
							}
						}
						return 1;
					}

					break;
				}
				case STRUCT_POWER: {
					if(eo.combine_divisions && mstruct[1].hasNegativeSign()) {
						bool b = false;
						size_t index = 0;
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
								if(b) {
									b = false;
									break;
								}
								if(mstruct == CHILD(i)) {
									index = i;
									b = true;
								}
								if(!b) break;
							}
						}
						if(b) {
							if(SIZE == 2) {
								if(index == 0) setToChild(2, true);
								else setToChild(1, true);
							} else {
								ERASE(index);
							}
							calculateAdd(m_one, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct, false);
							calculateMultiplyLast(eo, true, mparent, index_this);
							return 1;
						}
					}
					if(eo.transform_trigonometric_functions && SIZE == 2 && CHILD(0).isNumber() && mstruct[0].isFunction()) {
						if((mstruct[0].function() == CALCULATOR->f_cos || mstruct[0].function() == CALCULATOR->f_sin) && eo.protected_function != (mstruct[0].function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo()) {
							if(CHILD(1).isPower() && CHILD(1)[0].isFunction() && CHILD(1)[0].function() == (mstruct[0].function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin) && CHILD(1)[0].size() == 1 && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && mstruct[0][0] == CHILD(1)[0][0]) {
								if(CHILD(0).calculateSubtract(m_one, eo)) {
									add(m_one);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
							}
						} else if((mstruct[0].function() == CALCULATOR->f_cosh || mstruct[0].function() == CALCULATOR->f_sinh) && eo.protected_function != (mstruct[0].function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo()) {
							if(CHILD(1).isPower() && CHILD(1)[0].isFunction() && CHILD(1)[0].function() == (mstruct[0].function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh) && CHILD(1)[0].size() == 1 && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && mstruct[0][0] == CHILD(1)[0][0]) {
								if(CHILD(0).calculateAdd(m_one, eo)) {
									add(CHILD(1)[0].function() == CALCULATOR->f_sinh ? m_one : m_minus_one);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
							}
						}
					}
				}
				default: {
					if(SIZE == 2 && CHILD(0).isNumber() && CHILD(1) == mstruct) {
						CHILD(0).number()++;
						MERGE_APPROX_AND_PREC(mstruct)
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
						return 1;
					}
					if(eo.transform_trigonometric_functions && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[0].isFunction() && CHILD(1)[0].function() != eo.protected_function && CHILD(1)[1] == nr_two && CHILD(1)[0].size() == 1) {
						if(mstruct.isNumber() && CHILD(0).isNumber()) {
							if(CHILD(1)[0].function() == CALCULATOR->f_sin && mstruct.number() == -CHILD(0).number()) {
								CHILD(1)[0].setFunction(CALCULATOR->f_cos);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(1)[0].function() == CALCULATOR->f_cos && mstruct.number() == -CHILD(0).number()) {
								CHILD(1)[0].setFunction(CALCULATOR->f_sin);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(1)[0].function() == CALCULATOR->f_cosh && mstruct.number() == -CHILD(0).number()) {
								MERGE_APPROX_AND_PREC(mstruct)
								CHILD(1)[0].setFunction(CALCULATOR->f_sinh);
								return 1;
							} else if(CHILD(1)[0].function() == CALCULATOR->f_sinh && mstruct == CHILD(0)) {
								CHILD(1)[0].setFunction(CALCULATOR->f_cosh);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					} else if(eo.transform_trigonometric_functions && SIZE == 3 && CHILD(0).isMinusOne()) {
						size_t i = 0;
						if(CHILD(1).isPower() && CHILD(1)[0].isFunction() && (CHILD(1)[0].function() == CALCULATOR->f_sin || CHILD(1)[0].function() == CALCULATOR->f_cos || CHILD(1)[0].function() == CALCULATOR->f_cosh) && CHILD(1)[0].function() != eo.protected_function && CHILD(1)[1] == nr_two && CHILD(1)[0].size() == 1 && CHILD(2) == mstruct) i = 1;
						if(CHILD(2).isPower() && CHILD(2)[0].isFunction() && (CHILD(2)[0].function() == CALCULATOR->f_sin || CHILD(2)[0].function() == CALCULATOR->f_cos || CHILD(2)[0].function() == CALCULATOR->f_cosh) && CHILD(2)[0].function() != eo.protected_function && CHILD(2)[1] == nr_two && CHILD(2)[0].size() == 1 && CHILD(1) == mstruct) i = 2;
						if(i > 0) {
							if(CHILD(i)[0].function() == CALCULATOR->f_sin) {
								CHILD(i)[0].setFunction(CALCULATOR->f_cos);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(i)[0].function() == CALCULATOR->f_cos) {
								CHILD(i)[0].setFunction(CALCULATOR->f_sin);
								MERGE_APPROX_AND_PREC(mstruct)
								calculateNegate(eo, mparent, index_this);
								return 1;
							} else if(CHILD(i)[0].function() == CALCULATOR->f_cosh) {
								MERGE_APPROX_AND_PREC(mstruct)
								CHILD(i)[0].setFunction(CALCULATOR->f_sinh);
								return 1;
							}
						}
					}
					if(mstruct.isDateTime() || (mstruct.isFunction() && mstruct.function() == CALCULATOR->f_signum && eo.protected_function != CALCULATOR->f_signum)) {
						return 0;
					}
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isFunction() && (CHILD(0).function() == CALCULATOR->f_cos || CHILD(0).function() == CALCULATOR->f_sin) && eo.protected_function != (CHILD(0).function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin) && CHILD(0).size() == 1 && CHILD(1).isNumber() && CHILD(1).number().isTwo()) {
				if(eo.transform_trigonometric_functions && mstruct.isMinusOne()) {
					CHILD(0).setFunction(CHILD(0).function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin);
					negate();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function() == (CHILD(0).function() == CALCULATOR->f_sin ? CALCULATOR->f_cos : CALCULATOR->f_sin) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo() && CHILD(0)[0] == mstruct[0][0]) {
					// cos(x)^2+sin(x)^2=1
					set(1, 1, 0, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(eo.transform_trigonometric_functions && CHILD(0).isFunction() && (CHILD(0).function() == CALCULATOR->f_cosh || CHILD(0).function() == CALCULATOR->f_sinh) && eo.protected_function != (CHILD(0).function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh) && CHILD(0).size() == 1 && CHILD(1).isNumber() && CHILD(1).number().isTwo()) {
				// cosh(x)^2=sinh(x)^2+1
				if(CHILD(0).function() == CALCULATOR->f_sinh && mstruct.isOne()) {
					CHILD(0).setFunction(CALCULATOR->f_cosh);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(CHILD(0).function() == CALCULATOR->f_cosh && mstruct.isMinusOne()) {
					CHILD(0).setFunction(CALCULATOR->f_sinh);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function() == (CHILD(0).function() == CALCULATOR->f_sinh ? CALCULATOR->f_cosh : CALCULATOR->f_sinh) && mstruct[0].size() == 1 && mstruct[1].isNumber() && mstruct[1].number().isTwo() && CHILD(0)[0] == mstruct[0][0]) {
					if(CHILD(0).function() == CALCULATOR->f_sinh) {
						multiply(nr_two);
						add(m_one);
					} else {
						CHILD(0).setFunction(CALCULATOR->f_sinh);
						multiply(nr_two);
						add(m_minus_one);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			goto default_addition_merge;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_signum && mstruct.isMultiplication() && eo.protected_function != CALCULATOR->f_signum) {
				if(SIZE == 2 && CHILD(0).isAddition() && CHILD(0).size() == 2 && (CHILD(0)[0].isOne() || CHILD(0)[1].isOne()) && CHILD(0).representsReal(true)) {
					for(size_t im = 0; im < mstruct.size(); im++) {
						if(mstruct[im] == *this) {
							MathStructure m2(mstruct);
							m2.delChild(im + 1, true);
							if((CHILD(0)[0].isOne() && m2 == CHILD(0)[1]) || (CHILD(0)[1].isOne() && m2 == CHILD(0)[0])) {
								setFunction(CALCULATOR->f_abs);
								ERASE(1)
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					}
				}
			} else if(mstruct.isFunction() && ((o_function == CALCULATOR->f_asin && mstruct.function() == CALCULATOR->f_acos) || (o_function == CALCULATOR->f_acos && mstruct.function() == CALCULATOR->f_asin)) && eo.protected_function != CALCULATOR->f_acos && eo.protected_function != CALCULATOR->f_asin && SIZE == 1 && mstruct.size() == 1 && CHILD(0) == mstruct[0]) {
				//asin(x)+acos(x)=pi/2
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {set(90, 1, 0, true); break;}
					case ANGLE_UNIT_GRADIANS: {set(100, 1, 0, true); break;}
					case ANGLE_UNIT_RADIANS: {set(CALCULATOR->v_pi, true); multiply(nr_half); calculatesub(eo, eo, true); break;}
					default: {set(CALCULATOR->v_pi, true); multiply(nr_half); if(CALCULATOR->getRadUnit()) {multiply(CALCULATOR->getRadUnit(), true);} calculatesub(eo, eo, true); break;}
				}
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(mstruct.isFunction() && ((o_function == CALCULATOR->f_sinh && mstruct.function() == CALCULATOR->f_cosh) || (o_function == CALCULATOR->f_cosh && mstruct.function() == CALCULATOR->f_sinh)) && eo.protected_function != CALCULATOR->f_cosh && eo.protected_function != CALCULATOR->f_sinh && SIZE == 1 && mstruct.size() == 1 && CHILD(0) == mstruct[0]) {
				//sinh(x)+cosh(x)=e^x
				MathStructure *mexp = &CHILD(0);
				mexp->ref();
				set(CALCULATOR->v_e, true);
				calculatesub(eo, eo, true);
				raise_nocopy(mexp);
				MERGE_APPROX_AND_PREC(mstruct)
				calculateRaiseExponent(eo, mparent, index_this);
				return 1;
			} else if(mstruct.isFunction() && o_function == CALCULATOR->f_stripunits && mstruct.function() == CALCULATOR->f_stripunits && mstruct.size() == 1 && SIZE == 1) {
				mstruct[0].ref();
				CHILD(0).add_nocopy(&mstruct[0]);
				return 1;
			}
			goto default_addition_merge;
		}
		case STRUCT_DATETIME: {
			if(mstruct.isDateTime()) {
				if(o_datetime->add(*mstruct.datetime())) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isMinusOne() && mstruct[1].isDateTime() && (CALCULATOR->u_second || CALCULATOR->u_day)) {
				if(CALCULATOR->u_day && !mstruct[1].datetime()->timeIsSet() && !o_datetime->timeIsSet()) {
					Number ndays = mstruct[1].datetime()->daysTo(*o_datetime);
					set(ndays, true);
					multiply(CALCULATOR->u_day);
				} else {
					Number nsecs = mstruct[1].datetime()->secondsTo(*o_datetime, true);
					set(nsecs, true);
					multiply(CALCULATOR->u_second, true);
				}
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(CALCULATOR->u_second && ((mstruct.isUnit() && mstruct.unit()->baseUnit() == CALCULATOR->u_second && mstruct.unit()->baseExponent() == 1 && !mstruct.unit()->hasNonlinearRelationTo(CALCULATOR->u_second)) || (mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[0].number().isReal() && !mstruct[0].number().isInterval() && mstruct[1].isUnit() && mstruct[1].unit()->baseUnit() == CALCULATOR->u_second && mstruct[1].unit()->baseExponent() == 1 && !mstruct[1].unit()->hasNonlinearRelationTo(CALCULATOR->u_second)))) {
				MathStructure mmul(1, 1, 0);
				Unit *u;
				if(mstruct.isMultiplication()) {
					mmul = mstruct[0];
					u = mstruct[1].unit();
				} else {
					u = mstruct.unit();
				}
				if(CALCULATOR->u_month && u != CALCULATOR->u_year && (u == CALCULATOR->u_month || u->isChildOf(CALCULATOR->u_month))) {
					if(u != CALCULATOR->u_month) {
						CALCULATOR->u_month->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addMonths(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->u_year && (u == CALCULATOR->u_year || u->isChildOf(CALCULATOR->u_year))) {
					if(u != CALCULATOR->u_year) {
						CALCULATOR->u_year->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addYears(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->u_day && (u == CALCULATOR->u_day || u->isChildOf(CALCULATOR->u_day))) {
					if(u != CALCULATOR->u_day) {
						CALCULATOR->u_day->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addDays(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->u_hour && (u == CALCULATOR->u_hour || u->isChildOf(CALCULATOR->u_hour))) {
					if(u != CALCULATOR->u_hour) {
						CALCULATOR->u_hour->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addHours(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(CALCULATOR->u_minute && (u == CALCULATOR->u_minute || u->isChildOf(CALCULATOR->u_minute))) {
					if(u != CALCULATOR->u_minute) {
						CALCULATOR->u_minute->convert(u, mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addMinutes(mmul.number())) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else {
					MathStructure mmulb(mmul);
					if(u != CALCULATOR->u_second) {
						u->convertToBaseUnit(mmul);
						mmul.eval(eo);
					}
					if(mmul.isNumber() && o_datetime->addSeconds(mmul.number(), true)) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				}
			}
		}
		default: {
			default_addition_merge:
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {return -1;}
				case STRUCT_DATETIME: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {
					return 0;
				}
				default: {
					if(equals(mstruct)) {
						multiply_nocopy(new MathStructure(2, 1, 0), true);
						MERGE_APPROX_AND_PREC(mstruct)
						calculateMultiplyLast(eo, true, mparent, index_this);
						return 1;
					}
				}
			}
		}
	}
	return -1;
}

bool reducable(const MathStructure &mnum, const MathStructure &mden, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {}
		case STRUCT_ADDITION: {
			break;
		}
		default: {
			bool reduce = true;
			for(size_t i = 0; i < mden.size() && reduce; i++) {
				switch(mden[i].type()) {
					case STRUCT_MULTIPLICATION: {
						reduce = false;
						for(size_t i2 = 0; i2 < mden[i].size(); i2++) {
							if(mnum == mden[i][i2]) {
								reduce = true;
								if(!nr.isOne() && !nr.isFraction()) nr.set(1, 1, 0);
								break;
							} else if(mden[i][i2].isPower() && mden[i][i2][1].isNumber() && mden[i][i2][1].number().isReal() && mnum == mden[i][i2][0]) {
								if(!mden[i][i2][1].number().isPositive()) {
									break;
								}
								if(mden[i][i2][1].number().isLessThan(nr)) nr = mden[i][i2][1].number();
								reduce = true;
								break;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mden[i][1].isNumber() && mden[i][1].number().isReal() && mnum == mden[i][0]) {
							if(!mden[i][1].number().isPositive()) {
								reduce = false;
								break;
							}
							if(mden[i][1].number().isLessThan(nr)) nr = mden[i][1].number();
							break;
						}
					}
					default: {
						if(mnum != mden[i]) {
							reduce = false;
							break;
						}
						if(!nr.isOne() && !nr.isFraction()) nr.set(1, 1, 0);
					}
				}
			}
			return reduce;
		}
	}
	return false;
}
void reduce(const MathStructure &mnum, MathStructure &mden, Number &nr, const EvaluationOptions &eo) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {}
		case STRUCT_ADDITION: {
			break;
		}
		default: {
			for(size_t i = 0; i < mden.size(); i++) {
				switch(mden[i].type()) {
					case STRUCT_MULTIPLICATION: {
						for(size_t i2 = 0; i2 < mden[i].size(); i2++) {
							if(mden[i][i2] == mnum) {
								if(!nr.isOne()) {
									MathStructure *mexp = new MathStructure(1, 1, 0);
									mexp->number() -= nr;
									mden[i][i2].raise_nocopy(mexp);
									mden[i][i2].calculateRaiseExponent(eo);
									if(mden[i][i2].isOne() && mden[i].size() > 1) {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								} else {
									if(mden[i].size() == 1) {
										mden[i].set(m_one);
									} else {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								}
								break;
							} else if(mden[i][i2].isPower() && mden[i][i2][1].isNumber() && mden[i][i2][1].number().isReal() && mnum.equals(mden[i][i2][0])) {
								mden[i][i2][1].number() -= nr;
								if(mden[i][i2][1].number().isOne()) {
									mden[i][i2].setToChild(1, true, &mden[i], i2 + 1);
								} else {
									mden[i][i2].calculateRaiseExponent(eo);
									if(mden[i][i2].isOne() && mden[i].size() > 1) {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								}
								break;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mden[i][1].isNumber() && mden[i][1].number().isReal() && mnum.equals(mden[i][0])) {
							mden[i][1].number() -= nr;
							if(mden[i][1].number().isOne()) {
								mden[i].setToChild(1, true, &mden, i + 1);
							} else {
								mden[i].calculateRaiseExponent(eo, &mden, i);
							}
							break;
						}
					}
					default: {
						if(!nr.isOne()) {
							MathStructure *mexp = new MathStructure(1, 1, 0);
							mexp->number() -= nr;
							mden[i].raise_nocopy(mexp);
							mden[i].calculateRaiseExponent(eo, &mden, 1);
						} else {
							mden[i].set(m_one);
						}
					}
				}
			}
			mden.calculatesub(eo, eo, false);
		}
	}
}

bool addablePower(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct[0].representsNonNegative(true)) return true;
	if(mstruct[1].representsInteger()) return true;
	//return eo.allow_complex && mstruct[0].representsNegative(true) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsEven();
	return eo.allow_complex && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsEven();
}

int MathStructure::merge_multiplication(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool reversed, bool do_append) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.multiply(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	if(mstruct.isOne()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	} else if(isOne()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(o_number.isMinusInfinity(false)) {
			if(mstruct.representsPositive(false)) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			} else if(mstruct.representsNegative(false)) {
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(o_number.isPlusInfinity(false)) {
			if(mstruct.representsPositive(false)) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			} else if(mstruct.representsNegative(false)) {
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		if(eo.approximation == APPROXIMATION_EXACT) {
			MathStructure mtest(mstruct);
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				mtest.calculateFunctions(eo2);
				mtest.calculatesub(eo2, eo2);
				CALCULATOR->endTemporaryStopMessages();
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
			if(o_number.isMinusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				} else if(mtest.representsNegative(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(o_number.isPlusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				} else if(mtest.representsNegative(false)) {
					o_number.setMinusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(mstruct.number().isMinusInfinity(false)) {
			if(representsPositive(false)) {
				clear(true);
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(representsNegative(false)) {
				clear(true);
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(mstruct.number().isPlusInfinity(false)) {
			if(representsPositive(false)) {
				clear(true);
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(representsNegative(false)) {
				clear(true);
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		if(eo.approximation == APPROXIMATION_EXACT) {
			MathStructure mtest(*this);
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				mtest.calculateFunctions(eo2);
				mtest.calculatesub(eo2, eo2);
				CALCULATOR->endTemporaryStopMessages();
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
			if(mstruct.number().isMinusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					clear(true);
					o_number.setMinusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.representsNegative(false)) {
					clear(true);
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(mstruct.number().isPlusInfinity(false)) {
				if(mtest.representsPositive(false)) {
					clear(true);
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.representsNegative(false)) {
					clear(true);
					o_number.setMinusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	}

	if(representsUndefined() || mstruct.representsUndefined()) return -1;

	// x/(x^2+x)=1/(x+1)
	const MathStructure *mnum = NULL, *mden = NULL;
	bool b_nonzero = false;
	if(eo.reduce_divisions) {
		if(!isNumber() && mstruct.isPower() && mstruct[0].isAddition() && mstruct[0].size() > 1 && mstruct[1].isNumber() && mstruct[1].number().isMinusOne()) {
			if((!isPower() || !CHILD(1).hasNegativeSign()) && representsNumber() && mstruct[0].representsNumber()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mstruct[0].representsZero(true)) || mstruct[0].representsNonZero(true)) {
					b_nonzero = true;
				}
				if(b_nonzero || (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mstruct[0].representsZero(true))) {
					mnum = this;
					mden = &mstruct[0];
				}
			}
		} else if(!mstruct.isNumber() && isPower() && CHILD(0).isAddition() && CHILD(0).size() > 1 && CHILD(1).isNumber() && CHILD(1).number().isMinusOne()) {
			if((!mstruct.isPower() || !mstruct[1].hasNegativeSign()) && mstruct.representsNumber() && CHILD(0).representsNumber()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) || CHILD(0).representsNonZero(true)) {
					b_nonzero = true;
				}
				if(b_nonzero || (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))) {
					mnum = &mstruct;
					mden = &CHILD(0);
				}
			}
		}
	}

	if(mnum && mden && eo.reduce_divisions) {
		switch(mnum->type()) {
			case STRUCT_ADDITION: {
				break;
			}
			case STRUCT_MULTIPLICATION: {
				Number nr;
				vector<Number> nrs;
				vector<size_t> reducables;
				for(size_t i = 0; i < mnum->size(); i++) {
					switch((*mnum)[i].type()) {
						case STRUCT_ADDITION: {break;}
						case STRUCT_POWER: {
							if((*mnum)[i][1].isNumber() && (*mnum)[i][1].number().isReal()) {
								if((*mnum)[i][1].number().isPositive()) {
									nr.set((*mnum)[i][1].number());
									if(reducable((*mnum)[i][0], *mden, nr)) {
										nrs.push_back(nr);
										reducables.push_back(i);
									}
								}
								break;
							}
						}
						default: {
							nr.set(1, 1, 0);
							if(reducable((*mnum)[i], *mden, nr)) {
								nrs.push_back(nr);
								reducables.push_back(i);
							}
						}
					}
				}
				if(reducables.size() > 0) {
					if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
					if(mnum == this) {
						mstruct.ref();
						transform_nocopy(STRUCT_MULTIPLICATION, &mstruct);
					} else {
						transform(STRUCT_MULTIPLICATION);
						PREPEND_REF(&mstruct);
					}
					size_t i_erased = 0;
					for(size_t i = 0; i < reducables.size(); i++) {
						switch(CHILD(0)[reducables[i] - i_erased].type()) {
							case STRUCT_POWER: {
								if(CHILD(0)[reducables[i] - i_erased][1].isNumber() && CHILD(0)[reducables[i] - i_erased][1].number().isReal()) {
									reduce(CHILD(0)[reducables[i] - i_erased][0], CHILD(1)[0], nrs[i], eo);
									if(nrs[i] == CHILD(0)[reducables[i] - i_erased][1].number()) {
										CHILD(0).delChild(reducables[i] - i_erased + 1);
										i_erased++;
									} else {
										CHILD(0)[reducables[i] - i_erased][1].number() -= nrs[i];
										if(CHILD(0)[reducables[i] - i_erased][1].number().isOne()) {
											CHILD(0)[reducables[i] - i_erased].setToChild(1, true, &CHILD(0), reducables[i] - i_erased + 1);
										} else {
											CHILD(0)[reducables[i] - i_erased].calculateRaiseExponent(eo);
										}
										CHILD(0).calculateMultiplyIndex(reducables[i] - i_erased, eo, true);
									}
									break;
								}
							}
							default: {
								reduce(CHILD(0)[reducables[i] - i_erased], CHILD(1)[0], nrs[i], eo);
								if(nrs[i].isOne()) {
									CHILD(0).delChild(reducables[i] - i_erased + 1);
									i_erased++;
								} else {
									MathStructure mexp(1, 1);
									mexp.number() -= nrs[i];
									CHILD(0)[reducables[i] - i_erased].calculateRaise(mexp, eo);
									CHILD(0).calculateMultiplyIndex(reducables[i] - i_erased, eo, true);
								}
							}
						}
					}
					if(CHILD(0).size() == 0) {
						setToChild(2, true, mparent, index_this + 1);
					} else if(CHILD(0).size() == 1) {
						CHILD(0).setToChild(1, true, this, 1);
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
					} else {
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
					}
					return 1;
				}
				break;
			}
			case STRUCT_POWER: {
				if((*mnum)[1].isNumber() && (*mnum)[1].number().isReal()) {
					if((*mnum)[1].number().isPositive()) {
						Number nr((*mnum)[1].number());
						if(reducable((*mnum)[0], *mden, nr)) {
							if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
							if(nr != (*mnum)[1].number()) {
								MathStructure mnum2((*mnum)[0]);
								if(mnum == this) {
									CHILD(1).number() -= nr;
									if(CHILD(1).number().isOne()) {
										set(mnum2);
									} else {
										calculateRaiseExponent(eo);
									}
									mstruct.ref();
									transform_nocopy(STRUCT_MULTIPLICATION, &mstruct);
									reduce(mnum2, CHILD(1)[0], nr, eo);
									calculateMultiplyLast(eo);
								} else {
									transform(STRUCT_MULTIPLICATION);
									PREPEND(mstruct);
									CHILD(0)[1].number() -= nr;
									if(CHILD(0)[1].number().isOne()) {
										CHILD(0) = mnum2;
									} else {
										CHILD(0).calculateRaiseExponent(eo);
									}
									reduce(mnum2, CHILD(1)[0], nr, eo);
									calculateMultiplyIndex(0, eo);
								}
							} else {
								if(mnum == this) {
									MathStructure mnum2((*mnum)[0]);
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
										reduce(mnum2, (*mparent)[index_this][0], nr, eo);
									} else {
										set_nocopy(mstruct, true);
										reduce(mnum2, CHILD(0), nr, eo);
									}
								} else {
									reduce((*mnum)[0], CHILD(0), nr, eo);
								}
							}
							return 1;
						}
					}
					break;
				}
			}
			default: {
				Number nr(1, 1);
				if(reducable(*mnum, *mden, nr)) {
					if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
					if(mnum == this) {
						MathStructure mnum2(*mnum);
						if(!nr.isOne()) {
							reduce(*mnum, mstruct[0], nr, eo);
							mstruct.calculateRaiseExponent(eo);
							nr.negate();
							nr++;
							calculateRaise(nr, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						} else if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
							reduce(mnum2, (*mparent)[index_this][0], nr, eo);
							(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
						} else {
							set_nocopy(mstruct, true);
							reduce(mnum2, CHILD(0), nr, eo);
							calculateRaiseExponent(eo, mparent, index_this);
						}
					} else {
						reduce(*mnum, CHILD(0), nr, eo);
						if(!nr.isOne()) {
							calculateRaiseExponent(eo);
							nr.negate();
							nr++;
							mstruct.calculateRaise(nr, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						} else {
							calculateRaiseExponent(eo, mparent, index_this);
						}
					}
					return 1;
				}
			}
		}
	}

	if(mstruct.isFunction() && eo.protected_function != mstruct.function()) {
		if(((mstruct.function() == CALCULATOR->f_abs && mstruct.size() == 1 && mstruct[0].isFunction() && mstruct[0].function() == CALCULATOR->f_signum && mstruct[0].size() == 2) || (mstruct.function() == CALCULATOR->f_signum && mstruct.size() == 2 && mstruct[0].isFunction() && mstruct[0].function() == CALCULATOR->f_abs && mstruct[0].size() == 1)) && (equals(mstruct[0][0]) || (isFunction() && o_function == CALCULATOR->f_abs && SIZE == 1 && CHILD(0) == mstruct[0][0]) || (isPower() && CHILD(0) == mstruct[0][0]) || (isPower() && CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_abs && CHILD(0).size() == 1 && CHILD(0)[0] == mstruct[0][0]))) {
				// sgn(abs(x))*x^y=x^y
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
		} else if(mstruct.function() == CALCULATOR->f_signum && mstruct.size() == 2) {
			if(equals(mstruct[0]) && representsReal(true)) {
				// sgn(x)*x=abs(x)
				transform(CALCULATOR->f_abs);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(isPower() && CHILD(1).representsOdd() && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
				//sgn(x)*x^3=abs(x)^3
				CHILD(0).transform(CALCULATOR->f_abs);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(mstruct.function() == CALCULATOR->f_root && VALID_ROOT(mstruct)) {
			if(equals(mstruct[0]) && mstruct[0].representsReal(true) && mstruct[1].number().isOdd()) {
				// root(x, 3)*x=abs(x)^(1/3)*x
				mstruct[0].transform(STRUCT_FUNCTION);
				mstruct[0].setFunction(CALCULATOR->f_abs);
				mstruct[1].number().recip();
				mstruct.setType(STRUCT_POWER);
				transform(STRUCT_FUNCTION);
				setFunction(CALCULATOR->f_abs);
				mstruct.ref();
				multiply_nocopy(&mstruct);
				calculateMultiplyLast(eo);
				return 1;
			} else if(isPower() && CHILD(1).representsOdd() && CHILD(0) == mstruct[0] && CHILD(0).representsReal(true) && mstruct[1].number().isOdd()) {
				// root(x, 3)*x^3=abs(x)^(1/3)*x^3
				mstruct[0].transform(STRUCT_FUNCTION);
				mstruct[0].setFunction(CALCULATOR->f_abs);
				mstruct[1].number().recip();
				mstruct.setType(STRUCT_POWER);
				CHILD(0).transform(STRUCT_FUNCTION);
				CHILD(0).setFunction(CALCULATOR->f_abs);
				mstruct.ref();
				multiply_nocopy(&mstruct);
				calculateMultiplyLast(eo);
				return 1;
			} else if(isPower() && CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_abs && CHILD(0).size() == 1 && CHILD(0)[0].equals(mstruct[0]) && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().isFraction() && CHILD(1).number().denominator() == mstruct[1].number() && CHILD(0).representsReal(true) && CHILD(1).number().numerator() == mstruct[1].number() - 1) {
				// root(x, 3)*abs(x)^(2/3)=x
				SET_CHILD_MAP(0)
				SET_CHILD_MAP(0)
				return 1;
			}
		} else if(eo.transform_trigonometric_functions && mstruct.function() == CALCULATOR->f_sinc && mstruct.size() == 1 && equals(mstruct[0])) {
			// sinc(x)*x=sin(x)
			calculateMultiply(CALCULATOR->getRadUnit(), eo);
			transform(CALCULATOR->f_sin);
			if(eo.calculate_functions) calculateFunctions(eo, false);
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		}
	}
	if(isZero()) {
		if(mstruct.isFunction()) {
			if((mstruct.function() == CALCULATOR->f_ln || mstruct.function() == CALCULATOR->f_Ei) && mstruct.size() == 1) {
				if(mstruct[0].representsNonZero() || warn_about_assumed_not_value(mstruct[0], m_zero, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(mstruct.function() == CALCULATOR->f_li && mstruct.size() == 1) {
				if(mstruct.representsNumber(true) || warn_about_assumed_not_value(mstruct[0], m_one, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			}
		} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[1].representsNumber()) {
			if((mstruct[0].function() == CALCULATOR->f_ln || mstruct[0].function() == CALCULATOR->f_Ei) && mstruct[0].size() == 1) {
				if(mstruct[0][0].representsNonZero() || warn_about_assumed_not_value(mstruct[0][0], m_zero, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(mstruct[0].function() == CALCULATOR->f_li && mstruct[0].size() == 1) {
				if(mstruct[0].representsNumber(true) || warn_about_assumed_not_value(mstruct[0][0], m_one, eo)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			}
		}
	}

	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {
					return 0;
				}
				case STRUCT_VECTOR: {
					if(isMatrix() && mstruct.isMatrix()) {
						if(CHILD(0).size() != mstruct.size()) {
							CALCULATOR->error(true, _("The second matrix must have as many rows (was %s) as the first has columns (was %s) for matrix multiplication."), i2s(mstruct.size()).c_str(), i2s(CHILD(0).size()).c_str(), NULL);
							return -1;
						}
						MathStructure msave(*this);
						size_t rows = SIZE;
						clearMatrix(true);
						resizeMatrix(rows, mstruct[0].size(), m_zero);
						MathStructure mtmp;
						for(size_t index_r = 0; index_r < SIZE; index_r++) {
							for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
								for(size_t index = 0; index < msave[0].size(); index++) {
									mtmp = msave[index_r][index];
									mtmp.calculateMultiply(mstruct[index][index_c], eo);
									CHILD(index_r)[index_c].calculateAdd(mtmp, eo, &CHILD(index_r), index_c);
								}
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(isMatrix() && mstruct.isVector()) {
						if(SIZE != mstruct.size() || CHILD(0).size() != 1) {
							CALCULATOR->error(true, _("The second matrix must have as many rows (was %s) as the first has columns (was %s) for matrix multiplication."), i2s(1).c_str(), i2s(CHILD(0).size()).c_str(), NULL);
							return -1;
						}
						MathStructure msave(*this);
						size_t rows = SIZE;
						clearMatrix(true);
						resizeMatrix(rows, mstruct.size(), m_zero);
						MathStructure mtmp;
						for(size_t index_r = 0; index_r < SIZE; index_r++) {
							for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
								CHILD(index_r)[index_c].set(msave[index_r][0]);
								CHILD(index_r)[index_c].calculateMultiply(mstruct[index_c], eo);
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else {
						if(SIZE == mstruct.size()) {
							for(size_t i = 0; i < SIZE; i++) {
								mstruct[i].ref();
								CHILD(i).multiply_nocopy(&mstruct[i], true);
								CHILD(i).calculateMultiplyLast(eo, true);
							}
							m_type = STRUCT_ADDITION;
							MERGE_APPROX_AND_PREC(mstruct)
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						}
					}
					return -1;
				}
				default: {
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculateMultiply(mstruct, eo);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
		}
		case STRUCT_ADDITION: {
			if(eo.expand != 0 && containsType(STRUCT_DATETIME, false, true, false) > 0) return -1;
			switch(mstruct.type()) {
				case STRUCT_ADDITION: {
					if(eo.expand != 0 && SIZE < 1000 && mstruct.size() < 1000 && (SIZE * mstruct.size() < (eo.expand == -1 ? 50 : 500))) {

						if(eo.expand > -2 || (!containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0)) || (representsNonNegative(true) && mstruct.representsNonNegative(true))) {
							MathStructure msave(*this);
							CLEAR;
							for(size_t i = 0; i < mstruct.size(); i++) {
								if(CALCULATOR->aborted()) {
									set(msave);
									return -1;
								}
								APPEND(msave);
								mstruct[i].ref();
								LAST.multiply_nocopy(&mstruct[i], true);
								if(reversed) {
									LAST.swapChildren(1, LAST.size());
									LAST.calculateMultiplyIndex(0, eo, true, this, SIZE - 1);
								} else {
									LAST.calculateMultiplyLast(eo, true, this, SIZE - 1);
								}
							}
							MERGE_APPROX_AND_PREC(mstruct)
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(eo.expand <= -2 && (!mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) || representsNonNegative(true))) {
							for(size_t i = 0; i < SIZE; i++) {
								CHILD(i).calculateMultiply(mstruct, eo, this, i);
							}
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(eo.expand <= -2 && (!containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) || mstruct.representsNonNegative(true))) {
							return 0;
						}
					}
					if(equals(mstruct)) {
						raise_nocopy(new MathStructure(2, 1, 0));
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					break;
				}
				case STRUCT_POWER: {
					if(mstruct[1].isNumber() && *this == mstruct[0]) {
						if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true))
						|| (mstruct[1].isNumber() && mstruct[1].number().isReal() && !mstruct[1].number().isMinusOne())
						|| representsNonZero(true)
						|| mstruct[1].representsPositive()
						|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(*this, mstruct[1], eo))) {
							if(mparent) {
								mparent->swapChildren(index_this + 1, index_mstruct + 1);
								(*mparent)[index_this][1].number()++;
								(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
							} else {
								set_nocopy(mstruct, true);
								CHILD(1).number()++;
								calculateRaiseExponent(eo, mparent, index_this);
							}
							return 1;
						}
					}
					if(eo.expand == 0 && mstruct[0].isAddition()) return -1;
					if(eo.combine_divisions && mstruct[1].hasNegativeSign()) {
						int ret;
						vector<bool> merged;
						merged.resize(SIZE, false);
						size_t merges = 0;
						MathStructure *mstruct2 = new MathStructure(mstruct);
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isOne()) ret = -1;
							else ret = CHILD(i).merge_multiplication(*mstruct2, eo, NULL, 0, 0, false, false);
							if(ret == 0) {
								ret = mstruct2->merge_multiplication(CHILD(i), eo, NULL, 0, 0, true, false);
								if(ret >= 1) {
									mstruct2->ref();
									setChild_nocopy(mstruct2, i + 1);
								}
							}
							if(ret >= 1) {
								mstruct2->unref();
								if(i + 1 != SIZE) mstruct2 = new MathStructure(mstruct);
								merged[i] = true;
								merges++;
							} else {
								if(i + 1 == SIZE) mstruct2->unref();
								merged[i] = false;
							}
						}
						if(merges == 0) {
							return -1;
						} else if(merges == SIZE) {
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(merges == SIZE - 1) {
							for(size_t i = 0; i < SIZE; i++) {
								if(!merged[i]) {
									mstruct.ref();
									CHILD(i).multiply_nocopy(&mstruct, true);
									break;
								}
							}
							calculatesub(eo, eo, false, mparent, index_this);
						} else {
							MathStructure *mdiv = new MathStructure();
							merges = 0;
							for(size_t i = 0; i - merges < SIZE; i++) {
								if(!merged[i]) {
									CHILD(i - merges).ref();
									if(merges > 0) {
										(*mdiv)[0].add_nocopy(&CHILD(i - merges), merges > 1);
									} else {
										mdiv->multiply(mstruct);
										mdiv->setChild_nocopy(&CHILD(i - merges), 1);
									}
									ERASE(i - merges);
									merges++;
								}
							}
							add_nocopy(mdiv, true);
							calculatesub(eo, eo, false);
						}
						return 1;
					}
					if(eo.expand == 0 || (eo.expand < -1 && mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !representsNonNegative(true))) return -1;
				}
				case STRUCT_MULTIPLICATION: {
					if(do_append && (eo.expand == 0 || (eo.expand < -1 && mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !representsNonNegative(true)))) {
						transform(STRUCT_MULTIPLICATION);
						for(size_t i = 0; i < mstruct.size(); i++) {
							APPEND_REF(&mstruct[i]);
						}
						return 1;
					}
				}
				default: {
					if(eo.expand == 0 || (eo.expand < -1 && mstruct.containsInterval(true, false, false, eo.expand == -2 ? 1 : 0) && !representsNonNegative(true))) return -1;
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).multiply(mstruct, true);
						if(reversed) {
							CHILD(i).swapChildren(1, CHILD(i).size());
							CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
						} else {
							CHILD(i).calculateMultiplyLast(eo, true, this, i);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
			return -1;
		}
		case STRUCT_MULTIPLICATION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {
					if(eo.expand == 0 || containsType(STRUCT_DATETIME, false, true, false) > 0) {
						if(!do_append) return -1;
						APPEND_REF(&mstruct);
						return 1;
					}
					return 0;
				}
				case STRUCT_MULTIPLICATION: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(reversed) {
							PREPEND_REF(&mstruct[i]);
							calculateMultiplyIndex(0, eo, false);
						} else {
							APPEND_REF(&mstruct[i]);
							calculateMultiplyLast(eo, false);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
				case STRUCT_POWER: {
					if(mstruct[1].isNumber()) {
						if(*this == mstruct[0]) {
							if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true))
							|| (mstruct[1].isNumber() && mstruct[1].number().isReal() && !mstruct[1].number().isMinusOne())
							|| representsNonZero(true)
							|| mstruct[1].representsPositive()
							|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(*this, mstruct[1], eo))) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
									(*mparent)[index_this][1].number()++;
									(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
								} else {
									set_nocopy(mstruct, true);
									CHILD(1).number()++;
									calculateRaiseExponent(eo, mparent, index_this);
								}
								return 1;
							}
						} else {
							for(size_t i = 0; i < SIZE; i++) {
								int ret = CHILD(i).merge_multiplication(mstruct, eo, NULL, 0, 0, false, false);
								if(ret == 0) {
									ret = mstruct.merge_multiplication(CHILD(i), eo, NULL, 0, 0, true, false);
									if(ret >= 1) {
										if(ret == 2) ret = 3;
										else if(ret == 3) ret = 2;
										mstruct.ref();
										setChild_nocopy(&mstruct, i + 1);
									}
								}
								if(ret >= 1) {
									if(ret != 2) calculateMultiplyIndex(i, eo, true, mparent, index_this);
									return 1;
								}
							}
						}
					}
				}
				default: {
					if(do_append) {
						MERGE_APPROX_AND_PREC(mstruct)
						if(reversed) {
							PREPEND_REF(&mstruct);
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
						} else {
							APPEND_REF(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						}
						return 1;
					}
				}
			}
			return -1;
		}
		case STRUCT_POWER: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {
					return 0;
				}
				case STRUCT_POWER: {
					if(CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_abs && mstruct[0].isFunction() && mstruct[0].function() == CALCULATOR->f_root && mstruct[1].isMinusOne() && CHILD(0).size() == 1 && VALID_ROOT(mstruct[0]) && CHILD(0)[0].equals(mstruct[0][0]) && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().isFraction() && CHILD(1).number().denominator() == mstruct[0][1].number() && CHILD(0)[0].representsReal(true) && CHILD(1).number().numerator() == -(mstruct[0][1].number() - 1)) {
						// root(x, 3)^-1*abs(x)^(-2/3)=1/x
						SET_CHILD_MAP(0)
						SET_CHILD_MAP(0)
						raise(m_minus_one);
						return 1;
					}
					if(mstruct[0].isFunction() && mstruct[0].function() == CALCULATOR->f_abs && CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_root && CHILD(1).isMinusOne() && mstruct[0].size() == 1 && VALID_ROOT(CHILD(0)) && CHILD(0)[0].equals(mstruct[0][0]) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().isFraction() && mstruct[1].number().denominator() == CHILD(0)[1].number() && mstruct[0][0].representsReal(true) && mstruct[1].number().numerator() == -(CHILD(0)[1].number() - 1)) {
						// root(x, 3)^-1*abs(x)^(-2/3)=1/x
						SET_CHILD_MAP(0)
						SET_CHILD_MAP(0)
						raise(m_minus_one);
						return 1;
					}
					if(mstruct[0] == CHILD(0) || (CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isMinusOne() && CHILD(0)[1] == mstruct[0] && mstruct[1].representsEven())) {
						if(mstruct[0].isUnit() && mstruct[0].prefix()) CHILD(0).setPrefix(mstruct[0].prefix());
						bool b = eo.allow_complex || CHILD(0).representsNonNegative(true), b2 = true, b_warn = false;
						if(!b) {
							b = CHILD(1).representsInteger() && mstruct[1].representsInteger();
						}
						if(b) {
							b = false;
							bool b2test = false;
							if(IS_REAL(mstruct[1]) && IS_REAL(CHILD(1))) {
								if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
									b2 = true;
									b = true;
								} else if(!mstruct[1].number().isMinusOne() && !CHILD(1).number().isMinusOne()) {
									b2 = (mstruct[1].number() + CHILD(1).number()).isNegative();
									b = true;
									if(!b2) b2test = true;
								}
							}
							if(!b || b2test) {
								b = (!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))
								|| CHILD(0).representsNonZero(true)
								|| (CHILD(1).representsPositive() && mstruct[1].representsPositive())
								|| (CHILD(1).representsNegative() && mstruct[1].representsNegative());
								if(!b && eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) {
									b = true;
									b_warn = true;
								}
								if(b2test) {
									b2 = b;
									b = true;
								}
							}
						}
						if(b) {
							if(IS_REAL(CHILD(1)) && IS_REAL(mstruct[1])) {
								if(!b2 && !do_append) return -1;
								if(b_warn && !warn_about_denominators_assumed_nonzero(CHILD(0), eo)) return -1;
								if(b2) {
									CHILD(1).number() += mstruct[1].number();
									calculateRaiseExponent(eo, mparent, index_this);
								} else {
									if(CHILD(1).number().isNegative()) {
										CHILD(1).number()++;
										mstruct[1].number() += CHILD(1).number();
										CHILD(1).number().set(-1, 1, 0);
									} else {
										mstruct[1].number()++;
										CHILD(1).number() += mstruct[1].number();
										mstruct[1].number().set(-1, 1, 0);
									}
									MERGE_APPROX_AND_PREC(mstruct)
									transform(STRUCT_MULTIPLICATION);
									CHILD(0).calculateRaiseExponent(eo, this, 0);
									if(reversed) {
										PREPEND_REF(&mstruct);
										CHILD(0).calculateRaiseExponent(eo, this, 0);
										calculateMultiplyIndex(0, eo, true, mparent, index_this);
									} else {
										APPEND_REF(&mstruct);
										CHILD(1).calculateRaiseExponent(eo, this, 1);
										calculateMultiplyLast(eo, true, mparent, index_this);
									}
								}
								return 1;
							} else {
								MathStructure mstruct2(CHILD(1));
								if(mstruct2.calculateAdd(mstruct[1], eo)) {
									if(b_warn && !warn_about_denominators_assumed_nonzero_llgg(CHILD(0), CHILD(1), mstruct[1], eo)) return -1;
									CHILD(1) = mstruct2;
									calculateRaiseExponent(eo, mparent, index_this);
									return 1;
								}
							}
						}
					} else if(mstruct[1] == CHILD(1)) {
						if(!CHILD(0).isMultiplication() && !mstruct[0].isMultiplication() && (mstruct[1].representsInteger() || CHILD(0).representsPositive(true) || mstruct[0].representsPositive(true))) {
							MathStructure mstruct2(CHILD(0));
							if(mstruct2.calculateMultiply(mstruct[0], eo)) {
								CHILD(0) = mstruct2;
								MERGE_APPROX_AND_PREC(mstruct)
								calculateRaiseExponent(eo, mparent, index_this);
								return 1;
							}
						} else if(eo.transform_trigonometric_functions && CHILD(1).representsInteger() && CHILD(0).isFunction() && mstruct[0].isFunction() && eo.protected_function != mstruct[0].function() && eo.protected_function != CHILD(0).function() && CHILD(0).size() == 1 && mstruct[0].size() == 1 && CHILD(0)[0] == mstruct[0][0]) {
							if((CHILD(0).function() == CALCULATOR->f_cos && mstruct[0].function() == CALCULATOR->f_sin) || (CHILD(0).function() == CALCULATOR->f_sin && mstruct[0].function() == CALCULATOR->f_cos) || (CHILD(0).function() == CALCULATOR->f_cosh && mstruct[0].function() == CALCULATOR->f_sinh) || (CHILD(0).function() == CALCULATOR->f_sinh && mstruct[0].function() == CALCULATOR->f_cosh)) {
								// cos(x)^n*sin(x)^n=sin(2x)^n/2^n
								if(CHILD(0).function() == CALCULATOR->f_cosh) CHILD(0).setFunction(CALCULATOR->f_sinh);
								else if(CHILD(0).function() == CALCULATOR->f_cos) CHILD(0).setFunction(CALCULATOR->f_sin);
								CHILD(0)[0].calculateMultiply(nr_two, eo);
								CHILD(0).childUpdated(1);
								CHILD_UPDATED(0)
								MathStructure *mdiv = new MathStructure(2, 1, 0);
								mdiv->calculateRaise(CHILD(1), eo);
								mdiv->calculateInverse(eo);
								multiply_nocopy(mdiv);
								calculateMultiplyLast(eo);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if((CHILD(0).function() == CALCULATOR->f_tan && mstruct[0].function() == CALCULATOR->f_cos) || (CHILD(0).function() == CALCULATOR->f_cos && mstruct[0].function() == CALCULATOR->f_tan)) {
								// tan(x)^n*cos(x)^n=sin(x)^n
								CHILD(0).setFunction(CALCULATOR->f_sin);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if((CHILD(0).function() == CALCULATOR->f_tanh && mstruct[0].function() == CALCULATOR->f_cosh) || (CHILD(0).function() == CALCULATOR->f_cosh && mstruct[0].function() == CALCULATOR->f_tanh)) {
								// tanh(x)^n*cosh(x)^n=sinh(x)^n
								CHILD(0).setFunction(CALCULATOR->f_sin);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					} else if(eo.transform_trigonometric_functions && CHILD(1).isInteger() && mstruct[1].isInteger() && CHILD(0).isFunction() && mstruct[0].isFunction() && eo.protected_function != mstruct[0].function() && eo.protected_function != CHILD(0).function() && mstruct[0].size() == 1 && CHILD(0).size() == 1 && CHILD(0)[0] == mstruct[0][0]) {
						if(CHILD(1).number().isNonNegative() != mstruct[1].number().isNonNegative()) {
							if(CHILD(0).function() == CALCULATOR->f_sin) {
								if(mstruct[0].function() == CALCULATOR->f_cos) {
									CHILD(0).setFunction(CALCULATOR->f_tan);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunction(CALCULATOR->f_sin);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								} else if(mstruct[0].function() == CALCULATOR->f_tan) {
									CHILD(0).setFunction(CALCULATOR->f_cos);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunction(CALCULATOR->f_sin);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function() == CALCULATOR->f_cos) {
								if(mstruct[0].function() == CALCULATOR->f_sin) {
									mstruct[0].setFunction(CALCULATOR->f_tan);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunction(CALCULATOR->f_sin);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function() == CALCULATOR->f_tan) {
								if(mstruct[0].function() == CALCULATOR->f_sin) {
									mstruct[0].setFunction(CALCULATOR->f_cos);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunction(CALCULATOR->f_sin);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function() == CALCULATOR->f_sinh) {
								if(mstruct[0].function() == CALCULATOR->f_cosh) {
									CHILD(0).setFunction(CALCULATOR->f_tanh);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunction(CALCULATOR->f_sinh);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								} else if(mstruct[0].function() == CALCULATOR->f_tanh) {
									CHILD(0).setFunction(CALCULATOR->f_cosh);
									mstruct[1].number() += CHILD(1).number();
									if(mstruct[1].number().isZero()) {
										MERGE_APPROX_AND_PREC(mstruct)
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										mstruct[0].setFunction(CALCULATOR->f_sinh);
										CHILD(1).number() -= mstruct[1].number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function() == CALCULATOR->f_cosh) {
								if(mstruct[0].function() == CALCULATOR->f_sinh) {
									mstruct[0].setFunction(CALCULATOR->f_tanh);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunction(CALCULATOR->f_sinh);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							} else if(CHILD(0).function() == CALCULATOR->f_tanh) {
								if(mstruct[0].function() == CALCULATOR->f_sinh) {
									mstruct[0].setFunction(CALCULATOR->f_cosh);
									CHILD(1).number() += mstruct[1].number();
									if(CHILD(1).number().isZero()) {
										set(mstruct, true);
										return 1;
									} else if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
										CHILD(0).setFunction(CALCULATOR->f_sinh);
										mstruct[1].number() -= CHILD(1).number();
									}
									mstruct.ref();
									multiply_nocopy(&mstruct);
									calculateMultiplyLast(eo);
									return 1;
								}
							}
						} else {
							if((CHILD(0).function() == CALCULATOR->f_tan && mstruct[0].function() == CALCULATOR->f_cos)) {
								// tan(x)^n*cos(x)^m=sin(x)^n*cos(x)^(m-n)
								CHILD(0).setFunction(CALCULATOR->f_sin);
								mstruct[1].number() -= CHILD(1).number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if((CHILD(0).function() == CALCULATOR->f_cos && mstruct[0].function() == CALCULATOR->f_tan)) {
								// tan(x)^n*cos(x)^m=sin(x)^n*cos(x)^(m-n)
								mstruct[0].setFunction(CALCULATOR->f_sin);
								CHILD(1).number() -= mstruct[1].number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if((CHILD(0).function() == CALCULATOR->f_tanh && mstruct[0].function() == CALCULATOR->f_cosh)) {
								// tanh(x)^n*cosh(x)^m=sinh(x)^n*cosh(x)^(m-n)
								CHILD(0).setFunction(CALCULATOR->f_sinh);
								mstruct[1].number() -= CHILD(1).number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if((CHILD(0).function() == CALCULATOR->f_cosh && mstruct[0].function() == CALCULATOR->f_tanh)) {
								// tanh(x)^n*cosh(x)^m=sinh(x)^n*cosh(x)^(m-n)
								mstruct[0].setFunction(CALCULATOR->f_sinh);
								CHILD(1).number() -= mstruct[1].number();
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					} else if(mstruct[0].isMultiplication() && mstruct[0].size() == 2 && mstruct[0][0].isMinusOne() && mstruct[0][1] == CHILD(0) && CHILD(1).representsEven()) {
						return 0;
					}
					break;
				}
				case STRUCT_FUNCTION: {
					if(eo.protected_function != CALCULATOR->f_signum && mstruct.function() == CALCULATOR->f_signum && mstruct.size() == 2 && CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_abs && CHILD(0).size() == 1 && mstruct[0] == CHILD(0)[0] && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().numeratorIsOne() && !CHILD(1).number().denominatorIsEven() && CHILD(0)[0].representsReal(true)) {
						setType(STRUCT_FUNCTION);
						setFunction(CALCULATOR->f_root);
						CHILD(0).setToChild(1, true);
						CHILD(1).number().recip();
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					if(eo.transform_trigonometric_functions && CHILD(0).isFunction() && CHILD(0).size() == 1 && mstruct.size() == 1 && CHILD(1).isNumber() && CHILD(1).number().isNegative() && eo.protected_function != mstruct.function() && eo.protected_function != CHILD(0).function()) {
						if(CHILD(0).function() == CALCULATOR->f_sin) {
							if(mstruct.function() == CALCULATOR->f_cos && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_tan);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_tan);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if(mstruct.function() == CALCULATOR->f_tan && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_cos);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_cos);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function() == CALCULATOR->f_cos) {
							if(mstruct.function() == CALCULATOR->f_sin && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_tan);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_tan);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function() == CALCULATOR->f_tan) {
							if(mstruct.function() == CALCULATOR->f_sin && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_cos);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_cos);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function() == CALCULATOR->f_sinh) {
							if(mstruct.function() == CALCULATOR->f_cosh && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_tanh);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_tanh);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							} else if(mstruct.function() == CALCULATOR->f_tanh && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_cosh);
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_cosh);
								mstruct.raise(nr_minus_one);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function() == CALCULATOR->f_cosh) {
							if(mstruct.function() == CALCULATOR->f_sinh && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_tanh);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_tanh);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						} else if(CHILD(0).function() == CALCULATOR->f_tanh) {
							if(mstruct.function() == CALCULATOR->f_sinh && CHILD(0)[0] == mstruct[0]) {
								if(CHILD(1).number().isMinusOne()) {
									CHILD(0).setFunction(CALCULATOR->f_cosh);
									SET_CHILD_MAP(0)
									MERGE_APPROX_AND_PREC(mstruct)
									return 1;
								}
								mstruct.setFunction(CALCULATOR->f_cosh);
								CHILD(1).number()++;
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					}
					if(mstruct.function() == CALCULATOR->f_stripunits && mstruct.size() == 1) {
						if(m_type == STRUCT_POWER && CHILD(0).isVariable() && CHILD(0).variable()->isKnown() && mstruct[0].contains(CHILD(0), false) > 0) {
							if(separate_unit_vars(CHILD(0), eo, false)) {
								calculateRaiseExponent(eo);
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					}
				}
				default: {
					if(!mstruct.isNumber() && CHILD(1).isNumber() && CHILD(0) == mstruct) {
						if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))
						|| (CHILD(1).isNumber() && CHILD(1).number().isReal() && !CHILD(1).number().isMinusOne())
						|| CHILD(0).representsNonZero(true)
						|| CHILD(1).representsPositive()
						|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(CHILD(0), CHILD(1), eo))) {
							CHILD(1).number()++;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo, mparent, index_this);
							return 1;
						}
					}
					if(mstruct.isNumber() && CHILD(1).isNumber() && !CHILD(1).number().includesInfinity() && CHILD(0).isNumber() && CHILD(0).number().isRational() && !CHILD(0).number().isZero() && mstruct.number().isRational()) {
						if(CHILD(0).isInteger() && mstruct.number().denominator() == CHILD(0).number().numerator()) {
							CHILD(1).number()--;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo);
							if(!mstruct.number().numeratorIsOne()) calculateMultiply(mstruct.number().numerator(), eo, mparent, index_this);
							return 1;
						} else if(mstruct.number().denominator() == CHILD(0).number().numerator() && mstruct.number().numerator() == CHILD(0).number().denominator()) {
							CHILD(1).number()--;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo);
							return 1;
						}
					}
					if(mstruct.isZero() && (!eo.keep_zero_units || containsType(STRUCT_UNIT, false, true, true) <= 0 || (CHILD(0).isUnit() && CHILD(0).unit() == CALCULATOR->getRadUnit()) || (CHILD(0).isFunction() && CHILD(0).representsNumber(false))) && !representsUndefined(true, true, !eo.assume_denominators_nonzero) && representsNonMatrix()) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					if(CHILD(0).isFunction() && mstruct.isZero() && CHILD(1).representsNumber()) {
						if((CHILD(0).function() == CALCULATOR->f_ln || CHILD(0).function() == CALCULATOR->f_Ei) && SIZE == 1) {
							if(CHILD(0)[0].representsNonZero() || warn_about_assumed_not_value(CHILD(0)[0], m_zero, eo)) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 3;
							}
						} else if(CHILD(0).function() == CALCULATOR->f_li && SIZE == 1) {
							if(CHILD(0).representsNumber(true) || warn_about_assumed_not_value(CHILD(0)[0], m_one, eo)) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 3;
							}
						}
					}
					break;
				}
			}
			return -1;
		}
		case STRUCT_FUNCTION: {
			if(eo.protected_function != o_function) {
				if(((o_function == CALCULATOR->f_abs && SIZE == 1 && CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_signum && CHILD(0).size() == 2) || (o_function == CALCULATOR->f_signum && SIZE == 2 && CHILD(0).isFunction() && CHILD(0).function() == CALCULATOR->f_abs && CHILD(0).size() == 1)) && (CHILD(0)[0] == mstruct || (mstruct.isFunction() && mstruct.function() == CALCULATOR->f_abs && mstruct.size() == 1 && CHILD(0)[0] == mstruct[0]) || (mstruct.isPower() && mstruct[0] == CHILD(0)[0]) || (mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function() == CALCULATOR->f_abs && mstruct[0].size() == 1 && CHILD(0)[0] == mstruct[0][0]))) {
					// sgn(abs(x))*x^y=x^y
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				} else if(o_function == CALCULATOR->f_abs && SIZE == 1) {
					if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_abs && mstruct.size() == 1 && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
						// abs(x)*abs(x)=x^2
						SET_CHILD_MAP(0)
						MERGE_APPROX_AND_PREC(mstruct)
						calculateRaise(nr_two, eo);
						return 1;
					} else if(mstruct.isFunction() && eo.protected_function != mstruct.function() && mstruct.function() == CALCULATOR->f_signum && mstruct.size() == 2 && mstruct[0] == CHILD(0) && CHILD(0).representsScalar()) {
						// sgn(x)*abs(x)=x
						SET_CHILD_MAP(0)
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				} else if(o_function == CALCULATOR->f_signum && SIZE == 2) {
					if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_signum && mstruct.size() == 2 && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
						if(mstruct[1].isOne() && CHILD(1).isOne()) {
							set(1, 1, 0, true);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct[1] == CHILD(1)) {
							// sgn(x)*sgn(x)=sgn(abs(x))
							CHILD(0).transform(CALCULATOR->f_abs);
							CHILD_UPDATED(0)
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(mstruct.isFunction() && eo.protected_function != mstruct.function() && mstruct.function() == CALCULATOR->f_abs && mstruct.size() == 1 && mstruct[0] == CHILD(0) && CHILD(0).representsScalar()) {
						// sgn(x)*abs(x)=x
						SET_CHILD_MAP(0)
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(mstruct == CHILD(0) && CHILD(0).representsReal(true)) {
						// sgn(x)*x=abs(x)
						setFunction(CALCULATOR->f_abs);
						ERASE(1)
						return 1;
					} else if(mstruct.isPower() && mstruct[1].representsOdd() && mstruct[0] == CHILD(0) && CHILD(0).representsReal(true)) {
						//sgn(x)*x^3=abs(x)^3
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
						if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
						} else {
							set_nocopy(mstruct, true);
						}
						return 1;
					}
				} else if(o_function == CALCULATOR->f_root && THIS_VALID_ROOT) {
					if(CHILD(0) == mstruct && CHILD(0).representsReal(true) && CHILD(1).number().isOdd()) {
						// root(x, 3)*x=abs(x)^(1/3)*x
						CHILD(0).transform(STRUCT_FUNCTION);
						CHILD(0).setFunction(CALCULATOR->f_abs);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct.transform(STRUCT_FUNCTION);
						mstruct.setFunction(CALCULATOR->f_abs);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					} else if(mstruct.isPower() && mstruct[1].representsOdd() && CHILD(0) == mstruct[0] && CHILD(0).representsReal(true) && CHILD(1).number().isOdd()) {
						// root(x, 3)*x^3=abs(x)^(1/3)*x^3
						CHILD(0).transform(STRUCT_FUNCTION);
						CHILD(0).setFunction(CALCULATOR->f_abs);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_root && VALID_ROOT(mstruct) && CHILD(0) == mstruct[0] && CHILD(0).representsReal(true) && CHILD(1).number().isOdd() && mstruct[1].number().isOdd()) {
						// root(x, y)*root(x, z)=abs(x)^(1/y)*abs(x)^(1/z)
						CHILD(0).transform(STRUCT_FUNCTION);
						CHILD(0).setFunction(CALCULATOR->f_abs);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
						mstruct[1].number().recip();
						mstruct.setType(STRUCT_POWER);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[0].function() == CALCULATOR->f_abs && mstruct[0].size() == 1 && mstruct[0][0].equals(CHILD(0)) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().isFraction() && mstruct[0].number().denominator() == CHILD(1).number() && CHILD(0).representsReal(true) && mstruct[1].number().numerator() == CHILD(1).number() - 1) {
						// root(x, 3)*abs(x)^(2/3)=x
						SET_CHILD_MAP(0)
						return 1;
					}
				} else if((o_function == CALCULATOR->f_ln || o_function == CALCULATOR->f_Ei) && SIZE == 1 && mstruct.isZero()) {
					if(CHILD(0).representsNonZero() || warn_about_assumed_not_value(CHILD(0), m_zero, eo)) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
				} else if(o_function == CALCULATOR->f_li && SIZE == 1 && mstruct.isZero()) {
					if(representsNumber(true) || warn_about_assumed_not_value(CHILD(0), m_one, eo)) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
				} else if(eo.transform_trigonometric_functions && o_function == CALCULATOR->f_sinc && SIZE == 1 && CHILD(0) == mstruct) {
					// sinc(x)*x=sin(x)
					CHILD(0).calculateMultiply(CALCULATOR->getRadUnit(), eo);
					CHILD_UPDATED(0)
					setFunction(CALCULATOR->f_sin);
					if(eo.calculate_functions) calculateFunctions(eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(eo.transform_trigonometric_functions && mstruct.isFunction() && mstruct.size() == 1 && eo.protected_function != mstruct.function()) {
					if(o_function == CALCULATOR->f_tan && SIZE == 1) {
						if(mstruct.function() == CALCULATOR->f_cos && mstruct[0] == CHILD(0)) {
							// tan(x)*cos(x)=sin(x)
							setFunction(CALCULATOR->f_sin);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function == CALCULATOR->f_cos && SIZE == 1) {
						if(mstruct.function() == CALCULATOR->f_tan && mstruct[0] == CHILD(0)) {
							// tan(x)*cos(x)=sin(x)
							setFunction(CALCULATOR->f_sin);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct.function() == CALCULATOR->f_sin && mstruct[0] == CHILD(0)) {
							// cos(x)*sin(x)=sin(2x)/2
							setFunction(CALCULATOR->f_sin);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function == CALCULATOR->f_sin && SIZE == 1) {
						if(mstruct.function() == CALCULATOR->f_cos && mstruct[0] == CHILD(0)) {
							// cos(x)*sin(x)=sin(2x)/2
							setFunction(CALCULATOR->f_sin);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function == CALCULATOR->f_tanh && SIZE == 1) {
						if(mstruct.function() == CALCULATOR->f_cosh && mstruct[0] == CHILD(0)) {
							// tanh(x)*cosh(x)=sinh(x)
							setFunction(CALCULATOR->f_sinh);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function == CALCULATOR->f_sinh && SIZE == 1) {
						if(mstruct.function() == CALCULATOR->f_cosh && mstruct[0] == CHILD(0)) {
							// cosh(x)*sinh(x)=sinh(2x)/2
							setFunction(CALCULATOR->f_sinh);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(o_function == CALCULATOR->f_cosh && SIZE == 1) {
						if(mstruct.function() == CALCULATOR->f_tanh && mstruct[0] == CHILD(0)) {
							// tanh(x)*cosh(x)=sinh(x)
							setFunction(CALCULATOR->f_sinh);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct.function() == CALCULATOR->f_sinh && mstruct[0] == CHILD(0)) {
							// cosh(x)*sinh(x)=sinh(2x)/2
							setFunction(CALCULATOR->f_sinh);
							CHILD(0).calculateMultiply(nr_two, eo);
							CHILD_UPDATED(0)
							calculateDivide(nr_two, eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				}
			}
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1) {
				if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_stripunits && mstruct.size() == 1) {
					mstruct[0].ref();
					CHILD(0).multiply_nocopy(&mstruct[0]);
					return 1;
				} else if(mstruct.isVariable() && mstruct.variable()->isKnown() && CHILD(0).contains(mstruct, false) > 0) {
					if(separate_unit_vars(mstruct, eo, false)) {
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					}
				} else if(mstruct.isPower() && mstruct[0].isVariable() && mstruct[0].variable()->isKnown() && CHILD(0).contains(mstruct[0], false) > 0) {
					if(separate_unit_vars(mstruct[0], eo, false)) {
						mstruct.calculateRaiseExponent(eo);
						mstruct.ref();
						multiply_nocopy(&mstruct);
						calculateMultiplyLast(eo);
						return 1;
					}
				}
			}
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {}
				case STRUCT_POWER: {
					return 0;
				}
				case STRUCT_COMPARISON: {
					if(isComparison()) {
						mstruct.ref();
						transform_nocopy(STRUCT_LOGICAL_AND, &mstruct);
						return 1;
					}
				}
				default: {
					if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_stripunits && mstruct.size() == 1) {
						if(m_type == STRUCT_VARIABLE && o_variable->isKnown() && mstruct[0].contains(*this, false) > 0) {
							if(separate_unit_vars(*this, eo, false)) {
								mstruct.ref();
								multiply_nocopy(&mstruct);
								calculateMultiplyLast(eo);
								return 1;
							}
						}
					}
					if(mstruct.isZero() && (!eo.keep_zero_units || containsType(STRUCT_UNIT, false, true, true) <= 0 || (isUnit() && unit() == CALCULATOR->getRadUnit()) || (isFunction() && representsNumber(false))) && !representsUndefined(true, true, !eo.assume_denominators_nonzero) && representsNonMatrix()) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
					if(isZero() && !mstruct.representsUndefined(true, true, !eo.assume_denominators_nonzero) && (!eo.keep_zero_units || mstruct.containsType(STRUCT_UNIT, false, true, true) <= 0 || (mstruct.isUnit() && mstruct.unit() == CALCULATOR->getRadUnit())) && mstruct.representsNonMatrix()) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 2;
					}
					if(equals(mstruct)) {
						if(mstruct.isUnit() && mstruct.prefix()) o_prefix = mstruct.prefix();
						raise_nocopy(new MathStructure(2, 1, 0));
						MERGE_APPROX_AND_PREC(mstruct)
						calculateRaiseExponent(eo, mparent, index_this);
						return 1;
					}
					break;
				}
			}
			break;
		}
	}
	return -1;
}

bool test_if_numerator_not_too_large(Number &vb, Number &ve) {
	if(!vb.isRational()) return false;
	if(!mpz_fits_slong_p(mpq_numref(ve.internalRational()))) return false;
	long int exp = labs(mpz_get_si(mpq_numref(ve.internalRational())));
	if(vb.isRational()) {
		if((long long int) exp * mpz_sizeinbase(mpq_numref(vb.internalRational()), 10) <= 1000000LL && (long long int) exp * mpz_sizeinbase(mpq_denref(vb.internalRational()), 10) <= 1000000LL) return true;
	}
	return false;
}

bool is_negation(const MathStructure &m1, const MathStructure &m2) {
	if(m1.isAddition() && m2.isAddition() && m1.size() == m2.size()) {
		for(size_t i = 0; i < m1.size(); i++) {
			if(!is_negation(m1[i], m2[i])) return false;
		}
		return true;
	}
	if(m1.isNumber() && m2.isNumber()) {
		return m1.number() == -m2.number();
	}
	if(m1.isMultiplication() && m1.size() > 1) {
		if(m1[0].isNumber()) {
			if(m1[0].number().isMinusOne()) {
				if(m1.size() == 2) return m1[1] == m2;
				if(m2.isMultiplication() && m2.size() == m1.size() - 1) {
					for(size_t i = 1; i < m1.size(); i++) {
						if(!m1[i].equals(m2[i - 1], true, true)) return false;
					}
					return true;
				}
				return false;
			} else {
				if(m2.isMultiplication() && m2.size() == m1.size() && m2[0].isNumber()) {
					for(size_t i = 1; i < m1.size(); i++) {
						if(!m1[i].equals(m2[i], true, true)) return false;
					}
					return m1[0].number().equals(-m2[0].number(), true, true);
				}
				return false;
			}
		}
	}
	if(m2.isMultiplication() && m2.size() > 1) {
		if(m2[0].isNumber()) {
			if(m2[0].number().isMinusOne()) {
				if(m2.size() == 2) return m2[1] == m1;
				if(m1.isMultiplication() && m1.size() == m2.size() - 1) {
					for(size_t i = 1; i < m2.size(); i++) {
						if(!m2[i].equals(m1[i - 1], true, true)) return false;
					}
					return true;
				}
				return false;
			}
		}
	}
	return false;
}

int MathStructure::merge_power(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {

	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		// number^number
		Number nr(o_number);
		if(nr.raise(mstruct.number(), eo.approximation < APPROXIMATION_APPROXIMATE) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			// Exponentiation succeeded without inappropriate change in approximation status
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		if(!o_number.isMinusOne() && !o_number.isOne() && mstruct.number().isRational() && !mstruct.isInteger()) {
			if(o_number.isNegative()) {
				MathStructure mtest(*this);
				if(mtest.number().negate() && mtest.calculateRaise(mstruct, eo)) {
					set(mtest);
					MathStructure *mmul = new MathStructure(-1, 1, 0);
					mmul->calculateRaise(mstruct, eo);
					multiply_nocopy(mmul);
					calculateMultiplyLast(eo);
					return 1;
				}
			} else {
				Number exp_num(mstruct.number().numerator());
				if(!exp_num.isOne() && !exp_num.isMinusOne() && o_number.isPositive() && test_if_numerator_not_too_large(o_number, exp_num)) {
					// Try raise by exponent numerator if not very large
					nr = o_number;
					if(nr.raise(exp_num, eo.approximation < APPROXIMATION_APPROXIMATE) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
						o_number = nr;
						numberUpdated();
						nr.set(mstruct.number().denominator());
						nr.recip();
						calculateRaise(nr, eo, mparent, index_this);
						return 1;
					}
				}
				if(o_number.isPositive()) {
					Number nr_root(mstruct.number().denominator());
					if(eo.split_squares && o_number.isInteger() && nr_root.isLessThanOrEqualTo(LARGEST_RAISED_PRIME_EXPONENT)) {
						int root = nr_root.intValue();
						nr.set(1, 1, 0);
						bool b = true, overflow;
						long int val;
						while(b) {
							if(CALCULATOR->aborted()) break;
							b = false;
							overflow = false;
							val = o_number.lintValue(&overflow);
							if(overflow) {
								mpz_srcptr cval = mpq_numref(o_number.internalRational());
								for(size_t i = 0; root == 2 ? (i < NR_OF_SQUARE_PRIMES) : (RAISED_PRIMES[root - 3][i] != 0); i++) {
									if(CALCULATOR->aborted()) break;
									if(mpz_divisible_ui_p(cval, (unsigned long int) (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]))) {
										nr *= PRIMES[i];
										o_number /= (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]);
										b = true;
										break;
									}
								}
							} else {
								for(size_t i = 0; root == 2 ? (i < NR_OF_SQUARE_PRIMES) : (RAISED_PRIMES[root - 3][i] != 0); i++) {
									if(CALCULATOR->aborted()) break;
									if((root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]) > val) {
										break;
									} else if(val % (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]) == 0) {
										nr *= PRIMES[i];
										o_number /= (root == 2 ? SQUARE_PRIMES[i] : RAISED_PRIMES[root - 3][i]);
										b = true;
										break;
									}
								}
							}
						}
						if(!nr.isOne()) {
							transform(STRUCT_MULTIPLICATION);
							CHILD(0).calculateRaise(mstruct, eo, this, 0);
							PREPEND(nr);
							if(!mstruct.number().numeratorIsOne()) {
								CHILD(0).calculateRaise(mstruct.number().numerator(), eo, this, 0);
							}
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
							return 1;
						}
					}
					if(eo.split_squares && nr_root != 2) {
						// partial roots, e.g. 9^(1/4)=3^(1/2)
						if(nr_root.isEven()) {
							Number nr(o_number);
							if(nr.sqrt() && !nr.isApproximate()) {
								o_number = nr;
								mstruct.number().multiply(2);
								mstruct.ref();
								raise_nocopy(&mstruct);
								calculateRaiseExponent(eo, mparent, index_this);
								return 1;
							}
						}
						for(size_t i = 1; i < NR_OF_PRIMES; i++) {
							if(nr_root.isLessThanOrEqualTo(PRIMES[i])) break;
							if(nr_root.isIntegerDivisible(PRIMES[i])) {
								Number nr(o_number);
								if(nr.root(Number(PRIMES[i], 1)) && !nr.isApproximate()) {
									o_number = nr;
									mstruct.number().multiply(PRIMES[i]);
									mstruct.ref();
									raise_nocopy(&mstruct);
									calculateRaiseExponent(eo, mparent, index_this);
									return 1;
								}
							}
						}
					}
				}
			}
		}
		if(o_number.isMinusOne() && mstruct.number().isRational()) {
			if(mstruct.number().isInteger()) {
				if(mstruct.number().isEven()) set(m_one, true);
				else set(m_minus_one, true);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else {
				Number nr_floor(mstruct.number());
				nr_floor.floor();
				if(mstruct.number().denominatorIsTwo()) {
					if(nr_floor.isEven()) set(nr_one_i, true);
					else set(nr_minus_i, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else {
					mstruct.number() -= nr_floor;
					mstruct.numberUpdated();
					if(mstruct.number().denominator() == 3) {
						set(3, 1, 0, true);
						calculateRaise(nr_half, eo);
						if(nr_floor.isEven()) calculateMultiply(nr_one_i, eo);
						else calculateMultiply(nr_minus_i, eo);
						calculateMultiply(nr_half, eo);
						if(nr_floor.isEven() == mstruct.number().numeratorIsOne()) calculateAdd(nr_half, eo);
						else calculateAdd(nr_minus_half, eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(mstruct.number().denominator() == 4) {
						if(nr_floor.isEven() == mstruct.number().numeratorIsOne()) set(1, 1, 0, true);
						else set(-1, 1, 0, true);
						if(nr_floor.isEven()) calculateAdd(nr_one_i, eo);
						else calculateAdd(nr_minus_i, eo);
						multiply(nr_two);
						LAST.calculateRaise(nr_minus_half, eo);
						calculateMultiplyLast(eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(!nr_floor.isZero()) {
						mstruct.ref();
						raise_nocopy(&mstruct);
						calculateRaiseExponent(eo);
						if(nr_floor.isOdd()) calculateNegate(eo);
						return 1;
					}
				}
			}
		}
		if(o_number.isRational() && !o_number.isInteger() && !o_number.numeratorIsOne() && mstruct.number().isRational()) {
			Number num(o_number.numerator());
			Number den(o_number.denominator());
			set(num, true);
			calculateRaise(mstruct, eo);
			multiply(den);
			LAST.calculateRaise(mstruct, eo);
			LAST.calculateInverse(eo);
			calculateMultiplyLast(eo);
			return 1;
		}
		// If base numerator is larger than denominator, invert base and negate exponent
		if(o_number.isRational() && !o_number.isInteger() && !o_number.isZero() && ((o_number.isNegative() && o_number.isGreaterThan(nr_minus_one) && mstruct.number().isInteger()) || (o_number.isPositive() && o_number.isLessThan(nr_one)))) {
			mstruct.number().negate();
			o_number.recip();
			return 0;
		}
		return -1;
	}
	if(mstruct.isOne()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	} else if(isOne() && mstruct.representsNumber()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite(false)) {
		if(mstruct.representsNegative(false)) {
			o_number.clear();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		} else if(mstruct.representsNonZero(false) && mstruct.representsPositive(false)) {
			if(o_number.isMinusInfinity()) {
				if(mstruct.representsEven(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.representsOdd(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(o_number.isPlusInfinity()) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		MathStructure mtest(mstruct);
		CALCULATOR->beginTemporaryEnableIntervalArithmetic();
		if(CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->beginTemporaryStopMessages();
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
			mtest.calculateFunctions(eo2);
			mtest.calculatesub(eo2, eo2);
			CALCULATOR->endTemporaryStopMessages();
		}
		CALCULATOR->endTemporaryEnableIntervalArithmetic();
		if(mtest.representsNegative(false)) {
			o_number.clear();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		} else if(mtest.representsNonZero(false) && mtest.representsPositive(false)) {
			if(o_number.isMinusInfinity()) {
				if(mstruct.representsEven(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.representsOdd(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			} else if(o_number.isPlusInfinity()) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite(false)) {
		MathStructure mtest(*this);
		CALCULATOR->beginTemporaryEnableIntervalArithmetic();
		if(CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->beginTemporaryStopMessages();
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			if(eo2.interval_calculation == INTERVAL_CALCULATION_NONE) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
			mtest.calculateFunctions(eo2);
			mtest.calculatesub(eo2, eo2);
			CALCULATOR->endTemporaryStopMessages();
		}
		CALCULATOR->endTemporaryEnableIntervalArithmetic();
		if(mtest.isNumber()) {
			if(mtest.merge_power(mstruct, eo) > 0 && mtest.isNumber()) {
				if(mtest.number().isPlusInfinity()) {
					set(nr_plus_inf, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.number().isMinusInfinity()) {
					set(nr_minus_inf, true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mtest.number().isZero()) {
					clear(true);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	}

	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	if(isZero() && mstruct.representsPositive()) {
		return 1;
	}
	if(mstruct.isZero() && !representsUndefined(true, true)) {
		set(m_one);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}

	switch(m_type) {
		case STRUCT_VECTOR: {
			if(mstruct.isNumber() && mstruct.number().isInteger()) {
				if(isMatrix()) {
					if(matrixIsSquare()) {
						Number nr(mstruct.number());
						bool b_neg = false;
						if(nr.isNegative()) {
							nr.setNegative(false);
							b_neg = true;
						}
						if(!nr.isOne()) {
							MathStructure msave(*this);
							nr--;
							while(nr.isPositive()) {
								if(CALCULATOR->aborted()) {
									set(msave);
									return -1;
								}
								calculateMultiply(msave, eo);
								nr--;
							}
						}
						if(b_neg) {
							if(!invertMatrix(eo)) {
								if(mstruct.number().isMinusOne()) return -1;
								raise(nr_minus_one);
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					return -1;
				} else {
					if(mstruct.number().isMinusOne()) {
						return -1;
					}
					Number nr(mstruct.number());
					if(nr.isNegative()) {
						nr++;
					} else {
						nr--;
					}
					MathStructure msave(*this);
					calculateMultiply(msave, eo);
					calculateRaise(nr, eo);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}

			}
			goto default_power_merge;
		}
		case STRUCT_ADDITION: {
			if(mstruct.isNumber() && mstruct.number().isInteger() && containsType(STRUCT_DATETIME, false, true, false) <= 0) {
				if(eo.reduce_divisions && mstruct.number().isMinusOne()) {
					int bnum = -1, bden = -1;
					int inegs = 0;
					bool b_break = false;
					for(size_t i = 0; i < SIZE && !b_break; i++) {
						switch(CHILD(i).type()) {
							case STRUCT_NUMBER: {
								if(!CHILD(i).number().isRational() || CHILD(i).number().numeratorIsGreaterThan(1000000L) || CHILD(i).number().numeratorIsLessThan(-1000000L) || CHILD(i).number().denominatorIsGreaterThan(1000L)) {
									bnum = 0; bden = 0; inegs = 0; b_break = true;
								}
								if(bden != 0 && !CHILD(i).number().isInteger()) bden = 1;
								if(bnum != 0 && !CHILD(i).isZero()) {
									if(CHILD(i).number().numeratorIsOne() || CHILD(i).number().numeratorIsMinusOne()) bnum = 0;
									else bnum = 1;
								}
								if(CHILD(i).number().isNegative()) {
									inegs++;
									if(i == 0) inegs++;
								} else if(!CHILD(i).number().isZero()) inegs--;
								break;
							}
							case STRUCT_MULTIPLICATION: {
								if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
									if(!CHILD(i)[0].number().isRational() || CHILD(i)[0].number().numeratorIsGreaterThan(1000000L) || CHILD(i)[0].number().numeratorIsLessThan(-1000000L) || CHILD(i)[0].number().denominatorIsGreaterThan(1000L)) {
										bnum = 0; bden = 0; inegs = 0; b_break = true;
									}
									if(bden != 0 && !CHILD(i)[0].number().isInteger()) bden = 1;
									if(bnum != 0 && !CHILD(i)[0].isZero()) {
										if(CHILD(i)[0].number().numeratorIsOne() || CHILD(i)[0].number().numeratorIsMinusOne()) bnum = 0;
										else bnum = 1;
									}
									if(CHILD(i)[0].number().isNegative()) {
										inegs++;
										if(i == 0) inegs++;
									} else if(!CHILD(i)[0].number().isZero()) inegs--;
									break;
								}
							}
							default: {
								bnum = 0;
								inegs--;
								break;
							}
						}
					}
					if(bden < 0) bden = 0;
					if(bnum < 0) bnum = 0;
					if(bnum || bden) {
						Number nr_num, nr_den(1, 1, 0);
						for(size_t i = 0; i < SIZE && !nr_den.isZero(); i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {
									if(CHILD(i).number().isInteger()) {
										if(bnum && !nr_num.isOne() && !CHILD(i).number().isZero()) {
											if(nr_num.isZero()) nr_num = CHILD(i).number();
											else nr_num.gcd(CHILD(i).number());
										}
									} else {
										if(bnum && !nr_num.isOne() && !CHILD(i).number().isZero()) {
											if(nr_num.isZero()) nr_num = CHILD(i).number().numerator();
											else nr_num.gcd(CHILD(i).number().numerator());
										}
										if(bden) {
											nr_den.lcm(CHILD(i).number().denominator());
											if(nr_den.isGreaterThan(1000000L)) nr_den.clear();
										}
									}
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
										if(CHILD(i)[0].number().isInteger()) {
											if(bnum && !nr_num.isOne() && !CHILD(i)[0].number().isZero()) {
												if(nr_num.isZero()) nr_num = CHILD(i)[0].number();
												else nr_num.gcd(CHILD(i)[0].number());
											}
										} else {
											if(bnum && !nr_num.isOne() && !CHILD(i)[0].number().isZero()) {
												if(nr_num.isZero()) nr_num = CHILD(i)[0].number().numerator();
												else nr_num.gcd(CHILD(i)[0].number().numerator());
											}
											if(bden) {
												nr_den.lcm(CHILD(i)[0].number().denominator());
												if(nr_den.isGreaterThan(1000000L)) nr_den.clear();
											}
										}
										break;
									}
								}
								default: {
									break;
								}
							}
						}
						if(!nr_den.isZero() && (!nr_den.isOne() || !nr_num.isOne())) {
							Number nr(nr_den);
							nr.divide(nr_num);
							nr.setNegative(inegs > 0);
							for(size_t i = 0; i < SIZE; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_NUMBER: {
										CHILD(i).number() *= nr;
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
											CHILD(i)[0].number() *= nr;
											CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
											break;
										}
									}
									default: {
										CHILD(i).calculateMultiply(nr, eo);
									}
								}
							}
							calculatesub(eo, eo, false);
							mstruct.ref();
							raise_nocopy(&mstruct);
							calculateRaiseExponent(eo);
							calculateMultiply(nr, eo, mparent, index_this);
							return 1;
						}
					}
					if(inegs > 0) {
						for(size_t i = 0; i < SIZE; i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {CHILD(i).number().negate(); break;}
								case STRUCT_MULTIPLICATION: {
									if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
										CHILD(i)[0].number().negate();
										CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
										break;
									}
								}
								default: {
									CHILD(i).calculateNegate(eo);
								}
							}
						}
						mstruct.ref();
						raise_nocopy(&mstruct);
						negate();
						return 1;
					}
				} else if(eo.expand != 0 && !mstruct.number().isZero() && (eo.expand > -2 || !containsInterval())) {
					bool b = true;
					bool neg = mstruct.number().isNegative();
					Number m(mstruct.number());
					m.setNegative(false);
					if(SIZE > 1) {
						if(eo.expand == -1) {
							switch(SIZE) {
								case 4: {if(m.isGreaterThan(3)) {b = false;} break;}
								case 3: {if(m.isGreaterThan(4)) {b = false;} break;}
								case 2: {if(m.isGreaterThan(10)) {b = false;} break;}
								default: {
									if(SIZE > 8 || m.isGreaterThan(2)) b = false;
								}
							}
						} else {
							b = false;
							long int i_pow = m.lintValue(&b);
							if(b || i_pow > 300) {
								b = false;
							} else {
								Number num_terms;
								if(num_terms.binomial(i_pow + (long int) SIZE - 1, (long int) SIZE - 1)) {
									size_t tc = countTotalChildren() / SIZE;
									if(tc <= 4) tc = 0;
									else tc -= 4;
									b = num_terms.isLessThanOrEqualTo(tc > 1 ? 300 / tc : 300);
								}
							}
						}
					}
					if(b) {
						if(!representsNonMatrix()) {
							MathStructure mthis(*this);
							while(!m.isOne()) {
								if(CALCULATOR->aborted()) {
									set(mthis);
									goto default_power_merge;
								}
								calculateMultiply(mthis, eo);
								m--;
							}
						} else {
							MathStructure mstruct1(CHILD(0));
							MathStructure mstruct2(CHILD(1));
							for(size_t i = 2; i < SIZE; i++) {
								if(CALCULATOR->aborted()) goto default_power_merge;
								mstruct2.add(CHILD(i), true);
							}
							Number k(1);
							Number p1(m);
							Number p2(1);
							p1--;
							Number bn;
							MathStructure msave(*this);
							CLEAR
							APPEND(mstruct1);
							CHILD(0).calculateRaise(m, eo);
							while(k.isLessThan(m)) {
								if(CALCULATOR->aborted() || !bn.binomial(m, k)) {
									set(msave);
									goto default_power_merge;
								}
								APPEND_NEW(bn);
								LAST.multiply(mstruct1);
								if(!p1.isOne()) {
									LAST[1].raise_nocopy(new MathStructure(p1));
									LAST[1].calculateRaiseExponent(eo);
								}
								LAST.multiply(mstruct2, true);
								if(!p2.isOne()) {
									LAST[2].raise_nocopy(new MathStructure(p2));
									LAST[2].calculateRaiseExponent(eo);
								}
								LAST.calculatesub(eo, eo, false);
								k++;
								p1--;
								p2++;
							}
							APPEND(mstruct2);
							LAST.calculateRaise(m, eo);
							calculatesub(eo, eo, false);
						}
						if(neg) calculateInverse(eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				}
			}
			goto default_power_merge;
		}
		case STRUCT_MULTIPLICATION: {
			if(mstruct.representsInteger()) {
				// (xy)^a=x^a*y^a
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculateRaise(mstruct, eo);
				}
				MERGE_APPROX_AND_PREC(mstruct)
				calculatesub(eo, eo, false, mparent, index_this);
				return 1;
			} else if(!mstruct.isInfinite()) {
				// (-5xy)^z=5^z*x^z*(-y)^z && x >= 0 && y<0
				MathStructure mnew;
				mnew.setType(STRUCT_MULTIPLICATION);
				for(size_t i = 0; i < SIZE;) {
					if(CHILD(i).representsNonNegative(true)) {
						CHILD(i).ref();
						mnew.addChild_nocopy(&CHILD(i));
						ERASE(i);
					} else if(CHILD(i).isNumber() && CHILD(i).number().isNegative() && !CHILD(i).number().isMinusOne()) {
						// (-5)^z=5^z*(-1)^z
						CHILD(i).number().negate();
						mnew.addChild(CHILD(i));
						CHILD(i).number().set(-1, 1, 0);
						i++;
					} else {
						i++;
					}
				}
				if(mnew.size() > 0) {
					if(SIZE > 0) {
						if(SIZE == 1) SET_CHILD_MAP(0)
						mnew.addChild(*this);
					}
					set_nocopy(mnew, true);
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculateRaise(mstruct, eo);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
			goto default_power_merge;
		}
		case STRUCT_POWER: {
			if((eo.allow_complex && CHILD(1).representsFraction()) || (mstruct.representsInteger() && (eo.allow_complex || CHILD(0).representsInteger())) || representsNonNegative(true)) {
				if((((!eo.assume_denominators_nonzero || eo.warn_about_denominators_assumed_nonzero) && !CHILD(0).representsNonZero(true)) || CHILD(0).isZero()) && CHILD(1).representsNegative(true)) {
					if(!eo.assume_denominators_nonzero || CHILD(0).isZero() || !warn_about_denominators_assumed_nonzero(CHILD(0), eo)) break;
				}
				if(!CHILD(1).representsNonInteger() && !mstruct.representsInteger()) {
					if(CHILD(1).representsEven() && CHILD(0).representsReal(true)) {
						if(CHILD(0).representsNegative(true)) {
							CHILD(0).calculateNegate(eo);
						} else if(!CHILD(0).representsNonNegative(true)) {
							MathStructure mstruct_base(CHILD(0));
							CHILD(0).set(CALCULATOR->f_abs, &mstruct_base, NULL);
						}
					} else if(!CHILD(1).representsOdd() && !CHILD(0).representsNonNegative(true)) {
						goto default_power_merge;
					}
				}
				mstruct.ref();
				MERGE_APPROX_AND_PREC(mstruct)
				CHILD(1).multiply_nocopy(&mstruct, true);
				CHILD(1).calculateMultiplyLast(eo, true, this, 1);
				calculateRaiseExponent(eo, mparent, index_this);
				return 1;
			}
			if(mstruct.isNumber() && CHILD(0).isVariable() && CHILD(0).variable() == CALCULATOR->v_e && CHILD(1).isNumber() && CHILD(1).number().hasImaginaryPart() && !CHILD(1).number().hasRealPart() && mstruct.number().isReal()) {
				CALCULATOR->beginTemporaryEnableIntervalArithmetic();
				if(CALCULATOR->usesIntervalArithmetic()) {
					CALCULATOR->beginTemporaryStopMessages();
					Number nr(*CHILD(1).number().internalImaginary());
					nr.add(CALCULATOR->v_pi->get().number());
					nr.divide(CALCULATOR->v_pi->get().number() * 2);
					Number nr_u(nr.upperEndPoint());
					nr = nr.lowerEndPoint();
					nr_u.floor();
					nr.floor();
					if(!CALCULATOR->endTemporaryStopMessages() && nr == nr_u) {
						CALCULATOR->endTemporaryEnableIntervalArithmetic();
						nr.setApproximate(false);
						nr *= 2;
						nr.negate();
						nr *= nr_one_i;
						if(!nr.isZero()) {
							CHILD(1) += nr;
							CHILD(1).last() *= CALCULATOR->v_pi;
						}
						mstruct.ref();
						CHILD(1).multiply_nocopy(&mstruct, true);
						CHILD(1).calculateMultiplyLast(eo, true, this, 1);
						calculateRaiseExponent(eo, mparent, index_this);
						return true;
					}
				}
				CALCULATOR->endTemporaryEnableIntervalArithmetic();
			}
			goto default_power_merge;
		}
		case STRUCT_VARIABLE: {
			if(o_variable == CALCULATOR->v_e) {
				if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber()) {
					if(mstruct[1].isVariable() && mstruct[1].variable() == CALCULATOR->v_pi) {
						if(mstruct[0].number().isI() || mstruct[0].number().isMinusI()) {
							//e^(i*pi)=-1
							set(m_minus_one, true);
							return 1;
						} else if(mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().hasRealPart() && mstruct[0].number().internalImaginary()->isRational()) {
							// e^(a*i*pi)=(-1)^(a)
							set(-1, 1, 0, true);
							calculateRaise(*mstruct[0].number().internalImaginary(), eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					} else if(mstruct[0].number().isI() && mstruct[1].isFunction() && mstruct[1].function() == CALCULATOR->f_atan && mstruct[1].size() == 1 && !mstruct[1][0].containsUnknowns() && ((eo.expand != 0 && eo.expand > -2) || !mstruct[1][0].containsInterval(true, false, false, eo.expand == -2 ? 1 : 0))) {
						set(mstruct[1][0], true);
						calculateRaise(nr_two, eo);
						calculateAdd(m_one, eo);
						calculateRaise(nr_half, eo);
						calculateInverse(eo);
						multiply(mstruct[1][0]);
						LAST.calculateMultiply(nr_one_i, eo);
						LAST.calculateAdd(m_one, eo);
						calculateMultiplyLast(eo);
						MERGE_APPROX_AND_PREC(mstruct)
						return true;
					}
				} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_ln && mstruct.size() == 1) {
					if(mstruct[0].representsNumber() && (eo.allow_infinite || mstruct[0].representsNonZero())) {
						// e^ln(x)=x; x!=0
						set_nocopy(mstruct[0], true);
						return 1;
					}
				}
			}
			goto default_power_merge;
		}
		case STRUCT_FUNCTION: {
			if(eo.protected_function != o_function) {
				if(o_function == CALCULATOR->f_abs && SIZE == 1) {
					if(mstruct.representsEven() && CHILD(0).representsReal(true)) {
						// abs(x)^2=x^2
						SET_CHILD_MAP(0);
						mstruct.ref();
						raise_nocopy(&mstruct);
						calculateRaiseExponent(eo);
						return 1;
					}
				} else if(o_function == CALCULATOR->f_signum && CHILD(0).representsReal(true) && SIZE == 2 && ((CHILD(1).isZero() && mstruct.representsPositive()) || CHILD(1).isOne())) {
					if(mstruct.representsOdd()) {
						// sgn(x)^3=sgn(x)
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(mstruct.representsEven()) {
						if(CHILD(1).isOne() && CHILD(0).representsReal(true)) {
							SET_CHILD_MAP(0)
							return 1;
						} else {
							// sgn(x)^2=sgn(abs(x))
							CHILD(0).transform(CALCULATOR->f_abs);
							CHILD_UPDATED(0)
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				} else if(o_function == CALCULATOR->f_root && THIS_VALID_ROOT) {
					if(mstruct.representsEven() && CHILD(0).representsReal(true) && CHILD(1).number().isOdd()) {
						// root(x, 3)^2=abs(x)^(3/2)
						CHILD(0).transform(STRUCT_FUNCTION);
						CHILD(0).setFunction(CALCULATOR->f_abs);
						CHILD(1).number().recip();
						m_type = STRUCT_POWER;
						mstruct.ref();
						raise_nocopy(&mstruct);
						calculateRaiseExponent(eo);
						return 1;
					} else if(mstruct.isNumber() && mstruct.number().isInteger() && !mstruct.number().isMinusOne()) {
						if(mstruct == CHILD(1)) {
							// root(x, a)^a=x
							SET_CHILD_MAP(0)
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						} else if(mstruct.number().isIntegerDivisible(CHILD(1).number())) {
							// root(x, a)^(2a)=x^2
							mstruct.calculateDivide(CHILD(1).number(), eo);
							mstruct.ref();
							SET_CHILD_MAP(0)
							raise_nocopy(&mstruct);
							return 1;
						} else if(CHILD(1).number().isIntegerDivisible(mstruct.number())) {
							// root(x, 3a)^(a)=root(x, 3)
							Number nr(CHILD(1).number());
							if(nr.divide(mstruct.number())) {
								CHILD(1) = nr;
								CHILD_UPDATED(1)
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					}
				}
			}
			if(o_function == CALCULATOR->f_stripunits && SIZE == 1 && mstruct.containsType(STRUCT_UNIT, false, true, true) == 0) {
				mstruct.ref();
				CHILD(0).raise_nocopy(&mstruct);
				return 1;
			}
			goto default_power_merge;
		}
		default: {
			default_power_merge:

			if(mstruct.isAddition()) {
				bool b = representsNonNegative(true);
				if(!b) {
					b = true;
					bool bneg = representsNegative(true);
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(!mstruct[i].representsInteger() && (!bneg || !eo.allow_complex || !mstruct[i].isNumber() || !mstruct[i].number().isRational() || !mstruct[i].number().denominatorIsEven())) {
							b = false;
							break;
						}
					}
				}
				if(b) {
					MathStructure msave(*this);
					clear(true);
					m_type = STRUCT_MULTIPLICATION;
					MERGE_APPROX_AND_PREC(mstruct)
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND(msave);
						mstruct[i].ref();
						LAST.raise_nocopy(&mstruct[i]);
						LAST.calculateRaiseExponent(eo);
						calculateMultiplyLast(eo, false);
					}
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
			} else if(mstruct.isMultiplication() && mstruct.size() > 1) {
				bool b = representsNonNegative(true);
				if(!b) {
					b = true;
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(!mstruct[i].representsInteger()) {
							b = false;
							break;
						}
					}
				}
				if(b) {
					MathStructure mthis(*this);
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(i == 0) mthis.raise(mstruct[i]);
						if(!mstruct[i].representsReal(true) || (isZero() && !mstruct[i].representsPositive(true))) continue;
						if(i > 0) mthis[1] = mstruct[i];
						EvaluationOptions eo2 = eo;
						eo2.split_squares = false;
						// avoid abs(x)^(2y) loop
						if(mthis.calculateRaiseExponent(eo2) && (!mthis.isPower() || ((!isFunction() || o_function != CALCULATOR->f_abs || SIZE != 1 || !CHILD(0).equals(mthis[0], true, true)) && (!is_negation(mthis[0], *this))))) {
							set(mthis);
							if(mstruct.size() == 2) {
								if(i == 0) {
									mstruct[1].ref();
									raise_nocopy(&mstruct[1]);
								} else {
									mstruct[0].ref();
									raise_nocopy(&mstruct[0]);
								}
							} else {
								mstruct.ref();
								raise_nocopy(&mstruct);
								CHILD(1).delChild(i + 1);
							}
							calculateRaiseExponent(eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				}
			} else if(mstruct.isNumber() && mstruct.number().isRational() && !mstruct.number().isInteger() && !mstruct.number().numeratorIsOne() && !mstruct.number().numeratorIsMinusOne()) {
				if(representsNonNegative(true) && (m_type != STRUCT_FUNCTION || o_function != CALCULATOR->f_abs)) {
					if(isMultiplication() && SIZE == 2 && CHILD(0).isMinusOne() && mstruct.number().numeratorIsEven()) {
						bool b;
						if(mstruct.number().isNegative()) {
							MathStructure mtest(CHILD(1));
							b = mtest.calculateRaise(-mstruct.number().numerator(), eo);
							if(b && mtest.isPower() && mtest[1] == -mstruct.number().numerator()) b = false;
							if(!b) break;
							set(mtest, true);
							raise(m_minus_one);
							CHILD(1).number() /= mstruct.number().denominator();
						} else {
							MathStructure mtest(CHILD(1));
							b = mtest.calculateRaise(mstruct.number().numerator(), eo);
							if(b && mtest.isPower() && mtest[1] == mstruct.number().numerator()) b = false;
							if(!b) break;
							set(mtest, true);
							raise(m_one);
							CHILD(1).number() /= mstruct.number().denominator();
						}
						if(b) calculateRaiseExponent(eo);
						return 1;
					}
					bool b;
					if(mstruct.number().isNegative()) {
						b = calculateRaise(-mstruct.number().numerator(), eo);
						if(!b) {
							setToChild(1);
							break;
						}
						raise(m_minus_one);
						CHILD(1).number() /= mstruct.number().denominator();
					} else {
						b = calculateRaise(mstruct.number().numerator(), eo);
						if(!b) {
							setToChild(1);
							break;
						}
						raise(m_one);
						CHILD(1).number() /= mstruct.number().denominator();
					}
					if(b) calculateRaiseExponent(eo);
					return 1;
				}
			}
			break;
		}
	}

	return -1;
}

int MathStructure::merge_logical_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {
	if(equals(mstruct, true, true)) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(mstruct.representsNonZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(mstruct.isZero()) {
		if(isZero()) return 2;
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 3;
	}
	if(representsNonZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}

	if(CALCULATOR->aborted()) return -1;

	if(eo.test_comparisons && isLogicalOr()) {
		if(SIZE > 50) return -1;
		if(mstruct.isLogicalOr()) {
			if(mstruct.size() * SIZE > 50) return -1;
			for(size_t i = 0; i < SIZE; ) {
				MathStructure msave(CHILD(i));
				for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
					if(i2 > 0) {
						insertChild(msave, i + 1);
					}
					CHILD(i).calculateLogicalAnd(mstruct[i2], eo, this, i);
					i++;
				}
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).calculateLogicalAnd(mstruct, eo, this, i);
			}
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	} else if(eo.test_comparisons && mstruct.isLogicalOr()) {
		return 0;
	} else if(isComparison() && mstruct.isComparison()) {
		if(CHILD(0) == mstruct[0]) {
			ComparisonResult cr = mstruct[1].compare(CHILD(1));
			ComparisonType ct1 = ct_comp, ct2 = mstruct.comparisonType();
			switch(cr) {
				case COMPARISON_RESULT_NOT_EQUAL: {
					if(ct_comp == COMPARISON_EQUALS && ct2 == COMPARISON_EQUALS) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(ct_comp == COMPARISON_EQUALS && ct2 == COMPARISON_NOT_EQUALS) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 2;
					} else if(ct_comp == COMPARISON_NOT_EQUALS && ct2 == COMPARISON_EQUALS) {
						if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
						} else {
							set_nocopy(mstruct, true);
						}
						return 3;
					}
					return -1;
				}
				case COMPARISON_RESULT_EQUAL: {
					MERGE_APPROX_AND_PREC(mstruct)
					if(ct_comp == ct2) return 1;
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_LESS;
							if(ct2 == COMPARISON_LESS) return 3;
							return 1;
						} else if(ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS_GREATER) {
							ct_comp = COMPARISON_GREATER;
							if(ct2 == COMPARISON_GREATER) return 3;
							return 1;
						}
					} else if(ct2 == COMPARISON_NOT_EQUALS) {
						if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
							if(ct_comp == COMPARISON_LESS) return 2;
							ct_comp = COMPARISON_LESS;
							return 1;
						} else if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
							if(ct_comp == COMPARISON_GREATER) return 2;
							ct_comp = COMPARISON_GREATER;
							return 1;
						}
					} else if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_EQUALS) && (ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS)) {
						if(ct_comp == COMPARISON_EQUALS) return 2;
						ct_comp = COMPARISON_EQUALS;
						if(ct2 == COMPARISON_EQUALS) return 3;
						return 1;
					} else if((ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) && (ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS)) {
						if(ct_comp == COMPARISON_LESS) return 2;
						ct_comp = COMPARISON_LESS;
						if(ct2 == COMPARISON_LESS) return 3;
						return 1;
					} else if((ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) && (ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS_GREATER)) {
						if(ct_comp == COMPARISON_GREATER) return 2;
						ct_comp = COMPARISON_GREATER;
						if(ct2 == COMPARISON_GREATER) return 3;
						return 1;
					}
					clear(true);
					return 1;
				}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {
					switch(ct1) {
						case COMPARISON_LESS: {
							if(ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_NOT_EQUALS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_GREATER: {
							if(ct2 == COMPARISON_GREATER) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(ct2 == COMPARISON_EQUALS_LESS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
					}
					break;
				}
				case COMPARISON_RESULT_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_LESS: {
					switch(ct1) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) clear(true); return 1;}
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) clear(true); return 1;}
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
					}
					break;
				}
				default: {
					if(!eo.test_comparisons) return -1;
					if(comparisonType() == COMPARISON_EQUALS && !CHILD(1).contains(mstruct[0])) {
						mstruct.calculateReplace(CHILD(0), CHILD(1), eo);
						if(eo.isolate_x) {mstruct.isolate_x(eo, eo); mstruct.calculatesub(eo, eo, true);}
						mstruct.ref();
						add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
						calculateLogicalAndLast(eo);
						return 1;
					} else if(mstruct.comparisonType() == COMPARISON_EQUALS && !mstruct[1].contains(CHILD(0))) {
						calculateReplace(mstruct[0], mstruct[1], eo);
						if(eo.isolate_x) {isolate_x(eo, eo); calculatesub(eo, eo, true);}
						mstruct.ref();
						add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
						calculateLogicalAndLast(eo);
						return 1;
					}
					return -1;
				}
			}
		} else if(eo.test_comparisons && comparisonType() == COMPARISON_EQUALS && !CHILD(0).isNumber() && !CHILD(0).containsInterval() && CHILD(1).isNumber() && mstruct.contains(CHILD(0))) {
			mstruct.calculateReplace(CHILD(0), CHILD(1), eo);
			if(eo.isolate_x) {mstruct.isolate_x(eo, eo); mstruct.calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
			calculateLogicalAndLast(eo);
			return 1;
		} else if(eo.test_comparisons && mstruct.comparisonType() == COMPARISON_EQUALS && !mstruct[0].isNumber() && !mstruct[0].containsInterval() && mstruct[1].isNumber() && contains(mstruct[0])) {
			calculateReplace(mstruct[0], mstruct[1], eo);
			if(eo.isolate_x) {isolate_x(eo, eo); calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_AND);
			calculateLogicalAndLast(eo);
			return 1;
		}
	} else if(isLogicalAnd()) {
		if(mstruct.isLogicalAnd()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				APPEND_REF(&mstruct[i]);
			}
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		} else {
			APPEND_REF(&mstruct);
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		}
		return 1;
	} else if(mstruct.isLogicalAnd()) {
		transform(STRUCT_LOGICAL_AND);
		for(size_t i = 0; i < mstruct.size(); i++) {
			APPEND_REF(&mstruct[i]);
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	}
	return -1;

}

int MathStructure::merge_logical_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {

	if(mstruct.representsNonZero()) {
		if(isOne()) {
			MERGE_APPROX_AND_PREC(mstruct)
			return 2;
		}
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 3;
	}
	if(mstruct.isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(representsNonZero()) {
		if(!isOne()) set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(isZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(equals(mstruct, true, true)) {
		return 2;
	}
	if(isLogicalAnd()) {
		if(mstruct.isLogicalAnd()) {
			if(SIZE < mstruct.size()) {
				bool b = true;
				for(size_t i = 0; i < SIZE; i++) {
					b = false;
					for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
						if(CHILD(i) == mstruct[i2]) {
							b = true;
							break;
						}
					}
					if(!b) break;
				}
				if(b) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(SIZE > mstruct.size()) {
				bool b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					b = false;
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(mstruct[i] == CHILD(i2)) {
							b = true;
							break;
						}
					}
					if(!b) break;
				}
				if(b) {
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				}
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i) == mstruct) {
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				}
			}
		}
	} else if(mstruct.isLogicalAnd()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(equals(mstruct[i])) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			}
		}
	}

	if(isComparison() && mstruct.isComparison()) {
		if(CHILD(0) == mstruct[0]) {
			ComparisonResult cr = mstruct[1].compare(CHILD(1));
			ComparisonType ct1 = ct_comp, ct2 = mstruct.comparisonType();
			switch(cr) {
				case COMPARISON_RESULT_NOT_EQUAL: {
					return -1;
				}
				case COMPARISON_RESULT_EQUAL: {
					if(ct_comp == ct2) return 1;
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_EQUALS_GREATER: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_LESS: {ct_comp = COMPARISON_EQUALS_LESS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_EQUALS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_LESS: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_GREATER: {}
								case COMPARISON_EQUALS_GREATER: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_EQUALS_GREATER: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {ct_comp = COMPARISON_EQUALS_LESS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_NOT_EQUALS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_LESS: {}
								case COMPARISON_EQUALS_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_EQUALS_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {ct_comp = COMPARISON_EQUALS_GREATER; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_LESS: {ct_comp = COMPARISON_NOT_EQUALS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
					}
					break;
				}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {
					switch(ct1) {
						case COMPARISON_LESS: {
							if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_NOT_EQUALS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_GREATER: {
							if(ct2 == COMPARISON_GREATER) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(ct2 == COMPARISON_EQUALS_LESS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						default: {}
					}
					break;
				}
				case COMPARISON_RESULT_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_LESS: {
					switch(ct1) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
							}
							break;
						}
					}
					break;
				}
				default: {
					return -1;
				}
			}
		} else if(eo.test_comparisons && comparisonType() == COMPARISON_NOT_EQUALS && !CHILD(0).isNumber() && !CHILD(0).containsInterval() && CHILD(1).isNumber() && mstruct.contains(CHILD(0))) {
			mstruct.calculateReplace(CHILD(0), CHILD(1), eo);
			if(eo.isolate_x) {mstruct.isolate_x(eo, eo); mstruct.calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_OR);
			calculateLogicalOrLast(eo);
			return 1;
		} else if(eo.test_comparisons && mstruct.comparisonType() == COMPARISON_NOT_EQUALS && !mstruct[0].isNumber() && !mstruct[0].containsInterval() && mstruct[1].isNumber() && contains(mstruct[0])) {
			calculateReplace(mstruct[0], mstruct[1], eo);
			if(eo.isolate_x) {isolate_x(eo, eo); calculatesub(eo, eo, true);}
			mstruct.ref();
			add_nocopy(&mstruct, OPERATION_LOGICAL_OR);
			calculateLogicalOrLast(eo);
			return 1;
		}
	} else if(isLogicalOr()) {
		if(mstruct.isLogicalOr()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				APPEND_REF(&mstruct[i]);
			}
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		} else {
			APPEND_REF(&mstruct);
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		}
		return 1;
	} else if(mstruct.isLogicalOr()) {
		transform(STRUCT_LOGICAL_OR);
		for(size_t i = 0; i < mstruct.size(); i++) {
			APPEND_REF(&mstruct[i]);
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	}
	return -1;

}

int MathStructure::merge_logical_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {

	if(equals(mstruct)) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	bool bp1 = representsNonZero();
	bool bp2 = mstruct.representsNonZero();
	if(bp1 && bp2) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	bool bn1 = isZero();
	bool bn2 = mstruct.isZero();
	if(bn1 && bn2) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	if((bn1 && bp2) || (bp1 && bn2)) {
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	return -1;

	/*int b0, b1;
	if(isZero()) {
		b0 = 0;
	} else if(representsNonZero(true)) {
		b0 = 1;
	} else {
		b0 = -1;
	}
	if(mstruct.isZero()) {
		b1 = 0;
	} else if(mstruct.representsNonZero(true)) {
		b1 = 1;
	} else {
		b1 = -1;
	}

	if((b0 == 1 && b1 == 0) || (b0 == 0 && b1 == 1)) {
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	} else if(b0 >= 0 && b1 >= 0) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	} else if(b0 == 0) {
		set(mstruct, true);
		add(m_zero, OPERATION_NOT_EQUALS);
		calculatesub(eo, eo, false);
		return 1;
	} else if(b0 == 1) {
		set(mstruct, true);
		add(m_zero, OPERATION_EQUALS);
		calculatesub(eo, eo, false);
		return 1;
	} else if(b1 == 0) {
		add(m_zero, OPERATION_NOT_EQUALS);
		calculatesub(eo, eo, false);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	} else if(b1 == 1) {
		add(m_zero, OPERATION_EQUALS);
		calculatesub(eo, eo, false);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	MathStructure *mstruct2 = new MathStructure(*this);
	add(mstruct, OPERATION_LOGICAL_AND);
	LAST.calculateLogicalNot(eo);
	LAST.calculatesub(eo, eo, false);
	calculatesub(eo, eo, false);
	mstruct2->setLogicalNot();
	mstruct2->calculatesub(eo, eo, false);
	mstruct2->add(mstruct, OPERATION_LOGICAL_AND);
	mstruct2->calculatesub(eo, eo, false);
	add_nocopy(mstruct2, OPERATION_LOGICAL_OR);
	calculatesub(eo, eo, false);

	return 1;*/

}


int MathStructure::merge_bitwise_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitAnd(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_AND);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_BITWISE_AND: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_BITWISE_AND: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND_REF(&mstruct[i]);
					}
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					APPEND_REF(&mstruct);
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_BITWISE_AND: {
					return 0;
				}
				default: {}
			}
		}
	}
	return -1;
}
int MathStructure::merge_bitwise_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitOr(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_OR);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_BITWISE_OR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_BITWISE_OR: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND_REF(&mstruct[i]);
					}
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					APPEND_REF(&mstruct);
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_BITWISE_OR: {
					return 0;
				}
				default: {}
			}
		}
	}
	return -1;
}
int MathStructure::merge_bitwise_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitXor(mstruct.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mstruct.number().includesInfinity())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_XOR);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		default: {}
	}
	return -1;
}

#define MERGE_RECURSE			if(recursive) {\
						for(size_t i = 0; i < SIZE; i++) {\
							if(CALCULATOR->aborted()) break;\
							if(!CHILD(i).isNumber()) CHILD(i).calculatesub(eo, feo, true, this, i);\
						}\
						CHILDREN_UPDATED;\
					}

#define MERGE_ALL(FUNC, TRY_LABEL) 	size_t i2, i3 = SIZE;\
					bool do_abort = false; \
					for(size_t i = 0; i < SIZE - 1; i++) {\
						i2 = i + 1;\
						TRY_LABEL:\
						for(; i2 < i; i2++) {\
							if(CALCULATOR->aborted()) break;\
							int r = CHILD(i2).FUNC(CHILD(i), eo, this, i2, i);\
							if(r == 0) {\
								SWAP_CHILDREN(i2, i);\
								r = CHILD(i2).FUNC(CHILD(i), eo, this, i2, i, true);\
								if(r < 1) {\
									SWAP_CHILDREN(i2, i);\
								}\
							}\
							if(r >= 1) {\
								ERASE(i);\
								b = true;\
								i3 = i;\
								i = i2;\
								i2 = 0;\
								goto TRY_LABEL;\
							}\
						}\
						for(i2 = i + 1; i2 < SIZE; i2++) {\
							if(CALCULATOR->aborted()) break;\
							int r = CHILD(i).FUNC(CHILD(i2), eo, this, i, i2);\
							if(r == 0) {\
								SWAP_CHILDREN(i, i2);\
								r = CHILD(i).FUNC(CHILD(i2), eo, this, i, i2, true);\
								if(r < 1) {\
									SWAP_CHILDREN(i, i2);\
								} else if(r == 2) {\
									r = 3;\
								} else if(r == 3) {\
									r = 2;\
								}\
							}\
							if(r >= 1) {\
								ERASE(i2);\
								b = true;\
								if(r != 2) {\
									i2 = 0;\
									goto TRY_LABEL;\
								}\
								i2--;\
							} else if(CHILD(i).isDateTime()) {\
								do_abort = true;\
								break;\
							}\
						}\
						if(do_abort) break;\
						if(i3 < SIZE) {\
							if(i3 == SIZE - 1) break;\
							i = i3;\
							i3 = SIZE;\
							i2 = i + 1;\
							goto TRY_LABEL;\
						}\
					}

#define MERGE_ALL2			if(SIZE == 1) {\
						setToChild(1, false, mparent, index_this + 1);\
					} else if(SIZE == 0) {\
						clear(true);\
					} else {\
						evalSort();\
					}

Number dos_count_points(const MathStructure &m, bool b_unknown) {
	if(m.isPower() && (!b_unknown || m[0].containsUnknowns())) {
		Number nr = dos_count_points(m[0], b_unknown);
		if(!nr.isZero()) {
			if(m[0].isNumber() && m[0].number().isNonZero()) {
				nr *= m[1].number();
				if(nr.isNegative()) nr.negate();
			} else {
				nr *= 2;
			}
			return nr;
		}
	} else if(m.size() == 0 && ((b_unknown && m.isUnknown()) || (!b_unknown && !m.isNumber()))) {
		return nr_one;
	}
	Number nr;
	for(size_t i = 0; i < m.size(); i++) {
		nr += dos_count_points(m[i], b_unknown);
	}
	return nr;
}

bool do_simplification(MathStructure &mstruct, const EvaluationOptions &eo, bool combine_divisions, bool only_gcd, bool combine_only, bool recursive, bool limit_size, int i_run) {

	if(!eo.expand || !eo.assume_denominators_nonzero) return false;

	if(recursive) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			b = do_simplification(mstruct[i], eo, combine_divisions, only_gcd || (!mstruct.isComparison() && !mstruct.isLogicalAnd() && !mstruct.isLogicalOr()), combine_only, true, limit_size) || b;
			if(CALCULATOR->aborted()) return b;
		}
		if(b) mstruct.calculatesub(eo, eo, false);
		return do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, false, limit_size) || b;
	}
	if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && mstruct[0].isAddition() && mstruct[0].isRationalPolynomial()) {
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower() && msqrfree.calculateRaise(mstruct[1], eo)) {
			mstruct = msqrfree;
			return true;
		}
	} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_root && VALID_ROOT(mstruct) && mstruct[0].isAddition() && mstruct[0].isRationalPolynomial()) {
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower() && msqrfree[1].isInteger() && msqrfree[1].number().isPositive()) {
			if(msqrfree[1] == mstruct[1]) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) return false;
					msqrfree.delChild(2);
					msqrfree.setType(STRUCT_FUNCTION);
					msqrfree.setFunction(CALCULATOR->f_abs);
					mstruct = msqrfree;
				} else {
					mstruct = msqrfree[0];
				}
				return true;
			} else if(msqrfree[1].number().isIntegerDivisible(mstruct[1].number())) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) return false;
					msqrfree[0].transform(STRUCT_FUNCTION);
					msqrfree[0].setFunction(CALCULATOR->f_abs);
				}
				msqrfree[1].number().divide(mstruct[1].number());
				mstruct = msqrfree;
				mstruct.calculatesub(eo, eo, false);
				return true;
			} else if(mstruct[1].number().isIntegerDivisible(msqrfree[1].number())) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) return false;
					msqrfree[0].transform(STRUCT_FUNCTION);
					msqrfree[0].setFunction(CALCULATOR->f_abs);
				}
				Number new_root(mstruct[1].number());
				new_root.divide(msqrfree[1].number());
				mstruct[0] = msqrfree[0];
				mstruct[1] = new_root;
				return true;
			}
		}
	}
	if(!mstruct.isAddition() && !mstruct.isMultiplication()) return false;

	if(combine_divisions) {

		MathStructure divs, nums, numleft, mleft;

		EvaluationOptions eo2 = eo;
		eo2.do_polynomial_division = false;
		eo2.keep_zero_units = false;

		if(!mstruct.isAddition()) mstruct.transform(STRUCT_ADDITION);

		// find division by polynomial
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) return false;
			bool b = false;
			if(mstruct[i].isMultiplication()) {
				MathStructure div, num(1, 1, 0);
				bool b_num = false;
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(mstruct[i][i2].isPower() && mstruct[i][i2][1].isInteger() && mstruct[i][i2][1].number().isNegative()) {
						bool b_rat = mstruct[i][i2][0].isRationalPolynomial();
						if(!b_rat && mstruct[i][i2][0].isAddition() && mstruct[i][i2][0].size() > 1) {
							b_rat = true;
							for(size_t i3 = 0; i3 < mstruct[i][i2][0].size(); i3++) {
								if(!mstruct[i][i2][0][i3].representsZero(true) && !mstruct[i][i2][0][i3].isRationalPolynomial()) {
									b_rat = false;
									break;
								}
							}
						}
						if(!b_rat) {
							div.clear();
							break;
						}
						bool b_minone = mstruct[i][i2][1].isMinusOne();
						if(b_minone) {
							if(div.isZero()) div = mstruct[i][i2][0];
							else div.multiply(mstruct[i][i2][0], true);
						} else {
							mstruct[i][i2][1].number().negate();
							if(div.isZero()) div = mstruct[i][i2];
							else div.multiply(mstruct[i][i2], true);
							mstruct[i][i2][1].number().negate();
						}
					} else if(mstruct[i][i2].isRationalPolynomial() || mstruct[i][i2].representsZero(true)) {
						if(!b_num) {b_num = true; num = mstruct[i][i2];}
						else num.multiply(mstruct[i][i2], true);
					} else {
						div.clear();
						break;
					}
				}
				if(!div.isZero()) {
					bool b_found = false;
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == div) {
							if(!num.representsZero(true)) {
								if(num.isAddition()) {
									for(size_t i4 = 0; i4 < num.size(); i4++) {
										nums[i3].add(num[i4], true);
									}
								} else {
									nums[i3].add(num, true);
								}
							}
							b_found = true;
							b = true;
							break;
						}
					}
					if(!b_found && (eo.assume_denominators_nonzero || div.representsNonZero(true)) && !div.representsZero(true)) {
						if(!num.representsZero(true)) {
							divs.addChild(div);
							nums.addChild(num);
						}
						b = true;
					}
				}
			} else if(mstruct[i].isPower() && mstruct[i][1].isInteger() && mstruct[i][1].number().isNegative()) {
				bool b_rat = mstruct[i][0].isRationalPolynomial();
				if(!b_rat && mstruct[i][0].isAddition() && mstruct[i][0].size() > 1) {
					b_rat = true;
					for(size_t i2 = 0; i2 < mstruct[i][0].size(); i2++) {
						if(!mstruct[i][0][i2].representsZero(true) && !mstruct[i][0].isRationalPolynomial()) {
							b_rat = false;
							break;
						}
					}
				}
				if(b_rat) {
					bool b_minone = mstruct[i][1].isMinusOne();
					if(!b_minone) mstruct[i][1].number().negate();
					bool b_found = false;
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if((b_minone && divs[i3] == mstruct[i][0]) || (!b_minone && divs[i3] == mstruct[i])) {
							nums[i3].add(m_one, true);
							b_found = true;
							b = true;
							break;
						}
					}
					if(!b_found && (eo.assume_denominators_nonzero || mstruct[i][0].representsNonZero(true)) && !mstruct[i][0].representsZero(true)) {
						if(b_minone) divs.addChild(mstruct[i][0]);
						else divs.addChild(mstruct[i]);
						nums.addChild(m_one);
						b = true;
					}
					if(!b_minone) mstruct[i][1].number().negate();
				}
			}
			if(!b) {
				if(i_run <= 1 && mstruct[i].isRationalPolynomial()) numleft.addChild(mstruct[i]);
				else mleft.addChild(mstruct[i]);
			}
		}

		for(size_t i = 0; i < divs.size(); i++) {
			if(divs[i].isAddition()) {
				for(size_t i2 = 0; i2 < divs[i].size();) {
					if(divs[i][i2].representsZero(true)) {
						divs[i].delChild(i2 + 1);
					} else i2++;
				}
				if(divs[i].size() == 1) divs[i].setToChild(1);
				else if(divs[i].size() == 0) divs[i].clear();
			}
		}

		if(divs.size() == 0) {
			if(mstruct.size() == 1) mstruct.setToChild(1);
			return false;
		}
		bool b_ret = false;
		if(divs.size() > 1 || numleft.size() > 0) b_ret = true;

		/*divs.setType(STRUCT_VECTOR);
		nums.setType(STRUCT_VECTOR);
		numleft.setType(STRUCT_VECTOR);
		mleft.setType(STRUCT_VECTOR);
		cout << nums << ":" << divs << ":" << numleft << ":" << mleft << endl;*/

		if(i_run > 1) {
			for(size_t i = 0; i < divs.size();) {
				bool b = true;
				if(!divs[i].isRationalPolynomial() || !nums[i].isRationalPolynomial()) {
					if(mstruct.size() == 1) mstruct.setToChild(1);
					return false;
				}
				if(!combine_only && divs[i].isAddition() && nums[i].isAddition()) {
					MathStructure ca, cb, mgcd;
					if(MathStructure::gcd(nums[i], divs[i], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= nums[i].size() + divs[i].size())) {
						if(mgcd.representsNonZero(true) || !eo.warn_about_denominators_assumed_nonzero || warn_about_denominators_assumed_nonzero(mgcd, eo)) {
							if(cb.isOne()) {
								numleft.addChild(ca);
								nums.delChild(i + 1);
								divs.delChild(i + 1);
								b = false;
							} else {
								nums[i] = ca;
								divs[i] = cb;
							}
							b_ret = true;
						}
					}
				}
				if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
				if(b) i++;
			}

		} else {
			while(divs.size() > 0) {
				bool b = true;
				if(!divs[0].isRationalPolynomial() || !nums[0].isRationalPolynomial()) {
					if(mstruct.size() == 1) mstruct.setToChild(1);
					return false;
				}
				if(!combine_only && divs[0].isAddition() && nums[0].isAddition()) {
					MathStructure ca, cb, mgcd;
					if(MathStructure::gcd(nums[0], divs[0], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= nums[0].size() + divs[0].size())) {
						if(mgcd.representsNonZero(true) || !eo.warn_about_denominators_assumed_nonzero || warn_about_denominators_assumed_nonzero(mgcd, eo)) {
							if(cb.isOne()) {
								numleft.addChild(ca);
								nums.delChild(1);
								divs.delChild(1);
								b = false;
							} else {
								nums[0] = ca;
								divs[0] = cb;
							}
							b_ret = true;
						}
					}
				}
				if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
				if(b && divs.size() > 1) {
					if(!combine_only && divs[1].isAddition() && nums[1].isAddition()) {
						MathStructure ca, cb, mgcd;
						EvaluationOptions eo3 = eo2;
						eo3.transform_trigonometric_functions = false;
						if(MathStructure::gcd(nums[1], divs[1], mgcd, eo3, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= nums[1].size() + divs[1].size())) {
							if(mgcd.representsNonZero(true) || !eo.warn_about_denominators_assumed_nonzero || warn_about_denominators_assumed_nonzero(mgcd, eo)) {
								if(cb.isOne()) {
									numleft.addChild(ca);
									nums.delChild(2);
									divs.delChild(2);
									b = false;
								} else {
									nums[1] = ca;
									divs[1] = cb;
								}
							}
						}
					}
					if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
					if(b) {
						MathStructure ca, cb, mgcd;
						b = MathStructure::gcd(divs[0], divs[1], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne();
						if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
						bool b_merge = true;
						if(b) {
							if(limit_size && ((cb.isAddition() && ((divs[0].isAddition() && divs[0].size() * cb.size() > 200) || (nums[0].isAddition() && nums[0].size() * cb.size() > 200))) || (ca.isAddition() && nums[1].isAddition() && nums[1].size() * ca.size() > 200))) {
								b_merge = false;
							} else {
								if(!cb.isOne()) {
									divs[0].calculateMultiply(cb, eo2);
									nums[0].calculateMultiply(cb, eo2);
								}
								if(!ca.isOne()) {
									nums[1].calculateMultiply(ca, eo2);
								}
							}
						} else {
							if(limit_size && ((divs[1].isAddition() && ((divs[0].isAddition() && divs[0].size() * divs[1].size() > 200) || (nums[0].isAddition() && nums[0].size() * divs[1].size() > 200))) || (divs[0].isAddition() && nums[1].isAddition() && nums[1].size() * divs[0].size() > 200))) {
								b_merge = false;
							} else {
								nums[0].calculateMultiply(divs[1], eo2);
								nums[1].calculateMultiply(divs[0], eo2);
								divs[0].calculateMultiply(divs[1], eo2);
							}
						}
						if(b_merge) {
							nums[0].calculateAdd(nums[1], eo2);
							nums.delChild(2);
							divs.delChild(2);
						} else {
							size_t size_1 = 2, size_2 = 2;
							if(nums[0].isAddition()) size_1 += nums[0].size() - 1;
							if(divs[0].isAddition()) size_1 += divs[0].size() - 1;
							if(nums[1].isAddition()) size_2 += nums[1].size() - 1;
							if(divs[1].isAddition()) size_2 += divs[1].size() - 1;
							if(size_1 > size_2) {
								nums[0].calculateDivide(divs[0], eo);
								mleft.addChild(nums[0]);
								nums.delChild(1);
								divs.delChild(1);
							} else {
								nums[1].calculateDivide(divs[1], eo);
								mleft.addChild(nums[1]);
								nums.delChild(2);
								divs.delChild(2);
							}
						}
					}
				} else if(b && numleft.size() > 0) {
					if(limit_size && divs[0].isAddition() && numleft.size() > 0 && divs[0].size() * numleft.size() > 200) break;
					if(numleft.size() == 1) numleft.setToChild(1);
					else if(numleft.size() > 1) numleft.setType(STRUCT_ADDITION);
					numleft.calculateMultiply(divs[0], eo2);
					nums[0].calculateAdd(numleft, eo2);
					numleft.clear();
				} else if(b) break;
			}

			if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
			while(!combine_only && !only_gcd && divs.size() > 0 && divs[0].isAddition() && !nums[0].isNumber()) {
				bool b_unknown = divs[0].containsUnknowns();
				if(!b_unknown || nums[0].containsUnknowns()) {
					MathStructure mcomps;
					mcomps.resizeVector(nums[0].isAddition() ? nums[0].size() : 1, m_zero);
					if(nums[0].isAddition()) {
						for(size_t i = 0; i < nums[0].size(); i++) {
							if((b_unknown && nums[0][i].containsUnknowns()) || (!b_unknown && !nums[0][i].isNumber())) {
								mcomps[i].setType(STRUCT_MULTIPLICATION);
								if(nums[0][i].isMultiplication()) {
									for(size_t i2 = 0; i2 < nums[0][i].size(); i2++) {
										if((b_unknown && nums[0][i][i2].containsUnknowns()) || (!b_unknown && !nums[0][i][i2].isNumber())) {
											nums[0][i][i2].ref();
											mcomps[i].addChild_nocopy(&nums[0][i][i2]);
										}
									}
								} else {
									nums[0][i].ref();
									mcomps[i].addChild_nocopy(&nums[0][i]);
								}
							}
						}
					} else {
						mcomps[0].setType(STRUCT_MULTIPLICATION);
						if(nums[0].isMultiplication()) {
							for(size_t i2 = 0; i2 < nums[0].size(); i2++) {
								if((b_unknown && nums[0][i2].containsUnknowns()) || (!b_unknown && !nums[0][i2].isNumber())) {
									nums[0][i2].ref();
									mcomps[0].addChild_nocopy(&nums[0][i2]);
								}
							}
						} else {
							nums[0].ref();
							mcomps[0].addChild_nocopy(&nums[0]);
						}
					}
					bool b_found = false;
					unordered_map<size_t, size_t> matches;
					for(size_t i = 0; i < divs[0].size(); i++) {
						if((b_unknown && divs[0][i].containsUnknowns()) || (!b_unknown && !divs[0][i].isNumber())) {
							MathStructure mcomp1;
							mcomp1.setType(STRUCT_MULTIPLICATION);
							if(divs[0][i].isMultiplication()) {
								for(size_t i2 = 0; i2 < divs[0][i].size(); i2++) {
									if((b_unknown && divs[0][i][i2].containsUnknowns()) || (!b_unknown && !divs[0][i][i2].isNumber())) {
										divs[0][i][i2].ref();
										mcomp1.addChild_nocopy(&divs[0][i][i2]);
									}
								}
							} else {
								divs[0][i].ref();
								mcomp1.addChild_nocopy(&divs[0][i]);
							}
							b_found = false;
							for(size_t i2 = 0; i2 < mcomps.size(); i2++) {
								if(mcomps[i2] == mcomp1) {
									matches[i] = i2;
									b_found = true;
									break;
								}
							}
							if(!b_found) break;
						}
					}
					if(b_found) {
						b_found = false;
						size_t i_selected = 0;
						if(nums[0].isAddition()) {
							Number i_points;
							for(size_t i = 0; i < divs[0].size(); i++) {
								if((b_unknown && divs[0][i].containsUnknowns()) || (!b_unknown && !divs[0][i].isNumber())) {
									Number new_points = dos_count_points(divs[0][i], b_unknown);
									if(new_points > i_points) {i_points = new_points; i_selected = i;}
								}
							}
						}
						MathStructure mquo, mrem;
						if(polynomial_long_division(nums[0].isAddition() ? nums[0][matches[i_selected]] : nums[0], divs[0], m_zero, mquo, mrem, eo2, false) && !mquo.isZero() && mrem != (nums[0].isAddition() ? nums[0][matches[i_selected]] : nums[0])) {
							if(CALCULATOR->aborted()) return false;
							if((nums[0].isAddition() && nums[0].size() > 1) || !mrem.isZero() || divs[0].representsNonZero(true) || (eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[0], eo))) {
								mleft.addChild(mquo);
								if(nums[0].isAddition()) {
									nums[0].delChild(matches[i_selected] + 1, true);
									if(!mrem.isZero()) {
										nums[0].calculateAdd(mrem, eo2);
									}
								} else {
									if(mrem.isZero()) {
										divs.clear();
									} else {
										nums[0] = mrem;
									}
								}
								b_ret = true;
								b_found = true;
							}
						}
					}
					if(!b_found) break;
				} else {
					break;
				}
			}

			if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
			if(!combine_only && !only_gcd && divs.size() > 0 && divs[0].isAddition()) {
				bool b_unknown = divs[0].containsUnknowns() || nums[0].containsUnknowns();
				MathStructure mquo, mrem;
				vector<MathStructure> symsd, symsn;
				collect_symbols(nums[0], symsn);
				if(!symsn.empty()) {
					collect_symbols(divs[0], symsd);
				}
				for(size_t i = 0; i < symsd.size();) {
					bool b_found = false;
					if(!b_unknown || symsd[i].containsUnknowns()) {
						for(size_t i2 = 0; i2 < symsn.size(); i2++) {
							if(symsn[i2] == symsd[i]) {
								b_found = (nums[0].degree(symsd[i]) >= divs[0].degree(symsd[i]));
								break;
							}
						}
					}
					if(b_found) i++;
					else symsd.erase(symsd.begin() + i);
				}
				for(size_t i = 0; i < symsd.size(); i++) {
					if(polynomial_long_division(nums[0], divs[0], symsd[i], mquo, mrem, eo2, false) && !mquo.isZero() && mrem != nums[0]) {
						if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
						if(!mrem.isZero() || divs[0].representsNonZero(true) || (eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[0], eo))) {
							if(mrem.isZero()) {
								mleft.addChild(mquo);
								divs.clear();
								b_ret = true;
								break;
							} else {
								if(mquo.isAddition()) {
									for(size_t i = 0; i < mquo.size(); ) {
										MathStructure mtest(mquo[i]);
										mtest.calculateMultiply(divs[0], eo2);
										mtest.calculateAdd(mrem, eo2);
										Number point = dos_count_points(mtest, b_unknown);
										point -= dos_count_points(mquo[i], b_unknown);
										point -= dos_count_points(mrem, b_unknown);
										if(point <= 0) {
											mquo.delChild(i + 1);
											mrem = mtest;
										} else {
											i++;
										}
									}
									if(mquo.size() == 0) mquo.clear();
									else if(mquo.size() == 1) mquo.setToChild(1);
								}
								if(!mquo.isZero()) {
									Number point = dos_count_points(nums[0], b_unknown);
									point -= dos_count_points(mquo, b_unknown);
									point -= dos_count_points(mrem, b_unknown);
									if(point > 0) {
										mleft.addChild(mquo);
										MathStructure ca, cb, mgcd;
										if(mrem.isAddition() && MathStructure::gcd(mrem, divs[0], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= mrem.size() + divs[0].size())) {
											mrem = ca;
											divs[0] = cb;
										}
										mrem.calculateDivide(divs[0], eo2);
										mleft.addChild(mrem);
										divs.clear();
										b_ret = true;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		if(!b_ret) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}

		mstruct.clear(true);
		for(size_t i = 0; i < divs.size() && (i == 0 || i_run > 1); i++) {
			if(i == 0) mstruct = nums[0];
			else mstruct.add(nums[i], i > 1);
			if(only_gcd || combine_only) {
				divs[i].inverse();
				if(i == 0) mstruct.multiply(divs[i], true);
				else mstruct.last().multiply(divs[i], true);
			} else {
				if(i == 0) mstruct.calculateDivide(divs[i], eo);
				else mstruct.last().calculateDivide(divs[i], eo);
				if(i == 0 && i_run <= 1 && mstruct.isAddition()) {
					do_simplification(mstruct, eo, true, false, false, false, limit_size, 2);
				}
			}
		}
		for(size_t i = 0; i < mleft.size(); i++) {
			if(i == 0 && mstruct.isZero()) mstruct = mleft[i];
			else mstruct.calculateAdd(mleft[i], eo);
		}
		for(size_t i = 0; i < numleft.size(); i++) {
			if(i == 0 && mstruct.isZero()) mstruct = numleft[i];
			else mstruct.calculateAdd(numleft[i], eo);
		}

		return true;
	}

	MathStructure divs;
	// find division by polynomial
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && (mstruct[i][i2][0].isAddition() || combine_divisions) && mstruct[i][i2][1].isMinusOne() && mstruct[i][i2][0].isRationalPolynomial()) {
					bool b_found = false;
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == mstruct[i][i2][0]) {
							divs[i3].number()++;
							b_found = true;
							break;
						}
					}
					if(!b_found) divs.addChild(mstruct[i][i2][0]);
					break;
				}
			}
		} else if(mstruct[i].isPower() && (mstruct[i][0].isAddition() || combine_divisions) && mstruct[i][1].isMinusOne() && mstruct[i][0].isRationalPolynomial()) {
			bool b_found = false;
			for(size_t i3 = 0; i3 < divs.size(); i3++) {
				if(divs[i3] == mstruct[i][0]) {
					divs[i3].number()++;
					b_found = true;
					break;
				}
			}
			if(!b_found) divs.addChild(mstruct[i][0]);
		}
	}

	// check if denominators is zero
	for(size_t i = 0; i < divs.size(); ) {
		if((!combine_divisions && divs[i].number().isZero()) || (!eo.assume_denominators_nonzero && !divs[i].representsNonZero(true)) || divs[i].representsZero(true)) divs.delChild(i + 1);
		else i++;
	}

	if(divs.size() == 0) return false;

	// combine numerators with same denominator
	MathStructure nums, numleft, mleft;
	nums.resizeVector(divs.size(), m_zero);
	for(size_t i = 0; i < mstruct.size(); i++) {
		bool b = false;
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && (mstruct[i][i2][0].isAddition() || combine_divisions) && mstruct[i][i2][1].isMinusOne() && mstruct[i][i2][0].isRationalPolynomial()) {
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == mstruct[i][i2][0]) {
							if(mstruct[i].size() == 1) nums[i3].addChild(m_one);
							else if(mstruct[i].size() == 2) nums[i3].addChild(mstruct[i][i2 == 0 ? 1 : 0]);
							else {nums[i3].addChild(mstruct[i]); nums[i3][nums[i3].size() - 1].delChild(i2 + 1);}
							b = true;
							break;
						}
					}
					break;
				}
			}
		} else if(mstruct[i].isPower() && (mstruct[i][0].isAddition() || combine_divisions) && mstruct[i][1].isMinusOne() && mstruct[i][0].isRationalPolynomial()) {
			for(size_t i3 = 0; i3 < divs.size(); i3++) {
				if(divs[i3] == mstruct[i][0]) {
					nums[i3].addChild(m_one);
					b = true;
					break;
				}
			}
		}
		if(!b && combine_divisions) {
			if(mstruct[i].isRationalPolynomial()) numleft.addChild(mstruct[i]);
			else mleft.addChild(mstruct[i]);
		}
	}

	EvaluationOptions eo2 = eo;
	eo2.do_polynomial_division = false;
	eo2.keep_zero_units = false;

	// do polynomial division; give points to incomplete division
	vector<long int> points(nums.size(), -1);
	bool b = false, b_ready_candidate = false;
	for(size_t i = 0; i < divs.size(); i++) {
		if(CALCULATOR->aborted()) return false;
		if(nums[i].size() > 1 && divs[i].isAddition()) {
			nums[i].setType(STRUCT_ADDITION);
			MathStructure xvar;
			get_first_symbol(nums[i], xvar);
			if(nums[i].isRationalPolynomial() && nums[i].degree(xvar).isLessThan(100) && divs[i].degree(xvar).isLessThan(100)) {
				MathStructure mquo, mrem, ca, cb;
				if(MathStructure::gcd(nums[i], divs[i], mquo, eo2, &ca, &cb, false) && !mquo.isOne()) {
					if(!mquo.representsNonZero(true) && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(mquo, eo)) {
						nums[i].clear();
					} else {
						points[i] = 1;
						b_ready_candidate = true;
						b = true;
					}
					if(ca.isOne()) {
						if(cb.isOne()) {
							nums[i].set(1, 1, 0, true);
						} else {
							nums[i] = cb;
							nums[i].inverse();
						}
					} else if(cb.isOne()) {
						nums[i] = ca;
					} else {
						nums[i] = ca;
						nums[i].calculateDivide(cb, eo);
					}
				} else if(!only_gcd && polynomial_long_division(nums[i], divs[i], xvar, mquo, mrem, eo2, false) && !mquo.isZero() && mrem != nums[i]) {
					if(mrem.isZero() && !divs[i].representsNonZero(true) && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[i], eo)) {
						nums[i].clear();
					} else {
						long int point = 1;
						if(!mrem.isZero()) point = nums[i].size();
						nums[i].set(mquo);
						if(!mrem.isZero()) {
							if(mquo.isAddition()) point -= mquo.size();
							else point--;
							if(mrem.isAddition()) point -= mrem.size();
							else point--;
							mrem.calculateDivide(divs[i], eo2);
							nums[i].calculateAdd(mrem, eo2);
						}
						b = true;
						points[i] = point;
						if(point >= 0) {b_ready_candidate = true;}
						else if(b_ready_candidate) nums[i].clear();
					}
				} else {
					nums[i].clear();
				}
			}
		} else {
			nums[i].clear();
		}
	}

	if(!b) return false;

	if(b_ready_candidate) {
		// remove polynomial divisions that inrease complexity
		for(size_t i = 0; i < nums.size(); i++) {
			if(!nums[i].isZero() && points[i] < 0) nums[i].clear();
		}
	} else {
		// no simplying polynomial division found; see if result can be combined with other terms
		b = false;
		for(size_t i = 0; i < nums.size(); i++) {
			if(!nums[i].isZero()) {
				if(b) {
					nums[i].clear();
				} else {
					long int point = points[i];
					for(size_t i2 = 0; i2 < nums[i].size(); i2++) {
						bool b2 = false;
						if(!nums[i][i2].contains(divs[i], false, false, false)) {
							MathStructure mtest1(mstruct), mtest2(nums[i][i2]);
							for(size_t i3 = 0; i3 < mtest1.size(); i3++) {
								if(!mtest1[i3].contains(divs[i], false, false, false)) {
									int ret = mtest1[i3].merge_addition(mtest2, eo);
									if(ret > 0) {
										b2 = true;
										point++;
										if(mtest1[i3].isZero()) point++;
										break;
									}
									if(ret == 0) ret = mtest2.merge_addition(mtest1[i3], eo);
									if(ret > 0) {
										b2 = true;
										point++;
										if(mtest2.isZero()) point++;
										break;
									}
								}
							}
							if(b2) break;
						}
						if(point >= 0) break;
					}
					if(point >= 0) b = true;
					else nums[i].clear();
				}
			}
		}
	}

	if(!b) return false;
	// replace terms with polynomial division result
	for(size_t i = 0; i < mstruct.size(); ) {
		bool b_del = false;
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && mstruct[i][i2][0].isAddition() && mstruct[i][i2][1].isMinusOne() && mstruct[i][i2][0].isRationalPolynomial()) {
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == mstruct[i][i2][0]) {
							b_del = !nums[i3].isZero();
							break;
						}
					}
					break;
				}
			}
		} else if(mstruct[i].isPower() && mstruct[i][0].isAddition() && mstruct[i][1].isMinusOne() && mstruct[i][0].isRationalPolynomial()) {
			for(size_t i3 = 0; i3 < divs.size(); i3++) {
				if(divs[i3] == mstruct[i][0]) {
					b_del = !nums[i3].isZero();
					break;
				}
			}
		}
		if(b_del) mstruct.delChild(i + 1);
		else i++;
	}
	for(size_t i = 0; i < nums.size(); ) {
		if(nums[i].isZero()) {
			nums.delChild(i + 1);
		} else {
			nums[i].evalSort();
			i++;
		}
	}
	if(mstruct.size() == 0 && nums.size() == 1) {
		mstruct.set(nums[0]);
	} else {
		for(size_t i = 0; i < nums.size(); i++) {
			mstruct.addChild(nums[i]);
		}
		mstruct.evalSort();
	}
	return true;
}

bool fix_intervals(MathStructure &mstruct, const EvaluationOptions &eo, bool *failed, long int min_precision, bool function_middle) {
	if(mstruct.type() == STRUCT_NUMBER) {
		if(eo.interval_calculation != INTERVAL_CALCULATION_NONE) {
			if(!mstruct.number().isInterval(false) && mstruct.number().precision() >= 0 && (CALCULATOR->usesIntervalArithmetic() || mstruct.number().precision() <= PRECISION + 10)) {
				mstruct.number().precisionToInterval();
				mstruct.setPrecision(-1);
				mstruct.numberUpdated();
				return true;
			}
		} else if(mstruct.number().isInterval(false)) {
			if(!mstruct.number().intervalToPrecision(min_precision)) {
				if(failed) *failed = true;
				return false;
			}
			mstruct.numberUpdated();
			return true;
		}
	} else if(mstruct.type() == STRUCT_FUNCTION && (mstruct.function() == CALCULATOR->f_interval || mstruct.function() == CALCULATOR->f_uncertainty)) {
		if(eo.interval_calculation == INTERVAL_CALCULATION_NONE) {
			bool b = mstruct.calculateFunctions(eo, false);
			if(b) {
				fix_intervals(mstruct, eo, failed, function_middle);
				return true;
			} else if(function_middle && mstruct.type() == STRUCT_FUNCTION && mstruct.function() == CALCULATOR->f_interval && mstruct.size() == 2) {
				mstruct.setType(STRUCT_ADDITION);
				mstruct.divide(nr_two);
				return true;
			} else if(function_middle && mstruct.type() == STRUCT_FUNCTION && mstruct.function() == CALCULATOR->f_uncertainty && mstruct.size() >= 1) {
				mstruct.setToChild(1, true);
				return true;
			}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(fix_intervals(mstruct[i], eo, failed, function_middle)) {
				mstruct.childUpdated(i + 1);
				b = true;
			}
		}
		return b;
	}
	return false;
}

bool contains_zero_unit(const MathStructure &mstruct);
bool contains_zero_unit(const MathStructure &mstruct) {
	if(mstruct.isMultiplication() && mstruct.size() > 1 && mstruct[0].isZero()) {
		bool b = true;
		for(size_t i = 1; i < mstruct.size(); i++) {
			if(!mstruct[i].isUnit_exp()) {
				b = false;
				break;
			}
		}
		if(b) return true;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_zero_unit(mstruct[i])) return true;
	}
	return false;
}

bool test_var_int(const MathStructure &mstruct, bool *v = NULL) {
	if(mstruct.isVariable() && (mstruct.variable() == CALCULATOR->v_e || mstruct.variable() == CALCULATOR->v_pi)) {
		if(!v) return true;
		if(*v) return false;
		else *v = true;
		return true;
	}
	if(mstruct.isNumber() && mstruct.number().isReal()) {
		if(!v) {
			if(mstruct.number().isInterval()) {
				Number nr_int(mstruct.number());
				nr_int.round();
				return mstruct.number() < nr_int || mstruct.number() > nr_int;
			}
			if(mstruct.isApproximate()) {
				Number nr_f = mstruct.number();
				nr_f.floor();
				Number nr_c(nr_f);
				nr_c++;
				return COMPARISON_IS_NOT_EQUAL(mstruct.number().compareApproximately(nr_f)) && COMPARISON_IS_NOT_EQUAL(mstruct.number().compareApproximately(nr_c));
			}
			return !mstruct.number().isInterval() && !mstruct.number().isInteger();
		}
		if(mstruct.isApproximate()) return false;
		return mstruct.number().isRational();
	}
	if(mstruct.isMultiplication() || mstruct.isAddition() || (mstruct.isPower() && mstruct[1].isInteger())) {
		bool v2 = false;
		if(!v) v = &v2;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!test_var_int(mstruct[i], v)) return false;
		}
		if(*v) return true;
	}
	return false;
}

bool test_non_integer(const MathStructure &mstruct, const EvaluationOptions&) {
	if(test_var_int(mstruct)) return true;
	if(!mstruct.isApproximate()) {
		if((mstruct.isMultiplication() || mstruct.isAddition()) && mstruct.size() >= 2 && mstruct[0].isNumber() && mstruct[0].number().isReal() && !mstruct[0].number().isInterval() && !mstruct[0].number().isInteger()) {
			for(size_t i = 1; i < mstruct.size(); i++) {
				if(!mstruct[i].representsInteger()) return false;
			}
			return true;
		}
	}
	return false;
}

bool MathStructure::calculatesub(const EvaluationOptions &eo, const EvaluationOptions &feo, bool recursive, MathStructure *mparent, size_t index_this) {
	if(b_protected) return false;
	bool b = false;

	switch(m_type) {
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if((eo.approximation == APPROXIMATION_APPROXIMATE || (!o_variable->isApproximate() && !((KnownVariable*) o_variable)->get().containsInterval(true, false, false, 0, true))) && !((KnownVariable*) o_variable)->get().isAborted()) {
					set(((KnownVariable*) o_variable)->get());
					unformat(eo);
					if(eo.calculate_functions) {
						calculateFunctions(feo);
					}
					fix_intervals(*this, feo, NULL, PRECISION);
					b = true;
					calculatesub(eo, feo, true, mparent, index_this);
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILD(1).calculatesub(eo, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).merge_power(CHILD(1), eo) >= 1) {
				b = true;
				setToChild(1, false, mparent, index_this + 1);
			}
			break;
		}
		case STRUCT_ADDITION: {
			MERGE_RECURSE
			bool found_nonlinear_relations = false;
			if(eo.sync_units && (syncUnits(false, &found_nonlinear_relations, true, feo) || (found_nonlinear_relations && eo.sync_nonlinear_unit_relations))) {
				if(found_nonlinear_relations && eo.sync_nonlinear_unit_relations) {
					EvaluationOptions eo2 = eo;
					eo2.expand = -3;
					eo2.combine_divisions = false;
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculatesub(eo2, feo, true, this, i);
						CHILD(i).factorizeUnits();
					}
					CHILDREN_UPDATED;
					syncUnits(true, NULL, true, feo);
				}
				unformat(eo);
				MERGE_RECURSE
			}
			MERGE_ALL(merge_addition, try_add)
			MERGE_ALL2
			break;
		}
		case STRUCT_MULTIPLICATION: {

			MERGE_RECURSE
			if(eo.sync_units && syncUnits(eo.sync_nonlinear_unit_relations, NULL, true, feo)) {
				unformat(eo);
				MERGE_RECURSE
			}

			if(representsNonMatrix()) {
				if(SIZE > 2) {
					int nonintervals = 0, had_interval = false;
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).isNumber()) {
							if(CHILD(i).number().isInterval(false)) {
								had_interval = true;
								if(nonintervals >= 2) break;
							} else if(nonintervals < 2) {
								nonintervals++;
								if(nonintervals == 2 && had_interval) break;
							}
						}
					}
					if(had_interval && nonintervals >= 2) evalSort(false);
				}

				MERGE_ALL(merge_multiplication, try_multiply)

			} else {
				size_t i2, i3 = SIZE;
				for(size_t i = 0; i < SIZE - 1; i++) {
					i2 = i + 1;
					try_multiply_matrix:
					bool b_matrix = !CHILD(i).representsNonMatrix();
					if(i2 < i) {
						for(; ; i2--) {
							int r = CHILD(i2).merge_multiplication(CHILD(i), eo, this, i2, i);
							if(r == 0) {
								SWAP_CHILDREN(i2, i);
								r = CHILD(i2).merge_multiplication(CHILD(i), eo, this, i2, i, true);
								if(r < 1) {
									SWAP_CHILDREN(i2, i);
								}
							}
							if(r >= 1) {
								ERASE(i);
								b = true;
								i3 = i;
								i = i2;
								i2 = 0;
								goto try_multiply_matrix;
							}
							if(i2 == 0) break;
							if(b_matrix && !CHILD(i2).representsNonMatrix()) break;
						}
					}
					bool had_matrix = false;
					for(i2 = i + 1; i2 < SIZE; i2++) {
						if(had_matrix && !CHILD(i2).representsNonMatrix()) continue;
						int r = CHILD(i).merge_multiplication(CHILD(i2), eo, this, i, i2);
						if(r == 0) {
							SWAP_CHILDREN(i, i2);
							r = CHILD(i).merge_multiplication(CHILD(i2), eo, this, i, i2, true);
							if(r < 1) {
								SWAP_CHILDREN(i, i2);
							} else if(r == 2) {
								r = 3;
							} else if(r == 3) {
								r = 2;
							}
						}
						if(r >= 1) {
							ERASE(i2);
							b = true;
							if(r != 2) {
								i2 = 0;
								goto try_multiply_matrix;
							}
							i2--;
						}
						if(i == SIZE - 1) break;
						if(b_matrix && !CHILD(i2).representsNonMatrix()) had_matrix = true;
					}
					if(i3 < SIZE) {
						if(i3 == SIZE - 1) break;
						i = i3;
						i3 = SIZE;
						i2 = i + 1;
						goto try_multiply_matrix;
					}
				}
			}

			MERGE_ALL2

			break;
		}
		case STRUCT_BITWISE_AND: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_and, try_bitand)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_OR: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_or, try_bitor)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_XOR: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_xor, try_bitxor)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_NOT: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILDREN_UPDATED;
			}
			switch(CHILD(0).type()) {
				case STRUCT_NUMBER: {
					Number nr(CHILD(0).number());
					if(nr.bitNot() && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || CHILD(0).number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || CHILD(0).number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || CHILD(0).number().includesInfinity())) {
						set(nr, true);
					}
					break;
				}
				case STRUCT_VECTOR: {
					SET_CHILD_MAP(0);
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).setLogicalNot();
					}
					break;
				}
				case STRUCT_BITWISE_NOT: {
					setToChild(1);
					setToChild(1);
					break;
				}
				default: {}
			}
			break;
		}
		case STRUCT_LOGICAL_AND: {
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculatesub(eo, feo, true, this, i);
					CHILD_UPDATED(i)
					if(CHILD(i).isZero()) {
						clear(true);
						b = true;
						break;
					}
				}
				if(b) break;
			}
			MERGE_ALL(merge_logical_and, try_logand)
			if(SIZE == 1) {
				if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					setToChild(1, false, mparent, index_this + 1);
				} else if(CHILD(0).representsNonZero()) {
					set(1, 1, 0, true);
				} else if(CHILD(0).isZero()) {
					clear(true);
				} else {
					APPEND(m_zero);
					m_type = STRUCT_COMPARISON;
					ct_comp = COMPARISON_NOT_EQUALS;
				}
			} else if(SIZE == 0) {
				clear(true);
			} else {
				evalSort();
			}
			break;
		}
		case STRUCT_LOGICAL_OR: {
			bool isResistance = false;
			switch(CHILD(0).type()) {
				case STRUCT_MULTIPLICATION: {
					if(CHILD(0)[1] != 0 && CHILD(0)[1].unit() && CHILD(0)[1].unit()->name().find("ohm") != string::npos) {
						isResistance = true;
					}
					break;
				}
				case STRUCT_UNIT: {
					if (CHILD(0).unit() && CHILD(0).unit()->name().find("ohm") != string::npos) {
						isResistance = true;
					}
					break;
				}
				default: {}
			}

			if(isResistance) {
				MathStructure mstruct;
				for (size_t i = 0; i < SIZE; i++) {
					MathStructure mtemp(CHILD(i));
					mtemp.inverse();
					mstruct += mtemp;
				}
				mstruct.inverse();
				clear();
				set(mstruct);
				break;
			}
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculatesub(eo, feo, true, this, i);
					CHILD_UPDATED(i)
					if(CHILD(i).representsNonZero()) {
						set(1, 1, 0, true);
						b = true;
						break;
					}
				}
				if(b) break;
			}
			MERGE_ALL(merge_logical_or, try_logor)
			if(SIZE == 1) {
				if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					setToChild(1, false, mparent, index_this + 1);
				} else if(CHILD(0).representsNonZero()) {
					set(1, 1, 0, true);
				} else if(CHILD(0).isZero()) {
					clear(true);
				} else {
					APPEND(m_zero);
					m_type = STRUCT_COMPARISON;
					ct_comp = COMPARISON_NOT_EQUALS;
				}
			} else if(SIZE == 0) {
				clear(true);
			} else {
				evalSort();
			}
			break;
		}
		case STRUCT_LOGICAL_XOR: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILD(1).calculatesub(eo, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).merge_logical_xor(CHILD(1), eo) >= 1) {
				b = true;
				setToChild(1, false, mparent, index_this + 1);
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).representsNonZero()) {
				clear(true);
				b = true;
			} else if(CHILD(0).isZero()) {
				set(1, 1, 0, true);
				b = true;
			} else if(CHILD(0).isLogicalNot()) {
				setToChild(1);
				setToChild(1);
				if(!representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					add(m_zero, OPERATION_NOT_EQUALS);
					calculatesub(eo, feo, false);
				}
				b = true;
			}
			break;
		}
		case STRUCT_COMPARISON: {
			EvaluationOptions eo2 = eo;
			if(eo2.assume_denominators_nonzero == 1) eo2.assume_denominators_nonzero = false;
			if(recursive) {
				CHILD(0).calculatesub(eo2, feo, true, this, 0);
				CHILD(1).calculatesub(eo2, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(eo.sync_units && syncUnits(eo.sync_nonlinear_unit_relations, NULL, true, feo)) {
				unformat(eo);
				if(recursive) {
					CHILD(0).calculatesub(eo2, feo, true, this, 0);
					CHILD(1).calculatesub(eo2, feo, true, this, 1);
					CHILDREN_UPDATED;
				}
			}
			if(CHILD(0).isAddition() || CHILD(1).isAddition()) {
				size_t i2 = 0;
				for(size_t i = 0; !CHILD(0).isAddition() || i < CHILD(0).size(); i++) {
					if(CHILD(1).isAddition()) {
						for(; i2 < CHILD(1).size(); i2++) {
							if(CHILD(0).isAddition() && CHILD(0)[i] == CHILD(1)[i2]) {
								CHILD(0).delChild(i + 1);
								CHILD(1).delChild(i2 + 1);
								break;
							} else if(!CHILD(0).isAddition() && CHILD(0) == CHILD(1)[i2]) {
								CHILD(0).clear(true);
								CHILD(1).delChild(i2 + 1);
								break;
							}
						}
					} else if(CHILD(0)[i] == CHILD(1)) {
						CHILD(1).clear(true);
						CHILD(0).delChild(i + 1);
						break;
					}
					if(!CHILD(0).isAddition()) break;
				}
				if(CHILD(0).isAddition()) {
					if(CHILD(0).size() == 1) CHILD(0).setToChild(1, true);
					else if(CHILD(0).size() == 0) CHILD(0).clear(true);
				}
				if(CHILD(1).isAddition()) {
					if(CHILD(1).size() == 1) CHILD(1).setToChild(1, true);
					else if(CHILD(1).size() == 0) CHILD(1).clear(true);
				}
			}
			if(CHILD(0).isMultiplication() && CHILD(1).isMultiplication()) {
				size_t i1 = 0, i2 = 0;
				if(CHILD(0)[0].isNumber()) i1++;
				if(CHILD(1)[0].isNumber()) i2++;
				while(i1 < CHILD(0).size() && i2 < CHILD(1).size()) {
					if(CHILD(0)[i1] == CHILD(1)[i2] && CHILD(0)[i1].representsPositive(true)) {
						CHILD(0).delChild(i1 + 1);
						CHILD(1).delChild(i2 + 1);
					} else {
						break;
					}
				}
				if(CHILD(0).size() == 1) CHILD(0).setToChild(1, true);
				else if(CHILD(0).size() == 0) CHILD(0).set(1, 1, 0, true);
				if(CHILD(1).size() == 1) CHILD(1).setToChild(1, true);
				else if(CHILD(1).size() == 0) CHILD(1).set(1, 1, 0, true);
			}
			if(((CHILD(0).isNumber() || (CHILD(0).isVariable() && !CHILD(0).variable()->isKnown() && ((UnknownVariable*) CHILD(0).variable())->interval().isNumber())) && (CHILD(1).isNumber() || ((CHILD(1).isVariable() && !CHILD(1).variable()->isKnown() && ((UnknownVariable*) CHILD(1).variable())->interval().isNumber())))) || (CHILD(0).isDateTime() && CHILD(1).isDateTime())) {
				ComparisonResult cr;
				if(CHILD(0).isNumber()) {
					if(CHILD(1).isNumber()) cr = CHILD(1).number().compareApproximately(CHILD(0).number());
					else cr = ((UnknownVariable*) CHILD(1).variable())->interval().number().compareApproximately(CHILD(0).number());
				} else if(CHILD(1).isNumber()) {
					cr = CHILD(1).number().compareApproximately(((UnknownVariable*) CHILD(0).variable())->interval().number());
				} else if(CHILD(1).isVariable()) {
					cr = ((UnknownVariable*) CHILD(1).variable())->interval().number().compareApproximately(((UnknownVariable*) CHILD(0).variable())->interval().number());
				} else {
					cr = CHILD(1).compare(CHILD(0));
				}
				if(cr >= COMPARISON_RESULT_UNKNOWN) {
					break;
				}
				switch(ct_comp) {
					case COMPARISON_EQUALS: {
						if(cr == COMPARISON_RESULT_EQUAL) {
							set(1, 1, 0, true);
							b = true;
						} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_NOT_EQUALS: {
						if(cr == COMPARISON_RESULT_EQUAL) {
							clear(true);
							b = true;
						} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
							set(1, 1, 0, true);
							b = true;
						}
						break;
					}
					case COMPARISON_LESS: {
						if(cr == COMPARISON_RESULT_LESS) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_LESS && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_GREATER && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_GREATER: {
						if(cr == COMPARISON_RESULT_GREATER) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_GREATER && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_LESS && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
				}
				break;
			}
			if(!eo.test_comparisons) {
				break;
			}
			if(eo2.keep_zero_units && contains_zero_unit(*this)) {
				eo2.keep_zero_units = false;
				MathStructure mtest(*this);
				CALCULATOR->beginTemporaryStopMessages();
				mtest.calculatesub(eo2, feo, true);
				if(mtest.isNumber()) {
					CALCULATOR->endTemporaryStopMessages(true);
					set(mtest);
					b = true;
					break;
				}
				CALCULATOR->endTemporaryStopMessages();
			}
			if((CHILD(0).representsUndefined() && !CHILD(1).representsUndefined(true, true, true)) || (CHILD(1).representsUndefined() && !CHILD(0).representsUndefined(true, true, true))) {
				if(ct_comp == COMPARISON_EQUALS) {
					clear(true);
					b = true;
					break;
				} else if(ct_comp == COMPARISON_NOT_EQUALS) {
					set(1, 1, 0, true);
					b = true;
					break;
				}
			}
			if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_GREATER) && CHILD(1).isZero()) {
				if(CHILD(0).isLogicalNot() || CHILD(0).isLogicalAnd() || CHILD(0).isLogicalOr() || CHILD(0).isLogicalXor() || CHILD(0).isComparison()) {
					if(ct_comp == COMPARISON_EQUALS_LESS) {
						ERASE(1);
						m_type = STRUCT_LOGICAL_NOT;
						calculatesub(eo, feo, false, mparent, index_this);
					} else {
						setToChild(1, false, mparent, index_this + 1);
					}
					b = true;
				}
			} else if((ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_LESS) && CHILD(0).isZero()) {
				if(CHILD(1).isLogicalNot() || CHILD(1).isLogicalAnd() || CHILD(1).isLogicalOr() || CHILD(1).isLogicalXor() || CHILD(1).isComparison()) {
					if(ct_comp == COMPARISON_EQUALS_GREATER) {
						ERASE(0);
						m_type = STRUCT_LOGICAL_NOT;
						calculatesub(eo, feo, false, mparent, index_this);
					} else {
						setToChild(2, false, mparent, index_this + 1);
					}
					b = true;
				}
			}
			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				if((CHILD(0).representsReal(true) && CHILD(1).representsComplex(true)) || (CHILD(1).representsReal(true) && CHILD(0).representsComplex(true))) {
					if(ct_comp == COMPARISON_EQUALS) {
						clear(true);
					} else {
						set(1, 1, 0, true);
					}
					b = true;
				} else if((CHILD(0).representsZero(true) && CHILD(1).representsZero(true))) {
					if(ct_comp != COMPARISON_EQUALS) {
						clear(true);
					} else {
						set(1, 1, 0, true);
					}
					b = true;
				} else if(CHILD(0).isVariable() && !CHILD(0).variable()->isKnown() && CHILD(0).representsInteger() && test_non_integer(CHILD(1), eo)) {
					if(ct_comp == COMPARISON_EQUALS) clear(true);
					else set(1, 1, 0, true);
					b = true;
				}
			}
			if(b) break;
			if(CHILD(1).isNumber() && CHILD(0).isVariable() && !CHILD(0).variable()->isKnown()) {
				Assumptions *ass = ((UnknownVariable*) CHILD(0).variable())->assumptions();
				if(ass && ass->min()) {
					bool b_inc = ass->includeEqualsMin();
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {clear(true); b = true;}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_LESS: {
							if(CHILD(1).number() <= *ass->min()) {clear(true); b = true;}

							break;
						}
						case COMPARISON_GREATER: {
							if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(b_inc && CHILD(1).number() == *ass->min()) {ct_comp = COMPARISON_EQUALS; b = true;}
							else if((b_inc && CHILD(1).number() < *ass->min()) || (!b_inc && CHILD(1).number() <= *ass->min())) {clear(true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(CHILD(1).number() <= *ass->min()) {set(1, 1, 0, true); b = true;}
							break;
						}
					}
				}
				if(ass && ass->max() && isComparison()) {
					bool b_inc = ass->includeEqualsMax();
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {clear(true); b = true;}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_LESS: {
							if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_GREATER: {
							if(CHILD(1).number() >= *ass->max()) {clear(true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							if(CHILD(1).number() >= *ass->max()) {set(1, 1, 0, true); b = true;}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							if(b_inc && CHILD(1).number() == *ass->max()) {ct_comp = COMPARISON_EQUALS; b = true;}
							else if((b_inc && CHILD(1).number() > *ass->max()) || (!b_inc && CHILD(1).number() >= *ass->max())) {clear(true); b = true;}
							break;
						}
					}
				}
			}
			if(b) break;
			if(eo.approximation == APPROXIMATION_EXACT && eo.test_comparisons > 0) {
				bool b_intval = CALCULATOR->usesIntervalArithmetic();
				bool b_failed = false;
				EvaluationOptions eo3 = feo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				eo3.approximation = APPROXIMATION_APPROXIMATE;
				eo2.test_comparisons = false;
				MathStructure mtest(*this);
				for(int i = 0; i < 2; i++) {
					CALCULATOR->beginTemporaryEnableIntervalArithmetic();
					int b_ass = (i == 0 ? 2 : contains_ass_intval(mtest));
					if(b_ass == 0 || !CALCULATOR->usesIntervalArithmetic()) {CALCULATOR->endTemporaryEnableIntervalArithmetic(); break;}
					replace_interval_unknowns(mtest, i > 0);
					if(i == 0 && !b_intval) fix_intervals(mtest, eo2, &b_failed);
					if(!b_failed) {
						if(i == 0 && mtest[0].isAddition() && mtest[0].size() > 1 && mtest[1].isZero()) {
							mtest[1] = mtest[0][0];
							mtest[1].negate();
							mtest[0].delChild(1, true);
						}
						CALCULATOR->beginTemporaryStopMessages();
						if(b_ass == 2) mtest[0].calculateFunctions(eo3);
						mtest[0].calculatesub(eo2, eo3, true);
						if(b_ass == 2) mtest[1].calculateFunctions(eo3);
						mtest[1].calculatesub(eo2, eo3, true);
						CALCULATOR->endTemporaryEnableIntervalArithmetic();
						mtest.childrenUpdated();
						if(CALCULATOR->endTemporaryStopMessages(NULL, NULL, MESSAGE_ERROR) == 0) {
							eo2.approximation = eo.approximation;
							eo2.test_comparisons = -1;
							mtest.calculatesub(eo2, feo, false);
							if(mtest.isNumber()) {
								if(mtest.isZero()) clear(true);
								else set(1, 1, 0, true);
								b = true;
								return b;
							}
						}
					} else {
						CALCULATOR->endTemporaryEnableIntervalArithmetic();
						break;
					}
				}
			}
			eo2 = eo;
			if(eo2.assume_denominators_nonzero == 1) eo2.assume_denominators_nonzero = false;
			bool mtest_new = false;
			MathStructure *mtest;
			if(!CHILD(1).isZero()) {
				if(!eo.isolate_x || find_x_var().isUndefined()) {
					CHILD(0).calculateSubtract(CHILD(1), eo2);
					CHILD(1).clear();
					mtest = &CHILD(0);
					mtest->ref();
				} else {
					mtest = new MathStructure(CHILD(0));
					mtest->calculateSubtract(CHILD(1), eo2);
					remove_rad_unit(*mtest, eo2);
					mtest_new = true;
				}
			} else {
				mtest = &CHILD(0);
				mtest->ref();
			}
			int incomp = 0;
			if(mtest->isAddition()) {
				mtest->evalSort(true);
				incomp = compare_check_incompability(mtest);
				if(incomp == 1 && !mtest_new) {
					mtest->unref();
					mtest = new MathStructure(CHILD(0));
					if(remove_rad_unit(*mtest, eo2)) {
						mtest_new = true;
						if(mtest->isAddition()) {
							mtest->evalSort(true);
							incomp = compare_check_incompability(mtest);
						}
					}
				}
			}
			if(incomp <= 0) {
				if(mtest_new && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
					bool a_pos = CHILD(0).representsPositive(true);
					bool a_nneg = a_pos || CHILD(0).representsNonNegative(true);
					bool a_neg = !a_nneg && CHILD(0).representsNegative(true);
					bool a_npos = !a_pos && (a_neg || CHILD(0).representsNonPositive(true));
					bool b_pos = CHILD(1).representsPositive(true);
					bool b_nneg = b_pos || CHILD(1).representsNonNegative(true);
					bool b_neg = !b_nneg && CHILD(1).representsNegative(true);
					bool b_npos = !b_pos && (b_neg || CHILD(1).representsNonPositive(true));
					if(isApproximate()) {
						if((a_pos && b_neg) || (a_neg && b_pos)) {
							incomp = 1;
						}
					} else {
						if((a_pos && b_npos) || (a_npos && b_pos) || (a_nneg && b_neg) || (a_neg && b_nneg)) {
							incomp = 1;
						}
					}
				} else if(incomp < 0) {
					mtest->unref();
					break;
				}
			}

			switch(ct_comp) {
				case COMPARISON_EQUALS: {
					if(incomp > 0) {
						clear(true);
						b = true;
					} else if(mtest->representsZero(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonZero(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_NOT_EQUALS: {
					if(incomp > 0) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonZero(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsZero(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_LESS: {
					if(incomp > 0) {
					} else if(mtest->representsNegative(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonNegative(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_GREATER: {
					if(incomp > 0) {
					} else if(mtest->representsPositive(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonPositive(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_EQUALS_LESS: {
					if(incomp > 0) {
					} else if(mtest->representsNonPositive(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsPositive(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_EQUALS_GREATER: {
					if(incomp > 0) {
					} else if(mtest->representsNonNegative(true)) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNegative(true)) {
						clear(true);
						b = true;
					}
					break;
				}
			}
			mtest->unref();
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_abs || o_function == CALCULATOR->f_root || o_function == CALCULATOR->f_interval || o_function == CALCULATOR->f_uncertainty || o_function == CALCULATOR->f_signum || o_function == CALCULATOR->f_dirac || o_function == CALCULATOR->f_heaviside) {
				b = calculateFunctions(eo, false);
				if(b) {
					calculatesub(eo, feo, true, mparent, index_this);
					break;
				}
			} else if(o_function == CALCULATOR->f_stripunits) {
				b = calculateFunctions(eo, false);
				if(b) calculatesub(eo, feo, true, mparent, index_this);
				break;
			}
		}
		default: {
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).calculatesub(eo, feo, true, this, i)) b = true;
				}
				CHILDREN_UPDATED;
			}
			if(eo.sync_units && syncUnits(eo.sync_nonlinear_unit_relations, NULL, true, feo)) {
				unformat(eo);
				if(recursive) {
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).calculatesub(eo, feo, true, this, i)) b = true;
					}
					CHILDREN_UPDATED;
				}
			}
		}
	}
	return b;
}

#define MERGE_INDEX(FUNC, TRY_LABEL)	bool b = false;\
					TRY_LABEL:\
					for(size_t i = 0; i < index; i++) {\
						if(CALCULATOR->aborted()) break; \
						int r = CHILD(i).FUNC(CHILD(index), eo, this, i, index);\
						if(r == 0) {\
							SWAP_CHILDREN(i, index);\
							r = CHILD(i).FUNC(CHILD(index), eo, this, i, index, true);\
							if(r < 1) {\
								SWAP_CHILDREN(i, index);\
							} else if(r == 2) {\
								r = 3;\
							} else if(r == 3) {\
								r = 2;\
							}\
						}\
						if(r >= 1) {\
							ERASE(index);\
							if(!b && r == 2) {\
								b = true;\
								index = SIZE;\
								break;\
							} else {\
								b = true;\
								index = i;\
								goto TRY_LABEL;\
							}\
						}\
					}\
					for(size_t i = index + 1; i < SIZE; i++) {\
						if(CALCULATOR->aborted()) break; \
						int r = CHILD(index).FUNC(CHILD(i), eo, this, index, i);\
						if(r == 0) {\
							SWAP_CHILDREN(index, i);\
							r = CHILD(index).FUNC(CHILD(i), eo, this, index, i, true);\
							if(r < 1) {\
								SWAP_CHILDREN(index, i);\
							} else if(r == 2) {\
								r = 3;\
							} else if(r == 3) {\
								r = 2;\
							}\
						}\
						if(r >= 1) {\
							ERASE(i);\
							if(!b && r == 3) {\
								b = true;\
								break;\
							}\
							b = true;\
							if(r != 2) {\
								goto TRY_LABEL;\
							}\
							i--;\
						}\
					}

#define MERGE_INDEX2			if(b && check_size) {\
						if(SIZE == 1) {\
							setToChild(1, false, mparent, index_this + 1);\
						} else if(SIZE == 0) {\
							clear(true);\
						} else {\
							evalSort();\
						}\
						return true;\
					} else {\
						evalSort();\
						return b;\
					}


bool MathStructure::calculateMergeIndex(size_t index, const EvaluationOptions &eo, const EvaluationOptions &feo, MathStructure *mparent, size_t index_this) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			return calculateMultiplyIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_ADDITION: {
			return calculateAddIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_POWER: {
			return calculateRaiseExponent(eo, mparent, index_this);
		}
		case STRUCT_LOGICAL_AND: {
			return calculateLogicalAndIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_LOGICAL_OR: {
			return calculateLogicalOrIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_LOGICAL_XOR: {
			return calculateLogicalXorLast(eo, mparent, index_this);
		}
		case STRUCT_BITWISE_AND: {
			return calculateBitwiseAndIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_BITWISE_OR: {
			return calculateBitwiseOrIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_BITWISE_XOR: {
			return calculateBitwiseXorIndex(index, eo, true, mparent, index_this);
		}
		default: {}
	}
	return calculatesub(eo, feo, false, mparent, index_this);
}
bool MathStructure::calculateLogicalOrLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateLogicalOrIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateLogicalOrIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isLogicalOr()) {
		CALCULATOR->error(true, "calculateLogicalOrIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_logical_or, try_logical_or_index)

	if(b && check_size) {
		if(SIZE == 1) {
			if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
				setToChild(1, false, mparent, index_this + 1);
			} else if(CHILD(0).representsPositive()) {
				clear(true);
				o_number.setTrue();
			} else if(CHILD(0).representsNonPositive()) {
				clear(true);
				o_number.setFalse();
			} else {
				APPEND(m_zero);
				m_type = STRUCT_COMPARISON;
				ct_comp = COMPARISON_GREATER;
			}
		} else if(SIZE == 0) {
			clear(true);
		} else {
			evalSort();
		}
		return true;
	} else {
		evalSort();
		return false;
	}

}
bool MathStructure::calculateLogicalOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mor, OPERATION_LOGICAL_OR, true);
	LAST.evalSort();
	return calculateLogicalOrIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateLogicalXorLast(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {

	if(!isLogicalXor()) {
		CALCULATOR->error(true, "calculateLogicalXorLast() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(CHILD(0).merge_logical_xor(CHILD(1), eo, this, 0, 1) >= 1) {
		if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
			setToChild(1, false, mparent, index_this + 1);
		} else if(CHILD(0).representsPositive()) {
			clear(true);
			o_number.setTrue();
		} else if(CHILD(0).representsNonPositive()) {
			clear(true);
			o_number.setFalse();
		} else {
			APPEND(m_zero);
			m_type = STRUCT_COMPARISON;
			ct_comp = COMPARISON_GREATER;
		}
		return true;
	}
	return false;

}
bool MathStructure::calculateLogicalXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mxor, OPERATION_LOGICAL_XOR);
	LAST.evalSort();
	return calculateLogicalXorLast(eo, mparent, index_this);
}
bool MathStructure::calculateLogicalAndLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateLogicalAndIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateLogicalAndIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isLogicalAnd()) {
		CALCULATOR->error(true, "calculateLogicalAndIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_logical_and, try_logical_and_index)

	if(b && check_size) {
		if(SIZE == 1) {
			if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
				setToChild(1, false, mparent, index_this + 1);
			} else if(CHILD(0).representsPositive()) {
				clear(true);
				o_number.setTrue();
			} else if(CHILD(0).representsNonPositive()) {
				clear(true);
				o_number.setFalse();
			} else {
				APPEND(m_zero);
				m_type = STRUCT_COMPARISON;
				ct_comp = COMPARISON_GREATER;
			}
		} else if(SIZE == 0) {
			clear(true);
		} else {
			evalSort();
		}
		return true;
	} else {
		evalSort();
		return false;
	}

}
bool MathStructure::calculateLogicalAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mand, OPERATION_LOGICAL_AND, true);
	LAST.evalSort();
	return calculateLogicalAndIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateInverse(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	return calculateRaise(m_minus_one, eo, mparent, index_this);
}
bool MathStructure::calculateNegate(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.negate() && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
		if(!isMultiplication()) transform(STRUCT_MULTIPLICATION);
		PREPEND(m_minus_one);
		return false;
	}
	if(!isMultiplication()) transform(STRUCT_MULTIPLICATION);
	PREPEND(m_minus_one);
	return calculateMultiplyIndex(0, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseNot(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	transform(STRUCT_LOGICAL_NOT);
	return calculatesub(eo, eo, false, mparent, index_this);
}
bool MathStructure::calculateLogicalNot(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	transform(STRUCT_BITWISE_NOT);
	return calculatesub(eo, eo, false, mparent, index_this);
}
bool MathStructure::calculateRaiseExponent(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(!isPower()) {
		CALCULATOR->error(true, "calculateRaiseExponent() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(CHILD(0).merge_power(CHILD(1), eo, this, 0, 1) >= 1) {
		setToChild(1, false, mparent, index_this + 1);
		return true;
	}
	return false;
}
bool MathStructure::calculateRaise(const MathStructure &mexp, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(mexp.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.raise(mexp.number(), eo.approximation < APPROXIMATION_APPROXIMATE) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mexp.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mexp.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mexp.number().includesInfinity())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	raise(mexp);
	LAST.evalSort();
	return calculateRaiseExponent(eo, mparent, index_this);
}
bool MathStructure::calculateBitwiseAndLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseAndIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseAndIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseAnd()) {
		CALCULATOR->error(true, "calculateBitwiseAndIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_and, try_bitwise_and_index)
	MERGE_INDEX2

}
bool MathStructure::calculateBitwiseAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mand, OPERATION_BITWISE_AND, true);
	LAST.evalSort();
	return calculateBitwiseAndIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseOrLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseOrIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseOrIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseOr()) {
		CALCULATOR->error(true, "calculateBitwiseOrIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_or, try_bitwise_or_index)
	MERGE_INDEX2

}
bool MathStructure::calculateBitwiseOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mor, OPERATION_BITWISE_OR, true);
	LAST.evalSort();
	return calculateBitwiseOrIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseXorLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseXorIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseXorIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseXor()) {
		CALCULATOR->error(true, "calculateBitwiseXorIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_xor, try_bitwise_xor_index)
	MERGE_INDEX2

}
bool MathStructure::calculateBitwiseXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mxor, OPERATION_BITWISE_XOR, true);
	LAST.evalSort();
	return calculateBitwiseXorIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateMultiplyLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateMultiplyIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateMultiplyIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isMultiplication()) {
		CALCULATOR->error(true, "calculateMultiplyIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	bool b = false;
	try_multiply_matrix_index:
	bool b_matrix = !CHILD(index).representsNonMatrix();
	if(index > 0) {
		for(size_t i = index - 1; ; i--) {
			if(CALCULATOR->aborted()) break;
			int r = CHILD(i).merge_multiplication(CHILD(index), eo, this, i, index);
			if(r == 0) {
				SWAP_CHILDREN(i, index);
				r = CHILD(i).merge_multiplication(CHILD(index), eo, this, i, index, true);
				if(r < 1) {
					SWAP_CHILDREN(i, index);
				} else if(r == 2) {
					r = 3;
				} else if(r == 3) {
					r = 2;
				}
			}
			if(r >= 1) {
				ERASE(index);
				if(!b && r == 2) {
					b = true;
					index = SIZE;
					break;
				} else {
					b = true;
					index = i;
					goto try_multiply_matrix_index;
				}
			}
			if(i == 0) break;
			if(b_matrix && !CHILD(i).representsNonMatrix()) break;
		}
	}

	bool had_matrix = false;
	for(size_t i = index + 1; i < SIZE; i++) {
		if(had_matrix && !CHILD(i).representsNonMatrix()) continue;
		if(CALCULATOR->aborted()) break;
		int r = CHILD(index).merge_multiplication(CHILD(i), eo, this, index, i);
		if(r == 0) {
			SWAP_CHILDREN(index, i);
			r = CHILD(index).merge_multiplication(CHILD(i), eo, this, index, i, true);
			if(r < 1) {
				SWAP_CHILDREN(index, i);
			} else if(r == 2) {
				r = 3;
			} else if(r == 3) {
				r = 2;
			}
		}
		if(r >= 1) {
			ERASE(i);
			if(!b && r == 3) {
				b = true;
				break;
			}
			b = true;
			if(r != 2) {
				goto try_multiply_matrix_index;
			}
			i--;
		}
		if(i == SIZE - 1) break;
		if(b_matrix && !CHILD(i).representsNonMatrix()) had_matrix = true;
	}

	MERGE_INDEX2

}
bool MathStructure::calculateMultiply(const MathStructure &mmul, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(mmul.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.multiply(mmul.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mmul.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mmul.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mmul.number().includesInfinity())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	multiply(mmul, true);
	LAST.evalSort();
	return calculateMultiplyIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateDivide(const MathStructure &mdiv, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(mdiv.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.divide(mdiv.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mdiv.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mdiv.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || o_number.includesInfinity() || mdiv.number().includesInfinity())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	MathStructure *mmul = new MathStructure(mdiv);
	mmul->evalSort();
	multiply_nocopy(mmul, true);
	LAST.calculateInverse(eo, this, SIZE - 1);
	return calculateMultiplyIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateAddLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateAddIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateAddIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isAddition()) {
		CALCULATOR->error(true, "calculateAddIndex() error: %s. %s", format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_addition, try_add_index)
	MERGE_INDEX2

}
bool MathStructure::calculateAdd(const MathStructure &madd, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(madd.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.add(madd.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || madd.number().isApproximate())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	add(madd, true);
	LAST.evalSort();
	return calculateAddIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateSubtract(const MathStructure &msub, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(msub.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.subtract(msub.number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || msub.number().isApproximate())) {
			o_number = nr;
			numberUpdated();
			return true;
		}
	}
	MathStructure *madd = new MathStructure(msub);
	madd->evalSort();
	add_nocopy(madd, true);
	LAST.calculateNegate(eo, this, SIZE - 1);
	return calculateAddIndex(SIZE - 1, eo, true, mparent, index_this);
}

bool MathStructure::calculateFunctions(const EvaluationOptions &eo, bool recursive, bool do_unformat) {

	if(m_type == STRUCT_FUNCTION && o_function != eo.protected_function) {

		if(function_value) {
			function_value->unref();
			function_value = NULL;
		}

		if(!o_function->testArgumentCount(SIZE)) {
			return false;
		}

		if(o_function->maxargs() > -1 && (long int) SIZE > o_function->maxargs()) {
			if(o_function->maxargs() == 1 && o_function->getArgumentDefinition(1) && o_function->getArgumentDefinition(1)->handlesVector()) {
				bool b = false;
				for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
					CHILD(0)[i2].transform(o_function);
					if(CHILD(0)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
					CHILD(0).childUpdated(i2 + 1);
				}
				SET_CHILD_MAP(0)
				return b;
			}
			REDUCE(o_function->maxargs());
		}
		m_type = STRUCT_VECTOR;
		Argument *arg = NULL, *last_arg = NULL;
		int last_i = 0;

		for(size_t i = 0; i < SIZE; i++) {
			arg = o_function->getArgumentDefinition(i + 1);
			if(arg) {
				last_arg = arg;
				last_i = i;
				if(i > 0 && arg->type() == ARGUMENT_TYPE_SYMBOLIC && (CHILD(i).isZero() || CHILD(i).isUndefined())) {
					CHILD(i).set(CHILD(0).find_x_var(), true);
					if(CHILD(i).isUndefined() && CHILD(0).isVariable() && CHILD(0).variable()->isKnown()) CHILD(i).set(((KnownVariable*) CHILD(0).variable())->get().find_x_var(), true);
					if(CHILD(i).isUndefined()) {
						CALCULATOR->beginTemporaryStopMessages();
						MathStructure mtest(CHILD(0));
						mtest.eval(eo);
						CHILD(i).set(mtest.find_x_var(), true);
						CALCULATOR->endTemporaryStopMessages();
					}
					if(CHILD(i).isUndefined()) {
						CALCULATOR->error(false, _("No unknown variable/symbol was found."), NULL);
						CHILD(i).set(CALCULATOR->v_x, true);
					}
				}
				if(!arg->test(CHILD(i), i + 1, o_function, eo)) {
					if(arg->handlesVector() && CHILD(i).isVector()) {
						bool b = false;
						for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
							CHILD(i)[i2].transform(o_function);
							for(size_t i3 = 0; i3 < SIZE; i3++) {
								if(i3 < i) CHILD(i)[i2].insertChild(CHILD(i3), i3 + 1);
								else if(i3 > i) CHILD(i)[i2].addChild(CHILD(i3));
							}
							if(CHILD(i)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
							CHILD(i).childUpdated(i2 + 1);
						}
						SET_CHILD_MAP(i);
						return b;
					}
					m_type = STRUCT_FUNCTION;
					CHILD_UPDATED(i);
					return false;
				} else {
					CHILD_UPDATED(i);
				}
				if(arg->handlesVector()) {
					if(arg->tests() && !CHILD(i).isVector() && !CHILD(i).representsScalar()) {
						CHILD(i).eval(eo);
						CHILD_UPDATED(i);
					}
					if(CHILD(i).isVector()) {
						bool b = false;
						for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
							CHILD(i)[i2].transform(o_function);
							for(size_t i3 = 0; i3 < SIZE; i3++) {
								if(i3 < i) CHILD(i)[i2].insertChild(CHILD(i3), i3 + 1);
								else if(i3 > i) CHILD(i)[i2].addChild(CHILD(i3));
							}
							if(CHILD(i)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
							CHILD(i).childUpdated(i2 + 1);
						}
						SET_CHILD_MAP(i);
						return b;
					}
				}
			}
		}

		if(last_arg && o_function->maxargs() < 0 && last_i >= o_function->minargs()) {
			for(size_t i = last_i + 1; i < SIZE; i++) {
				if(!last_arg->test(CHILD(i), i + 1, o_function, eo)) {
					m_type = STRUCT_FUNCTION;
					CHILD_UPDATED(i);
					return false;
				} else {
					CHILD_UPDATED(i);
				}
			}
		}

		if(!o_function->testCondition(*this)) {
			m_type = STRUCT_FUNCTION;
			return false;
		}
		MathStructure *mstruct = new MathStructure();
		int ret = o_function->calculate(*mstruct, *this, eo);
		if(ret > 0) {
			set_nocopy(*mstruct, true);
			if(recursive) calculateFunctions(eo);
			mstruct->unref();
			if(do_unformat) unformat(eo);
			return true;
		} else {
			if(ret < 0) {
				ret = -ret;
				if(o_function->maxargs() > 0 && ret > o_function->maxargs()) {
					if(mstruct->isVector()) {
						if(do_unformat) mstruct->unformat(eo);
						for(size_t arg_i = 1; arg_i <= SIZE && arg_i <= mstruct->size(); arg_i++) {
							mstruct->getChild(arg_i)->ref();
							setChild_nocopy(mstruct->getChild(arg_i), arg_i);
						}
					}
				} else if(ret <= (long int) SIZE) {
					if(do_unformat) mstruct->unformat(eo);
					mstruct->ref();
					setChild_nocopy(mstruct, ret);
				}
			}
			/*if(eo.approximation == APPROXIMATION_EXACT) {
				mstruct->clear();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				CALCULATOR->beginTemporaryStopMessages();
				if(o_function->calculate(*mstruct, *this, eo2) > 0) {
					function_value = mstruct;
					function_value->ref();
					function_value->calculateFunctions(eo2);
				}
				if(CALCULATOR->endTemporaryStopMessages() > 0 && function_value) {
					function_value->unref();
					function_value = NULL;
				}
			}*/
			m_type = STRUCT_FUNCTION;
			mstruct->unref();
			for(size_t i = 0; i < SIZE; i++) {
				arg = o_function->getArgumentDefinition(i + 1);
				if(arg && arg->handlesVector()) {
					if(!CHILD(i).isVector()) {
						CHILD(i).calculatesub(eo, eo, false);
						if(!CHILD(i).isVector()) return false;
					}
					bool b = false;
					for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
						CHILD(i)[i2].transform(o_function);
						for(size_t i3 = 0; i3 < SIZE; i3++) {
							if(i3 < i) CHILD(i)[i2].insertChild(CHILD(i3), i3 + 1);
							else if(i3 > i) CHILD(i)[i2].addChild(CHILD(i3));
						}
						if(CHILD(i)[i2].calculateFunctions(eo, recursive, do_unformat)) b = true;
						CHILD(i).childUpdated(i2 + 1);
					}
					SET_CHILD_MAP(i);
					return b;
				}
			}
			return false;
		}
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			if(CHILD(i).calculateFunctions(eo, recursive, do_unformat)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
	}
	return b;

}

int evalSortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, bool b_abs = false);
int evalSortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, bool b_abs) {
	if(parent.isMultiplication()) {
		if((!mstruct1.representsNonMatrix() && !mstruct2.representsScalar()) || (!mstruct2.representsNonMatrix() && !mstruct1.representsScalar())) {
			return 0;
		}
	}
	if(b_abs || parent.isAddition()) {
		if(mstruct1.isMultiplication() && mstruct1.size() > 0) {
			size_t start = 0;
			while(mstruct1[start].isNumber() && mstruct1.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct2.isMultiplication()) {
				if(mstruct2.size() < 1) return -1;
				size_t start2 = 0;
				while(mstruct2[start2].isNumber() && mstruct2.size() > start2 + 1) {
					start2++;
				}
				for(size_t i = 0; ; i++) {
					if(i + start2 >= mstruct2.size()) {
						if(i + start >= mstruct1.size()) {
							if(start2 == start) {
								for(size_t i3 = 0; i3 < start; i3++) {
									i2 = evalSortCompare(mstruct1[i3], mstruct2[i3], parent, b_abs);
									if(i2 != 0) return i2;
								}
								return 0;
							}
							if(start2 > start) return -1;
						}
						return 1;
					}
					if(i + start >= mstruct1.size()) return -1;
					i2 = evalSortCompare(mstruct1[i + start], mstruct2[i + start2], parent, b_abs);
					if(i2 != 0) return i2;
				}
				if(mstruct1.size() - start == mstruct2.size() - start2) return 0;
				return -1;
			} else {
				i2 = evalSortCompare(mstruct1[start], mstruct2, parent, b_abs);
				if(i2 != 0) return i2;
				if(b_abs && start == 1 && (mstruct1[0].isMinusOne() || mstruct1[0].isOne())) return 0;
				return 1;
			}
		} else if(mstruct2.isMultiplication() && mstruct2.size() > 0) {
			size_t start = 0;
			while(mstruct2[start].isNumber() && mstruct2.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct1.isMultiplication()) {
				return 1;
			} else {
				i2 = evalSortCompare(mstruct1, mstruct2[start], parent, b_abs);
				if(i2 != 0) return i2;
				if(b_abs && start == 1 && (mstruct2[0].isMinusOne() || mstruct2[0].isOne())) return 0;
				return -1;
			}
		}
	}
	if(mstruct1.type() != mstruct2.type()) {
		if(!parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		if(!parent.isMultiplication() || (!mstruct1.isNumber() && !mstruct2.isNumber())) {
			if(mstruct2.isPower()) {
				int i = evalSortCompare(mstruct1, mstruct2[0], parent, b_abs);
				if(i == 0) {
					return evalSortCompare(m_one, mstruct2[1], parent, b_abs);
				}
				return i;
			}
			if(mstruct1.isPower()) {
				int i = evalSortCompare(mstruct1[0], mstruct2, parent, b_abs);
				if(i == 0) {
					return evalSortCompare(mstruct1[1], m_one, parent, b_abs);
				}
				return i;
			}
		}
		if(mstruct2.isAborted()) return -1;
		if(mstruct1.isAborted()) return 1;
		if(mstruct2.isInverse()) return -1;
		if(mstruct1.isInverse()) return 1;
		if(mstruct2.isDivision()) return -1;
		if(mstruct1.isDivision()) return 1;
		if(mstruct2.isNegate()) return -1;
		if(mstruct1.isNegate()) return 1;
		if(mstruct2.isLogicalAnd()) return -1;
		if(mstruct1.isLogicalAnd()) return 1;
		if(mstruct2.isLogicalOr()) return -1;
		if(mstruct1.isLogicalOr()) return 1;
		if(mstruct2.isLogicalXor()) return -1;
		if(mstruct1.isLogicalXor()) return 1;
		if(mstruct2.isLogicalNot()) return -1;
		if(mstruct1.isLogicalNot()) return 1;
		if(mstruct2.isComparison()) return -1;
		if(mstruct1.isComparison()) return 1;
		if(mstruct2.isBitwiseOr()) return -1;
		if(mstruct1.isBitwiseOr()) return 1;
		if(mstruct2.isBitwiseXor()) return -1;
		if(mstruct1.isBitwiseXor()) return 1;
		if(mstruct2.isBitwiseAnd()) return -1;
		if(mstruct1.isBitwiseAnd()) return 1;
		if(mstruct2.isBitwiseNot()) return -1;
		if(mstruct1.isBitwiseNot()) return 1;
		if(mstruct2.isUndefined()) return -1;
		if(mstruct1.isUndefined()) return 1;
		if(mstruct2.isFunction()) return -1;
		if(mstruct1.isFunction()) return 1;
		if(mstruct2.isAddition()) return -1;
		if(mstruct1.isAddition()) return 1;
		if(mstruct2.isMultiplication()) return -1;
		if(mstruct1.isMultiplication()) return 1;
		if(mstruct2.isPower()) return -1;
		if(mstruct1.isPower()) return 1;
		if(mstruct2.isUnit()) return -1;
		if(mstruct1.isUnit()) return 1;
		if(mstruct2.isSymbolic()) return -1;
		if(mstruct1.isSymbolic()) return 1;
		if(mstruct2.isVariable()) return -1;
		if(mstruct1.isVariable()) return 1;
		if(mstruct2.isDateTime()) return -1;
		if(mstruct1.isDateTime()) return 1;
		if(parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		return -1;
	}
	switch(mstruct1.type()) {
		case STRUCT_NUMBER: {
			if(CALCULATOR->aborted()) return 0;
			if(b_abs) {
				ComparisonResult cmp = mstruct1.number().compareAbsolute(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			}
			if(!mstruct1.number().hasImaginaryPart() && !mstruct2.number().hasImaginaryPart()) {
				if(mstruct1.number().isFloatingPoint()) {
					if(!mstruct2.number().isFloatingPoint()) return 1;
					if(mstruct1.number().isInterval()) {
						if(!mstruct2.number().isInterval()) return 1;
					} else if(mstruct2.number().isInterval()) return -1;
				} else if(mstruct2.number().isFloatingPoint()) return -1;
				ComparisonResult cmp = mstruct1.number().compare(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			} else {
				if(!mstruct1.number().hasRealPart()) {
					if(mstruct2.number().hasRealPart()) {
						return 1;
					} else {
						ComparisonResult cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
						if(cmp == COMPARISON_RESULT_LESS) return -1;
						else if(cmp == COMPARISON_RESULT_GREATER) return 1;
						return 0;
					}
				} else if(mstruct2.number().hasRealPart()) {
					ComparisonResult cmp = mstruct1.number().compareRealParts(mstruct2.number());
					if(cmp == COMPARISON_RESULT_EQUAL) {
						cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
					}
					if(cmp == COMPARISON_RESULT_LESS) return -1;
					else if(cmp == COMPARISON_RESULT_GREATER) return 1;
					return 0;
				} else {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_UNIT: {
			if(mstruct1.unit() < mstruct2.unit()) return -1;
			if(mstruct1.unit() == mstruct2.unit()) return 0;
			return 1;
		}
		case STRUCT_SYMBOLIC: {
			if(mstruct1.symbol() < mstruct2.symbol()) return -1;
			else if(mstruct1.symbol() == mstruct2.symbol()) return 0;
			return 1;
		}
		case STRUCT_VARIABLE: {
			if(mstruct1.variable() < mstruct2.variable()) return -1;
			else if(mstruct1.variable() == mstruct2.variable()) return 0;
			return 1;
		}
		case STRUCT_FUNCTION: {
			if(mstruct1.function() < mstruct2.function()) return -1;
			if(mstruct1.function() == mstruct2.function()) {
				for(size_t i = 0; i < mstruct2.size(); i++) {
					if(i >= mstruct1.size()) {
						return -1;
					}
					int i2 = evalSortCompare(mstruct1[i], mstruct2[i], parent, b_abs);
					if(i2 != 0) return i2;
				}
				return 0;
			}
			return 1;
		}
		case STRUCT_POWER: {
			int i = evalSortCompare(mstruct1[0], mstruct2[0], parent, b_abs);
			if(i == 0) {
				return evalSortCompare(mstruct1[1], mstruct2[1], parent, b_abs);
			}
			return i;
		}
		default: {
			if(mstruct2.size() < mstruct1.size()) return -1;
			else if(mstruct2.size() > mstruct1.size()) return 1;
			int ie;
			for(size_t i = 0; i < mstruct1.size(); i++) {
				ie = evalSortCompare(mstruct1[i], mstruct2[i], parent, b_abs);
				if(ie != 0) {
					return ie;
				}
			}
		}
	}
	return 0;
}

void MathStructure::evalSort(bool recursive, bool b_abs) {
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).evalSort(true, b_abs);
		}
	}
	//if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_LOGICAL_AND && m_type != STRUCT_LOGICAL_OR && m_type != STRUCT_LOGICAL_XOR && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	if(m_type == STRUCT_ADDITION && containsType(STRUCT_DATETIME, false, true, false) > 0) return;
	vector<size_t> sorted;
	sorted.reserve(SIZE);
	for(size_t i = 0; i < SIZE; i++) {
		if(i == 0) {
			sorted.push_back(v_order[i]);
		} else {
			if(evalSortCompare(CHILD(i), *v_subs[sorted.back()], *this, b_abs) >= 0) {
				sorted.push_back(v_order[i]);
			} else if(sorted.size() == 1) {
				sorted.insert(sorted.begin(), v_order[i]);
			} else {
				for(size_t i2 = sorted.size() - 2; ; i2--) {
					if(evalSortCompare(CHILD(i), *v_subs[sorted[i2]], *this, b_abs) >= 0) {
						sorted.insert(sorted.begin() + i2 + 1, v_order[i]);
						break;
					}
					if(i2 == 0) {
						sorted.insert(sorted.begin(), v_order[i]);
						break;
					}
				}
			}
		}
	}
	for(size_t i2 = 0; i2 < sorted.size(); i2++) {
		v_order[i2] = sorted[i2];
	}
}

int sortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, const PrintOptions &po);
int sortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, const PrintOptions &po) {
	if(parent.isMultiplication()) {
		if(!mstruct1.representsNonMatrix() && !mstruct2.representsNonMatrix()) {
			return 0;
		}
	}
	if(parent.isAddition() && po.sort_options.minus_last) {
		bool m1 = mstruct1.hasNegativeSign(), m2 = mstruct2.hasNegativeSign();
		if(m1 && !m2) {
			return 1;
		} else if(m2 && !m1) {
			return -1;
		}
	}
	if(parent.isAddition() && (mstruct1.isUnit() || (mstruct1.isMultiplication() && mstruct1.size() == 2 && mstruct1[1].isUnit())) && (mstruct2.isUnit() || (mstruct2.isMultiplication() && mstruct2.size() == 2 && mstruct2[1].isUnit()))) {
		Unit *u1, *u2;
		if(mstruct1.isUnit()) u1 = mstruct1.unit();
		else u1 = mstruct1[1].unit();
		if(mstruct2.isUnit()) u2 = mstruct2.unit();
		else u2 = mstruct2[1].unit();
		if(u1->isParentOf(u2)) return 1;
		if(u2->isParentOf(u1)) return -1;
	}
	bool isdiv1 = false, isdiv2 = false;
	if(!po.negative_exponents) {
		if(mstruct1.isMultiplication()) {
			for(size_t i = 0; i < mstruct1.size(); i++) {
				if(mstruct1[i].isPower() && mstruct1[i][1].hasNegativeSign()) {
					isdiv1 = true;
					break;
				}
			}
		} else if(mstruct1.isPower() && mstruct1[1].hasNegativeSign()) {
			isdiv1 = true;
		}
		if(mstruct2.isMultiplication()) {
			for(size_t i = 0; i < mstruct2.size(); i++) {
				if(mstruct2[i].isPower() && mstruct2[i][1].hasNegativeSign()) {
					isdiv2 = true;
					break;
				}
			}
		} else if(mstruct2.isPower() && mstruct2[1].hasNegativeSign()) {
			isdiv2 = true;
		}
	}
	if(parent.isAddition() && isdiv1 == isdiv2) {
		if(mstruct1.isMultiplication() && mstruct1.size() > 0) {
			size_t start = 0;
			while(mstruct1[start].isNumber() && mstruct1.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct2.isMultiplication()) {
				if(mstruct2.size() < 1) return -1;
				size_t start2 = 0;
				while(mstruct2[start2].isNumber() && mstruct2.size() > start2 + 1) {
					start2++;
				}
				for(size_t i = 0; i + start < mstruct1.size(); i++) {
					if(i + start2 >= mstruct2.size()) return 1;
					i2 = sortCompare(mstruct1[i + start], mstruct2[i + start2], parent, po);
					if(i2 != 0) return i2;
				}
				if(mstruct1.size() - start == mstruct2.size() - start2) return 0;
				if(parent.isMultiplication()) return -1;
				else return 1;
			} else {
				i2 = sortCompare(mstruct1[start], mstruct2, parent, po);
				if(i2 != 0) return i2;
			}
		} else if(mstruct2.isMultiplication() && mstruct2.size() > 0) {
			size_t start = 0;
			while(mstruct2[start].isNumber() && mstruct2.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct1.isMultiplication()) {
				return 1;
			} else {
				i2 = sortCompare(mstruct1, mstruct2[start], parent, po);
				if(i2 != 0) return i2;
			}
		}
	}
	if(mstruct1.isVariable() && mstruct1.variable() == CALCULATOR->v_C) return 1;
	if(mstruct2.isVariable() && mstruct2.variable() == CALCULATOR->v_C) return -1;
	if(mstruct1.type() != mstruct2.type()) {
		if(mstruct1.isVariable() && mstruct2.isSymbolic()) {
			if(parent.isMultiplication()) {
				if(mstruct1.variable()->isKnown()) return -1;
			}
			if(mstruct1.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name < mstruct2.symbol()) return -1;
			else return 1;
		}
		if(mstruct2.isVariable() && mstruct1.isSymbolic()) {
			if(parent.isMultiplication()) {
				if(mstruct2.variable()->isKnown()) return 1;
			}
			if(mstruct1.symbol() < mstruct2.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			else return 1;
		}
		if(!parent.isMultiplication() || (!mstruct1.isNumber() && !mstruct2.isNumber())) {
			if(mstruct2.isPower()) {
				int i = sortCompare(mstruct1, mstruct2[0], parent, po);
				if(i == 0) {
					return sortCompare(m_one, mstruct2[1], parent, po);
				}
				return i;
			}
			if(mstruct1.isPower()) {
				int i = sortCompare(mstruct1[0], mstruct2, parent, po);
				if(i == 0) {
					return sortCompare(mstruct1[1], m_one, parent, po);
				}
				return i;
			}
		}
		if(parent.isMultiplication()) {
			if(mstruct2.isUnit()) return -1;
			if(mstruct1.isUnit()) return 1;
			if(mstruct1.isAddition() && !mstruct2.isAddition() && !mstruct1.containsUnknowns() && (mstruct2.isUnknown_exp() || (mstruct2.isMultiplication() && mstruct2.containsUnknowns()))) return -1;
			if(mstruct2.isAddition() && !mstruct1.isAddition() && !mstruct2.containsUnknowns() && (mstruct1.isUnknown_exp() || (mstruct1.isMultiplication() && mstruct1.containsUnknowns()))) return 1;
		}
		if(mstruct2.isAborted()) return -1;
		if(mstruct1.isAborted()) return 1;
		if(mstruct2.isInverse()) return -1;
		if(mstruct1.isInverse()) return 1;
		if(mstruct2.isDivision()) return -1;
		if(mstruct1.isDivision()) return 1;
		if(mstruct2.isNegate()) return -1;
		if(mstruct1.isNegate()) return 1;
		if(mstruct2.isLogicalAnd()) return -1;
		if(mstruct1.isLogicalAnd()) return 1;
		if(mstruct2.isLogicalOr()) return -1;
		if(mstruct1.isLogicalOr()) return 1;
		if(mstruct2.isLogicalXor()) return -1;
		if(mstruct1.isLogicalXor()) return 1;
		if(mstruct2.isLogicalNot()) return -1;
		if(mstruct1.isLogicalNot()) return 1;
		if(mstruct2.isComparison()) return -1;
		if(mstruct1.isComparison()) return 1;
		if(mstruct2.isBitwiseOr()) return -1;
		if(mstruct1.isBitwiseOr()) return 1;
		if(mstruct2.isBitwiseXor()) return -1;
		if(mstruct1.isBitwiseXor()) return 1;
		if(mstruct2.isBitwiseAnd()) return -1;
		if(mstruct1.isBitwiseAnd()) return 1;
		if(mstruct2.isBitwiseNot()) return -1;
		if(mstruct1.isBitwiseNot()) return 1;
		if(mstruct2.isUndefined()) return -1;
		if(mstruct1.isUndefined()) return 1;
		if(mstruct2.isFunction()) return -1;
		if(mstruct1.isFunction()) return 1;
		if(mstruct2.isAddition()) return -1;
		if(mstruct1.isAddition()) return 1;
		if(!parent.isMultiplication()) {
			if(isdiv2 && mstruct2.isMultiplication()) return -1;
			if(isdiv1 && mstruct1.isMultiplication()) return 1;
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		if(mstruct2.isMultiplication()) return -1;
		if(mstruct1.isMultiplication()) return 1;
		if(mstruct2.isPower()) return -1;
		if(mstruct1.isPower()) return 1;
		if(mstruct2.isUnit()) return -1;
		if(mstruct1.isUnit()) return 1;
		if(mstruct2.isSymbolic()) return -1;
		if(mstruct1.isSymbolic()) return 1;
		if(mstruct2.isVariable()) return -1;
		if(mstruct1.isVariable()) return 1;
		if(mstruct2.isDateTime()) return -1;
		if(mstruct1.isDateTime()) return 1;
		if(parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		return -1;
	}
	switch(mstruct1.type()) {
		case STRUCT_NUMBER: {
			if(!mstruct1.number().hasImaginaryPart() && !mstruct2.number().hasImaginaryPart()) {
				ComparisonResult cmp;
				if(parent.isMultiplication() && mstruct2.number().isNegative() != mstruct1.number().isNegative()) cmp = mstruct2.number().compare(mstruct1.number());
				else cmp = mstruct1.number().compare(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			} else {
				if(!mstruct1.number().hasRealPart()) {
					if(mstruct2.number().hasRealPart()) {
						return 1;
					} else {
						ComparisonResult cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
						if(cmp == COMPARISON_RESULT_LESS) return -1;
						else if(cmp == COMPARISON_RESULT_GREATER) return 1;
						return 0;
					}
				} else if(mstruct2.number().hasRealPart()) {
					ComparisonResult cmp = mstruct1.number().compareRealParts(mstruct2.number());
					if(cmp == COMPARISON_RESULT_EQUAL) {
						cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
					}
					if(cmp == COMPARISON_RESULT_LESS) return -1;
					else if(cmp == COMPARISON_RESULT_GREATER) return 1;
					return 0;
				} else {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_UNIT: {
			if(mstruct1.unit() == mstruct2.unit()) return 0;
			if(mstruct1.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct1.isPlural(), po.use_reference_names).name < mstruct2.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct2.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			return 1;
		}
		case STRUCT_SYMBOLIC: {
			if(mstruct1.symbol() < mstruct2.symbol()) return -1;
			else if(mstruct1.symbol() == mstruct2.symbol()) return 0;
			return 1;
		}
		case STRUCT_VARIABLE: {
			if(mstruct1.variable() == mstruct2.variable()) return 0;
			if(parent.isMultiplication()) {
				if(mstruct1.variable()->isKnown() && !mstruct2.variable()->isKnown()) return -1;
				if(!mstruct1.variable()->isKnown() && mstruct2.variable()->isKnown()) return 1;
			}
			if(mstruct1.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names).name < mstruct2.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			return 1;
		}
		case STRUCT_FUNCTION: {
			if(mstruct1.function() == mstruct2.function()) {
				for(size_t i = 0; i < mstruct2.size(); i++) {
					if(i >= mstruct1.size()) {
						return -1;
					}
					int i2 = sortCompare(mstruct1[i], mstruct2[i], parent, po);
					if(i2 != 0) return i2;
				}
				return 0;
			}
			if(mstruct1.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names).name < mstruct2.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			return 1;
		}
		case STRUCT_POWER: {
			if(!po.negative_exponents) {
				bool b1 = mstruct1[1].hasNegativeSign();
				bool b2 = mstruct2[1].hasNegativeSign();
				if(b1 && !b2) return -1;
				if(b2 && !b1) return 1;
			}
			int i = sortCompare(mstruct1[0], mstruct2[0], parent, po);
			if(i == 0) {
				return sortCompare(mstruct1[1], mstruct2[1], parent, po);
			}
			return i;
		}
		case STRUCT_MULTIPLICATION: {
			if(isdiv1 != isdiv2) {
				if(isdiv1) return 1;
				return -1;
			}
		}
		case STRUCT_COMPARISON: {
			if(mstruct1.comparisonType() != mstruct2.comparisonType()) {
				if(mstruct2.comparisonType() == COMPARISON_EQUALS || ((mstruct1.comparisonType() == COMPARISON_LESS || mstruct1.comparisonType() == COMPARISON_EQUALS_LESS) && (mstruct2.comparisonType() == COMPARISON_GREATER || mstruct2.comparisonType() == COMPARISON_EQUALS_GREATER))) {
					return 1;
				}
				if(mstruct1.comparisonType() == COMPARISON_EQUALS || ((mstruct1.comparisonType() == COMPARISON_GREATER || mstruct1.comparisonType() == COMPARISON_EQUALS_GREATER) && (mstruct2.comparisonType() == COMPARISON_LESS || mstruct2.comparisonType() == COMPARISON_EQUALS_LESS))) {
					return -1;
				}
			}
		}
		default: {
			int ie;
			for(size_t i = 0; i < mstruct1.size(); i++) {
				if(i >= mstruct2.size()) return 1;
				ie = sortCompare(mstruct1[i], mstruct2[i], parent, po);
				if(ie != 0) {
					return ie;
				}
			}
		}
	}
	return 0;
}

void MathStructure::sort(const PrintOptions &po, bool recursive) {
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			CHILD(i).sort(po);
		}
	}
	if(m_type == STRUCT_COMPARISON) {
		if((CHILD(0).isZero() && !CHILD(1).isZero()) || (CHILD(0).isNumber() && !CHILD(1).isNumber())) {
			SWAP_CHILDREN(0, 1)
			if(ct_comp == COMPARISON_LESS) ct_comp = COMPARISON_GREATER;
			else if(ct_comp == COMPARISON_EQUALS_LESS) ct_comp = COMPARISON_EQUALS_GREATER;
			else if(ct_comp == COMPARISON_GREATER) ct_comp = COMPARISON_LESS;
			else if(ct_comp == COMPARISON_EQUALS_GREATER) ct_comp = COMPARISON_EQUALS_LESS;
		}
		return;
	}
	if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR && m_type != STRUCT_LOGICAL_AND && m_type != STRUCT_LOGICAL_OR) return;
	if(m_type == STRUCT_ADDITION && containsType(STRUCT_DATETIME, false, true, false) > 0) return;
	vector<size_t> sorted;
	bool b;
	PrintOptions po2 = po;
	po2.sort_options.minus_last = po.sort_options.minus_last && SIZE == 2;
	//!containsUnknowns();
	for(size_t i = 0; i < SIZE; i++) {
		if(CALCULATOR->aborted()) return;
		b = false;
		for(size_t i2 = 0; i2 < sorted.size(); i2++) {
			if(sortCompare(CHILD(i), *v_subs[sorted[i2]], *this, po2) < 0) {
				sorted.insert(sorted.begin() + i2, v_order[i]);
				b = true;
				break;
			}
		}
		if(!b) sorted.push_back(v_order[i]);
	}
	if(CALCULATOR->aborted()) return;
	if(m_type == STRUCT_ADDITION && SIZE > 2 && po.sort_options.minus_last && v_subs[sorted[0]]->hasNegativeSign()) {
		for(size_t i2 = 1; i2 < sorted.size(); i2++) {
			if(CALCULATOR->aborted()) return;
			if(!v_subs[sorted[i2]]->hasNegativeSign()) {
				sorted.insert(sorted.begin(), sorted[i2]);
				sorted.erase(sorted.begin() + (i2 + 1));
				break;
			}
		}
	}
	if(CALCULATOR->aborted()) return;
	for(size_t i2 = 0; i2 < sorted.size(); i2++) {
		v_order[i2] = sorted[i2];
	}
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
		count += CHILD(i).countTotalChildren() + 1;
	}
	return count;
}
bool try_isolate_x(MathStructure &mstruct, EvaluationOptions &eo3, const EvaluationOptions &eo) {
	if(mstruct.isProtected()) return false;
	if(mstruct.isComparison()) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mtest(mstruct);
		eo3.test_comparisons = false;
		eo3.warn_about_denominators_assumed_nonzero = false;
		mtest[0].calculatesub(eo3, eo);
		mtest[1].calculatesub(eo3, eo);
		eo3.test_comparisons = eo.test_comparisons;
		const MathStructure *x_var2;
		if(eo.isolate_var) x_var2 = eo.isolate_var;
		else x_var2 = &mstruct.find_x_var();
		if(x_var2->isUndefined() || (mtest[0] == *x_var2 && !mtest[1].contains(*x_var2))) {
			CALCULATOR->endTemporaryStopMessages();
			 return false;
		}
		if(mtest.isolate_x(eo3, eo, *x_var2, false)) {
			if(test_comparisons(mstruct, mtest, *x_var2, eo3) >= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				mstruct = mtest;
				return true;
			}
		}
		CALCULATOR->endTemporaryStopMessages();
	} else {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(try_isolate_x(mstruct[i], eo3, eo)) b = true;
		}
		return b;
	}
	return false;
}

bool compare_delete(MathStructure &mnum, MathStructure &mden, bool &erase1, bool &erase2, const EvaluationOptions &eo) {
	erase1 = false;
	erase2 = false;
	if(mnum == mden) {
		if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mnum.representsZero(true))
		|| mnum.representsNonZero(true)
		|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mnum.representsZero(true) && warn_about_denominators_assumed_nonzero(mnum, eo))) {
			erase1 = true;
			erase2 = true;
		} else {
			if(mnum.isPower()) {
				mnum.setToChild(1);
				mden.setToChild(1);
				return true;
			}
			return false;
		}
		return true;
	}
	if(!mnum.isPower() && !mden.isPower()) return false;
	MathStructure *mbase1, *mbase2, *mexp1 = NULL, *mexp2 = NULL;
	if(mnum.isPower()) {
		if(!IS_REAL(mnum[1])) return false;
		mexp1 = &mnum[1];
		mbase1 = &mnum[0];
	} else {
		mbase1 = &mnum;
	}
	if(mden.isPower()) {
		if(!IS_REAL(mden[1])) return false;
		mexp2 = &mden[1];
		mbase2 = &mden[0];
	} else {
		mbase2 = &mden;
	}
	if(mbase1->equals(*mbase2)) {
		if(mexp1 && mexp2) {
			if(mexp1->number().isLessThan(mexp2->number())) {
				erase1 = true;
				mexp2->number() -= mexp1->number();
				if(mexp2->isOne()) mden.setToChild(1, true);
			} else {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true))
				|| mbase2->representsNonZero(true)
				|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true) && warn_about_denominators_assumed_nonzero(*mbase2, eo))) {
					erase2 = true;
					mexp1->number() -= mexp2->number();
					if(mexp1->isOne()) mnum.setToChild(1, true);
				} else {
					if(mexp2->number().isFraction()) return false;
					mexp2->number()--;
					mexp1->number() -= mexp2->number();
					if(mexp1->isOne()) mnum.setToChild(1, true);
					if(mexp2->isOne()) mden.setToChild(1, true);
					return true;
				}

			}
			return true;
		} else if(mexp1) {
			if(mexp1->number().isFraction()) {
				erase1 = true;
				mbase2->raise(m_one);
				(*mbase2)[1].number() -= mexp1->number();
				return true;
			}
			if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true))
			|| mbase2->representsNonZero(true)
			|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true) && warn_about_denominators_assumed_nonzero(*mbase2, eo))) {
				mexp1->number()--;
				erase2 = true;
				if(mexp1->isOne()) mnum.setToChild(1, true);
				return true;
			}
			return false;

		} else if(mexp2) {
			if(mexp2->number().isFraction()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true))
				|| mbase2->representsNonZero(true)
				|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true) && warn_about_denominators_assumed_nonzero(*mbase2, eo))) {
					erase2 = true;
					mbase1->raise(m_one);
					(*mbase1)[1].number() -= mexp2->number();
					return true;
				}
				return false;
			}
			mexp2->number()--;
			erase1 = true;
			if(mexp2->isOne()) mden.setToChild(1, true);
			return true;
		}
	}
	return false;
}

bool factor1(const MathStructure &mstruct, MathStructure &mnum, MathStructure &mden, const EvaluationOptions &eo) {
	mnum.setUndefined();
	mden.setUndefined();
	if(mstruct.isAddition()) {
		bool b_num = false, b_den = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(mstruct[i][i2].isPower() && mstruct[i][i2][1].hasNegativeSign()) {
						b_den = true;
						if(b_num) break;
					} else {
						b_num = true;
						if(b_den) break;
					}
				}
			} else if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
				b_den = true;
				if(b_num) break;
			} else {
				b_num = true;
				if(b_den) break;
			}
		}
		if(b_num && b_den) {
			MathStructure *mden_cur = NULL;
			vector<int> multi_index;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mnum.isUndefined()) {
					mnum.transform(STRUCT_ADDITION);
				} else {
					mnum.addChild(m_undefined);
				}
				if(mstruct[i].isMultiplication()) {
					if(!mden_cur) {
						mden_cur = new MathStructure();
						mden_cur->setUndefined();
					}
					for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
						if(mstruct[i][i2].isPower() && mstruct[i][i2][1].hasNegativeSign()) {
							if(mden_cur->isUndefined()) {
								if(mstruct[i][i2][1].isMinusOne()) {
									*mden_cur = mstruct[i][i2][0];
								} else if(mstruct[i][i2][1].isNumber()) {
									*mden_cur = mstruct[i][i2];
									(*mden_cur)[1].number().negate();
								} else {
									*mden_cur = mstruct[i][i2];
									(*mden_cur)[1][0].number().negate();
								}
							} else {
								if(mstruct[i][i2][1].isMinusOne()) {
									mden_cur->multiply(mstruct[i][i2][0], true);
								} else if(mstruct[i][i2][1].isNumber()) {
									mden_cur->multiply(mstruct[i][i2], true);
									(*mden_cur)[mden_cur->size() - 1][1].number().negate();
								} else {
									mden_cur->multiply(mstruct[i][i2], true);
									(*mden_cur)[mden_cur->size() - 1][1][0].number().negate();
								}
							}
						} else {
							if(mnum[mnum.size() - 1].isUndefined()) {
								mnum[mnum.size() - 1] = mstruct[i][i2];
							} else {
								mnum[mnum.size() - 1].multiply(mstruct[i][i2], true);
							}
						}
					}
					if(mnum[mnum.size() - 1].isUndefined()) mnum[mnum.size() - 1].set(1, 1, 0);
					if(mden_cur->isUndefined()) {
						multi_index.push_back(-1);
					} else {
						multi_index.push_back(mden.size());
						if(mden.isUndefined()) {
							mden.transform(STRUCT_MULTIPLICATION);
							mden[mden.size() - 1].set_nocopy(*mden_cur);
							mden_cur->unref();
						} else {
							mden.addChild_nocopy(mden_cur);
						}
						mden_cur = NULL;
					}
				} else if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
					multi_index.push_back(mden.size());
					if(mden.isUndefined()) {
						mden.transform(STRUCT_MULTIPLICATION);
					} else {
						mden.addChild(m_undefined);
					}
					if(mstruct[i][1].isMinusOne()) {
						mden[mden.size() - 1] = mstruct[i][0];
					} else {
						mden[mden.size() - 1] = mstruct[i];
						unnegate_sign(mden[mden.size() - 1][1]);
					}
					mnum[mnum.size() - 1].set(1, 1, 0);
				} else {
					multi_index.push_back(-1);
					mnum[mnum.size() - 1] = mstruct[i];
				}
			}
			for(size_t i = 0; i < mnum.size(); i++) {
				if(multi_index[i] < 0 && mnum[i].isOne()) {
					if(mden.size() == 1) {
						mnum[i] = mden[0];
					} else {
						mnum[i] = mden;
					}
				} else {
					for(size_t i2 = 0; i2 < mden.size(); i2++) {
						if((long int) i2 != multi_index[i]) {
							mnum[i].multiply(mden[i2], true);
						}
					}
				}
				mnum[i].calculatesub(eo, eo, false);
			}
			if(mden.size() == 1) {
				mden.setToChild(1);
			} else {
				mden.calculatesub(eo, eo, false);
			}
			mnum.calculatesub(eo, eo, false);
		}
	} else if(mstruct.isMultiplication()) {
		bool b_num = false, b_den = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
				b_den = true;
				if(b_num) break;
			} else {
				b_num = true;
				if(b_den) break;
			}
		}
		if(b_den && b_num) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
					if(mden.isUndefined()) {
						if(mstruct[i][1].isMinusOne()) {
							mden = mstruct[i][0];
						} else {
							mden = mstruct[i];
							unnegate_sign(mden[1]);
						}
					} else {
						if(mstruct[i][1].isMinusOne()) {
							mden.multiply(mstruct[i][0], true);
						} else {
							mden.multiply(mstruct[i], true);
							unnegate_sign(mden[mden.size() - 1][1]);
						}
					}
				} else {
					if(mnum.isUndefined()) {
						mnum = mstruct[i];
					} else {
						mnum.multiply(mstruct[i], true);
					}
				}
			}
		}
		mden.calculatesub(eo, eo, false);
	}
	if(!mnum.isUndefined() && !mden.isUndefined()) {
		while(true) {
			mnum.factorize(eo, false, false, 0, true);
			mnum.evalSort(true);
			if(mnum.isMultiplication()) {
				for(size_t i = 0; i < mnum.size(); ) {
					if(mnum[i].isPower() && mnum[i][1].hasNegativeSign()) {
						if(mnum[i][1].isMinusOne()) {
							mnum[i][0].ref();
							mden.multiply_nocopy(&mnum[i][0], true);
						} else {
							mnum[i].ref();
							mden.multiply_nocopy(&mnum[i], true);
							unnegate_sign(mden[mden.size() - 1][1]);
						}
						mden.calculateMultiplyLast(eo);
						mnum.delChild(i + 1);
					} else {
						i++;
					}
				}
				if(mnum.size() == 0) mnum.set(1, 1, 0);
				else if(mnum.size() == 1) mnum.setToChild(1);
			}
			mden.factorize(eo, false, false, 0, true);
			mden.evalSort(true);
			bool b = false;
			if(mden.isMultiplication()) {
				for(size_t i = 0; i < mden.size(); ) {
					if(mden[i].isPower() && mden[i][1].hasNegativeSign()) {
						MathStructure *mden_inv = new MathStructure(mden[i]);
						if(mden_inv->calculateInverse(eo)) {
							mnum.multiply_nocopy(mden_inv, true);
							mnum.calculateMultiplyLast(eo);
							mden.delChild(i + 1);
							b = true;
						} else {
							mden_inv->unref();
							i++;
						}
					} else {
						i++;
					}
				}
				if(mden.size() == 0) mden.set(1, 1, 0);
				else if(mden.size() == 1) mden.setToChild(1);
			}
			if(!b) break;
		}
		bool erase1, erase2;
		if(mnum.isMultiplication()) {
			for(long int i = 0; i < (long int) mnum.size(); i++) {
				if(mden.isMultiplication()) {
					for(size_t i2 = 0; i2 < mden.size(); i2++) {
						if(compare_delete(mnum[i], mden[i2], erase1, erase2, eo)) {
							if(erase1) mnum.delChild(i + 1);
							if(erase2) mden.delChild(i2 + 1);
							i--;
							break;
						}
					}
				} else {
					if(compare_delete(mnum[i], mden, erase1, erase2, eo)) {
						if(erase1) mnum.delChild(i + 1);
						if(erase2) mden.set(1, 1, 0);
						break;
					}
				}
			}
		} else if(mden.isMultiplication()) {
			for(size_t i = 0; i < mden.size(); i++) {
				if(compare_delete(mnum, mden[i], erase1, erase2, eo)) {
					if(erase1) mnum.set(1, 1, 0);
					if(erase2) mden.delChild(i + 1);
					break;
				}
			}
		} else {
			if(compare_delete(mnum, mden, erase1, erase2, eo)) {
				if(erase1) mnum.set(1, 1, 0);
				if(erase2) mden.set(1, 1, 0);
			}
		}
		if(mnum.isMultiplication()) {
			if(mnum.size() == 0) mnum.set(1, 1, 0);
			else if(mnum.size() == 1) mnum.setToChild(1);
		}
		if(mden.isMultiplication()) {
			if(mden.size() == 0) mden.set(1, 1, 0);
			else if(mden.size() == 1) mden.setToChild(1);
		}
		return true;
	}
	return false;
}

bool combination_factorize_is_complicated(MathStructure &m) {
	if(m.isPower()) {
		return combination_factorize_is_complicated(m[0]) || combination_factorize_is_complicated(m[1]);
	}
	return m.size() > 0;
}

bool combination_factorize(MathStructure &mstruct) {
	bool retval = false;
	switch(mstruct.type()) {
		case STRUCT_ADDITION: {
			bool b = false;
			// 5/y + x/y + z = (5 + x)/y + z
			MathStructure mstruct_units(mstruct);
			MathStructure mstruct_new(mstruct);
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(mstruct_units[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
						if(!mstruct_units[i][i2].isPower() || !mstruct_units[i][i2][1].hasNegativeSign()) {
							mstruct_units[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
					else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
					for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
						if(mstruct_new[i][i2].isPower() && mstruct_new[i][i2][1].hasNegativeSign()) {
							mstruct_new[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
					else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
				} else if(mstruct_new[i].isPower() && mstruct_new[i][1].hasNegativeSign()) {
					mstruct_new[i].set(1, 1, 0);
				} else {
					mstruct_units[i].setUndefined();
				}
			}
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(!mstruct_units[i].isUndefined()) {
					for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
						if(mstruct_units[i2] == mstruct_units[i]) {
							mstruct_new[i].add(mstruct_new[i2], true);
							mstruct_new.delChild(i2 + 1);
							mstruct_units.delChild(i2 + 1);
							b = true;
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
					else mstruct_new[i].multiply(mstruct_units[i], true);
				}
			}
			if(b) {
				if(mstruct_new.size() == 1) {
					mstruct.set(mstruct_new[0], true);
				} else {
					mstruct = mstruct_new;
				}
				b = false;
				retval = true;
			}
			if(mstruct.isAddition()) {
				// y*f(x) + z*f(x) = (y+z)*f(x)
				MathStructure mstruct_units(mstruct);
				MathStructure mstruct_new(mstruct);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!combination_factorize_is_complicated(mstruct_units[i][i2])) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(combination_factorize_is_complicated(mstruct_new[i][i2])) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(combination_factorize_is_complicated(mstruct_units[i])) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) mstruct.set(mstruct_new[0], true);
					else mstruct = mstruct_new;
					retval = true;
				}
			}
			if(mstruct.isAddition()) {
				// 5x + pi*x + 5y + xy = (5 + pi)x + 5y + xy
				MathStructure mstruct_units(mstruct);
				MathStructure mstruct_new(mstruct);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!mstruct_units[i][i2].containsType(STRUCT_UNIT, true) && !mstruct_units[i][i2].containsUnknowns()) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(mstruct_new[i][i2].containsType(STRUCT_UNIT, true) || mstruct_new[i][i2].containsUnknowns()) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(mstruct_units[i].containsType(STRUCT_UNIT, true) || mstruct_units[i].containsUnknowns()) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) mstruct.set(mstruct_new[0], true);
					else mstruct = mstruct_new;
					retval = true;
				}
			}
			//if(retval) return retval;
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(combination_factorize(mstruct[i])) {
					mstruct.childUpdated(i);
					b = true;
				}
			}
			if(b) retval = true;
		}
	}
	return retval;
}

bool MathStructure::simplify(const EvaluationOptions &eo, bool unfactorize) {

	if(SIZE == 0) return false;
	if(unfactorize) {
		unformat();
		EvaluationOptions eo2 = eo;
		eo2.expand = true;
		eo2.combine_divisions = false;
		eo2.sync_units = false;
		calculatesub(eo2, eo2);
		bool b = do_simplification(*this, eo2, true, false, false);
		return combination_factorize(*this) || b;
	}
	return combination_factorize(*this);

}

bool MathStructure::expand(const EvaluationOptions &eo, bool unfactorize) {
	if(SIZE == 0) return false;
	EvaluationOptions eo2 = eo;
	eo2.sync_units = false;
	eo2.expand = true;
	if(unfactorize) calculatesub(eo2, eo2);
	do_simplification(*this, eo2, true, false, false);
	return false;
}

bool MathStructure::structure(StructuringMode structuring, const EvaluationOptions &eo, bool restore_first) {
	switch(structuring) {
		case STRUCTURING_NONE: {
			if(restore_first) {
				EvaluationOptions eo2 = eo;
				eo2.sync_units = false;
				calculatesub(eo2, eo2);
			}
			return false;
		}
		case STRUCTURING_FACTORIZE: {
			return factorize(eo, restore_first, 3, 0, true, 2, NULL, m_undefined, true, false, -1);
		}
		default: {
			return simplify(eo, restore_first);
		}
	}
}

void clean_multiplications(MathStructure &mstruct);
void clean_multiplications(MathStructure &mstruct) {
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication()) {
				size_t i2 = 0;
				for(; i2 < mstruct[i + i2].size(); i2++) {
					mstruct[i + i2][i2].ref();
					mstruct.insertChild_nocopy(&mstruct[i + i2][i2], i + i2 + 1);
				}
				mstruct.delChild(i + i2 + 1);
			}
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		clean_multiplications(mstruct[i]);
	}
}

bool containsComplexUnits(const MathStructure &mstruct) {
	if(mstruct.type() == STRUCT_UNIT && mstruct.unit()->hasNonlinearRelationTo(mstruct.unit()->baseUnit())) return true;
	if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_stripunits) return false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(containsComplexUnits(mstruct[i])) {
			return true;
		}
	}
	return false;
}

bool variablesContainsComplexUnits(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct.type() == STRUCT_VARIABLE && mstruct.variable()->isKnown() && (eo.approximation != APPROXIMATION_EXACT || !mstruct.variable()->isApproximate()) && ((KnownVariable*) mstruct.variable())->get().containsType(STRUCT_UNIT, false, true, true)) {
		return containsComplexUnits(((KnownVariable*) mstruct.variable())->get());
	}
	if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_stripunits) return false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(variablesContainsComplexUnits(mstruct[i], eo)) {
			return true;
		}
	}
	return false;
}


bool expandVariablesWithUnits(MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct.type() == STRUCT_VARIABLE && mstruct.variable()->isKnown() && (eo.approximation != APPROXIMATION_EXACT || !mstruct.variable()->isApproximate()) && ((KnownVariable*) mstruct.variable())->get().containsType(STRUCT_UNIT, false, true, true) != 0) {
		mstruct.set(((KnownVariable*) mstruct.variable())->get());
		return true;
	}
	bool retval = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(expandVariablesWithUnits(mstruct[i], eo)) {
			mstruct.childUpdated(i + 1);
			retval = true;
		}
	}
	return retval;
}

int limit_inf_cmp(const MathStructure &mstruct, const MathStructure &mcmp, const MathStructure &x_var) {
	if(mstruct.equals(mcmp)) return 0;
	bool b_multi1 = false, b_multi2 = false;
	const MathStructure *m1 = NULL, *m2 = NULL;
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].contains(x_var)) {
				if(!m1) {
					m1 = &mstruct[i];
				} else {
					int cmp = limit_inf_cmp(mstruct[i], *m1, x_var);
					if(cmp > 0) {
						m1 = &mstruct[i];
					} else if(cmp != -1) {
						return -2;
					}
					b_multi1 = true;
				}
			}
		}
	} else if(mstruct.contains(x_var, true)) {
		m1 = &mstruct;
	}
	if(mcmp.isMultiplication()) {
		for(size_t i = 0; i < mcmp.size(); i++) {
			if(mcmp[i].contains(x_var)) {
				if(!m2) {
					m2 = &mcmp[i];
				} else {
					int cmp = limit_inf_cmp(mcmp[i], *m2, x_var);
					if(cmp > 0) {
						m2 = &mcmp[i];
					} else if(cmp != -1) {
						return -2;
					}
					b_multi2 = true;
				}
			}
		}
	} else if(mcmp.contains(x_var, true)) {
		m2 = &mcmp;
	}
	if(!m1 && !m2) return 0;
	if(!m1) return -1;
	if(!m2) return 1;
	int itype1 = 0;
	int itype2 = 0;
	if(m1->isFunction() && m1->function() == CALCULATOR->f_gamma) itype1 = 4;
	else if(m1->isPower() && m1->exponent()->contains(x_var, true)) itype1 = 3;
	else if(m1->equals(x_var) || (m1->isPower() && m1->base()->equals(x_var) && m1->exponent()->representsPositive())) itype1 = 2;
	else if(m1->isFunction() && m1->function() == CALCULATOR->f_ln) itype1 = 1;
	else return -2;
	if(m2->isFunction() && m2->function() == CALCULATOR->f_gamma) itype2 = 4;
	else if(m2->isPower() && m2->exponent()->contains(x_var, true)) itype2 = 3;
	else if(m2->equals(x_var) || (m2->isPower() && m2->base()->equals(x_var) && m2->exponent()->representsPositive())) itype2 = 2;
	else if(m2->isFunction() && m2->function() == CALCULATOR->f_ln) itype2 = 1;
	else return -2;
	if(itype1 > itype2) return 1;
	if(itype2 > itype1) return -1;
	ComparisonResult cr = COMPARISON_RESULT_UNKNOWN;
	CALCULATOR->beginTemporaryEnableIntervalArithmetic();
	if(CALCULATOR->usesIntervalArithmetic()) {
		if(itype1 == 4) {
			cr = m1->getChild(1)->compare(*m2->getChild(1));
		} else if(itype1 == 1) {
			int cmp = limit_inf_cmp(*m1->getChild(1), *m2->getChild(1), x_var);
			if(cmp > 0) cr = COMPARISON_RESULT_LESS;
			else if(cmp == -1) cr = COMPARISON_RESULT_GREATER;
		} else if(itype1 == 3) {
			if(m1->exponent()->equals(*m2->exponent())) {
				if(m1->base()->contains(x_var, true) || m2->base()->contains(x_var, true)) {
					int cmp = limit_inf_cmp(*m1->base(), *m2->base(), x_var);
					if(cmp > 0) cr = COMPARISON_RESULT_LESS;
					else if(cmp == -1) cr = COMPARISON_RESULT_GREATER;
				} else {
					cr = m1->base()->compareApproximately(*m2->base());
				}
			} else if(m1->base()->equals(*m2->base())) {
				int cmp = limit_inf_cmp(*m1->exponent(), *m2->exponent(), x_var);
				if(cmp > 0) cr = COMPARISON_RESULT_LESS;
				else if(cmp == -1) cr = COMPARISON_RESULT_GREATER;
				else if(cmp == 0) cr = m1->exponent()->compareApproximately(*m2->exponent());
			}
		} else if(itype1 == 2) {
			if(m1->equals(x_var)) {
				if(m2->equals(x_var)) cr = COMPARISON_RESULT_EQUAL;
				else cr = m_one.compareApproximately(*m2->getChild(2));
			} else if(m2->equals(x_var)) {
				cr = m1->getChild(2)->compareApproximately(m_one);
			} else {
				cr = m1->getChild(2)->compareApproximately(*m2->getChild(2));
			}
		}
	}
	CALCULATOR->endTemporaryEnableIntervalArithmetic();
	if(cr == COMPARISON_RESULT_GREATER) return -1;
	else if(cr == COMPARISON_RESULT_LESS) return 1;
	else if(cr != COMPARISON_RESULT_EQUAL) return -2;
	if(!b_multi1 && !b_multi2) return 0;
	if(!b_multi1) return -1;
	if(!b_multi2) return 1;
	MathStructure mc1(mstruct), mc2(mcmp);
	for(size_t i = 0; i < mc1.size(); i++) {
		if(&mstruct[i] == m1) {mc1.delChild(i + 1, true); break;}
	}
	for(size_t i = 0; i < mc2.size(); i++) {
		if(&mcmp[i] == m2) {mc2.delChild(i + 1, true); break;}
	}
	return limit_inf_cmp(mc1, mc2, x_var);
}

bool is_limit_neg_power(const MathStructure &mstruct, const MathStructure &x_var, bool b_nil) {
	return mstruct.isPower() && (((!b_nil || !mstruct[1].contains(x_var, true)) && mstruct[1].representsNegative()) || (b_nil && mstruct[1].isMultiplication() && mstruct[1].size() == 2 && mstruct[1][1] == x_var && mstruct[1][0].representsNegative()));
}

bool limit_combine_divisions(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit) {
	if(mstruct.isAddition()) {
		bool b = false;
		bool b_nil = nr_limit.isInfinite(false) && nr_limit.number().isMinusInfinity();
		// 5/y + x/y + z = (5 + x)/y + z
		for(size_t i = 0; i < mstruct.size() && !b; i++) {
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size() && !b; i2++) {
					if(is_limit_neg_power(mstruct[i][i2], x_var, b_nil)) {
						b = true;
					}
				}
			} else if(is_limit_neg_power(mstruct[i], x_var, b_nil)) {
				b = true;
			}
		}
		if(!b) return false;
		b = false;
		MathStructure mstruct_units(mstruct);
		MathStructure mstruct_new(mstruct);
		for(size_t i = 0; i < mstruct_units.size(); i++) {
			if(mstruct_units[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
					if(!is_limit_neg_power(mstruct_units[i][i2], x_var, b_nil)) {
						mstruct_units[i].delChild(i2 + 1);
					} else {
						i2++;
					}
				}
				if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
				else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
				for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
					if(is_limit_neg_power(mstruct_new[i][i2], x_var, b_nil)) {
						mstruct_new[i].delChild(i2 + 1);
					} else {
						i2++;
					}
				}
				if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
				else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
			} else if(is_limit_neg_power(mstruct_new[i], x_var, b_nil)) {
				mstruct_new[i].set(1, 1, 0);
			} else {
				mstruct_units[i].setUndefined();
			}
		}
		for(size_t i = 0; i < mstruct_units.size(); i++) {
			if(!mstruct_units[i].isUndefined()) {
				for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
					if(mstruct_units[i2] == mstruct_units[i]) {
						mstruct_new[i].add(mstruct_new[i2], true);
						mstruct_new.delChild(i2 + 1);
						mstruct_units.delChild(i2 + 1);
						b = true;
					} else {
						i2++;
					}
				}
				if(mstruct_new[i].isOne()) {
					mstruct_new[i].set(mstruct_units[i]);
				} else if(mstruct_units[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct_units[i].size(); i2++) {
						mstruct_new[i].multiply(mstruct_units[i][i2], true);
					}
				} else {
					mstruct_new[i].multiply(mstruct_units[i], true);
				}
			}
		}
		if(b) {
			if(mstruct_new.size() == 1) {
				mstruct.set(mstruct_new[0], true);
			} else {
				mstruct = mstruct_new;
			}
			return true;
		}
	}
	return false;
}
bool limit_combine_divisions2(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo) {
	if(mstruct.isAddition()) {
		MathStructure mden(1, 1, 0);
		bool b = false;
		bool b_nil = nr_limit.isInfinite(false) && nr_limit.number().isMinusInfinity();
		size_t i_d = 0;
		for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
			if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
					if(is_limit_neg_power(mstruct[i2][i3], x_var, b_nil)) {
						mden *= mstruct[i2][i3];
						mden.last()[1].negate();
						i_d++;
					} else if(!mstruct[i2][i3].isOne() && !mstruct[i2][i3].isMinusOne()) {
						b = true;
					}
				}
			} else if(is_limit_neg_power(mstruct[i2], x_var, b_nil)) {
				mden *= mstruct[i2];
				mden.last()[1].negate();
				i_d++;
			} else {
				b = true;
			}
		}
		if(mden.isOne() || !b || i_d > 10) return false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
				if(i2 != i) {
					if(mstruct[i2].isMultiplication()) {
						for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
							if(is_limit_neg_power(mstruct[i2][i3], x_var, b_nil)) {
								mstruct[i].multiply(mstruct[i2][i3], true);
								mstruct[i].last()[1].negate();
							}
						}
					} else if(is_limit_neg_power(mstruct[i2], x_var, b_nil)) {
						mstruct[i].multiply(mstruct[i2], true);
						mstruct[i].last()[1].negate();
					}
				}
			}
		}
		for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
			if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size();) {
					if(is_limit_neg_power(mstruct[i2][i3], x_var, b_nil)) {
						mstruct[i2].delChild(i3 + 1);
					} else {
						i3++;
					}
				}
				if(mstruct[i2].size() == 0) mstruct[i2].set(1, 1, 0);
				else if(mstruct[i2].size() == 1) mstruct[i2].setToChild(1);
			} else if(is_limit_neg_power(mstruct[i2], x_var, b_nil)) {
				mstruct[i2].set(1, 1, 0);
			}
		}
		mden.calculatesub(eo, eo, true);
		mstruct.calculatesub(eo, eo, true);
		mstruct /= mden;
		return true;
	}
	return false;
}

bool contains_zero(const MathStructure &mstruct) {
	if(mstruct.isNumber() && !mstruct.number().isNonZero()) return true;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_zero(mstruct[i])) return true;
	}
	return false;
}
bool limit_contains_undefined(const MathStructure &mstruct) {
	bool b_zero = false, b_infinity = false;
	if(mstruct.isPower() && mstruct[0].isNumber() && ((!mstruct[0].number().isNonZero() && mstruct[1].representsNegative()) || mstruct[1].containsInfinity(true))) return true;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(limit_contains_undefined(mstruct[i])) return true;
		if(contains_zero(mstruct[i])) {
			if(b_infinity) return true;
			b_zero = true;
		}
		if(mstruct[i].containsInfinity(true)) {
			if(b_infinity || b_zero) return true;
			b_infinity = true;
		}
	}
	return false;
}
bool is_plus_minus_infinity(const MathStructure &mstruct) {
	return mstruct.isInfinite(false) || (mstruct.isPower() && mstruct[0].isZero() && mstruct[1].representsNegative()) || (mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].representsReal() && mstruct[1].isPower() && mstruct[1][0].isZero() && mstruct[1][1].representsNegative());
}

bool calculate_limit_sub(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, Number *polydeg = NULL, int lhop_depth = 0, bool keep_inf_x = false, bool reduce_addition = true) {

	if(CALCULATOR->aborted()) return false;
	if(mstruct == x_var) {
		if(keep_inf_x && nr_limit.isInfinite(false)) return true;
		mstruct.set(nr_limit, true);
		return true;
	}
	if(!mstruct.contains(x_var, true)) return true;

	switch(mstruct.type()) {
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isPower() && mstruct[i][0].isPower() && mstruct[i][0][0].contains(x_var, true) && !mstruct[i][0][0].representsNonPositive()) {
					MathStructure mtest(mstruct[i][0][0]);
					calculate_limit_sub(mtest, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					if(mtest.representsPositive()) {
						mstruct[i][0][1].calculateMultiply(mstruct[i][1], eo);
						mstruct[i].setToChild(1);
					}
				}
			}
			MathStructure mzero(1, 1, 0);
			MathStructure minfp(1, 1, 0);
			MathStructure mleft;
			vector<size_t> irecalc;
			MathStructure mbak(mstruct);
			bool b_inf = false;
			bool b_li = nr_limit.isInfinite(false);
			bool b_fail = false;
			bool b_test_inf = b_li;
			bool b_possible_zero = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				bool b = false;
				if(!b_fail && mstruct[i].isPower() && mstruct[i][0].contains(x_var, true) && mstruct[i][1].representsNegative()) {
					if(mstruct[i][1].isMinusOne()) {
						mstruct[i].setToChild(1);
					} else {
						mstruct[i][1].calculateNegate(eo);
					}
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					if(mstruct[i].isZero()) {
						b = true;
						if(b_test_inf && !mbak[i][1].contains(x_var, true)) b_test_inf = false;
						if(minfp.isOne()) {
							minfp = mbak[i];
							if(!b_li) {
								if(minfp[1].isMinusOne()) minfp.setToChild(1);
								else minfp[1].calculateNegate(eo);
							}
						} else {
							minfp.multiply(mbak[i], true);
							if(!b_li) {
								if(minfp.last()[1].isMinusOne()) minfp.last().setToChild(1);
								else minfp.last()[1].calculateNegate(eo);
							}
						}
						irecalc.push_back(i);
						mstruct[i].inverse();
					} else if(mstruct[i].isInfinite()) {
						b = true;
						if(mzero.isOne()) {
							mzero = mbak[i];
							if(b_li) {
								if(mzero[1].isMinusOne()) mzero.setToChild(1);
								else mzero[1].calculateNegate(eo);
							}
						} else {
							mzero.multiply(mbak[i], true);
							if(b_li) {
								if(mzero.last()[1].isMinusOne()) mzero.last().setToChild(1);
								else mzero.last()[1].calculateNegate(eo);
							}
						}
						mstruct[i].clear(true);
					} else {
						mstruct[i].calculateInverse(eo);
					}
				} else {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
				}
				if(!b_fail && !b) {
					if(mstruct[i].isZero()) {
						if(b_li && mbak[i].isPower()) {
							if(b_test_inf && !mbak[i][1].contains(x_var, true)) b_test_inf = false;
							if(mzero.isOne()) {
								mzero = mbak[i];
								if(mzero[1].isMinusOne()) mzero.setToChild(1);
								else mzero[1].calculateNegate(eo);
							} else {
								mzero.multiply(mbak[i], true);
								if(mzero.last()[1].isMinusOne()) mzero.last().setToChild(1);
								else mzero.last()[1].calculateNegate(eo);
							}
						} else {
							b_test_inf = false;
							if(mzero.isOne()) {
								mzero = mbak[i];
								if(b_li) {
									if(!mzero.isPower()) mzero.inverse();
									else if(mzero[1].isMinusOne()) mzero.setToChild(1);
									else mzero[1].calculateNegate(eo);
								}
							} else {
								mzero.multiply(mbak[i], true);
								if(b_li) {
									if(!mzero.last().isPower()) mzero.last().inverse();
									else if(mzero.last()[1].isMinusOne()) mzero.last().setToChild(1);
									else mzero.last()[1].calculateNegate(eo);
								}
							}
						}
					} else if(is_plus_minus_infinity(mstruct[i])) {
						if(mstruct[i].isInfinite()) {
							b_inf = true;
							if(minfp.isOne()) {
								minfp = mbak[i];
								if(!b_li) {
									if(!minfp.isPower()) minfp.inverse();
									else if(minfp[1].isMinusOne()) minfp.setToChild(1);
									else minfp[1].calculateNegate(eo);
								}
							} else {
								minfp.multiply(mbak[i], true);
								if(!b_li) {
									if(!minfp.last().isPower()) minfp.last().inverse();
									else if(minfp.last()[1].isMinusOne()) minfp.last().setToChild(1);
									else minfp.last()[1].calculateNegate(eo);
								}
							}
						} else {
							b_test_inf = false;
							if(minfp.isOne()) {
								minfp = mbak[i];
								if(!b_li) {
									if(!minfp.isPower()) minfp.inverse();
									else if(minfp[1].isMinusOne()) minfp[1].setToChild(1);
									else minfp[1].calculateNegate(eo);
								}
							} else {
								minfp.multiply(mbak[i], true);
								if(!b_li) {
									if(!minfp.last().isPower()) minfp.last().inverse();
									else if(minfp.last()[1].isMinusOne()) minfp.last().setToChild(1);
									else minfp.last()[1].calculateNegate(eo);
								}
							}
						}
					} else if(!mstruct[i].representsNonZero(true) || !mstruct[i].representsNumber(true)) {
						if(!mstruct[i].contains(x_var) && !mstruct.isZero() && mstruct[i].representsNumber(true)) {
							mstruct[i].ref();
							mleft.addChild_nocopy(&mstruct[i]);
							b_possible_zero = true;
						} else {
							b_fail = true;
						}
					} else {
						mstruct[i].ref();
						mleft.addChild_nocopy(&mstruct[i]);
					}
				}
			}
			if(b_li) b_inf = !b_inf;
			if(!b_fail && b_li && b_test_inf && !mzero.isOne() && !minfp.isOne()) {
				MathStructure mnum(minfp);
				MathStructure mden(mzero);
				mnum.calculatesub(eo, eo, false);
				mden.calculatesub(eo, eo, false);
				MathStructure mnum_bak(mnum), mden_bak(mden);
				calculate_limit_sub(mnum, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true);
				calculate_limit_sub(mden, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true);
				if(mnum.contains(x_var, true) && mden.contains(x_var, true)) {
					int cmp = limit_inf_cmp(mnum, mden, x_var);
					if(cmp == 0) {
						mstruct.set(mnum);
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						mstruct.calculateDivide(mden, eo2);
						if(!mstruct.contains(x_var)) {
							for(size_t i = 0; i < mleft.size(); i++) {
								mstruct.calculateMultiply(mleft[i], eo);
							}
							break;
						}
					}
					MathStructure mnum_b(mnum), mden_b(mden);
					calculate_limit_sub(mnum, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					calculate_limit_sub(mden, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					if(mnum.isInfinite(false) && mden.isInfinite(false)) {
						if(cmp > 0) {
							if(keep_inf_x) {
								mstruct.set(mnum_b, true);
								mstruct.calculateDivide(mden_b, eo);
								break;
							}
							mstruct.set(mnum, true);
							if(mden.number().isNegative()) mstruct.calculateNegate(eo);
							for(size_t i = 0; i < mleft.size(); i++) {
								mstruct.calculateMultiply(mleft[i], eo);
							}
							break;
						} else if(cmp == -1) {
							mstruct.clear(true);
							break;
						}
						if(mnum_b.isPower() && mden_b.isPower() && mnum_b[1].contains(x_var, true) && mden_b[1].contains(x_var, true)) {
							bool b = true;
							if(mden_b[1] != mnum_b[1]) {
								b = false;
								Number npow1(1, 1, 0), npow2;
								MathStructure *x_p = &mnum_b[1];
								if(mnum_b[1].isMultiplication() && mnum_b[1].size() == 2 && mnum_b[1][0].isNumber()) {
									npow1 = mnum_b[1][0].number();
									x_p = &mnum_b[1][1];
								}
								if(mden_b[1] == *x_p) npow2.set(1, 1, 0);
								else if(mden_b[1].isMultiplication() && mden_b[1].size() == 2 && mden_b[1][0].isNumber() && mden_b[1][1] == *x_p) npow2 = mden_b[1][0].number();
								if(!npow2.isZero() && npow1.isRational() && npow2.isRational()) {
									if(npow1.isGreaterThan(npow2)) {
										npow1 /= npow2;
										npow2 = npow1.denominator();
										npow1 = npow1.numerator();
									}
									if(mnum.number().isMinusInfinity() && !npow1.isOne()) {
										if(npow2.isOne() && mden.number().isPlusInfinity()) {
											mden_b[0].factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, true);
											if(mden_b[0].isPower() && mden_b[0][1].isInteger()) {
												mden_b[0][1].number() /= npow1;
												if(mden_b[0][1].number().isInteger()) {
													mden_b[0].calculateRaiseExponent(eo);
													b = true;
												}
											}
										}
									} else if(mden.number().isMinusInfinity() && !npow2.isOne()) {
										if(npow1.isOne() && mnum.number().isPlusInfinity()) {
											mnum_b[0].factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, true);
											if(mnum_b[0].isPower() && mnum_b[0][1].isInteger()) {
												mnum_b[0][1].number() /= npow2;
												if(mnum_b[0][1].number().isInteger()) {
													mnum_b[0].calculateRaiseExponent(eo);
													mnum_b[1] = mden_b[1];
													b = true;
												}
											}
										}
									} else {
										if(!npow1.isOne()) mnum_b[0].calculateRaise(npow1, eo);
										if(!npow2.isOne()) mden_b[0].calculateRaise(npow2, eo);
										mnum_b.childUpdated(1);
										mden_b.childUpdated(1);
										if(!npow1.isOne()) {
											if(&mnum_b[1] == x_p) {
												npow1.recip();
												mnum_b[1] *= npow1;
												mnum_b[1].swapChildren(1, 2);
											} else {
												mnum_b[1][0].number() /= npow1;
											}
										}
										b = true;
									}
								}
							}
							if(b) {
								mstruct.set(mnum_b[0], true);
								mstruct /= mden_b[0];
								mstruct.transform(CALCULATOR->f_ln);
								mstruct *= mnum_b[1];
								mstruct.raise(CALCULATOR->v_e);
								mstruct.swapChildren(1, 2);
								calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, keep_inf_x);
								for(size_t i = 0; i < mleft.size(); i++) {
									mstruct.calculateMultiply(mleft[i], eo);
								}
								break;
							}
						}
					}
				}
			}
			if(!b_possible_zero && !b_fail && lhop_depth < 5 && !mzero.isOne() && !minfp.isOne() && mzero.countTotalChildren(false) + minfp.countTotalChildren(false) < 50) {
				//L'Hôpital's rule
				MathStructure mden, mnum;
				for(size_t i2 = 0; i2 < 2; i2++) {
					if((i2 == 0) != b_inf) {
						mnum = mzero;
						mden = minfp;
						if(b_li) {
							mden.inverse();
							mnum.inverse();
						}
					} else {
						mnum = minfp;
						mden = mzero;
						if(!b_li) {
							mden.inverse();
							mnum.inverse();
						}
					}
					if(mnum.differentiate(x_var, eo) && !contains_diff_for(mnum, x_var) && mden.differentiate(x_var, eo) && !contains_diff_for(mden, x_var)) {
						mnum /= mden;
						mnum.eval(eo);
						calculate_limit_sub(mnum, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1);
						if(!limit_contains_undefined(mnum)) {
							mstruct.set(mnum, true);
							for(size_t i = 0; i < mleft.size(); i++) {
								mstruct.calculateMultiply(mleft[i], eo);
							}
							return true;
						}
					}
				}
			}
			for(size_t i = 0; i < irecalc.size(); i++) {
				mstruct[irecalc[i]] = mbak[irecalc[i]];
				calculate_limit_sub(mstruct[irecalc[i]], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
			if(keep_inf_x && mstruct.isInfinite(false)) {
				mstruct = mbak;
				if(reduce_addition) {
					for(size_t i = 0; i < mstruct.size(); i++) {
						calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, reduce_addition);
						if(mstruct[i].isZero()) mstruct[i] = mbak[i];
					}
					mstruct.childrenUpdated();
					mstruct.calculatesub(eo, eo, false);
				}
			}
			break;
		}
		case STRUCT_ADDITION: {
			MathStructure mbak(mstruct);
			bool b = limit_combine_divisions(mstruct, x_var, nr_limit);
			if(mstruct.isAddition()) b = limit_combine_divisions2(mstruct, x_var, nr_limit, eo);
			if(b) {
				if(lhop_depth > 0) return calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, polydeg, lhop_depth);
				if(calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, polydeg, lhop_depth) && !limit_contains_undefined(mstruct)) return true;
				mstruct = mbak;
			}
			if(nr_limit.isInfinite(false)) {
				size_t i_cmp = 0;
				b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true);
				}
				if(mstruct.contains(x_var, true) && (!keep_inf_x || reduce_addition)) {
					bool bfac = false;
					MathStructure mstruct_units(mstruct);
					MathStructure mstruct_new(mstruct);
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(mstruct_units[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
								if(!mstruct_units[i][i2].contains(x_var, true)) {
									mstruct_units[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
							else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
							for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
								if(mstruct_new[i][i2].contains(x_var, true)) {
									mstruct_new[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
							else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
						} else if(mstruct_units[i].contains(x_var, true)) {
							mstruct_new[i].set(1, 1, 0);
						} else {
							mstruct_units[i].setUndefined();
						}
					}
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(!mstruct_units[i].isUndefined()) {
							for(size_t i2 = i + 1; i2 < mstruct_units.size(); i2++) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_new[i].add(mstruct_new[i2], true);
									bfac = true;
								}
							}
							bool bfac2 = false;
							MathStructure mzero(mstruct_new[i]);
							mzero.calculatesub(eo, eo, false);
							if(bfac && mzero.isZero() && lhop_depth < 5) {
								MathStructure mfac(mbak[i]);
								bool b_diff = false;
								calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, false);
								if(mfac != mstruct[i]) b_diff = true;
								for(size_t i2 = i + 1; i2 < mstruct_units.size(); i2++) {
									if(mstruct_units[i2] == mstruct_units[i]) {
										mfac.add(mbak[i2], true);
										calculate_limit_sub(mfac.last(), x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, false);
										if(!b_diff && mfac.last() != mstruct[i2]) b_diff = true;
										bfac2 = true;
									}
								}
								if(bfac2) {
									mfac.calculatesub(eo, eo, false);
									if(!mfac.isAddition()) {
										calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
										mstruct_new[i].set(mfac, true);
									} else if(!b_diff) {
										bfac2 = false;
									} else if(mfac.size() == 2 && !mfac.containsType(STRUCT_FUNCTION)) {
										MathStructure mfac2(mfac);
										MathStructure mmul(mfac);
										mmul.last().calculateNegate(eo);
										mfac.calculateMultiply(mmul, eo);
										mfac.divide(mmul);
										calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
										if(limit_contains_undefined(mfac)) {
											mfac2.factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, false);
											if(!mfac2.isAddition()) {
												calculate_limit_sub(mfac2, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
												mstruct_new[i].set(mfac2, true);
											} else {
												bfac2 = false;
											}
										} else {
											mstruct_new[i].set(mfac, true);
										}
									} else {
										mfac.factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, false);
										if(!mfac.isAddition()) {
											calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
											mstruct_new[i].set(mfac, true);
										} else if(mstruct_units[i].isFunction() && mstruct_units[i].function() == CALCULATOR->f_ln) {
											mstruct_new[i].clear(true);
										} else {
											bfac2 = false;
										}
									}
								}
							}
							if(!bfac2) {
								if(mstruct_new[i].isOne()) {
									mstruct_new[i].set(mstruct_units[i]);
								} else if(mstruct_units[i].isMultiplication()) {
									for(size_t i2 = 0; i2 < mstruct_units[i].size(); i2++) {
										mstruct_new[i].multiply(mstruct_units[i][i2], true);
									}
								} else {
									mstruct_new[i].multiply(mstruct_units[i], true);
								}
							}
							for(size_t i2 = i + 1; i2 < mstruct_units.size(); i2++) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_units[i2].setUndefined();
									mstruct_new[i2].clear();
								}
							}
						}
					}
					if(bfac) {
						for(size_t i = 0; mstruct_new.size() > 1 && i < mstruct_new.size();) {
							if(mstruct_new[i].isZero()) {
								mstruct_new.delChild(i + 1);
							} else {
								i++;
							}
						}
						mstruct = mstruct_new;
						if(mstruct.size() == 1) {
							mstruct.setToChild(1, true);
							if(!keep_inf_x) calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
							break;
						} else if(!mstruct.contains(x_var, true)) {
							b = false;
						}
					}
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(i > 0 && mstruct[i].contains(x_var, true)) {
							int cmp = limit_inf_cmp(mstruct[i], mstruct[i_cmp], x_var);
							if(cmp > 0) {
								mstruct.delChild(i_cmp + 1, false);
								i--;
								i_cmp = i;
							} else if(cmp == -1) {
								mstruct.delChild(i + 1, false);
								i--;
							} else {
								b = false;
							}
						}
					}
					if(b) {
						MathStructure mbak(mstruct);
						for(size_t i = 0; i < mstruct.size();) {
							if(!mstruct[i].contains(x_var, true)) {
								if(mstruct[i].representsNumber()) {
									mstruct.delChild(i + 1, false);
								} else {
									mstruct = mbak;
									break;
								}
							} else {
								i++;
							}
						}
					}
				}
				if(!b || !keep_inf_x) {
					for(size_t i = 0; i < mstruct.size(); i++) {
						calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					}
				}
				mstruct.childrenUpdated();
				if(mstruct.size() == 1) mstruct.setToChild(1, true);
				else if(!b || !keep_inf_x) mstruct.calculatesub(eo, eo, false);
				break;
			} else {
				for(size_t i = 0; i < mstruct.size(); i++) {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth);
				}
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_POWER: {
			MathStructure mbak(mstruct);
			calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
			calculate_limit_sub(mstruct[1], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
			if(is_plus_minus_infinity(mstruct[1]) && (mstruct[0].isOne() || mstruct[0].isZero()) && mbak[1].contains(x_var, true) && mbak[0].contains(x_var, true)) {
				mstruct.set(mbak[0], true);
				mstruct.transform(CALCULATOR->f_ln);
				mstruct *= mbak[1];
				mstruct.raise(CALCULATOR->v_e);
				mstruct.swapChildren(1, 2);
				calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, keep_inf_x);
				break;
			}
			if(mstruct[0].isFunction() && (mstruct[0].function() == CALCULATOR->f_asin || mstruct[0].function() == CALCULATOR->f_acos) && mstruct[0].size() == 1 && mstruct[0][0].isInfinite(false) && mstruct[1].representsNegative()) {
				mstruct.clear(true);
				break;
			}
			if(keep_inf_x && nr_limit.isInfinite(false) && (mstruct[0].isInfinite(false) || mstruct[1].isInfinite(false))) {
				MathStructure mbak2(mstruct);
				mstruct.calculatesub(eo, eo, false);
				if(mstruct.isInfinite(false)) {
					mstruct = mbak2;
					if(mstruct[0].isInfinite(false)) {
						mstruct[0] = mbak[0];
						calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, reduce_addition && !mstruct[1].isInfinite(false));
					} else if(mbak[0].contains(x_var, true)) {
						mstruct[0] = mbak[0];
					}
					if(mstruct[1].isInfinite(false)) {
						mstruct[1] = mbak[1];
						calculate_limit_sub(mstruct[1], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, false);
					} else if(mbak[1].contains(x_var, true)) {
						mstruct[1] = mbak[1];
					}
				}
			} else if(mstruct[0].isNumber() && !mstruct[0].number().isNonZero() && mstruct[1].representsNegative() && mbak[0].contains(x_var, true) > 0) {
				bool b_test = true;
				int i_sgn = 0;
				if(mstruct[0].number().isInterval(false) && (mstruct[0].number().hasImaginaryPart() || !mstruct[1].isNumber())) {
					b_test = false;
				}
				if(b_test && ((mbak[0].isFunction() && mbak[0].function() == CALCULATOR->f_abs) || mstruct[1].representsEven())) {
					i_sgn = 1;
					b_test = false;
				} else if(b_test) {
					if(mstruct[0].number().isInterval(false) && !mstruct[0].number().isNonNegative() && !mstruct[0].number().isNonPositive()) {
						b_test = false;
					} else {
						MathStructure mpow(mbak[0]);
						if(mstruct[1].isMinusOne()) {
							MathStructure mfac(mpow);
							mfac.factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, true);
							if(mfac.isPower() && mfac[1].representsEven()) {
								i_sgn = 1;
								b_test = false;
							}
						} else {
							mpow ^= mstruct[1];
							mpow.last().calculateNegate(eo);
						}
						if(b_test) {
							MathStructure mdiff(mpow);
							if(mdiff.differentiate(x_var, eo) && !contains_diff_for(mdiff, x_var)) {
								mdiff.replace(x_var, nr_limit);
								CALCULATOR->beginTemporaryStopMessages();
								mdiff.eval(eo);
								if(!CALCULATOR->endTemporaryStopMessages()) {
									if(mdiff.representsPositive()) {
										b_test = false;
										if(approach_direction > 0) i_sgn = 1;
										else if(approach_direction < 0) i_sgn = -1;
									} else if(mdiff.representsNegative()) {
										b_test = false;
										if(approach_direction > 0) i_sgn = -1;
										else if(approach_direction < 0) i_sgn = 1;
									}
								}
							}
							if(b_test) {
								MathStructure mtestn(nr_limit);
								if(eo.approximation != APPROXIMATION_EXACT) {
									EvaluationOptions eo2 = eo;
									eo2.approximation = APPROXIMATION_APPROXIMATE;
									mtestn.eval(eo2);
								}
								if(mtestn.isNumber() && mtestn.number().isReal()) {
									for(int i = 10; i < 20; i++) {
										if(approach_direction == 0 || (i % 2 == (approach_direction < 0))) {
											Number nr_test(i % 2 == 0 ? 1 : -1, 1, -(i / 2));
											if(!mtestn.number().isZero()) {
												nr_test++;
												if(!nr_test.multiply(mtestn.number())) {i_sgn = 0; break;}
											}
											MathStructure mtest(mpow);
											mtest.replace(x_var, nr_test);
											CALCULATOR->beginTemporaryStopMessages();
											mtest.eval(eo);
											if(CALCULATOR->endTemporaryStopMessages()) {i_sgn = 0; break;}
											int new_sgn = 0;
											if(mtest.representsPositive()) new_sgn = 1;
											else if(mtest.representsNegative()) new_sgn = -1;
											if(new_sgn != 0 || mtest.isNumber()) {
												if(new_sgn == 0 || (i_sgn != 0 && i_sgn != new_sgn)) {i_sgn = 0; break;}
												i_sgn = new_sgn;
											}
										}
										if(CALCULATOR->aborted()) {i_sgn = 0; break;}
									}
								}
							}
						}
					}
				}
				if(i_sgn != 0) {
					if(mstruct[0].number().isInterval()) {
						Number nr_pow = mstruct[1].number();
						Number nr_high(mstruct[0].number());
						if(nr_pow.negate() && nr_high.raise(nr_pow) && !nr_high.hasImaginaryPart()) {
							if(nr_high.isNonNegative() && i_sgn > 0) nr_high = nr_high.upperEndPoint();
							else if(nr_high.isNonPositive() && i_sgn < 0) nr_high = nr_high.lowerEndPoint();
							if(nr_high.isNonZero() && nr_high.recip()) {
								Number nr;
								if(i_sgn > 0) nr.setInterval(nr_high, nr_plus_inf);
								else if(i_sgn < 0) nr.setInterval(nr_minus_inf, nr_high);
								mstruct.set(nr, true);
								if(b_test) CALCULATOR->error(false, _("Limit for %s determined graphically."), format_and_print(mbak).c_str(), NULL);
								break;
							}
						}
					} else {
						if(b_test) CALCULATOR->error(false, _("Limit for %s determined graphically."), format_and_print(mbak).c_str(), NULL);
						if(i_sgn > 0) mstruct.set(nr_plus_inf, true);
						else if(i_sgn < 0) mstruct.set(nr_minus_inf, true);
						break;
					}
				}
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_FUNCTION: {
			if(keep_inf_x && nr_limit.isInfinite(false) && mstruct.size() == 1 && (mstruct.function() == CALCULATOR->f_ln || mstruct.function() == CALCULATOR->f_gamma)) {
				MathStructure mbak(mstruct);
				calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
				if(mstruct[0].isInfinite(false) && (mstruct[0].number().isPlusInfinity() || mstruct.function() == CALCULATOR->f_ln)) {
					mstruct[0] = mbak[0];
					calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, mstruct.function() == CALCULATOR->f_ln && reduce_addition);
					break;
				}
			} else {
				for(size_t i = 0; i < mstruct.size(); i++) {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
				}
				mstruct.childrenUpdated();
			}
			if(approach_direction != 0 && mstruct.function() == CALCULATOR->f_gamma && mstruct.size() == 1 && mstruct[0].isInteger() && mstruct[0].number().isNonPositive()) {
				if((mstruct[0].number().isEven() && approach_direction < 0) || (mstruct[0].number().isOdd() && approach_direction > 0)) {
					mstruct.set(nr_minus_inf, true);
				} else {
					mstruct.set(nr_plus_inf, true);
				}
				break;
			}
			mstruct.calculateFunctions(eo, true);
			mstruct.calculatesub(eo, eo, false);
			break;
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false, reduce_addition);
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
		}
	}
	return true;
}

bool replace_equal_limits(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, bool at_top = true) {
	if(!nr_limit.isInfinite(false)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_equal_limits(mstruct[i], x_var, nr_limit, eo, approach_direction, false)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(at_top) return b_ret;
	if(mstruct.isFunction() && (mstruct.function() == CALCULATOR->f_sinh || mstruct.function() == CALCULATOR->f_cosh) && mstruct.size() == 1 && mstruct.contains(x_var, true)) {
		MathStructure mterm1(CALCULATOR->v_e);
		mterm1.raise(mstruct[0]);
		MathStructure mterm2(mterm1);
		mterm2[1].negate();
		mterm1 *= nr_half;
		mterm2 *= nr_half;
		mstruct = mterm1;
		if(mstruct.function() == CALCULATOR->f_sinh) mstruct -= mterm2;
		else mstruct += mterm2;
		return true;
	}
	return b_ret;
}
bool replace_equal_limits2(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, bool at_top = true) {
	if(!nr_limit.isInfinite(false)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_equal_limits2(mstruct[i], x_var, nr_limit, eo, approach_direction, false)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower() && mstruct[i][1] == x_var && (nr_limit.number().isMinusInfinity() || mstruct[i][0].representsNonNegative())) {
				for(size_t i2 = i + 1; i2 < mstruct.size();) {
					if(mstruct[i2].isPower() && mstruct[i2][1] == x_var && (nr_limit.number().isMinusInfinity() || mstruct[i2][0].representsNonNegative())) {
						mstruct[i][0].calculateMultiply(mstruct[i2][0], eo);
						mstruct.delChild(i2 + 1, false);
					} else {
						i2++;
					}
				}
				mstruct[i].childUpdated(1);
				mstruct.childUpdated(i + 1);
				if(mstruct.size() == 1) {
					mstruct.setToChild(1, true);
					break;
				}
			}
		}
	}
	return b_ret;
}
bool replace_equal_limits3(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, bool at_top = true) {
	if(!nr_limit.isInfinite(false)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_equal_limits3(mstruct[i], x_var, nr_limit, eo, approach_direction, false)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(at_top) return b_ret;
	if(mstruct.isFunction() && (mstruct.function() == CALCULATOR->f_asinh || mstruct.function() == CALCULATOR->f_acosh) && mstruct.size() == 1 && mstruct.contains(x_var, true)) {
		MathStructure mtest(mstruct[0]);
		calculate_limit_sub(mtest, x_var, nr_limit, eo, approach_direction);
		if(mtest.isInfinite(false)) {
			if(mtest.number().isPlusInfinity()) {
				mstruct.setFunction(CALCULATOR->f_ln);
				mstruct[0] *= nr_two;
				return true;
			} else if(mstruct.function() == CALCULATOR->f_asinh) {
				mstruct.setFunction(CALCULATOR->f_ln);
				mstruct[0] *= nr_two;
				mstruct.negate();
				return true;
			}
		}
	}
	return b_ret;
}

bool MathStructure::calculateLimit(const MathStructure &x_var, const MathStructure &limit, const EvaluationOptions &eo_pre, int approach_direction) {
	EvaluationOptions eo = eo_pre;
	eo.assume_denominators_nonzero = true;
	eo.warn_about_denominators_assumed_nonzero = false;
	eo.do_polynomial_division = false;
	UnknownVariable *var = new UnknownVariable("", format_and_print(x_var));
	Assumptions *ass = new Assumptions();
	MathStructure nr_limit(limit);
	if(eo.approximation != APPROXIMATION_EXACT && nr_limit.containsInterval(true, true, false, 0, true)) {
		eo.approximation = APPROXIMATION_EXACT_VARIABLES;
	}
	nr_limit.eval(eo);
	eo.approximation = eo_pre.approximation;
	if(nr_limit.representsReal() || nr_limit.isInfinite()) ass->setType(ASSUMPTION_TYPE_REAL);
	if(nr_limit.representsPositive()) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
	else if(nr_limit.representsNegative()) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
	else if(nr_limit.isZero()) {
		if(approach_direction < 0) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
		else if(approach_direction > 0) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
		else ass->setSign(ASSUMPTION_SIGN_NONZERO);
	}
	var->setAssumptions(ass);
	replace(x_var, var);
	eval(eo);
	CALCULATOR->beginTemporaryStopMessages();
	MathStructure mbak(*this);
	if(replace_equal_limits(*this, var, nr_limit, eo, approach_direction)) eval(eo);
	replace_equal_limits2(*this, var, nr_limit, eo, approach_direction);
	if(replace_equal_limits3(*this, var, nr_limit, eo, approach_direction)) {
		eval(eo);
		replace_equal_limits2(*this, var, nr_limit, eo, approach_direction);
	}
	do_simplification(*this, eo, true, false, false, true, true);
	eo.do_polynomial_division = true;
	calculate_limit_sub(*this, var, nr_limit, eo, approach_direction);
	if(CALCULATOR->aborted() || (containsInfinity(true) && !isInfinite(true)) || limit_contains_undefined(*this)) {
		set(mbak);
		replace(var, x_var);
		var->destroy();
		CALCULATOR->endTemporaryStopMessages();
		return false;
	}
	replace(var, nr_limit);
	var->destroy();
	CALCULATOR->endTemporaryStopMessages(true);
	return true;
}


#define IS_VAR_EXP(x, ofa) ((x.isVariable() && x.variable()->isKnown() && (!ofa || x.variable()->title() == "\b")) || (x.isPower() && x[0].isVariable() && x[0].variable()->isKnown() && (!ofa || x[0].variable()->title() == "\b")))

void factorize_variable(MathStructure &mstruct, const MathStructure &mvar, bool deg2) {
	if(deg2) {
		// ax^2+bx = (sqrt(b)*x+(a/sqrt(b))/2)^2-((a/sqrt(b))/2)^2
		MathStructure a_struct, b_struct, mul_struct(1, 1, 0);
		for(size_t i2 = 0; i2 < mstruct.size();) {
			bool b = false;
			if(mstruct[i2] == mvar) {
				a_struct.set(1, 1, 0);
				b = true;
			} else if(mstruct[i2].isPower() && mstruct[i2][0] == mvar && mstruct[i2][1].isNumber() && mstruct[i2][1].number().isTwo()) {
				b_struct.set(1, 1, 0);
				b = true;
			} else if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
					if(mstruct[i2][i3] == mvar) {
						a_struct = mstruct[i2];
						a_struct.delChild(i3 + 1);
						b = true;
						break;
					} else if(mstruct[i2][i3].isPower() && mstruct[i2][i3][0] == mvar && mstruct[i2][i3][1].isNumber() && mstruct[i2][i3][1].number().isTwo()) {
						b_struct = mstruct[i2];
						b_struct.delChild(i3 + 1);
						b = true;
						break;
					}
				}
			}
			if(b) {
				mstruct.delChild(i2 + 1);
			} else {
				i2++;
			}
		}
		if(b_struct == a_struct) {
			if(a_struct.isMultiplication() && a_struct.size() == 1) mul_struct = a_struct[0];
			else mul_struct = a_struct;
			a_struct.set(1, 1, 0);
			b_struct.set(1, 1, 0);
		} else if(b_struct.isMultiplication() && a_struct.isMultiplication()) {
			size_t i3 = 0;
			for(size_t i = 0; i < a_struct.size();) {
				bool b = false;
				for(size_t i2 = i3; i2 < b_struct.size(); i2++) {
					if(a_struct[i] == b_struct[i2]) {
						i3 = i2;
						if(mul_struct.isOne()) mul_struct = a_struct[i];
						else mul_struct.multiply(a_struct[i], true);
						a_struct.delChild(i + 1);
						b_struct.delChild(i2 + 1);
						b = true;
						break;
					}
				}
				if(!b) i++;
			}
		}
		if(a_struct.isMultiplication() && a_struct.size() == 0) a_struct.set(1, 1, 0);
		else if(a_struct.isMultiplication() && a_struct.size() == 1) a_struct.setToChild(1);
		if(b_struct.isMultiplication() && b_struct.size() == 0) b_struct.set(1, 1, 0);
		else if(b_struct.isMultiplication() && b_struct.size() == 1) b_struct.setToChild(1);
		if(!b_struct.isOne()) {
			b_struct.raise(nr_half);
			a_struct.divide(b_struct);
		}
		a_struct.multiply(nr_half);
		if(b_struct.isOne()) b_struct = mvar;
		else b_struct *= mvar;
		b_struct += a_struct;
		b_struct.raise(nr_two);
		a_struct.raise(nr_two);
		a_struct.negate();
		b_struct += a_struct;
		if(!mul_struct.isOne()) b_struct *= mul_struct;
		if(mstruct.size() == 0) mstruct = b_struct;
		else mstruct.addChild(b_struct);
	} else {
		vector<MathStructure*> left_structs;
		for(size_t i2 = 0; i2 < mstruct.size();) {
			bool b = false;
			if(mstruct[i2] == mvar) {
				mstruct[i2].set(1, 1, 0, true);
				b = true;
			} else if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
					if(mstruct[i2][i3] == mvar) {
						mstruct[i2].delChild(i3 + 1, true);
						b = true;
						break;
					}
				}
			}
			if(b) {
				i2++;
			} else {
				mstruct[i2].ref();
				left_structs.push_back(&mstruct[i2]);
				mstruct.delChild(i2 + 1);
			}
		}
		mstruct.multiply(mvar);
		for(size_t i = 0; i < left_structs.size(); i++) {
			mstruct.add_nocopy(left_structs[i], true);
			mstruct.evalSort(false);
		}
	}
}

bool var_contains_interval(const MathStructure &mstruct) {
	if(mstruct.isNumber()) return mstruct.number().isInterval();
	if(mstruct.isFunction() && (mstruct.function() == CALCULATOR->f_interval || mstruct.function() == CALCULATOR->f_uncertainty)) return true;
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) return var_contains_interval(((KnownVariable*) mstruct.variable())->get());
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(var_contains_interval(mstruct[i])) return true;
	}
	return false;
}

bool factorize_variables(MathStructure &mstruct, const EvaluationOptions &eo, bool ofa = false) {
	bool b = false;
	if(mstruct.type() == STRUCT_ADDITION) {
		vector<MathStructure> variables;
		vector<size_t> variable_count;
		vector<int> term_sgn;
		vector<bool> term_deg2;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(CALCULATOR->aborted()) break;
					if(IS_VAR_EXP(mstruct[i][i2], ofa)) {
						bool b_found = false;
						for(size_t i3 = 0; i3 < variables.size(); i3++) {
							if(variables[i3] == mstruct[i][i2]) {
								variable_count[i3]++;
								b_found = true;
								if(term_sgn[i3] == 1 && !mstruct[i].representsNonNegative(true)) term_sgn[i3] = 0;
								else if(term_sgn[i3] == -1 && !mstruct[i].representsNonPositive(true)) term_sgn[i3] = 0;
								break;
							}
						}
						if(!b_found) {
							variables.push_back(mstruct[i][i2]);
							variable_count.push_back(1);
							term_deg2.push_back(false);
							if(mstruct[i].representsNonNegative(true)) term_sgn.push_back(1);
							else if(mstruct[i].representsNonPositive(true)) term_sgn.push_back(-1);
							else term_sgn.push_back(0);
						}
					}
				}
			} else if(IS_VAR_EXP(mstruct[i], ofa)) {
				bool b_found = false;
				for(size_t i3 = 0; i3 < variables.size(); i3++) {
					if(variables[i3] == mstruct[i]) {
						variable_count[i3]++;
						b_found = true;
						if(term_sgn[i3] == 1 && !mstruct[i].representsNonNegative(true)) term_sgn[i3] = 0;
						else if(term_sgn[i3] == -1 && !mstruct[i].representsNonPositive(true)) term_sgn[i3] = 0;
						break;
					}
				}
				if(!b_found) {
					variables.push_back(mstruct[i]);
					variable_count.push_back(1);
					term_deg2.push_back(false);
					if(mstruct[i].representsNonNegative(true)) term_sgn.push_back(1);
					else if(mstruct[i].representsNonPositive(true)) term_sgn.push_back(-1);
					else term_sgn.push_back(0);
				}
			}
		}
		for(size_t i = 0; i < variables.size();) {
			bool b_erase = false;
			if(!var_contains_interval(variables[i])) {
				b_erase = true;
			} else if(variable_count[i] == 1 || term_sgn[i] != 0) {
				b_erase = true;
				if(!variables[i].isPower() || (variables[i][1].isNumber() && variables[i][1].number().isTwo())) {
					for(size_t i2 = i + 1; i2 < variables.size(); i2++) {
						if((variables[i].isPower() && !variables[i2].isPower() && variables[i][0] == variables[i2])|| (!variables[i].isPower() && variables[i2].isPower() && variables[i2][0] == variables[i] && variables[i2][1].isNumber() && variables[i2][1].number().isTwo())) {
							bool b_erase2 = false;
							if(variable_count[i2] == 1) {
								if(term_sgn[i] == 0 || term_sgn[i2] != term_sgn[i]) {
									if(variable_count[i] == 1) {
										term_deg2[i] = true;
										variable_count[i] = 2;
										term_sgn[i] = 0;
										if(variables[i].isPower()) variables[i].setToChild(1);
									} else {
										term_sgn[i] = 0;
									}
									b_erase = false;
								}
								b_erase2 = true;
							} else if(term_sgn[i2] != 0) {
								if(term_sgn[i] == 0 || term_sgn[i2] != term_sgn[i]) {
									if(variable_count[i] != 1) {
										term_sgn[i] = 0;
										b_erase = false;
									}
									term_sgn[i2] = 0;
								} else {
									b_erase2 = true;
								}
							}
							if(b_erase2) {
								variable_count.erase(variable_count.begin() + i2);
								variables.erase(variables.begin() + i2);
								term_deg2.erase(term_deg2.begin() + i2);
								term_sgn.erase(term_sgn.begin() + i2);
							}
							break;
						}
					}
				}
			}
			if(b_erase) {
				variable_count.erase(variable_count.begin() + i);
				variables.erase(variables.begin() + i);
				term_deg2.erase(term_deg2.begin() + i);
				term_sgn.erase(term_sgn.begin() + i);
			} else if(variable_count[i] == mstruct.size()) {
				factorize_variable(mstruct, variables[i], term_deg2[i]);
				if(CALCULATOR->aborted()) return true;
				factorize_variables(mstruct, eo, ofa);
				return true;
			} else {
				i++;
			}
		}
		if(variables.size() == 1) {
			factorize_variable(mstruct, variables[0], term_deg2[0]);
			if(CALCULATOR->aborted()) return true;
			factorize_variables(mstruct, eo, ofa);
			return true;
		}
		Number uncertainty;
		size_t u_index = 0;
		for(size_t i = 0; i < variables.size(); i++) {
			const MathStructure *v_ms;
			Number nr;
			if(variables[i].isPower()) v_ms = &((KnownVariable*) variables[i][0].variable())->get();
			else v_ms = &((KnownVariable*) variables[i].variable())->get();
			if(v_ms->isNumber()) nr = v_ms->number();
			else if(v_ms->isMultiplication() && v_ms->size() > 0 && (*v_ms)[0].isNumber()) nr = (v_ms)[0].number();
			else {
				MathStructure mtest(*v_ms);
				mtest.unformat(eo);
				mtest.calculatesub(eo, eo, true);
				if(mtest.isNumber()) nr = mtest.number();
				else if(mtest.isMultiplication() && mtest.size() > 0 && mtest[0].isNumber()) nr = mtest[0].number();
			}
			if(nr.isInterval()) {
				Number u_candidate(nr.uncertainty());
				if(variables[i].isPower() && variables[i][1].isNumber() && variables[i][1].number().isReal()) u_candidate.raise(variables[i][1].number());
				u_candidate.multiply(variable_count[i]);
				if(u_candidate.isGreaterThan(uncertainty)) {
					uncertainty = u_candidate;
					u_index = i;
				}
			}
		}
		if(!uncertainty.isZero()) {
			factorize_variable(mstruct, variables[u_index], term_deg2[u_index]);
			if(CALCULATOR->aborted()) return true;
			factorize_variables(mstruct, eo, ofa);
			return true;
		}

	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(factorize_variables(mstruct[i], eo, ofa)) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
		if(CALCULATOR->aborted()) return b;
	}
	return b;
}

void find_interval_variables(const MathStructure &mstruct, vector<KnownVariable*> &vars, vector<int> &v_count, vector<int> &v_prec) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) {
		KnownVariable *v = (KnownVariable*) mstruct.variable();
		int var_prec = PRECISION + 11;
		const MathStructure &mv = v->get();
		for(size_t i = 0; i < vars.size(); i++) {
			if(vars[i] == v) {
				v_count[i]++;
				return;
			}
		}
		if(mv.isNumber()) {
			if(mv.number().isInterval()) var_prec = mv.number().precision(1);
			else if(CALCULATOR->usesIntervalArithmetic() && mv.number().precision() >= 0) var_prec = mv.number().precision();
		} else if(mv.isMultiplication()) {
			for(size_t i = 0; i < mv.size(); i++) {
				if(mv[i].isNumber()) {
					if(mv[i].number().isInterval()) {var_prec = mv[i].number().precision(1); break;}
					else if(mv[i].number().precision() >= 0) {var_prec = mv[i].number().precision(); break;}
				}
			}
		}
		if(var_prec <= PRECISION + 10) {
			bool b = false;
			for(size_t i = 0; i < v_prec.size(); i++) {
				if(var_prec < v_prec[i]) {
					v_prec.insert(v_prec.begin() + i, var_prec);
					v_count.insert(v_count.begin() + i, 1);
					vars.insert(vars.begin() + i, v);
					b = true;
					break;
				}
			}
			if(!b) {
				v_prec.push_back(var_prec);
				v_count.push_back(1);
				vars.push_back(v);
			}
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		find_interval_variables(mstruct[i], vars, v_count, v_prec);
	}
}
bool contains_not_nonzero(MathStructure &m) {
	if(m.isNumber() && !m.number().isNonZero()) {
		return true;
	} else if(m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(contains_not_nonzero(m[i])) return true;
		}
	}
	return false;
}
bool contains_undefined(MathStructure &m, const EvaluationOptions &eo = default_evaluation_options, bool calc = false, const MathStructure &x_var = m_zero, const MathStructure &m_intval = nr_zero) {
	if(m.isPower() && (m[1].representsNegative() || (m[1].isNumber() && !m[1].number().isNonNegative()))) {
		if(calc) {
			m[0].replace(x_var, m_intval, true);
			m[0].calculatesub(eo, eo, true);
		}
		if(contains_not_nonzero(m[0])) return true;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_undefined(m[i], eo, calc, x_var, m_intval)) return true;
	}
	return false;
}
bool find_interval_zeroes(const MathStructure &mstruct, MathStructure &malts, const MathStructure &mvar, const Number &nr_intval, const EvaluationOptions &eo, int depth, const Number &nr_prec, int orig_prec = 0, int is_real = -1, int undef_depth = 0) {
	if(CALCULATOR->aborted()) return false;
	if(depth == 0) orig_prec = nr_intval.precision(1);
	MathStructure mtest(mstruct);
	mtest.replace(mvar, nr_intval);
	mtest.eval(eo);
	if(is_real < 0) is_real = mtest.representsNonComplex(true);
	ComparisonResult cmp;
	if(is_real == 0) {
		MathStructure m_re(CALCULATOR->f_re, &mtest, NULL);
		m_re.calculateFunctions(eo);
		cmp = m_re.compare(m_zero);
		MathStructure m_im(CALCULATOR->f_im, &mtest, NULL);
		m_im.calculateFunctions(eo);
		ComparisonResult cmp2 = m_im.compare(m_zero);
		if(COMPARISON_IS_NOT_EQUAL(cmp) || cmp2 == COMPARISON_RESULT_EQUAL || cmp == COMPARISON_RESULT_UNKNOWN) cmp = cmp2;
	} else {
		cmp = mtest.compare(m_zero);
	}
	if(COMPARISON_IS_NOT_EQUAL(cmp)) {
		return true;
	} else if(cmp != COMPARISON_RESULT_UNKNOWN || (undef_depth <= 5 && contains_undefined(mtest))) {
		if(cmp == COMPARISON_RESULT_EQUAL || (nr_intval.precision(1) > (orig_prec > PRECISION ? orig_prec + 5 : PRECISION + 5) || (!nr_intval.isNonZero() && nr_intval.uncertainty().isLessThan(nr_prec)))) {
			if(cmp == COMPARISON_RESULT_EQUAL && depth <= 3) return false;
			if(malts.size() > 0 && (cmp = malts.last().compare(nr_intval)) != COMPARISON_RESULT_UNKNOWN && COMPARISON_MIGHT_BE_EQUAL(cmp)) {
				malts.last().number().setInterval(malts.last().number(), nr_intval);
				if(malts.last().number().precision(1) < (orig_prec > PRECISION ? orig_prec + 3 : PRECISION + 3)) {
					return false;
				}
			} else {
				malts.addChild(nr_intval);
			}
			return true;
		}
		vector<Number> splits;
		nr_intval.splitInterval(2, splits);
		for(size_t i = 0; i < splits.size(); i++) {
			if(!find_interval_zeroes(mstruct, malts, mvar, splits[i], eo, depth + 1, nr_prec, orig_prec, is_real, cmp == COMPARISON_RESULT_UNKNOWN ? undef_depth + 1 : 0)) return false;
		}
		return true;
	}
	return false;
}
bool contains_interval_variable(const MathStructure &m, int i_type = 0) {
	if(i_type == 0 && m.isVariable() && m.containsInterval(true, true, false, 1, false)) return true;
	else if(i_type == 1 && m.containsInterval(true, false, false, 1, true)) return true;
	else if(i_type == 2 && m.containsInterval(true, true, false, 1, true)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_interval_variable(m[i])) return true;
	}
	return false;
}
bool function_differentiable(MathFunction *o_function) {
	return (o_function == CALCULATOR->f_sqrt || o_function == CALCULATOR->f_root || o_function == CALCULATOR->f_cbrt || o_function == CALCULATOR->f_ln || o_function == CALCULATOR->f_logn || o_function == CALCULATOR->f_arg || o_function == CALCULATOR->f_gamma || o_function == CALCULATOR->f_beta || o_function == CALCULATOR->f_abs || o_function == CALCULATOR->f_factorial || o_function == CALCULATOR->f_besselj || o_function == CALCULATOR->f_bessely || o_function == CALCULATOR->f_erf || o_function == CALCULATOR->f_erfc || o_function == CALCULATOR->f_li || o_function == CALCULATOR->f_Li || o_function == CALCULATOR->f_Ei || o_function == CALCULATOR->f_Si || o_function == CALCULATOR->f_Ci || o_function == CALCULATOR->f_Shi || o_function == CALCULATOR->f_Chi || o_function == CALCULATOR->f_abs || o_function == CALCULATOR->f_signum || o_function == CALCULATOR->f_heaviside || o_function == CALCULATOR->f_lambert_w || o_function == CALCULATOR->f_sinc || o_function == CALCULATOR->f_sin || o_function == CALCULATOR->f_cos || o_function == CALCULATOR->f_tan || o_function == CALCULATOR->f_asin || o_function == CALCULATOR->f_acos || o_function == CALCULATOR->f_atan || o_function == CALCULATOR->f_sinh || o_function == CALCULATOR->f_cosh || o_function == CALCULATOR->f_tanh || o_function == CALCULATOR->f_asinh || o_function == CALCULATOR->f_acosh || o_function == CALCULATOR->f_atanh || o_function == CALCULATOR->f_stripunits);
}

bool is_differentiable(const MathStructure &m) {
	if(m.isFunction() && !function_differentiable(m.function())) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(!is_differentiable(m[i])) return false;
	}
	return true;
}


bool calculate_differentiable_functions(MathStructure &m, const EvaluationOptions &eo, bool recursive = true, bool do_unformat = true) {
	if(m.isFunction() && m.function() != eo.protected_function && function_differentiable(m.function())) {
		return m.calculateFunctions(eo, recursive, do_unformat);
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < m.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(calculate_differentiable_functions(m[i], eo, recursive, do_unformat)) {
				m.childUpdated(i + 1);
				b = true;
			}
		}
	}
	return b;
}
bool calculate_nondifferentiable_functions(MathStructure &m, const EvaluationOptions &eo, bool recursive = true, bool do_unformat = true, int i_type = 0) {
	if(m.isFunction() && m.function() != eo.protected_function) {
		if((i_type <= 0 && !function_differentiable(m.function())) || (i_type >= 0 && !contains_interval_variable(m, i_type))) {
			if(m.calculateFunctions(eo, false, do_unformat)) {
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
		} else if(m.function() == CALCULATOR->f_abs && m.size() == 1) {
			EvaluationOptions eo3 = eo;
			eo3.split_squares = false;
			eo3.assume_denominators_nonzero = false;
			if(eo.approximation == APPROXIMATION_APPROXIMATE && !m.containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
			else eo3.approximation = APPROXIMATION_EXACT;
			m[0].calculatesub(eo3, eo);
			m.childUpdated(1);
			if(m[0].representsNegative(true)) {
				m.setToChild(1);
				m.negate();
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
			if(m[0].representsNonNegative(true)) {
				m.setToChild(1);
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
			if(m[0].isMultiplication()) {
				m.setToChild(1);
				for(size_t i = 0; i < m.size(); i++) {
					m[i].transform(CALCULATOR->f_abs);
				}
				m.childrenUpdated();
				if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
				return true;
			}
			if(eo.approximation != APPROXIMATION_EXACT) {
				eo3.approximation = APPROXIMATION_APPROXIMATE;
				MathStructure mtest(m[0]);
				mtest.calculatesub(eo3, eo);
				if(mtest.representsNegative(true)) {
					m.setToChild(1);
					m.negate();
					if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
					return true;
				}
				if(mtest.representsNonNegative(true)) {
					m.setToChild(1);
					if(recursive) calculate_nondifferentiable_functions(m, eo, recursive, do_unformat, i_type);
					return true;
				}
			}
		}
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < m.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(calculate_nondifferentiable_functions(m[i], eo, recursive, do_unformat, i_type)) {
				m.childUpdated(i + 1);
				b = true;
			}
		}
	}
	return b;
}

void remove_nonzero_mul(MathStructure &msolve, const MathStructure &u_var, const EvaluationOptions &eo) {
	if(!msolve.isMultiplication()) return;
	for(size_t i = 0; i < msolve.size();) {
		if(!msolve[i].contains(u_var, true)) {
			msolve[i].eval(eo);
			if(msolve[i].representsNonZero(true)) {
				if(msolve.size() == 2) {
					msolve.delChild(i + 1, true);
					break;
				}
				msolve.delChild(i + 1, true);
			} else {
				remove_nonzero_mul(msolve[i], u_var, eo);
				i++;
			}
		} else {
			remove_nonzero_mul(msolve[i], u_var, eo);
			i++;
		}
	}
}

void solve_intervals2(MathStructure &mstruct, vector<KnownVariable*> vars, const EvaluationOptions &eo_pre) {
	if(vars.size() > 0) {
		EvaluationOptions eo = eo_pre;
		eo.approximation = APPROXIMATION_EXACT_VARIABLES;
		eo.expand = false;
		if(eo.calculate_functions) calculate_differentiable_functions(mstruct, eo);
		KnownVariable *v = vars[0];
		vars.erase(vars.begin());
		UnknownVariable *u_var = new UnknownVariable("", "u");
		Number nr_intval;
		MathStructure mvar(u_var);
		const MathStructure &mv = v->get();
		MathStructure mmul(1, 1, 0);
		if(mv.isMultiplication()) {
			for(size_t i = 0; i < mv.size(); i++) {
				if(mv[i].isNumber() && mv[i].number().isInterval()) {
					mmul = mv;
					mmul.unformat(eo);
					mmul.delChild(i + 1, true);
					mvar.multiply(mmul);
					nr_intval = mv[i].number();
					u_var->setInterval(nr_intval);
					break;
				}
			}
		} else {
			nr_intval = mv.number();
			u_var->setInterval(mv);
		}
		MathStructure msolve(mstruct);
		msolve.replace(v, mvar);
		bool b = true;
		CALCULATOR->beginTemporaryStopMessages();

		if(!msolve.differentiate(u_var, eo) || msolve.countTotalChildren(false) > 10000 || contains_diff_for(msolve, u_var) || CALCULATOR->aborted()) {
			b = false;
		}

		MathStructure malts;
		malts.clearVector();
		if(b) {
			eo.keep_zero_units = false;
			eo.approximation = APPROXIMATION_APPROXIMATE;
			eo.expand = eo_pre.expand;
			MathStructure mtest(msolve);
			mtest.replace(u_var, nr_intval);
			mtest.eval(eo);
			if(mtest.countTotalChildren(false) < 100) {
				if(mtest.representsNonComplex(true)) {
					ComparisonResult cmp = mtest.compare(m_zero);
					if(!COMPARISON_IS_EQUAL_OR_GREATER(cmp) && !COMPARISON_IS_EQUAL_OR_LESS(cmp)) b = false;
				} else {
					MathStructure m_re(CALCULATOR->f_re, &mtest, NULL);
					m_re.calculateFunctions(eo);
					ComparisonResult cmp = m_re.compare(m_zero);
					if(!COMPARISON_IS_EQUAL_OR_GREATER(cmp) && !COMPARISON_IS_EQUAL_OR_LESS(cmp)) {
						b = false;
					} else {
						MathStructure m_im(CALCULATOR->f_im, &mtest, NULL);
						m_im.calculateFunctions(eo);
						ComparisonResult cmp = m_im.compare(m_zero);
						if(!COMPARISON_IS_EQUAL_OR_GREATER(cmp) && !COMPARISON_IS_EQUAL_OR_LESS(cmp)) b = false;
					}
				}
			} else {
				b = false;
			}
			eo.expand = false;
			eo.approximation = APPROXIMATION_EXACT_VARIABLES;
			if(!b) {
				b = true;
				msolve.calculatesub(eo, eo, true);
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.expand = eo_pre.expand;
				msolve.factorize(eo, false, false, 0, false, true, NULL, m_undefined, false, false, 1);
				remove_nonzero_mul(msolve, u_var, eo);
				if(msolve.isZero()) {
				} else if(contains_undefined(msolve) || msolve.countTotalChildren(false) > 1000 || msolve.containsInterval(true, true, false, 1, true)) {
					b = false;
				} else {
					MathStructure mtest(mstruct);
					mtest.replace(v, u_var);
					mtest.calculatesub(eo, eo, true);
					if(contains_undefined(mtest, eo, true, u_var, mv)) {
						b = false;
					} else {
						Number nr_prec(1, 1, -(PRECISION + 10));
						nr_prec *= nr_intval.uncertainty();
						b = find_interval_zeroes(msolve, malts, u_var, nr_intval, eo, 0, nr_prec);
					}
				}
				eo.expand = false;
				eo.approximation = APPROXIMATION_EXACT_VARIABLES;
			}
			eo.keep_zero_units = eo_pre.keep_zero_units;
		}
		CALCULATOR->endTemporaryStopMessages();
		CALCULATOR->beginTemporaryStopMessages();
		if(b) {
			malts.addChild(nr_intval.lowerEndPoint());
			malts.addChild(nr_intval.upperEndPoint());
			MathStructure mnew;
			for(size_t i = 0; i < malts.size(); i++) {
				MathStructure mlim(mstruct);
				if(!mmul.isOne()) malts[i] *= mmul;
				mlim.replace(v, malts[i]);
				mlim.calculatesub(eo, eo, true);
				vector<KnownVariable*> vars2 = vars;
				solve_intervals2(mlim, vars2, eo_pre);
				if(i == 0) {
					mnew = mlim;
				} else {
					MathStructure mlim1(mnew);
					if(!create_interval(mnew, mlim1, mlim)) {
						eo.approximation = APPROXIMATION_APPROXIMATE;
						eo.expand = eo_pre.expand;
						mlim.eval(eo);
						if(!create_interval(mnew, mlim1, mlim)) {
							mlim1.eval(eo);
							eo.expand = false;
							eo.approximation = APPROXIMATION_EXACT_VARIABLES;
							if(!create_interval(mnew, mlim1, mlim)) {
								b = false;
								break;
							}
						}
					}
				}
			}
			if(b) mstruct = mnew;
		}
		CALCULATOR->endTemporaryStopMessages(b);
		if(!b) {
			CALCULATOR->error(false, MESSAGE_CATEGORY_WIDE_INTERVAL, _("Interval potentially calculated wide."), NULL);
			mstruct.replace(v, v->get());
			mstruct.unformat(eo_pre);
			solve_intervals2(mstruct, vars, eo_pre);
		}
		u_var->destroy();
	}
}
KnownVariable *fix_find_interval_variable(MathStructure &mstruct) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) {
		const MathStructure &m = ((KnownVariable*) mstruct.variable())->get();
		if(contains_interval_variable(m)) return (KnownVariable*) mstruct.variable();
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		KnownVariable *v = fix_find_interval_variable(mstruct[i]);
		if(v) return v;
	}
	return NULL;
}
KnownVariable *fix_find_interval_variable2(MathStructure &mstruct) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) {
		const MathStructure &m = ((KnownVariable*) mstruct.variable())->get();
		if(m.isNumber()) return NULL;
		if(m.isMultiplication()) {
			bool b_intfound = false;;
			for(size_t i = 0; i < m.size(); i++) {
				if(m[i].containsInterval(true, false, false, 1)) {
					if(b_intfound || !m[i].isNumber()) return (KnownVariable*) mstruct.variable();
					b_intfound = true;
				}
			}
		} else if(m.containsInterval(true, false, false, 1)) {
			return (KnownVariable*) mstruct.variable();
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		KnownVariable *v = fix_find_interval_variable2(mstruct[i]);
		if(v) return v;
	}
	return NULL;
}
bool replace_intervals(MathStructure &m) {
	if(m.isNumber()) {
		int var_prec = 0;
		if(m.number().isInterval()) var_prec = m.number().precision(1);
		else if(CALCULATOR->usesIntervalArithmetic() && m.number().precision() >= 0) var_prec = m.number().precision();
		if(var_prec <= PRECISION + 10) {
			Variable *v = new KnownVariable("", format_and_print(m), m);
			m.set(v, true);
			v->destroy();
		}
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_intervals(m[i])) {
			m.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}
void fix_interval_variable(KnownVariable *v, MathStructure &mvar) {
	mvar = v->get();
	replace_intervals(mvar);
}

void solve_intervals(MathStructure &mstruct, const EvaluationOptions &eo, const EvaluationOptions &feo) {
	bool b = false;
	while(true) {
		KnownVariable *v = fix_find_interval_variable(mstruct);
		if(!v) break;
		b = true;
		MathStructure mvar;
		fix_interval_variable(v, mvar);
		mstruct.replace(v, mvar);
	}
	while(true) {
		KnownVariable *v = fix_find_interval_variable2(mstruct);
		if(!v) break;
		b = true;
		MathStructure mvar;
		fix_interval_variable(v, mvar);
		mstruct.replace(v, mvar);
	}
	if(b) {
		mstruct.unformat(eo);
		EvaluationOptions eo2 = eo;
		eo2.expand = false;
		mstruct.calculatesub(eo2, feo, true);
	}
	vector<KnownVariable*> vars; vector<int> v_count; vector<int> v_prec;
	find_interval_variables(mstruct, vars, v_count, v_prec);
	for(size_t i = 0; i < v_count.size();) {
		if(v_count[i] < 2 || (feo.approximation == APPROXIMATION_EXACT && vars[i]->title() != "\b")) {
			v_count.erase(v_count.begin() + i);
			v_prec.erase(v_prec.begin() + i);
			vars.erase(vars.begin() + i);
		} else {
			i++;
		}
	}
	if(mstruct.isComparison()) {
		if(feo.test_comparisons || feo.isolate_x) {
			mstruct[0].subtract(mstruct[1]);
			solve_intervals2(mstruct[0], vars, eo);
			mstruct[1].clear(true);
		} else {
			solve_intervals2(mstruct[0], vars, eo);
			solve_intervals2(mstruct[1], vars, eo);
		}
		return;
	}
	solve_intervals2(mstruct, vars, eo);
}

bool simplify_ln(MathStructure &mstruct) {
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(simplify_ln(mstruct[i])) b_ret = true;
	}
	if(mstruct.isAddition()) {
		size_t i_ln = (size_t) -1, i_ln_m = (size_t) -1;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isFunction() && mstruct[i].function() == CALCULATOR->f_ln && mstruct[i].size() == 1 && mstruct[i][0].isNumber() && mstruct[i][0].number().isReal()) {
				if(i_ln == (size_t) -1) {
					i_ln = i;
				} else {
					bool b = true;
					if(mstruct[i_ln].isMultiplication()) {
						if(mstruct[i_ln][1][0].number().raise(mstruct[i_ln][0].number(), true)) {mstruct[i_ln].setToChild(2, true); b_ret = true;}
						else b = false;
					}
					if(b && mstruct[i_ln][0].number().multiply(mstruct[i][0].number())) {
						mstruct.delChild(i + 1);
						i--;
						b_ret = true;
					}
				}
			} else if(mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1].isFunction() && mstruct[i][1].function() == CALCULATOR->f_ln && mstruct[i][1].size() == 1 && mstruct[i][1][0].isNumber() && mstruct[i][1][0].number().isReal() && mstruct[i][0].isInteger() && mstruct[i][0].number().isLessThan(1000) && mstruct[i][0].number().isGreaterThan(-1000)) {
				if(mstruct[i][0].number().isPositive()) {
					if(i_ln == (size_t) -1) {
						i_ln = i;
					} else {
						bool b = true;
						if(mstruct[i_ln].isMultiplication()) {
							if(mstruct[i_ln][1][0].number().raise(mstruct[i_ln][0].number())) {mstruct[i_ln].setToChild(2, true); b_ret = true;}
							else b = false;
						}
						if(b && mstruct[i][1][0].number().raise(mstruct[i][0].number(), true)) {
							if(mstruct[i_ln][0].number().multiply(mstruct[i][1][0].number())) {
								mstruct.delChild(i + 1);
								i--;
							} else {
								mstruct[i].setToChild(1, true);
							}
							b_ret = true;
						}
					}
				} else if(mstruct[i][0].number().isNegative()) {
					if(i_ln_m == (size_t) -1) {
						i_ln_m = i;
					} else {
						bool b = mstruct[i_ln_m][0].number().isMinusOne();
						if(!b && mstruct[i_ln_m][1][0].number().raise(-mstruct[i_ln_m][0].number())) {mstruct[i_ln_m][0].set(m_minus_one, true); b_ret = true; b = true;}
						bool b_m1 = b && mstruct[i][0].number().isMinusOne();
						if(b && (b_m1 || mstruct[i][1][0].number().raise(-mstruct[i][0].number(), true))) {
							if(mstruct[i_ln_m][1][0].number().multiply(mstruct[i][1][0].number())) {
								mstruct.delChild(i + 1);
								b_ret = true;
								i--;
							} else if(!b_m1) b_ret = true;
						}
					}
				}
			}
		}
		if(mstruct.size() == 1) mstruct.setToChild(1, true);
	}
	return b_ret;
}

void find_interval_create_var(const Number &nr, MathStructure &m, MathStructure &unc, MathStructure &unc2, KnownVariable **v, KnownVariable **v2) {
	if(nr.hasImaginaryPart() && nr.internalImaginary()->isInterval()) {
		if(nr.hasRealPart() && nr.isInterval(false)) {
			unc = nr.internalImaginary()->uncertainty();
			unc2 = nr.realPart().uncertainty();
			Number nmid(*nr.internalImaginary());
			nmid.intervalToMidValue();
			Number nmid2(nr.realPart());
			nmid2.intervalToMidValue();
			*v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
			(*v)->setApproximate(false);
			*v2 = new KnownVariable("", string("(") + format_and_print(nmid2) + ")", nmid2);
			(*v2)->setApproximate(false);
			m.set(*v);
			m.multiply(nr_one_i);
			m.add(*v2);
			(*v)->destroy();
			(*v2)->destroy();
		} else {
			unc = nr.internalImaginary()->uncertainty();
			Number nmid(*nr.internalImaginary());
			nmid.intervalToMidValue();
			*v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
			(*v)->setApproximate(false);
			m.set(*v);
			m.multiply(nr_one_i);
			(*v)->destroy();
		}
	} else {
		unc = nr.uncertainty();
		Number nmid(nr);
		nmid.intervalToMidValue();
		*v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
		(*v)->setApproximate(false);
		m.set(*v);
		(*v)->destroy();
	}
}

KnownVariable *find_interval_replace_var(MathStructure &m, MathStructure &unc, MathStructure &unc2, KnownVariable **v2, const EvaluationOptions &eo, MathStructure *mnew, Variable **prev_v, bool &b_failed) {
	if(eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables && m.isVariable() && m.variable()->isKnown() && (eo.approximation != APPROXIMATION_EXACT || m.variable()->title() == "\b")) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(!mvar.containsInterval(true, true, false, 1, true)) return NULL;
		if(mvar.isNumber()) {
			m.variable()->ref();
			*prev_v = m.variable();
			KnownVariable *v = NULL;
			find_interval_create_var(mvar.number(), m, unc, unc2, &v, v2);
			*mnew = m;
			return v;
		} else if(mvar.isMultiplication() && mvar[0].isNumber()) {
			if(mvar[0].number().isInterval(false)) {
				bool b = true;
				for(size_t i = 1; i < mvar.size(); i++) {
					if(mvar[i].containsInterval(true, true, false, 1, true)) {
						b = false;
						break;
					}
				}
				if(b) {
					m.variable()->ref();
					*prev_v = m.variable();
					KnownVariable *v = NULL;
					find_interval_create_var(mvar[0].number(), m, unc, unc2, &v, v2);
					for(size_t i = 1; i < mvar.size(); i++) {
						m.multiply(mvar[i], true);
					}
					*mnew = m;
					return v;
				}
			}
		} else if(mvar.isFunction() && mvar.function() == CALCULATOR->f_interval && mvar.size() == 2 && !mvar[0].containsInterval(true, true, false, 1, true) && !mvar[1].containsInterval(true, true, false, 1, true)) {
			if(mvar[0].isAddition() && mvar[0].size() == 2 && mvar[1].isAddition() && mvar[1].size() == 2) {
				const MathStructure *mmid = NULL, *munc = NULL;
				if(mvar[0][0].equals(mvar[1][0])) {
					mmid = &mvar[0][0];
					if(mvar[0][1].isNegate() && mvar[0][1][0].equals(mvar[1][1])) munc = &mvar[1][1];
					if(mvar[1][1].isNegate() && mvar[1][1][0].equals(mvar[0][1])) munc = &mvar[0][1];
				} else if(mvar[0][1].equals(mvar[1][1])) {
					mmid = &mvar[0][1];
					if(mvar[0][0].isNegate() && mvar[0][0][0].equals(mvar[1][0])) munc = &mvar[1][0];
					if(mvar[1][0].isNegate() && mvar[1][0][0].equals(mvar[0][0])) munc = &mvar[0][0];
				} else if(mvar[0][0].equals(mvar[1][1])) {
					mmid = &mvar[0][0];
					if(mvar[0][1].isNegate() && mvar[0][1][0].equals(mvar[1][0])) munc = &mvar[1][0];
					if(mvar[1][0].isNegate() && mvar[1][0][0].equals(mvar[0][1])) munc = &mvar[0][1];
				} else if(mvar[0][1].equals(mvar[1][0])) {
					mmid = &mvar[0][0];
					if(mvar[0][0].isNegate() && mvar[0][0][0].equals(mvar[1][1])) munc = &mvar[1][1];
					if(mvar[1][1].isNegate() && mvar[1][1][0].equals(mvar[0][0])) munc = &mvar[0][0];
				}
				if(mmid && munc) {
					unc = *munc;
					MathStructure mmid2(*mmid);
					KnownVariable *v = new KnownVariable("", string("(") + format_and_print(*mmid) + ")", mmid2);
					m.set(v);
					v->destroy();
					return v;
				}
			}
			unc = mvar[1];
			unc -= mvar[0];
			unc *= nr_half;
			MathStructure mmid(mvar[0]);
			mmid += mvar[1];
			mmid *= nr_half;
			KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mmid) + ")", mmid);
			m.variable()->ref();
			*prev_v = m.variable();
			m.set(v);
			*mnew = m;
			v->destroy();
			return v;
		} else if(mvar.isFunction() && mvar.function() == CALCULATOR->f_uncertainty && mvar.size() == 3 && mvar[2].isNumber() && !mvar[0].containsInterval(true, true, false, 1, true) && !mvar[1].containsInterval(true, true, false, 1, true)) {
			if(mvar[2].number().getBoolean()) {
				unc = mvar[1];
				unc *= mvar[0];
			} else {
				unc = mvar[1];
			}
			KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mvar[0]) + ")", mvar[0]);
			m.variable()->ref();
			*prev_v = m.variable();
			m.set(v);
			*mnew = m;
			v->destroy();
			return v;
		}
		b_failed = true;
	} else if(m.isNumber() && m.number().isInterval(false) && m.number().precision(true) <= PRECISION + 10) {
		KnownVariable *v = NULL;
		find_interval_create_var(m.number(), m, unc, unc2, &v, v2);
		return v;
	} else if(m.isFunction() && m.function() == CALCULATOR->f_interval && m.size() == 2 && !m[0].containsInterval(true, true, false, 1, true) && !m[1].containsInterval(true, true, false, 1, true)) {
		if(m[0].isAddition() && m[0].size() == 2 && m[1].isAddition() && m[1].size() == 2) {
			MathStructure *mmid = NULL, *munc = NULL;
			if(m[0][0].equals(m[1][0])) {
				mmid = &m[0][0];
				if(m[0][1].isNegate() && m[0][1][0].equals(m[1][1])) munc = &m[1][1];
				if(m[1][1].isNegate() && m[1][1][0].equals(m[0][1])) munc = &m[0][1];
			} else if(m[0][1].equals(m[1][1])) {
				mmid = &m[0][1];
				if(m[0][0].isNegate() && m[0][0][0].equals(m[1][0])) munc = &m[1][0];
				if(m[1][0].isNegate() && m[1][0][0].equals(m[0][0])) munc = &m[0][0];
			} else if(m[0][0].equals(m[1][1])) {
				mmid = &m[0][0];
				if(m[0][1].isNegate() && m[0][1][0].equals(m[1][0])) munc = &m[1][0];
				if(m[1][0].isNegate() && m[1][0][0].equals(m[0][1])) munc = &m[0][1];
			} else if(m[0][1].equals(m[1][0])) {
				mmid = &m[0][0];
				if(m[0][0].isNegate() && m[0][0][0].equals(m[1][1])) munc = &m[1][1];
				if(m[1][1].isNegate() && m[1][1][0].equals(m[0][0])) munc = &m[0][0];
			}
			if(mmid && munc) {
				unc = *munc;
				KnownVariable *v = new KnownVariable("", string("(") + format_and_print(*mmid) + ")", *mmid);
				m.set(v);
				v->destroy();
				return v;
			}
		}
		unc = m[1];
		unc -= m[0];
		unc *= nr_half;
		MathStructure mmid(m[0]);
		mmid += m[1];
		mmid *= nr_half;
		KnownVariable *v = new KnownVariable("", string("(") + format_and_print(mmid) + ")", mmid);
		m.set(v);
		v->destroy();
		return v;
	} else if(m.isFunction() && m.function() == CALCULATOR->f_uncertainty && m.size() == 3 && m[2].isNumber() && !m[0].containsInterval(true, true, false, 1, true) && !m[1].containsInterval(true, true, false, 1, true)) {
		if(m[2].number().getBoolean()) {
			unc = m[1];
			unc *= m[0];
		} else {
			unc = m[1];
		}
		KnownVariable *v = new KnownVariable("", string("(") + format_and_print(m[0]) + ")", m[0]);
		m.set(v);
		v->destroy();
		return v;
	}
	for(size_t i = 0; i < m.size(); i++) {
		KnownVariable *v = find_interval_replace_var(m[i], unc, unc2, v2, eo, mnew, prev_v, b_failed);
		if(b_failed) return NULL;
		if(v) return v;
	}
	return NULL;
}

bool find_interval_replace_var_nr(MathStructure &m) {
	if((m.isNumber() && m.number().isInterval(false) && m.number().precision(true) <= PRECISION + 10) || (m.isFunction() && m.function() == CALCULATOR->f_interval && m.size() == 2) || (m.isFunction() && m.function() == CALCULATOR->f_uncertainty && m.size() == 3)) {
		KnownVariable *v = new KnownVariable("", string("(") + format_and_print(m) + ")", m);
		m.set(v);
		v->destroy();
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(find_interval_replace_var_nr(m[i])) b = true;
	}
	return b;
}


bool replace_variables_with_interval(MathStructure &m, const EvaluationOptions &eo, bool in_nounit = false, bool only_argument_vars = false) {
	if(m.isVariable() && m.variable()->isKnown() && (!only_argument_vars || m.variable()->title() == "\b")) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(!mvar.containsInterval(true, true, false, 1, true)) return false;
		if(mvar.isNumber()) {
			return false;
		} else if(mvar.isMultiplication() && mvar[0].isNumber()) {
			if(mvar[0].number().isInterval(false)) {
				bool b = true;
				for(size_t i = 1; i < mvar.size(); i++) {
					if(mvar[i].containsInterval(true, true, false, 1, true)) {
						b = false;
						break;
					}
				}
				if(b) return false;
			}
		}
		m.set(mvar, true);
		if(in_nounit) m.removeType(STRUCT_UNIT);
		else m.unformat(eo);
		return true;
	}
	bool b = false;
	if(m.isFunction() && m.function() == CALCULATOR->f_stripunits && m.size() == 1) {
		b = replace_variables_with_interval(m[0], eo, true, only_argument_vars);
		if(b && m[0].containsType(STRUCT_UNIT, false, true, true) == 0) {
			m.setToChild(1, true);
		}
		return b;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_variables_with_interval(m[i], eo, in_nounit, only_argument_vars)) b = true;
	}
	return b;
}

bool contains_diff_for(const MathStructure &m, const MathStructure &x_var) {
	if(m.isFunction() && m.function() && m.function() == CALCULATOR->f_diff && m.size() >= 2 && m[1] == x_var) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_diff_for(m[i], x_var)) return true;
	}
	return false;
}

bool sync_approximate_units(MathStructure &m, const EvaluationOptions &feo, vector<KnownVariable*> *vars = NULL, vector<MathStructure> *uncs = NULL, bool do_intervals = true);

MathStructure calculate_uncertainty(MathStructure &m, const EvaluationOptions &eo, bool &b_failed) {
	vector<KnownVariable*> vars;
	vector<MathStructure> uncs;
	MathStructure unc, unc2;
	if(eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) replace_variables_with_interval(m, eo, false, eo.approximation == APPROXIMATION_EXACT);
	while(true) {
		Variable *prev_v = NULL;
		MathStructure mnew;
		KnownVariable *v2 = NULL;
		KnownVariable *v = find_interval_replace_var(m, unc, unc2, &v2, eo, &mnew, &prev_v, b_failed);
		if(!v) break;
		if(!mnew.isZero()) {
			m.replace(prev_v, mnew);
			prev_v->unref();
		}
		vars.push_back(v);
		uncs.push_back(unc);
		if(v2) {
			vars.push_back(v2);
			uncs.push_back(unc2);
		}
	}
	m.unformat(eo);
	if(eo.sync_units && eo.approximation != APPROXIMATION_EXACT) sync_approximate_units(m, eo, &vars, &uncs, true);
	if(b_failed || vars.empty()) return m_zero;
	MathStructure munc;
	UnknownVariable *uv = new UnknownVariable("", "x");
	MathStructure muv(uv);
	MathStructure *munc_i = NULL;
	for(size_t i = 0; i < vars.size(); i++) {
		if(!vars[i]->get().representsNonComplex(true)) {
			b_failed = true; return m_zero;
		}
		MathStructure *mdiff = new MathStructure(m);
		uv->setInterval(vars[i]->get());
		mdiff->replace(vars[i], muv);
		if(!mdiff->differentiate(muv, eo) || contains_diff_for(*mdiff, muv) || CALCULATOR->aborted()) {
			b_failed = true;
			return m_zero;
		}
		mdiff->replace(muv, vars[i]);
		if(!mdiff->representsNonComplex(true)) {
			MathStructure *mdiff_i = new MathStructure(*mdiff);
			mdiff->transform(CALCULATOR->f_re);
			mdiff_i->transform(CALCULATOR->f_im);
			mdiff_i->raise(nr_two);
			mdiff_i->multiply(uncs[i]);
			mdiff_i->last().raise(nr_two);
			if(!munc_i) {munc_i = mdiff_i;}
			else munc_i->add_nocopy(mdiff_i, true);
		}
		mdiff->raise(nr_two);
		mdiff->multiply(uncs[i]);
		mdiff->last().raise(nr_two);
		if(munc.isZero()) {munc.set_nocopy(*mdiff); mdiff->unref();}
		else munc.add_nocopy(mdiff, true);
	}
	uv->destroy();
	munc.raise(nr_half);
	if(munc_i) {
		munc_i->raise(nr_half);
		munc_i->multiply(nr_one_i);
		munc.add_nocopy(munc_i);
	}
	return munc;
}

Variable *find_interval_replace_var_comp(MathStructure &m, const EvaluationOptions &eo, Variable **v) {
	if(eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables && m.isVariable() && m.variable()->isKnown() && ((KnownVariable*) m.variable())->get().containsInterval(true, true, false, 1, true)) {
		UnknownVariable *uv = new UnknownVariable("", format_and_print(m));
		uv->setInterval(m);
		*v = m.variable();
		m.set(uv, true);
		return uv;
	} else if((m.isNumber() && m.number().isInterval(false) && m.number().precision(true) <= PRECISION + 10) || (m.isFunction() && m.function() == CALCULATOR->f_interval && m.size() == 2) || (m.isFunction() && m.function() == CALCULATOR->f_uncertainty && m.size() == 3)) {
		Variable *uv = NULL;
		if(eo.approximation == APPROXIMATION_EXACT || eo.approximation == APPROXIMATION_EXACT_VARIABLES) {
			uv = new KnownVariable("", string("(") + format_and_print(m) + ")", m);
		} else {
			uv = new UnknownVariable("", string("(") + format_and_print(m) + ")");
			((UnknownVariable*) uv)->setInterval(m);
		}
		*v = NULL;
		m.set(uv, true);
		return uv;
	}
	for(size_t i = 0; i < m.size(); i++) {
		Variable *uv = find_interval_replace_var_comp(m[i], eo, v);
		if(uv) return uv;
	}
	return NULL;
}

bool eval_comparison_sides(MathStructure &m, const EvaluationOptions &eo) {
	if(m.isComparison()) {
		MathStructure mbak(m);
		if(!m[0].isUnknown()) {
			bool ret = true;
			CALCULATOR->beginTemporaryStopMessages();
			m[0].eval(eo);
			if(m[0].containsFunction(CALCULATOR->f_uncertainty) && !mbak[0].containsFunction(CALCULATOR->f_uncertainty)) {
				CALCULATOR->endTemporaryStopMessages();
				m[0] = mbak[0];
				ret = false;
			} else {
				CALCULATOR->endTemporaryStopMessages(true);
			}
			CALCULATOR->beginTemporaryStopMessages();
			m[1].eval(eo);
			if(m[1].containsFunction(CALCULATOR->f_uncertainty) && !mbak[1].containsFunction(CALCULATOR->f_uncertainty)) {
				CALCULATOR->endTemporaryStopMessages();
				m[1] = mbak[1];
				ret = false;
			} else {
				CALCULATOR->endTemporaryStopMessages(true);
			}
			if(ret && !m.containsUnknowns()) {
				m.calculatesub(eo, eo, false);
				return true;
			}
			return false;
		} else {
			m[1].eval(eo);
			m.calculatesub(eo, eo, false);
			return true;
		}
	} else if(m.containsType(STRUCT_COMPARISON)) {
		bool ret = true;
		for(size_t i = 0; i < m.size(); i++) {
			if(!eval_comparison_sides(m[i], eo)) ret = false;
		}
		m.childrenUpdated();
		m.calculatesub(eo, eo, false);
		return ret;
	} else {
		m.eval(eo);
	}
	return true;
}

bool separate_unit_vars(MathStructure &m, const EvaluationOptions &eo, bool only_approximate, bool dry_run) {
	if(m.isVariable() && m.variable()->isKnown()) {
		const MathStructure &mvar = ((KnownVariable*) m.variable())->get();
		if(mvar.isMultiplication()) {
			bool b = false;
			for(size_t i = 0; i < mvar.size(); i++) {
				if(is_unit_multiexp(mvar[i])) {
					if(!b) b = !only_approximate || contains_approximate_relation_to_base(mvar[i], true);
				} else if(mvar[i].containsType(STRUCT_UNIT, false, true, true) != 0) {
					b = false;
					break;
				}
			}
			if(!b) return false;
			if(dry_run) return true;
			m.transform(CALCULATOR->f_stripunits);
			for(size_t i = 0; i < mvar.size(); i++) {
				if(is_unit_multiexp(mvar[i])) {
					m.multiply(mvar[i], i);
				}
			}
			m.unformat(eo);
			return true;
		}
	}
	if(m.isFunction() && m.function() == CALCULATOR->f_stripunits) return false;
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(separate_unit_vars(m[i], eo, only_approximate, dry_run)) {
			b = true;
			if(dry_run) return true;
		}
	}
	return b;
}

bool convert_to_default_angle_unit(MathStructure &m, const EvaluationOptions &eo) {
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(convert_to_default_angle_unit(m[i], eo)) b = true;
		if(m.isFunction() && m.function()->getArgumentDefinition(i + 1) && m.function()->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_ANGLE) {
			Unit *u = NULL;
			if(eo.parse_options.angle_unit == ANGLE_UNIT_DEGREES) u = CALCULATOR->getDegUnit();
			else if(eo.parse_options.angle_unit == ANGLE_UNIT_GRADIANS) u = CALCULATOR->getGraUnit();
			if(u && m[i].contains(CALCULATOR->getRadUnit(), false, false, false)) {
				m[i].divide(u);
				m[i].multiply(u);
				EvaluationOptions eo2 = eo;
				if(eo.approximation == APPROXIMATION_TRY_EXACT) eo2.approximation = APPROXIMATION_APPROXIMATE;
				eo2.calculate_functions = false;
				eo2.sync_units = true;
				m[i].calculatesub(eo2, eo2, true);
				b = true;
			}
		}
	}
	return b;
}

bool remove_add_zero_unit(MathStructure &m) {
	if(m.isAddition() && m.size() > 1) {
		bool b = false, b2 = false;
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isMultiplication() && m[i].size() > 1 && m[i][0].isZero() && !m[i].isApproximate()) {
				b = true;
			} else {
				b2 = true;
			}
			if(b && b2) break;
		}
		if(!b || !b2) return false;
		b = false;
		for(size_t i = 0; i < m.size();) {
			b2 = false;
			if(m[i].isMultiplication() && m[i].size() > 1 && m[i][0].isZero() && !m[i].isApproximate()) {
				b2 = true;
				for(size_t i2 = 1; i2 < m[i].size(); i2++) {
					if(!m[i][i2].isUnit_exp() || (m[i][i2].isPower() && m[i][i2][0].unit()->hasNonlinearRelationToBase()) || (m[i][i2].isUnit() && m[i][i2].unit()->hasNonlinearRelationToBase())) {
						b2 = false;
						break;
					}
				}
				if(b2) {
					b = true;
					m.delChild(i + 1);
					if(m.size() == 1) {
						m.setToChild(1, true);
						break;
					}
				}
			}
			if(!b2) i++;
		}
		return b;
	}
	return false;
}

bool contains_duplicate_interval_variables_eq(const MathStructure &mstruct, const MathStructure &xvar, vector<KnownVariable*> &vars) {
	if(mstruct.isVariable() && mstruct.variable()->isKnown() && ((KnownVariable*) mstruct.variable())->get().containsInterval(false, true, false, false)) {
		KnownVariable *v = (KnownVariable*) mstruct.variable();
		for(size_t i = 0; i < vars.size(); i++) {
			if(vars[i] == v) {
				return true;
			}
		}
		vars.push_back(v);
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_duplicate_interval_variables_eq(mstruct[i], xvar, vars)) return true;
	}
	return false;
}

bool fix_eqs(MathStructure &m, const EvaluationOptions &eo) {
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_eqs(m[i], eo)) m.childUpdated(i + 1);
	}
	if(m.isComparison()) {
		if(CALCULATOR->aborted()) return false;
		const MathStructure *x_var;
		if(eo.isolate_var && m.contains(*eo.isolate_var)) x_var = eo.isolate_var;
		else x_var = &m.find_x_var();
		if(!x_var->isUndefined()) {
			vector<KnownVariable*> vars;
			if(contains_duplicate_interval_variables_eq(m, *x_var, vars)) {
				if(!m[0].contains(*x_var)) {
					m.swapChildren(1, 2);
				} else if(m[0].isAddition()) {
					for(size_t i = 0; i < m[0].size();) {
						if(!m[0][i].contains(*x_var)) {
							m[0][i].calculateNegate(eo);
							m[0][i].ref();
							m[1].add_nocopy(&m[0][i], true);
							m[1].calculateAddLast(eo);
							m[0].delChild(i + 1);
						} else {
							i++;
						}
					}
					if(m[0].size() == 1) m[0].setToChild(1, true);
					else if(m[0].size() == 0) m[0].clear(true);
					m.childrenUpdated();
				}
				if(m[1].isAddition()) {
					for(size_t i = 0; i < m[1].size();) {
						if(m[1][i].contains(*x_var)) {
							m[1][i].calculateNegate(eo);
							m[1][i].ref();
							m[0].add_nocopy(&m[1][i], true);
							m[0].calculateAddLast(eo);
							m[1].delChild(i + 1);
						} else {
							i++;
						}
					}
					if(m[1].size() == 1) m[1].setToChild(1, true);
					else if(m[1].size() == 0) m[1].clear(true);
					m.childrenUpdated();
				} else if(m[1].contains(*x_var)) {
					m[0].calculateSubtract(m[1], eo);
					m[1].clear(true);
				}
				vars.clear();
				if(m[0].containsType(STRUCT_ADDITION) && contains_duplicate_interval_variables_eq(m[0], *x_var, vars)) m[0].factorize(eo, false, false, 0, false, 1, NULL, m_undefined, false, false, 3);
				return true;
			}
		}
	}
	return false;
}

Unit *find_log_unit(const MathStructure &m, bool toplevel = true) {
	if(!toplevel && m.isUnit() && m.unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) m.unit())->hasNonlinearExpression() && (((AliasUnit*) m.unit())->expression().find("log") != string::npos || ((AliasUnit*) m.unit())->inverseExpression().find("log") != string::npos || ((AliasUnit*) m.unit())->expression().find("ln") != string::npos || ((AliasUnit*) m.unit())->inverseExpression().find("ln") != string::npos)) {
		return ((AliasUnit*) m.unit())->firstBaseUnit();
	}
	if(m.isMultiplication() && toplevel && m.last().isUnit()) {
		Unit *u = find_log_unit(m.last(), false);
		if(u) {
			for(size_t i = 0; i < m.size(); i++) {
				if(m[i].containsType(STRUCT_UNIT, true)) return u;
			}
			return NULL;
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		Unit *u = find_log_unit(m[i], false);
		if(u) return u;
	}
	return NULL;
}
void convert_log_units(MathStructure &m, const EvaluationOptions &eo) {
	while(true) {
		Unit *u = find_log_unit(m);
		if(!u) break;
		CALCULATOR->error(false, "Log-based units were converted before calculation.", NULL);
		m.convert(u, true, NULL, false, eo);
	}
}
bool warn_ratio_units(MathStructure &m, bool top_level = true) {
	if(!top_level && m.isUnit() && ((m.unit()->subtype() == SUBTYPE_BASE_UNIT && m.unit()->referenceName() == "Np") || (m.unit()->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) m.unit())->baseUnit()->referenceName() == "Np"))) {
		CALCULATOR->error(true, "Logarithmic ratio units is treated as other units and the result might not be as expected.", NULL);
		return true;
	}
	if(m.isMultiplication() && top_level && m.last().isUnit()) {
		if(m.size() < 2) return false;
		for(size_t i = 0; i < m.size() - 1; i++) {
			if(warn_ratio_units(m[i], false)) return true;
		}
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			if(warn_ratio_units(m[i], false)) return true;
		}
	}
	return false;
}
int contains_interval_var(const MathStructure &m, bool structural_only, bool check_variables, bool check_functions, int ignore_high_precision_interval, bool include_interval_function);
int contains_function_interval(const MathStructure &m, bool structural_only = true, bool check_variables = false, bool check_functions = false, int ignore_high_precision_interval = 0, bool include_interval_function = false) {
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_function_interval(m[i], structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function)) return true;
	}
	if(m.isVariable() && m.variable()->isKnown() && m.variable()->title() == "\b") {
		if(ignore_high_precision_interval == 0) return true;
		return contains_interval_var(((KnownVariable*) m.variable())->get(), structural_only, check_variables, check_functions, ignore_high_precision_interval, include_interval_function);
	}
	return false;
}
bool replace_function_vars(MathStructure &m) {
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_function_vars(m[i])) return true;
	}
	if(m.isVariable() && m.variable()->isKnown() && m.variable()->title() == "\b") {
		m.set(((KnownVariable*) m.variable())->get(), true);
	}
	return false;
}
MathStructure &MathStructure::eval(const EvaluationOptions &eo) {

	if(m_type == STRUCT_NUMBER) {
		if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);
		return *this;
	}

	if(eo.structuring != STRUCTURING_NONE) warn_ratio_units(*this);

	unformat(eo);

	if(m_type == STRUCT_UNDEFINED || m_type == STRUCT_ABORTED || m_type == STRUCT_DATETIME || m_type == STRUCT_UNIT || m_type == STRUCT_SYMBOLIC || (m_type == STRUCT_VARIABLE && !o_variable->isKnown())) return *this;

	if(eo.structuring != STRUCTURING_NONE && eo.sync_units) convert_log_units(*this, eo);

	EvaluationOptions feo = eo;
	feo.structuring = STRUCTURING_NONE;
	feo.do_polynomial_division = false;
	feo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	EvaluationOptions eo2 = eo;
	eo2.structuring = STRUCTURING_NONE;
	eo2.expand = false;
	eo2.test_comparisons = false;
	eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	eo2.isolate_x = false;

	if(m_type == STRUCT_NUMBER) {
		if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);
		return *this;
	}

	if(eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
		if(eo.calculate_functions) calculate_nondifferentiable_functions(*this, feo, true, true, 0);
		if(((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) && containsInterval(true, true, false, 1, true)) || (eo.sync_units && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.approximation != APPROXIMATION_EXACT && sync_approximate_units(*this, eo)) || (eo.approximation == APPROXIMATION_EXACT && contains_function_interval(*this, true, true, false, 1, true))) {
			EvaluationOptions eo3 = eo2;
			eo3.split_squares = false;
			eo3.assume_denominators_nonzero = false;
			if(eo.approximation == APPROXIMATION_APPROXIMATE && !containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
			else eo3.approximation = APPROXIMATION_EXACT;
			vector<KnownVariable*> vars;
			vector<MathStructure> uncs;
			calculatesub(eo3, eo3);
			while(eo.sync_units && (separate_unit_vars(*this, feo, true) || sync_approximate_units(*this, feo, &vars, &uncs, false))) {
				calculatesub(eo3, eo3);
			}
			eo3.approximation = APPROXIMATION_APPROXIMATE;
			if(eo.sync_units) {
				sync_approximate_units(*this, feo, &vars, &uncs, true);
			}
			factorize_variables(*this, eo3);
			if(eo.approximation == APPROXIMATION_APPROXIMATE && !containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
			else eo3.approximation = APPROXIMATION_EXACT;
			eo3.expand = eo.expand;
			eo3.assume_denominators_nonzero = eo.assume_denominators_nonzero;
			solve_intervals(*this, eo3, feo);
		}
		if(eo.calculate_functions) calculate_differentiable_functions(*this, feo);
	} else if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA) {
		if(eo.calculate_functions) calculate_nondifferentiable_functions(*this, feo, true, true, -1);
		if(!isNumber() && (((eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) && containsInterval(true, true, false, 1, true)) || containsInterval(true, false, false, 1, true) || (eo.sync_units && eo.approximation != APPROXIMATION_EXACT && sync_approximate_units(*this, eo)) || (eo.approximation == APPROXIMATION_EXACT && contains_function_interval(*this, true, true, false, 1, true)))) {
			if(eo.calculate_functions) calculate_nondifferentiable_functions(*this, feo, true, true, (eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) ? 2 : 1);
			MathStructure munc, mbak(*this);
			if(eo.approximation != APPROXIMATION_EXACT && eo.approximation != APPROXIMATION_EXACT_VARIABLES && eo.calculate_variables) {
				find_interval_replace_var_nr(*this);
				EvaluationOptions eo3 = eo2;
				eo3.split_squares = false;
				if(eo.expand && eo.expand >= -1) eo3.expand = -1;
				eo3.assume_denominators_nonzero = eo.assume_denominators_nonzero;
				eo3.approximation = APPROXIMATION_EXACT;
				vector<KnownVariable*> vars;
				vector<MathStructure> uncs;
				calculatesub(eo3, eo3);
				while(eo.sync_units && (separate_unit_vars(*this, feo, true) || sync_approximate_units(*this, feo, &vars, &uncs, false))) {
					calculatesub(eo3, eo3);
				}
			}
			bool b_failed = false;
			if(containsType(STRUCT_COMPARISON)) {
				EvaluationOptions eo3 = eo;
				eo3.approximation = APPROXIMATION_EXACT;
				eo3.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
				eo3.structuring = STRUCTURING_NONE;
				eo3.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
				vector<Variable*> vars;
				while(true) {
					Variable *v = NULL;
					Variable *uv = find_interval_replace_var_comp(*this, eo3, &v);
					if(!uv) break;
					if(v) replace(v, uv);
					vars.push_back(uv);
				}
				eval(eo3);
				for(size_t i = 0; i < vars.size(); i++) {
					if(vars[i]->isKnown()) replace(vars[i], ((KnownVariable*) vars[i])->get());
					else replace(vars[i], ((UnknownVariable*) vars[i])->interval());
					vars[i]->destroy();
				}
				if(CALCULATOR->aborted()) return *this;
				if(eval_comparison_sides(*this, feo)) {
					if(eo.structuring != STRUCTURING_NONE) simplify_ln(*this);
					structure(eo.structuring, eo2, false);
					if(eo.structuring != STRUCTURING_NONE) simplify_ln(*this);
					clean_multiplications(*this);
				} else if(!CALCULATOR->aborted()) {
					CALCULATOR->error(false, _("Calculation of uncertainty propagation partially failed (using interval arithmetic instead when necessary)."), NULL);
					EvaluationOptions eo4 = eo;
					eo4.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
					eval(eo4);
				}
				return *this;
			} else {
				CALCULATOR->beginTemporaryStopMessages();
				munc = calculate_uncertainty(*this, eo, b_failed);
				if(!b_failed && !munc.isZero()) {
					EvaluationOptions eo3 = eo;
					eo3.keep_zero_units = false;
					eo3.structuring = STRUCTURING_NONE;
					eo3.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					eo3.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
					if(eo3.approximation == APPROXIMATION_TRY_EXACT) eo3.approximation = APPROXIMATION_APPROXIMATE;

					munc.eval(eo3);
					eo3.keep_zero_units = eo.keep_zero_units;
					eval(eo3);

					if(eo.keep_zero_units) remove_add_zero_unit(*this);
					b_failed = true;
					if(munc.isFunction() && munc.function() == CALCULATOR->f_abs && munc.size() == 1) {
						munc.setToChild(1);
					}
					bool one_prepended = false;
					test_munc:
					if(munc.isNumber()) {
						if(munc.isZero()) {
							CALCULATOR->endTemporaryStopMessages(true);
							return *this;
						} else if(isNumber()) {
							o_number.setUncertainty(munc.number());
							numberUpdated();
							CALCULATOR->endTemporaryStopMessages(true);
							return *this;
						} else if(isAddition()) {
							for(size_t i = 0; i < SIZE; i++) {
								if(CHILD(i).isNumber()) {
									b_failed = false;
									CHILD(i).number().setUncertainty(munc.number());
									CHILD(i).numberUpdated();
									CHILD_UPDATED(i);
									break;
								}
							}
						}
					} else {
						if(munc.isMultiplication()) {
							if(!munc[0].isNumber()) {
								munc.insertChild(m_one, 1);
								one_prepended = true;
							}
						} else {
							munc.transform(STRUCT_MULTIPLICATION);
							munc.insertChild(m_one, 1);
							one_prepended = true;
						}
						if(munc.isMultiplication()) {
							if(munc.size() == 2) {
								if(isMultiplication() && CHILD(0).isNumber() && (munc[1] == CHILD(1) || (munc[1].isFunction() && munc[1].function() == CALCULATOR->f_abs && munc[1].size() == 1 && CHILD(1) == munc[1][0]))) {
									CHILD(0).number().setUncertainty(munc[0].number());
									CHILD(0).numberUpdated();
									CHILD_UPDATED(0)
									b_failed = false;
								} else if(equals(munc[1]) || (munc[1].isFunction() && munc[1].function() == CALCULATOR->f_abs && munc[1].size() == 1 && equals(munc[1][0]))) {
									transform(STRUCT_MULTIPLICATION);
									PREPEND(m_one);
									CHILD(0).number().setUncertainty(munc[0].number());
									CHILD(0).numberUpdated();
									CHILD_UPDATED(0)
									b_failed = false;
								}
							} else if(isMultiplication()) {
								size_t i2 = 0;
								if(CHILD(0).isNumber()) i2++;
								if(SIZE + 1 - i2 == munc.size()) {
									bool b = true;
									for(size_t i = 1; i < munc.size(); i++, i2++) {
										if(!munc[i].equals(CHILD(i2)) && !(munc[i].isFunction() && munc[i].function() == CALCULATOR->f_abs && munc[i].size() == 1 && CHILD(i2) == munc[i][0])) {
											b = false;
											break;
										}
									}
									if(b) {
										if(!CHILD(0).isNumber()) {
											PREPEND(m_one);
										}
										CHILD(0).number().setUncertainty(munc[0].number());
										CHILD(0).numberUpdated();
										CHILD_UPDATED(0)
										b_failed = false;
									}
								}
							}
							if(b_failed) {
								bool b = false;
								for(size_t i = 0; i < munc.size(); i++) {
									if(munc[i].isFunction() && munc[i].function() == CALCULATOR->f_abs && munc[i].size() == 1) {
										munc[i].setToChild(1);
										b = true;
									}
								}
								if(b) {
									munc.eval(eo3);
									goto test_munc;
								}
							}
						}
					}
					if(b_failed && munc.countTotalChildren(false) < 50) {
						if(one_prepended && munc.isMultiplication() && munc[0].isOne()) munc.delChild(1, true);
						if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_ln(munc);}
						structure(eo.structuring, eo2, false);
						munc.structure(eo.structuring, eo2, false);
						if(eo.structuring != STRUCTURING_NONE) {simplify_ln(*this); simplify_ln(munc);}
						clean_multiplications(*this);
						clean_multiplications(munc);
						transform(CALCULATOR->f_uncertainty);
						addChild(munc);
						addChild(m_zero);
						CALCULATOR->endTemporaryStopMessages(true);
						return *this;
					}
					if(!b_failed) {
						CALCULATOR->endTemporaryStopMessages(true);
						if(eo.structuring != STRUCTURING_NONE) simplify_ln(*this);
						structure(eo.structuring, eo2, false);
						if(eo.structuring != STRUCTURING_NONE) simplify_ln(*this);
						clean_multiplications(*this);
						return *this;
					}
				}
				CALCULATOR->endTemporaryStopMessages(!b_failed);
				if(b_failed) {
					set(mbak);
					if(CALCULATOR->aborted()) return *this;
					CALCULATOR->error(false, _("Calculation of uncertainty propagation failed (using interval arithmetic instead)."), NULL);
					EvaluationOptions eo3 = eo;
					eo3.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
					eval(eo3);
					return *this;
				}
			}
		}
		if(eo.calculate_functions) calculate_differentiable_functions(*this, feo);
	} else if(eo.calculate_functions) {
		calculateFunctions(feo);
	}

	if(m_type == STRUCT_NUMBER) {
		if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);
		return *this;
	}

	if(eo2.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC || eo2.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA) eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;

	if(eo2.approximation == APPROXIMATION_TRY_EXACT || (eo2.approximation == APPROXIMATION_APPROXIMATE && (containsUnknowns() || containsInterval(false, true, false, 0)))) {
		EvaluationOptions eo3 = eo2;
		if(eo.approximation == APPROXIMATION_APPROXIMATE && !containsUnknowns()) eo3.approximation = APPROXIMATION_EXACT_VARIABLES;
		else eo3.approximation = APPROXIMATION_EXACT;
		eo3.split_squares = false;
		eo3.assume_denominators_nonzero = false;
		calculatesub(eo3, feo);
		if(m_type == STRUCT_NUMBER) {
			if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo);
			else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo);
			else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);
			return *this;
		}
		if(eo.interval_calculation < INTERVAL_CALCULATION_INTERVAL_ARITHMETIC && eo.expand && eo.expand >= -1 && !containsType(STRUCT_COMPARISON, true, true, true)) {
			unformat(eo);
			eo3.expand = -1;
			calculatesub(eo3, feo);
		}
		eo3.approximation = APPROXIMATION_APPROXIMATE;
		if(eo2.interval_calculation >= INTERVAL_CALCULATION_INTERVAL_ARITHMETIC && containsInterval(false, true, false, -1)) factorize_variables(*this, eo3);
		if(containsType(STRUCT_COMPARISON) && containsInterval(false, true, false, false)) {
			eo3.approximation = APPROXIMATION_EXACT;
			fix_eqs(*this, eo3);
		}
		eo2.approximation = APPROXIMATION_APPROXIMATE;
	} else if(eo2.approximation == APPROXIMATION_EXACT && contains_function_interval(*this, false, true, false, -1)) {
		EvaluationOptions eo3 = eo2;
		eo3.split_squares = false;
		eo3.assume_denominators_nonzero = false;
		calculatesub(eo3, feo);
		eo3.approximation = APPROXIMATION_APPROXIMATE;
		if(eo2.interval_calculation >= INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) factorize_variables(*this, eo3, true);
	}
	if(eo2.approximation == APPROXIMATION_EXACT) replace_function_vars(*this);

	calculatesub(eo2, feo);

	if(m_type == STRUCT_NUMBER) {
		if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo);
		else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);
		return *this;
	}
	if(m_type == STRUCT_UNDEFINED || m_type == STRUCT_ABORTED || m_type == STRUCT_DATETIME || m_type == STRUCT_UNIT || m_type == STRUCT_SYMBOLIC || (m_type == STRUCT_VARIABLE && !o_variable->isKnown())) return *this;
	if(CALCULATOR->aborted()) return *this;

	eo2.sync_units = false;
	eo2.isolate_x = eo.isolate_x;

	if(eo2.isolate_x) {
		eo2.assume_denominators_nonzero = false;
		if(isolate_x(eo2, feo)) {
			if(CALCULATOR->aborted()) return *this;
			if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
			calculatesub(eo2, feo);
		} else {
			if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
		}
		if(CALCULATOR->aborted()) return *this;
	}

	if(eo.expand != 0 || (eo.test_comparisons && containsType(STRUCT_COMPARISON))) {
		eo2.test_comparisons = eo.test_comparisons;
		eo2.expand = eo.expand;
		if(eo2.expand && (!eo.test_comparisons || !containsType(STRUCT_COMPARISON))) eo2.expand = -2;
		bool b = eo2.test_comparisons;
		if(!b && isAddition()) {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).containsType(STRUCT_ADDITION, false) == 1) {
					b = true;
					break;
				}
			}
		} else if(!b) {
			b = containsType(STRUCT_ADDITION, false) == 1;
		}
		if(b) {
			calculatesub(eo2, feo);
			if(CALCULATOR->aborted()) return *this;
			if(eo.do_polynomial_division) do_simplification(*this, eo2, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
			if(CALCULATOR->aborted()) return *this;
			if(eo2.isolate_x) {
				eo2.assume_denominators_nonzero = false;
				if(isolate_x(eo2, feo)) {
					if(CALCULATOR->aborted()) return *this;
					if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
					calculatesub(eo2, feo);
					if(containsType(STRUCT_ADDITION, false) == 1 && eo.do_polynomial_division) do_simplification(*this, eo2, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
				} else {
					if(eo.assume_denominators_nonzero) eo2.assume_denominators_nonzero = 2;
				}
				if(CALCULATOR->aborted()) return *this;
			}
		}
	}
	if(eo2.isolate_x && containsType(STRUCT_COMPARISON) && eo2.assume_denominators_nonzero) {
		eo2.assume_denominators_nonzero = 2;
		if(try_isolate_x(*this, eo2, feo)) {
			if(CALCULATOR->aborted()) return *this;
			calculatesub(eo2, feo);
			if(containsType(STRUCT_ADDITION, false) == 1 && eo.do_polynomial_division) do_simplification(*this, eo2, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
		}
	}

	simplify_functions(*this, eo2, feo);

	if(CALCULATOR->aborted()) return *this;

	if(eo.structuring != STRUCTURING_NONE) {

		if(eo.parse_options.angle_unit == ANGLE_UNIT_GRADIANS || eo.parse_options.angle_unit == ANGLE_UNIT_DEGREES) convert_to_default_angle_unit(*this, eo);
		simplify_ln(*this);

		if(eo.keep_zero_units) remove_add_zero_unit(*this);

		structure(eo.structuring, eo2, false);

		simplify_ln(*this);

	}

	clean_multiplications(*this);

	if(eo.complex_number_form == COMPLEX_NUMBER_FORM_EXPONENTIAL) complexToExponentialForm(eo);
	else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) complexToPolarForm(eo);
	else if(eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) complexToCisForm(eo);

	return *this;
}

bool factorize_fix_root_power(MathStructure &m) {
	if(!m[0].isFunction() || m[0].function() != CALCULATOR->f_root || !VALID_ROOT(m[0])) return false;
	if(m[1].isNumber() && m[1].number().isInteger() && !m[1].number().isMinusOne()) {
		if(m[1] == m[0][1]) {
			// root(x, a)^a=x
			m.setToChild(1, true);
			m.setToChild(1, true);
			return true;
		} else if(m[1].number().isIntegerDivisible(m[0][1].number())) {
			// root(x, a)^(2a)=x^2
			if(m[1].number().divide(m[0][1].number())) {
				m[0].setToChild(1, true);
				return true;
			}
		} else if(m[0][1].number().isIntegerDivisible(m[1].number())) {
			// root(x, 3a)^(a)=root(x, 3)
			if(m[0][1].number().divide(m[1].number())) {
				m.setToChild(1, true);
				m.childUpdated(2);
				return true;
			}
		}
	}
	return false;
}

bool factorize_find_multiplier(const MathStructure &mstruct, MathStructure &mnew, MathStructure &factor_mstruct, bool only_units) {
	factor_mstruct.set(m_one);
	switch(mstruct.type()) {
		case STRUCT_ADDITION: {
			if(!only_units) {
				bool bfrac = false, bint = true;
				idm1(mstruct, bfrac, bint);
				if(bfrac || bint) {
					Number gcd(1, 1);
					idm2(mstruct, bfrac, bint, gcd);
					if((bint || bfrac) && !gcd.isOne()) {
						if(bfrac) gcd.recip();
						factor_mstruct.set(gcd);
					}
				}
			}
			if(mstruct.size() > 0) {
				size_t i = 0;
				const MathStructure *cur_mstruct;
				while(true) {
					if(mstruct[0].isMultiplication()) {
						if(i >= mstruct[0].size()) {
							break;
						}
						cur_mstruct = &mstruct[0][i];
					} else {
						cur_mstruct = &mstruct[0];
					}
					if(!cur_mstruct->containsInterval(true) && !cur_mstruct->isNumber() && (!only_units || cur_mstruct->isUnit_exp())) {
						const MathStructure *exp = NULL;
						const MathStructure *bas;
						if(cur_mstruct->isPower() && IS_REAL((*cur_mstruct)[1]) && !(*cur_mstruct)[0].isNumber()) {
							exp = cur_mstruct->exponent();
							bas = cur_mstruct->base();
						} else {
							bas = cur_mstruct;
						}
						bool b = true;
						for(size_t i2 = 1; i2 < mstruct.size(); i2++) {
							b = false;
							size_t i3 = 0;
							const MathStructure *cmp_mstruct;
							while(true) {
								if(mstruct[i2].isMultiplication()) {
									if(i3 >= mstruct[i2].size()) {
										break;
									}
									cmp_mstruct = &mstruct[i2][i3];
								} else {
									cmp_mstruct = &mstruct[i2];
								}
								if(cmp_mstruct->equals(*bas)) {
									if(exp) {
										exp = NULL;
									}
									b = true;
									break;
								} else if(cmp_mstruct->isPower() && IS_REAL((*cmp_mstruct)[1]) && cmp_mstruct->base()->equals(*bas)) {
									if(exp) {
										if(cmp_mstruct->exponent()->number().isLessThan(exp->number())) {
											exp = cmp_mstruct->exponent();
										}
										b = true;
										break;
									} else {
										b = true;
										break;
									}
								}
								if(!mstruct[i2].isMultiplication()) {
									break;
								}
								i3++;
							}
							if(!b) break;
						}
						if(b) {
							b = !factor_mstruct.isOne();
							if(exp) {
								MathStructure *mstruct = new MathStructure(*bas);
								mstruct->raise(*exp);
								if(factor_mstruct.isOne()) {
									factor_mstruct.set_nocopy(*mstruct);
									mstruct->unref();
								} else {
									factor_mstruct.multiply_nocopy(mstruct, true);
								}
							} else {
								if(factor_mstruct.isOne()) factor_mstruct.set(*bas);
								else factor_mstruct.multiply(*bas, true);
							}
							if(b) {
								size_t i3 = 0;
								const MathStructure *cmp_mstruct;
								b = false;
								while(true) {
									if(i3 >= factor_mstruct.size() - 1) {
										break;
									}
									cmp_mstruct = &factor_mstruct[i3];
									if(cmp_mstruct->equals(factor_mstruct.last())) {
										if(exp) {
											exp = NULL;
										}
										b = true;
										break;
									} else if(cmp_mstruct->isPower() && IS_REAL((*cmp_mstruct)[1]) && cmp_mstruct->base()->equals(factor_mstruct.last())) {
										if(exp) {
											if(cmp_mstruct->exponent()->number().isLessThan(exp->number())) {
												exp = cmp_mstruct->exponent();
											}
											b = true;
											break;
										} else {
											b = true;
											break;
										}
									}
									i3++;
								}
								if(b) factor_mstruct.delChild(factor_mstruct.size(), true);
							}
						}
					}
					if(!mstruct[0].isMultiplication()) {
						break;
					}
					i++;
				}
			}
			if(!factor_mstruct.isOne()) {
				if(&mstruct != &mnew) mnew.set(mstruct);
				MathStructure *mfactor;
				size_t i = 0;
				while(true) {
					if(factor_mstruct.isMultiplication()) {
						if(i >= factor_mstruct.size()) break;
						mfactor = &factor_mstruct[i];
					} else {
						mfactor = &factor_mstruct;
					}
					for(size_t i2 = 0; i2 < mnew.size(); i2++) {
						switch(mnew[i2].type()) {
							case STRUCT_NUMBER: {
								if(mfactor->isNumber()) {
									mnew[i2].number() /= mfactor->number();
								}
								break;
							}
							case STRUCT_POWER: {
								if(!IS_REAL(mnew[i2][1])) {
									if(mfactor->isNumber()) {
										mnew[i2].transform(STRUCT_MULTIPLICATION);
										mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
										mnew[i2][0].number() /= mfactor->number();
									} else {
										mnew[i2].set(m_one);
									}
								} else if(mfactor->isNumber()) {
									mnew[i2].transform(STRUCT_MULTIPLICATION);
									mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
									mnew[i2][0].number() /= mfactor->number();
								} else if(mfactor->isPower() && IS_REAL((*mfactor)[1])) {
									if(mfactor->equals(mnew[i2])) {
										mnew[i2].set(m_one);
									} else {
										mnew[i2][1].number() -= mfactor->exponent()->number();
										if(mnew[i2][1].number().isOne()) {
											mnew[i2].setToChild(1, true);
										} else if(factorize_fix_root_power(mnew[i2])) {
											mnew.childUpdated(i2 + 1);
										}
									}
								} else {
									mnew[i2][1].number() -= 1;
									if(mnew[i2][1].number().isOne()) {
										mnew[i2].setToChild(1);
									} else if(mnew[i2][1].number().isZero()) {
										mnew[i2].set(m_one);
									} else if(factorize_fix_root_power(mnew[i2])) {
										mnew.childUpdated(i2 + 1);
									}
								}
								break;
							}
							case STRUCT_MULTIPLICATION: {
								bool b = true;
								if(mfactor->isNumber() && (mnew[i2].size() < 1 || !mnew[i2][0].isNumber())) {
									mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
								}
								for(size_t i3 = 0; i3 < mnew[i2].size() && b; i3++) {
									switch(mnew[i2][i3].type()) {
										case STRUCT_NUMBER: {
											if(mfactor->isNumber()) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3].number() /= mfactor->number();
												}
												b = false;
											}
											break;
										}
										case STRUCT_POWER: {
											if(!IS_REAL(mnew[i2][i3][1])) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
													b = false;
												}
											} else if(mfactor->isPower() && IS_REAL((*mfactor)[1]) && mfactor->base()->equals(mnew[i2][i3][0])) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3][1].number() -= mfactor->exponent()->number();
													if(mnew[i2][i3][1].number().isOne()) {
														MathStructure mstruct2(mnew[i2][i3][0]);
														mnew[i2][i3] = mstruct2;
													} else if(mnew[i2][i3][1].number().isZero()) {
														mnew[i2].delChild(i3 + 1);
													} else if(factorize_fix_root_power(mnew[i2][i3])) {
														mnew[i2].childUpdated(i3 + 1);
														mnew.childUpdated(i2 + 1);
													}
												}
												b = false;
											} else if(mfactor->equals(mnew[i2][i3][0])) {
												if(mnew[i2][i3][1].number() == 2) {
													MathStructure mstruct2(mnew[i2][i3][0]);
													mnew[i2][i3] = mstruct2;
												} else if(mnew[i2][i3][1].number().isOne()) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3][1].number() -= 1;
													if(factorize_fix_root_power(mnew[i2][i3])) {
														mnew[i2].childUpdated(i3 + 1);
														mnew.childUpdated(i2 + 1);
													}
												}
												b = false;
											}
											break;
										}
										default: {
											if(mfactor->equals(mnew[i2][i3])) {
												mnew[i2].delChild(i3 + 1);
												b = false;
											}
										}
									}
								}
								if(mnew[i2].size() == 1) {
									MathStructure mstruct2(mnew[i2][0]);
									mnew[i2] = mstruct2;
								}
								break;
							}
							default: {
								if(mfactor->isNumber()) {
									mnew[i2].transform(STRUCT_MULTIPLICATION);
									mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
									mnew[i2][0].number() /= mfactor->number();
								} else {
									mnew[i2].set(m_one);
								}
							}
						}
					}
					if(factor_mstruct.isMultiplication()) {
						i++;
					} else {
						break;
					}
				}
				return true;
			}
		}
		default: {}
	}
	return false;
}

bool get_first_symbol(const MathStructure &mpoly, MathStructure &xvar) {
	if(IS_A_SYMBOL(mpoly) || mpoly.isUnit()) {
		xvar = mpoly;
		return true;
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) {
			if(get_first_symbol(mpoly[i], xvar)) return true;
		}
	} else if(mpoly.isPower()) {
		return get_first_symbol(mpoly[0], xvar);
	}
	return false;
}

bool MathStructure::polynomialDivide(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args) {

	mquotient.clear();

	if(CALCULATOR->aborted()) return false;

	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber()) {
		mquotient = mnum;
		if(mnum.isNumber()) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	} else if(mnum.isNumber()) {
		return false;
	}

	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}

	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}


	MathStructure xvar;
	if(!get_first_symbol(mnum, xvar) && !get_first_symbol(mden, xvar)) return false;

	EvaluationOptions eo2 = eo;
	eo2.keep_zero_units = false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);

	MathStructure mrem(mnum);

	for(size_t i = 0; numdeg.isGreaterThanOrEqualTo(dendeg); i++) {
		if(i > 1000 || CALCULATOR->aborted()) {
			return false;
		}
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1, 0);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo2);
				}
			} else {
				MathStructure mcopy(numcoeff);
				if((mcopy.equals(mnum, true, true) && dencoeff.equals(mden, true, true)) || !MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo2, false)) {
					return false;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo2);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.add(numcoeff, true);
		if(numcoeff.isAddition() && mden.isAddition() && numcoeff.size() * mden.size() >= (eo.expand == -1 ? 50 : 500)) return false;
		numcoeff.calculateMultiply(mden, eo2);
		mrem.calculateSubtract(numcoeff, eo2);
		if(mrem.isZero()) {
			return true;
		}
		numdeg = mrem.degree(xvar);
	}
	return false;
}

bool polynomial_divide_integers(const vector<Number> &vnum, const vector<Number> &vden, vector<Number> &vquotient) {

	vquotient.clear();

	long int numdeg = vnum.size() - 1;
	long int dendeg = vden.size() - 1;
	Number dencoeff(vden[dendeg]);

	if(numdeg < dendeg) return false;

	vquotient.resize(numdeg - dendeg + 1, nr_zero);

	vector<Number> vrem = vnum;

	while(numdeg >= dendeg) {
		Number numcoeff(vrem[numdeg]);
		numdeg -= dendeg;
		if(!numcoeff.isIntegerDivisible(dencoeff)) break;
		numcoeff /= dencoeff;
		vquotient[numdeg] += numcoeff;
		for(size_t i = 0; i < vden.size(); i++) {
			vrem[numdeg + i] -= (vden[i] * numcoeff);
		}
		while(true) {
			if(vrem.back().isZero()) vrem.pop_back();
			else break;
			if(vrem.size() == 0) return true;
		}
		numdeg = (long int) vrem.size() - 1;
	}
	return false;
}

bool divide_in_z(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const sym_desc_vec &sym_stats, size_t var_i, const EvaluationOptions &eo) {

	mquotient.clear();
	if(mden.isZero()) return false;
	if(mnum.isZero()) return true;
	if(mden.isOne()) {
		mquotient = mnum;
		return true;
	}
	if(mnum.isNumber()) {
		if(!mden.isNumber()) {
			return false;
		}
		mquotient = mnum;
		return mquotient.number().divide(mden.number()) && mquotient.isInteger();
	}
	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}

	if(mden.isPower()) {
		MathStructure qbar(mnum);
		for(Number ni(mden[1].number()); ni.isPositive(); ni--) {
			if(!divide_in_z(qbar, mden[0], mquotient, sym_stats, var_i, eo)) return false;
			qbar = mquotient;
		}
		return true;
	}

	if(mden.isMultiplication()) {
		MathStructure qbar(mnum);
		for(size_t i = 0; i < mden.size(); i++) {
			sym_desc_vec sym_stats2;
			get_symbol_stats(mnum, mden[i], sym_stats2);
			if(!divide_in_z(qbar, mden[i], mquotient, sym_stats2, 0, eo)) return false;
			qbar = mquotient;
		}
		return true;
	}

	if(var_i >= sym_stats.size()) return false;
	const MathStructure &xvar = sym_stats[var_i].sym;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	if(dendeg.isGreaterThan(numdeg)) return false;
	MathStructure dencoeff;
	MathStructure mrem(mnum);
	mden.coefficient(xvar, dendeg, dencoeff);

	while(numdeg.isGreaterThanOrEqualTo(dendeg)) {
		if(CALCULATOR->aborted()) return false;
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		MathStructure term;
		if(!divide_in_z(numcoeff, dencoeff, term, sym_stats, var_i + 1, eo)) break;
		numdeg -= dendeg;
		if(!numdeg.isZero() && !term.isZero()) {
			if(term.isOne()) {
				term = xvar;
				if(!numdeg.isOne()) {
					term.raise(numdeg);
				}
			} else {
				term.multiply(xvar, true);
				if(!numdeg.isOne()) {
					term[term.size() - 1].raise(numdeg);
				}
				term.calculateMultiplyLast(eo);
			}
		}
		if(mquotient.isZero()) {
			mquotient = term;
		} else {
			mquotient.calculateAdd(term, eo);
		}
		if(term.isAddition() && mden.isAddition() && term.size() * mden.size() >= (eo.expand == -1 ? 50 : 500)) return false;
		term.calculateMultiply(mden, eo);
		mrem.calculateSubtract(term, eo);
		if(mrem.isZero()) {
			return true;
		}
		numdeg = mrem.degree(xvar);

	}
	return false;
}

bool prem(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mrem, const EvaluationOptions &eo, bool check_args) {

	mrem.clear();
	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isNumber()) {
		if(!mden.isNumber()) {
			mrem = mden;
		}
		return true;
	}
	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	mrem = mnum;
	MathStructure eb(mden);
	Number rdeg = mrem.degree(xvar);
	Number bdeg = eb.degree(xvar);
	MathStructure blcoeff;
	if(bdeg.isLessThanOrEqualTo(rdeg)) {
		eb.coefficient(xvar, bdeg, blcoeff);
		if(bdeg == 0) {
			eb.clear();
		} else {
			MathStructure mpow(xvar);
			mpow.raise(bdeg);
			mpow.calculateRaiseExponent(eo);
			//POWER_CLEAN(mpow)
			mpow.calculateMultiply(blcoeff, eo);
			eb.calculateSubtract(mpow, eo);
		}
	} else {
		blcoeff.set(1, 1, 0);
	}

	Number delta(rdeg);
	delta -= bdeg;
	delta++;
	int i = 0;
	while(rdeg.isGreaterThanOrEqualTo(bdeg) && !mrem.isZero()) {
		if(CALCULATOR->aborted() || delta < i / 10) {mrem.clear(); return false;}
		MathStructure rlcoeff;
		mrem.coefficient(xvar, rdeg, rlcoeff);
		MathStructure term(xvar);
		term.raise(rdeg);
		term[1].number() -= bdeg;
		term.calculateRaiseExponent(eo);
		//POWER_CLEAN(term)
		term.calculateMultiply(rlcoeff, eo);
		term.calculateMultiply(eb, eo);
		if(rdeg == 0) {
			mrem = term;
			mrem.calculateNegate(eo);
		} else {
			if(!rdeg.isZero()) {
				rlcoeff.multiply(xvar, true);
				if(!rdeg.isOne()) {
					rlcoeff[rlcoeff.size() - 1].raise(rdeg);
					rlcoeff[rlcoeff.size() - 1].calculateRaiseExponent(eo);
				}
				rlcoeff.calculateMultiplyLast(eo);
			}
			mrem.calculateSubtract(rlcoeff, eo);
			if(mrem.isAddition() && blcoeff.isAddition() && mrem.size() * blcoeff.size() >= (eo.expand == -1 ? 50 : 500)) {mrem.clear(); return false;}
			mrem.calculateMultiply(blcoeff, eo);
			mrem.calculateSubtract(term, eo);
		}
		rdeg = mrem.degree(xvar);
		i++;
	}
	delta -= i;
	blcoeff.raise(delta);
	blcoeff.calculateRaiseExponent(eo);
	mrem.calculateMultiply(blcoeff, eo);
	return true;
}


bool sr_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const sym_desc_vec &sym_stats, size_t var_i, const EvaluationOptions &eo) {

	if(var_i >= sym_stats.size()) return false;
	const MathStructure &xvar = sym_stats[var_i].sym;

	MathStructure c, d;
	Number adeg = m1.degree(xvar);
	Number bdeg = m2.degree(xvar);
	Number cdeg, ddeg;
	if(adeg.isGreaterThanOrEqualTo(bdeg)) {
		c = m1;
		d = m2;
		cdeg = adeg;
		ddeg = bdeg;
	} else {
		c = m2;
		d = m1;
		cdeg = bdeg;
		ddeg = adeg;
	}

	MathStructure cont_c, cont_d;
	c.polynomialContent(xvar, cont_c, eo);
	d.polynomialContent(xvar, cont_d, eo);

	MathStructure gamma;

	if(!MathStructure::gcd(cont_c, cont_d, gamma, eo, NULL, NULL, false)) return false;
	mgcd = gamma;
	if(ddeg.isZero()) {
		return true;
	}

	MathStructure prim_c, prim_d;
	c.polynomialPrimpart(xvar, cont_c, prim_c, eo);
	d.polynomialPrimpart(xvar, cont_d, prim_d, eo);
	c = prim_c;
	d = prim_d;

	MathStructure r;
	MathStructure ri(1, 1, 0);
	MathStructure psi(1, 1, 0);
	Number delta(cdeg);
	delta -= ddeg;

	while(true) {

		if(CALCULATOR->aborted()) return false;

		if(!prem(c, d, xvar, r, eo, false)) return false;

		if(r.isZero()) {
			mgcd = gamma;
			MathStructure mprim;
			d.polynomialPrimpart(xvar, mprim, eo);
			if(CALCULATOR->aborted()) return false;
			mgcd.calculateMultiply(mprim, eo);
			return true;
		}

		c = d;
		cdeg = ddeg;

		MathStructure psi_pow(psi);
		psi_pow.calculateRaise(delta, eo);
		ri.calculateMultiply(psi_pow, eo);

		if(!divide_in_z(r, ri, d, sym_stats, var_i, eo)) {
			return false;
		}

		ddeg = d.degree(xvar);
		if(ddeg.isZero()) {
			if(r.isNumber()) {
				mgcd = gamma;
			} else {
				r.polynomialPrimpart(xvar, mgcd, eo);
				if(CALCULATOR->aborted()) return false;
				mgcd.calculateMultiply(gamma, eo);
			}
			return true;
		}

		c.lcoefficient(xvar, ri);
		if(delta.isOne()) {
			psi = ri;
		} else if(!delta.isZero()) {
			MathStructure ri_pow(ri);
			ri_pow.calculateRaise(delta, eo);
			MathStructure psi_pow(psi);
			delta--;
			psi_pow.calculateRaise(delta, eo);
			divide_in_z(ri_pow, psi_pow, psi, sym_stats, var_i + 1, eo);
		}
		delta = cdeg;
		delta -= ddeg;

	}

	return false;

}

Number MathStructure::maxCoefficient() {
	if(isNumber()) {
		Number nr(o_number);
		nr.abs();
		return nr;
	} else if(isAddition()) {
		Number cur_max(overallCoefficient());
		cur_max.abs();
		for(size_t i = 0; i < SIZE; i++) {
			Number a(CHILD(i).overallCoefficient());
			a.abs();
			if(a.isGreaterThan(cur_max)) cur_max = a;
		}
		return cur_max;
	} else if(isMultiplication()) {
		Number nr(overallCoefficient());
		nr.abs();
		return nr;
	} else {
		return nr_one;
	}
}
void polynomial_smod(const MathStructure &mpoly, const Number &xi, MathStructure &msmod, const EvaluationOptions &eo, MathStructure *mparent, size_t index_smod) {
	if(mpoly.isNumber()) {
		msmod = mpoly;
		msmod.number().smod(xi);
	} else if(mpoly.isAddition()) {
		msmod.clear();
		msmod.setType(STRUCT_ADDITION);
		msmod.resizeVector(mpoly.size(), m_zero);
		for(size_t i = 0; i < mpoly.size(); i++) {
			polynomial_smod(mpoly[i], xi, msmod[i], eo, &msmod, i);
		}
		msmod.calculatesub(eo, eo, false, mparent, index_smod);
	} else if(mpoly.isMultiplication()) {
		msmod = mpoly;
		if(msmod.size() > 0 && msmod[0].isNumber()) {
			if(!msmod[0].number().smod(xi) || msmod[0].isZero()) {
				msmod.clear();
			}
		}
	} else {
		msmod = mpoly;
	}
}

bool interpolate(const MathStructure &gamma, const Number &xi, const MathStructure &xvar, MathStructure &minterp, const EvaluationOptions &eo) {
	MathStructure e(gamma);
	Number rxi(xi);
	rxi.recip();
	minterp.clear();
	for(long int i = 0; !e.isZero(); i++) {
		if(CALCULATOR->aborted()) return false;
		MathStructure gi;
		polynomial_smod(e, xi, gi, eo);
		if(minterp.isZero() && !gi.isZero()) {
			minterp = gi;
			if(i != 0) {
				if(minterp.isOne()) {
					minterp = xvar;
					if(i != 1) minterp.raise(i);
				} else {
					minterp.multiply(xvar, true);
					if(i != 1) minterp[minterp.size() - 1].raise(i);
					minterp.calculateMultiplyLast(eo);
				}
			}
		} else if(!gi.isZero()) {
			minterp.add(gi, true);
			if(i != 0) {
				if(minterp[minterp.size() - 1].isOne()) {
					minterp[minterp.size() - 1] = xvar;
					if(i != 1) minterp[minterp.size() - 1].raise(i);
				} else {
					minterp[minterp.size() - 1].multiply(xvar, true);
					if(i != 1) minterp[minterp.size() - 1][minterp[minterp.size() - 1].size() - 1].raise(i);
					minterp[minterp.size() - 1].calculateMultiplyLast(eo);
				}
			}
		}
		if(!gi.isZero()) e.calculateSubtract(gi, eo);
		e.calculateMultiply(rxi, eo);
	}
	minterp.calculatesub(eo, eo, false);
	return true;
}

bool heur_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const EvaluationOptions &eo, MathStructure *ca, MathStructure *cb, sym_desc_vec &sym_stats, size_t var_i) {

	if(m1.isZero() || m2.isZero())	return false;

	if(m1.isNumber() && m2.isNumber()) {
		mgcd = m1;
		if(!mgcd.number().gcd(m2.number())) mgcd.set(1, 1, 0);
		if(ca) {
			*ca = m1;
			ca->number() /= mgcd.number();
		}
		if(cb) {
			*cb = m2;
			cb->number() /= mgcd.number();
		}
		return true;
	}

	if(var_i >= sym_stats.size()) return false;
	const MathStructure &xvar = sym_stats[var_i].sym;

	Number nr_gc;
	integer_content(m1, nr_gc);
	Number nr_rgc;
	integer_content(m2, nr_rgc);
	nr_gc.gcd(nr_rgc);
	nr_rgc = nr_gc;
	nr_rgc.recip();
	MathStructure p(m1);
	p.calculateMultiply(nr_rgc, eo);
	MathStructure q(m2);
	q.calculateMultiply(nr_rgc, eo);
	Number maxdeg(p.degree(xvar));
	Number maxdeg2(q.degree(xvar));
	if(maxdeg2.isGreaterThan(maxdeg)) maxdeg = maxdeg2;
	Number mp(p.maxCoefficient());
	Number mq(q.maxCoefficient());
	Number xi;
	if(mp.isGreaterThan(mq)) {
		xi = mq;
	} else {
		xi = mp;
	}
	xi *= 2;
	xi += 2;

	for(int t = 0; t < 6; t++) {

		if(CALCULATOR->aborted()) return false;

		if(!xi.isInteger() || (maxdeg * xi.integerLength()).isGreaterThan(100000L)) {
			return false;
		}

		MathStructure cp, cq;
		MathStructure gamma;
		MathStructure psub(p);
		psub.calculateReplace(xvar, xi, eo, true);
		MathStructure qsub(q);
		qsub.calculateReplace(xvar, xi, eo, true);

		if(heur_gcd(psub, qsub, gamma, eo, &cp, &cq, sym_stats, var_i + 1)) {

			if(!interpolate(gamma, xi, xvar, mgcd, eo)) return false;

			Number ig;
			integer_content(mgcd, ig);
			ig.recip();
			mgcd.calculateMultiply(ig, eo);

			MathStructure dummy;

			if(divide_in_z(p, mgcd, ca ? *ca : dummy, sym_stats, var_i, eo) && divide_in_z(q, mgcd, cb ? *cb : dummy, sym_stats, var_i, eo)) {
				mgcd.calculateMultiply(nr_gc, eo);
				return true;
			}
		}

		Number xi2(xi);
		xi2.isqrt();
		xi2.isqrt();
		xi *= xi2;
		xi *= 73794L;
		xi.iquo(27011L);

	}

	return false;

}

int MathStructure::polynomialUnit(const MathStructure &xvar) const {
	MathStructure coeff;
	lcoefficient(xvar, coeff);
	if(coeff.hasNegativeSign()) return -1;
	return 1;
}

void integer_content(const MathStructure &mpoly, Number &icontent) {
	if(mpoly.isNumber()) {
		icontent = mpoly.number();
		icontent.abs();
	} else if(mpoly.isAddition()) {
		icontent.clear();
		Number l(1, 1);
		for(size_t i = 0; i < mpoly.size(); i++) {
			if(mpoly[i].isNumber()) {
				if(!icontent.isOne()) {
					Number c = icontent;
					icontent = mpoly[i].number().numerator();
					icontent.gcd(c);
				}
				Number l2 = l;
				l = mpoly[i].number().denominator();
				l.lcm(l2);
			} else if(mpoly[i].isMultiplication()) {
				if(!icontent.isOne()) {
					Number c = icontent;
					icontent = mpoly[i].overallCoefficient().numerator();
					icontent.gcd(c);
				}
				Number l2 = l;
				l = mpoly[i].overallCoefficient().denominator();
				l.lcm(l2);
			} else {
				icontent.set(1, 1, 0);
			}
		}
		icontent /= l;
	} else if(mpoly.isMultiplication()) {
		icontent = mpoly.overallCoefficient();
		icontent.abs();
	} else {
		icontent.set(1, 1, 0);
	}
}

bool MathStructure::lcm(const MathStructure &m1, const MathStructure &m2, MathStructure &mlcm, const EvaluationOptions &eo, bool check_args) {
	if(m1.isNumber() && m2.isNumber()) {
		mlcm = m1;
		return mlcm.number().lcm(m2.number());
	}
	if(check_args && (!m1.isRationalPolynomial() || !m2.isRationalPolynomial())) {
		return false;
	}
	MathStructure ca, cb;
	if(!MathStructure::gcd(m1, m2, mlcm, eo, &ca, &cb, false)) return false;
	mlcm.calculateMultiply(ca, eo);
	mlcm.calculateMultiply(cb, eo);
	return true;
}

void MathStructure::polynomialContent(const MathStructure &xvar, MathStructure &mcontent, const EvaluationOptions &eo) const {
	if(isZero()) {
		mcontent.clear();
		return;
	}
	if(isNumber()) {
		mcontent = *this;
		mcontent.number().setNegative(false);
		return;
	}

	MathStructure c;
	integer_content(*this, c.number());
	MathStructure r(*this);
	if(!c.isOne()) r.calculateDivide(c, eo);
	MathStructure lcoeff;
	r.lcoefficient(xvar, lcoeff);
	if(lcoeff.isInteger()) {
		mcontent = c;
		return;
	}
	Number deg(r.degree(xvar));
	Number ldeg(r.ldegree(xvar));
	if(deg == ldeg) {
		mcontent = lcoeff;
		if(lcoeff.polynomialUnit(xvar) == -1) {
			c.number().negate();
		}
		mcontent.calculateMultiply(c, eo);
		return;
	}
	mcontent.clear();
	MathStructure mtmp, coeff;
	for(Number i(ldeg); i.isLessThanOrEqualTo(deg); i++) {
		coefficient(xvar, i, coeff);
		mtmp = mcontent;
		if(!MathStructure::gcd(coeff, mtmp, mcontent, eo, NULL, NULL, false)) mcontent.set(1, 1, 0);
		if(mcontent.isOne()) break;
	}

	if(!c.isOne()) mcontent.calculateMultiply(c, eo);

}

void MathStructure::polynomialPrimpart(const MathStructure &xvar, MathStructure &mprim, const EvaluationOptions &eo) const {
	if(isZero()) {
		mprim.clear();
		return;
	}
	if(isNumber()) {
		mprim.set(1, 1, 0);
		return;
	}

	MathStructure c;
	polynomialContent(xvar, c, eo);
	if(c.isZero()) {
		mprim.clear();
		return;
	}
	bool b = (polynomialUnit(xvar) == -1);
	if(c.isNumber()) {
		if(b) c.number().negate();
		mprim = *this;
		mprim.calculateDivide(c, eo);
		return;
	}
	if(b) c.calculateNegate(eo);
	MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
}
void MathStructure::polynomialUnitContentPrimpart(const MathStructure &xvar, int &munit, MathStructure &mcontent, MathStructure &mprim, const EvaluationOptions &eo) const {

	if(isZero()) {
		munit = 1;
		mcontent.clear();
		mprim.clear();
		return;
	}

	if(isNumber()) {
		if(o_number.isNegative()) {
			munit = -1;
			mcontent = *this;
			mcontent.number().abs();
		} else {
			munit = 1;
			mcontent = *this;
		}
		mprim.set(1, 1, 0);
		return;
	}

	munit = polynomialUnit(xvar);

	polynomialContent(xvar, mcontent, eo);

	if(mcontent.isZero()) {
		mprim.clear();
		return;
	}
	if(mcontent.isNumber()) {
		mprim = *this;
		if(munit == -1) {
			Number c(mcontent.number());
			c.negate();
			mprim.calculateDivide(c, eo);
		} else if(!mcontent.isOne()) {
			mprim.calculateDivide(mcontent, eo);
		}
	} else {
		if(munit == -1) {
			MathStructure c(mcontent);
			c.calculateNegate(eo);
			MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
		} else {
			MathStructure::polynomialQuotient(*this, mcontent, xvar, mprim, eo, false);
		}
	}
}


void MathStructure::polynomialPrimpart(const MathStructure &xvar, const MathStructure &c, MathStructure &mprim, const EvaluationOptions &eo) const {
	if(isZero() || c.isZero()) {
		mprim.clear();
		return;
	}
	if(isNumber()) {
		mprim.set(1, 1, 0);
		return;
	}
	bool b = (polynomialUnit(xvar) == -1);
	if(c.isNumber()) {
		MathStructure cn(c);
		if(b) cn.number().negate();
		mprim = *this;
		mprim.calculateDivide(cn, eo);
		return;
	}
	if(b) {
		MathStructure cn(c);
		cn.calculateNegate(eo);
		MathStructure::polynomialQuotient(*this, cn, xvar, mprim, eo, false);
	} else {
		MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
	}
}

const Number& MathStructure::degree(const MathStructure &xvar) const {
	const Number *c = NULL;
	const MathStructure *mcur = NULL;
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			if(!c) {
				c = &nr_one;
			}
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if(!c || c->isLessThan((*mcur)[1].number())) {
				c = &(*mcur)[1].number();
			}
		} else if(mcur->isMultiplication()) {
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					if(!c) {
						c = &nr_one;
					}
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					if(!c || c->isLessThan((*mcur)[i2][1].number())) {
						c = &(*mcur)[i2][1].number();
					}
				}
			}
		}
		if(!isAddition()) break;
	}
	if(!c) return nr_zero;
	return *c;
}
const Number& MathStructure::ldegree(const MathStructure &xvar) const {
	const Number *c = NULL;
	const MathStructure *mcur = NULL;
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			c = &nr_one;
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if(!c || c->isGreaterThan((*mcur)[1].number())) {
				c = &(*mcur)[1].number();
			}
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					c = &nr_one;
					b = true;
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					if(!c || c->isGreaterThan((*mcur)[i2][1].number())) {
						c = &(*mcur)[i2][1].number();
					}
					b = true;
				}
			}
			if(!b) return nr_zero;
		} else {
			return nr_zero;
		}
		if(!isAddition()) break;
	}
	if(!c) return nr_zero;
	return *c;
}
void MathStructure::lcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const {
	coefficient(xvar, degree(xvar), mcoeff);
}
void MathStructure::tcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const {
	coefficient(xvar, ldegree(xvar), mcoeff);
}
void MathStructure::coefficient(const MathStructure &xvar, const Number &pownr, MathStructure &mcoeff) const {
	const MathStructure *mcur = NULL;
	mcoeff.clear();
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			if(pownr.isOne()) {
				if(mcoeff.isZero()) mcoeff.set(1, 1, 0);
				else mcoeff.add(m_one, true);
			}
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if((*mcur)[1].number() == pownr) {
				if(mcoeff.isZero()) mcoeff.set(1, 1, 0);
				else mcoeff.add(m_one, true);
			}
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				bool b2 = false;
				if((*mcur)[i2] == xvar) {
					b = true;
					if(pownr.isOne()) b2 = true;
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					b = true;
					if((*mcur)[i2][1].number() == pownr) b2 = true;
				}
				if(b2) {
					if(mcoeff.isZero()) {
						if(mcur->size() == 1) {
							mcoeff.set(1, 1, 0);
						} else {
							for(size_t i3 = 0; i3 < mcur->size(); i3++) {
								if(i3 != i2) {
									if(mcoeff.isZero()) mcoeff = (*mcur)[i3];
									else mcoeff.multiply((*mcur)[i3], true);
								}
							}
						}
					} else if(mcur->size() == 1) {
						mcoeff.add(m_one, true);
					} else {
						mcoeff.add(m_zero, true);
						for(size_t i3 = 0; i3 < mcur->size(); i3++) {
							if(i3 != i2) {
								if(mcoeff[mcoeff.size() - 1].isZero()) mcoeff[mcoeff.size() - 1] = (*mcur)[i3];
								else mcoeff[mcoeff.size() - 1].multiply((*mcur)[i3], true);
							}
						}
					}
					break;
				}
			}
			if(!b && pownr.isZero()) {
				if(mcoeff.isZero()) mcoeff = *mcur;
				else mcoeff.add(*mcur, true);
			}
		} else if(pownr.isZero()) {
			if(mcoeff.isZero()) mcoeff = *mcur;
			else mcoeff.add(*mcur, true);
		}
		if(!isAddition()) break;
	}
	mcoeff.evalSort();
}

bool MathStructure::polynomialQuotient(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args) {
	mquotient.clear();
	if(CALCULATOR->aborted()) return false;
	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber() && mnum.isNumber()) {
		mquotient = mnum;
		if(IS_REAL(mden) && IS_REAL(mnum)) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	}
	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}
	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	EvaluationOptions eo2 = eo;
	eo2.keep_zero_units = false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);
	MathStructure mrem(mnum);
	for(size_t i = 0; numdeg.isGreaterThanOrEqualTo(dendeg); i++) {
		if(i > 1000 || CALCULATOR->aborted()) {
			return false;
		}
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1, 0);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo2);
				}
			} else {
				MathStructure mcopy(numcoeff);
				if(!MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo2, false)) {
					return false;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo2);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.calculateAdd(numcoeff, eo2);
		numcoeff.calculateMultiply(mden, eo2);
		mrem.calculateSubtract(numcoeff, eo2);
		if(mrem.isZero()) break;
		numdeg = mrem.degree(xvar);
	}
	return true;

}

bool combine_powers(MathStructure &m, const MathStructure &x_var, const EvaluationOptions &eo) {
	bool b_ret = false;
	if(!m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(combine_powers(m[i], x_var, eo)) {
				m.childUpdated(i + 1);
				b_ret = true;
			}
		}
		return b_ret;
	}
	for(size_t i = 0; i < m.size() - 1; i++) {
		if(m[i].isPower() && !m[i][0].contains(x_var, true) && m[i][1].contains(x_var, true)) {
			for(size_t i2 = i + 1; i2 < m.size(); i2++) {
				// a^(f(x))*b^(g(x))=e^(f(x)/ln(a)+g(x)/ln(b))
				if(m[i2].isPower() && !m[i][0].contains(x_var, true) && m[i][1].contains(x_var, true)) {
					if(m[i2][0] != m[i][0]) {
						if(m[i2][0] != CALCULATOR->v_e) {
							MathStructure mln(m[i2][0]);
							mln.transform(CALCULATOR->f_ln);
							m[i2][1].calculateMultiply(mln, eo);
						}
						if(m[i][0] != CALCULATOR->v_e) {
							MathStructure mln(m[i][0]);
							mln.transform(CALCULATOR->f_ln);
							m[i][1].calculateMultiply(mln, eo);
							m[i][0] = CALCULATOR->v_e;
							m[i].childrenUpdated();
						}
					}
					m[i2][1].ref();
					m[i][1].add_nocopy(&m[i2][1], true);
					m[i][1].calculateAddLast(eo);
					m[i].childUpdated(2);
					m.childUpdated(i + 1);
					m.delChild(i2 + 1);
					b_ret = true;
				} else {
					i2++;
				}
			}
			if(b_ret && m.size() == 1) {
				m.setToChild(1, true);
			}
			return b_ret;
		}
	}
	return false;
}
bool remove_zerointerval_multiplier(MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct.isAddition()) {
		bool b;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(remove_zerointerval_multiplier(mstruct[i], eo)) b = true;
		}
		if(b) {
			mstruct.calculatesub(eo, eo, false);
			return true;
		}
	} else if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isNumber() && !mstruct[i].number().isNonZero()) {
				mstruct.clear(true);
				return true;
			}
		}
	} else if(mstruct.isNumber() && !mstruct.number().isNonZero()) {
		mstruct.clear(true);
		return true;
	}
	return false;
}
bool contains_zerointerval_multiplier(MathStructure &mstruct) {
	if(mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(contains_zerointerval_multiplier(mstruct[i])) return true;
		}
	} else if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isNumber() && !mstruct[i].number().isNonZero()) return true;
		}
	} else if(mstruct.isNumber() && !mstruct.number().isNonZero()) {
		return true;
	}
	return false;
}

bool polynomial_long_division(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar_pre, MathStructure &mquotient, MathStructure &mrem, const EvaluationOptions &eo, bool check_args, bool for_newtonraphson) {
	mquotient.clear();
	mrem.clear();

	if(CALCULATOR->aborted()) return false;
	if(mden.isZero()) {
		//division by zero
		mrem.set(mnum);
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber() && mnum.isNumber()) {
		mquotient = mnum;
		if(IS_REAL(mden) && IS_REAL(mnum)) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	} else if(mnum.isNumber()) {
		mrem.set(mnum);
		return false;
	}
	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}

	mrem.set(mnum);

	if(check_args && (!mnum.isRationalPolynomial(true, true) || !mden.isRationalPolynomial(true, true))) {
		return false;
	}

	MathStructure xvar(xvar_pre);
	if(xvar.isZero()) {
		vector<MathStructure> symsd, symsn;
		collect_symbols(mnum, symsn);
		if(symsn.empty()) return false;
		collect_symbols(mden, symsd);
		if(symsd.empty()) return false;
		for(size_t i = 0; i < symsd.size();) {
			bool b_found = false;
			for(size_t i2 = 0; i2 < symsn.size(); i2++) {
				if(symsn[i2] == symsd[i]) {
					b_found = (mnum.degree(symsd[i]) >= mden.degree(symsd[i]));
					break;
				}
			}
			if(b_found) i++;
			else symsd.erase(symsd.begin() + i);
		}
		for(size_t i = 0; i < symsd.size(); i++) {
			MathStructure mquotient2, mrem2;
			if(polynomial_long_division(mrem, mden, symsd[i], mquotient2, mrem2, eo, false, for_newtonraphson) && !mquotient2.isZero()) {
				mrem = mrem2;
				if(mquotient.isZero()) mquotient = mquotient2;
				else mquotient.calculateAdd(mquotient2, eo);
			}
			if(CALCULATOR->aborted()) {
				mrem = mnum;
				mquotient.clear();
				return false;
			}
		}
		return true;
	}

	EvaluationOptions eo2 = eo;
	eo2.keep_zero_units = false;
	eo2.do_polynomial_division = false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);

	for(size_t i = 0; numdeg.isGreaterThanOrEqualTo(dendeg); i++) {
		if(i > 10000 || CALCULATOR->aborted()) {
			mrem = mnum;
			mquotient.clear();
			return false;
		}
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1, 0);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo2);
				}
			} else {
				if(for_newtonraphson) {mrem = mnum; mquotient.clear(); return false;}
				MathStructure mcopy(numcoeff);
				if(!MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo2, check_args)) {
					if(CALCULATOR->aborted()) {mrem = mnum; mquotient.clear(); return false;}
					break;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo2);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.calculateAdd(numcoeff, eo2);
		if(numcoeff.isAddition() && mden.isAddition() && numcoeff.size() * mden.size() >= (eo.expand == -1 ? 50 : 500)) {mrem = mnum; mquotient.clear(); return false;}
		numcoeff.calculateMultiply(mden, eo2);
		mrem.calculateSubtract(numcoeff, eo2);
		if(contains_zerointerval_multiplier(mquotient)) {mrem = mnum; mquotient.clear(); return false;}
		if(mrem.isZero() || (for_newtonraphson && mrem.isNumber())) break;
		if(contains_zerointerval_multiplier(mrem)) {mrem = mnum; mquotient.clear(); return false;}
		numdeg = mrem.degree(xvar);
	}
	return true;

}

void add_symbol(const MathStructure &mpoly, sym_desc_vec &v) {
	sym_desc_vec::const_iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		if(it->sym == mpoly) return;
		++it;
	}
	sym_desc d;
	d.sym = mpoly;
	v.push_back(d);
}

void collect_symbols(const MathStructure &mpoly, sym_desc_vec &v) {
	if(IS_A_SYMBOL(mpoly) || mpoly.isUnit()) {
		add_symbol(mpoly, v);
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) collect_symbols(mpoly[i], v);
	} else if(mpoly.isPower()) {
		collect_symbols(mpoly[0], v);
	}
}

void add_symbol(const MathStructure &mpoly, vector<MathStructure> &v) {
	vector<MathStructure>::const_iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		if(*it == mpoly) return;
		++it;
	}
	v.push_back(mpoly);
}

void collect_symbols(const MathStructure &mpoly, vector<MathStructure> &v) {
	if(IS_A_SYMBOL(mpoly)) {
		add_symbol(mpoly, v);
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) collect_symbols(mpoly[i], v);
	} else if(mpoly.isPower()) {
		collect_symbols(mpoly[0], v);
	}
}

void get_symbol_stats(const MathStructure &m1, const MathStructure &m2, sym_desc_vec &v) {
	collect_symbols(m1, v);
	collect_symbols(m2, v);
	sym_desc_vec::iterator it = v.begin(), itend = v.end();
	while(it != itend) {
		it->deg_a = m1.degree(it->sym);
		it->deg_b = m2.degree(it->sym);
		if(it->deg_a.isGreaterThan(it->deg_b)) {
			it->max_deg = it->deg_a;
		} else {
			it->max_deg = it->deg_b;
		}
		it->ldeg_a = m1.ldegree(it->sym);
		it->ldeg_b = m2.ldegree(it->sym);
		MathStructure mcoeff;
		m1.lcoefficient(it->sym, mcoeff);
		it->max_lcnops = mcoeff.size();
		m2.lcoefficient(it->sym, mcoeff);
		if(mcoeff.size() > it->max_lcnops) it->max_lcnops = mcoeff.size();
		++it;
	}
	std::sort(v.begin(), v.end());

}

bool has_noninteger_coefficient(const MathStructure &mstruct) {
	if(mstruct.isNumber() && mstruct.number().isRational() && !mstruct.number().isInteger()) return true;
	if(mstruct.isFunction() || mstruct.isPower()) return false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(has_noninteger_coefficient(mstruct[i])) return true;
	}
	return false;
}

bool MathStructure::gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mresult, const EvaluationOptions &eo2, MathStructure *ca, MathStructure *cb, bool check_args) {

	EvaluationOptions eo = eo2;
	eo.keep_zero_units = false;

	if(ca) *ca = m1;
	if(cb) *cb = m2;
	mresult.set(1, 1, 0);

	if(CALCULATOR->aborted()) return false;

	if(m1.isOne() || m2.isOne()) {
		return true;
	}
	if(m1.isNumber() && m2.isNumber()) {
		mresult = m1;
		if(!mresult.number().gcd(m2.number())) mresult.set(1, 1, 0);
		if(ca || cb) {
			if(mresult.isZero()) {
				if(ca) ca->clear();
				if(cb) cb->clear();
			} else {
				if(ca) {
					*ca = m1;
					ca->number() /= mresult.number();
				}
				if(cb) {
					*cb = m2;
					cb->number() /= mresult.number();
				}
			}
		}
		return true;
	}

	if(m1 == m2) {
		if(ca) ca->set(1, 1, 0);
		if(cb) cb->set(1, 1, 0);
		mresult = m1;
		return true;
	}
	if(m1.isZero()) {
		if(ca) ca->clear();
		if(cb) cb->set(1, 1, 0);
		mresult = m2;
		return true;
	}
	if(m2.isZero()) {
		if(ca) ca->set(1, 1, 0);
		if(cb) cb->clear();
		mresult = m1;
		return true;
	}

	if(check_args && (!m1.isRationalPolynomial() || !m2.isRationalPolynomial())) {
		return false;
	}

	if(has_noninteger_coefficient(m1) || has_noninteger_coefficient(m2)) {
		Number nlcm1;
		lcm_of_coefficients_denominators(m1, nlcm1);
		Number nlcm2;
		lcm_of_coefficients_denominators(m2, nlcm2);
		nlcm1.lcm(nlcm2);
		if(!nlcm1.isOne()) {
			MathStructure mtmp1, mtmp2;
			multiply_lcm(m1, nlcm1, mtmp1, eo);
			multiply_lcm(m2, nlcm1, mtmp2, eo);
			if(mtmp1.equals(m1, true, true) || mtmp2.equals(m2, true, true)) return false;
			return gcd(mtmp1, mtmp2, mresult, eo, ca, cb, false);
		}
	}

	if(m1.isMultiplication()) {
		if(m2.isMultiplication() && m2.size() > m1.size())
			goto factored_2;
factored_1:
		mresult.clear();
		mresult.setType(STRUCT_MULTIPLICATION);
		MathStructure acc_ca;
		acc_ca.setType(STRUCT_MULTIPLICATION);
		MathStructure part_2(m2);
		MathStructure part_ca, part_cb;
		for(size_t i = 0; i < m1.size(); i++) {
			mresult.addChild(m_zero);
			MathStructure::gcd(m1[i], part_2, mresult[i], eo, &part_ca, &part_cb, false);
			if(CALCULATOR->aborted()) return false;
			acc_ca.addChild(part_ca);
			part_2 = part_cb;
		}
		mresult.calculatesub(eo, eo, false);
		if(ca) {
			*ca = acc_ca;
			ca->calculatesub(eo, eo, false);
		}
		if(cb) *cb = part_2;
		return true;
	} else if(m2.isMultiplication()) {
		if(m1.isMultiplication() && m1.size() > m2.size())
			goto factored_1;
factored_2:
		mresult.clear();
		mresult.setType(STRUCT_MULTIPLICATION);
		MathStructure acc_cb;
		acc_cb.setType(STRUCT_MULTIPLICATION);
		MathStructure part_1(m1);
		MathStructure part_ca, part_cb;
		for(size_t i = 0; i < m2.size(); i++) {
			mresult.addChild(m_zero);
			MathStructure::gcd(part_1, m2[i], mresult[i], eo, &part_ca, &part_cb, false);
			if(CALCULATOR->aborted()) return false;
			acc_cb.addChild(part_cb);
			part_1 = part_ca;
		}
		mresult.calculatesub(eo, eo, false);
		if(ca) *ca = part_1;
		if(cb) {
			*cb = acc_cb;
			cb->calculatesub(eo, eo, false);
		}
		return true;
	}

	if(m1.isPower()) {
		MathStructure p(m1[0]);
		if(m2.isPower()) {
			if(m1[0] == m2[0]) {
				if(m1[1].number().isLessThan(m2[1].number())) {
					if(ca) ca->set(1, 1, 0);
					if(cb) {
						*cb = m2;
						(*cb)[1].number() -= m1[1].number();
						POWER_CLEAN((*cb))
					}
					mresult = m1;
					return true;
				} else {
					if(ca) {
						*ca = m1;
						(*ca)[1].number() -= m2[1].number();
						POWER_CLEAN((*ca))
					}
					if(cb) cb->set(1, 1, 0);
					mresult = m2;
					return true;
				}
			} else {
				MathStructure p_co, pb_co;
				MathStructure p_gcd;
				if(!MathStructure::gcd(m1[0], m2[0], p_gcd, eo, &p_co, &pb_co, false)) return false;
				if(p_gcd.isOne()) {
					return true;
				} else {
					if(m1[1].number().isLessThan(m2[1].number())) {
						mresult = p_gcd;
						mresult.calculateRaise(m1[1], eo);
						mresult.multiply(m_zero, true);
						p_co.calculateRaise(m1[1], eo);
						pb_co.calculateRaise(m2[1], eo);
						p_gcd.raise(m2[1]);
						p_gcd[1].number() -= m1[1].number();
						p_gcd.calculateRaiseExponent(eo);
						p_gcd.calculateMultiply(pb_co, eo);
						bool b = MathStructure::gcd(p_co, p_gcd, mresult[mresult.size() - 1], eo, ca, cb, false);
						mresult.calculateMultiplyLast(eo);
						return b;
					} else {
						mresult = p_gcd;
						mresult.calculateRaise(m2[1], eo);
						mresult.multiply(m_zero, true);
						p_co.calculateRaise(m1[1], eo);
						pb_co.calculateRaise(m2[1], eo);
						p_gcd.raise(m1[1]);
						p_gcd[1].number() -= m2[1].number();
						p_gcd.calculateRaiseExponent(eo);
						p_gcd.calculateMultiply(p_co, eo);
						bool b = MathStructure::gcd(p_gcd, pb_co, mresult[mresult.size() - 1], eo, ca, cb, false);
						mresult.calculateMultiplyLast(eo);
						return b;
					}
				}
			}
		} else {
			if(m1[0] == m2) {
				if(ca) {
					*ca = m1;
					(*ca)[1].number()--;
					POWER_CLEAN((*ca))
				}
				if(cb) cb->set(1, 1, 0);
				mresult = m2;
				return true;
			}
			MathStructure p_co, bpart_co;
			MathStructure p_gcd;
			if(!MathStructure::gcd(m1[0], m2, p_gcd, eo, &p_co, &bpart_co, false)) return false;
			if(p_gcd.isOne()) {
				return true;
			} else {
				mresult = p_gcd;
				mresult.multiply(m_zero, true);
				p_co.calculateRaise(m1[1], eo);
				p_gcd.raise(m1[1]);
				p_gcd[1].number()--;
				p_gcd.calculateRaiseExponent(eo);
				p_gcd.calculateMultiply(p_co, eo);
				bool b = MathStructure::gcd(p_gcd, bpart_co, mresult[mresult.size() - 1], eo, ca, cb, false);
				mresult.calculateMultiplyLast(eo);
				return b;
			}
		}
	} else if(m2.isPower()) {
		if(m2[0] == m1) {
			if(ca) ca->set(1, 1, 0);
			if(cb) {
				*cb = m2;
				(*cb)[1].number()--;
				POWER_CLEAN((*cb))
			}
			mresult = m1;
			return true;
		}
		MathStructure p_co, apart_co;
		MathStructure p_gcd;
		if(!MathStructure::gcd(m1, m2[0], p_gcd, eo, &apart_co, &p_co, false)) return false;
		if(p_gcd.isOne()) {
			return true;
		} else {
			mresult = p_gcd;
			mresult.multiply(m_zero, true);
			p_co.calculateRaise(m2[1], eo);
			p_gcd.raise(m2[1]);
			p_gcd[1].number()--;
			p_gcd.calculateRaiseExponent(eo);
			p_gcd.calculateMultiply(p_co, eo);
			bool b = MathStructure::gcd(apart_co, p_gcd, mresult[mresult.size() - 1], eo, ca, cb, false);
			mresult.calculateMultiplyLast(eo);
			return b;
		}
	}
	if(IS_A_SYMBOL(m1) || m1.isUnit()) {
		MathStructure bex(m2);
		bex.calculateReplace(m1, m_zero, eo);
		if(!bex.isZero()) {
			return true;
		}
	}
	if(IS_A_SYMBOL(m2) || m2.isUnit()) {
		MathStructure aex(m1);
		aex.calculateReplace(m2, m_zero, eo);
		if(!aex.isZero()) {
			return true;
		}
	}

	sym_desc_vec sym_stats;
	get_symbol_stats(m1, m2, sym_stats);

	if(sym_stats.empty()) return false;

	size_t var_i = 0;
	const MathStructure &xvar = sym_stats[var_i].sym;
	
	/*for(size_t i = 0; i< sym_stats.size(); i++) {
		if(sym_stats[i].sym.size() > 0) {
			MathStructure m1b(m1), m2b(m2);
			vector<UnknownVariable*> vars;
			for(; i < sym_stats.size(); i++) {
				if(sym_stats[i].sym.size() > 0) {
					UnknownVariable *var = new UnknownVariable("", format_and_print(sym_stats[i].sym));
					m1b.replace(sym_stats[i].sym, var);
					m2b.replace(sym_stats[i].sym, var);
					vars.push_back(var);
				}
			}
			bool b = gcd(m1b, m2, mresult, eo2, ca, cb, false);
			size_t i2 = 0;
			for(i = 0; i < sym_stats.size(); i++) {
				if(sym_stats[i].sym.size() > 0) {
					mresult.replace(vars[i2], sym_stats[i].sym);
					if(ca) ca->replace(vars[i2], sym_stats[i].sym);
					if(cb) cb->replace(vars[i2], sym_stats[i].sym);
					vars[i2]->destroy();
					i2++;
				}
			}
			return b;
		}
	}*/

	Number ldeg_a(sym_stats[var_i].ldeg_a);
	Number ldeg_b(sym_stats[var_i].ldeg_b);
	Number min_ldeg;
	if(ldeg_a.isLessThan(ldeg_b)) {
		min_ldeg = ldeg_a;
	} else {
		min_ldeg = ldeg_b;
	}

	if(min_ldeg.isPositive()) {
		MathStructure aex(m1), bex(m2);
		MathStructure common(xvar);
		if(!min_ldeg.isOne()) common.raise(min_ldeg);
		aex.calculateDivide(common, eo);
		bex.calculateDivide(common, eo);
		MathStructure::gcd(aex, bex, mresult, eo, ca, cb, false);
		mresult.calculateMultiply(common, eo);
		return true;
	}

	if(sym_stats[var_i].deg_a.isZero()) {
		if(cb) {
			MathStructure c, mprim;
			int u;
			m2.polynomialUnitContentPrimpart(xvar, u, c, mprim, eo);
			if(!m2.equals(c, true, true)) MathStructure::gcd(m1, c, mresult, eo, ca, cb, false);
			if(CALCULATOR->aborted()) {
				if(ca) *ca = m1;
				if(cb) *cb = m2;
				mresult.set(1, 1, 0);
				return false;
			}
			if(u == -1) {
				cb->calculateNegate(eo);
			}
			cb->calculateMultiply(mprim, eo);
		} else {
			MathStructure c;
			m2.polynomialContent(xvar, c, eo);
			if(!m2.equals(c, true, true)) MathStructure::gcd(m1, c, mresult, eo, ca, cb, false);
		}
		return true;
	} else if(sym_stats[var_i].deg_b.isZero()) {
		if(ca) {
			MathStructure c, mprim;
			int u;
			m1.polynomialUnitContentPrimpart(xvar, u, c, mprim, eo);
			if(!m1.equals(c, true, true)) MathStructure::gcd(c, m2, mresult, eo, ca, cb, false);
			if(CALCULATOR->aborted()) {
				if(ca) *ca = m1;
				if(cb) *cb = m2;
				mresult.set(1, 1, 0);
				return false;
			}
			if(u == -1) {
				ca->calculateNegate(eo);
			}
			ca->calculateMultiply(mprim, eo);
		} else {
			MathStructure c;
			m1.polynomialContent(xvar, c, eo);
			if(!m1.equals(c, true, true)) MathStructure::gcd(c, m2, mresult, eo, ca, cb, false);
		}
		return true;
	}

	if(!heur_gcd(m1, m2, mresult, eo, ca, cb, sym_stats, var_i)) {
		if(!sr_gcd(m1, m2, mresult, sym_stats, var_i, eo)) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1, 0);
			return false;
		}
		if(!mresult.isOne()) {
			if(ca) {
				if(!MathStructure::polynomialDivide(m1, mresult, *ca, eo, false)) {
					if(ca) *ca = m1;
					if(cb) *cb = m2;
					mresult.set(1, 1, 0);
					return false;
				}
			}
			if(cb) {
				if(!MathStructure::polynomialDivide(m2, mresult, *cb, eo, false)) {
					if(ca) *ca = m1;
					if(cb) *cb = m2;
					mresult.set(1, 1, 0);
					return false;
				}
			}
		}
		if(CALCULATOR->aborted()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1, 0);
			return false;
		}
	}

	return true;
}

bool sqrfree_differentiate(const MathStructure &mpoly, const MathStructure &x_var, MathStructure &mdiff, const EvaluationOptions &eo) {
	if(mpoly == x_var) {
		mdiff.set(1, 1, 0);
		return true;
	}
	switch(mpoly.type()) {
		case STRUCT_ADDITION: {
			mdiff.clear();
			mdiff.setType(STRUCT_ADDITION);
			for(size_t i = 0; i < mpoly.size(); i++) {
				mdiff.addChild(m_zero);
				if(!sqrfree_differentiate(mpoly[i], x_var, mdiff[i], eo)) return false;
			}
			mdiff.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_VARIABLE: {}
		case STRUCT_FUNCTION: {}
		case STRUCT_SYMBOLIC: {}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			mdiff.clear();
			break;
		}
		case STRUCT_POWER: {
			if(mpoly[0] == x_var) {
				mdiff = mpoly[1];
				mdiff.multiply(x_var);
				if(!mpoly[1].number().isTwo()) {
					mdiff[1].raise(mpoly[1]);
					mdiff[1][1].number()--;
				}
				mdiff.evalSort(true);
			} else {
				mdiff.clear();
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mpoly.size() < 1) {
				mdiff.clear();
				break;
			} else if(mpoly.size() < 2) {
				return sqrfree_differentiate(mpoly[0], x_var, mdiff, eo);
			}
			mdiff.clear();
			for(size_t i = 0; i < mpoly.size(); i++) {
				if(mpoly[i] == x_var) {
					if(mpoly.size() == 2) {
						if(i == 0) mdiff = mpoly[1];
						else mdiff = mpoly[0];
					} else {
						mdiff.setType(STRUCT_MULTIPLICATION);
						for(size_t i2 = 0; i2 < mpoly.size(); i2++) {
							if(i2 != i) {
								mdiff.addChild(mpoly[i2]);
							}
						}
					}
					break;
				} else if(mpoly[i].isPower() && mpoly[i][0] == x_var) {
					mdiff = mpoly;
					if(mpoly[i][1].number().isTwo()) {
						mdiff[i].setToChild(1);
					} else {
						mdiff[i][1].number()--;
					}
					if(mdiff[0].isNumber()) {
						mdiff[0].number() *= mpoly[i][1].number();
					} else {
						mdiff.insertChild(mpoly[i][1].number(), 1);
					}
					mdiff.evalSort();
					break;
				}
			}
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}
bool fix_root_pow(MathStructure &m, const MathStructure &xvar, const EvaluationOptions &eo) {
	if(m.isPower() && m[0].contains(xvar) && m[1].isNumber()) {
		return m.calculateRaiseExponent(eo);
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_root_pow(m[i], xvar, eo)) {m.childUpdated(i + 1); b_ret = true;}
	}
	if(b_ret) m.calculatesub(eo, eo, false);
	return b_ret;
}

bool sqrfree_yun(const MathStructure &a, const MathStructure &xvar, MathStructure &factors, const EvaluationOptions &eo) {
	MathStructure w(a);
	MathStructure z;
	if(!sqrfree_differentiate(a, xvar, z, eo)) {
		return false;
	}
	MathStructure g;
	if(!MathStructure::gcd(w, z, g, eo)) {
		return false;
	}
	if(g.isOne()) {
		factors.addChild(a);
		return true;
	}
	MathStructure y;
	MathStructure tmp;
	do {
		tmp = w;
		if(!MathStructure::polynomialQuotient(tmp, g, xvar, w, eo)) {
			return false;
		}
		if(!MathStructure::polynomialQuotient(z, g, xvar, y, eo)) {
			return false;
		}
		if(!sqrfree_differentiate(w, xvar, tmp, eo)) {
			return false;
		}
		z = y;
		z.calculateSubtract(tmp, eo);
		if(!MathStructure::gcd(w, z, g, eo)) {
			return false;
		}
		factors.addChild(g);
	} while (!z.isZero());
	return true;
}

bool sqrfree_simple(const MathStructure &a, const MathStructure &xvar, MathStructure &factors, const EvaluationOptions &eo) {
	MathStructure w(a);
	size_t i = 0;
	while(true) {
		MathStructure z, zmod;
		if(!sqrfree_differentiate(w, xvar, z, eo)) return false;
		polynomial_smod(z, nr_three, zmod, eo);
		if(z == w) {
			factors.addChild(w);
			break;
		}
		MathStructure mgcd;
		if(!MathStructure::gcd(w, z, mgcd, eo)) return false;
		if(mgcd.isOne() || mgcd == w) {
			factors.addChild(w);
			break;
		}
		MathStructure tmp(w);
		if(!MathStructure::polynomialQuotient(tmp, mgcd, xvar, w, eo)) return false;
		if(!sqrfree_simple(mgcd, xvar, factors, eo)) return false;
		i++;
	}
	return true;
}

void lcmcoeff(const MathStructure &e, const Number &l, Number &nlcm);
void lcmcoeff(const MathStructure &e, const Number &l, Number &nlcm) {
	if(e.isNumber() && e.number().isRational()) {
		nlcm = e.number().denominator();
		nlcm.lcm(l);
	} else if(e.isAddition()) {
		nlcm.set(1, 1, 0);
		for(size_t i = 0; i < e.size(); i++) {
			Number c(nlcm);
			lcmcoeff(e[i], c, nlcm);
		}
		nlcm.lcm(l);
	} else if(e.isMultiplication()) {
		nlcm.set(1, 1, 0);
		for(size_t i = 0; i < e.size(); i++) {
			Number c(nlcm);
			lcmcoeff(e[i], nr_one, c);
			nlcm *= c;
		}
		nlcm.lcm(l);
	} else if(e.isPower()) {
		if(IS_A_SYMBOL(e[0]) || e[0].isUnit()) {
			nlcm = l;
		} else {
			lcmcoeff(e[0], l, nlcm);
			nlcm ^= e[1].number();
		}
	} else {
		nlcm = l;
	}
}

void lcm_of_coefficients_denominators(const MathStructure &e, Number &nlcm) {
	return lcmcoeff(e, nr_one, nlcm);
}

void multiply_lcm(const MathStructure &e, const Number &lcm, MathStructure &mmul, const EvaluationOptions &eo) {
	if(e.isMultiplication()) {
		Number lcm_accum(1, 1);
		mmul.clear();
		for(size_t i = 0; i < e.size(); i++) {
			Number op_lcm;
			lcmcoeff(e[i], nr_one, op_lcm);
			if(mmul.isZero()) {
				multiply_lcm(e[i], op_lcm, mmul, eo);
				if(mmul.isOne()) mmul.clear();
			} else {
				mmul.multiply(m_one, true);
				multiply_lcm(e[i], op_lcm, mmul[mmul.size() - 1], eo);
				if(mmul[mmul.size() - 1].isOne()) {
					mmul.delChild(i + 1);
					if(mmul.size() == 1) mmul.setToChild(1);
				}
			}
			lcm_accum *= op_lcm;
		}
		Number lcm2(lcm);
		lcm2 /= lcm_accum;
		if(mmul.isZero()) {
			mmul = lcm2;
		} else if(!lcm2.isOne()) {
			if(mmul.size() > 0 && mmul[0].isNumber()) {
				mmul[0].number() *= lcm2;
			} else {
				mmul.multiply(lcm2, true);
			}
		}
		mmul.evalSort();
	} else if(e.isAddition()) {
		mmul.clear();
		for (size_t i = 0; i < e.size(); i++) {
			if(mmul.isZero()) {
				multiply_lcm(e[i], lcm, mmul, eo);
			} else {
				mmul.add(m_zero, true);
				multiply_lcm(e[i], lcm, mmul[mmul.size() - 1], eo);
			}
		}
		mmul.evalSort();
	} else if(e.isPower()) {
		if(IS_A_SYMBOL(e[0]) || e[0].isUnit()) {
			mmul = e;
			if(!lcm.isOne()) {
				mmul *= lcm;
				mmul.evalSort();
			}
		} else {
			mmul = e;
			Number lcm_exp = e[1].number();
			lcm_exp.recip();
			multiply_lcm(e[0], lcm ^ lcm_exp, mmul[0], eo);
			if(mmul[0] != e[0]) {
				mmul.calculatesub(eo, eo, false);
			}
		}
	} else if(e.isNumber()) {
		mmul = e;
		mmul.number() *= lcm;
	} else if(IS_A_SYMBOL(e) || e.isUnit()) {
		mmul = e;
		if(!lcm.isOne()) {
			mmul *= lcm;
			mmul.evalSort();
		}
	} else {
		mmul = e;
		if(!lcm.isOne()) {
			mmul.calculateMultiply(lcm, eo);
			mmul.evalSort();
		}
	}
}

//from GiNaC
bool sqrfree(MathStructure &mpoly, const EvaluationOptions &eo) {
	vector<MathStructure> symbols;
	collect_symbols(mpoly, symbols);
	return sqrfree(mpoly, symbols, eo);
}
bool sqrfree(MathStructure &mpoly, const vector<MathStructure> &symbols, const EvaluationOptions &eo) {

	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = true;
	eo2.warn_about_denominators_assumed_nonzero = false;
	eo2.reduce_divisions = true;
	eo2.keep_zero_units = false;
	eo2.do_polynomial_division = false;
	eo2.sync_units = false;
	eo2.expand = true;
	eo2.calculate_functions = false;
	eo2.protected_function = CALCULATOR->f_signum;

	if(mpoly.size() == 0) {
		return true;
	}
	if(symbols.empty()) return true;

	size_t symbol_index = 0;
	if(symbols.size() > 1) {
		for(size_t i = 1; i < symbols.size(); i++) {
			if(mpoly.degree(symbols[symbol_index]).isGreaterThan(mpoly.degree(symbols[i]))) symbol_index = i;
		}
	}

	MathStructure xvar(symbols[symbol_index]);
	
	UnknownVariable *var = NULL;
	if(xvar.size() > 0) {
		var = new UnknownVariable("", format_and_print(xvar));
		var->setAssumptions(xvar);
		mpoly.replace(xvar, var);
		xvar = var;
	}

	Number nlcm;
	lcm_of_coefficients_denominators(mpoly, nlcm);

	MathStructure tmp;
	multiply_lcm(mpoly, nlcm, tmp, eo2);

	MathStructure factors;
	factors.clearVector();

	if(!sqrfree_yun(tmp, xvar, factors, eo2)) {
		if(var) tmp.replace(var, symbols[symbol_index]);
		factors.clearVector();
		factors.addChild(tmp);
	} else {
		if(var) tmp.replace(var, symbols[symbol_index]);
	}
	if(var) {mpoly.replace(var, symbols[symbol_index]); var->destroy();}

	vector<MathStructure> newsymbols;
	for(size_t i = 0; i < symbols.size(); i++) {
		if(i != symbol_index) newsymbols.push_back(symbols[i]);
	}

	if(newsymbols.size() > 0) {
		for(size_t i = 0; i < factors.size(); i++) {
			if(!sqrfree(factors[i], newsymbols, eo)) return false;
		}
	}

	mpoly.set(1, 1, 0);
	for(size_t i = 0; i < factors.size(); i++) {
		if(CALCULATOR->aborted()) return false;
		if(!factors[i].isOne()) {
			if(mpoly.isOne()) {
				mpoly = factors[i];
				if(i != 0) mpoly.raise(MathStructure((long int) i + 1, 1L, 0L));
			} else {
				mpoly.multiply(factors[i], true);
				mpoly[mpoly.size() - 1].raise(MathStructure((long int) i + 1, 1L, 0L));
			}
		}
	}

	if(CALCULATOR->aborted()) return false;
	if(mpoly.isZero()) {
		CALCULATOR->error(true, "mpoly is zero: %s. %s", format_and_print(tmp).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	MathStructure mquo;
	MathStructure mpoly_expand(mpoly);
	EvaluationOptions eo3 = eo;
	eo3.expand = true;
	mpoly_expand.calculatesub(eo3, eo3);

	MathStructure::polynomialQuotient(tmp, mpoly_expand, xvar, mquo, eo2);

	if(CALCULATOR->aborted()) return false;
	if(mquo.isZero()) {
		//CALCULATOR->error(true, "quo is zero: %s. %s", format_and_print(tmp).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(newsymbols.size() > 0) {
		if(!sqrfree(mquo, newsymbols, eo)) return false;
	}
	if(!mquo.isOne()) {
		mpoly.multiply(mquo, true);
	}
	if(!nlcm.isOne()) {
		nlcm.recip();
		mpoly.multiply(nlcm, true);
	}

	eo3.expand = false;
	mpoly.calculatesub(eo3, eo3, false);

	return true;

}

bool MathStructure::integerFactorize() {
	if(!isNumber()) return false;
	vector<Number> factors;
	if(!o_number.factorize(factors)) return false;
	if(factors.size() <= 1) return true;
	clear(true);
	bool b_pow = false;
	Number *lastnr = NULL;
	for(size_t i = 0; i < factors.size(); i++) {
		if(lastnr && factors[i] == *lastnr) {
			if(!b_pow) {
				LAST.raise(m_one);
				b_pow = true;
			}
			LAST[1].number()++;
		} else {
			APPEND(factors[i]);
			b_pow = false;
		}
		lastnr = &factors[i];
	}
	m_type = STRUCT_MULTIPLICATION;
	return true;
}
size_t count_powers(const MathStructure &mstruct) {
	if(mstruct.isPower()) {
		if(mstruct[1].isInteger()) {
			bool overflow = false;
			int c = mstruct.number().intValue(&overflow) - 1;
			if(overflow) return 0;
			if(c < 0) return -c;
			return c;
		}
	}
	size_t c = 0;
	for(size_t i = 0; i < mstruct.size(); i++) {
		c += count_powers(mstruct[i]);
	}
	return c;
}

bool gather_factors(const MathStructure &mstruct, const MathStructure &x_var, MathStructure &madd, MathStructure &mmul, MathStructure &mexp, bool mexp_as_x2 = false) {
	madd.clear();
	if(mexp_as_x2) mexp = m_zero;
	else mexp = m_one;
	mmul = m_zero;
	if(mstruct == x_var) {
		mmul = m_one;
		return true;
	} else if(mexp_as_x2 && mstruct.isPower()) {
		if(mstruct[1].isNumber() && mstruct[1].number().isTwo() && mstruct[0] == x_var) {
			mexp = m_one;
			return true;
		}
	} else if(!mexp_as_x2 && mstruct.isPower() && mstruct[1].isInteger() && mstruct[0] == x_var) {
		if(mstruct[0] == x_var) {
			mexp = mstruct[1];
			mmul = m_one;
			return true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() >= 2) {
		bool b_x = false;
		bool b2 = false;
		size_t i_x = 0;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b_x && mstruct[i] == x_var) {
				b_x = true;
				i_x = i;
			} else if(!b_x && mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && mstruct[i][0] == x_var) {
				b_x = true;
				b2 = true;
				i_x = i;
			} else if(!b_x && !mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isInteger() && mstruct[i][0] == x_var) {
				b_x = true;
				i_x = i;
				mexp = mstruct[i][1];
			} else if(mstruct[i].containsRepresentativeOf(x_var, true, true) != 0) {
				return false;
			}
		}
		if(!b_x) return false;
		if(mstruct.size() == 1) {
			if(b2) mexp = m_one;
			else mmul = m_one;
		} else if(mstruct.size() == 2) {
			if(b2) {
				if(i_x == 1) mexp = mstruct[0];
				else mexp = mstruct[1];
			} else {
				if(i_x == 1) mmul = mstruct[0];
				else mmul = mstruct[1];
			}
		} else {
			if(b2) {
				mexp = mstruct;
				mexp.delChild(i_x + 1, true);
			} else {
				mmul = mstruct;
				mmul.delChild(i_x + 1, true);
			}
		}
		return true;
	} else if(mstruct.isAddition()) {
		mmul.setType(STRUCT_ADDITION);
		if(mexp_as_x2) mexp.setType(STRUCT_ADDITION);
		madd.setType(STRUCT_ADDITION);
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i] == x_var) {
				if(mexp_as_x2 || mexp.isOne()) mmul.addChild(m_one);
				else return false;
			} else if(mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && mstruct[i][0] == x_var) {
				mexp.addChild(m_one);
			} else if(!mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isInteger() && mstruct[i][0] == x_var) {
				if(mmul.size() == 0) {
					mexp = mstruct[i][1];
				} else if(mexp != mstruct[i][1]) {
					return false;
				}
				mmul.addChild(m_one);
			} else if(mstruct[i].isMultiplication()) {
				bool b_x = false;
				bool b2 = false;
				size_t i_x = 0;
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(!b_x && mstruct[i][i2] == x_var) {
						if(!mexp_as_x2 && !mexp.isOne()) return false;
						i_x = i2;
						b_x = true;
					} else if(!b_x && mexp_as_x2 && mstruct[i][i2].isPower() && mstruct[i][i2][1].isNumber() && mstruct[i][i2][1].number().isTwo() && mstruct[i][i2][0] == x_var) {
						b2 = true;
						i_x = i2;
						b_x = true;
					} else if(!b_x && !mexp_as_x2 && mstruct[i][i2].isPower() && mstruct[i][i2][1].isInteger() && mstruct[i][i2][0] == x_var) {
						if(mmul.size() == 0) {
							mexp = mstruct[i][i2][1];
						} else if(mexp != mstruct[i][i2][1]) {
							return false;
						}
						i_x = i2;
						b_x = true;
					} else if(mstruct[i][i2].containsRepresentativeOf(x_var, true, true) != 0) {
						return false;
					}
				}
				if(b_x) {
					if(mstruct[i].size() == 1) {
						if(b2) mexp.addChild(m_one);
						else mmul.addChild(m_one);
					} else {
						if(b2) {
							mexp.addChild(mstruct[i]);
							mexp[mexp.size() - 1].delChild(i_x + 1, true);
							mexp.childUpdated(mexp.size());
						} else {
							mmul.addChild(mstruct[i]);
							mmul[mmul.size() - 1].delChild(i_x + 1, true);
							mmul.childUpdated(mmul.size());
						}
					}
				} else {
					madd.addChild(mstruct[i]);
				}
			} else if(mstruct[i].containsRepresentativeOf(x_var, true, true) != 0) {
				return false;
			} else {
				madd.addChild(mstruct[i]);
			}
		}
		if(mmul.size() == 0 && (!mexp_as_x2 || mexp.size() == 0)) {
			mmul.clear();
			if(mexp_as_x2) mexp.clear();
			return false;
		}
		if(mmul.size() == 0) mmul.clear();
		else if(mmul.size() == 1) mmul.setToChild(1);
		if(mexp_as_x2) {
			if(mexp.size() == 0) mexp.clear();
			else if(mexp.size() == 1) mexp.setToChild(1);
		}
		if(madd.size() == 0) madd.clear();
		else if(madd.size() == 1) madd.setToChild(1);
		return true;
	}
	return false;
}

bool MathStructure::factorize(const EvaluationOptions &eo_pre, bool unfactorize, int term_combination_levels, int max_msecs, bool only_integers, int recursive, struct timeval *endtime_p, const MathStructure &force_factorization, bool complete_square, bool only_sqrfree, int max_factor_degree) {

	if(CALCULATOR->aborted()) return false;
	struct timeval endtime;
	if(max_msecs > 0 && !endtime_p) {
#ifndef CLOCK_MONOTONIC
		gettimeofday(&endtime, NULL);
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		endtime.tv_sec = ts.tv_sec;
		endtime.tv_usec = ts.tv_nsec / 1000;
#endif
		endtime.tv_sec += max_msecs / 1000;
		long int usecs = endtime.tv_usec + (long int) (max_msecs % 1000) * 1000;
		if(usecs >= 1000000) {
			usecs -= 1000000;
			endtime.tv_sec++;
		}
		endtime.tv_usec = usecs;
		max_msecs = 0;
		endtime_p = &endtime;
	}

	EvaluationOptions eo = eo_pre;
	eo.sync_units = false;
	eo.structuring = STRUCTURING_NONE;
	if(unfactorize) {
		unformat(eo_pre);
		EvaluationOptions eo2 = eo;
		eo2.expand = true;
		eo2.combine_divisions = false;
		eo2.sync_units = false;
		calculatesub(eo2, eo2);
		do_simplification(*this, eo, true, false, true);
	} else if(term_combination_levels && isAddition()) {
		MathStructure *mdiv = new MathStructure;
		mdiv->setType(STRUCT_ADDITION);
		for(size_t i = 0; i < SIZE; ) {
			bool b = false;
			if(CHILD(i).isMultiplication()) {
				for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
					if(CHILD(i)[i2].isPower() && CHILD(i)[i2][1].hasNegativeSign()) {
						b = true;
						break;
					}
				}
			} else if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
				b = true;
			}
			if(b) {
				CHILD(i).ref();
				mdiv->addChild_nocopy(&CHILD(i));
				ERASE(i)
			} else {
				i++;
			}
		}
		if(mdiv->size() > 0) {
			bool b_ret = false;
			if(SIZE == 1 && recursive) {
				b_ret = CHILD(0).factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
			} else if(SIZE > 1) {
				b_ret = factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
			}
			if(mdiv->size() > 1) {
				// 5/y + x/y + z = (5 + x)/y + z
				MathStructure mstruct_units(*mdiv);
				MathStructure mstruct_new(*mdiv);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!mstruct_units[i][i2].isPower() || !mstruct_units[i][i2][1].hasNegativeSign()) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(mstruct_new[i][i2].isPower() && mstruct_new[i][i2][1].hasNegativeSign()) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(mstruct_new[i].isPower() && mstruct_new[i][1].hasNegativeSign()) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				bool b = false;
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) {
						mdiv->set_nocopy(mstruct_new[0], true);
					} else {
						mdiv->set_nocopy(mstruct_new);
					}
					b_ret = true;
				}
			}
			size_t index = 1;
			if(isAddition()) index = SIZE;
			if(index == 0) {
				set_nocopy(*mdiv);
			} else if(mdiv->isAddition()) {
				for(size_t i = 0; i < mdiv->size(); i++) {
					(*mdiv)[i].ref();
					add_nocopy(&(*mdiv)[i], true);
				}
				mdiv->unref();
			} else {
				add_nocopy(mdiv, true);
			}
			if(recursive) {
				for(; index < SIZE; index++) {
					b_ret = CHILD(index).factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree) || b_ret;
				}
			}
			return b_ret;
		}
		mdiv->unref();
	}

	MathStructure mden, mnum;
	evalSort(true);

	if(term_combination_levels >= -1 && isAddition() && isRationalPolynomial()) {
		MathStructure msqrfree(*this);
		eo.protected_function = CALCULATOR->f_signum;
		if(sqrfree(msqrfree, eo)) {
			if((!only_sqrfree || msqrfree.isPower()) && !equals(msqrfree) && (!msqrfree.isMultiplication() || msqrfree.size() != 2 || (!(msqrfree[0].isNumber() && msqrfree[1].isAddition()) && !(msqrfree[1].isNumber() && msqrfree[0].isAddition())))) {
				MathStructure mcopy(msqrfree);
				EvaluationOptions eo2 = eo;
				eo2.expand = true;
				eo2.calculate_functions = false;
				CALCULATOR->beginTemporaryStopMessages();
				mcopy.calculatesub(eo2, eo2);
				CALCULATOR->endTemporaryStopMessages();
				bool b_equal = equals(mcopy);
				if(!b_equal && !CALCULATOR->aborted()) {
					MathStructure mcopy2(*this);
					CALCULATOR->beginTemporaryStopMessages();
					mcopy.calculatesub(eo2, eo2, true);
					mcopy2.calculatesub(eo2, eo2, true);
					CALCULATOR->endTemporaryStopMessages();
					b_equal = mcopy.equals(mcopy2);
				}
				if(!b_equal) {
					eo.protected_function = eo_pre.protected_function;
					if(CALCULATOR->aborted()) return false;
					CALCULATOR->error(true, "factorized result is wrong: %s != %s. %s", format_and_print(msqrfree).c_str(), format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
				} else {
					eo.protected_function = eo_pre.protected_function;
					set(msqrfree);
					if(!isAddition()) {
						if(isMultiplication()) flattenMultiplication(*this);
						if(isMultiplication() && SIZE >= 2 && CHILD(0).isNumber()) {
							for(size_t i = 1; i < SIZE; i++) {
								if(CHILD(i).isNumber()) {
									CHILD(i).number() *= CHILD(0).number();
									CHILD(0).set(CHILD(i));
									delChild(i);
								} else if(CHILD(i).isPower() && CHILD(i)[0].isMultiplication() && CHILD(i)[0].size() >= 2 && CHILD(i)[0][0].isNumber() && CHILD(i)[0][0].number().isRational() && !CHILD(i)[0][0].number().isInteger() && CHILD(i)[1].isInteger()) {
									CHILD(i)[0][0].number().raise(CHILD(i)[1].number());
									CHILD(0).number().multiply(CHILD(i)[0][0].number());
									CHILD(i)[0].delChild(1);
									if(CHILD(i)[0].size() == 1) CHILD(i)[0].setToChild(1, true);
								}
							}
							if(SIZE > 1 && CHILD(0).isOne()) {
								ERASE(0);
							}
							if(SIZE == 1) SET_CHILD_MAP(0);
						}
						if(isMultiplication() && SIZE >= 2 && CHILD(0).isNumber() && CHILD(0).number().isRational() && !CHILD(0).number().isInteger()) {
							Number den = CHILD(0).number().denominator();
							for(size_t i = 1; i < SIZE; i++) {
								if(CHILD(i).isAddition()) {
									bool b = true;
									for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
										if(CHILD(i)[i2].isNumber()) {
											if(!CHILD(i)[i2].number().isIntegerDivisible(den)) {b = false; break;}
										} else if(CHILD(i)[i2].isMultiplication() && CHILD(i)[i2][0].isNumber()) {
											if(!CHILD(i)[i2][0].number().isIntegerDivisible(den)) {b = false; break;}
										} else {
											b = false;
											break;
										}
									}
									if(b) {
										for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
											if(CHILD(i)[i2].isNumber()) {
												CHILD(i)[i2].number().divide(den);
											} else if(CHILD(i)[i2].isMultiplication()) {
												CHILD(i)[i2][0].number().divide(den);
												if(CHILD(i)[i2][0].isOne() && CHILD(i)[i2].size() > 1) {
													CHILD(i)[i2].delChild(1);
													if(CHILD(i)[i2].size() == 1) {
														CHILD(i)[i2].setToChild(1, true);
													}
												}
											}
										}
										CHILD(0).set(CHILD(0).number().numerator(), true);
										if(SIZE > 1 && CHILD(0).isOne()) {
											ERASE(0);
										}
										if(SIZE == 1) SET_CHILD_MAP(0);
										break;
									}
								}
							}
						}
						if(isMultiplication()) {
							for(size_t i = 0; i < SIZE; i++) {
								if(CHILD(i).isPower() && CHILD(i)[1].isInteger()) {
									if(CHILD(i)[0].isAddition()) {
										bool b = true;
										for(size_t i2 = 0; i2 < CHILD(i)[0].size(); i2++) {
											if((!CHILD(i)[0][i2].isNumber() || !CHILD(i)[0][i2].number().isNegative()) && (!CHILD(i)[0][i2].isMultiplication() || CHILD(i)[0][i2].size() < 2 || !CHILD(i)[0][i2][0].isNumber() || !CHILD(i)[0][i2][0].number().isNegative())) {
												b = false;
												break;
											}
										}
										if(b) {
											for(size_t i2 = 0; i2 < CHILD(i)[0].size(); i2++) {
												if(CHILD(i)[0][i2].isNumber()) {
													CHILD(i)[0][i2].number().negate();
												} else {
													CHILD(i)[0][i2][0].number().negate();
													if(CHILD(i)[0][i2][0].isOne() && CHILD(i)[0][i2].size() > 1) {
														CHILD(i)[0][i2].delChild(1);
														if(CHILD(i)[0][i2].size() == 1) {
															CHILD(i)[0][i2].setToChild(1, true);
														}
													}
												}
											}
											if(CHILD(i)[1].number().isOdd()) {
												if(CHILD(0).isNumber()) CHILD(0).number().negate();
												else {
													PREPEND(MathStructure(-1, 1, 0));
													i++;
												}
											}
										}
									} else if(CHILD(i)[0].isMultiplication() && CHILD(i)[0].size() >= 2 && CHILD(i)[0][0].isNumber() && CHILD(i)[0][0].number().isNegative()) {
										CHILD(i)[0][0].number().negate();
										if(CHILD(i)[0][0].isOne() && CHILD(i)[0].size() > 1) {
											CHILD(i)[0].delChild(1);
											if(CHILD(i)[0].size() == 1) {
												CHILD(i)[0].setToChild(1, true);
											}
										}
										if(CHILD(i)[1].number().isOdd()) {
											if(CHILD(0).isNumber()) CHILD(0).number().negate();
											else {
												PREPEND(MathStructure(-1, 1, 0));
												i++;
											}
										}
									}
								} else if(CHILD(i).isAddition()) {
									bool b = true;
									for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
										if((!CHILD(i)[i2].isNumber() || !CHILD(i)[i2].number().isNegative()) && (!CHILD(i)[i2].isMultiplication() || CHILD(i)[i2].size() < 2 || !CHILD(i)[i2][0].isNumber() || !CHILD(i)[i2][0].number().isNegative())) {
											b = false;
											break;
										}
									}
									if(b) {
										for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
											if(CHILD(i)[i2].isNumber()) {
												CHILD(i)[i2].number().negate();
											} else {
												CHILD(i)[i2][0].number().negate();
												if(CHILD(i)[i2][0].isOne() && CHILD(i)[i2].size() > 1) {
													CHILD(i)[i2].delChild(1);
													if(CHILD(i)[i2].size() == 1) {
														CHILD(i)[i2].setToChild(1, true);
													}
												}
											}
										}
										if(CHILD(0).isNumber()) CHILD(0).number().negate();
										else {
											PREPEND(MathStructure(-1, 1, 0));
											i++;
										}
									}
								}
							}
							if(SIZE > 1 && CHILD(0).isOne()) {
								ERASE(0);
							}
							if(SIZE == 1) SET_CHILD_MAP(0);
						}
						if(isPower() && CHILD(1).isInteger()) {
							if(CHILD(0).isAddition()) {
								bool b = true;
								for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
									if((!CHILD(0)[i2].isNumber() || !CHILD(0)[i2].number().isNegative()) && (!CHILD(0)[i2].isMultiplication() || CHILD(0)[i2].size() < 2 || !CHILD(0)[i2][0].isNumber() || !CHILD(0)[i2][0].number().isNegative())) {
										b = false;
										break;
									}
								}
								if(b) {
									for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
										if(CHILD(0)[i2].isNumber()) {
											CHILD(0)[i2].number().negate();
										} else {
											CHILD(0)[i2][0].number().negate();
											if(CHILD(0)[i2][0].isOne() && CHILD(0)[i2].size() > 1) {
												CHILD(0)[i2].delChild(1);
												if(CHILD(0)[i2].size() == 1) {
													CHILD(0)[i2].setToChild(1, true);
												}
											}
										}
									}
									if(CHILD(1).number().isOdd()) {
										multiply(MathStructure(-1, 1, 0));
										CHILD_TO_FRONT(1)
									}
								}
							} else if(CHILD(0).isMultiplication() && CHILD(0).size() >= 2 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isNegative()) {
								CHILD(0)[0].number().negate();
								if(CHILD(0)[0].isOne() && CHILD(0).size() > 1) {
									CHILD(0).delChild(1);
									if(CHILD(0).size() == 1) {
										CHILD(0).setToChild(1, true);
									}
								}
								if(CHILD(1).number().isOdd()) {
									multiply(MathStructure(-1, 1, 0));
									CHILD_TO_FRONT(1)
								}
							}
						}
					}
					evalSort(true);
					factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					return true;
				}
			}
		}
		eo.protected_function = eo_pre.protected_function;
	}

	switch(type()) {
		case STRUCT_ADDITION: {

			if(CALCULATOR->aborted()) return false;
			if(term_combination_levels >= -1 && !only_sqrfree && max_factor_degree != 0) {

				if(SIZE <= 3 && SIZE > 1) {
					MathStructure *xvar = NULL;
					Number nr2(1, 1);
					if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
						xvar = &CHILD(0)[0];
					} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber()) {
						if(CHILD(0)[1].isPower()) {
							if(CHILD(0)[1][0].size() == 0 && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isTwo()) {
								xvar = &CHILD(0)[1][0];
								nr2.set(CHILD(0)[0].number());
							}
						}
					}
					if(xvar) {
						bool factorable = false;
						Number nr1, nr0;
						if(SIZE == 2 && CHILD(1).isNumber()) {
							factorable = true;
							nr0 = CHILD(1).number();
						} else if(SIZE == 3 && CHILD(2).isNumber()) {
							nr0 = CHILD(2).number();
							if(CHILD(1).isMultiplication()) {
								if(CHILD(1).size() == 2 && CHILD(1)[0].isNumber() && xvar->equals(CHILD(1)[1])) {
									nr1 = CHILD(1)[0].number();
									factorable = true;
								}
							} else if(xvar->equals(CHILD(1))) {
								nr1.set(1, 1, 0);
								factorable = true;
							}
						}
						if(factorable) {
							Number nr4ac(4, 1, 0);
							nr4ac *= nr2;
							nr4ac *= nr0;
							Number nr2a(2, 1, 0);
							nr2a *= nr2;
							Number sqrtb24ac(nr1);
							sqrtb24ac.raise(nr_two);
							sqrtb24ac -= nr4ac;
							if(sqrtb24ac.isNegative()) factorable = false;
							MathStructure mstructb24(sqrtb24ac);
							if(factorable) {
								if(!only_integers) {
									if(eo.approximation == APPROXIMATION_EXACT && !sqrtb24ac.isApproximate()) {
										sqrtb24ac.raise(nr_half);
										if(sqrtb24ac.isApproximate()) {
											mstructb24.raise(nr_half);
										} else {
											mstructb24.set(sqrtb24ac);
										}
									} else {
										mstructb24.number().raise(nr_half);
									}
								} else {
									mstructb24.number().raise(nr_half);
									if((!sqrtb24ac.isApproximate() && mstructb24.number().isApproximate()) || (sqrtb24ac.isInteger() && !mstructb24.number().isInteger())) {
										factorable = false;
									}
								}
							}
							if(factorable) {
								MathStructure m1(nr1), m2(nr1);
								Number mul1(1, 1), mul2(1, 1);
								if(mstructb24.isNumber()) {
									m1.number() += mstructb24.number();
									m1.number() /= nr2a;
									if(m1.number().isRational() && !m1.number().isInteger()) {
										mul1 = m1.number().denominator();
										m1.number() *= mul1;
									}
									m2.number() -= mstructb24.number();
									m2.number() /= nr2a;
									if(m2.number().isRational() && !m2.number().isInteger()) {
										mul2 = m2.number().denominator();
										m2.number() *= mul2;
									}
								} else {
									m1.calculateAdd(mstructb24, eo);
									m1.calculateDivide(nr2a, eo);
									if(m1.isNumber()) {
										if(m1.number().isRational() && !m1.number().isInteger()) {
											mul1 = m1.number().denominator();
											m1.number() *= mul1;
										}
									} else {
										bool bint = false, bfrac = false;
										idm1(m1, bfrac, bint);
										if(bfrac) {
											idm2(m1, bfrac, bint, mul1);
											idm3(m1, mul1, true);
										}
									}
									m2.calculateSubtract(mstructb24, eo);
									m2.calculateDivide(nr2a, eo);
									if(m2.isNumber()) {
										if(m2.number().isRational() && !m2.number().isInteger()) {
											mul2 = m2.number().denominator();
											m2.number() *= mul2;
										}
									} else {
										bool bint = false, bfrac = false;
										idm1(m2, bfrac, bint);
										if(bfrac) {
											idm2(m2, bfrac, bint, mul2);
											idm3(m2, mul2, true);
										}
									}
								}
								nr2 /= mul1;
								nr2 /= mul2;
								if(m1 == m2 && mul1 == mul2) {
									MathStructure xvar2(*xvar);
									if(!mul1.isOne()) xvar2 *= mul1;
									set(m1);
									add(xvar2, true);
									raise(MathStructure(2, 1, 0));
									if(!nr2.isOne()) {
										multiply(nr2);
									}
								} else {
									m1.add(*xvar, true);
									if(!mul1.isOne()) m1[m1.size() - 1] *= mul1;
									m2.add(*xvar, true);
									if(!mul2.isOne()) m2[m2.size() - 1] *= mul2;
									clear(true);
									m_type = STRUCT_MULTIPLICATION;
									if(!nr2.isOne()) {
										APPEND_NEW(nr2);
									}
									APPEND(m1);
									APPEND(m2);
								}
								EvaluationOptions eo2 = eo;
								eo2.expand = false;
								calculatesub(eo2, eo2, false);
								evalSort(true);
								return true;
							}
						}
					}
				}

				MathStructure *factor_mstruct = new MathStructure(1, 1, 0);
				MathStructure mnew;
				if(factorize_find_multiplier(*this, mnew, *factor_mstruct) && !factor_mstruct->isZero() && !mnew.isZero()) {
					mnew.factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					factor_mstruct->factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					clear(true);
					m_type = STRUCT_MULTIPLICATION;
					APPEND_REF(factor_mstruct);
					APPEND(mnew);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					calculatesub(eo2, eo2, false);
					factor_mstruct->unref();
					evalSort(true);
					return true;
				}
				factor_mstruct->unref();

				if(SIZE > 1 && CHILD(SIZE - 1).isNumber() && CHILD(SIZE - 1).number().isInteger() && max_factor_degree != 0) {
					MathStructure *xvar = NULL;
					Number qnr(1, 1);
					int degree = 1;
					bool overflow = false;
					int qcof = 1;
					if(CHILD(0).isPower() && !CHILD(0)[0].isNumber() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isInteger() && CHILD(0)[1].number().isPositive()) {
						xvar = &CHILD(0)[0];
						degree = CHILD(0)[1].number().intValue(&overflow);
					} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isInteger()) {
						if(CHILD(0)[1].isPower()) {
							if(CHILD(0)[1][0].size() == 0 && !CHILD(0)[1][0].isNumber() && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isInteger() && CHILD(0)[1][1].number().isPositive()) {
								xvar = &CHILD(0)[1][0];
								qcof = CHILD(0)[0].number().intValue(&overflow);
								if(!overflow) {
									if(qcof < 0) qcof = -qcof;
									degree = CHILD(0)[1][1].number().intValue(&overflow);
								}
							}
						}
					}

					int pcof = 1;
					if(!overflow) {
						pcof = CHILD(SIZE - 1).number().intValue(&overflow);
						if(pcof < 0) pcof = -pcof;
					}

					if(xvar && !overflow && degree <= 1000 && degree > 2 && qcof != 0 && pcof != 0) {
						bool b = true, b2 = true;
						for(size_t i = 1; b && i < SIZE - 1; i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {
									b = false;
									break;
								}
								case STRUCT_POWER: {
									if(!CHILD(i)[1].isNumber() || !xvar->equals(CHILD(i)[0]) || !CHILD(i)[1].number().isInteger() || !CHILD(i)[1].number().isPositive()) {
										b = false;
									}
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(!(CHILD(i).size() == 2) || !CHILD(i)[0].isNumber()) {
										b = false;
									} else if(CHILD(i)[1].isPower()) {
										if(!CHILD(i)[1][1].isNumber() || !xvar->equals(CHILD(i)[1][0]) || !CHILD(i)[1][1].number().isInteger() || !CHILD(i)[1][1].number().isPositive()) {
											b = false;
										}
									} else if(!xvar->equals(CHILD(i)[1])) {
										b = false;
									}
									if(b && b2 && !CHILD(i)[0].isInteger()) b2 = false;
									break;
								}
								default: {
									if(!xvar->equals(CHILD(i))) {
										b = false;
									}
								}
							}
						}

						if(b) {
							vector<Number> factors;
							factors.resize(degree + 1, Number());
							factors[0] = CHILD(SIZE - 1).number();
							vector<int> ps;
							vector<int> qs;
							vector<Number> zeroes;
							int curdeg = 1, prevdeg = 0;
							for(size_t i = 0; b && i < SIZE - 1; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_POWER: {
										curdeg = CHILD(i)[1].number().intValue(&overflow);
										if(curdeg == prevdeg || curdeg > degree || (prevdeg > 0 && curdeg > prevdeg) || overflow) {
											b = false;
										} else {
											factors[curdeg].set(1, 1, 0);
										}
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(CHILD(i)[1].isPower()) {
											curdeg = CHILD(i)[1][1].number().intValue(&overflow);
										} else {
											curdeg = 1;
										}
										if(curdeg == prevdeg || curdeg > degree || (prevdeg > 0 && curdeg > prevdeg) || overflow) {
											b = false;
										} else {
											factors[curdeg] = CHILD(i)[0].number();
										}
										break;
									}
									default: {
										curdeg = 1;
										factors[curdeg].set(1, 1, 0);
									}
								}
								prevdeg = curdeg;
							}

							while(b && degree > 2) {
								for(int i = 1; i <= 1000; i++) {
									if(i > pcof) break;
									if(pcof % i == 0) ps.push_back(i);
								}
								for(int i = 1; i <= 1000; i++) {
									if(i > qcof) break;
									if(qcof % i == 0) qs.push_back(i);
								}
								Number itest;
								int i2;
								size_t pi = 0, qi = 0;
								if(ps.empty() || qs.empty()) break;
								Number nrtest(ps[0], qs[0], 0);
								while(true) {
									itest.clear(); i2 = degree;
									while(true) {
										itest += factors[i2];
										if(i2 == 0) break;
										itest *= nrtest;
										i2--;
									}
									if(itest.isZero()) {
										break;
									}
									if(nrtest.isPositive()) {
										nrtest.negate();
									} else {
										qi++;
										if(qi == qs.size()) {
											qi = 0;
											pi++;
											if(pi == ps.size()) {
												break;
											}
										}
										nrtest.set(ps[pi], qs[qi], 0);
									}
								}
								if(itest.isZero()) {
									itest.clear(); i2 = degree;
									Number ntmp(factors[i2]);
									for(; i2 > 0; i2--) {
										itest += ntmp;
										ntmp = factors[i2 - 1];
										factors[i2 - 1] = itest;
										itest *= nrtest;
									}
									degree--;
									nrtest.negate();
									zeroes.push_back(nrtest);
									if(degree == 2) {
										break;
									}
									qcof = factors[degree].intValue(&overflow);
									if(!overflow) {
										if(qcof < 0) qcof = -qcof;
										pcof = factors[0].intValue(&overflow);
										if(!overflow) {
											if(pcof < 0) pcof = -pcof;
										}
									}
									if(overflow || qcof == 0 || pcof == 0) {
										break;
									}
								} else {
									break;
								}
								ps.clear();
								qs.clear();
							}

							if(zeroes.size() > 0) {
								MathStructure mleft;
								MathStructure mtmp;
								MathStructure *mcur;
								for(int i = degree; i >= 0; i--) {
									if(!factors[i].isZero()) {
										if(mleft.isZero()) {
											mcur = &mleft;
										} else {
											mleft.add(m_zero, true);
											mcur = &mleft[mleft.size() - 1];
										}
										if(i > 1) {
											if(!factors[i].isOne()) {
												mcur->multiply(*xvar);
												(*mcur)[0].set(factors[i]);
												mcur = &(*mcur)[1];
											} else {
												mcur->set(*xvar);
											}
											mtmp.set(i, 1, 0);
											mcur->raise(mtmp);
										} else if(i == 1) {
											if(!factors[i].isOne()) {
												mcur->multiply(*xvar);
												(*mcur)[0].set(factors[i]);
											} else {
												mcur->set(*xvar);
											}
										} else {
											mcur->set(factors[i]);
										}
									}
								}
								mleft.factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
								vector<long int> powers;
								vector<size_t> powers_i;
								int dupsfound = 0;
								for(size_t i = 0; i < zeroes.size() - 1; i++) {
									while(i + 1 < zeroes.size() && zeroes[i] == zeroes[i + 1]) {
										dupsfound++;
										zeroes.erase(zeroes.begin() + (i + 1));
									}
									if(dupsfound > 0) {
										powers_i.push_back(i);
										powers.push_back(dupsfound + 1);
										dupsfound = 0;
									}
								}
								MathStructure xvar2(*xvar);
								Number *nrmul;
								if(mleft.isMultiplication()) {
									set(mleft);
									evalSort();
									if(CHILD(0).isNumber()) {
										nrmul = &CHILD(0).number();
									} else if(CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber()) {
										nrmul = &CHILD(0)[0].number();
									} else {
										PREPEND(m_one);
										nrmul = &CHILD(0).number();
									}
								} else {
									clear(true);
									m_type = STRUCT_MULTIPLICATION;
									APPEND(m_one);
									APPEND(mleft);
									nrmul = &CHILD(0).number();
								}
								size_t pi = 0;
								for(size_t i = 0; i < zeroes.size(); i++) {
									if(zeroes[i].isInteger()) {
										APPEND(xvar2);
									} else {
										APPEND(m_zero);
									}
									mcur = &CHILD(SIZE - 1);
									if(pi < powers_i.size() && powers_i[pi] == i) {
										mcur->raise(MathStructure(powers[pi], 1L, 0L));
										mcur = &(*mcur)[0];
										if(zeroes[i].isInteger()) {
											mcur->add(zeroes[i]);
										} else {
											Number nr(zeroes[i].denominator());
											mcur->add(zeroes[i].numerator());
											(*mcur)[0] *= xvar2;
											(*mcur)[0][0].number() = nr;
											nr.raise(powers[pi]);
											nrmul->divide(nr);
										}
										pi++;
									} else {
										if(zeroes[i].isInteger()) {
											mcur->add(zeroes[i]);
										} else {
											nrmul->divide(zeroes[i].denominator());
											mcur->add(zeroes[i].numerator());
											(*mcur)[0] *= xvar2;
											(*mcur)[0][0].number() = zeroes[i].denominator();
										}
									}
								}
								if(CHILD(0).isNumber() && CHILD(0).number().isOne()) {
									ERASE(0);
								} else if(CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isOne()) {
									if(CHILD(0).size() == 1) {
										ERASE(0);
									} else if(CHILD(0).size() == 2) {
										CHILD(0).setToChild(2, true);
									} else {
										CHILD(0).delChild(1);
									}
								}
								evalSort(true);
								Number dupspow;
								for(size_t i = 0; i < SIZE - 1; i++) {
									mcur = NULL;
									if(CHILD(i).isPower()) {
										if(CHILD(i)[0].isAddition() && CHILD(i)[1].isNumber()) {
											mcur = &CHILD(i)[0];
										}
									} else if(CHILD(i).isAddition()) {
										mcur = &CHILD(i);
									}
									while(mcur && i + 1 < SIZE) {
										if(CHILD(i + 1).isPower()) {
											if(CHILD(i + 1)[0].isAddition() && CHILD(i + 1)[1].isNumber() && mcur->equals(CHILD(i + 1)[0])) {
												dupspow += CHILD(i + 1)[1].number();
											} else {
												mcur = NULL;
											}
										} else if(CHILD(i + 1).isAddition() && mcur->equals(CHILD(i + 1))) {
											dupspow++;
										} else {
											mcur = NULL;
										}
										if(mcur) {
											ERASE(i + 1);
										}
									}
									if(!dupspow.isZero()) {
										if(CHILD(i).isPower()) {
											CHILD(i)[1].number() += dupspow;
										} else {
											dupspow++;
											CHILD(i) ^= dupspow;
										}
										dupspow.clear();
									}
								}
								if(SIZE == 1) {
									setToChild(1, true);
								} else {
									EvaluationOptions eo2 = eo;
									eo2.expand = false;
									calculatesub(eo2, eo2, false);
								}
								evalSort(true);
								return true;
							}
						}

						if(b && b2 && (max_factor_degree < 0 || max_factor_degree >= 2) && degree > 3 && degree < 50) {
							// Kronecker method
							vector<Number> vnum;
							vnum.resize(degree + 1, nr_zero);
							bool overflow = false;
							for(size_t i = 0; b && i < SIZE; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_POWER: {
										if(CHILD(i)[0] == *xvar && CHILD(i)[1].isInteger()) {
											int curdeg = CHILD(i)[1].number().intValue(&overflow);
											if(curdeg < 0 || overflow || curdeg > degree) b = false;
											else vnum[curdeg] += 1;
										} else {
											b = false;
										}
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(CHILD(i).size() == 2 && CHILD(i)[0].isInteger()) {
											long int icoeff = CHILD(i)[0].number().intValue(&overflow);
											if(!overflow && CHILD(i)[1].isPower() && CHILD(i)[1][0] == *xvar && CHILD(i)[1][1].isInteger()) {
												int curdeg = CHILD(i)[1][1].number().intValue(&overflow);
												if(curdeg < 0 || overflow || curdeg > degree) b = false;
												else vnum[curdeg] += icoeff;
											} else if(!overflow && CHILD(i)[1] == *xvar) {
												vnum[1] += icoeff;
											} else {
												b = false;
											}
										} else {
											b = false;
										}
										break;
									}
									default: {
										if(CHILD(i).isInteger()) {
											long int icoeff = CHILD(i).number().intValue(&overflow);
											if(overflow) b = false;
											else vnum[0] += icoeff;
										} else if(CHILD(i) == *xvar) {
											vnum[1] += 1;
										} else {
											b = false;
										}
										break;
									}
								}
							}

							long int lcoeff = vnum[degree].lintValue();
							vector<int> vs;
							if(b && lcoeff != 0) {
								degree /= 2;
								if(max_factor_degree > 0 && degree > max_factor_degree) degree = max_factor_degree;
								for(int i = 0; i <= degree; i++) {
									if(CALCULATOR->aborted()) return false;
									MathStructure mcalc(*this);
									mcalc.calculateReplace(*xvar, Number((i / 2 + i % 2) * (i % 2 == 0 ? -1 : 1), 1), eo);
									mcalc.calculatesub(eo, eo, false);
									if(!mcalc.isInteger()) break;
									bool overflow = false;
									int v = ::abs(mcalc.number().intValue(&overflow));
									if(overflow) {
										if(i > 2) degree = i;
										else b = false;
										break;
									}
									vs.push_back(v);
								}
							}

							if(b) {
								vector<int> factors0, factorsl;
								factors0.push_back(1);
								for(int i = 2; i < vs[0] / 3 && i < 1000; i++) {
									if(vs[0] % i == 0) factors0.push_back(i);
								}
								if(vs[0] % 3 == 0) factors0.push_back(vs[0] / 3);
								if(vs[0] % 2 == 0) factors0.push_back(vs[0] / 2);
								factors0.push_back(vs[0]);
								for(int i = 2; i < lcoeff / 3 && i < 1000; i++) {
									if(lcoeff % i == 0) factorsl.push_back(i);
								}
								factorsl.push_back(1);
								if(lcoeff % 3 == 0) factorsl.push_back(lcoeff / 3);
								if(lcoeff % 2 == 0) factorsl.push_back(lcoeff / 2);
								factorsl.push_back(lcoeff);

								long long int cmax = 500000LL / (factors0.size() * factorsl.size());
								if(term_combination_levels != 0) cmax *= 10;
								if(degree >= 2 && cmax > 10) {
									vector<Number> vden;
									vector<Number> vquo;
									vden.resize(3, nr_zero);
									long int c0;
									for(size_t i = 0; i < factors0.size() * 2; i++) {
										c0 = factors0[i / 2];
										if(i % 2 == 1) c0 = -c0;
										long int c2;
										for(size_t i2 = 0; i2 < factorsl.size(); i2++) {
											c2 = factorsl[i2];
											long int c1max = vs[1] - c0 - c2, c1min;
											if(c1max < 0) {c1min = c1max; c1max = -vs[1] - c0 - c2;}
											else {c1min = -vs[1] - c0 - c2;}
											if(-(vs[2] - c0 - c2) < -(-vs[2] - c0 - c2)) {
												if(c1max > -(-vs[2] - c0 - c2)) c1max = -(-vs[2] - c0 - c2);
												if(c1min < -(vs[2] - c0 - c2)) c1min = -(vs[2] - c0 - c2);
											} else {
												if(c1max > -(vs[2] - c0 - c2)) c1max = -(vs[2] - c0 - c2);
												if(c1min < -(-vs[2] - c0 - c2)) c1min = -(-vs[2] - c0 - c2);
											}
											if(c1min < -cmax / 2) c1min = -cmax / 2;
											for(long int c1 = c1min; c1 <= c1max && c1 <= cmax / 2; c1++) {
												long int v1 = ::labs(c2 + c1 + c0);
												long int v2 = ::labs(c2 - c1 + c0);
												if(v1 != 0 && v2 != 0 && v1 <= vs[1] && v2 <= vs[2] && (c1 != 0 || c2 != 0) && vs[1] % v1 == 0 && vs[2] % v2 == 0) {
													vden[0] = c0; vden[1] = c1; vden[2] = c2;
													if(CALCULATOR->aborted()) return false;
													if(polynomial_divide_integers(vnum, vden, vquo)) {
														MathStructure mtest;
														mtest.setType(STRUCT_ADDITION);
														if(c2 != 0) {
															MathStructure *mpow = new MathStructure();
															mpow->setType(STRUCT_POWER);
															mpow->addChild(*xvar);
															mpow->addChild_nocopy(new MathStructure(2, 1, 0));
															if(c2 == 1) {
																mtest.addChild_nocopy(mpow);
															} else {
																MathStructure *mterm = new MathStructure();
																mterm->setType(STRUCT_MULTIPLICATION);
																mterm->addChild_nocopy(new MathStructure(c2, 1L, 0L));
																mterm->addChild_nocopy(mpow);
																mtest.addChild_nocopy(mterm);
															}
														}
														if(c1 == 1) {
															mtest.addChild(*xvar);
														} else if(c1 != 0) {
															MathStructure *mterm = new MathStructure();
															mterm->setType(STRUCT_MULTIPLICATION);
															mterm->addChild_nocopy(new MathStructure(c1, 1L, 0L));
															mterm->addChild(*xvar);
															mtest.addChild_nocopy(mterm);
														}
														mtest.addChild_nocopy(new MathStructure(c0, 1L, 0L));
														MathStructure mthis(*this);
														MathStructure mquo;
														if(mtest.size() > 1 && polynomialDivide(mthis, mtest, mquo, eo, false)) {
															mquo.factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
															set(mquo, true);
															multiply(mtest, true);
															return true;
														}
													}
												}
											}
										}
									}
								}
								for(int i_d = 3; i_d <= degree; i_d++) {
									if(CALCULATOR->aborted()) return false;
									long int t1max = ::pow(cmax / (i_d - 1), 1.0 / (i_d - 1));
									if(t1max < 1) break;
									if(t1max > 1000) t1max = 1000;
									long int c2totalmax = t1max;
									long int c2cur;
									for(int i = 0; i < i_d - 3; i++) {
										c2totalmax *= t1max;
									}
									vector<Number> vden;
									vector<Number> vquo;
									long int *vc = (long int*) malloc(sizeof(long int) * (i_d + 1));
									vden.resize(i_d + 1, nr_zero);
									int in = 0;
									for(size_t i = 0; i < factors0.size() * 2; i++) {
										vc[0] = factors0[i / 2] * (i % 2 == 1 ? -1 : 1);
										for(size_t i2 = 0; i2 < factorsl.size(); i2++) {
											vc[i_d] = factorsl[i2];
											for(long int c2p = 0; c2p <= c2totalmax; c2p++) {
												c2cur = c2p;
												for(int i = 2; i < i_d; i++) {
													vc[i] = c2cur % t1max;
													if(vc[i] % 2 == 1) vc[i] = -vc[i];
													vc[i] = vc[i] / 2 + vc[i] % 2;
													c2cur /= t1max;
												}
												long int c1max = t1max / 2 + t1max % 2, c1min = -t1max / 2 - t1max % 2;
												for(size_t i = 1; i < vs.size(); i++) {
													long int vsmax = vs[i] - vc[0];
													long int vsmin = -vs[i] - vc[0];
													int ix = (i / 2 + i % 2) * (i % 2 == 0 ? -1 : 1);
													int ixi = ix;
													for(int i2 = 2; i2 <= i_d; i2++) {
														ixi *= ix;
														vsmax -= vc[i2] * ixi;
													}
													vsmax /= ix;
													vsmin /= ix;
													if(vsmax < vsmin) {
														if(c1max > vsmin) c1max = vsmin;
														if(c1min < vsmax) c1min = vsmax;
													} else {
														if(c1max > vsmax) c1max = vsmax;
														if(c1min < vsmin) c1min = vsmin;
													}
												}
												for(long int c1 = c1min; c1 <= c1max; c1++) {
													vc[1] = c1;
													bool b = true;
													for(size_t i = 1; i < vs.size(); i++) {
														long int v = vc[0];
														int ix = (i / 2 + i % 2) * (i % 2 == 0 ? -1 : 1);
														int ixi = 1;
														for(int i2 = 1; i2 <= i_d; i2++) {
															ixi *= ix;
															v += vc[i2] * ixi;
														}
														if(v < 0) v = -v;
														if(v == 0 || v > vs[i] || vs[i] % v != 0) {
															b = false;
															break;
														}
													}
													in++;
													if(b) {
														if(CALCULATOR->aborted()) return false;
														for(size_t iden = 0; iden < vden.size(); iden++) {
															vden[iden] = vc[iden];
														}
														if(polynomial_divide_integers(vnum, vden, vquo)) {
															MathStructure mtest;
															mtest.setType(STRUCT_ADDITION);
															for(int i2 = i_d; i2 >= 2; i2--) {
																if(vc[i2] != 0) {
																	MathStructure *mpow = new MathStructure();
																	mpow->setType(STRUCT_POWER);
																	mpow->addChild(*xvar);
																	mpow->addChild_nocopy(new MathStructure(i2, 1, 0));
																	if(vc[i2] == 1) {
																		mtest.addChild_nocopy(mpow);
																	} else {
																		MathStructure *mterm = new MathStructure();
																		mterm->setType(STRUCT_MULTIPLICATION);
																		mterm->addChild_nocopy(new MathStructure(vc[i2], 1L, 0L));
																		mterm->addChild_nocopy(mpow);
																		mtest.addChild_nocopy(mterm);
																	}
																}
															}
															if(vc[1] == 1) {
																mtest.addChild(*xvar);
															} else if(vc[1] != 0) {
																MathStructure *mterm = new MathStructure();
																mterm->setType(STRUCT_MULTIPLICATION);
																mterm->addChild_nocopy(new MathStructure(vc[1], 1L, 0L));
																mterm->addChild(*xvar);
																mtest.addChild_nocopy(mterm);
															}
															mtest.addChild_nocopy(new MathStructure(vc[0], 1L, 0L));
															MathStructure mthis(*this);
															MathStructure mquo;
															if(mtest.size() > 1 && polynomialDivide(mthis, mtest, mquo, eo, false)) {
																mquo.factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
																free(vc);
																set(mquo, true);
																multiply(mtest, true);
																return true;
															}
														}
													}
												}
											}
										}
									}
									free(vc);
								}
							}
						}

					}
				}

				if(SIZE == 2 && max_factor_degree != 0) {
					Number nr1(1, 1, 0), nr2(1, 1, 0);
					bool b = true, b_nonnum = false;
					bool b1_neg = false, b2_neg = false;
					for(size_t i = 0; i < SIZE && b; i++) {
						b = false;
						if(CHILD(i).isInteger() && CHILD(i).number().integerLength() < 100) {
							b = true;
							if(i == 0) nr1 = CHILD(i).number();
							else nr2 = CHILD(i).number();
						} else if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
							b_nonnum = true;
							b = true;
							size_t i2 = 0;
							if(CHILD(i)[0].isInteger() && CHILD(i).number().integerLength() < 100) {
								if(i == 0) nr1 = CHILD(i)[0].number();
								else nr2 = CHILD(i)[0].number();
								i2++;
							}
							for(; i2 < CHILD(i).size(); i2++) {
								if(!CHILD(i)[i2].isPower() || !CHILD(i)[i2][1].isInteger() || !CHILD(i)[i2][1].number().isPositive() || !CHILD(i)[i2][1].number().isEven() || CHILD(i)[1].number().integerLength() >= 100 || !CHILD(i)[i2][0].representsNonMatrix()) {
									b = false;
									break;
								}
							}
						} else if(CHILD(i).isPower() && CHILD(i)[1].isNumber() && CHILD(i)[1].number().isInteger() && CHILD(i)[1].number().isPositive() && CHILD(i)[1].number().isEven() && CHILD(i)[1].number().integerLength() < 100 && CHILD(i)[0].representsNonMatrix()) {
							b_nonnum = true;
							b = true;
						}
					}
					if(!b_nonnum) b = false;
					if(b) {
						b1_neg = nr1.isNegative();
						b2_neg = nr2.isNegative();
						if(b1_neg == b2_neg) b = false;
					}
					if(b) {
						if(b1_neg) b = nr1.negate();
						if(b && !nr1.isOne()) {
							b = nr1.isPerfectSquare() && nr1.isqrt();
						}
					}
					if(b) {
						if(b2_neg) nr2.negate();
						if(!nr2.isOne()) {
							b = nr2.isPerfectSquare() && nr2.isqrt();
						}
					}
					if(b) {
						bool calc = false;
						MathStructure *mmul = new MathStructure(*this);
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isNumber()) {
								if(i == 0) {
									CHILD(i).number() = nr1;
									if(b1_neg) nr1.negate();
									(*mmul)[i].number() = nr1;
								} else {
									CHILD(i).number() = nr2;
									if(b2_neg) nr2.negate();
									(*mmul)[i].number() = nr2;
								}
							} else if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
								b = true;
								size_t i2 = 0;
								if(CHILD(i)[0].isNumber()) {
									if(i == 0) {
										CHILD(i)[0].number() = nr1;
										if(b1_neg) nr1.negate();
										(*mmul)[i][0].number() = nr1;
									} else {
										CHILD(i)[0].number() = nr2;
										if(b2_neg) nr2.negate();
										(*mmul)[i][0].number() = nr2;
									}
									i2++;
								}
								for(; i2 < CHILD(i).size(); i2++) {
									if(CHILD(i)[i2][1].number().isTwo()) {
										CHILD(i)[i2].setToChild(1, true);
										(*mmul)[i][i2].setToChild(1, true);
									} else {
										CHILD(i)[i2][1].number().divide(2);
										(*mmul)[i][i2][1].number().divide(2);
									}
									CHILD(i).childUpdated(i2 + 1);
									(*mmul)[i].childUpdated(i2 + 1);
								}
								if(CHILD(i)[0].isOne()) CHILD(i).delChild(1, true);
								if((*mmul)[i][0].isOne()) (*mmul)[i].delChild(1, true);
							} else if(CHILD(i).isPower()) {
								if(CHILD(i)[1].number().isTwo()) {
									CHILD(i).setToChild(1, true);
									(*mmul)[i].setToChild(1, true);
								} else {
									CHILD(i)[1].number().divide(2);
									(*mmul)[i][1].number().divide(2);
								}
							}
							if(CHILD(i).isAddition()) calc = true;
							CHILD_UPDATED(i)
							mmul->childUpdated(i + 1);
						}
						if(calc) {
							calculatesub(eo, eo, false);
							mmul->calculatesub(eo, eo, false);
						}
						if(recursive) {
							factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
							mmul->factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						}
						multiply_nocopy(mmul);
						evalSort(true);
						return true;
					}
				}

				//x^3-y^3=(x-y)(x^2+xy+y^2)
				if(max_factor_degree != 0 && SIZE == 2 && CHILD(0).isPower() && CHILD(0)[1].isNumber() && CHILD(0)[1].number() == 3 && CHILD(1).isMultiplication() && CHILD(1).size() == 2 && CHILD(1)[0].isMinusOne() && CHILD(1)[1].isPower() && CHILD(1)[1][1].isNumber() && CHILD(1)[1][1].number() == 3) {
					if(CHILD(0)[0].representsNonMatrix() && CHILD(1)[1][0].representsNonMatrix()) {
						MathStructure *m2 = new MathStructure(*this);
						(*m2)[0].setToChild(1, true);
						(*m2)[1][1].setToChild(1, true);
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						m2->calculatesub(eo2, eo2, false);
						CHILD(0)[1].set(2, 1, 0, true);
						CHILD(1).setToChild(2, true);
						CHILD(1)[1].set(2, 1, 0, true);
						MathStructure *m3 = new MathStructure(CHILD(0)[0]);
						m3->calculateMultiply(CHILD(1)[0], eo2);
						add_nocopy(m3, true);
						calculatesub(eo2, eo2, false);
						multiply_nocopy(m2, true);
						evalSort(true);
						return true;
					}
				}
				//-x^3+y^3=(-x+y)(x^2+xy+y^2)
				if(max_factor_degree != 0 && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[1].isNumber() && CHILD(1)[1].number() == 3 && CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isMinusOne() && CHILD(0)[1].isPower() && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number() == 3) {
					if(CHILD(1)[0].representsNonMatrix() && CHILD(0)[1][0].representsNonMatrix()) {
						MathStructure *m2 = new MathStructure(*this);
						(*m2)[1].setToChild(1, true);
						(*m2)[0][1].setToChild(1, true);
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						m2->calculatesub(eo2, eo2, false);
						CHILD(1)[1].set(2, 1, 0, true);
						CHILD(0).setToChild(2, true);
						CHILD(0)[1].set(2, 1, 0, true);
						MathStructure *m3 = new MathStructure(CHILD(0)[0]);
						m3->calculateMultiply(CHILD(1)[0], eo2);
						add_nocopy(m3, true);
						calculatesub(eo2, eo2, false);
						multiply_nocopy(m2, true);
						evalSort(true);
						return true;
					}
				}

				if(max_factor_degree != 0 && !only_integers && !force_factorization.isUndefined() && SIZE >= 2) {
					MathStructure mexp, madd, mmul;
					if(gather_factors(*this, force_factorization, madd, mmul, mexp) && !madd.isZero() && !mmul.isZero() && mexp.isInteger() && mexp.number().isGreaterThan(nr_two)) {
						if(!mmul.isOne()) madd.calculateDivide(mmul, eo);
						bool overflow = false;
						int n = mexp.number().intValue(&overflow);
						if(!overflow) {
							if(n % 4 == 0) {
								int i_u = 1;
								if(n != 4) {
									i_u = n / 4;
								}
								MathStructure m_sqrt2(2, 1, 0);
								m_sqrt2.calculateRaise(nr_half, eo);
								MathStructure m_sqrtb(madd);
								m_sqrtb.calculateRaise(nr_half, eo);
								MathStructure m_bfourth(madd);
								m_bfourth.calculateRaise(Number(1, 4), eo);
								m_sqrt2.calculateMultiply(m_bfourth, eo);
								MathStructure m_x(force_factorization);
								if(i_u != 1) m_x ^= i_u;
								m_sqrt2.calculateMultiply(m_x, eo);
								MathStructure *m2 = new MathStructure(force_factorization);
								m2->raise(Number(i_u * 2, 1));
								m2->add(m_sqrtb);
								m2->calculateAdd(m_sqrt2, eo);
								set(force_factorization, true);
								raise(Number(i_u * 2, 1));
								add(m_sqrtb);
								calculateSubtract(m_sqrt2, eo);
								multiply_nocopy(m2);
							} else {
								int i_u = 1;
								if(n % 2 == 0) {
									i_u = 2;
									n /= 2;
								}
								MathStructure *m2 = new MathStructure(madd);
								m2->calculateRaise(Number(n - 1, n), eo);
								for(int i = 1; i < n - 1; i++) {
									MathStructure *mterm = new MathStructure(madd);
									mterm->calculateRaise(Number(n - i - 1, n), eo);
									mterm->multiply(force_factorization);
									if(i != 1 || i_u != 1) {
										mterm->last().raise(Number(i * i_u, 1));
										mterm->childUpdated(mterm->size());
									}
									if(i % 2 == 1) mterm->calculateMultiply(m_minus_one, eo);
									m2->add_nocopy(mterm, true);
								}
								MathStructure *mterm = new MathStructure(force_factorization);
								mterm->raise(Number((n - 1) * i_u, 1));
								m2->add_nocopy(mterm, true);
								mterm = new MathStructure(force_factorization);
								if(i_u != 1) mterm->raise(Number(i_u, 1));
								set(madd, true);
								calculateRaise(Number(1, n), eo);
								add_nocopy(mterm);
								multiply_nocopy(m2);
							}
							if(!mmul.isOne()) multiply(mmul, true);
							evalSort(true);
							return true;
						}
					}
				}

				//-x-y = -(x+y)
				bool b = true;
				for(size_t i2 = 0; i2 < SIZE; i2++) {
					if((!CHILD(i2).isNumber() || !CHILD(i2).number().isNegative()) && (!CHILD(i2).isMultiplication() || CHILD(i2).size() < 2 || !CHILD(i2)[0].isNumber() || !CHILD(i2)[0].number().isNegative())) {
						b = false;
						break;
					}
				}
				if(b) {
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(CHILD(i2).isNumber()) {
							CHILD(i2).number().negate();
						} else {
							CHILD(i2)[0].number().negate();
							if(CHILD(i2)[0].isOne() && CHILD(i2).size() > 1) {
								CHILD(i2).delChild(1);
								if(CHILD(i2).size() == 1) {
									CHILD(i2).setToChild(1, true);
								}
							}
						}
					}
					multiply(MathStructure(-1, 1, 0));
					CHILD_TO_FRONT(1)
				}

				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
						for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
							if(CHILD(i)[i2].isAddition()) {
								for(size_t i3 = i + 1; i3 < SIZE; i3++) {
									if(CHILD(i3).isMultiplication() && CHILD(i3).size() > 1) {
										for(size_t i4 = 0; i4 < CHILD(i3).size(); i4++) {
											if(CHILD(i3)[i4].isAddition() && CHILD(i3)[i4] == CHILD(i)[i2]) {
												MathStructure *mfac = &CHILD(i)[i2];
												mfac->ref();
												CHILD(i).delChild(i2 + 1, true);
												CHILD(i3).delChild(i4 + 1, true);
												CHILD(i3).ref();
												CHILD(i).add_nocopy(&CHILD(i3));
												CHILD(i).calculateAddLast(eo);
												CHILD(i).multiply_nocopy(mfac);
												CHILD_UPDATED(i)
												delChild(i3 + 1, true);
												evalSort(true);
												factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
												return true;
											}
										}
									}
								}
								if(SIZE > 2) {
									MathStructure mtest(*this);
									mtest.delChild(i + 1);
									if(mtest == CHILD(i)[i2]) {
										CHILD(i).delChild(i2 + 1, true);
										SET_CHILD_MAP(i);
										add(m_one, true);
										multiply(mtest);
										evalSort(true);
										factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
										return true;
									}
								}
							}
						}
					}
				}
			}

			//complete the square
			if(max_factor_degree != 0 && (term_combination_levels != 0 || complete_square)) {
				if(only_integers) {
					if(SIZE <= 3 && SIZE > 1) {
						MathStructure *xvar = NULL;
						Number nr2(1, 1);
						if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
							xvar = &CHILD(0)[0];
						} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber()) {
							if(CHILD(0)[1].isPower()) {
								if(CHILD(0)[1][0].size() == 0 && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isTwo()) {
									xvar = &CHILD(0)[1][0];
									nr2.set(CHILD(0)[0].number());
								}
							}
						}
						if(xvar) {
							bool factorable = false;
							Number nr1, nr0;
							if(SIZE == 2 && CHILD(1).isNumber()) {
								factorable = true;
								nr0 = CHILD(1).number();
							} else if(SIZE == 3 && CHILD(2).isNumber()) {
								nr0 = CHILD(2).number();
								if(CHILD(1).isMultiplication()) {
									if(CHILD(1).size() == 2 && CHILD(1)[0].isNumber() && xvar->equals(CHILD(1)[1])) {
										nr1 = CHILD(1)[0].number();
										factorable = true;
									}
								} else if(xvar->equals(CHILD(1))) {
									nr1.set(1, 1, 0);
									factorable = true;
								}
							}
							if(factorable && !nr2.isZero() && !nr1.isZero()) {
								Number nrh(nr1);
								nrh /= 2;
								nrh /= nr2;
								if(nrh.isInteger()) {
									Number nrk(nrh);
									if(nrk.square()) {
										nrk *= nr2;
										nrk.negate();
										nrk += nr0;
										set(MathStructure(*xvar), true);
										add(nrh);
										raise(nr_two);
										if(!nr2.isOne()) multiply(nr2);
										if(!nrk.isZero()) add(nrk);
										evalSort(true);
										return true;
									}
								}
							}
						}
					}
				} else {
					MathStructure m2, m1, m0;
					const MathStructure *xvar = NULL;
					if(!force_factorization.isUndefined()) {
						xvar = &force_factorization;
					} else {
						if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
							xvar = &CHILD(0)[0];
						} else if(CHILD(0).isMultiplication()) {
							for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
								if(CHILD(0).isPower() && CHILD(0)[i2][0].size() == 0 && CHILD(0)[i2][1].isNumber() && CHILD(0)[i2][1].number().isTwo()) {
									xvar = &CHILD(0)[0];
								}
							}
						}
					}
					if(xvar && gather_factors(*this, *xvar, m0, m1, m2, true) && !m1.isZero() && !m2.isZero()) {
						MathStructure *mx = new MathStructure(*xvar);
						set(m1, true);
						calculateMultiply(nr_half, eo);
						if(!m2.isOne()) calculateDivide(m2, eo);
						add_nocopy(mx);
						calculateAddLast(eo);
						raise(nr_two);
						if(!m2.isOne()) multiply(m2);
						if(!m1.isOne()) m1.calculateRaise(nr_two, eo);
						m1.calculateMultiply(Number(-1, 4), eo);
						if(!m2.isOne()) {
							m2.calculateInverse(eo);
							m1.calculateMultiply(m2, eo);
						}
						m0.calculateAdd(m1, eo);
						if(!m0.isZero()) add(m0);
						if(recursive) {
							CHILD(0).factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
							CHILD(1).factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
							CHILDREN_UPDATED
						}
						evalSort(true);
						return true;
					}
				}
			}

			//Try factorize combinations of terms
			if(SIZE > 2 && term_combination_levels > 0) {
				bool b = false, b_ret = false;
				// 5/y + x/y + z = (5 + x)/y + z
				MathStructure mstruct_units(*this);
				MathStructure mstruct_new(*this);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!mstruct_units[i][i2].isPower() || !mstruct_units[i][i2][1].hasNegativeSign()) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(mstruct_new[i][i2].isPower() && mstruct_new[i][i2][1].hasNegativeSign()) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(mstruct_new[i].isPower() && mstruct_new[i][1].hasNegativeSign()) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) {
						set(mstruct_new[0], true);
						factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						return true;
					} else {
						set(mstruct_new);
					}
					b = false;
					b_ret = true;
				}
				// a*y + a*z + x = a(y + z) + x
				vector<MathStructure> syms;
				vector<size_t> counts;
				collect_symbols(*this, syms);
				size_t max_count = 0, max_i = 0;
				Number min_pow;
				for(size_t i = 0; i < syms.size(); i++) {
					if(syms[i].containsUnknowns()) {
						size_t count = 0;
						Number min_pow_i;
						for(size_t i2 = 0; i2 < SIZE; i2++) {
							if(CHILD(i2).isMultiplication()) {
								for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
									if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isNegative() && CHILD(i2)[i3][0] == syms[i]) {
										if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() > min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
										count++;
										break;
									}
								}
							} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isNegative() && CHILD(i2)[0] == syms[i]) {
								if(min_pow_i.isZero() || CHILD(i2)[1].number() > min_pow_i) min_pow_i = CHILD(i2)[1].number();
								count++;
							}
						}
						if(count > 1 && count > max_count) {
							max_count = count;
							min_pow = min_pow_i;
							max_i = i;
						}
					}
				}
				if(!max_count) {
					for(size_t i = 0; i < syms.size(); i++) {
						if(syms[i].containsUnknowns()) {
							size_t count = 0;
							Number min_pow_i;
							for(size_t i2 = 0; i2 < SIZE; i2++) {
								if(CHILD(i2).isMultiplication()) {
									for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
										if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isPositive() && CHILD(i2)[i3][0] == syms[i]) {
											if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() < min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
											count++;
											break;
										} else if(CHILD(i2)[i3] == syms[i]) {
											if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
											count++;
											break;
										}
									}
								} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isPositive() && CHILD(i2)[0] == syms[i]) {
									if(min_pow_i.isZero() || CHILD(i2)[1].number() < min_pow_i) min_pow_i = CHILD(i2)[1].number();
									count++;
								} else if(CHILD(i2) == syms[i]) {
									if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
									count++;
								}
							}
							if(count > 1 && count > max_count) {
								max_count = count;
								min_pow = min_pow_i;
								max_i = i;
							}
						}
					}
				}
				if(max_count > 0) {
					size_t i = max_i;
					vector<MathStructure*> mleft;
					for(size_t i2 = 0; i2 < SIZE;) {
						b = false;
						if(CHILD(i2).isMultiplication()) {
							for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
								if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[i3][1].number().isPositive() : CHILD(i2)[i3][1].number().isNegative()) && CHILD(i2)[i3][0] == syms[i]) {
									if(CHILD(i2)[i3][1] == min_pow) CHILD(i2).delChild(i3 + 1, true);
									else if(CHILD(i2)[i3][1] == min_pow + 1) CHILD(i2)[i3].setToChild(1, true);
									else {
										CHILD(i2)[i3][1].number() -= min_pow;
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								} else if(min_pow.isPositive() && CHILD(i2)[i3] == syms[i]) {
									if(min_pow.isOne()) CHILD(i2).delChild(i3 + 1, true);
									else {
										CHILD(i2)[i3].raise((-min_pow) + 1);
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								}
							}
						} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[1].number().isPositive() : CHILD(i2)[1].number().isNegative()) && CHILD(i2)[0] == syms[i]) {
							if(CHILD(i2)[1] == min_pow) CHILD(i2).set(1, 1, 0, true);
							else if(CHILD(i2)[1] == min_pow + 1) CHILD(i2).setToChild(1, true);
							else {
								CHILD(i2)[1].number() -= min_pow;
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						} else if(min_pow.isPositive() && CHILD(i2) == syms[i]) {
							if(min_pow.isOne()) CHILD(i2).set(1, 1, 0, true);
							else {
								CHILD(i2).raise((-min_pow) + 1);
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						}
						if(b) {
							i2++;
						} else {
							CHILD(i2).ref();
							mleft.push_back(&CHILD(i2));
							ERASE(i2)
						}
					}
					multiply(syms[i]);
					if(!min_pow.isOne()) LAST ^= min_pow;
					for(size_t i2 = 0; i2 < mleft.size(); i2++) {
						add_nocopy(mleft[i2], true);
					}
					factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					return true;
				}
				if(LAST.isNumber()) {
					MathStructure *mdel = &LAST;
					mdel->ref();
					delChild(SIZE, true);
					b = factorize(eo, false, term_combination_levels - 1, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					add_nocopy(mdel, true);
					if(term_combination_levels == 1) return b || b_ret;
					if(b) b_ret = true;
				}
				for(size_t i = 0; !b && i < SIZE; i++) {
					MathStructure *mdel = &CHILD(i);
					mdel->ref();
					delChild(i + 1, true);
					b = true;
					if(mdel->isMultiplication()) {
						for(size_t i2 = 0; i2 < mdel->size(); i2++) {
							if((*mdel)[i2].isPower() && (*mdel)[i2][0].containsUnknowns()) {
								if(contains((*mdel)[i2][0], false, false, false) > 0) {b = false; break;}
							} else if((*mdel)[i2].containsUnknowns()) {
								if(contains((*mdel)[i2], false, false, false) > 0) {b = false; break;}
							}
						}

					} else {
						b = contains(*mdel, false, false, false) <= 0;
					}
					if(b) {
						b = factorize(eo, false, term_combination_levels - 1, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						if(recursive) mdel->factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						add_nocopy(mdel, true);
						if(term_combination_levels == 1) return b || b_ret;
						if(b) b_ret = true;
						break;
					} else {
						insertChild_nocopy(mdel, i + 1);
					}
				}
				b = false;
				// a*y + a*z + x = a(y + z) + x
				syms.clear();
				counts.clear();
				collect_symbols(*this, syms);
				max_count = 0; max_i = 0;
				for(size_t i = 0; i < syms.size(); i++) {
					size_t count = 0;
					Number min_pow_i;
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(CHILD(i2).isMultiplication()) {
							for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
								if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isNegative() && CHILD(i2)[i3][0] == syms[i]) {
									if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() > min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
									count++;
									break;
								}
							}
						} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isNegative() && CHILD(i2)[0] == syms[i]) {
							if(min_pow_i.isZero() || CHILD(i2)[1].number() > min_pow_i) min_pow_i = CHILD(i2)[1].number();
							count++;
						}
					}
					if(count > 1 && count > max_count) {
						max_count = count;
						min_pow = min_pow_i;
						max_i = i;
					}
				}
				if(!max_count) {
					for(size_t i = 0; i < syms.size(); i++) {
						size_t count = 0;
						Number min_pow_i;
						for(size_t i2 = 0; i2 < SIZE; i2++) {
							if(CHILD(i2).isMultiplication()) {
								for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
									if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isPositive() && CHILD(i2)[i3][0] == syms[i]) {
										if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() < min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
										count++;
										break;
									} else if(CHILD(i2)[i3] == syms[i]) {
										if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
										count++;
										break;
									}
								}
							} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isPositive() && CHILD(i2)[0] == syms[i]) {
								if(min_pow_i.isZero() || CHILD(i2)[1].number() < min_pow_i) min_pow_i = CHILD(i2)[1].number();
								count++;
							} else if(CHILD(i2) == syms[i]) {
								if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
								count++;
							}
						}
						if(count > 1 && count > max_count) {
							max_count = count;
							min_pow = min_pow_i;
							max_i = i;
						}
					}
				}
				if(max_count > 0) {
					size_t i = max_i;
					vector<MathStructure*> mleft;
					for(size_t i2 = 0; i2 < SIZE;) {
						b = false;
						if(CHILD(i2).isMultiplication()) {
							for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
								if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[i3][1].number().isPositive() : CHILD(i2)[i3][1].number().isNegative()) && CHILD(i2)[i3][0] == syms[i]) {
									if(CHILD(i2)[i3][1] == min_pow) CHILD(i2).delChild(i3 + 1, true);
									else if(CHILD(i2)[i3][1] == min_pow + 1) CHILD(i2)[i3].setToChild(1, true);
									else {
										CHILD(i2)[i3][1].number() -= min_pow;
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								} else if(min_pow.isPositive() && CHILD(i2)[i3] == syms[i]) {
									if(min_pow.isOne()) CHILD(i2).delChild(i3 + 1, true);
									else {
										CHILD(i2)[i3].raise((-min_pow) + 1);
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								}
							}
						} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[1].number().isPositive() : CHILD(i2)[1].number().isNegative()) && CHILD(i2)[0] == syms[i]) {
							if(CHILD(i2)[1] == min_pow) CHILD(i2).set(1, 1, 0, true);
							else if(CHILD(i2)[1] == min_pow + 1) CHILD(i2).setToChild(1, true);
							else {
								CHILD(i2)[1].number() -= min_pow;
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						} else if(min_pow.isPositive() && CHILD(i2) == syms[i]) {
							if(min_pow.isOne()) CHILD(i2).set(1, 1, 0, true);
							else {
								CHILD(i2).raise((-min_pow) + 1);
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						}
						if(b) {
							i2++;
						} else {
							CHILD(i2).ref();
							mleft.push_back(&CHILD(i2));
							ERASE(i2)
						}
					}
					multiply(syms[i]);
					if(!min_pow.isOne()) LAST ^= min_pow;
					for(size_t i2 = 0; i2 < mleft.size(); i2++) {
						add_nocopy(mleft[i2], true);
					}
					factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					return true;
				}
				if(isAddition()) {
					b = false;
					// y*f(x) + z*f(x) = (y+z)*f(x)
					mstruct_units.set(*this);
					mstruct_new.set(*this);
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(mstruct_units[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
								if(!combination_factorize_is_complicated(mstruct_units[i][i2])) {
									mstruct_units[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
							else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
							for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
								if(combination_factorize_is_complicated(mstruct_new[i][i2])) {
									mstruct_new[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
							else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
						} else if(combination_factorize_is_complicated(mstruct_units[i])) {
							mstruct_new[i].set(1, 1, 0);
						} else {
							mstruct_units[i].setUndefined();
						}
					}
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(!mstruct_units[i].isUndefined()) {
							for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_new[i].add(mstruct_new[i2], true);
									mstruct_new.delChild(i2 + 1);
									mstruct_units.delChild(i2 + 1);
									b = true;
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
							else mstruct_new[i].multiply(mstruct_units[i], true);
						}
					}
					if(b) {
						if(mstruct_new.size() == 1) set(mstruct_new[0], true);
						else set(mstruct_new);
						factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						return true;
					}
				}
				if(isAddition()) {
					b = false;
					mstruct_units.set(*this);
					mstruct_new.set(*this);
					// 5x + pi*x + 5y + xy = (5 + pi)x + 5y + xy
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(mstruct_units[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
								if(!mstruct_units[i][i2].containsType(STRUCT_UNIT, true) && !mstruct_units[i][i2].containsUnknowns()) {
									mstruct_units[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
							else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
							for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
								if(mstruct_new[i][i2].containsType(STRUCT_UNIT, true) || mstruct_new[i][i2].containsUnknowns()) {
									mstruct_new[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
							else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
						} else if(mstruct_units[i].containsType(STRUCT_UNIT, true) || mstruct_units[i].containsUnknowns()) {
							mstruct_new[i].set(1, 1, 0);
						} else {
							mstruct_units[i].setUndefined();
						}
					}
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(!mstruct_units[i].isUndefined()) {
							for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_new[i].add(mstruct_new[i2], true);
									mstruct_new.delChild(i2 + 1);
									mstruct_units.delChild(i2 + 1);
									b = true;
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
							else mstruct_new[i].multiply(mstruct_units[i], true);
						}
					}
					if(b) {
						if(mstruct_new.size() == 1) set(mstruct_new[0], true);
						else set(mstruct_new);
						factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						return true;
					}
				}
				return b_ret;
			} else if(term_combination_levels != 0 && SIZE > 2) {
				int start_index = rand() % SIZE;
				int index = start_index;
				int best_index = -1;
				int run_index = 0;
				int max_run_index = SIZE - 3;
				if(term_combination_levels < -1) {
					run_index = -term_combination_levels - 2;
					max_run_index = run_index;
				} else if(term_combination_levels > 0 && term_combination_levels - 1 < max_run_index) {
					max_run_index = term_combination_levels -1;
				}

				MathStructure mbest;
				do {
					if(CALCULATOR->aborted()) break;
					if(endtime_p && endtime_p->tv_sec > 0) {
#ifndef CLOCK_MONOTONIC
						struct timeval curtime;
						gettimeofday(&curtime, NULL);
						if(curtime.tv_sec > endtime_p->tv_sec || (curtime.tv_sec == endtime_p->tv_sec && curtime.tv_usec > endtime_p->tv_usec)) {
#else
						struct timespec curtime;
						clock_gettime(CLOCK_MONOTONIC, &curtime);
						if(curtime.tv_sec > endtime_p->tv_sec || (curtime.tv_sec == endtime_p->tv_sec && curtime.tv_nsec / 1000 > endtime_p->tv_usec)) {
#endif
							CALCULATOR->error(false, _("Because of time constraints only a limited number of combinations of terms were tried during factorization. Repeat factorization to try other random combinations."), NULL);
							break;
						}
					}

					MathStructure mtest(*this);
					mtest.delChild(index + 1);
					if(mtest.factorize(eo, false, run_index == 0 ? 0 : -1 - run_index, 0, only_integers, false, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree)) {
						bool b = best_index < 0 || (mbest.isAddition() && !mtest.isAddition());
						if(!b && (mtest.isAddition() == mbest.isAddition())) {
							b = mtest.isAddition() && (mtest.size() < mbest.size());
							if(!b && (!mtest.isAddition() || mtest.size() == mbest.size())) {
								size_t c1 = mtest.countTotalChildren() + CHILD(index).countTotalChildren();
								size_t c2 = mbest.countTotalChildren() + CHILD(best_index).countTotalChildren();
								b = (c1 < c2);
								if(c1 == c2) {
									b = (count_powers(mtest) + count_powers(CHILD(index))) < (count_powers(mbest) + count_powers(CHILD(best_index)));
								}
							}
						}
						if(b) {
							mbest = mtest;
							best_index = index;
							if(mbest.isPower()) {
								break;
							}
						}
					}
					index++;
					if(index == (int) SIZE) index = 0;
					if(index == start_index) {
						if(best_index >= 0) {
							break;
						}
						run_index++;
						if(run_index > max_run_index) break;
					}
				} while(true);
				if(best_index >= 0) {
					mbest.add(CHILD(best_index), true);
					set(mbest);
					if(term_combination_levels >= -1 && (run_index > 0 || recursive)) {
						factorize(eo, false, term_combination_levels, 0, only_integers, true, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					}
					return true;
				}
			}
		}
		default: {
			if(term_combination_levels < -1) break;
			bool b = false;

			if(isComparison()) {
				EvaluationOptions eo2 = eo;
				eo2.assume_denominators_nonzero = false;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).factorize(eo2, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
			} else if(recursive && (recursive > 1 || !isAddition())) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
			}
			if(b) {
				EvaluationOptions eo2 = eo;
				eo2.expand = false;
				calculatesub(eo2, eo2, false);
				evalSort(true);
				if(isAddition()) {
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
							for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
								if(CHILD(i)[i2].isAddition()) {
									for(size_t i3 = i + 1; i3 < SIZE; i3++) {
										if(CHILD(i3).isMultiplication() && CHILD(i3).size() > 1) {
											for(size_t i4 = 0; i4 < CHILD(i3).size(); i4++) {
												if(CHILD(i3)[i4].isAddition() && CHILD(i3)[i4] == CHILD(i)[i2]) {
													MathStructure *mfac = &CHILD(i)[i2];
													mfac->ref();
													CHILD(i).delChild(i2 + 1, true);
													CHILD(i3).delChild(i4 + 1, true);
													CHILD(i3).ref();
													CHILD(i).add_nocopy(&CHILD(i3));
													CHILD(i).calculateAddLast(eo);
													CHILD(i).multiply_nocopy(mfac);
													CHILD_UPDATED(i)
													delChild(i3 + 1, true);
													evalSort(true);
													factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
													return true;
												}
											}
										}
									}
									if(SIZE > 2) {
										MathStructure mtest(*this);
										mtest.delChild(i + 1);
										if(mtest == CHILD(i)[i2]) {
											CHILD(i).delChild(i2 + 1, true);
											SET_CHILD_MAP(i);
											add(m_one, true);
											multiply(mtest);
											evalSort(true);
											factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
											return true;
										}
									}
								}
							}
						}
					}
				}
				return true;
			}
		}
	}
	return false;
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
void MathStructure::unformat(const EvaluationOptions &eo) {
	if(m_type == STRUCT_FUNCTION && o_function == CALCULATOR->f_stripunits) {
		EvaluationOptions eo2 = eo;
		eo2.keep_prefixes = true;
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).unformat(eo2);
		}
	} else {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).unformat(eo);
		}
	}
	switch(m_type) {
		case STRUCT_INVERSE: {
			APPEND(m_minus_one);
			m_type = STRUCT_POWER;
			break;
		}
		case STRUCT_NEGATE: {
			PREPEND(m_minus_one);
			m_type = STRUCT_MULTIPLICATION;
			break;
		}
		case STRUCT_DIVISION: {
			CHILD(1).raise(m_minus_one);
			m_type = STRUCT_MULTIPLICATION;
			break;
		}
		case STRUCT_UNIT: {
			if(o_prefix && !eo.keep_prefixes) {
				if(o_prefix == CALCULATOR->decimal_null_prefix || o_prefix == CALCULATOR->binary_null_prefix) {
					o_prefix = NULL;
				} else {
					Unit *u = o_unit;
					Prefix *p = o_prefix;
					set(p->value());
					multiply(u);
				}
				unformat(eo);
				break;
			} else if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				set(((CompositeUnit*) o_unit)->generateMathStructure(false, eo.keep_prefixes));
				unformat(eo);
				break;
			}
			b_plural = false;
			break;
		}
		default: {}
	}
}

void idm1(const MathStructure &mnum, bool &bfrac, bool &bint) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if((!bfrac || bint) && mnum.number().isRational()) {
				if(!mnum.number().isInteger()) {
					bint = false;
					bfrac = true;
				}
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if((!bfrac || bint) && mnum.size() > 0 && mnum[0].isNumber() && mnum[0].number().isRational()) {
				if(!mnum[0].number().isInteger()) {
					bint = false;
					bfrac = true;
				}

			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_ADDITION: {
			bool b_a = false;
			for(size_t i = 0; i < mnum.size() && (!bfrac || bint); i++) {
				if(mnum[i].isAddition()) b_a = true;
				else idm1(mnum[i], bfrac, bint);
			}
			if(b_a) bint = false;
			break;
		}
		default: {
			bint = false;
		}
	}
}
void idm2(const MathStructure &mnum, bool &bfrac, bool &bint, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(mnum.number().isRational()) {
				if(mnum.number().isInteger()) {
					if(bint) {
						if(mnum.number().isOne()) {
							bint = false;
						} else if(nr.isOne()) {
							nr = mnum.number();
						} else if(nr != mnum.number()) {
							nr.gcd(mnum.number());
							if(nr.isOne()) bint = false;
						}
					}
				} else {
					if(nr.isOne()) {
						nr = mnum.number().denominator();
					} else {
						Number nden(mnum.number().denominator());
						if(nr != nden) {
							Number ngcd(nden);
							ngcd.gcd(nr);
							nden /= ngcd;
							nr *= nden;
						}
					}
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber() && mnum[0].number().isRational()) {
				if(mnum[0].number().isInteger()) {
					if(bint) {
						if(mnum[0].number().isOne()) {
							bint = false;
						} else if(nr.isOne()) {
							nr = mnum[0].number();
						} else if(nr != mnum[0].number()) {
							nr.gcd(mnum[0].number());
							if(nr.isOne()) bint = false;
						}
					}
				} else {
					if(nr.isOne()) {
						nr = mnum[0].number().denominator();
					} else {
						Number nden(mnum[0].number().denominator());
						if(nr != nden) {
							Number ngcd(nden);
							ngcd.gcd(nr);
							nden /= ngcd;
							nr *= nden;
						}
					}
				}
			}
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size() && (bfrac || bint); i++) {
				if(!mnum[i].isAddition()) idm2(mnum[i], bfrac, bint, nr);
			}
			break;
		}
		default: {}
	}
}
int idm3(MathStructure &mnum, Number &nr, bool expand) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			mnum.number() *= nr;
			mnum.numberUpdated();
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				mnum[0].number() *= nr;
				if(mnum[0].number().isOne() && mnum.size() != 1) {
					mnum.delChild(1, true);
				}
				return -1;
			} else if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					if(mnum[i].isAddition()) {
						idm3(mnum[i], nr, true);
						return -1;
					}
				}
			}
			mnum.insertChild(nr, 1);
			return 1;
		}
		case STRUCT_ADDITION: {
			if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					idm3(mnum[i], nr, true);
				}
				break;
			}
		}
		default: {
			mnum.transform(STRUCT_MULTIPLICATION);
			mnum.insertChild(nr, 1);
			return -1;
		}
	}
	return 0;
}

void idm1b(const MathStructure &mnum, bool &bint, bool &bint2) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(mnum.number().isInteger() && !mnum.number().isOne()) {
				bint = true;
				if(mnum.number() > 9 || mnum.number() < -9) bint2 = true;
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				idm1b(mnum[0], bint, bint2);
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size(); i++) {
				if(mnum[i].isAddition()) bint = false;
				else idm1b(mnum[i], bint, bint2);
				if(!bint) break;
			}
			break;
		}
		default: {
			bint = false;
		}
	}
}
void idm2b(const MathStructure &mnum, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(nr.isZero() || mnum.number() < nr) nr = mnum.number();
			break;
		}
		case STRUCT_MULTIPLICATION: {
			idm2b(mnum[0], nr);
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size(); i++) {
				idm2b(mnum[i], nr);
			}
			break;
		}
		default: {}
	}
}
void idm3b(MathStructure &mnum, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			mnum.number() /= nr;
			break;
		}
		case STRUCT_MULTIPLICATION: {
			idm3b(mnum[0], nr);
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size(); i++) {
				idm3b(mnum[i], nr);
			}
			break;
		}
		default: {}
	}
}

bool displays_number_exact(Number nr, const PrintOptions &po, MathStructure *top_parent) {
	if(po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) return true;
	InternalPrintStruct ips_n;
	if(top_parent && top_parent->isApproximate()) ips_n.parent_approximate = true;
	if(po.show_ending_zeroes && po.restrict_to_parent_precision && ips_n.parent_approximate && (nr > 9 || nr < -9)) return false;
	if(top_parent && top_parent->precision() < 0) ips_n.parent_precision = top_parent->precision();
	bool approximately_displayed = false;
	PrintOptions po2 = po;
	po2.is_approximate = &approximately_displayed;
	nr.print(po2, ips_n);
	return !approximately_displayed;
}

bool fix_approximate_multiplier(MathStructure &m, const PrintOptions &po, MathStructure *top_parent = NULL) {
	if(!top_parent) top_parent = &m;
	if(po.number_fraction_format == FRACTION_DECIMAL) {
		PrintOptions po2 = po;
		po2.number_fraction_format = FRACTION_FRACTIONAL;
		po2.restrict_fraction_length = true;
		return fix_approximate_multiplier(m, po2, top_parent);
	}
	bool b_ret = false;
	if(m.isMultiplication() && m.size() >= 2 && m[0].isNumber() && m[0].number().isRational()) {
		for(size_t i = 1; i < m.size(); i++) {
			if(m[i].isAddition()) {
				bool mul_exact = displays_number_exact(m[0].number(), po, top_parent);
				bool b = false;
				for(size_t i2 = 0; i2 < m[i].size() && !b; i2++) {
					if(m[i][i2].isNumber() && (!mul_exact || !displays_number_exact(m[i][i2].number(), po, top_parent))) {
						b = true;
					} else if(m[i][i2].isMultiplication() && m[i][i2].size() >= 2 && m[i][i2][0].isNumber() && (!mul_exact || !displays_number_exact(m[i][i2][0].number(), po, top_parent))) {
						b = true;
					}
				}
				if(b) {
					for(size_t i2 = 0; i2 < m[i].size() && !b; i2++) {
						if(m[i][i2].isNumber()) {
							if(!m[i][i2].number().multiply(m[0].number())) {
								m[i][i2].multiply(m[0]);
							}
						} else if(m[i][i2].isMultiplication()) {
							if(m[i][i2].size() < 2 || !m[i][i2][0].isNumber() || !m[i][i2][0].number().multiply(m[0].number())) {
								m[i][i2].insertChild(m[0], 1);
							}
						} else {
							m[i][i2].multiply(m[0]);
							m[i][i2].swapChildren(1, 2);
						}
					}
					m.delChild(1, true);
					b_ret = true;
					break;
				}
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_approximate_multiplier(m[i], po, top_parent)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	return b_ret;
}

int idm3_test(bool &b_fail, const MathStructure &mnum, const Number &nr, bool expand, const PrintOptions &po, MathStructure *top_parent) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			Number nr2(mnum.number());
			nr2 *= nr;
			b_fail = !displays_number_exact(nr2, po, top_parent);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				Number nr2(mnum[0].number());
				nr2 *= nr;
				b_fail = !nr2.isOne() && !displays_number_exact(nr2, po, top_parent);
				return -1;
			} else if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					if(mnum[i].isAddition()) {
						idm3_test(b_fail, mnum[i], nr, true, po, top_parent);
						return -1;
					}
				}
			}
			b_fail = !displays_number_exact(nr, po, top_parent);
			return 1;
		}
		case STRUCT_ADDITION: {
			if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					idm3_test(b_fail, mnum[i], nr, true, po, top_parent);
					if(b_fail) break;
				}
				break;
			}
		}
		default: {
			b_fail = !displays_number_exact(nr, po, top_parent);
			return -1;
		}
	}
	return 0;
}

bool is_unit_multiexp(const MathStructure &mstruct) {
	if(mstruct.isUnit_exp()) return true;
	if(mstruct.isMultiplication()) {
		for(size_t i3 = 0; i3 < mstruct.size(); i3++) {
			if(!mstruct[i3].isUnit_exp()) {
				return false;
				break;
			}
		}
		return true;
	}
	if(mstruct.isPower() && mstruct[0].isMultiplication()) {
		for(size_t i3 = 0; i3 < mstruct[0].size(); i3++) {
			if(!mstruct[0][i3].isUnit_exp()) {
				return false;
				break;
			}
		}
		return true;
	}
	return false;
}

bool MathStructure::improve_division_multipliers(const PrintOptions &po, MathStructure *top_parent) {
	if(!top_parent) top_parent = this;
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			size_t inum = 0, iden = 0;
			bool bfrac = false, bint = true, bdiv = false, bnonunitdiv = false;
			size_t index1 = 0, index2 = 0;
			bool dofrac = !po.negative_exponents;
			for(size_t i2 = 0; i2 < SIZE; i2++) {
				if(CHILD(i2).isPower() && CHILD(i2)[1].isMinusOne()) {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2)[0])) {
						if(iden == 0) index1 = i2;
						iden++;
						bdiv = true;
						if(!CHILD(i2)[0].isUnit()) bnonunitdiv = true;
						if(CHILD(i2)[0].containsType(STRUCT_ADDITION)) {
							dofrac = true;
						}
					}
				} else if(!bdiv && !po.negative_exponents && CHILD(i2).isPower() && CHILD(i2)[1].hasNegativeSign()) {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2)[0])) {
						if(!bdiv) index1 = i2;
						bdiv = true;
						if(!CHILD(i2)[0].isUnit()) bnonunitdiv = true;
					}
				} else {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2))) {
						if(inum == 0) index2 = i2;
						inum++;
					}
				}
			}
			if(!bdiv || !bnonunitdiv) break;
			if(iden > 1 && !po.negative_exponents) {
				size_t i2 = index1 + 1;
				while(i2 < SIZE) {
					if(CHILD(i2).isPower() && CHILD(i2)[1].isMinusOne()) {
						CHILD(index1)[0].multiply(CHILD(i2)[0], true);
						ERASE(i2);
						if(index2 > i2) index2--;
					} else {
						i2++;
					}
				}
				iden = 1;
			}
			if(bint) bint = inum > 0 && iden == 1;
			if(inum > 0) idm1(CHILD(index2), bfrac, bint);
			if(iden > 0) idm1(CHILD(index1)[0], bfrac, bint);
			bool b = false;
			if(!dofrac) bfrac = false;
			if(bint || bfrac) {
				Number nr(1, 1);
				if(inum > 0) idm2(CHILD(index2), bfrac, bint, nr);
				if(iden > 0) idm2(CHILD(index1)[0], bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					if(bint) nr.recip();

					bool b_fail = false;
					if(inum > 1 && !CHILD(index2).isNumber()) {
						int i = idm3_test(b_fail, *this, nr, !po.allow_factorization, po, top_parent);
						if(i >= 0 && !b_fail && iden > 0) idm3_test(b_fail, CHILD(index1)[0], nr, !po.allow_factorization, po, top_parent);
					} else {
						if(inum != 0) idm3_test(b_fail, CHILD(index2), nr, !po.allow_factorization, po, top_parent);
						if(!b_fail && iden > 0) idm3_test(b_fail, CHILD(index1)[0], nr, !po.allow_factorization, po, top_parent);
					}
					if(!b_fail) {
						if(inum == 0) {
							PREPEND(MathStructure(nr));
							index1 += 1;
						} else if(inum > 1 && !CHILD(index2).isNumber()) {
							int i = idm3(*this, nr, !po.allow_factorization);
							if(i == 1) index1++;
							else if(i < 0) iden = 0;
						} else {
							idm3(CHILD(index2), nr, !po.allow_factorization);
						}
						if(iden > 0) {
							idm3(CHILD(index1)[0], nr, !po.allow_factorization);
						} else {
							MathStructure mstruct(nr);
							mstruct.raise(m_minus_one);
							insertChild(mstruct, index1);
						}
						b = true;
					}
				}
			}
			if(!b && po.show_ending_zeroes && po.restrict_to_parent_precision && top_parent->isApproximate() && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_BIJECTIVE_26 && inum > 0 && iden > 0) {
				bint = false;
				bool bint2 = false;
				idm1b(CHILD(index2), bint, bint2);
				if(bint) idm1b(CHILD(index1)[0], bint, bint2);
				if(bint && bint2) {
					Number nr;
					idm2b(CHILD(index2), nr);
					idm2b(CHILD(index1)[0], nr);
					idm3b(CHILD(index2), nr);
					idm3b(CHILD(index1)[0], nr);
				}
			}
			return b;
		}
		case STRUCT_DIVISION: {
			bool bint = true, bfrac = false;
			idm1(CHILD(0), bfrac, bint);
			idm1(CHILD(1), bfrac, bint);
			if(bint || bfrac) {
				Number nr(1, 1);
				idm2(CHILD(0), bfrac, bint, nr);
				idm2(CHILD(1), bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					if(bint) nr.recip();

					bool b_fail = false;
					idm3_test(b_fail, CHILD(0), nr, !po.allow_factorization, po, top_parent);
					if(b_fail) return false;
					idm3_test(b_fail, CHILD(1), nr, !po.allow_factorization, po, top_parent);
					if(b_fail) return false;

					idm3(CHILD(0), nr, !po.allow_factorization);
					idm3(CHILD(1), nr, !po.allow_factorization);
					return true;
				}
			}
			break;
		}
		case STRUCT_INVERSE: {
			bool bint = false, bfrac = false;
			idm1(CHILD(0), bfrac, bint);
			if(bint || bfrac) {
				Number nr(1, 1);
				idm2(CHILD(0), bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					bool b_fail = false;
					idm3_test(b_fail, CHILD(0), nr, !po.allow_factorization, po, top_parent);
					if(b_fail) return false;

					setToChild(1, true);
					idm3(*this, nr, !po.allow_factorization);
					transform_nocopy(STRUCT_DIVISION, new MathStructure(nr));
					SWAP_CHILDREN(0, 1);
					return true;
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(1).isMinusOne()) {
				bool bint = false, bfrac = false;
				idm1(CHILD(0), bfrac, bint);
				if(bint || bfrac) {
					Number nr(1, 1);
					idm2(CHILD(0), bfrac, bint, nr);
					if((bint || bfrac) && !nr.isOne()) {
						bool b_fail = false;
						idm3_test(b_fail, CHILD(0), nr, !po.allow_factorization, po, top_parent);
						if(b_fail) return false;

						idm3(CHILD(0), nr, !po.allow_factorization);
						transform(STRUCT_MULTIPLICATION);
						PREPEND(MathStructure(nr));
						return true;
					}
				}
				break;
			}
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				if(CHILD(i).improve_division_multipliers(po, top_parent)) b = true;
			}
			return b;
		}
	}
	return false;
}

bool use_prefix_with_unit(Unit *u, const PrintOptions &po) {
	if(!po.prefix && !po.use_unit_prefixes) {return u->referenceName() == "g" || u->referenceName() == "a";}
	if(po.prefix) return true;
	if(u->isCurrency()) return po.use_prefixes_for_currencies;
	if(po.use_prefixes_for_all_units) return true;
	return u->useWithPrefixesByDefault();
}
bool use_prefix_with_unit(const MathStructure &mstruct, const PrintOptions &po) {
	if(mstruct.isUnit()) return use_prefix_with_unit(mstruct.unit(), po);
	if(mstruct.isUnit_exp()) return use_prefix_with_unit(mstruct[0].unit(), po);
	return false;
}

bool has_prefix(const MathStructure &mstruct) {
	if(mstruct.isUnit()) return mstruct.prefix() != NULL;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(has_prefix(mstruct[i])) return true;
	}
	return false;
}

void MathStructure::setPrefixes(const PrintOptions &po, MathStructure *parent, size_t pindex) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			size_t i = SIZE, im = 0;
			bool b_im = false;
			for(size_t i2 = 0; i2 < SIZE; i2++) {
				if(CHILD(i2).isUnit_exp()) {
					if(CHILD(i2).unit_exp_prefix()) {
						b = false;
						return;
					}
					if(!b) {
						if(use_prefix_with_unit(CHILD(i2), po)) {
							b = true;
							if(i > i2) {i = i2; b_im = false;}
						} else if(i < i2) {
							i = i2;
							b_im = false;
						}
					}
				} else if(CHILD(i2).isPower() && CHILD(i2)[0].isMultiplication()) {
					for(size_t i3 = 0; i3 < CHILD(i2)[0].size(); i3++) {
						if(CHILD(i2)[0][i3].isUnit_exp()) {
							if(CHILD(i2)[0][i3].unit_exp_prefix()) {
								b = false;
								return;
							}
							if(!b) {
								if(use_prefix_with_unit(CHILD(i2)[0][i3], po)) {
									b = true;
									if(i > i2) {
										i = i2;
										im = i3;
										b_im = true;
									}
									break;
								} else if(i < i2 || (i == i2 && im < i3)) {
									i = i2;
									im = i3;
									b_im = true;
								}
							}
						}
					}
				}
			}
			if(b) {
				Number exp(1, 1);
				Number exp2(1, 1);
				bool b2 = false;
				MathStructure *munit = NULL, *munit2 = NULL;
				if(b_im) munit = &CHILD(i)[0][im];
				else munit = &CHILD(i);
				if(CHILD(i).isPower()) {
					if(CHILD(i)[1].isNumber() && CHILD(i)[1].number().isInteger() && !CHILD(i)[1].number().isZero()) {
						if(b_im && munit->isPower()) {
							if((*munit)[1].isNumber() && (*munit)[1].number().isInteger() && !(*munit)[1].number().isZero()) {
								exp = CHILD(i)[1].number();
								exp *= (*munit)[1].number();
							} else {
								b = false;
							}
						} else {
							exp = CHILD(i)[1].number();
						}
					} else {
						b = false;
					}
				}
				if(po.use_denominator_prefix && !exp.isNegative()) {
					for(size_t i2 = i + 1; i2 < SIZE; i2++) {
						if(CALCULATOR->aborted()) break;
						if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isNegative()) {
							if(CHILD(i2)[0].isUnit() && use_prefix_with_unit(CHILD(i2)[0], po)) {
								munit2 = &CHILD(i2)[0];
								if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
									break;
								}
								if(!b) {
									b = true;
									exp = CHILD(i2)[1].number();
									munit = munit2;
								} else {
									b2 = true;
									exp2 = CHILD(i2)[1].number();
								}
								break;
							} else if(CHILD(i2)[0].isMultiplication()) {
								bool b_break = false;
								for(size_t im2 = 0; im2 < CHILD(i2)[0].size(); im2++) {
									if(CHILD(i2)[0][im2].isUnit_exp() && use_prefix_with_unit(CHILD(i2)[0][im2], po) && (CHILD(i2)[0][im2].isUnit() || (CHILD(i2)[0][im2][1].isNumber() && (CHILD(i2)[0][im2][1].number().isPositive() || (!b && CHILD(i2)[0][im2][1].number().isNegative())) && CHILD(i2)[0][im2][1].number().isInteger()))) {
										Number exp_multi(1);
										if(CHILD(i2)[0][im2].isUnit()) {
											munit2 = &CHILD(i2)[0][im2];
										} else {
											munit2 = &CHILD(i2)[0][im2][0];
											exp_multi = CHILD(i2)[0][im2][1].number();
										}
										b_break = true;
										if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
											break;
										}
										if(!b) {
											b = true;
											exp = CHILD(i2)[1].number();
											exp *= exp_multi;
											i = i2;
										} else {
											b2 = true;
											exp2 = CHILD(i2)[1].number();
											exp2 *= exp_multi;
										}
										break;
									}
								}
								if(b_break) break;
							}
						}
					}
				} else if(exp.isNegative() && b) {
					bool had_unit = false;
					for(size_t i2 = i + 1; i2 < SIZE; i2++) {
						if(CALCULATOR->aborted()) break;
						bool b3 = false;
						if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isPositive()) {
							if(CHILD(i2)[0].isUnit()) {
								if(!use_prefix_with_unit(CHILD(i2)[0], po)) {
									had_unit = true;
								} else {
									munit2 = &CHILD(i2);
									if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
										break;
									}
									b3 = true;
									exp2 = exp;
									exp = CHILD(i2)[1].number();
								}
							} else if(CHILD(i2)[0].isMultiplication()) {
								bool b_break = false;
								for(size_t im2 = 0; im2 < CHILD(i2)[0].size(); im2++) {
									if(CHILD(i2)[0][im2].isUnit_exp() && (CHILD(i2)[0][im2].isUnit() || (CHILD(i2)[0][im2][1].isNumber() && CHILD(i2)[0][im2][1].number().isPositive() && CHILD(i2)[0][im2][1].number().isInteger()))) {
										if(!use_prefix_with_unit(CHILD(i2)[0][im2], po)) {
											had_unit = true;
										} else {
											Number exp_multi(1);
											if(CHILD(i2)[0][im2].isUnit()) {
												munit2 = &CHILD(i2)[0][im2];
											} else {
												munit2 = &CHILD(i2)[0][im2][0];
												exp_multi = CHILD(i2)[0][im2][1].number();
											}
											b_break = true;
											if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
												break;
											}
											exp2 = exp;
											exp = CHILD(i2)[1].number();
											exp *= exp_multi;
											b3 = true;
											break;
										}
									}
								}
								if(b_break) break;
							}
						} else if(CHILD(i2).isUnit()) {
							if(!use_prefix_with_unit(CHILD(i2), po)) {
								had_unit = true;
							} else {
								if(CHILD(i2).prefix()) break;
								exp2 = exp;
								exp.set(1, 1, 0);
								b3 = true;
								munit2 = &CHILD(i2);
							}
						}
						if(b3) {
							if(po.use_denominator_prefix) {
								b2 = true;
								MathStructure *munit3 = munit;
								munit = munit2;
								munit2 = munit3;
							} else {
								munit = munit2;
							}
							had_unit = false;
							break;
						}
					}
					if(had_unit && !po.use_denominator_prefix) b = false;
				}
				Number exp10;
				if(b) {
					if(po.prefix) {
						if(po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix) {
							if(munit->isUnit()) munit->setPrefix(po.prefix);
							else (*munit)[0].setPrefix(po.prefix);
							if(CHILD(0).isNumber()) {
								CHILD(0).number() /= po.prefix->value(exp);
							} else {
								PREPEND(po.prefix->value(exp));
								CHILD(0).number().recip();
							}
						}
					} else if(po.use_unit_prefixes && CHILD(0).isNumber() && exp.isInteger()) {
						exp10 = CHILD(0).number();
						exp10.abs();
						exp10.intervalToMidValue();
						if(exp10.isLessThanOrEqualTo(Number(1, 1, 1000)) && exp10.isGreaterThanOrEqualTo(Number(1, 1, -1000))) {
							bool use_binary_prefix = (CALCULATOR->usesBinaryPrefixes() > 1 || (CALCULATOR->usesBinaryPrefixes() == 1 && ((munit->isUnit() && munit->unit()->baseUnit()->referenceName() == "bit") || (munit->isPower() && (*munit)[0].unit()->baseUnit()->referenceName() == "bit"))));
							exp10.log(use_binary_prefix ? 2 : 10);
							exp10.intervalToMidValue();
							exp10.floor();
							if(b2 && exp10.isPositive() && (CALCULATOR->usesBinaryPrefixes() > 1 || (CALCULATOR->usesBinaryPrefixes() == 1 && ((munit2->isUnit() && munit2->unit()->baseUnit()->referenceName() == "bit") || (munit2->isPower() && (*munit2)[0].unit()->baseUnit()->referenceName() == "bit"))))) b2 = false;
							if(b2 && use_binary_prefix && CALCULATOR->usesBinaryPrefixes() == 1 && exp10.isNegative()) {
								exp10.clear();
							} else if(b2) {
								Number tmp_exp(exp10);
								tmp_exp.setNegative(false);
								Number e1(use_binary_prefix ? 10 : 3, 1, 0);
								e1 *= exp;
								Number e2(use_binary_prefix ? 10 : 3, 1, 0);
								e2 *= exp2;
								e2.setNegative(false);
								int i4 = 0;
								while(true) {
									tmp_exp -= e1;
									if(!tmp_exp.isPositive()) {
										break;
									}
									if(exp10.isNegative()) i4++;
									tmp_exp -= e2;
									if(tmp_exp.isNegative()) {
										break;
									}
									if(!exp10.isNegative())	i4++;
								}
								e2.setNegative(exp10.isNegative());
								e2 *= i4;
								exp10 -= e2;
							}
							Prefix *p = (use_binary_prefix > 0 ? (Prefix*) CALCULATOR->getOptimalBinaryPrefix(exp10, exp) : (Prefix*) CALCULATOR->getOptimalDecimalPrefix(exp10, exp, po.use_all_prefixes));
							if(p) {
								Number test_exp(exp10);
								if(use_binary_prefix) test_exp -= ((BinaryPrefix*) p)->exponent(exp);
								else test_exp -= ((DecimalPrefix*) p)->exponent(exp);
								if(test_exp.isInteger()) {
									if((exp10.isPositive() && exp10.compare(test_exp) == COMPARISON_RESULT_LESS) || (exp10.isNegative() && exp10.compare(test_exp) == COMPARISON_RESULT_GREATER)) {
										CHILD(0).number() /= p->value(exp);
										if(munit->isUnit()) munit->setPrefix(p);
										else (*munit)[0].setPrefix(p);
									}
								}
							}
						}
					} else if(!po.use_unit_prefixes) {
						Prefix *p = NULL;
						if((munit->isUnit() && munit->unit()->referenceName() == "g") || (munit->isPower() && (*munit)[0].unit()->referenceName() == "g")) {
							p = CALCULATOR->getExactDecimalPrefix(3);
						} else if((munit->isUnit() && munit->unit()->referenceName() == "a") || (munit->isPower() && (*munit)[0].unit()->referenceName() == "a")) {
							p = CALCULATOR->getExactDecimalPrefix(2);
						}
						if(p) {
							if(munit->isUnit()) munit->setPrefix(p);
							else (*munit)[0].setPrefix(p);
							if(CHILD(0).isNumber()) {
								CHILD(0).number() /= p->value(exp);
							} else {
								PREPEND(p->value(exp));
								CHILD(0).number().recip();
							}
						}
					}
					if(b2 && CHILD(0).isNumber() && (po.prefix || po.use_unit_prefixes) && (po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix)) {
						exp10 = CHILD(0).number();
						exp10.abs();
						exp10.intervalToMidValue();
						if(exp10.isLessThanOrEqualTo(Number(1, 1, 1000)) && exp10.isGreaterThanOrEqualTo(Number(1, 1, -1000))) {
							bool use_binary_prefix = (CALCULATOR->usesBinaryPrefixes() > 1 || (CALCULATOR->usesBinaryPrefixes() == 1 && ((munit2->isUnit() && munit2->unit()->baseUnit()->referenceName() == "bit") || (munit2->isPower() && (*munit2)[0].unit()->baseUnit()->referenceName() == "bit"))));
							exp10.log(use_binary_prefix ? 2 : 10);
							exp10.intervalToMidValue();
							exp10.floor();
							Prefix *p = (use_binary_prefix > 0 ? (Prefix*) CALCULATOR->getOptimalBinaryPrefix(exp10, exp2) : (Prefix*) CALCULATOR->getOptimalDecimalPrefix(exp10, exp2, po.use_all_prefixes));
							if(p) {
								Number test_exp(exp10);
								if(use_binary_prefix) test_exp -= ((BinaryPrefix*) p)->exponent(exp2);
								else test_exp -= ((DecimalPrefix*) p)->exponent(exp2);
								if(test_exp.isInteger()) {
									if((exp10.isPositive() && exp10.compare(test_exp) == COMPARISON_RESULT_LESS) || (exp10.isNegative() && exp10.compare(test_exp) == COMPARISON_RESULT_GREATER)) {
										CHILD(0).number() /= p->value(exp2);
										if(munit2->isUnit()) munit2->setPrefix(p);
										else (*munit2)[0].setPrefix(p);
									}
								}
							}
						}
					}
				}
				return;
			}
			break;
		}
		case STRUCT_UNIT: {
			if(!o_prefix && (po.prefix && po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix)) {
				transform(STRUCT_MULTIPLICATION, m_one);
				SWAP_CHILDREN(0, 1);
				setPrefixes(po, parent, pindex);
			}
			return;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isUnit()) {
				if(CHILD(1).isNumber() && CHILD(1).number().isReal() && !CHILD(0).prefix() && !o_prefix && (po.prefix && po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix)) {
					transform(STRUCT_MULTIPLICATION, m_one);
					SWAP_CHILDREN(0, 1);
					setPrefixes(po, parent, pindex);
				}
				return;
			}
			break;
		}
		default: {}
	}
	if(po.prefix || !has_prefix(*this)) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			CHILD(i).setPrefixes(po, this, i + 1);
		}
	}
}
bool split_unit_powers(MathStructure &mstruct);
bool split_unit_powers(MathStructure &mstruct) {
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) break;
		if(split_unit_powers(mstruct[i])) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	if(mstruct.isPower() && mstruct[0].isMultiplication()) {
		bool b2 = mstruct[1].isNumber();
		for(size_t i = 0; i < mstruct[0].size(); i++) {
			if(mstruct[0][i].isPower() && (!b2 || !mstruct[0][i][1].isNumber())) return b;
		}
		MathStructure mpower(mstruct[1]);
		mstruct.setToChild(1);
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower()) mstruct[i][1].number() *= mpower.number();
			else mstruct[i].raise(mpower);
		}
		mstruct.childrenUpdated();
		return true;
	}
	return b;
}
void MathStructure::postFormatUnits(const PrintOptions &po, MathStructure*, size_t) {
	switch(m_type) {
		case STRUCT_DIVISION: {
			if(po.place_units_separately) {
				vector<size_t> nums;
				bool b1 = false, b2 = false;
				if(CHILD(0).isMultiplication()) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isUnit_exp()) {
							nums.push_back(i);
						} else {
							b1 = true;
						}
					}
					b1 = b1 && !nums.empty();
				} else if(CHILD(0).isUnit_exp()) {
					b1 = true;
				}
				vector<size_t> dens;
				if(CHILD(1).isMultiplication()) {
					for(size_t i = 0; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isUnit_exp()) {
							dens.push_back(i);
						} else {
							b2 = true;
						}
					}
					b2 = b2 && !dens.empty();
				} else if(CHILD(1).isUnit_exp()) {
					if(CHILD(0).isUnit_exp()) {
						b1 = false;
					} else {
						b2 = true;
					}
				}
				if(b2 && !b1) b1 = true;
				if(b1) {
					MathStructure num = m_undefined;
					if(CHILD(0).isUnit_exp()) {
						num = CHILD(0);
						CHILD(0).set(m_one);
					} else if(nums.size() > 0) {
						num = CHILD(0)[nums[0]];
						for(size_t i = 1; i < nums.size(); i++) {
							num.multiply(CHILD(0)[nums[i]], i > 1);
						}
						for(size_t i = 0; i < nums.size(); i++) {
							CHILD(0).delChild(nums[i] + 1 - i);
						}
						if(CHILD(0).size() == 1) {
							CHILD(0).setToChild(1, true);
						}
					}
					MathStructure den = m_undefined;
					if(CHILD(1).isUnit_exp()) {
						den = CHILD(1);
						setToChild(1, true);
					} else if(dens.size() > 0) {
						den = CHILD(1)[dens[0]];
						for(size_t i = 1; i < dens.size(); i++) {
							den.multiply(CHILD(1)[dens[i]], i > 1);
						}
						for(size_t i = 0; i < dens.size(); i++) {
							CHILD(1).delChild(dens[i] + 1 - i);
						}
						if(CHILD(1).size() == 1) {
							CHILD(1).setToChild(1, true);
						}
					}
					if(num.isUndefined()) {
						transform(STRUCT_DIVISION, den);
					} else {
						if(!den.isUndefined()) {
							num.transform(STRUCT_DIVISION, den);
						}
						multiply(num, false);
					}
					if(CHILD(0).isDivision()) {
						if(CHILD(0)[0].isMultiplication()) {
							if(CHILD(0)[0].size() == 1) {
								CHILD(0)[0].setToChild(1, true);
							} else if(CHILD(0)[0].size() == 0) {
								CHILD(0)[0] = 1;
							}
						}
						if(CHILD(0)[1].isMultiplication()) {
							if(CHILD(0)[1].size() == 1) {
								CHILD(0)[1].setToChild(1, true);
							} else if(CHILD(0)[1].size() == 0) {
								CHILD(0).setToChild(1, true);
							}
						} else if(CHILD(0)[1].isOne()) {
							CHILD(0).setToChild(1, true);
						}
						if(CHILD(0).isDivision() && CHILD(0)[1].isNumber() && CHILD(0)[0].isMultiplication() && CHILD(0)[0].size() > 1 && CHILD(0)[0][0].isNumber()) {
							MathStructure *msave = new MathStructure;
							if(CHILD(0)[0].size() == 2) {
								msave->set(CHILD(0)[0][1]);
								CHILD(0)[0].setToChild(1, true);
							} else {
								msave->set(CHILD(0)[0]);
								CHILD(0)[0].setToChild(1, true);
								msave->delChild(1);
							}
							if(isMultiplication()) {
								insertChild_nocopy(msave, 2);
							} else {
								CHILD(0).multiply_nocopy(msave);
							}
						}
					}
					bool do_plural = po.short_multiplication;
					CHILD(0).postFormatUnits(po, this, 1);
					CHILD_UPDATED(0);
					switch(CHILD(0).type()) {
						case STRUCT_NUMBER: {
							if(CHILD(0).isZero() || CHILD(0).number().isOne() || CHILD(0).number().isMinusOne() || CHILD(0).number().isFraction()) {
								do_plural = false;
							}
							break;
						}
						case STRUCT_DIVISION: {
							if(CHILD(0)[0].isNumber() && CHILD(0)[1].isNumber()) {
								if(CHILD(0)[0].number().isLessThanOrEqualTo(CHILD(0)[1].number())) {
									do_plural = false;
								}
							}
							break;
						}
						case STRUCT_INVERSE: {
							if(CHILD(0)[0].isNumber() && CHILD(0)[0].number().isGreaterThanOrEqualTo(1)) {
								do_plural = false;
							}
							break;
						}
						default: {}
					}
					split_unit_powers(CHILD(1));
					switch(CHILD(1).type()) {
						case STRUCT_UNIT: {
							CHILD(1).setPlural(do_plural);
							break;
						}
						case STRUCT_POWER: {
							CHILD(1)[0].setPlural(do_plural);
							break;
						}
						case STRUCT_MULTIPLICATION: {
							if(po.limit_implicit_multiplication) CHILD(1)[0].setPlural(do_plural);
							else CHILD(1)[CHILD(1).size() - 1].setPlural(do_plural);
							break;
						}
						case STRUCT_DIVISION: {
							switch(CHILD(1)[0].type()) {
								case STRUCT_UNIT: {
									CHILD(1)[0].setPlural(do_plural);
									break;
								}
								case STRUCT_POWER: {
									CHILD(1)[0][0].setPlural(do_plural);
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(po.limit_implicit_multiplication) CHILD(1)[0][0].setPlural(do_plural);
									else CHILD(1)[0][CHILD(1)[0].size() - 1].setPlural(do_plural);
									break;
								}
								default: {}
							}
							break;
						}
						default: {}
					}
				}
			} else {
				for(size_t i = 0; i < SIZE; i++) {
					if(CALCULATOR->aborted()) break;
					CHILD(i).postFormatUnits(po, this, i + 1);
					CHILD_UPDATED(i);
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			b_plural = false;
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(SIZE > 1 && CHILD(1).isUnit_exp() && CHILD(0).isNumber()) {
				bool do_plural = po.short_multiplication && !(CHILD(0).isZero() || CHILD(0).number().isOne() || CHILD(0).number().isMinusOne() || CHILD(0).number().isFraction());
				size_t i = 2;
				for(; i < SIZE; i++) {
					if(CALCULATOR->aborted()) break;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(false);
					} else if(CHILD(i).isPower() && CHILD(i)[0].isUnit()) {
						CHILD(i)[0].setPlural(false);
					} else {
						break;
					}
				}
				if(do_plural) {
					if(po.limit_implicit_multiplication) i = 1;
					else i--;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(true);
					} else {
						CHILD(i)[0].setPlural(true);
					}
				}
			} else if(SIZE > 0) {
				int last_unit = -1;
				for(size_t i = 0; i < SIZE; i++) {
					if(CALCULATOR->aborted()) break;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(false);
						if(!po.limit_implicit_multiplication || last_unit < 0) {
							last_unit = i;
						}
					} else if(CHILD(i).isPower() && CHILD(i)[0].isUnit()) {
						CHILD(i)[0].setPlural(false);
						if(!po.limit_implicit_multiplication || last_unit < 0) {
							last_unit = i;
						}
					} else if(last_unit >= 0) {
						break;
					}
				}
				if(po.short_multiplication && last_unit > 0) {
					if(CHILD(last_unit).isUnit()) {
						CHILD(last_unit).setPlural(true);
					} else {
						CHILD(last_unit)[0].setPlural(true);
					}
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isUnit()) {
				CHILD(0).setPlural(false);
				break;
			}
		}
		default: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				CHILD(i).postFormatUnits(po, this, i + 1);
				CHILD_UPDATED(i);
			}
		}
	}
}
bool MathStructure::factorizeUnits() {
	switch(m_type) {
		case STRUCT_ADDITION: {
			if(containsType(STRUCT_DATETIME, false, true, false) > 0) return false;
			bool b = false;
			MathStructure mstruct_units(*this);
			MathStructure mstruct_new(*this);
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(CALCULATOR->aborted()) break;
				if(mstruct_units[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
						if(CALCULATOR->aborted()) break;
						if(!mstruct_units[i][i2].isUnit_exp()) {
							mstruct_units[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
					else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
					for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
						if(CALCULATOR->aborted()) break;
						if(mstruct_new[i][i2].isUnit_exp()) {
							mstruct_new[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
					else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
				} else if(mstruct_units[i].isUnit_exp()) {
					mstruct_new[i].set(1, 1, 0);
				} else {
					mstruct_units[i].setUndefined();
				}
			}
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(CALCULATOR->aborted()) break;
				if(!mstruct_units[i].isUndefined()) {
					for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
						if(mstruct_units[i2] == mstruct_units[i]) {
							mstruct_new[i].add(mstruct_new[i2], true);
							mstruct_new.delChild(i2 + 1);
							mstruct_units.delChild(i2 + 1);
							b = true;
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
					else mstruct_new[i].multiply(mstruct_units[i], true);
				}
			}
			if(b) {
				if(mstruct_new.size() == 1) set(mstruct_new[0], true);
				else set(mstruct_new, true);
				return true;
			}
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				if(CHILD(i).factorizeUnits()) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
	}
}
void MathStructure::prefixCurrencies(const PrintOptions &po) {
	if(isMultiplication()) {
		int index = -1;
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			if(CHILD(i).isUnit_exp()) {
				if(CHILD(i).isUnit() && CHILD(i).unit()->isCurrency()) {
					const ExpressionName *ename = &CHILD(i).unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, CHILD(i).isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
					bool do_prefix = false;
					if(ename->reference) do_prefix = (hasNegativeSign() ? CALCULATOR->place_currency_code_before_negative : CALCULATOR->place_currency_code_before);
					else if(ename->abbreviation) do_prefix = (hasNegativeSign() ? CALCULATOR->place_currency_sign_before_negative : CALCULATOR->place_currency_sign_before);
					if(!do_prefix || index >= 0) {
						index = -1;
						break;
					}
					index = i;
				} else {
					index = -1;
					break;
				}
			}
		}
		if(index >= 0) {
			v_order.insert(v_order.begin(), v_order[index]);
			v_order.erase(v_order.begin() + (index + 1));
		}
	} else {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			CHILD(i).prefixCurrencies(po);
		}
	}
}
void remove_multi_one(MathStructure &mstruct) {
	if(mstruct.isMultiplication() && mstruct.size() > 1) {
		if(mstruct[0].isOne() && !mstruct[1].isUnit_exp() && (mstruct.size() != 2 || !mstruct[1].isFunction() || mstruct[1].function()->referenceName() != "cis" || mstruct[1].size() != 1)) {
			if(mstruct.size() == 2) mstruct.setToChild(2, true);
			else mstruct.delChild(1);
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) break;
		remove_multi_one(mstruct[i]);
	}
}
bool unnegate_multiplier(MathStructure &mstruct, const PrintOptions &po) {
	if(mstruct.isMultiplication() && mstruct.size() >= 2 && mstruct[0].isNumber() && mstruct[0].number().isNegative()) {
		for(size_t i = 1; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) break;
			if(mstruct[i].isAddition() || (mstruct[i].isPower() && mstruct[i][0].isAddition() && mstruct[i][1].isMinusOne())) {
				MathStructure *mden;
				if(mstruct[i].isAddition()) mden = &mstruct[i];
				else mden = &mstruct[i][0];
				bool b_pos = false, b_neg = false;
				for(size_t i2 = 0; i2 < mden->size(); i2++) {
					if((*mden)[i2].hasNegativeSign()) {
						b_neg = true;
					} else {
						b_pos = true;
					}
					if(b_neg && b_pos) break;
				}
				if(b_neg && b_pos) {
					for(size_t i2 = 0; i2 < mden->size(); i2++) {
						if((*mden)[i2].isNumber()) {
							(*mden)[i2].number().negate();
						} else if((*mden)[i2].isMultiplication() && (*mden)[i2].size() > 0) {
							if((*mden)[i2][0].isNumber()) {
								if((*mden)[i2][0].number().isMinusOne() && (*mden)[i2].size() > 1) {
									if((*mden)[i2].size() == 2) (*mden)[i2].setToChild(2, true);
									else (*mden)[i2].delChild(1);
								} else (*mden)[i2][0].number().negate();
							} else {
								(*mden)[i2].insertChild(m_minus_one, 1);
							}
						} else {
							(*mden)[i2].negate();
						}
					}
					mden->sort(po, false);
					if(mstruct[0].number().isMinusOne()) {
						if(mstruct.size() == 2) mstruct.setToChild(2, true);
						else mstruct.delChild(1);
					} else {
						mstruct[0].number().negate();
					}
					return true;
				}
			}
		}
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) break;
		if(unnegate_multiplier(mstruct[i], po)) {
			b = true;
		}
	}
	if(b) {
		mstruct.sort(po, false);
		return true;
	}
	return false;
}
Unit *default_angle_unit(const EvaluationOptions &eo) {
	switch(eo.parse_options.angle_unit) {
		case ANGLE_UNIT_DEGREES: {return CALCULATOR->getDegUnit();}
		case ANGLE_UNIT_GRADIANS: {return CALCULATOR->getGraUnit();}
		case ANGLE_UNIT_RADIANS: {return CALCULATOR->getRadUnit();}
		default: {}
	}
	return NULL;
}

bool remove_angle_unit(MathStructure &m, Unit *u) {
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(remove_angle_unit(m[i], u)) b_ret = true;
		if(m.isFunction() && m.function()->getArgumentDefinition(i + 1) && m.function()->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_ANGLE) {
			if(m[i].isMultiplication()) {
				for(size_t i3 = 0; i3 < m[i].size(); i3++) {
					if(m[i][i3].isUnit() && !m[i][i3].prefix() && m[i][i3].unit() == u) {
						m[i].delChild(i3 + 1, true);
						b_ret = true;
						break;
					}
				}
			} else if(m[i].isAddition()) {
				bool b = true;
				for(size_t i2 = 0; i2 < m[i].size(); i2++) {
					bool b2 = false;
					if(m[i][i2].isMultiplication()) {
						for(size_t i3 = 0; i3 < m[i][i2].size(); i3++) {
							if(m[i][i2][i3].isUnit() && !m[i][i2][i3].prefix() && m[i][i2][i3].unit() == u) {
								b2 = true;
								break;
							}
						}
					}
					if(!b2) {
						b = false;
						break;
					}
				}
				if(b) {
					b_ret = true;
					for(size_t i2 = 0; i2 < m[i].size(); i2++) {
						for(size_t i3 = 0; i3 < m[i][i2].size(); i3++) {
							if(m[i][i2][i3].isUnit() && !m[i][i2][i3].prefix() && m[i][i2][i3].unit() == u) {
								m[i][i2].delChild(i3 + 1, true);
								break;
							}
						}
					}
				}
			}
		}
	}
	return b_ret;
}
bool MathStructure::removeDefaultAngleUnit(const EvaluationOptions &eo) {
	Unit *u = default_angle_unit(eo);
	if(!u) return false;
	return remove_angle_unit(*this, u);
}
void MathStructure::format(const PrintOptions &po) {
	if(!po.preserve_format) {
		if(po.place_units_separately) {
			factorizeUnits();
		}
		sort(po);
		setPrefixes(po);
		unnegate_multiplier(*this, po);
		fix_approximate_multiplier(*this, po);
		if(po.improve_division_multipliers) {
			if(improve_division_multipliers(po)) sort(po);
		}
		remove_multi_one(*this);
	}
	formatsub(po, NULL, 0, true, this);
	if(!po.preserve_format) {
		postFormatUnits(po);
		if(po.sort_options.prefix_currencies) {
			prefixCurrencies(po);
		}
	}
}

bool MathStructure::complexToExponentialForm(const EvaluationOptions &eo) {
	if(m_type == STRUCT_NUMBER && o_number.hasImaginaryPart()) {
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		eo2.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
		MathStructure mabs(CALCULATOR->f_abs, this, NULL);
		MathStructure marg(CALCULATOR->f_arg, this, NULL);
		marg *= nr_one_i;
		marg.eval(eo2);
		set(CALCULATOR->v_e, true);
		raise(marg);
		mabs.eval(eo2);
		if(!mabs.isOne()) multiply(mabs);
		evalSort(false);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).complexToExponentialForm(eo)) {b = true; CHILD_UPDATED(i);}
	}
	return b;
}
bool MathStructure::complexToPolarForm(const EvaluationOptions &eo) {
	if(m_type == STRUCT_NUMBER && o_number.hasImaginaryPart()) {
		MathStructure mabs(CALCULATOR->f_abs, this, NULL);
		MathStructure marg(CALCULATOR->f_arg, this, NULL);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		mabs.eval(eo2);
		marg.eval(eo2);
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {if(CALCULATOR->getDegUnit()) {marg.multiply(CALCULATOR->getDegUnit(), true);} break;}
			case ANGLE_UNIT_GRADIANS: {if(CALCULATOR->getGraUnit()) {marg.multiply(CALCULATOR->getGraUnit(), true);} break;}
			case ANGLE_UNIT_RADIANS: {if(CALCULATOR->getRadUnit()) {marg.multiply(CALCULATOR->getRadUnit(), true);} break;}
			default: {break;}
		}
		set(marg, true);
		transform(CALCULATOR->f_sin);
		multiply(nr_one_i);
		add_nocopy(new MathStructure(CALCULATOR->f_cos, &marg, NULL));
		if(!mabs.isOne()) multiply(mabs);
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_POWER && CHILD(0).isVariable() && CHILD(0).variable() == CALCULATOR->v_e && CHILD(1).isNumber() && CHILD(1).number().hasImaginaryPart() && !CHILD(1).number().hasRealPart()) {
		MathStructure marg(CHILD(1).number().imaginaryPart());
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {if(CALCULATOR->getDegUnit()) {marg.calculateMultiply(Number(180, 1), eo2); marg.calculateDivide(CALCULATOR->v_pi, eo2); marg.multiply(CALCULATOR->getDegUnit(), true);} break;}
			case ANGLE_UNIT_GRADIANS: {if(CALCULATOR->getGraUnit()) {marg.calculateMultiply(Number(200, 1), eo2); marg.calculateDivide(CALCULATOR->v_pi, eo2); marg.multiply(CALCULATOR->getGraUnit(), true);} break;}
			case ANGLE_UNIT_RADIANS: {if(CALCULATOR->getRadUnit()) {marg.multiply(CALCULATOR->getRadUnit(), true);} break;}
			default: {break;}
		}
		set(marg, true);
		transform(CALCULATOR->f_sin);
		multiply(nr_one_i);
		add_nocopy(new MathStructure(CALCULATOR->f_cos, &marg, NULL));
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_MULTIPLICATION && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[0].isVariable() && CHILD(1)[0].variable() == CALCULATOR->v_e && CHILD(1)[1].isNumber() && CHILD(1)[1].number().hasImaginaryPart() && !CHILD(1)[1].number().hasRealPart() && CHILD(0).isNumber() && !CHILD(0).number().hasImaginaryPart()) {
		MathStructure marg(CHILD(1)[1].number().imaginaryPart());
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {if(CALCULATOR->getDegUnit()) {marg.calculateMultiply(Number(180, 1), eo2); marg.calculateDivide(CALCULATOR->v_pi, eo2); marg.multiply(CALCULATOR->getDegUnit(), true);} break;}
			case ANGLE_UNIT_GRADIANS: {if(CALCULATOR->getGraUnit()) {marg.calculateMultiply(Number(200, 1), eo2); marg.calculateDivide(CALCULATOR->v_pi, eo2); marg.multiply(CALCULATOR->getGraUnit(), true);} break;}
			case ANGLE_UNIT_RADIANS: {if(CALCULATOR->getRadUnit()) {marg.multiply(CALCULATOR->getRadUnit(), true);} break;}
			default: {break;}
		}
		CHILD(1).set(marg, true);
		CHILD(1).transform(CALCULATOR->f_sin);
		CHILD(1).multiply(nr_one_i);
		CHILD(1).add_nocopy(new MathStructure(CALCULATOR->f_cos, &marg, NULL));
		CHILD_UPDATED(1)
		evalSort(true);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).complexToPolarForm(eo)) {b = true; CHILD_UPDATED(i);}
	}
	return b;
}
bool MathStructure::complexToCisForm(const EvaluationOptions &eo) {
	if(m_type == STRUCT_NUMBER && o_number.hasImaginaryPart()) {
		MathFunction *f = CALCULATOR->getActiveFunction("cis");
		if(!f) return false;
		MathStructure mabs(CALCULATOR->f_abs, this, NULL);
		MathStructure marg(CALCULATOR->f_arg, this, NULL);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		mabs.eval(eo2);
		marg.eval(eo2);
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {if(CALCULATOR->getDegUnit()) {marg.multiply(CALCULATOR->getDegUnit(), true);} break;}
			case ANGLE_UNIT_GRADIANS: {if(CALCULATOR->getGraUnit()) {marg.multiply(CALCULATOR->getGraUnit(), true);} break;}
			default: {break;}
		}
		set(marg, true);
		transform(f);
		multiply(mabs);
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_POWER && CHILD(0).isVariable() && CHILD(0).variable() == CALCULATOR->v_e && CHILD(1).isNumber() && CHILD(1).number().hasImaginaryPart() && !CHILD(1).number().hasRealPart()) {
		MathFunction *f = CALCULATOR->getActiveFunction("cis");
		if(!f) return false;
		set(CHILD(1).number().imaginaryPart(), true);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {if(CALCULATOR->getDegUnit()) {calculateMultiply(Number(180, 1), eo2); calculateDivide(CALCULATOR->v_pi, eo2); multiply(CALCULATOR->getDegUnit(), true);} break;}
			case ANGLE_UNIT_GRADIANS: {if(CALCULATOR->getGraUnit()) {calculateMultiply(Number(200, 1), eo2); calculateDivide(CALCULATOR->v_pi, eo2); multiply(CALCULATOR->getGraUnit(), true);} break;}
			default: {break;}
		}
		transform(f);
		multiply(m_one);
		evalSort(true);
		return true;
	} else if(m_type == STRUCT_MULTIPLICATION && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[0].isVariable() && CHILD(1)[0].variable() == CALCULATOR->v_e && CHILD(1)[1].isNumber() && CHILD(1)[1].number().hasImaginaryPart() && !CHILD(1)[1].number().hasRealPart() && CHILD(0).isNumber() && !CHILD(0).number().hasImaginaryPart()) {
		MathFunction *f = CALCULATOR->getActiveFunction("cis");
		if(!f) return false;
		CHILD(1).set(CHILD(1)[1].number().imaginaryPart(), true);
		EvaluationOptions eo2 = eo;
		eo2.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
		switch(eo2.parse_options.angle_unit) {
			case ANGLE_UNIT_DEGREES: {if(CALCULATOR->getDegUnit()) {CHILD(1).calculateMultiply(Number(180, 1), eo2); CHILD(1).calculateDivide(CALCULATOR->v_pi, eo2); CHILD(1).multiply(CALCULATOR->getDegUnit(), true);} break;}
			case ANGLE_UNIT_GRADIANS: {if(CALCULATOR->getGraUnit()) {CHILD(1).calculateMultiply(Number(200, 1), eo2); CHILD(1).calculateDivide(CALCULATOR->v_pi, eo2); CHILD(1).multiply(CALCULATOR->getGraUnit(), true);} break;}
			default: {break;}
		}
		CHILD(1).transform(f);
		CHILD_UPDATED(1)
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).complexToCisForm(eo)) {b = true; CHILD_UPDATED(i);}
	}
	return b;
}

void MathStructure::formatsub(const PrintOptions &po, MathStructure *parent, size_t pindex, bool recursive, MathStructure *top_parent) {

	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) break;
			if(i == 1 && m_type == STRUCT_POWER && po.number_fraction_format < FRACTION_FRACTIONAL && CHILD(1).isNumber() && CHILD(1).number().isRational() && !CHILD(1).number().isInteger() && CHILD(1).number().numeratorIsLessThan(10) && CHILD(1).number().numeratorIsGreaterThan(-10) && CHILD(1).number().denominatorIsLessThan(10)) {
				PrintOptions po2 = po;
				po2.number_fraction_format = FRACTION_FRACTIONAL;
				CHILD(i).formatsub(po2, this, i + 1, false, top_parent);
			} else {
				CHILD(i).formatsub(po, this, i + 1, true, top_parent);
			}
			CHILD_UPDATED(i);
		}
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			break;
		}
		case STRUCT_NEGATE: {
			break;
		}
		case STRUCT_DIVISION: {
			if(po.preserve_format) break;
			if(CHILD(0).isAddition() && CHILD(0).size() > 0 && CHILD(0)[0].isNegate()) {
				int imin = 1;
				for(size_t i = 1; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				bool b = CHILD(1).isAddition() && CHILD(1).size() > 0 && CHILD(1)[0].isNegate();
				if(b) {
					imin++;
					for(size_t i = 1; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isNegate()) {
							imin++;
						} else {
							imin--;
						}
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isNegate()) {
							CHILD(0)[i].setToChild(1, true);
						} else {
							CHILD(0)[i].transform(STRUCT_NEGATE);
						}
					}
					if(b) {
						for(size_t i = 0; i < CHILD(1).size(); i++) {
							if(CHILD(1)[i].isNegate()) {
								CHILD(1)[i].setToChild(1, true);
							} else {
								CHILD(1)[i].transform(STRUCT_NEGATE);
							}
						}
					} else {
						transform(STRUCT_NEGATE);
					}
					break;
				}
			} else if(CHILD(1).isAddition() && CHILD(1).size() > 0 && CHILD(1)[0].isNegate()) {
				int imin = 1;
				for(size_t i = 1; i < CHILD(1).size(); i++) {
					if(CHILD(1)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isNegate()) {
							CHILD(1)[i].setToChild(1, true);
						} else {
							CHILD(1)[i].transform(STRUCT_NEGATE);
						}
					}
					transform(STRUCT_NEGATE);
				}
			}
			break;
		}
		case STRUCT_INVERSE: {
			if(po.preserve_format) break;
			if((!parent || !parent->isMultiplication()) && CHILD(0).isAddition() && CHILD(0).size() > 0 && CHILD(0)[0].isNegate()) {
				int imin = 1;
				for(size_t i = 1; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isNegate()) {
							CHILD(0)[i].setToChild(1, true);
						} else {
							CHILD(0)[i].transform(STRUCT_NEGATE);
						}
					}
					transform(STRUCT_NEGATE);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(po.preserve_format) break;
			if(CHILD(0).isNegate()) {
				if(CHILD(0)[0].isOne()) {
					ERASE(0);
					if(SIZE == 1) {
						setToChild(1, true);
					}
				} else {
					CHILD(0).setToChild(1, true);
				}
				transform(STRUCT_NEGATE);
				CHILD(0).formatsub(po, this, 1, false, top_parent);
				break;
			}

			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) break;
				if(CHILD(i).isInverse()) {
					if(!po.negative_exponents || !CHILD(i)[0].isNumber()) {
						b = true;
						break;
					}
				} else if(CHILD(i).isDivision()) {
					if(!CHILD(i)[0].isNumber() || !CHILD(i)[1].isNumber() || (!po.negative_exponents && CHILD(i)[0].number().isOne())) {
						b = true;
						break;
					}
				}
			}

			if(b) {
				MathStructure *den = new MathStructure();
				MathStructure *num = new MathStructure();
				num->setUndefined();
				short ds = 0, ns = 0;
				MathStructure *mnum = NULL, *mden = NULL;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isInverse()) {
						mden = &CHILD(i)[0];
					} else if(CHILD(i).isDivision()) {
						mnum = &CHILD(i)[0];
						mden = &CHILD(i)[1];
					} else {
						mnum = &CHILD(i);
					}
					if(mnum && !mnum->isOne()) {
						if(ns > 0) {
							if(mnum->isMultiplication() && num->isNumber()) {
								for(size_t i2 = 0; i2 < mnum->size(); i2++) {
									num->multiply((*mnum)[i2], true);
								}
							} else {
								num->multiply(*mnum, ns > 1);
							}
						} else {
							num->set(*mnum);
						}
						ns++;
						mnum = NULL;
					}
					if(mden) {
						if(ds > 0) {
							if(mden->isMultiplication() && den->isNumber()) {
								for(size_t i2 = 0; i2 < mden->size(); i2++) {
									den->multiply((*mden)[i2], true);
								}
							} else {
								den->multiply(*mden, ds > 1);
							}
						} else {
							den->set(*mden);
						}
						ds++;
						mden = NULL;
					}
				}
				clear(true);
				m_type = STRUCT_DIVISION;
				if(num->isUndefined()) num->set(m_one);
				APPEND_POINTER(num);
				APPEND_POINTER(den);
				num->formatsub(po, this, 1, false, top_parent);
				den->formatsub(po, this, 2, false, top_parent);
				formatsub(po, parent, pindex, false, top_parent);
				break;
			}

			size_t index = 0;
			if(CHILD(0).isOne()) {
				index = 1;
			}
			switch(CHILD(index).type()) {
				case STRUCT_POWER: {
					if(!CHILD(index)[0].isUnit_exp()) {
						break;
					}
				}
				case STRUCT_UNIT: {
					if(index == 0) {
						if(!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && (!parent->isDivision() || pindex != 2))) {
							PREPEND(m_one);
							break;
						}
					}
					break;
				}
				case STRUCT_FUNCTION: {
					if(index == 1 && SIZE == 2 && CHILD(index).function()->referenceName() == "cis" && CHILD(index).size() == 1) break;
				}
				default: {
					if(index == 1) {
						ERASE(0);
						if(SIZE == 1) {
							setToChild(1, true);
						}
						break;
					}
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			if(po.preserve_format) break;
			if(!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && !(parent->isDivision() && pindex == 2))) {
				multiply(m_one);
				SWAP_CHILDREN(0, 1);
			}
			break;
		}
		case STRUCT_POWER: {
			if(po.preserve_format) break;
			if(!po.negative_exponents && CHILD(1).isNegate() && (!CHILD(0).isVector() || !CHILD(1).isMinusOne())) {
				if(CHILD(1)[0].isOne()) {
					m_type = STRUCT_INVERSE;
					ERASE(1);
				} else {
					CHILD(1).setToChild(1, true);
					transform(STRUCT_INVERSE);
				}
				formatsub(po, parent, pindex, true, top_parent);
				break;
			} else if(po.halfexp_to_sqrt && ((CHILD(1).isDivision() && CHILD(1)[0].isNumber() && CHILD(1)[0].number().isInteger() && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && ((!po.negative_exponents && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1)[0].isOne())) || (CHILD(1).isNumber() && CHILD(1).number().denominatorIsTwo() && ((!po.negative_exponents && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1).number().numeratorIsOne())) || (CHILD(1).isInverse() && CHILD(1)[0].isNumber() && CHILD(1)[0].number() == 2))) {
				if(CHILD(1).isInverse() || (CHILD(1).isDivision() && CHILD(1)[0].number().isOne()) || (CHILD(1).isNumber() && CHILD(1).number().numeratorIsOne())) {
					m_type = STRUCT_FUNCTION;
					ERASE(1)
					setFunction(CALCULATOR->f_sqrt);
				} else {
					if(CHILD(1).isNumber()) {
						CHILD(1).number() -= nr_half;
					} else {
						Number nr = CHILD(1)[0].number();
						nr /= CHILD(1)[1].number();
						nr.floor();
						CHILD(1).set(nr);
					}
					if(CHILD(1).number().isOne()) {
						setToChild(1, true);
						if(parent && parent->isMultiplication()) {
							parent->insertChild(MathStructure(CALCULATOR->f_sqrt, this, NULL), pindex + 1);
						} else {
							multiply(MathStructure(CALCULATOR->f_sqrt, this, NULL));
						}
					} else {
						if(parent && parent->isMultiplication()) {
							parent->insertChild(MathStructure(CALCULATOR->f_sqrt, &CHILD(0), NULL), pindex + 1);
						} else {
							multiply(MathStructure(CALCULATOR->f_sqrt, &CHILD(0), NULL));
						}
					}
				}
				formatsub(po, parent, pindex, false, top_parent);
				break;
			} else if(po.exp_to_root && CHILD(0).representsNonNegative(true) && ((CHILD(1).isDivision() && CHILD(1)[0].isNumber() && CHILD(1)[0].number().isInteger() && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isGreaterThan(1) && CHILD(1)[1].number().isLessThan(10) && ((!po.negative_exponents && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1)[0].isOne())) || (CHILD(1).isNumber() && CHILD(1).number().isRational() && !CHILD(1).number().isInteger() && CHILD(1).number().denominatorIsLessThan(10) && ((!po.negative_exponents && (CHILD(0).countChildren() == 0 || CHILD(0).isFunction())) || CHILD(1).number().numeratorIsOne())) || (CHILD(1).isInverse() && CHILD(1)[0].isNumber()  && CHILD(1)[0].number().isInteger() && CHILD(1)[0].number().isPositive() && CHILD(1)[0].number().isLessThan(10)))) {
				Number nr_int, nr_num, nr_den;
				if(CHILD(1).isNumber()) {
					nr_num = CHILD(1).number().numerator();
					nr_den = CHILD(1).number().denominator();
				} else if(CHILD(1).isDivision()) {
					nr_num.set(CHILD(1)[0].number());
					nr_den.set(CHILD(1)[1].number());
				} else if(CHILD(1).isInverse()) {
					nr_num.set(1, 1, 0);
					nr_den.set(CHILD(1)[0].number());
				}
				if(!nr_num.isOne() && (nr_num - 1).isIntegerDivisible(nr_den)) {
					nr_int = nr_num;
					nr_int--;
					nr_int.divide(nr_den);
					nr_num = 1;
				}
				MathStructure mbase(CHILD(0));
				CHILD(1) = nr_den;
				m_type = STRUCT_FUNCTION;
				setFunction(CALCULATOR->f_root);
				formatsub(po, parent, pindex, false, top_parent);
				if(!nr_num.isOne()) {
					raise(nr_num);
					formatsub(po, parent, pindex, false, top_parent);
				}
				if(!nr_int.isZero()) {
					if(!nr_int.isOne()) mbase.raise(nr_int);
					multiply(mbase);
					sort(po);
					formatsub(po, parent, pindex, false, top_parent);
				}
				break;
			}
			if(CHILD(0).isUnit_exp() && (!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && !(parent->isDivision() && pindex == 2)))) {
				multiply(m_one);
				SWAP_CHILDREN(0, 1);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(po.preserve_format) break;
			if(o_function == CALCULATOR->f_root && SIZE == 2 && CHILD(1) == 3) {
				ERASE(1)
				setFunction(CALCULATOR->f_cbrt);
			} else if(o_function == CALCULATOR->f_interval && SIZE == 2 && CHILD(0).isAddition() && CHILD(0).size() == 2 && CHILD(1).isAddition() && CHILD(1).size() == 2) {
				MathStructure *mmid = NULL, *munc = NULL;
				if(CHILD(0)[0].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[1];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				} else if(CHILD(0)[0].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				}
				if(mmid && munc) {
					setFunction(CALCULATOR->f_uncertainty);
					mmid->ref();
					munc->ref();
					CLEAR
					APPEND_POINTER(mmid)
					APPEND_POINTER(munc)
					APPEND(m_zero)
				}
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(o_variable == CALCULATOR->v_pinf || o_variable == CALCULATOR->v_minf) {
				set(((KnownVariable*) o_variable)->get());
			}
			break;
		}
		case STRUCT_NUMBER: {
			bool force_fraction = false;
			if(parent && parent->isMultiplication() && o_number.isRational()) {
				for(size_t i = 0; i < parent->size(); i++) {
					if((*parent)[i].isAddition()) {
						force_fraction = true;
						break;
					}
				}
			}
			if((o_number.isNegative() || ((parent || po.interval_display != INTERVAL_DISPLAY_SIGNIFICANT_DIGITS) && o_number.isInterval() && o_number.isNonPositive())) && (po.base != BASE_CUSTOM || !CALCULATOR->customOutputBase().isNegative())) {
				if((((po.base != 2 || !po.twos_complement) && (po.base != 16 || !po.hexadecimal_twos_complement)) || !o_number.isInteger()) && (!o_number.isMinusInfinity() || (parent && parent->isAddition()))) {
					o_number.negate();
					transform(STRUCT_NEGATE);
					formatsub(po, parent, pindex, true, top_parent);
				}
			} else if(po.number_fraction_format == FRACTION_COMBINED && po.base != BASE_SEXAGESIMAL && po.base != BASE_TIME && o_number.isRational() && !o_number.isInteger() && (!po.show_ending_zeroes || !po.restrict_to_parent_precision || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26 || ((!top_parent || !top_parent->isApproximate()) && !isApproximate()) || (o_number.denominatorIsLessThan(10) && o_number < 10 && o_number > -10))) {
				if(o_number.isFraction()) {
					Number num(o_number.numerator());
					Number den(o_number.denominator());
					clear(true);
					if(num.isOne()) {
						m_type = STRUCT_INVERSE;
					} else {
						m_type = STRUCT_DIVISION;
						APPEND_NEW(num);
					}
					APPEND_NEW(den);
				} else {
					Number frac(o_number);
					frac.frac();
					MathStructure *num = new MathStructure(frac.numerator());
					num->transform(STRUCT_DIVISION, frac.denominator());
					o_number.trunc();
					add_nocopy(num);
				}
			} else if((force_fraction || po.number_fraction_format == FRACTION_FRACTIONAL || po.base == BASE_ROMAN_NUMERALS || po.number_fraction_format == FRACTION_DECIMAL_EXACT) && po.base != BASE_SEXAGESIMAL && po.base != BASE_TIME && o_number.isRational() && !o_number.isInteger() && (force_fraction || !o_number.isApproximate())) {
				InternalPrintStruct ips_n;
				if(!force_fraction && (isApproximate() || (top_parent && top_parent->isApproximate()))) ips_n.parent_approximate = true;
				if(po.show_ending_zeroes && po.restrict_to_parent_precision && ips_n.parent_approximate && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_BIJECTIVE_26 && (o_number.numeratorIsGreaterThan(9) || o_number.numeratorIsLessThan(-9) || o_number.denominatorIsGreaterThan(9))) {
					break;
				}
				ips_n.parent_precision = precision();
				if(top_parent && top_parent->precision() < 0 && top_parent->precision() < ips_n.parent_precision) ips_n.parent_precision = top_parent->precision();
				bool approximately_displayed = false;
				PrintOptions po2 = po;
				po2.is_approximate = &approximately_displayed;
				if(!force_fraction && po.base != BASE_ROMAN_NUMERALS && po.base != BASE_BIJECTIVE_26 && po.number_fraction_format == FRACTION_DECIMAL_EXACT) {
					po2.number_fraction_format = FRACTION_DECIMAL;
					o_number.print(po2, ips_n);
					if(!approximately_displayed) break;
					approximately_displayed = false;
				}
				Number num(o_number.numerator());
				Number den(o_number.denominator());
				if(isApproximate()) {
					num.setApproximate();
					den.setApproximate();
				}
				num.print(po2, ips_n);
				if(!approximately_displayed || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) {
					den.print(po2, ips_n);
					if(!approximately_displayed || po.base == BASE_ROMAN_NUMERALS || po.base == BASE_BIJECTIVE_26) {
						clear(true);
						if(num.isOne()) {
							m_type = STRUCT_INVERSE;
						} else {
							m_type = STRUCT_DIVISION;
							APPEND_NEW(num);
						}
						APPEND_NEW(den);
					}
				}
			} else if(o_number.hasImaginaryPart()) {
				if(o_number.hasRealPart()) {
					Number re(o_number.realPart());
					Number im(o_number.imaginaryPart());
					MathStructure *mstruct = new MathStructure(im);
					if(im.isOne()) {
						mstruct->set(CALCULATOR->v_i);
					} else {
						mstruct->multiply_nocopy(new MathStructure(CALCULATOR->v_i));
						if(CALCULATOR->v_i->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name == "j") mstruct->swapChildren(1, 2);
					}
					o_number = re;
					add_nocopy(mstruct);
					formatsub(po, parent, pindex, true, top_parent);
				} else {
					Number im(o_number.imaginaryPart());
					if(im.isOne()) {
						set(CALCULATOR->v_i, true);
					} else if(im.isMinusOne()) {
						set(CALCULATOR->v_i, true);
						transform(STRUCT_NEGATE);
					} else {
						o_number = im;
						multiply_nocopy(new MathStructure(CALCULATOR->v_i));
						if(CALCULATOR->v_i->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name == "j") SWAP_CHILDREN(0, 1);
					}
					formatsub(po, parent, pindex, true, top_parent);
				}
			}
			break;
		}
		default: {}
	}
}

int namelen(const MathStructure &mstruct, const PrintOptions &po, const InternalPrintStruct&, bool *abbreviated = NULL) {
	const string *str;
	switch(mstruct.type()) {
		case STRUCT_FUNCTION: {
			const ExpressionName *ename = &mstruct.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		case STRUCT_VARIABLE:  {
			const ExpressionName *ename = &mstruct.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC:  {
			str = &mstruct.symbol();
			if(abbreviated) *abbreviated = false;
			break;
		}
		case STRUCT_UNIT:  {
			const ExpressionName *ename = &mstruct.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		default: {if(abbreviated) *abbreviated = false; return 0;}
	}
	if(text_length_is_one(*str)) return 1;
	return str->length();
}

bool MathStructure::needsParenthesis(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool flat_division, bool) const {
	switch(parent.type()) {
		case STRUCT_MULTIPLICATION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division && (index < parent.size() || po.excessive_parenthesis);}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return o_function == CALCULATOR->f_uncertainty;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite() || (o_number.hasImaginaryPart() && o_number.hasRealPart());}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return po.excessive_parenthesis;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_POWER: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_OR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_XOR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_NOT: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_OR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_XOR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_NOT: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_FUNCTION: {return o_function == CALCULATOR->f_uncertainty;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return (flat_division || po.excessive_parenthesis) && (o_number.isInfinite() || o_number.hasImaginaryPart());}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_ADDITION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return index > 1 || po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_POWER: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return index == 1 || flat_division || po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return index == 1 || flat_division || po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return true;}
				case STRUCT_NEGATE: {return index == 1 || CHILD(0).needsParenthesis(po, ips, parent, index, flat_division);}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return index == 1 || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return index == 1 || po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return o_function == CALCULATOR->f_uncertainty;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite() || o_number.hasImaginaryPart();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_NEGATE: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return true;}
				case STRUCT_NEGATE: {return true;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return o_number.isInfinite() || (o_number.hasImaginaryPart() && o_number.hasRealPart());}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_XOR: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return false;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis && o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_XOR: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis && o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_COMPARISON: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return po.excessive_parenthesis;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis && o_number.isInfinite();}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_ABORTED: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				case STRUCT_DATETIME: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_BITWISE_NOT: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return true;}
				case STRUCT_INVERSE: {return true;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return true;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return true;}
				case STRUCT_COMPARISON: {return true;}
				case STRUCT_FUNCTION: {return po.excessive_parenthesis;}
				case STRUCT_VECTOR: {return po.excessive_parenthesis;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis;}
				case STRUCT_VARIABLE: {return po.excessive_parenthesis;}
				case STRUCT_ABORTED: {return po.excessive_parenthesis;}
				case STRUCT_SYMBOLIC: {return po.excessive_parenthesis;}
				case STRUCT_UNIT: {return po.excessive_parenthesis;}
				case STRUCT_UNDEFINED: {return po.excessive_parenthesis;}
				default: {return true;}
			}
		}
		case STRUCT_FUNCTION: {
			return false;
		}
		case STRUCT_VECTOR: {
			return false;
		}
		default: {
			return true;
		}
	}
}

int MathStructure::neededMultiplicationSign(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool par, bool par_prev, bool flat_division, bool flat_power) const {
	if(!po.short_multiplication || po.base > 10 || po.base < 2) return MULTIPLICATION_SIGN_OPERATOR;
	if(index <= 1) return MULTIPLICATION_SIGN_NONE;
	if(par_prev && par) return MULTIPLICATION_SIGN_NONE;
	if(par_prev) {
		if(isUnit_exp()) return MULTIPLICATION_SIGN_SPACE;
		if(isUnknown_exp()) {
			if(isSymbolic() || (isPower() && CHILD(0).isSymbolic())) return MULTIPLICATION_SIGN_SPACE;
			return (namelen(isPower() ? CHILD(0) : *this, po, ips, NULL) > 1 ? MULTIPLICATION_SIGN_SPACE : MULTIPLICATION_SIGN_NONE);
		}
		if(isMultiplication() && SIZE > 0) {
			if(CHILD(0).isUnit_exp()) return MULTIPLICATION_SIGN_SPACE;
			if(CHILD(0).isUnknown_exp()) {
				if(CHILD(0).isSymbolic() || (CHILD(0).isPower() && CHILD(0)[0].isSymbolic())) return MULTIPLICATION_SIGN_SPACE;
				return (namelen(CHILD(0).isPower() ? CHILD(0)[0] : CHILD(0), po, ips, NULL) > 1 ? MULTIPLICATION_SIGN_SPACE : MULTIPLICATION_SIGN_NONE);
			}
		} else if(isDivision()) {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).isUnit_exp()) {
					return MULTIPLICATION_SIGN_OPERATOR;
				}
			}
			return MULTIPLICATION_SIGN_SPACE;
		}
		return MULTIPLICATION_SIGN_OPERATOR;
	}
	int t = parent[index - 2].type();
	if(flat_power && t == STRUCT_POWER) return MULTIPLICATION_SIGN_OPERATOR;
	if(par && t == STRUCT_POWER) return MULTIPLICATION_SIGN_SPACE;
	if(par) return MULTIPLICATION_SIGN_NONE;
	bool abbr_prev = false, abbr_this = false;
	int namelen_this = namelen(*this, po, ips, &abbr_this);
	int namelen_prev = namelen(parent[index - 2], po, ips, &abbr_prev);
	switch(t) {
		case STRUCT_MULTIPLICATION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {if(flat_division) return MULTIPLICATION_SIGN_OPERATOR; return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_ADDITION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_POWER: {if(flat_power) return MULTIPLICATION_SIGN_OPERATOR; break;}
		case STRUCT_NEGATE: {break;}
		case STRUCT_BITWISE_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_COMPARISON: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_FUNCTION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VECTOR: {break;}
		case STRUCT_NUMBER: {break;}
		case STRUCT_VARIABLE: {break;}
		case STRUCT_ABORTED: {break;}
		case STRUCT_SYMBOLIC: {break;}
		case STRUCT_UNIT: {
			if(m_type == STRUCT_UNIT) {
				if(!po.limit_implicit_multiplication && !abbr_prev && !abbr_this) {
					return MULTIPLICATION_SIGN_SPACE;
				}
				if(po.place_units_separately) {
					return MULTIPLICATION_SIGN_OPERATOR_SHORT;
				} else {
					return MULTIPLICATION_SIGN_OPERATOR;
				}
			} else if(m_type == STRUCT_NUMBER) {
				if(namelen_prev > 1) {
					return MULTIPLICATION_SIGN_SPACE;
				} else {
					return MULTIPLICATION_SIGN_NONE;
				}
			}
			//return MULTIPLICATION_SIGN_SPACE;
		}
		case STRUCT_UNDEFINED: {break;}
		default: {return MULTIPLICATION_SIGN_OPERATOR;}
	}
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_ADDITION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_POWER: {return CHILD(0).neededMultiplicationSign(po, ips, parent, index, par, par_prev, flat_division, flat_power);}
		case STRUCT_NEGATE: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_COMPARISON: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_FUNCTION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VECTOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_NUMBER: {
			if(t == STRUCT_VARIABLE && parent[index - 2].variable() == CALCULATOR->v_i) return MULTIPLICATION_SIGN_NONE;
			return MULTIPLICATION_SIGN_OPERATOR;
		}
		case STRUCT_VARIABLE: {
			if(!o_variable->isRegistered() && o_variable->getName(1).name.length() > 0 && o_variable->getName(1).name[0] >= '0' && o_variable->getName(1).name[0] <= '9') return MULTIPLICATION_SIGN_OPERATOR;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			if(po.limit_implicit_multiplication && t != STRUCT_NUMBER) return MULTIPLICATION_SIGN_OPERATOR;
			if(t != STRUCT_NUMBER && ((namelen_prev > 1 || namelen_this > 1) || equals(parent[index - 2]))) return MULTIPLICATION_SIGN_OPERATOR;
			if(namelen_this > 1 || (m_type == STRUCT_SYMBOLIC && !po.allow_non_usable)) return MULTIPLICATION_SIGN_SPACE;
			return MULTIPLICATION_SIGN_NONE;
		}
		case STRUCT_UNIT: {
			if((t == STRUCT_POWER && parent[index - 2][0].isUnit_exp()) || (o_unit == CALCULATOR->getDegUnit() && print(po) == SIGN_DEGREE)) {
				return MULTIPLICATION_SIGN_NONE;
			}
			return MULTIPLICATION_SIGN_SPACE;
		}
		case STRUCT_UNDEFINED: {return MULTIPLICATION_SIGN_OPERATOR;}
		default: {return MULTIPLICATION_SIGN_OPERATOR;}
	}
}

ostream& operator << (ostream &os, const MathStructure &mstruct) {
	os << format_and_print(mstruct);
	return os;
}
string MathStructure::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	if(ips.depth == 0 && po.is_approximate) *po.is_approximate = false;
	string print_str;
	InternalPrintStruct ips_n = ips;
	if(isApproximate()) ips_n.parent_approximate = true;
	if(precision() >= 0 && (ips_n.parent_precision < 0 || precision() < ips_n.parent_precision)) ips_n.parent_precision = precision();
	switch(m_type) {
		case STRUCT_NUMBER: {
			print_str = o_number.print(po, ips_n);
			break;
		}
		case STRUCT_ABORTED: {}
		case STRUCT_SYMBOLIC: {
			if(po.allow_non_usable) {
				print_str = s_sym;
			} else {
				if((text_length_is_one(s_sym) && s_sym.find("\'") == string::npos) || s_sym.find("\"") != string::npos) {
					print_str = "\'";
					print_str += s_sym;
					print_str += "\'";
				} else {
					print_str = "\"";
					print_str += s_sym;
					print_str += "\"";
				}
			}
			break;
		}
		case STRUCT_DATETIME: {
			print_str = "\"";
			print_str += o_datetime->print(po);
			print_str += "\"";
			break;
		}
		case STRUCT_ADDITION: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(CHILD(i).type() == STRUCT_NEGATE) {
						if(po.spacious) print_str += " ";
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) print_str += SIGN_MINUS;
						else print_str += "-";
						if(po.spacious) print_str += " ";
						ips_n.wrap = CHILD(i)[0].needsParenthesis(po, ips_n, *this, i + 1, true, true);
						print_str += CHILD(i)[0].print(po, ips_n);
					} else {
						if(po.spacious) print_str += " ";
						print_str += "+";
						if(po.spacious) print_str += " ";
						ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
						print_str += CHILD(i).print(po, ips_n);
					}
				} else {
					ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
					print_str += CHILD(i).print(po, ips_n);
				}
			}
			break;
		}
		case STRUCT_NEGATE: {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) print_str += SIGN_MINUS;
			else print_str = "-";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			ips_n.depth++;
			if(!po.preserve_format && SIZE == 2 && (CHILD(0).isNumber() || (CHILD(0).isNegate() && CHILD(0)[0].isNumber())) && CHILD(1).isFunction() && CHILD(1).size() == 1 && CHILD(1).function()->referenceName() == "cis" && (((CHILD(1)[0].isNumber() || (CHILD(1)[0].isNegate() && CHILD(1)[0][0].isNumber())) || (CHILD(1)[0].isMultiplication() && CHILD(1)[0].size() == 2 && (CHILD(1)[0][0].isNumber() || (CHILD(1)[0].isNegate() && CHILD(1)[0][0][0].isNumber())) && CHILD(1)[0][1].isUnit())) || (CHILD(1)[0].isNegate() && CHILD(1)[0][0].isMultiplication() && CHILD(1)[0][0].size() == 2 && CHILD(1)[0][0][0].isNumber() && CHILD(1)[0][0][1].isUnit()))) {
				ips_n.wrap = false;
				print_str += CHILD(0).print(po, ips_n);
				print_str += " ";
				print_str += "cis";
				print_str += " ";
				ips_n.wrap = ((CHILD(1)[0].isMultiplication() && CHILD(1)[0][1].neededMultiplicationSign(po, ips_n, CHILD(1)[0], 2, false, false, false, false) != MULTIPLICATION_SIGN_NONE) || (CHILD(1)[0].isNegate() && CHILD(1)[0][0].isMultiplication() && CHILD(1)[0][0][1].neededMultiplicationSign(po, ips_n, CHILD(1)[0][0], 2, false, false, false, false) != MULTIPLICATION_SIGN_NONE));
				print_str += CHILD(1)[0].print(po, ips_n);
				break;
			}
			bool par_prev = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				if(!po.short_multiplication && i > 0) {
					if(po.spacious) print_str += " ";
					if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIDOT;
					else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MIDDLEDOT;
					else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIPLICATION;
					else print_str += "*";
					if(po.spacious) print_str += " ";
				} else if(i > 0) {
					switch(CHILD(i).neededMultiplicationSign(po, ips_n, *this, i + 1, ips_n.wrap || (CHILD(i).isPower() && CHILD(i)[0].needsParenthesis(po, ips_n, CHILD(i), 1, true, true)), par_prev, true, true)) {
						case MULTIPLICATION_SIGN_SPACE: {print_str += " "; break;}
						case MULTIPLICATION_SIGN_OPERATOR: {
							if(po.spacious) {
								if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += " " SIGN_MULTIDOT " ";
								else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) print_str += " " SIGN_MIDDLEDOT " ";
								else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += " " SIGN_MULTIPLICATION " ";
								else print_str += " * ";
								break;
							}
						}
						case MULTIPLICATION_SIGN_OPERATOR_SHORT: {
							if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIDOT;
							else if(po.use_unicode_signs && (po.multiplication_sign == MULTIPLICATION_SIGN_DOT || po.multiplication_sign == MULTIPLICATION_SIGN_ALTDOT) && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MIDDLEDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MIDDLEDOT;
							else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIPLICATION;
							else print_str += "*";
							break;
						}
					}
				}
				print_str += CHILD(i).print(po, ips_n);
				par_prev = ips_n.wrap;
			}
			break;
		}
		case STRUCT_INVERSE: {
			ips_n.depth++;
			ips_n.division_depth++;
			ips_n.wrap = false;
			print_str = m_one.print(po, ips_n);
			if(po.spacious) print_str += " ";
			if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION;
			} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION_SLASH;
			} else {
				print_str += "/";
			}
			if(po.spacious) print_str += " ";
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_DIVISION: {
			ips_n.depth++;
			ips_n.division_depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, ips_n);
			if(po.spacious) print_str += " ";
			if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION;
			} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION_SLASH;
			} else {
				print_str += "/";
			}
			if(po.spacious) print_str += " ";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			print_str += CHILD(1).print(po, ips_n);
			break;
		}
		case STRUCT_POWER: {
			ips_n.depth++;
			ips_n.power_depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, ips_n);
			print_str += "^";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			PrintOptions po2 = po;
			po2.show_ending_zeroes = false;
			print_str += CHILD(1).print(po2, ips_n);
			break;
		}
		case STRUCT_COMPARISON: {
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, ips_n);
			if(po.spacious) print_str += " ";
			switch(ct_comp) {
				case COMPARISON_EQUALS: {
					if(po.use_unicode_signs && po.interval_display != INTERVAL_DISPLAY_INTERVAL && isApproximate() && containsInterval() && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_ALMOST_EQUAL;
					else print_str += "=";
					break;
				}
				case COMPARISON_NOT_EQUALS: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_NOT_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_NOT_EQUAL;
					else print_str += "!=";
					break;
				}
				case COMPARISON_GREATER: {print_str += ">"; break;}
				case COMPARISON_LESS: {print_str += "<"; break;}
				case COMPARISON_EQUALS_GREATER: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
					else print_str += ">=";
					break;
				}
				case COMPARISON_EQUALS_LESS: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
					else print_str += "<=";
					break;
				}
			}
			if(po.spacious) print_str += " ";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			print_str += CHILD(1).print(po, ips_n);
			break;
		}
		case STRUCT_BITWISE_AND: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "&";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_OR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "|";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_XOR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += " ";
					print_str += "xor";
					print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_NOT: {
			print_str = "~";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_LOGICAL_AND: {
			ips_n.depth++;
			if(!po.preserve_format && SIZE == 2 && CHILD(0).isComparison() && CHILD(1).isComparison() && CHILD(0).comparisonType() != COMPARISON_EQUALS && CHILD(0).comparisonType() != COMPARISON_NOT_EQUALS && CHILD(1).comparisonType() != COMPARISON_EQUALS && CHILD(1).comparisonType() != COMPARISON_NOT_EQUALS && CHILD(0)[0] == CHILD(1)[0]) {
				ips_n.wrap = CHILD(0)[1].needsParenthesis(po, ips_n, CHILD(0), 2, true, true);
				print_str += CHILD(0)[1].print(po, ips_n);
				if(po.spacious) print_str += " ";
				switch(CHILD(0).comparisonType()) {
					case COMPARISON_LESS: {print_str += ">"; break;}
					case COMPARISON_GREATER: {print_str += "<"; break;}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
						else print_str += ">=";
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
						else print_str += "<=";
						break;
					}
					default: {}
				}
				if(po.spacious) print_str += " ";

				ips_n.wrap = CHILD(0)[0].needsParenthesis(po, ips_n, CHILD(0), 1, true, true);
				print_str += CHILD(0)[0].print(po, ips_n);

				if(po.spacious) print_str += " ";
				switch(CHILD(1).comparisonType()) {
					case COMPARISON_GREATER: {print_str += ">"; break;}
					case COMPARISON_LESS: {print_str += "<"; break;}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
						else print_str += ">=";
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
						else print_str += "<=";
						break;
					}
					default: {}
				}
				if(po.spacious) print_str += " ";

				ips_n.wrap = CHILD(1)[1].needsParenthesis(po, ips_n, CHILD(1), 2, true, true);
				print_str += CHILD(1)[1].print(po, ips_n);

				break;
			}
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spell_out_logical_operators) {
						print_str += " ";
						print_str += _("and");
						print_str += " ";
					} else {
						if(po.spacious) print_str += " ";
						print_str += "&&";
						if(po.spacious) print_str += " ";
					}
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_OR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					if(po.spell_out_logical_operators) {
						print_str += " ";
						print_str += _("or");
						print_str += " ";
					} else {
						if(po.spacious) print_str += " ";
						print_str += "||";
						if(po.spacious) print_str += " ";
					}
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_XOR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += " ";
					print_str += "xor";
					print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			print_str = "!";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_VECTOR: {
			ips_n.depth++;
			print_str = "[";
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += po.comma();
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			print_str += "]";
			break;
		}
		case STRUCT_UNIT: {
			const ExpressionName *ename = &o_unit->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, b_plural, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			if(o_prefix) print_str += o_prefix->name(po.abbreviate_names && ename->abbreviation, po.use_unicode_signs, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(ename->suffix && !po.preserve_format && !po.use_reference_names) {
				size_t i = print_str.rfind('_');
				if(i != string::npos && i + 5 <= print_str.length() && print_str.substr(print_str.length() - 4, 4) == "unit") {
					if(i + 5 == print_str.length()) {
						print_str = print_str.substr(0, i);
						if(po.hide_underscore_spaces) gsub("_", " ", print_str);
					} else {
						print_str = print_str.substr(0, print_str.length() - 4);
					}
				}
			}
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			break;
		}
		case STRUCT_VARIABLE: {
			const ExpressionName *ename = &o_variable->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(ename->suffix && !po.preserve_format && !po.use_reference_names) {
				size_t i = print_str.rfind('_');
				if(i != string::npos && i + 9 <= print_str.length() && print_str.substr(print_str.length() - 8, 8) == "constant") {
					if(i + 9 == print_str.length()) {
						print_str = print_str.substr(0, i);
						if(po.hide_underscore_spaces) gsub("_", " ", print_str);
					} else {
						print_str = print_str.substr(0, print_str.length() - 8);
					}
				}
			}
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			ips_n.depth++;
			if(o_function == CALCULATOR->f_uncertainty && SIZE == 3 && CHILD(2).isZero()) {
				MathStructure *mmid = NULL, *munc = NULL;
				if(o_function == CALCULATOR->f_uncertainty) {
					mmid = &CHILD(0);
					munc = &CHILD(1);
				} else if(CHILD(0)[0].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[1];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				} else if(CHILD(0)[0].equals(CHILD(1)[1], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[1].isNegate() && CHILD(0)[1][0].equals(CHILD(1)[0], true, true)) munc = &CHILD(1)[0];
					if(CHILD(1)[0].isNegate() && CHILD(1)[0][0].equals(CHILD(0)[1], true, true)) munc = &CHILD(0)[1];
				} else if(CHILD(0)[1].equals(CHILD(1)[0], true, true)) {
					mmid = &CHILD(0)[0];
					if(CHILD(0)[0].isNegate() && CHILD(0)[0][0].equals(CHILD(1)[1], true, true)) munc = &CHILD(1)[1];
					if(CHILD(1)[1].isNegate() && CHILD(1)[1][0].equals(CHILD(0)[0], true, true)) munc = &CHILD(0)[0];
				}
				if(mmid && munc) {
					PrintOptions po2 = po;
					po2.show_ending_zeroes = false;
					po2.number_fraction_format = FRACTION_DECIMAL;
					ips_n.wrap = !CHILD(0).isNumber();
					print_str += CHILD(0).print(po2, ips_n);
					print_str += SIGN_PLUSMINUS;
					ips_n.wrap = !CHILD(1).isNumber();
					print_str += CHILD(1).print(po2, ips_n);
					break;
				}
			}
			const ExpressionName *ename = &o_function->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			print_str += "(";
			size_t argcount = SIZE;
			if(o_function == CALCULATOR->f_signum && argcount > 1) {
				argcount = 1;
			} else if(o_function->maxargs() > 0 && o_function->minargs() < o_function->maxargs() && SIZE > (size_t) o_function->minargs()) {
				while(true) {
					string defstr = o_function->getDefaultValue(argcount);
					Argument *arg = o_function->getArgumentDefinition(argcount);
					remove_blank_ends(defstr);
					if(defstr.empty()) break;
					if(CHILD(argcount - 1).isUndefined() && defstr == "undefined") {
						argcount--;
					} else if(argcount > 1 && arg && arg->type() == ARGUMENT_TYPE_SYMBOLIC && defstr == "undefined" && CHILD(argcount - 1) == CHILD(0).find_x_var()) {
						argcount--;
					} else if(CHILD(argcount - 1).isVariable() && (!arg || arg->type() != ARGUMENT_TYPE_TEXT) && defstr == CHILD(argcount - 1).variable()->referenceName()) {
						argcount--;
					} else if(CHILD(argcount - 1).isInteger() && (!arg || arg->type() != ARGUMENT_TYPE_TEXT) && defstr.find_first_not_of(NUMBERS) == string::npos && CHILD(argcount - 1).number() == s2i(defstr)) {
						argcount--;
					} else if(CHILD(argcount - 1).isSymbolic() && arg && arg->type() == ARGUMENT_TYPE_TEXT && CHILD(argcount - 1).symbol() == defstr) {
						argcount--;
					} else {
						break;
					}
					if(argcount == 0 || argcount == (size_t) o_function->minargs()) break;
				}
			}
			for(size_t i = 0; i < argcount; i++) {
				if(CALCULATOR->aborted()) return CALCULATOR->abortedMessage();
				if(i > 0) {
					print_str += po.comma();
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				if(o_function == CALCULATOR->f_interval) {
					PrintOptions po2 = po;
					po2.show_ending_zeroes = false;
					print_str += CHILD(i).print(po2, ips_n);
				} else {
					print_str += CHILD(i).print(po, ips_n);
				}
			}
			print_str += ")";
			break;
		}
		case STRUCT_UNDEFINED: {
			print_str = _("undefined");
			break;
		}
	}
	if(CALCULATOR->aborted()) print_str = CALCULATOR->abortedMessage();
	if(ips.wrap) {
		print_str.insert(0, "(");
		print_str += ")";
	}
	return print_str;
}

MathStructure &MathStructure::flattenVector(MathStructure &mstruct) const {
	if(!isVector()) {
		mstruct = *this;
		return mstruct;
	}
	MathStructure mstruct2;
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			CHILD(i).flattenVector(mstruct2);
			for(size_t i2 = 0; i2 < mstruct2.size(); i2++) {
				mstruct.addChild(mstruct2[i2]);
			}
		} else {
			mstruct.addChild(CHILD(i));
		}
	}
	return mstruct;
}
bool MathStructure::rankVector(bool ascending) {
	vector<int> ranked;
	vector<bool> ranked_equals_prev;
	bool b;
	for(size_t index = 0; index < SIZE; index++) {
		b = false;
		for(size_t i = 0; i < ranked.size(); i++) {
			if(CALCULATOR->aborted()) return false;
			ComparisonResult cmp = CHILD(index).compare(CHILD(ranked[i]));
			if(COMPARISON_NOT_FULLY_KNOWN(cmp)) {
				CALCULATOR->error(true, _("Unsolvable comparison at element %s when trying to rank vector."), i2s(index).c_str(), NULL);
				return false;
			}
			if((ascending && cmp == COMPARISON_RESULT_GREATER) || cmp == COMPARISON_RESULT_EQUAL || (!ascending && cmp == COMPARISON_RESULT_LESS)) {
				if(cmp == COMPARISON_RESULT_EQUAL) {
					ranked.insert(ranked.begin() + i + 1, index);
					ranked_equals_prev.insert(ranked_equals_prev.begin() + i + 1, true);
				} else {
					ranked.insert(ranked.begin() + i, index);
					ranked_equals_prev.insert(ranked_equals_prev.begin() + i, false);
				}
				b = true;
				break;
			}
		}
		if(!b) {
			ranked.push_back(index);
			ranked_equals_prev.push_back(false);
		}
	}
	int n_rep = 0;
	for(long int i = (long int) ranked.size() - 1; i >= 0; i--) {
		if(CALCULATOR->aborted()) return false;
		if(ranked_equals_prev[i]) {
			n_rep++;
		} else {
			if(n_rep) {
				MathStructure v(i + 1 + n_rep, 1L, 0L);
				v += i + 1;
				v *= MathStructure(1, 2, 0);
				for(; n_rep >= 0; n_rep--) {
					CHILD(ranked[i + n_rep]) = v;
				}
			} else {
				CHILD(ranked[i]).set(i + 1, 1L, 0L);
			}
			n_rep = 0;
		}
	}
	return true;
}
bool MathStructure::sortVector(bool ascending) {
	vector<size_t> ranked_mstructs;
	bool b;
	for(size_t index = 0; index < SIZE; index++) {
		b = false;
		for(size_t i = 0; i < ranked_mstructs.size(); i++) {
			if(CALCULATOR->aborted()) return false;
			ComparisonResult cmp = CHILD(index).compare(*v_subs[ranked_mstructs[i]]);
			if(COMPARISON_MIGHT_BE_LESS_OR_GREATER(cmp)) {
				CALCULATOR->error(true, _("Unsolvable comparison at element %s when trying to sort vector."), i2s(index).c_str(), NULL);
				return false;
			}
			if((ascending && COMPARISON_IS_EQUAL_OR_GREATER(cmp)) || (!ascending && COMPARISON_IS_EQUAL_OR_LESS(cmp))) {
				ranked_mstructs.insert(ranked_mstructs.begin() + i, v_order[index]);
				b = true;
				break;
			}
		}
		if(!b) {
			ranked_mstructs.push_back(v_order[index]);
		}
	}
	v_order = ranked_mstructs;
	return true;
}
MathStructure &MathStructure::getRange(int start, int end, MathStructure &mstruct) const {
	if(!isVector()) {
		if(start > 1) {
			mstruct.clear();
			return mstruct;
		} else {
			mstruct = *this;
			return mstruct;
		}
	}
	if(start < 1) start = 1;
	else if(start > (long int) SIZE) {
		mstruct.clear();
		return mstruct;
	}
	if(end < (int) 1 || end > (long int) SIZE) end = SIZE;
	else if(end < start) end = start;
	mstruct.clearVector();
	for(; start <= end; start++) {
		mstruct.addChild(CHILD(start - 1));
	}
	return mstruct;
}

void MathStructure::resizeVector(size_t i, const MathStructure &mfill) {
	if(i > SIZE) {
		while(i > SIZE) {
			APPEND(mfill);
		}
	} else if(i < SIZE) {
		REDUCE(i)
	}
}

size_t MathStructure::rows() const {
	if(m_type != STRUCT_VECTOR || SIZE == 0 || (SIZE == 1 && (!CHILD(0).isVector() || CHILD(0).size() == 0))) return 0;
	return SIZE;
}
size_t MathStructure::columns() const {
	if(m_type != STRUCT_VECTOR || SIZE == 0 || !CHILD(0).isVector()) return 0;
	return CHILD(0).size();
}
const MathStructure *MathStructure::getElement(size_t row, size_t column) const {
	if(row == 0 || column == 0 || row > rows() || column > columns()) return NULL;
	if(CHILD(row - 1).size() < column) return NULL;
	return &CHILD(row - 1)[column - 1];
}
MathStructure *MathStructure::getElement(size_t row, size_t column) {
	if(row == 0 || column == 0 || row > rows() || column > columns()) return NULL;
	if(CHILD(row - 1).size() < column) return NULL;
	return &CHILD(row - 1)[column - 1];
}
MathStructure &MathStructure::getArea(size_t r1, size_t c1, size_t r2, size_t c2, MathStructure &mstruct) const {
	size_t r = rows();
	size_t c = columns();
	if(r1 < 1) r1 = 1;
	else if(r1 > r) r1 = r;
	if(c1 < 1) c1 = 1;
	else if(c1 > c) c1 = c;
	if(r2 < 1 || r2 > r) r2 = r;
	else if(r2 < r1) r2 = r1;
	if(c2 < 1 || c2 > c) c2 = c;
	else if(c2 < c1) c2 = c1;
	mstruct.clearMatrix(); mstruct.resizeMatrix(r2 - r1 + 1, c2 - c1 + 1, m_undefined);
	for(size_t index_r = r1; index_r <= r2; index_r++) {
		for(size_t index_c = c1; index_c <= c2; index_c++) {
			mstruct[index_r - r1][index_c - c1] = CHILD(index_r - 1)[index_c - 1];
		}
	}
	return mstruct;
}
MathStructure &MathStructure::rowToVector(size_t r, MathStructure &mstruct) const {
	if(r > rows()) {
		mstruct = m_undefined;
		return mstruct;
	}
	if(r < 1) r = 1;
	mstruct = CHILD(r - 1);
	return mstruct;
}
MathStructure &MathStructure::columnToVector(size_t c, MathStructure &mstruct) const {
	if(c > columns()) {
		mstruct = m_undefined;
		return mstruct;
	}
	if(c < 1) c = 1;
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		mstruct.addChild(CHILD(i)[c - 1]);
	}
	return mstruct;
}
MathStructure &MathStructure::matrixToVector(MathStructure &mstruct) const {
	if(!isVector()) {
		mstruct = *this;
		return mstruct;
	}
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
				mstruct.addChild(CHILD(i)[i2]);
			}
		} else {
			mstruct.addChild(CHILD(i));
		}
	}
	return mstruct;
}
void MathStructure::setElement(const MathStructure &mstruct, size_t row, size_t column) {
	if(row > rows() || column > columns() || row < 1 || column < 1) return;
	CHILD(row - 1)[column - 1] = mstruct;
	CHILD(row - 1).childUpdated(column);
	CHILD_UPDATED(row - 1);
}
void MathStructure::addRows(size_t r, const MathStructure &mfill) {
	if(r == 0) return;
	size_t cols = columns();
	MathStructure mstruct; mstruct.clearVector();
	mstruct.resizeVector(cols, mfill);
	for(size_t i = 0; i < r; i++) {
		APPEND(mstruct);
	}
}
void MathStructure::addColumns(size_t c, const MathStructure &mfill) {
	if(c == 0) return;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			for(size_t i2 = 0; i2 < c; i2++) {
				CHILD(i).addChild(mfill);
			}
		}
	}
	CHILDREN_UPDATED;
}
void MathStructure::addRow(const MathStructure &mfill) {
	addRows(1, mfill);
}
void MathStructure::addColumn(const MathStructure &mfill) {
	addColumns(1, mfill);
}
void MathStructure::resizeMatrix(size_t r, size_t c, const MathStructure &mfill) {
	if(r > SIZE) {
		addRows(r - SIZE, mfill);
	} else if(r != SIZE) {
		REDUCE(r);
	}
	size_t cols = columns();
	if(c > cols) {
		addColumns(c - cols, mfill);
	} else if(c != cols) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).resizeVector(c, mfill);
		}
	}
}
bool MathStructure::matrixIsSquare() const {
	return rows() == columns();
}

bool MathStructure::isNumericMatrix() const {
	if(!isMatrix()) return false;
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(index_r).size(); index_c++) {
			if(!CHILD(index_r)[index_c].isNumber() || CHILD(index_r)[index_c].isInfinity()) return false;
		}
	}
	return true;
}

//from GiNaC
int MathStructure::pivot(size_t ro, size_t co, bool symbolic) {

	size_t k = ro;
	if(symbolic) {
		while((k < SIZE) && (CHILD(k)[co].isZero())) ++k;
	} else {
		size_t kmax = k + 1;
		Number mmax(CHILD(kmax)[co].number());
		mmax.abs();
		while(kmax < SIZE) {
			if(CHILD(kmax)[co].number().isNegative()) {
				Number ntmp(CHILD(kmax)[co].number());
				ntmp.negate();
				if(ntmp.isGreaterThan(mmax)) {
					mmax = ntmp;
					k = kmax;
				}
			} else if(CHILD(kmax)[co].number().isGreaterThan(mmax)) {
				mmax = CHILD(kmax)[co].number();
				k = kmax;
			}
			++kmax;
		}
		if(!mmax.isZero()) k = kmax;
	}
	if(k == SIZE) return -1;
	if(k == ro) return 0;

	SWAP_CHILDREN(ro, k)

	return k;

}


//from GiNaC
void determinant_minor(const MathStructure &mtrx, MathStructure &mdet, const EvaluationOptions &eo) {
	size_t n = mtrx.size();
	if(n == 1) {
		mdet = mtrx[0][0];
		return;
	}
	if(n == 2) {
		mdet = mtrx[0][0];
		mdet.calculateMultiply(mtrx[1][1], eo);
		mdet.add(mtrx[1][0], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[0][1], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		return;
	}
	if(n == 3) {
		mdet = mtrx[0][0];
		mdet.calculateMultiply(mtrx[1][1], eo);
		mdet.calculateMultiply(mtrx[2][2], eo);
		mdet.add(mtrx[0][0], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][2], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][1], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][1], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][0], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][2], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][2], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][0], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][1], eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][1], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][2], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][0], eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][2], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][1], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][0], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		return;
	}

	std::vector<size_t> Pkey;
	Pkey.reserve(n);
	std::vector<size_t> Mkey;
	Mkey.reserve(n - 1);
	typedef std::map<std::vector<size_t>, class MathStructure> Rmap;
	typedef std::map<std::vector<size_t>, class MathStructure>::value_type Rmap_value;
	Rmap A;
	Rmap B;
	for(size_t r = 0; r < n; ++r) {
		Pkey.erase(Pkey.begin(), Pkey.end());
		Pkey.push_back(r);
		A.insert(Rmap_value(Pkey, mtrx[r][n - 1]));
	}
	for(long int c = n - 2; c >= 0; --c) {
		Pkey.erase(Pkey.begin(), Pkey.end());
		Mkey.erase(Mkey.begin(), Mkey.end());
		for(size_t i = 0; i < n - c; ++i) Pkey.push_back(i);
		size_t fc = 0;
		do {
			mdet.clear();
			for(size_t r = 0; r < n - c; ++r) {
				if (mtrx[Pkey[r]][c].isZero()) continue;
				Mkey.erase(Mkey.begin(), Mkey.end());
				for(size_t i = 0; i < n - c; ++i) {
					if(i != r) Mkey.push_back(Pkey[i]);
				}
				mdet.add(mtrx[Pkey[r]][c], true);
				mdet[mdet.size() - 1].calculateMultiply(A[Mkey], eo);
				if(r % 2) mdet[mdet.size() - 1].calculateNegate(eo);
				mdet.calculateAddLast(eo);
			}
			if(!mdet.isZero()) B.insert(Rmap_value(Pkey, mdet));
			for(fc = n - c; fc > 0; --fc) {
				++Pkey[fc-1];
				if(Pkey[fc - 1] < fc + c) break;
			}
			if(fc < n - c && fc > 0) {
				for(size_t j = fc; j < n - c; ++j) Pkey[j] = Pkey[j - 1] + 1;
			}
		} while(fc);
		A = B;
		B.clear();
	}
	return;
}

//from GiNaC
int MathStructure::gaussianElimination(const EvaluationOptions &eo, bool det) {

	if(!isMatrix()) return 0;
	bool isnumeric = isNumericMatrix();

	size_t m = rows();
	size_t n = columns();
	int sign = 1;

	size_t r0 = 0;
	for(size_t c0 = 0; c0 < n && r0 < m - 1; ++c0) {
		int indx = pivot(r0, c0, true);
		if(indx == -1) {
			sign = 0;
			if(det) return 0;
		}
		if(indx >= 0) {
			if(indx > 0) sign = -sign;
			for(size_t r2 = r0 + 1; r2 < m; ++r2) {
				if(!CHILD(r2)[c0].isZero()) {
					if(isnumeric) {
						Number piv(CHILD(r2)[c0].number());
						piv /= CHILD(r0)[c0].number();
						for(size_t c = c0 + 1; c < n; ++c) {
							CHILD(r2)[c].number() -= piv * CHILD(r0)[c].number();
						}
					} else {
						MathStructure piv(CHILD(r2)[c0]);
						piv.calculateDivide(CHILD(r0)[c0], eo);
						for(size_t c = c0 + 1; c < n; ++c) {
							CHILD(r2)[c].add(piv, true);
							CHILD(r2)[c][CHILD(r2)[c].size() - 1].calculateMultiply(CHILD(r0)[c], eo);
							CHILD(r2)[c][CHILD(r2)[c].size() - 1].calculateNegate(eo);
							CHILD(r2)[c].calculateAddLast(eo);
						}
					}
				}
				for(size_t c = r0; c <= c0; ++c) CHILD(r2)[c].clear();
			}
			if(det) {
				for(size_t c = r0 + 1; c < n; ++c) CHILD(r0)[c].clear();
			}
			++r0;
		}
	}
	for(size_t r = r0 + 1; r < m; ++r) {
		for(size_t c = 0; c < n; ++c) CHILD(r)[c].clear();
	}

	return sign;
}

//from GiNaC
template <class It>
int permutation_sign(It first, It last)
{
	if (first == last)
		return 0;
	--last;
	if (first == last)
		return 0;
	It flag = first;
	int sign = 1;

	do {
		It i = last, other = last;
		--other;
		bool swapped = false;
		while (i != first) {
			if (*i < *other) {
				std::iter_swap(other, i);
				flag = other;
				swapped = true;
				sign = -sign;
			} else if (!(*other < *i))
				return 0;
			--i; --other;
		}
		if (!swapped)
			return sign;
		++flag;
		if (flag == last)
			return sign;
		first = flag;
		i = first; other = first;
		++other;
		swapped = false;
		while (i != last) {
			if (*other < *i) {
				std::iter_swap(i, other);
				flag = other;
				swapped = true;
				sign = -sign;
			} else if (!(*i < *other))
				return 0;
			++i; ++other;
		}
		if (!swapped)
			return sign;
		last = flag;
		--last;
	} while (first != last);

	return sign;
}

//from GiNaC
MathStructure &MathStructure::determinant(MathStructure &mstruct, const EvaluationOptions &eo) const {

	if(!matrixIsSquare()) {
		CALCULATOR->error(true, _("The determinant can only be calculated for square matrices."), NULL);
		mstruct = m_undefined;
		return mstruct;
	}

	if(SIZE == 1) {
		mstruct = CHILD(0)[0];
	} else if(isNumericMatrix()) {

		mstruct.set(1, 1, 0);
		MathStructure mtmp(*this);
		int sign = mtmp.gaussianElimination(eo, true);
		for(size_t d = 0; d < SIZE; ++d) {
			mstruct.number() *= mtmp[d][d].number();
		}
		mstruct.number() *= sign;

	} else {

		typedef std::pair<size_t, size_t> sizet_pair;
		std::vector<sizet_pair> c_zeros;
		for(size_t c = 0; c < CHILD(0).size(); ++c) {
			size_t acc = 0;
			for(size_t r = 0; r < SIZE; ++r) {
				if(CHILD(r)[c].isZero()) ++acc;
			}
			c_zeros.push_back(sizet_pair(acc, c));
		}

		std::sort(c_zeros.begin(), c_zeros.end());
		std::vector<size_t> pre_sort;
		for(std::vector<sizet_pair>::const_iterator i = c_zeros.begin(); i != c_zeros.end(); ++i) {
			pre_sort.push_back(i->second);
		}
		std::vector<size_t> pre_sort_test(pre_sort);

		int sign = permutation_sign(pre_sort_test.begin(), pre_sort_test.end());

		MathStructure result;
		result.clearMatrix();
		result.resizeMatrix(SIZE, CHILD(0).size(), m_zero);

		size_t c = 0;
		for(std::vector<size_t>::const_iterator i = pre_sort.begin(); i != pre_sort.end(); ++i,++c) {
			for(size_t r = 0; r < SIZE; ++r) result[r][c] = CHILD(r)[(*i)];
		}
		mstruct.clear();

		determinant_minor(result, mstruct, eo);

		if(sign != 1) {
			mstruct.calculateMultiply(sign, eo);
		}

	}

	mstruct.mergePrecision(*this);
	return mstruct;

}

MathStructure &MathStructure::permanent(MathStructure &mstruct, const EvaluationOptions &eo) const {
	if(!matrixIsSquare()) {
		CALCULATOR->error(true, _("The permanent can only be calculated for square matrices."), NULL);
		mstruct = m_undefined;
		return mstruct;
	}
	if(b_approx) mstruct.setApproximate();
	mstruct.setPrecision(i_precision);
	if(SIZE == 1) {
		if(CHILD(0).size() >= 1) {
			mstruct = CHILD(0)[0];
		}
	} else if(SIZE == 2) {
		mstruct = CHILD(0)[0];
		if(IS_REAL(mstruct) && IS_REAL(CHILD(1)[1])) {
			mstruct.number() *= CHILD(1)[1].number();
		} else {
			mstruct.calculateMultiply(CHILD(1)[1], eo);
		}
		if(IS_REAL(mstruct) && IS_REAL(CHILD(1)[0]) && IS_REAL(CHILD(0)[1])) {
			mstruct.number() += CHILD(1)[0].number() * CHILD(0)[1].number();
		} else {
			MathStructure mtmp = CHILD(1)[0];
			mtmp.calculateMultiply(CHILD(0)[1], eo);
			mstruct.calculateAdd(mtmp, eo);
		}
	} else {
		MathStructure mtrx;
		mtrx.clearMatrix();
		mtrx.resizeMatrix(SIZE - 1, CHILD(0).size() - 1, m_undefined);
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			for(size_t index_r2 = 1; index_r2 < SIZE; index_r2++) {
				for(size_t index_c2 = 0; index_c2 < CHILD(index_r2).size(); index_c2++) {
					if(index_c2 > index_c) {
						mtrx.setElement(CHILD(index_r2)[index_c2], index_r2, index_c2);
					} else if(index_c2 < index_c) {
						mtrx.setElement(CHILD(index_r2)[index_c2], index_r2, index_c2 + 1);
					}
				}
			}
			MathStructure mdet;
			mtrx.permanent(mdet, eo);
			if(IS_REAL(mdet) && IS_REAL(CHILD(0)[index_c])) {
				mdet.number() *= CHILD(0)[index_c].number();
			} else {
				mdet.calculateMultiply(CHILD(0)[index_c], eo);
			}
			if(IS_REAL(mdet) && IS_REAL(mstruct)) {
				mstruct.number() += mdet.number();
			} else {
				mstruct.calculateAdd(mdet, eo);
			}
		}
	}
	return mstruct;
}
void MathStructure::setToIdentityMatrix(size_t n) {
	clearMatrix();
	resizeMatrix(n, n, m_zero);
	for(size_t i = 0; i < n; i++) {
		CHILD(i)[i] = m_one;
	}
}
MathStructure &MathStructure::getIdentityMatrix(MathStructure &mstruct) const {
	mstruct.setToIdentityMatrix(columns());
	return mstruct;
}

//modified algorithm from eigenmath
bool MathStructure::invertMatrix(const EvaluationOptions &eo) {

	if(!matrixIsSquare()) return false;

	if(isNumericMatrix()) {

		int d, i, j, n = SIZE;

		MathStructure idstruct;
		Number mtmp;
		idstruct.setToIdentityMatrix(n);
		MathStructure mtrx(*this);

		for(d = 0; d < n; d++) {
			if(mtrx[d][d].isZero()) {
				for (i = d + 1; i < n; i++) {
					if(!mtrx[i][d].isZero()) break;
				}

				if(i == n) {
					CALCULATOR->error(true, _("Inverse of singular matrix."), NULL);
					return false;
				}

				mtrx[i].ref();
				mtrx[d].ref();
				MathStructure *mptr = &mtrx[d];
				mtrx.setChild_nocopy(&mtrx[i], d + 1);
				mtrx.setChild_nocopy(mptr, i + 1);

				idstruct[i].ref();
				idstruct[d].ref();
				mptr = &idstruct[d];
				idstruct.setChild_nocopy(&idstruct[i], d + 1);
				idstruct.setChild_nocopy(mptr, i + 1);

			}

			mtmp = mtrx[d][d].number();
			mtmp.recip();

			for(j = 0; j < n; j++) {

				if(j > d) {
					mtrx[d][j].number() *= mtmp;
				}

				idstruct[d][j].number() *= mtmp;

			}

			for(i = 0; i < n; i++) {

				if(i == d) continue;

				mtmp = mtrx[i][d].number();
				mtmp.negate();

				for(j = 0; j < n; j++) {

					if(j > d) {
						mtrx[i][j].number() += mtrx[d][j].number() * mtmp;
					}

					idstruct[i][j].number() += idstruct[d][j].number() * mtmp;

				}
			}
		}
		set_nocopy(idstruct);
		MERGE_APPROX_AND_PREC(idstruct)
	} else {
		MathStructure *mstruct = new MathStructure();
		determinant(*mstruct, eo);
		mstruct->calculateInverse(eo);
		adjointMatrix(eo);
		multiply_nocopy(mstruct, true);
		calculateMultiplyLast(eo);
	}
	return true;
}

bool MathStructure::adjointMatrix(const EvaluationOptions &eo) {
	if(!matrixIsSquare()) return false;
	if(SIZE == 1) {CHILD(0)[0].set(1, 1, 0); return true;}
	MathStructure msave(*this);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			msave.cofactor(index_r + 1, index_c + 1, CHILD(index_r)[index_c], eo);
		}
	}
	transposeMatrix();
	return true;
}
bool MathStructure::transposeMatrix() {
	MathStructure msave(*this);
	resizeMatrix(CHILD(0).size(), SIZE, m_undefined);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			CHILD(index_r)[index_c] = msave[index_c][index_r];
		}
	}
	return true;
}
MathStructure &MathStructure::cofactor(size_t r, size_t c, MathStructure &mstruct, const EvaluationOptions &eo) const {
	if(r < 1) r = 1;
	if(c < 1) c = 1;
	if(SIZE == 1 || r > SIZE || c > CHILD(0).size()) {
		mstruct = m_undefined;
		return mstruct;
	}
	r--; c--;
	mstruct.clearMatrix();
	mstruct.resizeMatrix(SIZE - 1, CHILD(0).size() - 1, m_undefined);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		if(index_r != r) {
			for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
				if(index_c > c) {
					if(index_r > r) {
						mstruct[index_r - 1][index_c - 1] = CHILD(index_r)[index_c];
					} else {
						mstruct[index_r][index_c - 1] = CHILD(index_r)[index_c];
					}
				} else if(index_c < c) {
					if(index_r > r) {
						mstruct[index_r - 1][index_c] = CHILD(index_r)[index_c];
					} else {
						mstruct[index_r][index_c] = CHILD(index_r)[index_c];
					}
				}
			}
		}
	}
	MathStructure mstruct2;
	mstruct = mstruct.determinant(mstruct2, eo);
	if((r + c) % 2 == 1) {
		mstruct.calculateNegate(eo);
	}
	return mstruct;
}

void gatherInformation(const MathStructure &mstruct, vector<Unit*> &base_units, vector<AliasUnit*> &alias_units, bool check_variables = false) {
	switch(mstruct.type()) {
		case STRUCT_UNIT: {
			switch(mstruct.unit()->subtype()) {
				case SUBTYPE_BASE_UNIT: {
					for(size_t i = 0; i < base_units.size(); i++) {
						if(base_units[i] == mstruct.unit()) {
							return;
						}
					}
					base_units.push_back(mstruct.unit());
					break;
				}
				case SUBTYPE_ALIAS_UNIT: {
					for(size_t i = 0; i < alias_units.size(); i++) {
						if(alias_units[i] == mstruct.unit()) {
							return;
						}
					}
					alias_units.push_back((AliasUnit*) (mstruct.unit()));
					break;
				}
				case SUBTYPE_COMPOSITE_UNIT: {
					gatherInformation(((CompositeUnit*) (mstruct.unit()))->generateMathStructure(), base_units, alias_units, check_variables);
					break;
				}
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(check_variables && mstruct.variable()->isKnown()) gatherInformation(((KnownVariable*) mstruct.variable())->get(), base_units, alias_units, check_variables);
			break;
		}
		case STRUCT_FUNCTION: {
			if(mstruct.function() == CALCULATOR->f_stripunits) break;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(!mstruct.function()->getArgumentDefinition(i + 1) || mstruct.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
					gatherInformation(mstruct[i], base_units, alias_units, check_variables);
				}
			}
			break;
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				gatherInformation(mstruct[i], base_units, alias_units, check_variables);
			}
			break;
		}
	}

}

int MathStructure::isUnitCompatible(const MathStructure &mstruct) const {
	if(!isMultiplication() && mstruct.isMultiplication()) return mstruct.isUnitCompatible(*this);
	int b1 = mstruct.containsRepresentativeOfType(STRUCT_UNIT, true, true);
	int b2 = containsRepresentativeOfType(STRUCT_UNIT, true, true);
	if(b1 < 0 || b2 < 0) return -1;
	if(b1 != b2) return false;
	if(!b1) return true;
	if(isMultiplication()) {
		size_t unit_count1 = 0, unit_count2 = 0;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isUnit_exp()) unit_count1++;
			else if(CHILD(i).containsRepresentativeOfType(STRUCT_UNIT, true, true) != 0) return -1;
		}
		if(mstruct.isMultiplication()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isUnit_exp()) unit_count2++;
				else if(mstruct[i].containsRepresentativeOfType(STRUCT_UNIT, true, true) != 0) return -1;
			}
		} else if(mstruct.isUnit_exp()) {
			if(unit_count1 > 1) return false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isUnit_exp()) return CHILD(1) == mstruct;
			}
		} else {
			return -1;
		}
		if(unit_count1 != unit_count2) return false;
		size_t i2 = 0;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isUnit_exp()) {
				for(; i2 < mstruct.size(); i2++) {
					if(mstruct[i2].isUnit_exp()) {
						if(CHILD(i) != mstruct[i2]) return false;
						i2++;
						break;
					}
				}
			}
		}
	} else if(isUnit_exp()) {
		if(mstruct.isUnit_exp()) return equals(mstruct);
	}
	return -1;
}

bool MathStructure::syncUnits(bool sync_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	if(SIZE == 0) return false;
	vector<Unit*> base_units;
	vector<AliasUnit*> alias_units;
	vector<CompositeUnit*> composite_units;
	gatherInformation(*this, base_units, alias_units);
	if(alias_units.empty() || base_units.size() + alias_units.size() == 1) return false;
	CompositeUnit *cu;
	bool b = false, do_erase = false;
	for(size_t i = 0; i < alias_units.size(); ) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			b = false;
			cu = (CompositeUnit*) alias_units[i]->baseUnit();
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(cu->containsRelativeTo(base_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
			for(size_t i2 = 0; !do_erase && i2 < alias_units.size(); i2++) {
				if(i != i2 && alias_units[i2]->baseUnit() != cu && cu->containsRelativeTo(alias_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			for(int i2 = 1; i2 <= (int) cu->countUnits(); i2++) {
				b = false;
				Unit *cub = cu->get(i2);
				switch(cub->subtype()) {
					case SUBTYPE_BASE_UNIT: {
						for(size_t i3 = 0; i3 < base_units.size(); i3++) {
							if(base_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) base_units.push_back(cub);
						break;
					}
					case SUBTYPE_ALIAS_UNIT: {
						for(size_t i3 = 0; i3 < alias_units.size(); i3++) {
							if(alias_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) alias_units.push_back((AliasUnit*) cub);
						break;
					}
					case SUBTYPE_COMPOSITE_UNIT: {
						gatherInformation(((CompositeUnit*) cub)->generateMathStructure(), base_units, alias_units);
						break;
					}
				}
			}
			i = 0;
		} else {
			i++;
		}
	}

	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		for(size_t i2 = 0; i2 < alias_units.size(); i2++) {
			if(i != i2 && alias_units[i]->baseUnit() == alias_units[i2]->baseUnit()) {
				if(alias_units[i2]->isParentOf(alias_units[i])) {
					do_erase = true;
					break;
				} else if(!alias_units[i]->isParentOf(alias_units[i2])) {
					b = false;
					for(size_t i3 = 0; i3 < base_units.size(); i3++) {
						if(base_units[i3] == alias_units[i2]->firstBaseUnit()) {
							b = true;
							break;
						}
					}
					if(!b) base_units.push_back((Unit*) alias_units[i]->baseUnit());
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(alias_units[i]->baseUnit() == base_units[i2]) {
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	b = false;
	bool fcr = false;
	for(size_t i = 0; i < composite_units.size(); i++) {
		if(convert(composite_units[i], sync_nonlinear_relations, (found_nonlinear_relations || sync_nonlinear_relations) ? &fcr : NULL, calculate_new_functions, feo)) b = true;
	}
	if(dissolveAllCompositeUnits()) b = true;
	for(size_t i = 0; i < base_units.size(); i++) {
		if(convert(base_units[i], sync_nonlinear_relations, (found_nonlinear_relations || sync_nonlinear_relations) ? &fcr : NULL, calculate_new_functions, feo)) b = true;
	}
	for(size_t i = 0; i < alias_units.size(); i++) {
		if(convert(alias_units[i], sync_nonlinear_relations, (found_nonlinear_relations || sync_nonlinear_relations) ? &fcr : NULL, calculate_new_functions, feo)) b = true;
	}
	if(b && sync_nonlinear_relations && fcr) CALCULATOR->error(false, _("Calculations involving conversion of units without proportional linear relationship (e.g. with multiple temperature units), might give unexpected results and is not recommended."), NULL);
	if(fcr && found_nonlinear_relations) *found_nonlinear_relations = fcr;
	return b;
}

bool has_approximate_relation_to_base(Unit *u, bool do_intervals) {
	if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
		if(((AliasUnit*) u)->isApproximate()) return do_intervals;
		if(((AliasUnit*) u)->expression().find_first_not_of(NUMBER_ELEMENTS EXPS) != string::npos && !((AliasUnit*) u)->hasNonlinearExpression()) return true;
		return has_approximate_relation_to_base(((AliasUnit*) u)->firstBaseUnit());
	} else if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		for(size_t i = 1; i <= ((CompositeUnit*) u)->countUnits(); i++) {
			if(has_approximate_relation_to_base(((CompositeUnit*) u)->get(i))) return true;
		}
	}
	return false;
}

bool MathStructure::testDissolveCompositeUnit(Unit *u) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) o_unit)->containsRelativeTo(u)) {
				set(((CompositeUnit*) o_unit)->generateMathStructure());
				return true;
			}
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT && o_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (o_unit->baseUnit()))->containsRelativeTo(u)) {
				if(convert(o_unit->baseUnit())) {
					convert(u);
					return true;
				}
			}
		}
	}
	return false;
}
bool test_dissolve_cu(MathStructure &mstruct, Unit *u, bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo, Prefix *new_prefix = NULL) {
	if(mstruct.isUnit()) {
		if(mstruct.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) mstruct.unit())->containsRelativeTo(u)) {
				mstruct.set(((CompositeUnit*) mstruct.unit())->generateMathStructure());
				return true;
			}
		} else if(mstruct.unit()->subtype() == SUBTYPE_ALIAS_UNIT && mstruct.unit()->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (mstruct.unit()->baseUnit()))->containsRelativeTo(u)) {
				if(mstruct.convert(mstruct.unit()->baseUnit(), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) {
					mstruct.convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
					return true;
				}
			}
		}
	}
	return false;
}
bool MathStructure::testCompositeUnit(Unit *u) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) o_unit)->containsRelativeTo(u)) {
				return true;
			}
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT && o_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (o_unit->baseUnit()))->containsRelativeTo(u)) {
				return true;
			}
		}
	}
	return false;
}
bool MathStructure::dissolveAllCompositeUnits() {
	switch(m_type) {
		case STRUCT_UNIT: {
			if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				set(((CompositeUnit*) o_unit)->generateMathStructure());
				return true;
			}
			break;
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).dissolveAllCompositeUnits()) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
	}
	return false;
}
bool MathStructure::setPrefixForUnit(Unit *u, Prefix *new_prefix) {
	if(m_type == STRUCT_UNIT && o_unit == u) {
		if(o_prefix != new_prefix) {
			Number new_value(1, 1);
			if(o_prefix) new_value = o_prefix->value();
			if(new_prefix) new_value.divide(new_prefix->value());
			o_prefix = new_prefix;
			multiply(new_value);
			return true;
		}
		return false;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).setPrefixForUnit(u, new_prefix)) {
			CHILD_UPDATED(i);
			b = true;
		}
	}
	return b;
}

bool searchSubMultiplicationsForComplexRelations(Unit *u, const MathStructure &mstruct) {
	int b_c = -1;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i].isUnit_exp()) {
			if((mstruct[i].isUnit() && u->hasNonlinearRelationTo(mstruct[i].unit())) || (mstruct[i].isPower() && u->hasNonlinearRelationTo(mstruct[i][0].unit()))) {
				return true;
			}
		} else if(b_c == -1 && mstruct[i].isMultiplication()) {
			b_c = -3;
		}
	}
	if(b_c == -3) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication() && searchSubMultiplicationsForComplexRelations(u, mstruct[i])) return true;
		}
	}
	return false;
}
bool MathStructure::convertToBaseUnits(bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo, bool avoid_approximate_variables) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			set(((CompositeUnit*) o_unit)->generateMathStructure());
			convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
			return true;
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT) {
			AliasUnit *au = (AliasUnit*) o_unit;
			if(au->hasNonlinearRelationTo(au->baseUnit())) {
				if(found_nonlinear_relations) *found_nonlinear_relations = true;
				if(!convert_nonlinear_relations) {
					if(!au->hasNonlinearExpression() && ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) || !au->hasApproximateExpression(avoid_approximate_variables, false))) {
						MathStructure mstruct_old(*this);
						if(convert(au->firstBaseUnit(), false, NULL, calculate_new_functions, feo) && !equals(mstruct_old)) {
							convertToBaseUnits(false, NULL, calculate_new_functions, feo, avoid_approximate_variables);
							return true;
						}
					}
					return false;
				}
			}
			if((feo.approximation == APPROXIMATION_EXACT || feo.approximation == APPROXIMATION_EXACT_VARIABLES) && au->hasApproximateRelationTo(au->baseUnit(), avoid_approximate_variables, false)) {
				MathStructure mstruct_old(*this);
				if(convert(au->firstBaseUnit(), false, NULL, calculate_new_functions, feo) && !equals(mstruct_old)) {
					convertToBaseUnits(false, NULL, calculate_new_functions, feo, avoid_approximate_variables);
					return true;
				}
				return false;
			}
			if(convert(au->baseUnit(), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) {
				convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
				return true;
			}
		}
		return false;
	} else if(m_type == STRUCT_MULTIPLICATION && (convert_nonlinear_relations || found_nonlinear_relations)) {
		AliasUnit *complex_au = NULL;
		if(convert_nonlinear_relations && convertToBaseUnits(false, NULL, calculate_new_functions, feo, avoid_approximate_variables)) {
			convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
			return true;
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) return false;
			if(CHILD(i).isUnit_exp() && CHILD(i).unit_exp_unit()->subtype() == SUBTYPE_ALIAS_UNIT) {
				AliasUnit *au = (AliasUnit*) CHILD(i).unit_exp_unit();
				if(au && au->hasNonlinearRelationTo(au->baseUnit())) {
					if(found_nonlinear_relations) {
						*found_nonlinear_relations = true;
					}
					if(convert_nonlinear_relations) {
						if(complex_au) {
							complex_au = NULL;
							convert_nonlinear_relations = false;
							break;
						} else {
							complex_au = au;
						}
					} else {
						break;
					}
				}
			}
		}
		if(convert_nonlinear_relations && complex_au && ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) || !complex_au->hasApproximateExpression(avoid_approximate_variables, false))) {
			MathStructure mstruct_old(*this);
			if(convert(complex_au->firstBaseUnit(), true, NULL, calculate_new_functions, feo) && !equals(mstruct_old)) {
				convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables);
				return true;
			}
		}
	} else if(m_type == STRUCT_FUNCTION) {
		if(o_function == CALCULATOR->f_stripunits) return false;
		bool b = false;
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) return b;
			if((!o_function->getArgumentDefinition(i + 1) || o_function->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && CHILD(i).convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CALCULATOR->aborted()) return b;
		if(CHILD(i).convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, avoid_approximate_variables)) {
			CHILD_UPDATED(i);
			b = true;
		}
	}
	return b;
}
bool convert_approximate(MathStructure &m, Unit *u, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals);
bool convert_approximate(MathStructure &m, const MathStructure unit_mstruct, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals) {
	bool b = false;
	if(unit_mstruct.type() == STRUCT_UNIT) {
		if(convert_approximate(m, unit_mstruct.unit(), feo, vars, uncs, units, do_intervals)) b = true;
	} else {
		for(size_t i = 0; i < unit_mstruct.size(); i++) {
			if(convert_approximate(m, unit_mstruct[i], feo, vars, uncs, units, do_intervals)) b = true;
		}
	}
	return b;
}
bool test_dissolve_cu_approximate(MathStructure &mstruct, Unit *u, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals) {
	if(mstruct.isUnit()) {
		if(mstruct.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) mstruct.unit())->containsRelativeTo(u)) {
				mstruct.set(((CompositeUnit*) mstruct.unit())->generateMathStructure());
				return true;
			}
		} else if(mstruct.unit()->subtype() == SUBTYPE_ALIAS_UNIT && mstruct.unit()->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (mstruct.unit()->baseUnit()))->containsRelativeTo(u)) {
				if(convert_approximate(mstruct, mstruct.unit()->baseUnit(), feo, vars, uncs, units, do_intervals)) {
					convert_approximate(mstruct, u, feo, vars, uncs, units, do_intervals);
					return true;
				}
			}
		}
	}
	return false;
}
bool convert_approximate(MathStructure &m, Unit *u, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, vector<Unit*> *units, bool do_intervals) {
	bool b = false;
	if(m.type() == STRUCT_UNIT && (m.unit() == u || m.prefix())) {
		return false;
	}
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT && !(m.type() == STRUCT_UNIT && m.unit()->baseUnit() == u)) {
		return convert_approximate(m, ((CompositeUnit*) u)->generateMathStructure(false, true), feo, vars, uncs, units, do_intervals);
	}
	if(m.type() == STRUCT_UNIT) {
		if(u->hasApproximateRelationTo(m.unit(), feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES, true) || u->hasNonlinearRelationTo(m.unit())) {
			return false;
		}
		if(!vars) return true;
		if(m.unit()->baseUnit() != u->baseUnit() && test_dissolve_cu_approximate(m, u, feo, vars, uncs, units, do_intervals)) {
			convert_approximate(m, u, feo, vars, uncs, units, do_intervals);
			return true;
		}
		MathStructure *exp = new MathStructure(1, 1, 0);
		MathStructure *mstruct = new MathStructure(1, 1, 0);
		Unit *u_m = m.unit();
		if(u_m->subtype() == SUBTYPE_ALIAS_UNIT && do_intervals) {
			for(size_t i = 0; i < units->size(); i++) {
				if((*units)[i] == u_m) {
					mstruct->set((*vars)[i]);
					u_m = ((AliasUnit*) u_m)->firstBaseUnit();
				}
			}
			if(u_m == m.unit()) {
				((AliasUnit*) u_m)->convertToFirstBaseUnit(*mstruct, *exp);
				if(mstruct->isNumber() && mstruct->number().isInterval()) {
					uncs->push_back(mstruct->number().uncertainty());
					units->push_back(u_m);
					Number nmid(mstruct->number());
					nmid.intervalToMidValue();
					nmid.setApproximate(false);
					KnownVariable *v = new KnownVariable("", string("(") + format_and_print(nmid) + ")", nmid);
					mstruct->set(v);
					vars->push_back(v);
					v->destroy();
					u_m = ((AliasUnit*) u_m)->firstBaseUnit();
				} else {
					mstruct->set(1, 1, 0);
					exp->set(1, 1, 0);
				}
			}
		}
		if(u->convert(u_m, *mstruct, *exp) && (do_intervals || (!mstruct->containsInterval(true, false, false, 1, true) && !exp->containsInterval(true, false, false, 1, true)))) {
			m.setUnit(u);
			if(!exp->isOne()) {
				if(feo.calculate_functions) {
					calculate_nondifferentiable_functions(*exp, feo, true, true, feo.interval_calculation != INTERVAL_CALCULATION_VARIANCE_FORMULA ? 0 : ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) ? 2 : 1));
				}
				if(do_intervals) exp->calculatesub(feo, feo, true);
				m.raise_nocopy(exp);
			} else {
				exp->unref();
			}
			if(!mstruct->isOne()) {
				if(feo.calculate_functions) {
					calculate_nondifferentiable_functions(*mstruct, feo, true, true, feo.interval_calculation != INTERVAL_CALCULATION_VARIANCE_FORMULA ? 0 : ((feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES) ? 2 : 1));
				}
				if(do_intervals) mstruct->calculatesub(feo, feo, true);
				m.multiply_nocopy(mstruct);
			} else {
				mstruct->unref();
			}
			return true;
		} else {
			exp->unref();
			mstruct->unref();
			return false;
		}
	} else {
		if(m.type() == STRUCT_FUNCTION) {
			if(m.function() == CALCULATOR->f_stripunits) return b;
			for(size_t i = 0; i < m.size(); i++) {
				if(m.size() > 100 && CALCULATOR->aborted()) return b;
				if((!m.function()->getArgumentDefinition(i + 1) || m.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && convert_approximate(m[i], u, feo, vars, uncs, units, do_intervals)) {
					m.childUpdated(i + 1);
					b = true;
				}
			}
			return b;
		}
		for(size_t i = 0; i < m.size(); i++) {
			if(m.size() > 100 && CALCULATOR->aborted()) return b;
			if(convert_approximate(m[i], u, feo, vars, uncs, units, do_intervals)) {
				m.childUpdated(i + 1);
				b = true;
			}
		}
		return b;
	}
	return b;
}

bool contains_approximate_relation_to_base(const MathStructure &m, bool do_intervals) {
	if(m.isUnit()) {
		return has_approximate_relation_to_base(m.unit(), do_intervals);
	} else if(m.isFunction() && m.function() == CALCULATOR->f_stripunits) {
		return false;
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_approximate_relation_to_base(m[i], do_intervals)) return true;
	}
	return false;
}

bool sync_approximate_units(MathStructure &m, const EvaluationOptions &feo, vector<KnownVariable*> *vars, vector<MathStructure> *uncs, bool do_intervals) {
	if(m.size() == 0) return false;
	if(!contains_approximate_relation_to_base(m, do_intervals)) return false;
	bool check_variables = (feo.approximation != APPROXIMATION_EXACT && feo.approximation != APPROXIMATION_EXACT_VARIABLES);
	vector<Unit*> base_units;
	vector<AliasUnit*> alias_units;
	vector<CompositeUnit*> composite_units;
	gatherInformation(m, base_units, alias_units, check_variables);
	if(alias_units.empty() || base_units.size() + alias_units.size() == 1) return false;
	CompositeUnit *cu;
	bool b = false, do_erase = false;
	for(size_t i = 0; i < alias_units.size(); ) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			b = false;
			cu = (CompositeUnit*) alias_units[i]->baseUnit();
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(cu->containsRelativeTo(base_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
			for(size_t i2 = 0; !do_erase && i2 < alias_units.size(); i2++) {
				if(i != i2 && alias_units[i2]->baseUnit() != cu && cu->containsRelativeTo(alias_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			for(int i2 = 1; i2 <= (int) cu->countUnits(); i2++) {
				b = false;
				Unit *cub = cu->get(i2);
				switch(cub->subtype()) {
					case SUBTYPE_BASE_UNIT: {
						for(size_t i3 = 0; i3 < base_units.size(); i3++) {
							if(base_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) base_units.push_back(cub);
						break;
					}
					case SUBTYPE_ALIAS_UNIT: {
						for(size_t i3 = 0; i3 < alias_units.size(); i3++) {
							if(alias_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) alias_units.push_back((AliasUnit*) cub);
						break;
					}
					case SUBTYPE_COMPOSITE_UNIT: {
						gatherInformation(((CompositeUnit*) cub)->generateMathStructure(), base_units, alias_units, check_variables);
						break;
					}
				}
			}
			i = 0;
		} else {
			i++;
		}
	}

	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		for(size_t i2 = 0; i2 < alias_units.size(); i2++) {
			if(i != i2 && alias_units[i]->baseUnit() == alias_units[i2]->baseUnit()) {
				if(alias_units[i2]->isParentOf(alias_units[i])) {
					do_erase = true;
					break;
				} else if(!alias_units[i]->isParentOf(alias_units[i2])) {
					b = false;
					for(size_t i3 = 0; i3 < base_units.size(); i3++) {
						if(base_units[i3] == alias_units[i2]->firstBaseUnit()) {
							b = true;
							break;
						}
					}
					if(!b) base_units.push_back((Unit*) alias_units[i]->baseUnit());
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(alias_units[i]->baseUnit() == base_units[i2]) {
					do_erase = true;
					break;
				}
			}
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	b = false;
	vector<Unit*> units;
	for(size_t i = 0; i < composite_units.size(); i++) {
		if(convert_approximate(m, composite_units[i], feo, vars, uncs, &units, do_intervals)) b = true;
	}
	if(m.dissolveAllCompositeUnits()) b = true;
	for(size_t i = 0; i < base_units.size(); i++) {
		if(convert_approximate(m, base_units[i], feo, vars, uncs, &units, do_intervals)) b = true;
	}
	for(size_t i = 0; i < alias_units.size(); i++) {
		if(convert_approximate(m, alias_units[i], feo, vars, uncs, &units, do_intervals)) b = true;
	}
	return b;
}


bool MathStructure::convert(Unit *u, bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo, Prefix *new_prefix) {
	if(m_type == STRUCT_ADDITION && containsType(STRUCT_DATETIME, false, true, false) > 0) return false;
	bool b = false;
	if(m_type == STRUCT_UNIT && o_unit == u) {
		if((new_prefix || o_prefix) && o_prefix != new_prefix) {
			Number new_value(1, 1);
			if(o_prefix) new_value = o_prefix->value();
			if(new_prefix) new_value.divide(new_prefix->value());
			o_prefix = new_prefix;
			multiply(new_value);
			return true;
		}
		return false;
	}
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT && !(m_type == STRUCT_UNIT && o_unit->baseUnit() == u)) {
		return convert(((CompositeUnit*) u)->generateMathStructure(false, true), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo);
	}
	if(m_type == STRUCT_UNIT) {
		if(u->hasNonlinearRelationTo(o_unit)) {
			if(found_nonlinear_relations) *found_nonlinear_relations = true;
			if(!convert_nonlinear_relations) return false;
		}
		if(o_unit->baseUnit() != u->baseUnit() && test_dissolve_cu(*this, u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
			convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
			return true;
		}
		MathStructure *exp = new MathStructure(1, 1, 0);
		MathStructure *mstruct = new MathStructure(1, 1, 0);
		if(o_prefix) {
			mstruct->set(o_prefix->value());
		}
		if(u->convert(o_unit, *mstruct, *exp) && (feo.approximation != APPROXIMATION_EXACT || (!mstruct->isApproximate() && !exp->isApproximate()))) {
			setUnit(u);
			o_prefix = new_prefix;
			if(new_prefix) {
				divide(new_prefix->value());
			}
			if(!exp->isOne()) {
				if(calculate_new_functions) exp->calculateFunctions(feo, true, false);
				raise_nocopy(exp);
			} else {
				exp->unref();
			}
			if(!mstruct->isOne()) {
				if(calculate_new_functions) mstruct->calculateFunctions(feo, true, false);
				multiply_nocopy(mstruct);
			} else {
				mstruct->unref();
			}
			return true;
		} else {
			exp->unref();
			mstruct->unref();
			return false;
		}
	} else {
		if(convert_nonlinear_relations || found_nonlinear_relations) {
			if(m_type == STRUCT_MULTIPLICATION) {
				long int b_c = -1;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isUnit_exp()) {
						Unit *u2 = CHILD(i).isPower() ? CHILD(i)[0].unit() : CHILD(i).unit();
						if(u->hasNonlinearRelationTo(u2)) {
							if(found_nonlinear_relations) *found_nonlinear_relations = true;

							b_c = i;
							break;
						}
					} else if(CHILD(i).isMultiplication()) {
						b_c = -3;
					}
				}
				if(b_c == -3) {
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).isMultiplication()) {
							if(searchSubMultiplicationsForComplexRelations(u, CHILD(i))) {
								if(!convert_nonlinear_relations) {
									*found_nonlinear_relations = true;
									break;
								}
								flattenMultiplication(*this);
								return convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							}
						}
					}
				}
				if(convert_nonlinear_relations && b_c >= 0) {
					if(flattenMultiplication(*this)) return convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
					MathStructure mstruct(1, 1);
					MathStructure mstruct2(1, 1);
					if(SIZE == 2) {
						if(b_c == 0) mstruct = CHILD(1);
						else mstruct = CHILD(0);
						if(mstruct.isUnit_exp()) {
							mstruct2 = mstruct;
							mstruct.set(1, 1, 0);
						}
					} else if(SIZE > 2) {
						mstruct = *this;
						size_t nr_of_del = 0;
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isUnit_exp()) {
								mstruct.delChild(i + 1 - nr_of_del);
								nr_of_del++;
								if((long int) i != b_c) {
									if(mstruct2.isOne()) mstruct2 = CHILD(i);
									else mstruct2.multiply(CHILD(i), true);
								}
							}
						}
						if(mstruct.size() == 1) mstruct.setToChild(1);
						else if(mstruct.size() == 0) mstruct.set(1, 1, 0);
					}
					MathStructure exp(1, 1);
					Unit *u2;
					bool b_p = false;
					if(CHILD(b_c).isPower()) {
						if(CHILD(b_c)[0].unit()->baseUnit() != u->baseUnit()) {
							if(CHILD(b_c)[0].unit()->subtype() != SUBTYPE_BASE_UNIT && (CHILD(b_c)[0].unit()->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) CHILD(b_c)[0].unit())->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT)) {
								convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo);
							} else {
								return false;
							}
							convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							return true;
						}
						exp = CHILD(b_c)[1];
						u2 = CHILD(b_c)[0].unit();
						if(CHILD(b_c)[0].prefix()) b_p = true;
					} else {
						if(CHILD(b_c).unit()->baseUnit() != u->baseUnit()) {
							if(CHILD(b_c).unit()->subtype() != SUBTYPE_BASE_UNIT && (CHILD(b_c).unit()->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) CHILD(b_c).unit())->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT)) {
								convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo);
							} else {
								return false;
							}
							convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							return true;
						}
						u2 = CHILD(b_c).unit();
						if(CHILD(b_c).prefix()) b_p = true;
					}
					size_t efc = 0, mfc = 0;
					if(calculate_new_functions) {
						efc = exp.countFunctions();
						mfc = mstruct.countFunctions();
					}
					if(u->convert(u2, mstruct, exp)) {
						if(feo.approximation == APPROXIMATION_EXACT && !isApproximate() && (mstruct.isApproximate() || exp.isApproximate())) return false;
						if(b_p) {
							unformat(feo);
							return convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
						}
						set(u);
						if(!exp.isOne()) {
							if(calculate_new_functions && exp.countFunctions() > efc) exp.calculateFunctions(feo, true, false);
							raise(exp);
						}
						if(!mstruct2.isOne()) {
							multiply(mstruct2);
						}
						if(!mstruct.isOne()) {
							if(calculate_new_functions && mstruct.countFunctions() > mfc) mstruct.calculateFunctions(feo, true, false);
							multiply(mstruct);
						}
						return true;
					}
					return false;
				}
			} else if(m_type == STRUCT_POWER) {
				if(CHILD(0).isUnit() && u->hasNonlinearRelationTo(CHILD(0).unit())) {
					if(found_nonlinear_relations) *found_nonlinear_relations = true;
					if(convert_nonlinear_relations) {
						if(CHILD(0).unit()->baseUnit() != u->baseUnit()) {
							if(CHILD(0).unit()->subtype() != SUBTYPE_BASE_UNIT && (CHILD(0).unit()->subtype() != SUBTYPE_ALIAS_UNIT || ((AliasUnit*) CHILD(0).unit())->firstBaseUnit()->subtype() != SUBTYPE_COMPOSITE_UNIT)) {
								convertToBaseUnits(convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo);
							} else {
								return false;
							}
							convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix);
							return true;
						}
						MathStructure exp(CHILD(1));
						MathStructure mstruct(1, 1);
						if(CHILD(0).prefix()) {
							mstruct.set(CHILD(0).prefix()->value());
							mstruct.raise(exp);
						}
						size_t efc = 0;
						if(calculate_new_functions) {
							efc = exp.countFunctions();
						}
						if(u->convert(CHILD(0).unit(), mstruct, exp)) {
							if(feo.approximation == APPROXIMATION_EXACT && !isApproximate() && (mstruct.isApproximate() || exp.isApproximate())) return false;
							set(u);
							if(!exp.isOne()) {
								if(calculate_new_functions && exp.countFunctions() > efc) exp.calculateFunctions(feo, true, false);
								raise(exp);
							}
							if(!mstruct.isOne()) {
								if(calculate_new_functions) mstruct.calculateFunctions(feo, true, false);
								multiply(mstruct);
							}
							return true;
						}
						return false;
					}
				}
			}
			/*if(m_type == STRUCT_MULTIPLICATION || m_type == STRUCT_POWER) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).convert(u, false, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
				return b;
			}*/
		}
		if(m_type == STRUCT_FUNCTION) {
			if(o_function == CALCULATOR->f_stripunits) return b;
			for(size_t i = 0; i < SIZE; i++) {
				if(CALCULATOR->aborted()) return b;
				if((!o_function->getArgumentDefinition(i + 1) || o_function->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && CHILD(i).convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CALCULATOR->aborted()) return b;
			if(CHILD(i).convert(u, convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, new_prefix)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}
	return b;
}
bool MathStructure::convert(const MathStructure unit_mstruct, bool convert_nonlinear_relations, bool *found_nonlinear_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	bool b = false;
	if(unit_mstruct.type() == STRUCT_UNIT) {
		if(convert(unit_mstruct.unit(), convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo, feo.keep_prefixes ? unit_mstruct.prefix() : NULL)) b = true;
	} else {
		for(size_t i = 0; i < unit_mstruct.size(); i++) {
			if(convert(unit_mstruct[i], convert_nonlinear_relations, found_nonlinear_relations, calculate_new_functions, feo)) b = true;
		}
	}
	return b;
}

bool contains_angle_unit(const MathStructure &m, const ParseOptions &po) {
	if(m.isUnit() && m.unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit()) return true;
	if(m.isVariable() && m.variable()->isKnown()) return contains_angle_unit(((KnownVariable*) m.variable())->get(), po);
	if(m.isFunction()) return po.angle_unit == ANGLE_UNIT_NONE && (m.function() == CALCULATOR->f_asin || m.function() == CALCULATOR->f_acos || m.function() == CALCULATOR->f_atan);
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_angle_unit(m[i], po)) return true;
	}
	return false;
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
	if(mstruct.isUnit() && mstruct.prefix() == NULL && m_type == STRUCT_UNIT && mstruct.unit() == o_unit) return 1;
	if(equals(mstruct, true, true)) return 1;
	size_t i_occ = 0;
	for(size_t i = 0; i < SIZE; i++) {
		i_occ += CHILD(i).countOccurrences(mstruct);
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
	if(include_interval_function && m.type() == STRUCT_FUNCTION && (m.function() == CALCULATOR->f_interval || m.function() == CALCULATOR->f_uncertainty)) return 1;
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
	if(include_interval_function && m_type == STRUCT_FUNCTION && (o_function == CALCULATOR->f_interval || o_function == CALCULATOR->f_uncertainty)) return 1;
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
			if(mtype == STRUCT_UNIT) {
				if(o_function == CALCULATOR->f_stripunits) return 0;
				if(o_function->subtype() == SUBTYPE_USER_FUNCTION || o_function == CALCULATOR->f_register || o_function == CALCULATOR->f_stack || o_function == CALCULATOR->f_load) return -1;
				// (eo.parse_options.angle_unit == ANGLE_UNIT_NONE && (o_function == CALCULATOR->f_asin || o_function == CALCULATOR->f_acos || o_function == CALCULATOR->f_atan || o_function == CALCULATOR->f_radtodef))
				if(o_function == CALCULATOR->f_ln || o_function == CALCULATOR->f_logn || o_function == CALCULATOR->f_arg || o_function == CALCULATOR->f_gamma || o_function == CALCULATOR->f_beta || o_function == CALCULATOR->f_factorial || o_function == CALCULATOR->f_besselj || o_function == CALCULATOR->f_bessely || o_function == CALCULATOR->f_erf || o_function == CALCULATOR->f_erfc || o_function == CALCULATOR->f_li || o_function == CALCULATOR->f_Li || o_function == CALCULATOR->f_Ei || o_function == CALCULATOR->f_Si || o_function == CALCULATOR->f_Ci || o_function == CALCULATOR->f_Shi || o_function == CALCULATOR->f_Chi || o_function == CALCULATOR->f_signum || o_function == CALCULATOR->f_heaviside || o_function == CALCULATOR->f_lambert_w || o_function == CALCULATOR->f_sinc || o_function == CALCULATOR->f_sin || o_function == CALCULATOR->f_cos || o_function == CALCULATOR->f_tan || o_function == CALCULATOR->f_sinh || o_function == CALCULATOR->f_cosh || o_function == CALCULATOR->f_tanh || o_function == CALCULATOR->f_asinh || o_function == CALCULATOR->f_acosh || o_function == CALCULATOR->f_atanh || o_function == CALCULATOR->f_asin || o_function == CALCULATOR->f_acos || o_function == CALCULATOR->f_atan) return 0;
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
	if(b_protected) b_protected = false;
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
			else if(!once_only) replace(mfrom, mto, once_only, exclude_function_arguments);
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
		if(CHILD(i).replace(mfrom, mto, once_only, exclude_function_arguments)) {
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
			if(mtype != STRUCT_UNIT || (o_function != CALCULATOR->f_sqrt && o_function != CALCULATOR->f_root && o_function != CALCULATOR->f_cbrt)) return b;
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

MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector, const EvaluationOptions &eo) const {
	if(steps < 1) {
		steps = 1;
	}
	MathStructure x_value(min);
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	if(steps > 1000000) {
		CALCULATOR->error(true, _("Too many data points"), NULL);
		return y_vector;
	}
	MathStructure step(max);
	step.calculateSubtract(min, eo);
	step.calculateDivide(steps - 1, eo);
	if(!step.isNumber() || step.number().isNegative()) {
		CALCULATOR->error(true, _("The selected min and max do not result in a positive, finite number of data points"), NULL);
		return y_vector;
	}
	y_vector.resizeVector(steps, m_zero);
	if(x_vector) x_vector->resizeVector(steps, m_zero);
	for(int i = 0; i < steps; i++) {
		if(x_vector) {
			(*x_vector)[i] = x_value;
		}
		y_value = *this;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		y_vector[i] = y_value;
		if(x_value.isNumber()) x_value.number().add(step.number());
		else x_value.calculateAdd(step, eo);
		if(CALCULATOR->aborted()) {
			y_vector.resizeVector(i, m_zero);
			if(x_vector) x_vector->resizeVector(i, m_zero);
			return y_vector;
		}
	}
	return y_vector;
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector, const EvaluationOptions &eo) const {
	MathStructure x_value(min);
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	if(min != max) {
		MathStructure mtest(max);
		mtest.calculateSubtract(min, eo);
		if(!step.isZero()) mtest.calculateDivide(step, eo);
		if(step.isZero() || !mtest.isNumber() || mtest.number().isNegative()) {
			CALCULATOR->error(true, _("The selected min, max and step size do not result in a positive, finite number of data points"), NULL);
			return y_vector;
		} else if(mtest.number().isGreaterThan(1000000)) {
			CALCULATOR->error(true, _("Too many data points"), NULL);
			return y_vector;
		}
		mtest.number().round();
		unsigned int steps = mtest.number().uintValue();
		y_vector.resizeVector(steps, m_zero);
		if(x_vector) x_vector->resizeVector(steps, m_zero);
	}
	ComparisonResult cr = max.compare(x_value);
	size_t i = 0;
	while(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
		if(x_vector) {
			if(i >= x_vector->size()) x_vector->addChild(x_value);
			else (*x_vector)[i] = x_value;
		}
		y_value = *this;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		if(i >= y_vector.size()) y_vector.addChild(y_value);
		else y_vector[i] = y_value;
		if(x_value.isNumber()) x_value.number().add(step.number());
		else x_value.calculateAdd(step, eo);
		cr = max.compare(x_value);
		if(CALCULATOR->aborted()) {
			y_vector.resizeVector(i, m_zero);
			if(x_vector) x_vector->resizeVector(i, m_zero);
			return y_vector;
		}
		i++;
	}
	y_vector.resizeVector(i, m_zero);
	if(x_vector) x_vector->resizeVector(i, m_zero);
	return y_vector;
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &x_vector, const EvaluationOptions &eo) const {
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	for(size_t i = 1; i <= x_vector.countChildren(); i++) {
		if(CALCULATOR->aborted()) {
			y_vector.clearVector();
			return y_vector;
		}
		y_value = *this;
		y_value.replace(x_mstruct, x_vector.getChild(i));
		y_value.eval(eo);
		y_vector.addChild(y_value);
	}
	return y_vector;
}

void remove_zero_mul(MathStructure &m);
void remove_zero_mul(MathStructure &m) {
	if(m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isZero()) {
				m.clear(true);
				return;
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		remove_zero_mul(m[i]);
	}
}
bool MathStructure::differentiate(const MathStructure &x_var, const EvaluationOptions &eo) {
	if(CALCULATOR->aborted()) {
		transform(CALCULATOR->f_diff);
		addChild(x_var);
		addChild(m_one);
		addChild(m_undefined);
		return false;
	}
	if(equals(x_var)) {
		set(m_one);
		return true;
	}
	if(containsRepresentativeOf(x_var, true, true) == 0) {
		clear(true);
		return true;
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE;) {
				if(CHILD(i).differentiate(x_var, eo)) {
					CHILD_UPDATED(i);
					if(SIZE > 1 && CHILD(i).isZero()) {ERASE(i);}
					else i++;
				} else i++;
			}
			if(SIZE == 1) SET_CHILD_MAP(0)
			break;
		}
		case STRUCT_LOGICAL_AND: {
			bool b = true;
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).isComparison()) {b = false; break;}
			}
			if(b) {
				MathStructure mtest(*this);
				mtest.setType(STRUCT_MULTIPLICATION);
				if(mtest.differentiate(x_var, eo) && mtest.containsFunction(CALCULATOR->f_diff, true) <= 0) {
					set(mtest);
					break;
				}
			}
		}
		case STRUCT_COMPARISON: {
			if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
				if(!CHILD(1).isZero()) CHILD(0) -= CHILD(1);
				SET_CHILD_MAP(0)
				if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_LESS) negate();
				MathStructure mstruct(*this);
				MathStructure mstruct2(*this);
				transform(CALCULATOR->f_heaviside);
				transform(CALCULATOR->f_dirac);
				mstruct2.transform(CALCULATOR->f_dirac);
				multiply(mstruct2);
				if(ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_EQUALS_LESS) multiply_nocopy(new MathStructure(2, 1, 0));
				else multiply_nocopy(new MathStructure(-2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				return true;
			}
			transform(CALCULATOR->f_diff);
			addChild(x_var);
			addChild(m_one);
			addChild(m_undefined);
			return false;
		}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			clear(true);
			break;
		}
		case STRUCT_POWER: {
			if(SIZE < 1) {
				clear(true);
				break;
			} else if(SIZE < 2) {
				setToChild(1, true);
				return differentiate(x_var, eo);
			}
			bool x_in_base = CHILD(0).containsRepresentativeOf(x_var, true, true) != 0;
			bool x_in_exp = CHILD(1).containsRepresentativeOf(x_var, true, true) != 0;
			if(x_in_base && !x_in_exp) {
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				if(!CHILD(1).isNumber() || !CHILD(1).number().add(-1)) CHILD(1) += m_minus_one;
				if(CHILD(0) == x_var) {
					multiply_nocopy(exp_mstruct);
				} else {
					MathStructure *base_mstruct = new MathStructure(CHILD(0));
					multiply_nocopy(exp_mstruct);
					base_mstruct->differentiate(x_var, eo);
					multiply_nocopy(base_mstruct);
				}
			} else if(!x_in_base && x_in_exp) {
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				exp_mstruct->differentiate(x_var, eo);
				if(CHILD(0).isVariable() && CHILD(0).variable() == CALCULATOR->v_e) {
					multiply_nocopy(exp_mstruct);
				} else {
					MathStructure *mstruct = new MathStructure(CALCULATOR->f_ln, &CHILD(0), NULL);
					multiply_nocopy(mstruct);
					multiply_nocopy(exp_mstruct);
				}
			} else if(x_in_base && x_in_exp) {
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				exp_mstruct->differentiate(x_var, eo);
				base_mstruct->differentiate(x_var, eo);
				base_mstruct->divide(CHILD(0));
				base_mstruct->multiply(CHILD(1));
				MathStructure *mstruct = new MathStructure(CALCULATOR->f_ln, &CHILD(0), NULL);
				mstruct->multiply_nocopy(exp_mstruct);
				mstruct->add_nocopy(base_mstruct);
				multiply_nocopy(mstruct);
			} else {
				clear(true);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_sqrt && SIZE == 1) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				raise(m_minus_one);
				multiply(nr_half);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function == CALCULATOR->f_root && THIS_VALID_ROOT) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				MathStructure *mexp = new MathStructure(CHILD(1));
				mexp->negate();
				mexp->add(m_one);
				raise_nocopy(mexp);
				divide(CHILD(0)[1]);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function == CALCULATOR->f_cbrt && SIZE == 1) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				raise(Number(-2, 1, 0));
				divide(nr_three);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if((o_function == CALCULATOR->f_ln && SIZE == 1) || (o_function == CALCULATOR->f_logn && SIZE == 2 && CHILD(1).isVariable() && CHILD(1).variable() == CALCULATOR->v_e)) {
				if(CHILD(0) == x_var) {
					setToChild(1, true);
					inverse();
				} else {
					MathStructure *mstruct = new MathStructure(CHILD(0));
					setToChild(1, true);
					inverse();
					mstruct->differentiate(x_var, eo);
					multiply_nocopy(mstruct);
				}
			} else if(o_function == CALCULATOR->f_logn && SIZE == 2) {
				MathStructure *mstruct = new MathStructure(CALCULATOR->f_ln, &CHILD(1), NULL);
				setFunction(CALCULATOR->f_ln);
				ERASE(1)
				divide_nocopy(mstruct);
				return differentiate(x_var, eo);
			} else if(o_function == CALCULATOR->f_beta && SIZE == 2) {
				MathStructure mstruct(CHILD(0));
				setToChild(1, true);
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_arg && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setFunction(CALCULATOR->f_dirac);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(CALCULATOR->v_pi);
				negate();
			} else if(o_function == CALCULATOR->f_gamma && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				MathStructure mstruct2(*this);
				mstruct2.setFunction(CALCULATOR->f_digamma);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_factorial && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				CHILD(0) += m_one;
				MathStructure mstruct2(*this);
				mstruct2.setFunction(CALCULATOR->f_digamma);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_besselj && SIZE == 2 && CHILD(0).isInteger()) {
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function == CALCULATOR->f_bessely && SIZE == 2 && CHILD(0).isInteger()) {
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function == CALCULATOR->f_erf && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				mexp.negate();
				set(CALCULATOR->v_e, true);
				raise(mexp);
				multiply(nr_two);
				multiply(CALCULATOR->v_pi);
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_ERFI && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				set(CALCULATOR->v_e, true);
				raise(mexp);
				multiply(nr_two);
				multiply(CALCULATOR->v_pi);
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_erfc && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				mexp.negate();
				set(CALCULATOR->v_e, true);
				raise(mexp);
				multiply(Number(-2, 1));
				multiply(CALCULATOR->v_pi);
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_S && SIZE == 1) {
				setFunction(CALCULATOR->f_sin);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->v_pi;
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_C && SIZE == 1) {
				setFunction(CALCULATOR->f_cos);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->v_pi;
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_li && SIZE == 1) {
				setFunction(CALCULATOR->f_ln);
				MathStructure mstruct(CHILD(0));
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_Li && SIZE == 2) {
				CHILD(0) += m_minus_one;
				MathStructure mstruct(CHILD(1));
				divide(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_Ei && SIZE == 1) {
				MathStructure mexp(CALCULATOR->v_e);
				mexp ^= CHILD(0);
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				mexp *= mdiff;
				setToChild(1, true);
				inverse();
				multiply(mexp);
			} else if(o_function == CALCULATOR->f_Si && SIZE == 1) {
				setFunction(CALCULATOR->f_sinc);
				CHILD_UPDATED(0)
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_Ci && SIZE == 1) {
				setFunction(CALCULATOR->f_cos);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				CHILD(0) *= CALCULATOR->getRadUnit();
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_Shi && SIZE == 1) {
				setFunction(CALCULATOR->f_sinh);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_Chi && SIZE == 1) {
				setFunction(CALCULATOR->f_cosh);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_abs && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				inverse();
				multiply(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_signum && SIZE == 2) {
				MathStructure mstruct(CHILD(0));
				ERASE(1)
				setFunction(CALCULATOR->f_dirac);
				multiply_nocopy(new MathStructure(2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_heaviside && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setFunction(CALCULATOR->f_dirac);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_lambert_w && SIZE >= 1) {
				MathStructure *mstruct = new MathStructure(*this);
				MathStructure *mstruct2 = new MathStructure(CHILD(0));
				mstruct->add(m_one);
				mstruct->multiply(CHILD(0));
				mstruct2->differentiate(x_var, eo);
				multiply_nocopy(mstruct2);
				divide_nocopy(mstruct);
			} else if(o_function == CALCULATOR->f_sin && SIZE == 1) {
				setFunction(CALCULATOR->f_cos);
				MathStructure mstruct(CHILD(0));
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function == CALCULATOR->f_cos && SIZE == 1) {
				setFunction(CALCULATOR->f_sin);
				MathStructure mstruct(CHILD(0));
				negate();
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function == CALCULATOR->f_tan && SIZE == 1) {
				setFunction(CALCULATOR->f_tan);
				MathStructure mstruct(CHILD(0));
				raise(2);
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
				if(CALCULATOR->getRadUnit()) divide(CALCULATOR->getRadUnit());
			} else if(o_function == CALCULATOR->f_sinh && SIZE == 1) {
				setFunction(CALCULATOR->f_cosh);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_cosh && SIZE == 1) {
				setFunction(CALCULATOR->f_sinh);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function == CALCULATOR->f_tanh && SIZE == 1) {
				setFunction(CALCULATOR->f_tanh);
				MathStructure mstruct(CHILD(0));
				raise(2);
				negate();
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function == CALCULATOR->f_asin && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_acos && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				negate();
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_atan && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_asinh && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_acosh && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				MathStructure mfac(*this);
				add(m_minus_one);
				raise(Number(-1, 2));
				mfac.add(m_one);
				mfac.raise(Number(-1, 2));
				multiply(mfac);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_atanh && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_sinc && SIZE == 1) {
				// diff(x)*(cos(x)/x-sin(x)/x^2)
				MathStructure m_cos(CALCULATOR->f_cos, &CHILD(0), NULL);
				if(CALCULATOR->getRadUnit()) m_cos[0].multiply(CALCULATOR->getRadUnit());
				m_cos.divide(CHILD(0));
				MathStructure m_sin(CALCULATOR->f_sin, &CHILD(0), NULL);
				if(CALCULATOR->getRadUnit()) m_sin[0].multiply(CALCULATOR->getRadUnit());
				MathStructure mstruct(CHILD(0));
				mstruct.raise(Number(-2, 1, 0));
				m_sin.multiply(mstruct);
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				set(m_sin);
				negate();
				add(m_cos);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_stripunits && SIZE == 1) {
				CHILD(0).differentiate(x_var, eo);
			} else if(o_function == CALCULATOR->f_integrate && SIZE >= 4 && CHILD(3) == x_var && CHILD(1).isUndefined() && CHILD(2).isUndefined()) {
				SET_CHILD_MAP(0);
			} else if(o_function == CALCULATOR->f_diff && (SIZE == 3 || (SIZE == 4 && CHILD(3).isUndefined())) && CHILD(1) == x_var) {
				CHILD(2) += m_one;
			} else if(o_function == CALCULATOR->f_diff && SIZE == 2 && CHILD(1) == x_var) {
				APPEND(MathStructure(2, 1, 0));
			} else if(o_function == CALCULATOR->f_diff && SIZE == 1 && x_var == (Variable*) CALCULATOR->v_x) {
				APPEND(x_var);
				APPEND(MathStructure(2, 1, 0));
			} else {
				if(!eo.calculate_functions || !calculateFunctions(eo, false)) {
					transform(CALCULATOR->f_diff);
					addChild(x_var);
					addChild(m_one);
					addChild(m_undefined);
					return false;
				} else {
					return differentiate(x_var, eo);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			MathStructure mstruct, vstruct;
			size_t idiv = 0;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isPower() && CHILD(i)[1].isNumber() && CHILD(i)[1].number().isNegative()) {
					if(idiv == 0) {
						if(CHILD(i)[1].isMinusOne()) {
							vstruct = CHILD(i)[0];
						} else {
							vstruct = CHILD(i);
							vstruct[1].number().negate();
						}
					} else {
						if(CHILD(i)[1].isMinusOne()) {
							vstruct.multiply(CHILD(i)[0], true);
						} else {
							vstruct.multiply(CHILD(i), true);
							vstruct[vstruct.size() - 1][1].number().negate();
						}
					}
					idiv++;
				}
			}
			if(idiv == SIZE) {
				set(-1, 1);
				MathStructure mdiv(vstruct);
				mdiv ^= MathStructure(2, 1, 0);
				mdiv.inverse();
				multiply(mdiv);
				MathStructure diff_u(vstruct);
				diff_u.differentiate(x_var, eo);
				multiply(diff_u, true);
				break;
			} else if(idiv > 0) {
				idiv = 0;
				for(size_t i = 0; i < SIZE; i++) {
					if(!CHILD(i).isPower() || !CHILD(i)[1].isNumber() || !CHILD(i)[1].number().isNegative()) {
						if(idiv == 0) {
							mstruct = CHILD(i);
						} else {
							mstruct.multiply(CHILD(i), true);
						}
						idiv++;
					}
				}
				MathStructure mdiv(vstruct);
				mdiv ^= MathStructure(2, 1, 0);
				MathStructure diff_v(vstruct);
				diff_v.differentiate(x_var, eo);
				MathStructure diff_u(mstruct);
				diff_u.differentiate(x_var, eo);
				vstruct *= diff_u;
				mstruct *= diff_v;
				set_nocopy(vstruct, true);
				subtract(mstruct);
				divide(mdiv);
				break;
			}
			if(SIZE > 2) {
				mstruct.set(*this);
				mstruct.delChild(mstruct.size());
				MathStructure mstruct2(LAST);
				MathStructure mstruct3(mstruct);
				mstruct.differentiate(x_var, eo);
				mstruct2.differentiate(x_var, eo);
				mstruct *= LAST;
				mstruct2 *= mstruct3;
				set(mstruct, true);
				add(mstruct2);
			} else {
				mstruct = CHILD(0);
				MathStructure mstruct2(CHILD(1));
				mstruct.differentiate(x_var, eo);
				mstruct2.differentiate(x_var, eo);
				mstruct *= CHILD(1);
				mstruct2 *= CHILD(0);
				set(mstruct, true);
				add(mstruct2);
			}
			break;
		}
		case STRUCT_SYMBOLIC: {
			if(representsNumber(true)) {
				clear(true);
			} else {
				transform(CALCULATOR->f_diff);
				addChild(x_var);
				addChild(m_one);
				addChild(m_undefined);
				return false;
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if(eo.approximation != APPROXIMATION_EXACT || !o_variable->isApproximate()) {
					set(((KnownVariable*) o_variable)->get(), true);
					unformat(eo);
					return differentiate(x_var, eo);
				} else {
					transform(CALCULATOR->f_diff);
					addChild(x_var);
					addChild(m_one);
					addChild(m_undefined);
					return false;
				}
			}
		}
		default: {
			transform(CALCULATOR->f_diff);
			addChild(x_var);
			addChild(m_one);
			addChild(m_undefined);
			return false;
		}
	}
	remove_zero_mul(*this);
	return true;
}

bool MathStructure::decomposeFractions(const MathStructure &x_var, const EvaluationOptions &eo) {
	MathStructure mtest2;
	bool b = false;
	int mmul_i = 0;
	if(isPower()) {
		if(CHILD(1).isMinusOne() && CHILD(0).isMultiplication() && CHILD(0).size() >= 2) {
			mtest2 = CHILD(0);
			b = true;
		}
	} else if(isMultiplication() && SIZE == 2) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isPower() && CHILD(i)[1].isMinusOne() && (CHILD(i)[0].isPower() || CHILD(i)[0].isMultiplication())) {
				mtest2 = CHILD(i)[0];
				b = true;
			} else if(CHILD(i) == x_var) {
				mmul_i = 1;
			} else if(CHILD(i).isPower() && CHILD(i)[0] == x_var && CHILD(i)[1].isInteger() && CHILD(i)[1].number().isPositive() && CHILD(i)[1].number().isLessThan(100)) {
				mmul_i = CHILD(i)[1].number().intValue();
			}
		}
		if(mmul_i == 0) b = false;
	}
	if(b) {
		if(!mtest2.isMultiplication()) mtest2.transform(STRUCT_MULTIPLICATION);
		MathStructure mfacs, mnew;
		mnew.setType(STRUCT_ADDITION);
		mfacs.setType(STRUCT_ADDITION);
		vector<int> i_degrees;
		i_degrees.resize(mtest2.size(), 1);
		for(size_t i = 0; i < mtest2.size() && b; i++) {
			if(CALCULATOR->aborted()) return false;
			MathStructure mfactor = mtest2[i];
			if(mtest2[i].isPower()) {
				if(!mtest2[i][1].isInteger() || !mtest2[i][1].number().isPositive() || mtest2[i][1].isOne() || mtest2[i][1].number().isGreaterThan(100)) {
					b = false;
					break;
				}
				mfactor = mtest2[i][0];
			}
			if(mfactor.isAddition()) {
				bool b2 = false;
				for(size_t i2 = 0; i2 < mfactor.size() && b; i2++) {
					if(mfactor[i2].isMultiplication()) {
						bool b_x = false;
						for(size_t i3 = 0; i3 < mfactor[i2].size() && b; i3++) {
							if(!b_x && mfactor[i2][i3].isPower() && mfactor[i2][i3][0] == x_var) {
								if(!mfactor[i2][i3][1].isInteger() || !mfactor[i2][i3][1].number().isPositive() || mfactor[i2][i3][1].isOne() || mfactor[i2][i3][1].number().isGreaterThan(100)) {
									b = false;
								} else if(mfactor[i2][i3][1].number().intValue() > i_degrees[i]) {
									i_degrees[i] = mfactor[i2][i3][1].number().intValue();
								}
								b_x = true;
							} else if(!b_x && mfactor[i2][i3] == x_var) {
								b_x = true;
							} else if(mfactor[i2][i3].containsRepresentativeOf(x_var, true, true) != 0) {
								b = false;
							}
						}
						if(!b_x) b2 = true;
					} else if(mfactor[i2].isPower() && mfactor[i2][0] == x_var) {
						if(!mfactor[i2][1].isInteger() || !mfactor[i2][1].number().isPositive() || mfactor[i2][1].isOne() || mfactor[i2][1].number().isGreaterThan(100)) {
							b = false;
						} else if(mfactor[i2][1].number().intValue() > i_degrees[i]) {
							i_degrees[i] = mfactor[i2][1].number().intValue();
						}
					} else if(mfactor[i2] == x_var) {
					} else if(mfactor[i2].containsRepresentativeOf(x_var, true, true) != 0) {
						b = false;
					} else {
						b2 = true;
					}
				}
				if(!b2) b = false;
			} else if(mfactor != x_var) {
				b = false;
			}
		}
		MathStructure mtest3, mnums3;
		mnums3.clearVector();
		mtest3.clearVector();
		if(b) {
			UnknownVariable *var = new UnknownVariable("", string("a"));
			var->setAssumptions(new Assumptions());
			MathStructure mvar(var);
			for(size_t i = 0; i < mtest2.size(); i++) {
				if(CALCULATOR->aborted()) return false;
				if(i_degrees[i] == 1) {
					MathStructure mnum(1, 1, 0);
					if(mtest2.size() != 1) {
						mnum = mtest2;
						mnum.delChild(i + 1, true);
					}
					MathStructure mx(mtest2[i]);
					mx.transform(COMPARISON_EQUALS, m_zero);
					mx.replace(x_var, mvar);
					mx.isolate_x(eo, mvar);
					mx.calculatesub(eo, eo, true);
					if(mx.isLogicalOr()) mx.setToChild(1);
					if(!mx.isComparison() || mx.comparisonType() != COMPARISON_EQUALS || mx[0] != var) {b = false; break;}
					mx.setToChild(2);
					if(mtest2.size() != 1) {
						mfacs.addChild(mnum);
						mnum.replace(x_var, mx);
						mnum.inverse();
					}
					if(mmul_i > 0) {
						mx ^= Number(mmul_i, 1);
						if(mtest2.size() == 1) mnum = mx;
						else mnum *= mx;
					}
					mnum.calculatesub(eo, eo, true);
					if(mtest2.size() == 1) mfacs.addChild(mnum);
					else mfacs.last() *= mnum;
					mnums3.addChild(mnum);
					mtest3.addChild(mtest2[i]);
				}
			}
			var->destroy();
		}
		if(b) {
			vector<UnknownVariable*> vars;
			bool b_solve = false;
			for(size_t i = 0; i < mtest2.size(); i++) {
				if(CALCULATOR->aborted()) return false;
				if(mtest2[i].isPower()) {
					int i_exp = mtest2[i][1].number().intValue();
					for(int i_exp_n = mtest2[i][1].number().intValue() - (i_degrees[i] == 1 ? 1 : 0); i_exp_n > 0; i_exp_n--) {
						if(i_exp_n == 1) {
							mtest3.addChild(mtest2[i][0]);
						} else {
							mtest3.addChild(mtest2[i]);
							if(i_exp_n != i_exp) mtest3.last()[1].number() = i_exp_n;
						}
						if(i_exp ==  i_exp_n) {
							if(mtest2.size() > 1) {
								mfacs.addChild(mtest2);
								mfacs.last().delChild(i + 1, true);
							}
						} else {
							mfacs.addChild(mtest2);
							if(i_exp - i_exp_n == 1) mfacs.last()[i].setToChild(1);
							else mfacs.last()[i][1].number() = i_exp - i_exp_n;
						}
						if(i_degrees[i] == 1) {
							UnknownVariable *var = new UnknownVariable("", string("a") + i2s(mtest3.size()));
							var->setAssumptions(new Assumptions());
							mnums3.addChild_nocopy(new MathStructure(var));
							vars.push_back(var);
						} else {
							mnums3.addChild_nocopy(new MathStructure());
							mnums3.last().setType(STRUCT_ADDITION);
							for(int i2 = 1; i2 <= i_degrees[i]; i2++) {
								UnknownVariable *var = new UnknownVariable("", string("a") + i2s(mtest3.size()) + i2s(i2));
								var->setAssumptions(new Assumptions());
								if(i2 == 1) {
									mnums3.last().addChild_nocopy(new MathStructure(var));
								} else {
									mnums3.last().addChild_nocopy(new MathStructure(var));
									mnums3.last().last() *= x_var;
									if(i2 > 2) mnums3.last().last().last() ^= Number(i2 - 1, 1);
								}
								vars.push_back(var);
							}
						}
						if(i_exp != i_exp_n || mtest2.size() > 1) mfacs.last() *= mnums3.last();
						else mfacs.addChild(mnums3.last());
						b_solve = true;
					}
				} else if(i_degrees[i] > 1) {
					mtest3.addChild(mtest2[i]);
					if(mtest2.size() > 1) {
						mfacs.addChild(mtest2);
						mfacs.last().delChild(i + 1, true);
					}
					mnums3.addChild_nocopy(new MathStructure());
					mnums3.last().setType(STRUCT_ADDITION);
					for(int i2 = 1; i2 <= i_degrees[i]; i2++) {
						UnknownVariable *var = new UnknownVariable("", string("a") + i2s(mtest3.size()) + i2s(i2));
						var->setAssumptions(new Assumptions());
						if(i2 == 1) {
							mnums3.last().addChild_nocopy(new MathStructure(var));
						} else {
							mnums3.last().addChild_nocopy(new MathStructure(var));
							mnums3.last().last() *= x_var;
							if(i2 > 2) mnums3.last().last().last() ^= Number(i2 - 1, 1);
						}
						vars.push_back(var);
					}
					if(mtest2.size() > 1) mfacs.last() *= mnums3.last();
					else mfacs.addChild(mnums3.last());
					b_solve = true;
				}
			}
			if(b_solve) {
				mfacs.childrenUpdated(true);
				MathStructure mfac_expand(mfacs);
				mfac_expand.calculatesub(eo, eo, true);
				size_t i_degree = 0;
				if(mfac_expand.isAddition()) {
					i_degree = mfac_expand.degree(x_var).uintValue();
					if(i_degree >= 100 || i_degree <= 0) b = false;
				}
				if(i_degree == 0) b = false;
				if(b) {
					MathStructure m_eqs;
					m_eqs.resizeVector(i_degree + 1, m_zero);
					for(size_t i = 0; i < m_eqs.size(); i++) {
						for(size_t i2 = 0; i2 < mfac_expand.size();) {
							if(CALCULATOR->aborted()) return false;
							bool b_add = false;
							if(i == 0) {
								if(!mfac_expand[i2].contains(x_var)) b_add = true;
							} else {
								if(mfac_expand[i2].isMultiplication() && mfac_expand[i2].size() >= 2) {
									for(size_t i3 = 0; i3 < mfac_expand[i2].size(); i3++) {
										if(i == 1 && mfac_expand[i2][i3] == x_var) b_add = true;
										else if(i > 1 && mfac_expand[i2][i3].isPower() && mfac_expand[i2][i3][0] == x_var && mfac_expand[i2][i3][1] == i) b_add = true;
										if(b_add) {
											mfac_expand[i2].delChild(i3 + 1, true);
											break;
										}
									}

								} else {
									if(i == 1 && mfac_expand[i2] == x_var) b_add = true;
									else if(i > 1 && mfac_expand[i2].isPower() && mfac_expand[i2][0] == x_var && mfac_expand[i2][1] == i) b_add = true;
									if(b_add) mfac_expand[i2].set(1, 1, 0);
								}
							}
							if(b_add) {
								if(m_eqs[i].isZero()) m_eqs[i] = mfac_expand[i2];
								else m_eqs[i].add(mfac_expand[i2], true);
								mfac_expand.delChild(i2 + 1);
							} else {
								i2++;
							}
						}
					}
					if(mfac_expand.size() == 0 && m_eqs.size() >= vars.size()) {
						for(size_t i = 0; i < m_eqs.size(); i++) {
							if(!m_eqs[i].isZero()) {
								m_eqs[i].transform(COMPARISON_EQUALS, i == (size_t) mmul_i ? m_one : m_zero);
							}
						}
						for(size_t i = 0; i < m_eqs.size();) {
							if(m_eqs[i].isZero()) {
								m_eqs.delChild(i + 1);
								if(i == (size_t) mmul_i) b = false;
							} else {
								i++;
							}
						}
						if(b) {
							MathStructure vars_m;
							vars_m.clearVector();
							for(size_t i = 0; i < vars.size(); i++) {
								vars_m.addChild_nocopy(new MathStructure(vars[i]));
							}
							for(size_t i = 0; i < m_eqs.size() && b; i++) {
								for(size_t i2 = 0; i2 < vars_m.size(); i2++) {
									if(m_eqs[i].isComparison() && m_eqs[i][0].contains(vars_m[i2], true)) {
										if(CALCULATOR->aborted() || m_eqs[i].countTotalChildren() > 1000) return false;
										m_eqs[i].isolate_x(eo, vars_m[i2]);
										if(CALCULATOR->aborted() || m_eqs[i].countTotalChildren() > 10000) return false;
										m_eqs[i].calculatesub(eo, eo, true);
										if(m_eqs[i].isLogicalOr()) m_eqs[i].setToChild(1);
										if(m_eqs[i].isComparison() && m_eqs[i].comparisonType() == COMPARISON_EQUALS && m_eqs[i][0] == vars_m[i2]) {
											for(size_t i3 = 0; i3 < m_eqs.size(); i3++) {
												if(i3 != i) {
													if(CALCULATOR->aborted()) return false;
													m_eqs[i3][0].calculateReplace(vars_m[i2], m_eqs[i][1], eo);
													if(CALCULATOR->aborted()) return false;
													m_eqs[i3][1].calculateReplace(vars_m[i2], m_eqs[i][1], eo);
												}
											}
											vars_m.delChild(i2 + 1);
										} else {
											b = false;
										}
										break;
									}
								}
							}
							for(size_t i = 0; i < m_eqs.size();) {
								if(CALCULATOR->aborted()) return false;
								m_eqs[i].calculatesub(eo, eo);
								if(m_eqs[i].isZero()) {b = false; break;}
								if(m_eqs[i].isOne()) m_eqs.delChild(i + 1);
								else i++;
							}
							if(b && vars_m.size() == 0) {
								for(size_t i = 0; i < vars.size(); i++) {
									for(size_t i2 = 0; i2 < m_eqs.size(); i2++) {
										if(m_eqs[i2][0] == vars[i]) {
											mnums3.replace(vars[i], m_eqs[i2][1]);
											break;
										}
									}
								}
							} else {
								b = false;
							}
						}
					} else {
						b = false;
					}
				}
			}
			for(size_t i = 0; i < vars.size(); i++) {
				vars[i]->destroy();
			}
			if(b) {
				for(size_t i = 0; i < mnums3.size(); i++) {
					mnums3.calculatesub(eo, eo, true);
					if(!mnums3[i].isZero()) {
						if(mnums3[i].isOne()) {
							mnew.addChild(mtest3[i]);
							mnew.last().inverse();
						} else {
							mnew.addChild(mnums3[i]);
							mnew.last() /= mtest3[i];
						}
					}
				}
			}
			if(b) {
				if(mnew.size() == 0) mnew.clear();
				else if(mnew.size() == 1) mnew.setToChild(1);
				mnew.childrenUpdated();
				if(equals(mnew, true)) return false;
				set(mnew, true);
				return true;
			}
		}
	}
	return false;
}

bool expand_partial_fractions(MathStructure &m, const EvaluationOptions &eo, bool do_simplify = true) {
	MathStructure mtest(m);
	if(do_simplify) {
		mtest.simplify(eo);
		mtest.calculatesub(eo, eo, true);
	}
	if(mtest.isPower() && mtest[1].representsNegative()) {
		MathStructure x_var = mtest[0].find_x_var();
		if(!x_var.isUndefined() && mtest[0].factorize(eo, false, 0, 0, false, false, NULL, x_var)) {
			if(mtest.decomposeFractions(x_var, eo) && mtest.isAddition()) {
				m = mtest;
				return true;
			}
		}
	} else if(mtest.isMultiplication()) {
		for(size_t i = 0; i < mtest.size(); i++) {
			if(mtest[i].isPower() && mtest[i][1].isMinusOne() && mtest[i][0].isAddition()) {
				MathStructure x_var = mtest[i][0].find_x_var();
				if(!x_var.isUndefined()) {
					MathStructure mfac(mtest);
					mfac.delChild(i + 1, true);
					bool b_poly = true;
					if(mfac.contains(x_var, true)) {
						MathStructure mquo, mrem;
						b_poly = polynomial_long_division(mfac, mtest[i][0], x_var, mquo, mrem, eo, true);
						if(b_poly && !mquo.isZero()) {
							m = mquo;
							if(!mrem.isZero()) {
								m += mrem;
								m.last() *= mtest[i];
								m.childrenUpdated();
							}
							expand_partial_fractions(m, eo, false);
							return true;
						}
					}
					if(b_poly && mtest[i][0].factorize(eo, false, 0, 0, false, false, NULL, x_var)) {
						MathStructure mmul(1, 1, 0);
						while(mtest[i][0].isMultiplication() && mtest[i][0].size() >= 2 && !mtest[i][0][0].containsRepresentativeOf(x_var, true)) {
							if(mmul.isOne()) {
								mmul = mtest[i][0][0];
								mmul.calculateInverse(eo);
							} else {
								mmul *= mtest[i][0][0];
								mmul.last().calculateInverse(eo);
								mmul.calculateMultiplyLast(eo);
							}
							mtest[i][0].delChild(1, true);
						}
						for(size_t i2 = 0; i2 < mtest.size();) {
							if(i2 != i && !mtest[i2].containsRepresentativeOf(x_var, true)) {
								if(mmul.isOne()) {
									mmul = mtest[i2];
								} else {
									mmul.calculateMultiply(mtest[i2], eo);
								}
								if(mtest.size() == 2) {
									mtest.setToChild(i + 1, true);
									break;
								} else {
									mtest.delChild(i2 + 1);
									if(i2 < i) i--;
								}
							} else {
								i2++;
							}
						}
						if(mtest.decomposeFractions(x_var, eo) && mtest.isAddition()) {
							m = mtest;
							if(!mmul.isOne()) {
								for(size_t i2 = 0; i2 < m.size(); i2++) {
									m[i2].calculateMultiply(mmul, eo);
								}
							}
							return true;
						}
					}
				}
			}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < mtest.size(); i++) {
			if(expand_partial_fractions(mtest[i], eo, false)) {
				b = true;
			}
		}
		if(b) {
			m = mtest;
			m.calculatesub(eo, eo, false);
			return true;
		}
	}
	return false;
}
bool MathStructure::expandPartialFractions(const EvaluationOptions &eo) {
	return expand_partial_fractions(*this, eo, true);
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
			} else if(mstruct->variable() == CALCULATOR->v_x) {
				return *mstruct;
			} else if(!x_mstruct->isVariable()) {
				x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->v_y) {
				x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->v_z && x_mstruct->variable() != CALCULATOR->v_y) {
				x_mstruct = mstruct;
			}
		} else if(mstruct->isSymbolic()) {
			if(!x_mstruct->isVariable() && (!x_mstruct->isSymbolic() || x_mstruct->symbol() > mstruct->symbol())) {
				x_mstruct = mstruct;
			}
		}
	}
	return *x_mstruct;
}

bool MathStructure::isRationalPolynomial(bool allow_non_rational_coefficient, bool allow_interval_coefficient) const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			if(allow_interval_coefficient) return o_number.isReal() && o_number.isNonZero();
			if(allow_non_rational_coefficient) return o_number.isReal() && !o_number.isInterval() && o_number.isNonZero();
			return o_number.isRational() && !o_number.isZero();
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isAddition() || CHILD(i).isMultiplication() || !CHILD(i).isRationalPolynomial(allow_non_rational_coefficient, allow_interval_coefficient)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isAddition() || !CHILD(i).isRationalPolynomial(allow_non_rational_coefficient, allow_interval_coefficient)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(1).isInteger() && CHILD(1).number().isNonNegative() && !CHILD(1).number().isOne() && CHILD(1).number() < 1000 && !CHILD(0).isNumber() && !CHILD(0).isMultiplication() && !CHILD(0).isAddition() && !CHILD(0).isPower() && CHILD(0).isRationalPolynomial(allow_non_rational_coefficient, allow_interval_coefficient);
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_uncertainty || o_function == CALCULATOR->f_interval || containsInterval() || containsInfinity()) return false;
		}
		case STRUCT_UNIT: {}
		case STRUCT_VARIABLE: {}
		case STRUCT_SYMBOLIC: {
			return representsNonMatrix() && !representsUndefined(true, true);
		}
		default: {}
	}
	return false;
}
const Number &MathStructure::overallCoefficient() const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			return o_number;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isNumber()) {
					return CHILD(i).number();
				}
			}
			return nr_one;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isNumber()) {
					return CHILD(i).number();
				}
			}
			return nr_zero;
		}
		case STRUCT_POWER: {
			return nr_zero;
		}
		default: {}
	}
	return nr_zero;
}

bool MathStructure::inParentheses() const {return b_parentheses;}
void MathStructure::setInParentheses(bool b) {b_parentheses = b;}


