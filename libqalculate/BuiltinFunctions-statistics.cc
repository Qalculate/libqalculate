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
#include "MathStructure-support.h"

#include <sstream>
#include <time.h>
#include <limits>
#include <algorithm>

using std::string;
using std::cout;
using std::vector;
using std::endl;

TotalFunction::TotalFunction() : MathFunction("total", 1) {
	setArgumentDefinition(1, new VectorArgument(""));
}
int TotalFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	mstruct.clear();
	bool b_calc = (eo.interval_calculation != INTERVAL_CALCULATION_VARIANCE_FORMULA && eo.interval_calculation != INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) || !vargs[0].containsInterval(true, true, false, 1, true);
	for(size_t index = 0; index < vargs[0].size(); index++) {
		if(CALCULATOR->aborted()) return 0;
		if(index == 0) mstruct = vargs[0][index];
		else if(b_calc) mstruct.calculateAdd(vargs[0][index], eo);
		else mstruct.add(vargs[0][index], true);
	}
	return 1;
}
PercentileFunction::PercentileFunction() : MathFunction("percentile", 2, 3) {
	Argument *varg = new VectorArgument("");
	varg->setHandleVector(true);
	setArgumentDefinition(1, varg);
	NumberArgument *arg = new NumberArgument();
	Number fr;
	arg->setMin(&fr);
	fr.set(100, 1, 0);
	arg->setMax(&fr);
	arg->setIncludeEqualsMin(true);
	arg->setIncludeEqualsMax(true);
	setArgumentDefinition(2, arg);
	IntegerArgument *iarg = new IntegerArgument();
	fr.set(1, 1, 0);
	iarg->setMin(&fr);
	fr.set(9, 1, 0);
	iarg->setMax(&fr);
	setArgumentDefinition(3, iarg);
	setDefaultValue(3, "8");
}
int PercentileFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	MathStructure v(vargs[0]);
	if(v.size() == 0) {mstruct.clear(); return 1;}
	MathStructure *mp;
	Number fr100(100, 1, 0);
	int i_variant = vargs[2].number().intValue();
	if(!v.sortVector()) {
		return 0;
	} else {
		Number pfr(vargs[1].number());
		if(pfr == fr100) {
			// Max value
			mstruct = v[v.size() - 1];
			return 1;
		} else if(pfr.isZero()) {
			// Min value
			mstruct = v[0];
			return 1;
		}
		pfr /= 100;
		if(pfr == nr_half) {
			// Median
			if(v.size() % 2 == 1) {
				mstruct = v[v.size() / 2];
			} else {
				mstruct = v[v.size() / 2 - 1];
				mstruct += v[v.size() / 2];
				mstruct *= nr_half;
			}
			return 1;
		}
		// Method numbers as in R
		switch(i_variant) {
			case 2: {
				Number ufr(pfr);
				ufr *= (long int) v.countChildren();
				if(ufr.isInteger()) {
					pfr = ufr;
					ufr++;
					mstruct = v[pfr.uintValue() - 1];
					if(ufr.uintValue() > v.size()) return 1;
					mstruct += v[ufr.uintValue() - 1];
					mstruct *= nr_half;
					return 1;
				}
			}
			case 1: {
				pfr *= (long int) v.countChildren();
				pfr.intervalToMidValue();
				pfr.ceil();
				size_t index = pfr.uintValue();
				if(index > v.size()) index = v.size();
				if(index == 0) index = 1;
				mstruct = v[index - 1];
				return 1;
			}
			case 3: {
				pfr *= (long int) v.countChildren();
				pfr.intervalToMidValue();
				pfr.round();
				size_t index = pfr.uintValue();
				if(index > v.size()) index = v.size();
				if(index == 0) index = 1;
				mstruct = v[index - 1];
				return 1;
			}
			case 4: {pfr *= (long int) v.countChildren(); break;}
			case 5: {pfr *= (long int) v.countChildren(); pfr += nr_half; break;}
			case 6: {pfr *= (long int) v.countChildren() + 1; break;}
			case 7: {pfr *= (long int) v.countChildren() - 1; pfr += 1; break;}
			case 9: {pfr *= Number(v.countChildren() * 4 + 1, 4); pfr += Number(3, 8); break;}
			case 8: {}
			default: {pfr *= Number(v.countChildren() * 3 + 1, 3); pfr += Number(1, 3); break;}
		}
		pfr.intervalToMidValue();
		Number ufr(pfr);
		ufr.ceil();
		Number lfr(pfr);
		lfr.floor();
		pfr -= lfr;
		size_t u_index = ufr.uintValue();
		size_t l_index = lfr.uintValue();
		if(u_index > v.size()) {
			mstruct = v[v.size() - 1];
			return 1;
		}
		if(l_index == 0) {
			mstruct = v[0];
			return 1;
		}
		mp = v.getChild(u_index);
		if(!mp) return 0;
		MathStructure gap(*mp);
		mp = v.getChild(l_index);
		if(!mp) return 0;
		gap -= *mp;
		gap *= pfr;
		mp = v.getChild(l_index);
		if(!mp) return 0;
		mstruct = *mp;
		mstruct += gap;
	}
	return 1;
}
MinFunction::MinFunction() : MathFunction("min", 1) {
	Argument *arg = new VectorArgument("");
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int MinFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	ComparisonResult cmp;
	const MathStructure *min = NULL;
	vector<const MathStructure*> unsolveds;
	bool b = false;
	for(size_t index = 0; index < vargs[0].size(); index++) {
		if(min == NULL) {
			min = &vargs[0][index];
		} else {
			cmp = min->compare(vargs[0][index]);
			if(cmp == COMPARISON_RESULT_LESS) {
				min = &vargs[0][index];
				b = true;
			} else if(COMPARISON_NOT_FULLY_KNOWN(cmp)) {
				if(CALCULATOR->showArgumentErrors()) {
					CALCULATOR->error(true, _("Unsolvable comparison in %s()."), name().c_str(), NULL);
				}
				unsolveds.push_back(&vargs[0][index]);
			} else {
				b = true;
			}
		}
	}
	if(min) {
		if(unsolveds.size() > 0) {
			if(!b) return 0;
			MathStructure margs; margs.clearVector();
			margs.addChild(*min);
			for(size_t i = 0; i < unsolveds.size(); i++) {
				margs.addChild(*unsolveds[i]);
			}
			mstruct.set(this, &margs, NULL);
			return 1;
		} else {
			mstruct = *min;
			return 1;
		}
	}
	return 0;
}
MaxFunction::MaxFunction() : MathFunction("max", 1) {
	Argument *arg = new VectorArgument("");
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int MaxFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	ComparisonResult cmp;
	const MathStructure *max = NULL;
	vector<const MathStructure*> unsolveds;
	bool b = false;
	for(size_t index = 0; index < vargs[0].size(); index++) {
		if(max == NULL) {
			max = &vargs[0][index];
		} else {
			cmp = max->compare(vargs[0][index]);
			if(cmp == COMPARISON_RESULT_GREATER) {
				max = &vargs[0][index];
				b = true;
			} else if(COMPARISON_NOT_FULLY_KNOWN(cmp)) {
				if(CALCULATOR->showArgumentErrors()) {
					CALCULATOR->error(true, _("Unsolvable comparison in %s()."), name().c_str(), NULL);
				}
				unsolveds.push_back(&vargs[0][index]);
			} else {
				b = true;
			}
		}
	}
	if(max) {
		if(unsolveds.size() > 0) {
			if(!b) return 0;
			MathStructure margs; margs.clearVector();
			margs.addChild(*max);
			for(size_t i = 0; i < unsolveds.size(); i++) {
				margs.addChild(*unsolveds[i]);
			}
			mstruct.set(this, &margs, NULL);
			return 1;
		} else {
			mstruct = *max;
			return 1;
		}
	}
	return 0;
}
ModeFunction::ModeFunction() : MathFunction("mode", 1) {
	Argument *arg = new VectorArgument("");
	arg->setHandleVector(true);
	setArgumentDefinition(1, arg);
}
int ModeFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].size() <= 0) {
		return 0;
	}
	size_t n = 0;
	bool b;
	vector<const MathStructure*> vargs_nodup;
	vector<size_t> is;
	const MathStructure *value = NULL;
	for(size_t index_c = 0; index_c < vargs[0].size(); index_c++) {
		b = true;
		for(size_t index = 0; index < vargs_nodup.size(); index++) {
			if(vargs_nodup[index]->equals(vargs[0][index_c])) {
				is[index]++;
				b = false;
				break;
			}
		}
		if(b) {
			vargs_nodup.push_back(&vargs[0][index_c]);
			is.push_back(1);
		}
	}
	for(size_t index = 0; index < is.size(); index++) {
		if(is[index] > n || (is[index] == n && comparison_is_equal_or_less(value->compare(*vargs_nodup[index])))) {
			n = is[index];
			value = vargs_nodup[index];
		}
	}
	if(value) {
		mstruct = *value;
		return 1;
	}
	return 0;
}

RandFunction::RandFunction() : MathFunction("rand", 0, 2) {
	setArgumentDefinition(1, new IntegerArgument());
	setDefaultValue(1, "0");
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE);
	Number nr(1, 1, 7);
	iarg->setMax(&nr);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "1");
}
int RandFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t n = (size_t) vargs[1].number().uintValue();
	if(n > 1) {mstruct.clearVector(); mstruct.resizeVector(n, m_zero);}
	Number nr;
	for(size_t i = 0; i < n; i++) {
		if(n > 1 && CALCULATOR->aborted()) return 0;
		if(vargs[0].number().isZero() || vargs[0].number().isNegative()) {
			nr.rand();
		} else {
			nr.intRand(vargs[0].number());
			nr++;
		}
		if(n > 1) mstruct[i] = nr;
		else mstruct = nr;
	}
	return 1;
}
bool RandFunction::representsReal(const MathStructure&, bool) const {return true;}
bool RandFunction::representsInteger(const MathStructure &vargs, bool) const {return vargs.size() > 0 && vargs[0].isNumber() && vargs[0].number().isPositive();}
bool RandFunction::representsNonNegative(const MathStructure&, bool) const {return true;}

RandnFunction::RandnFunction() : MathFunction("randnorm", 0, 3) {
	setDefaultValue(1, "0");
	setDefaultValue(2, "1");
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE);
	Number nr(1, 1, 7);
	iarg->setMax(&nr);
	setArgumentDefinition(3, iarg);
	setDefaultValue(3, "1");
}
int RandnFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t n = (size_t) vargs[2].number().uintValue();
	if(n > 1) {mstruct.clearVector(); mstruct.resizeVector(n, m_zero);}
#if MPFR_VERSION_MAJOR < 4
	Number nr_u, nr_v, nr_r2;
	for(size_t i = 0; i < n; i++) {
		do {
			if(CALCULATOR->aborted()) return 0;
			nr_u.rand(); nr_u *= 2; nr_u -= 1;
			nr_v.rand(); nr_v *= 2; nr_v -= 1;
			nr_r2 = (nr_u ^ 2) + (nr_v ^ 2);
		} while(nr_r2 > 1 || nr_r2.isZero());
		Number nr_rsq(nr_r2);
		nr_rsq.ln();
		nr_rsq /= nr_r2;
		nr_rsq *= -2;
		nr_rsq.sqrt();
		nr_u *= nr_rsq;
		if(n > 1) {
			mstruct[i] = nr_u;
			i++;
			if(i < n) {
				nr_v *= nr_rsq;
				mstruct[i] = nr_v;
			}
		} else {
			mstruct = nr_u;
		}
	}
#else
	Number nr;
	for(size_t i = 0; i < n; i++) {
		if(n > 1 && CALCULATOR->aborted()) return 0;
		nr.randn();
		if(n > 1) mstruct[i] = nr;
		else mstruct = nr;
	}
#endif
	if(!vargs[1].isOne()) mstruct *= vargs[1];
	if(!vargs[0].isZero()) mstruct += vargs[0];
	return 1;
}
bool RandnFunction::representsReal(const MathStructure&, bool) const {return true;}
bool RandnFunction::representsNonComplex(const MathStructure&, bool) const {return true;}
bool RandnFunction::representsNumber(const MathStructure&, bool) const {return true;}

RandPoissonFunction::RandPoissonFunction() : MathFunction("randpoisson", 1, 2) {
	setArgumentDefinition(1, new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE));
	IntegerArgument *iarg = new IntegerArgument("", ARGUMENT_MIN_MAX_POSITIVE, true, true, INTEGER_TYPE_SIZE);
	Number nr(1, 1, 7);
	iarg->setMax(&nr);
	setArgumentDefinition(2, iarg);
	setDefaultValue(2, "1");
}
int RandPoissonFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	size_t n = (size_t) vargs[1].number().uintValue();
	if(n > 1) {mstruct.clearVector(); mstruct.resizeVector(n, m_zero);}
	Number nr_L(vargs[0].number());
	nr_L.negate();
	nr_L.exp();
	Number nr_k, nr_p, nr_u;
	for(size_t i = 0; i < n; i++) {
		nr_k.clear(); nr_p = 1;
		do {
			if(CALCULATOR->aborted()) return 0;
			nr_k++;
			nr_u.rand();
			nr_p *= nr_u;
		} while(nr_p > nr_L);
		nr_k--;
		if(n > 1) mstruct[i] = nr_k;
		else mstruct = nr_k;
	}
	return 1;
}
bool RandPoissonFunction::representsReal(const MathStructure&, bool) const {return true;}
bool RandPoissonFunction::representsInteger(const MathStructure &vargs, bool) const {return true;}
bool RandPoissonFunction::representsNonNegative(const MathStructure&, bool) const {return true;}

