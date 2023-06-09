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
#include "Calculator.h"
#include "BuiltinFunctions.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "Unit.h"
#include "Prefix.h"
#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool function_differentiable(MathFunction *o_function) {
	return (o_function->id() == FUNCTION_ID_SQRT || o_function->id() == FUNCTION_ID_ROOT || o_function->id() == FUNCTION_ID_CBRT || o_function->id() == FUNCTION_ID_LOG || o_function->id() == FUNCTION_ID_LOGN || o_function->id() == FUNCTION_ID_ARG || o_function->id() == FUNCTION_ID_GAMMA || o_function->id() == FUNCTION_ID_I_GAMMA || o_function->id() == FUNCTION_ID_BETA || o_function->id() == FUNCTION_ID_INCOMPLETE_BETA || o_function->id() == FUNCTION_ID_ABS || o_function->id() == FUNCTION_ID_FACTORIAL || o_function->id() == FUNCTION_ID_BESSELJ || o_function->id() == FUNCTION_ID_BESSELY || o_function->id() == FUNCTION_ID_ERF || o_function->id() == FUNCTION_ID_ERFI || o_function->id() == FUNCTION_ID_ERFC || o_function->id() == FUNCTION_ID_LOGINT || o_function->id() == FUNCTION_ID_POLYLOG || o_function->id() == FUNCTION_ID_EXPINT || o_function->id() == FUNCTION_ID_SININT || o_function->id() == FUNCTION_ID_COSINT || o_function->id() == FUNCTION_ID_SINHINT || o_function->id() == FUNCTION_ID_COSHINT || o_function->id() == FUNCTION_ID_FRESNEL_C || o_function->id() == FUNCTION_ID_FRESNEL_S || o_function->id() == FUNCTION_ID_ABS || o_function->id() == FUNCTION_ID_SIGNUM || o_function->id() == FUNCTION_ID_HEAVISIDE || o_function->id() == FUNCTION_ID_LAMBERT_W || o_function->id() == FUNCTION_ID_SINC || o_function->id() == FUNCTION_ID_SIN || o_function->id() == FUNCTION_ID_COS || o_function->id() == FUNCTION_ID_TAN || o_function->id() == FUNCTION_ID_ASIN || o_function->id() == FUNCTION_ID_ACOS || o_function->id() == FUNCTION_ID_ATAN || o_function->id() == FUNCTION_ID_SINH || o_function->id() == FUNCTION_ID_COSH || o_function->id() == FUNCTION_ID_TANH || o_function->id() == FUNCTION_ID_ASINH || o_function->id() == FUNCTION_ID_ACOSH || o_function->id() == FUNCTION_ID_ATANH || o_function->id() == FUNCTION_ID_STRIP_UNITS);
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
		// x'=1
		set(m_one);
		return true;
	}
	if(containsRepresentativeOf(x_var, true, true) == 0) {
		// f(a)'=0
		clear(true);
		return true;
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			// sum rule: (f(x)+g(x))'=f'+g'
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
				// a&&b=a*b
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
				// f(x)>=0 = sgn(heaviside(f)) : sgn(heaviside(f))'=f'*2*dirac(f)*dirac(heaviside(f))
				// f(x)<0 = sgn(-heaviside(f)) : sgn(heaviside(-f))'=f'*-2*dirac(f)*dirac(heaviside(-f))
				// f(x)<=0 = sgn(heaviside(-f)) : sgn(-heaviside(f))'=f'*-2*dirac(f)*dirac(heaviside(f))
				// f(x)>0 = sgn(-heaviside(-f)) : sgn(-heaviside(-f))'=f'*2*dirac(f)*dirac(heaviside(-f))
				ComparisonType ct = ct_comp;
				if(!CHILD(1).isZero()) CHILD(0) -= CHILD(1);
				SET_CHILD_MAP(0)
				MathStructure mstruct2(*this);
				MathStructure mstruct(*this);
				if(ct == COMPARISON_EQUALS_LESS || ct == COMPARISON_GREATER) negate();
				transformById(FUNCTION_ID_HEAVISIDE);
				transformById(FUNCTION_ID_DIRAC);
				mstruct2.transformById(FUNCTION_ID_DIRAC);
				multiply(mstruct2);
				if(ct == COMPARISON_EQUALS_GREATER || ct == COMPARISON_LESS) multiply_nocopy(new MathStructure(2, 1, 0));
				else multiply_nocopy(new MathStructure(-2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				break;
			}
			transformById(FUNCTION_ID_DIFFERENTIATE);
			addChild(x_var);
			addChild(m_one);
			addChild(m_undefined);
			return false;
		}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			// a'=0
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
				// (f(x)^a)'=f'*a*f^(a-1)
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				if(!CHILD(1).isNumber() || !CHILD(1).number().add(-1)) CHILD(1) += m_minus_one;
				if(!CHILD(0).equals(x_var)) {
					MathStructure *base_mstruct = new MathStructure(CHILD(0));
					base_mstruct->differentiate(x_var, eo);
					multiply_nocopy(base_mstruct);
				}
				multiply_nocopy(exp_mstruct);
			} else if(!x_in_base && x_in_exp) {
				// (a^f(x))'=f'*ln(a)*a^f
				MathStructure *exp_mstruct = new MathStructure(CHILD(1));
				exp_mstruct->differentiate(x_var, eo);
				if(!CHILD(0).isVariable() || CHILD(0).variable()->id() != VARIABLE_ID_E) {
					MathStructure *mstruct = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0), NULL);
					multiply_nocopy(mstruct);
				}
				multiply_nocopy(exp_mstruct);
			} else if(x_in_base && x_in_exp) {
				// (f(x)^g(x))'=f^g*(ln(f)*g'+f'/f*g)
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
				// sqrt(f)'=f'/(2*sqrt(f))
				SET_CHILD_MAP(0)
				raise_nocopy(new MathStructure(-1, 2, 0));
				if(!CHILD(0).equals(x_var)) {
					MathStructure *base_mstruct = new MathStructure(CHILD(0));
					base_mstruct->differentiate(x_var, eo);
					multiply_nocopy(base_mstruct);
				}
				multiply_nocopy(new MathStructure(1, 2, 0));
			} else if(o_function->id() == FUNCTION_ID_ROOT && THIS_VALID_ROOT) {
				// root(f,a)'=f'/(a*root(f,a)^(a-1))
				MathStructure *base_mstruct = new MathStructure(CHILD(0));
				MathStructure *mexp = new MathStructure(CHILD(1));
				mexp->negate();
				mexp->add(m_one);
				raise_nocopy(mexp);
				divide(CHILD(0)[1]);
				base_mstruct->differentiate(x_var, eo);
				multiply_nocopy(base_mstruct);
			} else if(o_function->id() == FUNCTION_ID_CBRT && SIZE == 1) {
				// cbrt(f)'=f'/(3*cbrt(x)^2)
				raise_nocopy(new MathStructure(-2, 1, 0));
				if(!CHILD(0).equals(x_var)) {
					MathStructure *base_mstruct = new MathStructure(CHILD(0)[0]);
					base_mstruct->differentiate(x_var, eo);
					multiply_nocopy(base_mstruct);
				}
				multiply_nocopy(new MathStructure(1, 3, 0));
			} else if((o_function->id() == FUNCTION_ID_LOG && SIZE == 1) || (o_function->id() == FUNCTION_ID_LOGN && SIZE == 2 && CHILD(1).isVariable() && CHILD(1).variable()->id() == VARIABLE_ID_E)) {
				if(CHILD(0) == x_var) {
					// ln(x)'=1/x
					setToChild(1, true);
					inverse();
				} else {
					// ln(f)'=f'/f
					MathStructure *mstruct = new MathStructure(CHILD(0));
					setToChild(1, true);
					inverse();
					mstruct->differentiate(x_var, eo);
					multiply_nocopy(mstruct);
				}
			} else if(o_function->id() == FUNCTION_ID_LOGN && SIZE == 2) {
				// log(f,g)'=(log(f)/log(g))'
				MathStructure *mstruct = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(1), NULL);
				setFunctionId(FUNCTION_ID_LOG);
				ERASE(1)
				divide_nocopy(mstruct);
				return differentiate(x_var, eo);
			} else if(o_function->id() == FUNCTION_ID_BETA && SIZE == 2) {
				// beta(f,g)')=beta(f,g)*((f'*digamma(f)-digamma(f+g))+g'*(digamma(g)-digamma(f+g)))
				MathStructure mdigamma(CHILD(0));
				mdigamma += CHILD(1);
				mdigamma.transformById(FUNCTION_ID_DIGAMMA);
				MathStructure *m1 = NULL, *m2 = NULL;
				if(CHILD(0).containsRepresentativeOf(x_var, true, true) != 0) {
					m1 = new MathStructure(CHILD(0));
					m1->transformById(FUNCTION_ID_DIGAMMA);
					m1->subtract(mdigamma);
					MathStructure *mstruct = new MathStructure(CHILD(0));
					mstruct->differentiate(x_var, eo);
					m1->multiply_nocopy(mstruct);
				}
				if(CHILD(1).containsRepresentativeOf(x_var, true, true) != 0) {
					m2 = new MathStructure(CHILD(1));
					m2->transformById(FUNCTION_ID_DIGAMMA);
					m2->subtract(mdigamma);
					MathStructure *mstruct = new MathStructure(CHILD(1));
					mstruct->differentiate(x_var, eo);
					m2->multiply_nocopy(mstruct);
				}
				if(m1) {
					if(m2) m1->add_nocopy(m2);
					multiply_nocopy(m1);
				} else {
					multiply_nocopy(m2);
				}
			} else if(o_function->id() == FUNCTION_ID_INCOMPLETE_BETA && SIZE == 3 && CHILD(0).containsRepresentativeOf(x_var, true, true) != 0 && CHILD(1).containsRepresentativeOf(x_var, true, true) == 0 && CHILD(2).containsRepresentativeOf(x_var, true, true) == 0) {
				// betainc(f,a,b)'=f^(a-1)*(1-f)^(b-1)*f'/beta(a,b)
				MathStructure *mbeta = new MathStructure(*this);
				mbeta->setFunctionId(FUNCTION_ID_BETA);
				mbeta->delChild(1);
				MathStructure *m1 = new MathStructure(CHILD(0));
				MathStructure *m2 = new MathStructure(CHILD(0));
				CHILD(1).ref();
				m1->raise_nocopy(&CHILD(1));
				m1->last() += nr_minus_one;
				m2->negate();
				m2->add(m_one);
				CHILD(2).ref();
				m2->raise_nocopy(&CHILD(2));
				m2->last() += nr_minus_one;
				SET_CHILD_MAP(0)
				differentiate(x_var, eo);
				multiply_nocopy(m1);
				multiply_nocopy(m2);
				divide_nocopy(mbeta);
			} else if(o_function->id() == FUNCTION_ID_ARG && SIZE == 1) {
				// arg(x)'=f'*-pi*dirac(f)
				MathStructure mstruct(CHILD(0));
				setFunctionId(FUNCTION_ID_DIRAC);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
				negate();
			} else if(o_function->id() == FUNCTION_ID_GAMMA && SIZE == 1) {
				// gamma(f)'=f'*digamma(f)
				MathStructure mstruct(CHILD(0));
				MathStructure mstruct2(*this);
				mstruct2.setFunctionId(FUNCTION_ID_DIGAMMA);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_I_GAMMA && SIZE == 2 && CHILD(0).containsRepresentativeOf(x_var, true, true) == 0) {
				// igamma(a,f)'=-e^(-f)*f^(a-1)*f'
				MathStructure mstruct(CHILD(1));
				mstruct.raise(CHILD(0));
				mstruct[1] += nr_minus_one;
				MathStructure mstruct2(CALCULATOR->getVariableById(VARIABLE_ID_E));
				mstruct2 ^= CHILD(1);
				mstruct2[1].negate();
				SET_CHILD_MAP(1);
				differentiate(x_var, eo);
				multiply(mstruct);
				multiply(mstruct2);
				negate();
			} else if(o_function->id() == FUNCTION_ID_FACTORIAL && SIZE == 1) {
				// (f!)'=gamma(f+1)'=(f+1)'*digamma(f+1)
				MathStructure mstruct(CHILD(0));
				CHILD(0) += m_one;
				MathStructure mstruct2(*this);
				mstruct2.setFunctionId(FUNCTION_ID_DIGAMMA);
				multiply(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_BESSELJ && SIZE == 2 && CHILD(0).isInteger()) {
				// besselj(n,f)'=f'*(besselj(n-1,f)-besselj(n+1,f))/2
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function->id() == FUNCTION_ID_BESSELY && SIZE == 2 && CHILD(0).isInteger()) {
				// bessely(n,f)'=f'*(bessely(n-1,f)-bessely(n+1,f))/2
				MathStructure mstruct(CHILD(1));
				MathStructure mstruct2(*this);
				CHILD(0) += m_minus_one;
				mstruct2[0] += m_one;
				subtract(mstruct2);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				multiply(nr_half);
			} else if(o_function->id() == FUNCTION_ID_ERF && SIZE == 1) {
				// erfi(f)'=f'*2/(e^(f^2)*sqrt(pi))
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
				// erfi(f)'=f'*2*e^(f^2)/sqrt(pi)
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
				// erfc(f)'=f'*-2/(e^(f^2)*sqrt(pi))
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
				// fresnels(f)'=f'*fresnels(f^2*pi/2)
				setFunctionId(FUNCTION_ID_SIN);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_FRESNEL_C && SIZE == 1) {
				// fresnelc(f)'=f'*cos(f^2*pi/2)
				setFunctionId(FUNCTION_ID_COS);
				MathStructure mstruct(CHILD(0));
				CHILD(0) ^= nr_two;
				CHILD(0) *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
				CHILD(0) *= nr_half;
				CHILD(0) *= CALCULATOR->getRadUnit();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_LOGINT && SIZE == 1) {
				// li(f)'=f'/ln(f)
				setFunctionId(FUNCTION_ID_LOG);
				MathStructure mstruct(CHILD(0));
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_POLYLOG && SIZE == 2 && CHILD(0).containsRepresentativeOf(x_var, true, true) == 0) {
				// Li(a,f)'=f'*Li(a-1,f)/f
				CHILD(0) += m_minus_one;
				MathStructure mstruct(CHILD(1));
				divide(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_EXPINT && SIZE == 1) {
				// Ei(f)'=f'*e^f/f
				MathStructure mexp(CALCULATOR->getVariableById(VARIABLE_ID_E));
				mexp ^= CHILD(0);
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				mexp *= mdiff;
				setToChild(1, true);
				inverse();
				multiply(mexp);
			} else if(o_function->id() == FUNCTION_ID_SININT && SIZE == 1) {
				// Si(f)'=f'*sinc(f)
				setFunctionId(FUNCTION_ID_SINC);
				MathStructure mdiff(CHILD(0));
				if(o_function->getDefaultValue(2) == "pi") CHILD(0) /= CALCULATOR->getVariableById(VARIABLE_ID_PI);
				mdiff.differentiate(x_var, eo);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_COSINT && SIZE == 1) {
				// Ci(f)'=f'*cos(f)/f
				setFunctionId(FUNCTION_ID_COS);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				CHILD(0) *= CALCULATOR->getRadUnit();
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_SINHINT && SIZE == 1) {
				// Shi(f)'=f'*sinh(f)/f
				setFunctionId(FUNCTION_ID_SINH);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_COSHINT && SIZE == 1) {
				// Chi(f)'=f'*cosh(f)/f
				setFunctionId(FUNCTION_ID_COSH);
				MathStructure marg(CHILD(0));
				MathStructure mdiff(CHILD(0));
				mdiff.differentiate(x_var, eo);
				divide(marg);
				multiply(mdiff);
			} else if(o_function->id() == FUNCTION_ID_ABS && SIZE == 1) {
				// abs(f)'=f'*f/abs(f)
				MathStructure mstruct(CHILD(0));
				inverse();
				multiply(mstruct);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_SIGNUM && SIZE == 2) {
				// sgn(f)'=f'*2*dirac(f)
				MathStructure mstruct(CHILD(0));
				ERASE(1)
				setFunctionId(FUNCTION_ID_DIRAC);
				multiply_nocopy(new MathStructure(2, 1, 0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_HEAVISIDE && SIZE == 1) {
				// heaviside(f)'=f'*dirac(f)
				MathStructure mstruct(CHILD(0));
				setFunctionId(FUNCTION_ID_DIRAC);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_LAMBERT_W && SIZE >= 1) {
				// lambertw(f)'=f'*lambertw(f)/(f*(1+lambertw(f)))
				MathStructure *mstruct = new MathStructure(*this);
				MathStructure *mstruct2 = new MathStructure(CHILD(0));
				mstruct->add(m_one);
				mstruct->multiply(CHILD(0));
				mstruct2->differentiate(x_var, eo);
				multiply_nocopy(mstruct2);
				divide_nocopy(mstruct);
			} else if(o_function->id() == FUNCTION_ID_SIN && SIZE == 1) {
				// sin(f)'=f'*cos(f)
				setFunctionId(FUNCTION_ID_COS);
				MathStructure mstruct(CHILD(0));
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function->id() == FUNCTION_ID_COS && SIZE == 1) {
				// cos(f)'=f'*sin(f)
				setFunctionId(FUNCTION_ID_SIN);
				MathStructure mstruct(CHILD(0));
				negate();
				mstruct.calculateDivide(CALCULATOR->getRadUnit(), eo);
				if(mstruct != x_var) {
					mstruct.differentiate(x_var, eo);
					multiply(mstruct);
				}
			} else if(o_function->id() == FUNCTION_ID_TAN && SIZE == 1) {
				// tan(f)'=f'*(1+tan(f)^2)
				setFunctionId(FUNCTION_ID_TAN);
				MathStructure mstruct(CHILD(0));
				raise(2);
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
				divide(CALCULATOR->getRadUnit());
			} else if(o_function->id() == FUNCTION_ID_SINH && SIZE == 1) {
				// sinh(f)'=f'*cosh(f)
				setFunctionId(FUNCTION_ID_COSH);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_COSH && SIZE == 1) {
				// cosh(f)'=f'*sinh(f)
				setFunctionId(FUNCTION_ID_SINH);
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function->id() == FUNCTION_ID_TANH && SIZE == 1) {
				// tanh(f)'=f'*(1-tanh(f)^2)
				setFunctionId(FUNCTION_ID_TANH);
				MathStructure mstruct(CHILD(0));
				raise(2);
				negate();
				add(nr_one);
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
			} else if(o_function->id() == FUNCTION_ID_ASIN && SIZE == 1) {
				// asin(f)'=f'/sqrt(1-f^2)
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ACOS && SIZE == 1) {
				// acos(f)'=-f'/sqrt(1-f^2)
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
				// asin(f)'=f'/(1+f^2)
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ASINH && SIZE == 1) {
				// asinh(f)'=f'/sqrt(1+f^2)
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				add(m_one);
				raise(Number(-1, 2));
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_ACOSH && SIZE == 1) {
				// acosh(f)'=f'/(sqrt(f-1)*sqrt(f+1))
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
				// atanh(f)'=f'/(1-x^2)
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				SET_CHILD_MAP(0);
				raise(2);
				negate();
				add(m_one);
				raise(m_minus_one);
				multiply(mstruct);
			} else if(o_function->id() == FUNCTION_ID_SINC && SIZE == 1) {
				// sinc(f)'=f'*(cos(f)/f-sin(f)/f^2)
				if(o_function->getDefaultValue(2) == "pi") CHILD(0) *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
				MathStructure m_cos(CALCULATOR->getFunctionById(FUNCTION_ID_COS), &CHILD(0), NULL);
				m_cos[0].multiply(CALCULATOR->getRadUnit());
				m_cos.divide(CHILD(0));
				MathStructure m_sin(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), &CHILD(0), NULL);
				m_sin[0].multiply(CALCULATOR->getRadUnit());
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
				// nounit(f)'=nounit(f')
				CHILD(0).differentiate(x_var, eo);
			} else if(o_function->id() == FUNCTION_ID_INTEGRATE && SIZE >= 4 && CHILD(3) == x_var && CHILD(1).isUndefined() && CHILD(2).isUndefined()) {
				// integrate(f,undefined,undefined,x)'=f
				SET_CHILD_MAP(0);
			} else if(o_function->id() == FUNCTION_ID_DIFFERENTIATE && (SIZE == 3 || (SIZE == 4 && CHILD(3).isUndefined())) && CHILD(1) == x_var) {
				// diff(f,x,a)'=diff(f,x,a+1)
				CHILD(2) += m_one;
			} else if(o_function->id() == FUNCTION_ID_DIFFERENTIATE && SIZE == 2 && CHILD(1) == x_var) {
				// diff(f,x)'=diff(f,x,2)
				addChild(MathStructure(2, 1, 0));
			} else if(o_function->id() == FUNCTION_ID_DIFFERENTIATE && SIZE == 1 && x_var == CALCULATOR->getVariableById(VARIABLE_ID_X)) {
				// diff(f)'=diff(f,x,2)
				addChild(x_var);
				addChild(MathStructure(2, 1, 0));
			} else {
				// calculate non-differentiable function and try again
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
			// product rule: (f(x)g(x))'=f'g+fg'
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
			// y'=0
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
			// differentiate value of variable
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
			// ?'=diff(?,x,1)
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

