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

int limit_inf_cmp(const MathStructure &mstruct, const MathStructure &mcmp, const MathStructure &x_var) {
	if(mstruct.equals(mcmp)) return 0;
	bool b_multi1 = false, b_multi2 = false;
	const MathStructure *m1 = NULL, *m2 = NULL;
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].contains(x_var)) {
				if(!m1) {
					m1 = &mstruct[i];
				} else {
					int cmp = limit_inf_cmp(mstruct[i], *m1, x_var);
					if(cmp > 0) {
						m1 = &mstruct[i];
					} else if(cmp != -1) {
						return -2;
					}
					b_multi1 = true;
				}
			}
		}
	} else if(mstruct.contains(x_var, true)) {
		m1 = &mstruct;
	}
	if(mcmp.isMultiplication()) {
		for(size_t i = 0; i < mcmp.size(); i++) {
			if(mcmp[i].contains(x_var)) {
				if(!m2) {
					m2 = &mcmp[i];
				} else {
					int cmp = limit_inf_cmp(mcmp[i], *m2, x_var);
					if(cmp > 0) {
						m2 = &mcmp[i];
					} else if(cmp != -1) {
						return -2;
					}
					b_multi2 = true;
				}
			}
		}
	} else if(mcmp.contains(x_var, true)) {
		m2 = &mcmp;
	}
	if(!m1 && !m2) return 0;
	if(!m1) return -1;
	if(!m2) return 1;
	int itype1 = 0;
	int itype2 = 0;
	if(m1->isFunction() && m1->function()->id() == FUNCTION_ID_GAMMA) itype1 = 4;
	else if(m1->isPower() && m1->exponent()->contains(x_var, true)) itype1 = 3;
	else if(m1->equals(x_var) || (m1->isPower() && m1->base()->equals(x_var) && m1->exponent()->representsPositive())) itype1 = 2;
	else if(m1->isFunction() && m1->function()->id() == FUNCTION_ID_LOG) itype1 = 1;
	else return -2;
	if(m2->isFunction() && m2->function()->id() == FUNCTION_ID_GAMMA) itype2 = 4;
	else if(m2->isPower() && m2->exponent()->contains(x_var, true)) itype2 = 3;
	else if(m2->equals(x_var) || (m2->isPower() && m2->base()->equals(x_var) && m2->exponent()->representsPositive())) itype2 = 2;
	else if(m2->isFunction() && m2->function()->id() == FUNCTION_ID_LOG) itype2 = 1;
	else return -2;
	if(itype1 > itype2) return 1;
	if(itype2 > itype1) return -1;
	ComparisonResult cr = COMPARISON_RESULT_UNKNOWN;
	CALCULATOR->beginTemporaryEnableIntervalArithmetic();
	if(CALCULATOR->usesIntervalArithmetic()) {
		if(itype1 == 4) {
			cr = m1->getChild(1)->compare(*m2->getChild(1));
		} else if(itype1 == 1) {
			int cmp = limit_inf_cmp(*m1->getChild(1), *m2->getChild(1), x_var);
			if(cmp > 0) cr = COMPARISON_RESULT_LESS;
			else if(cmp == -1) cr = COMPARISON_RESULT_GREATER;
		} else if(itype1 == 3) {
			if(m1->exponent()->equals(*m2->exponent())) {
				if(m1->base()->contains(x_var, true) || m2->base()->contains(x_var, true)) {
					int cmp = limit_inf_cmp(*m1->base(), *m2->base(), x_var);
					if(cmp > 0) cr = COMPARISON_RESULT_LESS;
					else if(cmp == -1) cr = COMPARISON_RESULT_GREATER;
				} else {
					cr = m1->base()->compare(*m2->base());
				}
			} else if(m1->base()->equals(*m2->base())) {
				int cmp = limit_inf_cmp(*m1->exponent(), *m2->exponent(), x_var);
				if(cmp > 0) cr = COMPARISON_RESULT_LESS;
				else if(cmp == -1) cr = COMPARISON_RESULT_GREATER;
				else if(cmp == 0) cr = m1->exponent()->compare(*m2->exponent());
			}
		} else if(itype1 == 2) {
			if(m1->equals(x_var)) {
				if(m2->equals(x_var)) cr = COMPARISON_RESULT_EQUAL;
				else cr = m_one.compare(*m2->getChild(2));
			} else if(m2->equals(x_var)) {
				cr = m1->getChild(2)->compare(m_one);
			} else {
				cr = m1->getChild(2)->compare(*m2->getChild(2));
			}
		}
	}
	CALCULATOR->endTemporaryEnableIntervalArithmetic();
	if(cr == COMPARISON_RESULT_GREATER) return -1;
	else if(cr == COMPARISON_RESULT_LESS) return 1;
	else if(cr != COMPARISON_RESULT_EQUAL) return -2;
	if(!b_multi1 && !b_multi2) return 0;
	if(!b_multi1) return -1;
	if(!b_multi2) return 1;
	MathStructure mc1(mstruct), mc2(mcmp);
	for(size_t i = 0; i < mc1.size(); i++) {
		if(&mstruct[i] == m1) {mc1.delChild(i + 1, true); break;}
	}
	for(size_t i = 0; i < mc2.size(); i++) {
		if(&mcmp[i] == m2) {mc2.delChild(i + 1, true); break;}
	}
	return limit_inf_cmp(mc1, mc2, x_var);
}

bool is_limit_neg_power(const MathStructure &mstruct, const MathStructure &x_var, bool b_nil) {
	return mstruct.isPower() && (((!b_nil || !mstruct[1].contains(x_var, true)) && mstruct[1].representsNegative()) || (b_nil && mstruct[1].isMultiplication() && mstruct[1].size() == 2 && mstruct[1][1] == x_var && mstruct[1][0].representsNegative()));
}

bool limit_combine_divisions(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit) {
	if(mstruct.isAddition()) {
		bool b = false;
		bool b_nil = nr_limit.isInfinite(false) && nr_limit.number().isMinusInfinity();
		// 5/y + x/y + z = (5 + x)/y + z
		for(size_t i = 0; i < mstruct.size() && !b; i++) {
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size() && !b; i2++) {
					if(is_limit_neg_power(mstruct[i][i2], x_var, b_nil)) {
						b = true;
					}
				}
			} else if(is_limit_neg_power(mstruct[i], x_var, b_nil)) {
				b = true;
			}
		}
		if(!b) return false;
		b = false;
		MathStructure mstruct_units(mstruct);
		MathStructure mstruct_new(mstruct);
		for(size_t i = 0; i < mstruct_units.size(); i++) {
			if(mstruct_units[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
					if(!is_limit_neg_power(mstruct_units[i][i2], x_var, b_nil)) {
						mstruct_units[i].delChild(i2 + 1);
					} else {
						i2++;
					}
				}
				if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
				else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
				for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
					if(is_limit_neg_power(mstruct_new[i][i2], x_var, b_nil)) {
						mstruct_new[i].delChild(i2 + 1);
					} else {
						i2++;
					}
				}
				if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
				else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
			} else if(is_limit_neg_power(mstruct_new[i], x_var, b_nil)) {
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
				if(mstruct_new[i].isOne()) {
					mstruct_new[i].set(mstruct_units[i]);
				} else if(mstruct_units[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < mstruct_units[i].size(); i2++) {
						mstruct_new[i].multiply(mstruct_units[i][i2], true);
					}
				} else {
					mstruct_new[i].multiply(mstruct_units[i], true);
				}
			}
		}
		if(b) {
			if(mstruct_new.size() == 1) {
				mstruct.set(mstruct_new[0], true);
			} else {
				mstruct = mstruct_new;
			}
			return true;
		}
	}
	return false;
}
bool limit_combine_divisions2(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo) {
	if(mstruct.isAddition()) {
		MathStructure mden(1, 1, 0);
		bool b = false;
		bool b_nil = nr_limit.isInfinite(false) && nr_limit.number().isMinusInfinity();
		size_t i_d = 0;
		for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
			if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
					if(is_limit_neg_power(mstruct[i2][i3], x_var, b_nil)) {
						mden *= mstruct[i2][i3];
						mden.last()[1].negate();
						i_d++;
					} else if(!mstruct[i2][i3].isOne() && !mstruct[i2][i3].isMinusOne()) {
						b = true;
					}
				}
			} else if(is_limit_neg_power(mstruct[i2], x_var, b_nil)) {
				mden *= mstruct[i2];
				mden.last()[1].negate();
				i_d++;
			} else {
				b = true;
			}
		}
		if(mden.isOne() || !b || i_d > 10) return false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
				if(i2 != i) {
					if(mstruct[i2].isMultiplication()) {
						for(size_t i3 = 0; i3 < mstruct[i2].size(); i3++) {
							if(is_limit_neg_power(mstruct[i2][i3], x_var, b_nil)) {
								mstruct[i].multiply(mstruct[i2][i3], true);
								mstruct[i].last()[1].negate();
							}
						}
					} else if(is_limit_neg_power(mstruct[i2], x_var, b_nil)) {
						mstruct[i].multiply(mstruct[i2], true);
						mstruct[i].last()[1].negate();
					}
				}
			}
		}
		for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
			if(mstruct[i2].isMultiplication()) {
				for(size_t i3 = 0; i3 < mstruct[i2].size();) {
					if(is_limit_neg_power(mstruct[i2][i3], x_var, b_nil)) {
						mstruct[i2].delChild(i3 + 1);
					} else {
						i3++;
					}
				}
				if(mstruct[i2].size() == 0) mstruct[i2].set(1, 1, 0);
				else if(mstruct[i2].size() == 1) mstruct[i2].setToChild(1);
			} else if(is_limit_neg_power(mstruct[i2], x_var, b_nil)) {
				mstruct[i2].set(1, 1, 0);
			}
		}
		mden.calculatesub(eo, eo, true);
		mstruct.calculatesub(eo, eo, true);
		mstruct /= mden;
		return true;
	}
	return false;
}

bool contains_zero(const MathStructure &mstruct) {
	if(mstruct.isNumber() && !mstruct.number().isNonZero()) return true;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_zero(mstruct[i])) return true;
	}
	return false;
}
bool limit_contains_undefined(const MathStructure &mstruct) {
	bool b_zero = false, b_infinity = false;
	if(mstruct.isPower() && mstruct[0].isNumber() && ((!mstruct[0].number().isNonZero() && mstruct[1].representsNegative()) || mstruct[1].containsInfinity(true))) return true;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(limit_contains_undefined(mstruct[i])) return true;
		if(contains_zero(mstruct[i])) {
			if(b_infinity) return true;
			b_zero = true;
		}
		if(mstruct[i].containsInfinity(true)) {
			if(b_infinity || b_zero) return true;
			b_infinity = true;
		}
	}
	return false;
}
bool is_plus_minus_infinity(const MathStructure &mstruct) {
	return mstruct.isInfinite(false) || (mstruct.isPower() && mstruct[0].isZero() && mstruct[1].representsNegative()) || (mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[0].representsReal() && mstruct[1].isPower() && mstruct[1][0].isZero() && mstruct[1][1].representsNegative());
}

bool calculate_limit_sub(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, Number *polydeg = NULL, int lhop_depth = 0, bool keep_inf_x = false, bool reduce_addition = true) {

	if(CALCULATOR->aborted()) return false;
	if(mstruct == x_var) {
		if(keep_inf_x && nr_limit.isInfinite(false)) return true;
		mstruct.set(nr_limit, true);
		return true;
	}
	if(!mstruct.contains(x_var, true)) return true;

	switch(mstruct.type()) {
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isPower() && mstruct[i][0].isPower() && mstruct[i][0][0].contains(x_var, true) && !mstruct[i][0][0].representsNonPositive()) {
					MathStructure mtest(mstruct[i][0][0]);
					calculate_limit_sub(mtest, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					if(mtest.representsPositive()) {
						mstruct[i][0][1].calculateMultiply(mstruct[i][1], eo);
						mstruct[i].setToChild(1);
					}
				}
			}
			MathStructure mzero(1, 1, 0);
			MathStructure minfp(1, 1, 0);
			MathStructure mleft;
			vector<size_t> irecalc;
			MathStructure mbak(mstruct);
			bool b_inf = false;
			bool b_li = nr_limit.isInfinite(false);
			bool b_fail = false;
			bool b_test_inf = b_li;
			bool b_possible_zero = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				bool b = false;
				if(!b_fail && mstruct[i].isPower() && mstruct[i][0].contains(x_var, true) && mstruct[i][1].representsNegative()) {
					if(mstruct[i][1].isMinusOne()) {
						mstruct[i].setToChild(1);
					} else {
						mstruct[i][1].calculateNegate(eo);
					}
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					if(mstruct[i].isZero()) {
						b = true;
						if(b_test_inf && !mbak[i][1].contains(x_var, true)) b_test_inf = false;
						if(minfp.isOne()) {
							minfp = mbak[i];
							if(!b_li) {
								if(minfp[1].isMinusOne()) minfp.setToChild(1);
								else minfp[1].calculateNegate(eo);
							}
						} else {
							minfp.multiply(mbak[i], true);
							if(!b_li) {
								if(minfp.last()[1].isMinusOne()) minfp.last().setToChild(1);
								else minfp.last()[1].calculateNegate(eo);
							}
						}
						irecalc.push_back(i);
						mstruct[i].inverse();
					} else if(mstruct[i].isInfinite()) {
						b = true;
						if(mzero.isOne()) {
							mzero = mbak[i];
							if(b_li) {
								if(mzero[1].isMinusOne()) mzero.setToChild(1);
								else mzero[1].calculateNegate(eo);
							}
						} else {
							mzero.multiply(mbak[i], true);
							if(b_li) {
								if(mzero.last()[1].isMinusOne()) mzero.last().setToChild(1);
								else mzero.last()[1].calculateNegate(eo);
							}
						}
						mstruct[i].clear(true);
					} else {
						mstruct[i].calculateInverse(eo);
					}
				} else {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
				}
				if(!b_fail && !b) {
					if(mstruct[i].isZero()) {
						if(b_li && mbak[i].isPower()) {
							if(b_test_inf && !mbak[i][1].contains(x_var, true)) b_test_inf = false;
							if(mzero.isOne()) {
								mzero = mbak[i];
								if(mzero[1].isMinusOne()) mzero.setToChild(1);
								else mzero[1].calculateNegate(eo);
							} else {
								mzero.multiply(mbak[i], true);
								if(mzero.last()[1].isMinusOne()) mzero.last().setToChild(1);
								else mzero.last()[1].calculateNegate(eo);
							}
						} else {
							b_test_inf = false;
							if(mzero.isOne()) {
								mzero = mbak[i];
								if(b_li) {
									if(!mzero.isPower()) mzero.inverse();
									else if(mzero[1].isMinusOne()) mzero.setToChild(1);
									else mzero[1].calculateNegate(eo);
								}
							} else {
								mzero.multiply(mbak[i], true);
								if(b_li) {
									if(!mzero.last().isPower()) mzero.last().inverse();
									else if(mzero.last()[1].isMinusOne()) mzero.last().setToChild(1);
									else mzero.last()[1].calculateNegate(eo);
								}
							}
						}
					} else if(is_plus_minus_infinity(mstruct[i])) {
						if(mstruct[i].isInfinite()) {
							b_inf = true;
							if(minfp.isOne()) {
								minfp = mbak[i];
								if(!b_li) {
									if(!minfp.isPower()) minfp.inverse();
									else if(minfp[1].isMinusOne()) minfp.setToChild(1);
									else minfp[1].calculateNegate(eo);
								}
							} else {
								minfp.multiply(mbak[i], true);
								if(!b_li) {
									if(!minfp.last().isPower()) minfp.last().inverse();
									else if(minfp.last()[1].isMinusOne()) minfp.last().setToChild(1);
									else minfp.last()[1].calculateNegate(eo);
								}
							}
						} else {
							b_test_inf = false;
							if(minfp.isOne()) {
								minfp = mbak[i];
								if(!b_li) {
									if(!minfp.isPower()) minfp.inverse();
									else if(minfp[1].isMinusOne()) minfp[1].setToChild(1);
									else minfp[1].calculateNegate(eo);
								}
							} else {
								minfp.multiply(mbak[i], true);
								if(!b_li) {
									if(!minfp.last().isPower()) minfp.last().inverse();
									else if(minfp.last()[1].isMinusOne()) minfp.last().setToChild(1);
									else minfp.last()[1].calculateNegate(eo);
								}
							}
						}
					} else if(!mstruct[i].representsNonZero(true) || !mstruct[i].representsNumber(true)) {
						if(!mstruct[i].contains(x_var) && !mstruct.isZero() && mstruct[i].representsNumber(true)) {
							mstruct[i].ref();
							mleft.addChild_nocopy(&mstruct[i]);
							b_possible_zero = true;
						} else {
							b_fail = true;
						}
					} else {
						mstruct[i].ref();
						mleft.addChild_nocopy(&mstruct[i]);
					}
				}
			}
			if(b_li) b_inf = !b_inf;
			if(!b_fail && b_li && b_test_inf && !mzero.isOne() && !minfp.isOne()) {
				MathStructure mnum(minfp);
				MathStructure mden(mzero);
				mnum.calculatesub(eo, eo, false);
				mden.calculatesub(eo, eo, false);
				MathStructure mnum_bak(mnum), mden_bak(mden);
				calculate_limit_sub(mnum, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true);
				calculate_limit_sub(mden, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true);
				if(mnum.contains(x_var, true) && mden.contains(x_var, true)) {
					int cmp = limit_inf_cmp(mnum, mden, x_var);
					if(cmp == 0) {
						mstruct.set(mnum);
						EvaluationOptions eo2 = eo;
						eo2.expand = false;
						mstruct.calculateDivide(mden, eo2);
						if(!mstruct.contains(x_var)) {
							for(size_t i = 0; i < mleft.size(); i++) {
								mstruct.calculateMultiply(mleft[i], eo);
							}
							break;
						}
					}
					MathStructure mnum_b(mnum), mden_b(mden);
					calculate_limit_sub(mnum, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					calculate_limit_sub(mden, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					if(mnum.isInfinite(false) && mden.isInfinite(false)) {
						if(cmp > 0) {
							if(keep_inf_x) {
								mstruct.set(mnum_b, true);
								mstruct.calculateDivide(mden_b, eo);
								break;
							}
							mstruct.set(mnum, true);
							if(mden.number().isNegative()) mstruct.calculateNegate(eo);
							for(size_t i = 0; i < mleft.size(); i++) {
								mstruct.calculateMultiply(mleft[i], eo);
							}
							break;
						} else if(cmp == -1) {
							mstruct.clear(true);
							break;
						}
						if(mnum_b.isPower() && mden_b.isPower() && mnum_b[1].contains(x_var, true) && mden_b[1].contains(x_var, true)) {
							bool b = true;
							if(mden_b[1] != mnum_b[1]) {
								b = false;
								Number npow1(1, 1, 0), npow2;
								MathStructure *x_p = &mnum_b[1];
								if(mnum_b[1].isMultiplication() && mnum_b[1].size() == 2 && mnum_b[1][0].isNumber()) {
									npow1 = mnum_b[1][0].number();
									x_p = &mnum_b[1][1];
								}
								if(mden_b[1] == *x_p) npow2.set(1, 1, 0);
								else if(mden_b[1].isMultiplication() && mden_b[1].size() == 2 && mden_b[1][0].isNumber() && mden_b[1][1] == *x_p) npow2 = mden_b[1][0].number();
								if(!npow2.isZero() && npow1.isRational() && npow2.isRational()) {
									if(npow1.isGreaterThan(npow2)) {
										npow1 /= npow2;
										npow2 = npow1.denominator();
										npow1 = npow1.numerator();
									}
									if(mnum.number().isMinusInfinity() && !npow1.isOne()) {
										if(npow2.isOne() && mden.number().isPlusInfinity()) {
											mden_b[0].factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, true);
											if(mden_b[0].isPower() && mden_b[0][1].isInteger()) {
												mden_b[0][1].number() /= npow1;
												if(mden_b[0][1].number().isInteger()) {
													mden_b[0].calculateRaiseExponent(eo);
													b = true;
												}
											}
										}
									} else if(mden.number().isMinusInfinity() && !npow2.isOne()) {
										if(npow1.isOne() && mnum.number().isPlusInfinity()) {
											mnum_b[0].factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, true);
											if(mnum_b[0].isPower() && mnum_b[0][1].isInteger()) {
												mnum_b[0][1].number() /= npow2;
												if(mnum_b[0][1].number().isInteger()) {
													mnum_b[0].calculateRaiseExponent(eo);
													mnum_b[1] = mden_b[1];
													b = true;
												}
											}
										}
									} else {
										if(!npow1.isOne()) mnum_b[0].calculateRaise(npow1, eo);
										if(!npow2.isOne()) mden_b[0].calculateRaise(npow2, eo);
										mnum_b.childUpdated(1);
										mden_b.childUpdated(1);
										if(!npow1.isOne()) {
											if(&mnum_b[1] == x_p) {
												npow1.recip();
												mnum_b[1] *= npow1;
												mnum_b[1].swapChildren(1, 2);
											} else {
												mnum_b[1][0].number() /= npow1;
											}
										}
										b = true;
									}
								}
							}
							if(b) {
								mstruct.set(mnum_b[0], true);
								mstruct /= mden_b[0];
								mstruct.transformById(FUNCTION_ID_LOG);
								mstruct *= mnum_b[1];
								mstruct.raise(CALCULATOR->getVariableById(VARIABLE_ID_E));
								mstruct.swapChildren(1, 2);
								calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, keep_inf_x);
								for(size_t i = 0; i < mleft.size(); i++) {
									mstruct.calculateMultiply(mleft[i], eo);
								}
								break;
							}
						}
					}
				}
			}
			if(!b_possible_zero && !b_fail && lhop_depth < 5 && !mzero.isOne() && !minfp.isOne() && mzero.countTotalChildren(false) + minfp.countTotalChildren(false) < 250) {
				//L'Hôpital's rule
				MathStructure mden, mnum;
				for(size_t i2 = 0; i2 < 2; i2++) {
					if((i2 == 0) != b_inf) {
						mnum = mzero;
						mden = minfp;
						if(b_li) {
							mden.inverse();
							mnum.inverse();
						}
					} else {
						mnum = minfp;
						mden = mzero;
						if(!b_li) {
							mden.inverse();
							mnum.inverse();
						}
					}
					if(mnum.differentiate(x_var, eo) && !contains_diff_for(mnum, x_var) && mden.differentiate(x_var, eo) && !contains_diff_for(mden, x_var)) {
						mnum /= mden;
						mnum.eval(eo);
						calculate_limit_sub(mnum, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1);
						if(!limit_contains_undefined(mnum)) {
							mstruct.set(mnum, true);
							for(size_t i = 0; i < mleft.size(); i++) {
								mstruct.calculateMultiply(mleft[i], eo);
							}
							return true;
						}
					}
				}
			}
			for(size_t i = 0; i < irecalc.size(); i++) {
				mstruct[irecalc[i]] = mbak[irecalc[i]];
				calculate_limit_sub(mstruct[irecalc[i]], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
			if(keep_inf_x && mstruct.isInfinite(false)) {
				mstruct = mbak;
				if(reduce_addition) {
					for(size_t i = 0; i < mstruct.size(); i++) {
						calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, reduce_addition);
						if(mstruct[i].isZero()) mstruct[i] = mbak[i];
					}
					mstruct.childrenUpdated();
					mstruct.calculatesub(eo, eo, false);
				}
			}
			break;
		}
		case STRUCT_ADDITION: {
			MathStructure mbak(mstruct);
			bool b = limit_combine_divisions(mstruct, x_var, nr_limit);
			if(mstruct.isAddition()) b = limit_combine_divisions2(mstruct, x_var, nr_limit, eo);
			if(b) {
				if(lhop_depth > 0) return calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, polydeg, lhop_depth);
				if(calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, polydeg, lhop_depth) && !limit_contains_undefined(mstruct)) return true;
				mstruct = mbak;
			}
			if(nr_limit.isInfinite(false)) {
				size_t i_cmp = 0;
				b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true);
				}
				if(mstruct.contains(x_var, true) && (!keep_inf_x || reduce_addition)) {
					bool bfac = false;
					MathStructure mstruct_units(mstruct);
					MathStructure mstruct_new(mstruct);
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(mstruct_units[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < mstruct_units[i].size();) {
								if(!mstruct_units[i][i2].contains(x_var, true)) {
									mstruct_units[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_units[i].size() == 0) mstruct_units[i].setUndefined();
							else if(mstruct_units[i].size() == 1) mstruct_units[i].setToChild(1);
							for(size_t i2 = 0; i2 < mstruct_new[i].size();) {
								if(mstruct_new[i][i2].contains(x_var, true)) {
									mstruct_new[i].delChild(i2 + 1);
								} else {
									i2++;
								}
							}
							if(mstruct_new[i].size() == 0) mstruct_new[i].set(1, 1, 0);
							else if(mstruct_new[i].size() == 1) mstruct_new[i].setToChild(1);
						} else if(mstruct_units[i].contains(x_var, true)) {
							mstruct_new[i].set(1, 1, 0);
						} else {
							mstruct_units[i].setUndefined();
						}
					}
					for(size_t i = 0; i < mstruct_units.size(); i++) {
						if(!mstruct_units[i].isUndefined()) {
							for(size_t i2 = i + 1; i2 < mstruct_units.size(); i2++) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_new[i].add(mstruct_new[i2], true);
									bfac = true;
								}
							}
							bool bfac2 = false;
							MathStructure mzero(mstruct_new[i]);
							mzero.calculatesub(eo, eo, false);
							if(bfac && mzero.isZero() && lhop_depth < 5) {
								MathStructure mfac(mbak[i]);
								bool b_diff = false;
								calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, false);
								if(mfac != mstruct[i]) b_diff = true;
								for(size_t i2 = i + 1; i2 < mstruct_units.size(); i2++) {
									if(mstruct_units[i2] == mstruct_units[i]) {
										mfac.add(mbak[i2], true);
										calculate_limit_sub(mfac.last(), x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, false);
										if(!b_diff && mfac.last() != mstruct[i2]) b_diff = true;
										bfac2 = true;
									}
								}
								if(bfac2) {
									mfac.calculatesub(eo, eo, false);
									if(!mfac.isAddition()) {
										calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
										mstruct_new[i].set(mfac, true);
									} else if(!b_diff) {
										bfac2 = false;
									} else if(mfac.size() == 2 && !mfac.containsType(STRUCT_FUNCTION)) {
										MathStructure mfac2(mfac);
										MathStructure mmul(mfac);
										mmul.last().calculateNegate(eo);
										mfac.calculateMultiply(mmul, eo);
										mfac.divide(mmul);
										calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
										if(limit_contains_undefined(mfac)) {
											mfac2.factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, false);
											if(!mfac2.isAddition()) {
												calculate_limit_sub(mfac2, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
												mstruct_new[i].set(mfac2, true);
											} else {
												bfac2 = false;
											}
										} else {
											mstruct_new[i].set(mfac, true);
										}
									} else {
										mfac.factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, false);
										if(!mfac.isAddition()) {
											calculate_limit_sub(mfac, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth + 1, true);
											mstruct_new[i].set(mfac, true);
										} else if(mstruct_units[i].isFunction() && mstruct_units[i].function()->id() == FUNCTION_ID_LOG) {
											mstruct_new[i].clear(true);
										} else {
											bfac2 = false;
										}
									}
								}
							}
							if(!bfac2) {
								if(mstruct_new[i].isOne()) {
									mstruct_new[i].set(mstruct_units[i]);
								} else if(mstruct_units[i].isMultiplication()) {
									for(size_t i2 = 0; i2 < mstruct_units[i].size(); i2++) {
										mstruct_new[i].multiply(mstruct_units[i][i2], true);
									}
								} else {
									mstruct_new[i].multiply(mstruct_units[i], true);
								}
							}
							for(size_t i2 = i + 1; i2 < mstruct_units.size(); i2++) {
								if(mstruct_units[i2] == mstruct_units[i]) {
									mstruct_units[i2].setUndefined();
									mstruct_new[i2].clear();
								}
							}
						}
					}
					if(bfac) {
						for(size_t i = 0; mstruct_new.size() > 1 && i < mstruct_new.size();) {
							if(mstruct_new[i].isZero()) {
								mstruct_new.delChild(i + 1);
							} else {
								i++;
							}
						}
						mstruct = mstruct_new;
						if(mstruct.size() == 1) {
							mstruct.setToChild(1, true);
							if(!keep_inf_x) calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
							break;
						} else if(!mstruct.contains(x_var, true)) {
							b = false;
						}
					}
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(i > 0 && mstruct[i].contains(x_var, true)) {
							int cmp = limit_inf_cmp(mstruct[i], mstruct[i_cmp], x_var);
							if(cmp > 0) {
								mstruct.delChild(i_cmp + 1, false);
								i--;
								i_cmp = i;
							} else if(cmp == -1) {
								mstruct.delChild(i + 1, false);
								i--;
							} else {
								b = false;
							}
						}
					}
					if(b) {
						MathStructure mbak(mstruct);
						for(size_t i = 0; i < mstruct.size();) {
							if(!mstruct[i].contains(x_var, true)) {
								if(mstruct[i].representsNumber()) {
									mstruct.delChild(i + 1, false);
								} else {
									mstruct = mbak;
									break;
								}
							} else {
								i++;
							}
						}
					}
				}
				if(!b || !keep_inf_x) {
					for(size_t i = 0; i < mstruct.size(); i++) {
						calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
					}
				}
				mstruct.childrenUpdated();
				if(mstruct.size() == 1) mstruct.setToChild(1, true);
				else if(!b || !keep_inf_x) mstruct.calculatesub(eo, eo, false);
				break;
			} else {
				for(size_t i = 0; i < mstruct.size(); i++) {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth);
				}
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_POWER: {
			MathStructure mbak(mstruct);
			calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
			calculate_limit_sub(mstruct[1], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
			if(is_plus_minus_infinity(mstruct[1]) && (mstruct[0].isOne() || mstruct[0].isZero()) && mbak[1].contains(x_var, true) && mbak[0].contains(x_var, true)) {
				mstruct.set(mbak[0], true);
				mstruct.transformById(FUNCTION_ID_LOG);
				mstruct *= mbak[1];
				mstruct.raise(CALCULATOR->getVariableById(VARIABLE_ID_E));
				mstruct.swapChildren(1, 2);
				calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, keep_inf_x);
				break;
			}
			if(mstruct[0].isFunction() && (mstruct[0].function()->id() == FUNCTION_ID_ASIN || mstruct[0].function()->id() == FUNCTION_ID_ACOS) && mstruct[0].size() == 1 && mstruct[0][0].isInfinite(false) && mstruct[1].representsNegative()) {
				mstruct.clear(true);
				break;
			}
			if(keep_inf_x && nr_limit.isInfinite(false) && (mstruct[0].isInfinite(false) || mstruct[1].isInfinite(false))) {
				MathStructure mbak2(mstruct);
				mstruct.calculatesub(eo, eo, false);
				if(mstruct.isInfinite(false)) {
					mstruct = mbak2;
					if(mstruct[0].isInfinite(false)) {
						mstruct[0] = mbak[0];
						calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, reduce_addition && !mstruct[1].isInfinite(false));
					} else if(mbak[0].contains(x_var, true)) {
						mstruct[0] = mbak[0];
					}
					if(mstruct[1].isInfinite(false)) {
						mstruct[1] = mbak[1];
						calculate_limit_sub(mstruct[1], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, false);
					} else if(mbak[1].contains(x_var, true)) {
						mstruct[1] = mbak[1];
					}
				}
			} else if(mstruct[0].isNumber() && !mstruct[0].number().isNonZero() && mstruct[1].representsNegative() && mbak[0].contains(x_var, true) > 0) {
				bool b_test = true;
				int i_sgn = 0;
				if(mstruct[0].number().isInterval(false) && (mstruct[0].number().hasImaginaryPart() || !mstruct[1].isNumber())) {
					b_test = false;
				}
				if(b_test && ((mbak[0].isFunction() && mbak[0].function()->id() == FUNCTION_ID_ABS) || mstruct[1].representsEven())) {
					i_sgn = 1;
					b_test = false;
				} else if(b_test) {
					if(mstruct[0].number().isInterval(false) && !mstruct[0].number().isNonNegative() && !mstruct[0].number().isNonPositive()) {
						b_test = false;
					} else {
						MathStructure mpow(mbak[0]);
						if(mstruct[1].isMinusOne()) {
							MathStructure mfac(mpow);
							mfac.factorize(eo, false, false, 0, false, false, NULL, m_undefined, false, true);
							if(mfac.isPower() && mfac[1].representsEven()) {
								i_sgn = 1;
								b_test = false;
							}
						} else {
							mpow ^= mstruct[1];
							mpow.last().calculateNegate(eo);
						}
						if(b_test) {
							MathStructure mdiff(mpow);
							if(mdiff.differentiate(x_var, eo) && !contains_diff_for(mdiff, x_var)) {
								mdiff.replace(x_var, nr_limit);
								CALCULATOR->beginTemporaryStopMessages();
								mdiff.eval(eo);
								if(!CALCULATOR->endTemporaryStopMessages()) {
									if(mdiff.representsPositive()) {
										b_test = false;
										if(approach_direction > 0) i_sgn = 1;
										else if(approach_direction < 0) i_sgn = -1;
									} else if(mdiff.representsNegative()) {
										b_test = false;
										if(approach_direction > 0) i_sgn = -1;
										else if(approach_direction < 0) i_sgn = 1;
									}
								}
							}
							if(b_test) {
								MathStructure mtestn(nr_limit);
								if(eo.approximation != APPROXIMATION_EXACT) {
									EvaluationOptions eo2 = eo;
									eo2.approximation = APPROXIMATION_APPROXIMATE;
									mtestn.eval(eo2);
								}
								if(mtestn.isNumber() && mtestn.number().isReal()) {
									for(int i = 10; i < 20; i++) {
										if(approach_direction == 0 || (i % 2 == (approach_direction < 0))) {
											Number nr_test(i % 2 == 0 ? 1 : -1, 1, -(i / 2));
											if(!mtestn.number().isZero()) {
												nr_test++;
												if(!nr_test.multiply(mtestn.number())) {i_sgn = 0; break;}
											}
											MathStructure mtest(mpow);
											mtest.replace(x_var, nr_test);
											CALCULATOR->beginTemporaryStopMessages();
											mtest.eval(eo);
											if(CALCULATOR->endTemporaryStopMessages()) {i_sgn = 0; break;}
											int new_sgn = 0;
											if(mtest.representsPositive()) new_sgn = 1;
											else if(mtest.representsNegative()) new_sgn = -1;
											if(new_sgn != 0 || mtest.isNumber()) {
												if(new_sgn == 0 || (i_sgn != 0 && i_sgn != new_sgn)) {i_sgn = 0; break;}
												i_sgn = new_sgn;
											}
										}
										if(CALCULATOR->aborted()) {i_sgn = 0; break;}
									}
								}
							}
						}
					}
				}
				if(i_sgn != 0) {
					if(mstruct[0].number().isInterval()) {
						Number nr_pow = mstruct[1].number();
						Number nr_high(mstruct[0].number());
						if(nr_pow.negate() && nr_high.raise(nr_pow) && !nr_high.hasImaginaryPart()) {
							if(nr_high.isNonNegative() && i_sgn > 0) nr_high = nr_high.upperEndPoint();
							else if(nr_high.isNonPositive() && i_sgn < 0) nr_high = nr_high.lowerEndPoint();
							if(nr_high.isNonZero() && nr_high.recip()) {
								Number nr;
								if(i_sgn > 0) nr.setInterval(nr_high, nr_plus_inf);
								else if(i_sgn < 0) nr.setInterval(nr_minus_inf, nr_high);
								mstruct.set(nr, true);
								if(b_test) CALCULATOR->error(false, _("Limit for %s determined graphically."), format_and_print(mbak).c_str(), NULL);
								break;
							}
						}
					} else {
						if(b_test) CALCULATOR->error(false, _("Limit for %s determined graphically."), format_and_print(mbak).c_str(), NULL);
						if(i_sgn > 0) mstruct.set(nr_plus_inf, true);
						else if(i_sgn < 0) mstruct.set(nr_minus_inf, true);
						break;
					}
				}
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_FUNCTION: {
			if(keep_inf_x && nr_limit.isInfinite(false) && mstruct.size() == 1 && (mstruct.function()->id() == FUNCTION_ID_LOG || mstruct.function()->id() == FUNCTION_ID_GAMMA)) {
				MathStructure mbak(mstruct);
				calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
				if(mstruct[0].isInfinite(false) && (mstruct[0].number().isPlusInfinity() || mstruct.function()->id() == FUNCTION_ID_LOG)) {
					mstruct[0] = mbak[0];
					calculate_limit_sub(mstruct[0], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, true, mstruct.function()->id() == FUNCTION_ID_LOG && reduce_addition);
					break;
				}
			} else if(mstruct.function()->id() == FUNCTION_ID_TAN && mstruct.size() == 1) {
				MathStructure mbak(mstruct);
				mstruct.setFunctionId(FUNCTION_ID_SIN);
				mbak.setFunctionId(FUNCTION_ID_COS);
				mstruct /= mbak;
				return calculate_limit_sub(mstruct, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, keep_inf_x, reduce_addition);
			} else if(mstruct.function()->id() == FUNCTION_ID_FLOOR || mstruct.function()->id() == FUNCTION_ID_CEIL || mstruct.function()->id() == FUNCTION_ID_TRUNC) {
				MathStructure mlimit(mstruct[0]);
				calculate_limit_sub(mlimit, x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, keep_inf_x, reduce_addition);
				mlimit.transformById(FUNCTION_ID_FRAC);
				ComparisonResult cr = mlimit.compare(m_zero);
				mlimit.setToChild(1);
				if(!COMPARISON_IS_NOT_EQUAL(cr)) {
					if(approach_direction == 0 || cr != COMPARISON_RESULT_EQUAL) return false;
					if(mstruct.function()->id() == FUNCTION_ID_TRUNC) {
						cr = mlimit.compare(m_zero);
						if(cr != COMPARISON_RESULT_EQUAL && cr != COMPARISON_RESULT_GREATER && cr != COMPARISON_RESULT_LESS) return false;
					}
					MathStructure mdiff(mstruct[0]);
					int i_sgn = 0;
					if(mdiff.differentiate(x_var, eo) && !contains_diff_for(mdiff, x_var)) {
						mdiff.replace(x_var, nr_limit);
						ComparisonResult cr2 = mdiff.compare(m_zero);
						if(cr2 == COMPARISON_RESULT_GREATER) i_sgn = -1;
						else if(cr2 == COMPARISON_RESULT_LESS) i_sgn = 1;
					}
					if(i_sgn == 0) return false;
					i_sgn *= approach_direction;
					if(i_sgn < 0 && (mstruct.function()->id() == FUNCTION_ID_FLOOR || (mstruct.function()->id() == FUNCTION_ID_TRUNC && cr == COMPARISON_RESULT_LESS))) mlimit -= m_one;
					else if(i_sgn > 0 && (mstruct.function()->id() == FUNCTION_ID_CEIL || (mstruct.function()->id() == FUNCTION_ID_TRUNC && cr == COMPARISON_RESULT_GREATER))) mlimit += m_one;
				}
				mstruct[0] = mlimit;
				mstruct.childrenUpdated();
			} else {
				for(size_t i = 0; i < mstruct.size(); i++) {
					calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false);
				}
				mstruct.childrenUpdated();
			}
			if(approach_direction != 0 && mstruct.function()->id() == FUNCTION_ID_GAMMA && mstruct.size() == 1 && mstruct[0].isInteger() && mstruct[0].number().isNonPositive()) {
				if((mstruct[0].number().isEven() && approach_direction < 0) || (mstruct[0].number().isOdd() && approach_direction > 0)) {
					mstruct.set(nr_minus_inf, true);
				} else {
					mstruct.set(nr_plus_inf, true);
				}
				break;
			}
			mstruct.calculateFunctions(eo, true);
			mstruct.calculatesub(eo, eo, false);
			break;
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				calculate_limit_sub(mstruct[i], x_var, nr_limit, eo, approach_direction, NULL, lhop_depth, false, reduce_addition);
			}
			mstruct.childrenUpdated();
			mstruct.calculatesub(eo, eo, false);
		}
	}
	return true;
}

bool replace_equal_limits(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, bool at_top = true) {
	if(!nr_limit.isInfinite(false)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_equal_limits(mstruct[i], x_var, nr_limit, eo, approach_direction, false)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(at_top) return b_ret;
	if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_SINH || mstruct.function()->id() == FUNCTION_ID_COSH) && mstruct.size() == 1 && mstruct.contains(x_var, true)) {
		MathStructure mterm1(CALCULATOR->getVariableById(VARIABLE_ID_E));
		mterm1.raise(mstruct[0]);
		MathStructure mterm2(mterm1);
		mterm2[1].negate();
		mterm1 *= nr_half;
		mterm2 *= nr_half;
		mstruct = mterm1;
		if(mstruct.function()->id() == FUNCTION_ID_SINH) mstruct -= mterm2;
		else mstruct += mterm2;
		return true;
	}
	return b_ret;
}
bool replace_equal_limits2(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, bool at_top = true) {
	if(!nr_limit.isInfinite(false)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_equal_limits2(mstruct[i], x_var, nr_limit, eo, approach_direction, false)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower() && mstruct[i][1] == x_var && (nr_limit.number().isMinusInfinity() || mstruct[i][0].representsNonNegative())) {
				for(size_t i2 = i + 1; i2 < mstruct.size();) {
					if(mstruct[i2].isPower() && mstruct[i2][1] == x_var && (nr_limit.number().isMinusInfinity() || mstruct[i2][0].representsNonNegative())) {
						mstruct[i][0].calculateMultiply(mstruct[i2][0], eo);
						mstruct.delChild(i2 + 1, false);
					} else {
						i2++;
					}
				}
				mstruct[i].childUpdated(1);
				mstruct.childUpdated(i + 1);
				if(mstruct.size() == 1) {
					mstruct.setToChild(1, true);
					break;
				}
			}
		}
	}
	return b_ret;
}
bool replace_equal_limits3(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &nr_limit, const EvaluationOptions &eo, int approach_direction, bool at_top = true) {
	if(!nr_limit.isInfinite(false)) return false;
	bool b_ret = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(replace_equal_limits3(mstruct[i], x_var, nr_limit, eo, approach_direction, false)) {
			mstruct.childUpdated(i + 1);
			b_ret = true;
		}
	}
	if(at_top) return b_ret;
	if(mstruct.isFunction() && (mstruct.function()->id() == FUNCTION_ID_ASINH || mstruct.function()->id() == FUNCTION_ID_ACOSH) && mstruct.size() == 1 && mstruct.contains(x_var, true)) {
		MathStructure mtest(mstruct[0]);
		calculate_limit_sub(mtest, x_var, nr_limit, eo, approach_direction);
		if(mtest.isInfinite(false)) {
			if(mtest.number().isPlusInfinity()) {
				mstruct.setFunctionId(FUNCTION_ID_LOG);
				mstruct[0] *= nr_two;
				return true;
			} else if(mstruct.function()->id() == FUNCTION_ID_ASINH) {
				mstruct.setFunctionId(FUNCTION_ID_LOG);
				mstruct[0] *= nr_two;
				mstruct.negate();
				return true;
			}
		}
	}
	return b_ret;
}

bool MathStructure::calculateLimit(const MathStructure &x_var, const MathStructure &limit, const EvaluationOptions &eo_pre, int approach_direction) {
	EvaluationOptions eo = eo_pre;
	eo.assume_denominators_nonzero = true;
	eo.warn_about_denominators_assumed_nonzero = false;
	eo.do_polynomial_division = false;
	UnknownVariable *var = new UnknownVariable("", format_and_print(x_var));
	Assumptions *ass = new Assumptions();
	MathStructure nr_limit(limit);
	if(eo.approximation != APPROXIMATION_EXACT && nr_limit.containsInterval(true, true, false, 0, true)) {
		eo.approximation = APPROXIMATION_EXACT_VARIABLES;
	}
	nr_limit.eval(eo);
	MathStructure munit;
	if(nr_limit.isMultiplication()) {
		bool b = true;
		for(size_t i = 0; i < nr_limit.size(); i++) {
			if(is_unit_multiexp(nr_limit[i])) {b = true; break;}
		}
		if(b) {
			munit = x_var;
			for(size_t i = 0; i < nr_limit.size(); i++) {
				if(is_unit_multiexp(nr_limit[i])) {
					munit.multiply(nr_limit[i]);
					nr_limit.delChild(i + 1);
				}
			}
			if(nr_limit.size() == 0) nr_limit.set(1, 1, 0, true);
			else if(nr_limit.size() == 1) nr_limit.setToChild(1, true);
			replace(x_var, munit);
		}
	}
	eo.approximation = eo_pre.approximation;
	if(nr_limit.representsReal() || nr_limit.isInfinite()) ass->setType(ASSUMPTION_TYPE_REAL);
	if(nr_limit.representsPositive()) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
	else if(nr_limit.representsNegative()) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
	else if(nr_limit.isZero()) {
		if(approach_direction < 0) ass->setSign(ASSUMPTION_SIGN_NEGATIVE);
		else if(approach_direction > 0) ass->setSign(ASSUMPTION_SIGN_POSITIVE);
		else ass->setSign(ASSUMPTION_SIGN_NONZERO);
	}
	var->setAssumptions(ass);
	replace(x_var, var);
	eval(eo);
	CALCULATOR->beginTemporaryStopMessages();
	MathStructure mbak(*this);
	if(replace_equal_limits(*this, var, nr_limit, eo, approach_direction)) eval(eo);
	replace_equal_limits2(*this, var, nr_limit, eo, approach_direction);
	if(replace_equal_limits3(*this, var, nr_limit, eo, approach_direction)) {
		eval(eo);
		replace_equal_limits2(*this, var, nr_limit, eo, approach_direction);
	}
	do_simplification(*this, eo, true, false, false, true, true);
	eo.do_polynomial_division = true;
	calculate_limit_sub(*this, var, nr_limit, eo, approach_direction);
	if(CALCULATOR->aborted() || (containsInfinity(true) && !isInfinite(true)) || limit_contains_undefined(*this) || containsFunctionId(FUNCTION_ID_FLOOR) || containsFunctionId(FUNCTION_ID_CEIL) || containsFunctionId(FUNCTION_ID_TRUNC)) {
		set(mbak);
		replace(var, munit.isZero() ? x_var : munit);
		var->destroy();
		CALCULATOR->endTemporaryStopMessages();
		return false;
	}
	replace(var, nr_limit);
	var->destroy();
	CALCULATOR->endTemporaryStopMessages(true);
	return true;
}

