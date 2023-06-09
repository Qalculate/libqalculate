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
using std::ostream;
using std::endl;

bool factorize_fix_root_power(MathStructure &m) {
	if(!m[0].isFunction() || m[0].function()->id() != FUNCTION_ID_ROOT || !VALID_ROOT(m[0])) return false;
	if(m[1].isNumber() && m[1].number().isInteger() && !m[1].number().isMinusOne()) {
		if(m[1] == m[0][1]) {
			// root(x, a)^a=x
			m.setToChild(1, true);
			m.setToChild(1, true);
			return true;
		} else if(m[1].number().isIntegerDivisible(m[0][1].number())) {
			// root(x, a)^(2a)=x^2
			if(m[1].number().divide(m[0][1].number())) {
				m[0].setToChild(1, true);
				return true;
			}
		} else if(m[0][1].number().isIntegerDivisible(m[1].number())) {
			// root(x, 3a)^(a)=root(x, 3)
			if(m[0][1].number().divide(m[1].number())) {
				m.setToChild(1, true);
				m.childUpdated(2);
				return true;
			}
		}
	}
	return false;
}

bool sqrfree_differentiate(const MathStructure &mpoly, const MathStructure &x_var, MathStructure &mdiff, const EvaluationOptions &eo) {
	if(mpoly == x_var) {
		mdiff.set(1, 1, 0);
		return true;
	}
	switch(mpoly.type()) {
		case STRUCT_ADDITION: {
			mdiff.clear();
			mdiff.setType(STRUCT_ADDITION);
			for(size_t i = 0; i < mpoly.size(); i++) {
				mdiff.addChild(m_zero);
				if(!sqrfree_differentiate(mpoly[i], x_var, mdiff[i], eo)) return false;
			}
			mdiff.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_VARIABLE: {}
		case STRUCT_FUNCTION: {}
		case STRUCT_SYMBOLIC: {}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			mdiff.clear();
			break;
		}
		case STRUCT_POWER: {
			if(mpoly[0] == x_var) {
				mdiff = mpoly[1];
				mdiff.multiply(x_var);
				if(!mpoly[1].number().isTwo()) {
					mdiff[1].raise(mpoly[1]);
					mdiff[1][1].number()--;
				}
				mdiff.evalSort(true);
			} else {
				mdiff.clear();
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mpoly.size() < 1) {
				mdiff.clear();
				break;
			} else if(mpoly.size() < 2) {
				return sqrfree_differentiate(mpoly[0], x_var, mdiff, eo);
			}
			mdiff.clear();
			for(size_t i = 0; i < mpoly.size(); i++) {
				if(mpoly[i] == x_var) {
					if(mpoly.size() == 2) {
						if(i == 0) mdiff = mpoly[1];
						else mdiff = mpoly[0];
					} else {
						mdiff.setType(STRUCT_MULTIPLICATION);
						for(size_t i2 = 0; i2 < mpoly.size(); i2++) {
							if(i2 != i) {
								mdiff.addChild(mpoly[i2]);
							}
						}
					}
					break;
				} else if(mpoly[i].isPower() && mpoly[i][0] == x_var) {
					mdiff = mpoly;
					if(mpoly[i][1].number().isTwo()) {
						mdiff[i].setToChild(1);
					} else {
						mdiff[i][1].number()--;
					}
					if(mdiff[0].isNumber()) {
						mdiff[0].number() *= mpoly[i][1].number();
					} else {
						mdiff.insertChild(mpoly[i][1].number(), 1);
					}
					mdiff.evalSort();
					break;
				}
			}
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}
bool fix_root_pow(MathStructure &m, const MathStructure &xvar, const EvaluationOptions &eo) {
	if(m.isPower() && m[0].contains(xvar) && m[1].isNumber()) {
		return m.calculateRaiseExponent(eo);
	}
	bool b_ret = false;
	for(size_t i = 0; i < m.size(); i++) {
		if(fix_root_pow(m[i], xvar, eo)) {m.childUpdated(i + 1); b_ret = true;}
	}
	if(b_ret) m.calculatesub(eo, eo, false);
	return b_ret;
}

bool sqrfree_yun(const MathStructure &a, const MathStructure &xvar, MathStructure &factors, const EvaluationOptions &eo) {
	MathStructure w(a);
	MathStructure z;
	if(!sqrfree_differentiate(a, xvar, z, eo)) {
		return false;
	}
	MathStructure g;
	if(!MathStructure::gcd(w, z, g, eo)) {
		return false;
	}
	if(g.isOne()) {
		factors.addChild(a);
		return true;
	}
	MathStructure y;
	MathStructure tmp;
	do {
		tmp = w;
		if(!MathStructure::polynomialQuotient(tmp, g, xvar, w, eo)) {
			return false;
		}
		if(!MathStructure::polynomialQuotient(z, g, xvar, y, eo)) {
			return false;
		}
		if(!sqrfree_differentiate(w, xvar, tmp, eo)) {
			return false;
		}
		z = y;
		z.calculateSubtract(tmp, eo);
		if(!MathStructure::gcd(w, z, g, eo)) {
			return false;
		}
		factors.addChild(g);
	} while (!z.isZero());
	return true;
}

bool sqrfree_simple(const MathStructure &a, const MathStructure &xvar, MathStructure &factors, const EvaluationOptions &eo) {
	MathStructure w(a);
	while(true) {
		MathStructure z, zmod;
		if(!sqrfree_differentiate(w, xvar, z, eo)) return false;
		polynomial_smod(z, nr_three, zmod, eo);
		if(z == w) {
			factors.addChild(w);
			break;
		}
		MathStructure mgcd;
		if(!MathStructure::gcd(w, z, mgcd, eo)) return false;
		if(mgcd.isOne() || mgcd == w) {
			factors.addChild(w);
			break;
		}
		MathStructure tmp(w);
		if(!MathStructure::polynomialQuotient(tmp, mgcd, xvar, w, eo)) return false;
		if(!sqrfree_simple(mgcd, xvar, factors, eo)) return false;
	}
	return true;
}

void lcmcoeff(const MathStructure &e, const Number &l, Number &nlcm);
void lcmcoeff(const MathStructure &e, const Number &l, Number &nlcm) {
	if(e.isNumber() && e.number().isRational()) {
		nlcm = e.number().denominator();
		nlcm.lcm(l);
	} else if(e.isAddition()) {
		nlcm.set(1, 1, 0);
		for(size_t i = 0; i < e.size(); i++) {
			Number c(nlcm);
			lcmcoeff(e[i], c, nlcm);
		}
		nlcm.lcm(l);
	} else if(e.isMultiplication()) {
		nlcm.set(1, 1, 0);
		for(size_t i = 0; i < e.size(); i++) {
			Number c(nlcm);
			lcmcoeff(e[i], nr_one, c);
			nlcm *= c;
		}
		nlcm.lcm(l);
	} else if(e.isPower()) {
		if(IS_A_SYMBOL(e[0]) || e[0].isUnit()) {
			nlcm = l;
		} else {
			lcmcoeff(e[0], l, nlcm);
			nlcm ^= e[1].number();
		}
	} else {
		nlcm = l;
	}
}

void lcm_of_coefficients_denominators(const MathStructure &e, Number &nlcm) {
	return lcmcoeff(e, nr_one, nlcm);
}

void multiply_lcm(const MathStructure &e, const Number &lcm, MathStructure &mmul, const EvaluationOptions &eo) {
	if(e.isMultiplication()) {
		Number lcm_accum(1, 1);
		mmul.clear();
		for(size_t i = 0; i < e.size(); i++) {
			Number op_lcm;
			lcmcoeff(e[i], nr_one, op_lcm);
			if(mmul.isZero()) {
				multiply_lcm(e[i], op_lcm, mmul, eo);
				if(mmul.isOne()) mmul.clear();
			} else {
				mmul.multiply(m_one, true);
				multiply_lcm(e[i], op_lcm, mmul[mmul.size() - 1], eo);
				if(mmul[mmul.size() - 1].isOne()) {
					mmul.delChild(i + 1);
					if(mmul.size() == 1) mmul.setToChild(1);
				}
			}
			lcm_accum *= op_lcm;
		}
		Number lcm2(lcm);
		lcm2 /= lcm_accum;
		if(mmul.isZero()) {
			mmul = lcm2;
		} else if(!lcm2.isOne()) {
			if(mmul.size() > 0 && mmul[0].isNumber()) {
				mmul[0].number() *= lcm2;
			} else {
				mmul.multiply(lcm2, true);
			}
		}
		mmul.evalSort();
	} else if(e.isAddition()) {
		mmul.clear();
		for (size_t i = 0; i < e.size(); i++) {
			if(mmul.isZero()) {
				multiply_lcm(e[i], lcm, mmul, eo);
			} else {
				mmul.add(m_zero, true);
				multiply_lcm(e[i], lcm, mmul[mmul.size() - 1], eo);
			}
		}
		mmul.evalSort();
	} else if(e.isPower()) {
		if(IS_A_SYMBOL(e[0]) || e[0].isUnit()) {
			mmul = e;
			if(!lcm.isOne()) {
				mmul *= lcm;
				mmul.evalSort();
			}
		} else {
			mmul = e;
			Number lcm_exp = e[1].number();
			lcm_exp.recip();
			multiply_lcm(e[0], lcm ^ lcm_exp, mmul[0], eo);
			if(mmul[0] != e[0]) {
				mmul.calculatesub(eo, eo, false);
			}
		}
	} else if(e.isNumber()) {
		mmul = e;
		mmul.number() *= lcm;
	} else if(IS_A_SYMBOL(e) || e.isUnit()) {
		mmul = e;
		if(!lcm.isOne()) {
			mmul *= lcm;
			mmul.evalSort();
		}
	} else {
		mmul = e;
		if(!lcm.isOne()) {
			mmul.calculateMultiply(lcm, eo);
			mmul.evalSort();
		}
	}
}

//from GiNaC
bool sqrfree(MathStructure &mpoly, const EvaluationOptions &eo) {
	vector<MathStructure> symbols;
	collect_symbols(mpoly, symbols);
	return sqrfree(mpoly, symbols, eo);
}
bool sqrfree(MathStructure &mpoly, const vector<MathStructure> &symbols, const EvaluationOptions &eo) {

	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = true;
	eo2.warn_about_denominators_assumed_nonzero = false;
	eo2.reduce_divisions = true;
	eo2.keep_zero_units = false;
	eo2.do_polynomial_division = false;
	eo2.sync_units = false;
	eo2.expand = true;
	eo2.calculate_functions = false;
	eo2.protected_function = CALCULATOR->getFunctionById(FUNCTION_ID_SIGNUM);

	if(mpoly.size() == 0) {
		return true;
	}
	if(symbols.empty()) return true;

	size_t symbol_index = 0;
	if(symbols.size() > 1) {
		for(size_t i = 1; i < symbols.size(); i++) {
			if(mpoly.degree(symbols[symbol_index]).isGreaterThan(mpoly.degree(symbols[i]))) symbol_index = i;
		}
	}

	MathStructure xvar(symbols[symbol_index]);

	UnknownVariable *var = NULL;
	if(xvar.size() > 0) {
		var = new UnknownVariable("", format_and_print(xvar));
		var->setAssumptions(xvar);
		mpoly.replace(xvar, var);
		xvar = var;
	}

	Number nlcm;
	lcm_of_coefficients_denominators(mpoly, nlcm);

	MathStructure tmp;
	multiply_lcm(mpoly, nlcm, tmp, eo2);

	MathStructure factors;
	factors.clearVector();

	if(!sqrfree_yun(tmp, xvar, factors, eo2)) {
		if(var) tmp.replace(var, symbols[symbol_index]);
		factors.clearVector();
		factors.addChild(tmp);
	} else {
		if(var) tmp.replace(var, symbols[symbol_index]);
	}
	if(var) {mpoly.replace(var, symbols[symbol_index]); var->destroy();}

	vector<MathStructure> newsymbols;
	for(size_t i = 0; i < symbols.size(); i++) {
		if(i != symbol_index) newsymbols.push_back(symbols[i]);
	}

	if(newsymbols.size() > 0) {
		for(size_t i = 0; i < factors.size(); i++) {
			if(!sqrfree(factors[i], newsymbols, eo)) return false;
		}
	}

	mpoly.set(1, 1, 0);
	for(size_t i = 0; i < factors.size(); i++) {
		if(CALCULATOR->aborted()) return false;
		if(!factors[i].isOne()) {
			if(mpoly.isOne()) {
				mpoly = factors[i];
				if(i != 0) mpoly.raise(MathStructure((long int) i + 1, 1L, 0L));
			} else {
				mpoly.multiply(factors[i], true);
				mpoly[mpoly.size() - 1].raise(MathStructure((long int) i + 1, 1L, 0L));
			}
		}
	}

	if(CALCULATOR->aborted()) return false;
	if(mpoly.isZero()) {
		CALCULATOR->error(true, "mpoly is zero: %s. %s", format_and_print(tmp).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	MathStructure mquo;
	MathStructure mpoly_expand(mpoly);
	EvaluationOptions eo3 = eo;
	eo3.expand = true;
	mpoly_expand.calculatesub(eo3, eo3);

	MathStructure::polynomialQuotient(tmp, mpoly_expand, xvar, mquo, eo2);

	if(CALCULATOR->aborted()) return false;
	if(mquo.isZero()) {
		//CALCULATOR->error(true, "quo is zero: %s. %s", format_and_print(tmp).c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(newsymbols.size() > 0) {
		if(!sqrfree(mquo, newsymbols, eo)) return false;
	}
	if(!mquo.isOne()) {
		mpoly.multiply(mquo, true);
	}
	if(!nlcm.isOne()) {
		nlcm.recip();
		mpoly.multiply(nlcm, true);
	}

	eo3.expand = false;
	mpoly.calculatesub(eo3, eo3, false);

	return true;

}

bool MathStructure::integerFactorize() {
	if(!isNumber()) return false;
	vector<Number> factors;
	if(!o_number.factorize(factors)) return false;
	if(factors.size() <= 1) return true;
	clear(true);
	bool b_pow = false;
	Number *lastnr = NULL;
	for(size_t i = 0; i < factors.size(); i++) {
		if(lastnr && factors[i] == *lastnr) {
			if(!b_pow) {
				LAST.raise(m_one);
				b_pow = true;
			}
			LAST[1].number()++;
		} else {
			APPEND(factors[i]);
			b_pow = false;
		}
		lastnr = &factors[i];
	}
	m_type = STRUCT_MULTIPLICATION;
	return true;
}
size_t count_powers(const MathStructure &mstruct) {
	if(mstruct.isPower()) {
		if(mstruct[1].isInteger()) {
			bool overflow = false;
			int c = mstruct.number().intValue(&overflow) - 1;
			if(overflow) return 0;
			if(c < 0) return -c;
			return c;
		}
	}
	size_t c = 0;
	for(size_t i = 0; i < mstruct.size(); i++) {
		c += count_powers(mstruct[i]);
	}
	return c;
}

bool gather_factors(const MathStructure &mstruct, const MathStructure &x_var, MathStructure &madd, MathStructure &mmul, MathStructure &mexp, bool mexp_as_x2 = false) {
	madd.clear();
	if(mexp_as_x2) mexp = m_zero;
	else mexp = m_one;
	mmul = m_zero;
	if(mstruct == x_var) {
		mmul = m_one;
		return true;
	} else if(mexp_as_x2 && mstruct.isPower()) {
		if(mstruct[1].isNumber() && mstruct[1].number().isTwo() && mstruct[0] == x_var) {
			mexp = m_one;
			return true;
		}
	} else if(!mexp_as_x2 && mstruct.isPower() && mstruct[1].isInteger() && mstruct[0] == x_var) {
		if(mstruct[0] == x_var) {
			mexp = mstruct[1];
			mmul = m_one;
			return true;
		}
	} else if(mstruct.isMultiplication() && mstruct.size() >= 2) {
		bool b_x = false;
		bool b2 = false;
		size_t i_x = 0;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!b_x && mstruct[i] == x_var) {
				b_x = true;
				i_x = i;
			} else if(!b_x && mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && mstruct[i][0] == x_var) {
				b_x = true;
				b2 = true;
				i_x = i;
			} else if(!b_x && !mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isInteger() && mstruct[i][0] == x_var) {
				b_x = true;
				i_x = i;
				mexp = mstruct[i][1];
			} else if(mstruct[i].containsRepresentativeOf(x_var, true, true) != 0) {
				return false;
			}
		}
		if(!b_x) return false;
		if(mstruct.size() == 1) {
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
		} else {
			if(b2) {
				mexp = mstruct;
				mexp.delChild(i_x + 1, true);
			} else {
				mmul = mstruct;
				mmul.delChild(i_x + 1, true);
			}
		}
		return true;
	} else if(mstruct.isAddition()) {
		mmul.setType(STRUCT_ADDITION);
		if(mexp_as_x2) mexp.setType(STRUCT_ADDITION);
		madd.setType(STRUCT_ADDITION);
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i] == x_var) {
				if(mexp_as_x2 || mexp.isOne()) mmul.addChild(m_one);
				else return false;
			} else if(mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isTwo() && mstruct[i][0] == x_var) {
				mexp.addChild(m_one);
			} else if(!mexp_as_x2 && mstruct[i].isPower() && mstruct[i][1].isInteger() && mstruct[i][0] == x_var) {
				if(mmul.size() == 0) {
					mexp = mstruct[i][1];
				} else if(mexp != mstruct[i][1]) {
					return false;
				}
				mmul.addChild(m_one);
			} else if(mstruct[i].isMultiplication()) {
				bool b_x = false;
				bool b2 = false;
				size_t i_x = 0;
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(!b_x && mstruct[i][i2] == x_var) {
						if(!mexp_as_x2 && !mexp.isOne()) return false;
						i_x = i2;
						b_x = true;
					} else if(!b_x && mexp_as_x2 && mstruct[i][i2].isPower() && mstruct[i][i2][1].isNumber() && mstruct[i][i2][1].number().isTwo() && mstruct[i][i2][0] == x_var) {
						b2 = true;
						i_x = i2;
						b_x = true;
					} else if(!b_x && !mexp_as_x2 && mstruct[i][i2].isPower() && mstruct[i][i2][1].isInteger() && mstruct[i][i2][0] == x_var) {
						if(mmul.size() == 0) {
							mexp = mstruct[i][i2][1];
						} else if(mexp != mstruct[i][i2][1]) {
							return false;
						}
						i_x = i2;
						b_x = true;
					} else if(mstruct[i][i2].containsRepresentativeOf(x_var, true, true) != 0) {
						return false;
					}
				}
				if(b_x) {
					if(mstruct[i].size() == 1) {
						if(b2) mexp.addChild(m_one);
						else mmul.addChild(m_one);
					} else {
						if(b2) {
							mexp.addChild(mstruct[i]);
							mexp[mexp.size() - 1].delChild(i_x + 1, true);
							mexp.childUpdated(mexp.size());
						} else {
							mmul.addChild(mstruct[i]);
							mmul[mmul.size() - 1].delChild(i_x + 1, true);
							mmul.childUpdated(mmul.size());
						}
					}
				} else {
					madd.addChild(mstruct[i]);
				}
			} else if(mstruct[i].containsRepresentativeOf(x_var, true, true) != 0) {
				return false;
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
	}
	return false;
}

bool factorize_find_multiplier(const MathStructure &mstruct, MathStructure &mnew, MathStructure &factor_mstruct, bool only_units) {
	factor_mstruct.set(m_one);
	switch(mstruct.type()) {
		case STRUCT_ADDITION: {
			if(!only_units) {
				bool bfrac = false, bint = true;
				idm1(mstruct, bfrac, bint);
				if(bfrac || bint) {
					Number gcd(1, 1);
					idm2(mstruct, bfrac, bint, gcd);
					if((bint || bfrac) && !gcd.isOne()) {
						if(bfrac) gcd.recip();
						factor_mstruct.set(gcd);
					}
				}
			}
			size_t nfac = 0;
			if(mstruct.size() > 0) {
				size_t i = 0;
				const MathStructure *cur_mstruct;
				while(true) {
					if(mstruct[0].isMultiplication()) {
						if(i >= mstruct[0].size()) {
							break;
						}
						cur_mstruct = &mstruct[0][i];
					} else {
						cur_mstruct = &mstruct[0];
					}
					if(!cur_mstruct->containsInterval(true) && !cur_mstruct->isNumber() && (!only_units || cur_mstruct->isUnit_exp())) {
						const MathStructure *exp = NULL;
						const MathStructure *bas;
						if(cur_mstruct->isPower() && IS_REAL((*cur_mstruct)[1]) && !(*cur_mstruct)[0].isNumber()) {
							exp = cur_mstruct->exponent();
							bas = cur_mstruct->base();
						} else {
							bas = cur_mstruct;
						}
						bool b = true;
						for(size_t i2 = 1; i2 < mstruct.size(); i2++) {
							b = false;
							size_t i3 = 0;
							const MathStructure *cmp_mstruct;
							while(true) {
								if(mstruct[i2].isMultiplication()) {
									if(i3 >= mstruct[i2].size()) {
										break;
									}
									cmp_mstruct = &mstruct[i2][i3];
								} else {
									cmp_mstruct = &mstruct[i2];
								}
								if(cmp_mstruct->equals(*bas)) {
									if(exp) {
										exp = NULL;
									}
									b = true;
									break;
								} else if(cmp_mstruct->isPower() && IS_REAL((*cmp_mstruct)[1]) && cmp_mstruct->base()->equals(*bas)) {
									if(exp) {
										if(cmp_mstruct->exponent()->number().isLessThan(exp->number())) {
											exp = cmp_mstruct->exponent();
										}
										b = true;
										break;
									} else {
										b = true;
										break;
									}
								}
								if(!mstruct[i2].isMultiplication()) {
									break;
								}
								i3++;
							}
							if(!b) break;
						}
						if(b) {
							b = !factor_mstruct.isOne();
							if(exp) {
								MathStructure *mpow = new MathStructure(*bas);
								mpow->raise(*exp);
								if(factor_mstruct.isOne()) {
									factor_mstruct.set_nocopy(*mpow);
									mpow->unref();
								} else {
									factor_mstruct.multiply_nocopy(mpow, true);
								}
							} else {
								if(factor_mstruct.isOne()) factor_mstruct.set(*bas);
								else factor_mstruct.multiply(*bas, true);
							}
							nfac++;
							if(b) {
								size_t i3 = 0;
								const MathStructure *cmp_mstruct;
								b = false;
								while(true) {
									if(i3 >= factor_mstruct.size() - 1) {
										break;
									}
									cmp_mstruct = &factor_mstruct[i3];
									if(cmp_mstruct->equals(factor_mstruct.last())) {
										if(exp) {
											exp = NULL;
										}
										b = true;
										break;
									} else if(cmp_mstruct->isPower() && IS_REAL((*cmp_mstruct)[1]) && cmp_mstruct->base()->equals(factor_mstruct.last())) {
										if(exp) {
											if(cmp_mstruct->exponent()->number().isLessThan(exp->number())) {
												exp = cmp_mstruct->exponent();
											}
											b = true;
											break;
										} else {
											b = true;
											break;
										}
									}
									i3++;
								}
								if(b) {
									factor_mstruct.delChild(factor_mstruct.size(), true);
									nfac--;
								}
							}
						}
					}
					if(!mstruct[0].isMultiplication()) {
						break;
					}
					i++;
				}
			}
			if(!factor_mstruct.isOne()) {
				if(&mstruct != &mnew) mnew.set(mstruct);
				MathStructure *mfactor;
				size_t i = 0;
				int b_mul = factor_mstruct.isMultiplication();
				while(true) {
					if(b_mul > 0) {
						if(i >= factor_mstruct.size()) break;
						mfactor = &factor_mstruct[i];
					} else {
						mfactor = &factor_mstruct;
					}
					for(size_t i2 = 0; i2 < mnew.size(); i2++) {
						switch(mnew[i2].type()) {
							case STRUCT_NUMBER: {
								if(mfactor->isNumber()) {
									mnew[i2].number() /= mfactor->number();
								}
								break;
							}
							case STRUCT_POWER: {
								if(!IS_REAL(mnew[i2][1])) {
									if(mfactor->isNumber()) {
										mnew[i2].transform(STRUCT_MULTIPLICATION);
										mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
										mnew[i2][0].number() /= mfactor->number();
									} else {
										mnew[i2].set(m_one);
									}
								} else if(mfactor->isNumber()) {
									mnew[i2].transform(STRUCT_MULTIPLICATION);
									mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
									mnew[i2][0].number() /= mfactor->number();
								} else if(mfactor->isPower() && IS_REAL((*mfactor)[1])) {
									if(mfactor->equals(mnew[i2])) {
										mnew[i2].set(m_one);
									} else {
										mnew[i2][1].number() -= mfactor->exponent()->number();
										if(mnew[i2][1].number().isOne()) {
											mnew[i2].setToChild(1, true);
										} else if(factorize_fix_root_power(mnew[i2])) {
											mnew.childUpdated(i2 + 1);
										}
									}
								} else {
									mnew[i2][1].number() -= 1;
									if(mnew[i2][1].number().isOne()) {
										mnew[i2].setToChild(1);
									} else if(mnew[i2][1].number().isZero()) {
										mnew[i2].set(m_one);
									} else if(factorize_fix_root_power(mnew[i2])) {
										mnew.childUpdated(i2 + 1);
									}
								}
								break;
							}
							case STRUCT_MULTIPLICATION: {
								bool b = true;
								if(mfactor->isNumber() && (mnew[i2].size() < 1 || !mnew[i2][0].isNumber())) {
									mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
								}
								for(size_t i3 = 0; i3 < mnew[i2].size() && b; i3++) {
									switch(mnew[i2][i3].type()) {
										case STRUCT_NUMBER: {
											if(mfactor->isNumber()) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3].number() /= mfactor->number();
												}
												b = false;
											}
											break;
										}
										case STRUCT_POWER: {
											if(!IS_REAL(mnew[i2][i3][1])) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
													b = false;
												}
											} else if(mfactor->isPower() && IS_REAL((*mfactor)[1]) && mfactor->base()->equals(mnew[i2][i3][0])) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3][1].number() -= mfactor->exponent()->number();
													if(mnew[i2][i3][1].number().isOne()) {
														MathStructure mstruct2(mnew[i2][i3][0]);
														mnew[i2][i3] = mstruct2;
													} else if(mnew[i2][i3][1].number().isZero()) {
														mnew[i2].delChild(i3 + 1);
													} else if(factorize_fix_root_power(mnew[i2][i3])) {
														mnew[i2].childUpdated(i3 + 1);
														mnew.childUpdated(i2 + 1);
													}
												}
												b = false;
											} else if(mfactor->equals(mnew[i2][i3][0])) {
												if(mnew[i2][i3][1].number() == 2) {
													MathStructure mstruct2(mnew[i2][i3][0]);
													mnew[i2][i3] = mstruct2;
												} else if(mnew[i2][i3][1].number().isOne()) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3][1].number() -= 1;
													if(factorize_fix_root_power(mnew[i2][i3])) {
														mnew[i2].childUpdated(i3 + 1);
														mnew.childUpdated(i2 + 1);
													}
												}
												b = false;
											}
											break;
										}
										default: {
											if(mfactor->equals(mnew[i2][i3])) {
												mnew[i2].delChild(i3 + 1);
												b = false;
											}
										}
									}
								}
								if(mnew[i2].size() == 1) {
									MathStructure mstruct2(mnew[i2][0]);
									mnew[i2] = mstruct2;
								}
								if(b) {
									if(b_mul > 0 && nfac == 1 && &mstruct != &mnew) {
										b_mul = -1;
										mnew.set(mstruct);
									} else {
										return false;
									}
								}
								break;
							}
							default: {
								if(mfactor->isNumber()) {
									mnew[i2].transform(STRUCT_MULTIPLICATION);
									mnew[i2].insertChild(MathStructure(1, 1, 0), 1);
									mnew[i2][0].number() /= mfactor->number();
								} else {
									mnew[i2].set(m_one);
								}
							}
						}
					}
					if(b_mul > 0) {
						i++;
					} else if(b_mul < 0) {
						b_mul = 0;
					} else {
						break;
					}
				}
				return true;
			}
		}
		default: {}
	}
	return false;
}

bool polynomial_divide_integers(const vector<Number> &vnum, const vector<Number> &vden, vector<Number> &vquotient) {

	vquotient.clear();

	long int numdeg = vnum.size() - 1;
	long int dendeg = vden.size() - 1;
	Number dencoeff(vden[dendeg]);

	if(numdeg < dendeg) return false;

	vquotient.resize(numdeg - dendeg + 1, nr_zero);

	vector<Number> vrem = vnum;

	while(numdeg >= dendeg) {
		Number numcoeff(vrem[numdeg]);
		numdeg -= dendeg;
		if(!numcoeff.isIntegerDivisible(dencoeff)) break;
		numcoeff /= dencoeff;
		vquotient[numdeg] += numcoeff;
		for(size_t i = 0; i < vden.size(); i++) {
			vrem[numdeg + i] -= (vden[i] * numcoeff);
		}
		while(true) {
			if(vrem.back().isZero()) vrem.pop_back();
			else break;
			if(vrem.size() == 0) return true;
		}
		numdeg = (long int) vrem.size() - 1;
	}
	return false;
}

bool combination_factorize_is_complicated(MathStructure &m) {
	if(m.isPower()) {
		return combination_factorize_is_complicated(m[0]) || combination_factorize_is_complicated(m[1]);
	}
	return m.size() > 0;
}

bool combination_factorize(MathStructure &mstruct) {
	bool retval = false;
	switch(mstruct.type()) {
		case STRUCT_ADDITION: {
			bool b = false;
			// 5/y + x/y + z = (5 + x)/y + z
			MathStructure mstruct_units(mstruct);
			MathStructure mstruct_new(mstruct);
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(mstruct_units[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
						if(!mstruct_units[i][i2].isPower() || !mstruct_units[i][i2][1].hasNegativeSign()) {
							mstruct_units[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
					else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
					for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
						if(mstruct_new[i][i2].isPower() && mstruct_new[i][i2][1].hasNegativeSign()) {
							mstruct_new[i].delChild(i2 + 1);
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
					else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
				} else if(mstruct_new[i].isPower() && mstruct_new[i][1].hasNegativeSign()) {
					mstruct_new[i].set(1, 1, 0);
				} else {
					mstruct_units[i].setUndefined();
				}
			}
			for(size_t i = 0; i < mstruct_units.size(); i++) {
				if(!mstruct_units[i].isUndefined()) {
					for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
						if(mstruct_units[i2] == mstruct_units[i]) {
							mstruct_new[i].add(mstruct_new[i2], true);
							mstruct_new.delChild(i2 + 1);
							mstruct_units.delChild(i2 + 1);
							b = true;
						} else {
							i2++;
						}
					}
					if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
					else mstruct_new[i].multiply(mstruct_units[i], true);
				}
			}
			if(b) {
				if(mstruct_new.size() == 1) {
					mstruct.set(mstruct_new[0], true);
				} else {
					mstruct = mstruct_new;
				}
				b = false;
				retval = true;
			}
			if(mstruct.isAddition()) {
				// y*f(x) + z*f(x) = (y+z)*f(x)
				MathStructure mstruct_units(mstruct);
				MathStructure mstruct_new(mstruct);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!combination_factorize_is_complicated(mstruct_units[i][i2])) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(combination_factorize_is_complicated(mstruct_new[i][i2])) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(combination_factorize_is_complicated(mstruct_units[i])) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) mstruct.set(mstruct_new[0], true);
					else mstruct = mstruct_new;
					retval = true;
				}
			}
			if(mstruct.isAddition()) {
				// 5x + pi*x + 5y + xy = (5 + pi)x + 5y + xy
				MathStructure mstruct_units(mstruct);
				MathStructure mstruct_new(mstruct);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!mstruct_units[i][i2].containsType(STRUCT_UNIT, true) && !mstruct_units[i][i2].containsUnknowns()) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(mstruct_new[i][i2].containsType(STRUCT_UNIT, true) || mstruct_new[i][i2].containsUnknowns()) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(mstruct_units[i].containsType(STRUCT_UNIT, true) || mstruct_units[i].containsUnknowns()) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) mstruct.set(mstruct_new[0], true);
					else mstruct = mstruct_new;
					retval = true;
				}
			}
			//if(retval) return retval;
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(combination_factorize(mstruct[i])) {
					mstruct.childUpdated(i);
					b = true;
				}
			}
			if(b) retval = true;
		}
	}
	return retval;
}

bool MathStructure::factorize(const EvaluationOptions &eo_pre, bool unfactorize, int term_combination_levels, int max_msecs, bool only_integers, int recursive, struct timeval *endtime_p, const MathStructure &force_factorization, bool complete_square, bool only_sqrfree, int max_factor_degree) {

	if(CALCULATOR->aborted()) return false;
	struct timeval endtime;
	if(max_msecs > 0 && !endtime_p) {
#ifndef CLOCK_MONOTONIC
		gettimeofday(&endtime, NULL);
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		endtime.tv_sec = ts.tv_sec;
		endtime.tv_usec = ts.tv_nsec / 1000;
#endif
		endtime.tv_sec += max_msecs / 1000;
		long int usecs = endtime.tv_usec + (long int) (max_msecs % 1000) * 1000;
		if(usecs >= 1000000) {
			usecs -= 1000000;
			endtime.tv_sec++;
		}
		endtime.tv_usec = usecs;
		max_msecs = 0;
		endtime_p = &endtime;
	}

	EvaluationOptions eo = eo_pre;
	eo.sync_units = false;
	eo.structuring = STRUCTURING_NONE;
	if(unfactorize) {
		unformat(eo_pre);
		EvaluationOptions eo2 = eo;
		eo2.expand = true;
		eo2.combine_divisions = false;
		eo2.sync_units = false;
		calculatesub(eo2, eo2);
		do_simplification(*this, eo, true, false, true);
	} else if(term_combination_levels && isAddition()) {
		MathStructure *mdiv = new MathStructure;
		mdiv->setType(STRUCT_ADDITION);
		for(size_t i = 0; i < SIZE; ) {
			bool b = false;
			if(CHILD(i).isMultiplication()) {
				for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
					if(CHILD(i)[i2].isPower() && CHILD(i)[i2][1].hasNegativeSign()) {
						b = true;
						break;
					}
				}
			} else if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
				b = true;
			}
			if(b) {
				CHILD(i).ref();
				mdiv->addChild_nocopy(&CHILD(i));
				ERASE(i)
			} else {
				i++;
			}
		}
		if(mdiv->size() > 0) {
			bool b_ret = false;
			if(SIZE == 1 && recursive) {
				b_ret = CHILD(0).factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
			} else if(SIZE > 1) {
				b_ret = factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
			}
			if(mdiv->size() > 1) {
				// 5/y + x/y + z = (5 + x)/y + z
				MathStructure mstruct_units(*mdiv);
				MathStructure mstruct_new(*mdiv);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!mstruct_units[i][i2].isPower() || !mstruct_units[i][i2][1].hasNegativeSign()) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(mstruct_new[i][i2].isPower() && mstruct_new[i][i2][1].hasNegativeSign()) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(mstruct_new[i].isPower() && mstruct_new[i][1].hasNegativeSign()) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				bool b = false;
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) {
						mdiv->set_nocopy(mstruct_new[0], true);
					} else {
						mdiv->set_nocopy(mstruct_new);
					}
					b_ret = true;
				}
			}
			size_t index = 1;
			if(isAddition()) index = SIZE;
			if(index == 0) {
				set_nocopy(*mdiv);
				mdiv->unref();
			} else if(mdiv->isAddition()) {
				for(size_t i = 0; i < mdiv->size(); i++) {
					(*mdiv)[i].ref();
					add_nocopy(&(*mdiv)[i], true);
				}
				mdiv->unref();
			} else {
				add_nocopy(mdiv, true);
			}
			if(recursive) {
				for(; index < SIZE; index++) {
					b_ret = CHILD(index).factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree) || b_ret;
				}
			}
			return b_ret;
		}
		mdiv->unref();
	}

	MathStructure mden, mnum;
	evalSort(true);

	if(term_combination_levels >= -1 && isAddition() && isRationalPolynomial()) {
		MathStructure msqrfree(*this);
		eo.protected_function = CALCULATOR->getFunctionById(FUNCTION_ID_SIGNUM);
		if(sqrfree(msqrfree, eo)) {
			if((!only_sqrfree || msqrfree.isPower()) && !equals(msqrfree) && (!msqrfree.isMultiplication() || msqrfree.size() != 2 || (!(msqrfree[0].isNumber() && msqrfree[1].isAddition()) && !(msqrfree[1].isNumber() && msqrfree[0].isAddition())))) {
				MathStructure mcopy(msqrfree);
				EvaluationOptions eo2 = eo;
				eo2.expand = true;
				eo2.calculate_functions = false;
				CALCULATOR->beginTemporaryStopMessages();
				mcopy.calculatesub(eo2, eo2);
				CALCULATOR->endTemporaryStopMessages();
				bool b_equal = equals(mcopy);
				if(!b_equal && !CALCULATOR->aborted()) {
					MathStructure mcopy2(*this);
					CALCULATOR->beginTemporaryStopMessages();
					mcopy.calculatesub(eo2, eo2, true);
					mcopy2.calculatesub(eo2, eo2, true);
					CALCULATOR->endTemporaryStopMessages();
					b_equal = mcopy.equals(mcopy2);
				}
				if(!b_equal) {
					eo.protected_function = eo_pre.protected_function;
					if(CALCULATOR->aborted()) return false;
					CALCULATOR->error(true, "factorized result is wrong: %s != %s. %s", format_and_print(msqrfree).c_str(), format_and_print(*this).c_str(), _("This is a bug. Please report it."), NULL);
				} else {
					eo.protected_function = eo_pre.protected_function;
					set(msqrfree);
					if(!isAddition()) {
						if(isMultiplication()) flattenMultiplication(*this);
						if(isMultiplication() && SIZE >= 2 && CHILD(0).isNumber()) {
							for(size_t i = 1; i < SIZE; i++) {
								if(CHILD(i).isNumber()) {
									CHILD(i).number() *= CHILD(0).number();
									CHILD(0).set(CHILD(i));
									delChild(i);
								} else if(CHILD(i).isPower() && CHILD(i)[0].isMultiplication() && CHILD(i)[0].size() >= 2 && CHILD(i)[0][0].isNumber() && CHILD(i)[0][0].number().isRational() && !CHILD(i)[0][0].number().isInteger() && CHILD(i)[1].isInteger()) {
									CHILD(i)[0][0].number().raise(CHILD(i)[1].number());
									CHILD(0).number().multiply(CHILD(i)[0][0].number());
									CHILD(i)[0].delChild(1);
									if(CHILD(i)[0].size() == 1) CHILD(i)[0].setToChild(1, true);
								}
							}
							if(SIZE > 1 && CHILD(0).isOne()) {
								ERASE(0);
							}
							if(SIZE == 1) SET_CHILD_MAP(0);
						}
						if(isMultiplication() && SIZE >= 2 && CHILD(0).isNumber() && CHILD(0).number().isRational() && !CHILD(0).number().isInteger()) {
							Number den = CHILD(0).number().denominator();
							for(size_t i = 1; i < SIZE; i++) {
								if(CHILD(i).isAddition()) {
									bool b = true;
									for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
										if(CHILD(i)[i2].isNumber()) {
											if(!CHILD(i)[i2].number().isIntegerDivisible(den)) {b = false; break;}
										} else if(CHILD(i)[i2].isMultiplication() && CHILD(i)[i2][0].isNumber()) {
											if(!CHILD(i)[i2][0].number().isIntegerDivisible(den)) {b = false; break;}
										} else {
											b = false;
											break;
										}
									}
									if(b) {
										for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
											if(CHILD(i)[i2].isNumber()) {
												CHILD(i)[i2].number().divide(den);
											} else if(CHILD(i)[i2].isMultiplication()) {
												CHILD(i)[i2][0].number().divide(den);
												if(CHILD(i)[i2][0].isOne() && CHILD(i)[i2].size() > 1) {
													CHILD(i)[i2].delChild(1);
													if(CHILD(i)[i2].size() == 1) {
														CHILD(i)[i2].setToChild(1, true);
													}
												}
											}
										}
										CHILD(0).set(CHILD(0).number().numerator(), true);
										if(SIZE > 1 && CHILD(0).isOne()) {
											ERASE(0);
										}
										if(SIZE == 1) SET_CHILD_MAP(0);
										break;
									}
								}
							}
						}
						if(isMultiplication()) {
							for(size_t i = 0; i < SIZE; i++) {
								if(CHILD(i).isPower() && CHILD(i)[1].isInteger()) {
									if(CHILD(i)[0].isAddition()) {
										bool b = true;
										for(size_t i2 = 0; i2 < CHILD(i)[0].size(); i2++) {
											if((!CHILD(i)[0][i2].isNumber() || !CHILD(i)[0][i2].number().isNegative()) && (!CHILD(i)[0][i2].isMultiplication() || CHILD(i)[0][i2].size() < 2 || !CHILD(i)[0][i2][0].isNumber() || !CHILD(i)[0][i2][0].number().isNegative())) {
												b = false;
												break;
											}
										}
										if(b) {
											for(size_t i2 = 0; i2 < CHILD(i)[0].size(); i2++) {
												if(CHILD(i)[0][i2].isNumber()) {
													CHILD(i)[0][i2].number().negate();
												} else {
													CHILD(i)[0][i2][0].number().negate();
													if(CHILD(i)[0][i2][0].isOne() && CHILD(i)[0][i2].size() > 1) {
														CHILD(i)[0][i2].delChild(1);
														if(CHILD(i)[0][i2].size() == 1) {
															CHILD(i)[0][i2].setToChild(1, true);
														}
													}
												}
											}
											if(CHILD(i)[1].number().isOdd()) {
												if(CHILD(0).isNumber()) CHILD(0).number().negate();
												else {
													PREPEND(MathStructure(-1, 1, 0));
													i++;
												}
											}
										}
									} else if(CHILD(i)[0].isMultiplication() && CHILD(i)[0].size() >= 2 && CHILD(i)[0][0].isNumber() && CHILD(i)[0][0].number().isNegative()) {
										CHILD(i)[0][0].number().negate();
										if(CHILD(i)[0][0].isOne() && CHILD(i)[0].size() > 1) {
											CHILD(i)[0].delChild(1);
											if(CHILD(i)[0].size() == 1) {
												CHILD(i)[0].setToChild(1, true);
											}
										}
										if(CHILD(i)[1].number().isOdd()) {
											if(CHILD(0).isNumber()) CHILD(0).number().negate();
											else {
												PREPEND(MathStructure(-1, 1, 0));
												i++;
											}
										}
									}
								} else if(CHILD(i).isAddition()) {
									bool b = true;
									for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
										if((!CHILD(i)[i2].isNumber() || !CHILD(i)[i2].number().isNegative()) && (!CHILD(i)[i2].isMultiplication() || CHILD(i)[i2].size() < 2 || !CHILD(i)[i2][0].isNumber() || !CHILD(i)[i2][0].number().isNegative())) {
											b = false;
											break;
										}
									}
									if(b) {
										for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
											if(CHILD(i)[i2].isNumber()) {
												CHILD(i)[i2].number().negate();
											} else {
												CHILD(i)[i2][0].number().negate();
												if(CHILD(i)[i2][0].isOne() && CHILD(i)[i2].size() > 1) {
													CHILD(i)[i2].delChild(1);
													if(CHILD(i)[i2].size() == 1) {
														CHILD(i)[i2].setToChild(1, true);
													}
												}
											}
										}
										if(CHILD(0).isNumber()) CHILD(0).number().negate();
										else {
											PREPEND(MathStructure(-1, 1, 0));
											i++;
										}
									}
								}
							}
							if(SIZE > 1 && CHILD(0).isOne()) {
								ERASE(0);
							}
							if(SIZE == 1) SET_CHILD_MAP(0);
						}
						if(isPower() && CHILD(1).isInteger()) {
							if(CHILD(0).isAddition()) {
								bool b = true;
								for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
									if((!CHILD(0)[i2].isNumber() || !CHILD(0)[i2].number().isNegative()) && (!CHILD(0)[i2].isMultiplication() || CHILD(0)[i2].size() < 2 || !CHILD(0)[i2][0].isNumber() || !CHILD(0)[i2][0].number().isNegative())) {
										b = false;
										break;
									}
								}
								if(b) {
									for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
										if(CHILD(0)[i2].isNumber()) {
											CHILD(0)[i2].number().negate();
										} else {
											CHILD(0)[i2][0].number().negate();
											if(CHILD(0)[i2][0].isOne() && CHILD(0)[i2].size() > 1) {
												CHILD(0)[i2].delChild(1);
												if(CHILD(0)[i2].size() == 1) {
													CHILD(0)[i2].setToChild(1, true);
												}
											}
										}
									}
									if(CHILD(1).number().isOdd()) {
										multiply(MathStructure(-1, 1, 0));
										CHILD_TO_FRONT(1)
									}
								}
							} else if(CHILD(0).isMultiplication() && CHILD(0).size() >= 2 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isNegative()) {
								CHILD(0)[0].number().negate();
								if(CHILD(0)[0].isOne() && CHILD(0).size() > 1) {
									CHILD(0).delChild(1);
									if(CHILD(0).size() == 1) {
										CHILD(0).setToChild(1, true);
									}
								}
								if(CHILD(1).number().isOdd()) {
									multiply(MathStructure(-1, 1, 0));
									CHILD_TO_FRONT(1)
								}
							}
						}
					}
					evalSort(true);
					factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					return true;
				}
			}
		}
		eo.protected_function = eo_pre.protected_function;
	}

	switch(type()) {
		case STRUCT_ADDITION: {

			if(CALCULATOR->aborted()) return false;
			if(term_combination_levels >= -1 && !only_sqrfree && max_factor_degree != 0) {

				if(SIZE <= 3 && SIZE > 1) {
					MathStructure *xvar = NULL;
					Number nr2(1, 1);
					if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
						xvar = &CHILD(0)[0];
					} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber()) {
						if(CHILD(0)[1].isPower()) {
							if(CHILD(0)[1][0].size() == 0 && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isTwo()) {
								xvar = &CHILD(0)[1][0];
								nr2.set(CHILD(0)[0].number());
							}
						}
					}
					if(xvar) {
						bool factorable = false;
						Number nr1, nr0;
						if(SIZE == 2 && CHILD(1).isNumber()) {
							factorable = true;
							nr0 = CHILD(1).number();
						} else if(SIZE == 3 && CHILD(2).isNumber()) {
							nr0 = CHILD(2).number();
							if(CHILD(1).isMultiplication()) {
								if(CHILD(1).size() == 2 && CHILD(1)[0].isNumber() && xvar->equals(CHILD(1)[1])) {
									nr1 = CHILD(1)[0].number();
									factorable = true;
								}
							} else if(xvar->equals(CHILD(1))) {
								nr1.set(1, 1, 0);
								factorable = true;
							}
						}
						if(factorable) {
							Number nr4ac(4, 1, 0);
							nr4ac *= nr2;
							nr4ac *= nr0;
							Number nr2a(2, 1, 0);
							nr2a *= nr2;
							Number sqrtb24ac(nr1);
							sqrtb24ac.raise(nr_two);
							sqrtb24ac -= nr4ac;
							if(sqrtb24ac.isNegative()) factorable = false;
							MathStructure mstructb24(sqrtb24ac);
							if(factorable) {
								if(!only_integers) {
									if(eo.approximation == APPROXIMATION_EXACT && !sqrtb24ac.isApproximate()) {
										sqrtb24ac.raise(nr_half);
										if(sqrtb24ac.isApproximate()) {
											mstructb24.raise(nr_half);
										} else {
											mstructb24.set(sqrtb24ac);
										}
									} else {
										mstructb24.number().raise(nr_half);
									}
								} else {
									mstructb24.number().raise(nr_half);
									if((!sqrtb24ac.isApproximate() && mstructb24.number().isApproximate()) || (sqrtb24ac.isInteger() && !mstructb24.number().isInteger())) {
										factorable = false;
									}
								}
							}
							if(factorable) {
								MathStructure m1(nr1), m2(nr1);
								Number mul1(1, 1), mul2(1, 1);
								if(mstructb24.isNumber()) {
									m1.number() += mstructb24.number();
									m1.number() /= nr2a;
									if(m1.number().isRational() && !m1.number().isInteger()) {
										mul1 = m1.number().denominator();
										m1.number() *= mul1;
									}
									m2.number() -= mstructb24.number();
									m2.number() /= nr2a;
									if(m2.number().isRational() && !m2.number().isInteger()) {
										mul2 = m2.number().denominator();
										m2.number() *= mul2;
									}
								} else {
									m1.calculateAdd(mstructb24, eo);
									m1.calculateDivide(nr2a, eo);
									if(m1.isNumber()) {
										if(m1.number().isRational() && !m1.number().isInteger()) {
											mul1 = m1.number().denominator();
											m1.number() *= mul1;
										}
									} else {
										bool bint = false, bfrac = false;
										idm1(m1, bfrac, bint);
										if(bfrac) {
											idm2(m1, bfrac, bint, mul1);
											idm3(m1, mul1, true);
										}
									}
									m2.calculateSubtract(mstructb24, eo);
									m2.calculateDivide(nr2a, eo);
									if(m2.isNumber()) {
										if(m2.number().isRational() && !m2.number().isInteger()) {
											mul2 = m2.number().denominator();
											m2.number() *= mul2;
										}
									} else {
										bool bint = false, bfrac = false;
										idm1(m2, bfrac, bint);
										if(bfrac) {
											idm2(m2, bfrac, bint, mul2);
											idm3(m2, mul2, true);
										}
									}
								}
								nr2 /= mul1;
								nr2 /= mul2;
								if(m1 == m2 && mul1 == mul2) {
									MathStructure xvar2(*xvar);
									if(!mul1.isOne()) xvar2 *= mul1;
									set(m1);
									add(xvar2, true);
									raise(MathStructure(2, 1, 0));
									if(!nr2.isOne()) {
										multiply(nr2);
									}
								} else {
									m1.add(*xvar, true);
									if(!mul1.isOne()) m1[m1.size() - 1] *= mul1;
									m2.add(*xvar, true);
									if(!mul2.isOne()) m2[m2.size() - 1] *= mul2;
									clear(true);
									m_type = STRUCT_MULTIPLICATION;
									if(!nr2.isOne()) {
										APPEND_NEW(nr2);
									}
									APPEND(m1);
									APPEND(m2);
								}
								EvaluationOptions eo2 = eo;
								eo2.expand = false;
								calculatesub(eo2, eo2, false);
								evalSort(true);
								return true;
							}
						}
					}
				}

				MathStructure *factor_mstruct = new MathStructure(1, 1, 0);
				MathStructure mnew;
				if(factorize_find_multiplier(*this, mnew, *factor_mstruct) && !factor_mstruct->isZero() && !mnew.isZero()) {
					mnew.factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					factor_mstruct->factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					clear(true);
					m_type = STRUCT_MULTIPLICATION;
					APPEND_REF(factor_mstruct);
					APPEND(mnew);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					calculatesub(eo2, eo2, false);
					factor_mstruct->unref();
					evalSort(true);
					return true;
				}
				factor_mstruct->unref();

				if(SIZE > 1 && CHILD(SIZE - 1).isNumber() && CHILD(SIZE - 1).number().isInteger() && max_factor_degree != 0) {
					MathStructure *xvar = NULL;
					Number qnr(1, 1);
					int degree = 1;
					bool overflow = false;
					int qcof = 1;
					if(CHILD(0).isPower() && !CHILD(0)[0].isNumber() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isInteger() && CHILD(0)[1].number().isPositive()) {
						xvar = &CHILD(0)[0];
						degree = CHILD(0)[1].number().intValue(&overflow);
					} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isInteger()) {
						if(CHILD(0)[1].isPower()) {
							if(CHILD(0)[1][0].size() == 0 && !CHILD(0)[1][0].isNumber() && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isInteger() && CHILD(0)[1][1].number().isPositive()) {
								xvar = &CHILD(0)[1][0];
								qcof = CHILD(0)[0].number().intValue(&overflow);
								if(!overflow) {
									if(qcof < 0) qcof = -qcof;
									degree = CHILD(0)[1][1].number().intValue(&overflow);
								}
							}
						}
					}

					int pcof = 1;
					if(!overflow) {
						pcof = CHILD(SIZE - 1).number().intValue(&overflow);
						if(pcof < 0) pcof = -pcof;
					}

					if(xvar && !overflow && degree <= 1000 && degree > 2 && qcof != 0 && pcof != 0) {
						bool b = true, b2 = true;
						for(size_t i = 1; b && i < SIZE - 1; i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {
									b = false;
									break;
								}
								case STRUCT_POWER: {
									if(!CHILD(i)[1].isNumber() || !xvar->equals(CHILD(i)[0]) || !CHILD(i)[1].number().isInteger() || !CHILD(i)[1].number().isPositive()) {
										b = false;
									}
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(!(CHILD(i).size() == 2) || !CHILD(i)[0].isNumber()) {
										b = false;
									} else if(CHILD(i)[1].isPower()) {
										if(!CHILD(i)[1][1].isNumber() || !xvar->equals(CHILD(i)[1][0]) || !CHILD(i)[1][1].number().isInteger() || !CHILD(i)[1][1].number().isPositive()) {
											b = false;
										}
									} else if(!xvar->equals(CHILD(i)[1])) {
										b = false;
									}
									if(b && b2 && !CHILD(i)[0].isInteger()) b2 = false;
									break;
								}
								default: {
									if(!xvar->equals(CHILD(i))) {
										b = false;
									}
								}
							}
						}

						if(b) {
							vector<Number> factors;
							factors.resize(degree + 1, Number());
							factors[0] = CHILD(SIZE - 1).number();
							vector<int> ps;
							vector<int> qs;
							vector<Number> zeroes;
							int curdeg = 1, prevdeg = 0;
							for(size_t i = 0; b && i < SIZE - 1; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_POWER: {
										curdeg = CHILD(i)[1].number().intValue(&overflow);
										if(curdeg == prevdeg || curdeg > degree || (prevdeg > 0 && curdeg > prevdeg) || overflow) {
											b = false;
										} else {
											factors[curdeg].set(1, 1, 0);
										}
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(CHILD(i)[1].isPower()) {
											curdeg = CHILD(i)[1][1].number().intValue(&overflow);
										} else {
											curdeg = 1;
										}
										if(curdeg == prevdeg || curdeg > degree || (prevdeg > 0 && curdeg > prevdeg) || overflow) {
											b = false;
										} else {
											factors[curdeg] = CHILD(i)[0].number();
										}
										break;
									}
									default: {
										curdeg = 1;
										factors[curdeg].set(1, 1, 0);
									}
								}
								prevdeg = curdeg;
							}

							while(b && degree > 2) {
								for(int i = 1; i <= 1000; i++) {
									if(i > pcof) break;
									if(pcof % i == 0) ps.push_back(i);
								}
								for(int i = 1; i <= 1000; i++) {
									if(i > qcof) break;
									if(qcof % i == 0) qs.push_back(i);
								}
								Number itest;
								int i2;
								size_t pi = 0, qi = 0;
								if(ps.empty() || qs.empty()) break;
								Number nrtest(ps[0], qs[0], 0);
								while(true) {
									itest.clear(); i2 = degree;
									while(true) {
										itest += factors[i2];
										if(i2 == 0) break;
										itest *= nrtest;
										i2--;
									}
									if(itest.isZero()) {
										break;
									}
									if(nrtest.isPositive()) {
										nrtest.negate();
									} else {
										qi++;
										if(qi == qs.size()) {
											qi = 0;
											pi++;
											if(pi == ps.size()) {
												break;
											}
										}
										nrtest.set(ps[pi], qs[qi], 0);
									}
								}
								if(itest.isZero()) {
									itest.clear(); i2 = degree;
									Number ntmp(factors[i2]);
									for(; i2 > 0; i2--) {
										itest += ntmp;
										ntmp = factors[i2 - 1];
										factors[i2 - 1] = itest;
										itest *= nrtest;
									}
									degree--;
									nrtest.negate();
									zeroes.push_back(nrtest);
									if(degree == 2) {
										break;
									}
									qcof = factors[degree].intValue(&overflow);
									if(!overflow) {
										if(qcof < 0) qcof = -qcof;
										pcof = factors[0].intValue(&overflow);
										if(!overflow) {
											if(pcof < 0) pcof = -pcof;
										}
									}
									if(overflow || qcof == 0 || pcof == 0) {
										break;
									}
								} else {
									break;
								}
								ps.clear();
								qs.clear();
							}

							if(zeroes.size() > 0) {
								MathStructure mleft;
								MathStructure mtmp;
								MathStructure *mcur;
								for(int i = degree; i >= 0; i--) {
									if(!factors[i].isZero()) {
										if(mleft.isZero()) {
											mcur = &mleft;
										} else {
											mleft.add(m_zero, true);
											mcur = &mleft[mleft.size() - 1];
										}
										if(i > 1) {
											if(!factors[i].isOne()) {
												mcur->multiply(*xvar);
												(*mcur)[0].set(factors[i]);
												mcur = &(*mcur)[1];
											} else {
												mcur->set(*xvar);
											}
											mtmp.set(i, 1, 0);
											mcur->raise(mtmp);
										} else if(i == 1) {
											if(!factors[i].isOne()) {
												mcur->multiply(*xvar);
												(*mcur)[0].set(factors[i]);
											} else {
												mcur->set(*xvar);
											}
										} else {
											mcur->set(factors[i]);
										}
									}
								}
								mleft.factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
								vector<long int> powers;
								vector<size_t> powers_i;
								int dupsfound = 0;
								for(size_t i = 0; i < zeroes.size() - 1; i++) {
									while(i + 1 < zeroes.size() && zeroes[i] == zeroes[i + 1]) {
										dupsfound++;
										zeroes.erase(zeroes.begin() + (i + 1));
									}
									if(dupsfound > 0) {
										powers_i.push_back(i);
										powers.push_back(dupsfound + 1);
										dupsfound = 0;
									}
								}
								MathStructure xvar2(*xvar);
								Number *nrmul;
								if(mleft.isMultiplication()) {
									set(mleft);
									evalSort();
									if(CHILD(0).isNumber()) {
										nrmul = &CHILD(0).number();
									} else if(CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber()) {
										nrmul = &CHILD(0)[0].number();
									} else {
										PREPEND(m_one);
										nrmul = &CHILD(0).number();
									}
								} else {
									clear(true);
									m_type = STRUCT_MULTIPLICATION;
									APPEND(m_one);
									APPEND(mleft);
									nrmul = &CHILD(0).number();
								}
								size_t pi = 0;
								for(size_t i = 0; i < zeroes.size(); i++) {
									if(zeroes[i].isInteger()) {
										APPEND(xvar2);
									} else {
										APPEND(m_zero);
									}
									mcur = &CHILD(SIZE - 1);
									if(pi < powers_i.size() && powers_i[pi] == i) {
										mcur->raise(MathStructure(powers[pi], 1L, 0L));
										mcur = &(*mcur)[0];
										if(zeroes[i].isInteger()) {
											mcur->add(zeroes[i]);
										} else {
											Number nr(zeroes[i].denominator());
											mcur->add(zeroes[i].numerator());
											(*mcur)[0] *= xvar2;
											(*mcur)[0][0].number() = nr;
											nr.raise(powers[pi]);
											nrmul->divide(nr);
										}
										pi++;
									} else {
										if(zeroes[i].isInteger()) {
											mcur->add(zeroes[i]);
										} else {
											nrmul->divide(zeroes[i].denominator());
											mcur->add(zeroes[i].numerator());
											(*mcur)[0] *= xvar2;
											(*mcur)[0][0].number() = zeroes[i].denominator();
										}
									}
								}
								if(CHILD(0).isNumber() && CHILD(0).number().isOne()) {
									ERASE(0);
								} else if(CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isOne()) {
									if(CHILD(0).size() == 1) {
										ERASE(0);
									} else if(CHILD(0).size() == 2) {
										CHILD(0).setToChild(2, true);
									} else {
										CHILD(0).delChild(1);
									}
								}
								evalSort(true);
								Number dupspow;
								for(size_t i = 0; i < SIZE - 1; i++) {
									mcur = NULL;
									if(CHILD(i).isPower()) {
										if(CHILD(i)[0].isAddition() && CHILD(i)[1].isNumber()) {
											mcur = &CHILD(i)[0];
										}
									} else if(CHILD(i).isAddition()) {
										mcur = &CHILD(i);
									}
									while(mcur && i + 1 < SIZE) {
										if(CHILD(i + 1).isPower()) {
											if(CHILD(i + 1)[0].isAddition() && CHILD(i + 1)[1].isNumber() && mcur->equals(CHILD(i + 1)[0])) {
												dupspow += CHILD(i + 1)[1].number();
											} else {
												mcur = NULL;
											}
										} else if(CHILD(i + 1).isAddition() && mcur->equals(CHILD(i + 1))) {
											dupspow++;
										} else {
											mcur = NULL;
										}
										if(mcur) {
											ERASE(i + 1);
										}
									}
									if(!dupspow.isZero()) {
										if(CHILD(i).isPower()) {
											CHILD(i)[1].number() += dupspow;
										} else {
											dupspow++;
											CHILD(i) ^= dupspow;
										}
										dupspow.clear();
									}
								}
								if(SIZE == 1) {
									setToChild(1, true);
								} else {
									EvaluationOptions eo2 = eo;
									eo2.expand = false;
									calculatesub(eo2, eo2, false);
								}
								evalSort(true);
								return true;
							}
						}

						if(b && b2 && (max_factor_degree < 0 || max_factor_degree >= 2) && degree > 3 && degree < 50) {
							// Kronecker method
							vector<Number> vnum;
							vnum.resize(degree + 1, nr_zero);
							bool overflow = false;
							for(size_t i = 0; b && i < SIZE; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_POWER: {
										if(CHILD(i)[0] == *xvar && CHILD(i)[1].isInteger()) {
											int curdeg = CHILD(i)[1].number().intValue(&overflow);
											if(curdeg < 0 || overflow || curdeg > degree) b = false;
											else vnum[curdeg] += 1;
										} else {
											b = false;
										}
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(CHILD(i).size() == 2 && CHILD(i)[0].isInteger()) {
											long int icoeff = CHILD(i)[0].number().intValue(&overflow);
											if(!overflow && CHILD(i)[1].isPower() && CHILD(i)[1][0] == *xvar && CHILD(i)[1][1].isInteger()) {
												int curdeg = CHILD(i)[1][1].number().intValue(&overflow);
												if(curdeg < 0 || overflow || curdeg > degree) b = false;
												else vnum[curdeg] += icoeff;
											} else if(!overflow && CHILD(i)[1] == *xvar) {
												vnum[1] += icoeff;
											} else {
												b = false;
											}
										} else {
											b = false;
										}
										break;
									}
									default: {
										if(CHILD(i).isInteger()) {
											long int icoeff = CHILD(i).number().intValue(&overflow);
											if(overflow) b = false;
											else vnum[0] += icoeff;
										} else if(CHILD(i) == *xvar) {
											vnum[1] += 1;
										} else {
											b = false;
										}
										break;
									}
								}
							}

							long int lcoeff = vnum[degree].lintValue();
							vector<int> vs;
							if(b && lcoeff != 0) {
								degree /= 2;
								if(max_factor_degree > 0 && degree > max_factor_degree) degree = max_factor_degree;
								for(int i = 0; i <= degree; i++) {
									if(CALCULATOR->aborted()) return false;
									MathStructure mcalc(*this);
									mcalc.calculateReplace(*xvar, Number((i / 2 + i % 2) * (i % 2 == 0 ? -1 : 1), 1), eo);
									mcalc.calculatesub(eo, eo, false);
									if(!mcalc.isInteger()) break;
									bool overflow = false;
									int v = ::abs(mcalc.number().intValue(&overflow));
									if(overflow) {
										if(i > 2) degree = i;
										else b = false;
										break;
									}
									vs.push_back(v);
								}
							}

							if(b) {
								vector<int> factors0, factorsl;
								factors0.push_back(1);
								for(int i = 2; i < vs[0] / 3 && i < 1000; i++) {
									if(vs[0] % i == 0) factors0.push_back(i);
								}
								if(vs[0] % 3 == 0) factors0.push_back(vs[0] / 3);
								if(vs[0] % 2 == 0) factors0.push_back(vs[0] / 2);
								factors0.push_back(vs[0]);
								for(int i = 2; i < lcoeff / 3 && i < 1000; i++) {
									if(lcoeff % i == 0) factorsl.push_back(i);
								}
								factorsl.push_back(1);
								if(lcoeff % 3 == 0) factorsl.push_back(lcoeff / 3);
								if(lcoeff % 2 == 0) factorsl.push_back(lcoeff / 2);
								factorsl.push_back(lcoeff);

								long long int cmax = 500000LL / (factors0.size() * factorsl.size());
								if(term_combination_levels != 0) cmax *= 10;
								if(degree >= 2 && cmax > 10) {
									vector<Number> vden;
									vector<Number> vquo;
									vden.resize(3, nr_zero);
									long int c0;
									for(size_t i = 0; i < factors0.size() * 2; i++) {
										c0 = factors0[i / 2];
										if(i % 2 == 1) c0 = -c0;
										long int c2;
										for(size_t i2 = 0; i2 < factorsl.size(); i2++) {
											c2 = factorsl[i2];
											long int c1max = vs[1] - c0 - c2, c1min;
											if(c1max < 0) {c1min = c1max; c1max = -vs[1] - c0 - c2;}
											else {c1min = -vs[1] - c0 - c2;}
											if(-(vs[2] - c0 - c2) < -(-vs[2] - c0 - c2)) {
												if(c1max > -(-vs[2] - c0 - c2)) c1max = -(-vs[2] - c0 - c2);
												if(c1min < -(vs[2] - c0 - c2)) c1min = -(vs[2] - c0 - c2);
											} else {
												if(c1max > -(vs[2] - c0 - c2)) c1max = -(vs[2] - c0 - c2);
												if(c1min < -(-vs[2] - c0 - c2)) c1min = -(-vs[2] - c0 - c2);
											}
											if(c1min < -cmax / 2) c1min = -cmax / 2;
											for(long int c1 = c1min; c1 <= c1max && c1 <= cmax / 2; c1++) {
												long int v1 = ::labs(c2 + c1 + c0);
												long int v2 = ::labs(c2 - c1 + c0);
												if(v1 != 0 && v2 != 0 && v1 <= vs[1] && v2 <= vs[2] && (c1 != 0 || c2 != 0) && vs[1] % v1 == 0 && vs[2] % v2 == 0) {
													vden[0] = c0; vden[1] = c1; vden[2] = c2;
													if(CALCULATOR->aborted()) return false;
													if(polynomial_divide_integers(vnum, vden, vquo)) {
														MathStructure mtest;
														mtest.setType(STRUCT_ADDITION);
														if(c2 != 0) {
															MathStructure *mpow = new MathStructure();
															mpow->setType(STRUCT_POWER);
															mpow->addChild(*xvar);
															mpow->addChild_nocopy(new MathStructure(2, 1, 0));
															if(c2 == 1) {
																mtest.addChild_nocopy(mpow);
															} else {
																MathStructure *mterm = new MathStructure();
																mterm->setType(STRUCT_MULTIPLICATION);
																mterm->addChild_nocopy(new MathStructure(c2, 1L, 0L));
																mterm->addChild_nocopy(mpow);
																mtest.addChild_nocopy(mterm);
															}
														}
														if(c1 == 1) {
															mtest.addChild(*xvar);
														} else if(c1 != 0) {
															MathStructure *mterm = new MathStructure();
															mterm->setType(STRUCT_MULTIPLICATION);
															mterm->addChild_nocopy(new MathStructure(c1, 1L, 0L));
															mterm->addChild(*xvar);
															mtest.addChild_nocopy(mterm);
														}
														mtest.addChild_nocopy(new MathStructure(c0, 1L, 0L));
														MathStructure mthis(*this);
														MathStructure mquo;
														if(mtest.size() > 1 && polynomialDivide(mthis, mtest, mquo, eo, false)) {
															mquo.factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
															set(mquo, true);
															multiply(mtest, true);
															return true;
														}
													}
												}
											}
										}
									}
								}
								for(int i_d = 3; i_d <= degree; i_d++) {
									if(CALCULATOR->aborted()) return false;
									long int t1max = ::pow(cmax / (i_d - 1), 1.0 / (i_d - 1));
									if(t1max < 1) break;
									if(t1max > 1000) t1max = 1000;
									long int c2totalmax = t1max;
									long int c2cur;
									for(int i = 0; i < i_d - 3; i++) {
										c2totalmax *= t1max;
									}
									vector<Number> vden;
									vector<Number> vquo;
									long int *vc = (long int*) malloc(sizeof(long int) * (i_d + 1));
									vden.resize(i_d + 1, nr_zero);
									for(size_t i = 0; i < factors0.size() * 2; i++) {
										vc[0] = factors0[i / 2] * (i % 2 == 1 ? -1 : 1);
										for(size_t i2 = 0; i2 < factorsl.size(); i2++) {
											vc[i_d] = factorsl[i2];
											for(long int c2p = 0; c2p <= c2totalmax; c2p++) {
												c2cur = c2p;
												for(int i = 2; i < i_d; i++) {
													vc[i] = c2cur % t1max;
													if(vc[i] % 2 == 1) vc[i] = -vc[i];
													vc[i] = vc[i] / 2 + vc[i] % 2;
													c2cur /= t1max;
												}
												long int c1max = t1max / 2 + t1max % 2, c1min = -t1max / 2 - t1max % 2;
												for(size_t i = 1; i < vs.size(); i++) {
													long int vsmax = vs[i] - vc[0];
													long int vsmin = -vs[i] - vc[0];
													int ix = (i / 2 + i % 2) * (i % 2 == 0 ? -1 : 1);
													int ixi = ix;
													for(int i2 = 2; i2 <= i_d; i2++) {
														ixi *= ix;
														vsmax -= vc[i2] * ixi;
													}
													vsmax /= ix;
													vsmin /= ix;
													if(vsmax < vsmin) {
														if(c1max > vsmin) c1max = vsmin;
														if(c1min < vsmax) c1min = vsmax;
													} else {
														if(c1max > vsmax) c1max = vsmax;
														if(c1min < vsmin) c1min = vsmin;
													}
												}
												for(long int c1 = c1min; c1 <= c1max; c1++) {
													vc[1] = c1;
													bool b = true;
													for(size_t i = 1; i < vs.size(); i++) {
														long int v = vc[0];
														int ix = (i / 2 + i % 2) * (i % 2 == 0 ? -1 : 1);
														int ixi = 1;
														for(int i2 = 1; i2 <= i_d; i2++) {
															ixi *= ix;
															v += vc[i2] * ixi;
														}
														if(v < 0) v = -v;
														if(v == 0 || v > vs[i] || vs[i] % v != 0) {
															b = false;
															break;
														}
													}
													if(b) {
														if(CALCULATOR->aborted()) return false;
														for(size_t iden = 0; iden < vden.size(); iden++) {
															vden[iden] = vc[iden];
														}
														if(polynomial_divide_integers(vnum, vden, vquo)) {
															MathStructure mtest;
															mtest.setType(STRUCT_ADDITION);
															for(int i2 = i_d; i2 >= 2; i2--) {
																if(vc[i2] != 0) {
																	MathStructure *mpow = new MathStructure();
																	mpow->setType(STRUCT_POWER);
																	mpow->addChild(*xvar);
																	mpow->addChild_nocopy(new MathStructure(i2, 1, 0));
																	if(vc[i2] == 1) {
																		mtest.addChild_nocopy(mpow);
																	} else {
																		MathStructure *mterm = new MathStructure();
																		mterm->setType(STRUCT_MULTIPLICATION);
																		mterm->addChild_nocopy(new MathStructure(vc[i2], 1L, 0L));
																		mterm->addChild_nocopy(mpow);
																		mtest.addChild_nocopy(mterm);
																	}
																}
															}
															if(vc[1] == 1) {
																mtest.addChild(*xvar);
															} else if(vc[1] != 0) {
																MathStructure *mterm = new MathStructure();
																mterm->setType(STRUCT_MULTIPLICATION);
																mterm->addChild_nocopy(new MathStructure(vc[1], 1L, 0L));
																mterm->addChild(*xvar);
																mtest.addChild_nocopy(mterm);
															}
															mtest.addChild_nocopy(new MathStructure(vc[0], 1L, 0L));
															MathStructure mthis(*this);
															MathStructure mquo;
															if(mtest.size() > 1 && polynomialDivide(mthis, mtest, mquo, eo, false)) {
																mquo.factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
																free(vc);
																set(mquo, true);
																multiply(mtest, true);
																return true;
															}
														}
													}
												}
											}
										}
									}
									free(vc);
								}
							}
						}

					}
				}

				if(SIZE == 2 && max_factor_degree != 0) {
					Number nr1(1, 1, 0), nr2(1, 1, 0);
					bool b = true, b_nonnum = false;
					bool b1_neg = false, b2_neg = false;
					for(size_t i = 0; i < SIZE && b; i++) {
						b = false;
						if(CHILD(i).isInteger() && CHILD(i).number().integerLength() < 100) {
							b = true;
							if(i == 0) nr1 = CHILD(i).number();
							else nr2 = CHILD(i).number();
						} else if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
							b_nonnum = true;
							b = true;
							size_t i2 = 0;
							if(CHILD(i)[0].isInteger() && CHILD(i).number().integerLength() < 100) {
								if(i == 0) nr1 = CHILD(i)[0].number();
								else nr2 = CHILD(i)[0].number();
								i2++;
							}
							for(; i2 < CHILD(i).size(); i2++) {
								if(!CHILD(i)[i2].isPower() || !CHILD(i)[i2][1].isInteger() || !CHILD(i)[i2][1].number().isPositive() || !CHILD(i)[i2][1].number().isEven() || CHILD(i)[1].number().integerLength() >= 100 || !CHILD(i)[i2][0].representsNonMatrix()) {
									b = false;
									break;
								}
							}
						} else if(CHILD(i).isPower() && CHILD(i)[1].isNumber() && CHILD(i)[1].number().isInteger() && CHILD(i)[1].number().isPositive() && CHILD(i)[1].number().isEven() && CHILD(i)[1].number().integerLength() < 100 && CHILD(i)[0].representsNonMatrix()) {
							b_nonnum = true;
							b = true;
						}
					}
					if(!b_nonnum) b = false;
					if(b) {
						b1_neg = nr1.isNegative();
						b2_neg = nr2.isNegative();
						if(b1_neg == b2_neg) b = false;
					}
					if(b) {
						if(b1_neg) b = nr1.negate();
						if(b && !nr1.isOne()) {
							b = nr1.isPerfectSquare() && nr1.isqrt();
						}
					}
					if(b) {
						if(b2_neg) nr2.negate();
						if(!nr2.isOne()) {
							b = nr2.isPerfectSquare() && nr2.isqrt();
						}
					}
					if(b) {
						bool calc = false;
						MathStructure *mmul = new MathStructure(*this);
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isNumber()) {
								if(i == 0) {
									CHILD(i).number() = nr1;
									if(b1_neg) nr1.negate();
									(*mmul)[i].number() = nr1;
								} else {
									CHILD(i).number() = nr2;
									if(b2_neg) nr2.negate();
									(*mmul)[i].number() = nr2;
								}
							} else if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
								b = true;
								size_t i2 = 0;
								if(CHILD(i)[0].isNumber()) {
									if(i == 0) {
										CHILD(i)[0].number() = nr1;
										if(b1_neg) nr1.negate();
										(*mmul)[i][0].number() = nr1;
									} else {
										CHILD(i)[0].number() = nr2;
										if(b2_neg) nr2.negate();
										(*mmul)[i][0].number() = nr2;
									}
									i2++;
								}
								for(; i2 < CHILD(i).size(); i2++) {
									if(CHILD(i)[i2][1].number().isTwo()) {
										CHILD(i)[i2].setToChild(1, true);
										(*mmul)[i][i2].setToChild(1, true);
									} else {
										CHILD(i)[i2][1].number().divide(2);
										(*mmul)[i][i2][1].number().divide(2);
									}
									CHILD(i).childUpdated(i2 + 1);
									(*mmul)[i].childUpdated(i2 + 1);
								}
								if(CHILD(i)[0].isOne()) CHILD(i).delChild(1, true);
								if((*mmul)[i][0].isOne()) (*mmul)[i].delChild(1, true);
							} else if(CHILD(i).isPower()) {
								if(CHILD(i)[1].number().isTwo()) {
									CHILD(i).setToChild(1, true);
									(*mmul)[i].setToChild(1, true);
								} else {
									CHILD(i)[1].number().divide(2);
									(*mmul)[i][1].number().divide(2);
								}
							}
							if(CHILD(i).isAddition()) calc = true;
							CHILD_UPDATED(i)
							mmul->childUpdated(i + 1);
						}
						if(calc) {
							calculatesub(eo, eo, false);
							mmul->calculatesub(eo, eo, false);
						}
						if(recursive) {
							factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
							mmul->factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						}
						multiply_nocopy(mmul);
						evalSort(true);
						return true;
					}
				}

				//x^3-y^3=(x-y)(x^2+xy+y^2)
				if(max_factor_degree != 0 && SIZE == 2 && CHILD(0).isPower() && CHILD(0)[1].isNumber() && CHILD(0)[1].number() == 3 && CHILD(1).isMultiplication() && CHILD(1).size() == 2 && CHILD(1)[0].isMinusOne() && CHILD(1)[1].isPower() && CHILD(1)[1][1].isNumber() && CHILD(1)[1][1].number() == 3) {
					if(CHILD(0)[0].representsNonMatrix() && CHILD(1)[1][0].representsNonMatrix()) {
						MathStructure *m2 = new MathStructure(*this);
						(*m2)[0].setToChild(1, true);
						(*m2)[1][1].setToChild(1, true);
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						m2->calculatesub(eo2, eo2, false);
						CHILD(0)[1].set(2, 1, 0, true);
						CHILD(1).setToChild(2, true);
						CHILD(1)[1].set(2, 1, 0, true);
						MathStructure *m3 = new MathStructure(CHILD(0)[0]);
						m3->calculateMultiply(CHILD(1)[0], eo2);
						add_nocopy(m3, true);
						calculatesub(eo2, eo2, false);
						multiply_nocopy(m2, true);
						evalSort(true);
						return true;
					}
				}
				//-x^3+y^3=(-x+y)(x^2+xy+y^2)
				if(max_factor_degree != 0 && SIZE == 2 && CHILD(1).isPower() && CHILD(1)[1].isNumber() && CHILD(1)[1].number() == 3 && CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isMinusOne() && CHILD(0)[1].isPower() && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number() == 3) {
					if(CHILD(1)[0].representsNonMatrix() && CHILD(0)[1][0].representsNonMatrix()) {
						MathStructure *m2 = new MathStructure(*this);
						(*m2)[1].setToChild(1, true);
						(*m2)[0][1].setToChild(1, true);
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						m2->calculatesub(eo2, eo2, false);
						CHILD(1)[1].set(2, 1, 0, true);
						CHILD(0).setToChild(2, true);
						CHILD(0)[1].set(2, 1, 0, true);
						MathStructure *m3 = new MathStructure(CHILD(0)[0]);
						m3->calculateMultiply(CHILD(1)[0], eo2);
						add_nocopy(m3, true);
						calculatesub(eo2, eo2, false);
						multiply_nocopy(m2, true);
						evalSort(true);
						return true;
					}
				}

				if(max_factor_degree != 0 && !only_integers && !force_factorization.isUndefined() && SIZE >= 2) {
					MathStructure mexp, madd, mmul;
					if(gather_factors(*this, force_factorization, madd, mmul, mexp) && !madd.isZero() && !mmul.isZero() && mexp.isInteger() && mexp.number().isGreaterThan(nr_two)) {
						if(!mmul.isOne()) madd.calculateDivide(mmul, eo);
						bool overflow = false;
						int n = mexp.number().intValue(&overflow);
						if(!overflow) {
							if(n % 4 == 0) {
								int i_u = 1;
								if(n != 4) {
									i_u = n / 4;
								}
								MathStructure m_sqrt2(2, 1, 0);
								m_sqrt2.calculateRaise(nr_half, eo);
								MathStructure m_sqrtb(madd);
								m_sqrtb.calculateRaise(nr_half, eo);
								MathStructure m_bfourth(madd);
								m_bfourth.calculateRaise(Number(1, 4), eo);
								m_sqrt2.calculateMultiply(m_bfourth, eo);
								MathStructure m_x(force_factorization);
								if(i_u != 1) m_x ^= i_u;
								m_sqrt2.calculateMultiply(m_x, eo);
								MathStructure *m2 = new MathStructure(force_factorization);
								m2->raise(Number(i_u * 2, 1));
								m2->add(m_sqrtb);
								m2->calculateAdd(m_sqrt2, eo);
								set(force_factorization, true);
								raise(Number(i_u * 2, 1));
								add(m_sqrtb);
								calculateSubtract(m_sqrt2, eo);
								multiply_nocopy(m2);
							} else {
								int i_u = 1;
								if(n % 2 == 0) {
									i_u = 2;
									n /= 2;
								}
								MathStructure *m2 = new MathStructure(madd);
								m2->calculateRaise(Number(n - 1, n), eo);
								for(int i = 1; i < n - 1; i++) {
									MathStructure *mterm = new MathStructure(madd);
									mterm->calculateRaise(Number(n - i - 1, n), eo);
									mterm->multiply(force_factorization);
									if(i != 1 || i_u != 1) {
										mterm->last().raise(Number(i * i_u, 1));
										mterm->childUpdated(mterm->size());
									}
									if(i % 2 == 1) mterm->calculateMultiply(m_minus_one, eo);
									m2->add_nocopy(mterm, true);
								}
								MathStructure *mterm = new MathStructure(force_factorization);
								mterm->raise(Number((n - 1) * i_u, 1));
								m2->add_nocopy(mterm, true);
								mterm = new MathStructure(force_factorization);
								if(i_u != 1) mterm->raise(Number(i_u, 1));
								set(madd, true);
								calculateRaise(Number(1, n), eo);
								add_nocopy(mterm);
								multiply_nocopy(m2);
							}
							if(!mmul.isOne()) multiply(mmul, true);
							evalSort(true);
							return true;
						}
					}
				}

				//-x-y = -(x+y)
				bool b = true;
				for(size_t i2 = 0; i2 < SIZE; i2++) {
					if((!CHILD(i2).isNumber() || !CHILD(i2).number().isNegative()) && (!CHILD(i2).isMultiplication() || CHILD(i2).size() < 2 || !CHILD(i2)[0].isNumber() || !CHILD(i2)[0].number().isNegative())) {
						b = false;
						break;
					}
				}
				if(b) {
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(CHILD(i2).isNumber()) {
							CHILD(i2).number().negate();
						} else {
							CHILD(i2)[0].number().negate();
							if(CHILD(i2)[0].isOne() && CHILD(i2).size() > 1) {
								CHILD(i2).delChild(1);
								if(CHILD(i2).size() == 1) {
									CHILD(i2).setToChild(1, true);
								}
							}
						}
					}
					multiply(MathStructure(-1, 1, 0));
					CHILD_TO_FRONT(1)
				}

				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
						for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
							if(CHILD(i)[i2].isAddition()) {
								for(size_t i3 = i + 1; i3 < SIZE; i3++) {
									if(CHILD(i3).isMultiplication() && CHILD(i3).size() > 1) {
										for(size_t i4 = 0; i4 < CHILD(i3).size(); i4++) {
											if(CHILD(i3)[i4].isAddition() && CHILD(i3)[i4] == CHILD(i)[i2]) {
												MathStructure *mfac = &CHILD(i)[i2];
												mfac->ref();
												CHILD(i).delChild(i2 + 1, true);
												CHILD(i3).delChild(i4 + 1, true);
												CHILD(i3).ref();
												CHILD(i).add_nocopy(&CHILD(i3));
												CHILD(i).calculateAddLast(eo);
												CHILD(i).multiply_nocopy(mfac);
												CHILD_UPDATED(i)
												delChild(i3 + 1, true);
												evalSort(true);
												factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
												return true;
											}
										}
									}
								}
								if(SIZE > 2) {
									MathStructure mtest(*this);
									mtest.delChild(i + 1);
									if(mtest == CHILD(i)[i2]) {
										CHILD(i).delChild(i2 + 1, true);
										SET_CHILD_MAP(i);
										add(m_one, true);
										multiply(mtest);
										evalSort(true);
										factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
										return true;
									}
								}
							}
						}
					}
				}
			}

			//complete the square
			if(max_factor_degree != 0 && (term_combination_levels != 0 || complete_square)) {
				if(only_integers) {
					if(SIZE <= 3 && SIZE > 1) {
						MathStructure *xvar = NULL;
						Number nr2(1, 1);
						if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
							xvar = &CHILD(0)[0];
						} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber()) {
							if(CHILD(0)[1].isPower()) {
								if(CHILD(0)[1][0].size() == 0 && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isTwo()) {
									xvar = &CHILD(0)[1][0];
									nr2.set(CHILD(0)[0].number());
								}
							}
						}
						if(xvar) {
							bool factorable = false;
							Number nr1, nr0;
							if(SIZE == 2 && CHILD(1).isNumber()) {
								factorable = true;
								nr0 = CHILD(1).number();
							} else if(SIZE == 3 && CHILD(2).isNumber()) {
								nr0 = CHILD(2).number();
								if(CHILD(1).isMultiplication()) {
									if(CHILD(1).size() == 2 && CHILD(1)[0].isNumber() && xvar->equals(CHILD(1)[1])) {
										nr1 = CHILD(1)[0].number();
										factorable = true;
									}
								} else if(xvar->equals(CHILD(1))) {
									nr1.set(1, 1, 0);
									factorable = true;
								}
							}
							if(factorable && !nr2.isZero() && !nr1.isZero()) {
								Number nrh(nr1);
								nrh /= 2;
								nrh /= nr2;
								if(nrh.isInteger()) {
									Number nrk(nrh);
									if(nrk.square()) {
										nrk *= nr2;
										nrk.negate();
										nrk += nr0;
										set(MathStructure(*xvar), true);
										add(nrh);
										raise(nr_two);
										if(!nr2.isOne()) multiply(nr2);
										if(!nrk.isZero()) add(nrk);
										evalSort(true);
										return true;
									}
								}
							}
						}
					}
				} else {
					MathStructure m2, m1, m0;
					const MathStructure *xvar = NULL;
					if(!force_factorization.isUndefined()) {
						xvar = &force_factorization;
					} else {
						if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
							xvar = &CHILD(0)[0];
						} else if(CHILD(0).isMultiplication()) {
							for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
								if(CHILD(0).isPower() && CHILD(0)[i2][0].size() == 0 && CHILD(0)[i2][1].isNumber() && CHILD(0)[i2][1].number().isTwo()) {
									xvar = &CHILD(0)[0];
								}
							}
						}
					}
					if(xvar && gather_factors(*this, *xvar, m0, m1, m2, true) && !m1.isZero() && !m2.isZero()) {
						MathStructure *mx = new MathStructure(*xvar);
						set(m1, true);
						calculateMultiply(nr_half, eo);
						if(!m2.isOne()) calculateDivide(m2, eo);
						add_nocopy(mx);
						calculateAddLast(eo);
						raise(nr_two);
						if(!m2.isOne()) multiply(m2);
						if(!m1.isOne()) m1.calculateRaise(nr_two, eo);
						m1.calculateMultiply(Number(-1, 4), eo);
						if(!m2.isOne()) {
							m2.calculateInverse(eo);
							m1.calculateMultiply(m2, eo);
						}
						m0.calculateAdd(m1, eo);
						if(!m0.isZero()) add(m0);
						if(recursive) {
							CHILD(0).factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
							CHILD(1).factorize(eo, false, 0, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
							CHILDREN_UPDATED
						}
						evalSort(true);
						return true;
					}
				}
			}

			//Try factorize combinations of terms
			if(SIZE > 2 && term_combination_levels > 0) {
				bool b = false, b_ret = false;
				// 5/y + x/y + z = (5 + x)/y + z
				MathStructure mstruct_units(*this);
				MathStructure mstruct_new(*this);
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(mstruct_units[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
							if(!mstruct_units[i][i2].isPower() || !mstruct_units[i][i2][1].hasNegativeSign()) {
								mstruct_units[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
						else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
						for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
							if(mstruct_new[i][i2].isPower() && mstruct_new[i][i2][1].hasNegativeSign()) {
								mstruct_new[i].delChild(i2 + 1);
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
						else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
					} else if(mstruct_new[i].isPower() && mstruct_new[i][1].hasNegativeSign()) {
						mstruct_new[i].set(1, 1, 0);
					} else {
						mstruct_units[i].setUndefined();
					}
				}
				for(size_t i = 0; i < mstruct_units.size(); i++) {
					if(!mstruct_units[i].isUndefined()) {
						for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
							if(mstruct_units[i2] == mstruct_units[i]) {
								mstruct_new[i].add(mstruct_new[i2], true);
								mstruct_new.delChild(i2 + 1);
								mstruct_units.delChild(i2 + 1);
								b = true;
							} else {
								i2++;
							}
						}
						if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
						else mstruct_new[i].multiply(mstruct_units[i], true);
					}
				}
				if(b) {
					if(mstruct_new.size() == 1) {
						set(mstruct_new[0], true);
						factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						return true;
					} else {
						set(mstruct_new);
					}
					b = false;
					b_ret = true;
				}
				// a*y + a*z + x = a(y + z) + x
				vector<MathStructure> syms;
				vector<size_t> counts;
				collect_symbols(*this, syms);
				size_t max_count = 0, max_i = 0;
				Number min_pow;
				for(size_t i = 0; i < syms.size(); i++) {
					if(syms[i].containsUnknowns()) {
						size_t count = 0;
						Number min_pow_i;
						for(size_t i2 = 0; i2 < SIZE; i2++) {
							if(CHILD(i2).isMultiplication()) {
								for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
									if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isNegative() && CHILD(i2)[i3][0] == syms[i]) {
										if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() > min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
										count++;
										break;
									}
								}
							} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isNegative() && CHILD(i2)[0] == syms[i]) {
								if(min_pow_i.isZero() || CHILD(i2)[1].number() > min_pow_i) min_pow_i = CHILD(i2)[1].number();
								count++;
							}
						}
						if(count > 1 && count > max_count) {
							max_count = count;
							min_pow = min_pow_i;
							max_i = i;
						}
					}
				}
				if(!max_count) {
					for(size_t i = 0; i < syms.size(); i++) {
						if(syms[i].containsUnknowns()) {
							size_t count = 0;
							Number min_pow_i;
							for(size_t i2 = 0; i2 < SIZE; i2++) {
								if(CHILD(i2).isMultiplication()) {
									for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
										if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isPositive() && CHILD(i2)[i3][0] == syms[i]) {
											if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() < min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
											count++;
											break;
										} else if(CHILD(i2)[i3] == syms[i]) {
											if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
											count++;
											break;
										}
									}
								} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isPositive() && CHILD(i2)[0] == syms[i]) {
									if(min_pow_i.isZero() || CHILD(i2)[1].number() < min_pow_i) min_pow_i = CHILD(i2)[1].number();
									count++;
								} else if(CHILD(i2) == syms[i]) {
									if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
									count++;
								}
							}
							if(count > 1 && count > max_count) {
								max_count = count;
								min_pow = min_pow_i;
								max_i = i;
							}
						}
					}
				}
				if(max_count > 0) {
					size_t i = max_i;
					vector<MathStructure*> mleft;
					for(size_t i2 = 0; i2 < SIZE;) {
						b = false;
						if(CHILD(i2).isMultiplication()) {
							for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
								if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[i3][1].number().isPositive() : CHILD(i2)[i3][1].number().isNegative()) && CHILD(i2)[i3][0] == syms[i]) {
									if(CHILD(i2)[i3][1] == min_pow) CHILD(i2).delChild(i3 + 1, true);
									else if(CHILD(i2)[i3][1] == min_pow + 1) CHILD(i2)[i3].setToChild(1, true);
									else {
										CHILD(i2)[i3][1].number() -= min_pow;
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								} else if(min_pow.isPositive() && CHILD(i2)[i3] == syms[i]) {
									if(min_pow.isOne()) CHILD(i2).delChild(i3 + 1, true);
									else {
										CHILD(i2)[i3].raise((-min_pow) + 1);
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								}
							}
						} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[1].number().isPositive() : CHILD(i2)[1].number().isNegative()) && CHILD(i2)[0] == syms[i]) {
							if(CHILD(i2)[1] == min_pow) CHILD(i2).set(1, 1, 0, true);
							else if(CHILD(i2)[1] == min_pow + 1) CHILD(i2).setToChild(1, true);
							else {
								CHILD(i2)[1].number() -= min_pow;
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						} else if(min_pow.isPositive() && CHILD(i2) == syms[i]) {
							if(min_pow.isOne()) CHILD(i2).set(1, 1, 0, true);
							else {
								CHILD(i2).raise((-min_pow) + 1);
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						}
						if(b) {
							i2++;
						} else {
							CHILD(i2).ref();
							mleft.push_back(&CHILD(i2));
							ERASE(i2)
						}
					}
					multiply(syms[i]);
					if(!min_pow.isOne()) LAST ^= min_pow;
					for(size_t i2 = 0; i2 < mleft.size(); i2++) {
						add_nocopy(mleft[i2], true);
					}
					factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					return true;
				}
				if(LAST.isNumber()) {
					MathStructure *mdel = &LAST;
					mdel->ref();
					delChild(SIZE, true);
					b = factorize(eo, false, term_combination_levels - 1, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					add_nocopy(mdel, true);
					if(term_combination_levels == 1) return b || b_ret;
					if(b) b_ret = true;
				}
				for(size_t i = 0; !b && i < SIZE; i++) {
					MathStructure *mdel = &CHILD(i);
					mdel->ref();
					delChild(i + 1, true);
					b = true;
					if(mdel->isMultiplication()) {
						for(size_t i2 = 0; i2 < mdel->size(); i2++) {
							if((*mdel)[i2].isPower() && (*mdel)[i2][0].containsUnknowns()) {
								if(contains((*mdel)[i2][0], false, false, false) > 0) {b = false; break;}
							} else if((*mdel)[i2].containsUnknowns()) {
								if(contains((*mdel)[i2], false, false, false) > 0) {b = false; break;}
							}
						}

					} else {
						b = contains(*mdel, false, false, false) <= 0;
					}
					if(b) {
						b = factorize(eo, false, term_combination_levels - 1, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						if(recursive) mdel->factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						add_nocopy(mdel, true);
						if(term_combination_levels == 1) return b || b_ret;
						if(b) b_ret = true;
						break;
					} else {
						insertChild_nocopy(mdel, i + 1);
					}
				}
				b = false;
				// a*y + a*z + x = a(y + z) + x
				syms.clear();
				counts.clear();
				collect_symbols(*this, syms);
				max_count = 0; max_i = 0;
				for(size_t i = 0; i < syms.size(); i++) {
					size_t count = 0;
					Number min_pow_i;
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(CHILD(i2).isMultiplication()) {
							for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
								if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isNegative() && CHILD(i2)[i3][0] == syms[i]) {
									if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() > min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
									count++;
									break;
								}
							}
						} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isNegative() && CHILD(i2)[0] == syms[i]) {
							if(min_pow_i.isZero() || CHILD(i2)[1].number() > min_pow_i) min_pow_i = CHILD(i2)[1].number();
							count++;
						}
					}
					if(count > 1 && count > max_count) {
						max_count = count;
						min_pow = min_pow_i;
						max_i = i;
					}
				}
				if(!max_count) {
					for(size_t i = 0; i < syms.size(); i++) {
						size_t count = 0;
						Number min_pow_i;
						for(size_t i2 = 0; i2 < SIZE; i2++) {
							if(CHILD(i2).isMultiplication()) {
								for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
									if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && CHILD(i2)[i3][1].number().isPositive() && CHILD(i2)[i3][0] == syms[i]) {
										if(min_pow_i.isZero() || CHILD(i2)[i3][1].number() < min_pow_i) min_pow_i = CHILD(i2)[i3][1].number();
										count++;
										break;
									} else if(CHILD(i2)[i3] == syms[i]) {
										if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
										count++;
										break;
									}
								}
							} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && CHILD(i2)[1].number().isPositive() && CHILD(i2)[0] == syms[i]) {
								if(min_pow_i.isZero() || CHILD(i2)[1].number() < min_pow_i) min_pow_i = CHILD(i2)[1].number();
								count++;
							} else if(CHILD(i2) == syms[i]) {
								if(min_pow_i.isZero() || min_pow_i > 1) min_pow_i = 1;
								count++;
							}
						}
						if(count > 1 && count > max_count) {
							max_count = count;
							min_pow = min_pow_i;
							max_i = i;
						}
					}
				}
				if(max_count > 0) {
					size_t i = max_i;
					vector<MathStructure*> mleft;
					for(size_t i2 = 0; i2 < SIZE;) {
						b = false;
						if(CHILD(i2).isMultiplication()) {
							for(size_t i3 = 0; i3 < CHILD(i2).size(); i3++) {
								if(CHILD(i2)[i3].isPower() && CHILD(i2)[i3][1].isNumber() && CHILD(i2)[i3][1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[i3][1].number().isPositive() : CHILD(i2)[i3][1].number().isNegative()) && CHILD(i2)[i3][0] == syms[i]) {
									if(CHILD(i2)[i3][1] == min_pow) CHILD(i2).delChild(i3 + 1, true);
									else if(CHILD(i2)[i3][1] == min_pow + 1) CHILD(i2)[i3].setToChild(1, true);
									else {
										CHILD(i2)[i3][1].number() -= min_pow;
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								} else if(min_pow.isPositive() && CHILD(i2)[i3] == syms[i]) {
									if(min_pow.isOne()) CHILD(i2).delChild(i3 + 1, true);
									else {
										CHILD(i2)[i3].raise((-min_pow) + 1);
										factorize_fix_root_power(CHILD(i2)[i3]);
									}
									b = true;
									break;
								}
							}
						} else if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isRational() && (min_pow.isPositive() ? CHILD(i2)[1].number().isPositive() : CHILD(i2)[1].number().isNegative()) && CHILD(i2)[0] == syms[i]) {
							if(CHILD(i2)[1] == min_pow) CHILD(i2).set(1, 1, 0, true);
							else if(CHILD(i2)[1] == min_pow + 1) CHILD(i2).setToChild(1, true);
							else {
								CHILD(i2)[1].number() -= min_pow;
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						} else if(min_pow.isPositive() && CHILD(i2) == syms[i]) {
							if(min_pow.isOne()) CHILD(i2).set(1, 1, 0, true);
							else {
								CHILD(i2).raise((-min_pow) + 1);
								factorize_fix_root_power(CHILD(i2));
							}
							b = true;
						}
						if(b) {
							i2++;
						} else {
							CHILD(i2).ref();
							mleft.push_back(&CHILD(i2));
							ERASE(i2)
						}
					}
					multiply(syms[i]);
					if(!min_pow.isOne()) LAST ^= min_pow;
					for(size_t i2 = 0; i2 < mleft.size(); i2++) {
						add_nocopy(mleft[i2], true);
					}
					factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					return true;
				}
				if(isAddition()) {
					b = false;
					// y*f(x) + z*f(x) = (y+z)*f(x)
					mstruct_units.set(*this);
					mstruct_new.set(*this);
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(mstruct_units[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
								if(!combination_factorize_is_complicated(mstruct_units[i][i2])) {
									mstruct_units[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
							else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
							for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
								if(combination_factorize_is_complicated(mstruct_new[i][i2])) {
									mstruct_new[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
							else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
						} else if(combination_factorize_is_complicated(mstruct_units[i])) {
							mstruct_new[i].set(1, 1, 0);
						} else {
							mstruct_units[i].setUndefined();
						}
					}
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(!mstruct_units[i].isUndefined()) {
							for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_new[i].add(mstruct_new[i2], true);
									mstruct_new.delChild(i2 + 1);
									mstruct_units.delChild(i2 + 1);
									b = true;
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
							else mstruct_new[i].multiply(mstruct_units[i], true);
						}
					}
					if(b) {
						if(mstruct_new.size() == 1) set(mstruct_new[0], true);
						else set(mstruct_new);
						factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						return true;
					}
				}
				if(isAddition()) {
					b = false;
					mstruct_units.set(*this);
					mstruct_new.set(*this);
					// 5x + pi*x + 5y + xy = (5 + pi)x + 5y + xy
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(mstruct_units[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
								if(!mstruct_units[i][i2].containsType(STRUCT_UNIT, true) && !mstruct_units[i][i2].containsUnknowns()) {
									mstruct_units[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
							else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
							for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
								if(mstruct_new[i][i2].containsType(STRUCT_UNIT, true) || mstruct_new[i][i2].containsUnknowns()) {
									mstruct_new[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
							else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
						} else if(mstruct_units[i].containsType(STRUCT_UNIT, true) || mstruct_units[i].containsUnknowns()) {
							mstruct_new[i].set(1, 1, 0);
						} else {
							mstruct_units[i].setUndefined();
						}
					}
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(!mstruct_units[i].isUndefined()) {
							for(size_t i2 = i + 1; i2 < mstruct_units.size();) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_new[i].add(mstruct_new[i2], true);
									mstruct_new.delChild(i2 + 1);
									mstruct_units.delChild(i2 + 1);
									b = true;
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].isOne()) mstruct_new[i].set(mstruct_units[i]);
							else mstruct_new[i].multiply(mstruct_units[i], true);
						}
					}
					if(b) {
						if(mstruct_new.size() == 1) set(mstruct_new[0], true);
						else set(mstruct_new);
						factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
						return true;
					}
				}
				return b_ret;
			} else if(term_combination_levels != 0 && SIZE > 2) {
				int start_index = rand() % SIZE;
				int index = start_index;
				int best_index = -1;
				int run_index = 0;
				int max_run_index = SIZE - 3;
				if(term_combination_levels < -1) {
					run_index = -term_combination_levels - 2;
					max_run_index = run_index;
				} else if(term_combination_levels > 0 && term_combination_levels - 1 < max_run_index) {
					max_run_index = term_combination_levels -1;
				}

				MathStructure mbest;
				do {
					if(CALCULATOR->aborted()) break;
					if(endtime_p && endtime_p->tv_sec > 0) {
#ifndef CLOCK_MONOTONIC
						struct timeval curtime;
						gettimeofday(&curtime, NULL);
						if(curtime.tv_sec > endtime_p->tv_sec || (curtime.tv_sec == endtime_p->tv_sec && curtime.tv_usec > endtime_p->tv_usec)) {
#else
						struct timespec curtime;
						clock_gettime(CLOCK_MONOTONIC, &curtime);
						if(curtime.tv_sec > endtime_p->tv_sec || (curtime.tv_sec == endtime_p->tv_sec && curtime.tv_nsec / 1000 > endtime_p->tv_usec)) {
#endif
							CALCULATOR->error(false, _("Because of time constraints only a limited number of combinations of terms were tried during factorization. Repeat factorization to try other random combinations."), NULL);
							break;
						}
					}

					MathStructure mtest(*this);
					mtest.delChild(index + 1);
					if(mtest.factorize(eo, false, run_index == 0 ? 0 : -1 - run_index, 0, only_integers, false, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree)) {
						bool b = best_index < 0 || (mbest.isAddition() && !mtest.isAddition());
						if(!b && (mtest.isAddition() == mbest.isAddition())) {
							b = mtest.isAddition() && (mtest.size() < mbest.size());
							if(!b && (!mtest.isAddition() || mtest.size() == mbest.size())) {
								size_t c1 = mtest.countTotalChildren() + CHILD(index).countTotalChildren();
								size_t c2 = mbest.countTotalChildren() + CHILD(best_index).countTotalChildren();
								b = (c1 < c2);
								if(c1 == c2) {
									b = (count_powers(mtest) + count_powers(CHILD(index))) < (count_powers(mbest) + count_powers(CHILD(best_index)));
								}
							}
						}
						if(b) {
							mbest = mtest;
							best_index = index;
							if(mbest.isPower()) {
								break;
							}
						}
					}
					index++;
					if(index == (int) SIZE) index = 0;
					if(index == start_index) {
						if(best_index >= 0) {
							break;
						}
						run_index++;
						if(run_index > max_run_index) break;
					}
				} while(true);
				if(best_index >= 0) {
					mbest.add(CHILD(best_index), true);
					set(mbest);
					if(term_combination_levels >= -1 && (run_index > 0 || recursive)) {
						factorize(eo, false, term_combination_levels, 0, only_integers, true, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
					}
					return true;
				}
			}
		}
		default: {
			if(term_combination_levels < -1) break;
			bool b = false;

			if(isComparison()) {
				EvaluationOptions eo2 = eo;
				eo2.assume_denominators_nonzero = false;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).factorize(eo2, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
			} else if(recursive && (recursive > 1 || !isAddition())) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
			}
			if(b) {
				EvaluationOptions eo2 = eo;
				eo2.expand = false;
				calculatesub(eo2, eo2, false);
				evalSort(true);
				if(isAddition()) {
					for(size_t i = 0; i < SIZE; i++) {
						if(CHILD(i).isMultiplication() && CHILD(i).size() > 1) {
							for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
								if(CHILD(i)[i2].isAddition()) {
									for(size_t i3 = i + 1; i3 < SIZE; i3++) {
										if(CHILD(i3).isMultiplication() && CHILD(i3).size() > 1) {
											for(size_t i4 = 0; i4 < CHILD(i3).size(); i4++) {
												if(CHILD(i3)[i4].isAddition() && CHILD(i3)[i4] == CHILD(i)[i2]) {
													MathStructure *mfac = &CHILD(i)[i2];
													mfac->ref();
													CHILD(i).delChild(i2 + 1, true);
													CHILD(i3).delChild(i4 + 1, true);
													CHILD(i3).ref();
													CHILD(i).add_nocopy(&CHILD(i3));
													CHILD(i).calculateAddLast(eo);
													CHILD(i).multiply_nocopy(mfac);
													CHILD_UPDATED(i)
													delChild(i3 + 1, true);
													evalSort(true);
													factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
													return true;
												}
											}
										}
									}
									if(SIZE > 2) {
										MathStructure mtest(*this);
										mtest.delChild(i + 1);
										if(mtest == CHILD(i)[i2]) {
											CHILD(i).delChild(i2 + 1, true);
											SET_CHILD_MAP(i);
											add(m_one, true);
											multiply(mtest);
											evalSort(true);
											factorize(eo, false, term_combination_levels, 0, only_integers, recursive, endtime_p, force_factorization, complete_square, only_sqrfree, max_factor_degree);
											return true;
										}
									}
								}
							}
						}
					}
				}
				return true;
			}
		}
	}
	return false;
}

