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

#define FR_FUNCTION(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC(vargs[1].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2Rm(FUNC)	Number nr(vargs[1].number()); if(!nr.FUNC(mstruct.number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[1].isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[1].number().isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[1].number().includesInfinity() && !mstruct.number().includesInfinity())) {return ret;} else {mstruct.set(nr); return 1;}
#define NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(i)			NumberArgument *arg_non_complex##i = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);

bool has_interval_unknowns(MathStructure &m) {
	if(m.isVariable() && !m.variable()->isKnown()) {
		Assumptions *ass = ((UnknownVariable*) m.variable())->assumptions();
		return !((UnknownVariable*) m.variable())->interval().isUndefined() || (ass && ((ass->sign() != ASSUMPTION_SIGN_UNKNOWN && ass->sign() != ASSUMPTION_SIGN_NONZERO) || ass->min() || ass->max()));
	}
	for(size_t i = 0; i < m.size(); i++) {
		if(has_interval_unknowns(m[i])) return true;
	}
	return false;
}

bool bernoulli_poly(MathStructure &m, Number n, const MathStructure &mx, const EvaluationOptions &eo) {
	m.clear();
	Number bin, k, nmk(n), nrB;
	while(k <= n) {
		if(nmk.isEven() || nmk.isOne()) {
			nrB.set(nmk);
			if(!bin.binomial(n, k) || !nrB.bernoulli() || !nrB.multiply(bin)) return false;
			if(eo.approximation == APPROXIMATION_EXACT && nrB.isApproximate()) return false;
			m.add(nrB, true);
			m.last().multiply(mx);
			m.last().last().raise(k);
			m.childUpdated(m.size());
		}
		nmk--;
		k++;
	}
	if(m.isAddition()) m.delChild(1, true);
	return true;
}

ZetaFunction::ZetaFunction() : MathFunction("zeta", 1, 2, SIGN_ZETA) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false);
	setArgumentDefinition(2, arg);
	setDefaultValue(2, "1");
}
int ZetaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs.size() == 1 || vargs[1].isOne()) {
		if(vargs[0].number().isInteger()) {
			if(vargs[0].number().isZero()) {
				mstruct.set(-1, 2, 0);
				return 1;
			} else if(vargs[0].number().isMinusOne()) {
				mstruct.set(-1, 12, 0);
				return 1;
			} else if(vargs[0].number().isNegative() && vargs[0].number().isEven()) {
				mstruct.clear();
				return 1;
			} else if(vargs[0].number().isNegative() && vargs[0].number() >= -497) {
				Number nr(vargs[0].number());
				nr.negate();
				nr++;
				nr.bernoulli();
				nr.divide(vargs[0].number() - 1);
				if(vargs[0].number().isEven()) nr.negate();
				mstruct.set(nr);
				mstruct.mergePrecision(vargs[0]);
				return 1;
			} else if(vargs[0].number().isEven() && vargs[0].number() <= 498) {
				Number nr(vargs[0].number());
				nr.bernoulli();
				mstruct.set(nr);
				mstruct.multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				mstruct.last().multiply(nr_two);
				mstruct.last().raise(vargs[0]);
				mstruct.multiply(vargs[0]);
				mstruct.last().transformById(FUNCTION_ID_FACTORIAL);
				mstruct.last().inverse();
				if(vargs[0].number().isIntegerDivisible(4)) mstruct.multiply(Number(-1, 2));
				else mstruct.multiply(nr_half);
				mstruct.childrenUpdated(true);
				return 1;
			}
		}
		FR_FUNCTION(zeta)
	}
	if(vargs[0].number().isZero()) {
		mstruct.set(1, 2, 0);
		if(!vargs[1].isZero()) mstruct.subtract(vargs[1]);
		return 1;
	} else if(vargs[0].number().isInteger() && vargs[0].number().isNegative()) {
		Number nr(vargs[0].number());
		nr.negate();
		nr++;
		MathStructure m2(vargs[1]);
		replace_f_interval(m2, eo);
		replace_intervals_f(m2);
		if(bernoulli_poly(mstruct, nr, m2, eo)) {
			mstruct.divide(nr);
			mstruct.negate();
			return 1;
		}
	} else if(vargs[1].number().isInteger() && vargs[1].number() >= 2 && ((eo.approximation == APPROXIMATION_EXACT && vargs[1].number() <= 1000) || (vargs[1].number() <= 50 && vargs[0].number() >= -10 && vargs[0].number() <= 10))) {
		MathStructure m1(vargs[0]);
		replace_f_interval(m1, eo);
		replace_intervals_f(m1);
		mstruct = m1;
		m1.negate();
		mstruct.transform(this);
		mstruct.addChild(m_one);
		mstruct.add(m_minus_one);
		for(long int i = 2; vargs[1].number() > i; i++) {
			mstruct.add(Number(i, 1), true);
			mstruct.last().raise(m1);
			mstruct.last().negate();
		}
		return true;
	}
	FR_FUNCTION_2(zeta)
}
GammaFunction::GammaFunction() : MathFunction("gamma", 1, 1, SIGN_CAPITAL_GAMMA) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1);
}
int GammaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isRational() && (eo.approximation == APPROXIMATION_EXACT || (eo.approximation == APPROXIMATION_TRY_EXACT && vargs[0].number().isLessThan(1000)))) {
		if(vargs[0].number().isInteger() && vargs[0].number().isPositive()) {
			mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_FACTORIAL), &vargs[0], NULL);
			mstruct[0] -= 1;
			return 1;
		} else if(vargs[0].number().denominatorIsTwo()) {
			Number nr(vargs[0].number());
			nr.floor();
			if(nr.isZero()) {
				MathStructure mtmp(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), &mtmp, NULL);
				return 1;
			} else if(nr.isPositive()) {
				Number nr2(nr);
				nr2 *= 2;
				nr2 -= 1;
				nr2.doubleFactorial();
				Number nr3(2, 1, 0);
				nr3 ^= nr;
				nr2 /= nr3;
				mstruct = nr2;
				MathStructure mtmp1(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				MathStructure mtmp2(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), &mtmp1, NULL);
				mstruct *= mtmp2;
				return 1;
			} else {
				nr.negate();
				Number nr2(nr);
				nr2 *= 2;
				nr2 -= 1;
				nr2.doubleFactorial();
				Number nr3(2, 1, 0);
				nr3 ^= nr;
				if(nr.isOdd()) nr3.negate();
				nr3 /= nr2;
				mstruct = nr3;
				MathStructure mtmp1(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				MathStructure mtmp2(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), &mtmp1, NULL);
				mstruct *= mtmp2;
				return 1;
			}
		}
	}
	FR_FUNCTION(gamma)
}
DigammaFunction::DigammaFunction() : MathFunction("digamma", 1) {
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(1);
}
int DigammaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isOne()) {
		mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_EULER));
		mstruct.negate();
		return 1;
	}
	FR_FUNCTION(digamma)
}
BetaFunction::BetaFunction() : MathFunction("beta", 2, 2, SIGN_CAPITAL_BETA) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
}
int BetaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector() || vargs[1].isVector()) return 0;
	MathStructure m1(vargs[0]);
	MathStructure m2(vargs[1]);
	m1.eval(eo);
	m2.eval(eo);
	if(m1.isVector() || m2.isVector()) {
		mstruct.setVector(&m1, &m2, NULL);
		return -3;
	}
	if(!m1.isNumber() && m2.isInteger() && m2.number().isPositive() && m2.number() <= 13) {
		mstruct = m1;
		Number i(m2.number());
		i--;
		while(i.isPositive()) {
			mstruct.multiply(m1, true);
			mstruct.last() += i;
			i--;
		}
		Number fac(m2.number());
		fac--;
		fac.factorial();
		mstruct.inverse();
		mstruct *= fac;
		return 1;
	}
	mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_GAMMA), &m1, NULL);
	MathStructure mstruct2(CALCULATOR->getFunctionById(FUNCTION_ID_GAMMA), &m2, NULL);
	mstruct *= mstruct2;
	mstruct2[0] += m1;
	mstruct /= mstruct2;
	return 1;
}
AiryFunction::AiryFunction() : MathFunction("airy", 1) {
	NumberArgument *arg = new NumberArgument();
	Number fr(-500, 1, 0);
	arg->setMin(&fr);
	fr.set(500, 1, 0);
	arg->setMax(&fr);
	setArgumentDefinition(1, arg);
}
int AiryFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(airy)
}
BesseljFunction::BesseljFunction() : MathFunction("besselj", 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, true, INTEGER_TYPE_SLONG));
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(2);
}
int BesseljFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	int ret = 0;
	if(!mstruct.isNumber()) {
		mstruct.eval(eo);
		if(!mstruct.equals(vargs[0], true, true)) ret = -1;
	}
	Argument *arg = getArgumentDefinition(1);
	if(arg) {
		arg->setTests(true);
		bool b = true;
		if(!mstruct.isInteger()) {
			MathStructure m; m.setUndefined();
			b = getArgumentDefinition(1)->test(m, 1, this, eo);
		} else {
			b = getArgumentDefinition(1)->test(mstruct, 1, this, eo);
		}
		arg->setTests(false);
		if(!b) return ret;
	} else if(!mstruct.isNumber()) {
		return ret;
	}
	FR_FUNCTION_2Rm(besselj)
}
BesselyFunction::BesselyFunction() : MathFunction("bessely", 2) {
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, false, true, INTEGER_TYPE_SLONG);
	Number nmax(1000);
	iarg->setMax(&nmax);
	setArgumentDefinition(1, iarg);
	NON_COMPLEX_NUMBER_ARGUMENT_NO_ERROR(2);
}
int BesselyFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	int ret = 0;
	if(!mstruct.isNumber()) {
		mstruct.eval(eo);
		if(!mstruct.equals(vargs[0], true, true)) ret = -1;
	}
	Argument *arg = getArgumentDefinition(1);
	if(arg) {
		arg->setTests(true);
		bool b = true;
		if(!mstruct.isInteger()) {
			MathStructure m; m.setUndefined();
			b = getArgumentDefinition(1)->test(m, 1, this, eo);
		} else {
			b = getArgumentDefinition(1)->test(mstruct, 1, this, eo);
		}
		arg->setTests(false);
		if(!b) return ret;
	} else if(!mstruct.isNumber()) {
		return ret;
	}
	FR_FUNCTION_2Rm(bessely)
}
ErfFunction::ErfFunction() : MathFunction("erf", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool ErfFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool ErfFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNegative();}
bool ErfFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative();}
bool ErfFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonPositive();}
bool ErfFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool ErfFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ErfFunction::representsRational(const MathStructure&, bool) const {return false;}
bool ErfFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool ErfFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool ErfFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool ErfFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero();}
bool ErfFunction::representsEven(const MathStructure&, bool) const {return false;}
bool ErfFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool ErfFunction::representsUndefined(const MathStructure&) const {return false;}
int ErfFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	if(!nr.erf() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {
		if(vargs[0].number().hasImaginaryPart() && !vargs[0].number().hasRealPart()) {
			mstruct = vargs[0].number().imaginaryPart();
			MathFunction *f = CALCULATOR->getFunctionById(FUNCTION_ID_ERFI);
			mstruct.transform(f);
			mstruct *= nr_one_i;
			return 1;
		}
		return 0;
	}
	mstruct.set(nr);
	return 1;
}
ErfiFunction::ErfiFunction() : MathFunction("erfi", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool ErfiFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool ErfiFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNegative();}
bool ErfiFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative();}
bool ErfiFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonPositive();}
bool ErfiFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool ErfiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ErfiFunction::representsRational(const MathStructure&, bool) const {return false;}
bool ErfiFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ErfiFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool ErfiFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool ErfiFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero();}
bool ErfiFunction::representsEven(const MathStructure&, bool) const {return false;}
bool ErfiFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool ErfiFunction::representsUndefined(const MathStructure&) const {return false;}
int ErfiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	Number nr(vargs[0].number());
	if(!nr.erfi() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {
		if(vargs[0].number().hasImaginaryPart() && !vargs[0].number().hasRealPart()) {
			mstruct = vargs[0].number().imaginaryPart();
			mstruct.transformById(FUNCTION_ID_ERF);
			mstruct *= nr_one_i;
			return 1;
		}
		return 0;
	}
	mstruct.set(nr);
	return 1;
}
ErfcFunction::ErfcFunction() : MathFunction("erfc", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool ErfcFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ErfcFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool ErfcFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ErfcFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool ErfcFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool ErfcFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ErfcFunction::representsRational(const MathStructure&, bool) const {return false;}
bool ErfcFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool ErfcFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool ErfcFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool ErfcFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ErfcFunction::representsEven(const MathStructure&, bool) const {return false;}
bool ErfcFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool ErfcFunction::representsUndefined(const MathStructure&) const {return false;}
int ErfcFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(erfc)
}
ErfinvFunction::ErfinvFunction() : MathFunction("erfinv", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false);
	arg->setMax(&nr_one);
	arg->setMin(&nr_minus_one);
	setArgumentDefinition(1, arg);
}
int ErfinvFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(erfinv)
}

LiFunction::LiFunction() : MathFunction("Li", 2) {
	names[0].case_sensitive = true;
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false);
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
}
bool LiFunction::representsReal(const MathStructure &vargs, bool) const {
	return vargs.size() == 2 && vargs[0].representsInteger() && vargs[1].representsReal() && (vargs[1].representsNonPositive() || ((vargs[1].isNumber() && vargs[1].number().isLessThanOrEqualTo(1)) || (vargs[1].isVariable() && vargs[1].variable()->isKnown() && ((KnownVariable*) vargs[1].variable())->get().isNumber() && ((KnownVariable*) vargs[1].variable())->get().number().isLessThanOrEqualTo(1)))) && ((vargs[0].representsPositive() || ((vargs[1].isNumber() && comparison_is_not_equal(vargs[1].number().compare(nr_one))) || (vargs[1].isVariable() && vargs[1].variable()->isKnown() && ((KnownVariable*) vargs[1].variable())->get().isNumber() && comparison_is_not_equal(((KnownVariable*) vargs[1].variable())->get().number().compare(nr_one))))));
}
bool LiFunction::representsNonComplex(const MathStructure &vargs, bool) const {
	return vargs.size() == 2 && vargs[0].representsInteger() && vargs[1].representsNonComplex() && (vargs[1].representsNonPositive() || ((vargs[1].isNumber() && vargs[1].number().isLessThanOrEqualTo(1)) || (vargs[1].isVariable() && vargs[1].variable()->isKnown() && ((KnownVariable*) vargs[1].variable())->get().isNumber() && ((KnownVariable*) vargs[1].variable())->get().number().isLessThanOrEqualTo(1))));
}
bool LiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[0].representsInteger() && (vargs[0].representsPositive() || (vargs[1].representsNumber() && ((vargs[1].isNumber() && comparison_is_not_equal(vargs[1].number().compare(nr_one))) || (vargs[1].isVariable() && vargs[1].variable()->isKnown() && ((KnownVariable*) vargs[1].variable())->get().isNumber() && comparison_is_not_equal(((KnownVariable*) vargs[1].variable())->get().number().compare(nr_one))))));}
int LiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[1].isVector()) return 0;
	if(vargs[0].isInteger()) {
		if(vargs[0].number().isOne()) {
			mstruct.set(1, 1, 0);
			mstruct -= vargs[1];
			mstruct.transformById(FUNCTION_ID_LOG);
			mstruct.negate();
			return true;
		} else if(vargs[0].number().isZero()) {
			mstruct.set(1, 1, 0);
			mstruct -= vargs[1];
			mstruct.inverse();
			mstruct += nr_minus_one;
			return true;
		} else if(vargs[0].number().isNegative()) {
			if(vargs[0].number().isMinusOne()) {
				mstruct.set(1, 1, 0);
				mstruct -= vargs[1];
				mstruct ^= Number(-2, 1);
				mstruct *= vargs[1];
				return true;
			} else if(vargs[0].number() == -2) {
				mstruct.set(1, 1, 0);
				mstruct -= vargs[1];
				mstruct ^= Number(-3, 1);
				mstruct *= vargs[1];
				mstruct.last() += m_one;
				mstruct.last() *= vargs[1];
				return true;
			} else if(vargs[0].number() == -3) {
				mstruct.set(1, 1, 0);
				mstruct -= vargs[1];
				mstruct ^= Number(-4, 1);
				mstruct *= vargs[1];
				mstruct.last() ^= nr_two;
				mstruct.last() += Number(4, 1);
				mstruct.last().last() *= vargs[1];
				mstruct.last() += m_one;
				mstruct.last() *= vargs[1];
				return true;
			} else if(vargs[0].number() == -4) {
				mstruct.set(1, 1, 0);
				mstruct -= vargs[1];
				mstruct ^= Number(-5, 1);
				mstruct *= vargs[1];
				mstruct.last() ^= nr_two;
				mstruct.last() += Number(10, 1);
				mstruct.last().last() *= vargs[1];
				mstruct.last() += m_one;
				mstruct.last() *= vargs[1];
				mstruct.last().last() += m_one;
				mstruct.last() *= vargs[1];
				return true;
			}
		}
	}
	mstruct = vargs[1];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -2;
	if(vargs[0].number() >= 1 && mstruct.isOne()) {
		mstruct = vargs[0];
		mstruct.transformById(FUNCTION_ID_ZETA);
		return true;
	}
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.polylog(vargs[0].number()) && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || vargs[0].isApproximate() || mstruct.isApproximate()) && (eo.allow_complex || !nr.isComplex() || vargs[0].number().isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || vargs[0].number().includesInfinity() || mstruct.number().includesInfinity())) {
			mstruct.set(nr); return 1;
		}
	}
	return -2;
}

HeavisideFunction::HeavisideFunction() : MathFunction("heaviside", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	arg->setComplexAllowed(false);
	setArgumentDefinition(1, arg);
}
bool HeavisideFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative(true);}
bool HeavisideFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool HeavisideFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool HeavisideFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool HeavisideFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero() && vargs[0].representsReal(true);}
bool HeavisideFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool HeavisideFunction::representsRational(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool HeavisideFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
bool HeavisideFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal(true);}
bool HeavisideFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool HeavisideFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative(true);}
bool HeavisideFunction::representsEven(const MathStructure&, bool) const {return false;}
bool HeavisideFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool HeavisideFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 1 && vargs[0].representsUndefined();}
int HeavisideFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(!mstruct.representsNonComplex(true)) return false;
	if(mstruct.representsPositive(true)) {
		mstruct.set(1, 1, 0);
		return 1;
	}
	if(mstruct.representsNegative(true)) {
		mstruct.clear();
		return 1;
	}
	if(mstruct.isZero()) {
		mstruct = nr_half;
		return 1;
	}
	if(mstruct.isNumber() && mstruct.number().isInterval()) {
		if(!mstruct.number().isNonNegative()) {
			mstruct.number().setInterval(nr_half, nr_one);
		} else if(!mstruct.number().isNonPositive()) {
			mstruct.number().setInterval(nr_zero, nr_half);
		} else {
			mstruct.number().setInterval(nr_zero, nr_one);
		}
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT || has_interval_unknowns(mstruct)) {
		ComparisonResult cr = mstruct.compare(m_zero);
		if(cr == COMPARISON_RESULT_LESS) {
			mstruct.set(1, 1, 0);
			return 1;
		} else if(cr == COMPARISON_RESULT_GREATER) {
			mstruct.clear();
			return 1;
		}
	}
	return -1;
}
DiracFunction::DiracFunction() : MathFunction("dirac", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	setArgumentDefinition(1, arg);
}
bool DiracFunction::representsPositive(const MathStructure&, bool allow_units) const {return false;}
bool DiracFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsNonNegative(const MathStructure&, bool) const {return true;}
bool DiracFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsNumber(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsRational(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
bool DiracFunction::representsReal(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsComplex(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsNonZero(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsEven(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool DiracFunction::representsUndefined(const MathStructure &vargs) const {return vargs.size() == 1 && vargs[0].representsUndefined();}
int DiracFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(!mstruct.representsNonComplex(true)) return false;
	if(mstruct.representsNonZero(true)) {
		mstruct.clear();
		return 1;
	}
	if(mstruct.isZero()) {
		mstruct = nr_plus_inf;
		return 1;
	}
	if(mstruct.isNumber() && mstruct.number().isInterval() && !mstruct.number().isNonZero()) {
		mstruct.number().setInterval(nr_zero, nr_plus_inf);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_EXACT || has_interval_unknowns(mstruct)) {
		ComparisonResult cr = mstruct.compare(m_zero);
		if(COMPARISON_IS_NOT_EQUAL(cr)) {
			mstruct.clear();
			return 1;
		}
	}
	return -1;
}

