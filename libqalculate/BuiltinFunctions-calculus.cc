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
#define NON_COMPLEX_NUMBER_ARGUMENT(i)				NumberArgument *arg_non_complex##i = new NumberArgument(); arg_non_complex##i->setComplexAllowed(false); setArgumentDefinition(i, arg_non_complex##i);

liFunction::liFunction() : MathFunction("li", 1) {
	names[0].case_sensitive = true;
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool liFunction::representsReal(const MathStructure &vargs, bool) const {
	return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative() && ((vargs[0].isNumber() && !COMPARISON_IS_NOT_EQUAL(vargs[0].number().compare(nr_one))) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && COMPARISON_IS_NOT_EQUAL(((KnownVariable*) vargs[0].variable())->get().number().compare(nr_one))));
}
bool liFunction::representsNonComplex(const MathStructure &vargs, bool) const {
	return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();
}
bool liFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber() && ((vargs[0].isNumber() && COMPARISON_IS_NOT_EQUAL(vargs[0].number().compare(nr_one))) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && COMPARISON_IS_NOT_EQUAL(((KnownVariable*) vargs[0].variable())->get().number().compare(nr_one))));}
int liFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(logint)
}
EiFunction::EiFunction() : MathFunction("Ei", 1) {
	names[0].case_sensitive = true;
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool EiFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonZero();}
bool EiFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool EiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber() && vargs[0].representsNonZero();}
int EiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION(expint)
}
FresnelSFunction::FresnelSFunction() : MathFunction("fresnels", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	Number fr(-6, 1);
	arg->setMin(&fr);
	fr = 6;
	arg->setMax(&fr);
	setArgumentDefinition(1, arg);
}
bool FresnelSFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool FresnelSFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNegative();}
bool FresnelSFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative();}
bool FresnelSFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonPositive();}
bool FresnelSFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool FresnelSFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool FresnelSFunction::representsRational(const MathStructure&, bool) const {return false;}
bool FresnelSFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool FresnelSFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool FresnelSFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool FresnelSFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero();}
bool FresnelSFunction::representsEven(const MathStructure&, bool) const {return false;}
bool FresnelSFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool FresnelSFunction::representsUndefined(const MathStructure&) const {return false;}
int FresnelSFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(contains_angle_unit(mstruct, eo.parse_options)) {mstruct /= CALCULATOR->getRadUnit(); mstruct.eval(eo);}
	if(!mstruct.isNumber()) return -1;
	Number nr(mstruct.number()); if(!nr.fresnels() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {
		return -1;
	}
	mstruct.set(nr);
	return 1;
}
FresnelCFunction::FresnelCFunction() : MathFunction("fresnelc", 1) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	Number fr(-6, 1);
	arg->setMin(&fr);
	fr = 6;
	arg->setMax(&fr);
	setArgumentDefinition(1, arg);
}
bool FresnelCFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool FresnelCFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNegative();}
bool FresnelCFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative();}
bool FresnelCFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonPositive();}
bool FresnelCFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool FresnelCFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool FresnelCFunction::representsRational(const MathStructure&, bool) const {return false;}
bool FresnelCFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool FresnelCFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
bool FresnelCFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsComplex();}
bool FresnelCFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonZero();}
bool FresnelCFunction::representsEven(const MathStructure&, bool) const {return false;}
bool FresnelCFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool FresnelCFunction::representsUndefined(const MathStructure&) const {return false;}
int FresnelCFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(contains_angle_unit(mstruct, eo.parse_options)) {mstruct /= CALCULATOR->getRadUnit(); mstruct.eval(eo);}
	if(!mstruct.isNumber()) return -1;
	Number nr(mstruct.number()); if(!nr.fresnelc() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {
		return -1;
	}
	mstruct.set(nr);
	return 1;
}
SiFunction::SiFunction() : MathFunction("Si", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool SiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool SiFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool SiFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
int SiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(contains_angle_unit(mstruct, eo.parse_options)) {mstruct /= CALCULATOR->getRadUnit(); mstruct.eval(eo);}
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.isPlusInfinity()) {
			mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI), true);
			mstruct *= nr_half;
			return 1;
		}
		if(nr.isMinusInfinity()) {
			mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI), true);
			mstruct *= nr_minus_half;
			return 1;
		}
		if(nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			mstruct.transformById(FUNCTION_ID_SINHINT);
			mstruct *= nr_one_i;
			return 1;
		}
		if(nr.sinint() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || mstruct.isApproximate()) && (eo.allow_complex || !nr.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || mstruct.number().includesInfinity())) {
			mstruct.set(nr);
			return 1;
		}
	}
	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
		mstruct.transform(this);
		mstruct.negate();
		return 1;
	}
	return -1;
}
CiFunction::CiFunction() : MathFunction("Ci", 1) {
	names[0].case_sensitive = true;
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool CiFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool CiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool CiFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative(true);}
int CiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(contains_angle_unit(mstruct, eo.parse_options)) {mstruct /= CALCULATOR->getRadUnit(); mstruct.eval(eo);}
	if(mstruct.isNumber()) {
		if(mstruct.number().isNegative()) {
			if(!eo.allow_complex) return -1;
			mstruct.negate();
			mstruct.transform(this);
			mstruct += CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct.last() *= nr_one_i;
			return 1;
		}
		Number nr(mstruct.number());
		if(nr.isComplex() && nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			if(nr.imaginaryPartIsNegative()) mstruct.negate();
			mstruct.transformById(FUNCTION_ID_COSHINT);
			mstruct += CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct.last() *= nr_half;
			if(nr.imaginaryPartIsPositive()) mstruct.last() *= nr_one_i;
			else mstruct.last() *= nr_minus_i;
			return 1;
		}
		if(nr.cosint() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || mstruct.isApproximate()) && (eo.allow_complex || !nr.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || mstruct.number().includesInfinity())) {
			mstruct.set(nr);
			return 1;
		}
	}
	return -1;
}
ShiFunction::ShiFunction() : MathFunction("Shi", 1) {
	names[0].case_sensitive = true;
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool ShiFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool ShiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
bool ShiFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
int ShiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			mstruct.transformById(FUNCTION_ID_SININT);
			mstruct *= nr_one_i;
			return 1;
		}
		if(nr.sinhint() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || mstruct.isApproximate()) && (eo.allow_complex || !nr.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || mstruct.number().includesInfinity())) {
			mstruct.set(nr);
			return 1;
		}
	}
	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
		mstruct.transform(this);
		mstruct.negate();
		return 1;
	}
	return -1;
}
ChiFunction::ChiFunction() : MathFunction("Chi", 1) {
	names[0].case_sensitive = true;
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool ChiFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsPositive();}
bool ChiFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonNegative();}
bool ChiFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNumber();}
int ChiFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isNumber()) {
		if(eo.allow_complex && mstruct.number().isNegative()) {
			if(!eo.allow_complex) return -1;
			mstruct.negate();
			mstruct.transform(this);
			mstruct += CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct.last() *= nr_one_i;
			return 1;
		}
		Number nr(mstruct.number());
		if(nr.isComplex() && nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			if(nr.imaginaryPartIsNegative()) mstruct.negate();
			mstruct.transformById(FUNCTION_ID_COSINT);
			mstruct += CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct.last() *= nr_half;
			if(nr.imaginaryPartIsPositive()) mstruct.last() *= nr_one_i;
			else mstruct.last() *= nr_minus_i;
			return 1;
		}
		if(nr.coshint() && (eo.approximation != APPROXIMATION_EXACT || !nr.isApproximate() || mstruct.isApproximate()) && (eo.allow_complex || !nr.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.includesInfinity() || mstruct.number().includesInfinity())) {
			mstruct.set(nr);
			return 1;
		}
	}
	return -1;
}

IGammaFunction::IGammaFunction() : MathFunction("igamma", 2) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool IGammaFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[1].representsPositive() || (vargs[0].representsInteger() && vargs[0].representsPositive()));}
bool IGammaFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[1].representsNonNegative() || (vargs[0].representsInteger() && vargs[0].representsNonNegative()));}
bool IGammaFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[1].representsNonZero() || vargs[0].representsPositive());}
int IGammaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	FR_FUNCTION_2(igamma)
}


bool betainc(Number &nvalue, const Number &nr_x, const Number &nr_a, const Number &nr_b) {

	long int max_steps = 16;
	long int min_steps = 6;
	bool safety_measures = true;

	Number R1[max_steps], R2[max_steps];
	Number *Rp = &R1[0], *Rc = &R2[0];
	Number h(nr_x);
	Number acc, acc_i, c, ntmp, prevunc, prevunc_i, nunc, nunc_i;

	nvalue.clear();

	Number nr_am1(nr_a), nr_bm1(nr_b);
	nr_am1--; nr_bm1--;
	Number nrtmp;

	Number mf(1, 1, 0);
	if((!nrtmp.isZero() && !nrtmp.raise(nr_am1)) || !mf.multiply(nrtmp) || mf.includesInfinity()) return false;
	Rp[0] = mf;

	mf.set(1, 1, 0);
	nrtmp = nr_x;
	if(!mf.subtract(nr_x) || (!mf.isZero() && !mf.raise(nr_bm1)) || !nrtmp.raise(nr_am1) || !mf.multiply(nrtmp) || mf.includesInfinity()) return false;

	if(!Rp[0].add(mf) || !Rp[0].multiply(nr_half) || !Rp[0].multiply(h)) return false;

	for(long int i = 1; i < max_steps; i++) {

		if(CALCULATOR->aborted()) break;

		if(!h.multiply(nr_half)) return false;

		c.clear();

		long int ep = 1 << (i - 1);
		ntmp.clear(); ntmp += h;
		for(long int j = 1; j <= ep; j++){
			if(CALCULATOR->aborted()) break;
			mf.set(1, 1, 0);
			nrtmp = ntmp;
			if(!mf.subtract(ntmp) || (!mf.isZero() && !mf.raise(nr_bm1)) || (!nrtmp.isZero() && !nrtmp.raise(nr_am1)) || !mf.multiply(nrtmp) || mf.includesInfinity()) return false;
			if(CALCULATOR->aborted()) break;
			if(!c.add(mf)) return false;
			ntmp += h; ntmp += h;
		}
		if(CALCULATOR->aborted()) break;

		Rc[0] = h;
		ntmp = Rp[0];
		if(!ntmp.multiply(nr_half) || !Rc[0].multiply(c) || !Rc[0].add(ntmp)) return false;

		for(long int j = 1; j <= i; ++j){
			if(CALCULATOR->aborted()) break;
			ntmp = 4;
			ntmp ^= j;
			Rc[j] = ntmp;
			ntmp--; ntmp.recip();
			if(!Rc[j].multiply(Rc[j - 1]) || !Rc[j].subtract(Rp[j - 1]) || !Rc[j].multiply(ntmp)) return false;
		}
		if(CALCULATOR->aborted()) break;

		if(i >= min_steps - 1 && !Rp[i - 1].includesInfinity() && !Rc[i].includesInfinity()) {
			if(Rp[i - 1].hasImaginaryPart()) nunc = Rp[i - 1].realPart();
			else nunc = Rp[i - 1];
			if(Rc[i].hasImaginaryPart()) nunc -= Rc[i].realPart();
			else nunc -= Rc[i];
			nunc.abs();
			if(safety_measures) nunc *= 10;
			nunc.intervalToMidValue();
			if(Rp[i - 1].hasImaginaryPart() || Rc[i].hasImaginaryPart()) {
				nunc_i = Rp[i - 1].imaginaryPart();
				nunc_i -= Rc[i].imaginaryPart();
				nunc_i.abs();
			} else {
				nunc_i.clear();
			}
			if(safety_measures) nunc_i *= 10;
			nunc_i.intervalToMidValue();
			long int prec = PRECISION + 1;
			acc.set(1, 1, -prec);
			ntmp = Rc[i - 1];
			ntmp.intervalToMidValue();
			if(ntmp.hasImaginaryPart()) {
				if(ntmp.hasRealPart()) acc *= ntmp.realPart();
			} else {
				if(!ntmp.isZero()) acc *= ntmp;
			}
			acc.abs();
			acc.intervalToMidValue();
			nvalue = Rc[i - 1];
			if(nunc <= acc) {
				if(!nunc_i.isZero()) {
					acc_i.set(1, 1, -prec);
					if(ntmp.hasImaginaryPart()) acc_i *= ntmp.imaginaryPart();
					acc_i.abs();
					acc_i.intervalToMidValue();
					if(nunc_i <= acc_i) {
						if(!safety_measures) {
							nunc.setImaginaryPart(nunc_i);
							nvalue.setUncertainty(nunc);
							return true;
						}
						if(!prevunc.isZero() || !prevunc_i.isZero() || (nunc.isZero() && nunc_i.isZero())) {
							if(nunc <= prevunc && nunc_i <= prevunc_i) {
								if(!ntmp.hasRealPart()) prevunc = acc;
								if(!ntmp.hasImaginaryPart()) prevunc.setImaginaryPart(acc_i);
								else prevunc.setImaginaryPart(prevunc_i);
								nvalue.setUncertainty(prevunc);
							} else {
								acc.setImaginaryPart(acc_i);
								nvalue.setUncertainty(acc);
							}
							return true;
						}
						prevunc = nunc;
						prevunc_i = nunc_i;
					} else {
						prevunc.clear();
						prevunc_i.clear();
					}
				} else {
					if(!safety_measures) {
						nvalue.setUncertainty(nunc);
						return true;
					}
					if(!prevunc.isZero() || nunc.isZero()) {
						if(!prevunc_i.isZero()) nunc.setImaginaryPart(prevunc_i);
						if(!ntmp.isZero() && nunc <= prevunc) nvalue.setUncertainty(prevunc);
						else nvalue.setUncertainty(acc);
						return true;
					}
					prevunc = nunc;
					prevunc_i = nunc_i;
				}
			} else {
				prevunc.clear();
				prevunc_i.clear();
			}
		}

		Number *rt = Rp;
		Rp = Rc;
		Rc = rt;
	}
	if(!nunc.isZero() || !nunc_i.isZero()) {
		acc.set(1, 1, -2);
		ntmp = nvalue;
		ntmp.intervalToMidValue();
		if(ntmp.hasImaginaryPart()) {
			if(ntmp.hasRealPart()) acc *= ntmp.realPart();
		} else {
			if(!ntmp.isZero()) acc *= ntmp;
		}
		acc.abs();
		acc.intervalToMidValue();
		if(nunc > acc) return false;
		if(!ntmp.hasRealPart()) nunc = acc;
		if(!nunc_i.isZero()) {
			acc.set(1, 1, -2);
			if(ntmp.hasImaginaryPart()) acc *= ntmp.imaginaryPart();
			acc.abs();
			acc.intervalToMidValue();
			if(nunc_i > acc) return false;
			if(ntmp.hasImaginaryPart()) nunc.setImaginaryPart(nunc_i);
			else nunc.setImaginaryPart(acc);
		}
		if(safety_measures) nunc *= 10;
		nvalue.setUncertainty(nunc);
		return true;
	}
	return false;
}

IncompleteBetaFunction::IncompleteBetaFunction() : MathFunction("betainc", 3) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
	setArgumentDefinition(2, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
	setArgumentDefinition(3, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
int IncompleteBetaFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].number().isZero()) {
		mstruct.clear();
		return 1;
	} else if(vargs[0].number().isOne()) {
		mstruct = vargs[1];
		mstruct.transformById(FUNCTION_ID_BETA);
		mstruct.addChild(vargs[2]);
		return 1;
	} else if(vargs[1].number().isOne()) {
		mstruct.set(1, 1, 0);
		mstruct -= vargs[0];
		mstruct ^= vargs[2];
		mstruct.negate();
		mstruct += m_one;
		mstruct /= vargs[2];
		return 1;
	} else if(vargs[2].number().isOne()) {
		mstruct = vargs[0];
		mstruct ^= vargs[1];
		mstruct += m_zero;
		mstruct.last() ^= vargs[1];
		mstruct.last().negate();
		mstruct /= vargs[1];
		return 1;
	}
	if(vargs[1].number().isInteger() && vargs[2].number().isInteger() && vargs[1].number().isPositive() && vargs[2].number().isPositive()) {
		if(vargs[0].number().isInterval() && vargs[0].number() <= 1 && vargs[0].number() >= -1) {
			mstruct = vargs[0].number().lowerEndPoint();
			mstruct.transform(this);
			mstruct.addChild(vargs[1]);
			mstruct.addChild(vargs[2]);
			mstruct.transformById(FUNCTION_ID_INTERVAL);
			mstruct.addChild(vargs[0].number().upperEndPoint());
			mstruct.last().transform(this);
			mstruct.last().addChild(vargs[1]);
			mstruct.last().addChild(vargs[2]);
			if(!vargs[0].number().isNonZero()) {
				mstruct.transformById(FUNCTION_ID_INTERVAL);
				mstruct.insertChild(m_zero, 1);
			}
			return 1;
		}
		//sum(binomial(p+q−1;\i)*x^\i*(1−x)^((p+q−1)−\i);p;p+q−1;\i)
		Number n(vargs[1].number()); n += vargs[2].number(); n--;
		Number i(vargs[1].number());
		Number v, v_i, x_m1(1, 1, 0), x_i, x_m1_i;
		x_m1 -= vargs[0].number();
		while(i <= n) {
			if(CALCULATOR->aborted()) return 0;
			x_i = vargs[0].number(); x_m1_i = x_m1;
			if(!v_i.binomial(n, i) || !x_i.raise(i) || !x_m1_i.raise(n - i) || !v_i.multiply(x_i) || !v_i.multiply(x_m1_i)) return 0;
			v += v_i;
			i++;
		}
		mstruct = v;
		mstruct *= vargs[1];
		mstruct.last().transformById(FUNCTION_ID_BETA);
		mstruct.last().addChild(vargs[2]);
		return 1;
	} else if(vargs[0].number().isNonNegative() && vargs[0].number().isFraction() && !vargs[1].number().isInterval() && (!vargs[1].number().isInteger() || vargs[1].number().isPositive()) && !vargs[2].number().isInterval()) {
		if(vargs[0].number().isInterval()) {
			mstruct = vargs[0].number().lowerEndPoint();
			mstruct.transform(this);
			mstruct.addChild(vargs[1]);
			mstruct.addChild(vargs[2]);
			mstruct.transformById(FUNCTION_ID_INTERVAL);
			mstruct.addChild(vargs[0].number().upperEndPoint());
			mstruct.last().transform(this);
			mstruct.last().addChild(vargs[1]);
			mstruct.last().addChild(vargs[2]);
			return 1;
		}
		Number x(vargs[0].number()), p(vargs[1].number()), q(vargs[2].number());
		Number nr_prec(1, 1, -(PRECISION + 10));
		Number nr_add;
		if(x > nr_half && (!q.isInteger() || q.isPositive())) {
			Number term_i;
			Number term, term_prev;
			Number w(1, 1, 0);
			if(!w.subtract(x) || !w.multiply(2)) return 0;
			Number w_pow(w);
			if(!w_pow.raise(q)) return 0;
			Number w_mul;
			Number div_q(q);
			Number twopowq(2, 1, 0);
			if(!twopowq.raise(q)) return 0;
			Number i(1, 1, 0);
			Number num_p(1, 1, 0);
			Number div_fac(1, 1, 0);
			Number nr_pmul;
			while(true) {
				if(CALCULATOR->aborted()) return 0;
				term_prev = term;
				term_i = num_p;
				w_mul.set(1, 1, 0);
				if(!w_mul.subtract(w_pow) || !term_i.multiply(w_mul) || !term_i.divide(div_q) || !term_i.divide(div_fac) || !term_i.divide(twopowq) || !term.add(term_i)) return 0;
				term_i.abs();
				if(term_i < nr_prec) {
					term.setUncertainty(term_i);
					break;
				}
				div_q++;
				nr_pmul = i;
				if(!w_pow.multiply(w) || !nr_pmul.subtract(p) || !num_p.multiply(nr_pmul) || !div_fac.multiply(i) || !twopowq.multiply(2)) return 0;
				i++;
			}
			nr_add = term;
			x.set(1, 2, 0);
		}
		Number nr(x);
		Number term_i;
		Number term(1, 1, 0), term_prev;
		Number div_p(p);
		Number i(1, 1, 0);
		Number num_q(1, 1, 0);
		Number div_fac(1, 1, 0);
		if(!nr.raise(p) || !term.divide(div_p)) return 0;
		div_p++;
		num_q -= q;
		Number nr_qmul;
		Number x_pow(x);
		nr_prec /= nr;
		while(true) {
			if(CALCULATOR->aborted()) return 0;
			term_prev = term;
			term_i = num_q;
			if(!term_i.divide(div_p) || !term_i.divide(div_fac) || !term_i.multiply(x_pow) || !term.add(term_i)) return 0;
			term_i.abs();
			if(term_i < nr_prec) {
				term.setUncertainty(term_i);
				break;
			}
			PrintOptions po;
			po.preserve_precision = true;
			po.preserve_format = true;
			i++; div_p++;
			nr_qmul = i;
			if(!nr_qmul.subtract(q) || !num_q.multiply(nr_qmul) || !div_fac.multiply(i) || !x_pow.multiply(x)) return 0;
		}
		if(!nr.multiply(term) || !nr.add(nr_add)) return 0;
		mstruct.set(nr);
		return 1;
	}
	Number nr(vargs[0].number()); if(!betainc(nr, vargs[0].number(), vargs[1].number(), vargs[2].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
}

LimitFunction::LimitFunction() : MathFunction("limit", 2, 4) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
	setArgumentDefinition(3, new SymbolicArgument());
	setDefaultValue(3, "x");
	IntegerArgument *iarg = new IntegerArgument();
	iarg->setMin(&nr_minus_one);
	iarg->setMax(&nr_one);
	setArgumentDefinition(4, iarg);
	setDefaultValue(4, "0");
}
int LimitFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[1].isVector()) return 0;
	mstruct = vargs[0];
	EvaluationOptions eo2 = eo;
	eo2.approximation = APPROXIMATION_EXACT;
	if(mstruct.calculateLimit(vargs[2], vargs[1], eo2, vargs[3].number().intValue())) return 1;
	CALCULATOR->error(true, _("Unable to find limit."), NULL);
	return -1;
}
bool replace_diff_x(MathStructure &m, const MathStructure &mfrom, const MathStructure &mto) {
	if(m.equals(mfrom, true, true)) {
		m = mto;
		return true;
	}
	if(m.type() == STRUCT_FUNCTION && m.function()->id() == FUNCTION_ID_DIFFERENTIATE) {
		if(m.size() >= 4 && m[1] == mfrom && m[3].isUndefined()) {
			m[3] = mto;
			return true;
		}
		return false;
	}
	bool b = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_diff_x(m[i], mfrom, mto)) {
			b = true;
			m.childUpdated(i + 1);
		}
	}
	return b;
}

DeriveFunction::DeriveFunction() : MathFunction("diff", 1, 4) {
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
	Argument *arg = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SINT);
	arg->setHandleVector(false);
	setArgumentDefinition(3, arg);
	setDefaultValue(3, "1");
	setDefaultValue(4, "undefined");
}
int DeriveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	int i = vargs[2].number().intValue();
	mstruct = vargs[0];
	bool b = false;
	while(i) {
		if(CALCULATOR->aborted()) return 0;
		if(!mstruct.differentiate(vargs[1], eo) && !b) {
			return 0;
		}
		b = true;
		i--;
		if(i > 0) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			eo2.calculate_functions = false;
			mstruct.eval(eo2);
		}
	}
	if(!vargs[3].isUndefined()) replace_diff_x(mstruct, vargs[1], vargs[3]);
	return 1;
}

RombergFunction::RombergFunction() : MathFunction("romberg", 3, 6) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	NON_COMPLEX_NUMBER_ARGUMENT(2)
	NON_COMPLEX_NUMBER_ARGUMENT(3)
	setCondition("\\z > \\y");
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG);
	Number nr(2, 1);
	iarg->setMin(&nr);
	setArgumentDefinition(4, iarg);
	setDefaultValue(4, "6");
	setArgumentDefinition(5, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SLONG));
	setDefaultValue(5, "20");
	setArgumentDefinition(6, new SymbolicArgument());
	setDefaultValue(6, "undefined");
}
int RombergFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure minteg(vargs[0]);
	EvaluationOptions eo2 = eo;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	Number nr_interval;
	nr_interval.setInterval(vargs[1].number(), vargs[2].number());
	UnknownVariable *var = new UnknownVariable("", format_and_print(vargs[5]));
	var->setInterval(nr_interval);
	MathStructure x_var(var);
	minteg.replace(vargs[5], x_var);
	var->destroy();
	minteg.eval(eo2);
	Number nr;
	eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
	eo2.warn_about_denominators_assumed_nonzero = false;
	CALCULATOR->beginTemporaryStopMessages();
	if(romberg(minteg, nr, x_var, eo2, vargs[1].number(), vargs[2].number(), vargs[4].number().lintValue(), vargs[3].number().lintValue(), false)) {
		CALCULATOR->endTemporaryStopMessages();
		mstruct = nr;
		return 1;
	}
	CALCULATOR->endTemporaryStopMessages();
	CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
	return 0;
}
MonteCarloFunction::MonteCarloFunction() : MathFunction("montecarlo", 4, 5) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	NON_COMPLEX_NUMBER_ARGUMENT(2)
	NON_COMPLEX_NUMBER_ARGUMENT(3)
	setCondition("\\z > \\y");
	setArgumentDefinition(4, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE));
	setArgumentDefinition(5, new SymbolicArgument());
	setDefaultValue(5, "undefined");
}
int MonteCarloFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure minteg(vargs[0]);
	EvaluationOptions eo2 = eo;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	Number nr_interval;
	nr_interval.setInterval(vargs[1].number(), vargs[2].number());
	UnknownVariable *var = new UnknownVariable("", format_and_print(vargs[4]));
	var->setInterval(nr_interval);
	MathStructure x_var(var);
	minteg.replace(vargs[4], x_var);
	var->destroy();
	minteg.eval(eo2);
	Number nr;
	eo2.interval_calculation = INTERVAL_CALCULATION_NONE;
	if(montecarlo(minteg, nr, x_var, eo2, vargs[1].number(), vargs[2].number(), vargs[3].number())) {
		mstruct = nr;
		return 1;
	}
	CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
	return 0;
}

IntegrateFunction::IntegrateFunction() : MathFunction("integrate", 1, 5) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	setDefaultValue(2, "undefined");
	arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
	setDefaultValue(3, "undefined");
	arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(3, arg);
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "undefined");
	setArgumentDefinition(5, new BooleanArgument());
	setDefaultValue(5, "0");
}
int IntegrateFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	if(vargs.size() >= 2 && (vargs.size() == 2 || vargs[2].isUndefined()) && (vargs[1].isSymbolic() || (vargs[1].isVariable() && !vargs[1].variable()->isKnown()))) {
		if(mstruct.integrate(vargs.size() < 3 ? m_undefined : vargs[2], vargs.size() < 4 ? m_undefined : vargs[3], vargs[1], eo, vargs.size() >= 4 && vargs[4].number().getBoolean(), true)) return 1;
	} else {
		if(mstruct.integrate(vargs.size() < 2 ? m_undefined : vargs[1], vargs.size() < 3 ? m_undefined : vargs[2], vargs.size() < 3 ? CALCULATOR->getVariableById(VARIABLE_ID_X) : vargs[3], eo, vargs.size() >= 4 && vargs[4].number().getBoolean(), true)) return 1;
	}
	return -1;
}

