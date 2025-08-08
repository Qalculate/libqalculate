/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "BuiltinFunctions.h"
#include "MathStructure.h"
#include "Calculator.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

/* Collects x degree, coefficient, degree zero term. By default only one x term (a single exponent) is allowed.
If radunit is true all coefficient must be a multiple of the radian unit. The unit is then removed. Used for trigonometric functions.
If mexp_as_x2 is true, reuire a second degree polynomial and return second degree coefficient in mexp.
If mexp_as_fx is true allow x multipliers of any form (e.g. e^x or sin(x)) and return the the whole x multiplier in mexp.
*/
bool integrate_info(const MathStructure &mstruct, const MathStructure &x_var, MathStructure &madd, MathStructure &mmul, MathStructure &mexp, bool radunit = false, bool mexp_as_x2 = false, bool mexp_as_fx = false) {
	if(radunit && mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[1] == CALCULATOR->getRadUnit()) return integrate_info(mstruct[0], x_var, madd, mmul, mexp, false, mexp_as_x2, mexp_as_fx);
	madd.clear();
	if(mexp_as_x2 || mexp_as_fx) mexp = m_zero;
	else mexp = m_one;
	mmul = m_zero;
	if(!mexp_as_fx && mstruct == x_var) {
		if(radunit) return false;
		mmul = m_one;
		return true;
	} else if(mexp_as_x2 && mstruct.isPower()) {
		if(radunit) return false;
		if(mstruct[1].isNumber() && mstruct[1].number().isTwo() && mstruct[0] == x_var) {
			mexp = m_one;
			return true;
		}
	} else if(!mexp_as_fx && !mexp_as_x2 && mstruct.isPower() && mstruct[1].containsRepresentativeOf(x_var, true, true) == 0) {
		if(radunit) return false;
		if(mstruct[0] == x_var) {
			mexp = mstruct[1];
			mmul = m_one;
			return true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() >= 2) {
		bool b_x = false;
		bool b_rad = false;
		bool b2 = false;
		size_t i_x = 0, i_rad = 0;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b_x && !mexp_as_fx && mstruct[i] == x_var) {
				b_x = true;
				i_x = i;
			} else if(!b_x && mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && mstruct[i][0] == x_var) {
				b_x = true;
				b2 = true;
				i_x = i;
			} else if(!b_x && !mexp_as_fx && !mexp_as_x2 && mstruct[i].isPower() && mstruct[i][0] == x_var && mstruct[i][1].containsRepresentativeOf(x_var, true, true) == 0) {
				b_x = true;
				i_x = i;
				mexp = mstruct[i][1];
			} else if(mstruct[i].containsRepresentativeOf(x_var, true, true) != 0) {
				if(!b_x && mexp_as_fx && (mexp.isZero() || mexp == mstruct[i])) {
					mexp = mstruct[i];
					b_x = true;
					i_x = i;
				} else {
					return false;
				}
			} else if(!b_rad && radunit && mstruct[i] == CALCULATOR->getRadUnit()) {
				b_rad = true;
				i_rad = i;
			}
		}
		if(!b_x || (radunit && !b_rad)) return false;
		if(mstruct.size() == 1 || (radunit && mstruct.size() == 2)) {
			if(b2) mexp = m_one;
			else mmul = m_one;
		} else if(mstruct.size() == 2) {
			if(b2) {
				if(i_x == 1) mexp = mstruct[0];
				else mexp = mstruct[1];
			} else {
				if(i_x == 1) mmul = mstruct[0];
				else mmul = mstruct[1];
			}
		} else if(radunit && mstruct.size() == 3) {
			if((i_x == 1 && i_rad == 2) || (i_x == 2 && i_rad == 1)) {
				if(b2) mexp = mstruct[0];
				else mmul = mstruct[0];
			} else if((i_x == 0 && i_rad == 2) || (i_x == 2 && i_rad == 0)) {
				if(b2) mexp = mstruct[1];
				else mmul = mstruct[1];
			} else {
				if(b2) mexp = mstruct[2];
				else mmul = mstruct[2];
			}
		} else {
			if(b2) {
				mexp = mstruct;
				mexp.delChild(i_x + 1, true);
				if(radunit) {
					mexp.delChild(i_rad < i_x ? i_rad + 1 : i_rad, true);
				}
			} else {
				mmul = mstruct;
				mmul.delChild(i_x + 1, true);
				if(radunit) {
					mmul.delChild(i_rad < i_x ? i_rad + 1 : i_rad, true);
				}
			}
		}
		return true;
	} else if(mstruct.isAddition()) {
		mmul.setType(STRUCT_ADDITION);
		if(mexp_as_x2) mexp.setType(STRUCT_ADDITION);
		madd.setType(STRUCT_ADDITION);
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mexp_as_fx && mstruct[i] == x_var) {
				if(mexp_as_x2 || mexp.isOne()) mmul.addChild(m_one);
				else return false;
			} else if(mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && mstruct[i][0] == x_var) {
				mexp.addChild(m_one);
			} else if(!mexp_as_fx && !mexp_as_x2 && mstruct[i].isPower() && mstruct[i][0] == x_var && mstruct[i][1].containsRepresentativeOf(x_var, true, true) == 0) {
				if(mmul.size() == 0) {
					mexp = mstruct[i][1];
				} else if(mexp != mstruct[i][1]) {
					return false;
				}
				mmul.addChild(m_one);
			} else if(mstruct[i].isMultiplication()) {
				bool b_x = false;
				bool b_rad = false;
				bool b2 = false;
				size_t i_x = 0, i_rad = 0;
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(!b_x && !mexp_as_fx && mstruct[i][i2] == x_var) {
						if(!mexp_as_x2 && !mexp.isOne()) return false;
						i_x = i2;
						b_x = true;
					} else if(!b_x && mexp_as_x2 && mstruct[i][i2].isPower() && mstruct[i][i2][1].isNumber() && mstruct[i][i2][1].number().isTwo() && mstruct[i][i2][0] == x_var) {
						b2 = true;
						i_x = i2;
						b_x = true;
					} else if(!b_x && !mexp_as_fx && !mexp_as_x2 && mstruct[i][i2].isPower() && mstruct[i][i2][0] == x_var && mstruct[i][i2][1].containsRepresentativeOf(x_var, true, true) == 0) {
						if(mmul.size() == 0) {
							mexp = mstruct[i][i2][1];
						} else if(mexp != mstruct[i][i2][1]) {
							return false;
						}
						i_x = i2;
						b_x = true;
					} else if(mstruct[i][i2].containsRepresentativeOf(x_var, true, true) != 0) {
						if(!b_x && mexp_as_fx && (mexp.isZero() || mexp == mstruct[i][i2])) {
							mexp = mstruct[i][i2];
							i_x = i2;
							b_x = true;
						} else {
							return false;
						}
					} else if(!b_rad && radunit && mstruct[i][i2] == CALCULATOR->getRadUnit()) {
						b_rad = true;
						i_rad = i2;
					}
				}
				if(radunit && !b_rad) return false;
				if(b_x) {
					if(mstruct[i].size() == 1) {
						if(b2) mexp.addChild(m_one);
						else mmul.addChild(m_one);
					} else if(radunit && mstruct[i].size() == 2) {
						if(b2) mexp.addChild(m_one);
						else mmul.addChild(m_one);
					} else {
						if(b2) {
							mexp.addChild(mstruct[i]);
							mexp[mexp.size() - 1].delChild(i_x + 1, true);
							if(radunit) mexp[mexp.size() - 1].delChild(i_rad < i_x ? i_rad + 1 : i_rad, true);
							mexp.childUpdated(mexp.size());
						} else {
							mmul.addChild(mstruct[i]);
							mmul[mmul.size() - 1].delChild(i_x + 1, true);
							if(radunit) mmul[mmul.size() - 1].delChild(i_rad < i_x ? i_rad + 1 : i_rad, true);
							mmul.childUpdated(mmul.size());
						}
					}
				} else {
					madd.addChild(mstruct[i]);
					if(radunit) {
						madd[madd.size() - 1].delChild(i_rad + 1, true);
					}
				}
			} else if(radunit && mstruct[i] == CALCULATOR->getRadUnit()) {
				madd.addChild(mstruct[i]);
			} else if(radunit || mstruct[i].containsRepresentativeOf(x_var, true, true) != 0) {
				if(!radunit && mexp_as_fx && (mexp.isZero() || mexp == mstruct[i])) {
					mexp = mstruct[i];
					mmul.addChild(m_one);
				} else {
					return false;
				}
			} else {
				madd.addChild(mstruct[i]);
			}
		}
		if(mmul.size() == 0 && (!mexp_as_x2 || mexp.size() == 0)) {
			mmul.clear();
			if(mexp_as_x2) mexp.clear();
			return false;
		}
		if(mmul.size() == 0) mmul.clear();
		else if(mmul.size() == 1) mmul.setToChild(1);
		if(mexp_as_x2) {
			if(mexp.size() == 0) mexp.clear();
			else if(mexp.size() == 1) mexp.setToChild(1);
		}
		if(madd.size() == 0) madd.clear();
		else if(madd.size() == 1) madd.setToChild(1);
		return true;
	} else if(!radunit && mexp_as_fx && mstruct.contains(x_var, true)) {
		mexp = mstruct;
		mmul = m_one;
		return true;
	}
	return false;
}

/*bool test_absln_comp_cmplx(const MathStructure &mstruct) {
	if(mstruct.number().isComplex() && (!mstruct.number().hasRealPart() || mstruct.number().hasPositiveSign()) && mstruct.number().internalImaginary()->isPositive()) return true;
	if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().numeratorIsOne() && mstruct[0].representsNonComplex(true)) return true;
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].representsNonNegative(true)) {
				if(!test_absln_comp_cmplx(mstruct[i])) {
					return false;
				}
			}
		}
		return true;
	} else if(mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!mstruct[i].representsNonNegative(true)) {
				if(!test_absln_comp_cmplx(mstruct[i])) {
					return false;
				}
			}
		}
		return true;
	}
	return false;
}*/

/* Transform mstruct to logarithm (ln(mstruct)).
If use_abs != 0, logarithms of negative numbers are avoided. ln(abs(mstruct)) is used if signedness is unknown and mstruct is not complex. If abs > 0, mstruct is assumed real if it is not possible to determine if mstruct is complex or not.
*/
bool transform_absln(MathStructure &mstruct, int use_abs, bool definite_integral, const MathStructure &x_var, const EvaluationOptions &eo) {
	if(use_abs != 0 && mstruct.representsNonComplex(true)) {
		if(mstruct.representsNonPositive(true)) {
			mstruct.negate();
		} else if(!mstruct.representsNonNegative(true)) {
			mstruct.transformById(FUNCTION_ID_ABS);
		}
		mstruct.transformById(FUNCTION_ID_LOG);
	} else if(use_abs != 0 && !mstruct.representsComplex(true)) {
		if(definite_integral) use_abs = -1;
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mtest(mstruct);
		EvaluationOptions eo2 = eo;
		eo2.expand = true;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
		mtest.eval(eo2);
		CALCULATOR->endTemporaryStopMessages();
		if(mtest.representsNonComplex(true)) {
			if(mstruct.representsNonPositive(true)) {
				mstruct.negate();
			} else if(!mtest.representsNonNegative(true)) {
				mstruct.transformById(FUNCTION_ID_ABS);
			}
			mstruct.transformById(FUNCTION_ID_LOG);
		} else if(mtest.representsComplex(true)) {
			mstruct.transformById(FUNCTION_ID_LOG);
		} else {
			if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
				CALCULATOR->beginTemporaryStopMessages();
				KnownVariable *var = new KnownVariable("", format_and_print(x_var), ((UnknownVariable*) x_var.variable())->interval());
				mtest.replace(x_var, var);
				mtest.eval(eo2);
				CALCULATOR->endTemporaryStopMessages();
				if(mtest.representsNonComplex(true)) {
					if(mstruct.representsNonPositive(true)) {
						mstruct.negate();
					} else if(!mtest.representsNonNegative(true)) {
						mstruct.transformById(FUNCTION_ID_ABS);
					}
					mstruct.transformById(FUNCTION_ID_LOG);
				} else if(use_abs > 0) {
					CALCULATOR->error(false, "Integral assumed real", NULL);
					mstruct.transformById(FUNCTION_ID_ABS);
					mstruct.transformById(FUNCTION_ID_LOG);
				} else {
					mstruct.transformById(FUNCTION_ID_LOG);
				}
				var->destroy();
			} else if(use_abs > 0) {
				CALCULATOR->error(false, "Integral assumed real", NULL);
				mstruct.transformById(FUNCTION_ID_ABS);
				mstruct.transformById(FUNCTION_ID_LOG);
			} else {
				mstruct.transformById(FUNCTION_ID_LOG);
			}
		}
	} else {
		mstruct.transformById(FUNCTION_ID_LOG);
	}
	return true;
}

// Make sure that m does not contains division by zero. Used after differentiation.
bool check_zero_div(const MathStructure &m, const MathStructure &x_var, const EvaluationOptions &eo, bool top = true) {
	if(top && (!x_var.isVariable() || x_var.variable()->isKnown() || ((UnknownVariable*) x_var.variable())->interval().isUndefined())) return true;
	if(m.isPower() && m[1].compare(m_zero) == COMPARISON_RESULT_GREATER && m[0].contains(x_var, true) > 0 && comparison_might_be_equal(m[0].compare(m_zero))) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(!check_zero_div(m[i], x_var, eo)) return false;
	}
	/*if(m.isFunction() && m.size() == 1 && (m.function()->id() == FUNCTION_ID_TAN || m.function()->id() == FUNCTION_ID_TANH) && m[0].contains(x_var, true) > 0) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		eo2.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		eo2.assume_denominators_nonzero = false;
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mfunc(m);
		mfunc.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
		bool b = mfunc.calculateFunctions(eo2);
		CALCULATOR->endTemporaryStopMessages();
		if(!b) return false;
	}*/
	return true;
}

// 1/(ln(x)+5)=1/(ln(x*e^5)) if x>0
bool combine_ln(MathStructure &m, const MathStructure &x_var, const EvaluationOptions &eo, int depth = 0) {
	if(m.isAddition() && depth > 0 && m.containsFunctionId(FUNCTION_ID_LOG)) {
		size_t i_log = 0, i_mul_log = 0;
		bool b = false;
		MathStructure *mi = NULL;
		for(size_t i = 0; i < m.size(); i++) {
			if(!b && m[i].isMultiplication() && m[i].containsFunctionId(FUNCTION_ID_LOG) && m[i].contains(x_var, true)) {
				for(size_t i2 = 0; i2 < m[i].size(); i2++) {
					if(!b && m[i][i2].isFunction() && m[i][i2].function()->id() == FUNCTION_ID_LOG && m[i][i2].size() == 1 && m[i][i2][0].contains(x_var)) {
						b = true;
						i_log = i;
						i_mul_log = i2;
					} else if(m[i][i2].containsRepresentativeOf(x_var, true, true) != 0 || !m[i][i2].representsReal(true)) {
						b = false;
						break;
					}
				}
				if(!b) break;
			} else if(!b && m[i].isFunction() && m[i].function()->id() == FUNCTION_ID_LOG && m[i].size() == 1 && m[i][0].contains(x_var)) {
				b = true;
				i_log = i;
				break;
			} else if(!mi && m[i].isMultiplication() && m[i].size() == 2 && m[i][1].isVariable() && m[i][1].variable()->id() == VARIABLE_ID_PI && m[i][0].isNumber() && m[i][0].number().hasImaginaryPart() && !m[i][0].number().hasRealPart() && m[i][0].number().internalImaginary()->isReal()) {
				mi = &m[i][0];
			} else if(m[i].containsRepresentativeOf(x_var, true, true) != 0 || !m[i].representsReal(true)) {
				b = false;
				break;
			}
		}
		if(b && ((m[i_log].isMultiplication() && m[i_log][i_mul_log][0].compare(m_zero) == COMPARISON_RESULT_LESS) || (m[i_log].isFunction() && m[i_log][0].compare(m_zero) == COMPARISON_RESULT_LESS))) {
			MathStructure mmul(1, 1, 0);
			if(mi) mmul = *mi->number().internalImaginary();
			MathStructure *m_e = new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_E));
			MathStructure *mpow = new MathStructure;
			mpow->set_nocopy(m);
			mpow->delChild(i_log + 1, true);
			if(!mmul.isOne()) mpow->calculateDivide(mmul, eo);
			m_e->raise_nocopy(mpow);
			m_e->calculateRaiseExponent(eo);
			m.setToChild(i_log + 1, true);
			if(m.isMultiplication()) {
				MathStructure *mexp = new MathStructure;
				mexp->set_nocopy(m);
				mexp->delChild(i_mul_log + 1, true);
				if(!mmul.isOne()) mexp->calculateDivide(mmul, eo);
				m.setToChild(i_mul_log + 1, true);
				m[0].raise_nocopy(mexp);
				m[0].calculateRaiseExponent(eo);
			} else if(!mmul.isOne()) {
				MathStructure *mexp = new MathStructure(mmul);
				mexp->calculateInverse(eo);
				m[0].raise_nocopy(mexp);
				m[0].calculateRaiseExponent(eo);
			}
			m[0].multiply_nocopy(m_e);
			m[0].calculateMultiplyLast(eo);
			m.childUpdated(1);
			if(!mmul.isOne()) {m.multiply(mmul); m.swapChildren(1, 2);}
			return true;
		}
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(combine_ln(m[i], x_var, eo, depth + 1)) {
			m.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(b_ret) {
		m.calculatesub(eo, eo, false);
	}
	return b_ret;
}

/* Determines the integral of mfac * ((mpowmul * mstruct) + mpowadd)^mpow, where mstruct is a function with x_var in argument.
*/
int integrate_function(MathStructure &mstruct, const MathStructure &x_var, const EvaluationOptions &eo, const MathStructure &mpow, const MathStructure &mfac, const MathStructure &mpowadd, const MathStructure &mpowmul, int use_abs, bool definite_integral, int max_part_depth, vector<MathStructure*> *parent_parts) {
	if(mpow.containsRepresentativeOf(x_var, true, true) != 0) return false;
	// mpow != x
	if(!mpowadd.isZero() || !mpowmul.isOne()) {
		if(!mfac.isOne() || !mpow.isMinusOne()) return false;
		// mpowadd != 0, mpowmul != 1, mfac = 1, mpow = -1: 1/((mpowmul*mstruct)+mpowadd)
		if((mstruct.function()->id() == FUNCTION_ID_SIN || mstruct.function()->id() == FUNCTION_ID_COS || mstruct.function()->id() == FUNCTION_ID_SINH || mstruct.function()->id() == FUNCTION_ID_COSH || mstruct.function()->id() == FUNCTION_ID_LOG) && mstruct.size() == 1) {
			MathStructure mexp, mmul, madd;
			if(integrate_info(mstruct[0], x_var, madd, mmul, mexp, (mstruct.function()->id() == FUNCTION_ID_SIN || mstruct.function()->id() == FUNCTION_ID_COS))) {
				if(mexp.isOne()) {
					bool neg_equals = false;
					if((mstruct.function()->id() == FUNCTION_ID_SIN || mstruct.function()->id() == FUNCTION_ID_COS || mstruct.function()->id() == FUNCTION_ID_COSH) && mpowadd != mpowmul) {
						MathStructure mpowaddneg(mpowadd);
						mpowaddneg.calculateNegate(eo);
						neg_equals = (mpowaddneg == mpowmul);
					}
					if(mstruct.function()->id() == FUNCTION_ID_SIN) {
						if(mpowadd == mpowmul || neg_equals) {
							//1/(c*sin(ax+b)+c): 2*sin((a x + b)/2)*(sin((a x + b)/2)+cos((a x + b)/2))/(a*(c*sin((a x + b))+c))
							MathStructure mdiv(mstruct);
							mstruct[0] *= nr_half;
							MathStructure msin(mstruct);
							MathStructure mcos(mstruct);
							mcos.setFunctionId(FUNCTION_ID_COS);
							mstruct *= nr_two;
							if(mpowadd != mpowmul) msin.negate();
							msin += mcos;
							mstruct *= msin;
							mdiv *= mpowmul;
							mdiv += mpowadd;
							if(!mmul.isOne()) mdiv *= mmul;
							mstruct /= mdiv;
							return true;
						} else {
							//1/(d*sin(ax+b)+c): (2 atan((c*tan((a x + b)/2)+d)/sqrt(c^2-d^2)))/(a*sqrt(c^2-d^2))
							if(definite_integral && x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
								MathStructure mtest(mstruct[0]);
								mtest[0] /= CALCULATOR->getRadUnit();
								mtest[0] /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
								mtest[0] /= nr_two;
								mtest[0].transformById(FUNCTION_ID_ABS);
								mtest[0].transformById(FUNCTION_ID_FRAC);
								ComparisonResult cr = mtest.compare(nr_half);
								if(COMPARISON_MIGHT_BE_EQUAL(cr)) return -1;
							}
							mstruct.setFunctionId(FUNCTION_ID_TAN);
							mstruct[0] *= nr_half;
							mstruct *= mpowadd;
							mstruct += mpowmul;
							MathStructure msqrtc2md2(mpowadd);
							if(!mpowadd.isOne()) msqrtc2md2 ^= nr_two;
							MathStructure mmpowmul2(mpowmul);
							if(!mpowmul.isOne()) mmpowmul2 ^= nr_two;
							mmpowmul2.negate();
							msqrtc2md2 += mmpowmul2;
							msqrtc2md2 ^= Number(-1, 2, 0);
							mstruct *= msqrtc2md2;
							mstruct.transformById(FUNCTION_ID_ATAN);
							mstruct *= msqrtc2md2;
							if(!mmul.isOne()) mstruct /= mmul;
							mstruct *= nr_two;
							return true;
						}
					} else if(mstruct.function()->id() == FUNCTION_ID_COS) {
						if(mpowadd == mpowmul) {
							//1/(c*cos(ax+b)+c): tan((a x + b)/2)/(ac)
							mstruct[0] *= nr_half;
							mstruct.setFunctionId(FUNCTION_ID_TAN);
							if(!mpowadd.isOne()) mstruct /= mpowadd;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						} else if(neg_equals) {
							//1/(c*cos(ax+b)-c): cos((a x + b)/2)/(ac*sin((a x + b)/2))
							mstruct[0] *= nr_half;
							MathStructure msin(mstruct);
							msin.setFunctionId(FUNCTION_ID_SIN);
							mstruct /= msin;
							if(!mpowmul.isOne()) mstruct /= mpowmul;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						} else {
							//1/(d*cos(ax+b)+c): -(2*atanh(((c-d)*tan((a x + b)/2))/sqrt(d^2-c^2)))/(a*sqrt(d^2-c^2))
							if(definite_integral && x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
								MathStructure mtest(mstruct[0]);
								mtest[0] /= CALCULATOR->getRadUnit();
								mtest[0] /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
								mtest[0] /= nr_two;
								mtest[0].transformById(FUNCTION_ID_ABS);
								mtest[0].transformById(FUNCTION_ID_FRAC);
								ComparisonResult cr = mtest.compare(nr_half);
								if(COMPARISON_MIGHT_BE_EQUAL(cr)) return -1;
							}
							mstruct.setFunctionId(FUNCTION_ID_TAN);
							mstruct[0] *= nr_half;
							mstruct *= mpowadd;
							mstruct.last() -= mpowmul;
							mstruct.childUpdated(mstruct.size());
							MathStructure msqrtc2md2(mpowadd);
							if(!mpowadd.isOne()) msqrtc2md2 ^= nr_two;
							msqrtc2md2.negate();
							MathStructure mmpowmul2(mpowmul);
							if(!mpowmul.isOne()) mmpowmul2 ^= nr_two;
							msqrtc2md2 += mmpowmul2;
							msqrtc2md2 ^= Number(-1, 2, 0);
							mstruct *= msqrtc2md2;
							mstruct.transformById(FUNCTION_ID_ATANH);
							mstruct *= msqrtc2md2;
							if(!mmul.isOne()) mstruct /= mmul;
							mstruct *= Number(-2, 1);
							return true;
						}
					} else if(mstruct.function()->id() == FUNCTION_ID_SINH) {
						//1/(d*sinh(ax+b)+c): (2 atan((-c*tan((a x + b)/2)+d)/sqrt(-c^2-d^2)))/(a*sqrt(-c^2-d^2))
						mstruct.setFunctionId(FUNCTION_ID_TANH);
						mstruct[0] *= nr_half;
						mstruct *= mpowadd;
						mstruct.negate();
						mstruct += mpowmul;
						MathStructure msqrtc2md2(mpowadd);
						if(!mpowadd.isOne()) msqrtc2md2 ^= nr_two;
						msqrtc2md2.negate();
						MathStructure mmpowmul2(mpowmul);
						if(!mpowmul.isOne()) mmpowmul2 ^= nr_two;
						mmpowmul2.negate();
						msqrtc2md2 += mmpowmul2;
						msqrtc2md2 ^= Number(-1, 2, 0);
						mstruct *= msqrtc2md2;
						mstruct.transformById(FUNCTION_ID_ATAN);
						mstruct *= msqrtc2md2;
						if(!mmul.isOne()) mstruct /= mmul;
						mstruct *= nr_two;
						return true;
					} else if(mstruct.function()->id() == FUNCTION_ID_COSH) {
						if(mpowadd == mpowmul) {
							//1/(c*cosh(ax+b)+c): tanh((a x + b)/2)/(ac)
							mstruct[0] *= nr_half;
							mstruct.setFunctionId(FUNCTION_ID_TANH);
							if(!mpowadd.isOne()) mstruct /= mpowadd;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						} else if(neg_equals) {
							//1/(c*cosh(ax+b)-c): -cosh((a x + b)/2)/(ac*sinh((a x + b)/2))
							mstruct[0] *= nr_half;
							MathStructure msin(mstruct);
							msin.setFunctionId(FUNCTION_ID_SINH);
							mstruct.negate();
							mstruct /= msin;
							if(!mpowmul.isOne()) mstruct /= mpowmul;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						} else {
							//1/(d*cos(ax+b)+c): -(2*atan(((c-d)*tanh((a x + b)/2))/sqrt(d^2-c^2)))/(a*sqrt(d^2-c^2))
							mstruct.setFunctionId(FUNCTION_ID_TANH);
							mstruct[0] *= nr_half;
							mstruct *= mpowadd;
							mstruct.last() -= mpowmul;
							mstruct.childUpdated(mstruct.size());
							MathStructure msqrtc2md2(mpowadd);
							if(!mpowadd.isOne()) msqrtc2md2 ^= nr_two;
							msqrtc2md2.negate();
							MathStructure mmpowmul2(mpowmul);
							if(!mpowmul.isOne()) mmpowmul2 ^= nr_two;
							msqrtc2md2 += mmpowmul2;
							msqrtc2md2 ^= Number(-1, 2, 0);
							mstruct *= msqrtc2md2;
							mstruct.transformById(FUNCTION_ID_ATAN);
							mstruct *= msqrtc2md2;
							if(!mmul.isOne()) mstruct /= mmul;
							mstruct *= Number(-2, 1);
							return true;
						}
					} else if(mstruct.function()->id() == FUNCTION_ID_LOG) {
						//1/(d*ln(ax+b)+c): (e^(-c/d)*Ei(c/d+ln(ax+b)))/(a*d)
						MathStructure mpadm(mpowadd);
						if(!mpowmul.isOne()) mpadm /= mpowmul;
						mstruct += mpadm;
						mstruct.transformById(FUNCTION_ID_EXPINT);
						mpadm.negate();
						mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_E);
						mstruct.last() ^= mpadm;
						mstruct.childUpdated(mstruct.size());
						if(!mmul.isOne()) mstruct /= mmul;
						if(!mpowmul.isOne()) mstruct /= mpowmul;
						return true;
					}
				} else if(mstruct.function()->id() == FUNCTION_ID_LOG && madd.isZero()) {
					//1/(d*ln(ax^b)+c): (x*(ax^b)^(-1/b)*e^(-c/(bd))*Ei((c+d*ln(ax^b))/(bd)))/(bd)
					MathStructure mem(mexp);
					if(!mpowmul.isOne()) mem *= mpowmul;
					mem.inverse();
					MathStructure marg(mstruct[0]);
					mexp.inverse();
					mexp.negate();
					marg ^= mexp;
					MathStructure mexpe(CALCULATOR->getVariableById(VARIABLE_ID_E));
					if(!mpowadd.isZero()) {
						MathStructure mepow(mpowadd);
						mepow.negate();
						mepow *= mem;
						mexpe ^= mepow;
					}
					if(!mpowmul.isOne()) mstruct *= mpowmul;
					if(!mpowadd.isZero()) mstruct += mpowadd;
					mstruct *= mem;
					mstruct.transformById(FUNCTION_ID_EXPINT);
					if(!mpowadd.isZero()) mstruct *= mexpe;
					mstruct *= marg;
					mstruct *= x_var;
					mstruct *= mem;
					return true;
				}
			}
		}
		return false;
	}
	// mpowadd=0 and mpowmul=1: mfac*mstruct^mpow

	if(mstruct.function()->id() == FUNCTION_ID_LOG && mstruct.size() == 1) {
		if(mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct[0]) && mpow.isOne() && mfac.isOne() && (!definite_integral || comparison_is_not_equal(mstruct[0][0].compare(m_zero)))) {
			MathStructure mexp, mmul, madd;
			if(integrate_info(mstruct[0][0], x_var, madd, mmul, mexp) && mexp.isOne()) {
				if(madd.isZero()) {
					if(mmul.isZero()) {
						mstruct *= x_var;
						mstruct += x_var;
						mstruct.last().divide(mstruct[0][1]);
						mstruct.childUpdated(mstruct.size());
						mstruct.last().negate();
						return true;
					}
					MathStructure mterm(mstruct[0]);
					mterm ^= nr_two;
					mterm *= mstruct[0];
					mterm.inverse();
					if(!mmul.isOne()) mterm *= mmul;
					mterm *= x_var;
					mterm.last() ^= nr_two;
					mterm *= Number(-1, 3);
					mstruct *= x_var;
					mstruct += mterm;
					return true;
				}
				MathStructure mterm1(x_var);
				if(!mmul.isOne()) mterm1 *= mmul;
				if(!madd.isOne()) mterm1 /= madd;
				mterm1 += m_one;
				if(!transform_absln(mterm1, use_abs, definite_integral, x_var, eo)) return -1;
				if(!mmul.isOne()) mterm1 /= mmul;
				if(!madd.isOne()) mterm1 *= madd;
				mterm1 /= mstruct[0][1];
				MathStructure mterm2(x_var);
				mterm2 /= mstruct[0][1];
				mstruct *= x_var;
				mstruct += mterm1;
				mstruct -= mterm2;
				return true;
			}
		}
		MathStructure mexp, mmul, madd;
		if(integrate_info(mstruct[0], x_var, madd, mmul, mexp, false, false, true) && (mexp == x_var || (mexp.isPower() && mexp[0] == x_var && ((mexp[1] == x_var && madd.isZero() && mpow.isOne() && (!definite_integral || x_var.representsNonNegative(true))) || mexp[1].containsRepresentativeOf(x_var, true, true) == 0)) || (mpow.isOne() && madd.isZero() && mexp.isFunction() && mexp.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mexp) && mexp[0] == x_var) || (mpow.isOne() && madd.isZero() && mexp.isPower() && mexp[1].isInteger() && mexp[0].isFunction() && mexp[0].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mexp[0]) && mexp[0][0] == x_var))) {
			bool b_root =  false;
			if(mexp == x_var) mexp.set(1, 1, 0);
			else if(mexp.isPower() && mexp[0] == x_var) mexp.setToChild(2);
			else if(mexp.isPower()) {mexp[0].setToChild(2); mexp[0].number().recip(); mexp[0].number() *= mexp[1].number(); mexp.setToChild(1); b_root = true;}
			else {mexp.setToChild(2); mexp.number().recip(); b_root = true;}
			bool do_if = false;
			// if mstruct[0]=0 for lower or upper limit do if(mstruct[0]=0,0,f(x))
			if(definite_integral && x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
				MathStructure mtest(((UnknownVariable*) x_var.variable())->interval());
				mtest.eval(eo);
				if(mtest.isNumber()) {
					MathStructure mtest2(mstruct[0]);
					mtest2.replace(x_var, mtest.number().lowerEndPoint());
					mtest2.eval(eo);
					do_if = (mtest2.isNumber() && !mtest2.number().isNonZero());
					if(!do_if) {
						mtest2 = mstruct[0];
						mtest2.replace(x_var, mtest.number().upperEndPoint());
						mtest2.eval(eo);
						do_if = (mtest2.isNumber() && !mtest2.number().isNonZero());
					}
				}
			}
			MathStructure mif;
			MathStructure marg(mstruct[0]);
			bool b = false;
			if(mexp == x_var) {
				if(mfac.isOne()) {
					// ln(x^x): x*ln(x^x)-x^2*(ln(x)*2+1)/4
					mstruct *= x_var;
					MathStructure mterm(x_var);
					mterm.transformById(FUNCTION_ID_LOG);
					mterm *= nr_two;
					mterm += m_one;
					mterm *= x_var;
					mterm.last() ^= nr_two;
					mterm *= Number(-1, 4);
					mstruct += mterm;
					b = true;
				} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
					// ln(x^x)*x^a: -x(-(a+2)^2*ln(x^x)+(a+2)(x*ln(x))+(a+1)x)/((a+1)(a+2)^2)
					if(mfac == x_var) mexp.set(1, 1, 0);
					else mexp = mfac[1];
					MathStructure mexpp1(mexp);
					mexpp1 += m_one;
					MathStructure mexpp2(mexp);
					mexpp2 += nr_two;
					MathStructure mterm1(x_var);
					mterm1.transformById(FUNCTION_ID_LOG);
					mterm1 *= x_var;
					mterm1 *= mexpp2;
					MathStructure mterm2(x_var);
					mterm2 *= mexpp1;
					mstruct.negate();
					mexpp2 ^= nr_two;
					mstruct *= mexpp2;
					mstruct += mterm1;
					mstruct += mterm2;
					mstruct *= x_var;
					mstruct.last() ^= mexpp1;
					mexpp2 *= mexpp1;
					mstruct /= mexpp2;
					mstruct.negate();
					b = true;
				}
			} else if(mfac.isOne() && (mexp.isOne() || madd.isZero())) {
				if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-1)) {
					if(mpow.isOne()) {
						if(madd.isZero()) {
							// ln(ax^b): x(ln(ax^b)-b)
							mstruct -= mexp;
							mstruct.multiply(x_var);
							b = true;
						} else {
							// ln(ax+b): (ax+b)(ln(ax)-1)/a
							MathStructure marg(mstruct[0]);
							mstruct *= marg;
							mstruct -= marg;
							if(!mmul.isOne()) mstruct.divide(mmul);
							b = true;
						}
					} else if(mpow.number().isTwo()) {
						// ln(ax+b)^2: (ln(ax+b)^2-2*ln(ax+b)+2)(ax+b)/a
						// ln(ax^b)^2: x(ln(ax^b)^2-2*ln(ax^b)*b+2b^2)
						MathStructure marg(mstruct[0]);
						MathStructure mterm2(mstruct);
						mstruct ^= mpow;
						mterm2 *= Number(-2, 1);
						if(mexp.isOne()) {
							mterm2 += nr_two;
						} else {
							mterm2 *= mexp;
							mterm2 += mexp;
							mterm2.last() ^= nr_two;
							mterm2.last() *= nr_two;
						}
						mstruct += mterm2;
						if(madd.isZero()) {
							mstruct.multiply(x_var);
						} else {
							mstruct *= marg;
							if(!mmul.isOne()) mstruct /= mmul;
						}
						b = true;
					} else if(mpow.number().isMinusOne()) {
						if(mexp.isOne()) {
							// ln(ax+b)^-1: li(ax+b)/a
							if(!definite_integral || comparison_is_not_equal(mstruct[0].compare(m_zero))) {
								mstruct.setFunctionId(FUNCTION_ID_LOGINT);
								if(!mmul.isOne()) mstruct /= mmul;
								b = true;
								do_if = false;
							}
						}
					} else {
						// ln(ax^b)^n: x*sum(ln(ax^b)^i*n!/i!*(-b)^(n-i), 0, n)
						// ln(ax+b)^n: (ax+b)/a*sum(ln(ax+b)^i*n!/i!*(-1)^(n-i), 0, n)
						unsigned long int n = mpow.number().uintValue();
						MathStructure marg(mstruct[0]);
						Number nfac(mpow.number());
						nfac.factorial();
						for(size_t i = 0; i <= n; i++) {
							MathStructure mterm(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &marg, NULL);
							mterm ^= Number(i);
							mterm *= nfac;
							Number ifac(i);
							ifac.factorial();
							mterm /= ifac;
							MathStructure m1pow(mexp);
							m1pow.negate();
							m1pow ^= Number(n - i);
							mterm *= m1pow;
							if(i == 0) mstruct = mterm;
							else mstruct += mterm;
						}
						if(madd.isZero()) {
							mstruct.multiply(x_var);
						} else {
							mstruct *= marg;
							if(!mmul.isOne()) mstruct /= mmul;
						}
						b = true;
					}
				}
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne()) {
					if(mpow.isMinusOne()) {
						if(mexp.isOne() && madd.isZero()) {
							mstruct.transformById(FUNCTION_ID_LOG);
							b = true;
						}
					} else if(madd.isZero()) {
						MathStructure mpowp1(mpow);
						mpowp1 += m_one;
						mstruct ^= mpowp1;
						mstruct /= mpowp1;
						if(!mexp.isOne()) mstruct /= mexp;
						b = true;
					} else if(mpow.isOne()) {
						MathStructure m_axcb(x_var);
						if(!mexp.isOne()) m_axcb ^= mexp;
						if(!mmul.isOne()) m_axcb *= mmul;
						if(!madd.isZero()) m_axcb /= madd;
						mstruct *= m_axcb;
						mstruct.last().negate();
						mstruct.last().transformById(FUNCTION_ID_LOG);
						MathStructure mterm2(m_axcb);
						mterm2 += m_one;
						mterm2.transformById(FUNCTION_ID_POLYLOG);
						mterm2.insertChild(nr_two, 1);
						mstruct += mterm2;
						if(!mexp.isOne()) mstruct /= mexp;
						b = true;
					}
					do_if = do_if && !madd.isZero();
				} else if(madd.isZero()) {
					if(mpow.isOne()) {
						mfacexp += m_one;
						mstruct *= mfacexp;
						mstruct -= mexp;
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mfacexp ^= Number(-2, 1);
						mstruct *= mfacexp;
						mstruct.childrenUpdated();
						b = true;
					} else if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100)) {
						if(mpow.isMinusOne()) {
							if(!definite_integral || comparison_is_not_equal(mstruct[0].compare(m_zero))) {
								if(mexp.isOne()) {
									MathStructure mmulfac(mmul);
									if(!mmul.isOne()) {
										mmulfac *= x_var;
										mmulfac ^= mfacexp;
										mmulfac[1].negate();
										mmulfac *= x_var;
										mmulfac.last() ^= mfacexp;
										mmulfac.childrenUpdated(true);
									}
									mfacexp += m_one;
									mstruct *= mfacexp;
									mstruct.transformById(FUNCTION_ID_EXPINT);
									if(!mmul.isOne()) {
										mstruct *= mmulfac;
										mstruct /= mmul;
									}
									b = true;
								} else {
									MathStructure mepow(mstruct);
									mfacexp += m_one;
									mstruct *= mfacexp;
									mstruct /= mexp;
									mstruct.transformById(FUNCTION_ID_EXPINT);
									mepow.negate();
									mepow += x_var;
									mepow.last().transformById(FUNCTION_ID_LOG);
									mepow.last() *= mexp;
									mepow *= mfacexp;
									mepow /= mexp;
									MathStructure memul(CALCULATOR->getVariableById(VARIABLE_ID_E));
									memul ^= mepow;
									mstruct *= memul;
									mstruct /= mexp;
									b = true;
								}
							}
						} else if(mpow.number().isNegative()) {
							if(mexp.isOne()) {
								MathStructure mpowp1(mpow);
								mpowp1 += m_one;
								MathStructure mpowp1n(mpowp1);
								mpowp1n.negate();
								MathStructure mterm(mstruct);
								mstruct ^= mpowp1;
								mstruct *= x_var;
								mstruct.last() ^= mfacexp;
								if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
								mfacexp += m_one;
								mstruct *= mfacexp;
								mstruct /= mpowp1n;
								mterm ^= mpowp1;
								mterm *= x_var;
								mterm.last() ^= mfacexp;
								mterm /= mpowp1n;
								mstruct -= mterm;
								mstruct.childrenUpdated(true);
								b = true;
							}
						} else {
							MathStructure mterm(mstruct);
							mstruct ^= mpow;
							mstruct.last() += nr_minus_one;
							mstruct *= x_var;
							mstruct.last() ^= mfacexp;
							if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
							mstruct *= mpow;
							mfacexp += m_one;
							mstruct /= mfacexp;
							mstruct.negate();
							mstruct *= mexp;
							mterm ^= mpow;
							mterm *= x_var;
							mterm.last() ^= mfacexp;
							mterm /= mfacexp;
							mstruct += mterm;
							mstruct.childrenUpdated(true);
							b = true;
						}
					}
				} else if(mfac == x_var && mexp.isOne()) {
					if(mpow.isOne()) {
						MathStructure mterm2(x_var);
						if(!mmul.isOne()) mterm2 *= mmul;
						mterm2 += madd;
						mterm2.last() *= Number(-2, 1);
						mterm2 *= x_var;
						if(!mmul.isOne()) mterm2 /= mmul;
						mterm2 *= Number(-1, 4);
						MathStructure marg(x_var);
						if(!mmul.isOne()) marg *= mmul;
						marg += madd;
						mstruct *= marg;
						marg.last() *= nr_minus_one;
						mstruct *= marg;
						if(!mmul.isOne()) {
							mstruct *= mmul;
							mstruct.last() ^= Number(-2, 1);
						}
						mstruct *= nr_half;
						mstruct += mterm2;
						mstruct.childrenUpdated(true);
						b = true;
						if(do_if) mif = mterm2;
					}
				}
			} else if(!b_root && mfac.isFunction() && mfac.function()->id() == FUNCTION_ID_LOG && mfac.size() == 1 && madd.isZero() && mpow.isOne()) {
				MathStructure mexp2, mmul2, madd2;
				if(integrate_info(mfac[0], x_var, madd2, mmul2, mexp2) && madd2.isZero()) {
					MathStructure mterm2(mfac);
					mterm2.negate();
					mterm2 += nr_two;
					if(!mexp2.isOne()) {
						mterm2.last() *= mexp2;
						mterm2.childUpdated(mterm2.size());
					}
					if(!mexp.isOne()) mterm2 *= mexp;
					mstruct *= mfac;
					mstruct.last() -= mexp2;
					mstruct.childUpdated(mstruct.size());
					mstruct += mterm2;
					mstruct *= x_var;
					b = true;
				}
			}
			if(b) {
				if(do_if) {
					mstruct.transformById(FUNCTION_ID_IF);
					mstruct.insertChild(mif, 1);
					mstruct.insertChild(marg, 1);
					mstruct.addChild(m_zero);
					mstruct[0].transform(COMPARISON_EQUALS, m_zero);
				}
				return true;
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_LAMBERT_W && mstruct.size() >= 1) {
		if(mfac == x_var && mpow.isOne() && mstruct[0] == x_var) {
			MathStructure mthis(mstruct);
			mstruct ^= nr_two;
			mstruct *= nr_two;
			mstruct += nr_one;
			mstruct *= mthis;
			mstruct.last() *= nr_two;
			mstruct.last() += nr_minus_one;
			mstruct *= x_var;
			mstruct.last() ^= nr_two;
			mstruct *= mthis;
			mstruct.last() ^= Number(-2, 1);
			mstruct *= Number(1, 8);
			return true;
		} else if(mfac.isPower() && mfac[0] == x_var && mfac[1].isMinusOne() && mpow.isOne() && mstruct[0] == x_var) {
			MathStructure mthis(mstruct);
			mstruct += nr_two;
			mstruct *= mthis;
			mstruct *= nr_half;
			return true;
		} else if((mpow.isOne() || (mpow.isNumber() && mpow.number().isTwo())) && mfac.isOne()) {
			MathStructure mexp, mmul, madd;
			if(integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
				if(mpow.isOne()) {
					MathStructure mthis(mstruct);
					mstruct ^= nr_two;
					mstruct -= mthis;
					mstruct += m_one;
					mstruct *= mthis[0];
					mstruct /= mthis;
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else {
					MathStructure mthis(mstruct);
					mstruct ^= nr_three;
					mstruct += mthis;
					mstruct.last() ^= nr_two;
					mstruct.last() *= Number(-2, 1);
					mstruct += mthis;
					mstruct.last() *= Number(4, 1);
					mstruct += Number(-4, 1);
					mstruct *= mthis[0];
					mstruct /= mthis;
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				}
			}
		}
		return false;
	} else if(mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2) {
		if(mstruct[0].representsNonComplex(true) && mpow.isNumber() && mpow.number().isRational() && mpow.number().isPositive()) {
			MathStructure minteg(x_var);
			if(!mfac.isOne()) {
				minteg = mfac;
				if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
			}
			if(!mpow.number().isInteger()) {
				MathStructure mmul(-1, 1, 0);
				Number nr_frac(mpow.number());
				Number nr_int(mpow.number());
				nr_frac.frac();
				nr_int.trunc();
				mmul ^= nr_frac;
				if(nr_int.isEven()) mmul += nr_minus_one;
				else mmul += m_one;
				mstruct *= mmul;
				mstruct -= mmul[0];
				if(nr_int.isEven()) mstruct += nr_minus_one;
				else mstruct += m_one;
				if(nr_int.isEven()) mstruct.negate();
				mstruct /= nr_two;
			} else if(mpow.number().isEven()) {
				mstruct ^= nr_two;
				mstruct += nr_three;
				mstruct *= Number(1, 4);
			}
			mstruct *= minteg;
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_DIRAC && mstruct.size() == 1 && mstruct[0].representsNonComplex(true)) {
		MathStructure mexp, mmul, madd;
		if(mpow.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(mfac == x_var && madd.representsNonZero()) {
				mstruct.setFunctionId(FUNCTION_ID_HEAVISIDE);
				if(!mmul.isOne()) {
					mmul ^= nr_two;
					mstruct /= mmul;
				}
				mstruct *= madd;
				mstruct.negate();
				return true;
			} else if(mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0 && madd.representsNonZero()) {
				mstruct.setFunctionId(FUNCTION_ID_HEAVISIDE);
				madd.negate();
				madd ^= mfac[1];
				mstruct *= madd;
				if(mmul.isOne()) {
					mexp = mfac[1];
					mexp += m_one;
					mexp.negate();
					mmul ^= mexp;
					mstruct *= mmul;
				}
				return true;
			} else if(mfac.isOne()) {
				mstruct.setFunctionId(FUNCTION_ID_HEAVISIDE);
				if(!mmul.isOne()) mstruct /= mmul;
				return true;
			}
		}
		return false;
	} else if(mstruct.function()->id() == FUNCTION_ID_ARG && mstruct.size() == 1 && mstruct[0].representsNonComplex(true)) {
		MathStructure mexp, mmul, madd;
		if(mpow.representsPositive() && integrate_info(mstruct[0], x_var, madd, mmul, mexp)) {
			MathStructure minteg(x_var);
			if(!mfac.isOne()) {
				minteg = mfac;
				if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
			}
			mstruct.setFunctionId(FUNCTION_ID_SIGNUM);
			mstruct += nr_minus_one;
			mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			if(!mpow.isOne()) {
				mstruct.last() ^= mpow;
				mstruct.childUpdated(mstruct.size());
			}
			mstruct *= Number(-1, 2);
			mstruct *= minteg;
			return true;
		}
		return false;
	} else if(mstruct.function()->id() == FUNCTION_ID_HEAVISIDE && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(mfac.isOne()) {
				if(mmul.representsNonNegative()) {
					MathStructure mfacn(x_var);
					if(!madd.isZero()) {
						if(!mmul.isOne()) madd /= mmul;
						mfacn += madd;
					}
					mstruct *= mfacn;
					return true;
				} else if(mmul.representsNegative()) {
					if(madd.isZero()) {
						mstruct[0] = x_var;
						mstruct *= x_var;
						mstruct.negate();
						mstruct += x_var;
						return true;
					}
					mstruct.setToChild(1);
					mmul.negate();
					madd.negate();
					mstruct /= mmul;
					MathStructure mfacn(x_var);
					mfacn *= mmul;
					mfacn += madd;
					mfacn.transformById(FUNCTION_ID_HEAVISIDE);
					mstruct *= mfacn;
					mstruct += x_var;
					return true;
				}
			}
		}
		return false;
	} else if(mstruct.function()->id() == FUNCTION_ID_SINC && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mfac.isOne() && mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-2) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(mpow.isOne()) {
				if(mstruct.function()->getDefaultValue(2) == "pi") {mstruct[0] *= CALCULATOR->getVariableById(VARIABLE_ID_PI); mmul *= CALCULATOR->getVariableById(VARIABLE_ID_PI);}
				mstruct.setFunctionId(FUNCTION_ID_SININT);
				if(!mmul.isOne()) mstruct.divide(mmul);
				return true;
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_SIN && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp, true)) {
			if(mfac.isOne()) {
				if(mexp.isOne()) {
					if(mpow.isOne()) {
						mstruct.setFunctionId(FUNCTION_ID_COS);
						mstruct.multiply(m_minus_one);
						if(!mmul.isOne()) mstruct.divide(mmul);
					} else if(mpow.number().isTwo()) {
						if(!madd.isZero()) {
							mstruct[0] = x_var;
							mstruct[0] *= CALCULATOR->getRadUnit();
						}
						mstruct[0] *= nr_two;
						mstruct /= 4;
						if(madd.isZero() && !mmul.isOne()) mstruct /= mmul;
						mstruct.negate();
						MathStructure xhalf(x_var);
						xhalf *= nr_half;
						mstruct += xhalf;
						if(!madd.isZero()) {
							MathStructure marg(x_var);
							if(!mmul.isOne()) marg *= mmul;
							marg += madd;
							mstruct.replace(x_var, marg);
							if(!mmul.isOne()) mstruct.divide(mmul);
						}
					} else if(mpow.number().isMinusOne()) {
						MathStructure mcot(mstruct);
						mcot.setFunctionId(FUNCTION_ID_TAN);
						mcot.inverse();
						mstruct.inverse();
						mstruct += mcot;
						if(!transform_absln(mstruct, use_abs, definite_integral, x_var, eo)) return -1;
						if(!mmul.isOne()) mstruct.divide(mmul);
						mstruct.negate();
					} else if(mpow.number() == -2) {
						mstruct.setFunctionId(FUNCTION_ID_TAN);
						mstruct.inverse();
						mstruct.negate();
						if(!mmul.isOne()) mstruct.divide(mmul);
					} else if(mpow.number().isPositive()) {
						MathStructure mbak(mstruct);
						MathStructure nm1(mpow);
						nm1 += nr_minus_one;
						mstruct ^= nm1;
						MathStructure mcos(mbak);
						mcos.setFunctionId(FUNCTION_ID_COS);
						mstruct *= mcos;
						mstruct.negate();
						mmul *= mpow;
						mstruct /= mmul;
						MathStructure minteg(mbak);
						MathStructure nm2(mpow);
						nm2 += Number(-2, 1);
						minteg ^= nm2;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg *= nm1;
						minteg /= mpow;
						mstruct += minteg;
					} else {
						MathStructure mbak(mstruct);
						MathStructure np1(mpow);
						np1 += m_one;
						mstruct ^= np1;
						MathStructure mcos(mbak);
						mcos.setFunctionId(FUNCTION_ID_COS);
						mstruct *= mcos;
						mstruct /= np1;
						mstruct /= mmul;
						MathStructure minteg(mbak);
						MathStructure np2(mpow);
						np2 += nr_two;
						minteg ^= np2;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg *= np2;
						minteg /= np1;
						mstruct += minteg;
					}
					return true;
				} else if(mexp.isNumber() && mexp.number().isTwo() && madd.isZero() && mpow.isOne()) {
					mstruct = nr_two;
					mstruct /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					mstruct ^= nr_half;
					mstruct *= x_var;
					mstruct *= mmul;
					mstruct.last() ^= nr_half;
					mstruct.transformById(FUNCTION_ID_FRESNEL_S);
					mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					mstruct.last() *= nr_half;
					mstruct.last() ^= nr_half;
					mstruct *= mmul;
					mstruct.last() ^= Number(-1, 2);
					return true;
				}
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne() && !mexp.isZero()) {
					if(madd.isZero()) {
						if(mpow.isOne()) {
							mstruct[0] /= CALCULATOR->getRadUnit();
							mstruct.setFunctionId(FUNCTION_ID_SININT);
							if(!mexp.isOne()) mstruct /= mexp;
							return true;
						} else if(mpow.number().isTwo()) {
							mstruct[0] *= nr_two;
							mstruct[0] /= CALCULATOR->getRadUnit();
							mstruct.setFunctionId(FUNCTION_ID_COSINT);
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct.negate();
							mstruct += x_var;
							if(!transform_absln(mstruct.last(), use_abs, definite_integral, x_var, eo)) return -1;
							mstruct *= nr_half;
							return true;
						} else if(mpow.number() == 3) {
							mstruct[0] /= CALCULATOR->getRadUnit();
							mstruct.setFunctionId(FUNCTION_ID_SININT);
							MathStructure mterm2(mstruct);
							mstruct[0] *= nr_three;
							mterm2 *= Number(-3, 1);
							mstruct += mterm2;
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct *= Number(-1, 4);
							return true;
						}
					} else if(mpow.isOne()) {
						MathStructure mterm2;
						mstruct = x_var;
						if(!mexp.isOne()) mstruct ^= mexp;
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct.transformById(FUNCTION_ID_SININT);
						madd *= CALCULATOR->getRadUnit();
						mstruct *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &madd, NULL);
						mterm2 = x_var;
						if(!mexp.isOne()) mterm2 ^= mexp;
						if(!mmul.isOne()) mterm2 *= mmul;
						mterm2.transformById(FUNCTION_ID_COSINT);
						mterm2 *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), &madd, NULL);
						mstruct += mterm2;
						if(!mexp.isOne()) mstruct /= mexp;
						return true;
					}
				} else if(mexp.isOne() && mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct);
						mterm2.setFunctionId(FUNCTION_ID_COS);
						mterm2 *= x_var;
						if(!mmul.isOne()) {
							mterm2 /= mmul;
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct -= mterm2;
						return true;
					} else if(mfacexp.isInteger() && mfacexp.number().isPositive() && mfacexp.number().isLessThan(100)) {
						mstruct.setFunctionId(FUNCTION_ID_COS);
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						mterm2.childUpdated(mterm2.size());
						if(!mmul.isOne()) mterm2 /= mmul;
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.childUpdated(mstruct.size());
						mstruct.last().last() += nr_minus_one;
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct *= mfacexp;
						if(!mmul.isOne()) mstruct /= mmul;
						mstruct -= mterm2;
						return true;
					} else if(madd.isZero() && mfacexp.isInteger() && mfacexp.number().isNegative() && mfacexp.number().isGreaterThan(-100)) {
						mfacexp += m_one;
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						mterm2.childUpdated(mterm2.size());
						mterm2 /= mfacexp;
						mstruct.setFunctionId(FUNCTION_ID_COS);
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.childUpdated(mstruct.size());
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct /= mfacexp;
						mstruct.negate();
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct += mterm2;
						return true;
					}
				} else if(mexp.isOne() && mpow.number().isTwo()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct);
						MathStructure mterm3(x_var);
						mterm3 ^= nr_two;
						mterm3 *= Number(1, 4);
						mterm2[0] *= nr_two;
						mterm2 *= x_var;
						if(!mmul.isOne()) mterm2 /= mmul;
						mterm2 *= Number(-1, 4);
						mstruct.setFunctionId(FUNCTION_ID_COS);
						mstruct[0] *= nr_two;
						if(!mmul.isOne()) {
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct *= Number(-1, 8);
						mstruct += mterm2;
						mstruct += mterm3;
						mstruct.childrenUpdated(true);
						return true;
					} else if(mfacexp.number().isTwo()) {
						MathStructure mterm2(mstruct);
						MathStructure mterm3(x_var);
						mterm3 ^= nr_three;
						mterm3 *= Number(1, 6);
						mterm2[0] *= nr_two;
						MathStructure mterm21(1, 8, 0);
						if(!mmul.isOne()) {
							mterm21 *= mmul;
							mterm21.last() ^= Number(-3, 1);
						}
						MathStructure mterm22(x_var);
						mterm22 ^= 2;
						if(!mmul.isOne()) mterm22 /= mmul;
						mterm22 *= Number(-1, 4);
						mterm21 += mterm22;
						mterm2 *= mterm21;
						mstruct.setFunctionId(FUNCTION_ID_COS);
						mstruct[0] *= nr_two;
						mstruct *= x_var;
						if(!mmul.isOne()) {
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct *= Number(-1, 4);
						mstruct += mterm2;
						mstruct += mterm3;
						mstruct.childrenUpdated(true);
						return true;
					}
				}
			} else if(mexp.isOne() && mfac.isFunction()) {
				if(mfac.function()->id() == FUNCTION_ID_SIN && mfac.size() == 1 && mpow.isOne()) {
					MathStructure mexpf, mmulf, maddf;
					if(integrate_info(mfac[0], x_var, maddf, mmulf, mexpf, true) && mexpf.isOne() && mmul != mmulf) {
						MathStructure mterm2(mstruct);
						mterm2[0] += mfac[0];
						mstruct[0] -= mfac[0];
						MathStructure mden1(mmul);
						mden1 -= mmulf;
						mden1 *= nr_two;
						MathStructure mden2(mmul);
						mden2 += mmulf;
						mden2 *= nr_two;
						mterm2 /= mden2;
						mstruct /= mden1;
						mstruct -= mterm2;
						return true;
					}
				} else if(mfac.function()->id() == FUNCTION_ID_COS && mfac.size() == 1) {
					if(mstruct[0] == mfac[0]) {
						UnknownVariable *var = new UnknownVariable("", format_and_print(mstruct));
						MathStructure mtest(var);
						if(!mpow.isOne()) mtest ^= mpow;
						CALCULATOR->beginTemporaryStopMessages();
						if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
							MathStructure m_interval(mstruct);
							m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
							var->setInterval(m_interval);
						} else {
							var->setInterval(mstruct);
						}
						if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							mtest.replace(var, mstruct);
							var->destroy();
							mstruct = mtest;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						}
						CALCULATOR->endTemporaryStopMessages();
						var->destroy();
					} else if(mpow.isOne()) {
						MathStructure mexpf, mmulf, maddf;
						if(integrate_info(mfac[0], x_var, maddf, mmulf, mexpf, true) && mexpf.isOne()) {
							if(mmul != mmulf) {
								mstruct.setFunctionId(FUNCTION_ID_COS);
								MathStructure mterm2(mstruct);
								mterm2[0] += mfac[0];
								mstruct[0] -= mfac[0];
								MathStructure mden1(mmul);
								mden1 -= mmulf;
								mden1 *= nr_two;
								MathStructure mden2(mmul);
								mden2 += mmulf;
								mden2 *= nr_two;
								mterm2 /= mden2;
								mstruct /= mden1;
								mstruct.negate();
								mstruct -= mterm2;
								return true;
							} else if(madd == maddf) {
								mstruct ^= nr_two;
								if(!mmul.isOne()) mstruct /= mmul;
								mstruct *= nr_half;
								return true;
							} else {
								MathStructure mterm2(mfac);
								mterm2[0].add(mstruct[0]);
								mterm2.childUpdated(1);
								if(!mmul.isOne()) mterm2 /= mmul;
								mterm2 *= Number(-1, 4);
								mstruct[0] = maddf;
								mstruct[0] -= madd;
								mstruct[0] *= CALCULATOR->getRadUnit();
								mstruct.childUpdated(1);
								mstruct *= x_var;
								mstruct *= Number(-1, 2);
								mstruct += mterm2;
								return true;
							}
						}
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_COS && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp, true)) {
			if(mfac.isOne()) {
				if(mexp.isOne()) {
					if(mpow.isOne()) {
						mstruct.setFunctionId(FUNCTION_ID_SIN);
						if(!mmul.isOne()) mstruct.divide(mmul);
					} else if(mpow.number().isTwo()) {
						mstruct.setFunctionId(FUNCTION_ID_SIN);
						if(!madd.isZero()) {
							mstruct[0] = x_var;
							mstruct[0] *= CALCULATOR->getRadUnit();
						}
						mstruct[0] *= nr_two;
						mstruct /= 4;
						if(madd.isZero() && !mmul.isOne()) mstruct /= mmul;
						MathStructure xhalf(x_var);
						xhalf *= nr_half;
						mstruct += xhalf;
						if(!madd.isZero()) {
							MathStructure marg(x_var);
							if(!mmul.isOne()) marg *= mmul;
							marg += madd;
							mstruct.replace(x_var, marg);
							if(!mmul.isOne()) mstruct.divide(mmul);
						}
					} else if(mpow.number().isMinusOne()) {
						MathStructure mtan(mstruct);
						mtan.setFunctionId(FUNCTION_ID_TAN);
						mstruct.inverse();
						mstruct += mtan;
						if(!transform_absln(mstruct, use_abs, definite_integral, x_var, eo)) return -1;
						if(!mmul.isOne()) mstruct.divide(mmul);
					} else if(mpow.number() == -2) {
						mstruct.setFunctionId(FUNCTION_ID_TAN);
						if(!mmul.isOne()) mstruct.divide(mmul);
					} else if(mpow.number().isPositive()) {
						MathStructure mbak(mstruct);
						MathStructure nm1(mpow);
						nm1 += nr_minus_one;
						mstruct ^= nm1;
						MathStructure msin(mbak);
						msin.setFunctionId(FUNCTION_ID_SIN);
						mstruct *= msin;
						mmul *= mpow;
						mstruct /= mmul;
						MathStructure minteg(mbak);
						MathStructure nm2(mpow);
						nm2 += Number(-2, 1);
						minteg ^= nm2;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg *= nm1;
						minteg /= mpow;
						mstruct += minteg;
					} else {
						MathStructure mbak(mstruct);
						MathStructure np1(mpow);
						np1 += m_one;
						mstruct ^= np1;
						MathStructure mcos(mbak);
						mcos.setFunctionId(FUNCTION_ID_SIN);
						mstruct *= mcos;
						mstruct /= np1;
						mstruct /= mmul;
						mstruct.negate();
						MathStructure minteg(mbak);
						MathStructure np2(mpow);
						np2 += nr_two;
						minteg ^= np2;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg *= np2;
						minteg /= np1;
						mstruct += minteg;
					}
					return true;
				} else if(mexp.isNumber() && mexp.number().isTwo() && madd.isZero() && mpow.isOne()) {
					mstruct = nr_two;
					mstruct /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					mstruct ^= nr_half;
					mstruct *= x_var;
					mstruct *= mmul;
					mstruct.last() ^= nr_half;
					mstruct.transformById(FUNCTION_ID_FRESNEL_C);
					mstruct *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					mstruct.last() *= nr_half;
					mstruct.last() ^= nr_half;
					mstruct *= mmul;
					mstruct.last() ^= Number(-1, 2);
					return true;
				}
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne() && !mexp.isZero()) {
					if(madd.isZero()) {
						if(mpow.isOne()) {
							mstruct[0] /= CALCULATOR->getRadUnit();
							mstruct.setFunctionId(FUNCTION_ID_COSINT);
							if(!mexp.isOne()) mstruct /= mexp;
							return true;
						} else if(mpow.number().isTwo()) {
							mstruct[0] *= nr_two;
							mstruct[0] /= CALCULATOR->getRadUnit();
							mstruct.setFunctionId(FUNCTION_ID_COSINT);
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct += x_var;
							mstruct.last().transformById(FUNCTION_ID_LOG);
							mstruct *= nr_half;
							return true;
						} else if(mpow.number() == 3) {
							mstruct[0] /= CALCULATOR->getRadUnit();
							mstruct.setFunctionId(FUNCTION_ID_COSINT);
							MathStructure mterm2(mstruct);
							mstruct[0] *= nr_three;
							mterm2 *= Number(3, 1);
							mstruct += mterm2;
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct *= Number(1, 4);
							return true;
						}
					} else if(mpow.isOne()) {
						MathStructure mterm2;
						mstruct = x_var;
						if(!mexp.isOne()) mstruct ^= mexp;
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct.transformById(FUNCTION_ID_COSINT);
						madd *= CALCULATOR->getRadUnit();
						mstruct *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &madd, NULL);
						mterm2 = x_var;
						if(!mexp.isOne()) mterm2 ^= mexp;
						if(!mmul.isOne()) mterm2 *= mmul;
						mterm2.transformById(FUNCTION_ID_SININT);
						mterm2 *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), &madd, NULL);
						mstruct -= mterm2;
						if(!mexp.isOne()) mstruct /= mexp;
						return true;
					}
				} else if(mexp.isOne() && mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct);
						mterm2.setFunctionId(FUNCTION_ID_SIN);
						mterm2 *= x_var;
						if(!mmul.isOne()) {
							mterm2 /= mmul;
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct += mterm2;
						return true;
					} else if(mfacexp.isInteger() && mfacexp.number().isPositive() && mfacexp.number().isLessThan(100)) {
						mstruct.setFunctionId(FUNCTION_ID_SIN);
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						if(!mmul.isOne()) mterm2 /= mmul;
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.last().last() += nr_minus_one;
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct *= mfacexp;
						if(!mmul.isOne()) mstruct /= mmul;
						mstruct.negate();
						mstruct += mterm2;
						mstruct.childrenUpdated(true);
						return true;
					} else if(madd.isZero() && mfacexp.isInteger() && mfacexp.number().isNegative() && mfacexp.number().isGreaterThan(-100)) {
						mfacexp += m_one;
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						mterm2.childUpdated(mterm2.size());
						mterm2 /= mfacexp;
						mstruct.setFunctionId(FUNCTION_ID_SIN);
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.childUpdated(mstruct.size());
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct /= mfacexp;
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct += mterm2;
						return true;
					}
				} else if(mexp.isOne() && mpow.number().isTwo()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct);
						mterm2.setFunctionId(FUNCTION_ID_SIN);
						MathStructure mterm3(x_var);
						mterm3 ^= nr_two;
						mterm3 *= Number(1, 4);
						mterm2[0] *= nr_two;
						mterm2 *= x_var;
						if(!mmul.isOne()) mterm2 /= mmul;
						mterm2 *= Number(1, 4);
						mstruct[0] *= nr_two;
						if(!mmul.isOne()) {
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct *= Number(1, 8);
						mstruct += mterm2;
						mstruct += mterm3;
						return true;
					} else if(mfacexp.number().isTwo()) {
						MathStructure mterm2(mstruct);
						mterm2.setFunctionId(FUNCTION_ID_SIN);
						MathStructure mterm3(x_var);
						mterm3 ^= nr_three;
						mterm3 *= Number(1, 6);
						mterm2[0] *= nr_two;
						MathStructure mterm21(-1, 8, 0);
						if(!mmul.isOne()) {
							mterm21 *= mmul;
							mterm21.last() ^= Number(-3, 1);
						}
						MathStructure mterm22(x_var);
						mterm22 ^= 2;
						if(!mmul.isOne()) mterm22 /= mmul;
						mterm22 *= Number(1, 4);
						mterm21 += mterm22;
						mterm2 *= mterm21;
						mstruct[0] *= nr_two;
						mstruct *= x_var;
						if(!mmul.isOne()) {
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct *= Number(1, 4);
						mstruct += mterm2;
						mstruct += mterm3;
						mstruct.childrenUpdated(true);
						return true;
					}
				}
			} else if(mexp.isOne() && mfac.isFunction()) {
				if(mfac.function()->id() == FUNCTION_ID_COS && mfac.size() == 1 && mpow.isOne()) {
					MathStructure mexpf, mmulf, maddf;
					if(integrate_info(mfac[0], x_var, maddf, mmulf, mexpf, true) && mexpf.isOne() && mmulf != mmul) {
						mstruct.setFunctionId(FUNCTION_ID_SIN);
						MathStructure mterm2(mstruct);
						mterm2[0] += mfac[0];
						mstruct[0] -= mfac[0];
						MathStructure mden1(mmul);
						mden1 -= mmulf;
						mden1 *= nr_two;
						MathStructure mden2(mmul);
						mden2 += mmulf;
						mden2 *= nr_two;
						mterm2 /= mden2;
						mstruct /= mden1;
						mstruct += mterm2;
						mstruct.childrenUpdated(true);
						return true;
					}
				} else if(mfac.function()->id() == FUNCTION_ID_SIN && mfac.size() == 1) {
					if(mstruct[0] == mfac[0]) {
						UnknownVariable *var = new UnknownVariable("", format_and_print(mstruct));
						MathStructure mtest(var);
						if(!mpow.isOne()) mtest ^= mpow;
						mtest.negate();
						CALCULATOR->beginTemporaryStopMessages();
						if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
							MathStructure m_interval(mstruct);
							m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
							var->setInterval(m_interval);
						} else {
							var->setInterval(mstruct);
						}
						if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							mtest.replace(var, mstruct);
							var->destroy();
							mstruct = mtest;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						}
						CALCULATOR->endTemporaryStopMessages();
						var->destroy();
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_TAN && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mfac.isOne() && mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-1) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp, true) && mexp.isOne() && (!definite_integral || mstruct[0].representsNonComplex(true))) {
			if(mpow.isOne()) {
				mstruct.setFunctionId(FUNCTION_ID_COS);
				if(!transform_absln(mstruct, use_abs, definite_integral, x_var, eo)) return -1;
				mstruct.negate();
				if(!mmul.isOne()) mstruct.divide(mmul);
			} else if(mpow.number().isMinusOne()) {
				mstruct.setFunctionId(FUNCTION_ID_SIN);
				if(!transform_absln(mstruct, use_abs, definite_integral, x_var, eo)) return -1;
				if(!mmul.isOne()) mstruct.divide(mmul);
			} else if(mpow.number().isTwo()) {
				MathStructure marg(x_var);
				if(!mmul.isOne()) marg *= mmul;
				marg += madd;
				mstruct -= marg;
				if(!mmul.isOne()) mstruct.divide(mmul);
			} else {
				MathStructure minteg(mstruct);
				MathStructure nm1(mpow);
				nm1 += nr_minus_one;
				mstruct ^= nm1;
				mmul *= nm1;
				mstruct /= mmul;
				MathStructure nm2(mpow);
				nm2 += Number(-2, 1);
				minteg ^= nm2;
				if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
				mstruct -= minteg;
			}
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ASIN && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp)) {
			if(definite_integral && !madd.isZero() && (!mmul.representsNonComplex(true) || !mexp.representsInteger()) && comparison_might_be_equal(x_var.compare(m_zero))) return false;
			if(mexp.isOne() && mfac.isOne()) {
				if(mpow.isOne()) {
					MathStructure marg(mstruct[0]);
					if(!madd.isZero()) mstruct[0] = x_var;
					mstruct.multiply(x_var);
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(madd.isZero() && !mmul.isOne()) {
						MathStructure mmul2(mmul);
						mmul2 ^= nr_two;
						mterm *= mmul2;
					}
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					if(madd.isZero() && !mmul.isOne()) mterm /= mmul;
					mstruct.add(mterm);
					if(!madd.isZero()) {
						mstruct.replace(x_var, marg);
						if(!mmul.isOne()) mstruct.divide(mmul);
					}
					return true;
				} else if(mpow.isMinusOne()) {
					mstruct.transformById(FUNCTION_ID_COSINT);
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(mpow.number() == -2) {
					MathStructure mterm(mstruct[0]);
					mterm ^= nr_two;
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					mterm /= mstruct;
					mstruct.transformById(FUNCTION_ID_SININT);
					mstruct += mterm;
					if(!mmul.isOne()) mstruct /= mmul;
					mstruct.negate();
					return true;
				} else if(mpow.number().isPositive() && madd.isZero()) {
					MathStructure mpowm1(mpow);
					if(mpow == nr_two) mpowm1.set(1, 1, 0, true);
					else mpowm1 += nr_minus_one;
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(!mmul.isOne()) {
						mterm *= mmul;
						mterm.last() ^= nr_two;
					}
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					mterm *= mstruct;
					if(!mpowm1.isOne()) mterm.last() ^= mpowm1;
					mterm *= mpow;
					if(!mmul.isOne()) mterm /= mmul;
					MathStructure minteg;
					if(mpowm1.isOne()) {
						minteg = x_var;
					} else {
						minteg = mstruct;
						minteg ^= mpow;
						minteg.last() += Number(-2, 1);
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					}
					minteg *= mpow;
					if(!mpowm1.isOne()) minteg *= mpowm1;
					mstruct ^= mpow;
					mstruct *= x_var;
					mstruct += mterm;
					mstruct -= minteg;
					return true;
				} else if(madd.isZero()) {
					MathStructure mpowp1(mpow);
					mpowp1 += m_one;
					MathStructure mpowp2(mpow);
					mpowp2 += nr_two;
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(!mmul.isOne()) {
						mterm *= mmul;
						mterm.last() ^= nr_two;
					}
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					mterm *= mstruct;
					mterm.last() ^= mpowp1;
					mterm /= mpowp1;
					if(!mmul.isOne()) mterm /= mmul;
					MathStructure minteg(mstruct);
					minteg ^= mpowp2;
					if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					minteg /= mpowp1;
					minteg /= mpowp2;
					mstruct ^= mpowp2;
					mstruct *= x_var;
					mstruct /= mpowp1;
					mstruct /= mpowp2;
					mstruct += mterm;
					mstruct -= minteg;
					return true;
				}
			} else if(mexp.isOne() && (mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0))) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct[0]);
						mterm2 ^= nr_two;
						mterm2.negate();
						mterm2 += m_one;
						mterm2 ^= nr_half;
						MathStructure mfac2(x_var);
						if(!mmul.isOne()) mfac2 *= mmul;
						if(!madd.isZero()) {
							mfac2 += madd;
							mfac2.last() *= Number(-3, 1);
						}
						mterm2 *= mfac2;
						MathStructure mfac1(x_var);
						mfac1 ^= nr_two;
						if(!mmul.isOne()) {
							mfac1 *= mmul;
							mfac1.last() ^= nr_two;
						}
						mfac1 *= nr_two;
						if(!madd.isZero()) {
							mfac1 += madd;
							mfac1.last() ^= nr_two;
							mfac1.last() *= Number(-2, 1);
						}
						mfac1 += nr_minus_one;
						mstruct *= mfac1;
						mstruct += mterm2;
						if(!mmul.isOne()) {
							mstruct *= mmul;
							mstruct.last() ^= Number(-2, 1);
						}
						mstruct *= Number(1, 4);
						return true;
					} else if(mfacexp == nr_two && madd.isZero()) {
						mstruct *= x_var;
						mstruct.last() ^= nr_three;
						mstruct *= Number(1, 3);
						MathStructure mterm(x_var);
						mterm ^= nr_two;
						if(!mmul.isOne()) {
							mterm *= mmul;
							mterm.last() ^= nr_two;
						}
						MathStructure mfac1(mterm);
						mfac1 += nr_two;
						mterm.negate();
						mterm += m_one;
						mterm ^= nr_half;
						mterm *= mfac1;
						if(!mmul.isOne()) {
							mmul ^= Number(-3, 1);
							mterm *= mmul;
						}
						mterm *= Number(1, 9);
						mstruct += mterm;
						return true;
					} else if(!mfacexp.isMinusOne() && madd.isZero()) {
						mfacexp += m_one;
						MathStructure minteg(x_var);
						minteg ^= nr_two;
						if(!mmul.isOne()) {
							minteg *= mmul;
							minteg.last() ^= nr_two;
						}
						minteg.negate();
						minteg += m_one;
						minteg ^= Number(-1, 2);
						minteg *= x_var;
						minteg.last() ^= mfacexp;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						if(minteg.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							if(!mmul.isOne()) minteg *= mmul;
							minteg /= mfacexp;
							mstruct *= x_var;
							mstruct.last() ^= mfacexp;
							mstruct /= mfacexp;
							mstruct -= minteg;
							return true;
						}
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ACOS && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp)) {
			if(definite_integral && !madd.isZero() && (!mmul.representsNonComplex(true) || !mexp.representsInteger()) && comparison_might_be_equal(x_var.compare(m_zero))) return false;
			if(mexp.isOne() && mfac.isOne()) {
				if(mpow.isOne()) {
					MathStructure marg(mstruct[0]);
					if(!madd.isZero()) mstruct[0] = x_var;
					mstruct.multiply(x_var);
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(madd.isZero() && !mmul.isOne()) {
						MathStructure mmul2(mmul);
						mmul2 ^= nr_two;
						mterm *= mmul2;
					}
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					if(madd.isZero() && !mmul.isOne()) mterm /= mmul;
					mstruct.subtract(mterm);
					if(!madd.isZero()) {
						mstruct.replace(x_var, marg);
						if(!mmul.isOne()) mstruct.divide(mmul);
					}
					return true;
				} else if(mpow.isMinusOne()) {
					mstruct.transformById(FUNCTION_ID_SININT);
					mstruct.negate();
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(mpow.number() == -2) {
					MathStructure mterm(mstruct[0]);
					mterm ^= nr_two;
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					mterm /= mstruct;
					mstruct.transformById(FUNCTION_ID_COSINT);
					mstruct.negate();
					mstruct += mterm;
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(mpow.number().isPositive() && madd.isZero()) {
					MathStructure mpowm1(mpow);
					if(mpow == nr_two) mpowm1.set(1, 1, 0, true);
					else mpowm1 += nr_minus_one;
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(!mmul.isOne()) {
						mterm *= mmul;
						mterm.last() ^= nr_two;
					}
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					mterm *= mstruct;
					if(!mpowm1.isOne()) mterm.last() ^= mpowm1;
					mterm *= mpow;
					if(!mmul.isOne()) mterm /= mmul;
					MathStructure minteg;
					if(mpowm1.isOne()) {
						minteg = x_var;
					} else {
						minteg = mstruct;
						minteg ^= mpow;
						minteg.last() += Number(-2, 1);
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					}
					minteg *= mpow;
					if(!mpowm1.isOne()) minteg *= mpowm1;
					mstruct ^= mpow;
					mstruct *= x_var;
					mstruct -= mterm;
					mstruct -= minteg;
					return true;
				} else if(madd.isZero()) {
					MathStructure mpowp1(mpow);
					mpowp1 += m_one;
					MathStructure mpowp2(mpow);
					mpowp2 += nr_two;
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(!mmul.isOne()) {
						mterm *= mmul;
						mterm.last() ^= nr_two;
					}
					mterm.negate();
					mterm += m_one;
					mterm ^= nr_half;
					mterm *= mstruct;
					mterm.last() ^= mpowp1;
					mterm /= mpowp1;
					if(!mmul.isOne()) mterm /= mmul;
					MathStructure minteg(mstruct);
					minteg ^= mpowp2;
					if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					minteg /= mpowp1;
					minteg /= mpowp2;
					mstruct ^= mpowp2;
					mstruct *= x_var;
					mstruct /= mpowp1;
					mstruct /= mpowp2;
					mstruct -= mterm;
					mstruct -= minteg;
					return true;
				}
			} else if(mexp.isOne() && (mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0))) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct[0]);
						MathStructure mterm3(mstruct);
						mterm2 ^= nr_two;
						mterm2.negate();
						mterm2 += m_one;
						mterm2 ^= nr_half;
						MathStructure mfac2(x_var);
						if(!mmul.isOne()) {
							mfac2 *= mmul;
						}
						mfac2.negate();
						if(!madd.isZero()) {
							mfac2 += madd;
							mfac2.last() *= nr_three;
						}
						mterm2 *= mfac2;
						mterm3.setFunctionId(FUNCTION_ID_ASIN);
						MathStructure mfac3(1, 1, 0);
						if(!madd.isZero()) {
							mfac3 += madd;
							mfac3.last() ^= nr_two;
							mfac3.last() *= nr_two;
						}
						mterm3 *= mfac3;
						mstruct *= x_var;
						mstruct.last() ^= nr_two;
						if(!mmul.isOne()) {
							mstruct *= mmul;
							mstruct.last() ^= nr_two;
						}
						mstruct *= nr_two;
						mstruct += mterm2;
						mstruct += mterm3;
						if(!mmul.isOne()) {
							mstruct *= mmul;
							mstruct.last() ^= Number(-2, 1);
						}
						mstruct *= Number(1, 4);
						return true;
					} else if(mfacexp == nr_two && madd.isZero()) {
						mstruct *= x_var;
						mstruct.last() ^= nr_three;
						mstruct *= Number(1, 3);
						MathStructure mterm(x_var);
						mterm ^= nr_two;
						if(!mmul.isOne()) {
							mterm *= mmul;
							mterm.last() ^= nr_two;
						}
						MathStructure mfac1(mterm);
						mfac1 += nr_two;
						mterm.negate();
						mterm += m_one;
						mterm ^= nr_half;
						mterm *= mfac1;
						if(!mmul.isOne()) {
							mmul ^= Number(-3, 1);
							mterm *= mmul;
						}
						mterm *= Number(-1, 9);
						mstruct += mterm;
						return true;
					} else if(!mfacexp.isMinusOne() && madd.isZero()) {
						mfacexp += m_one;
						MathStructure minteg(x_var);
						minteg ^= nr_two;
						if(!mmul.isOne()) {
							minteg *= mmul;
							minteg.last() ^= nr_two;
						}
						minteg.negate();
						minteg += m_one;
						minteg ^= Number(-1, 2);
						minteg *= x_var;
						minteg.last() ^= mfacexp;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						if(minteg.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							if(!mmul.isOne()) minteg *= mmul;
							minteg /= mfacexp;
							mstruct *= x_var;
							mstruct.last() ^= mfacexp;
							mstruct /= mfacexp;
							mstruct += minteg;
							return true;
						}
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ATAN && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && (!definite_integral || x_var.representsNonZero(true) || madd.representsReal(true))) {
			if(!mexp.isOne()) {
				if(mexp.isMinusOne() && mfac.isOne() && madd.isZero()) {
					mstruct *= x_var;
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					mterm += mmul;
					if(!mmul.isOne()) mterm.last() ^= nr_two;
					mterm.transformById(FUNCTION_ID_LOG);
					if(!mmul.isOne()) mterm *= mmul;
					mterm *= nr_half;
					mstruct += mterm;
					return true;
				}
			} else if(mfac.isOne()) {
				MathStructure marg(mstruct[0]);
				mstruct.multiply(marg);
				MathStructure mterm(marg);
				mterm ^= nr_two;
				mterm += m_one;
				if(!transform_absln(mterm, use_abs, definite_integral, x_var, eo)) return -1;
				mterm *= Number(-1, 2);
				mstruct += mterm;
				if(!mmul.isOne()) mstruct.divide(mmul);
				return true;
			} else if(madd.isZero() && (mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0))) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne()) {
					mstruct.setFunctionId(FUNCTION_ID_POLYLOG);
					mstruct.insertChild(nr_two, 1);
					MathStructure mterm(mstruct);
					mstruct[1] *= nr_minus_i;
					mterm[1] *= nr_one_i;
					mterm.negate();
					mstruct += mterm;
					mstruct *= nr_one_i;
					mstruct *= nr_half;
					return true;
				} else if(mfacexp.isOne()) {
					MathStructure mterm1(x_var);
					mterm1 ^= nr_two;
					mterm1 *= mstruct;
					mterm1 *= nr_half;
					MathStructure mterm2(x_var);
					if(!mmul.isOne()) mterm2 /= mmul;
					mterm2 *= nr_minus_half;
					if(!mmul.isOne()) {mstruct *= mmul; mstruct.last() ^= Number(-2, 1);}
					mstruct *= nr_half;
					mstruct += mterm1;
					mstruct += mterm2;
					return true;
				} else {
					mfacexp += m_one;
					MathStructure mxexp(x_var);
					mxexp ^= mfacexp;
					MathStructure minteg(x_var);
					minteg ^= nr_two;
					if(!mmul.isOne()) {minteg *= mmul; minteg.last() ^= nr_two;}
					minteg += m_one;
					if(!definite_integral || comparison_is_not_equal(minteg.compare(m_zero))) {
						minteg ^= nr_minus_one;
						minteg *= mxexp;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg /= mfacexp;
						if(!mmul.isOne()) minteg *= mmul;
						mstruct *= mxexp;
						mstruct /= mfacexp;
						mstruct -= minteg;
						return true;
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_SINH && mstruct.size() == 1) {
		if(mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_LOG && mstruct[0].size() == 1 && (!definite_integral || comparison_is_not_equal(mstruct[0][0].compare(m_zero)))) {
			MathStructure mtest(mstruct[0][0]);
			mtest *= Number(-2, 1);
			mtest.inverse();
			mtest.add(mstruct[0][0]);
			mtest.last() *= nr_half;
			if(!mpow.isOne()) mtest ^= mpow;
			if(!mfac.isOne()) mtest *= mfac;
			CALCULATOR->beginTemporaryStopMessages();
			if(mtest.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				mstruct.set(mtest, true);
				return true;
			}
			CALCULATOR->endTemporaryStopMessages();
		}
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp)) {
			if(mfac.isOne() && mexp.isOne()) {
				if(mpow.isOne()) {
					mstruct.setFunctionId(FUNCTION_ID_COSH);
					if(!mmul.isOne()) mstruct.divide(mmul);
				} else if(mpow.number().isTwo()) {
					MathStructure marg(mstruct[0]);
					if(!madd.isZero()) mstruct[0] = x_var;
					mstruct[0] *= nr_two;
					mstruct /= 4;
					if(madd.isZero() && !mmul.isOne()) mstruct /= mmul;
					MathStructure xhalf(x_var);
					xhalf *= nr_half;
					mstruct -= xhalf;
					if(!madd.isZero()) {
						mstruct.replace(x_var, marg);
						if(!mmul.isOne()) mstruct.divide(mmul);
					}
				} else if(mpow.number().isMinusOne()) {
					mstruct.setFunctionId(FUNCTION_ID_TANH);
					mstruct[0] *= nr_half;
					if(!transform_absln(mstruct, use_abs, definite_integral, x_var, eo)) return -1;
					if(!mmul.isOne()) mstruct.divide(mmul);
				} else if(mpow.number().isPositive()) {
					MathStructure mbak(mstruct);
					MathStructure nm1(mpow);
					nm1 += nr_minus_one;
					mstruct ^= nm1;
					MathStructure mcos(mbak);
					mcos.setFunctionId(FUNCTION_ID_COSH);
					mstruct *= mcos;
					mmul *= mpow;
					mstruct /= mmul;
					MathStructure minteg(mbak);
					MathStructure nm2(mpow);
					nm2 += Number(-2, 1);
					minteg ^= nm2;
					if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					minteg *= nm1;
					minteg /= mpow;
					mstruct -= minteg;
				} else {
					MathStructure mbak(mstruct);
					MathStructure np1(mpow);
					np1 += m_one;
					MathStructure np2(mpow);
					np2 += nr_two;
					mstruct ^= np1;
					MathStructure mcos(mbak);
					mcos.setFunctionId(FUNCTION_ID_COSH);
					mstruct *= mcos;
					mstruct /= np1;
					if(!mmul.isOne()) mstruct /= mmul;
					MathStructure minteg(mbak);
					minteg ^= np2;
					if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					minteg *= np2;
					minteg /= np1;
					mstruct -= minteg;
				}
				return true;
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne() && !mexp.isZero()) {
					if(madd.isZero()) {
						if(mpow.isOne()) {
							mstruct.setFunctionId(FUNCTION_ID_SINHINT);
							if(!mexp.isOne()) mstruct /= mexp;
							return true;
						} else if(mpow.number().isTwo()) {
							mstruct[0] *= nr_two;
							mstruct.setFunctionId(FUNCTION_ID_COSHINT);
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct += x_var;
							mstruct.last().transformById(FUNCTION_ID_LOG);
							mstruct.last().negate();
							mstruct *= nr_half;
							return true;
						} else if(mpow.number() == 3) {
							mstruct.setFunctionId(FUNCTION_ID_SINHINT);
							MathStructure mterm2(mstruct);
							mstruct[0] *= nr_three;
							mterm2 *= Number(-3, 1);
							mstruct += mterm2;
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct *= Number(1, 4);
							return true;
						}
					} else if(mpow.isOne()) {
						MathStructure mterm2;
						mstruct = x_var;
						if(!mexp.isOne()) mstruct ^= mexp;
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct.transformById(FUNCTION_ID_SINHINT);
						mstruct *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COSH), &madd, NULL);
						mterm2 = x_var;
						if(!mexp.isOne()) mterm2 ^= mexp;
						if(!mmul.isOne()) mterm2 *= mmul;
						mterm2.transformById(FUNCTION_ID_COSHINT);
						mterm2 *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SINH), &madd, NULL);
						mstruct += mterm2;
						if(!mexp.isOne()) mstruct /= mexp;
						return true;
					}
				} else if(mexp.isOne() && mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct);
						mterm2.setFunctionId(FUNCTION_ID_COSH);
						mterm2 *= x_var;
						if(!mmul.isOne()) {
							mterm2 /= mmul;
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct.negate();
						mstruct += mterm2;
						return true;
					} else if(mfacexp.isInteger() && mfacexp.number().isPositive() && mfacexp.number().isLessThan(100)) {
						mstruct.setFunctionId(FUNCTION_ID_COSH);
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						if(!mmul.isOne()) mterm2 /= mmul;
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.last().last() += nr_minus_one;
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct *= mfacexp;
						if(!mmul.isOne()) mstruct /= mmul;
						mstruct.negate();
						mstruct += mterm2;
						mstruct.childrenUpdated(true);
						return true;
					} else if(madd.isZero() && mfacexp.isInteger() && mfacexp.number().isNegative() && mfacexp.number().isGreaterThan(-100)) {
						mfacexp += m_one;
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						mterm2.childUpdated(mterm2.size());
						mterm2 /= mfacexp;
						mstruct.setFunctionId(FUNCTION_ID_COSH);
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.childUpdated(mstruct.size());
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct /= mfacexp;
						mstruct.negate();
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct += mterm2;
						return true;
					}
				}
			} else if(mfac.isFunction() && mexp.isOne()) {
				if(mfac.function()->id() == FUNCTION_ID_SINH && mfac.size() == 1 && mpow.isOne()) {
					MathStructure mexpf, mmulf, maddf;
					if(integrate_info(mfac[0], x_var, maddf, mmulf, mexpf) && mexpf.isOne() && mmul != mmulf) {
						MathStructure mterm2(mstruct);
						mterm2[0] += mfac[0];
						mstruct[0] -= mfac[0];
						MathStructure mden1(mmul);
						mden1 -= mmulf;
						mden1 *= nr_two;
						MathStructure mden2(mmul);
						mden2 += mmulf;
						mden2 *= nr_two;
						mterm2 /= mden2;
						mstruct /= mden1;
						mstruct.negate();
						mstruct += mterm2;
						return true;
					}
				} else if(mfac.function()->id() == FUNCTION_ID_COSH && mfac.size() == 1) {
					if(mstruct[0] == mfac[0]) {
						UnknownVariable *var = new UnknownVariable("", format_and_print(mstruct));
						MathStructure mtest(var);
						if(!mpow.isOne()) mtest ^= mpow;
						CALCULATOR->beginTemporaryStopMessages();
						if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
							MathStructure m_interval(mstruct);
							m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
							var->setInterval(m_interval);
						} else {
							var->setInterval(mstruct);
						}
						if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							mtest.replace(var, mstruct);
							var->destroy();
							mstruct = mtest;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						}
						CALCULATOR->endTemporaryStopMessages();
						var->destroy();
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_COSH && mstruct.size() == 1) {
		if(mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_LOG && mstruct[0].size() == 1 && (!definite_integral || comparison_is_not_equal(mstruct[0][0].compare(m_zero)))) {
			MathStructure mtest(mstruct[0][0]);
			mtest *= nr_two;
			mtest.inverse();
			mtest.add(mstruct[0][0]);
			mtest.last() *= nr_half;
			if(!mpow.isOne()) mtest ^= mpow;
			if(!mfac.isOne()) mtest *= mfac;
			CALCULATOR->beginTemporaryStopMessages();
			if(mtest.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				mstruct.set(mtest, true);
				return true;
			}
			CALCULATOR->endTemporaryStopMessages();
		}
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp)) {
			if(mfac.isOne() && mexp.isOne()) {
				if(mpow.isOne()) {
					mstruct.setFunctionId(FUNCTION_ID_SINH);
					if(!mmul.isOne()) mstruct.divide(mmul);
					return true;
				} else if(mpow.number().isTwo()) {
					MathStructure marg(mstruct[0]);
					if(!madd.isZero()) mstruct[0] = x_var;
					mstruct.setFunctionId(FUNCTION_ID_SINH);
					mstruct[0] *= nr_two;
					mstruct /= 4;
					if(madd.isZero() && !mmul.isOne()) mstruct /= mmul;
					MathStructure xhalf(x_var);
					xhalf *= nr_half;
					mstruct += xhalf;
					if(!madd.isZero()) {
						mstruct.replace(x_var, marg);
						if(!mmul.isOne()) mstruct.divide(mmul);
					}
					return true;
				} else if(mpow.number().isMinusOne()) {
					mstruct.setFunctionId(FUNCTION_ID_SINH);
					mstruct.transformById(FUNCTION_ID_ATAN);
					if(!mmul.isOne()) mstruct.divide(mmul);
					return true;
				} else if(mpow.number().isPositive()) {
					MathStructure mbak(mstruct);
					MathStructure nm1(mpow);
					nm1 += nr_minus_one;
					mstruct ^= nm1;
					MathStructure msin(mbak);
					msin.setFunctionId(FUNCTION_ID_SINH);
					mstruct *= msin;
					mmul *= mpow;
					mstruct /= mmul;
					MathStructure minteg(mbak);
					MathStructure nm2(mpow);
					nm2 += Number(-2, 1);
					minteg ^= nm2;
					if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					minteg *= nm1;
					minteg /= mpow;
					mstruct += minteg;
					return true;
				} else {
					MathStructure mbak(mstruct);
					MathStructure np1(mpow);
					np1 += m_one;
					MathStructure np2(mpow);
					np2 += nr_two;
					mstruct ^= np1;
					MathStructure mcos(mbak);
					mcos.setFunctionId(FUNCTION_ID_SINH);
					mstruct *= mcos;
					mstruct /= np1;
					if(!mmul.isOne()) mstruct /= mmul;
					mstruct.negate();
					MathStructure minteg(mbak);
					minteg ^= np2;
					if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
					minteg *= np2;
					minteg /= np1;
					mstruct += minteg;
					return true;
				}
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne() && !mexp.isZero()) {
					if(madd.isZero()) {
						if(mpow.isOne()) {
							mstruct.setFunctionId(FUNCTION_ID_COSHINT);
							if(!mexp.isOne()) mstruct /= mexp;
							return true;
						} else if(mpow.number().isTwo()) {
							mstruct[0] *= nr_two;
							mstruct.setFunctionId(FUNCTION_ID_COSHINT);
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct += x_var;
							mstruct.last().transformById(FUNCTION_ID_LOG);
							mstruct *= nr_half;
							return true;
						} else if(mpow.number() == 3) {
							mstruct.setFunctionId(FUNCTION_ID_COSHINT);
							MathStructure mterm2(mstruct);
							mstruct[0] *= nr_three;
							mterm2 *= nr_three;
							mstruct += mterm2;
							if(!mexp.isOne()) mstruct /= mexp;
							mstruct *= Number(1, 4);
							return true;
						}
					} else if(mpow.isOne()) {
						MathStructure mterm2;
						mstruct = x_var;
						if(!mexp.isOne()) mstruct ^= mexp;
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct.transformById(FUNCTION_ID_SINHINT);
						mstruct *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SINH), &madd, NULL);
						mterm2 = x_var;
						if(!mexp.isOne()) mterm2 ^= mexp;
						if(!mmul.isOne()) mterm2 *= mmul;
						mterm2.transformById(FUNCTION_ID_COSHINT);
						mterm2 *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_COSH), &madd, NULL);
						mstruct += mterm2;
						if(!mexp.isOne()) mstruct /= mexp;
						return true;
					}
				} else if(mexp.isOne() && mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct);
						mterm2.setFunctionId(FUNCTION_ID_SINH);
						mterm2 *= x_var;
						if(!mmul.isOne()) {
							mterm2 /= mmul;
							mmul ^= nr_two;
							mstruct /= mmul;
						}
						mstruct.negate();
						mstruct += mterm2;
						return true;
					} else if(mfacexp.isInteger() && mfacexp.number().isPositive() && mfacexp.number().isLessThan(100)) {
						mstruct.setFunctionId(FUNCTION_ID_SINH);
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						if(!mmul.isOne()) mterm2 /= mmul;
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.last().last() += nr_minus_one;
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct *= mfacexp;
						if(!mmul.isOne()) mstruct /= mmul;
						mstruct.negate();
						mstruct += mterm2;
						mstruct.childrenUpdated(true);
						return true;
					} else if(madd.isZero() && mfacexp.isInteger() && mfacexp.number().isNegative() && mfacexp.number().isGreaterThan(-100)) {
						mfacexp += m_one;
						MathStructure mterm2(mstruct);
						mterm2 *= x_var;
						mterm2.last() ^= mfacexp;
						mterm2.childUpdated(mterm2.size());
						mterm2 /= mfacexp;
						mstruct.setFunctionId(FUNCTION_ID_SINH);
						mstruct *= x_var;
						mstruct.last() ^= mfacexp;
						mstruct.childUpdated(mstruct.size());
						if(mstruct.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						mstruct /= mfacexp;
						mstruct.negate();
						if(!mmul.isOne()) mstruct *= mmul;
						mstruct += mterm2;
						return true;
					}
				}
			} else if(mfac.isFunction() && mexp.isOne()) {
				if(mfac.function()->id() == FUNCTION_ID_COSH && mfac.size() == 1 && mpow.isOne()) {
					MathStructure mexpf, mmulf, maddf;
					if(integrate_info(mfac[0], x_var, maddf, mmulf, mexpf) && mexpf.isOne() && mmulf != mmul) {
						mstruct.setFunctionId(FUNCTION_ID_SINH);
						MathStructure mterm2(mstruct);
						mterm2[0] += mfac[0];
						mstruct[0] -= mfac[0];
						MathStructure mden1(mmul);
						mden1 -= mmulf;
						mden1 *= nr_two;
						MathStructure mden2(mmul);
						mden2 += mmulf;
						mden2 *= nr_two;
						mterm2 /= mden2;
						mstruct /= mden1;
						mstruct += mterm2;
						mstruct.childrenUpdated(true);
						return true;
					}
				} else if(mfac.function()->id() == FUNCTION_ID_SINH && mfac.size() == 1) {
					if(mstruct[0] == mfac[0]) {
						UnknownVariable *var = new UnknownVariable("", format_and_print(mstruct));
						MathStructure mtest(var);
						if(!mpow.isOne()) mtest ^= mpow;
						CALCULATOR->beginTemporaryStopMessages();
						if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
							MathStructure m_interval(mstruct);
							m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
							var->setInterval(m_interval);
						} else {
							var->setInterval(mstruct);
						}
						if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							mtest.replace(var, mstruct);
							var->destroy();
							mstruct = mtest;
							if(!mmul.isOne()) mstruct /= mmul;
							return true;
						}
						CALCULATOR->endTemporaryStopMessages();
						var->destroy();
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_TANH && mstruct.size() == 1) {
		if(mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_LOG && mstruct[0].size() == 1 && (!definite_integral || comparison_is_not_equal(mstruct[0][0].compare(m_zero)))) {
			MathStructure mtest(mstruct[0][0]);
			mtest ^= nr_two;
			mtest += m_one;
			mtest.inverse();
			mtest *= Number(-2, 1);
			mtest += m_one;
			if(!mpow.isOne()) mtest ^= mpow;
			if(!mfac.isOne()) mtest *= mfac;
			CALCULATOR->beginTemporaryStopMessages();
			if(mtest.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				mstruct.set(mtest, true);
				return true;
			}
			CALCULATOR->endTemporaryStopMessages();
		}
		MathStructure mexp, mmul, madd;
		if(mfac.isOne() && mpow.isInteger() && mpow.number().isLessThanOrEqualTo(10) && mpow.number().isGreaterThanOrEqualTo(-1) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne() && (!definite_integral || madd.representsNonComplex(true) || x_var.representsNonZero())) {
			if(mpow.isOne()) {
				mstruct.setFunctionId(FUNCTION_ID_COSH);
				mstruct.transformById(FUNCTION_ID_LOG);
				if(!mmul.isOne()) mstruct.divide(mmul);
			} else if(mpow.number().isTwo()) {
				MathStructure marg(mstruct[0]);
				if(!madd.isZero()) mstruct[0] = x_var;
				if(madd.isZero() && !mmul.isOne()) mstruct /= mmul;
				mstruct.negate();
				mstruct += x_var;
				if(!madd.isZero()) {
					mstruct.replace(x_var, marg);
					if(!mmul.isOne()) mstruct.divide(mmul);
				}
			} else if(mpow.number().isMinusOne()) {
				mstruct.setFunctionId(FUNCTION_ID_SINH);
				if(!transform_absln(mstruct, use_abs, definite_integral, x_var, eo)) return -1;
				if(!mmul.isOne()) mstruct.divide(mmul);
			} else {
				MathStructure minteg(mstruct);
				MathStructure nm1(mpow);
				nm1 += nr_minus_one;
				mstruct ^= nm1;
				mmul *= nm1;
				mstruct /= mmul;
				mstruct.negate();
				MathStructure nm2(mpow);
				nm2 += Number(-2, 1);
				minteg ^= nm2;
				if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
				mstruct += minteg;
			}
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ASINH && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(definite_integral && !madd.representsNonComplex(true) && comparison_might_be_equal(x_var.compare(m_zero))) return false;
			if(mfac.isOne()) {
				if(mpow.isOne()) {
					MathStructure marg(mstruct[0]);
					if(!madd.isZero()) mstruct[0] = x_var;
					mstruct.multiply(x_var);
					MathStructure mterm(x_var);
					mterm ^= nr_two;
					if(madd.isZero() && !mmul.isOne()) {
						MathStructure mmul2(mmul);
						mmul2 ^= nr_two;
						mterm *= mmul2;
					}
					mterm += m_one;
					mterm ^= nr_half;
					if(madd.isZero() && !mmul.isOne()) mterm /= mmul;
					mstruct.subtract(mterm);
					if(!madd.isZero()) {
						mstruct.replace(x_var, marg);
						if(!mmul.isOne()) mstruct.divide(mmul);
					}
					return true;
				} else if(mpow.isMinusOne()) {
					mstruct.transformById(FUNCTION_ID_COSHINT);
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(mpow.number() == -2) {
					MathStructure mterm(mstruct[0]);
					mterm ^= nr_two;
					mterm += m_one;
					mterm ^= nr_half;
					mterm /= mstruct;
					mstruct.transformById(FUNCTION_ID_SINHINT);
					mstruct -= mterm;
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(madd.isZero()) {
					if(mpow.number().isPositive()) {
						MathStructure mpowm1(mpow);
						if(mpow == nr_two) mpowm1.set(1, 1, 0, true);
						else mpowm1 += nr_minus_one;
						MathStructure mterm(mstruct);
						if(!mpowm1.isOne()) mterm ^= mpowm1;
						MathStructure mfac1(x_var);
						mfac1 ^= nr_two;
						if(!mmul.isOne()) {
							mfac1 *= mmul;
							mfac1.last() ^= nr_two;
						}
						mfac1 += m_one;
						mfac1 ^= nr_half;
						mterm *= mfac1;
						mterm *= mpow;
						if(!mmul.isOne()) mterm /= mmul;
						MathStructure minteg;
						if(mpowm1.isOne()) {
							minteg = x_var;
						} else {
							minteg = mstruct;
							minteg ^= mpow;
							minteg.last() += Number(-2, 1);
							if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						}
						minteg *= mpow;
						if(!mpowm1.isOne()) minteg *= mpowm1;
						mstruct ^= mpow;
						mstruct *= x_var;
						mstruct -= mterm;
						mstruct += minteg;
						return true;
					} else {
						MathStructure mpowp1(mpow);
						mpowp1 += m_one;
						MathStructure mpowp2(mpow);
						mpowp2 += nr_two;
						MathStructure mterm(mstruct);
						mterm ^= mpowp1;
						MathStructure mfac1(x_var);
						mfac1 ^= nr_two;
						if(!mmul.isOne()) {
							mfac1 *= mmul;
							mfac1.last() ^= nr_two;
						}
						mfac1 += m_one;
						mfac1 ^= nr_half;
						mterm *= mfac1;
						if(!mmul.isOne()) mterm /= mmul;
						mterm /= mpowp1;
						MathStructure minteg(mstruct);
						minteg ^= mpowp2;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg /= mpowp1;
						minteg /= mpowp2;
						mstruct ^= mpowp2;
						mstruct *= x_var;
						mstruct /= mpowp1;
						mstruct /= mpowp2;
						mstruct.negate();
						mstruct += mterm;
						mstruct += minteg;
						return true;
					}
				}
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct[0]);
						mterm2 ^= nr_two;
						mterm2 += m_one;
						mterm2 ^= nr_half;
						MathStructure mfac2(x_var);
						if(!mmul.isOne()) mfac2 *= mmul;
						mfac2.negate();
						if(!madd.isZero()) {
							mfac2 += madd;
							mfac2.last() *= nr_three;
						}
						mterm2 *= mfac2;
						MathStructure mfac1(x_var);
						mfac1 ^= nr_two;
						if(!mmul.isOne()) {
							mfac1 *= mmul;
							mfac1.last() ^= nr_two;
						}
						mfac1 *= nr_two;
						if(!madd.isZero()) {
							mfac1 += madd;
							mfac1.last() ^= nr_two;
							mfac1.last() *= Number(-2, 1);
						}
						mfac1 += m_one;
						mstruct *= mfac1;
						mstruct += mterm2;
						if(!mmul.isOne()) {
							mstruct *= mmul;
							mstruct.last() ^= Number(-2, 1);
						}
						mstruct *= Number(1, 4);
						return true;
					} else if(mfacexp == nr_two && madd.isZero()) {
						mstruct *= x_var;
						mstruct.last() ^= nr_three;
						mstruct *= Number(1, 3);
						MathStructure mterm(x_var);
						mterm ^= nr_two;
						if(!mmul.isOne()) {
							mterm *= mmul;
							mterm.last() ^= nr_two;
						}
						MathStructure mfac1(mterm);
						mfac1 += Number(-2, 1);
						mterm += m_one;
						mterm ^= nr_half;
						mterm *= mfac1;
						if(!mmul.isOne()) {
							mmul ^= Number(-3, 1);
							mterm *= mmul;
						}
						mterm *= Number(-1, 9);
						mstruct += mterm;
						return true;
					} else if(!mfacexp.isMinusOne() && madd.isZero()) {
						mfacexp += m_one;
						MathStructure minteg(x_var);
						minteg ^= nr_two;
						if(!mmul.isOne()) {
							minteg *= mmul;
							minteg.last() ^= nr_two;
						}
						minteg += m_one;
						minteg ^= Number(-1, 2);
						minteg *= x_var;
						minteg.last() ^= mfacexp;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						if(minteg.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							if(!mmul.isOne()) minteg *= mmul;
							minteg /= mfacexp;
							mstruct *= x_var;
							mstruct.last() ^= mfacexp;
							mstruct /= mfacexp;
							mstruct -= minteg;
							return true;
						}
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ACOSH && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isInteger() && mpow.number().isLessThanOrEqualTo(100) && mpow.number().isGreaterThanOrEqualTo(-100) && !mpow.isZero() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(definite_integral && (!mmul.representsNonComplex(true) || !x_var.representsNonComplex(true))) return false;
			if(mfac.isOne()) {
				if(mpow.isOne()) {
					MathStructure marg(mstruct[0]);
					if(!madd.isZero()) mstruct[0] = x_var;
					MathStructure mterm(mstruct[0]);
					MathStructure msqrt2(mstruct[0]);
					mstruct.multiply(x_var);
					mterm += m_one;
					mterm ^= nr_half;
					msqrt2 += m_minus_one;
					msqrt2 ^= nr_half;
					mterm *= msqrt2;
					if(madd.isZero() && !mmul.isOne()) {
						mterm /= mmul;
					}
					mstruct.subtract(mterm);
					if(!madd.isZero()) {
						mstruct.replace(x_var, marg);
						if(!mmul.isOne()) mstruct.divide(mmul);
					}
					return true;
				} else if(mpow.isMinusOne()) {
					mstruct.transformById(FUNCTION_ID_SINHINT);
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(mpow.number() == -2) {
					MathStructure msqrt(mstruct[0]);
					msqrt += m_one;
					msqrt.inverse();
					msqrt *= mstruct[0];
					msqrt.last() += nr_minus_one;
					msqrt ^= nr_half;
					MathStructure macosh(mstruct);
					mstruct.transformById(FUNCTION_ID_COSHINT);
					mstruct *= macosh;
					mstruct *= msqrt;
					mstruct += x_var;
					if(!mmul.isOne()) mstruct.last() *= mmul;
					mstruct.last().negate();
					if(!madd.isZero()) mstruct -= madd;
					mstruct += m_one;
					mstruct /= macosh;
					mstruct /= msqrt;
					if(!mmul.isOne()) mstruct /= mmul;
					return true;
				} else if(madd.isZero()) {
					if(mpow.number().isPositive()) {
						MathStructure mpowm1(mpow);
						if(mpow == nr_two) mpowm1.set(1, 1, 0, true);
						else mpowm1 += nr_minus_one;
						MathStructure mterm(mstruct);
						if(!mpowm1.isOne()) mterm ^= mpowm1;
						MathStructure mfac1(x_var);
						if(!mmul.isOne()) mfac1 *= mmul;
						mfac1 += m_one;
						mfac1 ^= nr_half;
						MathStructure mfac2(x_var);
						if(!mmul.isOne()) mfac2 *= mmul;
						mfac2 += nr_minus_one;
						mfac2 ^= nr_half;
						mterm *= mfac1;
						mterm *= mfac2;
						mterm *= mpow;
						if(!mmul.isOne()) mterm /= mmul;
						MathStructure minteg;
						if(mpowm1.isOne()) {
							minteg = x_var;
						} else {
							minteg = mstruct;
							minteg ^= mpow;
							minteg.last() += Number(-2, 1);
							if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						}
						minteg *= mpow;
						if(!mpowm1.isOne()) minteg *= mpowm1;
						mstruct ^= mpow;
						mstruct *= x_var;
						mstruct -= mterm;
						mstruct += minteg;
						return true;
					} else {
						MathStructure mpowp1(mpow);
						mpowp1 += m_one;
						MathStructure mpowp2(mpow);
						mpowp2 += nr_two;
						MathStructure mterm(mstruct);
						mterm ^= mpowp1;
						MathStructure mfac1(x_var);
						if(!mmul.isOne()) mfac1 *= mmul;
						mfac1 += m_one;
						mfac1 ^= nr_half;
						MathStructure mfac2(x_var);
						if(!mmul.isOne()) mfac2 *= mmul;
						mfac2 += nr_minus_one;
						mfac2 ^= nr_half;
						mterm *= mfac1;
						mterm *= mfac2;
						if(!mmul.isOne()) mterm /= mmul;
						mterm /= mpowp1;
						MathStructure minteg(mstruct);
						minteg ^= mpowp2;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg /= mpowp1;
						minteg /= mpowp2;
						mstruct ^= mpowp2;
						mstruct *= x_var;
						mstruct /= mpowp1;
						mstruct /= mpowp2;
						mstruct.negate();
						mstruct += mterm;
						mstruct += minteg;
						return true;
					}
				}
			} else if(mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mpow.isOne()) {
					if(mfacexp.isOne()) {
						MathStructure mterm2(mstruct[0]);
						MathStructure mterm2b(mstruct[0]);
						mterm2 += nr_minus_one;
						mterm2 ^= nr_half;
						mterm2b += m_one;
						mterm2b ^= nr_half;
						mterm2 *= mterm2b;
						MathStructure mfac2(x_var);
						if(!mmul.isOne()) {
							mfac2 *= mmul;
						}
						mfac2.negate();
						if(!madd.isZero()) {
							mfac2 += madd;
							mfac2.last() *= nr_three;
						}
						mterm2 *= mfac2;
						MathStructure mfac1(x_var);
						mfac1 ^= nr_two;
						if(!mmul.isOne()) {
							mfac1 *= mmul;
							mfac1.last() ^= nr_two;
						}
						mfac1 *= nr_two;
						if(!madd.isZero()) {
							mfac1 += madd;
							mfac1.last() ^= nr_two;
							mfac1.last() *= Number(-2, 1);
						}
						mfac1 += nr_minus_one;
						mstruct *= mfac1;
						mstruct += mterm2;
						if(!mmul.isOne()) {
							mstruct *= mmul;
							mstruct.last() ^= Number(-2, 1);
						}
						mstruct *= Number(1, 4);
						return true;
					} else if(mfacexp == nr_two && madd.isZero()) {
						mstruct *= x_var;
						mstruct.last() ^= nr_three;
						mstruct *= Number(1, 3);
						MathStructure mterm(x_var);
						if(!mmul.isOne()) mterm *= mmul;
						mterm += m_one;
						mterm ^= nr_half;
						MathStructure mfac1(x_var);
						if(!mmul.isOne()) mfac1 *= mmul;
						mfac1 += nr_minus_one;
						mfac1 ^= nr_half;
						mterm *= mfac1;
						MathStructure mfac2(x_var);
						mfac2 ^= nr_two;
						if(!mmul.isOne()) {
							mfac2 *= mmul;
							mfac2.last() ^= nr_two;
						}
						mfac2 += nr_two;
						mterm *= mfac2;
						if(!mmul.isOne()) {
							mmul ^= Number(-3, 1);
							mterm *= mmul;
						}
						mterm *= Number(-1, 9);
						mstruct += mterm;
						return true;
					} else if(!mfacexp.isMinusOne() && madd.isZero()) {
						mfacexp += m_one;
						MathStructure minteg(x_var);
						if(!mmul.isOne()) minteg *= mmul;
						minteg += m_one;
						minteg ^= Number(-1, 2);
						MathStructure mfac1(x_var);
						if(!mmul.isOne()) mfac1 *= mmul;
						mfac1 += nr_minus_one;
						mfac1 ^= Number(-1, 2);
						minteg *= mfac1;
						minteg *= x_var;
						minteg.last() ^= mfacexp;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						if(minteg.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							if(!mmul.isOne()) minteg *= mmul;
							minteg /= mfacexp;
							mstruct *= x_var;
							mstruct.last() ^= mfacexp;
							mstruct /= mfacexp;
							mstruct -= minteg;
							return true;
						}
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ATANH && mstruct.size() == 1) {
		MathStructure mexp, mmul, madd;
		if(mpow.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp)) {
			if(!mexp.isOne()) {
				if(mexp.isMinusOne() && mfac.isOne() && madd.isZero()) {
					mstruct *= x_var;
					if(madd.isZero()) {
						MathStructure mterm(mmul);
						if(!mmul.isOne()) mterm ^= nr_two;
						mterm.negate();
						mterm += x_var;
						mterm.last() ^= nr_two;
						mterm.transformById(FUNCTION_ID_LOG);
						if(!mmul.isOne()) mterm *= mmul;
						mterm *= nr_half;
						mstruct += mterm;
					} else {
						MathStructure mterm1(x_var);
						mterm1 *= madd;
						mterm1 += mmul;
						MathStructure mterm2(mterm1);
						mterm1 += x_var;
						mterm2 -= x_var;
						mterm1.transformById(FUNCTION_ID_LOG);
						mterm2.transformById(FUNCTION_ID_LOG);
						mterm1 *= mmul;
						mterm2 *= mmul;
						madd *= nr_two;
						madd += nr_two;
						mterm1 /= madd;
						madd[0].negate();
						mterm2 /= madd;
						mstruct += mterm1;
						mstruct += mterm2;
					}
					return true;
				}
			} else if(mfac.isOne()) {
				MathStructure marg(mstruct[0]);
				mstruct.multiply(marg);
				MathStructure mterm(marg);
				mterm ^= nr_two;
				mterm.negate();
				mterm += m_one;
				mterm.transformById(FUNCTION_ID_LOG);
				mterm *= nr_half;
				mstruct.add(mterm);
				if(!mmul.isOne()) mstruct.divide(mmul);
				return true;
			} else if(madd.isZero() && (mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].isInteger()))) {
				MathStructure mfacexp(1, 1, 0);
				if(mfac != x_var) mfacexp = mfac[1];
				if(mfacexp.isMinusOne()) {
					mstruct.setFunctionId(FUNCTION_ID_POLYLOG);
					mstruct.insertChild(nr_two, 1);
					MathStructure mterm(mstruct);
					mterm[1].negate();
					mterm.negate();
					mstruct += mterm;
					mstruct *= nr_half;
					return true;
				} else if(mfacexp.isOne()) {
					MathStructure mterm1(x_var);
					mterm1 ^= nr_two;
					mterm1 *= mstruct;
					mterm1 *= nr_half;
					MathStructure mterm2(x_var);
					if(!mmul.isOne()) mterm2 /= mmul;
					mterm2 *= nr_half;
					if(!mmul.isOne()) {mstruct *= mmul; mstruct.last() ^= Number(-2, 1);}
					mstruct *= nr_minus_half;
					mstruct += mterm1;
					mstruct += mterm2;
					return true;
				} else {
					mfacexp += m_one;
					MathStructure mxexp(x_var);
					mxexp ^= mfacexp;
					MathStructure minteg(x_var);
					minteg ^= nr_two;
					if(!mmul.isOne()) {minteg *= mmul; minteg.last() ^= nr_two;}
					minteg.negate();
					minteg += m_one;
					if(!definite_integral || comparison_is_not_equal(minteg.compare(m_zero))) {
						minteg ^= nr_minus_one;
						minteg *= mxexp;
						if(minteg.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) return -1;
						minteg /= mfacexp;
						if(!mmul.isOne()) minteg *= mmul;
						mstruct *= mxexp;
						mstruct /= mfacexp;
						mstruct -= minteg;
						return true;
					}
				}
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct)) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(mfac.isOne()) {
				MathStructure np1(mstruct[1]);
				mstruct.multiply(mstruct[0]);
				np1.inverse();
				np1 += m_one;
				if(!mmul.isOne()) np1 *= mmul;
				mstruct.divide(np1);
				return true;
			} else if(mfac == x_var) {
				MathStructure nm1(mstruct[1]);
				nm1.inverse();
				nm1 += m_one;
				MathStructure mnum(x_var);
				mnum *= nm1;
				if(!mmul.isOne()) mnum *= mmul;
				if(!madd.isZero()) mnum -= madd;
				MathStructure mden(mstruct[1]);
				mden.inverse();
				mden += nr_two;
				mden *= nm1;
				if(!mmul.isOne()) {
					mden *= mmul;
					mden.last() ^= nr_two;
				}
				mstruct.multiply(mstruct[0]);
				mstruct *= mnum;
				mstruct /= mden;
				return true;
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ERFC && mstruct.size() == 1) {
		MathStructure mtest(mstruct);
		mtest.setFunctionId(FUNCTION_ID_ERF);
		mtest.negate();
		mtest += m_one;
		if(!mpow.isOne()) mtest ^= mpow;
		if(!mfac.isOne()) mtest *= mfac;
		CALCULATOR->beginTemporaryStopMessages();
		if(mtest.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth - 1, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE, true) <= 0) {
			CALCULATOR->endTemporaryStopMessages(true);
			mstruct = mtest;
			return true;
		}
		CALCULATOR->endTemporaryStopMessages();
	} else if(mstruct.function()->id() == FUNCTION_ID_ERF && mstruct.size() == 1) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && mfac.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			MathStructure mterm2(CALCULATOR->getVariableById(VARIABLE_ID_E));
			mterm2 ^= mstruct[0];
			mterm2.last() ^= nr_two;
			mterm2.last().negate();
			mterm2 *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mterm2.last() ^= Number(-1, 2);
			if(!mmul.isOne()) mterm2 /= mmul;
			if(madd.isZero()) {
				mstruct *= x_var;
			} else {
				mstruct *= madd;
				if(!mmul.isOne()) {
					mstruct.last() /= mmul;
					mstruct.childrenUpdated();
				}
				mstruct.last() += x_var;
			}
			mstruct += mterm2;
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_ERFI && mstruct.size() == 1) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && mfac.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			MathStructure mterm2(CALCULATOR->getVariableById(VARIABLE_ID_E));
			mterm2 ^= mstruct[0];
			mterm2.last() ^= nr_two;
			mterm2 *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mterm2.last() ^= Number(-1, 2);
			if(!mmul.isOne()) mterm2 /= mmul;
			if(madd.isZero()) {
				mstruct *= x_var;
			} else {
				mstruct *= madd;
				if(!mmul.isOne()) {
					mstruct.last() /= mmul;
					mstruct.childrenUpdated();
				}
				mstruct.last() += x_var;
			}
			mstruct -= mterm2;
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_FRESNEL_S && mstruct.size() == 1) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && mfac.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne() && madd.isOne()) {
			MathStructure mterm(x_var);
			mterm ^= nr_two;
			if(!mmul.isOne()) {
				mmul ^= nr_two;
				mterm *= mmul;
			}
			mterm *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mterm *= nr_half;
			mterm *= CALCULATOR->getRadUnit();
			mterm.transformById(FUNCTION_ID_COS);
			if(!mmul.isOne()) mterm /= mmul;
			mterm /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct *= x_var;
			mstruct += mterm;
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_FRESNEL_C && mstruct.size() == 1) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && mfac.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne() && madd.isOne()) {
			MathStructure mterm(x_var);
			mterm ^= nr_two;
			if(!mmul.isOne()) {
				mmul ^= nr_two;
				mterm *= mmul;
			}
			mterm *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mterm *= nr_half;
			mterm *= CALCULATOR->getRadUnit();
			mterm.transformById(FUNCTION_ID_SIN);
			if(!mmul.isOne()) mterm /= mmul;
			mterm /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
			mstruct *= x_var;
			mstruct -= mterm;
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_DIGAMMA && mstruct.size() == 1) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && mfac.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			mstruct.setFunctionId(FUNCTION_ID_GAMMA);
			mstruct.transformById(FUNCTION_ID_LOG);
			if(!mmul.isOne()) mstruct /= mmul;
			return true;
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_LOGINT && mstruct.size() == 1) {
		MathStructure madd, mmul, mexp;
		if(mpow.isOne() && integrate_info(mstruct[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(mfac.isOne()) {
				MathStructure mEi(mstruct);
				mstruct *= mEi[0];
				mEi.setFunctionId(FUNCTION_ID_LOG);
				mEi *= nr_two;
				mEi.transformById(FUNCTION_ID_EXPINT);
				mstruct -= mEi;
				if(!mmul.isOne()) mstruct /= mmul;
				return true;
			} else if(madd.isZero() && mfac.isPower() && mfac[0] == x_var && mfac[1].isMinusOne()) {
				MathStructure mln(mstruct);
				mln.setFunctionId(FUNCTION_ID_LOG);
				mstruct *= mln;
				mstruct -= mln[0];
				return true;
			}
		}
	} else if(mstruct.function()->id() == FUNCTION_ID_DIFFERENTIATE && (mstruct.size() == 3 || (mstruct.size() == 4 && mstruct[3].isUndefined())) && mstruct[1] == x_var) {
		if(!mpow.isOne() || !mfac.isOne()) return false;
		if(mstruct[2].isOne()) {
			mstruct.setToChild(1, true);
		} else {
			mstruct[2] += m_minus_one;
		}
		return true;
	} else if(mstruct.function()->id() == FUNCTION_ID_DIFFERENTIATE && mstruct.size() == 2 && mstruct[1] == x_var) {
		if(!mpow.isOne() || !mfac.isOne()) return false;
		mstruct.setToChild(1, true);
		return true;
	} else {
		return false;
	}
	if(mstruct.size() == 0) return false;
	bool by_parts_tested = false;

	if(mfac.isOne() && (mstruct.function()->id() == FUNCTION_ID_LOG || mstruct.function()->id() == FUNCTION_ID_ASIN || mstruct.function()->id() == FUNCTION_ID_ACOS || mstruct.function()->id() == FUNCTION_ID_ATAN || mstruct.function()->id() == FUNCTION_ID_ASINH || mstruct.function()->id() == FUNCTION_ID_ACOSH || mstruct.function()->id() == FUNCTION_ID_ATANH)) {
		by_parts_tested = true;
		//integrate by parts
		if(max_part_depth > 0) {
			MathStructure minteg(mstruct);
			if(!mpow.isOne()) minteg ^= mpow;
			CALCULATOR->beginTemporaryStopMessages();
			if(minteg.differentiate(x_var, eo) && minteg.containsFunctionId(FUNCTION_ID_DIFFERENTIATE, true) <= 0 && (!definite_integral || check_zero_div(minteg, x_var, eo))) {
				minteg *= x_var;
				EvaluationOptions eo2 = eo;
				eo2.expand = true;
				eo2.combine_divisions = false;
				eo2.sync_units = false;
				minteg.evalSort(true);
				minteg.calculateFunctions(eo);
				minteg.calculatesub(eo2, eo2, true);
				combine_ln(minteg, x_var, eo2);
				do_simplification(minteg, eo2, true, false, false, true, true);
				if(minteg.integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth - 1, parent_parts) > 0 && minteg.containsFunctionId(FUNCTION_ID_INTEGRATE, true) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					if(!mpow.isOne()) mstruct ^= mpow;
					mstruct.multiply(x_var);
					mstruct.subtract(minteg);
					return true;
				}
			}
			CALCULATOR->endTemporaryStopMessages();
		}
	}

	MathStructure madd, mmul, mexp;
	if(integrate_info(mstruct[0], x_var, madd, mmul, mexp, false, false, true) && !mexp.isZero()) {
		if(mexp.isPower() && (mexp[1] == x_var || (mexp[1].isMultiplication() && mexp[1].size() >= 2 && mexp[1].last() == x_var)) && mfac.isPower() && (mfac[1] == x_var || (mfac[1].isMultiplication() && mfac[1].size() >= 2 && mfac[1].last() == x_var)) && (mfac[0] == mexp[0] || (mfac[0].isNumber() && mfac[0].number().isRational() && mexp[0].isNumber() && mexp[0].number().isRational() && mexp[1] == mfac[1])) && mexp[0].containsRepresentativeOf(x_var, true, true) == 0) {
			Number pow1(1, 1), pow2;
			MathStructure morig(mexp);
			bool b = true;
			if(mfac[1].isMultiplication()) {
				if(!mexp[1].isMultiplication()) {
					if(mfac[1].size() != 2 || !mfac[1][0].isNumber()) b = false;
					else {pow2 = mfac[1][0].number(); pow2--;}
				} else {
					for(size_t i = 0; i < mexp[1].size() - 1; i++) {
						if(i == 0 && mexp[1][i].isNumber()) {
							pow1 = mexp[1][i].number();
						} else if(mfac[1][i].containsRepresentativeOf(x_var, true, true) != 0) {
							b = false;
							break;
						}
					}
					if(b) {
						if(mexp[1] == mfac[1]) {
							pow1.set(1, 1);
						} else if(mfac[1].size() - (mfac[1][0].isNumber() ? 1 : 0) != mexp[1].size() - (mexp[1][0].isNumber() ? 1 : 0)) {
							b = false;
						} else if(b) {
							for(size_t i = 0; i < mfac[1].size() - 1; i++) {
								if(i == 0 && mfac[1][i].isNumber()) {
									pow2 = mfac[1][i].number();
									pow2--;
								} else if(mfac[1][i] != mexp[1][i + (mexp[1][0].isNumber() ? 1 : 0) - (mfac[1][0].isNumber() ? 1 : 0)]) {
									b = false;
									break;
								}
							}
						}
					}
				}
			} else if(mexp[1].isMultiplication()) {
				if(mexp[1].size() != 2 || !mexp[1][0].isNumber()) b = false;
				else pow1 = mexp[1][0].number();
			}
			if(b && !pow1.isOne()) morig[1].delChild(1, true);
			if(b && mfac[0] != mexp[0]) {
				bool b1 = mfac[0].number() < mexp[0].number();
				if(mfac[0].number().isFraction() || mexp[0].number().isFraction()) b1 = !b1;
				Number nlog(b1 ? mexp[0].number() : mfac[0].number());
				nlog.log(b1 ? mfac[0].number() : mexp[0].number());
				if(!nlog.isInteger()) {
					nlog.round();
					b = nlog.isInteger() && (b1 ? ((mfac[0].number() ^ nlog) == mexp[0].number()) : ((mfac[0].number() ^ nlog) == mexp[0].number()));
				}
				if(b) {
					if(b1) {
						pow1 = nlog;
						morig = mfac;
					} else {
						pow2 = nlog;
						pow2--;
					}
				}
			}
			if(b) {
				UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(morig) + RIGHT_PARENTHESIS);
				MathStructure mtest(var);
				if(!pow1.isOne()) mtest ^= pow1;
				if(!mmul.isOne()) mtest *= mmul;
				if(!madd.isZero()) mtest += madd;
				mtest.transform(mstruct.function());
				if(!mpow.isOne()) mtest ^= mpow;
				if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
					MathStructure m_interval(morig);
					m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
					var->setInterval(m_interval);
				} else {
					var->setInterval(morig);
				}
				if(!pow2.isZero()) {
					mtest *= var;
					if(!pow2.isOne()) mtest.last() ^= pow2;
					mtest.swapChildren(1, 2);
				}
				CALCULATOR->beginTemporaryStopMessages();
				if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					mstruct.set(mtest, true);
					mstruct.replace(var, morig);
					if(!morig[0].isVariable() || morig[0].variable()->id() != VARIABLE_ID_E) mstruct.divide_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &morig[0], NULL));
					if(morig[1].isMultiplication()) {
						morig[1].delChild(morig[1].size(), true);
						mstruct /= morig[1];
					}
					var->destroy();
					return true;
				}
				CALCULATOR->endTemporaryStopMessages();
				var->destroy();
			}
		} else if(mexp == x_var && !madd.isZero() && (mfac.isOne() || mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].isInteger()))) {
			MathStructure morig(x_var);
			if(!mmul.isOne()) morig *= mmul;
			morig += madd;
			UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(morig) + RIGHT_PARENTHESIS);
			MathStructure mtest(var);
			mtest.transform(mstruct.function());
			if(!mpow.isOne()) mtest ^= mpow;
			if(!mfac.isOne()) {
				MathStructure mrepl(var);
				mrepl -= madd;
				mrepl /= mmul;
				MathStructure mfacnew(mfac);
				mfacnew.replace(x_var, mrepl);
				mtest *= mfacnew;
			}
			if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
				MathStructure m_interval(morig);
				m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
				var->setInterval(m_interval);
			} else {
				var->setInterval(morig);
			}
			CALCULATOR->beginTemporaryStopMessages();
			if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				mstruct.set(mtest, true);
				mstruct.replace(var, morig);
				if(!mmul.isOne()) mstruct /= mmul;
				var->destroy();
				return true;
			}
			CALCULATOR->endTemporaryStopMessages();
			var->destroy();
		} else if(mfac.isOne() && (mexp.isPower() && mexp[0] == x_var && mexp[1].isNumber() && !mexp[1].number().isInteger() && mexp[1].number().isRational())) {
			Number num(mexp[1].number().numerator());
			Number den(mexp[1].number().denominator());
			if(num.isPositive() || num.isMinusOne()) {
				MathStructure morig(x_var);
				if(num.isNegative()) den.negate();
				Number den_inv(den);
				den_inv.recip();
				morig ^= den_inv;
				UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(morig) + RIGHT_PARENTHESIS);
				Number den_m1(den);
				den_m1--;
				MathStructure mtest(var);
				if(!num.isOne() && !num.isMinusOne()) mtest ^= num;
				if(!mmul.isOne()) mtest *= mmul;
				if(!madd.isZero()) mtest += madd;
				mtest.transform(mstruct.function());
				if(!mpow.isOne()) mtest ^= mpow;
				mtest *= var;
				if(!den_m1.isOne()) {
					mtest.last() ^= den_m1;
					mtest.childUpdated(mtest.size());
				}
				if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
					MathStructure m_interval(morig);
					m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
					var->setInterval(m_interval);
				} else {
					var->setInterval(morig);
				}
				CALCULATOR->beginTemporaryStopMessages();
				if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					mstruct.set(mtest, true);
					mstruct.replace(var, morig);
					mstruct.multiply(den);
					var->destroy();
					return true;
				}
				CALCULATOR->endTemporaryStopMessages();
				var->destroy();
			}
		} else if((mfac.isOne() || mfac == x_var || (mfac.isPower() && mfac[0] == x_var && mfac[1].isInteger())) && (mexp.isPower() && mexp[0] != x_var && mexp[1].isNumber())) {
			MathStructure madd2, mmul2, mexp2;
			if(integrate_info(mexp[0], x_var, madd2, mmul2, mexp2) && (!madd.isZero() || mexp != x_var) && mexp2.isOne()) {
				UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mexp[0]) + RIGHT_PARENTHESIS);
				if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
					MathStructure m_interval(mexp[0]);
					m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
					var->setInterval(m_interval);
				} else {
					var->setInterval(mexp[0]);
				}
				MathStructure mtest(var);
				mtest ^= mexp[1];
				if(!mmul.isOne()) mtest *= mmul;
				if(!madd.isZero()) mtest += madd;
				mtest.transform(mstruct.function());
				if(!mpow.isOne()) mtest ^= mpow;
				if(!mfac.isOne()) {
					mtest *= var;
					if(!madd2.isZero()) {
						mtest.last() -= madd2;
						mtest.childUpdated(mtest.size());
					}
					if(mfac.isPower()) {
						mtest.last() ^= mfac[1];
						mtest.childUpdated(mtest.size());
					}
				}
				CALCULATOR->beginTemporaryStopMessages();
				if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					mstruct.set(mtest, true);
					mstruct.replace(var, mexp[0]);
					if(!mmul2.isOne()) {
		 				mstruct /= mmul2;
						if(!mfac.isOne()) {
							if(mfac.isPower()) mmul2 ^= mfac[1];
							mstruct /= mmul2;
						}
					}
					var->destroy();
					return true;
				}
				CALCULATOR->endTemporaryStopMessages();
				var->destroy();
			}
		}
	}
	if(mstruct[0].isAddition()) {
		bool b = true;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].containsType(STRUCT_ADDITION, true) == 1) {
				b = false;
				break;
			}
		}
		if(b) {
			MathStructure mtest(mstruct);
			if(mtest[0].factorize(eo, false, 0, 0, false, false, NULL, x_var, true, true)) {
				if(integrate_function(mtest, x_var, eo, mpow, mfac, mpowadd, mpowmul, use_abs, definite_integral, max_part_depth, parent_parts)) {
					mstruct = mtest;
					return true;
				}
			}
		}
	}

	if(!mfac.isOne()) return false;
	MathStructure *m_func = NULL, *m_pow = NULL;
	if(mstruct[0].isFunction() && mstruct[0].contains(x_var, true) > 0) {
		m_func = &mstruct[0];
	} else if(mstruct[0].isPower() && mstruct[0][0].isFunction() && mstruct[0][0].contains(x_var, true) > 0) {
		m_func = &mstruct[0][0];
	} else if(mstruct[0].isPower() && mstruct[0][1].contains(x_var, true) > 0) {
		m_pow = &mstruct[0];
	} else if(mstruct[0].isMultiplication()) {
		for(size_t i = 0; i < mstruct[0].size(); i++) {
			if(mstruct[0][i].isFunction() && mstruct[0][i].contains(x_var, true) > 0) {
				m_func = &mstruct[0][i];
			} else if(mstruct[0][i].isPower() && mstruct[0][i][0].isFunction() && mstruct[0][i][0].contains(x_var, true) > 0) {
				m_func = &mstruct[0][i][0];
			} else if(mstruct[0][i].isPower() && mstruct[0][i][1].contains(x_var, true) > 0) {
				m_pow = &mstruct[0][i];
			}
		}
	} else if(mstruct[0].isAddition()) {
		for(size_t i2 = 0; i2 < mstruct[0].size(); i2++) {
			if(mstruct[0][i2].isFunction() && mstruct[0][i2].contains(x_var, true) > 0) {
				m_func = &mstruct[0][i2];
			} else if(mstruct[0][i2].isPower() && mstruct[0][i2][0].isFunction() && mstruct[0][i2][0].contains(x_var, true) > 0) {
				m_func = &mstruct[0][i2][0];
			} else if(mstruct[0][i2].isPower() && mstruct[0][i2][1].contains(x_var, true) > 0) {
				m_pow = &mstruct[0][i2];
			} else if(mstruct[0][i2].isMultiplication()) {
				for(size_t i = 0; i < mstruct[0][i2].size(); i++) {
					if(mstruct[0][i2][i].isFunction() && mstruct[0][i2][i].contains(x_var, true) > 0) {
						m_func = &mstruct[0][i2][i];
					} else if(mstruct[0][i2][i].isPower() && mstruct[0][i2][i][0].isFunction() && mstruct[0][i2][i][0].contains(x_var, true) > 0) {
						m_func = &mstruct[0][i2][i][0];
					} else if(mstruct[0][i2][i].isPower() && mstruct[0][i2][i][1].contains(x_var, true) > 0) {
						m_pow = &mstruct[0][i2][i];
					}
				}
			}
		}
	}
	if(m_func && m_pow) return false;
	if(m_func) {
		if((m_func->function()->id() == FUNCTION_ID_LOG || m_func->function()->id() == FUNCTION_ID_ASIN || m_func->function()->id() == FUNCTION_ID_ACOS || m_func->function()->id() == FUNCTION_ID_ASINH || m_func->function()->id() == FUNCTION_ID_ACOSH) && m_func->size() == 1 && integrate_info((*m_func)[0], x_var, madd, mmul, mexp) && mexp.isOne()) {
			MathStructure m_orig(*m_func);
			UnknownVariable *var = new UnknownVariable("", format_and_print(m_orig));
			MathStructure mtest(mstruct);
			if(!mpow.isOne()) mtest ^= mpow;
			mtest[0].replace(m_orig, var, (*m_func)[0].containsInterval());
			if(mtest[0].containsRepresentativeOf(x_var, true, true) == 0) {
				if(m_func->function()->id() == FUNCTION_ID_LOG) {
					MathStructure m_epow(CALCULATOR->getVariableById(VARIABLE_ID_E));
					m_epow ^= var;
					mtest *= m_epow;
				} else if(m_func->function()->id() == FUNCTION_ID_ASIN) {
					MathStructure m_cos(var);
					m_cos *= CALCULATOR->getRadUnit();
					m_cos.transformById(FUNCTION_ID_COS);
					mtest *= m_cos;
				} else if(m_func->function()->id() == FUNCTION_ID_ACOS) {
					MathStructure m_sin(var);
					m_sin *= CALCULATOR->getRadUnit();
					m_sin.transformById(FUNCTION_ID_SIN);
					mtest *= m_sin;
					mmul.negate();
				} else if(m_func->function()->id() == FUNCTION_ID_ASINH) {
					MathStructure m_cos(var);
					m_cos.transformById(FUNCTION_ID_COSH);
					mtest *= m_cos;
				} else if(m_func->function()->id() == FUNCTION_ID_ACOSH) {
					MathStructure m_sin(var);
					m_sin.transformById(FUNCTION_ID_SINH);
					mtest *= m_sin;
				}
				if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
					MathStructure m_interval(m_orig);
					m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
					var->setInterval(m_interval);
				} else {
					var->setInterval(m_orig);
				}
				CALCULATOR->beginTemporaryStopMessages();
				if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					mstruct.set(mtest, true);
					mstruct.replace(var, m_orig);
					if(!mmul.isOne()) mstruct /= mmul;
					var->destroy();
					return true;
				}
				CALCULATOR->endTemporaryStopMessages();
			}
			var->destroy();
		}
	}
	if(m_pow) {
		if((*m_pow)[0].containsRepresentativeOf(x_var, true, true) == 0 && integrate_info((*m_pow)[1], x_var, madd, mmul, mexp) && mexp.isOne()) {
			if(definite_integral && ((*m_pow)[0].compare(m_zero) != COMPARISON_RESULT_LESS)) return false;
			MathStructure m_orig(*m_pow);
			UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(m_orig) + RIGHT_PARENTHESIS);
			MathStructure mtest(mstruct);
			mtest[0].replace(m_orig, var, m_pow->containsInterval());
			if(mtest[0].containsRepresentativeOf(x_var, true, true) == 0) {
				if(!mpow.isOne()) mtest ^= mpow;
				mtest /= var;
				if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
					MathStructure m_interval(m_orig);
					m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
					var->setInterval(m_interval);
				} else {
					var->setInterval(m_orig);
				}
				CALCULATOR->beginTemporaryStopMessages();
				if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					mstruct.set(mtest, true);
					mstruct.replace(var, m_orig);
					MathStructure m_ln(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &m_orig[0], NULL);
					mstruct /= m_ln;
					if(!mmul.isOne()) mstruct /= mmul;
					var->destroy();
					return true;
				}
				CALCULATOR->endTemporaryStopMessages();
			}
			var->destroy();
		}
	}

	if(!by_parts_tested && mstruct.function()->id() != FUNCTION_ID_SIN && mstruct.function()->id() != FUNCTION_ID_COS && mstruct.function()->id() != FUNCTION_ID_TAN && mstruct.function()->id() != FUNCTION_ID_SINH && mstruct.function()->id() != FUNCTION_ID_COSH && mstruct.function()->id() != FUNCTION_ID_TANH) {
		//integrate by parts
		if(max_part_depth > 0) {
			MathStructure minteg(mstruct);
			if(!mpow.isOne()) minteg ^= mpow;
			CALCULATOR->beginTemporaryStopMessages();
			if(minteg.differentiate(x_var, eo) && minteg.containsFunctionId(FUNCTION_ID_DIFFERENTIATE, true) <= 0 && (!definite_integral || check_zero_div(minteg, x_var, eo))) {
				minteg *= x_var;
				EvaluationOptions eo2 = eo;
				eo2.expand = true;
				eo2.combine_divisions = false;
				eo2.sync_units = false;
				minteg.evalSort(true);
				minteg.calculateFunctions(eo);
				minteg.calculatesub(eo2, eo2, true);
				combine_ln(minteg, x_var, eo2);
				do_simplification(minteg, eo2, true, false, false, true, true);
				if(minteg.integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth - 1, parent_parts) > 0 && minteg.containsFunctionId(FUNCTION_ID_INTEGRATE, true) <= 0) {
					CALCULATOR->endTemporaryStopMessages(true);
					if(!mpow.isOne()) mstruct ^= mpow;
					mstruct.multiply(x_var);
					mstruct.subtract(minteg);
					return true;
				}
			}
			CALCULATOR->endTemporaryStopMessages();
		}
	}
	return false;
}

#define CANNOT_INTEGRATE {MathStructure minteg(CALCULATOR->getFunctionById(FUNCTION_ID_INTEGRATE), this, &m_undefined, &m_undefined, &x_var, &m_zero, NULL); set(minteg); return false;}
#define CANNOT_INTEGRATE_INTERVAL {MathStructure minteg(CALCULATOR->getFunctionById(FUNCTION_ID_INTEGRATE), this, &m_undefined, &m_undefined, &x_var, &m_zero, NULL); set(minteg); return -1;}

int contains_unsolved_integrate(const MathStructure &mstruct, MathStructure *this_mstruct, vector<MathStructure*> *parent_parts) {
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_INTEGRATE) {
		if(this_mstruct->equals(mstruct[0], true)) return 3;
		for(size_t i = 0; i < parent_parts->size(); i++) {
			if(mstruct[0].equals(*(*parent_parts)[i], true)) return 2;
		}
		return 1;
	}
	int ret = 0;
	for(size_t i = 0; i < mstruct.size(); i++) {
		int ret_i = contains_unsolved_integrate(mstruct[i], this_mstruct, parent_parts);
		if(ret_i == 1) {
			return 1;
		} else if(ret_i > ret) {
			ret = ret_i;
		}
	}
	return ret;
}

// remove abs() when possible
bool fix_abs_x(MathStructure &mstruct, const MathStructure &x_var, const EvaluationOptions &eo, bool definite_integral) {
	bool b = false;
	if(mstruct.isFunction() && mstruct.size() == 1 && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ABS && mstruct[0].size() == 1) {
		if(!definite_integral && (mstruct.function()->id() == FUNCTION_ID_SIN || mstruct.function()->id() == FUNCTION_ID_TAN || mstruct.function()->id() == FUNCTION_ID_SINH || mstruct.function()->id() == FUNCTION_ID_TANH || mstruct.function()->id() == FUNCTION_ID_ASIN || mstruct.function()->id() == FUNCTION_ID_ATAN || mstruct.function()->id() == FUNCTION_ID_ASINH || mstruct.function()->id() == FUNCTION_ID_ATANH) && mstruct[0][0].representsNonComplex(true)) {
			// sin(abs(f(x)))=sgn(f(x))*sin(f(x)) if not f(x) is complex
			mstruct[0].setToChild(1, true);
			mstruct.multiply_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SIGNUM), &mstruct[0], &m_zero, NULL));
			mstruct.evalSort(false);
			b = true;
		} else if((mstruct.function()->id() == FUNCTION_ID_COS || mstruct.function()->id() == FUNCTION_ID_COSH) && mstruct[0][0].representsNonComplex(true)) {
			// cos(abs(f(x)))=cos(f(x)) if not f(x) is complex
			mstruct[0].setToChild(1, true);
			b = true;
		} else if(mstruct.function()->id() == FUNCTION_ID_LOG) {
			// ln(abs(f(x)))=ln(f(x)^2)/2 if not f(x) is complex
			if(mstruct[0][0].representsNonComplex(true)) {
				b = true;
			} else {
				MathStructure mtest(mstruct[0][0]);
				transform_absln(mtest, -1, false, x_var, eo);
				b = !mtest.isFunction() || mtest.function()->id() != FUNCTION_ID_LOG || (mtest.size() > 0 && mtest[0].isFunction() && mtest[0].function()->id() == FUNCTION_ID_ABS);
			}
			if(b) {
				mstruct[0].setToChild(1, true);
				mstruct[0] ^= nr_two;
				mstruct /= nr_two;
			}
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(fix_abs_x(mstruct[i], x_var, eo, definite_integral)) b = true;
	}
	return b;
}

MathStructure *find_abs_x2(MathStructure &mstruct, const MathStructure &x_var, const MathStructure *parent = NULL, int level = 0) {
	if(mstruct.isFunction()) {
		if(((mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1) || (mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct) && mstruct[1].number().isOdd())) && mstruct[0].contains(x_var, true) > 0 && mstruct[0].representsNonComplex(true)) {
			return &mstruct[0];
		}
		if((!parent || parent->isMultiplication() || parent->isAddition()) && level <= 2 && mstruct.function()->id() == FUNCTION_ID_LOG && mstruct.size() == 1) {
			if((mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ROOT) || (mstruct[0].isPower() && mstruct[0][1].isInteger() && mstruct[0][0].isFunction() && mstruct[0][0].function()->id() == FUNCTION_ID_ROOT)) return NULL;
			if(mstruct[0].isMultiplication() && mstruct[0].size() == 2 && ((mstruct[0][1].isFunction() && mstruct[0][1].function()->id() == FUNCTION_ID_ROOT) || (mstruct[0][1].isPower() && mstruct[0][1][1].isInteger() && mstruct[0][1][0].isFunction() && mstruct[0][1][0].function()->id() == FUNCTION_ID_ROOT))) return NULL;
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		MathStructure *m = find_abs_x2(mstruct[i], x_var, &mstruct, level + 1);
		if(m) return m;
	}
	return NULL;
}
bool fix_sgn_x(MathStructure &mstruct, const MathStructure &x_var, const EvaluationOptions &eo) {
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2) {
		MathStructure mtest(mstruct);
		KnownVariable *var = new KnownVariable("", format_and_print(x_var), ((UnknownVariable*) x_var.variable())->interval());
		mtest.replace(x_var, var);
		CALCULATOR->beginTemporaryStopMessages();
		mtest.eval(eo);
		var->destroy();
		if(!CALCULATOR->endTemporaryStopMessages() && !mtest.isFunction()) {
			mstruct.set(mtest);
			return true;
		}
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(fix_sgn_x(mstruct[i], x_var, eo)) b = true;
	}
	return b;
}
bool replace_abs_x(MathStructure &mstruct, const MathStructure &marg, bool b_minus, const MathStructure *parent = NULL, int level = 0) {
	if(mstruct.isFunction()) {
		if(mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1 && mstruct[0].equals(marg, true)) {
			mstruct.setToChild(1);
			if(b_minus) mstruct.negate();
			return true;
		} else if(mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct) && mstruct[1].number().isOdd() && mstruct[0].equals(marg, true)) {
			if(b_minus) mstruct[0].negate();
			mstruct[1].number().recip();
			mstruct.setType(STRUCT_POWER);
			mstruct.childrenUpdated();
			if(b_minus) mstruct.negate();
			return true;
		}
		if((!parent || parent->isMultiplication() || parent->isAddition()) && level <= 2 && mstruct.function()->id() == FUNCTION_ID_LOG && mstruct.size() == 1) {
			if((mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ROOT) || (mstruct[0].isPower() && mstruct[0][1].isInteger() && mstruct[0][0].isFunction() && mstruct[0][0].function()->id() == FUNCTION_ID_ROOT)) return false;
			if(mstruct[0].isMultiplication() && mstruct[0].size() == 2 && ((mstruct[0][1].isFunction() && mstruct[0][1].function()->id() == FUNCTION_ID_ROOT) || (mstruct[0][1].isPower() && mstruct[0][1][1].isInteger() && mstruct[0][1][0].isFunction() && mstruct[0][1][0].function()->id() == FUNCTION_ID_ROOT))) return false;
		}
	}
	if(mstruct.isPower() && mstruct[1].isInteger() && mstruct[1].number().isOdd() && mstruct[0].isFunction() && mstruct[0].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct[0]) && mstruct[0][1].number().isOdd() && mstruct[0][0].equals(marg, true)) {
		mstruct[1].number().divide(mstruct[0][1].number());
		mstruct[0].setToChild(1, true);
		if(mstruct[1].number().isOne()) mstruct.setToChild(1, true);
		if(b_minus) mstruct[0].negate();
		mstruct.childrenUpdated();
		if(b_minus) mstruct.negate();
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_abs_x(mstruct[i], marg, b_minus, &mstruct, level + 1)) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	return b;
}

bool test_sign_zero(const MathStructure &m, const MathStructure &x_var, const MathStructure &mzero, const EvaluationOptions &eo) {
	if(m.contains(x_var, true) <= 0) return false;
	if(m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isPower() && m[i][1].representsNegative() && test_sign_zero(m[i][0], x_var, mzero, eo)) return true;
		}
	}
	CALCULATOR->beginTemporaryStopMessages();
	MathStructure mtest(m);
	mtest.replace(x_var, mzero);
	mtest.transform(COMPARISON_EQUALS, m_zero);
	mtest.eval(eo);
	return !CALCULATOR->endTemporaryStopMessages() && mtest.isOne();
}

void do_signum(MathStructure &m1, const MathStructure &marg) {
	/*m1 *= marg;
	m1 *= marg;
	m1.last().transformById(FUNCTION_ID_ABS);
	m1.last().inverse();*/
	m1 *= MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_SIGNUM), &marg, &m_zero, NULL);
}
bool try_sign(MathStructure &m1, MathStructure &m2, const MathStructure &marg, const MathStructure &x_var, const MathStructure &mzero, const EvaluationOptions &eo, bool *test_zero = NULL) {
	if(m1.equals(m2, true)) return true;
	if(m1.isNumber() && m1.number().isReal() && m2.isNumber() && m2.number().isReal()) {
		m2.number().negate();
		if(m1.number().equals(m2.number(), true)) {
			m2.number().negate();
			if(!test_sign_zero(m1, x_var, mzero, eo)) {
				if(!test_zero) return false;
				*test_zero = true;
			}
			if(!test_zero || !(*test_zero)) do_signum(m1, marg);
			return true;
		}
		m2.number().negate();
		return false;
	}
	if(m1.type() != m2.type()) {
		if((m1.isMultiplication() && m1.size() == 2 && m1[0].isMinusOne() && try_sign(m1[1], m2, marg, x_var, mzero, eo)) || (m2.isMultiplication() && m2.size() == 2 && m2[0].isMinusOne() && try_sign(m1, m2[1], marg, x_var, mzero, eo))) {
			if(!test_sign_zero(m1, x_var, mzero, eo)) {
				if(!test_zero) return false;
				*test_zero = true;
			}
			if(!test_zero || !(*test_zero)) do_signum(m1, marg);
			return true;
		}
		return false;
	}
	if(m1.size() == 0) return false;
	if(m1.size() != m2.size()) {
		if(m1.isMultiplication() && ((m1.size() == m2.size() + 1 && m1[0].isMinusOne()) || (m2.size() == m1.size() + 1 && m2[0].isMinusOne()))) {
			if(m1.size() > m2.size()) {
				for(size_t i = 0; i < m2.size(); i++) {
					if(!try_sign(m1[i + 1], m2[i], marg, x_var, mzero, eo)) return false;
				}
			} else {
				for(size_t i = 0; i < m1.size(); i++) {
					if(!try_sign(m1[i], m2[i + 1], marg, x_var, mzero, eo)) return false;
				}
			}
			if(!test_sign_zero(m1, x_var, mzero, eo)) {
				if(!test_zero) return false;
				*test_zero = true;
			}
			if(!test_zero || !(*test_zero)) do_signum(m1, marg);
			return true;
		}
		return false;
	}
	if(m1.isFunction() && m1.function() != m2.function()) return false;
	if(m1.isComparison() && m1.comparisonType() != m2.comparisonType()) return false;
	if(m1.isMultiplication() && m1.size() > 1 && m1[0].isNumber() && !m1[0].equals(m2[0], true)) {
		if(!m1[0].isNumber()) return false;
		m2[0].number().negate();
		if(m1[0].number().equals(m2[0].number(), true)) {
			m2[0].number().negate();
			for(size_t i = 1; i < m1.size(); i++) {
				if(!try_sign(m1[i], m2[i], marg, x_var, mzero, eo)) return false;
			}
			if(!test_sign_zero(m1, x_var, mzero, eo)) {
				if(!test_zero) return false;
				*test_zero = true;
			}
			if(!test_zero || !(*test_zero)) do_signum(m1, marg);
			return true;
		}
		m2[0].number().negate();
		return false;
	}/* else if(m1.isPower()) {
		bool b_tz = false;
		if(!try_sign(m1[0], m2[0], marg, x_var, mzero, eo, &b_tz) || b_tz) return false;
		if(!try_sign(m1[1], m2[1], marg, x_var, mzero, eo, &b_tz)) return false;
		if(b_tz && (test_sign_zero(m1[1], x_var, mzero, eo) || !test_sign_zero(m1[0], x_var, mzero, eo))) return false;
		return true;
	}*/
	bool b_tz = false;
	bool b_equal = false;
	for(size_t i = 0; i < m1.size(); i++) {
		if(!b_equal && m1.isAddition() && m1[i].equals(m2[i], true)) b_equal = true;
		else if(!try_sign(m1[i], m2[i], marg, x_var, mzero, eo, m1.isAddition() ? &b_tz : NULL)) return false;
		if(b_tz && b_equal) break;
	}
	if(b_tz) {
		if(!test_sign_zero(m1, x_var, mzero, eo)) {
			if(!test_zero) return false;
			*test_zero = true;
		}
		if(!test_zero || !(*test_zero)) do_signum(m1, marg);
	}
	return true;
}
bool contains_imaginary(const MathStructure &m) {
	if(m.isNumber()) return m.number().hasImaginaryPart();
	if(m.isVariable() && m.variable()->isKnown()) return contains_imaginary(((KnownVariable*) m.variable())->get());
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_imaginary(m[i])) return true;
	}
	return false;
}

int MathStructure::integrate(const MathStructure &x_var, const EvaluationOptions &eo_pre, bool simplify_first, int use_abs, bool definite_integral, bool try_abs, int max_part_depth, vector<MathStructure*> *parent_parts) {

	if(CALCULATOR->aborted()) CANNOT_INTEGRATE

	EvaluationOptions eo = eo_pre;
	eo.protected_function = CALCULATOR->getFunctionById(FUNCTION_ID_INTEGRATE);
	EvaluationOptions eo2 = eo;
	eo2.expand = true;
	eo2.combine_divisions = false;
	eo2.sync_units = false;
	EvaluationOptions eo_t = eo;
	eo_t.approximation = APPROXIMATION_TRY_EXACT;
	eo_t.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;

	if(simplify_first) {
		unformat(eo_pre);
		calculateFunctions(eo2);
		calculatesub(eo2, eo2);
		combine_ln(*this, x_var, eo2);
		if(CALCULATOR->aborted()) CANNOT_INTEGRATE
	}
	bool recalc = false;

	// Try to remove abs() and sgn()

	if(fix_abs_x(*this, x_var, eo, definite_integral)) recalc = true;

	MathStructure *mfound = NULL;
	if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
		if(fix_sgn_x(*this, x_var, eo_t)) recalc = true;
		while(true) {
			mfound = find_abs_x2(*this, x_var);
			if(mfound) {
				MathStructure m_interval(*mfound);
				m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
				CALCULATOR->beginTemporaryStopMessages();
				m_interval.eval(eo_t);
				if(!CALCULATOR->endTemporaryStopMessages()) break;
				if(m_interval.representsNonNegative(true)) {
					replace_abs_x(*this, MathStructure(*mfound), false);
					recalc = true;
				} else if(m_interval.representsNegative(true)) {
					replace_abs_x(*this, MathStructure(*mfound), true);
					recalc = true;
				} else {
					break;
				}
			} else {
				break;
			}
		}
	}
	if(recalc) {
		calculatesub(eo2, eo2);
		combine_ln(*this, x_var, eo2);
	}

	// x: x^2/2
	if(equals(x_var)) {
		raise(2);
		multiply(MathStructure(1, 2, 0));
		return true;
	}

	// a: a*x
	if(containsRepresentativeOf(x_var, true, true) == 0) {
		multiply(x_var);
		return true;
	}

	if(m_type == STRUCT_ADDITION) {
		// f(x)+g(x): integral of f(x) + integral of g(x)
		bool b = false;
		MathStructure mbak(*this);
		for(size_t i = 0; i < SIZE; i++) {
			int bint = CHILD(i).integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts);
			if(bint < 0) {
				set(mbak);
				CANNOT_INTEGRATE_INTERVAL
			}
			if(bint > 0) b = true;
			CHILD_UPDATED(i);
		}
		if(!b) return false;
		return true;
	} else if(m_type == STRUCT_MULTIPLICATION) {
		// a*f(x): a * integral of f(x)
		MathStructure mstruct;
		bool b = false;
		for(size_t i = 0; i < SIZE;) {
			if(CHILD(i).containsRepresentativeOf(x_var, true, true) == 0) {
				if(b) {
					mstruct *= CHILD(i);
				} else {
					mstruct = CHILD(i);
					b = true;
				}
				ERASE(i);
			} else {
				i++;
			}
		}
		if(b) {
			if(SIZE == 1) {
				setToChild(1, true);
			} else if(SIZE == 0) {
				set(mstruct, true);
				return true;
			}
			if(integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) {
				multiply(mstruct);
				CANNOT_INTEGRATE_INTERVAL
			}
			multiply(mstruct);
			return true;
		}

		for(size_t i = 0; i < SIZE; i++) {
			if((CHILD(i).isFunction() && CHILD(i).function()->id() == FUNCTION_ID_ABS && CHILD(i).size() == 1) || (CHILD(i).isPower() && CHILD(i)[0].isFunction() && CHILD(i)[0].function()->id() == FUNCTION_ID_ABS && CHILD(i)[0].size() == 1 && CHILD(i)[1].representsOdd())) {
				if(definite_integral) CANNOT_INTEGRATE
				bool b = (m_type == STRUCT_POWER ? CHILD(0)[0].representsNonComplex(true) : CHILD(0).representsNonComplex(true));
				if(!b && !(m_type == STRUCT_POWER ? CHILD(0)[0].representsComplex(true) : CHILD(0).representsComplex(true))) {
					MathStructure mtest(m_type == STRUCT_POWER ? CHILD(0)[0] : CHILD(0));
					CALCULATOR->beginTemporaryStopMessages();
					mtest.eval(eo_t);
					CALCULATOR->endTemporaryStopMessages();
					b = mtest.representsNonComplex(true);
					if(!b && !mtest.representsComplex(true)) {
						if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
							CALCULATOR->beginTemporaryStopMessages();
							mtest.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
							mtest.eval(eo_t);
							CALCULATOR->endTemporaryStopMessages();
							b = mtest.representsNonComplex(true);
							if(!b && !mtest.representsComplex(true) && !contains_imaginary(mtest) && !contains_imaginary(((UnknownVariable*) x_var.variable())->interval())) {
								CALCULATOR->error(false, "Integral assumed real", NULL);
								b = true;
							}
						} else if(!contains_imaginary(mtest)) {
							CALCULATOR->error(false, "Integral assumed real", NULL);
							b = true;
						}
					}
				}
				if(b) {
					MathStructure mmul(CHILD(i).isPower() ? CHILD(i)[0] : CHILD(i));
					if(CHILD(i).isPower()) CHILD(i)[0].setToChild(1, true);
					else CHILD(i).setToChild(1, true);
					mmul.inverse();
					mmul *= CHILD(i).isPower() ? CHILD(i)[0] : CHILD(i);
					if(integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) {
						multiply(mmul);
						CANNOT_INTEGRATE_INTERVAL
					}
					multiply(mmul);
					return true;
				}
			}
		}
	} else if((m_type == STRUCT_FUNCTION && o_function->id() == FUNCTION_ID_ABS && SIZE == 1) || (m_type == STRUCT_POWER && CHILD(0).isFunction() && CHILD(0).function()->id() == FUNCTION_ID_ABS && CHILD(0).size() == 1 && CHILD(1).representsOdd())) {
		bool b = (m_type == STRUCT_POWER ? CHILD(0)[0].representsNonComplex(true) : CHILD(0).representsNonComplex(true));
		if(!b && !(m_type == STRUCT_POWER ? CHILD(0)[0].representsComplex(true) : CHILD(0).representsComplex(true))) {
			MathStructure mtest(m_type == STRUCT_POWER ? CHILD(0)[0] : CHILD(0));
			CALCULATOR->beginTemporaryStopMessages();
			mtest.eval(eo_t);
			CALCULATOR->endTemporaryStopMessages();
			b = mtest.representsNonComplex(true);
			if(!b && !mtest.representsComplex(true)) {
				if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
					CALCULATOR->beginTemporaryStopMessages();
					mtest.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
					mtest.eval(eo_t);
					CALCULATOR->endTemporaryStopMessages();
					b = mtest.representsNonComplex(true);
					if(!b && !mtest.representsComplex(true) && !contains_imaginary(mtest) && !contains_imaginary(((UnknownVariable*) x_var.variable())->interval())) {
						CALCULATOR->error(false, "Integral assumed real", NULL);
						b = true;
					}
				} else if(!contains_imaginary(mtest)) {
					CALCULATOR->error(false, "Integral assumed real", NULL);
					b = true;
				}
			}
		}
		if(b) {
			if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
				if(fix_sgn_x(*this, x_var, eo_t)) recalc = true;
				while(true) {
					mfound = find_abs_x2(*this, x_var);
					if(mfound) {
						MathStructure m_interval(*mfound);
						m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
						CALCULATOR->beginTemporaryStopMessages();
						m_interval.eval(eo_t);
						if(!CALCULATOR->endTemporaryStopMessages()) break;
						if(m_interval.representsNonNegative(true)) {
							replace_abs_x(*this, MathStructure(*mfound), false);
							recalc = true;
						} else if(m_interval.representsNegative(true)) {
							replace_abs_x(*this, MathStructure(*mfound), true);
							recalc = true;
						} else {
							break;
						}
					} else {
						break;
					}
				}
			}
			if(m_type == STRUCT_FUNCTION && CHILD(0).isFunction() && (CHILD(0).function()->id() == FUNCTION_ID_SIN || CHILD(0).function()->id() == FUNCTION_ID_COS) && CHILD(0).size() == 1) {
				MathStructure madd, mmul, mexp;
				if(integrate_info(CHILD(0)[0], x_var, madd, mmul, mexp, true) && mexp.isOne() && madd.isZero()) {
					// abs(sin(ax)): 2/a*floor(ax/pi)-1/a*cos(ax-floor(ax/pi)*pi)
					// abs(cos(ax)): 2/a*floor(ax/pi+1/2)+1/a*sin(ax-floor(ax/pi+1/2)*pi)
					setToChild(1, true);
					MathStructure mdivpi(x_var);
					if(!mmul.isOne()) mdivpi *= mmul;
					mdivpi /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					if(o_function->id() == FUNCTION_ID_COS) mdivpi += nr_half;
					mdivpi.transformById(FUNCTION_ID_FLOOR);
					if(o_function->id() == FUNCTION_ID_SIN) setFunctionId(FUNCTION_ID_COS);
					else setFunctionId(FUNCTION_ID_SIN);
					CHILD(0) += mdivpi;
					CHILD(0).last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					CHILD(0).last() *= CALCULATOR->getRadUnit();
					CHILD(0).last().negate();
					CHILD(0).childUpdated(CHILD(0).size());
					CHILD_UPDATED(0)
					if(o_function->id() == FUNCTION_ID_COS) negate();
					if(!mmul.isOne()) divide(mmul);
					add(mdivpi);
					LAST *= nr_two;
					if(!mmul.isOne()) LAST /= mmul;
					CHILD_UPDATED(SIZE - 1)
					return true;
				}
			}
			MathStructure mstruct(m_type == STRUCT_POWER ? CHILD(0) : *this);
			if(m_type != STRUCT_POWER || (CHILD(1).isInteger() && !CHILD(1).number().isMinusOne() && CHILD(1).number().isOdd())) {
				MathStructure mexp, mmul, madd;
				if(integrate_info(m_type == STRUCT_POWER ? CHILD(0)[0] : CHILD(0), x_var, madd, mmul, mexp) && mexp.isOne()) {
					MathStructure mpowp1(m_type == STRUCT_POWER ? CHILD(1) : nr_two);
					if(m_type == STRUCT_POWER) {
						mpowp1 += m_one;
						setToChild(1, true);
					}
					MathStructure msgn(*this);
					msgn.setFunctionId(FUNCTION_ID_SIGNUM);
					setToChild(1, true);
					raise(mpowp1);
					divide(mmul);
					divide(mpowp1);
					multiply(msgn);
					return true;
				}
			}
			if(definite_integral) CANNOT_INTEGRATE
			MathStructure mmul;
			MathStructure mbak(*this);
			if(isPower()) {
				mmul = CHILD(0);
				CHILD(0).setToChild(1, true);
				mmul.inverse();
				mmul *= CHILD(0);
			} else {
				mmul = *this;
				setToChild(1, true);
				mmul.inverse();
				mmul *= *this;
			}
			int b_int = integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts);
			if(b_int <= 0) {
				set(mbak);
				if(b_int < 0) CANNOT_INTEGRATE_INTERVAL
				CANNOT_INTEGRATE
			}
			multiply(mmul);
			return true;
		}
	}
	mfound = try_abs ? find_abs_x2(*this, x_var) : NULL;
	if(mfound) {
		MathStructure mtest(*this);
		MathStructure mtest_m(mtest);
		if(!replace_abs_x(mtest, *mfound, false) || !replace_abs_x(mtest_m, *mfound, true)) CANNOT_INTEGRATE
		eo_t.isolate_x = true;
		eo_t.isolate_var = &x_var;
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mpos(*mfound);
		mpos.transform(COMPARISON_EQUALS_GREATER, m_zero);
		mpos.eval(eo_t);
		UnknownVariable *var_p = NULL, *var_m = NULL;
		if(!CALCULATOR->endTemporaryStopMessages() && mpos.isComparison() && (mpos.comparisonType() == COMPARISON_EQUALS_GREATER || mpos.comparisonType() == COMPARISON_EQUALS_LESS) && mpos[0] == x_var && mpos[1].isNumber()) {
			var_p = new UnknownVariable("", format_and_print(x_var));
			var_m = new UnknownVariable("", format_and_print(x_var));
			Number nr_interval_p, nr_interval_m;
			if(x_var.isVariable() && !x_var.variable()->isKnown() && ((UnknownVariable*) x_var.variable())->interval().isNumber() && ((UnknownVariable*) x_var.variable())->interval().number().isInterval() && ((UnknownVariable*) x_var.variable())->interval().number().isReal() && ((UnknownVariable*) x_var.variable())->interval().number().upperEndPoint().isGreaterThanOrEqualTo(mpos[1].number()) && ((UnknownVariable*) x_var.variable())->interval().number().lowerEndPoint().isLessThanOrEqualTo(mpos[1].number())) {
				if(mpos.comparisonType() == COMPARISON_EQUALS_GREATER) {
					nr_interval_p.setInterval(mpos[1].number(), ((UnknownVariable*) x_var.variable())->interval().number().upperEndPoint());
					nr_interval_m.setInterval(((UnknownVariable*) x_var.variable())->interval().number().lowerEndPoint(), mpos[1].number());
				} else {
					nr_interval_m.setInterval(mpos[1].number(), ((UnknownVariable*) x_var.variable())->interval().number().upperEndPoint());
					nr_interval_p.setInterval(((UnknownVariable*) x_var.variable())->interval().number().lowerEndPoint(), mpos[1].number());
				}
			} else {

				if(mpos.comparisonType() == COMPARISON_EQUALS_GREATER) {
					nr_interval_p.setInterval(mpos[1].number(), nr_plus_inf);
					nr_interval_m.setInterval(nr_minus_inf, mpos[1].number());
				} else {
					nr_interval_m.setInterval(mpos[1].number(), nr_plus_inf);
					nr_interval_p.setInterval(nr_minus_inf, mpos[1].number());
				}
			}
			var_p->setInterval(nr_interval_p);
			var_m->setInterval(nr_interval_m);
			mtest.replace(x_var, var_p);
			mtest_m.replace(x_var, var_m);
		}
		int bint1 = mtest.integrate(var_p ? var_p : x_var, eo, true, use_abs, true, definite_integral, max_part_depth, parent_parts);
		if(var_p) {
			mtest.replace(var_p, x_var);
			var_p->destroy();
		}
		if(bint1 <= 0 || mtest.containsFunctionId(FUNCTION_ID_INTEGRATE, true) > 0) {
			if(var_m) var_m->destroy();
			if(bint1 < 0) CANNOT_INTEGRATE_INTERVAL
			CANNOT_INTEGRATE;
		}
		int bint2 = mtest_m.integrate(var_m ? var_m : x_var, eo, true, use_abs, false, definite_integral, max_part_depth, parent_parts);
		if(var_m) {
			mtest_m.replace(var_m, x_var);
			var_m->destroy();
		}
		if(bint2 < 0) CANNOT_INTEGRATE_INTERVAL
		if(bint2 == 0 || mtest_m.containsFunctionId(FUNCTION_ID_INTEGRATE, true) > 0) CANNOT_INTEGRATE;
		MathStructure m1(mtest), m2(mtest_m);
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mzero(*mfound);
		mzero.transform(COMPARISON_EQUALS, m_zero);
		mzero.eval(eo_t);
		if(!CALCULATOR->endTemporaryStopMessages() && mzero.isComparison() && mzero.comparisonType() == COMPARISON_EQUALS && mzero[0] == x_var) {
			mzero.setToChild(2);
			CALCULATOR->beginTemporaryStopMessages();
			m1.calculatesub(eo2, eo2, true);
			m2.calculatesub(eo2, eo2, true);
			m1.evalSort(true, true);
			m2.evalSort(true, true);
			eo_t.test_comparisons = true;
			eo_t.isolate_x = false;
			eo_t.isolate_var = NULL;
			if((definite_integral && m1.equals(m2, true)) || (!definite_integral && try_sign(m1, m2, *mfound, x_var, mzero, eo_t))) {
				set(m1, true);
				CALCULATOR->endTemporaryStopMessages(true);
				return true;
			}
			CALCULATOR->endTemporaryStopMessages();
		}
		/*MathStructure mcmp(*mfound);
		mcmp.transform(COMPARISON_EQUALS_GREATER, m_zero);
		set(mtest);
		multiply(mcmp);
		mcmp.setComparisonType(COMPARISON_LESS);
		mtest_m *= mcmp;
		add(mtest_m);*/
		/*eo2.test_comparisons = true;
		calculatesub(eo2, eo2, true);*/

		///return true;
	}

	switch(m_type) {
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			// a: a*x
			multiply(x_var);
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(1).isNumber() || CHILD(1).containsRepresentativeOf(x_var, true, true) == 0) {
				bool b_minusone = (CHILD(1).isNumber() && CHILD(1).number().isMinusOne());
				MathStructure madd, mmul, mmul2, mexp;
				if(CHILD(0).isFunction()) {
					MathStructure mbase(CHILD(0));
					int bint = integrate_function(mbase, x_var, eo, CHILD(1), m_one, m_zero, m_one, use_abs, definite_integral, max_part_depth, parent_parts);
					if(bint < 0) CANNOT_INTEGRATE_INTERVAL
					if(bint) {
						set(mbase, true);
						return true;
					}
				} else if(integrate_info(CHILD(0), x_var, madd, mmul, mmul2, false, true)) {
					if(mmul2.isZero()) {
						if(madd.isZero() && mmul.isOne()) {
							if(b_minusone) {
								// x^-1: ln(x)
								if(!transform_absln(CHILD(0), use_abs, definite_integral, x_var, eo)) CANNOT_INTEGRATE_INTERVAL
								SET_CHILD_MAP(0);
							} else {
								// x^a: x^(a+1)/(a+1)
								CHILD(1) += m_one;
								MathStructure mstruct(CHILD(1));
								divide(mstruct);
							}
							return true;
						} else if(b_minusone) {
							// (ax+b)^-1: ln(ax+b)/a
							if(!transform_absln(CHILD(0), use_abs, definite_integral, x_var, eo)) CANNOT_INTEGRATE_INTERVAL
							SET_CHILD_MAP(0);
							if(!mmul.isOne()) divide(mmul);
							return true;
						} else {
							if(definite_integral && !madd.isZero() && (!x_var.representsNonComplex(true) || !mmul.representsNonComplex(true)) && !CHILD(1).representsInteger() && comparison_might_be_equal(x_var.compare(m_zero)) && !comparison_is_equal_or_less(madd.compare(m_zero))) CANNOT_INTEGRATE
							// (ax+b)^c: (ax+b)^(c+1)/(a(c+1))
							mexp = CHILD(1);
							mexp += m_one;
							SET_CHILD_MAP(0);
							raise(mexp);
							if(!mmul.isOne()) mexp *= mmul;
							divide(mexp);
							return true;
						}
					} else if(mmul.isZero() && !madd.isZero() && CHILD(1).number().denominatorIsTwo()) {
						MathStructure mmulsqrt2(mmul2);
						if(!mmul2.isOne()) mmulsqrt2 ^= nr_half;
						Number num(CHILD(1).number().numerator());
						if(num.isOne()) {
							// sqrt(ax^2+b): (ln(sqrt(ax^2+b)+x*sqrt(a))*b+sqrt(a)*b*sqrt(ax^2+b))/(2*sqrt(a))
							MathStructure mthis(*this);
							add(x_var);
							if(!mmul2.isOne()) LAST *= mmulsqrt2;
							if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mthis); CANNOT_INTEGRATE_INTERVAL}
							multiply(madd);
							mthis *= x_var;
							if(!mmul2.isOne()) mthis *= mmulsqrt2;
							add(mthis);
							multiply(nr_half);
							if(!mmul2.isOne()) divide(mmulsqrt2);
							childrenUpdated(true);
							return true;
						} else if(num == 3) {
							// (ax^2+b)^(3/2):
							// (ln(sqrt(ax^2+b)+x*sqrt(a))*b^2*3/8+(ax^2+b)^(3/2)*x*sqrt(a)/4+sqrt(ax^2+b)*x*sqrt(a)*b*3/8)/sqrt(a)
							MathStructure mterm3(*this);
							CHILD(1).number().set(1, 2, 0, true);
							MathStructure mterm2(*this);
							add(x_var);
							if(!mmul2.isOne()) LAST *= mmulsqrt2;
							if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mterm3); CANNOT_INTEGRATE_INTERVAL}
							multiply(madd);
							LAST ^= nr_two;
							multiply(Number(3, 8, 0), true);
							mterm2 *= x_var;
							if(!mmul2.isOne()) mterm2 *= mmulsqrt2;
							mterm2 *= madd;
							mterm2 *= Number(3, 8, 0);
							mterm3 *= x_var;
							if(!mmul2.isOne()) mterm3 *= mmulsqrt2;
							mterm3 *= Number(1, 4, 0);
							add(mterm2);
							add(mterm3);
							if(!mmul2.isOne()) divide(mmulsqrt2);
							return true;
						} else if(num == 5) {
							// (ax^2+b)^(5/2):
							// sqrt(ax^2+b)*x^5*a^2/6+ln(sqrt(ax^2+b)*sqrt(a)+ax)*b^3/sqrt(a)*5/16+sqrt(ax^2+b)*x*b^2*11/16+sqrt(ax^2+b)*x^3*b*a*13/24
							CHILD(1).number().set(1, 2, 0, true);
							MathStructure mterm2(*this);
							MathStructure mterm3(*this);
							MathStructure mterm4(*this);
							multiply(x_var);
							LAST ^= Number(5, 1);
							if(!mmul2.isOne()) {
								multiply(mmul2, true);
								LAST ^= nr_two;
							}
							multiply(Number(1, 6), true);
							if(!mmul2.isOne()) mterm2 *= mmulsqrt2;
							mterm2 += x_var;
							if(!mmul2.isOne()) mterm2.last() *= mmul2;
							if(!transform_absln(mterm2, use_abs, definite_integral, x_var, eo)) {set(mterm3); CANNOT_INTEGRATE_INTERVAL}
							mterm2 *= madd;
							mterm2.last() ^= nr_three;
							if(!mmul2.isOne()) mterm2 /= mmulsqrt2;
							mterm2 *= Number(5, 16);
							mterm3 *= x_var;
							mterm3 *= madd;
							mterm3.last() ^= nr_two;
							mterm3 *= Number(11, 16);
							mterm4 *= x_var;
							mterm4.last() ^= nr_three;
							mterm4 *= madd;
							if(!mmul2.isOne()) mterm4 *= mmul2;
							mterm4 *= Number(13, 24);
							add(mterm2);
							add(mterm3);
							add(mterm4);
							childrenUpdated(true);
							return true;
						} else if(num.isMinusOne()) {
							// 1/sqrt(ax^2+b): ln(sqrt(ax^2+b)*sqrt(a)+ax)/sqrt(a)
							MathStructure mbak(*this);
							CHILD(1).number().set(1, 2, 0, true);
							if(!mmul2.isOne()) multiply(mmulsqrt2);
							add(x_var);
							if(!mmul2.isOne()) LAST.multiply(mmul2);
							if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
							if(!mmul2.isOne()) divide(mmulsqrt2);
							return true;
						} else if(num == -3) {
							// (ax^2+b)^(-3/2): x/(b*sqrt(ax^2+b))
							CHILD(1).number().set(-1, 2, 0, true);
							madd.inverse();
							multiply(madd);
							multiply(x_var);
							return true;
						} else if(num == -5) {
							// (ax^2+b)^(-5/2): (x(2ax^2+3b)/(3b^2*(ax^2+b)^(3/2))
							CHILD(1).number().set(-3, 2, 0, true);
							mmul = x_var;
							mmul ^= nr_two;
							if(!mmul2.isOne()) mmul *= mmul2;
							mmul *= nr_two;
							mmul += madd;
							mmul.last() *= nr_three;
							multiply(mmul);
							multiply(x_var);
							multiply(madd);
							LAST ^= Number(-2, 1);
							multiply(Number(1, 3));
							return true;
						}
					} else if(b_minusone) {
						// 1/(ax^2+bx+c)
						// y=4ac-b^2
						MathStructure m4acmb2(madd);
						m4acmb2 *= mmul2;
						m4acmb2 *= Number(4, 1);
						m4acmb2 += mmul;
						m4acmb2.last() ^= nr_two;
						m4acmb2.last().negate();
						m4acmb2.calculatesub(eo, eo, true);
						if(!m4acmb2.representsNonZero(true)) {
							if(m4acmb2.isZero()) {
								// y=0: -2/(2ax+b)
								set(x_var, true);
								multiply(mmul2);
								multiply(nr_two, true);
								add(mmul);
								inverse();
								multiply(Number(-2, 1));
								return true;
							} else if(!warn_about_denominators_assumed_nonzero(m4acmb2, eo)) {
								CANNOT_INTEGRATE
							}
						}
						if(m4acmb2.representsNegative(true)) {
							// y<0: ln((2ax+b-sqrt(-4ac+b^2))/(2ax+b+sqrt(-4ac+b^2)))/sqrt(-4ac+b^2)
							MathStructure mbak(*this);
							MathStructure m2axpb(x_var);
							m2axpb *= mmul2;
							m2axpb *= nr_two;
							m2axpb += mmul;
							MathStructure mb2m4ac(madd);
							mb2m4ac *= mmul2;
							mb2m4ac *= Number(-4, 1);
							mb2m4ac += mmul;
							mb2m4ac.last() ^= nr_two;
							mb2m4ac ^= nr_half;
							set(m2axpb);
							subtract(mb2m4ac);
							multiply(m2axpb);
							LAST += mb2m4ac;
							LAST ^= nr_minus_one;
							if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
							divide(mb2m4ac);
							return true;
						}
						// y!=0: 2y*atan((2ax+b)/y^2)
						m4acmb2 ^= Number(-1, 2);
						set(x_var, true);
						multiply(mmul2);
						multiply(nr_two, true);
						add(mmul);
						multiply(m4acmb2);
						transformById(FUNCTION_ID_ATAN);
						multiply(nr_two);
						multiply(m4acmb2, true);
						return true;
					} else if(CHILD(1).isInteger() && CHILD(1).number().isNegative()) {
						// (ax^2+bx+c)^(-n): integral of (ax^2+bx+c)^(-n+1)*(-2a*(-2n+3)*c*a*4+1/(b^(2)*(n-1)))+(2ax+b)*c*a*4+1/b^(2)(ax^2+bx+c)^(-n+1)/(n-1)
						MathStructure m2nm3(CHILD(1));
						m2nm3 *= nr_two;
						m2nm3 += Number(3, 1);
						CHILD(1).number()++;
						MathStructure mnp1(CHILD(1));
						mnp1.number().negate();
						mnp1.number().recip();
						MathStructure mthis(*this);
						if(integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) CANNOT_INTEGRATE_INTERVAL
						MathStructure m4acmb2(madd);
						m4acmb2 *= mmul2;
						m4acmb2 *= Number(4, 1);
						m4acmb2 += mmul;
						m4acmb2.last() ^= nr_two;
						m4acmb2.last().negate();
						m4acmb2.inverse();
						MathStructure mfac1(mmul2);
						mfac1 *= Number(-2, 1);
						mfac1 *= m2nm3;
						mfac1 *= mnp1;
						mfac1 *= m4acmb2;
						multiply(mfac1);
						MathStructure mterm2(x_var);
						mterm2 *= mmul2;
						mterm2 *= nr_two;
						mterm2 += mmul;
						mterm2 *= m4acmb2;
						mterm2 *= mthis;
						mterm2 *= mnp1;
						add(mterm2);
						return true;
					}
				} else if(integrate_info(CHILD(0), x_var, madd, mmul, mexp, false, false)) {
					if(madd.isZero() && !b_minusone) {
						// (ax^b)^n (n != -1): (ax^b)^n*x/(b*n+1)
						MathStructure mden(mexp);
						mden *= CHILD(1);
						mden += m_one;
						multiply(x_var);
						divide(mden);
						return true;
					} else if(b_minusone && mexp.isNumber() && mexp.number().isNegative() && mexp.number().isInteger()) {
						if(!madd.isZero()) {
							// 1/(ax^(-n)+b): integral of 1/(x^n*b+a)*x^n
							Number nexp(mexp.number());
							nexp.negate();
							MathStructure mtest(x_var);
							mtest ^= nexp;
							mtest *= madd;
							mtest += mmul;
							mtest.inverse();
							mtest *= x_var;
							mtest.last() ^= nexp;
							CALCULATOR->beginTemporaryStopMessages();
							if(mtest.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
								CALCULATOR->endTemporaryStopMessages(true);
								set(mtest, true);
								return true;
							}
							CALCULATOR->endTemporaryStopMessages();
						}
					} else if(mexp.isNumber() && mexp.number().isRational() && !mexp.number().isInteger()) {
						// (ax^(n/d)+b)^c:
						// integral of (ay^(n=-1?1:n)+b)^c*(y^(sgn(n)/d))^(sgn(n)*d-1) where y=x^(sgn(n)/d) and divide with sgn(n)*d
						Number num(mexp.number().numerator());
						Number den(mexp.number().denominator());
						MathStructure morig(x_var);
						if(num.isNegative()) den.negate();
						Number den_inv(den);
						den_inv.recip();
						morig ^= den_inv;
						UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(morig) + RIGHT_PARENTHESIS);
						Number den_m1(den);
						den_m1--;
						MathStructure mtest(var);
						if(!num.isOne() && !num.isMinusOne()) mtest ^= num;
						if(!mmul.isOne()) mtest *= mmul;
						if(!madd.isZero()) mtest += madd;
						mtest ^= CHILD(1);
						mtest *= var;
						if(!den_m1.isOne()) {
							mtest.last() ^= den_m1;
							mtest.childUpdated(mtest.size());
						}
						if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
							MathStructure m_interval(morig);
							m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
							var->setInterval(m_interval);
						} else {
							var->setInterval(morig);
						}
						CALCULATOR->beginTemporaryStopMessages();
						if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							set(mtest, true);
							replace(var, morig);
							multiply(den);
							var->destroy();
							return true;
						}
						CALCULATOR->endTemporaryStopMessages();
						var->destroy();
					}
				} else if(integrate_info(CHILD(0), x_var, madd, mmul, mexp, false, false, true) && !madd.isZero()) {
					if(mexp.isFunction()) {
						// (a*f(x)+b)^c
						MathStructure mfunc(mexp);
						int bint = integrate_function(mfunc, x_var, eo, CHILD(1), m_one, madd, mmul, use_abs, definite_integral, max_part_depth, parent_parts);
						if(bint < 0) CANNOT_INTEGRATE_INTERVAL
						if(bint) {
							set(mfunc, true);
							return true;
						}
					} else if(mexp.isPower() && mexp[1].containsRepresentativeOf(x_var, true, true) == 0) {
						MathStructure mbase(mexp[0]);
						if(integrate_info(mbase, x_var, madd, mmul, mexp, false, false) && mexp.isOne() && (!madd.isZero() || !mmul.isOne())) {
							// ((ax^c1+b)^c2+b2)^c3: integral of (y^c2+b^2)^c3 where y=ax^c1+b and divide with a
							MathStructure mtest(*this);

							UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mbase) + RIGHT_PARENTHESIS);
							mtest.replace(mbase, var, mbase.containsInterval());
							if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
								MathStructure m_interval(mbase);
								m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
								var->setInterval(m_interval);
							} else {
								var->setInterval(mbase);
							}
							CALCULATOR->beginTemporaryStopMessages();
							if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
								CALCULATOR->endTemporaryStopMessages(true);
								set(mtest, true);
								replace(var, mbase);
								if(!mmul.isOne()) divide(mmul);
								var->destroy();
								return true;
							}
							CALCULATOR->endTemporaryStopMessages();
							var->destroy();
						}
					}
				}
				if(CHILD(0).isAddition()) {
					MathStructure mtest(*this);
					// factor base
					if(mtest[0].factorize(eo, false, 0, 0, false, false, NULL, x_var)) {
						mmul = m_one;
						while(mtest[0].isMultiplication() && mtest[0].size() >= 2 && mtest[0][0].containsRepresentativeOf(x_var, true, true) == 0) {
							MathStructure *mchild = &mtest[0][0];
							mchild->ref();
							mtest[0].delChild(1, true);
							if(mtest[1].representsInteger(true) || comparison_is_equal_or_less(mchild->compare(m_zero)) || comparison_is_equal_or_less(mtest[0].compare(m_zero))) {
								if(mmul.isOne()) mmul = *mchild;
								else mmul *= *mchild;
								mchild->unref();
							} else {
								mtest[0].multiply_nocopy(mchild, true);
							}
						}
						if(!mmul.isOne()) {
							mmul ^= mtest[1];
						}
						MathStructure mtest2(mtest);
						if(mtest2.decomposeFractions(x_var, eo) && mtest2.isAddition()) {
							bool b = true;
							CALCULATOR->beginTemporaryStopMessages();
							for(size_t i = 0; i < mtest2.size(); i++) {
								if(mtest2[i].integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) <= 0) {
									b = false;
									break;
								}
							}
							CALCULATOR->endTemporaryStopMessages(b);
							if(b) {
								set(mtest2, true);
								if(!mmul.isOne()) multiply(mmul);
								return true;
							}
						}
						MathStructure mmul2(1, 1, 0);
						if(mtest[0].isMultiplication() && mtest[0].size() >= 2 && !mtest[0][0].isAddition()) {
							MathStructure *mchild = &mtest[0][0];
							mchild->ref();
							mtest[0].delChild(1, true);
							if(mtest[1].representsInteger(true) || comparison_is_equal_or_less(mchild->compare(m_zero)) || comparison_is_equal_or_less(mtest[0].compare(m_zero))) {
								mmul2 = *mchild;
								mmul2.calculateRaise(mtest[1], eo2);
								mchild->unref();
							} else {
								mtest[0].multiply_nocopy(mchild, true);
							}
						}
						if(mtest[0].isPower()) {
							if(mtest[1].representsInteger() || mtest[0][1].representsFraction() || comparison_is_equal_or_less(mtest[0][0].compare(m_zero))) {
								mtest[1].calculateMultiply(mtest[0][1], eo2);
								mtest[0].setToChild(1);
							} else if(mtest[0][1].representsEven()) {
								if(comparison_is_equal_or_greater(mtest[0][0].compare(m_zero))) {
									mtest[1].calculateMultiply(mtest[0][1], eo2);
									mtest[0].setToChild(1);
									mtest[0].calculateNegate(eo2);
								} else if(!definite_integral && mtest[0][0].representsReal(true)) {
									mtest[1].calculateMultiply(mtest[0][1], eo2);
									mtest[0].setToChild(1);
									mtest[0].transformById(FUNCTION_ID_ABS);
								}
							}
						}
						mtest[0].evalSort(false);
						if(!mmul2.isOne()) {
							mtest *= mmul2;
							mtest.evalSort(false);
						}
						CALCULATOR->beginTemporaryStopMessages();
						if(mtest.integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							set(mtest, true);
							if(!mmul.isOne()) multiply(mmul);
							return true;
						}
						CALCULATOR->endTemporaryStopMessages();
					}
				}
			} else if((CHILD(0).isNumber() && !CHILD(0).number().isOne()) || (!CHILD(0).isNumber() && CHILD(0).containsRepresentativeOf(x_var, true, true) == 0)) {
				MathStructure madd, mmul, mmul2, mexp;
				if(integrate_info(CHILD(1), x_var, madd, mmul, mexp)) {
					if(mexp.isOne()) {
						if(CHILD(0).isVariable() && CHILD(0).variable()->id() == VARIABLE_ID_E) {
							// e^(ax+b): e^(ax+b)/a
							if(!mmul.isOne()) divide(mmul);
							return true;
						} else if(CHILD(0).isNumber() || warn_about_assumed_not_value(CHILD(0), m_one, eo)) {
							// c^(ax+b): c^(ax+b)/(a*ln(c))
							MathStructure mmulfac(CHILD(0));
							mmulfac.transformById(FUNCTION_ID_LOG);
							if(!mmul.isOne()) mmulfac *= mmul;
							divide(mmulfac);
							return true;
						}
					} else if(madd.isZero() || comparison_is_not_equal(madd.compare(m_zero))) {
						if(mmul.representsPositive(true) && comparison_is_equal_or_greater(CHILD(0).compare(m_one)) && CHILD(0).compare(m_zero) == COMPARISON_RESULT_LESS) {
							madd.negate();
							mmul.negate();
							CHILD(0).inverse();
						}
						if(mexp == nr_two) {
							if(!madd.isZero()) {madd ^= CHILD(0); madd.swapChildren(1, 2);}
							bool mmul_neg = mmul.representsNegative(true);
							if(mmul_neg) mmul.negate();
							if(CHILD(0).isVariable() && CHILD(0).variable()->id() == VARIABLE_ID_E) {
								set(mmul);
								if(!mmul.isOne()) raise(nr_half);
								multiply(x_var);
								if(!mmul_neg) multiply(nr_one_i);
								transformById(FUNCTION_ID_ERF);
								if(!mmul.isOne()) {
									mmul ^= Number(-1, 2);
									multiply(mmul);
								}
								multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
								LAST ^= nr_half;
								multiply(nr_half);
								if(!mmul_neg) multiply(nr_minus_i);
							} else {
								MathStructure mlog(CHILD(0));
								mlog.transformById(FUNCTION_ID_LOG);
								mlog ^= nr_half;
								set(mmul);
								if(!mmul.isOne()) raise(nr_half);
								multiply(x_var);
								multiply(mlog);
								if(!mmul_neg) multiply(nr_one_i);
								transformById(FUNCTION_ID_ERF);
								if(!mmul.isOne()) {
									mmul ^= Number(-1, 2);
									multiply(mmul);
								}
								multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
								LAST ^= nr_half;
								multiply(nr_half);
								if(!mmul_neg) multiply(nr_minus_i);
								divide(mlog);
							}
							if(!madd.isZero()) multiply(madd);
							return true;
						} else if(mexp.isNumber() && mexp.number().isRational() && !mexp.isInteger()) {
							Number num(mexp.number().numerator());
							Number den(mexp.number().denominator());
							MathStructure morig(x_var);
							Number den_inv(den);
							den_inv.recip();
							morig ^= den_inv;

							UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(morig) + RIGHT_PARENTHESIS);
							MathStructure mvar(var);
							if(!num.isOne()) mvar ^= num;
							MathStructure mtest(CHILD(0));
							mtest ^= mvar;
							if(!mmul.isOne()) mtest.last() *= mmul;
							if(!madd.isZero()) mtest.last() += madd;
							mtest.childUpdated(mtest.size());
							Number npow(den);
							npow--;
							mtest *= var;
							if(!npow.isOne()) mtest.last() ^= npow;
							if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
								MathStructure m_interval(morig);
								m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
								var->setInterval(m_interval);
							} else {
								var->setInterval(morig);
							}
							CALCULATOR->beginTemporaryStopMessages();
							if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
								CALCULATOR->endTemporaryStopMessages(true);
								set(mtest, true);
								replace(var, morig);
								multiply(den);
								var->destroy();
								return true;
							}
							CALCULATOR->endTemporaryStopMessages();
							var->destroy();
						}
						// a^(b+cx^d): -x*a^b*(-c*x^d*ln(a))^(-1/d)*igamma(1/d, -c*x^d*ln(a))/d
						if(definite_integral) CANNOT_INTEGRATE
						if(!madd.isZero()) {madd ^= CHILD(0); madd.swapChildren(1, 2);}
						MathStructure mexpinv(mexp);
						mexpinv.inverse();
						MathStructure marg2(CHILD(0));
						marg2.transformById(FUNCTION_ID_LOG);
						marg2 *= x_var;
						marg2.last() ^= mexp;
						marg2 *= mmul;
						marg2.negate();
						set(CALCULATOR->getFunctionById(FUNCTION_ID_I_GAMMA), &mexpinv, &marg2, NULL);
						if(!madd.isZero()) multiply(madd);
						multiply(x_var);
						multiply(mexpinv);
						mexpinv.negate();
						multiply(marg2);
						LAST ^= mexpinv;
						negate();
						return true;
					}
				} else if(integrate_info(CHILD(1), x_var, madd, mmul, mmul2, false, true)) {
					// a^(bx-cx^2+d): sqrt(pi)*a^(b^2/(4c)+d)*erf(sqrt(ln(a))*(2cx-b)/(2*sqrt(c)))/(2*sqrt(c)*sqrt(ln(a))
					MathStructure sqrt2(1, 1, 0);
					if(mmul2.isMinusOne()) {
						mmul2 = m_one;
					} else {
						mmul2.negate();
						sqrt2 = mmul2;
						sqrt2 ^= Number(-1, 2);
					}
					MathStructure loga(1, 1, 0);
					if(CHILD(0) != CALCULATOR->getVariableById(VARIABLE_ID_E)) {loga = CHILD(0); loga.transformById(FUNCTION_ID_LOG);}
					loga ^= nr_half;
					MathStructure merf(x_var);
					if(!mmul2.isOne()) merf *= mmul2;
					merf *= nr_two;
					merf -= mmul;
					if(!loga.isOne()) merf *= loga;
					merf *= nr_half;
					if(!sqrt2.isOne()) merf *= sqrt2;
					merf.transformById(FUNCTION_ID_ERF);
					MathStructure mapow(mmul);
					if(!mapow.isOne()) mapow ^= nr_two;
					mapow *= Number(1, 4);
					if(!mmul2.isOne()) mapow /= mmul2;
					if(!madd.isZero()) mapow += madd;
					SET_CHILD_MAP(0)
					raise(mapow);
					multiply(merf);
					multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
					LAST ^= nr_half;
					if(!sqrt2.isOne()) multiply(sqrt2);
					if(!loga.isOne()) divide(loga);
					multiply(nr_half);
					return true;
				}
			}
			CANNOT_INTEGRATE
		}
		case STRUCT_FUNCTION: {
			MathStructure mfunc(*this);
			int bint = integrate_function(mfunc, x_var, eo, m_one, m_one, m_zero, m_one, use_abs, definite_integral, max_part_depth, parent_parts);
			if(bint < 0) CANNOT_INTEGRATE_INTERVAL
			if(!bint) CANNOT_INTEGRATE
			set(mfunc, true);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(combine_powers(*this, x_var, eo2, eo2) && m_type != STRUCT_MULTIPLICATION) return integrate(x_var, eo, false, use_abs, definite_integral, try_abs, max_part_depth, parent_parts);
			if(SIZE == 2) {
				if(CHILD(0) != x_var && (CHILD(1) == x_var || (CHILD(1).isPower() && CHILD(1)[0] == x_var))) SWAP_CHILDREN(0, 1);
				if(CHILD(1).isPower() && CHILD(1)[1].containsRepresentativeOf(x_var, true, true) == 0) {
					MathStructure madd, mmul, mmul2;
					MathStructure mexp(CHILD(1)[1]);
					if(integrate_info(CHILD(1)[0], x_var, madd, mmul, mmul2, false, true)) {
						if(mmul2.isZero() && mexp.isNumber()) {
							if(CHILD(0) == x_var) {
								MathStructure mbak(*this);
								SET_CHILD_MAP(1)
								SET_CHILD_MAP(0)
								if(mexp.number().isMinusOne()) {
									MathStructure mterm(x_var);
									if(!mmul.isOne()) mterm /= mmul;
									if(!madd.isZero()) {
										if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										multiply(madd);
										if(!mmul.isOne()) {
											MathStructure a2(mmul);
											a2 ^= nr_two;
											divide(a2);
										}
										negate();
										add(mterm);
									} else {
										set(mterm, true);
									}
								} else if(mexp.number() == -2) {
									MathStructure mterm(*this);
									if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
									MathStructure a2(mmul);
									if(!mmul.isOne()) {
										a2 ^= nr_two;
										divide(a2);
									}
									if(!madd.isZero()) {
										if(!mmul.isOne()) mterm *= a2;
										mterm.inverse();
										mterm *= madd;
									}
									add(mterm);
								} else if(mexp.number().isNegative()) {
									MathStructure onemn(1, 1, 0);
									onemn += mexp;
									MathStructure nm1(mexp);
									nm1.negate();
									MathStructure nm2(nm1);
									nm1 += nr_minus_one;
									nm2 += Number(-2, 1);
									raise(nm1);
									multiply(nm2);
									multiply(nm1);
									if(!mmul.isOne()) {
										MathStructure a2(mmul);
										a2 ^= nr_two;
										multiply(a2);
									}
									inverse();
									MathStructure mnum(x_var);
									mnum *= onemn;
									if(!mmul.isOne()) mnum *= mmul;
									if(!madd.isZero()) mnum -= madd;
									multiply(mnum);
								} else {
									MathStructure nm1(mexp);
									nm1 += m_one;
									raise(nm1);
									MathStructure mnum(x_var);
									mnum *= nm1;
									if(!mmul.isOne()) mnum *= mmul;
									mnum -= madd;
									MathStructure mden(mexp);
									mden += nr_two;
									mden *= nm1;
									if(!mmul.isOne()) {
										mden *= mmul;
										mden.last() ^= nr_two;
									}
									multiply(mnum);
									divide(mden);
									return true;
								}
								return true;
							} else if(CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1] == nr_two) {
								MathStructure mbak(*this);
								SET_CHILD_MAP(1)
								SET_CHILD_MAP(0)
								if(mexp.number().isMinusOne()) {
									MathStructure mterm(x_var);
									mterm ^= nr_two;
									if(!mmul.isOne()) mterm *= mmul;
									if(!madd.isZero()) {
										MathStructure mtwobx(-2 ,1, 0);
										mtwobx *= madd;
										mtwobx *= x_var;
										mterm += mtwobx;
									}
									if(!mmul.isOne()) {
										MathStructure a2(mmul);
										a2 ^= nr_two;
										a2 *= nr_two;
										mterm /= a2;
									} else {
										mterm /= nr_two;
									}
									if(!madd.isZero()) {
										if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										MathStructure b2(madd);
										b2 ^= nr_two;
										multiply(b2);
										if(!mmul.isOne()) {
											MathStructure a3(mmul);
											a3 ^= nr_three;
											divide(a3);
										}
										add(mterm);
									} else {
										set(mterm, true);
									}
								} else if(mexp.number() == -2) {
									MathStructure mterm1(x_var);
									if(!mmul.isOne()) mterm1 *= mmul;
									if(!madd.isZero()) {
										MathStructure mterm2(*this);
										if(!transform_absln(mterm2, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										mterm2 *= madd;
										mterm2 *= -2;
										MathStructure mterm3(*this);
										mterm3.inverse();
										madd ^= nr_two;
										mterm3 *= madd;
										mterm3.negate();
										set(mterm1);
										add(mterm2);
										add(mterm3);
									} else {
										set(mterm1);
									}
									if(!mmul.isOne()) {
										MathStructure a3(mmul);
										a3 ^= nr_three;
										divide(a3);
									}
								} else if(mexp.number() == -3) {
									if(!madd.isZero()) {
										MathStructure mterm2(*this);
										mterm2.inverse();
										mterm2 *= madd;
										mterm2 *= nr_two;
										MathStructure mterm3(*this);
										mterm3 ^= nr_two;
										mterm3 *= nr_two;
										mterm3.inverse();
										madd ^= nr_two;
										mterm3 *= madd;
										mterm3.negate();
										if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										add(mterm2);
										add(mterm3);
									} else {
										if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
									}
									if(!mmul.isOne()) {
										MathStructure a3(mmul);
										a3 ^= nr_three;
										divide(a3);
									}
								} else {
									MathStructure mterm2(*this);
									MathStructure mterm3(*this);
									raise(nr_three);
									CHILD(1) += mexp;
									MathStructure mden(mexp);
									mden += nr_three;
									divide(mden);
									if(!madd.isZero()) {
										mterm2 ^= nr_two;
										mterm2[1] += mexp;
										mterm2 *= madd;
										mterm2 *= -2;
										mden = mexp;
										mden += nr_two;
										mterm2 /= mden;
										mterm3 ^= m_one;
										mterm3[1] += mexp;
										madd ^= nr_two;
										mterm3 *= madd;
										mden = mexp;
										mden += m_one;
										mterm3 /= mden;
										add(mterm2);
										add(mterm3);
									}
									if(!mmul.isOne()) {
										MathStructure a3(mmul);
										a3 ^= nr_three;
										divide(a3);
									}
								}
								return true;
							} else if(CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1] == nr_minus_one && !madd.isZero()) {
								if(mexp.number().isMinusOne()) {
									MathStructure mbak(*this);
									SET_CHILD_MAP(1)
									SET_CHILD_MAP(0)
									divide(x_var);
									if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
									divide(madd);
									negate();
									return true;
								}
							} else if(CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1] == -2 && !madd.isZero()) {
								if(mexp.number().isMinusOne()) {
									MathStructure mbak(*this);
									SET_CHILD_MAP(1)
									SET_CHILD_MAP(0)
									divide(x_var);
									if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
									MathStructure madd2(madd);
									madd2 ^= nr_two;
									divide(madd2);
									if(!mmul.isOne()) multiply(mmul);
									madd *= x_var;
									madd.inverse();
									subtract(madd);
									return true;
								} else if(mexp.number() == -2) {
									MathStructure mbak(*this);
									SET_CHILD_MAP(1)
									SET_CHILD_MAP(0)
									MathStructure mterm2(madd);
									mterm2 ^= nr_two;
									MathStructure mterm3(mterm2);
									mterm2 *= *this;
									mterm2.inverse();
									divide(x_var);
									if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
									MathStructure madd3(madd);
									madd3 ^= nr_three;
									divide(madd3);
									multiply(Number(-2, 1, 0));
									mterm3 *= x_var;
									if(!mmul.isOne()) mterm3 *= mmul;
									mterm3.inverse();
									add(mterm2);
									add(mterm3);
									mmul.negate();
									multiply(mmul);
									return true;
								}
							}
						} else if(mmul.isZero() && !madd.isZero() && mexp.isNumber() && mexp.number().denominatorIsTwo()) {
							Number num(mexp.number());
							if(CHILD(0) == x_var) {
								num.add(1);
								SET_CHILD_MAP(1)
								SET_CHILD_MAP(0)
								raise(num);
								num.multiply(2);
								num.recip();
								multiply(num);
								if(!mmul2.isOne()) divide(mmul2);
								return true;
							} else if(CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isInteger()) {
								if(CHILD(0)[1].number() == 2) {
									if(num == nr_half) {
										MathStructure mbak(*this);
										SET_CHILD_MAP(1)
										MathStructure mterm2(*this);
										if(!mmul2.isOne()) {
											mterm2 *= mmul2;
											mterm2.last() ^= nr_half;
										}
										mterm2 += x_var;
										if(!mmul2.isOne()) mterm2.last() *= mmul2;
										if(!transform_absln(mterm2, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										mterm2.multiply(madd);
										mterm2.last() ^= nr_two;
										multiply(x_var);
										if(!mmul2.isOne()) {
											multiply(mmul2);
											LAST ^= nr_half;
										}
										multiply(x_var);
										LAST ^= nr_two;
										if(!mmul2.isOne()) LAST *= mmul2;
										LAST *= nr_two;
										LAST += madd;
										subtract(mterm2);
										multiply(Number(1, 8));
										if(!mmul2.isOne()) {
											multiply(mmul2);
											LAST ^= Number(-3, 2);
										}
										return true;
									}
								} else if(CHILD(0)[1].number() == -1) {
									if(num == nr_minus_half) {
										MathStructure mbak(*this);
										SET_CHILD_MAP(1)
										SET_CHILD_MAP(0)
										raise(nr_half);
										multiply(madd);
										LAST ^= nr_half;
										add(madd);
										transformById(FUNCTION_ID_LOG);
										negate();
										add(x_var);
										if(!transform_absln(LAST, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										multiply(madd);
										LAST ^= nr_minus_half;
										return true;
									} else if(num == nr_half) {
										MathStructure mbak(*this);
										SET_CHILD_MAP(1)
										MathStructure mterm2(*this);
										mterm2 *= madd;
										mterm2.last() ^= nr_half;
										mterm2 += madd;
										mterm2.transformById(FUNCTION_ID_LOG);
										mterm2 *= madd;
										mterm2.last() ^= nr_half;
										MathStructure mterm3(x_var);
										if(!transform_absln(mterm3, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
										mterm3 *= madd;
										mterm3.last() ^= nr_half;
										subtract(mterm2);
										add(mterm3);
										return true;
									}
								}
							}
						} else if(mmul2.isZero()) {
							if(CHILD(0) == x_var) {
								SET_CHILD_MAP(1)
								CHILD(1) += m_one;
								MathStructure mnum(x_var);
								mnum *= CHILD(1);
								if(!mmul.isOne()) mnum *= mmul;
								mnum -= madd;
								MathStructure mden(CHILD(1)[0]);
								mden += nr_two;
								mden *= CHILD(1);
								if(!mmul.isOne()) {
									mden *= mmul;
									mden[mden.size() - 1] ^= nr_two;
								}
								multiply(mnum);
								divide(mden);
								return true;
							}
						} else if(mexp.isMinusOne() && CHILD(0) == x_var) {
							MathStructure mbak(*this);
							SET_CHILD_MAP(1)
							MathStructure mterm2(CHILD(0));
							if(integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) CANNOT_INTEGRATE_INTERVAL
							multiply(mmul);
							divide(mmul2);
							multiply(Number(-1, 2));
							if(!transform_absln(mterm2, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
							mterm2 /= mmul2;
							mterm2 *= nr_half;
							add(mterm2);
							return true;
						} else if(mexp.isMinusOne() && CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1].isMinusOne()) {
							MathStructure mbak(*this);
							SET_CHILD_MAP(1)
							MathStructure mterm2(CHILD(0));
							if(integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) CANNOT_INTEGRATE_INTERVAL
							multiply(mmul);
							divide(madd);
							multiply(Number(-1, 2));
							mterm2.inverse();
							mterm2 *= x_var;
							mterm2.last() ^= nr_two;
							if(!transform_absln(mterm2, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
							mterm2 /= madd;
							mterm2 *= nr_half;
							add(mterm2);
							return true;
						} else if(mexp.isInteger() && mexp.number().isNegative() && CHILD(0) == x_var) {
							MathStructure mbak(*this);
							SET_CHILD_MAP(1)
							MathStructure m2nm3(CHILD(1));
							m2nm3 *= nr_two;
							m2nm3 += Number(3, 1);
							CHILD(1).number()++;
							MathStructure mnp1(CHILD(1));
							mnp1.number().negate();
							mnp1.number().recip();
							MathStructure mthis(*this);
							if(integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
							MathStructure m4acmb2(madd);
							m4acmb2 *= mmul2;
							m4acmb2 *= Number(4, 1);
							m4acmb2 += mmul;
							m4acmb2.last() ^= nr_two;
							m4acmb2.last().negate();
							m4acmb2.inverse();
							MathStructure mfac1(mmul);
							mfac1 *= m2nm3;
							mfac1 *= mnp1;
							mfac1 *= m4acmb2;
							multiply(mfac1);
							MathStructure mterm2(x_var);
							mterm2 *= mmul;
							mterm2 += madd;
							mterm2.last() *= nr_two;
							mterm2 *= m4acmb2;
							mterm2 *= mthis;
							mterm2 *= mnp1;
							subtract(mterm2);
							return true;
						}
					}

					if(CHILD(0) == x_var || (CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isRational())) {
						Number nexp(1, 1, 0);
						if(CHILD(0).isPower()) nexp = CHILD(0)[1].number();
						MathStructure madd, mmul, mpow;
						if(integrate_info(CHILD(1)[0], x_var, madd, mmul, mpow, false, false, true)) {
							if(mpow.isPower() && mpow[0] == x_var) {
								mpow.setToChild(2);
								if(mpow.isNumber() && mpow.number().isRational() && !mpow.number().isOne()) {
									if(!nexp.isOne() && mpow.isInteger()) {
										if(mexp.isMinusOne() && nexp.isMinusOne()) {
											MathStructure mbak(*this);
											if(mpow.number().isNegative()) {
												set(x_var, true);
												mpow.number().negate();
												raise(mpow);
												if(!madd.isOne()) multiply(madd);
												add(mmul);
												if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
												divide(mpow);
												if(!madd.isOne()) divide(madd);
											} else {
												SET_CHILD_MAP(1)
												SET_CHILD_MAP(0)
												if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
												add(x_var);
												if(!transform_absln(LAST, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
												LAST *= mpow;
												LAST.negate();
												negate();
												divide(mpow);
												if(!madd.isOne()) divide(madd);
											}
											return true;
										}
										Number mpowmexp(mpow.number());
										mpowmexp -= nexp;
										if(mpowmexp.isOne()) {
											if(mexp.isMinusOne()) {
												MathStructure mbak(*this);
												if(mpow.number().isNegative()) {
													set(x_var, true);
													mpow.number().negate();
													raise(mpow);
													if(!madd.isOne()) multiply(madd);
													add(mmul);
													if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
													divide(mpow);
													divide(madd);
													add(x_var);
													if(!transform_absln(LAST, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
													LAST.negate();
													divide(mpow);
													divide(madd);
													negate();
												} else {
													SET_CHILD_MAP(1)
													SET_CHILD_MAP(0)
													if(!transform_absln(*this, use_abs, definite_integral, x_var, eo)) {set(mbak); CANNOT_INTEGRATE_INTERVAL}
													divide(mpow);
													if(!mmul.isOne()) divide(mmul);
												}
											} else {
												SET_CHILD_MAP(1)
												MathStructure mden(CHILD(1));
												CHILD(1) += m_one;
												mden *= mpow;
												if(!mmul.isOne()) mden *= mmul;
												mden += mpow;
												if(!mmul.isOne()) mden.last() *= mmul;
												divide(mden);
											}
											return true;
										}
									}
									if(x_var.representsNonNegative(true) || mpow.number().isEven() || mpow.number().isFraction()) {
										for(int i = 1; i <= 3; i++) {
											if(i > 1 && (!mpow.isInteger() || !mpow.number().isIntegerDivisible(i) || mpow.number() == i)) break;
											UnknownVariable *var = NULL;
											MathStructure m_replace(x_var);
											MathStructure mtest(CHILD(1));
											Number new_pow(nexp);
											bool b = false;
											if(i == 1) {
												m_replace ^= mpow;
												var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(m_replace) + RIGHT_PARENTHESIS);
												mtest.replace(m_replace, var);
												new_pow++;
												new_pow -= mpow.number();
												new_pow /= mpow.number();
												b = true;
											} else if(i == 2) {
												new_pow = nexp;
												new_pow++;
												new_pow -= 2;
												new_pow /= 2;
												if(new_pow.isInteger()) {
													b = true;
													m_replace ^= nr_two;
													var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(m_replace) + RIGHT_PARENTHESIS);
													MathStructure m_prev(x_var), m_new(var);
													m_prev ^= mpow;
													m_new ^= mpow;
													m_new[1].number() /= 2;
													mtest.replace(m_prev, m_new);
												}
											} else if(i == 3) {
												new_pow++;
												new_pow -= 3;
												new_pow /= 3;
												if(new_pow.isInteger()) {
													b = true;
													m_replace ^= nr_three;
													var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(m_replace) + RIGHT_PARENTHESIS);
													MathStructure m_prev(x_var), m_new(var);
													m_prev ^= mpow;
													m_new ^= mpow;
													m_new[1].number() /= 3;
													mtest.replace(m_prev, m_new);
												}
											}
											if(b) {
												if(!new_pow.isZero()) {
													mtest *= var;
													mtest.swapChildren(1, mtest.size());
													if(!new_pow.isOne()) mtest[0] ^= new_pow;
												}
												CALCULATOR->beginTemporaryStopMessages();
												if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
													MathStructure m_interval(m_replace);
													m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
													var->setInterval(m_interval);
												} else {
													var->setInterval(m_replace);
												}
												if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0) {
													CALCULATOR->endTemporaryStopMessages(true);
													mtest.replace(var, m_replace);
													set(mtest, true);
													divide(m_replace[1]);
													var->destroy();
													return true;
												}
												CALCULATOR->endTemporaryStopMessages();
												var->destroy();
											}
										}
									}
								}
							} else if(mpow.isPower() && nexp.isInteger() && !madd.isZero() && mpow[1].containsRepresentativeOf(x_var, true, true) == 0) {
								MathStructure mbase(mpow[0]);
								if(integrate_info(mbase, x_var, madd, mmul, mpow, false, false) && mpow.isOne() && (!madd.isZero() || !mmul.isOne())) {
									MathStructure mtest(CHILD(1));

									UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mbase) + RIGHT_PARENTHESIS);
									mtest.replace(mbase, var, mbase.containsInterval());
									if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
										MathStructure m_interval(mbase);
										m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
										var->setInterval(m_interval);
									} else {
										var->setInterval(mbase);
									}
									mtest *= var;
									if(!madd.isZero()) {
										mtest.last() -= madd;
										mtest.childUpdated(mtest.size());
									}
									if(!nexp.isOne()) {
										mtest.last() ^= nexp;
										mtest.childUpdated(mtest.size());
									}
									CALCULATOR->beginTemporaryStopMessages();
									if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
										CALCULATOR->endTemporaryStopMessages(true);
										set(mtest, true);
										replace(var, mbase);
										if(!mmul.isOne()) {
											nexp++;
											if(!nexp.isOne()) mmul ^= nexp;
											divide(mmul);
										}
										var->destroy();
										return true;
									}
									CALCULATOR->endTemporaryStopMessages();
									var->destroy();
								}
							} /*else if(mpow.isFunction() && mpow.function()->id() == FUNCTION_ID_LOG && mpow.size() == 1 && mpow[0] == x_var) {
								MathStructure mtest(CHILD(1));
								UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mpow) + RIGHT_PARENTHESIS);
								mtest.replace(mpow, var, mpow.containsInterval());
								if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
									MathStructure m_interval(mpow);
									m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
									var->setInterval(m_interval);
								} else {
									var->setInterval(mpow);
								}
								nexp++;
								mtest *= CALCULATOR->getVariableById(VARIABLE_ID_E);
								mtest.last() ^= nexp;
								mtest.last().last() *= var;
								mmul = CALCULATOR->getVariableById(VARIABLE_ID_E);
								mmul ^= nexp;
								mmul.last() *= Number(-2, 1);
								CALCULATOR->beginTemporaryStopMessages();
								if(mtest.integrate(var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
									CALCULATOR->endTemporaryStopMessages(true);
									set(mtest, true);
									replace(var, mpow);
									multiply(mmul);
									var->destroy();
									return true;
								}
								CALCULATOR->endTemporaryStopMessages();
								var->destroy();
							}*/
						}
					}
					if(CHILD(0).isPower() && (CHILD(0)[1] == x_var || (CHILD(0)[1].isMultiplication() && CHILD(0)[1].size() >= 2 && CHILD(0)[1].last() == x_var)) && CHILD(0)[0].containsRepresentativeOf(x_var, true, true) == 0) {
						MathStructure mpow, madd, mmul;
						if(integrate_info(CHILD(1)[0], x_var, madd, mmul, mpow, false, false, true) && mpow.isPower() && (mpow[1] == x_var || (mpow[1].isMultiplication() && mpow[1].size() >= 2 && mpow[1].last() == x_var)) && (CHILD(0)[0] == mpow[0] || (CHILD(0)[0].isNumber() && CHILD(0)[0].number().isRational() && mpow[0].isNumber() && mpow[0].number().isRational() && mpow[1] == CHILD(0)[1]))) {
							Number pow1(1, 1), pow2;
							MathStructure morig(mpow);
							bool b  = true;
							if(CHILD(0)[1].isMultiplication()) {
								if(!mpow[1].isMultiplication()) {
									if(CHILD(0)[1].size() != 2 || !CHILD(0)[1][0].isNumber()) b = false;
									else {pow2 = CHILD(0)[1][0].number(); pow2--;}
								} else {
									for(size_t i = 0; i < mpow[1].size() - 1; i++) {
										if(i == 0 && mpow[1][i].isNumber()) {
											pow1 = mpow[1][i].number();
										} else if(CHILD(0)[1][i].containsRepresentativeOf(x_var, true, true) != 0) {
											b = false;
											break;
										}
									}
									if(b) {
										if(mpow[1] == CHILD(0)[1]) {
											pow1.set(1, 1);
										} else if(CHILD(0)[1].size() - (CHILD(0)[1][0].isNumber() ? 1 : 0) != mpow[1].size() - (mpow[1][0].isNumber() ? 1 : 0)) {
											b = false;
										} else if(b) {
											for(size_t i = 0; i < CHILD(0)[1].size() - 1; i++) {
												if(i == 0 && CHILD(0)[1][i].isNumber()) {
													pow2 = CHILD(0)[1][i].number();
													pow2--;
												} else if(CHILD(0)[1][i] != mpow[1][i + (mpow[1][0].isNumber() ? 1 : 0) - (CHILD(0)[1][0].isNumber() ? 1 : 0)]) {
													b = false;
													break;
												}
											}
										}
									}
								}
							} else if(mpow[1].isMultiplication()) {
								if(mpow[1].size() != 2 || !mpow[1][0].isNumber()) b = false;
								else pow1 = mpow[1][0].number();
							}
							if(b && !pow1.isOne()) morig[1].delChild(1, true);
							if(b && CHILD(0)[0] != mpow[0]) {
								bool b1 = CHILD(0)[0].number() < mpow[0].number();
								if(mpow[0].number().isFraction() || CHILD(0)[0].number().isFraction()) b1 = !b1;
								Number nlog(b1 ? mpow[0].number() : CHILD(0)[0].number());
								nlog.log(b1 ? CHILD(0)[0].number() : mpow[0].number());
								if(!nlog.isInteger()) {
									nlog.round();
									b = nlog.isInteger() && (b1 ? ((CHILD(0)[0].number() ^ nlog) == mpow[0].number()) : ((mpow[0].number() ^ nlog) == CHILD(0)[0].number()));
								}
								if(b) {
									if(b1) {
										pow1 = nlog;
										morig = CHILD(0);
									} else {
										pow2 = nlog;
										pow2--;
									}
								}
							}
							if(b) {
								UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(morig) + RIGHT_PARENTHESIS);
								MathStructure mtest(var);
								if(!pow1.isOne()) mtest ^= pow1;
								if(!mmul.isOne()) mtest *= mmul;
								if(!madd.isZero()) mtest += madd;
								if(!mexp.isOne()) mtest ^= mexp;
								if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
									MathStructure m_interval(morig);
									m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
									var->setInterval(m_interval);
								} else {
									var->setInterval(morig);
								}
								if(!pow2.isZero()) {
									mtest *= var;
									if(!pow2.isOne()) mtest.last() ^= pow2;
									mtest.swapChildren(1, 2);
								}
								CALCULATOR->beginTemporaryStopMessages();
								if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
									CALCULATOR->endTemporaryStopMessages(true);
									set(mtest, true);
									replace(var, morig);
									if(!morig[0].isVariable() || morig[0].variable()->id() != VARIABLE_ID_E) divide_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &morig[0], NULL));
									if(morig[1].isMultiplication()) {
										morig[1].delChild(morig[1].size(), true);
										divide(morig[1]);
									}
									var->destroy();
									return true;
								}
								CALCULATOR->endTemporaryStopMessages();
								var->destroy();
							}
						}
					}
					if(CHILD(1)[1].number().isMinusOne() && CHILD(1)[0].isMultiplication() && CHILD(1)[0].size() == 2 && CHILD(1)[0][0].isAddition() && CHILD(1)[0][1].isAddition() && (CHILD(0) == x_var || (CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()))) {
						MathStructure madd1, mmul1, mexp1;
						if(integrate_info(CHILD(1)[0][0], x_var, madd1, mmul1, mexp1) && mexp1.isOne()) {
							MathStructure madd2, mexp2;
							if(integrate_info(CHILD(1)[0][1], x_var, madd2, mmul2, mexp2) && mexp2.isOne()) {
								MathStructure mnum1, mnum2, mx1(madd2), mx2(madd1);
								mx1.negate();
								if(!mmul2.isOne()) mx1 /= mmul2;
								mnum1 = mx1;
								if(!mmul1.isOne()) mx1 *= mmul1;
								mx1 += madd1;
								mnum1 /= mx1;
								mx2.negate();
								if(!mmul1.isOne()) mx2 /= mmul1;
								mnum2 = mx2;
								if(!mmul2.isOne()) mx2 *= mmul2;
								mx2 += madd2;
								mnum2 /= mx2;
								mnum2 /= CHILD(1)[0][0];
								mnum1 /= CHILD(1)[0][1];
								if(CHILD(0) != x_var) {
									mnum1 *= x_var;
									mnum2 *= x_var;
								}
								mnum2 += mnum1;
								CALCULATOR->beginTemporaryStopMessages();
								if(mnum2.integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0) {
									CALCULATOR->endTemporaryStopMessages(true);
									set(mnum2, true);
									return true;
								}
								CALCULATOR->endTemporaryStopMessages();
							}
						}
					}
					if(CHILD(1)[0].isAddition()) {
						bool b_poly = false;
						if(CHILD(1)[1].isMinusOne()) {
							MathStructure mquo, mrem;
							b_poly = polynomial_long_division(CHILD(0), CHILD(1)[0], x_var, mquo, mrem, eo, true);
							if(b_poly && !mquo.isZero()) {
								MathStructure mtest(mquo);
								if(!mrem.isZero()) {
									mtest += mrem;
									mtest.last() *= CHILD(1);
									mtest.childrenUpdated();
								}
								CALCULATOR->beginTemporaryStopMessages();
								if(mtest.integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0) {
									CALCULATOR->endTemporaryStopMessages(true);
									set(mtest, true);
									return true;
								}
								CALCULATOR->endTemporaryStopMessages();
							}
						}
						MathStructure mtest(*this);
						if(mtest[1][0].factorize(eo, false, 0, 0, false, false, NULL, x_var)) {
							mmul = m_one;
							while(mtest[1][0].isMultiplication() && mtest[1][0].size() >= 2 && mtest[1][0][0].containsRepresentativeOf(x_var, true, true) == 0) {
								MathStructure *mchild = &mtest[1][0][0];
								mchild->ref();
								mtest[1][0].delChild(1, true);
								if(CHILD(1)[1].representsInteger(true) || comparison_is_equal_or_less(mchild->compare(m_zero)) || comparison_is_equal_or_less(mtest[1][0].compare(m_zero))) {
									if(mmul.isOne()) mmul = *mchild;
									else mmul *= *mchild;
									mchild->unref();
								} else {
									mtest[1][0].multiply_nocopy(mchild, true);
								}
							}
							if(!mmul.isOne()) {
								mmul ^= CHILD(1)[1];
							}
							if(b_poly) {
								MathStructure mtest2(mtest);
								if(mtest2.decomposeFractions(x_var, eo)) {
									if(mtest2.isAddition()) {
										bool b = true;
										CALCULATOR->beginTemporaryStopMessages();
										for(size_t i = 0; i < mtest2.size(); i++) {
											if(mtest2[i].integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) <= 0) {
												b = false;
												break;
											}
										}
										CALCULATOR->endTemporaryStopMessages(b);
										if(b) {
											set(mtest2, true);
											if(!mmul.isOne()) multiply(mmul);
											return true;
										}
									}
								} else {
									mtest2 = mtest[1];
									if(mtest2.decomposeFractions(x_var, eo) && mtest2.isAddition()) {
										if(mtest2.isAddition()) {
											bool b = true;
											CALCULATOR->beginTemporaryStopMessages();
											for(size_t i = 0; i < mtest2.size(); i++) {
												mtest2[i] *= mtest[0];
												if(mtest2[i].integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) <= 0) {
													b = false;
													break;
												}
											}
											CALCULATOR->endTemporaryStopMessages(b);
											if(b) {
												set(mtest2, true);
												if(!mmul.isOne()) multiply(mmul);
												return true;
											}
										}
									}
								}
							}
							mmul2 = m_one;
							if(mtest[1][0].isMultiplication() && mtest[1][0].size() >= 2 && !mtest[1][0][0].isAddition()) {
								MathStructure *mchild = &mtest[1][0][0];
								mchild->ref();
								mtest[1][0].delChild(1, true);
								if(CHILD(1)[1].representsInteger(true) || comparison_is_equal_or_less(mchild->compare(m_zero)) || comparison_is_equal_or_less(mtest[1][0].compare(m_zero))) {
									mmul2 = *mchild;
									mmul2.calculateRaise(CHILD(1)[1], eo2);
									mchild->unref();
								} else {
									mtest[1][0].multiply_nocopy(mchild, true);
								}
							}
							if(mtest[1][0].isPower()) {
								if(mtest[1][1].representsInteger() || mtest[1][0][1].representsFraction() || comparison_is_equal_or_less(mtest[1][0][0].compare(m_zero))) {
									mtest[1][1].calculateMultiply(mtest[1][0][1], eo2);
									mtest[1][0].setToChild(1);
								} else if(mtest[1][0][1].representsEven()) {
									if(comparison_is_equal_or_greater(mtest[1][0][0].compare(m_zero))) {
										mtest[1][1].calculateMultiply(mtest[1][0][1], eo2);
										mtest[1][0].setToChild(1);
										mtest[1][0].calculateNegate(eo2);
									} else if(!definite_integral && mtest[1][0][0].representsReal(true)) {
										mtest[1][1].calculateMultiply(mtest[1][0][1], eo2);
										mtest[1][0].setToChild(1);
										mtest[1][0].transformById(FUNCTION_ID_ABS);
									}
								}
							}
							mtest[1][0].evalSort(false);
							if(!mmul2.isOne()) {
								mtest *= mmul2;
								mtest.evalSort(false);
							}
							CALCULATOR->beginTemporaryStopMessages();
							if(mtest.integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0) {
								CALCULATOR->endTemporaryStopMessages(true);
								set(mtest);
								if(!mmul.isOne()) multiply(mmul);
								return true;
							}
							CALCULATOR->endTemporaryStopMessages();
						}
						if(CHILD(1)[1].number().isMinusOne() && CHILD(0) == x_var) {
							if(integrate_info(CHILD(1)[0], x_var, madd, mmul, mmul2, false, true) && !mmul2.isZero() && mmul.isZero() && !madd.isZero()) {
								SET_CHILD_MAP(1)
								SET_CHILD_MAP(0)
								transformById(FUNCTION_ID_LOG);
								mmul2 *= Number(-2, 1);
								multiply(mmul2);
								return true;
							}
						}
					}
				} else if(CHILD(1).isPower() && ((CHILD(1)[0].isNumber() && !CHILD(1)[0].number().isOne()) || (!CHILD(1)[0].isNumber() && CHILD(1)[0].containsRepresentativeOf(x_var, true, true) == 0))) {
					MathStructure mexp(1, 1, 0);
					MathStructure madd, mmul, mpow;
					if(CHILD(0).isPower() && CHILD(0)[0] == x_var) {
						mexp = CHILD(0)[1];
						if(!mexp.isInteger()) {
							if(!definite_integral && integrate_info(CHILD(1)[1], x_var, madd, mmul, mpow, false, false)) {
								if(!madd.isZero()) {
									madd ^= CHILD(1)[0];
									madd.swapChildren(1, 2);
								}
								if(mpow.isOne() && CHILD(1)[0].isVariable() && CHILD(1)[0].variable()->id() == VARIABLE_ID_E) {
									setToChild(1, true);
									setToChild(2, true);
									if(mmul.isMinusOne()) {
										add(m_one);
										transformById(FUNCTION_ID_I_GAMMA);
										addChild(x_var);
										negate();
										if(!madd.isZero()) multiply(madd);
										return true;
									} else {
										add(m_one);
										transformById(FUNCTION_ID_I_GAMMA);
										addChild(x_var);
										CHILD(1) *= mmul;
										CHILD(1).negate();
										multiply(x_var);
										LAST ^= mexp;
										multiply(x_var);
										LAST *= mmul;
										LAST.negate();
										LAST ^= mexp;
										LAST.last().negate();
										divide(mmul);
										if(!madd.isZero()) multiply(madd);
										return true;
									}
								} else {
									MathStructure mlog(1, 1, 0);
									if(!CHILD(1)[0].isVariable() || CHILD(1)[0].variable()->id() != VARIABLE_ID_E) mlog.set(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(1)[0], NULL);
									setToChild(1, true);
									setToChild(2, true);
									add(m_one);
									if(!mpow.isOne()) divide(mpow);
									transformById(FUNCTION_ID_I_GAMMA);
									addChild(x_var);
									if(!mpow.isOne()) CHILD(1) ^= mpow;
									if(!mmul.isOne()) CHILD(1) *= mmul;
									if(!mlog.isOne()) CHILD(1) *= mlog;
									CHILD(1).negate();
									multiply(x_var);
									LAST ^= mexp;
									LAST.last() += m_one;
									multiply(x_var);
									if(!mpow.isOne()) LAST ^= mpow;
									if(!mmul.isOne()) LAST *= mmul;
									if(!mlog.isOne()) LAST *= mlog;
									LAST.negate();
									LAST ^= mexp;
									LAST.last() += m_one;
									LAST.last().negate();
									if(!mpow.isOne()) LAST.last().divide(mpow);
									negate();
									if(!mpow.isOne()) divide(mpow);
									if(!madd.isZero()) multiply(madd);
									return true;
								}
							}
							CANNOT_INTEGRATE;
						}
					} else if(CHILD(0) != x_var) CANNOT_INTEGRATE;
					if(mexp.number() < 100 && mexp.number() > -100 && integrate_info(CHILD(1)[1], x_var, madd, mmul, mpow, false, false) && mpow.isInteger()) {
						bool b_e = CHILD(1)[0].isVariable() && CHILD(1)[0].variable()->id() == VARIABLE_ID_E;
						if(b_e || CHILD(1)[0].isNumber() || warn_about_assumed_not_value(CHILD(1)[0], m_one, eo)) {
							if(mpow.isOne()) {
								SET_CHILD_MAP(1)
								if(!b_e) {
									if(mmul.isOne()) {
										mmul = CHILD(0);
										mmul.transformById(FUNCTION_ID_LOG);
									} else {
										MathStructure lnbase(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0), NULL);
										mmul *= lnbase;
									}
								}
								if(mexp.isOne()) {
									MathStructure mmulfac(x_var);
									if(!mmul.isOne()) mmulfac *= mmul;
									mmulfac += nr_minus_one;
									multiply(mmulfac);
									if(!mmul.isOne()) {
										mmul ^= Number(-2, 1);;
										multiply(mmul);
									}
								} else if(mexp.number().isTwo()) {
									MathStructure mmulfac(x_var);
									mmulfac ^= nr_two;
									if(!mmul.isOne()) mmulfac /= mmul;
									mmulfac += x_var;
									mmulfac.last() *= Number(-2, 1);
									if(!mmul.isOne()) {
										mmulfac.last() *= mmul;
										mmulfac.last().last() ^= Number(-2, 1);
									}
									mmulfac += nr_two;
									if(!mmul.isOne()) {
										mmulfac.last() *= mmul;
										mmulfac.last().last() ^= Number(-3, 1);
									}
									mmulfac.childrenUpdated(true);
									multiply(mmulfac);
								} else if(mexp.isMinusOne()) {
									if(!madd.isZero()) {
										madd.raise(CHILD(0));
										madd.swapChildren(1, 2);
									}
									set(x_var, true);
									if(!mmul.isOne()) multiply(mmul);
									transformById(FUNCTION_ID_EXPINT);
									if(!madd.isZero()) multiply(madd);
								} else if(mexp.number().isNegative()) {
									MathStructure mterm2(*this);
									mexp += m_one;
									multiply(x_var);
									LAST ^= mexp;
									CHILDREN_UPDATED
									if(integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) CANNOT_INTEGRATE_INTERVAL
									if(!mmul.isOne()) multiply(mmul);
									mterm2 *= x_var;
									mterm2.last() ^= mexp;
									mterm2.childrenUpdated();
									subtract(mterm2);
									mexp.negate();
									divide(mexp);
								} else {
									MathStructure mterm2(*this);
									multiply(x_var);
									LAST ^= mexp;
									LAST[1] += nr_minus_one;
									LAST.childUpdated(2);
									CHILDREN_UPDATED
									if(integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts) < 0) CANNOT_INTEGRATE_INTERVAL
									multiply(mexp);
									if(!mmul.isOne()) divide(mmul);
									negate();
									mterm2 *= x_var;
									mterm2.last() ^= mexp;
									mterm2.childrenUpdated();
									if(!mmul.isOne()) mterm2.divide(mmul);
									add(mterm2);
								}
								return true;
							} else {
								Number mpowmexp(mpow.number());
								mpowmexp -= mexp.number();
								if(mpowmexp.isOne()) {
									SET_CHILD_MAP(1)
									MathStructure malog(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0), NULL);
									divide(mpow);
									if(!mmul.isOne()) divide(mmul);
									if(!b_e) divide(malog);
									return true;
								} else if(mexp.isMinusOne() && mpow.number().isPositive()) {
									MathStructure malog;
									if(b_e) {
										malog = x_var;
										malog ^= mpow;
									} else {
										malog.set(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(1)[0], NULL);
										malog *= x_var;
										malog.last() ^= mpow;
									}
									if(!mmul.isOne()) malog *= mmul;
									malog.transformById(FUNCTION_ID_EXPINT);
									if(madd.isZero()) {
										set(malog, true);
									} else {
										SET_CHILD_MAP(1)
										SET_CHILD_MAP(0)
										raise(madd);
										multiply(malog);
									}
									divide(mpow);
									return true;
								}
							}
						}
					}
				}
			}
			if(SIZE >= 2) {
				for(size_t i = 0; i < SIZE; i++) {
					if((CHILD(i).isFunction() && CHILD(i).function()->id() == FUNCTION_ID_SIGNUM) || (CHILD(i).isPower() && CHILD(i)[0].isFunction() && CHILD(i)[0].function()->id() == FUNCTION_ID_SIGNUM)) {
						MathStructure mfunc(CHILD(i).isPower() ? CHILD(i)[0] : CHILD(i));
						MathStructure mmul(*this);
						mmul.delChild(i + 1, true);
						CALCULATOR->beginTemporaryStopMessages();
						int bint = integrate_function(mfunc, x_var, eo, CHILD(i).isPower() ? CHILD(i)[1] : m_one, mmul, m_zero, m_one, use_abs, definite_integral, max_part_depth, parent_parts);
						CALCULATOR->endTemporaryStopMessages(bint > 0);
						if(bint < 0) CANNOT_INTEGRATE_INTERVAL
						if(bint) {
							set(mfunc, true);
							return true;
						}
					}
				}
				for(size_t i = 0; i < SIZE; i++) {
					if((CHILD(i).isFunction() && CHILD(i).function()->id() != FUNCTION_ID_SIGNUM) || (CHILD(i).isPower() && CHILD(i)[0].isFunction() && CHILD(i)[0].function()->id() != FUNCTION_ID_SIGNUM)) {
						MathStructure mfunc(CHILD(i).isPower() ? CHILD(i)[0] : CHILD(i));
						MathStructure mmul(*this);
						mmul.delChild(i + 1, true);
						CALCULATOR->beginTemporaryStopMessages();
						int bint = integrate_function(mfunc, x_var, eo, CHILD(i).isPower() ? CHILD(i)[1] : m_one, mmul, m_zero, m_one, use_abs, definite_integral, max_part_depth, parent_parts);
						CALCULATOR->endTemporaryStopMessages(bint > 0);
						if(bint < 0) CANNOT_INTEGRATE_INTERVAL
						if(bint) {
							set(mfunc, true);
							return true;
						}
					}
				}
				MathStructure mbak(*this);
				if(CHILD(0) == x_var || (CHILD(0).isPower() && CHILD(0)[0] == x_var && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isRational())) {
					Number nexp(1, 1, 0);
					if(CHILD(0).isPower()) nexp = CHILD(0)[1].number();
					MathStructure mpow(1, 1, 0);
					bool b = true;
					MathStructure madd, mmul, mpown;
					for(size_t i = 1; i < SIZE; i++) {
						if(CHILD(i).isFunction() && CHILD(i).size() >= 1) {
							if(!integrate_info(CHILD(i)[0], x_var, madd, mmul, mpown, false, false) || !mpown.isNumber() || !mpown.number().isRational() || mpown.number().isOne() || (!mpow.isOne() && mpow != mpown)) {
								b = false;
								break;
							}
							mpow = mpown;
						} else if(CHILD(i).isPower() && CHILD(i)[1].containsRepresentativeOf(x_var, true, true) == 0) {
							if((CHILD(i)[0].isFunction() && CHILD(i)[0].size() == 1)) {
								if(!integrate_info(CHILD(i)[0][0], x_var, madd, mmul, mpown, false, false)) {b = false; break;}
							} else if(integrate_info(CHILD(i)[0], x_var, madd, mmul, mpown, false, false, true)) {
								if(mpown.isPower() && mpown[0] == x_var) {
									mpown.setToChild(2);
									if(SIZE == 2 && mpown.isInteger()) {b = false; break;}
								} else if(mpown.isFunction() && mpown.size() >= 1) {
									MathStructure marg(mpown[0]);
									if(!integrate_info(marg, x_var, madd, mmul, mpown, false, false)) {b = false; break;	}
								} else {
									b = false; break;
								}
							} else {
								b = false; break;
							}
							if(!mpown.isNumber() || !mpown.number().isRational() || mpown.number().isOne() || (!mpow.isOne() && mpow != mpown)) {
								b = false;
								break;
							}
							mpow = mpown;
						} else if(CHILD(i).isPower() && CHILD(i)[0].containsRepresentativeOf(x_var, true, true) == 0) {
							if(!integrate_info(CHILD(i)[1], x_var, madd, mmul, mpown, false, false) || !mpown.isNumber() || !mpown.number().isRational() || mpown.number().isOne() || (!mpow.isOne() && mpow != mpown)) {
								b = false;
								break;
							}
							mpow = mpown;
						} else {
							b = false;
							break;
						}
					}
					if(b && (x_var.representsNonNegative(true) || mpow.number().isEven() || mpow.number().isFraction())) {
						for(int i = 1; i <= 3; i++) {
							if(!mpow.isInteger()) {
								if(i > 2) break;
							} else if(i > 1 && (!mpow.number().isIntegerDivisible(i) || mpow.number() == i)) {
								break;
							}
							if(CALCULATOR->aborted()) CANNOT_INTEGRATE
							UnknownVariable *var = NULL;
							MathStructure m_replace(x_var);
							MathStructure mtest(*this);
							mtest.delChild(1, true);
							Number new_pow(nexp);
							b = false;
							if(i == 1) {
								m_replace ^= mpow;
								var = new UnknownVariable("", "");
								mtest.replace(m_replace, var);
								new_pow++;
								new_pow -= mpow.number();
								new_pow /= mpow.number();
								b = true;
							} else if(i == 2) {
								if(!mpow.number().isInteger()) {
									Number nden = mpow.number().denominator();
									nden.recip();
									nden--;
									nden.negate();
									new_pow = nexp;
									new_pow += nden;
									new_pow *= mpow.number().denominator();
									if(new_pow.isInteger()) {
										b = true;
										m_replace ^= mpow.number().denominator();
										m_replace[1].number().recip();
										var = new UnknownVariable("", "");
										MathStructure m_prev(x_var), m_new(var);
										m_prev ^= mpow;
										if(!mpow.number().numeratorIsOne()) m_new ^= mpow.number().numerator();
										mtest.replace(m_prev, m_new);
									}
								} else if((mpow.number() / 2).isEven()) {
									new_pow = nexp;
									new_pow++;
									new_pow -= 2;
									new_pow /= 2;
									if(new_pow.isInteger()) {
										b = true;
										m_replace ^= nr_two;
										var = new UnknownVariable("", "");
										MathStructure m_prev(x_var), m_new(var);
										m_prev ^= mpow;
										m_new ^= mpow;
										m_new[1].number() /= 2;
										mtest.replace(m_prev, m_new);
									}
								}
							} else if(i == 3) {
								new_pow++;
								new_pow -= 3;
								new_pow /= 3;
								if(new_pow.isInteger()) {
									b = true;
									m_replace ^= nr_three;
									var = new UnknownVariable("", "");
									MathStructure m_prev(x_var), m_new(var);
									m_prev ^= mpow;
									m_new ^= mpow;
									m_new[1].number() /= 3;
									mtest.replace(m_prev, m_new);
								}
							}
							if(b) {
								if(!new_pow.isZero()) {
									mtest *= var;
									mtest.swapChildren(1, mtest.size());
									if(!new_pow.isOne()) mtest[0] ^= new_pow;
								}
								CALCULATOR->beginTemporaryStopMessages();
								var->setName(string(LEFT_PARENTHESIS) + format_and_print(m_replace) + RIGHT_PARENTHESIS);
								if(x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined()) {
									MathStructure m_interval(m_replace);
									m_interval.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
									var->setInterval(m_interval);
								} else {
									var->setInterval(m_replace);
								}
								if(mtest.integrate(var, eo, false, use_abs, definite_integral, true, max_part_depth, parent_parts) > 0 && mtest.containsFunctionId(FUNCTION_ID_INTEGRATE) <= 0) {
									CALCULATOR->endTemporaryStopMessages(true);
									mtest.replace(var, m_replace);
									set(mtest, true);
									if(m_replace.isPower()) divide(m_replace[1]);
									var->destroy();
									return true;
								}
								CALCULATOR->endTemporaryStopMessages();
								var->destroy();
							}
						}
					}
				}

				vector<MathStructure*> parent_parts_pre;
				if(parent_parts) {
					for(size_t i = 0; i < parent_parts->size(); i++) {
						if(equals(*(*parent_parts)[i], true)) CANNOT_INTEGRATE
					}
				} else {
					parent_parts = &parent_parts_pre;
				}
				size_t pp_size = parent_parts->size();
				bool b = false;

				// integration by parts: u(x)*v(x): u*integral of(v) - integral of (u'(x) * integral of v(x))
				for(size_t i = 0; !b && max_part_depth > 0 && (i < SIZE || (SIZE == 3 && i < SIZE * 2)) && SIZE < 10; i++) {
					if(CALCULATOR->aborted()) CANNOT_INTEGRATE
					CALCULATOR->beginTemporaryStopMessages();
					MathStructure mstruct_u;
					MathStructure mstruct_v;
					MathStructure minteg_v;

					// select two parts
					if(SIZE == 3 && i >= 3) {
						mstruct_v = CHILD(i - 3);
						mstruct_u = *this;
						mstruct_u.delChild(i - 2);
					} else {
						mstruct_u = CHILD(i);
						if(SIZE == 2 && i == 0) mstruct_v = CHILD(1);
						else if(SIZE == 2 && i == 1) mstruct_v = CHILD(0);
						else {mstruct_v = *this; mstruct_v.delChild(i + 1);}
					}

					MathStructure mdiff_u(mstruct_u);
					if(mdiff_u.differentiate(x_var, eo2) && mdiff_u.containsFunctionId(FUNCTION_ID_DIFFERENTIATE, true) <= 0 && (!definite_integral || check_zero_div(mdiff_u, x_var, eo))) {
						minteg_v = mstruct_v;
						if(minteg_v.integrate(x_var, eo, false, use_abs, definite_integral, true, 0, parent_parts) > 0) {
							parent_parts->push_back(this);
							MathStructure minteg_2(minteg_v);
							if(!mdiff_u.isOne()) minteg_2 *= mdiff_u;
							if(minteg_2.countTotalChildren() < 1000) {
								minteg_2.evalSort(true);
								minteg_2.calculateFunctions(eo2);
								minteg_2.calculatesub(eo2, eo2, true);
								combine_ln(minteg_2, x_var, eo2);
								do_simplification(minteg_2, eo2, true, false, false, true, true);
							}
							if(minteg_2.countTotalChildren() < 100 && minteg_2.integrate(x_var, eo, false, use_abs, definite_integral, true, max_part_depth - 1, parent_parts) > 0) {
								int cui = contains_unsolved_integrate(minteg_2, this, parent_parts);
								if(cui == 3) {
									MathStructure mfunc(CALCULATOR->getFunctionById(FUNCTION_ID_INTEGRATE), this, &m_undefined, &m_undefined, &x_var, &m_zero, NULL);
									UnknownVariable *var = new UnknownVariable("", format_and_print(mfunc));
									var->setAssumptions(mfunc);
									MathStructure mvar(var);
									minteg_2.replace(mfunc, mvar);
									MathStructure msolve(mstruct_u);
									msolve.multiply(minteg_v);
									msolve.evalSort(false);
									msolve.calculatesub(eo2, eo2, true);
									msolve.subtract(minteg_2);
									msolve.evalSort(false);
									msolve.calculatesub(eo2, eo2, true);
									MathStructure msolve_d(msolve);
									if(msolve_d.differentiate(mvar, eo2) && comparison_is_not_equal(msolve_d.compare(m_one))) {
										msolve.transform(COMPARISON_EQUALS, mvar);
										msolve.isolate_x(eo2, mvar);
										if(msolve.isComparison() && msolve.comparisonType() == COMPARISON_EQUALS && msolve[0] == mvar && msolve[1].contains(mvar, true) <= 0) {
											b = true;
											set(msolve[1], true);
										}
									}
									var->destroy();
								} else if(cui != 1) {
									set(mstruct_u);
									multiply(minteg_v);
									subtract(minteg_2);
									b = true;
								}
							}
							parent_parts->pop_back();
						}
					}
					CALCULATOR->endTemporaryStopMessages(b);
				}
				while(parent_parts->size() > pp_size) parent_parts->pop_back();
				if(b) return true;
			}
			CANNOT_INTEGRATE
			break;
		}
		case STRUCT_SYMBOLIC: {
			if(representsNumber(true)) {
				// y: x*y
				multiply(x_var);
			} else {
				CANNOT_INTEGRATE
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if(eo.approximation != APPROXIMATION_EXACT || !o_variable->isApproximate()) {
					set(((KnownVariable*) o_variable)->get(), true);
					unformat(eo);
					return integrate(x_var, eo, true, use_abs, definite_integral, true, max_part_depth, parent_parts);
				} else if(containsRepresentativeOf(x_var, true, true) != 0) {
					CANNOT_INTEGRATE
				}
			}
			// y: x*y
			if(representsNumber(true)) {
				multiply(x_var);
				break;
			}
		}
		default: {
			CANNOT_INTEGRATE
		}
	}
	return true;
}

// Type 0: Simpson's rule, 1: trapezoid, 2: Simpson's 3/8, 3: Boole's

bool numerical_integration(const MathStructure &mstruct, Number &nvalue, const MathStructure &x_var, const EvaluationOptions &eo2, const Number &nr_begin, const Number &nr_end, int i_samples, int type = 0) {
	if((type == 0 && i_samples % 2 == 1) || (type == 2 && i_samples % 3 == 2) || (type == 3 && i_samples % 4 == 3)) {
		i_samples++;
	} else if((type == 2 && i_samples % 3 == 1) || (type == 3 && i_samples % 4 == 2)) {
		i_samples += 2;
	} else if(type == 3 && i_samples % 4 == 1) {
		i_samples += 3;
	}
	Number nr_step(nr_end);
	nr_step -= nr_begin;
	nr_step /= i_samples;
	MathStructure m_a = mstruct;
	m_a.replace(x_var, nr_begin, false, false, true);
	m_a.eval(eo2);
	if(!m_a.isNumber()) return false;
	nvalue = m_a.number();
	m_a = mstruct;
	m_a.replace(x_var, nr_end, false, false, true);
	m_a.eval(eo2);
	if(!m_a.isNumber()) return false;
	if(!nvalue.add(m_a.number())) return false;
	if(type == 1 && !nvalue.multiply(nr_half)) return false;
	if(type == 3 && !nvalue.multiply(7)) return false;
	for(int i = 1; i < i_samples; i++) {
		if(CALCULATOR->aborted()) {
			return false;
		}
		Number nr(nr_step);
		nr *= i;
		nr += nr_begin;
		MathStructure m_a(mstruct);
		m_a.replace(x_var, nr, false, false, true);
		m_a.eval(eo2);
		if(!m_a.isNumber()) return false;
		if((type == 0 && i % 2 == 0) || (type == 2 && i % 3 == 0)) {
			if(!m_a.number().multiply(nr_two)) return false;
		} else if(type == 0) {
			if(!m_a.number().multiply(4)) return false;
		} else if(type == 2) {
			if(!m_a.number().multiply(nr_three)) return false;
		} else if(type == 3 && i % 4 == 0) {
			if(!m_a.number().multiply(14)) return false;
		} else if(type == 3 && i % 2 == 0) {
			if(!m_a.number().multiply(12)) return false;
		} else if(type == 3) {
			if(!m_a.number().multiply(32)) return false;
		}
		if(!nvalue.add(m_a.number())) return false;
	}
	if(!nvalue.multiply(nr_step)) return false;
	if(type == 0 && !nvalue.multiply(Number(1, 3))) return false;
	if(type == 2 && !nvalue.multiply(Number(3, 8))) return false;
	if(type == 3 && !nvalue.multiply(Number(2, 45))) return false;
	return true;
}
bool montecarlo(const MathStructure &minteg, Number &nvalue, const MathStructure &x_var, const EvaluationOptions &eo, Number a, Number b, Number n) {
	Number range(b); range -= a;
	MathStructure m;
	Number u;
	nvalue.clear();
	vector<Number> v;
	Number i(1, 1);
	while(i <= n) {
		if(CALCULATOR->aborted()) {
			n = i;
			break;
		}
		u.rand();
		u *= range;
		u += a;
		m = minteg;
		m.replace(x_var, u, false, false, true);
		m.eval(eo);
		if(!m.isNumber() || m.number().includesInfinity()) return false;
		if(!m.number().multiply(range)) return false;
		if(!nvalue.add(m.number())) return false;
		v.push_back(m.number());
		i++;
	}
	if(!nvalue.divide(n)) return false;
	Number var;
	for(size_t i = 0; i < v.size(); i++) {
		if(!v[i].subtract(nvalue) || !v[i].square() || !var.add(v[i])) return false;
	}
	if(!var.divide(n) || !var.sqrt()) return false;
	Number nsqrt(n); if(!nsqrt.sqrt() || !var.divide(nsqrt)) return false;
	nvalue.setUncertainty(var);
	return true;
}
bool has_wide_trig_interval(const MathStructure &m, const MathStructure &x_var, const EvaluationOptions &eo, Number a, Number b) {
	for(size_t i = 0; i < m.size(); i++) {
		if(has_wide_trig_interval(m[i], x_var, eo, a, b)) return true;
	}
	if(m.isFunction() && m.size() == 1 && (m.function()->id() == FUNCTION_ID_SINC || m.function()->id() == FUNCTION_ID_SIN || m.function()->id() == FUNCTION_ID_COS)) {
		Number nr_interval;
		nr_interval.setInterval(a, b);
		MathStructure mtest(m[0]);
		mtest.replace(x_var, nr_interval, false, false, true);
		CALCULATOR->beginTemporaryStopMessages();
		mtest.eval(eo);
		CALCULATOR->endTemporaryStopMessages();
		return mtest.isMultiplication() && mtest.size() >= 1 && mtest[0].isNumber() && (mtest[0].number().uncertainty().realPart() > 100 || mtest[0].number().uncertainty().imaginaryPart() > 100);
	}
	return false;
}

#define CLEANUP_ROMBERG delete[] R1; delete[] R2;
bool romberg(const MathStructure &minteg, Number &nvalue, const MathStructure &x_var, const EvaluationOptions &eo, Number a, Number b, long int max_steps, long int min_steps, bool safety_measures) {

	bool auto_max = max_steps <= 0;
	if(auto_max) max_steps = 22;
	if(min_steps > max_steps) max_steps = min_steps;

	Number *R1 = new Number[max_steps];
	Number *R2 = new Number[max_steps];
	Number *Rp = &R1[0], *Rc = &R2[0];
	Number h(b); h -= a;
	Number acc, acc_i, c, ntmp, prevunc, prevunc_i, nunc, nunc_i;

	nvalue.clear();

	MathStructure mf(minteg);
	mf.replace(x_var, a, false, false, true);
	mf.eval(eo);
	if(!mf.isNumber() || mf.number().includesInfinity()) {
		if(!a.setToFloatingPoint()) {CLEANUP_ROMBERG; return false;}
		mpfr_nextabove(a.internalLowerFloat());
		mpfr_nextabove(a.internalUpperFloat());
		mf = minteg;
		mf.replace(x_var, a, false, false, true);
		mf.eval(eo);
	}
	if(!mf.isNumber()) {CLEANUP_ROMBERG; return false;}
	Rp[0] = mf.number();

	mf = minteg;
	mf.replace(x_var, b, false, false, true);
	mf.eval(eo);
	if(!mf.isNumber() || mf.number().includesInfinity()) {
		if(!b.setToFloatingPoint()) {CLEANUP_ROMBERG; return false;}
		mpfr_nextbelow(b.internalLowerFloat());
		mpfr_nextbelow(b.internalUpperFloat());
		mf = minteg;
		mf.replace(x_var, a, false, false, true);
		mf.eval(eo);
	}
	if(!mf.isNumber()) {CLEANUP_ROMBERG; return false;}

	if(!Rp[0].add(mf.number()) || !Rp[0].multiply(nr_half) || !Rp[0].multiply(h)) {CLEANUP_ROMBERG; return false;}

	if(safety_measures && min_steps < 15 && has_wide_trig_interval(minteg, x_var, eo, a, b)) min_steps = (max_steps < 15 ? max_steps : 15);

	for(long int i = 1; i < max_steps; i++) {

		if(CALCULATOR->aborted()) break;

		if(!h.multiply(nr_half)) {CLEANUP_ROMBERG; return false;}

		c.clear();

		long int ep = 1 << (i - 1);
		ntmp = a; ntmp += h;
		for(long int j = 1; j <= ep; j++){
			if(CALCULATOR->aborted()) break;
			mf = minteg;
			mf.replace(x_var, ntmp, false, false, true);
			mf.eval(eo);
			if(CALCULATOR->aborted()) break;
			if(!mf.isNumber() || mf.number().includesInfinity()) {
				Number ntmp2(ntmp);
				if(ntmp2.setToFloatingPoint()) {CLEANUP_ROMBERG; return false;}
				if(j % 2 == 0) {mpfr_nextabove(ntmp2.internalLowerFloat()); mpfr_nextabove(ntmp2.internalUpperFloat());}
				else {mpfr_nextbelow(ntmp2.internalLowerFloat()); mpfr_nextbelow(ntmp2.internalUpperFloat());}
				mf = minteg;
				mf.replace(x_var, ntmp2, false, false, true);
				mf.eval(eo);
				if(CALCULATOR->aborted()) break;
			}
			if(!mf.isNumber() || !c.add(mf.number())) {CLEANUP_ROMBERG; return false;}
			ntmp += h; ntmp += h;
		}
		if(CALCULATOR->aborted()) break;

		Rc[0] = h;
		ntmp = Rp[0];
		if(!ntmp.multiply(nr_half) || !Rc[0].multiply(c) || !Rc[0].add(ntmp)) {CLEANUP_ROMBERG; return false;}

		for(long int j = 1; j <= i; ++j){
			if(CALCULATOR->aborted()) break;
			ntmp = 4;
			ntmp ^= j;
			Rc[j] = ntmp;
			ntmp--; ntmp.recip();
			if(!Rc[j].multiply(Rc[j - 1]) || !Rc[j].subtract(Rp[j - 1]) || !Rc[j].multiply(ntmp)) {CLEANUP_ROMBERG; return false;}
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
			long int prec = PRECISION + (auto_max ? 3 : 1);
			if(auto_max) {
				if(i > 10) prec += 10 - ((!prevunc.isZero() || !prevunc_i.isZero()) ? i - 1 : i);
				if(prec < 4) prec = 4;
			}
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
							CLEANUP_ROMBERG
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
							CLEANUP_ROMBERG
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
						CLEANUP_ROMBERG
						return true;
					}
					if(!prevunc.isZero() || nunc.isZero()) {
						if(!prevunc_i.isZero()) nunc.setImaginaryPart(prevunc_i);
						if(!ntmp.isZero() && nunc <= prevunc) nvalue.setUncertainty(prevunc);
						else nvalue.setUncertainty(acc);
						CLEANUP_ROMBERG
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
	CLEANUP_ROMBERG
	if(!nunc.isZero() || !nunc_i.isZero()) {
		acc.set(1, 1, auto_max ? -3 : -2);
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
			acc.set(1, 1, -3);
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
int numerical_integration_part(const MathStructure &minteg, const MathStructure &x_var, const MathStructure &merr_pre, const MathStructure &merr_diff, KnownVariable *v, Number &nvalue, EvaluationOptions &eo, const Number &nr, int type = 0, int depth = 0, bool nzerodiff = false) {
	if(CALCULATOR->aborted()) return 0;
	eo.interval_calculation = INTERVAL_CALCULATION_NONE;
	MathStructure merr(merr_pre);
	int i_ret = 1;
	v->set(nr);
	bool do_parts = true;
	/*if(nzerodiff) {
		v->set(nr.lowerEndPoint());
		MathStructure mlow(merr);
		mlow.eval(eo);
		v->set(nr.upperEndPoint());
		merr.eval(eo);
		if(!merr.isNumber() || !mlow.isNumber() || merr.number().includesInfinity() || mlow.number().includesInfinity()) return 0;
		merr.number().setInterval(mlow.number(), merr.number());
		do_parts = false;
	} else if(depth > 1 && !merr_diff.isUndefined()) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure mzero(merr_diff);
		mzero.eval(eo);
		if(CALCULATOR->endTemporaryStopMessages() == 0 && mzero.isNumber() && mzero.number().isNonZero() && (!mzero.number().hasImaginaryPart() || mzero.number().internalImaginary()->isNonZero())) {
			v->set(nr.lowerEndPoint());
			MathStructure mlow(merr);
			mlow.eval(eo);
			v->set(nr.upperEndPoint());
			merr.eval(eo);
			if(!merr.isNumber() || !mlow.isNumber() || merr.number().includesInfinity() || mlow.number().includesInfinity()) return 0;
			merr.number().setInterval(mlow.number(), merr.number());
			do_parts = false;
			nzerodiff = true;
		}
	}*/
	if(depth > 0 && do_parts) {
		CALCULATOR->beginTemporaryStopMessages();
		merr.eval(eo);
		do_parts = (CALCULATOR->endTemporaryStopMessages() > 0 || !merr.isNumber() || merr.number().includesInfinity());
	}
	if(do_parts) {
		integrate_parts:
		if(depth > 4) {
			if(numerical_integration(minteg, nvalue, x_var, eo, nr.lowerEndPoint(), nr.upperEndPoint(), 1002, type)) {nvalue.setApproximate(); return -1;}
			return 0;
		}
		vector<Number> parts;
		nr.splitInterval(depth == 0 ? 5 : 5, parts);
		nvalue.clear();
		Number n_i;
		for(size_t i = 0; i < parts.size(); i++) {
			int i_reti = numerical_integration_part(minteg, x_var, merr_pre, merr_diff, v, n_i, eo, parts[i], type, depth + 1, nzerodiff);
			if(i_reti != 0) {
				if(i_reti < i_ret) i_ret = i_reti;
				if(!nvalue.add(n_i)) return false;
			} else {
				return 0;
			}
		}
		return i_ret;
	} else {
		CALCULATOR->beginTemporaryStopIntervalArithmetic();
		eo.interval_calculation = INTERVAL_CALCULATION_NONE;
		Number nr_interval(merr.number());
		Number nr_interval_abs;
		Number nr1(nr_interval.upperEndPoint(true));
		Number nr2(nr_interval.lowerEndPoint(true));
		nr1.abs();
		nr2.abs();
		if(nr1.isGreaterThan(nr2)) nr_interval_abs = nr1;
		else nr_interval_abs = nr2;
		if(merr.number().hasImaginaryPart()) {
			if(merr.number().hasRealPart()) {
				nr1 = merr.number().realPart().upperEndPoint();
				nr2 = merr.number().realPart().lowerEndPoint();
				nr1.abs();
				nr2.abs();
				if(nr1.isGreaterThan(nr2)) nr_interval = nr1;
				else nr_interval = nr2;
				nr1 = merr.number().imaginaryPart().upperEndPoint();
				nr2 = merr.number().imaginaryPart().lowerEndPoint();
				nr1.abs();
				nr2.abs();
				if(nr1.isGreaterThan(nr2)) nr_interval.setImaginaryPart(nr1);
				else nr_interval.setImaginaryPart(nr2);
			} else {
				nr_interval.setImaginaryPart(nr_interval_abs);
			}
		} else {
			nr_interval = nr_interval_abs;
		}
		Number nr_samples(6, 1);
		Number nr_range(nr.upperEndPoint());
		nr_range -= nr.lowerEndPoint();
		int i_run = 0;
		while(true) {
			if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopIntervalArithmetic(); return 0;}
			if(numerical_integration(minteg, nvalue, x_var, eo, nr.lowerEndPoint(), nr.upperEndPoint(), nr_samples.intValue(), type)) {
				if(nr.includesInfinity()) {
					CALCULATOR->endTemporaryStopIntervalArithmetic();
					return 0;
				}
				Number nr_prec(nr_range);
				if(type == 0 || type == 2) {
					nr_prec /= nr_samples;
					nr_prec ^= 4;
					nr_prec *= nr_range;
					nr_prec /= (type == 2 ? 80 : 180);
				} else if(type == 1) {
					nr_prec /= nr_samples;
					nr_prec ^= 2;
					nr_prec *= nr_range;
					nr_prec /= 12;
				} else if(type == 3) {
					nr_prec ^= 7;
					//nr_prec *= Number(8, 945); ?
					nr_prec /= (nr_samples ^ 6);
				}
				nr_prec *= nr_interval;
				Number nr_prec_abs(nr_prec);
				nr_prec_abs.abs();
				Number mabs(nvalue);
				mabs.abs();
				Number max_error(1, 1, -(PRECISION + 1 - (i_run * 2)));
				max_error *= mabs;
				if(nr_prec_abs >= max_error) {
					while(true) {
						if(type == 0 || type == 2) {
							nr_samples = nr_range;
							nr_samples ^= 5;
							nr_samples /= (type == 2 ? 80 : 180);
							nr_samples *= nr_interval_abs;
							nr_samples /= max_error;
							nr_samples.root(4);
						} else if(type == 3) {
							nr_samples = nr_range;
							nr_samples ^= 7;
							//nr_samples *= Number(8, 945); ?
							nr_samples *= nr_interval_abs;
							nr_samples /= max_error;
							nr_samples.root(6);
						} else {
							nr_samples = nr_range;
							nr_samples ^= 3;
							nr_samples /= 12;
							nr_samples *= nr_interval_abs;
							nr_samples /= max_error;
							nr_samples.sqrt();
						}
						nr_samples.intervalToMidValue();
						nr_samples.ceil();
						if(type == 0 && nr_samples.isOdd()) {
							nr_samples++;
						} else if(type == 2 && !nr_samples.isIntegerDivisible(3)) {
							Number nmod(nr_samples);
							nmod.mod(3);
							if(nmod == 1) nr_samples += 2;
							else nr_samples += 1;
						} else if(type == 3 && !nr_samples.isIntegerDivisible(4)) {
							Number nmod(nr_samples);
							nmod.mod(4);
							if(nmod == 1) nr_samples += 3;
							if(nmod == 2) nr_samples += 2;
							else nr_samples += 1;
						}
						if(nr_samples < 10000) break;
						i_run++;
						if(depth <= 3 && PRECISION + 1 - (i_run * 2) < (PRECISION > 10 ? 10 : PRECISION) - depth) goto integrate_parts;
						if(PRECISION + 1 - (i_run * 2) < 5) {
							if(nr_samples < 50000) break;
							if(PRECISION + 1 - (i_run * 2) < 2 - depth) {
								if(numerical_integration(minteg, nvalue, x_var, eo, nr.lowerEndPoint(), nr.upperEndPoint(), 1002, type)) {nvalue.setApproximate(); return -1;}
								return 0;
							}
						}
						max_error *= Number(1, 1, (PRECISION + 1 - ((i_run - 1) * 2)));
						max_error *= Number(1, 1, -(PRECISION + 1 - (i_run * 2)));
					}
				} else {
					CALCULATOR->endTemporaryStopIntervalArithmetic();
					nvalue.setUncertainty(nr_prec);
					return 1;
				}
			} else {
				CALCULATOR->endTemporaryStopIntervalArithmetic();
				return 0;
			}
			i_run++;
		}
	}
	return 0;
}
bool contains_complex(const MathStructure &mstruct) {
	if(mstruct.isNumber()) return mstruct.number().isComplex();
	if(mstruct.isVariable() && mstruct.variable()->isKnown()) return contains_complex(((KnownVariable*) mstruct.variable())->get());
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_complex(mstruct[i])) return true;
	}
	return false;
}

int check_denominators(const MathStructure &m, const MathStructure &mi, const MathStructure &mx, const EvaluationOptions &eo) {
	if(m.contains(mx, false, true, true) == 0) return true;
	int ret = true;
	for(size_t i = 0; i < m.size(); i++) {
		int ret_i = check_denominators(m[i], mi, mx, eo);
		if(!ret_i) return false;
		if(ret_i < 0) {
			if(ret > 0) ret = ret_i;
			else ret += ret_i;
		}
	}
	if(m.isPower()) {
		bool b_neg = m[1].representsNegative();
		if(b_neg && (m[1].representsFraction() || (m[1].isMinusOne() && m[0].isFunction() && (m[0].function()->id() == FUNCTION_ID_SQRT || m[0].function()->id() == FUNCTION_ID_CBRT)))) {
			return -1;
		} else if(!b_neg && !m[1].representsNonNegative()) {
			if(!m[0].representsNonZero()) {
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				eo2.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
				CALCULATOR->beginTemporaryStopMessages();
				KnownVariable *v = new KnownVariable("", "v", mi);
				MathStructure mpow(m[1]);
				mpow.replace(mx, v, true, false, true);
				mpow.eval(eo2);
				b_neg = mpow.representsNegative();
				CALCULATOR->endTemporaryStopMessages();
				v->destroy();
			}
		}
		if(b_neg && !m[0].representsNonZero()) {
			if(m[0].isZero()) return false;
			EvaluationOptions eo2 = eo;
			eo2.approximation = APPROXIMATION_APPROXIMATE;
			eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
			CALCULATOR->beginTemporaryStopMessages();
			MathStructure mbase(m[0]);
			KnownVariable *v = new KnownVariable("", "v", mi);
			mbase.replace(mx, v, true, false, true);
			bool b_multiple = mbase.contains(mx, true) > 0;
			if(b_multiple) mbase.replace(mx, v, false, false, true);
			mbase.eval(eo2);
			CALCULATOR->endTemporaryStopMessages();
			/*if(mbase.isZero()) {
				v->destroy(); return false;
			} else if(!b_multiple && mbase.isNumber()) {
				if(!mbase.number().isNonZero()) {v->destroy(); return false;}
			} else if(!mbase.isNumber() || !mbase.number().isNonZero()) {
				CALCULATOR->beginTemporaryStopMessages();
				eo2.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
				mbase = m[0];
				mbase.replace(mx, v, false, false, true);
				mbase.eval(eo2);
				CALCULATOR->endTemporaryStopMessages();
			}*/
			if(mbase.isZero()) {
				v->destroy(); return false;
			} else if(!b_multiple && mbase.isNumber()) {
				if(!mbase.number().isNonZero()) {v->destroy(); return false;}
			} else if(!mbase.isNumber() || !mbase.number().isNonZero()) {
				CALCULATOR->beginTemporaryStopMessages();
				mbase = m[0];
				eo2.isolate_x = true;
				eo2.isolate_var = &mx;
				eo2.test_comparisons = true;
				eo2.approximation = APPROXIMATION_EXACT;
				mbase.transform(COMPARISON_NOT_EQUALS, m_zero);
				mbase.eval(eo2);
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				eo2.isolate_x = false;
				eo2.isolate_var = NULL;
				if(!mbase.isNumber()) {
					MathStructure mtest(mbase);
					mtest.replace(mx, v, false, false, true);
					mtest.eval(eo2);
					if(mtest.isNumber()) mbase = mtest;
				}
				CALCULATOR->endTemporaryStopMessages();
				if(!mbase.isOne()) {
					if(mbase.isZero()) {v->destroy(); return false;}
					bool b = false;
					if(mbase.isComparison()) {
						CALCULATOR->beginTemporaryStopMessages();
						mbase[0].eval(eo2); mbase[1].eval(eo2);
						CALCULATOR->endTemporaryStopMessages();
					}
					if(mbase.isComparison() && mbase.comparisonType() == COMPARISON_NOT_EQUALS && (mbase[0] == mx || mbase[0].isFunction()) && mbase[1].isNumber() && mi.isNumber()) {
						ComparisonResult cr = COMPARISON_RESULT_UNKNOWN;
						if(mbase[0].isFunction()) {
							MathStructure mfunc(mbase[0]);
							mfunc.replace(mx, v, false, false, true);
							mfunc.eval(eo2);
							if(mfunc.isNumber()) cr = mbase[1].number().compare(mfunc.number());
						} else {
							cr = mbase[1].number().compare(mi.number());
						}
						b = COMPARISON_IS_NOT_EQUAL(cr);
						if(!b && cr != COMPARISON_RESULT_UNKNOWN) {v->destroy(); return false;}
					} else if(mbase.isLogicalAnd()) {
						for(size_t i = 0; i < mbase.size(); i++) {
							if(mbase[i].isComparison()) {mbase[i][0].eval(eo2); mbase[i][1].eval(eo2);}
							if(mbase[i].isComparison() && mbase[i].comparisonType() == COMPARISON_NOT_EQUALS && (mbase[i][0] == mx || mbase[i][0].isFunction()) && mbase[i][1].isNumber() && mi.isNumber()) {
								ComparisonResult cr = COMPARISON_RESULT_UNKNOWN;
								if(mbase[i][0].isFunction()) {
									MathStructure mfunc(mbase[i][0]);
									mfunc.replace(mx, v, false, false, true);
									mfunc.eval(eo2);
									if(mfunc.isNumber()) cr = mbase[i][1].number().compare(mfunc.number());
								} else {
									cr = mbase[i][1].number().compare(mi.number());
								}
								b = COMPARISON_IS_NOT_EQUAL(cr);
								if(!b && cr != COMPARISON_RESULT_UNKNOWN) {v->destroy(); return false;}
							}
						}
					}
					if(!b) {
						CALCULATOR->endTemporaryStopMessages();
						CALCULATOR->endTemporaryStopMessages();
						CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), format_and_print(mbase).c_str(), NULL);
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->beginTemporaryStopMessages();
					}
				}
			}
			v->destroy();
		}
	} else if(m.isVariable()) {
		if(m.variable()->isKnown()) return check_denominators(((KnownVariable*) m.variable())->get(), mi, mx, eo);
	} else if(m.isFunction() && (m.function()->id() == FUNCTION_ID_TAN || m.function()->id() == FUNCTION_ID_TANH || !m.representsNumber(true))) {
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		eo2.assume_denominators_nonzero = false;
		MathStructure mfunc(m);
		bool b = mfunc.calculateFunctions(eo2);
		if(b && !check_denominators(mfunc, mi, mx, eo)) return false;
		if(mfunc.isFunction() && (mfunc.function()->id() == FUNCTION_ID_TAN || mfunc.function()->id() == FUNCTION_ID_TANH)) {
			mfunc.replace(mx, mi, false, false, true);
			eo2.interval_calculation = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
			CALCULATOR->beginTemporaryStopMessages();
			b = mfunc.calculateFunctions(eo2);
			CALCULATOR->endTemporaryStopMessages();
			if(!b) return false;
		}
	}
	return ret;
}
bool replace_atanh(MathStructure &m, const MathStructure &x_var, const MathStructure &m1, const MathStructure &m2, const EvaluationOptions &eo) {
	bool b = false;
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_ATANH && m.size() == 1 && m[0].contains(x_var, true)) {
		/*MathStructure mtest(m[0]);
		mtest.replace(x_var, m1, false, false, true);
		b = (mtest.compare(m_one) == COMPARISON_RESULT_EQUAL) || (mtest.compare(m_minus_one) == COMPARISON_RESULT_EQUAL);
		if(!b) {
			mtest = m[0];
			mtest.replace(x_var, m2, false, false, true);
			b = (mtest.compare(m_one) == COMPARISON_RESULT_EQUAL) || (mtest.compare(m_minus_one) == COMPARISON_RESULT_EQUAL);
		}*/
		b = true;
		if(b) {
			MathStructure marg(m[0]);
			m = marg;
			m += m_one;
			m.transformById(FUNCTION_ID_LOG);
			m *= nr_half;
			m += marg;
			m.last().negate();
			m.last() += m_one;
			m.last().transformById(FUNCTION_ID_LOG);
			m.last() *= Number(-1, 2);
			return true;
		}
	}
	if(m.isPower() && m[1].isInteger() && (m[1].number() > 10 || m[1].number() < -10)) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(replace_atanh(m[i], x_var, m1, m2, eo)) b = true;
	}
	if(b) {
		m.childrenUpdated();
		m.calculatesub(eo, eo, false);
	}
	return b;
}
MathStructure *find_abs_x(MathStructure &mstruct, const MathStructure &x_var) {
	for(size_t i = 0; i < mstruct.size(); i++) {
		MathStructure *m = find_abs_x(mstruct[i], x_var);
		if(m) return m;
	}
	if(mstruct.isFunction() && ((mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1) || (mstruct.function()->id() == FUNCTION_ID_ROOT && mstruct.size() == 2 && mstruct[1].isInteger() && mstruct[1].number().isOdd()))) {
		return &mstruct;
	}
	return NULL;
}
bool replace_abs(MathStructure &mstruct, const MathStructure &mabs, bool neg) {
	if(mstruct.equals(mabs, true, true)) {
		if(mabs.function()->id() == FUNCTION_ID_ROOT) {
			mstruct[1].inverse();
			mstruct.setType(STRUCT_POWER);
			if(neg) {
				mstruct[0].negate();
				mstruct.negate();
			}
		} else {
			mstruct.setToChild(1, true);
			if(neg) mstruct.negate();
		}
		return true;
	}
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_abs(mstruct[i], mabs, neg)) b_ret = true;
	}
	return b_ret;
}

// Check definite integral for functions that cannot be calculated
bool contains_incalc_function(const MathStructure &mstruct, const EvaluationOptions &eo) {
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_incalc_function(mstruct[i], eo)) return true;
	}
	if(mstruct.isFunction()) {
		if((mstruct.function()->id() == FUNCTION_ID_FRESNEL_S || mstruct.function()->id() == FUNCTION_ID_FRESNEL_C) && mstruct.size() == 1) {
			if(mstruct[0].representsComplex()) return true;
			MathStructure mtest(mstruct[0]);
			mtest.eval(eo);
			return !mtest.isNumber() || !(mtest.number() >= -6) || !(mtest.number() <= 6);
		} else if(mstruct.function()->id() == FUNCTION_ID_POLYLOG || ((mstruct.function()->id() == FUNCTION_ID_EXPINT || mstruct.function()->id() == FUNCTION_ID_ERF || mstruct.function()->id() == FUNCTION_ID_ERFI) && mstruct.size() == 1 && !mstruct[0].representsReal())) {
			MathStructure mtest(mstruct);
			mtest.eval(eo);
			return !mtest.isNumber();
		} else if(mstruct.function()->id() == FUNCTION_ID_I_GAMMA && mstruct.size() == 2) {
#if MPFR_VERSION_MAJOR < 4
			return true;
#else
			return !comparison_is_equal_or_less(mstruct[1].compare(m_zero));
#endif
		}
	}
	return false;
}

bool test_definite_ln(const MathStructure &m, const MathStructure &mi, const MathStructure &mx, const EvaluationOptions &eo) {
	for(size_t i = 0; i < m.size(); i++) {
		if(!test_definite_ln(m[i], mi, mx, eo)) return false;
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_LOG && m.size() == 1 && m[0].contains(mx, true) > 0 && !m[0].representsNonComplex(true)) {
		MathStructure mtest(m[0]);
		mtest.replace(mx, mi, false, false, true);
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
		CALCULATOR->beginTemporaryStopMessages();
		mtest.eval(eo2);
		CALCULATOR->endTemporaryStopMessages();
		if(mtest.isNumber() && mtest.number().hasImaginaryPart() && !mtest.number().imaginaryPartIsNonZero() && !mtest.number().realPart().isNonNegative()) return false;
	}
	return true;
}

void replace_intervals(MathStructure &m, vector<KnownVariable*> vars) {
	if(m.isNumber() && m.number().isInterval()) {
		KnownVariable *v = new KnownVariable("", format_and_print(m), m);
		m.set(v, true);
		vars.push_back(v);
		return;
	}
	for(size_t i = 0; i < m.size(); i++) {
		replace_intervals(m[i], vars);
	}
}
void restore_intervals(MathStructure &m, MathStructure &m2, vector<KnownVariable*> vars, const EvaluationOptions &eo) {
	for(size_t i = 0; i < vars.size(); i++) {
		if(eo.approximation == APPROXIMATION_EXACT) {
			m.replace(vars[i], vars[i]->get(), false, false, true);
			m2.replace(vars[i], vars[i]->get(), false, false, true);
		}
		vars[i]->destroy();
	}
}
size_t count_ln(const MathStructure &m) {
	size_t n = 0;
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_LOG) n++;
	for(size_t i = 0; i < m.size(); i++) {
		n += count_ln(m[i]);
	}
	return n;
}

bool MathStructure::integrate(const MathStructure &lower_limit, const MathStructure &upper_limit, const MathStructure &x_var_pre, const EvaluationOptions &eo, bool force_numerical, bool simplify_first) {

	if(!lower_limit.isUndefined() && lower_limit == upper_limit) {
		clear();
		return true;
	}

	MathStructure m1(lower_limit), m2(upper_limit);
	MathStructure x_var = x_var_pre;
	if(m1.isUndefined() != m2.isUndefined()) {
		if(m1.isUndefined()) m1.set(nr_minus_inf);
		else m2.set(nr_plus_inf);
	}
	m1.eval(eo);
	m2.eval(eo);
	int definite_integral = 0;
	if(!m1.isUndefined() && !m2.isUndefined()) definite_integral = -1;
	if(definite_integral < 0 && (!m1.isNumber() || !m1.number().isMinusInfinity()) && (!m2.isNumber() || !m2.number().isPlusInfinity())) definite_integral = 1;
	if(definite_integral > 0 && m1 == m2) {
		clear();
		return true;
	}

	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo2 = eo;
	if(simplify_first) eo2.approximation = APPROXIMATION_EXACT;
	CALCULATOR->beginTemporaryStopMessages();
	MathStructure mstruct_pre(*this);
	MathStructure m_interval;
	int den_check = 1;
	if(!m1.isUndefined()) {
		m_interval.set(CALCULATOR->getFunctionById(FUNCTION_ID_INTERVAL), &m1, &m2, NULL);
		CALCULATOR->beginTemporaryStopMessages();
		EvaluationOptions eo3 = eo;
		eo3.approximation = APPROXIMATION_APPROXIMATE;
		m_interval.calculateFunctions(eo3);
		CALCULATOR->endTemporaryStopMessages();
		UnknownVariable *var = new UnknownVariable("", format_and_print(x_var_pre));
		var->setInterval(m_interval);
		x_var.set(var);
		mstruct_pre.replace(x_var_pre, x_var, false, false, true);
		var->destroy();
		if(definite_integral) den_check = check_denominators(mstruct_pre, m_interval, x_var, eo);
		if(!den_check) {
			if(definite_integral < 0) {
				definite_integral = 0;
			} else {
				CALCULATOR->endTemporaryStopMessages();
				CALCULATOR->endTemporaryStopMessages(true);
				CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
				return false;
			}
		}
	}
	MathStructure mstruct(mstruct_pre);
	vector<KnownVariable*> vars;
	replace_intervals(mstruct, vars);
	eo2.do_polynomial_division = false;

	if(simplify_first) mstruct.eval(eo2);

	if(definite_integral > 0 && mstruct.isAddition() && m1.isNumber() && m1.number().isReal() && m2.isNumber() && m2.number().isReal()) {
		mstruct.replace(x_var, x_var_pre, false, false, true);
		MathStructure mbak(mstruct);
		Number nr;
		for(size_t i = 0; i < mstruct.size();) {
			if(!mstruct[i].integrate(lower_limit, upper_limit, x_var_pre, eo, force_numerical, simplify_first)) {
				CALCULATOR->endTemporaryStopMessages();
				CALCULATOR->endTemporaryStopMessages();
				CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
				if(simplify_first) set(mbak);
				return false;
			}
			if(mstruct[i].isNumber()) {
				if(nr.add(mstruct[i].number())) mstruct.delChild(i + 1);
				else i++;
			} else i++;
		}
		mstruct.childrenUpdated();
		if(mstruct.size() == 0) mstruct.set(nr, true);
		else if(!nr.isZero() || nr.isApproximate()) mstruct.addChild(nr);
		CALCULATOR->endTemporaryStopMessages(true);
		CALCULATOR->endTemporaryStopMessages(true);
		set(mstruct);
		return true;
	}
	if(simplify_first) do_simplification(mstruct, eo2, true, false, false, true, true);
	eo2.do_polynomial_division = eo.do_polynomial_division;
	MathStructure mbak(mstruct);

	if(!force_numerical || definite_integral == 0) {
		int use_abs = -1;
		/*if(m1.isUndefined() && x_var.representsReal() && !contains_complex(mstruct)) {
			use_abs = 1;
		}*/
		if(definite_integral) replace_atanh(mstruct, x_var, lower_limit, upper_limit, eo2);
		if(definite_integral && !simplify_first) combine_ln(mstruct, x_var, eo2);

		int b = mstruct.integrate(x_var, eo2, simplify_first, use_abs, definite_integral, true, (definite_integral && eo.approximation != APPROXIMATION_EXACT && PRECISION < 20) ? 2 : 4);

		if(b < 0) {
			restore_intervals(mstruct, mbak, vars, eo);
			CALCULATOR->endTemporaryStopMessages(true);
			CALCULATOR->endTemporaryStopMessages(true);
			CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
			if(simplify_first) {
				set(mbak);
				if(definite_integral != 0) replace(x_var, x_var_pre);
			}
			return false;
		}
		if(simplify_first && eo.approximation != APPROXIMATION_EXACT && (!b || mstruct.containsFunctionId(FUNCTION_ID_INTEGRATE, true) > 0)) {
			vector<CalculatorMessage> blocked_messages;
			CALCULATOR->endTemporaryStopMessages(false, &blocked_messages);
			CALCULATOR->beginTemporaryStopMessages();
			MathStructure mbak_integ(mstruct);
			eo2.approximation = eo.approximation;
			eo2.do_polynomial_division = false;
			mstruct = mstruct_pre;
			if(definite_integral) replace_atanh(mstruct, x_var, lower_limit, upper_limit, eo2);
			mstruct.eval(eo2);
			do_simplification(mstruct, eo2, true, false, false, true, true);
			eo2.do_polynomial_division = eo.do_polynomial_division;
			int b2 = mstruct.integrate(x_var, eo2, true, use_abs, definite_integral, true, (definite_integral && eo.approximation != APPROXIMATION_EXACT && PRECISION < 20) ? 2 : 4);
			if(b2 < 0) {
				restore_intervals(mstruct, mbak, vars, eo);
				CALCULATOR->endTemporaryStopMessages(true);
				CALCULATOR->endTemporaryStopMessages(true);
				CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
				if(simplify_first) {
					set(mbak);
					if(definite_integral != 0) replace(x_var, x_var_pre);
				}
				return false;
			}
			if(b2 && (!b || mstruct.containsFunctionId(FUNCTION_ID_INTEGRATE, true) <= 0)) {
				CALCULATOR->endTemporaryStopMessages(true);
				b = true;
			} else {
				CALCULATOR->endTemporaryStopMessages(false);
				if(b) {
					CALCULATOR->addMessages(&blocked_messages);
					mstruct = mbak_integ;
				}
			}
		} else {
			restore_intervals(mstruct, mbak, vars, eo);
			CALCULATOR->endTemporaryStopMessages(true);
		}
		eo2.approximation = eo.approximation;
		if(b) {
			if(definite_integral && mstruct.containsFunctionId(FUNCTION_ID_INTEGRATE, true) <= 0 && ((den_check < 0 && count_ln(mstruct) <= (size_t) -den_check) || test_definite_ln(mstruct, m_interval, x_var, eo))) {
				CALCULATOR->endTemporaryStopMessages(true);
				MathStructure mstruct_lower(mstruct);
				if(m1.containsInfinity() || m2.containsInfinity()) {
					CALCULATOR->beginTemporaryStopMessages();
					EvaluationOptions eo3 = eo;
					eo3.approximation = APPROXIMATION_EXACT;
					if(m1.containsInfinity()) {
						b = mstruct_lower.calculateLimit(x_var, m1, eo3) && !mstruct_lower.containsInfinity();
					} else {
						mstruct_lower.replace(x_var, lower_limit, false, false, true);
						b = eo.approximation == APPROXIMATION_EXACT || !contains_incalc_function(mstruct_lower, eo);
					}
					MathStructure mstruct_upper(mstruct);
					if(m2.containsInfinity()) {
						b = mstruct_upper.calculateLimit(x_var, m2, eo3) && !mstruct_upper.containsInfinity();
					} else {
						mstruct_upper.replace(x_var, upper_limit, false, false, true);
						b = eo.approximation == APPROXIMATION_EXACT || !contains_incalc_function(mstruct_upper, eo);
					}
					if(b) {
						set(mstruct_upper);
						subtract(mstruct_lower);
						return true;
					} else {
						if(definite_integral > 0) {
							CALCULATOR->endTemporaryStopMessages(true);
							CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
							if(simplify_first) {
								set(mbak);
								replace(x_var, x_var_pre);
							}
							return false;
						} else {
							definite_integral = 0;
						}
					}
				} else {
					mstruct_lower.replace(x_var, lower_limit, false, false, true);
					if(eo.approximation == APPROXIMATION_EXACT || !m1.isNumber() || !m1.number().isReal() || !contains_incalc_function(mstruct_lower, eo)) {
						mstruct.replace(x_var, upper_limit, false, false, true);
						if(eo.approximation != APPROXIMATION_EXACT && m2.isNumber() && m2.number().isReal() && contains_incalc_function(mstruct, eo)) {
							mstruct = mbak;
						} else {
							set(mstruct);
							subtract(mstruct_lower);
							return true;
						}
					}
				}
			} else if(definite_integral < 0) {
				definite_integral = 0;
			} else if(definite_integral > 0) {
				mstruct = mbak;
			}
			if(!definite_integral) {
				set(mstruct);
				if(!m1.isUndefined()) replace(x_var, x_var_pre);
				CALCULATOR->endTemporaryStopMessages(true);
				add(CALCULATOR->getVariableById(VARIABLE_ID_C));
				return true;
			}
		}
		if(!b) {
			if(definite_integral <= 0) {
				CALCULATOR->endTemporaryStopMessages(true);
				CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
				if(simplify_first) {
					set(mbak);
					replace(x_var, x_var_pre);
				}
				return false;
			} else {
				mstruct = mbak;
			}
		}

		// Calculate definite integral with abs() or root() with appropriate intervals
		MathStructure *mabs = find_abs_x(mbak, x_var);
		if(mabs && !is_differentiable((*mabs)[0])) mabs = NULL;
		if(mabs && !(*mabs)[0].representsNonComplex(true)) {
			MathStructure mtest((*mabs)[0]);
			mtest.transformById(FUNCTION_ID_IM);
			if(mtest.compare(m_zero) != COMPARISON_RESULT_EQUAL) mabs = NULL;
		}
		if(mabs) {
			bool b_reversed = comparison_is_equal_or_greater(m2.compare(m1));
			MathStructure m0((*mabs)[0]);
			m0.transform(COMPARISON_EQUALS, m_zero);
			EvaluationOptions eo3 = eo;
			eo3.approximation = APPROXIMATION_EXACT;
			eo3.isolate_x = true;
			eo3.isolate_var = &x_var;
			m0.eval(eo3);
			bool b_exit = false;
			if(m0.isZero()) {
				m0 = (*mabs)[0];
				m0.replace(x_var, lower_limit, false, false, true);
				ComparisonResult cr1 = m0.compare(m_zero);
				if(COMPARISON_IS_EQUAL_OR_LESS(cr1)) {
					if(replace_abs(mbak, *mabs, false)) {
						mbak.replace(x_var, x_var_pre, false, false, true);
						CALCULATOR->endTemporaryStopMessages(true);
						if(mbak.integrate(lower_limit, upper_limit, x_var_pre, eo, false, true)) {
							set(mbak);
							return true;
						}
						if(simplify_first) set(mbak);
						return false;
					}
				} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr1)) {
					if(replace_abs(mbak, *mabs, true)) {
						mbak.replace(x_var, x_var_pre, false, false, true);
						CALCULATOR->endTemporaryStopMessages(true);
						if(mbak.integrate(lower_limit, upper_limit, x_var_pre, eo, false, true)) {
							set(mbak);
							return true;
						}
						if(simplify_first) set(mbak);
						return false;
					}
				}
			} else if(m0.isComparison() && m0.comparisonType() == COMPARISON_EQUALS && m0[0] == x_var && m0[1].contains(x_var, true) == 0) {
				CALCULATOR->endTemporaryStopMessages();
				CALCULATOR->beginTemporaryStopMessages();
				m0.setToChild(2, true);
				ComparisonResult cr1 = m0.compare(b_reversed ? m2 : m1);
				ComparisonResult cr2 = m0.compare(b_reversed ? m1 : m2);
				if(COMPARISON_IS_EQUAL_OR_GREATER(cr1) || COMPARISON_IS_EQUAL_OR_LESS(cr2)) {
					MathStructure mtest((*mabs)[0]);
					if(b_reversed) mtest.replace(x_var, COMPARISON_IS_EQUAL_OR_GREATER(cr1) ? lower_limit : upper_limit, false, false, true);
					else mtest.replace(x_var, COMPARISON_IS_EQUAL_OR_GREATER(cr1) ? upper_limit : lower_limit, false, false, true);
					ComparisonResult cr = mtest.compare(m_zero);
					if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
						if(replace_abs(mbak, *mabs, false)) {
							mbak.replace(x_var, x_var_pre, false, false, true);
							CALCULATOR->endTemporaryStopMessages(true);
							if(mbak.integrate(lower_limit, upper_limit, x_var_pre, eo, false, true)) {
								set(mbak);
								return true;
							}
							if(simplify_first) set(mbak);
							return false;
						}
					} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
						if(replace_abs(mbak, *mabs, true)) {
							mbak.replace(x_var, x_var_pre, false, false, true);
							CALCULATOR->endTemporaryStopMessages(true);
							if(mbak.integrate(lower_limit, upper_limit, x_var_pre, eo, false, true)) {
								set(mbak);
								return true;
							}
							if(simplify_first) set(mbak);
							return false;
						}
					}
				} else if(cr1 == COMPARISON_RESULT_LESS && cr2 == COMPARISON_RESULT_GREATER) {
					MathStructure mtest((*mabs)[0]);
					mtest.replace(x_var, lower_limit, false, false, true);
					ComparisonResult cr = mtest.compare(m_zero);
					MathStructure minteg1(mbak), minteg2;
					b = false;
					if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
						if(replace_abs(minteg1, *mabs, false)) b = true;
					} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
						if(replace_abs(minteg1, *mabs, true)) b = true;
					}
					if(b) {
						minteg1.replace(x_var, x_var_pre, false, false, true);
						CALCULATOR->beginTemporaryStopMessages();
						b = minteg1.integrate(lower_limit, m0, x_var_pre, eo, false, true);
						b_exit = CALCULATOR->endTemporaryStopMessages(NULL, NULL, MESSAGE_ERROR) == 0;
						if(!b_exit) b = false;
					}
					if(b) {
						mtest = (*mabs)[0];
						mtest.replace(x_var, upper_limit, false, false, true);
						cr = mtest.compare(m_zero);
						minteg2 = mbak;
						b = false;
						if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
							if(replace_abs(minteg2, *mabs, false)) b = true;
						} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
							if(replace_abs(minteg2, *mabs, true)) b = true;
						}
					}
					if(b) {
						minteg2.replace(x_var, x_var_pre, false, false, true);
						CALCULATOR->beginTemporaryStopMessages();
						b = minteg2.integrate(m0, upper_limit, x_var_pre, eo, false, true);
						b_exit = CALCULATOR->endTemporaryStopMessages(NULL, NULL, MESSAGE_ERROR) == 0;
						if(!b_exit) b = false;
					}
					if(b) {
						CALCULATOR->endTemporaryStopMessages(true);
						set(minteg1);
						add(minteg2);
						return true;
					}
				}
			} else if(m0.isLogicalOr() && m0.size() <= 10) {
				vector<MathStructure> zeroes;
				bool b = true;
				for(size_t i = 0; i < m0.size(); i++) {
					if(!m0[i].isComparison() || m0[i].comparisonType() != COMPARISON_EQUALS || m0[i][0] != x_var) {
						b = false;
						break;
					} else {
						for(size_t i2 = 0; i2 < zeroes.size(); i2++) {
							ComparisonResult cr = m0[i][1].compare(zeroes[i2]);
							if(cr == COMPARISON_RESULT_GREATER) {zeroes.insert(zeroes.begin() + i2, m0[i][1]); break;}
							else if(cr != COMPARISON_RESULT_LESS) {b = false; break;}
						}
						if(!b) break;
						if(zeroes.size() == i) zeroes.push_back(m0[i][1]);
					}
				}
				if(b) {
					vector<MathStructure> integs;
					for(size_t i = 0; i <= zeroes.size(); i++) {
						MathStructure mtest((*mabs)[0]);
						if(i == 0) {
							mtest.replace(x_var, b_reversed ? upper_limit : lower_limit, false, false, true);
						} else if(i == zeroes.size()) {
							mtest.replace(x_var, b_reversed ? lower_limit : upper_limit, false, false, true);
						} else {
							MathStructure mrepl(zeroes[i - 1]);
							mrepl += zeroes[i];
							mrepl *= nr_half;
							mtest.replace(x_var, mrepl, false, false, true);
						}
						ComparisonResult cr = mtest.compare(m_zero);
						MathStructure minteg(mbak);
						b = false;
						if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
							if(replace_abs(minteg, *mabs, false)) b = true;
						} else if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
							if(replace_abs(minteg, *mabs, true)) b = true;
						}
						if(b) {
							minteg.replace(x_var, x_var_pre, false, false, true);
							CALCULATOR->beginTemporaryStopMessages();
							b = minteg.integrate(i == 0 ? (b_reversed ? upper_limit : lower_limit) : zeroes[i - 1], i == zeroes.size() ? (b_reversed ? lower_limit : upper_limit) : zeroes[i], x_var_pre, eo, false, true);
							b_exit = CALCULATOR->endTemporaryStopMessages(NULL, NULL, MESSAGE_ERROR) == 0;
							if(!b_exit) b = false;
							if(b_reversed) minteg.negate();
						}
						if(!b) break;
						integs.push_back(minteg);
					}
					if(b) {
						for(size_t i = 0; i < integs.size(); i++) {
							if(i == 0) set(integs[i]);
							else add(integs[i], true);
						}
						CALCULATOR->endTemporaryStopMessages(true);
						return true;
					}
				}
			}
			if(b_exit) {
				CALCULATOR->endTemporaryStopMessages();
				CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
				if(simplify_first) {
					set(mbak);
					replace(x_var, x_var_pre);
				}
				return false;
			}
		}
		CALCULATOR->endTemporaryStopMessages();
		if(mstruct.containsInterval() && eo.approximation == APPROXIMATION_EXACT) {
			CALCULATOR->error(false, _("Unable to integrate the expression exactly."), NULL);
			if(simplify_first) {
				set(mstruct);
				replace(x_var, x_var_pre);
			}
			return false;
		}
	} else {
		CALCULATOR->endTemporaryStopMessages();
		CALCULATOR->endTemporaryStopMessages();
		restore_intervals(mstruct, mbak, vars, eo);
	}

	if(m1.isNumber() && m1.number().isReal() && m2.isNumber() && m2.number().isReal()) {

		if(eo.approximation != APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_APPROXIMATE;

		mstruct = mstruct_pre;
		mstruct.eval(eo2);

		eo2.interval_calculation = INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC;
		eo2.warn_about_denominators_assumed_nonzero = false;

		Number nr_begin, nr_end;
		bool b_reversed = false;
		if(m1.number().isGreaterThan(m2.number())) {
			nr_begin = m2.number();
			nr_end = m1.number();
			b_reversed = true;
		} else {
			nr_begin = m1.number();
			nr_end = m2.number();
		}
		if(eo.approximation != APPROXIMATION_EXACT) {
			Number nr;
			CALCULATOR->beginTemporaryStopMessages();
			if(romberg(mstruct, nr, x_var, eo2, nr_begin, nr_end)) {
				CALCULATOR->endTemporaryStopMessages();
				if(b_reversed) nr.negate();
				if(!force_numerical) CALCULATOR->error(false, _("Definite integral was approximated."), NULL);
				set(nr);
				return true;
			}
			CALCULATOR->endTemporaryStopMessages();
			CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
			if(simplify_first) {
				set(mbak);
				replace(x_var, x_var_pre);
			}
			return false;
		}
		Number nr_range(nr_end);
		nr_range -= nr_begin;
		MathStructure merr(mstruct);
		CALCULATOR->beginTemporaryStopMessages();
		eo2.expand = false;
		for(size_t i = 0; i < 4; i++) {
			if(merr.containsFunctionId(FUNCTION_ID_DIFFERENTIATE, true) > 0 || !merr.differentiate(x_var, eo2)) {
				break;
			}
			merr.calculatesub(eo2, eo2, true);
			if(CALCULATOR->aborted() || merr.countTotalChildren() > 200) {
				break;
			}
		}
		eo2.expand = eo.expand;
		CALCULATOR->endTemporaryStopMessages();
		if(merr.isZero()) {
			Number nr;
			if(numerical_integration(mstruct, nr, x_var, eo2, nr_begin, nr_end, 12, 0)) {
				if(b_reversed) nr.negate();
				set(nr);
				return true;
			}
		}
		CALCULATOR->error(false, _("Unable to integrate the expression exactly."), NULL);
		if(simplify_first) {
			set(mbak);
			replace(x_var, x_var_pre);
		}
		return false;
	}
	if(simplify_first) {
		set(mbak);
		replace(x_var, x_var_pre);
	}
	CALCULATOR->error(false, _("Unable to integrate the expression."), NULL);
	return false;
}



