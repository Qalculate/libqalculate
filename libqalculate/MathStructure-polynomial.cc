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
#include <algorithm>
#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool sym_desc::operator<(const sym_desc &x) const {
	if (max_deg == x.max_deg) return max_lcnops < x.max_lcnops;
	else return max_deg.isLessThan(x.max_deg);
}

int MathStructure::polynomialUnit(const MathStructure &xvar) const {
	MathStructure coeff;
	lcoefficient(xvar, coeff);
	if(coeff.hasNegativeSign()) return -1;
	return 1;
}

void integer_content(const MathStructure &mpoly, Number &icontent) {
	if(mpoly.isNumber()) {
		icontent = mpoly.number();
		icontent.abs();
	} else if(mpoly.isAddition()) {
		icontent.clear();
		Number l(1, 1);
		for(size_t i = 0; i < mpoly.size(); i++) {
			if(mpoly[i].isNumber()) {
				if(!icontent.isOne()) {
					Number c = icontent;
					icontent = mpoly[i].number().numerator();
					icontent.gcd(c);
				}
				Number l2 = l;
				l = mpoly[i].number().denominator();
				l.lcm(l2);
			} else if(mpoly[i].isMultiplication()) {
				if(!icontent.isOne()) {
					Number c = icontent;
					icontent = mpoly[i].overallCoefficient().numerator();
					icontent.gcd(c);
				}
				Number l2 = l;
				l = mpoly[i].overallCoefficient().denominator();
				l.lcm(l2);
			} else {
				icontent.set(1, 1, 0);
			}
		}
		icontent /= l;
	} else if(mpoly.isMultiplication()) {
		icontent = mpoly.overallCoefficient();
		icontent.abs();
	} else {
		icontent.set(1, 1, 0);
	}
}
void MathStructure::polynomialContent(const MathStructure &xvar, MathStructure &mcontent, const EvaluationOptions &eo) const {
	if(isZero()) {
		mcontent.clear();
		return;
	}
	if(isNumber()) {
		mcontent = *this;
		mcontent.number().setNegative(false);
		return;
	}

	MathStructure c;
	integer_content(*this, c.number());
	MathStructure r(*this);
	if(!c.isOne()) r.calculateDivide(c, eo);
	MathStructure lcoeff;
	r.lcoefficient(xvar, lcoeff);
	if(lcoeff.isInteger()) {
		mcontent = c;
		return;
	}
	Number deg(r.degree(xvar));
	Number ldeg(r.ldegree(xvar));
	if(deg == ldeg) {
		mcontent = lcoeff;
		if(lcoeff.polynomialUnit(xvar) == -1) {
			c.number().negate();
		}
		mcontent.calculateMultiply(c, eo);
		return;
	}
	mcontent.clear();
	MathStructure mtmp, coeff;
	for(Number i(ldeg); i.isLessThanOrEqualTo(deg); i++) {
		coefficient(xvar, i, coeff);
		mtmp = mcontent;
		if(!MathStructure::gcd(coeff, mtmp, mcontent, eo, NULL, NULL, false)) mcontent.set(1, 1, 0);
		if(mcontent.isOne()) break;
	}

	if(!c.isOne()) mcontent.calculateMultiply(c, eo);

}

void MathStructure::polynomialPrimpart(const MathStructure &xvar, MathStructure &mprim, const EvaluationOptions &eo) const {
	if(isZero()) {
		mprim.clear();
		return;
	}
	if(isNumber()) {
		mprim.set(1, 1, 0);
		return;
	}

	MathStructure c;
	polynomialContent(xvar, c, eo);
	if(c.isZero()) {
		mprim.clear();
		return;
	}
	bool b = (polynomialUnit(xvar) == -1);
	if(c.isNumber()) {
		if(b) c.number().negate();
		mprim = *this;
		mprim.calculateDivide(c, eo);
		return;
	}
	if(b) c.calculateNegate(eo);
	MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
}
void MathStructure::polynomialUnitContentPrimpart(const MathStructure &xvar, int &munit, MathStructure &mcontent, MathStructure &mprim, const EvaluationOptions &eo) const {

	if(isZero()) {
		munit = 1;
		mcontent.clear();
		mprim.clear();
		return;
	}

	if(isNumber()) {
		if(o_number.isNegative()) {
			munit = -1;
			mcontent = *this;
			mcontent.number().abs();
		} else {
			munit = 1;
			mcontent = *this;
		}
		mprim.set(1, 1, 0);
		return;
	}

	munit = polynomialUnit(xvar);

	polynomialContent(xvar, mcontent, eo);

	if(mcontent.isZero()) {
		mprim.clear();
		return;
	}
	if(mcontent.isNumber()) {
		mprim = *this;
		if(munit == -1) {
			Number c(mcontent.number());
			c.negate();
			mprim.calculateDivide(c, eo);
		} else if(!mcontent.isOne()) {
			mprim.calculateDivide(mcontent, eo);
		}
	} else {
		if(munit == -1) {
			MathStructure c(mcontent);
			c.calculateNegate(eo);
			MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
		} else {
			MathStructure::polynomialQuotient(*this, mcontent, xvar, mprim, eo, false);
		}
	}
}


void MathStructure::polynomialPrimpart(const MathStructure &xvar, const MathStructure &c, MathStructure &mprim, const EvaluationOptions &eo) const {
	if(isZero() || c.isZero()) {
		mprim.clear();
		return;
	}
	if(isNumber()) {
		mprim.set(1, 1, 0);
		return;
	}
	bool b = (polynomialUnit(xvar) == -1);
	if(c.isNumber()) {
		MathStructure cn(c);
		if(b) cn.number().negate();
		mprim = *this;
		mprim.calculateDivide(cn, eo);
		return;
	}
	if(b) {
		MathStructure cn(c);
		cn.calculateNegate(eo);
		MathStructure::polynomialQuotient(*this, cn, xvar, mprim, eo, false);
	} else {
		MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
	}
}

const Number& MathStructure::degree(const MathStructure &xvar) const {
	const Number *c = NULL;
	const MathStructure *mcur = NULL;
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			if(!c) {
				c = &nr_one;
			}
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if(!c || c->isLessThan((*mcur)[1].number())) {
				c = &(*mcur)[1].number();
			}
		} else if(mcur->isMultiplication()) {
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					if(!c) {
						c = &nr_one;
					}
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					if(!c || c->isLessThan((*mcur)[i2][1].number())) {
						c = &(*mcur)[i2][1].number();
					}
				}
			}
		}
		if(!isAddition()) break;
	}
	if(!c) return nr_zero;
	return *c;
}
const Number& MathStructure::ldegree(const MathStructure &xvar) const {
	const Number *c = NULL;
	const MathStructure *mcur = NULL;
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			c = &nr_one;
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if(!c || c->isGreaterThan((*mcur)[1].number())) {
				c = &(*mcur)[1].number();
			}
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					c = &nr_one;
					b = true;
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					if(!c || c->isGreaterThan((*mcur)[i2][1].number())) {
						c = &(*mcur)[i2][1].number();
					}
					b = true;
				}
			}
			if(!b) return nr_zero;
		} else {
			return nr_zero;
		}
		if(!isAddition()) break;
	}
	if(!c) return nr_zero;
	return *c;
}
void MathStructure::lcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const {
	coefficient(xvar, degree(xvar), mcoeff);
}
void MathStructure::tcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const {
	coefficient(xvar, ldegree(xvar), mcoeff);
}
void MathStructure::coefficient(const MathStructure &xvar, const Number &pownr, MathStructure &mcoeff) const {
	const MathStructure *mcur = NULL;
	mcoeff.clear();
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			if(pownr.isOne()) {
				if(mcoeff.isZero()) mcoeff.set(1, 1, 0);
				else mcoeff.add(m_one, true);
			}
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if((*mcur)[1].number() == pownr) {
				if(mcoeff.isZero()) mcoeff.set(1, 1, 0);
				else mcoeff.add(m_one, true);
			}
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				bool b2 = false;
				if((*mcur)[i2] == xvar) {
					b = true;
					if(pownr.isOne()) b2 = true;
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					b = true;
					if((*mcur)[i2][1].number() == pownr) b2 = true;
				}
				if(b2) {
					if(mcoeff.isZero()) {
						if(mcur->size() == 1) {
							mcoeff.set(1, 1, 0);
						} else {
							for(size_t i3 = 0; i3 < mcur->size(); i3++) {
								if(i3 != i2) {
									if(mcoeff.isZero()) mcoeff = (*mcur)[i3];
									else mcoeff.multiply((*mcur)[i3], true);
								}
							}
						}
					} else if(mcur->size() == 1) {
						mcoeff.add(m_one, true);
					} else {
						mcoeff.add(m_zero, true);
						for(size_t i3 = 0; i3 < mcur->size(); i3++) {
							if(i3 != i2) {
								if(mcoeff[mcoeff.size() - 1].isZero()) mcoeff[mcoeff.size() - 1] = (*mcur)[i3];
								else mcoeff[mcoeff.size() - 1].multiply((*mcur)[i3], true);
							}
						}
					}
					break;
				}
			}
			if(!b && pownr.isZero()) {
				if(mcoeff.isZero()) mcoeff = *mcur;
				else mcoeff.add(*mcur, true);
			}
		} else if(pownr.isZero()) {
			if(mcoeff.isZero()) mcoeff = *mcur;
			else mcoeff.add(*mcur, true);
		}
		if(!isAddition()) break;
	}
	mcoeff.evalSort();
}

bool MathStructure::polynomialQuotient(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args) {
	mquotient.clear();
	if(CALCULATOR->aborted()) return false;
	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber() && mnum.isNumber()) {
		mquotient = mnum;
		if(IS_REAL(mden) && IS_REAL(mnum)) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	}
	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}
	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	EvaluationOptions eo2 = eo;
	eo2.keep_zero_units = false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);
	MathStructure mrem(mnum);
	for(size_t i = 0; numdeg.isGreaterThanOrEqualTo(dendeg); i++) {
		if(i > 1000 || CALCULATOR->aborted()) {
			return false;
		}
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1, 0);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo2);
				}
			} else {
				MathStructure mcopy(numcoeff);
				if(!MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo2, false)) {
					return false;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo2);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.calculateAdd(numcoeff, eo2);
		numcoeff.calculateMultiply(mden, eo2);
		mrem.calculateSubtract(numcoeff, eo2);
		if(mrem.isZero()) break;
		numdeg = mrem.degree(xvar);
	}
	return true;

}
void add_symbol(const MathStructure &mpoly, sym_desc_vec &v) {
	sym_desc_vec::const_iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		if(it->sym == mpoly) return;
		++it;
	}
	sym_desc d;
	d.sym = mpoly;
	d.max_lcnops = 0;
	v.push_back(d);
}

void collect_symbols(const MathStructure &mpoly, sym_desc_vec &v) {
	if(IS_A_SYMBOL(mpoly) || mpoly.isUnit()) {
		add_symbol(mpoly, v);
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) collect_symbols(mpoly[i], v);
	} else if(mpoly.isPower()) {
		collect_symbols(mpoly[0], v);
	}
}

void add_symbol(const MathStructure &mpoly, vector<MathStructure> &v) {
	vector<MathStructure>::const_iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		if(*it == mpoly) return;
		++it;
	}
	v.push_back(mpoly);
}

void collect_symbols(const MathStructure &mpoly, vector<MathStructure> &v) {
	if(IS_A_SYMBOL(mpoly)) {
		add_symbol(mpoly, v);
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) collect_symbols(mpoly[i], v);
	} else if(mpoly.isPower()) {
		collect_symbols(mpoly[0], v);
	}
}

void get_symbol_stats(const MathStructure &m1, const MathStructure &m2, sym_desc_vec &v) {
	collect_symbols(m1, v);
	collect_symbols(m2, v);
	sym_desc_vec::iterator it = v.begin(), itend = v.end();
	while(it != itend) {
		it->deg_a = m1.degree(it->sym);
		it->deg_b = m2.degree(it->sym);
		if(it->deg_a.isGreaterThan(it->deg_b)) {
			it->max_deg = it->deg_a;
		} else {
			it->max_deg = it->deg_b;
		}
		it->ldeg_a = m1.ldegree(it->sym);
		it->ldeg_b = m2.ldegree(it->sym);
		MathStructure mcoeff;
		m1.lcoefficient(it->sym, mcoeff);
		it->max_lcnops = mcoeff.size();
		m2.lcoefficient(it->sym, mcoeff);
		if(mcoeff.size() > it->max_lcnops) it->max_lcnops = mcoeff.size();
		++it;
	}
	std::sort(v.begin(), v.end());

}

bool get_first_symbol(const MathStructure &mpoly, MathStructure &xvar) {
	if(IS_A_SYMBOL(mpoly) || mpoly.isUnit()) {
		xvar = mpoly;
		return true;
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) {
			if(get_first_symbol(mpoly[i], xvar)) return true;
		}
	} else if(mpoly.isPower()) {
		return get_first_symbol(mpoly[0], xvar);
	}
	return false;
}

bool MathStructure::polynomialDivide(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args) {

	mquotient.clear();

	if(CALCULATOR->aborted()) return false;

	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber()) {
		mquotient = mnum;
		if(mnum.isNumber()) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	} else if(mnum.isNumber()) {
		return false;
	}

	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}

	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}


	MathStructure xvar;
	if(!get_first_symbol(mnum, xvar) && !get_first_symbol(mden, xvar)) return false;

	EvaluationOptions eo2 = eo;
	eo2.keep_zero_units = false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);

	MathStructure mrem(mnum);

	for(size_t i = 0; numdeg.isGreaterThanOrEqualTo(dendeg); i++) {
		if(i > 1000 || CALCULATOR->aborted()) {
			return false;
		}
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1, 0);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo2);
				}
			} else {
				MathStructure mcopy(numcoeff);
				if((mcopy.equals(mnum, true, true) && dencoeff.equals(mden, true, true)) || !MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo2, false)) {
					return false;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo2);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.add(numcoeff, true);
		if(numcoeff.isAddition() && mden.isAddition() && numcoeff.size() * mden.size() >= (eo.expand == -1 ? 50 : 500)) return false;
		numcoeff.calculateMultiply(mden, eo2);
		mrem.calculateSubtract(numcoeff, eo2);
		if(mrem.isZero()) {
			return true;
		}
		numdeg = mrem.degree(xvar);
	}
	return false;
}

bool divide_in_z(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const sym_desc_vec &sym_stats, size_t var_i, const EvaluationOptions &eo) {

	mquotient.clear();
	if(mden.isZero()) return false;
	if(mnum.isZero()) return true;
	if(mden.isOne()) {
		mquotient = mnum;
		return true;
	}
	if(mnum.isNumber()) {
		if(!mden.isNumber()) {
			return false;
		}
		mquotient = mnum;
		return mquotient.number().divide(mden.number()) && mquotient.isInteger();
	}
	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}

	if(mden.isPower()) {
		MathStructure qbar(mnum);
		for(Number ni(mden[1].number()); ni.isPositive(); ni--) {
			if(!divide_in_z(qbar, mden[0], mquotient, sym_stats, var_i, eo)) return false;
			qbar = mquotient;
		}
		return true;
	}

	if(mden.isMultiplication()) {
		MathStructure qbar(mnum);
		for(size_t i = 0; i < mden.size(); i++) {
			sym_desc_vec sym_stats2;
			get_symbol_stats(mnum, mden[i], sym_stats2);
			if(!divide_in_z(qbar, mden[i], mquotient, sym_stats2, 0, eo)) return false;
			qbar = mquotient;
		}
		return true;
	}

	if(var_i >= sym_stats.size()) return false;
	const MathStructure &xvar = sym_stats[var_i].sym;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	if(dendeg.isGreaterThan(numdeg)) return false;
	MathStructure dencoeff;
	MathStructure mrem(mnum);
	mden.coefficient(xvar, dendeg, dencoeff);

	while(numdeg.isGreaterThanOrEqualTo(dendeg)) {
		if(CALCULATOR->aborted()) return false;
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		MathStructure term;
		if(!divide_in_z(numcoeff, dencoeff, term, sym_stats, var_i + 1, eo)) break;
		numdeg -= dendeg;
		if(!numdeg.isZero() && !term.isZero()) {
			if(term.isOne()) {
				term = xvar;
				if(!numdeg.isOne()) {
					term.raise(numdeg);
				}
			} else {
				term.multiply(xvar, true);
				if(!numdeg.isOne()) {
					term[term.size() - 1].raise(numdeg);
				}
				term.calculateMultiplyLast(eo);
			}
		}
		if(mquotient.isZero()) {
			mquotient = term;
		} else {
			mquotient.calculateAdd(term, eo);
		}
		if(term.isAddition() && mden.isAddition() && term.size() * mden.size() >= (eo.expand == -1 ? 50 : 500)) return false;
		term.calculateMultiply(mden, eo);
		mrem.calculateSubtract(term, eo);
		if(mrem.isZero()) {
			return true;
		}
		numdeg = mrem.degree(xvar);

	}
	return false;
}

bool prem(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mrem, const EvaluationOptions &eo, bool check_args) {

	mrem.clear();
	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isNumber()) {
		if(!mden.isNumber()) {
			mrem = mden;
		}
		return true;
	}
	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	mrem = mnum;
	MathStructure eb(mden);
	Number rdeg = mrem.degree(xvar);
	Number bdeg = eb.degree(xvar);
	MathStructure blcoeff;
	if(bdeg.isLessThanOrEqualTo(rdeg)) {
		eb.coefficient(xvar, bdeg, blcoeff);
		if(bdeg == 0) {
			eb.clear();
		} else {
			MathStructure mpow(xvar);
			mpow.raise(bdeg);
			mpow.calculateRaiseExponent(eo);
			//POWER_CLEAN(mpow)
			mpow.calculateMultiply(blcoeff, eo);
			eb.calculateSubtract(mpow, eo);
		}
	} else {
		blcoeff.set(1, 1, 0);
	}

	Number delta(rdeg);
	delta -= bdeg;
	delta++;
	int i = 0;
	while(rdeg.isGreaterThanOrEqualTo(bdeg) && !mrem.isZero()) {
		if(CALCULATOR->aborted() || delta < i / 10) {mrem.clear(); return false;}
		MathStructure rlcoeff;
		mrem.coefficient(xvar, rdeg, rlcoeff);
		MathStructure term(xvar);
		term.raise(rdeg);
		term[1].number() -= bdeg;
		term.calculateRaiseExponent(eo);
		//POWER_CLEAN(term)
		term.calculateMultiply(rlcoeff, eo);
		term.calculateMultiply(eb, eo);
		if(rdeg == 0) {
			mrem = term;
			mrem.calculateNegate(eo);
		} else {
			if(!rdeg.isZero()) {
				rlcoeff.multiply(xvar, true);
				if(!rdeg.isOne()) {
					rlcoeff[rlcoeff.size() - 1].raise(rdeg);
					rlcoeff[rlcoeff.size() - 1].calculateRaiseExponent(eo);
				}
				rlcoeff.calculateMultiplyLast(eo);
			}
			mrem.calculateSubtract(rlcoeff, eo);
			if(mrem.isAddition() && blcoeff.isAddition() && mrem.size() * blcoeff.size() >= (eo.expand == -1 ? 50 : 500)) {mrem.clear(); return false;}
			mrem.calculateMultiply(blcoeff, eo);
			mrem.calculateSubtract(term, eo);
		}
		rdeg = mrem.degree(xvar);
		i++;
	}
	delta -= i;
	blcoeff.raise(delta);
	blcoeff.calculateRaiseExponent(eo);
	mrem.calculateMultiply(blcoeff, eo);
	return true;
}

Number MathStructure::maxCoefficient() {
	if(isNumber()) {
		Number nr(o_number);
		nr.abs();
		return nr;
	} else if(isAddition()) {
		Number cur_max(overallCoefficient());
		cur_max.abs();
		for(size_t i = 0; i < SIZE; i++) {
			Number a(CHILD(i).overallCoefficient());
			a.abs();
			if(a.isGreaterThan(cur_max)) cur_max = a;
		}
		return cur_max;
	} else if(isMultiplication()) {
		Number nr(overallCoefficient());
		nr.abs();
		return nr;
	} else {
		return nr_one;
	}
}
void polynomial_smod(const MathStructure &mpoly, const Number &xi, MathStructure &msmod, const EvaluationOptions &eo, MathStructure *mparent, size_t index_smod) {
	if(mpoly.isNumber()) {
		msmod = mpoly;
		msmod.number().smod(xi);
	} else if(mpoly.isAddition()) {
		msmod.clear();
		msmod.setType(STRUCT_ADDITION);
		msmod.resizeVector(mpoly.size(), m_zero);
		for(size_t i = 0; i < mpoly.size(); i++) {
			polynomial_smod(mpoly[i], xi, msmod[i], eo, &msmod, i);
		}
		msmod.calculatesub(eo, eo, false, mparent, index_smod);
	} else if(mpoly.isMultiplication()) {
		msmod = mpoly;
		if(msmod.size() > 0 && msmod[0].isNumber()) {
			if(!msmod[0].number().smod(xi) || msmod[0].isZero()) {
				msmod.clear();
			}
		}
	} else {
		msmod = mpoly;
	}
}
bool contains_zerointerval_multiplier(MathStructure &mstruct) {
	if(mstruct.isAddition()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(contains_zerointerval_multiplier(mstruct[i])) return true;
		}
	} else if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isNumber() && !mstruct[i].number().isNonZero()) return true;
		}
	} else if(mstruct.isNumber() && !mstruct.number().isNonZero()) {
		return true;
	}
	return false;
}

bool polynomial_long_division(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar_pre, MathStructure &mquotient, MathStructure &mrem, const EvaluationOptions &eo, bool check_args, bool for_newtonraphson) {
	mquotient.clear();
	mrem.clear();

	if(CALCULATOR->aborted()) return false;
	if(mden.isZero()) {
		//division by zero
		mrem.set(mnum);
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber() && mnum.isNumber()) {
		mquotient = mnum;
		if(IS_REAL(mden) && IS_REAL(mnum)) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	} else if(mnum.isNumber()) {
		mrem.set(mnum);
		return false;
	}
	if(mnum == mden) {
		mquotient.set(1, 1, 0);
		return true;
	}

	mrem.set(mnum);

	if(check_args && (!mnum.isRationalPolynomial(true, true) || !mden.isRationalPolynomial(true, true))) {
		return false;
	}

	MathStructure xvar(xvar_pre);
	if(xvar.isZero()) {
		vector<MathStructure> symsd, symsn;
		collect_symbols(mnum, symsn);
		if(symsn.empty()) return false;
		collect_symbols(mden, symsd);
		if(symsd.empty()) return false;
		for(size_t i = 0; i < symsd.size();) {
			bool b_found = false;
			for(size_t i2 = 0; i2 < symsn.size(); i2++) {
				if(symsn[i2] == symsd[i]) {
					b_found = (mnum.degree(symsd[i]) >= mden.degree(symsd[i]));
					break;
				}
			}
			if(b_found) i++;
			else symsd.erase(symsd.begin() + i);
		}
		for(size_t i = 0; i < symsd.size(); i++) {
			MathStructure mquotient2, mrem2;
			if(polynomial_long_division(mrem, mden, symsd[i], mquotient2, mrem2, eo, false, for_newtonraphson) && !mquotient2.isZero()) {
				mrem = mrem2;
				if(mquotient.isZero()) mquotient = mquotient2;
				else mquotient.calculateAdd(mquotient2, eo);
			}
			if(CALCULATOR->aborted()) {
				mrem = mnum;
				mquotient.clear();
				return false;
			}
		}
		return true;
	}

	EvaluationOptions eo2 = eo;
	eo2.keep_zero_units = false;
	eo2.do_polynomial_division = false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);

	for(size_t i = 0; numdeg.isGreaterThanOrEqualTo(dendeg); i++) {
		if(i > 10000 || CALCULATOR->aborted()) {
			mrem = mnum;
			mquotient.clear();
			return false;
		}
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1, 0);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo2);
				}
			} else {
				if(for_newtonraphson) {mrem = mnum; mquotient.clear(); return false;}
				MathStructure mcopy(numcoeff);
				if(!MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo2, check_args)) {
					if(CALCULATOR->aborted()) {mrem = mnum; mquotient.clear(); return false;}
					break;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo2);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.calculateAdd(numcoeff, eo2);
		if(numcoeff.isAddition() && mden.isAddition() && numcoeff.size() * mden.size() >= (eo.expand == -1 ? 50 : 500)) {mrem = mnum; mquotient.clear(); return false;}
		numcoeff.calculateMultiply(mden, eo2);
		mrem.calculateSubtract(numcoeff, eo2);
		if(contains_zerointerval_multiplier(mquotient)) {mrem = mnum; mquotient.clear(); return false;}
		if(mrem.isZero() || (for_newtonraphson && mrem.isNumber())) break;
		if(contains_zerointerval_multiplier(mrem)) {mrem = mnum; mquotient.clear(); return false;}
		numdeg = mrem.degree(xvar);
	}
	return true;

}
bool MathStructure::isRationalPolynomial(bool allow_non_rational_coefficient, bool allow_interval_coefficient) const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			if(allow_interval_coefficient) return o_number.isReal() && o_number.isNonZero();
			if(allow_non_rational_coefficient) return o_number.isReal() && !o_number.isInterval() && o_number.isNonZero();
			return o_number.isRational() && !o_number.isZero();
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isAddition() || CHILD(i).isMultiplication() || !CHILD(i).isRationalPolynomial(allow_non_rational_coefficient, allow_interval_coefficient)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isAddition() || !CHILD(i).isRationalPolynomial(allow_non_rational_coefficient, allow_interval_coefficient)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(1).isInteger() && CHILD(1).number().isNonNegative() && !CHILD(1).number().isOne() && CHILD(1).number() < 1000 && !CHILD(0).isNumber() && !CHILD(0).isMultiplication() && !CHILD(0).isAddition() && !CHILD(0).isPower() && CHILD(0).isRationalPolynomial(allow_non_rational_coefficient, allow_interval_coefficient);
		}
		case STRUCT_FUNCTION: {
			if(o_function->id() == FUNCTION_ID_UNCERTAINTY || o_function->id() == FUNCTION_ID_INTERVAL || containsInterval() || containsInfinity()) return false;
		}
		case STRUCT_UNIT: {}
		case STRUCT_VARIABLE: {}
		case STRUCT_SYMBOLIC: {
			return representsNonMatrix() && !representsUndefined(true, true);
		}
		default: {}
	}
	return false;
}
const Number &MathStructure::overallCoefficient() const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			return o_number;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isNumber()) {
					return CHILD(i).number();
				}
			}
			return nr_one;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isNumber()) {
					return CHILD(i).number();
				}
			}
			return nr_zero;
		}
		case STRUCT_POWER: {
			return nr_zero;
		}
		default: {}
	}
	return nr_zero;
}

