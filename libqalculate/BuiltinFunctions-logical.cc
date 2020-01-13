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
	int b0, b1;
	if(vargs[0].representsNonPositive(true)) {
		b0 = 0;
	} else if(vargs[0].representsPositive(true)) {
		b0 = 1;
	} else {
		b0 = -1;
	}
	if(vargs[1].representsNonPositive(true)) {
		b1 = 0;
	} else if(vargs[1].representsPositive(true)) {
		b1 = 1;
	} else {
		b1 = -1;
	}
	if((b0 == 1 && b1 == 0) || (b0 == 0 && b1 == 1)) {
		mstruct = m_one;
		return 1;
	} else if(b0 >= 0 && b1 >= 0) {
		return 1;
	} else if(b0 == 0) {
		mstruct = vargs[1];
		mstruct.add(m_zero, OPERATION_GREATER);
		return 1;
	} else if(b0 == 1) {
		mstruct = vargs[1];
		mstruct.add(m_zero, OPERATION_EQUALS_LESS);
		return 1;
	} else if(b1 == 0) {
		mstruct = vargs[0];
		mstruct.add(m_zero, OPERATION_GREATER);
		return 1;
	} else if(b1 == 1) {
		mstruct = vargs[0];
		mstruct.add(m_zero, OPERATION_EQUALS_LESS);
		return 1;
	}
	mstruct = vargs[1];
	mstruct.setLogicalNot();
	mstruct.add(vargs[0], OPERATION_LOGICAL_AND);
	MathStructure mstruct2(vargs[0]);
	mstruct2.setLogicalNot();
	mstruct2.add(vargs[1], OPERATION_LOGICAL_AND);
	mstruct.add(mstruct2, OPERATION_LOGICAL_OR);
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
	mstruct.eval(eo);
	if(mstruct.isVector()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].isNumber() && vargs[3].isZero()) {
				return -1;
			}
			int result = mstruct[i].number().getBoolean();
			if(result > 0) {
				if(vargs[1].isVector() && vargs[1].size() == vargs[0].size()) {
					mstruct = vargs[1][i];
				} else {
					mstruct = vargs[1];
				}
				return 1;
			} else if(result < 0 && vargs[3].isZero()) {
				return -1;
			}
		}
		mstruct = vargs[2];
		return 1;
	}
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
ForFunction::ForFunction() : MathFunction("for", 7) {
	setArgumentDefinition(2, new SymbolicArgument());
	setArgumentDefinition(7, new SymbolicArgument());
}
int ForFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct = vargs[4];
	MathStructure mcounter = vargs[0];
	MathStructure mtest;
	MathStructure mcount;
	MathStructure mupdate;
	while(true) {
		mtest = vargs[2];
		mtest.replace(vargs[1], mcounter);
		mtest.eval(eo);
		if(!mtest.isNumber() || CALCULATOR->aborted()) return 0;
		if(!mtest.number().getBoolean()) {
			break;
		}
		if(vargs[5].isComparison() && vargs[5].comparisonType() == COMPARISON_EQUALS && vargs[5][0] == vargs[6]) mupdate = vargs[5][1];
		else mupdate = vargs[5];
		mupdate.replace(vargs[1], mcounter, vargs[6], mstruct);
		mstruct = mupdate;
		mstruct.calculatesub(eo, eo, false);

		if(vargs[3].isComparison() && vargs[3].comparisonType() == COMPARISON_EQUALS && vargs[3][0] == vargs[1]) mcount = vargs[3][1];
		else mcount = vargs[3];
		mcount.replace(vargs[1], mcounter);
		mcounter = mcount;
		mcounter.calculatesub(eo, eo, false);
	}
	return 1;

}

