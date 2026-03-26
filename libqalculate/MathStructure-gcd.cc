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
			if(CALCULATOR->aborted() || mprim.isZero()) {
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
			if(CALCULATOR->aborted() || mprim.isZero()) {
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

void find_lcm_powden(const MathStructure &mstruct, const MathStructure &mbase, Number &nr) {
	if(mstruct.isFunction()) return;
	if(mstruct.isPower() && mstruct[0] == mbase && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && mstruct[1].number().denominator() != nr) {
		if(mstruct[1].number().denominatorIsLessThan(5) || mstruct[1].number().numeratorIsLessThan(10)) nr.lcm(mstruct[1].number().denominator());
		else nr.clear();
		return;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		find_lcm_powden(mstruct[i], mbase, nr);
		if(nr.isZero()) return;
	}
}
void replace_lcm_powden(MathStructure &mstruct, UnknownVariable *var, bool replace_number = false) {
	if(mstruct.isFunction()) return;
	if(mstruct.isPower() && mstruct[0] == var->interval()[0] && mstruct[1].isNumber() && mstruct[1].number().isRational()) {
		if(mstruct[1] == var->interval()[1]) {
			mstruct.set(var, true);
		} else {
			mstruct[0].set(var, true);
			mstruct[1].number().divide(var->interval()[1].number());
		}
		return;
	} else if(!mstruct.isNumber() && mstruct == var->interval()[0]) {
		mstruct.set(var, true);
		mstruct.raise(m_one);
		mstruct[1].number().divide(var->interval()[1].number());
		return;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		replace_lcm_powden(mstruct[i], var);
		if(replace_number && mstruct.isAddition() && mstruct[i].isNumber() && mstruct[i].number().isRational() && var->interval()[0].isNumber() && mstruct[i].number().compareAbsolute(1000) == COMPARISON_RESULT_GREATER && var->interval()[0].number().compareAbsolute(1000) == COMPARISON_RESULT_GREATER) {
			if(mstruct[i] == var->interval()[0]) {
				mstruct[i].set(var, true);
				mstruct[i].raise(m_one);
				mstruct[i][1].number().divide(var->interval()[1].number());
			} else {
				if(mstruct[i].number().subtract(var->interval()[0].number())) {
					mstruct.add(var, true);
					mstruct.last().raise(m_one);
					mstruct.last()[1].number().divide(var->interval()[1].number());
				}
			}
		}
	}
}
void replace_fracpow(MathStructure &mstruct, vector<UnknownVariable*> &uv, bool replace_number, MathStructure *top_mstruct) {
	if(mstruct.isFunction()) return;
	if(top_mstruct && mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && (!mstruct[0].isNumber() || mstruct[0].number().isPositive()) && mstruct[0].isRationalPolynomial() && mstruct[1].number().denominatorIsLessThan(5) && mstruct[1].number().numeratorIsLessThan(10)) {
		Number nr(mstruct[1].number().denominator());
		find_lcm_powden(*top_mstruct, mstruct[0], nr);
		if(!nr.isZero()) {
			MathStructure mvar(mstruct[0]);
			nr.recip();
			mvar.raise(nr);
			UnknownVariable *var = new UnknownVariable("replace_fracpow", string(LEFT_PARENTHESIS) + format_and_print(mvar) + RIGHT_PARENTHESIS);
			var->setInterval(mvar);
			uv.push_back(var);
			replace_lcm_powden(*top_mstruct, var, replace_number && uv.size() == 1 && !top_mstruct->containsUnknowns());
			return;
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		replace_fracpow(mstruct[i], uv, replace_number, top_mstruct ? top_mstruct : &mstruct);
	}
}
bool restore_fracpow(MathStructure &mstruct, UnknownVariable *uv, const EvaluationOptions &eo, bool b_eval) {
	bool b_ret = false;
	if(mstruct.isPower() && mstruct[0].isVariable() && mstruct[0].variable() == uv && mstruct[1].isNumber()) {
		mstruct[0].set(uv->interval(), true);
		mstruct[0][1].number() *= mstruct[1].number();
		mstruct.setToChild(1, true);
		if(mstruct[1].number().isOne()) mstruct.setToChild(1, true);
		else if(mstruct[0].isNumber() && mstruct[1].number().isInteger()) mstruct.calculateRaiseExponent(eo);
		return true;
	} else if(mstruct.isVariable() && mstruct.variable() == uv) {
		mstruct.set(uv->interval(), true);
		return true;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(restore_fracpow(mstruct[i], uv, eo, b_eval)) {
			b_eval = true;
			b_ret = true;
		}
	}
	if(b_ret && b_eval) return mstruct.calculatesub(eo, eo, false);
	return false;
}
bool contains_replace_fracpow_var(const MathStructure &mstruct) {
	if(mstruct.isVariable() && !mstruct.variable()->isKnown() && ((UnknownVariable*) mstruct.variable())->interval().isPower() && ((UnknownVariable*) mstruct.variable())->title() == "replace_fracpow") return true;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_replace_fracpow_var(mstruct[i])) return true;
	}
	return false;
}
int test_replace_fracpow(const MathStructure &mstruct, bool two = false, bool top = true) {
	if(top && contains_replace_fracpow_var(mstruct)) return false;
	if(!top && mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && (two || !mstruct[0].isNumber() || mstruct[0].number().isPositive()) && mstruct[0].isRationalPolynomial() && (two || mstruct[1].number().denominatorIsLessThan(5))) {
		return true;
	}
	if(top && !two && mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger()) return test_replace_fracpow(mstruct[0], false);
	if(mstruct.isPower() && mstruct[1].isMinusOne()) return test_replace_fracpow(mstruct[0], false);
	if(!mstruct.isAddition() && !mstruct.isMultiplication()) return false;
	int b = -1;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isRational() && !mstruct[i][1].number().isInteger() && (two || !mstruct[i][0].isNumber() || mstruct[i][0].number().isPositive()) && mstruct[i][0].isRationalPolynomial() && (two || mstruct[i][1].number().denominatorIsLessThan(5))) {
			if(two) return true;
			b = true;
		} else if(!mstruct[i].isMultiplication() && !mstruct[i].isAddition() && !mstruct[i].isPower()) {
			if(!two && !mstruct[i].isRationalPolynomial()) {
				return false;
			}
		} else {
			int b_i = test_replace_fracpow(mstruct[i], two, false);
			if(b_i == 0) {
				return false;
			} else if(b_i > 0) {
				if(two) return true;
				b = b_i;
			}
		}
	}
	return b;
}
bool test_replace_fracpow_factor(const MathStructure &mstruct) {
	if(!mstruct.isAddition() || contains_replace_fracpow_var(mstruct)) return false;
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct.isPower() && i > 0) break;
		if(mstruct[i].isMultiplication()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				if(mstruct[i][i2].isPower() && mstruct[i][i2][1].isNumber() && mstruct[i][i2][1].number().isRational() && !mstruct[i][i2][1].number().isInteger() && (!mstruct[i][i2][0].isNumber() || mstruct[i][i2][0].number().isPositive()) && mstruct[i][i2][0].isRationalPolynomial() && mstruct[i][i2][1].number().denominatorIsLessThan(5)) {
					b = true;
				} else if(!mstruct[i][i2].isRationalPolynomial()) {
					return false;
				}
			}
		} else if(mstruct[i].isPower() && mstruct[i][1].isNumber() && mstruct[i][1].number().isRational() && !mstruct[i][1].number().isInteger() && (!mstruct[i][0].isNumber() || mstruct[i][0].number().isPositive()) && mstruct[i][0].isRationalPolynomial() && mstruct[i][1].number().denominatorIsLessThan(5)) {
			b = true;
		} else if(!mstruct[i].isRationalPolynomial()) {
			return false;
		}
	}
	return b;
}

#define I_RUN_ARG(x) ((depth * 100) + x)

bool do_function_simplification(MathStructure &mstruct, const EvaluationOptions &eo, bool recursive, int depth) {
	if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().isRational() && !mstruct[1].number().isInteger() && mstruct[0].isAddition() && (mstruct[0].isRationalPolynomial())) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower() && msqrfree.calculateRaise(mstruct[1], eo)) {
			mstruct = msqrfree;
			CALCULATOR->endTemporaryStopMessages(true);
			return true;
		}
		bool bfrac = false, bint = true;
		idm1(mstruct[0], bfrac, bint);
		if(bfrac || bint) {
			Number gcd(1, 1);
			idm2(mstruct[0], bfrac, bint, gcd);
			if((bint || bfrac) && !gcd.isOne()) {
				if(bfrac) gcd.recip();
				msqrfree = mstruct[0];
				msqrfree.calculateDivide(gcd, eo);
				if(sqrfree(msqrfree, eo) && msqrfree.isPower()) {
					msqrfree *= gcd;
					if(msqrfree.calculateRaise(mstruct[1], eo) && (msqrfree.isAddition() || (msqrfree.isMultiplication() && !msqrfree.last().isPower()))) {
						mstruct = msqrfree;
						CALCULATOR->endTemporaryStopMessages(true);
						return true;
					}
				}
			}
		}
		CALCULATOR->endTemporaryStopMessages();
	} else if(mstruct.isPower() && mstruct[1].isNumber() && mstruct[1].number().denominatorIsTwo()) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure msqrfree(mstruct[0]);
		bool bfrac = false, bint = true;
		idm1(msqrfree, bfrac, bint);
		if(bfrac || bint) {
			Number gcd(1, 1);
			idm2(msqrfree, bfrac, bint, gcd);
			if((bint || bfrac) && !gcd.isOne()) {
				if(bfrac) gcd.recip();
				msqrfree.calculateDivide(gcd, eo);
				if(sqrt_to_square(msqrfree, eo)) {
					msqrfree *= gcd;
					if(msqrfree.calculateRaise(mstruct[1], eo) && (msqrfree.isAddition() || (msqrfree.isMultiplication() && !msqrfree.last().isPower()))) {
						mstruct = msqrfree;
						CALCULATOR->endTemporaryStopMessages(true);
						return true;
					}
				}
			}
		} else if(sqrt_to_square(msqrfree, eo) && msqrfree.calculateRaise(mstruct[1], eo)) {
			mstruct = msqrfree;
			CALCULATOR->endTemporaryStopMessages(true);
			return true;
		}
		CALCULATOR->endTemporaryStopMessages();
	} else if(mstruct.function() && mstruct.function()->id() == FUNCTION_ID_LOG && mstruct.size() == 1 && mstruct[0].representsPositive() && mstruct[0].isRationalPolynomial()) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower()) {
			msqrfree.transformById(FUNCTION_ID_LOG);
			EvaluationOptions eo2 = eo;
			eo2.expand = false;
			if(msqrfree.calculateFunctions(eo2) && msqrfree.isMultiplication()) {
				mstruct = msqrfree;
				CALCULATOR->endTemporaryStopMessages(true);
				return true;
			}
		}
		bool bfrac = false, bint = true;
		idm1(mstruct[0], bfrac, bint);
		if(bfrac || bint) {
			Number gcd(1, 1);
			idm2(mstruct[0], bfrac, bint, gcd);
			if((bint || bfrac) && !gcd.isOne()) {
				if(bfrac) gcd.recip();
				msqrfree = mstruct[0];
				msqrfree.calculateDivide(gcd, eo);
				if(sqrfree(msqrfree, eo) && msqrfree.isPower()) {
					msqrfree *= gcd;
					msqrfree.transformById(FUNCTION_ID_LOG);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					if(msqrfree.calculateFunctions(eo2) && msqrfree.isAddition() && msqrfree.last().isMultiplication()) {
						mstruct = msqrfree;
						CALCULATOR->endTemporaryStopMessages(true);
						return true;
					}
				}
			}
		}
		CALCULATOR->endTemporaryStopMessages();
	} else if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(mstruct) && mstruct[0].isAddition() && mstruct[0].isRationalPolynomial()) {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure msqrfree(mstruct[0]);
		if(sqrfree(msqrfree, eo) && msqrfree.isPower() && msqrfree[1].isInteger() && msqrfree[1].number().isPositive()) {
			if(msqrfree[1] == mstruct[1]) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) {CALCULATOR->endTemporaryStopMessages(); return false;}
					msqrfree.delChild(2);
					msqrfree.setType(STRUCT_FUNCTION);
					msqrfree.setFunctionId(FUNCTION_ID_ABS);
					mstruct = msqrfree;
				} else {
					mstruct = msqrfree[0];
				}
				CALCULATOR->endTemporaryStopMessages(true);
				return true;
			} else if(msqrfree[1].number().isIntegerDivisible(mstruct[1].number())) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) {CALCULATOR->endTemporaryStopMessages(); return false;}
					msqrfree[0].transform(STRUCT_FUNCTION);
					msqrfree[0].setFunctionId(FUNCTION_ID_ABS);
				}
				msqrfree[1].number().divide(mstruct[1].number());
				mstruct = msqrfree;
				mstruct.calculatesub(eo, eo, false);
				CALCULATOR->endTemporaryStopMessages(true);
				return true;
			} else if(mstruct[1].number().isIntegerDivisible(msqrfree[1].number())) {
				if(msqrfree[1].number().isEven()) {
					if(!msqrfree[0].representsReal(true)) {CALCULATOR->endTemporaryStopMessages(); return false;}
					msqrfree[0].transform(STRUCT_FUNCTION);
					msqrfree[0].setFunctionId(FUNCTION_ID_ABS);
				}
				Number new_root(mstruct[1].number());
				new_root.divide(msqrfree[1].number());
				mstruct[0] = msqrfree[0];
				mstruct[1] = new_root;
				CALCULATOR->endTemporaryStopMessages(true);
				return true;
			}
		}
		CALCULATOR->endTemporaryStopMessages();
	}
	if(recursive && depth < 10) {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(do_function_simplification(mstruct[i], eo, true, depth + 1)) b = true;
		}
		if(b) return true;
	}
	return false;
}

int simplify_highest_power(const MathStructure &m) {
	int p = 0;
	if(m.isPower() && m[1].isNumber() && m[1].number().isRational()) {
		if(m[1].number().isInteger()) p = m[1].number().intValue();
		else p = m[1].number().numerator().intValue();
		if(p < 0) {
			p = -p;
			int p0 = simplify_highest_power(m[0]);
			if(p0 > p) p = p0;
		}
		return p;
	}
	if(!m.isAddition() && !m.isMultiplication()) return 0;
	for(size_t i = 0; i < m.size(); i++) {
		int p_i = simplify_highest_power(m[i]);
		if(p_i > p) p = p_i;
	}
	return p;
}
int simplify_cumbersome_number(const MathStructure &m) {
	if(m.isNumber()) {
		if(m.number().hasImaginaryPart()) return 4;
		if(!m.number().isRational()) return 3;
		if(m.number().denominatorIsGreaterThan(20)) {return 1;}
		return 0;
	}
	if(m.isPower() && m[1].isNumber() && !m[1].number().isInteger()) return 2;
	int v = 0;
	for(size_t i = 0; i < m.size(); i++) {
		int v_i = simplify_cumbersome_number(m[i]);
		if(v_i > v) v = v_i;
		if(v >= 4) return v;
	}
	return v;
}

bool contains_abs_sym(const MathStructure &m, const MathStructure &sym) {
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_ABS && (m[0].size() == 0 || m[0].isMultiplication() || m[0].isAddition() || m[0].isPower()) && m[0].contains(sym, false)) return true;
	if(m.isMultiplication() || m.isAddition() || m.isPower()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(contains_abs_sym(m[i], sym)) return true;
		}
	}
	return false;
}

bool do_simplification(MathStructure &mstruct, const EvaluationOptions &eo, bool combine_divisions, bool only_gcd, bool combine_only, bool recursive, bool limit_size, int i_run_pre) {

	if(!eo.expand || !eo.assume_denominators_nonzero || mstruct.size() == 0) return false;

	int i_run = i_run_pre % 100;
	int depth = i_run_pre / 100;
	if(i_run > 50) {
		i_run -= 100;
		depth++;
	}

	if(!combine_only && i_run == 1 && depth == 0) {
		if(simplify_roots(mstruct, eo, true)) {
			MathStructure mbak(mstruct);
			if(simplify_roots(mstruct, eo, false)) {
				depth++;
				if(do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, recursive, limit_size, I_RUN_ARG(1))) {
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					mstruct.calculatesub(eo2, eo);
					return true;
				} else {
					mstruct = mbak;
					return false;
				}
			}
		}
	}

	if(!combine_only && combine_divisions && i_run == 1) {
		bool b_ret = do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, recursive, limit_size, I_RUN_ARG(-1));
		if(test_replace_fracpow(mstruct) > 0) {
			if(CALCULATOR->aborted()) return b_ret;
			vector<UnknownVariable*> uv;
			MathStructure mbak(mstruct);
			replace_fracpow(mstruct, uv, false);
			if(uv.size() > 0) {
				mstruct.evalSort(true);
				if(do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, recursive, limit_size, I_RUN_ARG(-1))) {
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					for(size_t i = uv.size(); i > 0; i--) {
						restore_fracpow(mstruct, uv[i - 1], eo2, true);
						uv[i - 1]->destroy();
					}
					mstruct.evalSort(true);
					if(mstruct == mbak) {
					} else if(simplify_highest_power(mstruct) > simplify_highest_power(mbak)) {
						mstruct = mbak;
					} else {
						b_ret = true;
					}
				} else {
					mstruct = mbak;
				}
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
			if(test_replace_fracpow(mstruct[i])) {
				vector<UnknownVariable*> uv;
				MathStructure mbak(mstruct[i]);
				replace_fracpow(mstruct[i], uv, false);
				if(uv.size() > 0) {
					mstruct[i].evalSort(true);
					if(do_simplification(mstruct[i], eo, combine_divisions, only_gcd || (!mstruct.isComparison() && !mstruct.isLogicalAnd() && !mstruct.isLogicalOr()), combine_only, false, limit_size, I_RUN_ARG(i_run))) {
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						for(size_t i2 = uv.size(); i2 > 0; i2--) {
							restore_fracpow(mstruct[i], uv[i2 - 1], eo2, true);
							uv[i2 - 1]->destroy();
						}
						mstruct[i].evalSort(true);
						if(mstruct[i] == mbak) {
						} else if(simplify_highest_power(mstruct[i]) > simplify_highest_power(mbak)) {
							mstruct[i] = mbak;
						} else {
							b = true;
						}
					} else {
						mstruct[i] = mbak;
					}
				}
			}
			if(CALCULATOR->aborted()) return b;
		}
		if(b) {
			mstruct.evalSort(true);
			mstruct.calculatesub(eo, eo, false);
		}
		depth--;
		return do_simplification(mstruct, eo, combine_divisions, only_gcd, combine_only, false, limit_size, I_RUN_ARG(i_run)) || b;
	}
	if(do_function_simplification(mstruct, eo, false)) return true;
	if(!mstruct.isAddition() && !mstruct.isMultiplication()) return false;

	EvaluationOptions eo2 = eo;
	eo2.do_polynomial_division = false;
	eo2.keep_zero_units = false;

	MathStructure divs, nums, numleft, mleft, nrdivs, nrnums;

	if(!mstruct.isAddition()) mstruct.transform(STRUCT_ADDITION);

	CALCULATOR->beginTemporaryStopMessages();

#define DIVISION_BY_POLYNOMIAL_ABORT {if(mstruct.size() == 1) {mstruct.setToChild(1);} CALCULATOR->endTemporaryStopMessages(); return false;}

	// find division by polynomial
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
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

	if(!b_ret && divs.size() == 0) DIVISION_BY_POLYNOMIAL_ABORT

	if(divs.size() > 1 || numleft.size() > 0) b_ret = true;

	/*divs.setType(STRUCT_VECTOR);
	nums.setType(STRUCT_VECTOR);
	numleft.setType(STRUCT_VECTOR);
	mleft.setType(STRUCT_VECTOR);
	cout << nums << ":" << divs << ":" << numleft << ":" << mleft << endl;*/

	if(i_run > 1 || !combine_divisions) {
		for(size_t i = 0; i < divs.size();) {
			bool b = true;

			if(!divs[i].isRationalPolynomial() || (!nums[i].isRationalPolynomial() && !nums[i].isZero())) DIVISION_BY_POLYNOMIAL_ABORT
			if(nums[i].isZero() && !divs[i].representsNonZero(true) && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[i], eo)) DIVISION_BY_POLYNOMIAL_ABORT

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
			if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
			if(b) i++;
		}

	} else {
		while(divs.size() > 0) {
			bool b = true;

			if(!divs[0].isRationalPolynomial() || (!nums[0].isRationalPolynomial() && !nums[0].isZero())) DIVISION_BY_POLYNOMIAL_ABORT
			if(nums[0].isZero() && !divs[0].representsNonZero(true) && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(divs[0], eo)) DIVISION_BY_POLYNOMIAL_ABORT

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
			if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
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
				if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
				if(b) {
					MathStructure ca, cb, mgcd;
					b = MathStructure::gcd(divs[0], divs[1], mgcd, eo2, &ca, &cb, false) && !mgcd.isOne();
					if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
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

		if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
		while(!combine_only && !only_gcd && divs.size() > 0 && divs[0].isAddition() && !nums[0].isNumber()) {
			if(CALCULATOR->aborted()) break;
			bool b_unknown = divs[0].containsUnknowns();
			if(!b_unknown || nums[0].containsUnknowns()) {
				MathStructure mcomps;
				mcomps.resizeVector(nums[0].isAddition() ? nums[0].size() : 1, m_zero);
				if(nums[0].isAddition()) {
					if(mcomps.size() < nums[0].size()) DIVISION_BY_POLYNOMIAL_ABORT
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
					if(CALCULATOR->aborted()) break;
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
						if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
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

		if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
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
				if(b_found && contains_abs_sym(divs[0], symsd[i])) b_found = false;
				if(b_found) i++;
				else symsd.erase(symsd.begin() + i);
			}
			for(size_t i = 0; i < symsd.size(); i++) {
				if(polynomial_long_division(nums[0], divs[0], symsd[i], mquo, mrem, eo2, false) && !mquo.isZero() && mrem != nums[0]) {
					if(CALCULATOR->aborted()) DIVISION_BY_POLYNOMIAL_ABORT
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
									int hp = simplify_highest_power(cb);
									if(!ca.isNumber() && hp > 1 && hp <= 5) {
										MathStructure mtest(mrem);
										CALCULATOR->beginTemporaryStopMessages();
										expand_partial_fractions(mtest, eo, false);
										if(mtest.countTotalChildren(true) < mrem.countTotalChildren(true) && simplify_highest_power(mtest) < hp && simplify_cumbersome_number(mtest) <= simplify_cumbersome_number(mrem)) {
											mrem.set_nocopy(mtest);
											CALCULATOR->endTemporaryStopMessages(true);
										} else {
											CALCULATOR->endTemporaryStopMessages();
										}
									}
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

	if(!b_ret) DIVISION_BY_POLYNOMIAL_ABORT

	MathStructure mbak;
	if(combine_divisions && !only_gcd && !combine_only && i_run <= 1) mbak.set(mstruct);
	mstruct.clear(true);
	for(size_t i = 0; i < divs.size() && (i == 0 || i_run > 1 || !combine_divisions); i++) {
		if(i == 0) mstruct = nums[0];
		else mstruct.add(nums[i], i > 1);
		if(!combine_divisions || only_gcd || combine_only) {
			divs[i].inverse();
			if(i == 0) mstruct.multiply(divs[i], true);
			else mstruct.last().multiply(divs[i], true);
		} else {
			if(i == 0) mstruct.calculateDivide(divs[i], eo);
			else mstruct.last().calculateDivide(divs[i], eo);
			if(i == 0 && i_run <= 1) {
				do_simplification(mstruct, eo, true, false, false, false, limit_size, I_RUN_ARG(2));
				int hp = simplify_highest_power(divs[0]);
				if(!nums[0].isNumber() && hp > 1 && hp <= 5) {
					CALCULATOR->beginTemporaryStopMessages();
					MathStructure mtest(mstruct);
					mtest.calculatesub(eo, eo, false);
					expand_partial_fractions(mtest, eo, false);
					if(mtest.countTotalChildren(true) < mstruct.countTotalChildren(true) && simplify_highest_power(mtest) < hp && simplify_cumbersome_number(mtest) <= simplify_cumbersome_number(mstruct)) {
						mstruct.set_nocopy(mtest);
						CALCULATOR->endTemporaryStopMessages(true);
					} else {
						CALCULATOR->endTemporaryStopMessages();
					}
				}
				if(simplify_highest_power(mstruct) > simplify_highest_power(mbak)) {
					mstruct.set(mbak);
					CALCULATOR->endTemporaryStopMessages();
					return false;
				}
			}
		}
	}
	CALCULATOR->endTemporaryStopMessages(true);

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

