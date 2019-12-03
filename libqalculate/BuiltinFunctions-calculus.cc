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
#include "mathstructure-support.h"

#include <sstream>
#include <time.h>
#include <limits>
#include <algorithm>

using std::string;
using std::cout;
using std::vector;
using std::endl;

#define FR_FUNCTION(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}
#define FR_FUNCTION_2(FUNC)	Number nr(vargs[0].number()); if(!nr.FUNC(vargs[1].number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !vargs[0].isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex() && !vargs[1].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity() && !vargs[1].number().includesInfinity())) {return 0;} else {mstruct.set(nr); return 1;}

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
			mstruct.set(CALCULATOR->v_pi, true);
			mstruct *= nr_half;
			return 1;
		}
		if(nr.isMinusInfinity()) {
			mstruct.set(CALCULATOR->v_pi, true);
			mstruct *= nr_minus_half;
			return 1;
		}
		if(nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			mstruct.transform(CALCULATOR->f_Shi);
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
			mstruct += CALCULATOR->v_pi;
			mstruct.last() *= nr_one_i;
			return 1;
		}
		Number nr(mstruct.number());
		if(nr.isComplex() && nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			if(nr.imaginaryPartIsNegative()) mstruct.negate();
			mstruct.transform(CALCULATOR->f_Chi);
			mstruct += CALCULATOR->v_pi;
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
			mstruct.transform(CALCULATOR->f_Si);
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
			mstruct += CALCULATOR->v_pi;
			mstruct.last() *= nr_one_i;
			return 1;
		}
		Number nr(mstruct.number());
		if(nr.isComplex() && nr.hasImaginaryPart() && !nr.hasRealPart()) {
			mstruct.set(nr.imaginaryPart());
			if(nr.imaginaryPartIsNegative()) mstruct.negate();
			mstruct.transform(CALCULATOR->f_Ci);
			mstruct += CALCULATOR->v_pi;
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
	if(m.type() == STRUCT_FUNCTION && m.function() == CALCULATOR->f_diff) {
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

