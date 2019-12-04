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
	return (o_function == CALCULATOR->f_sqrt || o_function == CALCULATOR->f_root || o_function == CALCULATOR->f_cbrt || o_function == CALCULATOR->f_ln || o_function == CALCULATOR->f_logn || o_function == CALCULATOR->f_arg || o_function == CALCULATOR->f_gamma || o_function == CALCULATOR->f_beta || o_function == CALCULATOR->f_abs || o_function == CALCULATOR->f_factorial || o_function == CALCULATOR->f_besselj || o_function == CALCULATOR->f_bessely || o_function == CALCULATOR->f_erf || o_function == CALCULATOR->f_erfc || o_function == CALCULATOR->f_li || o_function == CALCULATOR->f_Li || o_function == CALCULATOR->f_Ei || o_function == CALCULATOR->f_Si || o_function == CALCULATOR->f_Ci || o_function == CALCULATOR->f_Shi || o_function == CALCULATOR->f_Chi || o_function == CALCULATOR->f_abs || o_function == CALCULATOR->f_signum || o_function == CALCULATOR->f_heaviside || o_function == CALCULATOR->f_lambert_w || o_function == CALCULATOR->f_sinc || o_function == CALCULATOR->f_sin || o_function == CALCULATOR->f_cos || o_function == CALCULATOR->f_tan || o_function == CALCULATOR->f_asin || o_function == CALCULATOR->f_acos || o_function == CALCULATOR->f_atan || o_function == CALCULATOR->f_sinh || o_function == CALCULATOR->f_cosh || o_function == CALCULATOR->f_tanh || o_function == CALCULATOR->f_asinh || o_function == CALCULATOR->f_acosh || o_function == CALCULATOR->f_atanh || o_function == CALCULATOR->f_stripunits);
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
		transform(CALCULATOR->f_diff);
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
				if(mtest.differentiate(x_var, eo) && mtest.containsFunction(CALCULATOR->f_diff, true) <= 0) {
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
				transform(CALCULATOR->f_heaviside);
				transform(CALCULATOR->f_dirac);
				mstruct2.transform(CALCULATOR->f_dirac);
				multiply(mstruct2);
				if(ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_EQUALS_LESS) multiply_nocopy(new MathStructure(2, 1, 0));
				else multiply_nocopy(new MathStructure(-2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				return true;
			}
			transform(CALCULATOR->f_diff);
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
				if(CHILD(0).isVariable() && CHILD(0).variable() == CALCULATOR->v_e) {
					multiply_nocopy(exp_mstruct);
				} else {
					MathStructure *mstruct = new MathStructure(CALCULATOR->f_ln, &CHILD(0), NULL);
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
				MathStructure *mstruct = new MathStructure(CALCULATOR->f_ln, &CHILD(0), NULL);
				mstruct->multiply_nocopy(exp_mstruct);
				mstruct->add_nocopy(base_mstruct);
				multiply_nocopy(mstruct);
			} else {
				clear(true);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_sqrt && SIZE == 1) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				raise(m_minus_one);
				multiply(nr_half);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function == CALCULATOR->f_root && THIS_VALID_ROOT) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				MathStructure *mexp = new MathStructure(CHILD(1));
				mexp->negate();
				mexp->add(m_one);
				raise_nocopy(mexp);
				divide(CHILD(0)[1]);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function == CALCULATOR->f_cbrt && SIZE == 1) {
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				raise(Number(-2, 1, 0));
				divide(nr_three);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if((o_function == CALCULATOR->f_ln && SIZE == 1) || (o_function == CALCULATOR->f_logn && SIZE == 2 && CHILD(1).isVariable() && CHILD(1).variable() == CALCULATOR->v_e)) {
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
			} else if(o_function == CALCULATOR->f_logn && SIZE == 2) {
				MathStructure *mstruct = new MathStructure(CALCULATOR->f_ln, &CHILD(1), NULL);
				setFunction(CALCULATOR->f_ln);
				ERASE(1)
				divide_nocopy(mstruct);
				return differentiate(x_var, eo);
			} else if(o_function == CALCULATOR->f_beta && SIZE == 2) {
				MathStructure mstruct(CHILD(0));
				setToChild(1, true);
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_arg && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setFunction(CALCULATOR->f_dirac);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(CALCULATOR->v_pi);
				negate();
			} else if(o_function == CALCULATOR->f_gamma && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				MathStructure mstruct2(*this);
				mstruct2.setFunction(CALCULATOR->f_digamma);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_factorial && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				CHILD(0) += m_one;
				MathStructure mstruct2(*this);
				mstruct2.setFunction(CALCULATOR->f_digamma);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_besselj && SIZE == 2 && CHILD(0).isInteger()) {
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function == CALCULATOR->f_bessely && SIZE == 2 && CHILD(0).isInteger()) {
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function == CALCULATOR->f_erf && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				mexp.negate();
				set(CALCULATOR->v_e, true);
				raise(mexp);
				multiply(nr_two);
				multiply(CALCULATOR->v_pi);
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_ERFI && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				set(CALCULATOR->v_e, true);
				raise(mexp);
				multiply(nr_two);
				multiply(CALCULATOR->v_pi);
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_erfc && SIZE == 1) {
				MathStructure mdiff(CHILD(0));
				MathStructure mexp(CHILD(0));
				mexp ^= nr_two;
				mexp.negate();
				set(CALCULATOR->v_e, true);
				raise(mexp);
				multiply(Number(-2, 1));
				multiply(CALCULATOR->v_pi);
				LAST ^= Number(-1, 2);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_S && SIZE == 1) {
				setFunction(CALCULATOR->f_sin);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->v_pi;
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_C && SIZE == 1) {
				setFunction(CALCULATOR->f_cos);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->v_pi;
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_li && SIZE == 1) {
				setFunction(CALCULATOR->f_ln);
				MathStructure mstruct(CHILD(0));
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_Li && SIZE == 2) {
				CHILD(0) += m_minus_one;
				MathStructure mstruct(CHILD(1));
				divide(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_Ei && SIZE == 1) {
				MathStructure mexp(CALCULATOR->v_e);
				mexp ^= CHILD(0);
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				mexp *= mdiff;
				setToChild(1, true);
				inverse();
				multiply(mexp);
			} else if(o_function == CALCULATOR->f_Si && SIZE == 1) {
				setFunction(CALCULATOR->f_sinc);
				CHILD_UPDATED(0)
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_Ci && SIZE == 1) {
				setFunction(CALCULATOR->f_cos);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				CHILD(0) *= CALCULATOR->getRadUnit();
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_Shi && SIZE == 1) {
				setFunction(CALCULATOR->f_sinh);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_Chi && SIZE == 1) {
				setFunction(CALCULATOR->f_cosh);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function == CALCULATOR->f_abs && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				inverse();
				multiply(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_signum && SIZE == 2) {
				MathStructure mstruct(CHILD(0));
				ERASE(1)
				setFunction(CALCULATOR->f_dirac);
				multiply_nocopy(new MathStructure(2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_heaviside && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setFunction(CALCULATOR->f_dirac);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_lambert_w && SIZE >= 1) {
				MathStructure *mstruct = new MathStructure(*this);
				MathStructure *mstruct2 = new MathStructure(CHILD(0));
				mstruct->add(m_one);
				mstruct->multiply(CHILD(0));
				mstruct2->differentiate(x_var, eo);
				multiply_nocopy(mstruct2);
				divide_nocopy(mstruct);
			} else if(o_function == CALCULATOR->f_sin && SIZE == 1) {
				setFunction(CALCULATOR->f_cos);
				MathStructure mstruct(CHILD(0));
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function == CALCULATOR->f_cos && SIZE == 1) {
				setFunction(CALCULATOR->f_sin);
				MathStructure mstruct(CHILD(0));
				negate();
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function == CALCULATOR->f_tan && SIZE == 1) {
				setFunction(CALCULATOR->f_tan);
				MathStructure mstruct(CHILD(0));
				raise(2);
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
				if(CALCULATOR->getRadUnit()) divide(CALCULATOR->getRadUnit());
			} else if(o_function == CALCULATOR->f_sinh && SIZE == 1) {
				setFunction(CALCULATOR->f_cosh);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_cosh && SIZE == 1) {
				setFunction(CALCULATOR->f_sinh);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function == CALCULATOR->f_tanh && SIZE == 1) {
				setFunction(CALCULATOR->f_tanh);
				MathStructure mstruct(CHILD(0));
				raise(2);
				negate();
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function == CALCULATOR->f_asin && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_acos && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				negate();
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_atan && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_asinh && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_acosh && SIZE == 1) {
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
			} else if(o_function == CALCULATOR->f_atanh && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_sinc && SIZE == 1) {
				// diff(x)*(cos(x)/x-sin(x)/x^2)
				MathStructure m_cos(CALCULATOR->f_cos, &CHILD(0), NULL);
				if(CALCULATOR->getRadUnit()) m_cos[0].multiply(CALCULATOR->getRadUnit());
				m_cos.divide(CHILD(0));
				MathStructure m_sin(CALCULATOR->f_sin, &CHILD(0), NULL);
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
			} else if(o_function == CALCULATOR->f_stripunits && SIZE == 1) {
				CHILD(0).differentiate(x_var, eo);
			} else if(o_function == CALCULATOR->f_integrate && SIZE >= 4 && CHILD(3) == x_var && CHILD(1).isUndefined() && CHILD(2).isUndefined()) {
				SET_CHILD_MAP(0);
			} else if(o_function == CALCULATOR->f_diff && (SIZE == 3 || (SIZE == 4 && CHILD(3).isUndefined())) && CHILD(1) == x_var) {
				CHILD(2) += m_one;
			} else if(o_function == CALCULATOR->f_diff && SIZE == 2 && CHILD(1) == x_var) {
				APPEND(MathStructure(2, 1, 0));
			} else if(o_function == CALCULATOR->f_diff && SIZE == 1 && x_var == (Variable*) CALCULATOR->v_x) {
				APPEND(x_var);
				APPEND(MathStructure(2, 1, 0));
			} else {
				if(!eo.calculate_functions || !calculateFunctions(eo, false)) {
					transform(CALCULATOR->f_diff);
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
				transform(CALCULATOR->f_diff);
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
					transform(CALCULATOR->f_diff);
					addChild(x_var);
					addChild(m_one);
					addChild(m_undefined);
					return false;
				}
			}
		}
		default: {
			transform(CALCULATOR->f_diff);
			addChild(x_var);
			addChild(m_one);
			addChild(m_undefined);
			return false;
		}
	}
	remove_zero_mul(*this);
	return true;
}

