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
#include "MathStructure-support.h"

#include <sstream>
#include <time.h>
#include <limits>
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
using std::endl;

#define FR_FUNCTION(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC(vargs[1].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}

#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(i)		NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);
#define RATIONAL_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_rational##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_rational##i->setRationalNumber(true); setArgumentDefinition(i, arg_rational##i);
#define RATIONAL_POLYNOMIAL_ARGUMENT(i)				Argument *arg_poly##i = new Argument(); arg_poly##i->setRationalPolynomial(true); setArgumentDefinition(i, arg_poly##i);
#define RATIONAL_POLYNOMIAL_ARGUMENT_HV(i)			Argument *arg_poly##i = new Argument(); arg_poly##i->setRationalPolynomial(true); arg_poly##i->setHandleVector(true); setArgumentDefinition(i, arg_poly##i);

OddFunction::OddFunction() : MathFunction("odd", 1) {
	Argument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int OddFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].representsOdd()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(vargs[0].representsEven()) {
		mstruct.clear();
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.representsOdd()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(mstruct.representsEven()) {
		mstruct.clear();
		return 1;
	}
	return -1;
}
EvenFunction::EvenFunction() : MathFunction("even", 1) {
	Argument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int EvenFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].representsEven()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(vargs[0].representsOdd()) {
		mstruct.clear();
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.representsEven()) {
		mstruct.set(1, 1, 0);
		return 1;
	} else if(mstruct.representsOdd()) {
		mstruct.clear();
		return 1;
	}
	return -1;
}
AbsFunction::AbsFunction() : MathFunction("abs", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool AbsFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool AbsFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsInteger(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsInteger(allow_units);}
bool AbsFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsRational(allow_units);}
bool AbsFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AbsFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool AbsFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool AbsFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool AbsFunction::representsEven(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsEven(allow_units);}
bool AbsFunction::representsOdd(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsOdd(allow_units);}
bool AbsFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 1 && vargs[0].representsUndefined();}
int AbsFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		if(eo.approximation != APPROXIMATION_APPROXIMATE && mstruct.number().hasImaginaryPart() && mstruct.number().hasRealPart()) {
			MathStructure m_i(mstruct.number().imaginaryPart());
			m_i ^= nr_two;
			mstruct.number().clearImaginary();
			mstruct.numberUpdated();
			mstruct ^= nr_two;
			mstruct += m_i;
			mstruct ^= nr_half;
			return 1;
		}
		Number nr(mstruct.number());
		if(!nr.abs() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate())) {
			return -1;
		}
		mstruct = nr;
		return 1;
	}
	if(mstruct.isPower() && mstruct[0].representsPositive()) {
		if(mstruct[1].isNumber() && !mstruct[1].number().hasRealPart()) {
			mstruct.set(1, 1, 0, true);
			return 1;
		} else if(mstruct[1].isMultiplication() && mstruct.size() > 0 && mstruct[1][0].isNumber() && !mstruct[1][0].number().hasRealPart()) {
			bool b = true;
			for(size_t i = 1; i < mstruct[1].size(); i++) {
				if(!mstruct[1][i].representsNonComplex()) {b = false; break;}
			}
			if(b) {
				mstruct.set(1, 1, 0, true);
				return 1;
			}
		}
	}
	if(mstruct.representsNegative(true)) {
		mstruct.negate();
		return 1;
	}
	if(mstruct.representsNonNegative(true)) {
		return 1;
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].transform(STRUCT_FUNCTION);
			mstruct[i].setFunction(this);
		}
		mstruct.childrenUpdated();
		return 1;
	}
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2) {
		mstruct[0].transform(this);
		mstruct.childUpdated(1);
		return 1;
	}
	if(mstruct.isPower() && mstruct[1].representsReal()) {
		mstruct[0].transform(this);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT || has_interval_unknowns(mstruct)) {
		ComparisonResult cr = mstruct.compare(m_zero);
		if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
			return 1;
		} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
			mstruct.negate();
			return 1;
		}
	}
	return -1;
}
GcdFunction::GcdFunction() : MathFunction("gcd", 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(1)
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(2)
}
int GcdFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(MathStructure::gcd(vargs[0], vargs[1], mstruct, eo)) {
		return 1;
	}
	return 0;
}
LcmFunction::LcmFunction() : MathFunction("lcm", 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(1)
	RATIONAL_POLYNOMIAL_ARGUMENT_HV(2)
}
int LcmFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(MathStructure::lcm(vargs[0], vargs[1], mstruct, eo)) {
		return 1;
	}
	return 0;
}

SignumFunction::SignumFunction() : MathFunction("sgn", 1, 2) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	setDefaultValue(2, "0");
}
bool SignumFunction::representsPositive(const MathStructure&, bool allow_units) const {return false;}
bool SignumFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool SignumFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNonNegative(true);}
bool SignumFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNonPositive(true);}
bool SignumFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNumber(true);}
bool SignumFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsNonComplex(true);}
bool SignumFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsReal(true);}
bool SignumFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() >= 1 && vargs[0].representsComplex(true);}
bool SignumFunction::representsNonZero(const MathStructure &vargs, bool) const {return (vargs.size() == 2 && !vargs[1].isZero()) || (vargs.size() >= 1 && vargs[0].representsNonZero(true));}
bool SignumFunction::representsEven(const MathStructure&, bool) const {return false;}
bool SignumFunction::representsOdd(const MathStructure &vargs, bool b) const {return representsNonZero(vargs, b);}
bool SignumFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() >= 1 && vargs[0].representsUndefined();}
int SignumFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber() && (vargs.size() == 1 || vargs[1].isZero())) {
		Number nr(mstruct.number());
		if(!nr.signum() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate())) {
			if(mstruct.number().isNonZero()) {
				MathStructure *mabs = new MathStructure(mstruct);
				mabs->transformById(FUNCTION_ID_ABS);
				mstruct.divide_nocopy(mabs);
				return 1;
			}
			return -1;
		} else {
			mstruct = nr;
			return 1;
		}
	}
	if((vargs.size() > 1 && vargs[1].isOne() && mstruct.representsNonNegative(true)) || mstruct.representsPositive(true)) {
		mstruct.set(1, 1, 0);
		return 1;
	}
	if((vargs.size() > 1 && vargs[1].isMinusOne() && mstruct.representsNonPositive(true)) || mstruct.representsNegative(true)) {
		mstruct.set(-1, 1, 0);
		return 1;
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(vargs.size() > 1) mstruct[i].transform(STRUCT_FUNCTION, vargs[1]);
			else mstruct[i].transform(STRUCT_FUNCTION);
			mstruct[i].setFunction(this);

		}
		mstruct.childrenUpdated();
		return 1;
	}
	if(vargs.size() > 1 && mstruct.isZero()) {
		mstruct.set(vargs[1], true);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT || has_interval_unknowns(mstruct)) {
		ComparisonResult cr = mstruct.compare(m_zero);
		if(cr == COMPARISON_RESULT_LESS || (vargs.size() > 1 && vargs[1].isOne() && COMPARISON_IS_EQUAL_OR_LESS(cr))) {
			mstruct.set(1, 1, 0);
			return 1;
		} else if(cr == COMPARISON_RESULT_GREATER || (vargs.size() > 1 && vargs[1].isMinusOne() && COMPARISON_IS_EQUAL_OR_GREATER(cr))) {
			mstruct.set(-1, 1, 0);
			return 1;
		}
	}
	return -1;
}

CeilFunction::CeilFunction() : MathFunction("ceil", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int CeilFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(ceil)
}
bool CeilFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool CeilFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool CeilFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool CeilFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool CeilFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool CeilFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CeilFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool CeilFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool CeilFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool CeilFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool CeilFunction::representsUndefined(const MathStructure&) const {return false;}

FloorFunction::FloorFunction() : MathFunction("floor", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int FloorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(floor)
}
bool FloorFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool FloorFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool FloorFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool FloorFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool FloorFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool FloorFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool FloorFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool FloorFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool FloorFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool FloorFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool FloorFunction::representsUndefined(const MathStructure&) const {return false;}

TruncFunction::TruncFunction() : MathFunction("trunc", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int TruncFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(trunc)
}
bool TruncFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool TruncFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool TruncFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TruncFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool TruncFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool TruncFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool TruncFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool TruncFunction::representsUndefined(const MathStructure&) const {return false;}

RoundFunction::RoundFunction() : MathFunction("round", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int RoundFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(round)
}
bool RoundFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool RoundFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonPositive();}
bool RoundFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool RoundFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool RoundFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool RoundFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsEven();}
bool RoundFunction::representsOdd(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsInteger() && vargs[0].representsOdd();}
bool RoundFunction::representsUndefined(const MathStructure&) const {return false;}

FracFunction::FracFunction() : MathFunction("frac", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int FracFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(frac)
}
IntFunction::IntFunction() : MathFunction("int", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
}
int IntFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(trunc)
}
NumeratorFunction::NumeratorFunction() : MathFunction("numerator", 1) {
	NumberArgument *arg_rational_1 = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg_rational_1->setRationalNumber(true);
	arg_rational_1->setHandleVector(true);
	setArgumentDefinition(1, arg_rational_1);
}
int NumeratorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].isNumber()) {
		if(vargs[0].number().isInteger()) {
			mstruct = vargs[0];
			return 1;
		} else if(vargs[0].number().isRational()) {
			mstruct.set(vargs[0].number().numerator());
			return 1;
		}
		return 0;
	} else if(vargs[0].representsInteger()) {
		mstruct = vargs[0];
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.representsInteger()) {
		return 1;
	} else if(mstruct.isNumber() && mstruct.number().isRational()) {
		Number nr(mstruct.number().numerator());
		mstruct.set(nr);
		return 1;
	}
	return -1;
}
DenominatorFunction::DenominatorFunction() : MathFunction("denominator", 1) {
	RATIONAL_NUMBER_ARGUMENT_NO_ERROR(1)
}
int DenominatorFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.set(vargs[0].number().denominator());
	return 1;
}
RemFunction::RemFunction() : MathFunction("rem", 2) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(2)
}
int RemFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(rem)
}
ModFunction::ModFunction() : MathFunction("mod", 2) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1)
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR_NONZERO(2)
}
int ModFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(mod)
}

BernoulliFunction::BernoulliFunction() : MathFunction("bernoulli", 1, 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
	setDefaultValue(2, "0");
}
int BernoulliFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs.size() > 1 && !vargs[1].isZero()) {
		MathStructure m2(vargs[1]);
		replace_f_interval(m2, eo);
		replace_intervals_f(m2);
		mstruct.clear();
		Number bin, k, nmk(vargs[0].number()), nrB;
		while(k <= vargs[0].number()) {
			if(nmk.isEven() || nmk.isOne()) {
				nrB.set(nmk);
				if(!bin.binomial(vargs[0].number(), k) || !nrB.bernoulli() || !nrB.multiply(bin)) return 0;
				if(eo.approximation == APPROXIMATION_EXACT && nrB.isApproximate()) return 0;
				mstruct.add(nrB, true);
				mstruct.last().multiply(m2);
				mstruct.last().last().raise(k);
				mstruct.childUpdated(mstruct.size());
			}
			nmk--;
			k++;
		}
		if(mstruct.isAddition()) mstruct.delChild(1, true);
		return 1;
	}
	FR_FUNCTION(bernoulli)
}

PolynomialUnitFunction::PolynomialUnitFunction() : MathFunction("punit", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int PolynomialUnitFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct.set(vargs[0].polynomialUnit(vargs[1]), 0);
	return 1;
}
PolynomialPrimpartFunction::PolynomialPrimpartFunction() : MathFunction("primpart", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int PolynomialPrimpartFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].polynomialPrimpart(vargs[1], mstruct, eo);
	return 1;
}
PolynomialContentFunction::PolynomialContentFunction() : MathFunction("pcontent", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int PolynomialContentFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	vargs[0].polynomialContent(vargs[1], mstruct, eo);
	return 1;
}
CoeffFunction::CoeffFunction() : MathFunction("coeff", 2, 3) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONNEGATIVE));
	setArgumentDefinition(3, new SymbolicArgument());
	setDefaultValue(3, "undefined");
}
int CoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].coefficient(vargs[2], vargs[1].number(), mstruct);
	return 1;
}
LCoeffFunction::LCoeffFunction() : MathFunction("lcoeff", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int LCoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].lcoefficient(vargs[1], mstruct);
	return 1;
}
TCoeffFunction::TCoeffFunction() : MathFunction("tcoeff", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int TCoeffFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	vargs[0].tcoefficient(vargs[1], mstruct);
	return 1;
}
DegreeFunction::DegreeFunction() : MathFunction("degree", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int DegreeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].degree(vargs[1]);
	return 1;
}
LDegreeFunction::LDegreeFunction() : MathFunction("ldegree", 1, 2) {
	RATIONAL_POLYNOMIAL_ARGUMENT(1)
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
int LDegreeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0].ldegree(vargs[1]);
	return 1;
}


BinFunction::BinFunction() : MathFunction("bin", 1, 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "0");
}
int BinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_BINARY;
	po.twos_complement = vargs[1].number().getBoolean();
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
OctFunction::OctFunction() : MathFunction("oct", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int OctFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_OCTAL;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
DecFunction::DecFunction() : MathFunction("dec", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int DecFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_DECIMAL;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
HexFunction::HexFunction() : MathFunction("hex", 1, 2) {
	setArgumentDefinition(1, new TextArgument());
	setArgumentDefinition(2, new BooleanArgument());
	setDefaultValue(2, "0");
}
int HexFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	ParseOptions po = eo.parse_options;
	po.base = BASE_HEXADECIMAL;
	po.hexadecimal_twos_complement = vargs[1].number().getBoolean();
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}

BaseFunction::BaseFunction() : MathFunction("base", 2, 3) {
	setArgumentDefinition(1, new TextArgument());
	Argument *arg = new Argument();
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
	IntegerArgument *arg2 = new IntegerArgument();
	arg2->setMin(&nr_zero);
	arg2->setMax(&nr_three);
	setArgumentDefinition(3, arg2);
	setArgumentDefinition(3, new TextArgument());
	setDefaultValue(3, "0");
}
int BaseFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[1].isVector()) return 0;
	Number nbase;
	int idigits = 0;
	string sdigits;
	if(vargs.size() > 2) sdigits = vargs[2].symbol();
	if(sdigits.empty() || sdigits == "0" || sdigits == "auto") idigits = 0;
	else if(sdigits == "1") idigits = 1;
	else if(sdigits == "2") idigits = 2;
	else if(sdigits == "3" || sdigits == "Unicode" || sdigits == "unicode" || sdigits == "escaped") idigits = 3;
	else idigits = -1;
	if(vargs[1].isNumber() && idigits == 0) {
		nbase = vargs[1].number();
	} else {
		mstruct = vargs[1];
		mstruct.eval(eo);
		if(mstruct.isVector()) return -2;
		if(idigits == 0 && !mstruct.isNumber() && eo.approximation == APPROXIMATION_EXACT) {
			MathStructure mstruct2(mstruct);
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_TRY_EXACT;
			CALCULATOR->beginTemporaryStopMessages();
			mstruct2.eval(eo2);
			if(mstruct2.isVector() || mstruct2.isNumber()) {
				mstruct = mstruct2;
				CALCULATOR->endTemporaryStopMessages(true);
				if(mstruct.isVector()) return -2;
			} else {
				CALCULATOR->endTemporaryStopMessages();
			}
		}
		if(mstruct.isNumber() && idigits == 0) {
			nbase = mstruct.number();
		} else {
			string number = vargs[0].symbol();
			size_t i_dot = number.length();
			vector<Number> digits;
			bool b_minus = false;
			if(idigits < 0) {
				unordered_map<string, long int> vdigits;
				string schar;
				long int v = 0;
				for(size_t i = 0; i < sdigits.length(); v++) {
					size_t l = 1;
					while(i + l < sdigits.length() && sdigits[i + l] <= 0 && (unsigned char) sdigits[i + l] < 0xC0) l++;
					vdigits[sdigits.substr(i, l)] = v;
					i += l;
				}
				remove_blanks(number);
				i_dot = number.length();
				for(size_t i = 0; i < number.length();) {
					size_t l = 1;
					while(i + l < number.length() && number[i + l] <= 0 && (unsigned char) number[i + l] < 0xC0) l++;
					unordered_map<string, long int>::iterator it = vdigits.find(number.substr(i, l));
					if(it == vdigits.end()) {
						if(l == 1 && (number[i] == CALCULATOR->getDecimalPoint()[0] || (!eo.parse_options.dot_as_separator && number[i] == '.'))) {
							if(i_dot == number.length()) i_dot = digits.size();
						} else {
							CALCULATOR->error(true, _("Character \'%s\' was ignored in the number \"%s\" with base %s."), number.substr(i, l).c_str(), number.c_str(), format_and_print(mstruct).c_str(), NULL);
						}
					} else {
						digits.push_back(it->second);
					}
					i += l;
				}
			} else if(idigits <= 2) {
				remove_blanks(number);
				bool b_case = (idigits == 2);
				i_dot = number.length();
				for(size_t i = 0; i < number.length(); i++) {
					long int c = -1;
					if(number[i] >= '0' && number[i] <= '9') {
						c = number[i] - '0';
					} else if(number[i] >= 'a' && number[i] <= 'z') {
						if(b_case) c = number[i] - 'a' + 36;
						else c = number[i] - 'a' + 10;
					} else if(number[i] >= 'A' && number[i] <= 'Z') {
						c = number[i] - 'A' + 10;
					} else if(number[i] == CALCULATOR->getDecimalPoint()[0] || (!eo.parse_options.dot_as_separator && number[i] == '.')) {
						if(i_dot == number.length()) i_dot = digits.size();
					} else if(number[i] == '-' && digits.empty()) {
						b_minus = !b_minus;
					} else {
						string str_char = number.substr(i, 1);
						while(i + 1 < number.length() && number[i + 1] < 0 && number[i + 1] && (unsigned char) number[i + 1] < 0xC0) {
							i++;
							str_char += number[i];
						}
						CALCULATOR->error(true, _("Character \'%s\' was ignored in the number \"%s\" with base %s."), str_char.c_str(), number.c_str(), format_and_print(mstruct).c_str(), NULL);
					}
					if(c >= 0) {
						digits.push_back(c);
					}
				}
			} else {
				for(size_t i = 0; i < number.length(); i++) {
					long int c = (unsigned char) number[i];
					bool b_esc = false;
					if(number[i] == '\\' && i < number.length() - 1) {
						i++;
						Number nrd;
						if(is_in(NUMBERS, number[i])) {
							size_t i2 = number.find_first_not_of(NUMBERS, i);
							if(i2 == string::npos) i2 = number.length();
							nrd.set(number.substr(i, i2 - i));
							i = i2 - 1;
							b_esc = true;
						} else if(number[i] == 'x' && i < number.length() - 1 && is_in(NUMBERS "ABCDEFabcdef", number[i + 1])) {
							i++;
							size_t i2 = number.find_first_not_of(NUMBERS "ABCDEFabcdef", i);
							if(i2 == string::npos) i2 = number.length();
							ParseOptions po;
							po.base = BASE_HEXADECIMAL;
							nrd.set(number.substr(i, i2 - i), po);
							i = i2 - 1;
							b_esc = true;
						}
						if(digits.empty() && number[i] == (char) -30 && i + 3 < number.length() && number[i + 1] == (char) -120 && number[i + 2] == (char) -110) {
							i += 2;
							b_minus = !b_minus;
							b_esc = true;
						} else if(digits.empty() && number[i] == '-') {
							b_minus = !b_minus;
							b_esc = true;
						} else if(i_dot == number.size() && (number[i] == CALCULATOR->getDecimalPoint()[0] || (!eo.parse_options.dot_as_separator && number[i] == '.'))) {
							i_dot = digits.size();
							b_esc = true;
						} else if(b_esc) {
							digits.push_back(nrd);

						} else if(number[i] != '\\') {
							i--;
						}
					}
					if(!b_esc) {
						if((c & 0x80) != 0) {
							if(c<0xe0) {
								i++;
								if(i >= number.length()) return -2;
								c = ((c & 0x1f) << 6) | (((unsigned char) number[i]) & 0x3f);
							} else if(c<0xf0) {
								i++;
								if(i + 1 >= number.length()) return -2;
								c = (((c & 0xf) << 12) | ((((unsigned char) number[i]) & 0x3f) << 6)|(((unsigned char) number[i + 1]) & 0x3f));
								i++;
							} else {
								i++;
								if(i + 2 >= number.length()) return -2;
								c = ((c & 7) << 18) | ((((unsigned char) number[i]) & 0x3f) << 12) | ((((unsigned char) number[i + 1]) & 0x3f) << 6) | (((unsigned char) number[i + 2]) & 0x3f);
								i += 2;
							}
						}
						digits.push_back(c);
					}
				}
			}
			MathStructure mbase = mstruct;
			mstruct.clear();
			if(i_dot > digits.size()) i_dot = digits.size();
			for(size_t i = 0; i < digits.size(); i++) {
				long int exp = i_dot - 1 - i;
				MathStructure m;
				if(exp != 0) {
					m = mbase;
					m.raise(Number(exp, 1));
					m.multiply(digits[i]);
				} else {
					m.set(digits[i]);
				}
				if(mstruct.isZero()) mstruct = m;
				else mstruct.add(m, true);
			}
			if(b_minus) mstruct.negate();
			return 1;
		}
	}
	ParseOptions po = eo.parse_options;
	if(nbase.isInteger() && nbase >= 2 && nbase <= 36) {
		po.base = nbase.intValue();
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	} else {
		po.base = BASE_CUSTOM;
		Number cb_save = CALCULATOR->customInputBase();
		CALCULATOR->setCustomInputBase(nbase);
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
		CALCULATOR->setCustomInputBase(cb_save);
	}
	return 1;
}
RomanFunction::RomanFunction() : MathFunction("roman", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int RomanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].symbol().find_first_not_of("0123456789.:" SIGNS) == string::npos && vargs[0].symbol().find_first_not_of("0" SIGNS) != string::npos) {
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), eo.parse_options);
		PrintOptions po; po.base = BASE_ROMAN_NUMERALS;
		mstruct.eval(eo);
		mstruct.set(mstruct.print(po), true, true);
		return 1;
	}
	ParseOptions po = eo.parse_options;
	po.base = BASE_ROMAN_NUMERALS;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
BijectiveFunction::BijectiveFunction() : MathFunction("bijective", 1) {
	setArgumentDefinition(1, new TextArgument());
}
int BijectiveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].symbol().find_first_not_of("0123456789.:" SIGNS) == string::npos && vargs[0].symbol().find_first_not_of(SIGNS) != string::npos) {
		CALCULATOR->parse(&mstruct, vargs[0].symbol(), eo.parse_options);
		PrintOptions po; po.base = BASE_BIJECTIVE_26;
		mstruct.eval(eo);
		mstruct.set(mstruct.print(po), true, true);
		return 1;
	}
	ParseOptions po = eo.parse_options;
	po.base = BASE_BIJECTIVE_26;
	CALCULATOR->parse(&mstruct, vargs[0].symbol(), po);
	return 1;
}
ImFunction::ImFunction() : MathFunction("im", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int ImFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		mstruct = mstruct.number().imaginaryPart();
		return 1;
	} else if(mstruct.representsReal(!eo.keep_zero_units)) {
		mstruct.clear(true);
		return 1;
	} else if(mstruct.isUnit_exp()) {
		mstruct *= m_zero;
		mstruct.swapChildren(1, 2);
		return 1;
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo() && mstruct[0].representsNegative()) {
		mstruct[0].negate();
		Number num = mstruct[1].number().numerator();
		num.rem(4);
		if(num == 3 || num == -1) mstruct.negate();
		return 1;
	} else if(mstruct.isMultiplication() && mstruct.size() > 0) {
		if(mstruct[0].isNumber()) {
			Number nr = mstruct[0].number();
			mstruct.delChild(1, true);
			if(nr.hasImaginaryPart()) {
				if(nr.hasRealPart()) {
					MathStructure *madd = new MathStructure(mstruct);
					mstruct.transformById(FUNCTION_ID_RE);
					madd->transform(this);
					madd->multiply(nr.realPart());
					mstruct.multiply(nr.imaginaryPart());
					mstruct.add_nocopy(madd);
					return 1;
				}
				mstruct.transformById(FUNCTION_ID_RE);
				mstruct.multiply(nr.imaginaryPart());
				return 1;
			}
			mstruct.transform(this);
			mstruct.multiply(nr.realPart());
			return 1;
		}
		MathStructure *mreal = NULL;
		for(size_t i = 0; i < mstruct.size();) {
			if(mstruct[i].representsReal(true)) {
				if(!mreal) {
					mreal = new MathStructure(mstruct[i]);
				} else {
					mstruct[i].ref();
					if(!mreal->isMultiplication()) mreal->transform(STRUCT_MULTIPLICATION);
					mreal->addChild_nocopy(&mstruct[i]);
				}
				mstruct.delChild(i + 1);
			} else {
				i++;
			}
		}
		if(mreal) {
			if(mstruct.size() == 0) mstruct.clear(true);
			else if(mstruct.size() == 1) mstruct.setToChild(1, true);
			mstruct.transform(this);
			mstruct.multiply_nocopy(mreal);
			return 1;
		}
	}
	return -1;
}
bool ImFunction::representsPositive(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ImFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ImFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool ImFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool ImFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool ImFunction::representsEven(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ImFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool ImFunction::representsUndefined(const MathStructure&) const {return false;}

ReFunction::ReFunction() : MathFunction("re", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int ReFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		mstruct = mstruct.number().realPart();
		return 1;
	} else if(mstruct.representsReal(true)) {
		return 1;
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo() && mstruct[0].representsNegative()) {
		mstruct.clear(true);
		return 1;
	} else if(mstruct.isMultiplication() && mstruct.size() > 0) {
		if(mstruct[0].isNumber()) {
			Number nr = mstruct[0].number();
			mstruct.delChild(1, true);
			if(nr.hasImaginaryPart()) {
				if(nr.hasRealPart()) {
					MathStructure *madd = new MathStructure(mstruct);
					mstruct.transformById(FUNCTION_ID_IM);
					madd->transform(this);
					madd->multiply(nr.realPart());
					mstruct.multiply(-nr.imaginaryPart());
					mstruct.add_nocopy(madd);
					return 1;
				}
				mstruct.transformById(FUNCTION_ID_IM);
				mstruct.multiply(-nr.imaginaryPart());
				return 1;
			}
			mstruct.transform(this);
			mstruct.multiply(nr.realPart());
			return 1;
		}
		MathStructure *mreal = NULL;
		for(size_t i = 0; i < mstruct.size();) {
			if(mstruct[i].representsReal(true)) {
				if(!mreal) {
					mreal = new MathStructure(mstruct[i]);
				} else {
					mstruct[i].ref();
					if(!mreal->isMultiplication()) mreal->transform(STRUCT_MULTIPLICATION);
					mreal->addChild_nocopy(&mstruct[i]);
				}
				mstruct.delChild(i + 1);
			} else {
				i++;
			}
		}
		if(mreal) {
			if(mstruct.size() == 0) mstruct.clear(true);
			else if(mstruct.size() == 1) mstruct.setToChild(1, true);
			mstruct.transform(this);
			mstruct.multiply_nocopy(mreal);
			return 1;
		}
	}
	return -1;
}
bool ReFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsPositive(allow_units);}
bool ReFunction::representsNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNegative(allow_units);}
bool ReFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonNegative(allow_units);}
bool ReFunction::representsNonPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonPositive(allow_units);}
bool ReFunction::representsInteger(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsInteger(allow_units);}
bool ReFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool ReFunction::representsRational(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsRational(allow_units);}
bool ReFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool ReFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
bool ReFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool ReFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsReal(allow_units) && vargs[0].representsNonZero(true);}
bool ReFunction::representsEven(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsEven(allow_units);}
bool ReFunction::representsOdd(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsOdd(allow_units);}
bool ReFunction::representsUndefined(const MathStructure&) const {return false;}

ArgFunction::ArgFunction() : MathFunction("arg", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool ArgFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
bool ArgFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber(true);}
bool ArgFunction::representsNonComplex(const MathStructure &vargs, bool) const {return true;}
int ArgFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;

	MathStructure msave;

	arg_test_non_number:
	if(!mstruct.isNumber()) {
		if(mstruct.representsPositive(true)) {
			mstruct.clear();
			return 1;
		}
		if(mstruct.representsNegative(true)) {
			switch(eo.parse_options.angle_unit) {
				case ANGLE_UNIT_DEGREES: {mstruct.set(180, 1, 0); break;}
				case ANGLE_UNIT_GRADIANS: {mstruct.set(200, 1, 0); break;}
				case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI)); break;}
				default: {mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI)); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
			}
			return 1;
		}
		if(!msave.isZero()) {
			mstruct = msave;
			return -1;
		}
		if(mstruct.isMultiplication()) {
			bool b = false;
			for(size_t i = 0; i < mstruct.size();) {
				if(mstruct[i].representsPositive()) {
					mstruct.delChild(i + 1);
					b = true;
				} else {
					if(!mstruct[i].isMinusOne() && mstruct[i].representsNegative()) {
						mstruct[i].set(-1, 1, 0, true);
						b = true;
					}
					i++;
				}
			}
			if(b) {
				if(mstruct.size() == 1) {
					mstruct.setToChild(1);
				} else if(mstruct.size() == 0) {
					mstruct.clear(true);
				}
				mstruct.transform(STRUCT_FUNCTION);
				mstruct.setFunction(this);
				return 1;
			}
		}
		if(mstruct.isPower() && mstruct[0].representsComplex() && mstruct[1].representsInteger()) {
			mstruct.setType(STRUCT_MULTIPLICATION);
			mstruct[0].transform(STRUCT_FUNCTION);
			mstruct[0].setFunction(this);
			return 1;
		}
		if(mstruct.isPower() && mstruct[0].isVariable() && mstruct[0].variable()->id() == VARIABLE_ID_E && mstruct[1].isNumber() && mstruct[1].number().hasImaginaryPart() && !mstruct[1].number().hasRealPart()) {
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				Number nr(*mstruct[1].number().internalImaginary());
				Number nrpi; nrpi.pi();
				nr.add(nrpi);
				nr.divide(nrpi);
				nr.divide(2);
				Number nr_u(nr.upperEndPoint());
				nr = nr.lowerEndPoint();
				nr_u.floor();
				nr.floor();
				if(!CALCULATOR->endTemporaryStopMessages() && nr == nr_u) {
					CALCULATOR->endTemporaryEnableIntervalArithmetic();
					nr.setApproximate(false);
					nr *= 2;
					nr.negate();
					mstruct = mstruct[1].number().imaginaryPart();
					if(!nr.isZero()) {
						mstruct += nr;
						mstruct.last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					}
					return true;
				}
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
		}
		if(eo.approximation == APPROXIMATION_EXACT) {
			msave = mstruct;
			if(!test_eval(mstruct, eo)) {
				mstruct = msave;
				return -1;
			}
		}
	}
	if(mstruct.isNumber()) {
		if(!mstruct.number().hasImaginaryPart()) {
			if(!mstruct.number().isNonZero()) {
				if(!msave.isZero()) mstruct = msave;
				return -1;
			}
			if(mstruct.number().isNegative()) {
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {mstruct.set(180, 1, 0); break;}
					case ANGLE_UNIT_GRADIANS: {mstruct.set(200, 1, 0); break;}
					case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI)); break;}
					default: {mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI)); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
				}
			} else {
				mstruct.clear();
			}
		} else if(!mstruct.number().hasRealPart() && mstruct.number().imaginaryPartIsNonZero()) {
			bool b_neg = mstruct.number().imaginaryPartIsNegative();
			switch(eo.parse_options.angle_unit) {
				case ANGLE_UNIT_DEGREES: {mstruct.set(90, 1, 0); break;}
				case ANGLE_UNIT_GRADIANS: {mstruct.set(100, 1, 0); break;}
				case ANGLE_UNIT_RADIANS: {mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI)); mstruct.multiply(nr_half); break;}
				default: {mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI)); mstruct.multiply(nr_half); if(CALCULATOR->getRadUnit()) mstruct *= CALCULATOR->getRadUnit();}
			}
			if(b_neg) mstruct.negate();
		} else if(!msave.isZero()) {
			mstruct = msave;
			return -1;
		} else if(!mstruct.number().realPartIsNonZero()) {
			FR_FUNCTION(arg)
		} else {
			MathStructure new_nr(mstruct.number().imaginaryPart());
			if(!new_nr.number().divide(mstruct.number().realPart())) return -1;
			if(mstruct.number().realPartIsNegative()) {
				if(mstruct.number().imaginaryPartIsNegative()) {
					mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
					switch(eo.parse_options.angle_unit) {
						case ANGLE_UNIT_DEGREES: {mstruct.add(-180); break;}
						case ANGLE_UNIT_GRADIANS: {mstruct.add(-200); break;}
						case ANGLE_UNIT_RADIANS: {mstruct.subtract(CALCULATOR->getVariableById(VARIABLE_ID_PI)); break;}
						default: {MathStructure msub(CALCULATOR->getVariableById(VARIABLE_ID_PI)); if(CALCULATOR->getRadUnit()) msub *= CALCULATOR->getRadUnit(); mstruct.subtract(msub);}
					}
				} else if(mstruct.number().imaginaryPartIsNonNegative()) {
					mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
					switch(eo.parse_options.angle_unit) {
						case ANGLE_UNIT_DEGREES: {mstruct.add(180); break;}
						case ANGLE_UNIT_GRADIANS: {mstruct.add(200); break;}
						case ANGLE_UNIT_RADIANS: {mstruct.add(CALCULATOR->getVariableById(VARIABLE_ID_PI)); break;}
						default: {MathStructure madd(CALCULATOR->getVariableById(VARIABLE_ID_PI)); if(CALCULATOR->getRadUnit()) madd *= CALCULATOR->getRadUnit(); mstruct.add(madd);}
					}
				} else {
					FR_FUNCTION(arg)
				}
			} else {
				mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
			}
		}
		return 1;
	}
	if(!msave.isZero()) {
		goto arg_test_non_number;
	}
	return -1;
}

IsNumberFunction::IsNumberFunction() : MathFunction("isNumber", 1) {
}
int IsNumberFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	if(!mstruct.isNumber()) mstruct.eval(eo);
	if(mstruct.isNumber()) {
		mstruct.number().setTrue();
	} else {
		mstruct.clear();
		mstruct.number().setFalse();
	}
	return 1;
}

#define IS_NUMBER_FUNCTION(x, y) x::x() : MathFunction(#y, 1) {} int x::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {mstruct = vargs[0]; if(!mstruct.isNumber()) mstruct.eval(eo); if(mstruct.isNumber() && mstruct.number().y()) {mstruct.number().setTrue();} else {mstruct.clear(); mstruct.number().setFalse();} return 1;}

IS_NUMBER_FUNCTION(IsIntegerFunction, isInteger)
IS_NUMBER_FUNCTION(IsRealFunction, isReal)
IS_NUMBER_FUNCTION(IsRationalFunction, isRational)

