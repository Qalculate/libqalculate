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

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool test_eval(MathStructure &mtest, const EvaluationOptions &eo) {
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	CALCULATOR->beginTemporaryEnableIntervalArithmetic();
	if(!CALCULATOR->usesIntervalArithmetic()) {CALCULATOR->endTemporaryEnableIntervalArithmetic(); return false;}
	CALCULATOR->beginTemporaryStopMessages();
	mtest.calculateFunctions(eo2);
	mtest.calculatesub(eo2, eo2, true);
	CALCULATOR->endTemporaryEnableIntervalArithmetic();
	if(CALCULATOR->endTemporaryStopMessages()) return false;
	return true;
}

bool calculate_arg(MathStructure &mstruct, const EvaluationOptions &eo) {

	MathStructure msave;

	if(!mstruct.isNumber()) {
		if(mstruct.isPower() && mstruct[0] == CALCULATOR->v_e && mstruct[1].isNumber() && mstruct[1].number().hasImaginaryPart() && !mstruct[1].number().hasRealPart()) {
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				Number nr(*mstruct[1].number().internalImaginary());
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
					mstruct = mstruct[1].number().imaginaryPart();
					if(!nr.isZero()) {
						mstruct += nr;
						mstruct.last() *= CALCULATOR->v_pi;
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
				return false;
			}
		}
	}
	if(mstruct.isNumber()) {
		if(!mstruct.number().hasImaginaryPart()) {
			return false;
		} else if(!mstruct.number().hasRealPart() && mstruct.number().imaginaryPartIsNonZero()) {
			bool b_neg = mstruct.number().imaginaryPartIsNegative();
			mstruct.set(CALCULATOR->v_pi); mstruct.multiply(nr_half);
			if(b_neg) mstruct.negate();
		} else if(!msave.isZero()) {
			mstruct = msave;
			return false;
		} else if(!mstruct.number().realPartIsNonZero()) {
			return false;
		} else {
			MathStructure new_nr(mstruct.number().imaginaryPart());
			if(!new_nr.number().divide(mstruct.number().realPart())) return false;
			if(mstruct.number().realPartIsNegative()) {
				if(mstruct.number().imaginaryPartIsNegative()) {
					mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
					switch(eo.parse_options.angle_unit) {
						case ANGLE_UNIT_DEGREES: {mstruct.divide_nocopy(new MathStructure(180, 1, 0)); mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi)); break;}
						case ANGLE_UNIT_GRADIANS: {mstruct.divide_nocopy(new MathStructure(200, 1, 0)); mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi)); break;}
						case ANGLE_UNIT_RADIANS: {break;}
						default: {if(CALCULATOR->getRadUnit()) {mstruct /= CALCULATOR->getRadUnit();} break;}
					}
					mstruct.subtract(CALCULATOR->v_pi);
				} else if(mstruct.number().imaginaryPartIsNonNegative()) {
					mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
					switch(eo.parse_options.angle_unit) {
						case ANGLE_UNIT_DEGREES: {mstruct.divide_nocopy(new MathStructure(180, 1, 0)); mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi)); break;}
						case ANGLE_UNIT_GRADIANS: {mstruct.divide_nocopy(new MathStructure(200, 1, 0)); mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi)); break;}
						case ANGLE_UNIT_RADIANS: {break;}
						default: {if(CALCULATOR->getRadUnit()) {mstruct /= CALCULATOR->getRadUnit();} break;}
					}
					mstruct.add(CALCULATOR->v_pi);
				} else {
					return false;
				}
			} else {
				mstruct.set(CALCULATOR->f_atan, &new_nr, NULL);
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {mstruct.divide_nocopy(new MathStructure(180, 1, 0)); mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi)); break;}
					case ANGLE_UNIT_GRADIANS: {mstruct.divide_nocopy(new MathStructure(200, 1, 0)); mstruct.multiply_nocopy(new MathStructure(CALCULATOR->v_pi)); break;}
					case ANGLE_UNIT_RADIANS: {break;}
					default: {if(CALCULATOR->getRadUnit()) {mstruct /= CALCULATOR->getRadUnit();} break;}
				}
			}
		}
		return true;
	}
	if(!msave.isZero()) {
		mstruct = msave;
	}
	return false;

}

SqrtFunction::SqrtFunction() : MathFunction("sqrt", 1) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int SqrtFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	if(!vargs[0].representsScalar()) {
		mstruct.eval(eo);
		if(mstruct.isVector()) return -1;
	}
	mstruct.raise(nr_half);
	return 1;
}
bool SqrtFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsPositive(allow_units);}
bool SqrtFunction::representsNegative(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonNegative(allow_units);}
bool SqrtFunction::representsNonPositive(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool SqrtFunction::representsRational(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonNegative(allow_units);}
bool SqrtFunction::representsNonComplex(const MathStructure &vargs, bool allow_units) const {return representsReal(vargs, allow_units);}
bool SqrtFunction::representsComplex(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNegative(allow_units);}
bool SqrtFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNonZero(allow_units);}
bool SqrtFunction::representsEven(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool SqrtFunction::representsUndefined(const MathStructure&) const {return false;}
CbrtFunction::CbrtFunction() : MathFunction("cbrt", 1) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int CbrtFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[0].representsNegative(true)) {
		mstruct = vargs[0];
		mstruct.negate();
		mstruct.raise(Number(1, 3, 0));
		mstruct.negate();
	} else if(vargs[0].representsNonNegative(true)) {
		mstruct = vargs[0];
		mstruct.raise(Number(1, 3, 0));
	} else {
		MathStructure mroot(3, 1, 0);
		mstruct.set(CALCULATOR->f_root, &vargs[0], &mroot, NULL);
	}
	return 1;
}
RootFunction::RootFunction() : MathFunction("root", 2) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	NumberArgument *arg2 = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, true);
	arg2->setComplexAllowed(false);
	arg2->setRationalNumber(true);
	arg2->setHandleVector(true);
	setArgumentDefinition(2, arg2);
}
int RootFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	if(vargs[1].number().isOne()) {
		mstruct = vargs[0];
		return 1;
	}
	if(!vargs[1].number().isInteger() || !vargs[1].number().isPositive()) {
		mstruct = vargs[0];
		if(!vargs[0].representsScalar()) {
			mstruct.eval(eo);
		}
		if(mstruct.isVector()) return -1;
		Number nr_root(vargs[1].number().numerator());
		nr_root.setPrecisionAndApproximateFrom(vargs[1].number());
		Number nr_pow(vargs[1].number().denominator());
		nr_pow.setPrecisionAndApproximateFrom(vargs[1].number());
		if(nr_root.isNegative()) {
			nr_root.negate();
			nr_pow.negate();
		}
		if(nr_root.isOne()) {
			mstruct ^= nr_pow;
		} else if(nr_root.isZero()) {
			mstruct ^= nr_zero;
		} else {
			mstruct ^= nr_pow;
			mstruct.transform(this);
			mstruct.addChild(nr_root);
		}
		return 1;
	}
	if(vargs[0].representsNonNegative(true)) {
		mstruct = vargs[0];
		Number nr_exp(vargs[1].number());
		nr_exp.recip();
		mstruct.raise(nr_exp);
		return 1;
	} else if(vargs[1].number().isOdd() && vargs[0].representsNegative(true)) {
		mstruct = vargs[0];
		mstruct.negate();
		Number nr_exp(vargs[1].number());
		nr_exp.recip();
		mstruct.raise(nr_exp);
		mstruct.negate();
		return 1;
	}
	bool eval_mstruct = !vargs[0].isNumber();
	Number nr;
	if(eval_mstruct) {
		mstruct = vargs[0];
		if(eo.approximation == APPROXIMATION_TRY_EXACT && mstruct.representsScalar()) {
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_EXACT;
			mstruct.eval(eo2);
		} else {
			mstruct.eval(eo);
		}
		if(mstruct.isVector()) {
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return -1;
		}
		if(mstruct.representsNonNegative(true)) {
			Number nr_exp(vargs[1].number());
			nr_exp.recip();
			mstruct.raise(nr_exp);
			return 1;
		} else if(vargs[1].number().isOdd() && mstruct.representsNegative(true)) {
			mstruct.negate();
			Number nr_exp(vargs[1].number());
			nr_exp.recip();
			mstruct.raise(nr_exp);
			mstruct.negate();
			return 1;
		}
		if(!mstruct.isNumber()) {
			if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isInteger()) {
				if(mstruct[1] == vargs[1]) {
					if(mstruct[1].number().isEven()) {
						if(!mstruct[0].representsReal(true)) return -1;
						mstruct.delChild(2);
						mstruct.setType(STRUCT_FUNCTION);
						mstruct.setFunction(CALCULATOR->f_abs);
					} else {
						mstruct.setToChild(1);
					}
					return 1;
				} else if(mstruct[1].number().isIntegerDivisible(vargs[1].number())) {
					if(mstruct[1].number().isEven()) {
						if(!mstruct[0].representsReal(true)) return -1;
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
					}
					mstruct[1].divide(vargs[1].number());
					return 1;
				} else if(!mstruct[1].number().isMinusOne() && vargs[1].number().isIntegerDivisible(mstruct[1].number())) {
					if(mstruct[1].number().isEven()) {
						if(!mstruct[0].representsReal(true)) return -1;
						mstruct[0].transform(STRUCT_FUNCTION);
						mstruct[0].setFunction(CALCULATOR->f_abs);
					}
					Number new_root(vargs[1].number());
					new_root.divide(mstruct[1].number());
					bool bdiv = new_root.isNegative();
					if(bdiv) new_root.negate();
					mstruct[1] = new_root;
					mstruct.setType(STRUCT_FUNCTION);
					mstruct.setFunction(this);
					if(bdiv) mstruct.raise(nr_minus_one);
					return 1;
				} else if(mstruct[1].number().isMinusOne()) {
					mstruct[0].transform(STRUCT_FUNCTION, vargs[1]);
					mstruct[0].setFunction(this);
					return 1;
				} else if(mstruct[1].number().isNegative()) {
					mstruct[1].number().negate();
					mstruct.transform(STRUCT_FUNCTION, vargs[1]);
					mstruct.setFunction(this);
					mstruct.raise(nr_minus_one);
					return 1;
				}
			} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsTwo()) {
				Number new_root(vargs[1].number());
				new_root.multiply(Number(2, 1, 0));
				if(mstruct[1].number().numeratorIsOne()) {
					mstruct[1].number().set(new_root, true);
					mstruct.setType(STRUCT_FUNCTION);
					mstruct.setFunction(this);
				} else {
					mstruct[1].number().set(mstruct[1].number().numerator(), true);
					mstruct.transform(STRUCT_FUNCTION);
					mstruct.addChild(new_root);
					mstruct.setFunction(this);
				}
				return 1;
			} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_sqrt && mstruct.size() == 1) {
				Number new_root(vargs[1].number());
				new_root.multiply(Number(2, 1, 0));
				mstruct.addChild(new_root);
				mstruct.setFunction(this);
				return 1;
			} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_root && mstruct.size() == 2 && mstruct[1].isNumber() && mstruct[1].number().isInteger() && mstruct[1].number().isPositive()) {
				Number new_root(vargs[1].number());
				new_root.multiply(mstruct[1].number());
				mstruct[1] = new_root;
				return 1;
			} else if(mstruct.isMultiplication()) {
				bool b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					if(!mstruct[i].representsReal(true)) {
						b = false;
						break;
					}
				}
				if(b) {
					if(vargs[1].number().isOdd()) {
						bool b_neg = false;
						for(size_t i = 0; i < mstruct.size(); i++) {
							if(mstruct[i].isNumber() && mstruct[i].number().isNegative() && !mstruct[i].isMinusOne()) {
								mstruct[i].negate();
								b_neg = !b_neg;
							}
							mstruct[i].transform(STRUCT_FUNCTION, vargs[1]);
							mstruct[i].setFunction(this);
						}
						if(b_neg) mstruct.insertChild_nocopy(new MathStructure(-1, 1, 0), 1);
						return 1;
					} else {
						for(size_t i = 0; i < mstruct.size(); i++) {
							if(mstruct[i].isNumber() && mstruct[i].number().isNegative() && !mstruct[i].isMinusOne()) {
								MathStructure *mmul = new MathStructure(this, &mstruct[i], &vargs[1], NULL);
								(*mmul)[0].negate();
								mstruct[i] = nr_minus_one;
								mstruct.transform(STRUCT_FUNCTION, vargs[1]);
								mstruct.setFunction(this);
								mstruct.multiply_nocopy(mmul);
								return true;
							} else if(mstruct[i].representsPositive()) {
								mstruct[i].transform(STRUCT_FUNCTION, vargs[1]);
								mstruct[i].setFunction(this);
								mstruct[i].ref();
								MathStructure *mmul = &mstruct[i];
								mstruct.delChild(i + 1, true);
								mstruct.transform(STRUCT_FUNCTION, vargs[1]);
								mstruct.setFunction(this);
								mstruct.multiply_nocopy(mmul);
								return true;
							}
						}
					}
				}
			}
			return -1;
		}
		nr = mstruct.number();
	} else {
		nr = vargs[0].number();
	}
	if(!nr.root(vargs[1].number()) || (eo.approximation < APPROXIMATION_APPROXIMATE && nr.isApproximate() && !vargs[0].isApproximate() && !mstruct.isApproximate() && !vargs[1].isApproximate()) || (!eo.allow_complex && nr.isComplex() && !vargs[0].number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !vargs[0].number().includesInfinity())) {
		if(!eval_mstruct) {
			if(vargs[0].number().isNegative() && vargs[1].number().isOdd()) {
				mstruct.set(this, &vargs[0], &vargs[1], NULL);
				mstruct[0].number().negate();
				mstruct.negate();
				return 1;
			}
			return 0;
		} else if(mstruct.number().isNegative() && vargs[1].number().isOdd()) {
			mstruct.number().negate();
			mstruct.transform(STRUCT_FUNCTION, vargs[1]);
			mstruct.setFunction(this);
			mstruct.negate();
			return 1;
		}
	} else {
		mstruct.set(nr);
		return 1;
	}
	return -1;
}
bool RootFunction::representsPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsPositive(allow_units);}
bool RootFunction::representsNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsOdd() && vargs[1].representsPositive() && vargs[0].representsNegative(allow_units);}
bool RootFunction::representsNonNegative(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsNonNegative(allow_units);}
bool RootFunction::representsNonPositive(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsOdd() && vargs[1].representsPositive() && vargs[0].representsNonPositive(allow_units);}
bool RootFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool RootFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsNumber(allow_units);}
bool RootFunction::representsRational(const MathStructure&, bool) const {return false;}
bool RootFunction::representsReal(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsReal(allow_units) && (vargs[0].representsNonNegative(allow_units) || vargs[1].representsOdd());}
bool RootFunction::representsNonComplex(const MathStructure &vargs, bool allow_units) const {return representsReal(vargs, allow_units);}
bool RootFunction::representsComplex(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && (vargs[0].representsComplex(allow_units) || (vargs[1].representsEven() && vargs[0].representsNegative(allow_units)));}
bool RootFunction::representsNonZero(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 2 && vargs[1].representsInteger() && vargs[1].representsPositive() && vargs[0].representsNonZero(allow_units);}
bool RootFunction::representsEven(const MathStructure&, bool) const {return false;}
bool RootFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool RootFunction::representsUndefined(const MathStructure&) const {return false;}

SquareFunction::SquareFunction() : MathFunction("sq", 1) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int SquareFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = vargs[0];
	mstruct ^= 2;
	return 1;
}

ExpFunction::ExpFunction() : MathFunction("exp", 1) {
	Argument *arg = new Argument("", false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int ExpFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	mstruct = CALCULATOR->v_e;
	mstruct ^= vargs[0];
	return 1;
}

LogFunction::LogFunction() : MathFunction("ln", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool LogFunction::representsPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive() && ((vargs[0].isNumber() && vargs[0].number().isGreaterThan(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && ((KnownVariable*) vargs[0].variable())->get().number().isGreaterThan(nr_one)));}
bool LogFunction::representsNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative() && ((vargs[0].isNumber() && vargs[0].number().isLessThan(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && ((KnownVariable*) vargs[0].variable())->get().number().isLessThan(nr_one)));}
bool LogFunction::representsNonNegative(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive() && ((vargs[0].isNumber() && vargs[0].number().isGreaterThanOrEqualTo(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && ((KnownVariable*) vargs[0].variable())->get().number().isGreaterThanOrEqualTo(nr_one)));}
bool LogFunction::representsNonPositive(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative() && ((vargs[0].isNumber() && vargs[0].number().isLessThanOrEqualTo(nr_one)) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && ((KnownVariable*) vargs[0].variable())->get().number().isLessThanOrEqualTo(nr_one)));}
bool LogFunction::representsInteger(const MathStructure&, bool) const {return false;}
bool LogFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units) && vargs[0].representsNonZero(allow_units);}
bool LogFunction::representsRational(const MathStructure&, bool) const {return false;}
bool LogFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsPositive();}
bool LogFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNonNegative();}
bool LogFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal() && vargs[0].representsNegative();}
bool LogFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (vargs[0].representsNonPositive() || (vargs[0].isNumber() && COMPARISON_IS_NOT_EQUAL(vargs[0].number().compare(nr_one))) || (vargs[0].isVariable() && vargs[0].variable()->isKnown() && ((KnownVariable*) vargs[0].variable())->get().isNumber() && COMPARISON_IS_NOT_EQUAL(((KnownVariable*) vargs[0].variable())->get().number().compare(nr_one))));}
bool LogFunction::representsEven(const MathStructure&, bool) const {return false;}
bool LogFunction::representsOdd(const MathStructure&, bool) const {return false;}
bool LogFunction::representsUndefined(const MathStructure&) const {return false;}
int LogFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;

	mstruct = vargs[0];

	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_e) {
		mstruct.set(m_one);
		return true;
	} else if(mstruct.isPower()) {
		if(mstruct[0].isVariable() && mstruct[0].variable() == CALCULATOR->v_e) {
			if(mstruct[1].representsReal()) {
				mstruct.setToChild(2, true);
				return true;
			}
		} else if(eo.approximation != APPROXIMATION_APPROXIMATE && ((mstruct[0].representsPositive(true) && mstruct[1].representsReal()) || (mstruct[1].isNumber() && mstruct[1].number().isFraction()))) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
			mstruct2 *= mstruct[1];
			mstruct = mstruct2;
			return true;
		}
	}

	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		if(mstruct.isVector()) {CALCULATOR->endTemporaryStopMessages(true); return -1;}
	} else {
		mstruct.eval(eo);
		if(mstruct.isVector()) return -1;
	}

	bool b = false;
	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_e) {
		mstruct.set(m_one);
		b = true;
	} else if(mstruct.isPower()) {
		if(mstruct[0].isVariable() && mstruct[0].variable() == CALCULATOR->v_e) {
			if(mstruct[1].representsReal()) {
				mstruct.setToChild(2, true);
				b = true;
			}
		} else if((mstruct[0].representsPositive(true) && mstruct[1].representsReal()) || (mstruct[1].isNumber() && mstruct[1].number().isFraction())) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
			mstruct2 *= mstruct[1];
			mstruct = mstruct2;
			b = true;
		}
	} else if(mstruct.isMultiplication()) {
		b = true;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].representsPositive()) {
				b = false;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
			for(size_t i = 1; i < mstruct.size(); i++) {
				mstruct2.add(MathStructure(CALCULATOR->f_ln, &mstruct[i], NULL), i > 1);
			}
			mstruct = mstruct2;
		}
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(b);
	if(b) return 1;
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		mstruct.eval(eo2);
	}

	if(mstruct.isNumber()) {
		if(eo.allow_complex && mstruct.number().isMinusOne()) {
			mstruct = CALCULATOR->v_i->get();
			mstruct *= CALCULATOR->v_pi;
			return 1;
		} else if(mstruct.number().isI()) {
			mstruct.set(1, 2, 0);
			mstruct *= CALCULATOR->v_pi;
			mstruct *= CALCULATOR->v_i->get();
			return 1;
		} else if(mstruct.number().isMinusI()) {
			mstruct.set(-1, 2, 0);
			mstruct *= CALCULATOR->v_pi;
			mstruct *= CALCULATOR->v_i->get();
			return 1;
		} else if(eo.allow_complex && eo.allow_infinite && mstruct.number().isMinusInfinity()) {
			mstruct = CALCULATOR->v_pi;
			mstruct *= CALCULATOR->v_i->get();
			Number nr; nr.setPlusInfinity();
			mstruct += nr;
			return 1;
		}
		Number nr(mstruct.number());
		if(nr.ln() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
		if(mstruct.number().isRational() && mstruct.number().isPositive()) {
			if(mstruct.number().isInteger()) {
				if(mstruct.number().isLessThanOrEqualTo(PRIMES[NR_OF_PRIMES - 1])) {
					vector<Number> factors;
					mstruct.number().factorize(factors);
					if(factors.size() > 1) {
						mstruct.clear(true);
						mstruct.setType(STRUCT_ADDITION);
						for(size_t i = 0; i < factors.size(); i++) {
							if(i > 0 && factors[i] == factors[i - 1]) {
								if(mstruct.last().isMultiplication()) mstruct.last().last().number()++;
								else mstruct.last() *= nr_two;
							} else {
								mstruct.addChild(MathStructure(CALCULATOR->f_ln, NULL));
								mstruct.last().addChild(factors[i]);
							}
						}
						if(mstruct.size() == 1) mstruct.setToChild(1, true);
						return 1;
					}
				}
			} else {
				MathStructure mstruct2(CALCULATOR->f_ln, NULL);
				mstruct2.addChild(mstruct.number().denominator());
				mstruct.number().set(mstruct.number().numerator());
				mstruct.transform(CALCULATOR->f_ln);
				mstruct -= mstruct2;
				return 1;
			}
		} else if(mstruct.number().hasImaginaryPart()) {
			if(mstruct.number().hasRealPart()) {
				MathStructure *marg = new MathStructure(mstruct);
				if(calculate_arg(*marg, eo)) {
					mstruct.transform(CALCULATOR->f_abs);
					mstruct.transform(CALCULATOR->f_ln);
					marg->multiply(CALCULATOR->v_i->get());
					mstruct.add_nocopy(marg);
					return 1;
				}
				marg->unref();
			} else {
				bool b_neg = mstruct.number().imaginaryPartIsNegative();
				if(mstruct.number().abs()) {
					mstruct.transform(this);
					mstruct += b_neg ? nr_minus_half : nr_half;
					mstruct.last() *= CALCULATOR->v_pi;
					mstruct.last().multiply(CALCULATOR->v_i->get(), true);
				}
				return 1;
			}
		}
	} else if(mstruct.isPower()) {
		if((mstruct[0].representsPositive(true) && mstruct[1].representsReal()) || (mstruct[1].isNumber() && mstruct[1].number().isFraction())) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
			mstruct2 *= mstruct[1];
			mstruct = mstruct2;
			return 1;
		}
		if(mstruct[1].isNumber() && mstruct[1].number().isTwo() && mstruct[0].representsPositive()) {
			mstruct.setToChild(1, true);
			mstruct.transform(this);
			mstruct *= nr_two;
			return 1;
		}
		if(eo.approximation == APPROXIMATION_EXACT && !mstruct[1].isNumber()) {
			CALCULATOR->beginTemporaryStopMessages();
			MathStructure mtest(mstruct[1]);
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			mtest.eval(eo2);
			if(!CALCULATOR->endTemporaryStopMessages() && mtest.isNumber() && mtest.number().isFraction()) {
				MathStructure mstruct2;
				mstruct2.set(CALCULATOR->f_ln, &mstruct[0], NULL);
				mstruct2 *= mstruct[1];
				mstruct = mstruct2;
				return 1;
			}
		}
	}
	if(eo.allow_complex && mstruct.representsNegative()) {
		mstruct.negate();
		mstruct.transform(CALCULATOR->f_ln);
		mstruct += CALCULATOR->v_pi;
		mstruct.last() *= nr_one_i;
		return 1;
	}
	if(!mstruct.representsReal()) {
		MathStructure *marg = new MathStructure(mstruct);
		if(calculate_arg(*marg, eo)) {
			mstruct.transform(CALCULATOR->f_abs);
			mstruct.transform(CALCULATOR->f_ln);
			marg->multiply(CALCULATOR->v_i->get());
			mstruct.add_nocopy(marg);
			return 1;
		}
		marg->unref();
	}

	return -1;

}
LognFunction::LognFunction() : MathFunction("log", 1, 2) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONZERO, false);
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
	setDefaultValue(2, "e");
}
int LognFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector() || vargs[1].isVector()) return 0;
	if(vargs[1].isVariable() && vargs[1].variable() == CALCULATOR->v_e) {
		mstruct.set(CALCULATOR->f_ln, &vargs[0], NULL);
		return 1;
	}
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	MathStructure mstructv2 = vargs[1];
	mstructv2.eval(eo);
	if(mstructv2.isVector()) return -2;
	if(mstruct.isPower()) {
		if((mstruct[0].representsPositive(true) && mstruct[1].representsReal()) || (mstruct[1].isNumber() && mstruct[1].number().isFraction())) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_logn, &mstruct[0], &mstructv2, NULL);
			mstruct2 *= mstruct[1];
			mstruct = mstruct2;
			return 1;
		}
	} else if(mstruct.isMultiplication()) {
		bool b = true;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].representsPositive()) {
				b = false;
				break;
			}
		}
		if(b) {
			MathStructure mstruct2;
			mstruct2.set(CALCULATOR->f_logn, &mstruct[0], &mstructv2, NULL);
			for(size_t i = 1; i < mstruct.size(); i++) {
				mstruct2.add(MathStructure(CALCULATOR->f_logn, &mstruct[i], &mstructv2, NULL), i > 1);
			}
			mstruct = mstruct2;
			return 1;
		}
	} else if(mstruct.isNumber() && mstructv2.isNumber()) {
		Number nr(mstruct.number());
		if(nr.log(mstructv2.number()) && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	mstruct.set(CALCULATOR->f_ln, &vargs[0], NULL);
	mstruct.divide_nocopy(new MathStructure(CALCULATOR->f_ln, &vargs[1], NULL));
	return 1;
}
CisFunction::CisFunction() : MathFunction("cis", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int CisFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	if(vargs[0].isVector()) return 0;
	if(contains_angle_unit(vargs[0], eo.parse_options)) {
		if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][1] == CALCULATOR->getRadUnit()) {
			mstruct = vargs[0][0];
		} else if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0] == CALCULATOR->getRadUnit()) {
			mstruct = vargs[0][1];
		} else if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][1] == CALCULATOR->getDegUnit()) {
			mstruct = vargs[0][0];
			mstruct *= CALCULATOR->v_pi;
			mstruct.multiply(Number(1, 180), true);
		} else if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0] == CALCULATOR->getDegUnit()) {
			mstruct = vargs[0][1];
			mstruct *= CALCULATOR->v_pi;
			mstruct.multiply(Number(1, 180), true);
		} else if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][1] == CALCULATOR->getGraUnit()) {
			mstruct = vargs[0][0];
			mstruct *= CALCULATOR->v_pi;
			mstruct.multiply(Number(1, 200), true);
		} else if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0] == CALCULATOR->getGraUnit()) {
			mstruct = vargs[0][1];
			mstruct *= CALCULATOR->v_pi;
			mstruct.multiply(Number(1, 200), true);
		} else {
			mstruct = vargs[0];
			mstruct.convert(CALCULATOR->getRadUnit());
			mstruct /= CALCULATOR->getRadUnit();
		}
	} else {
		mstruct = vargs[0];
	}

	if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_pi) {
		mstruct.set(-1, 1, 0, true);
		return 1;
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isInteger() && mstruct[1].isVariable() && mstruct[1].variable() == CALCULATOR->v_pi) {
		if(mstruct[0].number().isEven()) {
			mstruct.set(1, 1, 0, true);
			return 1;
		} else if(mstruct[0].number().isOdd()) {
			mstruct.set(-1, 1, 0, true);
			return 1;
		}
	}

	mstruct *= CALCULATOR->v_i;
	mstruct ^= CALCULATOR->v_e;
	mstruct.swapChildren(1, 2);

	return 1;

}
LambertWFunction::LambertWFunction() : MathFunction("lambertw", 1, 2) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	setArgumentDefinition(2, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
	setDefaultValue(2, "0");
}
bool LambertWFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[0].representsNumber() && (vargs[1].isZero() || vargs[0].representsNonZero()));}
bool LambertWFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[1].isZero() && vargs[0].representsNonNegative());}
bool LambertWFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[0].isZero() || (vargs[1].isZero() && vargs[0].representsNonNegative()));}
bool LambertWFunction::representsComplex(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[0].representsComplex() || (vargs[0].representsNonZero() && (vargs[1].isInteger() && (!vargs[1].isMinusOne() || vargs[0].representsPositive()) && !vargs[1].isZero())));}
bool LambertWFunction::representsNonZero(const MathStructure &vargs, bool) const {return vargs.size() == 2 && (vargs[1].representsNonZero() || vargs[0].representsNonZero());}
int LambertWFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];

	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
		if(mstruct.isVector()) {CALCULATOR->endTemporaryStopMessages(true); return -1;}
	} else {
		mstruct.eval(eo);
		if(mstruct.isVector()) return -1;
	}

	bool b = false;
	if(!vargs[1].isZero()) {
		if(mstruct.isZero()) {
			mstruct.set(nr_minus_inf, true);
			b = true;
		} else if(vargs[1].isMinusOne()) {
			if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isPower() && mstruct[1][0].isVariable() && mstruct[1][0].variable() == CALCULATOR->v_e && mstruct[0].number() <= nr_minus_one && mstruct[1][1] == mstruct[0]) {
				mstruct.setToChild(1, true);
				b = true;
			}
		}

	} else {
		if(mstruct.isZero()) {
			b = true;
		} else if(mstruct.isVariable() && mstruct.variable() == CALCULATOR->v_e) {
			mstruct.set(1, 1, 0, true);
			b = true;
		} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isMinusOne() && mstruct[1].isPower() && mstruct[1][0].isVariable() && mstruct[1][0].variable() == CALCULATOR->v_e && mstruct[1][1].isMinusOne()) {
			mstruct.set(-1, 1, 0, true);
			b = true;
		}
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(b);
	if(b) return 1;
	if(eo.approximation == APPROXIMATION_EXACT) return -1;
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mstruct = vargs[0];
		mstruct.eval(eo2);
	}
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.lambertW(vargs[1].number()) && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	return -1;
}

