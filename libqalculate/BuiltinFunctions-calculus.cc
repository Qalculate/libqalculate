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

