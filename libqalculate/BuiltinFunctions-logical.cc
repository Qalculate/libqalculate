/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016, 2018, 2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

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

BitSetFunction::BitSetFunction() : MathFunction("bitset", 2, 5) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_ULONG));
	setArgumentDefinition(3, new BooleanArgument());
	setDefaultValue(3, "1");
	setArgumentDefinition(4, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setDefaultValue(4, "0");
	setArgumentDefinition(5, new BooleanArgument());
	setDefaultValue(5, "0");
}
int BitSetFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	unsigned int bits = vargs[3].number().uintValue();
	bool b_signed = vargs[4].number().getBoolean();
	bool b_set = vargs[2].number().getBoolean();
	unsigned long int max_index = vargs[1].number().ulintValue();
	nr.bitSet(max_index, b_set);
	if(bits > 0 && max_index > bits) {
		Number nrbits = max_index;
		nrbits.log(nr_two);
		nrbits.ceil();
		nrbits.exp2();
		bits = nrbits.uintValue();
	}
	if(bits > 0 && max_index == bits && (b_signed || vargs[0].number().isNegative()) && (b_set != vargs[0].number().isNegative())) {
		PrintOptions po;
		po.base = BASE_BINARY;
		po.base_display = BASE_DISPLAY_NONE;
		po.binary_bits = bits;
		po.twos_complement = true;
		po.min_exp = 0;
		string str = nr.print(po);
		if(str.length() > bits) str = str.substr(str.length() - bits, bits);
		ParseOptions pa;
		pa.base = BASE_BINARY;
		pa.twos_complement = true;
		pa.binary_bits = bits;
		nr.set(str, pa);
	}
	mstruct = nr;
	return 1;
}

SetBitsFunction::SetBitsFunction() : MathFunction("setbits", 4, 6) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_ULONG));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_ULONG));
	setArgumentDefinition(4, new IntegerArgument());
	setArgumentDefinition(5, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setDefaultValue(5, "0");
	setArgumentDefinition(6, new BooleanArgument());
	setDefaultValue(6, "0");
}
int SetBitsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	unsigned long int first_pos = vargs[1].number().ulintValue();
	unsigned long int last_pos = vargs[2].number().ulintValue();
	unsigned int bits = vargs[4].number().uintValue();
	bool b_signed = vargs[5].number().getBoolean();
	unsigned long int max_index = last_pos;
	unsigned long int n = 0;
	if(last_pos < first_pos) {
		max_index = last_pos;
		while(first_pos - n >= last_pos) {
			if(CALCULATOR->aborted()) return 0;
			nr.bitSet(first_pos - n, vargs[3].number().bitGet(n + 1));
			n++;
		}
	} else {
		while(first_pos + n <= last_pos) {
			if(CALCULATOR->aborted()) return 0;
			nr.bitSet(first_pos + n, vargs[3].number().bitGet(n + 1));
			n++;
		}
	}
	if(bits > 0 && max_index > bits) {
		Number nrbits = max_index;
		nrbits.log(nr_two);
		nrbits.ceil();
		nrbits.exp2();
		bits = nrbits.uintValue();
	}
	if(bits > 0 && max_index == bits && (b_signed || vargs[0].number().isNegative()) && (vargs[3].number().bitGet(last_pos - first_pos) != vargs[0].number().isNegative())) {
		PrintOptions po;
		po.base = BASE_BINARY;
		po.base_display = BASE_DISPLAY_NONE;
		po.binary_bits = bits;
		po.twos_complement = true;
		po.min_exp = 0;
		string str = nr.print(po);
		if(str.length() > bits) str = str.substr(str.length() - bits, bits);
		ParseOptions pa;
		pa.base = BASE_BINARY;
		pa.twos_complement = true;
		pa.binary_bits = bits;
		nr.set(str, pa);
	}
	mstruct = nr;
	return 1;
}

BitGetFunction::BitGetFunction() : MathFunction("bitget", 2, 3) {
	setArgumentDefinition(1, new IntegerArgument());
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_ULONG));
	setArgumentDefinition(3, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_ULONG));
	setDefaultValue(3, "0");
}
int BitGetFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	unsigned long int first_pos = vargs[1].number().ulintValue();
	unsigned long int last_pos = vargs[2].number().ulintValue();
	if(last_pos == 0 || last_pos == first_pos) {
		mstruct.set(nr.bitGet(first_pos), 1, 0);
	} else {
		Number nr_new;
		Number pos_value(1, 1, 0);
		if(last_pos < first_pos) {
			while(first_pos >= last_pos) {
				if(CALCULATOR->aborted()) return 0;
				if(nr.bitGet(first_pos)) nr_new += pos_value;
				first_pos--;
				pos_value *= 2;
			}
		} else {
			while(first_pos <= last_pos) {
				if(CALCULATOR->aborted()) return 0;
				if(nr.bitGet(first_pos)) nr_new += pos_value;
				first_pos++;
				pos_value *= 2;
			}
		}
		mstruct = nr_new;
	}
	return 1;
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
		if(mstruct.isMatrix() && mstruct.columns() == 1 && mstruct.rows() > 1) mstruct.transposeMatrix();
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].eval(eo);
			if(!mstruct[i].isNumber() && vargs[3].isZero()) {
				return -1;
			}
			int result = mstruct[i].number().getBoolean();
			if(result > 0) {
				MathStructure m2(vargs[1]);
				if(m2.isMatrix() && m2.columns() == 1 && m2.rows() > 1) m2.transposeMatrix();
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
bool calculate_userfunctions2(MathStructure &m, const MathStructure &x_mstruct, const MathStructure &x_mstruct2, const EvaluationOptions &eo, size_t depth = 1) {
	if(!check_recursive_function_depth(depth)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(calculate_userfunctions2(m[i], x_mstruct, x_mstruct2, eo, depth + 1)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(m.isFunction()) {
		if(!m.contains(x_mstruct, true) && !m.contains(x_mstruct2, true) && !m.containsFunctionId(FUNCTION_ID_RAND, true, true, true) && !m.containsFunctionId(FUNCTION_ID_RANDN, true, true, true) && !m.containsFunctionId(FUNCTION_ID_RAND_POISSON, true, true, true)) {
			if(m.calculateFunctions(eo, false)) {
				b_ret = true;
				calculate_userfunctions2(m, x_mstruct, x_mstruct2, eo, depth + 1);
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
				calculate_userfunctions2(m, x_mstruct, x_mstruct2, eo, depth + 1);
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
	if(!check_recursive_depth(mstruct)) return 0;
	return 1;

}

ForEachFunction::ForEachFunction() : MathFunction("foreach", 3, 5) {
	setArgumentDefinition(1, new MatrixArgument());
	setArgumentDefinition(4, new SymbolicArgument());
	setArgumentDefinition(5, new SymbolicArgument());
	setDefaultValue(4, "y");
	setDefaultValue(5, "x");
}
int ForEachFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vector<Variable*> vars;
	mstruct = vargs[1];
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
	MathStructure m5(vargs[2]);
	if(vargs[2].isComparison() && vargs[2].comparisonType() == COMPARISON_EQUALS && vargs[2][0] == vargs[3]) m5 = vargs[2][1];
	else m5 = vargs[2];
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
	if(calculate_userfunctions2(m5, vargs[3], vargs[4], eo)) {
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
	MathStructure mupdate;
	for(size_t i = 0; i < vargs[0].size(); i++) {
		for(size_t i2 = 0; i2 < vargs[0][i].size(); i2++) {
			if(CALCULATOR->aborted()) {
				for(size_t i = 0; i < vars.size(); i++) vars[i]->destroy();
				return 0;
			}
			mupdate = m5;
			calculate_replace2(mupdate, vargs[4], vargs[0][i][i2], vargs[3], mstruct, eo);
			mstruct = mupdate;
		}
	}
	for(size_t i = 0; i < vars.size(); i++) {
		if(vars[i]->isKnown()) mstruct.replace(vars[i], ((KnownVariable*) vars[i])->get());
		else mstruct.replace(vars[i], ((UnknownVariable*) vars[i])->interval());
		vars[i]->destroy();
	}
	if(!check_recursive_depth(mstruct)) return 0;
	return 1;

}
