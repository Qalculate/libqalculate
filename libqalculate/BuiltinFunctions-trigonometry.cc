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

bool is_real_angle_value(const MathStructure &mstruct) {
	if(mstruct.isUnit()) {
		return mstruct.unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit();
	} else if(mstruct.isMultiplication()) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b && mstruct[i].isUnit()) {
				if(mstruct[i].unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit()) {
					b = true;
				} else {
					return false;
				}
			} else if(!mstruct[i].representsReal()) {
				return false;
			}
		}
		return b;
	} else if(mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!is_real_angle_value(mstruct[i])) return false;
		}
		return true;
	}
	return false;
}
bool is_infinite_angle_value(const MathStructure &mstruct) {
	if(mstruct.isMultiplication() && mstruct.size() == 2) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b && mstruct[i].isUnit()) {
				if(mstruct[i].unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit()) {
					b = true;
				} else {
					return false;
				}
			} else if(!mstruct[i].isNumber() || !mstruct[i].number().isInfinite()) {
				return false;
			}
		}
		return b;
	}
	return false;
}
bool is_number_angle_value(const MathStructure &mstruct, bool allow_infinity = false) {
	if(mstruct.isUnit()) {
		return mstruct.unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit();
	} else if(mstruct.isMultiplication()) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b && mstruct[i].isUnit()) {
				if(mstruct[i].unit()->baseUnit() == CALCULATOR->getRadUnit()->baseUnit()) {
					b = true;
				} else {
					return false;
				}
			} else if(!mstruct[i].representsNumber()) {
				if(!allow_infinity || ((!mstruct[i].isNumber() || !mstruct[i].number().isInfinite()) && (!mstruct[i].isPower() || !mstruct[i][0].representsNumber() || !mstruct[i][1].representsNumber())) || mstruct[i].representsUndefined(true)) {
					return false;
				}
			}
		}
		return b;
	} else if(mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!is_number_angle_value(mstruct[i])) return false;
		}
		return true;
	}
	return false;
}

bool has_predominately_negative_sign(const MathStructure &mstruct) {
	if(mstruct.hasNegativeSign() && !mstruct.containsType(STRUCT_ADDITION, true)) return true;
	if(mstruct.containsInfinity(false, false, false) > 0) return false;
	if(mstruct.isAddition() && mstruct.size() > 0) {
		size_t p_count = 0;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].hasNegativeSign()) {
				p_count++;
				if(p_count > mstruct.size() / 2) return true;
			}
		}
		if(mstruct.size() % 2 == 0 && p_count == mstruct.size() / 2) return mstruct[0].hasNegativeSign();
	}
	return false;
}

void negate_struct(MathStructure &mstruct) {
	if(mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].negate();
		}
	} else {
		mstruct.negate();
	}
}

bool trig_remove_i(MathStructure &mstruct) {
	if(mstruct.isNumber() && mstruct.number().hasImaginaryPart() && !mstruct.number().hasRealPart()) {
		mstruct.number() /= nr_one_i;
		return true;
	} else if(mstruct.isMultiplication() && mstruct.size() > 1 && mstruct[0].isNumber() && mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().hasRealPart()) {
		mstruct[0].number() /= nr_one_i;
		return true;
	} else if(mstruct.isAddition() && mstruct.size() > 0) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!(mstruct[i].isNumber() && mstruct[i].number().hasImaginaryPart() && !mstruct[i].number().hasRealPart()) && !(mstruct[i].isMultiplication() && mstruct[i].size() > 1 && mstruct[i][0].isNumber() && mstruct[i][0].number().hasImaginaryPart() && !mstruct[i][0].number().hasRealPart())) {
				return false;
			}
		}
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isNumber()) mstruct[i].number() /= nr_one_i;
			else mstruct[i][0].number() /= nr_one_i;
		}
		return true;
	}
	return false;
}

MathStructure angle_units_in_turn(const EvaluationOptions &eo, long int num, long int den, bool recip) {
	AngleUnit au = eo.parse_options.angle_unit;
	if(au == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()) au = ANGLE_UNIT_NONE;
	switch(au) {
		case ANGLE_UNIT_DEGREES: {return Number(recip ? num : 360 * num, recip ? 360 * den : den, 0);}
		case ANGLE_UNIT_GRADIANS: {return Number(recip ? num : 400 * num, recip ? 400 * den : den, 0);}
		case ANGLE_UNIT_CUSTOM: {
			Unit *u = CALCULATOR->customAngleUnit();
			if(!u->isLocal()) {
				if(u->referenceName() == "arcmin") {
					return Number(recip ? num : 360 * 60 * num, recip ? 360 * 60 * den : den, 0);
				} else if(u->referenceName() == "arcsec") {
					return Number(recip ? num : 360 * 3600 * num, recip ? 360 * 3600 * den : den, 0);
				} else if(u->referenceName() == "turn") {
					return Number(num, den, 0);
				}
			}
			MathStructure m(recip ? num : 360 * num, recip ? 360 * den : den);
			if(recip) CALCULATOR->getDegUnit()->convert(u, m);
			else u->convert(CALCULATOR->getDegUnit(), m);
			EvaluationOptions eo2 = eo;
			eo2.calculate_functions = false;
			eo2.approximation = APPROXIMATION_EXACT;
			m.calculatesub(eo2, eo2, true);
			return m;
		}
		case ANGLE_UNIT_RADIANS: {
			if(num == 1 && den == 2) return CALCULATOR->getVariableById(VARIABLE_ID_PI);
			MathStructure m(recip ? num : num * 2, recip ? den * 2 : den, 0L);
			if(recip) m /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			else m *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			return m;
		}
		default: {
			if(num == 1 && den == 2) return CALCULATOR->getVariableById(VARIABLE_ID_PI);
			MathStructure m(recip ? num : num * 2, recip ? den * 2 : den, 0L);
			if(recip) m /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			else m *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			if(recip) m /= CALCULATOR->getRadUnit();
			else m *= CALCULATOR->getRadUnit();
			return m;
		}
	}
	return nr_zero;
}

void convert_to_radians(const MathStructure &mpre, MathStructure &mstruct, const EvaluationOptions &eo) {
	bool b = false;
	if(mpre.isMultiplication() && mpre.size() == 2) {
		for(size_t i = 1; ; i--) {
			if(mpre[i].isUnit()) {
				if(mpre[i].unit() == CALCULATOR->getRadUnit()) {
					if(i == 1) mstruct = mpre[0];
					else mstruct = mpre[1];
					b = true;
				} else if(mpre[i].unit() == CALCULATOR->getDegUnit()) {
					if(i == 1) mstruct = mpre[0];
					else mstruct = mpre[1];
					mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					mstruct.multiply(Number(1, 180), true);
					b = true;
				} else if(mpre[i].unit() == CALCULATOR->getGraUnit()) {
					if(i == 1) mstruct = mpre[0];
					else mstruct = mpre[1];
					mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					mstruct.multiply(Number(1, 200), true);
					b = true;
				} else if(mpre[i].unit()->isChildOf(CALCULATOR->getRadUnit()) && !mpre[i].unit()->hasNonlinearRelationToBase()) {
					if(i == 1) mstruct = mpre[0];
					else mstruct = mpre[1];
					if(!mpre[i].unit()->isLocal()) {
						if(mpre[i].unit()->referenceName() == "arcmin") {
							mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
							mstruct.multiply(Number(1, 180 * 60));
							b = true;
						} else if(mpre[i].unit()->referenceName() == "arcsec") {
							mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
							mstruct.multiply(Number(1, 180 * 3600));
							b = true;
						} else if(mpre[i].unit()->referenceName() == "turn") {
							mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
							mstruct.multiply(nr_two);
							b = true;
						}
					}
					if(!b) {
						CALCULATOR->getRadUnit()->convert(mpre[i].unit(), mstruct);
						EvaluationOptions eo2 = eo;
						eo2.calculate_functions = false;
						eo2.approximation = APPROXIMATION_EXACT;
						mstruct.calculatesub(eo2, eo2, true);
						b = true;
					}
				}
			}
			if(b || i == 0) break;
		}
	}
	if(!b) {
		mstruct = mpre;
		if(HAS_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit) || contains_angle_unit(mstruct, eo.parse_options, 2) != 0) {
			mstruct.convert(CALCULATOR->getRadUnit());
			mstruct /= CALCULATOR->getRadUnit();
		}
	}
}

void fix_leftover_angle_unit(MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && ((mstruct[1].isUnit() && mstruct[1].unit()->baseUnit() == CALCULATOR->getRadUnit() && mstruct[1].unit()->baseExponent() == 1) || (mstruct[1].isPower() && mstruct[1][0].isUnit() && mstruct[1][0].unit()->baseUnit() == CALCULATOR->getRadUnit() && mstruct[1][0].unit()->baseExponent() == 1 && mstruct[1][1].isMinusOne()))) {
		if((mstruct[1].isPower() && mstruct[1][0].unit() == CALCULATOR->getRadUnit()) || (mstruct[1].isUnit() && mstruct[1].unit() == CALCULATOR->getRadUnit())) {
			mstruct.setToChild(1, true);
		} else {
			mstruct.convert(CALCULATOR->getRadUnit());
			if(mstruct[1].isPower()) mstruct *= CALCULATOR->getRadUnit();
			else mstruct /= CALCULATOR->getRadUnit();
			mstruct.eval(eo);
		}
	}
}

#define TRIGONOMETRIC_FUNCTION_PREPARATIONS \
	convert_to_radians(vargs[0], mstruct, eo); \
\
	MathFunction *f = NULL;\
	if(!f && eo.approximation == APPROXIMATION_APPROXIMATE && !DEFAULT_RADIANS(eo.parse_options.angle_unit)) {\
		if(mstruct.isMultiplication() && mstruct.size() == 3 && mstruct[0].isFunction() && mstruct[0].size() == 1 && mstruct[1].isVariable() && mstruct[1].variable()->id() == VARIABLE_ID_PI && mstruct[2].isNumber() && !mstruct[2].number().isZero() && mstruct[2].equals(angle_units_in_turn(eo, 2, 1, true))) {\
			f = mstruct[0].function();\
		}\
	}\
\
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {\
		EvaluationOptions eo2 = eo;\
		eo2.approximation = APPROXIMATION_EXACT;\
		CALCULATOR->beginTemporaryStopMessages();\
		mstruct.eval(eo2);\
	} else if(!f) {\
		mstruct.eval(eo);\
	}\
\
	if(mstruct.isVector()) {\
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);\
		for(size_t i = 0; i < mstruct.size(); i++) {\
			mstruct[i] *= CALCULATOR->getRadUnit();\
		}\
		return -1;\
	}\
\
	bool b = false, b_recalc = true;\
\
	if(!DEFAULT_RADIANS(eo.parse_options.angle_unit)) {\
		if(!f && mstruct.isMultiplication() && mstruct.size() == 3 && mstruct[2].isFunction() && mstruct[2].size() == 1 && mstruct[1].isVariable() && mstruct[1].variable()->id() == VARIABLE_ID_PI && mstruct[0].equals(angle_units_in_turn(eo, 2, 1, true))) {\
			f = mstruct[2].function();\
		}\
	} else if(mstruct.isFunction() && mstruct.size() == 1) {\
		f = mstruct.function();\
	}

SinFunction::SinFunction() : MathFunction("sin", 1) {
	Argument *arg = new AngleArgument();
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool SinFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && ((allow_units && (vargs[0].representsNumber(true) || vargs[0].representsNonComplex(true))) || (!allow_units && is_number_angle_value(vargs[0], true)));}
bool SinFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (is_real_angle_value(vargs[0]) || is_infinite_angle_value(vargs[0]));}
bool SinFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
int SinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	if(vargs[0].isVector()) return 0;

	if(eo.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0].isFunction() && vargs[0][0].size() == 1 && vargs[0][1].isUnit() && vargs[0][1].unit() == CALCULATOR->customAngleUnit()) {
		if(vargs[0][0].function()->id() == FUNCTION_ID_ASIN) {
			mstruct = vargs[0][0][0];
			return 1;
		} else if(vargs[0][0].function()->id() == FUNCTION_ID_ACOS) {
			mstruct = vargs[0][0][0];
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_half;
			return 1;
		} else if(vargs[0][0].function()->id() == FUNCTION_ID_ATAN && !vargs[0][0][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct = vargs[0][0][0];
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			mstruct.multiply_nocopy(mmul);
			return 1;
		}
	}

	TRIGONOMETRIC_FUNCTION_PREPARATIONS

	if(mstruct.isVariable() && mstruct.variable()->id() == VARIABLE_ID_PI) {
		mstruct.clear();
		b = true;
	} else if(f) {
		if(f->id() == FUNCTION_ID_ASIN) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1, true);
			b = true;
		} else if(f->id() == FUNCTION_ID_ACOS) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_half;
			b = true;
		} else if(f->id() == FUNCTION_ID_ATAN && (mstruct.isFunction() ? !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true) : (mstruct[2].isFunction() ? !mstruct[2][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true) : !mstruct[0][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)))) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			mstruct.multiply_nocopy(mmul);
			b = true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isVariable() && mstruct[1].variable()->id() == VARIABLE_ID_PI) {
		if(mstruct[0].number().isInteger()) {
			mstruct.clear();
			b = true;
		} else if(!mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().includesInfinity() && !mstruct[0].number().isInterval()) {
			Number nr(mstruct[0].number());
			nr.frac();
			Number nr_int(mstruct[0].number());
			nr_int.floor();
			bool b_even = nr_int.isEven();
			nr.setNegative(false);
			if(nr.equals(Number(1, 2, 0))) {
				if(b_even) mstruct.set(1, 1, 0);
				else mstruct.set(-1, 1, 0);
				b = true;
			} else if(nr.equals(Number(1, 4, 0)) || nr.equals(Number(3, 4, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 3, 0)) || nr.equals(Number(2, 3, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 6, 0)) || nr.equals(Number(5, 6, 0))) {
				if(b_even) mstruct.set(1, 2, 0);
				else mstruct.set(-1, 2, 0);
				b = true;
			} else if(eo.approximation == APPROXIMATION_EXACT && (mstruct[0].number().isNegative() || !mstruct[0].number().isFraction() || mstruct[0].number().isGreaterThan(nr_half))) {
				nr_int = mstruct[0].number();
				nr_int.floor();
				Number nr_frac = mstruct[0].number();
				nr_frac -= nr_int;
				if(nr_frac.isGreaterThan(nr_half)) {
					nr_frac -= nr_half;
					mstruct[0].number() = nr_half;
					mstruct[0].number() -= nr_frac;
				} else {
					mstruct[0].number() = nr_frac;
				}
				if(nr_int.isOdd()) {
					mstruct *= CALCULATOR->getRadUnit();
					mstruct.transform(this);
					mstruct.negate();
					b = true;
				}
			}
		}
	} else if(mstruct.isMultiplication() && mstruct.size() >= 2) {
		bool b_pi = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isVariable() && mstruct[i].variable()->id() == VARIABLE_ID_PI) {
				b_pi = !b_pi;
				if(!b_pi) break;
			} else if(!mstruct[i].representsInteger()) {
				b_pi = false;
				break;
			}
		}
		if(b_pi) {
			mstruct.clear();
			b = true;
		}
	} else if(mstruct.isAddition()) {
		size_t i = 0;
		bool b_negate = false;
		for(; i < mstruct.size(); i++) {
			if((mstruct[i].isVariable() && mstruct[i].variable()->id() == VARIABLE_ID_PI) || (mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1].isVariable() && mstruct[i][1].variable()->id() == VARIABLE_ID_PI && mstruct[i][0].isNumber())) {
				if(contains_angle_unit(mstruct, eo.parse_options, 0)) break;
				if(mstruct[i].isVariable() || mstruct[i][0].number().isInteger()) {
					b_negate = mstruct[i].isVariable() || mstruct[i][0].number().isOdd();
					mstruct.delChild(i + 1);
					b_recalc = false;
					break;
				} else if(mstruct[i][0].number().isReal() && (mstruct[i][0].number().isNegative() || !mstruct[i][0].number().isFraction())) {
					Number nr_int = mstruct[i][0].number();
					nr_int.floor();
					mstruct[i][0].number() -= nr_int;
					b_negate = nr_int.isOdd();
					b_recalc = false;
					break;
				}
			}
		}
		b = b_negate;
		if(b_negate) {
			mstruct *= CALCULATOR->getRadUnit();
			mstruct.transform(this);
			mstruct.negate();
		}
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(b);
	if(b) return 1;
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		if(b_recalc) convert_to_radians(vargs[0], mstruct, eo2);
		mstruct.eval(eo2);
	}

	fix_leftover_angle_unit(mstruct, eo);
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.sin() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}

	if(trig_remove_i(mstruct)) {
		mstruct.transformById(FUNCTION_ID_SINH);
		mstruct *= nr_one_i;
		return 1;
	}

	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
		mstruct *= CALCULATOR->getRadUnit();
		mstruct.transform(this);
		mstruct.negate();
		return 1;
	}

	if(mstruct.isVector()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i] *= CALCULATOR->getRadUnit();
		}
	} else {
		mstruct *= CALCULATOR->getRadUnit();
	}
	return -1;
}

CosFunction::CosFunction() : MathFunction("cos", 1) {
	Argument *arg = new AngleArgument();
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool CosFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && ((allow_units && (vargs[0].representsNumber(true) || vargs[0].representsNonComplex(true))) || (!allow_units && is_number_angle_value(vargs[0], true)));}
bool CosFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (is_real_angle_value(vargs[0]) || is_infinite_angle_value(vargs[0]));}
bool CosFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
int CosFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	if(vargs[0].isVector()) return 0;

	if(eo.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0].isFunction() && vargs[0][0].size() == 1 && vargs[0][1].isUnit() && vargs[0][1].unit() == CALCULATOR->customAngleUnit()) {
		if(vargs[0][0].function()->id() == FUNCTION_ID_ACOS) {
			mstruct = vargs[0][0][0];
			return 1;
		} else if(vargs[0][0].function()->id() == FUNCTION_ID_ASIN) {
			mstruct = vargs[0][0][0];
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_half;
			return 1;
		} else if(vargs[0][0].function()->id() == FUNCTION_ID_ATAN) {
			mstruct = vargs[0][0][0];
			mstruct ^= nr_two;
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			return 1;
		}
	}

	TRIGONOMETRIC_FUNCTION_PREPARATIONS

	if(mstruct.isVariable() && mstruct.variable()->id() == VARIABLE_ID_PI) {
		mstruct = -1;
		b = true;
	} else if(f) {
		if(f->id() == FUNCTION_ID_ACOS) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1, true);
			b = true;
		} else if(f->id() == FUNCTION_ID_ASIN) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_half;
			b = true;
		} else if(f->id() == FUNCTION_ID_ATAN) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1);
			mstruct ^= nr_two;
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			b = true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isVariable() && mstruct[1].variable()->id() == VARIABLE_ID_PI) {
		if(mstruct[0].number().isInteger()) {
			if(mstruct[0].number().isEven()) {
				mstruct = 1;
			} else {
				mstruct = -1;
			}
			b = true;
		} else if(!mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().includesInfinity() && !mstruct[0].number().isInterval()) {
			Number nr(mstruct[0].number());
			nr.frac();
			Number nr_int(mstruct[0].number());
			nr_int.trunc();
			bool b_even = nr_int.isEven();
			nr.setNegative(false);
			if(nr.equals(Number(1, 2, 0))) {
				mstruct.clear();
				b = true;
			} else if(nr.equals(Number(1, 4, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(3, 4, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 3, 0))) {
				if(b_even) mstruct.set(1, 2, 0);
				else mstruct.set(-1, 2, 0);
				b = true;
			} else if(nr.equals(Number(2, 3, 0))) {
				if(b_even) mstruct.set(-1, 2, 0);
				else mstruct.set(1, 2, 0);
				b = true;
			} else if(nr.equals(Number(1, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(!b_even) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(5, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.divide_nocopy(new MathStructure(2, 1, 0));
				if(b_even) mstruct.negate();
				b = true;
			} else if(eo.approximation == APPROXIMATION_EXACT && (mstruct[0].number().isNegative() || !mstruct[0].number().isFraction() || mstruct[0].number().isGreaterThan(nr_half))) {
				nr_int = mstruct[0].number();
				nr_int.floor();
				Number nr_frac = mstruct[0].number();
				nr_frac -= nr_int;
				if(nr_frac.isGreaterThan(nr_half)) {
					nr_frac -= nr_half;
					mstruct[0].number() = nr_half;
					mstruct[0].number() -= nr_frac;
					nr_int++;
				} else {
					mstruct[0].number() = nr_frac;
				}
				if(nr_int.isOdd()) {
					mstruct *= CALCULATOR->getRadUnit();
					mstruct.transform(this);
					mstruct.negate();
					b = true;
				}
			}
		}
	} else if(mstruct.isAddition()) {
		size_t i = 0;
		bool b_negate = false;
		for(; i < mstruct.size(); i++) {
			if((mstruct[i].isVariable() && mstruct[i].variable()->id() == VARIABLE_ID_PI) || (mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1].isVariable() && mstruct[i][1].variable()->id() == VARIABLE_ID_PI && mstruct[i][0].isNumber())) {
				if(contains_angle_unit(mstruct, eo.parse_options, 0)) break;
				if(mstruct[i].isVariable() || mstruct[i][0].number().isInteger()) {
					b_negate = mstruct[i].isVariable() || mstruct[i][0].number().isOdd();
					mstruct.delChild(i + 1);
					b_recalc = false;
					break;
				} else if(mstruct[i][0].number().isReal() && (mstruct[i][0].number().isNegative() || !mstruct[i][0].number().isFraction())) {
					Number nr_int = mstruct[i][0].number();
					nr_int.floor();
					mstruct[i][0].number() -= nr_int;
					b_negate = nr_int.isOdd();
					b_recalc = false;
					break;
				}
			}
		}
		b = b_negate;
		if(b_negate) {
			mstruct *= CALCULATOR->getRadUnit();
			mstruct.transform(this);
			mstruct.negate();
		}
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(b);
	if(b) return 1;
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		if(b_recalc) convert_to_radians(vargs[0], mstruct, eo2);
		mstruct.eval(eo2);
	}

	fix_leftover_angle_unit(mstruct, eo);
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.cos() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}
	if(trig_remove_i(mstruct)) {
		mstruct.transformById(FUNCTION_ID_COSH);
		return 1;
	}
	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
	}
	if(mstruct.isVector()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i] *= CALCULATOR->getRadUnit();
		}
	} else {
		mstruct *= CALCULATOR->getRadUnit();
	}
	return -1;
}

TanFunction::TanFunction() : MathFunction("tan", 1) {
	Argument *arg = new AngleArgument();
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool TanFunction::representsNumber(const MathStructure&, bool) const {return false;}
bool TanFunction::representsReal(const MathStructure&, bool) const {return false;}
bool TanFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex(true);}
bool TanFunction::representsUndefined(const MathStructure &vargs) const {
	if(vargs.size() != 1) return false;
	if(vargs[0].isMultiplication() && vargs[0].size() == 3 && vargs[0][0].isNumber() && vargs[0][0].number().denominatorIsTwo() && vargs[0][1].isVariable() && vargs[0][1].variable() == CALCULATOR->getVariableById(VARIABLE_ID_PI) && vargs[0][2].isUnit() && vargs[0][2].unit() == CALCULATOR->getRadUnit()) {
		return true;
	}
	if(vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0].isMultiplication() && vargs[0][0].size() == 2 &&  vargs[0][0][0].isNumber() && vargs[0][0][0].number().denominatorIsTwo() && vargs[0][0][1].isVariable() && vargs[0][0][1].variable() == CALCULATOR->getVariableById(VARIABLE_ID_PI) && vargs[0][1].isUnit() && vargs[0][1].unit() == CALCULATOR->getRadUnit()) {
		return true;
	}
	return false;
}
int TanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	if(vargs[0].isVector()) return 0;

	if(eo.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && vargs[0].isMultiplication() && vargs[0].size() == 2 && vargs[0][0].isFunction() && vargs[0][0].size() == 1 && vargs[0][1].isUnit() && vargs[0][1].unit() == CALCULATOR->customAngleUnit()) {
		if(vargs[0][0].function()->id() == FUNCTION_ID_ATAN) {
			mstruct = vargs[0][0][0];
			return 1;
		} else if(vargs[0][0].function()->id() == FUNCTION_ID_ASIN && !vargs[0][0][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct = vargs[0][0][0];
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			mstruct.multiply_nocopy(mmul);
			return 1;
		} else if(vargs[0][0].function()->id() == FUNCTION_ID_ACOS && !vargs[0][0][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct = vargs[0][0][0];
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_half;
			mstruct.divide_nocopy(mmul);
			return 1;
		}
	}

	TRIGONOMETRIC_FUNCTION_PREPARATIONS

	if(mstruct.isVariable() && mstruct.variable()->id() == VARIABLE_ID_PI) {
		mstruct.clear();
		b = true;
	} else if(f) {
		if(f->id() == FUNCTION_ID_ATAN) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1, true);
			b = true;
		} else if(f->id() == FUNCTION_ID_ASIN && (mstruct.isFunction() ? !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true) : (mstruct[2].isFunction() ? !mstruct[2][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true) : !mstruct[0][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)))) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			mstruct.multiply_nocopy(mmul);
			b = true;
		} else if(f->id() == FUNCTION_ID_ACOS && (mstruct.isFunction() ? !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true) : (mstruct[2].isFunction() ? !mstruct[2][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true) : !mstruct[0][0].containsInterval(eo.approximation == APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation != APPROXIMATION_EXACT, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)))) {
			if(!mstruct.isFunction()) mstruct.setToChild(mstruct[0].isFunction() ? 1 : 3, true);
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_half;
			mstruct.divide_nocopy(mmul);
			b = true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isVariable() && mstruct[1].variable()->id() == VARIABLE_ID_PI) {
		if(mstruct[0].number().isInteger()) {
			mstruct.clear();
			b = true;
		} else if(!mstruct[0].number().hasImaginaryPart() && !mstruct[0].number().includesInfinity() && !mstruct[0].number().isInterval()) {
			Number nr(mstruct[0].number());
			nr.frac();
			bool b_neg = nr.isNegative();
			nr.setNegative(false);
			if(nr.equals(nr_half)) {
				if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
				if(!mstruct[0].number().numeratorIsOne() && !mstruct[0].number().numeratorEquals(3)) {
					Number nr_int(mstruct[0].number());
					nr_int.floor();
					bool b_even = nr_int.isEven();
					if(b_even) mstruct[0].set(nr_half, true);
					else mstruct[0].set(Number(3, 2), true);
					mstruct.childUpdated(1);
				}
				mstruct *= CALCULATOR->getRadUnit();
				return -1;
			} else if(nr.equals(Number(1, 4, 0))) {
				if(b_neg) mstruct.set(-1, 1, 0);
				else mstruct.set(1, 1, 0);
				b = true;
			} else if(nr.equals(Number(3, 4, 0))) {
				if(!b_neg) mstruct.set(-1, 1, 0);
				else mstruct.set(1, 1, 0);
				b = true;
			} else if(nr.equals(Number(1, 3, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(2, 3, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(!b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(-1, 2, 0));
				if(b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(5, 6, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(-1, 2, 0));
				if(!b_neg) mstruct.negate();
				b = true;
			} else if(nr.equals(Number(1, 8, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(b_neg) {
					mstruct.negate();
					mstruct.add_nocopy(new MathStructure(1, 1, 0));
				} else {
					mstruct.add_nocopy(new MathStructure(-1, 1, 0));
				}
				b = true;
			} else if(nr.equals(Number(7, 8, 0))) {
				mstruct.set(2, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(!b_neg) {
					mstruct.negate();
					mstruct.add_nocopy(new MathStructure(1, 1, 0));
				} else {
					mstruct.add_nocopy(new MathStructure(-1, 1, 0));
				}
				b = true;
			} else if(nr.equals(Number(1, 12, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(b_neg) {
					mstruct.add_nocopy(new MathStructure(-2, 1, 0));
				} else {
					mstruct.negate();
					mstruct.add_nocopy(new MathStructure(2, 1, 0));
				}
				b = true;
			} else if(nr.equals(Number(11, 12, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				if(!b_neg) {
					mstruct.add_nocopy(new MathStructure(-2, 1, 0));
				} else {
					mstruct.negate();
					mstruct.add_nocopy(new MathStructure(2, 1, 0));
				}
				b = true;
			} else if(nr.equals(Number(5, 12, 0))) {
				mstruct.set(3, 1, 0);
				mstruct.raise_nocopy(new MathStructure(1, 2, 0));
				mstruct.add_nocopy(new MathStructure(2, 1, 0));
				if(b_neg) mstruct.negate();
				b = true;
			} else if(eo.approximation == APPROXIMATION_EXACT && (mstruct[0].number().isNegative() || !mstruct[0].number().isFraction() || mstruct[0].number().isGreaterThan(nr_half))) {
				Number nr_int(mstruct[0].number());
				nr_int.floor();
				Number nr_frac = mstruct[0].number();
				nr_frac -= nr_int;
				if(nr_frac.isGreaterThan(nr_half)) {
					nr_frac -= nr_half;
					mstruct[0].number() = nr_half;
					mstruct[0].number() -= nr_frac;
					mstruct *= CALCULATOR->getRadUnit();
					mstruct.transform(this);
					mstruct.negate();
					b = true;
				} else {
					mstruct[0].number() = nr_frac;
				}
			}
		}
	} else if(mstruct.isMultiplication() && mstruct.size() >= 2) {
		bool b_pi = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isVariable() && mstruct[i].variable()->id() == VARIABLE_ID_PI) {
				b_pi = !b_pi;
				if(!b_pi) break;
			} else if(!mstruct[i].representsInteger()) {
				b_pi = false;
				break;
			}
		}
		if(b_pi) {
			mstruct.clear();
			b = true;
		}
	} else if(mstruct.isAddition()) {
		size_t i = 0;
		for(; i < mstruct.size(); i++) {
			if((mstruct[i].isVariable() && mstruct[i].variable()->id() == VARIABLE_ID_PI) || (mstruct[i].isMultiplication() && mstruct[i].size() == 2 && mstruct[i][1].isVariable() && mstruct[i][1].variable()->id() == VARIABLE_ID_PI && mstruct[i][0].isNumber())) {
				if(contains_angle_unit(mstruct, eo.parse_options, 0)) break;
				if(mstruct[i].isVariable() || mstruct[i][0].number().isInteger()) {
					mstruct.delChild(i + 1);
					b_recalc = false;
					break;
				} else if(mstruct[i][0].number().isReal() && (mstruct[i][0].number().isNegative() || !mstruct[i][0].number().isFraction())) {
					Number nr_int = mstruct[i][0].number();
					nr_int.floor();
					mstruct[i][0].number() -= nr_int;
					b_recalc = false;
					break;
				}
			}
		}
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(b);
	if(b) return 1;
	if(eo.approximation == APPROXIMATION_TRY_EXACT && !mstruct.isNumber()) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		if(b_recalc) convert_to_radians(vargs[0], mstruct, eo2);
		mstruct.eval(eo2);
	}

	fix_leftover_angle_unit(mstruct, eo);
	if(mstruct.isNumber()) {
		Number nr(mstruct.number());
		if(nr.tan() && !(eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) && !(!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) && !(!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			mstruct.set(nr, true);
			return 1;
		}
	}

	if(trig_remove_i(mstruct)) {
		mstruct.transformById(FUNCTION_ID_TANH);
		mstruct *= nr_one_i;
		return 1;
	}

	if(has_predominately_negative_sign(mstruct)) {
		negate_struct(mstruct);
		mstruct *= CALCULATOR->getRadUnit();
		mstruct.transform(this);
		mstruct.negate();
		return 1;
	}

	if(mstruct.isVector()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i] *= CALCULATOR->getRadUnit();
		}
	} else {
		mstruct *= CALCULATOR->getRadUnit();
	}

	return -1;
}

void set_fraction_of_turn(MathStructure &mstruct, const EvaluationOptions &eo, long int num, long int den) {
	if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
		if(num == 1 && den == 2) {
			mstruct.set(CALCULATOR->getVariableById(VARIABLE_ID_PI));
		} else {
			mstruct.set(num * 2, den, 0L);
			mstruct.multiply_nocopy(new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_PI)));
		}
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else {
		mstruct = angle_units_in_turn(eo, num, den);
	}
}
void add_fraction_of_turn(MathStructure &mstruct, const EvaluationOptions &eo, long int num, long int den, bool append) {
	if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
		mstruct.add(CALCULATOR->getVariableById(VARIABLE_ID_PI), append);
		if(num != 1 || den != 2) mstruct.last() *= Number(num * 2, den);
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else {
		mstruct.add(angle_units_in_turn(eo, num, den), append);
	}
}
void multiply_by_fraction_of_radian(MathStructure &mstruct, const EvaluationOptions &eo, long int num, long int den) {
	if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
		if(num != 1 && den != 1) {
			mstruct.multiply(Number(num, den, 0L));
		}
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else {
		mstruct *= angle_units_in_turn(eo, num, den * 2);
		mstruct.divide_nocopy(new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_PI)));
	}
}
AsinFunction::AsinFunction() : MathFunction("asin", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool AsinFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
int AsinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
	} else {
		mstruct.eval(eo);
	}
	if(mstruct.isVector()) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return -1;
	}
	if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0] == nr_half && mstruct[1].isPower() && mstruct[1][1] == nr_half) {
		if(mstruct[1][0] == nr_two) {
			set_fraction_of_turn(mstruct, eo, 1, 8);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		} else if(mstruct[1][0] == nr_three) {
			set_fraction_of_turn(mstruct, eo, 1, 6);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		}
	} else if(mstruct.isPower() && mstruct[1] == nr_minus_half && mstruct[0] == nr_two) {
		set_fraction_of_turn(mstruct, eo, 1, 8);
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		if(!mstruct.isNumber()) {
			CALCULATOR->endTemporaryStopMessages(false);
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			mstruct = vargs[0];
			mstruct.eval(eo2);
		} else {
			CALCULATOR->endTemporaryStopMessages(true);
		}
	}
	if(!mstruct.isNumber()) {
		if(trig_remove_i(mstruct)) {
			mstruct.transformById(FUNCTION_ID_ASINH);
			mstruct *= nr_one_i;
			multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
			return 1;
		}
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	if(mstruct.number().isZero()) {
		mstruct.clear();
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else if(mstruct.number().isOne()) {
		set_fraction_of_turn(mstruct, eo, 1, 4);
	} else if(mstruct.number().isMinusOne()) {
		set_fraction_of_turn(mstruct, eo, -1, 4);
	} else if(mstruct.number().equals(nr_half)) {
		set_fraction_of_turn(mstruct, eo, 1, 12);
	} else {
		Number nr = mstruct.number();
		if(!nr.asin() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			if(trig_remove_i(mstruct)) {
				mstruct.transformById(FUNCTION_ID_ASINH);
				mstruct *= nr_one_i;
				multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
				return 1;
			}
			if(has_predominately_negative_sign(mstruct)) {mstruct.number().negate(); mstruct.transform(this); mstruct.negate(); return 1;}
			return -1;
		}
		mstruct = nr;
		multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
	}
	return 1;

}

AcosFunction::AcosFunction() : MathFunction("acos", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool AcosFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
int AcosFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
	} else {
		mstruct.eval(eo);
	}
	if(mstruct.isVector()) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return -1;
	}
	if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0] == nr_half && mstruct[1].isPower() && mstruct[1][1] == nr_half) {
		if(mstruct[1][0] == nr_two) {
			set_fraction_of_turn(mstruct, eo, 1, 8);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		} else if(mstruct[1][0] == nr_three) {
			set_fraction_of_turn(mstruct, eo, 1, 12);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		}
	} else if(mstruct.isPower() && mstruct[1] == nr_minus_half && mstruct[0] == nr_two) {
		set_fraction_of_turn(mstruct, eo, 1, 8);
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return 1;
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		if(!mstruct.isNumber()) {
			CALCULATOR->endTemporaryStopMessages(false);
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			mstruct = vargs[0];
			mstruct.eval(eo2);
		} else {
			CALCULATOR->endTemporaryStopMessages(true);
		}
	}
	if(!mstruct.isNumber()) {
		if(has_predominately_negative_sign(mstruct)) {
			negate_struct(mstruct); mstruct.transformById(FUNCTION_ID_ASIN);
			add_fraction_of_turn(mstruct, eo, 1, 4);
			return 1;
		}
		return -1;
	}
	if(mstruct.number().isZero()) {
		set_fraction_of_turn(mstruct, eo, 1, 4);
	} else if(mstruct.number().isOne()) {
		mstruct.clear();
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else if(mstruct.number().isMinusOne()) {
		set_fraction_of_turn(mstruct, eo, 1, 2);
	} else if(mstruct.number().equals(nr_half)) {
		set_fraction_of_turn(mstruct, eo, 1, 6);
	} else {
		Number nr = mstruct.number();
		if(!nr.acos() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			if(has_predominately_negative_sign(mstruct)) {
				mstruct.number().negate();
				mstruct.transformById(FUNCTION_ID_ASIN);
				add_fraction_of_turn(mstruct, eo, 1, 4);
				return 1;
			}
			return -1;
		}
		mstruct = nr;
		multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
	}
	return 1;

}

AtanFunction::AtanFunction() : MathFunction("atan", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool AtanFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && (vargs[0].representsReal(allow_units) || (vargs[0].isNumber() && !vargs[0].number().isI() && !vargs[0].number().isMinusI()));}
bool AtanFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool AtanFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
int AtanFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		mstruct.eval(eo2);
	} else {
		mstruct.eval(eo);
	}
	if(mstruct.isVector()) {
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return -1;
	}
	if(mstruct.isAddition() && mstruct.size() == 2 && ((mstruct[0].isPower() && mstruct[0][1].isMinusOne() && mstruct[0][0].isAddition() && mstruct[0][0].size() == 2 && mstruct[0][0][1].isOne() && mstruct[0][0][0].isPower() && mstruct[0][0][0][0] == nr_three && mstruct[0][0][0][1] == nr_half && mstruct[1].isMultiplication() && mstruct[1].size() == 3 && mstruct[1][0].isMinusOne() && mstruct[1][2] == mstruct[0] && mstruct[1][1] == mstruct[0][0][0]) || (mstruct[0].isPower() && mstruct[0][0] == nr_three && mstruct[0][1] == nr_half && mstruct[1].isNumber() && mstruct[1].number() == -2))) {
		set_fraction_of_turn(mstruct, eo, -1, 24);
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return 1;
	} else if(mstruct.isAddition() && mstruct.size() == 2 && ((mstruct[0].isPower() && mstruct[0][1].isMinusOne() && mstruct[0][0].isAddition() && mstruct[0][0].size() == 2 && mstruct[0][0][1].isMinusOne() && mstruct[0][0][0].isPower() && mstruct[0][0][0][0] == nr_three && mstruct[0][0][0][1] == nr_half && mstruct[1].isMultiplication() && mstruct[1].size() == 2 && mstruct[1][1] == mstruct[0] && mstruct[1][0] == mstruct[0][0][0]) || (mstruct[0].isPower() && mstruct[0][0] == nr_three && mstruct[0][1] == nr_half && mstruct[1].isNumber() && mstruct[1].number() == 2))) {
		set_fraction_of_turn(mstruct, eo, 5, 24);
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return 1;
	} else if(mstruct.isAddition() && mstruct.size() == 2 && mstruct[1].isMinusOne() && mstruct[0].isPower() && mstruct[0][1] == nr_half && mstruct[0][0] == nr_two) {
		set_fraction_of_turn(mstruct, eo, 1, 16);
		if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
		return 1;
	} else if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].isNumber() && mstruct[1].isPower() && mstruct[1][1] == nr_half && mstruct[1][0] == nr_three && mstruct[0].number() == Number(1, 3)) {
		if(mstruct[1][0] == nr_three) {
			set_fraction_of_turn(mstruct, eo, 1, 12);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		}
	} else if(mstruct.isPower() && mstruct[1] == nr_half) {
		if(mstruct[0] == nr_three) {
			set_fraction_of_turn(mstruct, eo, 1, 6);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		} else if(mstruct[0].isAddition() && mstruct[0].size() == 2 && mstruct[0][0].isMultiplication() && mstruct[0][0].size() == 2 && mstruct[0][0][0].isNumber() && mstruct[0][0][0].number() == -2 && mstruct[0][0][1].isPower() && mstruct[0][0][1][1] == nr_half && mstruct[0][0][1][0].isNumber() && mstruct[0][0][1][0].number() == 5 && mstruct[0][1].isNumber() && mstruct[0][1].number() == 5) {
			set_fraction_of_turn(mstruct, eo, 1, 10);
			if(eo.approximation == APPROXIMATION_TRY_EXACT) CALCULATOR->endTemporaryStopMessages(true);
			return 1;
		}
	}
	if(eo.approximation == APPROXIMATION_TRY_EXACT) {
		if(!mstruct.isNumber()) {
			CALCULATOR->endTemporaryStopMessages(false);
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			mstruct = vargs[0];
			mstruct.eval(eo2);
		} else {
			CALCULATOR->endTemporaryStopMessages(true);
		}
	}
	if(!mstruct.isNumber()) {
		if(trig_remove_i(mstruct)) {
			mstruct.transformById(FUNCTION_ID_ATANH);
			mstruct *= nr_one_i;
			multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
			return 1;
		}
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	if(mstruct.number().isZero()) {
		mstruct.clear();
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else if(eo.allow_infinite && mstruct.number().isI()) {
		Number nr; nr.setImaginaryPart(nr_plus_inf);
		mstruct = nr;
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else if(eo.allow_infinite && mstruct.number().isMinusI()) {
		Number nr; nr.setImaginaryPart(nr_minus_inf);
		mstruct = nr;
		if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
	} else if(mstruct.number().isPlusInfinity()) {
		set_fraction_of_turn(mstruct, eo, 1, 4);
	} else if(mstruct.number().isMinusInfinity()) {
		set_fraction_of_turn(mstruct, eo, -1, 4);
	} else if(mstruct.number().isOne()) {
		set_fraction_of_turn(mstruct, eo, 1, 8);
	} else if(mstruct.number().isMinusOne()) {
		set_fraction_of_turn(mstruct, eo, -1, 8);
	} else {
		Number nr = mstruct.number();
		if(!nr.atan() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
			if(trig_remove_i(mstruct)) {
				mstruct.transformById(FUNCTION_ID_ATANH);
				mstruct *= nr_one_i;
				multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
				return 1;
			}
			if(has_predominately_negative_sign(mstruct)) {mstruct.number().negate(); mstruct.transform(this); mstruct.negate(); return 1;}
			return -1;
		}
		mstruct = nr;
		multiply_by_fraction_of_radian(mstruct, eo, 1, 1);

	}
	return 1;

}

SinhFunction::SinhFunction() : MathFunction("sinh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool SinhFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool SinhFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool SinhFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
int SinhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isFunction() && mstruct.size() == 1) {
		if(mstruct.function()->id() == FUNCTION_ID_ASINH) {
			mstruct.setToChild(1, true);
			return 1;
		} else if(mstruct.function()->id() == FUNCTION_ID_ACOSH && !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, false, false, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct += nr_minus_one;
			mstruct ^= nr_half;
			*mmul += nr_one;
			*mmul ^= nr_half;
			mstruct.multiply_nocopy(mmul);
			return 1;
		} else if(mstruct.function()->id() == FUNCTION_ID_ATANH && !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, false, false, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			mstruct.multiply_nocopy(mmul);
			return 1;
		}
	}
	if(!mstruct.isNumber()) {
		if(trig_remove_i(mstruct)) {mstruct *= CALCULATOR->getRadUnit(); mstruct.transformById(FUNCTION_ID_SIN); mstruct *= nr_one_i; return 1;}
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	Number nr = mstruct.number();
	if(!nr.sinh() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
		if(trig_remove_i(mstruct)) {mstruct *= CALCULATOR->getRadUnit(); mstruct.transformById(FUNCTION_ID_SIN); mstruct *= nr_one_i; return 1;}
		if(has_predominately_negative_sign(mstruct)) {mstruct.number().negate(); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	mstruct = nr;
	return 1;
}
CoshFunction::CoshFunction() : MathFunction("cosh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool CoshFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool CoshFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool CoshFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
int CoshFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isFunction() && mstruct.size() == 1) {
		if(mstruct.function()->id() == FUNCTION_ID_ACOSH) {
			mstruct.setToChild(1, true);
			return 1;
		} else if(mstruct.function()->id() == FUNCTION_ID_ASINH) {
			mstruct.setToChild(1);
			mstruct ^= nr_two;
			mstruct += nr_one;
			mstruct ^= nr_half;
			return 1;
		} else if(mstruct.function()->id() == FUNCTION_ID_ATANH) {
			mstruct.setToChild(1);
			mstruct ^= nr_two;
			mstruct.negate();
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			return 1;
		}
	}
	if(!mstruct.isNumber()) {
		if(trig_remove_i(mstruct)) {mstruct *= CALCULATOR->getRadUnit(); mstruct.transformById(FUNCTION_ID_COS); return 1;}
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); return -1;}
		return -1;
	}
	Number nr = mstruct.number();
	if(!nr.cosh() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
		if(trig_remove_i(mstruct)) {mstruct *= CALCULATOR->getRadUnit(); mstruct.transformById(FUNCTION_ID_COS); return 1;}
		if(has_predominately_negative_sign(mstruct)) mstruct.number().negate();
		return -1;
	}
	mstruct = nr;
	return 1;
}
TanhFunction::TanhFunction() : MathFunction("tanh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool TanhFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool TanhFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool TanhFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
int TanhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(mstruct.isFunction() && mstruct.size() == 1) {
		if(mstruct.function()->id() == FUNCTION_ID_ATANH) {
			mstruct.setToChild(1, true);
			return 1;
		} else if(mstruct.function()->id() == FUNCTION_ID_ASINH && !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, false, false, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			mstruct ^= nr_two;
			mstruct += nr_one;
			mstruct ^= nr_minus_half;
			mstruct.multiply_nocopy(mmul);
			return 1;
		} else if(mstruct.function()->id() == FUNCTION_ID_ACOSH && !mstruct[0].containsInterval(eo.approximation == APPROXIMATION_EXACT, false, false, eo.approximation == APPROXIMATION_EXACT ? 1 : 0, true)) {
			mstruct.setToChild(1);
			MathStructure *mmul = new MathStructure(mstruct);
			MathStructure *mmul2 = new MathStructure(mstruct);
			*mmul2 ^= nr_minus_one;
			mstruct += nr_minus_one;
			mstruct ^= nr_half;
			*mmul += nr_one;
			*mmul ^= nr_half;
			mstruct.multiply_nocopy(mmul);
			mstruct.multiply_nocopy(mmul2);
			return 1;
		}
	}
	if(!mstruct.isNumber()) {
		if(trig_remove_i(mstruct)) {mstruct *= CALCULATOR->getRadUnit(); mstruct.transformById(FUNCTION_ID_TAN); mstruct *= nr_one_i; return 1;}
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	Number nr = mstruct.number();
	if(!nr.tanh() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
		if(has_predominately_negative_sign(mstruct)) {mstruct.number().negate(); mstruct.transform(this); mstruct.negate(); return 1;}
		if(trig_remove_i(mstruct)) {mstruct *= CALCULATOR->getRadUnit(); mstruct.transformById(FUNCTION_ID_TAN); mstruct *= nr_one_i; return 1;}
		return -1;
	}
	mstruct = nr;
	return 1;
}
AsinhFunction::AsinhFunction() : MathFunction("asinh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
bool AsinhFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
bool AsinhFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsReal();}
bool AsinhFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
int AsinhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(!mstruct.isNumber()) {
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	Number nr = mstruct.number();
	if(!nr.asinh() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
		if(has_predominately_negative_sign(mstruct)) {mstruct.number().negate(); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	mstruct = nr;
	return 1;
}
AcoshFunction::AcoshFunction() : MathFunction("acosh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false));
}
bool AcoshFunction::representsNumber(const MathStructure &vargs, bool allow_units) const {return vargs.size() == 1 && vargs[0].representsNumber(allow_units);}
int AcoshFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(eo.allow_complex && vargs[0].isZero()) {
		mstruct.set(1, 2, 0);
		mstruct.number() *= nr_one_i;
		mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
		return 1;
	} else if(vargs[0].isOne()) {
		mstruct.clear();
		return 1;
	} else if(eo.approximation != APPROXIMATION_APPROXIMATE && eo.allow_complex && vargs[0].number() <= -1) {
		mstruct = nr_one_i;
		mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
		mstruct.add_nocopy(new MathStructure(this, &vargs[0], NULL));
		mstruct.last()[0].negate();
		return 1;
	}
	Number nr(vargs[0].number());
	if(!nr.acosh() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
		return 0;
	}
	mstruct = nr;
	return 1;
}
AtanhFunction::AtanhFunction() : MathFunction("atanh", 1) {
	setArgumentDefinition(1, new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false));
}
int AtanhFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	if(!mstruct.isNumber()) {
		if(has_predominately_negative_sign(mstruct)) {negate_struct(mstruct); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	if(eo.allow_complex && mstruct.number().includesInfinity()) {
		if(mstruct.number().isPlusInfinity() || (!mstruct.number().hasRealPart() && mstruct.number().hasImaginaryPart() && mstruct.number().internalImaginary()->isMinusInfinity())) {
			mstruct = nr_minus_half;
			mstruct *= nr_one_i;
			mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			return true;
		} else if(mstruct.number().isMinusInfinity() || (!mstruct.number().hasRealPart() && mstruct.number().hasImaginaryPart() && mstruct.number().internalImaginary()->isPlusInfinity())) {
			mstruct = nr_half;
			mstruct *= nr_one_i;
			mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			return true;
		}
	} else if(eo.approximation != APPROXIMATION_APPROXIMATE && eo.allow_complex && mstruct.number() > 1) {
		mstruct.set(-1, 2, 0);
		mstruct.number() *= nr_one_i;
		mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
		mstruct.add_nocopy(new MathStructure(this, &vargs[0], NULL));
		mstruct.last()[0].inverse();
		return 1;
	} else if(eo.approximation != APPROXIMATION_APPROXIMATE && eo.allow_complex && mstruct.number() < -1) {
		mstruct.set(1, 2, 0);
		mstruct.number() *= nr_one_i;
		mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
		mstruct.add_nocopy(new MathStructure(this, &vargs[0], NULL));
		mstruct.last()[0].inverse();
		mstruct.last()[0].negate();
		mstruct.last().negate();
		return 1;
	}
	Number nr = mstruct.number();
	if(!nr.atanh() || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity())) {
		if(has_predominately_negative_sign(mstruct)) {mstruct.number().negate(); mstruct.transform(this); mstruct.negate(); return 1;}
		return -1;
	}
	mstruct = nr;
	return 1;
}
SincFunction::SincFunction() : MathFunction("sinc", 1) {
	Argument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, true, false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
bool SincFunction::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (vargs[0].representsNumber() || is_number_angle_value(vargs[0]));}
bool SincFunction::representsReal(const MathStructure &vargs, bool) const {return vargs.size() == 1 && (vargs[0].representsReal() || is_real_angle_value(vargs[0]));}
bool SincFunction::representsNonComplex(const MathStructure &vargs, bool) const {return vargs.size() == 1 && vargs[0].representsNonComplex();}
int SincFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isZero()) {
		mstruct.set(1, 1, 0, true);
		return 1;
	} else if(vargs[0].representsNonZero(true)) {
		mstruct = vargs[0];
		if(getDefaultValue(2) == "pi") mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
		bool b = replace_f_interval(mstruct, eo);
		b = replace_intervals_f(mstruct) || b;
		MathStructure *m_sin = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), &mstruct, NULL);
		(*m_sin)[0].multiply(CALCULATOR->getRadUnit());
		mstruct.inverse();
		mstruct.multiply_nocopy(m_sin);
		if(b) mstruct.eval(eo);
		return 1;
	}
	return -1;
}

Atan2Function::Atan2Function() : MathFunction("atan2", 2) {
	NumberArgument *arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
	arg = new NumberArgument("", ARGUMENT_MIN_MAX_NONE, false, false);
	arg->setComplexAllowed(false);
	arg->setHandleVector(true);
	setArgumentDefinition(2, arg);
}
bool Atan2Function::representsNumber(const MathStructure &vargs, bool) const {return vargs.size() == 2 && vargs[0].representsNumber() && vargs[1].representsNumber();}
int Atan2Function::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	if(vargs[0].isVector()) return 0;
	mstruct = vargs[0];
	mstruct.eval(eo);
	if(mstruct.isVector()) return -1;
	MathStructure m2(vargs[1]);
	m2.eval(eo);
	if(m2.isVector()) {
		mstruct.transform(STRUCT_VECTOR, m2);
		return -3;
	}

	if(!mstruct.isNumber() || !m2.isNumber()) {
		ComparisonResult cr_im = mstruct.compare(m_zero);
		ComparisonResult cr_re = m2.compare(m_zero);
		if(cr_im == COMPARISON_RESULT_EQUAL) {
			if(cr_re == COMPARISON_RESULT_LESS) {
				mstruct.clear();
				if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
				return 1;
			} else if(cr_re == COMPARISON_RESULT_GREATER) {
				set_fraction_of_turn(mstruct, eo, 1, 2);
				return 1;
			}
		} else if(cr_im == COMPARISON_RESULT_LESS || cr_im == COMPARISON_RESULT_GREATER) {
			if(cr_re == COMPARISON_RESULT_EQUAL) {
				int i_sgn = 0;
				if(cr_im == COMPARISON_RESULT_LESS) i_sgn = 1;
				else if(cr_im == COMPARISON_RESULT_GREATER) i_sgn = -1;
				if(i_sgn != 0) {
					set_fraction_of_turn(mstruct, eo, 1, 4);
					if(i_sgn < 0) mstruct.negate();
					return 1;
				}
			} else if(cr_re == COMPARISON_RESULT_GREATER) {
				if(cr_im == COMPARISON_RESULT_GREATER) {
					mstruct.divide(m2);
					mstruct.transformById(FUNCTION_ID_ATAN);
					add_fraction_of_turn(mstruct, eo, -1, 2);
					return 1;
				} else if(cr_im == COMPARISON_RESULT_LESS) {
					mstruct.divide(m2);
					mstruct.transformById(FUNCTION_ID_ATAN);
					add_fraction_of_turn(mstruct, eo, 1, 2);
					return 1;
				}
			} else if(cr_re == COMPARISON_RESULT_LESS) {
				mstruct.divide(m2);
				mstruct.transformById(FUNCTION_ID_ATAN);
				return 1;
			}
		}
	}
	if(mstruct.isNumber() && m2.isNumber()) {
		if(m2.number().hasImaginaryPart()) {
			if(!mstruct.number().add(m2.number().imaginaryPart())) {
				mstruct.transform(STRUCT_VECTOR, m2);
				return -3;
			}
			m2.number().clearImaginary();
		}
		if(mstruct.number().hasImaginaryPart()) {
			if(!m2.number().subtract(mstruct.number().imaginaryPart())) {
				mstruct.transform(STRUCT_VECTOR, m2);
				return -3;
			}
			mstruct.number().clearImaginary();
		}
		if(mstruct.number().isZero()) {
			if(!m2.number().isNonZero()) return 0;
			if(m2.number().isNegative()) {
				set_fraction_of_turn(mstruct, eo, 1, 2);
			} else {
				mstruct.clear();
				if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) mstruct *= CALCULATOR->getRadUnit();
			}
		} else if(m2.number().isZero() && mstruct.number().isNonZero()) {
			bool b_neg = mstruct.number().hasNegativeSign();
			set_fraction_of_turn(mstruct, eo, 1, 4);
			if(b_neg) mstruct.negate();
		} else if(!m2.number().isNonZero() || (!mstruct.number().isNonZero() && m2.number().isNegative())) {
			Number nr(mstruct.number());
			if(!nr.atan2(m2.number()) || (eo.approximation == APPROXIMATION_EXACT && nr.isApproximate() && !mstruct.isApproximate() && !m2.isApproximate()) || (!eo.allow_complex && nr.isComplex() && !mstruct.number().isComplex() && !m2.number().isComplex()) || (!eo.allow_infinite && nr.includesInfinity() && !mstruct.number().includesInfinity() && !m2.number().includesInfinity())) {
				mstruct.transform(STRUCT_VECTOR, m2);
				return -3;
			} else {
				mstruct.set(nr);
				multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
				return 1;
			}
		} else {
			MathStructure new_nr(mstruct);
			if(!new_nr.number().divide(m2.number())) {
				mstruct.transform(STRUCT_VECTOR, m2);
				return -3;
			}
			if(m2.number().isNegative()) {
				if(mstruct.number().isNegative()) {
					mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
					add_fraction_of_turn(mstruct, eo, -1, 2);
				} else {
					mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
					add_fraction_of_turn(mstruct, eo, 1, 2);
				}
			} else {
				mstruct.set(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), &new_nr, NULL);
			}
		}
		return 1;
	}
	mstruct.transform(STRUCT_VECTOR, m2);
	return -3;
}
RadiansToDefaultAngleUnitFunction::RadiansToDefaultAngleUnitFunction() : MathFunction("radtodef", 1) {
}
int RadiansToDefaultAngleUnitFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct = vargs[0];
	multiply_by_fraction_of_radian(mstruct, eo, 1, 1);
	return 1;
}

