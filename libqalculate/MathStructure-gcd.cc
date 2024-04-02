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

#if HAVE_UNORDERED_MAP
#	include <unordered_map>
	using std::unordered_map;
#elif 	defined(__GNUC__)

#	ifndef __has_include
#	define __has_include(x) 0
#	endif

#	if (defined(__clang__) && __has_include(<tr1/unordered_map>)) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 3)
#		include <tr1/unordered_map>
		namespace Sgi = std;
#		define unordered_map std::tr1::unordered_map
#	else
#		if __GNUC__ < 3
#			include <hash_map.h>
			namespace Sgi { using ::hash_map; }; // inherit globals
#		else
#			include <ext/hash_map>
#			if __GNUC__ == 3 && __GNUC_MINOR__ == 0
				namespace Sgi = std;               // GCC 3.0
#			else
				namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
#			endif
#		endif
#		define unordered_map Sgi::hash_map
#	endif
#else      // ...  there are other compilers, right?
	namespace Sgi = std;
#	define unordered_map Sgi::hash_map
#endif

#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

bool sr_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const sym_desc_vec &sym_stats, size_t var_i, const EvaluationOptions &eo) {

	if(var_i >= sym_stats.size()) return false;
	const MathStructure &xvar = sym_stats[var_i].sym;

	MathStructure c, d;
	Number adeg = m1.degree(xvar);
	Number bdeg = m2.degree(xvar);
	Number cdeg, ddeg;
	if(adeg.isGreaterThanOrEqualTo(bdeg)) {
		c = m1;
		d = m2;
		cdeg = adeg;
		ddeg = bdeg;
	} else {
		c = m2;
		d = m1;
		cdeg = bdeg;
		ddeg = adeg;
	}

	MathStructure cont_c, cont_d;
	c.polynomialContent(xvar, cont_c, eo);
	d.polynomialContent(xvar, cont_d, eo);

	MathStructure gamma;

	if(!MathStructure::gcd(cont_c, cont_d, gamma, eo, NULL, NULL, false)) return false;
	mgcd = gamma;
	if(ddeg.isZero()) {
		return true;
	}

	MathStructure prim_c, prim_d;
	c.polynomialPrimpart(xvar, cont_c, prim_c, eo);
	d.polynomialPrimpart(xvar, cont_d, prim_d, eo);
	c = prim_c;
	d = prim_d;

	MathStructure r;
	MathStructure ri(1, 1, 0);
	MathStructure psi(1, 1, 0);
	Number delta(cdeg);
	delta -= ddeg;

	while(true) {

		if(CALCULATOR->aborted()) return false;

		if(!prem(c, d, xvar, r, eo, false)) return false;

		if(r.isZero()) {
			mgcd = gamma;
			MathStructure mprim;
			d.polynomialPrimpart(xvar, mprim, eo);
			if(CALCULATOR->aborted()) return false;
			mgcd.calculateMultiply(mprim, eo);
			return true;
		}

		c = d;
		cdeg = ddeg;

		MathStructure psi_pow(psi);
		psi_pow.calculateRaise(delta, eo);
		ri.calculateMultiply(psi_pow, eo);

		if(!divide_in_z(r, ri, d, sym_stats, var_i, eo)) {
			return false;
		}

		ddeg = d.degree(xvar);
		if(ddeg.isZero()) {
			if(r.isNumber()) {
				mgcd = gamma;
			} else {
				r.polynomialPrimpart(xvar, mgcd, eo);
				if(CALCULATOR->aborted()) return false;
				mgcd.calculateMultiply(gamma, eo);
			}
			return true;
		}

		c.lcoefficient(xvar, ri);
		if(delta.isOne()) {
			psi = ri;
		} else if(!delta.isZero()) {
			MathStructure ri_pow(ri);
			ri_pow.calculateRaise(delta, eo);
			MathStructure psi_pow(psi);
			delta--;
			psi_pow.calculateRaise(delta, eo);
			divide_in_z(ri_pow, psi_pow, psi, sym_stats, var_i + 1, eo);
		}
		delta = cdeg;
		delta -= ddeg;

	}

	return false;

}


bool interpolate(const MathStructure &gamma, const Number &xi, const MathStructure &xvar, MathStructure &minterp, const EvaluationOptions &eo) {
	MathStructure e(gamma);
	Number rxi(xi);
	rxi.recip();
	minterp.clear();
	for(long int i = 0; !e.isZero(); i++) {
		if(CALCULATOR->aborted()) return false;
		MathStructure gi;
		polynomial_smod(e, xi, gi, eo);
		if(minterp.isZero() && !gi.isZero()) {
			minterp = gi;
			if(i != 0) {
				if(minterp.isOne()) {
					minterp = xvar;
					if(i != 1) minterp.raise(i);
				} else {
					minterp.multiply(xvar, true);
					if(i != 1) minterp[minterp.size() - 1].raise(i);
					minterp.calculateMultiplyLast(eo);
				}
			}
		} else if(!gi.isZero()) {
			minterp.add(gi, true);
			if(i != 0) {
				if(minterp[minterp.size() - 1].isOne()) {
					minterp[minterp.size() - 1] = xvar;
					if(i != 1) minterp[minterp.size() - 1].raise(i);
				} else {
					minterp[minterp.size() - 1].multiply(xvar, true);
					if(i != 1) minterp[minterp.size() - 1][minterp[minterp.size() - 1].size() - 1].raise(i);
					minterp[minterp.size() - 1].calculateMultiplyLast(eo);
				}
			}
		}
		if(!gi.isZero()) e.calculateSubtract(gi, eo);
		e.calculateMultiply(rxi, eo);
	}
	minterp.calculatesub(eo, eo, false);
	return true;
}

bool heur_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const EvaluationOptions &eo, MathStructure *ca, MathStructure *cb, sym_desc_vec &sym_stats, size_t var_i) {

	if(m1.isZero() || m2.isZero())	return false;

	if(m1.isNumber() && m2.isNumber()) {
		mgcd = m1;
		if(!m1.isInteger() || !m2.isInteger() || !mgcd.number().gcd(m2.number())) mgcd.set(1, 1, 0);
		if(ca) {
			*ca = m1;
			ca->number() /= mgcd.number();
		}
		if(cb) {
			*cb = m2;
			cb->number() /= mgcd.number();
		}
		return true;
	}

	if(var_i >= sym_stats.size()) return false;
	const MathStructure &xvar = sym_stats[var_i].sym;

	Number nr_gc;
	integer_content(m1, nr_gc);
	Number nr_rgc;
	integer_content(m2, nr_rgc);
	nr_gc.gcd(nr_rgc);
	nr_rgc = nr_gc;
	nr_rgc.recip();
	MathStructure p(m1);
	p.calculateMultiply(nr_rgc, eo);
	MathStructure q(m2);
	q.calculateMultiply(nr_rgc, eo);
	Number maxdeg(p.degree(xvar));
	Number maxdeg2(q.degree(xvar));
	if(maxdeg2.isGreaterThan(maxdeg)) maxdeg = maxdeg2;
	Number mp(p.maxCoefficient());
	Number mq(q.maxCoefficient());
	Number xi;
	if(mp.isGreaterThan(mq)) {
		xi = mq;
	} else {
		xi = mp;
	}
	xi *= 2;
	xi += 2;

	for(int t = 0; t < 6; t++) {

		if(CALCULATOR->aborted()) return false;

		if(!xi.isInteger() || (maxdeg * xi.integerLength()).isGreaterThan(100000L)) {
			return false;
		}

		MathStructure cp, cq;
		MathStructure gamma;
		MathStructure psub(p);
		psub.calculateReplace(xvar, xi, eo, true);
		MathStructure qsub(q);
		qsub.calculateReplace(xvar, xi, eo, true);

		if(heur_gcd(psub, qsub, gamma, eo, &cp, &cq, sym_stats, var_i + 1)) {

			if(!interpolate(gamma, xi, xvar, mgcd, eo)) return false;

			Number ig;
			integer_content(mgcd, ig);
			ig.recip();
			mgcd.calculateMultiply(ig, eo);

			MathStructure dummy;

			if(divide_in_z(p, mgcd, ca ? *ca : dummy, sym_stats, var_i, eo) && divide_in_z(q, mgcd, cb ? *cb : dummy, sym_stats, var_i, eo)) {
				mgcd.calculateMultiply(nr_gc, eo);
				return true;
			}
		}

		Number xi2(xi);
		xi2.isqrt();
		xi2.isqrt();
		xi *= xi2;
		xi *= 73794L;
		xi.iquo(27011L);

	}

	return false;

}

bool MathStructure::lcm(const MathStructure &m1, const MathStructure &m2, MathStructure &mlcm, const EvaluationOptions &eo, bool check_args) {
	if(m1.isNumber() && m2.isNumber()) {
		mlcm = m1;
		if(!mlcm.isInteger() || !m2.isInteger()) return mlcm.number().multiply(m2.number());
		return mlcm.number().lcm(m2.number());
	}
	if(check_args && (!m1.isRationalPolynomial() || !m2.isRationalPolynomial())) {
		return false;
	}
	MathStructure ca, cb;
	if(!MathStructure::gcd(m1, m2, mlcm, eo, &ca, &cb, false)) return false;
	mlcm.calculateMultiply(ca, eo);
	mlcm.calculateMultiply(cb, eo);
	return true;
}

bool has_noninteger_coefficient(const MathStructure &mstruct) {
	if(mstruct.isNumber() && mstruct.number().isRational() && !mstruct.number().isInteger()) return true;
	if(mstruct.isFunction() || mstruct.isPower()) return false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(has_noninteger_coefficient(mstruct[i])) return true;
	}
	return false;
}

bool MathStructure::gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mresult, const EvaluationOptions &eo2, MathStructure *ca, MathStructure *cb, bool check_args) {

	EvaluationOptions eo = eo2;
	eo.keep_zero_units = false;

	if(ca) *ca = m1;
	if(cb) *cb = m2;
	mresult.set(1, 1, 0);

	if(CALCULATOR->aborted()) return false;

	if(m1.isOne() || m2.isOne()) {
		return true;
	}
	if(m1.isNumber() && m2.isNumber()) {
		mresult = m1;
		if(!m1.isInteger() || !m2.isInteger() || !mresult.number().gcd(m2.number())) mresult.set(1, 1, 0);
		if(ca || cb) {
			if(mresult.isZero()) {
				if(ca) ca->clear();
				if(cb) cb->clear();
			} else {
				if(ca) {
					*ca = m1;
					ca->number() /= mresult.number();
				}
				if(cb) {
					*cb = m2;
					cb->number() /= mresult.number();
				}
			}
		}
		return true;
	}

	if(m1 == m2) {
		if(ca) ca->set(1, 1, 0);
		if(cb) cb->set(1, 1, 0);
		mresult = m1;
		return true;
	}
	if(m1.isZero()) {
		if(ca) ca->clear();
		if(cb) cb->set(1, 1, 0);
		mresult = m2;
		return true;
	}
	if(m2.isZero()) {
		if(ca) ca->set(1, 1, 0);
		if(cb) cb->clear();
		mresult = m1;
		return true;
	}

	if(check_args && (!m1.isRationalPolynomial() || !m2.isRationalPolynomial())) {
		return false;
	}

	if(has_noninteger_coefficient(m1) || has_noninteger_coefficient(m2)) {
		Number nlcm1;
		lcm_of_coefficients_denominators(m1, nlcm1);
		Number nlcm2;
		lcm_of_coefficients_denominators(m2, nlcm2);
		if(!nlcm1.isInteger() || !nlcm2.isInteger()) nlcm1.multiply(nlcm2);
		else nlcm1.lcm(nlcm2);
		if(!nlcm1.isOne()) {
			MathStructure mtmp1, mtmp2;
			multiply_lcm(m1, nlcm1, mtmp1, eo);
			multiply_lcm(m2, nlcm1, mtmp2, eo);
			if(mtmp1.equals(m1, true, true) || mtmp2.equals(m2, true, true)) return false;
			return gcd(mtmp1, mtmp2, mresult, eo, ca, cb, false);
		}
	}

	if(m1.isMultiplication()) {
		if(m2.isMultiplication() && m2.size() > m1.size())
			goto factored_2;
factored_1:
		mresult.clear();
		mresult.setType(STRUCT_MULTIPLICATION);
		MathStructure acc_ca;
		acc_ca.setType(STRUCT_MULTIPLICATION);
		MathStructure part_2(m2);
		MathStructure part_ca, part_cb;
		for(size_t i = 0; i < m1.size(); i++) {
			mresult.addChild(m_zero);
			MathStructure::gcd(m1[i], part_2, mresult[i], eo, &part_ca, &part_cb, false);
			if(CALCULATOR->aborted()) return false;
			acc_ca.addChild(part_ca);
			part_2 = part_cb;
		}
		mresult.calculatesub(eo, eo, false);
		if(ca) {
			*ca = acc_ca;
			ca->calculatesub(eo, eo, false);
		}
		if(cb) *cb = part_2;
		return true;
	} else if(m2.isMultiplication()) {
		if(m1.isMultiplication() && m1.size() > m2.size())
			goto factored_1;
factored_2:
		mresult.clear();
		mresult.setType(STRUCT_MULTIPLICATION);
		MathStructure acc_cb;
		acc_cb.setType(STRUCT_MULTIPLICATION);
		MathStructure part_1(m1);
		MathStructure part_ca, part_cb;
		for(size_t i = 0; i < m2.size(); i++) {
			mresult.addChild(m_zero);
			MathStructure::gcd(part_1, m2[i], mresult[i], eo, &part_ca, &part_cb, false);
			if(CALCULATOR->aborted()) return false;
			acc_cb.addChild(part_cb);
			part_1 = part_ca;
		}
		mresult.calculatesub(eo, eo, false);
		if(ca) *ca = part_1;
		if(cb) {
			*cb = acc_cb;
			cb->calculatesub(eo, eo, false);
		}
		return true;
	}

	if(m1.isPower()) {
		MathStructure p(m1[0]);
		if(m2.isPower()) {
			if(m1[0] == m2[0]) {
				if(m1[1].number().isLessThan(m2[1].number())) {
					if(ca) ca->set(1, 1, 0);
					if(cb) {
						*cb = m2;
						(*cb)[1].number() -= m1[1].number();
						POWER_CLEAN((*cb))
					}
					mresult = m1;
					return true;
				} else {
					if(ca) {
						*ca = m1;
						(*ca)[1].number() -= m2[1].number();
						POWER_CLEAN((*ca))
					}
					if(cb) cb->set(1, 1, 0);
					mresult = m2;
					return true;
				}
			} else {
				MathStructure p_co, pb_co;
				MathStructure p_gcd;
				if(!MathStructure::gcd(m1[0], m2[0], p_gcd, eo, &p_co, &pb_co, false)) return false;
				if(p_gcd.isOne()) {
					return true;
				} else {
					if(m1[1].number().isLessThan(m2[1].number())) {
						mresult = p_gcd;
						mresult.calculateRaise(m1[1], eo);
						mresult.multiply(m_zero, true);
						p_co.calculateRaise(m1[1], eo);
						pb_co.calculateRaise(m2[1], eo);
						p_gcd.raise(m2[1]);
						p_gcd[1].number() -= m1[1].number();
						p_gcd.calculateRaiseExponent(eo);
						p_gcd.calculateMultiply(pb_co, eo);
						bool b = MathStructure::gcd(p_co, p_gcd, mresult[mresult.size() - 1], eo, ca, cb, false);
						mresult.calculateMultiplyLast(eo);
						return b;
					} else {
						mresult = p_gcd;
						mresult.calculateRaise(m2[1], eo);
						mresult.multiply(m_zero, true);
						p_co.calculateRaise(m1[1], eo);
						pb_co.calculateRaise(m2[1], eo);
						p_gcd.raise(m1[1]);
						p_gcd[1].number() -= m2[1].number();
						p_gcd.calculateRaiseExponent(eo);
						p_gcd.calculateMultiply(p_co, eo);
						bool b = MathStructure::gcd(p_gcd, pb_co, mresult[mresult.size() - 1], eo, ca, cb, false);
						mresult.calculateMultiplyLast(eo);
						return b;
					}
				}
			}
		} else {
			if(m1[0] == m2) {
				if(ca) {
					*ca = m1;
					(*ca)[1].number()--;
					POWER_CLEAN((*ca))
				}
				if(cb) cb->set(1, 1, 0);
				mresult = m2;
				return true;
			}
			MathStructure p_co, bpart_co;
			MathStructure p_gcd;
			if(!MathStructure::gcd(m1[0], m2, p_gcd, eo, &p_co, &bpart_co, false)) return false;
			if(p_gcd.isOne()) {
				return true;
			} else {
				mresult = p_gcd;
				mresult.multiply(m_zero, true);
				p_co.calculateRaise(m1[1], eo);
				p_gcd.raise(m1[1]);
				p_gcd[1].number()--;
				p_gcd.calculateRaiseExponent(eo);
				p_gcd.calculateMultiply(p_co, eo);
				bool b = MathStructure::gcd(p_gcd, bpart_co, mresult[mresult.size() - 1], eo, ca, cb, false);
				mresult.calculateMultiplyLast(eo);
				return b;
			}
		}
	} else if(m2.isPower()) {
		if(m2[0] == m1) {
			if(ca) ca->set(1, 1, 0);
			if(cb) {
				*cb = m2;
				(*cb)[1].number()--;
				POWER_CLEAN((*cb))
			}
			mresult = m1;
			return true;
		}
		MathStructure p_co, apart_co;
		MathStructure p_gcd;
		if(!MathStructure::gcd(m1, m2[0], p_gcd, eo, &apart_co, &p_co, false)) return false;
		if(p_gcd.isOne()) {
			return true;
		} else {
			mresult = p_gcd;
			mresult.multiply(m_zero, true);
			p_co.calculateRaise(m2[1], eo);
			p_gcd.raise(m2[1]);
			p_gcd[1].number()--;
			p_gcd.calculateRaiseExponent(eo);
			p_gcd.calculateMultiply(p_co, eo);
			bool b = MathStructure::gcd(apart_co, p_gcd, mresult[mresult.size() - 1], eo, ca, cb, false);
			mresult.calculateMultiplyLast(eo);
			return b;
		}
	}
	if(IS_A_SYMBOL(m1) || m1.isUnit()) {
		MathStructure bex(m2);
		bex.calculateReplace(m1, m_zero, eo);
		if(!bex.isZero()) {
			return true;
		}
	}
	if(IS_A_SYMBOL(m2) || m2.isUnit()) {
		MathStructure aex(m1);
		aex.calculateReplace(m2, m_zero, eo);
		if(!aex.isZero()) {
			return true;
		}
	}

	sym_desc_vec sym_stats;
	get_symbol_stats(m1, m2, sym_stats);

	if(sym_stats.empty()) return false;

	size_t var_i = 0;
	const MathStructure &xvar = sym_stats[var_i].sym;

	/*for(size_t i = 0; i< sym_stats.size(); i++) {
		if(sym_stats[i].sym.size() > 0) {
			MathStructure m1b(m1), m2b(m2);
			vector<UnknownVariable*> vars;
			for(; i < sym_stats.size(); i++) {
				if(sym_stats[i].sym.size() > 0) {
					UnknownVariable *var = new UnknownVariable("", format_and_print(sym_stats[i].sym));
					m1b.replace(sym_stats[i].sym, var);
					m2b.replace(sym_stats[i].sym, var);
					vars.push_back(var);
				}
			}
			bool b = gcd(m1b, m2, mresult, eo2, ca, cb, false);
			size_t i2 = 0;
			for(i = 0; i < sym_stats.size(); i++) {
				if(sym_stats[i].sym.size() > 0) {
					mresult.replace(vars[i2], sym_stats[i].sym);
					if(ca) ca->replace(vars[i2], sym_stats[i].sym);
					if(cb) cb->replace(vars[i2], sym_stats[i].sym);
					vars[i2]->destroy();
					i2++;
				}
			}
			return b;
		}
	}*/

	Number ldeg_a(sym_stats[var_i].ldeg_a);
	Number ldeg_b(sym_stats[var_i].ldeg_b);
	Number min_ldeg;
	if(ldeg_a.isLessThan(ldeg_b)) {
		min_ldeg = ldeg_a;
	} else {
		min_ldeg = ldeg_b;
	}

	if(min_ldeg.isPositive()) {
		MathStructure aex(m1), bex(m2);
		MathStructure common(xvar);
		if(!min_ldeg.isOne()) common.raise(min_ldeg);
		aex.calculateDivide(common, eo);
		bex.calculateDivide(common, eo);
		MathStructure::gcd(aex, bex, mresult, eo, ca, cb, false);
		mresult.calculateMultiply(common, eo);
		return true;
	}

	if(sym_stats[var_i].deg_a.isZero()) {
		if(cb) {
			MathStructure c, mprim;
			int u;
			m2.polynomialUnitContentPrimpart(xvar, u, c, mprim, eo);
			if(!m2.equals(c, true, true)) MathStructure::gcd(m1, c, mresult, eo, ca, cb, false);
			if(CALCULATOR->aborted()) {
				if(ca) *ca = m1;
				if(cb) *cb = m2;
				mresult.set(1, 1, 0);
				return false;
			}
			if(u == -1) {
				cb->calculateNegate(eo);
			}
			cb->calculateMultiply(mprim, eo);
		} else {
			MathStructure c;
			m2.polynomialContent(xvar, c, eo);
			if(!m2.equals(c, true, true)) MathStructure::gcd(m1, c, mresult, eo, ca, cb, false);
		}
		return true;
	} else if(sym_stats[var_i].deg_b.isZero()) {
		if(ca) {
			MathStructure c, mprim;
			int u;
			m1.polynomialUnitContentPrimpart(xvar, u, c, mprim, eo);
			if(!m1.equals(c, true, true)) MathStructure::gcd(c, m2, mresult, eo, ca, cb, false);
			if(CALCULATOR->aborted()) {
				if(ca) *ca = m1;
				if(cb) *cb = m2;
				mresult.set(1, 1, 0);
				return false;
			}
			if(u == -1) {
				ca->calculateNegate(eo);
			}
			ca->calculateMultiply(mprim, eo);
		} else {
			MathStructure c;
			m1.polynomialContent(xvar, c, eo);
			if(!m1.equals(c, true, true)) MathStructure::gcd(c, m2, mresult, eo, ca, cb, false);
		}
		return true;
	}

	if(!heur_gcd(m1, m2, mresult, eo, ca, cb, sym_stats, var_i)) {
		if(!sr_gcd(m1, m2, mresult, sym_stats, var_i, eo)) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1, 0);
			return false;
		}
		if(!mresult.isOne()) {
			if(ca) {
				if(!MathStructure::polynomialDivide(m1, mresult, *ca, eo, false)) {
					if(ca) *ca = m1;
					if(cb) *cb = m2;
					mresult.set(1, 1, 0);
					return false;
				}
			}
			if(cb) {
				if(!MathStructure::polynomialDivide(m2, mresult, *cb, eo, false)) {
					if(ca) *ca = m1;
					if(cb) *cb = m2;
					mresult.set(1, 1, 0);
					return false;
				}
			}
		}
		if(CALCULATOR->aborted()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1, 0);
			return false;
		}
	}

	return true;
}


Number dos_count_points(const MathStructure &m, bool b_unknown) {
	if(m.isPower() && (!b_unknown || m[0].containsUnknowns())) {
		Number nr = dos_count_points(m[0], b_unknown);
		if(!nr.isZero()) {
			if(m[0].isNumber() && m[0].number().isNonZero()) {
				nr *= m[1].number();
				if(nr.isNegative()) nr.negate();
			} else {
				nr *= 2;
			}
			return nr;
		}
	} else if(m.size() == 0 && ((b_unknown && m.isUnknown()) || (!b_unknown && !m.isNumber()))) {
		return nr_one;
	}
	Number nr;
	for(size_t i = 0; i < m.size(); i++) {
		nr += dos_count_points(m[i], b_unknown);
	}
	return nr;
}

void replace_fracpow(MathStructure &mstruct, vector<UnknownVariable*> &uv, bool b_top = true) {
	if(mstruct.isFunction()) return;
	if(!b_top && mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && mstruct[0].isRationalPolynomial()) {
		if(!mstruct[1].number().numeratorIsOne()) {
			Number num(mstruct[1].number().numerator());
			mstruct[1].number().divide(num);
			mstruct.raise(num);
			replace_fracpow(mstruct[0], uv, false);
			return;
		}
		for(size_t i = 0; i < uv.size(); i++) {
			if(uv[i]->interval() == mstruct) {
				mstruct.set(uv[i], true);
				return;
			}
		}
		UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mstruct) + RIGHT_PARENTHESIS);
		var->setInterval(mstruct);
		mstruct.set(var, true);
		uv.push_back(var);
		return;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		replace_fracpow(mstruct[i], uv, false);
	}
}
bool restore_fracpow(MathStructure &mstruct, UnknownVariable *uv, const EvaluationOptions &eo, bool b_eval) {
	if(mstruct.isPower() && mstruct[0].isVariable() && mstruct[0].variable() == uv && mstruct[1].isInteger()) {
		mstruct[0].set(uv->interval(), true);
		if(mstruct[0][1].number().numeratorIsOne()) {
			mstruct[0][1].number() *= mstruct[1].number();
			mstruct.setToChild(1, true);
			if(mstruct[1].number().isOne()) mstruct.setToChild(1, true);
			else if(mstruct[0].isNumber()) mstruct.calculateRaiseExponent(eo);
		}
		return true;
	} else if(mstruct.isVariable() && mstruct.variable() == uv) {
		mstruct.set(uv->interval(), true);
		return true;
	}
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		b_ret = restore_fracpow(mstruct[i], uv, eo, b_eval) || b_ret;
	}
	if(b_ret && b_eval) return mstruct.calculatesub(eo, eo, false);
	return false;
}

#define I_RUN_ARG(x) ((depth * 100) + x)

bool do_simplification(MathStructure &mstruct, const EvaluationOptions &eo, bool combine_divisions, bool only_gcd, bool combine_only, bool recursive, bool limit_size, int i_run_pre) {

	if(!eo.expand || !eo.assume_denominators_nonzero || mstruct.size() == 0) return false;

	int i_run = i_run_pre % 100;
	int depth = i_run_pre / 100;
	if(i_run > 50) {
		i_run -= 100;
		depth++;
	}

	if(!combine_only && combine_divisions && i_run == 1) {
		bool b_ret = do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, recursive, limit_size, I_RUN_ARG(-1));
		vector<UnknownVariable*> uv;
		replace_fracpow(mstruct, uv);
		if(uv.size() > 0) {
			MathStructure mbak(mstruct);
			mstruct.evalSort(true);
			bool b = do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, recursive, limit_size, I_RUN_ARG(-1)) && mbak != mstruct;
			EvaluationOptions eo2 = eo;
			eo2.expand = false;
			for(size_t i = 0; i < uv.size(); i++) {
				restore_fracpow(mstruct, uv[i], eo2, b);
				uv[i]->destroy();
			}
			mstruct.evalSort(true);
			if(b) {
				b_ret = true;
				if(mstruct.calculatesub(eo, eo)) do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, recursive, limit_size, I_RUN_ARG(-1));
			}
		}
		return b_ret;
	}

	if(recursive && depth < 10) {
		bool b = false;
		depth++;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(do_simplification(mstruct[i], eo, combine_divisions, only_gcd || (!mstruct.isComparison() && !mstruct.isLogicalAnd() && !mstruct.isLogicalOr()), combine_only, true, limit_size, I_RUN_ARG(i_run))) b = true;
			if(CALCULATOR->aborted()) return b;
		}
		if(b) mstruct.calculatesub(eo, eo, false);
		depth--;
		return do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, false, limit_size, I_RUN_ARG(i_run)) || b;
	}
	if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && mstruct[0].isAddition() && mstruct[0].isRationalPolynomial()) {
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower() && msqrfree.calculateRaise(mstruct[1], eo)) {
			mstruct = msqrfree;
			return true;
		}
	} else if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct) && mstruct[0].isAddition() && mstruct[0].isRationalPolynomial()) {
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower() && msqrfree[1].isInteger() && msqrfree[1].number().isPositive()) {
			if(msqrfree[1] == mstruct[1]) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) return false;
					msqrfree.delChild(2);
					msqrfree.setType(STRUCT_FUNCTION);
					msqrfree.setFunctionId(FUNCTION_ID_ABS);
					mstruct = msqrfree;
				} else {
					mstruct = msqrfree[0];
				}
				return true;
			} else if(msqrfree[1].number().isIntegerDivisible(mstruct[1].number())) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) return false;
					msqrfree[0].transform(STRUCT_FUNCTION);
					msqrfree[0].setFunctionId(FUNCTION_ID_ABS);
				}
				msqrfree[1].number().divide(mstruct[1].number());
				mstruct = msqrfree;
				mstruct.calculatesub(eo, eo, false);
				return true;
			} else if(mstruct[1].number().isIntegerDivisible(msqrfree[1].number())) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) return false;
					msqrfree[0].transform(STRUCT_FUNCTION);
					msqrfree[0].setFunctionId(FUNCTION_ID_ABS);
				}
				Number new_root(mstruct[1].number());
				new_root.divide(msqrfree[1].number());
				mstruct[0] = msqrfree[0];
				mstruct[1] = new_root;
				return true;
			}
		}
	}
	if(!mstruct.isAddition() && !mstruct.isMultiplication()) return false;

	if(combine_divisions) {

		EvaluationOptions eo2 = eo;
		eo2.do_polynomial_division = false;
		eo2.keep_zero_units = false;

		MathStructure divs, nums, numleft, mleft, nrdivs, nrnums;

		if(!mstruct.isAddition()) mstruct.transform(STRUCT_ADDITION);

		// find division by polynomial
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(CALCULATOR->aborted()) return false;
			bool b = false;
			if(mstruct[i].isMultiplication()) {
				MathStructure div, num(1, 1, 0);
				bool b_num = false;
				bool b_rat_m = true;
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(mstruct[i][i2].isPower() && mstruct[i][i2][1].isInteger() && mstruct[i][i2][1].number().isNegative()) {
						if(b_rat_m) {
							bool b_rat = mstruct[i][i2][0].isRationalPolynomial();
							if(!b_rat && mstruct[i][i2][0].isAddition() && mstruct[i][i2][0].size() > 1) {
								b_rat = true;
								for(size_t i3 = 0; i3 < mstruct[i][i2][0].size(); i3++) {
									if(!mstruct[i][i2][0][i3].representsZero(true) && !mstruct[i][i2][0][i3].isRationalPolynomial()) {
										b_rat = false;
										break;
									}
								}
							}
							if(!b_rat) {
								b_rat_m = false;
							}
						}
						bool b_minone = mstruct[i][i2][1].isMinusOne();
						if(b_minone) {
							if(div.isZero()) div = mstruct[i][i2][0];
							else div.multiply(mstruct[i][i2][0], true);
						} else {
							mstruct[i][i2][1].number().negate();
							if(div.isZero()) div = mstruct[i][i2];
							else div.multiply(mstruct[i][i2], true);
							mstruct[i][i2][1].number().negate();
						}
					} else {
						if(b_rat_m && !mstruct[i][i2].isRationalPolynomial() && !mstruct[i][i2].representsZero(true)) b_rat_m = false;
						if(!b_num) {b_num = true; num = mstruct[i][i2];}
						else num.multiply(mstruct[i][i2], true);
					}
				}
				if(!div.isZero()) {
					bool b_found = false;
					for(size_t i3 = 0; i3 < (b_rat_m ? divs.size() : nrdivs.size()); i3++) {
						if((b_rat_m && divs[i3] == div) || (!b_rat_m && nrdivs[i3] == div)) {
							if(!num.representsZero(true)) {
								if(num.isAddition()) {
									for(size_t i4 = 0; i4 < num.size(); i4++) {
										if(b_rat_m) nums[i3].add(num[i4], true);
										else nrnums[i3].add(num[i4], true);
									}
								} else {
									if(b_rat_m) nums[i3].add(num, true);
									else nrnums[i3].add(num, true);
								}
							}
							b_found = true;
							b = true;
							break;
						}
					}
					if(!b_found && (eo.assume_denominators_nonzero || div.representsNonZero(true)) && !div.representsZero(true)) {
						if(!num.representsZero(true)) {
							if(b_rat_m) {
								divs.addChild(div);
								nums.addChild(num);
							} else {
								nrdivs.addChild(div);
								nrnums.addChild(num);
							}
						}
						b = true;
					}
				}
			} else if(mstruct[i].isPower() && mstruct[i][1].isInteger() && mstruct[i][1].number().isNegative()) {
				bool b_rat = mstruct[i][0].isRationalPolynomial();
				if(!b_rat && mstruct[i][0].isAddition() && mstruct[i][0].size() > 1) {
					b_rat = true;
					for(size_t i2 = 0; i2 < mstruct[i][0].size(); i2++) {
						if(!mstruct[i][0][i2].representsZero(true) && !mstruct[i][0].isRationalPolynomial()) {
							b_rat = false;
							break;
						}
					}
				}
				bool b_minone = mstruct[i][1].isMinusOne();
				if(!b_minone) mstruct[i][1].number().negate();
				bool b_found = false;
				for(size_t i3 = 0; i3 < (b_rat ? divs.size() : nrdivs.size()); i3++) {
					if((b_minone && ((b_rat && divs[i3] == mstruct[i][0]) || (!b_rat && nrdivs[i3] == mstruct[i][0]))) || (!b_minone && ((b_rat && divs[i3] == mstruct[i]) || (!b_rat && nrdivs[i3] == mstruct[i])))) {
						if(b_rat) nums[i3].add(m_one, true);
						else nrnums[i3].add(m_one, true);
						b_found = true;
						b = true;
						break;
					}
				}
				if(!b_found && (eo.assume_denominators_nonzero || mstruct[i][0].representsNonZero(true)) && !mstruct[i][0].representsZero(true)) {
					if(b_rat) {
						if(b_minone) divs.addChild(mstruct[i][0]);
						else divs.addChild(mstruct[i]);
						nums.addChild(m_one);
					} else {
						if(b_minone) nrdivs.addChild(mstruct[i][0]);
						else nrdivs.addChild(mstruct[i]);
						nrnums.addChild(m_one);
					}
					b = true;
				}
				if(!b_minone) mstruct[i][1].number().negate();
			}
			if(!b) {
				if(i_run <= 1 && mstruct[i].isRationalPolynomial()) numleft.addChild(mstruct[i]);
				else mleft.addChild(mstruct[i]);
			}
		}

		for(size_t i = 0; i < divs.size(); i++) {
			if(divs[i].isAddition()) {
				for(size_t i2 = 0; i2 < divs[i].size();) {
					if(divs[i][i2].representsZero(true)) {
						divs[i].delChild(i2 + 1);
					} else i2++;
				}
				if(divs[i].size() == 1) divs[i].setToChild(1);
				else if(divs[i].size() == 0) divs[i].clear();
			}
		}

		bool b_ret = false;
		for(size_t i = 0; i < nrdivs.size(); i++) {
			bool b = false;
			if(nrnums[i].isAddition()) {
				MathStructure factor_mstruct(1, 1, 0);
				MathStructure mnew;
				if(factorize_find_multiplier(nrnums[i], mnew, factor_mstruct) && !factor_mstruct.isZero() && !mnew.isZero()) {
					mnew.evalSort(true);
					if(mnew == nrdivs[i]) {
						nrnums[i].set(factor_mstruct);
						b = true;
					}
				}
			}
			if(b) {
				b_ret = true;
			} else {
				if(nrdivs[i].isPower() && nrdivs[i][1].isNumber()) nrdivs[i][1].number().negate();
				else nrdivs[i] ^= nr_minus_one;
				nrnums[i] *= nrdivs[i];
			}
			mleft.addChild(nrnums[i]);
		}

		if(!b_ret && divs.size() == 0) {
			if(mstruct.size() == 1) mstruct.setToChild(1);
			return false;
		}
		if(divs.size() > 1 || numleft.size() > 0) b_ret = true;

		/*divs.setType(STRUCT_VECTOR);
		nums.setType(STRUCT_VECTOR);
		numleft.setType(STRUCT_VECTOR);
		mleft.setType(STRUCT_VECTOR);
		cout << nums << ":" << divs << ":" << numleft << ":" << mleft << endl;*/

		if(i_run > 1) {
			for(size_t i = 0; i < divs.size();) {
				bool b = true;
				if(!divs[i].isRationalPolynomial() || !nums[i].isRationalPolynomial()) {
					if(mstruct.size() == 1) mstruct.setToChild(1);
					return false;
				}
				if(!combine_only && divs[i].isAddition() && nums[i].isAddition()) {
					MathStructure ca, cb, mgcd;
					if(MathStructure::gcd(nums[i], divs[i], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= nums[i].size() + divs[i].size())) {
						if(mgcd.representsNonZero(true) || !eo.warn_about_denominators_assumed_nonzero || warn_about_denominators_assumed_nonzero(mgcd, eo)) {
							if(cb.isOne()) {
								numleft.addChild(ca);
								nums.delChild(i + 1);
								divs.delChild(i + 1);
								b = false;
							} else {
								nums[i] = ca;
								divs[i] = cb;
							}
							b_ret = true;
						}
					}
				}
				if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
				if(b) i++;
			}

		} else {
			while(divs.size() > 0) {
				bool b = true;
				if(!divs[0].isRationalPolynomial() || !nums[0].isRationalPolynomial()) {
					if(mstruct.size() == 1) mstruct.setToChild(1);
					return false;
				}
				if(!combine_only && divs[0].isAddition() && nums[0].isAddition()) {
					MathStructure ca, cb, mgcd;
					if(MathStructure::gcd(nums[0], divs[0], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= nums[0].size() + divs[0].size())) {
						if(mgcd.representsNonZero(true) || !eo.warn_about_denominators_assumed_nonzero || warn_about_denominators_assumed_nonzero(mgcd, eo)) {
							if(cb.isOne()) {
								numleft.addChild(ca);
								nums.delChild(1);
								divs.delChild(1);
								b = false;
							} else {
								nums[0] = ca;
								divs[0] = cb;
							}
							b_ret = true;
						}
					}
				}
				if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
				if(b && divs.size() > 1) {
					if(!combine_only && divs[1].isAddition() && nums[1].isAddition()) {
						MathStructure ca, cb, mgcd;
						EvaluationOptions eo3 = eo2;
						eo3.transform_trigonometric_functions = false;
						if(MathStructure::gcd(nums[1], divs[1], mgcd, eo3, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= nums[1].size() + divs[1].size())) {
							if(mgcd.representsNonZero(true) || !eo.warn_about_denominators_assumed_nonzero || warn_about_denominators_assumed_nonzero(mgcd, eo)) {
								if(cb.isOne()) {
									numleft.addChild(ca);
									nums.delChild(2);
									divs.delChild(2);
									b = false;
								} else {
									nums[1] = ca;
									divs[1] = cb;
								}
							}
						}
					}
					if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
					if(b) {
						MathStructure ca, cb, mgcd;
						b = MathStructure::gcd(divs[0], divs[1], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne();
						if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
						bool b_merge = true;
						if(b) {
							if(limit_size && ((cb.isAddition() && ((divs[0].isAddition() && divs[0].size() * cb.size() > 200) || (nums[0].isAddition() && nums[0].size() * cb.size() > 200))) || (ca.isAddition() && nums[1].isAddition() && nums[1].size() * ca.size() > 200))) {
								b_merge = false;
							} else {
								if(!cb.isOne()) {
									divs[0].calculateMultiply(cb, eo2);
									nums[0].calculateMultiply(cb, eo2);
								}
								if(!ca.isOne()) {
									nums[1].calculateMultiply(ca, eo2);
								}
							}
						} else {
							if(limit_size && ((divs[1].isAddition() && ((divs[0].isAddition() && divs[0].size() * divs[1].size() > 200) || (nums[0].isAddition() && nums[0].size() * divs[1].size() > 200))) || (divs[0].isAddition() && nums[1].isAddition() && nums[1].size() * divs[0].size() > 200))) {
								b_merge = false;
							} else {
								nums[0].calculateMultiply(divs[1], eo2);
								nums[1].calculateMultiply(divs[0], eo2);
								divs[0].calculateMultiply(divs[1], eo2);
							}
						}
						if(b_merge) {
							nums[0].calculateAdd(nums[1], eo2);
							nums.delChild(2);
							divs.delChild(2);
						} else {
							size_t size_1 = 2, size_2 = 2;
							if(nums[0].isAddition()) size_1 += nums[0].size() - 1;
							if(divs[0].isAddition()) size_1 += divs[0].size() - 1;
							if(nums[1].isAddition()) size_2 += nums[1].size() - 1;
							if(divs[1].isAddition()) size_2 += divs[1].size() - 1;
							if(size_1 > size_2) {
								nums[0].calculateDivide(divs[0], eo);
								mleft.addChild(nums[0]);
								nums.delChild(1);
								divs.delChild(1);
							} else {
								nums[1].calculateDivide(divs[1], eo);
								mleft.addChild(nums[1]);
								nums.delChild(2);
								divs.delChild(2);
							}
						}
					}
				} else if(b && numleft.size() > 0) {
					if(limit_size && divs[0].isAddition() && numleft.size() > 0 && divs[0].size() * numleft.size() > 200) break;
					if(numleft.size() == 1) numleft.setToChild(1);
					else if(numleft.size() > 1) numleft.setType(STRUCT_ADDITION);
					numleft.calculateMultiply(divs[0], eo2);
					nums[0].calculateAdd(numleft, eo2);
					numleft.clear();
				} else if(b) break;
			}

			if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
			while(!combine_only && !only_gcd && divs.size() > 0 && divs[0].isAddition() && !nums[0].isNumber()) {
				bool b_unknown = divs[0].containsUnknowns();
				if(!b_unknown || nums[0].containsUnknowns()) {
					MathStructure mcomps;
					mcomps.resizeVector(nums[0].isAddition() ? nums[0].size() : 1, m_zero);
					if(nums[0].isAddition()) {
						if(mcomps.size() < nums[0].size()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
						for(size_t i = 0; i < nums[0].size(); i++) {
							if((b_unknown && nums[0][i].containsUnknowns()) || (!b_unknown && !nums[0][i].isNumber())) {
								mcomps[i].setType(STRUCT_MULTIPLICATION);
								if(nums[0][i].isMultiplication()) {
									for(size_t i2 = 0; i2 < nums[0][i].size(); i2++) {
										if((b_unknown && nums[0][i][i2].containsUnknowns()) || (!b_unknown && !nums[0][i][i2].isNumber())) {
											nums[0][i][i2].ref();
											mcomps[i].addChild_nocopy(&nums[0][i][i2]);
										}
									}
								} else {
									nums[0][i].ref();
									mcomps[i].addChild_nocopy(&nums[0][i]);
								}
							}
						}
					} else {
						mcomps[0].setType(STRUCT_MULTIPLICATION);
						if(nums[0].isMultiplication()) {
							for(size_t i2 = 0; i2 < nums[0].size(); i2++) {
								if((b_unknown && nums[0][i2].containsUnknowns()) || (!b_unknown && !nums[0][i2].isNumber())) {
									nums[0][i2].ref();
									mcomps[0].addChild_nocopy(&nums[0][i2]);
								}
							}
						} else {
							nums[0].ref();
							mcomps[0].addChild_nocopy(&nums[0]);
						}
					}
					bool b_found = false;
					unordered_map<size_t, size_t> matches;
					for(size_t i = 0; i < divs[0].size(); i++) {
						if((b_unknown && divs[0][i].containsUnknowns()) || (!b_unknown && !divs[0][i].isNumber())) {
							MathStructure mcomp1;
							mcomp1.setType(STRUCT_MULTIPLICATION);
							if(divs[0][i].isMultiplication()) {
								for(size_t i2 = 0; i2 < divs[0][i].size(); i2++) {
									if((b_unknown && divs[0][i][i2].containsUnknowns()) || (!b_unknown && !divs[0][i][i2].isNumber())) {
										divs[0][i][i2].ref();
										mcomp1.addChild_nocopy(&divs[0][i][i2]);
									}
								}
							} else {
								divs[0][i].ref();
								mcomp1.addChild_nocopy(&divs[0][i]);
							}
							b_found = false;
							for(size_t i2 = 0; i2 < mcomps.size(); i2++) {
								if(mcomps[i2] == mcomp1) {
									matches[i] = i2;
									b_found = true;
									break;
								}
							}
							if(!b_found) break;
						}
					}
					if(b_found) {
						b_found = false;
						size_t i_selected = 0;
						if(nums[0].isAddition()) {
							Number i_points;
							for(size_t i = 0; i < divs[0].size(); i++) {
								if((b_unknown && divs[0][i].containsUnknowns()) || (!b_unknown && !divs[0][i].isNumber())) {
									Number new_points = dos_count_points(divs[0][i], b_unknown);
									if(new_points > i_points) {i_points = new_points; i_selected = i;}
								}
							}
						}
						MathStructure mquo, mrem;
						if(polynomial_long_division(nums[0].isAddition() ? nums[0][matches[i_selected]] : nums[0], divs[0], m_zero, mquo, mrem, eo2, false) && !mquo.isZero() && mrem != (nums[0].isAddition() ? nums[0][matches[i_selected]] : nums[0])) {
							if(CALCULATOR->aborted()) return false;
							if((nums[0].isAddition() && nums[0].size() > 1) || !mrem.isZero() || divs[0].representsNonZero(true) || (eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[0], eo))) {
								mleft.addChild(mquo);
								if(nums[0].isAddition()) {
									nums[0].delChild(matches[i_selected] + 1, true);
									if(!mrem.isZero()) {
										nums[0].calculateAdd(mrem, eo2);
									}
								} else {
									if(mrem.isZero()) {
										divs.clear();
									} else {
										nums[0] = mrem;
									}
								}
								b_ret = true;
								b_found = true;
							}
						}
					}
					if(!b_found) break;
				} else {
					break;
				}
			}

			if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
			if(!combine_only && !only_gcd && divs.size() > 0 && divs[0].isAddition()) {
				bool b_unknown = divs[0].containsUnknowns() || nums[0].containsUnknowns();
				MathStructure mquo, mrem;
				vector<MathStructure> symsd, symsn;
				collect_symbols(nums[0], symsn);
				if(!symsn.empty()) {
					collect_symbols(divs[0], symsd);
				}
				for(size_t i = 0; i < symsd.size();) {
					bool b_found = false;
					if(!b_unknown || symsd[i].containsUnknowns()) {
						for(size_t i2 = 0; i2 < symsn.size(); i2++) {
							if(symsn[i2] == symsd[i]) {
								b_found = (nums[0].degree(symsd[i]) >= divs[0].degree(symsd[i]));
								break;
							}
						}
					}
					if(b_found) i++;
					else symsd.erase(symsd.begin() + i);
				}
				for(size_t i = 0; i < symsd.size(); i++) {
					if(polynomial_long_division(nums[0], divs[0], symsd[i], mquo, mrem, eo2, false) && !mquo.isZero() && mrem != nums[0]) {
						if(CALCULATOR->aborted()) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}
						if(!mrem.isZero() || divs[0].representsNonZero(true) || (eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[0], eo))) {
							if(mrem.isZero()) {
								mleft.addChild(mquo);
								divs.clear();
								b_ret = true;
								break;
							} else {
								if(mquo.isAddition()) {
									for(size_t i = 0; i < mquo.size(); ) {
										MathStructure mtest(mquo[i]);
										mtest.calculateMultiply(divs[0], eo2);
										mtest.calculateAdd(mrem, eo2);
										Number point = dos_count_points(mtest, b_unknown);
										point -= dos_count_points(mquo[i], b_unknown);
										point -= dos_count_points(mrem, b_unknown);
										if(point <= 0) {
											mquo.delChild(i + 1);
											mrem = mtest;
										} else {
											i++;
										}
									}
									if(mquo.size() == 0) mquo.clear();
									else if(mquo.size() == 1) mquo.setToChild(1);
								}
								if(!mquo.isZero()) {
									Number point = dos_count_points(nums[0], b_unknown);
									point -= dos_count_points(mquo, b_unknown);
									point -= dos_count_points(mrem, b_unknown);
									if(point > 0) {
										mleft.addChild(mquo);
										MathStructure ca, cb, mgcd;
										if(mrem.isAddition() && MathStructure::gcd(mrem, divs[0], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne() && (!cb.isAddition() || !ca.isAddition() || ca.size() + cb.size() <= mrem.size() + divs[0].size())) {
											mrem = ca;
											divs[0] = cb;
										}
										mrem.calculateDivide(divs[0], eo2);
										mleft.addChild(mrem);
										divs.clear();
										b_ret = true;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		if(!b_ret) {if(mstruct.size() == 1) mstruct.setToChild(1); return false;}

		mstruct.clear(true);
		for(size_t i = 0; i < divs.size() && (i == 0 || i_run > 1); i++) {
			if(i == 0) mstruct = nums[0];
			else mstruct.add(nums[i], i > 1);
			if(only_gcd || combine_only) {
				divs[i].inverse();
				if(i == 0) mstruct.multiply(divs[i], true);
				else mstruct.last().multiply(divs[i], true);
			} else {
				if(i == 0) mstruct.calculateDivide(divs[i], eo);
				else mstruct.last().calculateDivide(divs[i], eo);
				if(i == 0 && i_run <= 1 && mstruct.isAddition()) {
					do_simplification(mstruct, eo, true, false, false, false, limit_size, I_RUN_ARG(2));
				}
			}
		}

		for(size_t i = 0; i < mleft.size(); i++) {
			if(i == 0 && mstruct.isZero()) mstruct = mleft[i];
			else mstruct.calculateAdd(mleft[i], eo);
		}
		for(size_t i = 0; i < numleft.size(); i++) {
			if(i == 0 && mstruct.isZero()) mstruct = numleft[i];
			else mstruct.calculateAdd(numleft[i], eo);
		}

		return true;
	}

	MathStructure divs;
	// find division by polynomial
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && (mstruct[i][i2][0].isAddition() || combine_divisions) && mstruct[i][i2][1].isMinusOne() && mstruct[i][i2][0].isRationalPolynomial()) {
					bool b_found = false;
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == mstruct[i][i2][0]) {
							divs[i3].number()++;
							b_found = true;
							break;
						}
					}
					if(!b_found) divs.addChild(mstruct[i][i2][0]);
					break;
				}
			}
		} else if(mstruct[i].isPower() && (mstruct[i][0].isAddition() || combine_divisions) && mstruct[i][1].isMinusOne() && mstruct[i][0].isRationalPolynomial()) {
			bool b_found = false;
			for(size_t i3 = 0; i3 < divs.size(); i3++) {
				if(divs[i3] == mstruct[i][0]) {
					divs[i3].number()++;
					b_found = true;
					break;
				}
			}
			if(!b_found) divs.addChild(mstruct[i][0]);
		}
	}

	// check if denominators is zero
	for(size_t i = 0; i < divs.size(); ) {
		if((!combine_divisions && divs[i].number().isZero()) || (!eo.assume_denominators_nonzero && !divs[i].representsNonZero(true)) || divs[i].representsZero(true)) divs.delChild(i + 1);
		else i++;
	}

	if(divs.size() == 0) return false;

	// combine numerators with same denominator
	MathStructure nums, numleft, mleft;
	nums.resizeVector(divs.size(), m_zero);
	if(nums.size() < divs.size()) return false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		bool b = false;
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && (mstruct[i][i2][0].isAddition() || combine_divisions) && mstruct[i][i2][1].isMinusOne() && mstruct[i][i2][0].isRationalPolynomial()) {
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == mstruct[i][i2][0]) {
							if(mstruct[i].size() == 1) nums[i3].addChild(m_one);
							else if(mstruct[i].size() == 2) nums[i3].addChild(mstruct[i][i2 == 0 ? 1 : 0]);
							else {nums[i3].addChild(mstruct[i]); nums[i3][nums[i3].size() - 1].delChild(i2 + 1);}
							b = true;
							break;
						}
					}
					break;
				}
			}
		} else if(mstruct[i].isPower() && (mstruct[i][0].isAddition() || combine_divisions) && mstruct[i][1].isMinusOne() && mstruct[i][0].isRationalPolynomial()) {
			for(size_t i3 = 0; i3 < divs.size(); i3++) {
				if(divs[i3] == mstruct[i][0]) {
					nums[i3].addChild(m_one);
					b = true;
					break;
				}
			}
		}
		if(!b && combine_divisions) {
			if(mstruct[i].isRationalPolynomial()) numleft.addChild(mstruct[i]);
			else mleft.addChild(mstruct[i]);
		}
	}

	EvaluationOptions eo2 = eo;
	eo2.do_polynomial_division = false;
	eo2.keep_zero_units = false;

	// do polynomial division; give points to incomplete division
	vector<long int> points(nums.size(), -1);
	bool b = false, b_ready_candidate = false;
	for(size_t i = 0; i < divs.size(); i++) {
		if(CALCULATOR->aborted()) return false;
		if(nums[i].size() > 1 && divs[i].isAddition()) {
			nums[i].setType(STRUCT_ADDITION);
			MathStructure xvar;
			get_first_symbol(nums[i], xvar);
			if(nums[i].isRationalPolynomial() && nums[i].degree(xvar).isLessThan(100) && divs[i].degree(xvar).isLessThan(100)) {
				MathStructure mquo, mrem, ca, cb;
				if(MathStructure::gcd(nums[i], divs[i], mquo, eo2, &ca, &cb, false) && !mquo.isOne()) {
					if(!mquo.representsNonZero(true) && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(mquo, eo)) {
						nums[i].clear();
					} else {
						points[i] = 1;
						b_ready_candidate = true;
						b = true;
					}
					if(ca.isOne()) {
						if(cb.isOne()) {
							nums[i].set(1, 1, 0, true);
						} else {
							nums[i] = cb;
							nums[i].inverse();
						}
					} else if(cb.isOne()) {
						nums[i] = ca;
					} else {
						nums[i] = ca;
						nums[i].calculateDivide(cb, eo);
					}
				} else if(!only_gcd && polynomial_long_division(nums[i], divs[i], xvar, mquo, mrem, eo2, false) && !mquo.isZero() && mrem != nums[i]) {
					if(mrem.isZero() && !divs[i].representsNonZero(true) && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[i], eo)) {
						nums[i].clear();
					} else {
						long int point = 1;
						if(!mrem.isZero()) point = nums[i].size();
						nums[i].set(mquo);
						if(!mrem.isZero()) {
							if(mquo.isAddition()) point -= mquo.size();
							else point--;
							if(mrem.isAddition()) point -= mrem.size();
							else point--;
							mrem.calculateDivide(divs[i], eo2);
							nums[i].calculateAdd(mrem, eo2);
						}
						b = true;
						points[i] = point;
						if(point >= 0) {b_ready_candidate = true;}
						else if(b_ready_candidate) nums[i].clear();
					}
				} else {
					nums[i].clear();
				}
			}
		} else {
			nums[i].clear();
		}
	}

	if(!b) return false;

	if(b_ready_candidate) {
		// remove polynomial divisions that increase complexity
		for(size_t i = 0; i < nums.size(); i++) {
			if(!nums[i].isZero() && points[i] < 0) nums[i].clear();
		}
	} else {
		// no simplifying polynomial division found; see if result can be combined with other terms
		b = false;
		for(size_t i = 0; i < nums.size(); i++) {
			if(!nums[i].isZero()) {
				if(b) {
					nums[i].clear();
				} else {
					long int point = points[i];
					for(size_t i2 = 0; i2 < nums[i].size(); i2++) {
						bool b2 = false;
						if(!nums[i][i2].contains(divs[i], false, false, false)) {
							MathStructure mtest1(mstruct), mtest2(nums[i][i2]);
							for(size_t i3 = 0; i3 < mtest1.size(); i3++) {
								if(!mtest1[i3].contains(divs[i], false, false, false)) {
									int ret = mtest1[i3].merge_addition(mtest2, eo);
									if(ret > 0) {
										b2 = true;
										point++;
										if(mtest1[i3].isZero()) point++;
										break;
									}
									if(ret == 0) ret = mtest2.merge_addition(mtest1[i3], eo);
									if(ret > 0) {
										b2 = true;
										point++;
										if(mtest2.isZero()) point++;
										break;
									}
								}
							}
							if(b2) break;
						}
						if(point >= 0) break;
					}
					if(point >= 0) b = true;
					else nums[i].clear();
				}
			}
		}
	}

	if(!b) return false;
	// replace terms with polynomial division result
	for(size_t i = 0; i < mstruct.size(); ) {
		bool b_del = false;
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && mstruct[i][i2][0].isAddition() && mstruct[i][i2][1].isMinusOne() && mstruct[i][i2][0].isRationalPolynomial()) {
					for(size_t i3 = 0; i3 < divs.size(); i3++) {
						if(divs[i3] == mstruct[i][i2][0]) {
							b_del = !nums[i3].isZero();
							break;
						}
					}
					break;
				}
			}
		} else if(mstruct[i].isPower() && mstruct[i][0].isAddition() && mstruct[i][1].isMinusOne() && mstruct[i][0].isRationalPolynomial()) {
			for(size_t i3 = 0; i3 < divs.size(); i3++) {
				if(divs[i3] == mstruct[i][0]) {
					b_del = !nums[i3].isZero();
					break;
				}
			}
		}
		if(b_del) mstruct.delChild(i + 1);
		else i++;
	}
	for(size_t i = 0; i < nums.size(); ) {
		if(nums[i].isZero()) {
			nums.delChild(i + 1);
		} else {
			nums[i].evalSort();
			i++;
		}
	}
	if(mstruct.size() == 0 && nums.size() == 1) {
		mstruct.set(nums[0]);
	} else {
		for(size_t i = 0; i < nums.size(); i++) {
			mstruct.addChild(nums[i]);
		}
		mstruct.evalSort();
	}
	return true;
}

