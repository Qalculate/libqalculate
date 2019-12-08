/*
    Qalculate (library)

    Copyright (C) 2003-2007, 2008, 2016-2019  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "MathStructure.h"
#include "MathStructure-support.h"
#include "Calculator.h"
#include "BuiltinFunctions.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "Unit.h"
#include "Prefix.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool function_differentiable(MathFunction *o_function) {
	return (o_function->id() == FUNCTION_ID_SQRT || o_function->id() == FUNCTION_ID_ROOT || o_function->id() == FUNCTION_ID_CBRT || o_function->id() == FUNCTION_ID_LOG || o_function->id() == FUNCTION_ID_LOGN || o_function->id() == FUNCTION_ID_ARG || o_function->id() == FUNCTION_ID_GAMMA || o_function->id() == FUNCTION_ID_BETA || o_function->id() == FUNCTION_ID_ABS || o_function->id() == FUNCTION_ID_FACTORIAL || o_function->id() == FUNCTION_ID_BESSELJ || o_function->id() == FUNCTION_ID_BESSELY || o_function->id() == FUNCTION_ID_ERF || o_function->id() == FUNCTION_ID_ERFI || o_function->id() == FUNCTION_ID_ERFC || o_function->id() == FUNCTION_ID_LOGINT || o_function->id() == FUNCTION_ID_POLYLOG || o_function->id() == FUNCTION_ID_EXPINT || o_function->id() == FUNCTION_ID_SININT || o_function->id() == FUNCTION_ID_COSINT || o_function->id() == FUNCTION_ID_SINHINT || o_function->id() == FUNCTION_ID_COSHINT || o_function->id() == FUNCTION_ID_FRESNEL_C || o_function->id() == FUNCTION_ID_FRESNEL_S || o_function->id() == FUNCTION_ID_ABS || o_function->id() == FUNCTION_ID_SIGNUM || o_function->id() == FUNCTION_ID_HEAVISIDE || o_function->id() == FUNCTION_ID_LAMBERT_W || o_function->id() == FUNCTION_ID_SINC || o_function->id() == FUNCTION_ID_SIN || o_function->id() == FUNCTION_ID_COS || o_function->id() == FUNCTION_ID_TAN || o_function->id() == FUNCTION_ID_ASIN || o_function->id() == FUNCTION_ID_ACOS || o_function->id() == FUNCTION_ID_ATAN || o_function->id() == FUNCTION_ID_SINH || o_function->id() == FUNCTION_ID_COSH || o_function->id() == FUNCTION_ID_TANH || o_function->id() == FUNCTION_ID_ASINH || o_function->id() == FUNCTION_ID_ACOSH || o_function->id() == FUNCTION_ID_ATANH || o_function->id() == FUNCTION_ID_STRIP_UNITS);
}

bool is_differentiable(const MathStructure &m) {
	if(m.isFunction() && !function_differentiable(m.function())) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(!is_differentiable(m[i])) return false;
	}
	return true;
}

void remove_zero_mul(MathStructure &m) {
	if(m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].isZero()) {
				m.clear(true);
				return;
			}
		}
	}
	for(size_t i = 0; i < m.size(); i++) {
		remove_zero_mul(m[i]);
	}
}
bool MathStructure::differentiate(const MathStructure &x_var, const EvaluationOptions &eo) {
	if(CALCULATOR->aborted()) {
		transformById(FUNCTION_ID_DIFFERENTIATE);
		addChild(x_var);
		addChild(m_one);
		addChild(m_undefined);
		return false;
	}
	if(equals(x_var)) {
		set(m_one);
		return true;
	}
	if(containsRepresentativeOf(x_var, true, true) == 0) {
		clear(true);
		return true;
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE;) {
				if(CHILD(i).differentiate(x_var, eo)) {
					CHILD_UPDATED(i);
					if(SIZE > 1 && CHILD(i).isZero()) {ERASE(i);}
					else i++;
				} else i++;
			}
			if(SIZE == 1) SET_CHILD_MAP(0)
			break;
		}
		case STRUCT_LOGICAL_AND: {
			bool b = true;
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).isComparison()) {b = false; break;}
			}
			if(b) {
				MathStructure mtest(*this);
				mtest.setType(STRUCT_MULTIPLICATION);
				if(mtest.differentiate(x_var, eo) && mtest.containsFunctionId(FUNCTION_ID_DIFFERENTIATE, true) <= 0) {
					set(mtest);
					break;
				}
			}
		}
		case STRUCT_COMPARISON: {
			if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
				if(!CHILD(1).isZero()) CHILD(0) -= CHILD(1);
				SET_CHILD_MAP(0)
				if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_LESS) negate();
				MathStructure mstruct(*this);
				MathStructure mstruct2(*this);
				transformById(FUNCTION_ID_HEAVISIDE);
				transformById(FUNCTION_ID_DIRAC);
				mstruct2.transformById(FUNCTION_ID_DIRAC);
				multiply(mstruct2);
				if(ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_EQUALS_LESS) multiply_nocopy(new MathStructure(2, 1, 0));
				else multiply_nocopy(new MathStructure(-2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				return true;
			}
			transformById(FUNCTION_ID_DIFFERENTIATE);
			addChild(x_var);
			addChild(m_one);
			addChild(m_undefined);
			return false;
		}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			clear(true);
			break;
		}
		case STRUCT_POWER: {
			if(SIZE < 1) {
				clear(true);
				break;
			} else if(SIZE < 2) {
				setToChild(1, true);
				return differentiate(x_var, eo);
			}
			bool x_in_base = CHILD(0).containsRepresentativeOf(x_var, true, true) != 0;
			bool x_in_exp = CHILD(1).containsRepresentativeOf(x_var, true, true) != 0;
			if(x_in_base && !x_in_exp) {
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				if(!CHILD(1).isNumber() || !CHILD(1).number().add(-1)) CHILD(1) += m_minus_one;
				if(CHILD(0) == x_var) {
					multiply_nocopy(exp_mstruct);
				} else {
					MathStructure *base_mstruct = new MathStructure(CHILD(0));
					multiply_nocopy(exp_mstruct);
					base_mstruct->differentiate(x_var, eo);
					multiply_nocopy(base_mstruct);
				}
			} else if(!x_in_base && x_in_exp) {
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				exp_mstruct->differentiate(x_var, eo);
				if(CHILD(0).isVariable() && CHILD(0).variable()->id() == VARIABLE_ID_E) {
					multiply_nocopy(exp_mstruct);
				} else {
					MathStructure *mstruct = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0), NULL);
					multiply_nocopy(mstruct);
					multiply_nocopy(exp_mstruct);
				}
			} else if(x_in_base && x_in_exp) {
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				exp_mstruct->differentiate(x_var, eo);
				base_mstruct->differentiate(x_var, eo);
				base_mstruct->divide(CHILD(0));
				base_mstruct->multiply(CHILD(1));
				MathStructure *mstruct = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0), NULL);
				mstruct->multiply_nocopy(exp_mstruct);
				mstruct->add_nocopy(base_mstruct);
				multiply_nocopy(mstruct);
			} else {
				clear(true);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_SQRT && SIZE == 1) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				raise(m_minus_one);
				multiply(nr_half);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function->id() == FUNCTION_ID_ROOT && THIS_VALID_ROOT) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				MathStructure *mexp = new MathStructure(CHILD(1));
				mexp->negate();
				mexp->add(m_one);
				raise_nocopy(mexp);
				divide(CHILD(0)[1]);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function->id() == FUNCTION_ID_CBRT && SIZE == 1) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				raise(Number(-2, 1, 0));
				divide(nr_three);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if((o_function->id() == FUNCTION_ID_LOG && SIZE == 1) || (o_function->id() == FUNCTION_ID_LOGN && SIZE == 2 && CHILD(1).isVariable() && CHILD(1).variable()->id() == VARIABLE_ID_E)) {
				if(CHILD(0) == x_var) {
					setToChild(1, true);
					inverse();
				} else {
					MathStructure *mstruct = new MathStructure(CHILD(0));
					setToChild(1, true);
					inverse();
					mstruct->differentiate(x_var, eo);
					multiply_nocopy(mstruct);
				}
			} else if(o_function->id() == FUNCTION_ID_LOGN && SIZE == 2) {
				MathStructure *mstruct = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(1), NULL);
				setFunctionId(FUNCTION_ID_LOG);
				ERASE(1)
				divide_nocopy(mstruct);
				return differentiate(x_var, eo);
			} else if(o_function->id() == FUNCTION_ID_BETA && SIZE == 2) {
				MathStructure mstruct(CHILD(0));
				setToChild(1, true);
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ARG && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setFunctionId(FUNCTION_ID_DIRAC);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				negate();
			} else if(o_function->id() == FUNCTION_ID_GAMMA && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				MathStructure mstruct2(*this);
				mstruct2.setFunctionId(FUNCTION_ID_DIGAMMA);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_FACTORIAL && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				CHILD(0) += m_one;
				MathStructure mstruct2(*this);
				mstruct2.setFunctionId(FUNCTION_ID_DIGAMMA);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_BESSELJ && SIZE == 2 && CHILD(0).isInteger()) {
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function->id() == FUNCTION_ID_BESSELY && SIZE == 2 && CHILD(0).isInteger()) {
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function->id() == FUNCTION_ID_ERF && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				mexp.negate();
				set(CALCULATOR->getVariableById(VARIABLE_ID_E), true);
				raise(mexp);
				multiply(nr_two);
				multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_ERFI && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				set(CALCULATOR->getVariableById(VARIABLE_ID_E), true);
				raise(mexp);
				multiply(nr_two);
				multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_ERFC && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				mexp.negate();
				set(CALCULATOR->getVariableById(VARIABLE_ID_E), true);
				raise(mexp);
				multiply(Number(-2, 1));
				multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_S && SIZE == 1) {
				setFunctionId(FUNCTION_ID_SIN);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_C && SIZE == 1) {
				setFunctionId(FUNCTION_ID_COS);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_LOGINT && SIZE == 1) {
				setFunctionId(FUNCTION_ID_LOG);
				MathStructure mstruct(CHILD(0));
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_POLYLOG && SIZE == 2) {
				CHILD(0) += m_minus_one;
				MathStructure mstruct(CHILD(1));
				divide(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_EXPINT && SIZE == 1) {
				MathStructure mexp(CALCULATOR->getVariableById(VARIABLE_ID_E));
				mexp ^= CHILD(0);
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				mexp *= mdiff;
				setToChild(1, true);
				inverse();
				multiply(mexp);
			} else if(o_function->id() == FUNCTION_ID_SININT && SIZE == 1) {
				setFunctionId(FUNCTION_ID_SINC);
				CHILD_UPDATED(0)
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_COSINT && SIZE == 1) {
				setFunctionId(FUNCTION_ID_COS);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				CHILD(0) *= CALCULATOR->getRadUnit();
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_SINHINT && SIZE == 1) {
				setFunctionId(FUNCTION_ID_SINH);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_COSHINT && SIZE == 1) {
				setFunctionId(FUNCTION_ID_COSH);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_ABS && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				inverse();
				multiply(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_SIGNUM && SIZE == 2) {
				MathStructure mstruct(CHILD(0));
				ERASE(1)
				setFunctionId(FUNCTION_ID_DIRAC);
				multiply_nocopy(new MathStructure(2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_HEAVISIDE && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setFunctionId(FUNCTION_ID_DIRAC);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_LAMBERT_W && SIZE >= 1) {
				MathStructure *mstruct = new MathStructure(*this);
				MathStructure *mstruct2 = new MathStructure(CHILD(0));
				mstruct->add(m_one);
				mstruct->multiply(CHILD(0));
				mstruct2->differentiate(x_var, eo);
				multiply_nocopy(mstruct2);
				divide_nocopy(mstruct);
			} else if(o_function->id() == FUNCTION_ID_SIN && SIZE == 1) {
				setFunctionId(FUNCTION_ID_COS);
				MathStructure mstruct(CHILD(0));
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function->id() == FUNCTION_ID_COS && SIZE == 1) {
				setFunctionId(FUNCTION_ID_SIN);
				MathStructure mstruct(CHILD(0));
				negate();
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function->id() == FUNCTION_ID_TAN && SIZE == 1) {
				setFunctionId(FUNCTION_ID_TAN);
				MathStructure mstruct(CHILD(0));
				raise(2);
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
				if(CALCULATOR->getRadUnit()) divide(CALCULATOR->getRadUnit());
			} else if(o_function->id() == FUNCTION_ID_SINH && SIZE == 1) {
				setFunctionId(FUNCTION_ID_COSH);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_COSH && SIZE == 1) {
				setFunctionId(FUNCTION_ID_SINH);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function->id() == FUNCTION_ID_TANH && SIZE == 1) {
				setFunctionId(FUNCTION_ID_TANH);
				MathStructure mstruct(CHILD(0));
				raise(2);
				negate();
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function->id() == FUNCTION_ID_ASIN && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ACOS && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				negate();
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ATAN && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ASINH && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ACOSH && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				MathStructure mfac(*this);
				add(m_minus_one);
				raise(Number(-1, 2));
				mfac.add(m_one);
				mfac.raise(Number(-1, 2));
				multiply(mfac);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ATANH && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_SINC && SIZE == 1) {
				// diff(x)*(cos(x)/x-sin(x)/x^2)
				MathStructure m_cos(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &CHILD(0), NULL);
				if(CALCULATOR->getRadUnit()) m_cos[0].multiply(CALCULATOR->getRadUnit());
				m_cos.divide(CHILD(0));
				MathStructure m_sin(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), &CHILD(0), NULL);
				if(CALCULATOR->getRadUnit()) m_sin[0].multiply(CALCULATOR->getRadUnit());
				MathStructure mstruct(CHILD(0));
				mstruct.raise(Number(-2, 1, 0));
				m_sin.multiply(mstruct);
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				set(m_sin);
				negate();
				add(m_cos);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_STRIP_UNITS && SIZE == 1) {
				CHILD(0).differentiate(x_var, eo);
			} else if(o_function->id() == FUNCTION_ID_INTEGRATE && SIZE >= 4 && CHILD(3) == x_var && CHILD(1).isUndefined() && CHILD(2).isUndefined()) {
				SET_CHILD_MAP(0);
			} else if(o_function->id() == FUNCTION_ID_DIFFERENTIATE && (SIZE == 3 || (SIZE == 4 && CHILD(3).isUndefined())) && CHILD(1) == x_var) {
				CHILD(2) += m_one;
			} else if(o_function->id() == FUNCTION_ID_DIFFERENTIATE && SIZE == 2 && CHILD(1) == x_var) {
				APPEND(MathStructure(2, 1, 0));
			} else if(o_function->id() == FUNCTION_ID_DIFFERENTIATE && SIZE == 1 && x_var == CALCULATOR->getVariableById(VARIABLE_ID_X)) {
				APPEND(x_var);
				APPEND(MathStructure(2, 1, 0));
			} else {
				if(!eo.calculate_functions || !calculateFunctions(eo, false)) {
					transformById(FUNCTION_ID_DIFFERENTIATE);
					addChild(x_var);
					addChild(m_one);
					addChild(m_undefined);
					return false;
				} else {
					return differentiate(x_var, eo);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			MathStructure mstruct, vstruct;
			size_t idiv = 0;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isPower() && CHILD(i)[1].isNumber() && CHILD(i)[1].number().isNegative()) {
					if(idiv == 0) {
						if(CHILD(i)[1].isMinusOne()) {
							vstruct = CHILD(i)[0];
						} else {
							vstruct = CHILD(i);
							vstruct[1].number().negate();
						}
					} else {
						if(CHILD(i)[1].isMinusOne()) {
							vstruct.multiply(CHILD(i)[0], true);
						} else {
							vstruct.multiply(CHILD(i), true);
							vstruct[vstruct.size() - 1][1].number().negate();
						}
					}
					idiv++;
				}
			}
			if(idiv == SIZE) {
				set(-1, 1);
				MathStructure mdiv(vstruct);
				mdiv ^= MathStructure(2, 1, 0);
				mdiv.inverse();
				multiply(mdiv);
				MathStructure diff_u(vstruct);
				diff_u.differentiate(x_var, eo);
				multiply(diff_u, true);
				break;
			} else if(idiv > 0) {
				idiv = 0;
				for(size_t i = 0; i < SIZE; i++) {
					if(!CHILD(i).isPower() || !CHILD(i)[1].isNumber() || !CHILD(i)[1].number().isNegative()) {
						if(idiv == 0) {
							mstruct = CHILD(i);
						} else {
							mstruct.multiply(CHILD(i), true);
						}
						idiv++;
					}
				}
				MathStructure mdiv(vstruct);
				mdiv ^= MathStructure(2, 1, 0);
				MathStructure diff_v(vstruct);
				diff_v.differentiate(x_var, eo);
				MathStructure diff_u(mstruct);
				diff_u.differentiate(x_var, eo);
				vstruct *= diff_u;
				mstruct *= diff_v;
				set_nocopy(vstruct, true);
				subtract(mstruct);
				divide(mdiv);
				break;
			}
			if(SIZE > 2) {
				mstruct.set(*this);
				mstruct.delChild(mstruct.size());
				MathStructure mstruct2(LAST);
				MathStructure mstruct3(mstruct);
				mstruct.differentiate(x_var, eo);
				mstruct2.differentiate(x_var, eo);
				mstruct *= LAST;
				mstruct2 *= mstruct3;
				set(mstruct, true);
				add(mstruct2);
			} else {
				mstruct = CHILD(0);
				MathStructure mstruct2(CHILD(1));
				mstruct.differentiate(x_var, eo);
				mstruct2.differentiate(x_var, eo);
				mstruct *= CHILD(1);
				mstruct2 *= CHILD(0);
				set(mstruct, true);
				add(mstruct2);
			}
			break;
		}
		case STRUCT_SYMBOLIC: {
			if(representsNumber(true)) {
				clear(true);
			} else {
				transformById(FUNCTION_ID_DIFFERENTIATE);
				addChild(x_var);
				addChild(m_one);
				addChild(m_undefined);
				return false;
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if(eo.approximation != APPROXIMATION_EXACT || !o_variable->isApproximate()) {
					set(((KnownVariable*) o_variable)->get(), true);
					unformat(eo);
					return differentiate(x_var, eo);
				} else {
					transformById(FUNCTION_ID_DIFFERENTIATE);
					addChild(x_var);
					addChild(m_one);
					addChild(m_undefined);
					return false;
				}
			}
		}
		default: {
			transformById(FUNCTION_ID_DIFFERENTIATE);
			addChild(x_var);
			addChild(m_one);
			addChild(m_undefined);
			return false;
		}
	}
	remove_zero_mul(*this);
	return true;
}

