/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016, 2018  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "BuiltinFunctions.h"
#include "util.h"
#include "MathStructure.h"
#include "Number.h"
#include "Calculator.h"
#include "Variable.h"
#include "Unit.h"

#include <sstream>
#include <time.h>
#include <limits>
#include <algorithm>

#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

#define FR_FUNCTION_2(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC(vargs[1].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}

BitXorFunction::BitXorFunction() : MathFunction("xor", 2) {
	ArgumentSet *arg = new ArgumentSet();
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONE));
	arg->addArgument(new VectorArgument);
	setArgumentDefinition(1, arg);
	arg = new ArgumentSet();
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONE));
	arg->addArgument(new VectorArgument);
	setArgumentDefinition(2, arg);
}
int BitXorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isNumber() && vargs[1].isNumber()) {
		Number nr(vargs[0].number());
		if(nr.bitXor(vargs[1].number()) && (eo.approximation >= APPROXIMATION_APPROXIMATE || !nr.isApproximate() || vargs[0].number().isApproximate() || vargs[1].number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || vargs[0].number().isComplex() || vargs[1].number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || vargs[0].number().includesInfinity() || vargs[1].number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	} else if(vargs[0].isVector() && vargs[1].isVector()) {
		int i1 = 0, i2 = 1;
		if(vargs[0].size() < vargs[1].size()) {
			i1 = 1;
			i2 = 0;
		}
		mstruct.clearVector();
		mstruct.resizeVector(vargs[i1].size(), m_undefined);
		if(mstruct.size() < vargs[i1].size()) return 0;
		size_t i = 0;
		for(; i < vargs[i2].size(); i++) {
			mstruct[i].set(CALCULATOR->getFunctionById(FUNCTION_ID_XOR), &vargs[i1][i], &vargs[i2][0], NULL);
		}
		for(; i < vargs[i1].size(); i++) {
			mstruct[i] = vargs[i1][i];
			mstruct[i].add(m_zero, OPERATION_GREATER);
		}
		return 1;
	}
	mstruct = vargs[0];
	mstruct.add(vargs[1], OPERATION_BITWISE_XOR);
	return 0;
}
XorFunction::XorFunction() : MathFunction("lxor", 2) {
}
int XorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	mstruct.transform(STRUCT_LOGICAL_XOR, vargs[1]);
	return 1;
}

ShiftFunction::ShiftFunction() : MathFunction("shift", 2, 3) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "1");
}
int ShiftFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs.size() >= 3 && !vargs[2].number().getBoolean() && vargs[1].number().isNegative()) {
		Number nr(vargs[0].number());
		Number nr_div(vargs[1].number());
		if(!nr_div.negate() || !nr_div.exp2() || !nr.divide(nr_div) || !nr.trunc()) return false;
		mstruct.set(nr);
		return 1;
	}
	FR_FUNCTION_2(shift)
}
CircularShiftFunction::CircularShiftFunction() : MathFunction("bitrot", 2, 4) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument());
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(3, "0");
	setDefaultValue(4, "1");
}
int CircularShiftFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isZero()) {
		mstruct.clear();
		return 1;
	}
	Number nr(vargs[0].number());
	unsigned int bits = vargs[2].number().uintValue();
	if(bits == 0) {
		bits = nr.integerLength();
		if(bits <= 8) bits = 8;
		else if(bits <= 16) bits = 16;
		else if(bits <= 32) bits = 32;
		else if(bits <= 64) bits = 64;
		else if(bits <= 128) bits = 128;
		else {
			bits = (unsigned int) ::ceil(::log2(bits));
			bits = ::pow(2, bits);
		}
	}
	Number nr_n(vargs[1].number());
	nr_n.rem(bits);
	if(nr_n.isZero()) {
		mstruct = nr;
		return 1;
	}
	PrintOptions po;
	po.base = BASE_BINARY;
	po.base_display = BASE_DISPLAY_NORMAL;
	po.binary_bits = bits;
	string str = nr.print(po);
	remove_blanks(str);
	if(str.length() < bits) return 0;
	if(nr_n.isNegative()) {
		nr_n.negate();
		std::rotate(str.rbegin(), str.rbegin() + nr_n.uintValue(), str.rend());
	} else {
		std::rotate(str.begin(), str.begin() + nr_n.uintValue(), str.end());
	}
	ParseOptions pao;
	pao.base = BASE_BINARY;
	pao.twos_complement = vargs[3].number().getBoolean();
	mstruct = Number(str, pao);
	return 1;
}
BitCmpFunction::BitCmpFunction() : MathFunction("bitcmp", 1, 3) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setDefaultValue(2, "0");
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "0");
}
int BitCmpFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	if(vargs.size() >= 3 && vargs[2].number().getBoolean()) {
		if(nr.bitNot()) {
			mstruct = nr;
			return 1;
		}
		return 0;
	}
	unsigned int bits = vargs[1].number().uintValue();
	if(bits == 0) {
		bits = nr.integerLength();
		if(bits <= 8) bits = 8;
		else if(bits <= 16) bits = 16;
		else if(bits <= 32) bits = 32;
		else if(bits <= 64) bits = 64;
		else if(bits <= 128) bits = 128;
		else {
			bits = (unsigned int) ::ceil(::log2(bits));
			bits = ::pow(2, bits);
		}
	}
	if(nr.bitCmp(bits)) {
		mstruct = nr;
		return 1;
	}
	return 0;
}

IFFunction::IFFunction() : MathFunction("if", 3, 4) {
	setArgumentDefinition(4, new BooleanArgument());
	setDefaultValue(4, "0");
}
bool IFFunction::representsScalar(const MathStructure &vargs) const {
	return vargs.size() >= 3 && vargs[1].representsScalar() && vargs[2].representsScalar();
}
bool IFFunction::representsNonMatrix(const MathStructure &vargs) const {
	return vargs.size() >= 3 && vargs[1].representsNonMatrix() && vargs[2].representsNonMatrix();
}
int IFFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isNumber()) {
		int result = vargs[0].number().getBoolean();
		if(result > 0) {
			mstruct = vargs[1];
		} else if(result == 0 || vargs[3].isOne()) {
			mstruct = vargs[2];
		} else {
			return 0;
		}
		return 1;
	}
	mstruct = vargs[0];
	if(mstruct.isVector() || (mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_HORZCAT || mstruct.function()->id() == FUNCTION_ID_VERTCAT))) {
		MathStructure m2(vargs[1]);
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].eval(eo);
			if(!mstruct[i].isNumber() && vargs[3].isZero()) {
				return -1;
			}
			int result = mstruct[i].number().getBoolean();
			if(result > 0) {
				if(!m2.isVector() && (!m2.isFunction() || (m2.function()->id() != FUNCTION_ID_HORZCAT && m2.function()->id() != FUNCTION_ID_VERTCAT)) && !m2.representsScalar()) m2.eval(eo);
				if((m2.isVector() || (m2.isFunction() && (m2.function()->id() == FUNCTION_ID_HORZCAT || m2.function()->id() == FUNCTION_ID_VERTCAT))) && m2.size() > 0) {
					if(i >= m2.size()) mstruct = m2[i % m2.size()];
					else mstruct = m2[i];
				} else {
					mstruct = m2;
				}
				return 1;
			} else if(result < 0 && vargs[3].isZero()) {
				return -1;
			}
		}
		mstruct = vargs[2];
		return 1;
	}
	mstruct.eval(eo);
	if(mstruct.isNumber()) {
		int result = mstruct.number().getBoolean();
		if(result > 0) {
			mstruct = vargs[1];
		} else if(result == 0 || vargs[3].isOne()) {
			mstruct = vargs[2];
		} else {
			return -1;
		}
		return 1;
	}
	if(vargs[3].isOne()) {
		mstruct = vargs[2];
		return 1;
	}
	return -1;
}

bool calculate_replace2(MathStructure &m, const MathStructure &mfrom1, const MathStructure &mto1, const MathStructure &mfrom2, const MathStructure &mto2, const EvaluationOptions &eo) {
	if(m.equals(mfrom1, true, true)) {
		m.set(mto1);
		return true;
	}
	if(m.equals(mfrom2, true, true)) {
		m.set(mto2);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(calculate_replace2(m[i], mfrom1, mto1, mfrom2, mto2, eo)) {
			b = true;
			m.childUpdated(i + 1);
		}
	}
	if(b) {
		m.calculatesub(eo, eo, false);
		if(eo.calculate_functions && m.type() == STRUCT_FUNCTION) m.calculateFunctions(eo, false);
	}
	return b;
}
bool calculate_userfunctions2(MathStructure &m, const MathStructure &x_mstruct, const MathStructure &x_mstruct2, const EvaluationOptions &eo) {
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(calculate_userfunctions2(m[i], x_mstruct, x_mstruct2, eo)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(m.isFunction()) {
		if(!m.contains(x_mstruct, true) && !m.contains(x_mstruct2, true) && !m.containsFunctionId(FUNCTION_ID_RAND, true, true, true) && !m.containsFunctionId(FUNCTION_ID_RANDN, true, true, true) && !m.containsFunctionId(FUNCTION_ID_RAND_POISSON, true, true, true)) {
			if(m.calculateFunctions(eo, false)) {
				b_ret = true;
				calculate_userfunctions2(m, x_mstruct, x_mstruct2, eo);
			}
		} else if(m.function()->subtype() == SUBTYPE_USER_FUNCTION && m.function()->condition().empty()) {
			bool b = true;
			for(size_t i = 0; i < ((UserFunction*) m.function())->countSubfunctions(); i++) {
				if(((UserFunction*) m.function())->subfunctionPrecalculated(i + 1)) {
					b = false;
					break;
				}
			}
			for(size_t i = 0; b && i < m.size(); i++) {
				Argument *arg = m.function()->getArgumentDefinition(i + 1);
				if(arg && arg->tests() && (arg->type() != ARGUMENT_TYPE_FREE || !arg->getCustomCondition().empty() || arg->rationalPolynomial() || arg->zeroForbidden() || (arg->handlesVector() && m[i].isVector())) && m[i].contains(x_mstruct, true)) {
					b = false;
					break;
				}
			}
			if(b && m.calculateFunctions(eo, false)) {
				calculate_userfunctions2(m, x_mstruct, x_mstruct2, eo);
				b_ret = true;
			}
		}
	}
	return b_ret;
}

ForFunction::ForFunction() : MathFunction("for", 7) {
	setArgumentDefinition(2, new SymbolicArgument());
	setArgumentDefinition(7, new SymbolicArgument());
}
int ForFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	vector<Variable*> vars;
	mstruct = vargs[4];
	if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
		while(true) {
			Variable *v = NULL;
			Variable *uv = find_interval_replace_var_comp(mstruct, eo, &v);
			if(!uv) break;
			if(v) mstruct.replace(v, uv);
			vars.push_back(uv);
		}
	}
	mstruct.eval(eo);
	MathStructure m5(vargs[5]);
	if(vargs[5].isComparison() && vargs[5].comparisonType() == COMPARISON_EQUALS && vargs[5][0] == vargs[6]) m5 = vargs[5][1];
	else m5 = vargs[5];
	MathStructure mbak(m5);
	if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
		while(true) {
			Variable *v = NULL;
			Variable *uv = find_interval_replace_var_comp(m5, eo, &v);
			if(!uv) break;
			if(v) m5.replace(v, uv);
			vars.push_back(uv);
		}
	}
	EvaluationOptions eo2 = eo;
	eo2.calculate_functions = false;
	eo2.expand = false;
	CALCULATOR->beginTemporaryStopMessages();
	m5.eval(eo2);
	if(calculate_userfunctions2(m5, vargs[6], vargs[1], eo)) {
		if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
			while(true) {
				Variable *v = NULL;
				Variable *uv = find_interval_replace_var_comp(m5, eo, &v);
				if(!uv) break;
				if(v) m5.replace(v, uv);
				vars.push_back(uv);
			}
		}
		m5.calculatesub(eo2, eo2, true);
	}
	int im = 0;
	if(CALCULATOR->endTemporaryStopMessages(NULL, &im) > 0 || im > 0) {
		m5 = mbak;
	}
	MathStructure mcounter = vargs[0];
	mcounter.eval(eo);
	MathStructure mtest;
	MathStructure mcount;
	MathStructure mupdate;
	while(true) {
		mtest = vargs[2];
		mtest.replace(vargs[1], mcounter);
		mtest.eval(eo);
		if(!mtest.isNumber() || CALCULATOR->aborted()) {
			for(size_t i = 0; i < vars.size(); i++) vars[i]->destroy();
			return 0;
		}
		if(!mtest.number().getBoolean()) {
			break;
		}
		mupdate = m5;
		calculate_replace2(mupdate, vargs[1], mcounter, vargs[6], mstruct, eo);
		mstruct = mupdate;
		if(vargs[3].isComparison() && vargs[3].comparisonType() == COMPARISON_EQUALS && vargs[3][0] == vargs[1]) mcount = vargs[3][1];
		else mcount = vargs[3];
		mcount.calculateReplace(vargs[1], mcounter, eo, true);
		mcounter = mcount;
	}
	for(size_t i = 0; i < vars.size(); i++) {
		if(vars[i]->isKnown()) mstruct.replace(vars[i], ((KnownVariable*) vars[i])->get());
		else mstruct.replace(vars[i], ((UnknownVariable*) vars[i])->interval());
		vars[i]->destroy();
	}
	return 1;

}

