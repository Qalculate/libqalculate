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

bool combine_powers(MathStructure &m, const MathStructure &x_var, const EvaluationOptions &eo, const EvaluationOptions &feo) {
	bool b_ret = false;
	if(!m.isMultiplication()) {
		for(size_t i = 0; i < m.size(); i++) {
			if(combine_powers(m[i], x_var, eo, feo)) {
				m.childUpdated(i + 1);
				b_ret = true;
			}
		}
		return b_ret;
	}
	for(size_t i = 0; i < m.size() - 1; i++) {
		if(m[i].isPower() && !m[i][0].contains(x_var, true) && m[i][1].contains(x_var, true)) {
			for(size_t i2 = i + 1; i2 < m.size(); i2++) {
				// a^(f(x))*b^(g(x))=e^(f(x)/ln(a)+g(x)/ln(b))
				if(m[i2].isPower() && !m[i2][0].contains(x_var, true) && m[i2][1].contains(x_var, true)) {
					if(m[i2][0] != m[i][0]) {
						if(!m[i2][0].isVariable() || m[i2][0].variable()->id() != VARIABLE_ID_E) {
							MathStructure mln(m[i2][0]);
							mln.transformById(FUNCTION_ID_LOG);
							mln.calculateFunctions(feo);
							m[i2][1].calculateMultiply(mln, eo);
						}
						if(!m[i][0].isVariable() || m[i][0].variable()->id() != VARIABLE_ID_E) {
							MathStructure mln(m[i][0]);
							mln.transformById(FUNCTION_ID_LOG);
							mln.calculateFunctions(feo);
							m[i][1].calculateMultiply(mln, eo);
							m[i][0] = CALCULATOR->getVariableById(VARIABLE_ID_E);
							m[i][0].calculatesub(eo, feo);
							m[i].childrenUpdated();
						}
					}
					m[i2][1].ref();
					m[i][1].add_nocopy(&m[i2][1], true);
					m[i][1].calculateAddLast(eo);
					m[i].childUpdated(2);
					m.childUpdated(i + 1);
					m.delChild(i2 + 1);
					b_ret = true;
				} else {
					i2++;
				}
			}
			if(b_ret && m.size() == 1) {
				m.setToChild(1, true);
			}
			return b_ret;
		}
	}
	return false;
}

bool isUnit_multi(const MathStructure &mstruct) {
	if(!mstruct.isMultiplication() || mstruct.size() == 0) return false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if((i > 0 || !mstruct[i].isNumber()) && !mstruct[i].isUnit_exp()) return false;
	}
	return true;
}

MathStructure *find_mvar(MathStructure &m, const MathStructure &x_var, MathStructure &mcoeff) {
	if(m.isAddition()) {
		MathStructure *mvar = find_mvar(m[0], x_var, mcoeff);
		if(!mvar) return NULL;
		for(size_t i = 1; i < m.size(); i++) {
			MathStructure mcoeffi;
			MathStructure *mvari = find_mvar(m[i], x_var, mcoeffi);
			if(!mvari || !mvari->equals(*mvar)) return NULL;
			mcoeff.add(mcoeffi, true);
		}
		mcoeff.evalSort(false);
		return mvar;
	}
	if(m.isMultiplication()) {
		MathStructure *mvar = NULL;
		size_t i_x = 0;
		for(size_t i = 0; i < m.size(); i++) {
			if(m[i].contains(x_var, true)) {
				if(mvar) return NULL;
				mvar = &m[i];
				i_x = i;
			}
		}
		mcoeff = m;
		mcoeff.delChild(i_x + 1, true);
		return mvar;
	}
	mcoeff = m_one;
	return &m;
}

bool get_multiplier(const MathStructure &mstruct, const MathStructure &xvar, MathStructure &mcoeff, size_t exclude_i = (size_t) -1) {
	const MathStructure *mcur = NULL;
	mcoeff.clear();
	for(size_t i = 0; ; i++) {
		if(mstruct.isAddition()) {
			if(i == exclude_i) i++;
			if(i >= mstruct.size()) break;
			mcur = &mstruct[i];
		} else {
			mcur = &mstruct;
		}
		if((*mcur) == xvar) {
			if(mcoeff.isZero()) mcoeff.set(1, 1, 0);
			else mcoeff.add(m_one, true);
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					b = true;
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
				} else if(xvar.isMultiplication() && xvar.size() > 0 && (*mcur)[i2] == xvar[0]) {
					if(mcur->size() - i2 < xvar.size()) break;
					b = true;
					for(size_t i3 = 1; i3 < xvar.size(); i3++) {
						if((*mcur)[i2 + i3] != xvar[i3]) {b = false; break;}
					}
					if(!b) break;
					if(mcoeff.isZero()) {
						if(mcur->size() == xvar.size()) {
							mcoeff.set(1, 1, 0);
						} else {
							for(size_t i3 = 0; i3 < mcur->size(); i3++) {
								if(i3 < i2 || i3 >= i2 + xvar.size()) {
									if(mcoeff.isZero()) mcoeff = (*mcur)[i3];
									else mcoeff.multiply((*mcur)[i3], true);
								}
							}
						}
					} else if(mcur->size() == xvar.size()) {
						mcoeff.add(m_one, true);
					} else {
						mcoeff.add(m_zero, true);
						for(size_t i3 = 0; i3 < mcur->size(); i3++) {
							if(i3 < i2 || i3 >= i2 + xvar.size()) {
								if(mcoeff[mcoeff.size() - 1].isZero()) mcoeff[mcoeff.size() - 1] = (*mcur)[i3];
								else mcoeff[mcoeff.size() - 1].multiply((*mcur)[i3], true);
							}
						}
					}
					break;
				}
			}
			if(!b) {
				mcoeff.clear();
				return false;
			}
		} else {
			mcoeff.clear();
			return false;
		}
		if(!mstruct.isAddition()) break;
	}
	if(mcoeff.isZero()) return false;
	mcoeff.evalSort();
	return true;
}
bool get_power(const MathStructure &mstruct, const MathStructure &xvar, MathStructure &mpow) {
	if(mstruct == xvar) {
		mpow = m_one;
		return true;
	}
	if(mstruct.isPower() && mstruct[0] == xvar) {
		mpow = mstruct[1];
		return true;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(get_power(mstruct[i], xvar, mpow)) return true;
	}
	return false;
}
const MathStructure *get_power_term(const MathStructure &mstruct, const MathStructure &xvar) {
	if(mstruct == xvar) {
		return &mstruct;
	}
	if(mstruct.isPower() && mstruct[0] == xvar) {
		return &mstruct;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		const MathStructure *mterm = get_power_term(mstruct[i], xvar);
		if(mterm) return mterm;
	}
	return NULL;
}

int test_comparisons(const MathStructure &msave, MathStructure &mthis, const MathStructure &x_var, const EvaluationOptions &eo, bool sub, int alt) {
	if(mthis.isComparison() && mthis[0] == x_var) {
		if(mthis.comparisonType() != COMPARISON_EQUALS && mthis.comparisonType() != COMPARISON_NOT_EQUALS) return 1;
		MathStructure mtest;
		EvaluationOptions eo2 = eo;
		eo2.calculate_functions = false;
		eo2.isolate_x = false;
		eo2.test_comparisons = true;
		eo2.warn_about_denominators_assumed_nonzero = false;
		eo2.assume_denominators_nonzero = false;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mtest = mthis;
		mtest.eval(eo2);
		if(mtest.isComparison()) {
			eo2.calculate_functions = true;
			mtest = msave;
			mtest.replace(x_var, mthis[1]);
			if(CALCULATOR->usesIntervalArithmetic()) {
				MathStructure mtest2(mtest[0]);
				if(!mtest[1].isZero()) {
					mtest2.subtract(mtest[1]);
				}
				CALCULATOR->beginTemporaryStopMessages();
				mtest2.eval(eo2);
				if(CALCULATOR->endTemporaryStopMessages() > 0) {
					if(!sub) mthis = msave;
					return -1;
				}
				if(mtest2.isNumber()) {
					if(mtest.comparisonType() == COMPARISON_LESS || mtest.comparisonType() == COMPARISON_EQUALS_LESS) {
						if(!mtest2.number().hasImaginaryPart() && mtest2.number().lowerEndPoint().isNonPositive()) return 1;
					} else if(mtest.comparisonType() == COMPARISON_GREATER || mtest.comparisonType() == COMPARISON_EQUALS_GREATER) {
						if(!mtest2.number().hasImaginaryPart() && mtest2.number().upperEndPoint().isNonNegative()) return 1;
					} else if(mtest.comparisonType() == COMPARISON_NOT_EQUALS) {
						if(!mtest2.number().isNonZero()) return 1;
						else if(mtest2.number().isInterval() || mtest2.number().isRational()) {mthis = m_one; return 0;}
					} else {
						if(!mtest2.number().isNonZero()) return 1;
						else if(mtest2.number().isInterval() || mtest2.number().isRational()) {mthis.clear(); return 0;}
					}
				} else if(mtest2.isUnit_exp()) {
					if(mtest.comparisonType() == COMPARISON_GREATER || mtest.comparisonType() == COMPARISON_EQUALS_GREATER) return 1;
					else if(mtest.comparisonType() == COMPARISON_EQUALS) {mthis.clear(); return 0;}
					else if(mtest.comparisonType() == COMPARISON_NOT_EQUALS) {mthis = m_one; return 0;}
				} else if(isUnit_multi(mtest2)) {
					if(mtest.comparisonType() == COMPARISON_LESS || mtest.comparisonType() == COMPARISON_EQUALS_LESS) {
						if(mtest2[0].isNumber() && !mtest2[0].number().hasImaginaryPart() && mtest2[0].number().lowerEndPoint().isNonPositive()) return 1;
					} else if(mtest.comparisonType() == COMPARISON_GREATER || mtest.comparisonType() == COMPARISON_EQUALS_GREATER) {
						if(!mtest2[0].isNumber() || (!mtest2[0].number().hasImaginaryPart() && mtest2.number().upperEndPoint().isNonNegative())) return 1;

					} else if(mtest.comparisonType() == COMPARISON_NOT_EQUALS) {
						if(!mtest2[0].isNumber()) {mthis = m_one; return 0;}
						if(!mtest2[0].number().isNonZero()) return 1;
						else if(mtest2[0].number().isInterval() || mtest2[0].number().isRational()) {mthis = m_one; return 0;}
					} else {
						if(!mtest2[0].isNumber()) {mthis.clear(); return 0;}
						if(!mtest2[0].number().isNonZero()) return 1;
						else if(mtest2[0].number().isInterval() || mtest2[0].number().isRational()) {mthis.clear(); return 0;}
					}
				}

			}
			CALCULATOR->beginTemporaryStopMessages();
			if(mtest[1].isZero() && mtest[0].isAddition() && mtest[0].size() > 1) {
				mtest[1].subtract(mtest[0][0]);
				mtest[0].delChild(1, true);
			}
			mtest.eval(eo2);
			if(CALCULATOR->endTemporaryStopMessages() > 0) {
				if(!sub) mthis = msave;
				return -1;
			}
			if(mtest.isNumber()) {
				if(mtest.number().getBoolean() == 1) {
					return 1;
				} else if(mtest.number().getBoolean() == 0) {
					if(mtest.isApproximate() && mtest.comparisonType() != COMPARISON_EQUALS && mtest.comparisonType() != COMPARISON_NOT_EQUALS) return -1;
					if(mtest.comparisonType() == COMPARISON_EQUALS) mthis.clear();
					else if(mtest.comparisonType() == COMPARISON_NOT_EQUALS) mthis = m_one;
					return 0;
				}
			}
		} else {
			mthis = mtest;
			if(mtest.isNumber()) {
				if(mtest.number().getBoolean() == 0) {
					return 0;
				}
			}
			return 1;
		}
		if(!sub) mthis = msave;
		return -1;
	}
	if(alt && mthis.isComparison()) {
		if(!sub && alt != 1) mthis = msave;
		return -1;
	}
	if(mthis.isLogicalOr() || mthis.isLogicalAnd()) {
		int i_ret = 1;
		for(size_t i = 0; i < mthis.size(); i++) {
			if(test_comparisons(msave, mthis[i], x_var, eo, true, alt) < 0) {
				if(alt != 1) {
					mthis = msave;
					return -1;
				}
				i_ret = -1;
			}
		}
		return i_ret;
	}
	if(sub) return 1;
	else return -1;
}

bool isx_deabsify(MathStructure &mstruct) {
	switch(mstruct.type()) {
		case STRUCT_FUNCTION: {
			if(mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1 && mstruct[0].representsNonComplex(true)) {
				mstruct.setToChild(1, true);
				return true;
			}
			break;
		}
		case STRUCT_POWER: {
			if(mstruct[1].isMinusOne()) {
				return isx_deabsify(mstruct[0]);
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(isx_deabsify(mstruct[i])) b = true;
			}
			return b;
		}
		default: {}
	}
	return false;
}

int newton_raphson(const MathStructure &mstruct, MathStructure &x_value, const MathStructure &x_var, const EvaluationOptions &eo) {
	if(mstruct == x_var) {
		x_value = m_zero;
		return 1;
	}

	if(CALCULATOR->aborted() || !mstruct.isAddition() || mstruct.size() > 200) return -1;
	if(mstruct.size() == 2) {
		if(mstruct[1] == x_var && mstruct[0].isNumber() && mstruct[0].number().isReal()) {
			x_value = mstruct[0];
			x_value.number().negate();
			return 1;
		}
		if(mstruct[0] == x_var && mstruct[1].isNumber() && mstruct[1].number().isReal()) {
			x_value = mstruct[1];
			x_value.number().negate();
			return 1;
		}
	}

	Number nr;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i] != x_var) {
			switch(mstruct[i].type()) {
				case STRUCT_NUMBER: {nr = mstruct[i].number(); break;}
				case STRUCT_MULTIPLICATION: {
					if(mstruct[i].size() == 2 && mstruct[i][0].isNumber() && (mstruct[i][1] == x_var || (mstruct[i][1].isPower() && mstruct[i][1][0] == x_var && mstruct[i][1][1].isNumber() && mstruct[i][1][1].number().isInteger() && mstruct[i][1][1].number().isPositive()))) {
						break;
					}
					return -1;
				}
				case STRUCT_POWER: {
					if(mstruct[i][0] == x_var && mstruct[i][1].isNumber() && mstruct[i][1].number().isInteger() && mstruct[i][1].number().isPositive()) {
						break;
					}
				}
				default: {return -1;}
			}
		}
	}

	MathStructure mdiff(mstruct);
	if(!mdiff.differentiate(x_var, eo)) return -1;
	MathStructure minit(mstruct);
	minit.divide(mdiff);
	minit.negate();
	minit.add(x_var);
	minit.eval(eo);

	Number nr_target_high(1, 1, -(PRECISION) - 10);
	Number nr_target_low(1, 1, 0);
	nr_target_low -= nr_target_high;
	nr_target_high++;
	MathStructure mguess(2, 1, 0);
	Number ndeg(mstruct.degree(x_var));
	bool overflow = false;
	int ideg = ndeg.intValue(&overflow);
	if(overflow || ideg > 100) return -1;
	nr.negate();
	if(!nr.isZero()) {
		bool b_neg = nr.isNegative();
		if(b_neg) nr.negate();
		if(nr.root(ndeg)) {
			if(ndeg.isOdd() && b_neg) nr.negate();
			mguess = nr;
		} else {
			nr = mguess.number();
		}
	} else {
		nr = mguess.number();
	}

	for(int i = 0; i < 100 + PRECISION + ideg * 2; i++) {

		mguess.number().setToFloatingPoint();
		if(mguess.number().hasImaginaryPart()) mguess.number().internalImaginary()->setToFloatingPoint();

		if(CALCULATOR->aborted()) return -1;

		MathStructure mtest(minit);
		mtest.replace(x_var, mguess);
		mtest.eval(eo);

		Number nrdiv(mguess.number());
		if(!mtest.isNumber() || !nrdiv.divide(mtest.number())) {
			return -1;
		}

		if(nrdiv.isLessThan(nr_target_high) && nrdiv.isGreaterThan(nr_target_low)) {
			if(CALCULATOR->usesIntervalArithmetic()) {
				if(!x_value.number().setInterval(mguess.number(), mtest.number())) return -1;
			} else {
				x_value = mtest;
				if(x_value.number().precision() < 0 || x_value.number().precision() > PRECISION + 10) x_value.number().setPrecision(PRECISION + 10);
			}
			x_value.numberUpdated();
			return 1;
		}
		mguess = mtest;
	}

	nr.negate();
	mguess = nr;
	for(int i = 0; i < 100 + PRECISION + ideg * 2; i++) {

		mguess.number().setToFloatingPoint();
		if(mguess.number().hasImaginaryPart()) mguess.number().internalImaginary()->setToFloatingPoint();

		if(CALCULATOR->aborted()) return -1;

		MathStructure mtest(minit);
		mtest.replace(x_var, mguess);
		mtest.eval(eo);

		Number nrdiv(mguess.number());
		if(!mtest.isNumber() || !nrdiv.divide(mtest.number())) {
			return -1;
		}
		if(nrdiv.isLessThan(nr_target_high) && nrdiv.isGreaterThan(nr_target_low)) {
			if(CALCULATOR->usesIntervalArithmetic()) {
				if(!x_value.number().setInterval(mguess.number(), mtest.number())) return -1;
			} else {
				x_value = mtest;
				if(x_value.number().precision() < 0 || x_value.number().precision() > PRECISION + 10) x_value.number().setPrecision(PRECISION + 10);
			}
			x_value.numberUpdated();
			return 1;
		}
		mguess = mtest;
	}

	return 0;

}

int find_interval_precision(const MathStructure &mstruct);

int find_interval_precision(const MathStructure &mstruct) {
	if(mstruct.isNumber()) {
		return mstruct.number().precision(1);
	}
	int iv_prec = -1;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(iv_prec > -1) {
		 	if(find_interval_precision(mstruct[i]) > -1) return 0;
		} else {
			iv_prec = find_interval_precision(mstruct[i]);
		}
	}
	return iv_prec;
}

MathStructure *find_abs_sgn(MathStructure &mstruct, const MathStructure &x_var);
MathStructure *find_abs_sgn(MathStructure &mstruct, const MathStructure &x_var) {
	switch(mstruct.type()) {
		case STRUCT_FUNCTION: {
			if(((mstruct.function()->id() == FUNCTION_ID_ABS && mstruct.size() == 1) || (mstruct.function()->id() == FUNCTION_ID_SIGNUM && mstruct.size() == 2)) && mstruct[0].contains(x_var, false) && mstruct[0].representsNonComplex()) {
				return &mstruct;
			}
			break;
		}
		case STRUCT_POWER: {
			return find_abs_sgn(mstruct[0], x_var);
		}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				MathStructure *m = find_abs_sgn(mstruct[i], x_var);
				if(m) return m;
			}
			break;
		}
		default: {break;}
	}
	return NULL;
}

bool is_units_with_multiplier(const MathStructure &mstruct) {
	if(!mstruct.isMultiplication() || mstruct.size() == 0 || !mstruct[0].isNumber()) return false;
	for(size_t i = 1; i < mstruct.size(); i++) {
		if(!mstruct[i].isUnit_exp()) return false;
	}
	return true;
}

bool fix_n_multiple(MathStructure &mstruct, const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_var) {
	bool b_ret = false;
	if(mstruct.isComparison()) {
		if(mstruct.comparisonType() == COMPARISON_EQUALS && x_var.isVariable() && !x_var.variable()->isKnown() && !((UnknownVariable*) x_var.variable())->interval().isUndefined() && mstruct[1].contains(CALCULATOR->getVariableById(VARIABLE_ID_N))) {
			MathStructure mtest(mstruct);
			mtest.replace(x_var, ((UnknownVariable*) x_var.variable())->interval());
			EvaluationOptions eo2 = eo;
			EvaluationOptions feo2 = feo;
			if(eo.approximation == APPROXIMATION_EXACT) {
				eo2.approximation = APPROXIMATION_TRY_EXACT;
				feo2.approximation = APPROXIMATION_TRY_EXACT;
			}
			CALCULATOR->beginTemporaryEnableIntervalArithmetic();
			if(CALCULATOR->usesIntervalArithmetic()) {
				CALCULATOR->beginTemporaryStopMessages();
				mtest.calculateFunctions(feo2);
				if(mtest.isolate_x(eo2, feo2, CALCULATOR->getVariableById(VARIABLE_ID_N))) {
					if(CALCULATOR->endTemporaryStopMessages() == 0) {
						if(mtest.isZero()) {
							mstruct.clear(true);
							b_ret = true;
						} else if(mtest.isComparison() && mtest.comparisonType() == COMPARISON_EQUALS && mtest[0].isVariable() && mtest[0].variable() == CALCULATOR->getVariableById(VARIABLE_ID_N) && mtest[1].isNumber()) {
							if(mtest[1].number().isInteger()) {
								mstruct.calculateReplace(CALCULATOR->getVariableById(VARIABLE_ID_N), mtest[1], eo);
								b_ret = true;
							} else if(mtest[1].number().isInterval()) {
								Number nr_int; bool b_multiple = false;
								if(mtest[1].number().getCentralInteger(nr_int, &b_multiple)) {
									mstruct.calculateReplace(CALCULATOR->getVariableById(VARIABLE_ID_N), nr_int, eo);
									b_ret = true;
								} else if(!b_multiple) {
									mstruct.clear(true);
									b_ret = true;
								}
							} else {
								mstruct.clear(true);
								b_ret = true;
							}
						}
					}
				} else {
					CALCULATOR->endTemporaryStopMessages();
				}
			}
			CALCULATOR->endTemporaryEnableIntervalArithmetic();
		}
	} else {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(fix_n_multiple(mstruct[i], eo, feo, x_var)) {
				mstruct.childUpdated(i + 1);
				b_ret = true;
			}
		}
		if(b_ret) mstruct.calculatesub(eo, feo, false);
	}
	return b_ret;
}

bool MathStructure::isolate_x_sub(const EvaluationOptions &eo, EvaluationOptions &eo2, const MathStructure &x_var, MathStructure *morig) {
	if(!isComparison()) {
		cout << "isolate_x_sub: " << *this << " is not a comparison." << endl;
		return false;
	}

	if(CHILD(0) == x_var) return false;
	if(contains(x_var, true) <= 0) return false;

	if(CALCULATOR->aborted()) return false;

	switch(CHILD(0).type()) {
		case STRUCT_ADDITION: {
			bool b = false;
			// x+y=z => x=z-y
			for(size_t i = 0; i < CHILD(0).size();) {
				if(!CHILD(0)[i].contains(x_var)) {
					CHILD(0)[i].calculateNegate(eo2);
					CHILD(0)[i].ref();
					CHILD(1).add_nocopy(&CHILD(0)[i], true);
					CHILD(1).calculateAddLast(eo2);
					CHILD(0).delChild(i + 1);
					b = true;
				} else {
					i++;
				}
			}
			if(b) {
				CHILD_UPDATED(0);
				CHILD_UPDATED(1);
				if(CHILD(0).size() == 1) {
					CHILD(0).setToChild(1, true);
				} else if(CHILD(0).size() == 0) {
					CHILD(0).clear(true);
				}
				isolate_x_sub(eo, eo2, x_var, morig);
				return true;
			}
			if(CALCULATOR->aborted()) return false;

			// ax^(2n)+bx^n=c
			if(CHILD(0).size() >= 2 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
				bool sqpow = false, nopow = false;
				MathStructure mstruct_a, mstruct_b, mpow_a, mpow_b;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i] == x_var || (CHILD(0)[i].isPower() && CHILD(0)[i][0] == x_var)) {
						b = false;
						const MathStructure *mexp = &m_one;
						if(CHILD(0)[i] != x_var) mexp = &CHILD(0)[i][1];
						if(nopow) {
							if(*mexp == mpow_b) {
								mstruct_b.add(m_one, true);
								b = true;
							} else if(!sqpow) {
								MathStructure mtest(*mexp);
								mtest.calculateMultiply(nr_half, eo);
								if(mtest == mpow_b) {
									mpow_a = *mexp;
									mstruct_a.set(m_one, true);
									b = true;
								} else {
									mtest = *mexp;
									mtest.calculateMultiply(nr_two, eo);
									if(mtest == mpow_b) {
										mpow_a = mpow_b;
										mpow_b = *mexp;
										mstruct_a = mstruct_b;
										mstruct_b.set(m_one, true);
										b = true;
									}
								}
								if(b) sqpow = true;
							} else if(*mexp == mpow_a) {
								mstruct_a.add(m_one, true);
								b = true;
							}
						} else if(sqpow) {
							if(*mexp == mpow_a) {
								mstruct_a.add(m_one, true);
								b = true;
							} else {
								MathStructure mtest(*mexp);
								mtest.calculateMultiply(nr_two, eo);
								if(mtest == mpow_a) {
									mpow_b = *mexp;
									mstruct_b.set(m_one, true);
									b = true;
								} else {
									mtest = *mexp;
									mtest.calculateMultiply(nr_half, eo);
									if(mtest == mpow_a) {
										mpow_b = mpow_a;
										mpow_a = *mexp;
										mstruct_b = mstruct_a;
										mstruct_a.set(m_one, true);
										b = true;
									}
								}
								if(b) nopow = true;
							}
						} else {
							if(mexp->isOne()) {
								mpow_b = *mexp;
								nopow = true;
								mstruct_b.set(m_one);
							} else {
								mpow_a = *mexp;
								sqpow = true;
								mstruct_a.set(m_one);
							}
							b = true;
						}
						if(!b) {
							sqpow = false;
							nopow = false;
							break;
						}
					} else if(CHILD(0)[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
							if(CHILD(0)[i][i2] == x_var || (CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][0] == x_var)) {
								b = false;
								const MathStructure *mexp = &m_one;
								if(CHILD(0)[i][i2] != x_var) mexp = &CHILD(0)[i][i2][1];
								if(nopow) {
									if(*mexp == mpow_b) {
										MathStructure *madd = new MathStructure(CHILD(0)[i]);
										madd->delChild(i2 + 1, true);
										mstruct_b.add_nocopy(madd, true);
										b = true;
									} else if(!sqpow) {
										MathStructure mtest(*mexp);
										mtest.calculateMultiply(nr_half, eo);
										if(mtest == mpow_b) {
											mpow_a = *mexp;
											mstruct_a = CHILD(0)[i];
											mstruct_a.delChild(i2 + 1, true);
											b = true;
										} else {
											mtest = *mexp;
											mtest.calculateMultiply(nr_two, eo);
											if(mtest == mpow_b) {
												mpow_a = mpow_b;
												mpow_b = *mexp;
												mstruct_a = mstruct_b;
												mstruct_b = CHILD(0)[i];
												mstruct_b.delChild(i2 + 1, true);
												b = true;
											}
										}
										if(b) sqpow = true;
									} else if(*mexp == mpow_a) {
										MathStructure *madd = new MathStructure(CHILD(0)[i]);
										madd->delChild(i2 + 1, true);
										mstruct_a.add_nocopy(madd, true);
										b = true;
									}
								} else if(sqpow) {
									if(*mexp == mpow_a) {
										MathStructure *madd = new MathStructure(CHILD(0)[i]);
										madd->delChild(i2 + 1, true);
										mstruct_a.add_nocopy(madd, true);
										b = true;
									} else {
										MathStructure mtest(*mexp);
										mtest.calculateMultiply(nr_two, eo);
										if(mtest == mpow_a) {
											mpow_b = *mexp;
											mstruct_b.add(m_one, true);
											mstruct_b = CHILD(0)[i];
											mstruct_b.delChild(i2 + 1, true);
											b = true;
										} else {
											mtest = *mexp;
											mtest.calculateMultiply(nr_half, eo);
											if(mtest == mpow_a) {
												mpow_b = mpow_a;
												mpow_a = *mexp;
												mstruct_b = mstruct_a;
												mstruct_a = CHILD(0)[i];
												mstruct_a.delChild(i2 + 1, true);
												b = true;
											}
										}
										if(b) nopow = true;
									}
								} else {
									if(mexp->isOne()) {
										mpow_b = *mexp;
										nopow = true;
										mstruct_b = CHILD(0)[i];
										mstruct_b.delChild(i2 + 1, true);
									} else {
										mpow_a = *mexp;
										sqpow = true;
										mstruct_a = CHILD(0)[i];
										mstruct_a.delChild(i2 + 1, true);
									}
									b = true;
								}
								if(!b) {
									sqpow = false;
									nopow = false;
									break;
								}
							} else if(CHILD(0)[i][i2].contains(x_var)) {
								sqpow = false;
								nopow = false;
								break;
							}
						}
						if(!sqpow && !nopow) break;
					} else {
						sqpow = false;
						nopow = false;
						break;
					}
				}
				b = false;
				if(sqpow && nopow && !mstruct_a.representsZero(true)) {
					b = mstruct_a.representsNonZero(true);
					if(!b && eo2.approximation == APPROXIMATION_EXACT) {
						MathStructure mtest(mstruct_a);
						mtest.add(m_zero, OPERATION_NOT_EQUALS);
						EvaluationOptions eo3 = eo2;
						eo3.test_comparisons = true;
						mtest.calculatesub(eo3, eo, false);
						b = mtest.isOne();
					}
				}

				if(b) {
					int a_iv = find_interval_precision(mstruct_a);
					int b_iv = find_interval_precision(mstruct_b);
					int c_iv = find_interval_precision(CHILD(1));
					if(a_iv >= 0 && (c_iv < 0 || c_iv > a_iv) && CHILD(1).representsNonZero()) {
						//x=(-2c)/(b+/-sqrt(b^2-4ac))
						MathStructure mbak(*this);
						bool stop_iv = false;
						if(c_iv >= 0 && c_iv <= PRECISION) {
							stop_iv = true;
						} else if(b_iv >= 0) {
							MathStructure mstruct_bl;
							MathStructure mstruct_bu;
							stop_iv = true;
							if(mstruct_b.isNumber() && mstruct_b.number().isNonZero() && !mstruct_b.number().hasImaginaryPart()) {
								mstruct_bl = mstruct_b.number().lowerEndPoint();
								mstruct_bu = mstruct_b.number().upperEndPoint();
								stop_iv = false;
							} else if(is_units_with_multiplier(mstruct_b) && mstruct_b[0].number().isNonZero() && !mstruct_b[0].number().hasImaginaryPart()) {
								mstruct_bl = mstruct_b;
								mstruct_bl[0].number() = mstruct_b[0].number().lowerEndPoint();
								mstruct_bu = mstruct_b;
								mstruct_bu[0].number() = mstruct_b[0].number().upperEndPoint();
								stop_iv = false;
							}
							if(!stop_iv) {
								// Lower b+sqrt(b^2-4ac)
								MathStructure b2l(mstruct_bl);
								b2l.calculateRaise(nr_two, eo2);
								MathStructure ac(4, 1);
								ac.calculateMultiply(mstruct_a, eo2);
								ac.calculateMultiply(CHILD(1), eo2);
								b2l.calculateAdd(ac, eo2);

								b2l.calculateRaise(nr_half, eo2);
								MathStructure mstruct_1l(mstruct_bl);
								mstruct_1l.calculateAdd(b2l, eo2);

								// Upper -b+sqrt(b^2-4ac)
								MathStructure b2u(mstruct_bu);
								b2u.calculateRaise(nr_two, eo2);
								b2u.calculateAdd(ac, eo2);

								b2u.calculateRaise(nr_half, eo2);
								MathStructure mstruct_1u(mstruct_bu);
								mstruct_1u.calculateAdd(b2u, eo2);

								MathStructure mstruct_1(mstruct_1l);
								mstruct_1.transform(STRUCT_FUNCTION, mstruct_1u);
								mstruct_1.setFunctionId(FUNCTION_ID_INTERVAL);
								mstruct_1.calculateFunctions(eo, false);

								// Lower -b-sqrt(b^2-4ac)
								MathStructure mstruct_2l(mstruct_bl);
								mstruct_2l.calculateSubtract(b2l, eo2);

								// Upper -b-sqrt(b^2-4ac)
								MathStructure mstruct_2u(mstruct_bu);
								mstruct_2u.calculateSubtract(b2u, eo2);

								MathStructure mstruct_2(mstruct_2l);
								mstruct_2.transform(STRUCT_FUNCTION, mstruct_2u);
								mstruct_2.setFunctionId(FUNCTION_ID_INTERVAL);
								mstruct_2.calculateFunctions(eo, false);

								MathStructure mstruct_c(CHILD(1));
								mstruct_c.calculateMultiply(nr_two, eo2);

								mstruct_1.calculateInverse(eo2);
								mstruct_2.calculateInverse(eo2);
								mstruct_1.calculateMultiply(mstruct_c, eo2);
								mstruct_2.calculateMultiply(mstruct_c, eo2);

								CHILD(0) = x_var;
								if(!mpow_b.isOne()) {CHILD(0) ^= mpow_b; CHILD_UPDATED(0);}
								if(mstruct_1 == mstruct_2) {
									CHILD(1) = mstruct_1;
									isolate_x_sub(eo, eo2, x_var, morig);
								} else {
									CHILD(1) = mstruct_1;
									MathStructure *mchild2 = new MathStructure(CHILD(0));
									isolate_x_sub(eo, eo2, x_var, morig);
									mchild2->transform(STRUCT_COMPARISON, mstruct_2);
									mchild2->setComparisonType(ct_comp);
									mchild2->isolate_x_sub(eo, eo2, x_var, morig);
									if(ct_comp == COMPARISON_NOT_EQUALS) {
										transform_nocopy(STRUCT_LOGICAL_AND, mchild2);
									} else {
										transform_nocopy(STRUCT_LOGICAL_OR, mchild2);
									}
									calculatesub(eo2, eo, false);
								}
								CHILDREN_UPDATED
								return true;
							}
						}
						EvaluationOptions eo3 = eo2;
						if(b && stop_iv) {
							CALCULATOR->beginTemporaryStopIntervalArithmetic();
							eo3.interval_calculation = INTERVAL_CALCULATION_NONE;
							bool failed = false;
							fix_intervals(*this, eo3, &failed);
							if(failed) {
								set(mbak);
								CALCULATOR->endTemporaryStopIntervalArithmetic();
								b = false;
							}
						}
						if(b) {
							MathStructure b2(mstruct_b);
							b2.calculateRaise(nr_two, eo3);
							MathStructure ac(4, 1);
							ac.calculateMultiply(mstruct_a, eo3);
							ac.calculateMultiply(CHILD(1), eo3);
							b2.calculateAdd(ac, eo3);

							b2.calculateRaise(nr_half, eo3);
							MathStructure mstruct_1(mstruct_b);
							mstruct_1.calculateAdd(b2, eo3);
							MathStructure mstruct_2(mstruct_b);
							mstruct_2.calculateSubtract(b2, eo3);

							MathStructure mstruct_c(CHILD(1));
							mstruct_c.calculateMultiply(nr_two, eo3);

							mstruct_1.calculateInverse(eo3);
							mstruct_2.calculateInverse(eo3);
							mstruct_1.calculateMultiply(mstruct_c, eo3);
							mstruct_2.calculateMultiply(mstruct_c, eo3);

							CHILD(0) = x_var;
							if(!mpow_b.isOne()) {CHILD(0) ^= mpow_b; CHILD_UPDATED(0);}
							if(mstruct_1 == mstruct_2) {
								CHILD(1) = mstruct_1;
								isolate_x_sub(eo, eo3, x_var, morig);
							} else {
								CHILD(1) = mstruct_1;
								MathStructure *mchild2 = new MathStructure(CHILD(0));
								isolate_x_sub(eo, eo3, x_var, morig);
								mchild2->transform(STRUCT_COMPARISON, mstruct_2);
								mchild2->setComparisonType(ct_comp);
								mchild2->isolate_x_sub(eo, eo3, x_var, morig);
								if(ct_comp == COMPARISON_NOT_EQUALS) {
									transform_nocopy(STRUCT_LOGICAL_AND, mchild2);
								} else {
									transform_nocopy(STRUCT_LOGICAL_OR, mchild2);
								}
								calculatesub(eo3, eo, false);
							}
							CHILDREN_UPDATED;
							if(stop_iv) {
								CALCULATOR->endTemporaryStopIntervalArithmetic();
								CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
								fix_intervals(*this, eo2);
							}
							return true;
						}
					} else if(CHILD(1).isZero()) {
						// x=0 || x=-a/b || (a=0 && b=0)
						ComparisonType ct = ct_comp;
						CHILD(0) = x_var;
						if(!mpow_b.isOne()) {CHILD(0) ^= mpow_b; CHILD_UPDATED(0);}
						MathStructure *mchild2 = new MathStructure(CHILD(0));
						CHILD(1) = mstruct_b;
						CHILD(1).calculateDivide(mstruct_a, eo2);
						CHILD(1).calculateNegate(eo2);
						isolate_x_sub(eo, eo2, x_var, morig);
						if(!mstruct_a.representsNonZero()) {
							MathStructure *mtest = new MathStructure(mstruct_a);
							mtest->transform(ct == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, m_zero);
							mtest->calculatesub(eo2, eo, false);
							transform_nocopy(ct == COMPARISON_NOT_EQUALS ? STRUCT_LOGICAL_OR : STRUCT_LOGICAL_AND, mtest);
							calculatesub(eo2, eo, false);
						}
						mchild2->transform(ct, m_zero);
						mchild2->isolate_x_sub(eo, eo2, x_var, morig);
						transform_nocopy(ct == COMPARISON_NOT_EQUALS ? STRUCT_LOGICAL_AND : STRUCT_LOGICAL_OR, mchild2);
						if(!mstruct_b.representsNonZero() && !mstruct_a.representsNonZero()) {
							MathStructure *mchild3a = new MathStructure(mstruct_a);
							mchild3a->transform(ct, m_zero);
							MathStructure *mchild3b = new MathStructure(mstruct_b);
							mchild3b->transform(ct, m_zero);
							mchild3a->transform_nocopy(ct == COMPARISON_NOT_EQUALS ? STRUCT_LOGICAL_OR : STRUCT_LOGICAL_AND, mchild3b);
							mchild3a->calculatesub(eo2, eo, false);
							add_nocopy(mchild3a, ct == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
						}
						calculatesub(eo2, eo, false);
						return true;
					} else {
						// x=(-b+/-sqrt(b^2-4ac))/(2a)
						MathStructure mbak(*this);
						bool stop_iv = false;
						if(a_iv >= 0 && a_iv <= PRECISION) {
							stop_iv = true;
						} else if(b_iv >= 0) {
							MathStructure mstruct_bl;
							MathStructure mstruct_bu;
							stop_iv = true;
							if(mstruct_b.isNumber() && mstruct_b.number().isNonZero() && !mstruct_b.number().hasImaginaryPart()) {
								mstruct_bl = mstruct_b.number().lowerEndPoint();
								mstruct_bu = mstruct_b.number().upperEndPoint();
								stop_iv = false;
							} else if(is_units_with_multiplier(mstruct_b) && mstruct_b[0].number().isNonZero() && !mstruct_b[0].number().hasImaginaryPart()) {
								mstruct_bl = mstruct_b;
								mstruct_bl[0].number() = mstruct_b[0].number().lowerEndPoint();
								mstruct_bu = mstruct_b;
								mstruct_bu[0].number() = mstruct_b[0].number().upperEndPoint();
								stop_iv = false;
							}
							if(!stop_iv) {
								// Lower -b+sqrt(b^2-4ac)
								MathStructure b2l(mstruct_bl);
								b2l.calculateRaise(nr_two, eo2);
								MathStructure ac(4, 1);
								ac.calculateMultiply(mstruct_a, eo2);
								ac.calculateMultiply(CHILD(1), eo2);
								b2l.calculateAdd(ac, eo2);

								b2l.calculateRaise(nr_half, eo2);
								mstruct_bl.calculateNegate(eo2);
								MathStructure mstruct_1l(mstruct_bl);
								mstruct_1l.calculateAdd(b2l, eo2);

								// Upper -b+sqrt(b^2-4ac)
								MathStructure b2u(mstruct_bu);
								b2u.calculateRaise(nr_two, eo2);
								b2u.calculateAdd(ac, eo2);

								b2u.calculateRaise(nr_half, eo2);
								mstruct_bu.calculateNegate(eo2);
								MathStructure mstruct_1u(mstruct_bu);
								mstruct_1u.calculateAdd(b2u, eo2);

								MathStructure mstruct_1(mstruct_1l);
								mstruct_1.transform(STRUCT_FUNCTION, mstruct_1u);
								mstruct_1.setFunctionId(FUNCTION_ID_INTERVAL);
								mstruct_1.calculateFunctions(eo, false);

								// Lower -b-sqrt(b^2-4ac)
								MathStructure mstruct_2l(mstruct_bl);
								mstruct_2l.calculateSubtract(b2l, eo2);

								// Upper -b-sqrt(b^2-4ac)
								MathStructure mstruct_2u(mstruct_bu);
								mstruct_2u.calculateSubtract(b2u, eo2);

								MathStructure mstruct_2(mstruct_2l);
								mstruct_2.transform(STRUCT_FUNCTION, mstruct_2u);
								mstruct_2.setFunctionId(FUNCTION_ID_INTERVAL);
								mstruct_2.calculateFunctions(eo, false);

								mstruct_a.calculateMultiply(nr_two, eo2);
								mstruct_a.calculateInverse(eo2);
								mstruct_1.calculateMultiply(mstruct_a, eo2);
								mstruct_2.calculateMultiply(mstruct_a, eo2);
								CHILD(0) = x_var;
								if(!mpow_b.isOne()) {CHILD(0) ^= mpow_b; CHILD_UPDATED(0);}
								if(mstruct_1 == mstruct_2) {
									CHILD(1) = mstruct_1;
									isolate_x_sub(eo, eo2, x_var, morig);
								} else {
									CHILD(1) = mstruct_1;
									MathStructure *mchild2 = new MathStructure(CHILD(0));
									isolate_x_sub(eo, eo2, x_var, morig);
									mchild2->transform(STRUCT_COMPARISON, mstruct_2);
									mchild2->setComparisonType(ct_comp);
									mchild2->isolate_x_sub(eo, eo2, x_var, morig);
									if(ct_comp == COMPARISON_NOT_EQUALS) {
										transform_nocopy(STRUCT_LOGICAL_AND, mchild2);
									} else {
										transform_nocopy(STRUCT_LOGICAL_OR, mchild2);
									}
									calculatesub(eo2, eo, false);
								}
								CHILDREN_UPDATED
								return true;
							}
						}
						EvaluationOptions eo3 = eo2;
						if(b && stop_iv) {
							CALCULATOR->beginTemporaryStopIntervalArithmetic();
							eo3.interval_calculation = INTERVAL_CALCULATION_NONE;
							bool failed = false;
							fix_intervals(*this, eo3, &failed);
							if(failed) {
								set(mbak);
								CALCULATOR->endTemporaryStopIntervalArithmetic();
								b = false;
							}
						}
						if(b) {
							MathStructure b2(mstruct_b);
							b2.calculateRaise(nr_two, eo3);
							MathStructure ac(4, 1);
							ac.calculateMultiply(mstruct_a, eo3);
							ac.calculateMultiply(CHILD(1), eo3);
							b2.calculateAdd(ac, eo3);
							b2.calculateRaise(nr_half, eo3);
							mstruct_b.calculateNegate(eo3);
							MathStructure mstruct_1(mstruct_b);
							mstruct_1.calculateAdd(b2, eo3);
							MathStructure mstruct_2(mstruct_b);
							mstruct_2.calculateSubtract(b2, eo3);
							mstruct_a.calculateMultiply(nr_two, eo3);
							mstruct_a.calculateInverse(eo3);
							mstruct_1.calculateMultiply(mstruct_a, eo3);
							mstruct_2.calculateMultiply(mstruct_a, eo3);
							CHILD(0) = x_var;
							if(!mpow_b.isOne()) {CHILD(0) ^= mpow_b; CHILD_UPDATED(0);}
							if(mstruct_1 == mstruct_2) {
								CHILD(1) = mstruct_1;
								isolate_x_sub(eo, eo3, x_var, morig);
							} else {
								CHILD(1) = mstruct_1;
								MathStructure *mchild2 = new MathStructure(CHILD(0));
								isolate_x_sub(eo, eo3, x_var, morig);
								mchild2->transform(STRUCT_COMPARISON, mstruct_2);
								mchild2->setComparisonType(ct_comp);
								mchild2->isolate_x_sub(eo, eo3, x_var, morig);
								if(ct_comp == COMPARISON_NOT_EQUALS) {
									transform_nocopy(STRUCT_LOGICAL_AND, mchild2);
								} else {
									transform_nocopy(STRUCT_LOGICAL_OR, mchild2);
								}
								calculatesub(eo3, eo, false);
							}
							CHILDREN_UPDATED;
							if(stop_iv) {
								CALCULATOR->endTemporaryStopIntervalArithmetic();
								CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
								fix_intervals(*this, eo2);
							}
							return true;
						}
					}
				}
			}

			if(CALCULATOR->aborted()) return false;

			// a*b^(dx)+cx=0 => -lambertw(a*d*log(b)/c)/(d*log(b))
			// a*b^(dx)+cx=y => (y*d*log(b)-c*lambertw(a*b^(y*d/c)*log(b)/c))/(c*d*log(b))
			if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(0).size() > 1) {
				combine_powers(*this, x_var, eo2, eo);
				size_t i_px = 0, i_mpx = 0;
				MathStructure mbak(*this);
				MathStructure *mvar = NULL, *m_b_p;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
							if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][1].contains(x_var) && !CHILD(0)[i][i2][0].contains(x_var)) {
								mvar = &CHILD(0)[i][i2][1];
								m_b_p = &CHILD(0)[i][i2][0];
								i_px = i;
								i_mpx = i2;
								break;
							}
						}
						if(mvar) break;
					} else if(CHILD(0)[i].isPower() && CHILD(0)[i][1].contains(x_var) && !CHILD(0)[i][0].contains(x_var)) {
						mvar = &CHILD(0)[i][1];
						m_b_p = &CHILD(0)[i][0];
						i_px = i;
						break;
					}
				}
				if(mvar) {
					MathStructure m_b(*m_b_p);
					if((mvar->isAddition() || mvar->isMultiplication()) && m_b.representsPositive()) {
						MathStructure *mvar2 = NULL;
						if(mvar->isMultiplication()) {
							for(size_t i = 0; i < mvar->size(); i++) {
								if((*mvar)[i].contains(x_var)) {mvar2 = &(*mvar)[i]; break;}
							}
						} else if(mvar->isAddition()) {
							for(size_t i = 0; i < mvar->size(); i++) {
								if((*mvar)[i].contains(x_var)) {
									mvar2 = &(*mvar)[i];
									if(mvar2->isMultiplication()) {
										for(size_t i2 = 0; i2 < mvar2->size(); i2++) {
											if((*mvar2)[i2].contains(x_var)) {mvar2 = &(*mvar2)[i2]; break;}
										}
									}
									break;
								}
							}
						}
						if(mvar2) {
							MathStructure m_b_exp;
							if(get_multiplier(*mvar, *mvar2, m_b_exp) && m_b.representsReal() && !m_b.contains(x_var)) {
								m_b ^= m_b_exp;
								mvar = mvar2;
							} else {
								mvar = NULL;
							}
						}
					}
					MathStructure m_c;
					if(mvar && get_multiplier(CHILD(0), *mvar, m_c, i_px) && m_c.representsNonZero() && m_c.contains(x_var) == 0 && m_b.representsNonZero()) {
						MathStructure mlogb(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &m_b, NULL);
						if(mlogb.calculateFunctions(eo)) mlogb.calculatesub(eo2, eo, true);
						MathStructure *marg = NULL;
						if(mlogb.representsNonZero()) {
							marg = new MathStructure(mlogb);
							if(CHILD(0)[i_px].isMultiplication()) {
								marg->multiply(CHILD(0)[i_px]);
								marg->last().delChild(i_mpx + 1, true);
								if(!marg->last().representsNonZero()) {marg->unref(); marg = NULL;}
								else marg->calculateMultiplyLast(eo2);
							}
						}
						if(marg) {
							if(!CHILD(1).isZero()) {
								marg->multiply(m_b);
								marg->last().raise(CHILD(1));
								if(!m_c.isOne()) marg->last().last().calculateDivide(m_c, eo2);
								marg->last().calculateRaiseExponent(eo2);
								marg->calculateMultiplyLast(eo2);
							}
							if(!m_c.isOne()) marg->calculateDivide(m_c, eo2);
						}
						if(marg && mvar->representsNonComplex() && m_b.representsPositive()) {
							if(marg->representsComplex()) {
								marg->unref();
								if(ct_comp == COMPARISON_EQUALS) clear(true);
								else set(1, 1, 0, true);
								return true;
							}
							MathStructure *mreq1 = NULL;
							MathStructure *marg2 = NULL;
							MathStructure *mreq2 = NULL;
							if(!marg->representsNonNegative()) {
								mreq1 = new MathStructure(*marg);
								mreq2 = new MathStructure(*marg);
								marg2 = new MathStructure(*marg);
								marg2->transformById(FUNCTION_ID_LAMBERT_W);
								marg2->addChild(m_minus_one);
								if(marg2->calculateFunctions(eo)) marg2->calculatesub(eo2, eo, true);
								marg2->calculateNegate(eo2);
								if(CHILD(1).isZero()) {
									marg2->calculateDivide(mlogb, eo2);
								} else {
									if(!m_c.isOne()) marg2->calculateMultiply(m_c, eo2);
									marg2->add(CHILD(1));
									marg2->last().calculateMultiply(mlogb, eo);
									marg2->calculateAddLast(eo2);
									marg2->calculateDivide(mlogb, eo2);
									if(!m_c.isOne()) marg2->calculateDivide(m_c, eo2);
								}
								mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_E));
								mreq1->last().calculateRaise(m_minus_one, eo2);
								mreq1->last().calculateNegate(eo2);
								mreq1->childUpdated(2);
								mreq1->isolate_x(eo2, eo);
								mreq2->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
								mreq2->isolate_x(eo2, eo);
								marg2->transform(ct_comp, *mvar);
								marg2->swapChildren(1, 2);
								marg2->isolate_x_sub(eo, eo2, x_var, morig);
								marg2->add(*mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								marg2->add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
								marg2->calculatesub(eo2, eo, false);
							}
							marg->transformById(FUNCTION_ID_LAMBERT_W);
							marg->addChild(m_zero);
							if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
							marg->calculateNegate(eo2);
							if(CHILD(1).isZero()) {
								marg->calculateDivide(mlogb, eo2);
							} else {
								if(!m_c.isOne()) marg->calculateMultiply(m_c, eo2);
								marg->add(CHILD(1));
								marg->last().calculateMultiply(mlogb, eo);
								marg->calculateAddLast(eo2);
								marg->calculateDivide(mlogb, eo2);
								if(!m_c.isOne()) marg->calculateDivide(m_c, eo2);
							}
							setChild_nocopy(marg, 2, true);
							mvar->ref();
							CHILD(0).clear(true);
							setChild_nocopy(mvar, 1, true);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							if(mreq1) {
								add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								calculatesub(eo2, eo, false);
							}
							if(marg2) {
								add_nocopy(marg2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								calculatesub(eo2, eo, false);
							}
							return true;
						} else if(marg) {
							marg->transformById(FUNCTION_ID_LAMBERT_W);
							marg->addChild(CALCULATOR->getVariableById(VARIABLE_ID_N));
							if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
							marg->calculateNegate(eo2);
							if(CHILD(1).isZero()) {
								marg->calculateDivide(mlogb, eo2);
							} else {
								if(!m_c.isOne()) marg->calculateMultiply(m_c, eo2);
								marg->add(CHILD(1));
								marg->last().calculateMultiply(mlogb, eo);
								marg->calculateAddLast(eo2);
								marg->calculateDivide(mlogb, eo2);
								if(!m_c.isOne()) marg->calculateDivide(m_c, eo2);
							}
							setChild_nocopy(marg, 2, true);
							mvar->ref();
							CHILD(0).clear(true);
							setChild_nocopy(mvar, 1, true);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						}
					}
				}
				if(i_px == 0) {
					// a^(2x)+a^x, a^(3x)+a^x, a^(3x)+a^(2x), a^(-x)+a^x
					MathStructure *mvar1 = NULL, *mvar2 = NULL;
					b = true;
					for(size_t i = 0; i < CHILD(0).size() && b; i++) {
						b = false;
						if(CHILD(0)[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
								if(!b && CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][1].contains(x_var) && !CHILD(0)[i][i2][0].contains(x_var)) {
									if(mvar2) {
										if(mvar1->equals(CHILD(0)[i][i2]) || mvar2->equals(CHILD(0)[i][i2])) {
											b = true;
										}
									} else if(mvar1) {
										if(!mvar1->equals(CHILD(0)[i][i2])) {
											mvar2 = &CHILD(0)[i][i2];
										}
										b = true;
									} else {
										mvar1 =  &CHILD(0)[i][i2];
										b = true;
									}
									if(!b) break;
								} else if(CHILD(0)[i][i2].contains(x_var)) {
									b = false;
									break;
								}
							}
						} else if(CHILD(0)[i].isPower() && CHILD(0)[i][1].contains(x_var) && !CHILD(0)[i][0].contains(x_var)) {
							if(mvar2) {
								if(mvar1->equals(CHILD(0)[i]) || mvar2->equals(CHILD(0)[i])) {
									b = true;
								}
							} else if(mvar1) {
								if(!mvar1->equals(CHILD(0)[i])) {
									mvar2 = &CHILD(0)[i];
								}
								b = true;
							} else {
								mvar1 =  &CHILD(0)[i];
								b = true;
							}
						}
					}
					if(b && mvar2 && mvar1->base()->representsPositive()) {
						bool b_two = false, b_three = false, b_m_one = false;
						if(mvar1->base()->equals(*mvar2->base())) {
							MathStructure m1m2((*mvar1)[1]);
							m1m2.calculateMultiply(nr_two, eo2);
							if(m1m2.equals((*mvar2)[1])) {
								b_two = true;
							}
							if(!b_two && !b_m_one) {
								MathStructure m1mm1((*mvar1)[1]);
								m1mm1.calculateMultiply(nr_minus_one, eo2);
								if(m1mm1.equals((*mvar2)[1])) {
									b_m_one = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								}
							}
							if(!b_two && !b_m_one) {
								MathStructure m2m2((*mvar2)[1]);
								m2m2.calculateMultiply(nr_two, eo2);
								if(m2m2.equals((*mvar1)[1])) {
									b_two = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								}
							}
							if(!b_two && !b_m_one) {
								MathStructure m2mm1((*mvar2)[1]);
								m2mm1.calculateMultiply(nr_minus_one, eo2);
								if(m2mm1.equals((*mvar1)[1])) {
									b_m_one = true;
								}
							}
							if(!b_two && !b_m_one && !b_three) {
								MathStructure m1m3((*mvar1)[1]);
								m1m3.calculateMultiply(nr_three, eo2);
								if(m1m3.equals((*mvar2)[1])) {
									b_three = true;
								}
							}
							if(!b_two && !b_m_one && !b_three) {
								MathStructure m2m3((*mvar2)[1]);
								m2m3.calculateMultiply(nr_three, eo2);
								if(m2m3.equals((*mvar1)[1])) {
									b_three = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								}
							}
							if(!b_two && !b_m_one && !b_three) {
								MathStructure m1m3((*mvar1)[1]);
								m1m3.calculateMultiply(Number(3, 2), eo2);
								if(m1m3.equals((*mvar2)[1])) {
									b_three = true;
									b_two = true;
								}
							}
							if(!b_two && !b_m_one && !b_three) {
								MathStructure m2m3((*mvar2)[1]);
								m2m3.calculateMultiply(Number(3, 2), eo2);
								if(m2m3.equals((*mvar1)[1])) {
									b_three = true;
									b_two = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								}
							}
						} else if(mvar1->base()->isNumber() && mvar2->base()->isNumber() && !mvar1->base()->number().isInterval() && !mvar2->base()->number().isInterval()) {
							if(mvar1->base()->number() < mvar2->base()->number()) {
								if(mvar2->base()->number() == (mvar1->base()->number() ^ 2)) {
									b_two = true;
								} else if(mvar2->base()->number() == (mvar1->base()->number() ^ 3)) {
									b_three = true;
								} else if(mvar2->base()->number() == (mvar1->base()->number() ^ Number(3, 2))) {
									b_three = true;
									b_two = true;
								} else if(mvar1->base()->number() == (mvar2->base()->number() ^ -1)) {
									b_m_one = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								}
							} else {
								if(mvar1->base()->number() == (mvar2->base()->number() ^ 2)) {
									b_two = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								} else if(mvar1->base()->number() == (mvar2->base()->number() ^ 3)) {
									b_three = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								} else if(mvar1->base()->number() == (mvar2->base()->number() ^ Number(3, 2))) {
									b_three = true;
									b_two = true;
									MathStructure *mtmp = mvar2;
									mvar2 = mvar1;
									mvar1 = mtmp;
								} else if(mvar2->base()->number() == (mvar1->base()->number() ^ -1)) {
									b_m_one = true;
								}
							}
						}
						if(b_two || b_three || b_m_one) {
							MathStructure mv(*mvar1);
							if(b_three && b_two) {
								if(mvar1->base()->equals(*mvar2->base())) mv[1].calculateMultiply(nr_half, eo2);
								else mv[0].number().sqrt();
							}
							MathStructure mv2(*mvar2);
							UnknownVariable *var = new UnknownVariable("", format_and_print(mv));
							var->setInterval(mv);
							MathStructure u_var(var);
							if(b_three && b_two) {
								MathStructure mv1(*mvar1);
								MathStructure u_var1(var);
								u_var1.raise(nr_two);
								replace(mv1, u_var1);
							} else {
								replace(mv, u_var);
							}
							MathStructure u_var2(var);
							u_var2.raise(b_three ? nr_three : (b_two ? nr_two : nr_minus_one));
							replace(mv2, u_var2);
							b = isolate_x_sub(eo, eo2, u_var);
							calculateReplace(u_var, mv, eo2);
							var->destroy();
							if(b && contains(mbak)) {
								set(mbak);
								return false;
							}
							if(b) isolate_x(eo, eo2, x_var);
							return b;
						}
					}
				}
				if(i_px == 0) {
					// a^x+b^x+...
					mvar = NULL;
					int i_neg = 0, i_pos = 0;
					bool neg_first = true, pos_first = true;
					bool b_mul = false, b_mulnum = true;
					Number mulsum, basesum;
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isMultiplication() && CHILD(0)[i].size() == 2 && CHILD(0)[i][0].isNumber() && CHILD(0)[i][0].number().isReal() && CHILD(0)[i][1].isPower() && ((i == 0 && CHILD(0)[i][1][1].contains(x_var)) || (i > 0 && CHILD(0)[i][1][1] == *mvar)) && CHILD(0)[i][1][0].isNumber() && CHILD(0)[i][1][0].number().isPositive()) {
							if(CHILD(0)[i][0].number().isPositive()) {
								if(!i_neg) neg_first = false;
								if(i_neg) pos_first = false;
								i_pos++;
								b_mul = true;
							} else if(CHILD(0)[i][0].number().isNegative()) {
								if(i_pos) neg_first = false;
								if(!i_pos) pos_first = false;
								i_neg++;
								if(!b_mul && !CHILD(0)[i][0].number().isMinusOne()) b_mul = true;
							} else {
								mvar = NULL;
								break;
							}
							mulsum += CHILD(0)[i][0].number();
							basesum += CHILD(0)[i][1][0].number() * CHILD(0)[i][0].number();
							if(i == 0) {
								if(!CHILD(0)[i][1][1].representsReal()) break;
								mvar = &CHILD(0)[i][1][1];
							}
						} else if(CHILD(0)[i].isMultiplication() && CHILD(1).isZero()) {
							b_mulnum = false;
							bool b = false;
							size_t i_x = 0;
							for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
								if(!b && CHILD(0)[i][i2].isPower() && ((i == 0 && CHILD(0)[i][i2][1].contains(x_var)) || (i > 0 && CHILD(0)[i][i2][1] == *mvar)) && CHILD(0)[i][i2][0].isNumber() && CHILD(0)[i][i2][0].number().isPositive()) {
									i_x = i2;
									b = true;
								} else if(CHILD(0)[i][i2].contains(x_var)) {
									b = false;
									break;
								}
							}
							if(b) {
								MathStructure *m_i = &CHILD(0)[i][i_x];
								m_i->ref();
								CHILD(0)[i].delChild(i_x + 1);
								if(CHILD(0)[i].representsPositive()) {
									if(!i_neg) neg_first = false;
									if(i_neg) pos_first = false;
									i_pos++;
									b_mul = true;
								} else if(CHILD(0)[i].representsNegative()) {
									if(i_pos) neg_first = false;
									if(!i_pos) pos_first = false;
									i_neg++;
									b_mul = true;
								} else {
									b = false;
								}
								CHILD(0)[i].insertChild_nocopy(m_i, i_x + 1);
							}
							if(b && i == 0) {
								if(!CHILD(0)[i][i_x][1].representsReal()) break;
								mvar = &CHILD(0)[i][i_x][1];
							}
							if(!b) {
								mvar = NULL;
								break;
							}
						} else if(CHILD(0)[i].isPower() && ((i == 0 && CHILD(0)[i][1].contains(x_var)) || (i > 0 && CHILD(0)[i][1] == *mvar)) && CHILD(0)[i][0].isNumber() && CHILD(0)[i][0].number().isPositive()) {
							if(!i_neg) neg_first = false;
							if(i_neg) pos_first = false;
							i_pos++;
							mulsum++;
							basesum += CHILD(0)[i][0].number();
							if(i == 0) {
								if(!CHILD(0)[i][1].representsReal()) break;
								mvar = &CHILD(0)[i][1];
							}
						} else {
							mvar = NULL;
							break;
						}
					}
					if(mvar) {
						if((pos_first && !i_neg && CHILD(1).representsNonPositive()) || (neg_first && !i_pos && CHILD(1).representsNonNegative())) {
							if(ct_comp == COMPARISON_EQUALS) {
								clear(true);
							} else {
								set(1, 1, 0, true);
							}
							return true;
						}
						if((neg_first || pos_first) && b_mulnum && CHILD(1) == mulsum && (CHILD(1).isZero() || ((pos_first && CHILD(1).representsNonNegative()) || (neg_first && CHILD(1).representsNonPositive())))) {
							CHILD(1).clear();
							if(!mvar->equals(x_var)) {
								MathStructure mx(*mvar);
								CHILD(0) = mx;
								isolate_x(eo, eo2, x_var);
							} else {
								CHILD(0) = x_var;
							}
							return true;
						}
						if((neg_first || pos_first) && b_mulnum && CHILD(1) == basesum && ((CHILD(1).representsNonNegative() && pos_first) || (CHILD(1).representsNonPositive() && neg_first))) {
							CHILD(1).set(1, 1, 0);
							if(!mvar->equals(x_var)) {
								MathStructure mx(*mvar);
								CHILD(0) = mx;
								isolate_x(eo, eo2, x_var);
							} else {
								CHILD(0) = x_var;
							}
							return true;
						}
						if(CHILD(1).isZero() && i_pos && i_neg && !containsInterval()) {
							MathStructure mtest(*this);
							mtest[0].calculateDivide(CHILD(0).last(), eo);
							mtest.childUpdated(1);
							if(mtest.isolate_x_sub(eo, eo2, x_var, morig)) {
								mtest.calculatesub(eo, eo2, true);
								if(mtest.isComparison() && mtest[0] == x_var) {
									set(mtest);
									return true;
								}
							}
						}
						if(eo2.expand && eo.approximation != APPROXIMATION_EXACT && (neg_first || pos_first) && b_mulnum) {
							MathFunction *f = CALCULATOR->getFunctionById(FUNCTION_ID_NEWTON_RAPHSON);
							if(f) {
								UnknownVariable *var = NULL;
								MathStructure mx;
								if(!mvar->equals(x_var)) {
									var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(*mvar) + RIGHT_PARENTHESIS);
									var->setInterval(*mvar);
									mx.set(var);
								} else {
									mx = x_var;
								}
								if((neg_first && i_pos && !CHILD(1).representsNonPositive()) || (pos_first && i_neg && !CHILD(1).representsNonNegative())) {
									MathStructure mdiff(CHILD(0));
									if(var) mdiff.replace(*mvar, mx);
									if(mdiff.differentiate(mx, eo)) {
										MathStructure msolve(f, NULL);
										msolve.addChild(mdiff);
										msolve.addChild(nr_two);
										msolve.addChild(mx);
										msolve.addChild(Number(-10, 1, 0));
										msolve.addChild(Number(20, 1, 0));
										CALCULATOR->beginTemporaryStopMessages();
										msolve.calculateFunctions(eo, true);
										bool b = msolve.isNumber();
										if(b) {
											MathStructure mdiffy(CHILD(0));
											MathStructure mvar_copy(*mvar);
											mdiffy.replace(mvar_copy, msolve);
											mdiffy.eval(eo);
											b = mdiffy.isNumber();
											if(b) {
												ComparisonResult cr = mdiffy.compare(CHILD(1));
												if((cr == COMPARISON_RESULT_GREATER && neg_first) || (cr == COMPARISON_RESULT_LESS && pos_first)) {
													if(ct_comp == COMPARISON_EQUALS) {
														clear(true);
													} else {
														set(1, 1, 0, true);
													}
													return true;
												}
											}
										}
										if(b) {
											CALCULATOR->endTemporaryStopMessages(true);
											MathStructure msolve1(f, NULL);
											msolve1.addChild(*this);
											msolve1.addChild(msolve.number() + 1);
											if(var) msolve1[0].replace(*mvar, mx);
											msolve1.addChild(mx);
											msolve1.addChild(Number(-10, 1, 0));
											msolve1.addChild(Number(20, 1, 0));
											CALCULATOR->beginTemporaryStopMessages();
											msolve1.calculateFunctions(eo, true);
											if(msolve1.isNumber()) {
												MathStructure msolve2(f, NULL);
												msolve2.addChild(*this);
												msolve2.addChild(msolve.number() - 1);
												if(var) msolve2[0].replace(*mvar, mx);
												msolve2.addChild(mx);
												msolve2.addChild(Number(-10, 1, 0));
												msolve2.addChild(Number(20, 1, 0));
												msolve2.calculateFunctions(eo, true);
												if(msolve2.isNumber() && msolve2.number() < msolve1.number()) {
													CALCULATOR->endTemporaryStopMessages(true);
													CHILD(1) = msolve1;
													ComparisonType cmp_type = ct_comp;
													if(var) {
														mx = *mvar;
														CHILD(0) = mx;
														CHILDREN_UPDATED;
														isolate_x(eo, eo2, x_var);
													} else {
														CHILD(0) = x_var;
													}
													MathStructure *malt = new MathStructure(x_var);
													malt->transform(cmp_type, msolve2);
													if(var) {
														(*malt)[0] = mx;
														malt->childUpdated(1);
														malt->isolate_x(eo, eo2, x_var);
													}
													add_nocopy(malt, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
													calculatesub(eo2, eo, false);
													if(var) var->destroy();
													return true;
												}
											}
											CALCULATOR->endTemporaryStopMessages();
										}
										CALCULATOR->endTemporaryStopMessages();
									}
								} else {
									MathStructure msolve(f, NULL);
									msolve.addChild(*this);
									msolve.addChild(nr_two);
									if(var) msolve[0].replace(*mvar, mx);
									msolve.addChild(mx);
									msolve.addChild(Number(-10, 1, 0));
									msolve.addChild(Number(20, 1, 0));
									CALCULATOR->beginTemporaryStopMessages();
									msolve.calculateFunctions(eo, true);
									if(msolve.isNumber()) {
										CALCULATOR->endTemporaryStopMessages(true);
										CHILD(1) = msolve;
										if(var) {
											mx = *mvar;
											CHILD(0) = mx;
											CHILDREN_UPDATED;
											isolate_x(eo, eo2, x_var);
										} else {
											CHILD(0) = x_var;
										}
										if(var) var->destroy();
										return true;
									}
									CALCULATOR->endTemporaryStopMessages();
								}
								if(var) var->destroy();
							}
						}
					}
				}
				if(CHILD(0).containsFunctionId(FUNCTION_ID_LOG)) {
					// x+ln(x)=lambertw(x)
					MathStructure *mln = NULL;
					mvar = NULL;
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isMultiplication()) {
							MathStructure *mln2 = NULL;
							for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
								if(CHILD(0)[i][i2].contains(x_var)) {
									if(!mln2 && CHILD(0)[i][i2].isFunction() && CHILD(0)[i][i2].function()->id() == FUNCTION_ID_LOG && CHILD(0)[i][i2].size() == 1) {
										mln2 = &CHILD(0)[i][i2];
									} else {
										mln2 = NULL;
										break;
									}
								}
							}
							if(mln2) {
								if(mln) {
									if(*mln2 != *mln) {
										if(!mvar) mvar = &CHILD(0)[i];
									}
								} else {
									mln = mln2;
								}
							} else if(!mvar) {
								mvar = &CHILD(0)[i];
							}
						} else if(!mln && CHILD(0)[i].isFunction() && CHILD(0)[i].function()->id() == FUNCTION_ID_LOG && CHILD(0)[i].size() == 1) {
							mln = &CHILD(0)[i];
						} else if(!mvar) {
							mvar = &CHILD(0)[i];
						}
					}
					if(mvar && mln) {
						MathStructure mx(1, 1, 0);
						MathStructure mmul2;
						if(mvar->isMultiplication()) {
							for(size_t i = 0; i < mvar->size(); i++) {
								if((*mvar)[i].contains(x_var)) {
									if(mx.isOne()) mx = (*mvar)[i];
									else mx.multiply((*mvar)[i], true);
								}
							}
						} else {
							mx = *mvar;
						}
						if(!mx.isOne() && !get_multiplier((*mln)[0], mx, mmul2)) {
							mx = m_one;
						}
						if(!mx.isOne()) {
							MathStructure mbak(*this);
							MathStructure mlns;
							MathStructure mmul1;
							mlns.setType(STRUCT_ADDITION);
							bool b_mul = false;
							for(size_t i = 0; i < CHILD(0).size();) {
								if(&CHILD(0)[i] == mln || CHILD(0)[i] == *mln) {
									if(&CHILD(0)[i] != mln) b_mul = true;
									CHILD(0)[i].ref();
									mlns.addChild_nocopy(&CHILD(0)[i]);
									CHILD(0).delChild(i + 1);
								} else if(CHILD(0)[i].isMultiplication()) {
									b = false;
									for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
										if(CHILD(0)[i][i2] == *mln) {
											CHILD(0)[i].ref();
											mlns.addChild_nocopy(&CHILD(0)[i]);
											CHILD(0).delChild(i + 1);
											b = true;
											b_mul = true;
											break;
										}
									}
									if(!b) i++;
								} else {
									i++;
								}
							}
							if(CHILD(0).size() == 1) CHILD(0).setToChild(1, true);
							if(!get_multiplier(CHILD(0), mx, mmul1)) {
								mx = m_one;
							}
							if(!mx.isOne()) {
								if(mlns.size() == 0) {
									mx = m_one;
								} else if(b_mul) {
									MathStructure mmul3;
									MathStructure mln2(*mln);
									if(!get_multiplier(mlns, mln2, mmul3)) {
										mx = m_one;
									} else if(!mmul3.isOne()) {
										mmul1.calculateDivide(mmul3, eo2);
										CHILD(1).calculateDivide(mmul3, eo2);
									}
								}
							}
							if(!mx.isOne() && mmul1.representsNonZero() && mmul2.representsNonZero() && !mmul2.contains(x_var)) {
								MathStructure *marg;
								if(!CHILD(1).isZero()) {
									marg = new MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_E));
									marg->calculateRaise(CHILD(1), eo2);
									if(!mmul1.isOne()) marg->calculateMultiply(mmul1, eo2);
								} else {
									marg = new MathStructure(mmul1);
								}
								if(!mmul2.isOne()) marg->calculateDivide(mmul2, eo2);
								MathStructure *mreq1 = NULL;
								if(mx.representsNonComplex() && !marg->representsNonNegative()) {
									mreq1 = new MathStructure(*marg);
									mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_E));
									mreq1->last().calculateRaise(m_minus_one, eo2);
									mreq1->last().calculateNegate(eo2);
									mreq1->childUpdated(2);
									mreq1->isolate_x(eo2, eo);
								}
								marg->transformById(FUNCTION_ID_LAMBERT_W);
								marg->addChild(m_zero);
								if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
								CHILD(0).set(mx, true);
								setChild_nocopy(marg, 2);
								if(!mmul1.isOne()) CHILD(1).calculateDivide(mmul1, eo2);
								CHILDREN_UPDATED
								isolate_x_sub(eo, eo2, x_var, morig);
								if(mreq1) {
									add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
									calculatesub(eo2, eo, false);
								}
								return true;
							}
							set(mbak);
						}
					}
				}
				if(CHILD(0).containsFunctionId(FUNCTION_ID_COSH) && CHILD(0).containsFunctionId(FUNCTION_ID_SINH)) {
					// a*cosh(x) + b*cosh(x)
					MathStructure *marg = NULL, *m_cosh = NULL, *m_sinh = NULL;
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isMultiplication()) {
							bool b_found = false;
							for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
								if(CHILD(0)[i][i2].contains(x_var)) {
									if(!b_found && CHILD(0)[i][i2].isFunction() && (CHILD(0)[i][i2].function()->id() == FUNCTION_ID_COSH || CHILD(0)[i][i2].function()->id() == FUNCTION_ID_SINH) && CHILD(0)[i][i2].size() == 1 && (!marg || marg->equals(CHILD(0)[i][i2][0]))) {
										if(!marg) marg = &CHILD(0)[i][i2][0];
										if(CHILD(0)[i][i2].function()->id() == FUNCTION_ID_COSH) m_cosh = &CHILD(0)[i][i2];
										else m_sinh = &CHILD(0)[i][i2];
										b_found = true;
									} else {
										b_found = false;
										break;
									}
								}
							}
							if(!b_found) {
								marg = NULL;
								break;
							}
						} else if(CHILD(0)[i].isFunction() && (CHILD(0)[i].function()->id() == FUNCTION_ID_COSH || CHILD(0)[i].function()->id() == FUNCTION_ID_SINH) && CHILD(0)[i].size() == 1 && (!marg || marg->equals(CHILD(0)[i][0]))) {
							if(!marg) marg = &CHILD(0)[i][0];
							if(CHILD(0)[i].function()->id() == FUNCTION_ID_COSH) m_cosh = &CHILD(0)[i];
							else m_sinh = &CHILD(0)[i];
						} else {
							marg = NULL;
							break;
						}
					}
					if(marg && m_cosh && m_sinh) {
						MathStructure mtest(*this);
						EvaluationOptions eo3 = eo;
						eo3.approximation = APPROXIMATION_EXACT;
						MathStructure m_ex(CALCULATOR->getVariableById(VARIABLE_ID_E));
						m_ex.raise(*marg);
						MathStructure m_emx(m_ex);
						m_emx.last().calculateMultiply(nr_minus_one, eo2);
						m_ex *= nr_half;
						m_emx *= nr_half;
						m_ex.swapChildren(1, 2);
						m_emx.swapChildren(1, 2);
						MathStructure mr_sinh(m_ex);
						mr_sinh.calculateSubtract(m_emx, eo3);
						MathStructure mr_cosh(m_ex);
						mr_cosh.calculateAdd(m_emx, eo3);
						eo3.calculate_functions = false;
						mtest[0].calculateReplace(*m_sinh, mr_sinh, eo3);
						mtest[0].calculateReplace(*m_cosh, mr_cosh, eo3);
						if(mtest.isolate_x_sub(eo, eo3, x_var, morig)) {
							if(eo2.approximation != APPROXIMATION_EXACT) mtest.calculatesub(eo2, eo, true);
							set(mtest);
							return true;
						}
					}
				}
			}

			// x+x^(1/a)=b => x=(b-x)^a
			if(eo2.expand && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(0).size() <= 10 && CHILD(0).size() > 1) {
				int i_root = 0;
				size_t root_index = 0;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(CALCULATOR->aborted()) return false;
					if(CHILD(0)[i].isPower()) {
						if(CHILD(0)[i][1].isNumber() && !CHILD(0)[i][1].isInteger() && CHILD(0)[i][1].number().numeratorIsOne() && CHILD(0)[i][1].number().denominatorIsLessThan(10)) {
							if(i_root) {
								if(i_root != CHILD(0)[i][1].number().denominator().intValue()) {
									i_root = 0;
									break;
								}
							} else {
								i_root = CHILD(0)[i][1].number().denominator().intValue();
								root_index = i;
							}
						} else if(!CHILD(0)[i][1].isNumber() || !CHILD(0)[i][1].number().isInteger()) {
							i_root = 0;
							break;
						}
					} else if(CHILD(0)[i].isFunction() && CHILD(0)[i].function()->id() == FUNCTION_ID_ROOT) {
						if(VALID_ROOT(CHILD(0)[i]) && CHILD(0)[i][1].number().isLessThan(10)) {
							if(i_root) {
								if(i_root != CHILD(0)[i][1].number().intValue()) {
									i_root = 0;
									break;
								}
							} else {
								i_root = CHILD(0)[i][1].number().intValue();
								root_index = i;
							}
						} else {
							i_root = 0;
							break;
						}
					} else if(CHILD(0)[i].isMultiplication()) {
						bool b_break = false;
						for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
							if(CHILD(0)[i][i2].contains(x_var)) {
								if(CHILD(0)[i][i2].isPower()) {
									if(CHILD(0)[i][i2][1].isNumber() && !CHILD(0)[i][i2][1].number().isInteger() && CHILD(0)[i][i2][1].number().numeratorIsOne() && CHILD(0)[i][i2][1].number().denominatorIsLessThan(10)) {
										if(i_root) {
											if(i_root != CHILD(0)[i][i2][1].number().denominator().intValue()) {
												i_root = 0;
												b_break = true;
												break;
											}
										} else {
											i_root = CHILD(0)[i][i2][1].number().denominator().intValue();
											root_index = i;
										}
									} else if(!CHILD(0)[i][i2][1].isNumber() || !CHILD(0)[i][i2][1].number().isInteger()) {
										i_root = 0;
										b_break = true;
										break;
									}
								} else if(CHILD(0)[i][i2].isFunction() && CHILD(0)[i][i2].function()->id() == FUNCTION_ID_ROOT) {
									if(VALID_ROOT(CHILD(0)[i][i2]) && CHILD(0)[i][i2][1].number().isLessThan(10)) {
										if(i_root) {
											if(i_root != CHILD(0)[i][i2][1].number().intValue()) {
												i_root = 0;
												b_break = true;
												break;
											}
										} else {
											i_root = CHILD(0)[i][i2][1].number().intValue();
											root_index = i;
										}
									} else {
										i_root = 0;
										b_break = true;
										break;
									}
								}
							}
						}
						if(b_break) break;
					}
				}
				if(i_root) {
					CALCULATOR->beginTemporaryStopMessages();
					MathStructure mtest(*this);
					MathStructure msub;
					if(mtest[0].size() == 2) {
						msub = mtest[0][root_index == 0 ? 1 : 0];
					} else {
						msub = mtest[0];
						msub.delChild(root_index + 1);
					}
					mtest[0].setToChild(root_index + 1);
					mtest[1].calculateSubtract(msub, eo2);
					if(mtest[0].calculateRaise(i_root, eo2) && mtest[1].calculateRaise(i_root, eo2)) {
						mtest.childrenUpdated();
						if(eo2.expand && morig) {
							MathStructure mtest2(mtest[0]);
							mtest2.calculateSubtract(mtest[1], eo2);
							if(mtest2.factorize(eo2, false, false, 0, false, false, NULL, m_undefined, false, false, 3) && mtest2.isMultiplication()) {
								for(size_t i = 0; i < mtest2.size(); i++) {
									if(mtest2[i].equals(CHILD(0), true, true)) {
										i_root = 0;
										break;
									}
								}
							}
						}
						if(i_root && mtest.isolate_x(eo2, eo, x_var)) {
							if((mtest.isLogicalAnd() || mtest.isLogicalOr() || mtest.isComparison()) && test_comparisons(*this, mtest, x_var, eo, false, eo2.expand ? 1 : 2) < 0) {
								if(eo2.expand) {
									CALCULATOR->endTemporaryStopMessages(true);
									add(mtest, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
									calculatesub(eo2, eo, false);
									return true;
								}
							} else {
								CALCULATOR->endTemporaryStopMessages(true);
								set(mtest);
								return true;
							}
						}
						CALCULATOR->endTemporaryStopMessages();
					}

				}
			}

			// x+y/x=z => x*x+y=z*x
			MathStructure mdiv;
			mdiv.setUndefined();
			MathStructure mdiv_inv;
			bool b_multiple_div = false;
			for(size_t i = 0; i < CHILD(0).size() && !b_multiple_div; i++) {
				if(CALCULATOR->aborted()) return false;
				if(CHILD(0)[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
						if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][1].isNumber() && CHILD(0)[i][i2][1].number().isReal() && CHILD(0)[i][i2][1].number().isNegative() && CHILD(0)[i][i2][0].contains(x_var)) {
							if(!mdiv.isUndefined()) b_multiple_div = true;
							else mdiv = CHILD(0)[i][i2];
							break;
						}
					}
				} else if(CHILD(0)[i].isPower() && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number().isReal() && CHILD(0)[i][1].number().isNegative()) {
					if(!mdiv.isUndefined()) b_multiple_div = true;
					else mdiv = CHILD(0)[i];
				}
			}
			if(!mdiv.isUndefined() && countTotalChildren() > 200) mdiv.setUndefined();
			if(!mdiv.isUndefined() && b_multiple_div && mdiv.containsInterval()) {
				MathStructure mbak(*this);
				CALCULATOR->beginTemporaryStopIntervalArithmetic();
				bool failed = false;
				EvaluationOptions eo3 = eo2;
				eo3.interval_calculation = INTERVAL_CALCULATION_NONE;
				fix_intervals(*this, eo3, &failed);
				if(!failed && isolate_x_sub(eo, eo3, x_var, morig)) {
					CALCULATOR->endTemporaryStopIntervalArithmetic();
					if((CALCULATOR->usesIntervalArithmetic() && eo.approximation != APPROXIMATION_EXACT) || mbak.containsInterval()) CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
					fix_intervals(*this, eo2);
					return true;
				}
				CALCULATOR->endTemporaryStopIntervalArithmetic();
				set(mbak);
			} else if(!mdiv.isUndefined()) {
				if(mdiv[1].isMinusOne()) {
					mdiv_inv = mdiv[0];
				} else {
					mdiv_inv = mdiv;
					mdiv_inv[1].number().negate();
				}
				EvaluationOptions eo3 = eo2;
				eo3.expand = true;
				bool b2 = false;
				MathStructure mbak(*this);
				bool strict_check = false;
				for(size_t i3 = 0; i3 < CHILD(0).size(); i3++) {
					bool b = false;
					if(!b2 || !mdiv.containsInterval()) {
						if(CHILD(0)[i3].isPower() && CHILD(0)[i3].equals(mdiv, true)) {
							CHILD(0)[i3].set(1, 1, 0, true);
							b = true;
						} else if(CHILD(0)[i3].isMultiplication()) {
							for(size_t i4 = 0; i4 < CHILD(0)[i3].size(); i4++) {
								if(CHILD(0)[i3][i4].isPower() && CHILD(0)[i3][i4].equals(mdiv, true)) {
									b = true;
									CHILD(0)[i3].delChild(i4 + 1);
									if(CHILD(0)[i3].size() == 0) CHILD(0)[i3].set(1, 1, 0, true);
									else if(CHILD(0)[i3].size() == 1) CHILD(0)[i3].setToChild(1, true);
									break;
								}
							}
						}
					}
					if(!b) {
						CHILD(0)[i3].calculateMultiply(mdiv_inv, eo3);
						strict_check = true;
					} else {
						b2 = true;
					}
				}
				CHILD(0).childrenUpdated();
				CHILD(0).evalSort();
				CHILD(0).calculatesub(eo2, eo, false);
				CHILD(1).calculateMultiply(mdiv_inv, eo3);
				CHILDREN_UPDATED
				if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
					bool not_equals = ct_comp == COMPARISON_NOT_EQUALS;
					isolate_x(eo, eo2, x_var);
					if(!strict_check || eo2.expand || ((m_type != STRUCT_COMPARISON || CHILD(0) == x_var || !CHILD(0).isAddition() || CHILD(0).size() < 3) && ((m_type != STRUCT_LOGICAL_AND && m_type != STRUCT_LOGICAL_OR) || SIZE == 0 || !CHILD(0).isComparison() || CHILD(0)[0] == x_var || !CHILD(0)[0].isAddition() || CHILD(0)[0].size() < 3))) {
						if(!mdiv[0].representsNonZero(true)) {
							MathStructure *mtest = new MathStructure(mdiv[0]);
							mtest->add(m_zero, not_equals ? OPERATION_EQUALS : OPERATION_NOT_EQUALS);
							mtest->isolate_x_sub(eo, eo2, x_var);
							add_nocopy(mtest, not_equals ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							calculatesub(eo2, eo, false);
						}
						return true;
					}
					set(mbak);
				} else {
					MathStructure *malt = new MathStructure(*this);
					if(ct_comp == COMPARISON_EQUALS_GREATER) {
						ct_comp = COMPARISON_EQUALS_LESS;
					} else if(ct_comp == COMPARISON_GREATER) {
						ct_comp = COMPARISON_LESS;
					} else if(ct_comp == COMPARISON_EQUALS_LESS) {
						ct_comp = COMPARISON_EQUALS_GREATER;
					} else if(ct_comp == COMPARISON_LESS) {
						ct_comp = COMPARISON_GREATER;
					}
					isolate_x(eo, eo2, x_var);
					MathStructure *mtest = new MathStructure(mdiv);
					mtest->add(m_zero, OPERATION_LESS);
					MathStructure *mtest_alt = new MathStructure(*mtest);
					mtest_alt->setComparisonType(COMPARISON_GREATER);
					mtest->isolate_x_sub(eo, eo2, x_var);
					add_nocopy(mtest, OPERATION_LOGICAL_AND);
					calculatesub(eo2, eo, false);
					malt->isolate_x(eo, eo2, x_var);
					mtest_alt->isolate_x_sub(eo, eo2, x_var);
					malt->add_nocopy(mtest_alt, OPERATION_LOGICAL_AND);
					malt->calculatesub(eo2, eo, false);
					add_nocopy(malt, OPERATION_LOGICAL_OR);
					calculatesub(eo2, eo, false);
					return true;
				}
			}

			// x^a+x^(b/c)=d => x^(1/c)^(a*c)+x^(1/c)^b=d; f(x)^a+f(x)^b=c => u^a+u^b=c
			MathStructure mbase;
			b = true;
			Number nr_root;
			bool b_f_root = false;

			for(size_t i = 0; i < CHILD(0).size(); i++) {
				if(CALCULATOR->aborted()) return false;
				if(!mbase.isZero() && CHILD(0)[i] == mbase) {
				} else if(CHILD(0)[i].isPower() && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number().isPositive() && CHILD(0)[i][1].number().isRational()) {
					if(CHILD(0)[i][0].isFunction() && CHILD(0)[i][0].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(CHILD(0)[i][0])) {
						if(mbase.isZero()) mbase = CHILD(0)[i][0][0];
						else if(mbase != CHILD(0)[i][0][0]) {b = false; break;}
						if(!nr_root.isZero()) {
							if(!b_f_root) {b = false; break;}
							nr_root.lcm(CHILD(0)[i][0][1].number());
						} else {
							nr_root = CHILD(0)[i][0][1].number();
						}
						b_f_root = true;
					} else {
						if(mbase.isZero()) mbase = CHILD(0)[i][0];
						else if(mbase != CHILD(0)[i][0]) {b = false; break;}
						if(!CHILD(0)[i][1].number().isInteger()) {
							if(b_f_root) {b = false; break;}
							if(!nr_root.isZero()) {
								nr_root.lcm(CHILD(0)[i][1].number().denominator());
							} else {
								nr_root = CHILD(0)[i][1].number().denominator();
							}
						}
					}
				} else if(CHILD(0)[i].isMultiplication()) {
					for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
						if(!mbase.isZero() && CHILD(0)[i][i2] == mbase) {
						} else if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][1].isNumber() && CHILD(0)[i][i2][1].number().isPositive() && CHILD(0)[i][i2][1].number().isRational()) {
							if(CHILD(0)[i][i2][0].isFunction() && CHILD(0)[i][i2][0].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(CHILD(0)[i][i2][0])) {
								if(mbase.isZero()) mbase = CHILD(0)[i][i2][0][0];
								else if(mbase != CHILD(0)[i][i2][0][0]) {b = false; break;}
								if(!nr_root.isZero()) {
									if(!b_f_root) {b = false; break;}
									nr_root.lcm(CHILD(0)[i][i2][0][1].number());
								} else {
									nr_root = CHILD(0)[i][i2][0][1].number();
								}
								b_f_root = true;
							} else {
								if(mbase.isZero()) mbase = CHILD(0)[i][i2][0];
								else if(mbase != CHILD(0)[i][i2][0]) {b = false; break;}
								if(!CHILD(0)[i][i2][1].number().isInteger()) {
									if(b_f_root) {b = false; break;}
									if(!nr_root.isZero()) {
										nr_root.lcm(CHILD(0)[i][i2][1].number().denominator());
									} else {
										nr_root = CHILD(0)[i][i2][1].number().denominator();
									}
								}
							}
						} else if(CHILD(0)[i][i2].isFunction() && CHILD(0)[i][i2].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(CHILD(0)[i][i2])) {
							if(mbase.isZero()) mbase = CHILD(0)[i][i2][0];
							else if(mbase != CHILD(0)[i][i2][0]) {b = false; break;}
							if(!nr_root.isZero()) {
								if(!b_f_root) {b = false; break;}
								nr_root.lcm(CHILD(0)[i][i2][1].number());
							} else {
								nr_root = CHILD(0)[i][i2][1].number();
							}
							b_f_root = true;
						} else if(CHILD(0)[i][i2].isNumber() && !CHILD(0)[i][i2].number().isZero()) {
						} else if(mbase.isZero()) {
							mbase = CHILD(0)[i][i2];
						} else {
							b = false; break;
						}
					}
					if(!b) break;
				} else if(CHILD(0)[i].isFunction() && CHILD(0)[i].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(CHILD(0)[i])) {
					if(mbase.isZero()) mbase = CHILD(0)[i][0];
					else if(mbase != CHILD(0)[i][0]) {b = false; break;}
					if(!nr_root.isZero()) {
						if(!b_f_root) {b = false; break;}
						nr_root.lcm(CHILD(0)[i][1].number());
					} else {
						nr_root = CHILD(0)[i][1].number();
					}
					b_f_root = true;
				} else if(mbase.isZero()) {
					mbase = CHILD(0)[i];
				} else {
					b = false; break;
				}
			}
			if(b && !mbase.isZero() && nr_root.isLessThan(100)) {
				MathStructure mvar(mbase);
				if(!nr_root.isZero()) {
					if(b_f_root) {
						MathStructure mroot(nr_root);
						mvar.set(CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), &mbase, &mroot, NULL);
					} else {
						Number nr_pow(nr_root);
						nr_pow.recip();
						mvar.raise(nr_pow);
					}
				}
				UnknownVariable *var = NULL;
				if(mvar != x_var) {
					var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mvar) + RIGHT_PARENTHESIS);
					var->setInterval(mvar);
				}
				if(!nr_root.isZero()) {
					MathStructure mbak(*this);
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CALCULATOR->aborted()) {set(mbak); return false;}
						if(CHILD(0)[i] == mbase) {
							CHILD(0)[i] = var;
							CHILD(0)[i].raise(nr_root);
						} else if(CHILD(0)[i].isPower() && CHILD(0)[i][1].isNumber()) {
							if(CHILD(0)[i][0].isFunction() && CHILD(0)[i][0].function()->id() == FUNCTION_ID_ROOT && CHILD(0)[i][0] != mbase) {
								CHILD(0)[i][1].number().multiply(nr_root);
								CHILD(0)[i][1].number().divide(CHILD(0)[i][0][1].number());
								if(CHILD(0)[i][1].isOne()) CHILD(0)[i] = var;
								else CHILD(0)[i][0] = var;
							} else if(CHILD(0)[i][0] == mbase) {
								CHILD(0)[i][1].number().multiply(nr_root);
								if(CHILD(0)[i][1].isOne()) CHILD(0)[i] = var;
								else CHILD(0)[i][0] = var;
							} else {
								// should not happen
							}
						} else if(CHILD(0)[i].isMultiplication()) {
							for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
								if(CHILD(0)[i][i2] == mbase) {
									CHILD(0)[i][i2] = var;
									CHILD(0)[i][i2].raise(nr_root);
								} else if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][1].isNumber()) {
									if(CHILD(0)[i][i2][0].isFunction() && CHILD(0)[i][i2][0].function()->id() == FUNCTION_ID_ROOT && CHILD(0)[i][i2][0] != mbase) {
										CHILD(0)[i][i2][1].number().multiply(nr_root);
										CHILD(0)[i][i2][1].number().divide(CHILD(0)[i][i2][0][1].number());
										if(CHILD(0)[i][i2][1].isOne()) CHILD(0)[i][i2] = var;
										else CHILD(0)[i][i2][0] = var;
									} else if(CHILD(0)[i][i2][0] == mbase) {
										CHILD(0)[i][i2][1].number().multiply(nr_root);
										if(CHILD(0)[i][i2][1].isOne()) CHILD(0)[i][i2] = var;
										else CHILD(0)[i][i2][0] = var;
									} else {
										// should not happen
									}
								} else if(CHILD(0)[i][i2].isFunction() && CHILD(0)[i][i2].function()->id() == FUNCTION_ID_ROOT) {
									Number nr_pow(nr_root);
									nr_pow.divide(CHILD(0)[i][i2][1].number());
									CHILD(0)[i][i2][1] = nr_pow;
									CHILD(0)[i][i2][0] = var;
									CHILD(0)[i][i2].setType(STRUCT_POWER);
								} else if(CHILD(0)[i][i2].isNumber() && !CHILD(0)[i][i2].number().isZero()) {
								} else {
									// should not happen
								}
							}
						} else if(CHILD(0)[i].isFunction() && CHILD(0)[i].function()->id() == FUNCTION_ID_ROOT) {
							Number nr_pow(nr_root);
							nr_pow.divide(CHILD(0)[i][1].number());
							CHILD(0)[i][1] = nr_pow;
							CHILD(0)[i][0] = var;
							CHILD(0)[i].setType(STRUCT_POWER);
						} else {
							// should not happen
						}
					}
					MathStructure u_var(var);
					replace(mvar, u_var);
					CHILD(0).calculatesub(eo2, eo2, true);
					CHILD_UPDATED(0)
					b = isolate_x_sub(eo, eo2, u_var);
					calculateReplace(u_var, mvar, eo2);
					var->destroy();
					if(b && contains(mbak)) {
						set(mbak);
						return false;
					}
					if(b) isolate_x(eo, eo2, x_var);
					return b;
				} else if(mvar != x_var) {
					MathStructure mbak(*this);
					MathStructure u_var(var);
					replace(mvar, u_var);
					CHILD(0).calculatesub(eo2, eo2, true);
					CHILD_UPDATED(0)
					b = isolate_x_sub(eo, eo2, u_var);
					calculateReplace(u_var, mvar, eo2);
					var->destroy();
					if(b && contains(mbak)) {
						set(mbak);
						return false;
					}
					if(b) isolate_x(eo, eo2, x_var);
					return b;
				}
			}

			if(!eo2.expand) break;

			// Try factorization
			if(!morig || !equals(*morig)) {
				MathStructure mtest(*this);
				if(!mtest[1].isZero()) {
					mtest[0].calculateSubtract(CHILD(1), eo2);
					mtest[1].clear();
					mtest.childrenUpdated();
				}
				b = eo2.do_polynomial_division && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && do_simplification(mtest[0], eo, true, true, false, false, true);
				if(b && mtest[0].isMultiplication() && mtest[0].size() == 2 && mtest[0][1].isPower() && mtest[0][1][1].representsNegative()) {
					MathStructure mreq(mtest);
					mtest[0].setToChild(1, true);
					mtest.childUpdated(1);
					eo2.do_polynomial_division = false;
					if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
						mreq.setComparisonType(ct_comp == COMPARISON_EQUALS ? COMPARISON_NOT_EQUALS : COMPARISON_EQUALS);
						mreq[0].setToChild(2, true);
						mreq[0][1].calculateNegate(eo2);
						mreq[0].calculateRaiseExponent(eo2);
						mreq.childUpdated(1);
						mreq.isolate_x_sub(eo, eo2, x_var, this);
						mtest.add(mreq, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
						mtest.calculatesub(eo2, eo, false);
						set_nocopy(mtest);
						return true;
					}
				} else {
					if(b) {
						mtest = *this;
						if(!mtest[1].isZero()) {
							mtest[0].calculateSubtract(CHILD(1), eo2);
							mtest[1].clear();
							mtest.childrenUpdated();
						}
					}
					if(mtest[0].factorize(eo2, false, false, 0, false, false, NULL, m_undefined, false, false, 3) && !(mtest[0].isMultiplication() && mtest[0].size() == 2 && ((mtest[0][0].isNumber() && mtest[0][1].isAddition()) || mtest[0][0] == CHILD(1) || mtest[0][1] == CHILD(1)))) {
						mtest.childUpdated(1);
						if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
							set_nocopy(mtest);
							return true;
						}
					}
				}
			}

			// ax^3+bx^2+cx=d
			if(CHILD(0).size() >= 2 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
				bool cbpow = false, sqpow = false, nopow = false;
				MathStructure mstruct_a, mstruct_b, mstruct_c;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(CALCULATOR->aborted()) return false;
					if(CHILD(0)[i].isPower() && CHILD(0)[i][0] == x_var && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number() == 3) {
						if(cbpow) mstruct_a.add(m_one, true);
						else {cbpow = true; mstruct_a.set(m_one);}
					} else if(CHILD(0)[i].isPower() && CHILD(0)[i][0] == x_var && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number() == 2) {
						if(sqpow) mstruct_b.add(m_one, true);
						else {sqpow = true; mstruct_b.set(m_one);}
					} else if(CHILD(0)[i] == x_var) {
						if(nopow) mstruct_c.add(m_one, true);
						else {nopow = true; mstruct_c.set(m_one);}
					} else if(CHILD(0)[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
							if(CHILD(0)[i][i2] == x_var) {
								if(nopow) {
									MathStructure *madd = new MathStructure(CHILD(0)[i]);
									madd->delChild(i2 + 1, true);
									mstruct_c.add_nocopy(madd, true);
								} else {
									nopow = true;
									mstruct_c = CHILD(0)[i];
									mstruct_c.delChild(i2 + 1, true);
								}
							} else if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][0] == x_var && CHILD(0)[i][i2][1].isNumber() && CHILD(0)[i][i2][1].number() == 2) {
								if(sqpow) {
									MathStructure *madd = new MathStructure(CHILD(0)[i]);
									madd->delChild(i2 + 1, true);
									mstruct_b.add_nocopy(madd, true);
								} else {
									sqpow = true;
									mstruct_b = CHILD(0)[i];
									mstruct_b.delChild(i2 + 1, true);
								}
							} else if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][0] == x_var && CHILD(0)[i][i2][1].isNumber() && CHILD(0)[i][i2][1].number() == 3) {
								if(cbpow) {
									MathStructure *madd = new MathStructure(CHILD(0)[i]);
									madd->delChild(i2 + 1, true);
									mstruct_a.add_nocopy(madd, true);
								} else {
									cbpow = true;
									mstruct_a = CHILD(0)[i];
									mstruct_a.delChild(i2 + 1, true);
								}
							} else if(CHILD(0)[i][i2].contains(x_var)) {
								cbpow = false;
								sqpow = false;
								nopow = false;
								break;
							}
						}
						if(!cbpow && !sqpow && !nopow) break;
					} else {
						cbpow = false;
						sqpow = false;
						nopow = false;
						break;
					}
				}
				bool b = false;
				if(cbpow && (nopow || sqpow) && !mstruct_a.representsZero(true)) {
					b = mstruct_a.representsNonZero(true);
					if(!b && eo2.approximation == APPROXIMATION_EXACT) {
						MathStructure mtest(mstruct_a);
						mtest.add(m_zero, OPERATION_NOT_EQUALS);
						EvaluationOptions eo3 = eo2;
						eo3.test_comparisons = true;
						mtest.calculatesub(eo3, eo, false);
						b = mtest.isOne();
					}
				}
				if(b) {

					MathStructure mbak(*this);

					bool stop_iv = containsInterval(true, false, false, -2, false);

					cube_eq_stop_iv:

					CALCULATOR->beginTemporaryStopMessages();

					EvaluationOptions eo3 = eo2;
					bool failed = false;
					if(stop_iv) {
						CALCULATOR->beginTemporaryStopIntervalArithmetic();
						eo3.interval_calculation = INTERVAL_CALCULATION_NONE;
						fix_intervals(*this, eo3, &failed);
					}
					if(failed) {
						set(mbak);
						CALCULATOR->endTemporaryStopIntervalArithmetic();
						CALCULATOR->endTemporaryStopMessages();
					} else {
						eo3.keep_zero_units = false;

						MathStructure mstruct_d(CHILD(1));
						mstruct_d.calculateNegate(eo3);

						// 18abcd - 4b^3*d + b^2*c^2 - 4a*c^3 - 27a^2*d^2
						MathStructure mdelta(18, 1, 0);
						mdelta.multiply(mstruct_a);
						mdelta.multiply(mstruct_b, true);
						mdelta.multiply(mstruct_c, true);
						mdelta.multiply(mstruct_d, true);
						MathStructure *mdelta_b = new MathStructure(mstruct_b);
						mdelta_b->raise(nr_three);
						mdelta_b->multiply(Number(-4, 1, 0));
						mdelta_b->multiply(mstruct_d);
						MathStructure *mdelta_c = new MathStructure(mstruct_b);
						mdelta_c->raise(nr_two);
						MathStructure *mdelta_c2 = new MathStructure(mstruct_c);
						mdelta_c2->raise(nr_two);
						mdelta_c->multiply_nocopy(mdelta_c2);
						MathStructure *mdelta_d = new MathStructure(mstruct_c);
						mdelta_d->raise(nr_three);
						mdelta_d->multiply(Number(-4, 1, 0));
						mdelta_d->multiply(mstruct_a, true);
						MathStructure *mdelta_e = new MathStructure(mstruct_a);
						mdelta_e->raise(nr_two);
						MathStructure *mdelta_e2 = new MathStructure(mstruct_d);
						mdelta_e2->raise(nr_two);
						mdelta_e->multiply_nocopy(mdelta_e2);
						mdelta_e->multiply(Number(-27, 1, 0));
						mdelta.add_nocopy(mdelta_b);
						mdelta.add_nocopy(mdelta_c, true);
						mdelta.add_nocopy(mdelta_d, true);
						mdelta.add_nocopy(mdelta_e, true);
						mdelta.calculatesub(eo3, eo);

						if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopMessages(); if(stop_iv) CALCULATOR->endTemporaryStopIntervalArithmetic(); set(mbak); return false;}

						int b_zero = -1;
						int b_real = -1;
						if(mdelta.representsNonZero(true)) b_zero = 0;
						else if(mdelta.representsZero(true)) b_zero = 1;

						if(b_zero == 0) {
							if(mdelta.representsPositive(true)) {
								b_real = 1;
							} else if(mdelta.representsNegative(true)) {
								b_real = 0;
							}
						} else if(b_zero < 0) {
							ComparisonResult cr = mdelta.compare(m_zero);
							if(cr == COMPARISON_RESULT_EQUAL) {
								b_zero = 1;
							} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
								if(cr == COMPARISON_RESULT_GREATER) b_real = 0;
								else if(cr == COMPARISON_RESULT_LESS) b_real = 1;
								b_zero = 0;
							}
						}
						if(b_real < 0 && mdelta.representsComplex(true)) b_real = -2;
						if(b_real == -1) b_zero = -1;

						if(b_real > 0 && (!mstruct_a.representsNonComplex() || !mstruct_b.representsNonComplex() || !mstruct_c.representsNonComplex() || !mstruct_d.representsNonComplex())) b_real = -2;

						MathStructure mdelta0;
						int b0_zero = -1;

						if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopMessages(); if(stop_iv) CALCULATOR->endTemporaryStopIntervalArithmetic(); set(mbak); return false;}

						if(b_zero >= 0) {
							// b^2 - 3ac
							mdelta0 = mstruct_b;
							mdelta0.raise(nr_two);
							MathStructure *mdelta0_b = new MathStructure(-3, 1, 0);
							mdelta0_b->multiply(mstruct_a);
							mdelta0_b->multiply(mstruct_c, true);
							mdelta0.add_nocopy(mdelta0_b);
							mdelta0.calculatesub(eo3, eo, true);
							if(mdelta0.representsNonZero(true)) b0_zero = 0;
							else if(mdelta0.representsZero(true)) b0_zero = 1;
							else {
								ComparisonResult cr = mdelta0.compare(m_zero);
								if(cr == COMPARISON_RESULT_EQUAL) {
									b0_zero = 1;
								} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
									b0_zero = 0;
								}
							}
						}

						if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopMessages(); if(stop_iv) CALCULATOR->endTemporaryStopIntervalArithmetic(); set(mbak); return false;}

						if(b_zero == 1) {
							if(b0_zero == 1) {
								// -b/(3a)
								CHILD(0) = x_var;
								CHILD(1).set(-1, 3);
								CHILD(1).calculateMultiply(mstruct_b, eo3);
								CHILD(1).calculateDivide(mstruct_a, eo3);
								CHILDREN_UPDATED;
								if(stop_iv) {
									CALCULATOR->endTemporaryStopIntervalArithmetic();
									CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
									fix_intervals(*this, eo2);
								} else if(containsInterval(true, false, false, -2, false)) {
									set(mbak);
									stop_iv = true;
									CALCULATOR->endTemporaryStopMessages();
									goto cube_eq_stop_iv;
								}
								CALCULATOR->endTemporaryStopMessages(true);
								return true;
							} else if(b0_zero == 0) {

								MathStructure *malt1 = new MathStructure(x_var);
								malt1->transform(STRUCT_COMPARISON, m_zero);
								malt1->setComparisonType(comparisonType());
								MathStructure *malt2 = new MathStructure(*malt1);

								// (9ad - bc) / (2*delta0)
								(*malt1)[1].set(9, 1, 0);
								(*malt1)[1].calculateMultiply(mstruct_a, eo3);
								(*malt1)[1].calculateMultiply(mstruct_d, eo3);
								MathStructure *malt1_2b = new MathStructure(-1, 1, 0);
								malt1_2b->calculateMultiply(mstruct_b, eo3);
								malt1_2b->calculateMultiply(mstruct_c, eo3);
								(*malt1)[1].add_nocopy(malt1_2b);
								(*malt1)[1].calculateAddLast(eo3);
								(*malt1)[1].calculateDivide(nr_two, eo3);
								(*malt1)[1].calculateDivide(mdelta0, eo3);

								// (4abc - 9a^2*d - b^3) / (a*delta0)
								(*malt2)[1].set(4, 1, 0);
								(*malt2)[1].calculateMultiply(mstruct_a, eo3);
								(*malt2)[1].calculateMultiply(mstruct_b, eo3);
								(*malt2)[1].calculateMultiply(mstruct_c, eo3);
								MathStructure *malt2_2b = new MathStructure(mstruct_a);
								malt2_2b->calculateRaise(nr_two, eo3);
								malt2_2b->calculateMultiply(Number(-9, 1, 0), eo3);
								malt2_2b->calculateMultiply(mstruct_d, eo3);
								(*malt2)[1].add_nocopy(malt2_2b);
								(*malt2)[1].calculateAddLast(eo3);
								MathStructure *malt2_2c = new MathStructure(mstruct_b);
								malt2_2c->calculateRaise(nr_three, eo3);
								malt2_2c->calculateNegate(eo3);
								(*malt2)[1].add_nocopy(malt2_2c);
								(*malt2)[1].calculateAddLast(eo3);
								(*malt2)[1].calculateDivide(mstruct_a, eo3);
								(*malt2)[1].calculateDivide(mdelta0, eo3);

								if(ct_comp == COMPARISON_NOT_EQUALS) {
									clear(true);
									setType(STRUCT_LOGICAL_AND);
								} else {
									clear(true);
									setType(STRUCT_LOGICAL_OR);
								}

								malt1->childUpdated(2);
								malt2->childUpdated(2);

								addChild_nocopy(malt1);
								addChild_nocopy(malt2);

								calculatesub(eo3, eo, false);

								if(stop_iv) {
									CALCULATOR->endTemporaryStopIntervalArithmetic();
									CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
									fix_intervals(*this, eo2);
								} else if(containsInterval(true, false, false, -2, false)) {
									set(mbak);
									stop_iv = true;
									CALCULATOR->endTemporaryStopMessages();
									goto cube_eq_stop_iv;
								}
								CALCULATOR->endTemporaryStopMessages(true);
								return true;
							}
						}

						if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopMessages(); if(stop_iv) CALCULATOR->endTemporaryStopIntervalArithmetic(); return false;}

						MathStructure mdelta1;
						bool b_neg = false;

						if(b_zero == 0 && b0_zero >= 0) {

							// 2b^3 - 9abc + 27a^2*d
							mdelta1 = mstruct_b;
							mdelta1.raise(nr_three);
							mdelta1.multiply(nr_two);
							MathStructure *mdelta1_b = new MathStructure(-9, 1, 0);
							mdelta1_b->multiply(mstruct_a);
							mdelta1_b->multiply(mstruct_b, true);
							mdelta1_b->multiply(mstruct_c, true);
							MathStructure *mdelta1_c = new MathStructure(mstruct_a);
							mdelta1_c->raise(nr_two);
							mdelta1_c->multiply(Number(27, 1, 0));
							mdelta1_c->multiply(mstruct_d, true);
							mdelta1.add_nocopy(mdelta1_b);
							mdelta1.add_nocopy(mdelta1_c, true);
							mdelta1.calculatesub(eo3, eo, true);

							if(b0_zero == 1) {

								if(mdelta1.representsNegative(true)) {
									b_neg = true;
								} else if(!mdelta1.representsPositive(true)) {
									if(mdelta1.representsZero(true)) {
										// -b/(3a)
										CHILD(0) = x_var;
										CHILD(1).set(-1, 3);
										CHILD(1).calculateMultiply(mstruct_b, eo3);
										CHILD(1).calculateDivide(mstruct_a, eo3);
										CHILDREN_UPDATED;
										if(stop_iv) {
											CALCULATOR->endTemporaryStopIntervalArithmetic();
											CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
											fix_intervals(*this, eo2);
										} else if(containsInterval(true, false, false, -2, false)) {
											set(mbak);
											stop_iv = true;
											CALCULATOR->endTemporaryStopMessages();
											goto cube_eq_stop_iv;
										}
										CALCULATOR->endTemporaryStopMessages(true);
										return true;
									}
									ComparisonResult cr = mdelta1.compare(m_zero);
									if(cr == COMPARISON_RESULT_EQUAL) {
										// -b/(3a)
										CHILD(0) = x_var;
										CHILD(1).set(-1, 3);
										CHILD(1).calculateMultiply(mstruct_b, eo3);
										CHILD(1).calculateDivide(mstruct_a, eo3);
										CHILDREN_UPDATED;
										if(stop_iv) {
											CALCULATOR->endTemporaryStopIntervalArithmetic();
											CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
											fix_intervals(*this, eo2);
										} else if(containsInterval(true, false, false, -2, false)) {
											set(mbak);
											stop_iv = true;
											CALCULATOR->endTemporaryStopMessages();
											goto cube_eq_stop_iv;
										}
										CALCULATOR->endTemporaryStopMessages(true);
										return true;
									} else if(cr == COMPARISON_RESULT_GREATER) {
										b_neg = true;
									} else if(cr != COMPARISON_RESULT_LESS) {
										b_zero = -1;
									}
								}
							}
						}

						if(b_zero == 0 && b0_zero == 0) {

							// ((delta1 +-sqrt(delta1^2-4*delta0^3))/2)^(1/3)
							MathStructure mC;
							MathStructure *md0_43 = new MathStructure(mdelta0);
							md0_43->raise(nr_three);
							md0_43->multiply(Number(-4, 1, 0));
							MathStructure *md1_2 = new MathStructure(mdelta1);
							md1_2->raise(nr_two);
							md1_2->add_nocopy(md0_43);
							md1_2->raise(nr_half);
							if(b_neg) md1_2->calculateNegate(eo3);
							md1_2->calculatesub(eo3, eo, true);

							if(CALCULATOR->aborted()) {CALCULATOR->endTemporaryStopMessages(); if(stop_iv) CALCULATOR->endTemporaryStopIntervalArithmetic(); set(mbak); return false;}

							mC = mdelta1;
							mC.add_nocopy(md1_2);
							mC.calculateAddLast(eo3);
							mC.calculateDivide(nr_two, eo3);
							if(b_real == 0 && x_var.representsReal(true)) {
								if(!mC.representsComplex(true)) {
									mC.transform(STRUCT_FUNCTION);
									mC.addChild(nr_three);
									mC.setFunctionId(FUNCTION_ID_ROOT);
									if(mC.calculateFunctions(eo)) mC.calculatesub(eo3, eo, true);
									// x = -1/(3a)*(b + C + delta0/C)
									CHILD(0) = x_var;
									CHILD(1).set(-1, 3, 0);
									CHILD(1).calculateDivide(mstruct_a, eo3);
									MathStructure *malt1_2b = new MathStructure(mdelta0);
									malt1_2b->calculateDivide(mC, eo3);
									malt1_2b->calculateAdd(mC, eo3);
									malt1_2b->calculateAdd(mstruct_b, eo3);
									CHILD(1).multiply_nocopy(malt1_2b);
									CHILD(1).calculateMultiplyLast(eo3);
									CHILDREN_UPDATED;
									if(stop_iv) {
										CALCULATOR->endTemporaryStopIntervalArithmetic();
										CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
										fix_intervals(*this, eo2);
									} else if(containsInterval(true, false, false, -2, false)) {
										set(mbak);
										stop_iv = true;
										CALCULATOR->endTemporaryStopMessages();
										goto cube_eq_stop_iv;
									}
									CALCULATOR->endTemporaryStopMessages(true);
									return true;
								}
							} else if(eo3.allow_complex) {

								if(mC.representsNegative(true)) {
									mC.calculateNegate(eo3);
									mC.calculateRaise(Number(1, 3, 0), eo3);
									mC.calculateNegate(eo3);
								} else {
									mC.calculateRaise(Number(1, 3, 0), eo3);
								}

								// x = -1/(3a)*(b + C + delta0/C)

								MathStructure *malt1 = new MathStructure(x_var);
								malt1->transform(STRUCT_COMPARISON, Number(-1, 3, 0));
								malt1->setComparisonType(comparisonType());
								(*malt1)[1].calculateDivide(mstruct_a, eo3);

								MathStructure *malt2 = new MathStructure(*malt1);
								MathStructure *malt3 = new MathStructure(*malt1);

								MathStructure *malt1_2b = new MathStructure(mdelta0);
								malt1_2b->calculateDivide(mC, eo3);
								malt1_2b->calculateAdd(mC, eo3);
								malt1_2b->calculateAdd(mstruct_b, eo3);
								(*malt1)[1].multiply_nocopy(malt1_2b);
								(*malt1)[1].calculateMultiplyLast(eo3);

								MathStructure cbrt_mul(nr_three);
								cbrt_mul.calculateRaise(nr_half, eo3);
								cbrt_mul.calculateMultiply(nr_one_i, eo3);
								MathStructure cbrt_mul2(cbrt_mul);
								cbrt_mul.calculateMultiply(nr_half, eo3);
								cbrt_mul.calculateAdd(Number(-1, 2, 0), eo3);
								cbrt_mul2.calculateMultiply(Number(-1, 2, 0), eo3);
								cbrt_mul2.calculateAdd(Number(-1, 2, 0), eo3);

								MathStructure mC2(mC);
								mC2.calculateMultiply(cbrt_mul, eo3);
								MathStructure mC3(mC);
								mC3.calculateMultiply(cbrt_mul2, eo3);

								MathStructure *malt2_2b = new MathStructure(mdelta0);
								malt2_2b->calculateDivide(mC2, eo3);
								malt2_2b->calculateAdd(mC2, eo3);
								malt2_2b->calculateAdd(mstruct_b, eo3);
								(*malt2)[1].multiply_nocopy(malt2_2b);
								(*malt2)[1].calculateMultiplyLast(eo3);

								MathStructure *malt3_2b = new MathStructure(mdelta0);
								malt3_2b->calculateDivide(mC3, eo3);
								malt3_2b->calculateAdd(mC3, eo3);
								malt3_2b->calculateAdd(mstruct_b, eo3);
								(*malt3)[1].multiply_nocopy(malt3_2b);
								(*malt3)[1].calculateMultiplyLast(eo3);

								if(b_real == 1) {
									if((*malt1)[1].isNumber()) {
										(*malt1)[1].number().clearImaginary();
									} else if((*malt1)[1].isMultiplication() && (*malt1)[1][0].isNumber()) {
										bool b = true;
										for(size_t i = 1; i < (*malt1)[1].size(); i++) {
											if(!(*malt1)[1][i].representsReal(true)) {
												b = false;
												break;
											}
										}
										if(b) (*malt1)[1][0].number().clearImaginary();
									}
									if((*malt2)[1].isNumber()) {
										(*malt2)[1].number().clearImaginary();
									} else if((*malt2)[1].isMultiplication() && (*malt2)[1][0].isNumber()) {
										bool b = true;
										for(size_t i = 1; i < (*malt2)[1].size(); i++) {
											if(!(*malt2)[1][i].representsReal(true)) {
												b = false;
												break;
											}
										}
										if(b) (*malt2)[1][0].number().clearImaginary();
									}
									if((*malt3)[1].isNumber()) {
										(*malt3)[1].number().clearImaginary();
									} else if((*malt3)[1].isMultiplication() && (*malt3)[1][0].isNumber()) {
										bool b = true;
										for(size_t i = 1; i < (*malt3)[1].size(); i++) {
											if(!(*malt3)[1][i].representsReal(true)) {
												b = false;
												break;
											}
										}
										if(b) (*malt3)[1][0].number().clearImaginary();
									}
								}

								if(b_real < 1 || !x_var.representsReal(true) || (!(*malt1)[1].representsComplex(true) && !(*malt2)[1].representsComplex(true) && !(*malt3)[1].representsComplex(true))) {

									if(ct_comp == COMPARISON_NOT_EQUALS) {
										clear(true);
										setType(STRUCT_LOGICAL_AND);
									} else {
										clear(true);
										setType(STRUCT_LOGICAL_OR);
									}

									malt1->childUpdated(2);
									malt2->childUpdated(2);
									malt3->childUpdated(2);

									addChild_nocopy(malt1);
									addChild_nocopy(malt2);
									addChild_nocopy(malt3);

									calculatesub(eo3, eo, false);
									if(stop_iv) {
										CALCULATOR->endTemporaryStopIntervalArithmetic();
										CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
										fix_intervals(*this, eo2);
									} else if(containsInterval(true, false, false, -2, false)) {
										set(mbak);
										stop_iv = true;
										goto cube_eq_stop_iv;
										CALCULATOR->endTemporaryStopMessages();
									}
									CALCULATOR->endTemporaryStopMessages(true);
									return true;
								}
							}
						}
						CALCULATOR->endTemporaryStopMessages();
						if(stop_iv) CALCULATOR->endTemporaryStopIntervalArithmetic();
					}
				}
			}


			// abs(x)+x=a => -x+x=a || x+x=a; sgn(x)+x=a => -1+x=a || 0+x=a || 1+x=a
			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				MathStructure *m = find_abs_sgn(CHILD(0), x_var);
				if(m && m->function()->id() == FUNCTION_ID_ABS) {

					MathStructure mabs(*m);

					ComparisonType cmp_type = ct_comp;
					MathStructure *malt = new MathStructure(*this);
					MathStructure mabs_minus(mabs[0]);
					mabs_minus.calculateNegate(eo2);
					(*malt)[0].replace(mabs, mabs_minus, mabs.containsInterval());
					(*malt)[0].calculatesub(eo2, eo, true);
					malt->childUpdated(1);
					MathStructure *mcheck = new MathStructure(mabs[0]);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_EQUALS_GREATER : OPERATION_LESS);
					mcheck->isolate_x_sub(eo, eo2, x_var);
					malt->isolate_x_sub(eo, eo2, x_var);
					malt->add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					malt->calculatesub(eo2, eo, false);

					mcheck = new MathStructure(mabs[0]);
					CHILD(0).replace(mabs, mabs[0], mabs.containsInterval());
					CHILD(0).calculatesub(eo2, eo, true);
					CHILD_UPDATED(0)
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LESS : OPERATION_EQUALS_GREATER);
					mcheck->isolate_x_sub(eo, eo2, x_var);
					isolate_x_sub(eo, eo2, x_var);
					add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					calculatesub(eo2, eo, false);
					add_nocopy(malt, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
					calculatesub(eo2, eo, false);

					return true;
				} else if(m && m->function()->id() == FUNCTION_ID_SIGNUM) {
					MathStructure mabs(*m);

					ComparisonType cmp_type = ct_comp;
					MathStructure *malt = new MathStructure(*this);
					(*malt)[0].replace(mabs, m_minus_one, mabs.containsInterval());
					(*malt)[0].calculatesub(eo2, eo, true);
					malt->childUpdated(1);
					MathStructure *mcheck = new MathStructure(mabs[0]);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? (mabs[1].isMinusOne() ? OPERATION_GREATER : OPERATION_EQUALS_GREATER) : (mabs[1].isMinusOne() ? OPERATION_EQUALS_LESS : OPERATION_LESS));
					mcheck->isolate_x_sub(eo, eo2, x_var);
					malt->isolate_x_sub(eo, eo2, x_var);
					malt->add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					malt->calculatesub(eo2, eo, false);

					MathStructure *malt0 = NULL;
					if(!mabs[1].isOne() && !mabs[1].isMinusOne()) {
						malt0 = new MathStructure(*this);
						(*malt0)[0].replace(mabs, mabs[1], mabs.containsInterval());
						(*malt0)[0].calculatesub(eo2, eo, true);
						malt0->childUpdated(1);
						mcheck = new MathStructure(mabs[0]);
						mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_NOT_EQUALS : OPERATION_EQUALS);
						mcheck->isolate_x_sub(eo, eo2, x_var);
						malt0->isolate_x_sub(eo, eo2, x_var);
						malt0->add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
						malt0->calculatesub(eo2, eo, false);
					}

					mcheck = new MathStructure(mabs[0]);
					CHILD(0).replace(mabs, m_one, mabs.containsInterval());
					CHILD(0).calculatesub(eo2, eo, true);
					CHILD_UPDATED(0)
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? (mabs[1].isOne() ? OPERATION_LESS : OPERATION_EQUALS_LESS) : (mabs[1].isOne() ? OPERATION_EQUALS_GREATER : OPERATION_GREATER));
					mcheck->isolate_x_sub(eo, eo2, x_var);
					isolate_x_sub(eo, eo2, x_var);
					add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					calculatesub(eo2, eo, false);
					add_nocopy(malt, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
					if(malt0) add_nocopy(malt0, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
					calculatesub(eo2, eo, false);
					return true;
				}
			}

			// Use newton raphson to calculate approximate solution for polynomial
			MathStructure x_value;
			if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(1).isNumber() && eo.approximation != APPROXIMATION_EXACT && !x_var.representsComplex(true)) {
				MathStructure mtest(CHILD(0));
				if(!CHILD(0).isZero()) mtest.calculateSubtract(CHILD(1), eo2);
				CALCULATOR->beginTemporaryStopIntervalArithmetic();
				EvaluationOptions eo3 = eo2;
				eo3.interval_calculation = INTERVAL_CALCULATION_NONE;
				bool failed = false;
				if(CALCULATOR->usesIntervalArithmetic()) failed = true;
				else fix_intervals(mtest, eo3, &failed);
				if(failed) {
					CALCULATOR->endTemporaryStopIntervalArithmetic();
				} else {
					MathStructure mbak(*this);
					int ret = -1;
					eo3.approximation = APPROXIMATION_APPROXIMATE;
					if(ct_comp == COMPARISON_EQUALS) clear(true);
					else set(1, 1, 0, true);
					while((ret = newton_raphson(mtest, x_value, x_var, eo3)) > 0) {
						if(isNumber()) {
							set(x_var, true);
							transform(STRUCT_COMPARISON, x_value);
							setComparisonType(mbak.comparisonType());
						} else {
							if(isComparison()) transform(mbak.comparisonType() == COMPARISON_NOT_EQUALS ? STRUCT_LOGICAL_AND : STRUCT_LOGICAL_OR);
							MathStructure *mnew = new MathStructure(x_var);
							mnew->transform(STRUCT_COMPARISON, x_value);
							mnew->setComparisonType(mbak.comparisonType());
							addChild_nocopy(mnew);
						}
						MathStructure mdiv(x_var);
						mdiv.calculateSubtract(x_value, eo3);
						MathStructure mtestcopy(mtest);
						MathStructure mrem;
						if(!polynomial_long_division(mtestcopy, mdiv, x_var, mtest, mrem, eo3, false, true) || !mrem.isNumber()) {
							ret = -1;
							break;
						}
						if(!mtest.contains(x_var)) break;
					}
					CALCULATOR->endTemporaryStopIntervalArithmetic();
					if(ret < 0) {
						set(mbak);
					} else {
						if(!x_var.representsReal()) CALCULATOR->error(false, _("Not all complex roots were calculated for %s."), format_and_print(mbak).c_str(), NULL);
						if((CALCULATOR->usesIntervalArithmetic() && eo.approximation != APPROXIMATION_EXACT) || mbak.containsInterval()) CALCULATOR->error(false, _("Interval arithmetic was disabled during calculation of %s."), format_and_print(mbak).c_str(), NULL);
						fix_intervals(*this, eo2);
						return true;
					}
				}
			}

			// Try factorization
			if((!morig || !equals(*morig)) && !CHILD(1).isZero()) {
				MathStructure mtest(*this);
				if(mtest[0].factorize(eo2, false, false, 0, false, false, NULL, m_undefined, false, false, 3) && !(mtest[0].isMultiplication() && mtest[0].size() == 2 && (mtest[0][0].isNumber() || mtest[0][0] == CHILD(1) || mtest[0][1] == CHILD(1)))) {
					mtest.childUpdated(1);
					if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
						set_nocopy(mtest);
						return true;
					}
				}
			}

			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(!representsNonMatrix()) return false;
			//x*y=0 => x=0 || y=0
			if(CHILD(1).isZero()) {
				if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
					vector<int> checktype;
					bool bdoit = false;
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CALCULATOR->aborted()) return false;
						if(CHILD(0)[i].isPower() && !CHILD(0)[i][1].representsNonNegative()) {
							if(CHILD(0)[i][1].representsNegative()) {
								checktype.push_back(1);
							} else {
								checktype.push_back(2);
								bdoit = true;
							}
						} else {
							checktype.push_back(0);
							bdoit = true;
						}
					}
					if(!bdoit) break;
					MathStructure *mcheckpowers = NULL;
					ComparisonType ct = ct_comp;
					setToChild(1);
					if(ct == COMPARISON_NOT_EQUALS) {
						setType(STRUCT_LOGICAL_AND);
					} else {
						setType(STRUCT_LOGICAL_OR);
					}
					MathStructure mbak(*this);
					for(size_t i = 0; i < SIZE;) {
						if(CALCULATOR->aborted()) {
							set(mbak);
							if(mcheckpowers) mcheckpowers->unref();
							return false;
						}
						if(checktype[i] > 0) {
							MathStructure *mcheck = new MathStructure(CHILD(i)[0]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_EQUALS);
							else mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck->isolate_x_sub(eo, eo2, x_var);
							if(checktype[i] == 2) {
								MathStructure *mexpcheck = new MathStructure(CHILD(i)[1]);
								if(ct_comp == COMPARISON_NOT_EQUALS) mexpcheck->add(m_zero, OPERATION_LESS);
								else mexpcheck->add(m_zero, OPERATION_EQUALS_GREATER);
								mexpcheck->isolate_x_sub(eo, eo2, x_var);
								if(ct_comp == COMPARISON_NOT_EQUALS) mexpcheck->add_nocopy(mcheck, OPERATION_LOGICAL_AND, true);
								else mexpcheck->add_nocopy(mcheck, OPERATION_LOGICAL_OR, true);
								mexpcheck->calculatesub(eo2, eo, false);
								mcheck = mexpcheck;
							}
							if(mcheckpowers) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheckpowers->add_nocopy(mcheck, OPERATION_LOGICAL_OR, true);
								else mcheckpowers->add_nocopy(mcheck, OPERATION_LOGICAL_AND, true);
							} else {
								mcheckpowers = mcheck;
							}
						}
						if(checktype[i] == 1) {
							ERASE(i)
							checktype.erase(checktype.begin() + i);
						} else {
							CHILD(i).transform(STRUCT_COMPARISON, m_zero);
							CHILD(i).setComparisonType(ct);
							CHILD(i).isolate_x_sub(eo, eo2, x_var, morig);
							CHILD_UPDATED(i)
							i++;
						}
					}
					if(SIZE == 1) setToChild(1);
					else if(SIZE == 0) clear(true);
					else calculatesub(eo2, eo, false);
					if(mcheckpowers) {
						mcheckpowers->calculatesub(eo2, eo, false);
						if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(mcheckpowers, OPERATION_LOGICAL_OR, true);
						else add_nocopy(mcheckpowers, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
						calculatesub(eo2, eo, false);
					}
					return true;
				} else if(CHILD(0).size() >= 2) {
					MathStructure mless1(CHILD(0)[0]);
					MathStructure mless2;
					if(CHILD(0).size() == 2) {
						mless2 = CHILD(0)[1];
					} else {
						mless2 = CHILD(0);
						mless2.delChild(1);
					}

					int checktype1 = 0, checktype2 = 0;
					MathStructure *mcheck1 = NULL, *mcheck2 = NULL;
					if(mless1.isPower() && !mless1[1].representsNonNegative()) {
						if(mless1[1].representsNegative()) {
							checktype1 = 1;
						} else if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER) {
							checktype1 = 2;
							mcheck1 = new MathStructure(mless1[1]);
							mcheck1->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheck1->isolate_x_sub(eo, eo2, x_var);
							MathStructure *mcheck = new MathStructure(mless1[0]);
							mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck->isolate_x_sub(eo, eo2, x_var);
							mcheck1->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							mcheck1->calculatesub(eo2, eo, false);
						}
					}
					if(mless2.isPower() && !mless2[1].representsNonNegative()) {
						if(mless2[1].representsNegative()) {
							checktype2 = 1;
						} else if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER) {
							checktype2 = 2;
							mcheck2 = new MathStructure(mless2[1]);
							mcheck2->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheck2->isolate_x_sub(eo, eo2, x_var);
							MathStructure *mcheck = new MathStructure(mless2[0]);
							mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck->isolate_x_sub(eo, eo2, x_var);
							mcheck2->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							mcheck2->calculatesub(eo2, eo, false);
						}
					}

					mless1.transform(STRUCT_COMPARISON, m_zero);
					if(checktype1 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mless1.setComparisonType(COMPARISON_EQUALS_LESS);
					} else {
						mless1.setComparisonType(COMPARISON_LESS);
					}
					mless1.isolate_x_sub(eo, eo2, x_var);
					mless2.transform(STRUCT_COMPARISON, m_zero);
					mless2.setComparisonType(COMPARISON_LESS);
					if(checktype2 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mless2.setComparisonType(COMPARISON_EQUALS_LESS);
					} else {
						mless2.setComparisonType(COMPARISON_LESS);
					}
					mless2.isolate_x_sub(eo, eo2, x_var);
					MathStructure mgreater1(CHILD(0)[0]);
					mgreater1.transform(STRUCT_COMPARISON, m_zero);
					if(checktype1 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mgreater1.setComparisonType(COMPARISON_EQUALS_GREATER);
					} else {
						mgreater1.setComparisonType(COMPARISON_GREATER);
					}
					mgreater1.isolate_x_sub(eo, eo2, x_var);
					MathStructure mgreater2;
					if(CHILD(0).size() == 2) {
						mgreater2 = CHILD(0)[1];
					} else {
						mgreater2 = CHILD(0);
						mgreater2.delChild(1);
					}
					mgreater2.transform(STRUCT_COMPARISON, m_zero);
					if(checktype2 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mgreater2.setComparisonType(COMPARISON_EQUALS_GREATER);
					} else {
						mgreater2.setComparisonType(COMPARISON_GREATER);
					}
					mgreater2.isolate_x_sub(eo, eo2, x_var);
					clear();

					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						set(mless1);
						transform(STRUCT_LOGICAL_AND, mgreater2);
						calculatesub(eo2, eo, false);
						transform(STRUCT_LOGICAL_OR, mless2);
						CHILD(1).transform(STRUCT_LOGICAL_AND, mgreater1);
						CHILD(1).calculatesub(eo2, eo, false);
						CHILD_UPDATED(1)
						calculatesub(eo2, eo, false);
					} else {
						set(mless1);
						transform(STRUCT_LOGICAL_AND, mless2);
						calculatesub(eo2, eo, false);
						transform(STRUCT_LOGICAL_OR, mgreater1);
						CHILD(1).transform(STRUCT_LOGICAL_AND, mgreater2);
						CHILD(1).calculatesub(eo2, eo, false);
						CHILD_UPDATED(1)
						calculatesub(eo2, eo, false);
					}
					if(checktype1 == 2) {
						add_nocopy(mcheck1, OPERATION_LOGICAL_AND, true);
						calculatesub(eo2, eo, false);
					}
					if(checktype2 == 2) {
						add_nocopy(mcheck2, OPERATION_LOGICAL_AND, true);
						calculatesub(eo2, eo, false);
					}
					return true;

				}
			}
			// x/(x+a)=b => x=b(x+a)
			for(size_t i = 0; i < CHILD(0).size(); i++) {
				if(CALCULATOR->aborted()) return false;
				if(CHILD(0)[i].isPower() && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number().isNegative() && CHILD(0)[i][1].number().isReal()) {
					MathStructure *mtest;
					if(CHILD(0)[i][1].number().isMinusOne()) {
						CHILD(1).multiply(CHILD(0)[i][0]);
						mtest = new MathStructure(CHILD(0)[i][0]);
					} else {
						CHILD(0)[i][1].number().negate();
						CHILD(1).multiply(CHILD(0)[i]);
						mtest = new MathStructure(CHILD(0)[i]);
					}
					CHILD(0).delChild(i + 1);
					if(CHILD(0).size() == 1) CHILD(0).setToChild(1);
					CHILD(0).calculateSubtract(CHILD(1), eo);
					ComparisonType cmp_type = ct_comp;
					CHILD(1).clear();
					CHILDREN_UPDATED
					if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
						isolate_x_sub(eo, eo2, x_var, morig);
						mtest->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_EQUALS : OPERATION_NOT_EQUALS);
						mtest->isolate_x_sub(eo, eo2, x_var);
						add_nocopy(mtest, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
						calculatesub(eo2, eo, false);
						return true;
					} else {
						MathStructure *malt = new MathStructure(*this);
						if(ct_comp == COMPARISON_EQUALS_GREATER) {
							ct_comp = COMPARISON_EQUALS_LESS;
						} else if(ct_comp == COMPARISON_GREATER) {
							ct_comp = COMPARISON_LESS;
						} else if(ct_comp == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_EQUALS_GREATER;
						} else if(ct_comp == COMPARISON_LESS) {
							ct_comp = COMPARISON_GREATER;
						}
						isolate_x_sub(eo, eo2, x_var, morig);
						mtest->add(m_zero, OPERATION_LESS);
						MathStructure *mtest_alt = new MathStructure(*mtest);
						mtest_alt->setComparisonType(COMPARISON_GREATER);
						mtest->isolate_x_sub(eo, eo2, x_var);
						add_nocopy(mtest, OPERATION_LOGICAL_AND);
						calculatesub(eo2, eo, false);
						malt->isolate_x_sub(eo, eo2, x_var, morig);
						mtest_alt->isolate_x_sub(eo, eo2, x_var);
						malt->add_nocopy(mtest_alt, OPERATION_LOGICAL_AND);
						malt->calculatesub(eo2, eo, false);
						add_nocopy(malt, OPERATION_LOGICAL_OR);
						calculatesub(eo2, eo, false);
						return true;
					}
				}
			}
			bool b = false;
			int zero1;
			if(CHILD(1).isZero()) zero1 = 1;
			else if(CHILD(1).representsNonZero(true)) zero1 = 0;
			else zero1 = 2;
			MathStructure *mcheckmulti = NULL, *mtryzero = NULL, *mchecknegative = NULL, *mchecknonzeropow = NULL;
			MathStructure mchild2(CHILD(1));
			MathStructure mbak(*this);
			ComparisonType ct_orig = ct_comp;
			for(size_t i = 0; i < CHILD(0).size(); i++) {
				if(CALCULATOR->aborted()) {
					set(mbak);
					if(mcheckmulti) mcheckmulti->unref();
					if(mtryzero) mcheckmulti->unref();
					if(mchecknegative) mcheckmulti->unref();
					if(mchecknonzeropow) mcheckmulti->unref();
					return false;
				}
				if(!CHILD(0)[i].contains(x_var)) {
					bool nonzero = false;
					if(CHILD(0)[i].isPower() && !CHILD(0)[i][1].representsNonNegative(true) && !CHILD(0)[i][0].representsNonZero(true)) {
						MathStructure *mcheck = new MathStructure(CHILD(0)[i][0]);
						if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_EQUALS);
						else mcheck->add(m_zero, OPERATION_NOT_EQUALS);
						mcheck->isolate_x(eo2, eo);
						if(CHILD(0)[i][1].representsNegative(true)) {
							nonzero = true;
						} else {
							MathStructure *mcheckor = new MathStructure(CHILD(0)[i][1]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheckor->add(m_zero, OPERATION_EQUALS_LESS);
							else mcheckor->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheckor->isolate_x(eo2, eo);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add_nocopy(mcheckor, OPERATION_LOGICAL_AND);
							else mcheck->add_nocopy(mcheckor, OPERATION_LOGICAL_OR);
							mcheck->calculatesub(eo2, eo, false);
						}
						if(mchecknonzeropow) {
							if(ct_comp == COMPARISON_NOT_EQUALS) mchecknonzeropow->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							else mchecknonzeropow->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
						} else {
							mchecknonzeropow = mcheck;
						}
					}
					if(!nonzero && !CHILD(0)[i].representsNonZero(true)) {
						if(zero1 != 1 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
							MathStructure *mcheck = new MathStructure(CHILD(0)[i]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_EQUALS);
							else mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck->isolate_x(eo2, eo);
							if(mcheckmulti) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheckmulti->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
								else mcheckmulti->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
							} else {
								mcheckmulti = mcheck;
							}
						}
						if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
							if(!mtryzero) {
								mtryzero = new MathStructure(CHILD(0)[i]);
								mtryzero->add(m_zero, OPERATION_EQUALS);
								mtryzero->isolate_x(eo2, eo);
								MathStructure *mcheck = new MathStructure(mchild2);
								switch(ct_orig) {
									case COMPARISON_LESS: {mcheck->add(m_zero, OPERATION_GREATER); break;}
									case COMPARISON_GREATER: {mcheck->add(m_zero, OPERATION_LESS); break;}
									case COMPARISON_EQUALS_LESS: {mcheck->add(m_zero, OPERATION_EQUALS_GREATER); break;}
									case COMPARISON_EQUALS_GREATER: {mcheck->add(m_zero, OPERATION_EQUALS_LESS); break;}
									default: {}
								}
								mcheck->isolate_x(eo2, eo);
								mtryzero->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
							} else {
								MathStructure *mcheck = new MathStructure(CHILD(0)[i]);
								mcheck->add(m_zero, OPERATION_EQUALS);
								mcheck->isolate_x(eo2, eo);
								(*mtryzero)[0].add_nocopy(mcheck, OPERATION_LOGICAL_OR);
								mtryzero[0].calculateLogicalOrLast(eo2);
							}
						} else if(zero1 > 0) {
							MathStructure *mcheck = new MathStructure(CHILD(0)[i]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							else mcheck->add(m_zero, OPERATION_EQUALS);
							mcheck->isolate_x(eo2, eo);
							if(zero1 == 2) {
								MathStructure *mcheck2 = new MathStructure(mchild2);
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheck2->add(m_zero, OPERATION_NOT_EQUALS);
								else mcheck2->add(m_zero, OPERATION_EQUALS);
								mcheck2->isolate_x(eo2, eo);
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheck2->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
								else mcheck2->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
								mcheck2->calculatesub(eo2, eo, false);
								mcheck = mcheck2;
							}
							if(mtryzero) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mtryzero->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
								else mtryzero->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							} else {
								mtryzero = mcheck;
							}
						}
					}
					if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
						if(CHILD(0)[i].representsNegative()) {
							switch(ct_comp) {
								case COMPARISON_LESS: {ct_comp = COMPARISON_GREATER; break;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_LESS; break;}
								case COMPARISON_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
								case COMPARISON_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_LESS; break;}
								default: {}
							}
						} else if(!CHILD(0)[i].representsNonNegative()) {
							if(mchecknegative) {
								mchecknegative->multiply(CHILD(0)[i]);
							} else {
								mchecknegative = new MathStructure(CHILD(0)[i]);
							}
						}
					}
					if(zero1 != 1) {
						CHILD(1).calculateDivide(CHILD(0)[i], eo2);
						CHILD_UPDATED(1);
					}
					CHILD(0).delChild(i + 1);
					b = true;
				}
			}
			if(combine_powers(*this, x_var, eo2, eo) && m_type != STRUCT_MULTIPLICATION) return isolate_x_sub(eo, eo2, x_var, morig);
			if(!b && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(0).size() >= 2) {
				// (x+a)*b^(c*x)=d => x=(lambertw(b^(a*c)*c*d*ln(b))-a*c*ln(b)))/(c*ln(b))
				if(CALCULATOR->aborted()) {
					set(mbak);
					if(mcheckmulti) mcheckmulti->unref();
					if(mtryzero) mcheckmulti->unref();
					if(mchecknegative) mcheckmulti->unref();
					if(mchecknonzeropow) mcheckmulti->unref();
					return false;
				}
				size_t i_px = 0;
				MathStructure *mvar = NULL;
				MathStructure m_c;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isPower() && CHILD(0)[i][1].contains(x_var) && !CHILD(0)[i][0].contains(x_var)) {
						mvar = find_mvar(CHILD(0)[i][1], x_var, m_c);
						if(mvar) i_px = i;
						else break;
					}
				}
				if(mvar) {
					size_t i_x = 0;
					bool found_mvar = false;
					MathStructure mmul(1, 1, 0), m_a;
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(i != i_px) {
							if(CHILD(0)[i].contains(x_var)) {
								if(!found_mvar && CHILD(0)[i].equals(*mvar)) {
									found_mvar = true;
									i_x = i;
								} else if(!found_mvar && CHILD(0)[i].isAddition()) {
									i_x = i;
									for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
										if(CHILD(0)[i][i2].contains(*mvar)) {
											if(!found_mvar && CHILD(0)[i][i2].equals(*mvar)) {
												found_mvar = true;
											} else if(!found_mvar && CHILD(0)[i][i2].isMultiplication()) {
												for(size_t i3 = 0; i3 < CHILD(0)[i][i2].size(); i3++) {
													if(CHILD(0)[i][i2][i3].equals(*mvar)) {
														mmul = CHILD(0)[i][i2];
														mmul.delChild(i3 + 1, true);
														break;
													}
												}
												if(mmul.contains(*mvar)) {
													found_mvar = false;
													break;
												}
											} else {
												found_mvar = false;
												break;
											}
											m_a = CHILD(0)[i];
											m_a.delChild(i2 + 1, true);
											if(!mmul.isOne()) m_a.calculateDivide(mmul, eo2);
										}
									}
									if(!found_mvar) break;
								} else {
									found_mvar = false;
									break;
								}
							}
						}
					}
					if(found_mvar) {
						MathStructure mln(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0)[i_px][0], NULL);
						if(mln.calculateFunctions(eo)) mln.calculatesub(eo2, eo, true);
						MathStructure *marg = NULL;
						if(mln.representsNonZero()) {
							marg = new MathStructure(mln);
							if(comparison_might_be_equal(m_c.compare(m_zero))) {marg->unref(); marg = NULL;}
							else if(!m_c.isOne()) marg->calculateMultiply(m_c, eo2);
						}
						if(marg) {
							marg->calculateMultiply(CHILD(1), eo2);
							if(CHILD(0).size() > 2) {
								marg->multiply(CHILD(0));
								marg->last().delChild(i_x + 1);
								marg->last().delChild(i_px + (i_px < i_x ? 1 : 0), true);
								if(!marg->last().representsNonZero()) {
									marg->unref(); marg = NULL;
								} else {
									marg->last().calculateRaise(nr_minus_one, eo2);
									marg->calculateMultiplyLast(eo2);
								}
							}
							if(!mmul.isOne()) marg->calculateDivide(mmul, eo2);

						}
						if(marg && !m_a.isZero()) {
							marg->multiply(CHILD(0)[i_px][0]);
							marg->last().raise(m_a);
							if(!m_c.isOne()) marg->last().last().calculateMultiply(m_c, eo2);
							marg->last().calculateRaiseExponent(eo2);
							marg->calculateMultiplyLast(eo2);
						}
						if(marg && mvar->representsNonComplex() && CHILD(0)[i_px][0].representsPositive()) {
							if(mcheckmulti) mcheckmulti->unref();
							if(mtryzero) mcheckmulti->unref();
							if(mchecknegative) mcheckmulti->unref();
							if(mchecknonzeropow) mcheckmulti->unref();
							if(marg->representsComplex()) {
								marg->unref();
								if(ct_comp == COMPARISON_EQUALS) clear(true);
								else set(1, 1, 0, true);
								return true;
							}
							MathStructure *mreq1 = NULL;
							MathStructure *marg2 = NULL;
							MathStructure *mreq2 = NULL;

							if(m_a == CHILD(1)) {
								marg2 = new MathStructure(*mvar);
								marg2->transform(ct_comp, m_zero);
								marg2->calculatesub(eo2, eo, false);
							}
							if(!m_a.isZero()) {
								if(!m_c.isOne()) m_a.calculateMultiply(m_c, eo2);
								m_a.calculateMultiply(mln, eo2);
							}
							if(!marg2 && !marg->representsNonNegative()) {
								mreq1 = new MathStructure(*marg);
								mreq2 = new MathStructure(*marg);
								marg2 = new MathStructure(*marg);
								marg2->transformById(FUNCTION_ID_LAMBERT_W);
								marg2->addChild(m_minus_one);
								if(marg2->calculateFunctions(eo)) marg2->calculatesub(eo2, eo, true);
								if(!m_a.isZero()) marg2->calculateSubtract(m_a, eo2);
								if(!m_c.isOne()) marg2->calculateDivide(m_c, eo2);
								marg2->calculateDivide(mln, eo2);
								mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_E));
								mreq1->last().calculateRaise(m_minus_one, eo2);
								mreq1->last().calculateNegate(eo2);
								mreq1->childUpdated(2);
								mreq1->isolate_x(eo2, eo);
								mreq2->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, m_zero);
								mreq2->isolate_x(eo2, eo);
								marg2->transform(ct_comp, *mvar);
								marg2->swapChildren(1, 2);
								marg2->isolate_x_sub(eo, eo2, x_var, morig);
								marg2->add(*mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								marg2->add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
								marg2->calculatesub(eo2, eo, false);
							}
							marg->transformById(FUNCTION_ID_LAMBERT_W);
							marg->addChild(m_zero);
							if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
							if(!m_a.isZero()) marg->calculateSubtract(m_a, eo2);
							if(!m_c.isOne()) marg->calculateDivide(m_c, eo2);
							marg->calculateDivide(mln, eo2);
							setChild_nocopy(marg, 2, true);
							mvar->ref();
							CHILD(0).clear(true);
							setChild_nocopy(mvar, 1, true);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							if(mreq1) {
								add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								calculatesub(eo2, eo, false);
							}
							if(marg2) {
								add_nocopy(marg2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								calculatesub(eo2, eo, false);
							}
							return true;
						} else if(marg) {
							if(mcheckmulti) mcheckmulti->unref();
							if(mtryzero) mcheckmulti->unref();
							if(mchecknegative) mcheckmulti->unref();
							if(mchecknonzeropow) mcheckmulti->unref();
							marg->transformById(FUNCTION_ID_LAMBERT_W);
							marg->addChild(CALCULATOR->getVariableById(VARIABLE_ID_N));
							if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
							if(!m_a.isZero()) {
								m_a.calculateMultiply(mln, eo2);
								marg->calculateSubtract(m_a, eo2);
							}
							if(!m_c.isOne()) marg->calculateDivide(m_c, eo2);
							marg->calculateDivide(mln, eo2);
							setChild_nocopy(marg, 2, true);
							mvar->ref();
							CHILD(0).clear(true);
							setChild_nocopy(mvar, 1, true);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						}
					}
				}
				if(CHILD(0).size() == 2) {
					// x*ln(x)=a => e^lambertw(a)
					bool b_swap = false;
					if(CHILD(0)[0].isFunction() && CHILD(0)[0].function()->id() == FUNCTION_ID_LOG && CHILD(0)[0].size() == 1) {
						CHILD(0).swapChildren(1, 2);
						b_swap = true;
					}
					if(CHILD(0)[1].isFunction() && CHILD(0)[1].function()->id() == FUNCTION_ID_LOG && CHILD(0)[1].size() == 1) {
						MathStructure mmul(1, 1, 0), mexp(1, 1, 0);
						mvar = NULL;
						if(CHILD(0)[0] == CHILD(0)[1][0]) {
							mvar = &CHILD(0)[0];
						} else if(CHILD(0)[0].isPower() && CHILD(0)[0][0] == CHILD(0)[1][0]) {
							mexp = CHILD(0)[0][1];
							mvar = &CHILD(0)[0][0];
						} else {
							if(get_multiplier(CHILD(0)[1][0], CHILD(0)[0], mmul)) {
								mvar = &CHILD(0)[0];
							} else if(CHILD(0)[0].isPower() && get_multiplier(CHILD(0)[1][0], CHILD(0)[0][0], mmul)) {
								mexp = CHILD(0)[0][1];
								mvar = &CHILD(0)[0][0];
							}

						}
						if(mvar && mexp.isInteger() && (mexp.number().isOne() || mexp.number().isTwo()) && mmul.representsNonZero() && !mmul.contains(x_var)) {
							if(mcheckmulti) mcheckmulti->unref();
							if(mtryzero) mcheckmulti->unref();
							if(mchecknegative) mcheckmulti->unref();
							if(mchecknonzeropow) mcheckmulti->unref();
							MathStructure *marg;
							if(mexp.isOne() || mmul.isOne()) {
								marg = new MathStructure(CHILD(1));
								if(!mmul.isOne()) marg->calculateMultiply(mmul, eo2);
								else if(!mexp.isOne()) marg->calculateMultiply(mexp, eo2);
							} else {
								marg = new MathStructure(mmul);
								marg->calculateRaise(mexp, eo2);
								marg->calculateMultiply(CHILD(1), eo2);
								marg->calculateMultiply(mexp, eo2);
							}
							if(mvar->representsNonComplex()) {
								MathStructure *mreq1 = NULL;
								MathStructure *marg2 = NULL;
								MathStructure *mreq2 = NULL;
								if(!marg->representsPositive()) {
									mreq1 = new MathStructure(*marg);
									mreq2 = new MathStructure(*marg);
									marg2 = new MathStructure(*marg);
									marg2->transformById(FUNCTION_ID_LAMBERT_W);
									marg2->addChild(m_minus_one);
									if(marg2->calculateFunctions(eo)) marg2->calculatesub(eo2, eo, true);
									if(!mexp.isOne()) marg2->calculateDivide(mexp, eo2);
									marg2->transform(STRUCT_POWER);
									marg2->insertChild(CALCULATOR->getVariableById(VARIABLE_ID_E), 1);
									(*marg2)[0].calculatesub(eo2, eo, true);
									marg2->calculateRaiseExponent(eo2);
									if(!mmul.isOne()) marg2->calculateDivide(mmul, eo2);
									mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_E));
									mreq1->last().calculateRaise(m_minus_one, eo2);
									mreq1->last().calculateNegate(eo2);
									mreq1->childUpdated(2);
									mreq1->isolate_x(eo2, eo);
									mreq2->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, m_zero);
									mreq2->isolate_x(eo2, eo);
									marg2->transform(ct_comp, *mvar);
									marg2->swapChildren(1, 2);
									marg2->isolate_x_sub(eo, eo2, x_var, morig);
									marg2->add(*mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
									marg2->add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
									marg2->calculatesub(eo2, eo, false);
								}
								marg->transformById(FUNCTION_ID_LAMBERT_W);
								marg->addChild(m_zero);
								if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
								if(!mexp.isOne()) marg->calculateDivide(mexp, eo2);
								CHILD(0).setToChild(1, true);
								if(!mexp.isOne()) CHILD(0).setToChild(1, true);
								CHILD(1).set(CALCULATOR->getVariableById(VARIABLE_ID_E));
								CHILD(1).raise_nocopy(marg);
								CHILD(1).calculateRaiseExponent(eo2);
								if(!mmul.isOne()) CHILD(1).calculateDivide(mmul, eo2);
								CHILDREN_UPDATED
								isolate_x_sub(eo, eo2, x_var, morig);
								if(mreq1) {
									add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
									calculatesub(eo2, eo, false);
								}
								if(marg2) {
									add_nocopy(marg2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
									calculatesub(eo2, eo, false);
								}
							} else {
								if(!mexp.isOne()) marg->calculateMultiply(mexp, eo2);
								if(!mmul.isOne()) marg->calculateDivide(mmul, eo2);
								MathStructure *marg2 = new MathStructure(*marg);
								MathStructure *marg3 = new MathStructure(*marg);
								marg2->transformById(FUNCTION_ID_LAMBERT_W);
								marg2->addChild(m_minus_one);
								if(marg2->calculateFunctions(eo)) marg2->calculatesub(eo2, eo, true);
								if(!mexp.isOne()) marg2->calculateDivide(mexp, eo2);
								marg2->transform(STRUCT_POWER);
								marg2->insertChild(CALCULATOR->getVariableById(VARIABLE_ID_E), 1);
								(*marg2)[0].calculatesub(eo2, eo, true);
								marg2->calculateRaiseExponent(eo2);
								if(!mmul.isOne()) marg2->calculateDivide(mmul, eo2);
								marg3->transformById(FUNCTION_ID_LAMBERT_W);
								marg3->addChild(m_one);
								if(marg3->calculateFunctions(eo)) marg3->calculatesub(eo2, eo, true);
								if(!mexp.isOne()) marg3->calculateDivide(mexp, eo2);
								marg3->transform(STRUCT_POWER);
								marg3->insertChild(CALCULATOR->getVariableById(VARIABLE_ID_E), 1);
								(*marg3)[0].calculatesub(eo2, eo, true);
								marg3->calculateRaiseExponent(eo2);
								if(!mmul.isOne()) marg3->calculateDivide(mmul, eo2);
								marg2->transform(ct_comp, *mvar);
								marg2->swapChildren(1, 2);
								marg2->isolate_x_sub(eo, eo2, x_var, morig);
								marg3->transform(ct_comp, *mvar);
								marg3->swapChildren(1, 2);
								marg3->isolate_x_sub(eo, eo2, x_var, morig);
								marg->transformById(FUNCTION_ID_LAMBERT_W);
								marg->addChild(m_zero);
								if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
								if(!mexp.isOne()) marg->calculateDivide(mexp, eo2);
								CHILD(0).setToChild(1, true);
								if(!mexp.isOne()) CHILD(0).setToChild(1, true);
								CHILD(1).set(CALCULATOR->getVariableById(VARIABLE_ID_E));
								CHILD(1).raise_nocopy(marg);
								CHILD(1).calculateRaiseExponent(eo2);
								if(!mmul.isOne()) CHILD(1).calculateDivide(mmul, eo2);
								CHILDREN_UPDATED
								isolate_x_sub(eo, eo2, x_var, morig);
								add_nocopy(marg2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								add_nocopy(marg3, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
								calculatesub(eo2, eo, false);
							}
							return true;
						}
					}
					if(b_swap) CHILD(0).swapChildren(1, 2);
				}
			}
			if(b) {
				if(CHILD(0).size() == 1) {
					CHILD(0).setToChild(1);
				}
				if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(1).contains(x_var)) {
					CHILD(0).calculateSubtract(CHILD(1), eo2);
					CHILD(1).clear();
				}
				if(mchecknegative) {
					MathStructure *mneg = new MathStructure(*this);
					switch(ct_comp) {
						case COMPARISON_LESS: {mneg->setComparisonType(COMPARISON_GREATER); break;}
						case COMPARISON_GREATER: {mneg->setComparisonType(COMPARISON_LESS); break;}
						case COMPARISON_EQUALS_LESS: {mneg->setComparisonType(COMPARISON_EQUALS_GREATER); break;}
						case COMPARISON_EQUALS_GREATER: {mneg->setComparisonType(COMPARISON_EQUALS_LESS); break;}
						default: {}
					}
					isolate_x_sub(eo, eo2, x_var, morig);
					//mneg->isolate_x_sub(eo, eo2, x_var);
					mchecknegative->add(m_zero, OPERATION_GREATER);
					add(*mchecknegative, OPERATION_LOGICAL_AND, true);
					SWAP_CHILDREN(0, SIZE - 1)
					calculatesub(eo2, eo, false);
					//LAST.isolate_x(eo2, eo);
					mchecknegative->setComparisonType(COMPARISON_LESS);
					mchecknegative->isolate_x(eo2, eo);
					mneg->add_nocopy(mchecknegative, OPERATION_LOGICAL_AND, true);
					mneg->calculatesub(eo2, eo, false);
					add_nocopy(mneg, OPERATION_LOGICAL_OR, true);
					calculatesub(eo2, eo, false);
				} else {
					isolate_x_sub(eo, eo2, x_var, morig);
				}
				if(mcheckmulti) {
					mcheckmulti->calculatesub(eo2, eo, false);
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						add_nocopy(mcheckmulti, OPERATION_LOGICAL_OR, true);
					} else {
						add_nocopy(mcheckmulti, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
					}
					calculatesub(eo2, eo, false);
				}
				if(mchecknonzeropow) {
					mchecknonzeropow->calculatesub(eo2, eo, false);
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						add_nocopy(mchecknonzeropow, OPERATION_LOGICAL_OR, true);
					} else {
						add_nocopy(mchecknonzeropow, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
					}
					calculatesub(eo2, eo, false);
				}
				if(mtryzero) {
					mtryzero->calculatesub(eo2, eo, false);
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						add_nocopy(mtryzero, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
					} else {
						add_nocopy(mtryzero, OPERATION_LOGICAL_OR, true);
					}
					calculatesub(eo2, eo, false);
				}
				return true;
			}

			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				// x*(x+a)^(1/b)=c => (x*(x+a)^(1/b))^b=c^b
				Number nrexp;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(CALCULATOR->aborted()) return false;
					if(CHILD(0)[i].isPower() && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number().isRational()) {
						if(!CHILD(0)[i][1].number().isInteger()) {
							if(nrexp.isZero()) nrexp = CHILD(0)[i][1].number().denominator();
							else nrexp.lcm(CHILD(0)[i][1].number().denominator());
						}
					} else if(CHILD(0)[i].isFunction() && CHILD(0)[i].function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(CHILD(0)[i])) {
						if(nrexp.isZero()) nrexp = CHILD(0)[i][1].number();
						else nrexp.lcm(CHILD(0)[i][1].number());
					} else if(CHILD(0)[i] != x_var && !(CHILD(0)[i].isFunction() && CHILD(0)[i].function()->id() == FUNCTION_ID_ABS && CHILD(0)[i].size() == 1 && CHILD(0)[i][0].representsReal(true))) {
						nrexp.clear();
						break;
					}
				}
				if(!nrexp.isZero()) {
					MathStructure mtest(*this);
					if(mtest[0].calculateRaise(nrexp, eo2)) {
						mtest[1].calculateRaise(nrexp, eo2);
						mtest.childrenUpdated();
						if(mtest.isolate_x(eo2, eo, x_var)) {
							if((mtest.isLogicalAnd() || mtest.isLogicalOr() || mtest.isComparison()) && test_comparisons(*this, mtest, x_var, eo, false, eo.expand ? 1 : 2) < 0) {
								if(eo.expand) {
									add(mtest, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
									calculatesub(eo2, eo, false);
									return true;
								}
							} else {
								set(mtest);
								return true;
							}
						}
					}
				}
			}
			if(!eo2.expand) break;
			// abs(x)*x=a => -x*x=a || x*x=a; sgn(x)*x=a => -1*x=a || 0*x=a || 1*x=a
			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				MathStructure *m = find_abs_sgn(CHILD(0), x_var);
				if(m && m->function()->id() == FUNCTION_ID_ABS) {

					MathStructure mabs(*m);

					ComparisonType cmp_type = ct_comp;
					MathStructure *malt = new MathStructure(*this);
					MathStructure mabs_minus(mabs[0]);
					mabs_minus.calculateNegate(eo2);
					(*malt)[0].replace(mabs, mabs_minus, mabs.containsInterval());
					(*malt)[0].calculatesub(eo2, eo, true);
					MathStructure *mcheck = new MathStructure(mabs[0]);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_EQUALS_GREATER : OPERATION_LESS);
					mcheck->isolate_x_sub(eo, eo2, x_var);
					malt->isolate_x_sub(eo, eo2, x_var);
					malt->add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					malt->calculatesub(eo2, eo, false);

					mcheck = new MathStructure(mabs[0]);
					CHILD(0).replace(mabs, mabs[0], mabs.containsInterval());
					CHILD(0).calculatesub(eo2, eo, true);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LESS : OPERATION_EQUALS_GREATER);
					mcheck->isolate_x_sub(eo, eo2, x_var);
					isolate_x_sub(eo, eo2, x_var);
					add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					calculatesub(eo2, eo, false);
					add_nocopy(malt, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
					calculatesub(eo2, eo, false);
					return true;
				} else if(m && m->function()->id() == FUNCTION_ID_SIGNUM) {
					MathStructure mabs(*m);

					ComparisonType cmp_type = ct_comp;
					MathStructure *malt = new MathStructure(*this);
					(*malt)[0].replace(mabs, m_minus_one, mabs.containsInterval());
					(*malt)[0].calculatesub(eo2, eo, true);
					MathStructure *mcheck = new MathStructure(mabs[0]);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_EQUALS_GREATER : OPERATION_LESS);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? (mabs[1].isMinusOne() ? OPERATION_GREATER : OPERATION_EQUALS_GREATER) : (mabs[1].isMinusOne() ? OPERATION_EQUALS_LESS : OPERATION_LESS));
					mcheck->isolate_x_sub(eo, eo2, x_var);
					malt->isolate_x_sub(eo, eo2, x_var);
					malt->add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					malt->calculatesub(eo2, eo, false);

					MathStructure *malt0 = NULL;
					if(!mabs[1].isOne() && !mabs[1].isMinusOne()) {
						malt0 = new MathStructure(*this);
						(*malt0)[0].replace(mabs, mabs[1], mabs.containsInterval());
						(*malt0)[0].calculatesub(eo2, eo, true);
						mcheck = new MathStructure(mabs[0]);
						mcheck->add(mabs[1], cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_NOT_EQUALS : OPERATION_EQUALS);
						mcheck->isolate_x_sub(eo, eo2, x_var);
						malt0->isolate_x_sub(eo, eo2, x_var);
						malt0->add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
						malt0->calculatesub(eo2, eo, false);
					}

					mcheck = new MathStructure(mabs[0]);
					CHILD(0).replace(mabs, m_one, mabs.containsInterval());
					CHILD(0).calculatesub(eo2, eo, true);
					mcheck->add(m_zero, cmp_type == COMPARISON_NOT_EQUALS ? (mabs[1].isOne() ? OPERATION_LESS : OPERATION_EQUALS_LESS) : (mabs[1].isOne() ? OPERATION_EQUALS_GREATER : OPERATION_GREATER));
					mcheck->isolate_x_sub(eo, eo2, x_var);
					isolate_x_sub(eo, eo2, x_var);
					add_nocopy(mcheck, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					calculatesub(eo2, eo, false);
					add_nocopy(malt, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
					if(malt0) add_nocopy(malt0, cmp_type == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
					calculatesub(eo2, eo, false);
					return true;
				}
			}
			// Try factorization
			if(!CHILD(1).isZero() && (!morig || !equals(*morig))) {
				MathStructure mtest(*this);
				mtest[0].calculateSubtract(CHILD(1), eo2);
				mtest[1].clear();
				mtest.childrenUpdated();
				if(mtest[0].factorize(eo2, false, false, 0, false, false, NULL, m_undefined, false, false, 3) && !(mtest[0].isMultiplication() && mtest[0].size() == 2 && (mtest[0][0].isNumber() || mtest[0][0] == CHILD(1) || mtest[0][1] == CHILD(1)))) {
					mtest.childUpdated(1);
					if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
						set_nocopy(mtest);
						return true;
					}
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0)[0].contains(x_var)) {
				if(CHILD(0)[1].contains(x_var)) {
					if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && !CHILD(1).isZero()) {
						// x^(a*x)=b => x=e^(lambertw(ln(x)/a))
						MathStructure mmul(1, 1, 0);
						const MathStructure *mvar = get_power_term(CHILD(0)[1], CHILD(0)[0]);
						if(!mvar || !get_multiplier(CHILD(0)[1], *mvar, mmul) || mmul.contains(x_var) || comparison_might_be_equal(mmul.compare(m_zero))) return false;
						MathStructure mexp(1, 1, 0);
						if(mvar->isPower() && *mvar != CHILD(0)[0]) mexp = (*mvar)[1];
						if(mmul.isOne() && mexp.isOne() && CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(0)[0].representsNonComplex()) {
							// x is real, a=1, b is rational
							if(CHILD(1).number().isInteger()) {
								if(CHILD(1).number().isOne()) {
									// 1^1=1
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number().isMinusOne()) {
									// (-1)^(-1)=-1
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number() == 4) {
									// 2^2=4
									CHILD(1).set(2, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number() == 27) {
									// 3^3=27
									CHILD(1).set(3, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number() == 256) {
									// 4^4=256
									CHILD(1).set(4, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number() == 3125) {
									// 5^5=3125
									CHILD(1).set(5, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number() == 46656) {
									// 6^6=46656
									CHILD(1).set(6, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(CHILD(1).number() == 823543L) {
									// 7^7=823543
									CHILD(1).set(7, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(eo.approximation == APPROXIMATION_EXACT && CHILD(1).number() > 823543L) {
									CALCULATOR->beginTemporaryStopMessages();
									Number nr(CHILD(1).number());
									// n=round(e^(lambertw(ln(b))))
									if(nr.ln() && nr.lambertW() && nr.exp() && nr.round()) {
										nr.setApproximate(false);
										Number nrtest(nr);
										nrtest.raise(nr, true);
										// if b=n^n, x=n
										if(!CALCULATOR->endTemporaryStopMessages() && !nrtest.isApproximate() && CHILD(1).number() == nrtest) {
											CHILD(1).set(nr, true);
											CHILD(0).setToChild(1, true);
											return true;
										}
									} else {
										CALCULATOR->endTemporaryStopMessages();
									}
								}
							} else if(CHILD(1).number().numeratorIsOne()) {
								Number nr_den = CHILD(1).number().denominator();
								if(nr_den == 4) {
									// (-2)^(-2)=1/4
									CHILD(1).set(-2, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(nr_den == 256) {
									// (-4)^(-4)=1/256
									CHILD(1).set(-4, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(nr_den == 46656) {
									// (-6)^(-6)=1/46656
									CHILD(1).set(-6, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(nr_den > 46656) {
									CALCULATOR->beginTemporaryStopMessages();
									// n=-round(e^(lambertw(ln(b))))
									if(nr_den.ln() && nr_den.lambertW() && nr_den.exp() && nr_den.round() && nr_den.negate()) {
										nr_den.setApproximate(false);
										Number nrtest(nr_den);
										nrtest.raise(nr_den, true);
										// if b=n^n, x=n
										if(!CALCULATOR->endTemporaryStopMessages() && !nrtest.isApproximate() && CHILD(1).number() == nrtest) {
											CHILD(1).set(nr_den, true);
											CHILD(0).setToChild(1, true);
											return true;
										}
									} else {
										CALCULATOR->endTemporaryStopMessages();
									}
								}
							} else if(CHILD(1).number().numeratorIsMinusOne()) {
								Number nr_den = CHILD(1).number().denominator();
								if(nr_den == 27) {
									// (-3)^(-3)=-1/27
									CHILD(1).set(-3, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(nr_den == 3125) {
									// (-5)^(-5)=-1/3125
									CHILD(1).set(-5, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(nr_den == 823543L) {
									// (-7)^(-7)=-1/823543
									CHILD(1).set(-7, 1, 0, true);
									CHILD(0).setToChild(1, true);
									return true;
								} else if(nr_den < 823543L) {
									// x^x!=-1/m if x is not integer
									if(ct_comp == COMPARISON_NOT_EQUALS) set(1, 1, 0, true);
									else clear(true);
									return true;
								} else {
									CALCULATOR->beginTemporaryStopMessages();
									// n=-round(e^(lambertw(ln(b))))
									if(nr_den.ln() && nr_den.lambertW() && nr_den.exp() && nr_den.negate() && nr_den.round()) {
										nr_den.setApproximate(false);
										Number nrtest(nr_den);
										nrtest.raise(nr_den, true);
										// if b=n^n, x=n
										if(!CALCULATOR->endTemporaryStopMessages() && !nrtest.isApproximate()) {
											if(CHILD(1).number() == nrtest) {
												CHILD(1).set(nr_den, true);
												CHILD(0).setToChild(1, true);
												return true;
											}
											// x^x!=-1/m if x is not integer
											if(ct_comp == COMPARISON_NOT_EQUALS) set(1, 1, 0, true);
											else clear(true);
											return true;
										}
									} else if(!CALCULATOR->endTemporaryStopMessages()) {
										return false;
									}
								}
							} else if(CHILD(1).number().isNegative()) {
								// x^x!=-n/m if x is not integer and n is not 1
								if(ct_comp == COMPARISON_NOT_EQUALS) set(1, 1, 0, true);
								else clear(true);
								return true;
							}
						} else if(mmul.isOne() && mexp.isMinusOne() && CHILD(0)[0].representsNonComplex() && (CHILD(1).isOne() || CHILD(1).isMinusOne())) {
							CHILD(0).setToChild(1, true);
							return true;
						}
						//if(!mexp.representsPositive()) return false;
						MathStructure *marg = new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(1), NULL);
						if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
						if(CHILD(0)[0].representsNonComplex() && CHILD(1).compare(m_zero) == COMPARISON_RESULT_LESS) {
							// x is real, b > 0:
							// x=e^lambertw(ln(b)/a) if ln(b)/a >= -1/e
							// x=e^lambertw(ln(b)/a, -1) if -1/e <= ln(b)/a < 0
							if(!mexp.isOne()) marg->calculateMultiply(mexp, eo2);
							if(!mmul.isOne()) marg->calculateDivide(mmul, eo2);
							MathStructure *mreq1 = NULL;
							MathStructure *marg2 = NULL;
							MathStructure *mreq2 = NULL;
							if(!marg->representsNonNegative()) {
								mreq1 = new MathStructure(*marg);
								mreq2 = new MathStructure(*marg);
								marg2 = new MathStructure(*marg);
								marg2->transformById(FUNCTION_ID_LAMBERT_W);
								marg2->addChild(m_minus_one);
								if(marg2->calculateFunctions(eo)) marg2->calculatesub(eo2, eo, true);
								if(!mexp.isOne()) marg2->calculateDivide(mexp, eo2);
								marg2->transform(STRUCT_POWER);
								marg2->insertChild(CALCULATOR->getVariableById(VARIABLE_ID_E), 1);
								(*marg2)[0].calculatesub(eo2, eo, true);
								marg2->calculateRaiseExponent(eo2);
								mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_E));
								mreq1->last().calculateRaise(m_minus_one, eo2);
								mreq1->last().calculateNegate(eo2);
								mreq1->childUpdated(2);
								mreq1->isolate_x(eo2, eo);
								mreq2->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
								mreq2->isolate_x(eo2, eo);
								marg2->transform(ct_comp, CHILD(0)[0]);
								marg2->swapChildren(1, 2);
								marg2->isolate_x_sub(eo, eo2, x_var, morig);
								marg2->add(*mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								marg2->add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
								marg2->calculatesub(eo2, eo, false);
							}
							marg->transformById(FUNCTION_ID_LAMBERT_W);
							marg->addChild(m_zero);
							if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
							if(!mexp.isOne()) marg->calculateDivide(mexp, eo2);
							CHILD(0).setToChild(1, true);
							CHILD(1).set(CALCULATOR->getVariableById(VARIABLE_ID_E));
							CHILD(1).raise_nocopy(marg);
							CHILD(1).calculateRaiseExponent(eo2);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							if(mreq1) {
								add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								calculatesub(eo2, eo, false);
							}
							if(marg2) {
								add_nocopy(marg2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
								calculatesub(eo2, eo, false);
							}
						} else {
							// x is not real or b < 0:
							// x=e^lambertw(ln(b)+2i*pi*n/a) || x=e^lambertw(ln(b)+2i*pi*n/a, -1) || x=e^lambertw(ln(b)+2i*pi*n/a, 1)
							marg->add(nr_one_i);
							marg->last().multiply(nr_two);
							marg->last().multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI), true);
							marg->last().multiply(CALCULATOR->getVariableById(VARIABLE_ID_N), true);
							marg->evalSort(true);
							MathStructure *mreq2 = NULL;
							if(!CHILD(1).representsNonZero()) {
								mreq2 = new MathStructure(CHILD(1));
								mreq2->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_NOT_EQUALS : COMPARISON_EQUALS, m_zero);
								mreq2->isolate_x(eo2, eo);
							}
							MathStructure *mreq1 = NULL;
							if(!CHILD(1).isNumber() || !CHILD(1).number().isFraction()) {
								mreq1 = new MathStructure(*marg);
								mreq1->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_NOT_EQUALS : COMPARISON_EQUALS, m_zero);
								mreq1->isolate_x(eo2, eo);
								EvaluationOptions eo3 = eo2;
								eo3.test_comparisons = true;
								mreq1->calculatesub(eo3, eo, false);
								if(mreq1->isNumber()) {
									if(mreq1->number().getBoolean() == (ct_comp == COMPARISON_NOT_EQUALS)) {
										clear(true);
										return true;
									} else {
										mreq1->unref();
										mreq1 = NULL;
									}
								}
							}
							if(!mexp.isOne()) marg->calculateMultiply(mexp, eo2);
							if(!mmul.isOne()) marg->calculateDivide(mmul, eo2);
							MathStructure *marg2 = new MathStructure(*marg);
							MathStructure *marg3 = new MathStructure(*marg);
							marg2->transformById(FUNCTION_ID_LAMBERT_W);
							marg2->addChild(m_minus_one);
							if(marg2->calculateFunctions(eo)) marg2->calculatesub(eo2, eo, true);
							if(!mexp.isOne()) marg2->calculateDivide(mexp, eo2);
							MathStructure *mreq3a = new MathStructure(*marg2);
							mreq3a->transformById(FUNCTION_ID_IM);
							mreq3a->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
							mreq3a->last().negate();
							mreq3a->last().calculatesub(eo2, eo, true);
							mreq3a->childUpdated(2);
							marg2->transform(STRUCT_POWER);
							marg2->insertChild(CALCULATOR->getVariableById(VARIABLE_ID_E), 1);
							(*marg2)[0].calculatesub(eo2, eo, true);
							marg2->calculateRaiseExponent(eo2);
							marg3->transformById(FUNCTION_ID_LAMBERT_W);
							marg3->addChild(m_one);
							if(marg3->calculateFunctions(eo)) marg3->calculatesub(eo2, eo, true);
							if(!mexp.isOne()) marg3->calculateDivide(mexp, eo2);
							MathStructure *mreq3b = new MathStructure(*marg3);
							mreq3b->transformById(FUNCTION_ID_IM);
							mreq3b->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
							mreq3b->last().calculatesub(eo2, eo, true);
							mreq3b->childUpdated(2);
							marg3->transform(STRUCT_POWER);
							marg3->insertChild(CALCULATOR->getVariableById(VARIABLE_ID_E), 1);
							(*marg3)[0].calculatesub(eo2, eo, true);
							marg3->calculateRaiseExponent(eo2);
							marg2->transform(ct_comp, CHILD(0)[0]);
							marg2->swapChildren(1, 2);
							marg2->isolate_x_sub(eo, eo2, x_var, morig);
							marg2->add(*mreq3a, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							if(mreq1) marg2->add(*mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							if(mreq2) marg2->add(*mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							marg2->calculatesub(eo2, eo, false);
							marg3->transform(ct_comp, CHILD(0)[0]);
							marg3->swapChildren(1, 2);
							marg3->isolate_x_sub(eo, eo2, x_var, morig);
							marg3->add(*mreq3b, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							if(mreq1) marg3->add(*mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							if(mreq2) marg3->add(*mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							marg3->calculatesub(eo2, eo, false);
							marg->transformById(FUNCTION_ID_LAMBERT_W);
							marg->addChild(m_zero);
							if(marg->calculateFunctions(eo)) marg->calculatesub(eo2, eo, true);
							if(!mexp.isOne()) marg->calculateDivide(mexp, eo2);
							CHILD(0).setToChild(1, true);
							CHILD(1).set(CALCULATOR->getVariableById(VARIABLE_ID_E));
							CHILD(1).raise_nocopy(marg);
							CHILD(1).calculateRaiseExponent(eo2);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							if(mreq1) add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							if(mreq2) add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							if(mreq1 || mreq2) calculatesub(eo2, eo, false);
							add_nocopy(marg2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							add_nocopy(marg3, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
							calculatesub(eo2, eo, false);
						}
						return true;
					}
				} else if(CHILD(0)[1].isNumber() && CHILD(0)[1].number().isRational()) {
					// x^a=b
					if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && !CHILD(0)[1].number().isInteger() && !CHILD(0)[1].number().isFraction() && !CHILD(0).representsNonComplex(true)) {
						MathStructure mvar(CHILD(0)[0]);
						mvar.raise(CHILD(0)[1].number().numerator());

						UnknownVariable *var = new UnknownVariable("", string(LEFT_PARENTHESIS) + format_and_print(mvar) + RIGHT_PARENTHESIS);
						var->setInterval(mvar);
						MathStructure mu(var);
						MathStructure mtest(mu);
						mtest.raise(CHILD(0)[1]);
						mtest[1].number().divide(CHILD(0)[1].number().numerator());
						mtest.transform(ct_comp, CHILD(1));
						if(mtest.isolate_x_sub(eo, eo2, mu)) {
							mtest.replace(mu, mvar);
							if((mtest.isLogicalAnd() || mtest.isLogicalOr() || mtest.isComparison()) && test_comparisons(*this, mtest, x_var, eo, false, eo.expand ? 1 : 2) < 0) {
								if(eo.expand) {
									add(mtest, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
									calculatesub(eo2, eo, false);
									var->destroy();
									return true;
								}
							} else {
								set(mtest);
								var->destroy();
								return true;
							}
						}
						var->destroy();
						return false;
					}
					bool b_neg = CHILD(0)[1].number().isNegative();
					bool b_nonzero = !CHILD(1).isZero() && CHILD(1).representsNonZero(true);
					if(b_neg && CHILD(1).isZero()) {
						if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
							CHILD(0).setToChild(1);
							CHILD(1) = nr_plus_inf;
							CHILDREN_UPDATED
							MathStructure *malt = new MathStructure(*this);
							(*malt)[1] = nr_minus_inf;
							isolate_x_sub(eo, eo2, x_var, morig);
							malt->isolate_x_sub(eo, eo2, x_var, morig);
							add_nocopy(malt, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
							calculatesub(eo, eo2, false);
							return true;
						}
						if(CHILD(0)[1].number().isInteger() && CHILD(0)[1].number().isEven()) {
							if(ct_comp == COMPARISON_EQUALS_LESS) {
								clear(true);
								return true;
							}
							ct_comp = COMPARISON_NOT_EQUALS;
							CHILD(1).clear(true);
							CHILD(0).setToChild(1);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						}
						if(ct_comp == COMPARISON_EQUALS_GREATER) {
							ct_comp = COMPARISON_GREATER;
						} else if(ct_comp == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_LESS;
						}
						CHILD(1).clear(true);
						CHILD(0).setToChild(1);
						CHILDREN_UPDATED
						isolate_x_sub(eo, eo2, x_var, morig);
						return true;
					} else if(CHILD(1).isZero()) {
						if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS && CHILD(0)[1].number().isInteger() && CHILD(0)[1].number().isEven()) {
							if(ct_comp == COMPARISON_LESS) {
								clear(true);
								return true;
							}
							if(ct_comp == COMPARISON_EQUALS_LESS) {
								ct_comp = COMPARISON_EQUALS;
								CHILD(0).setToChild(1);
								CHILDREN_UPDATED
								isolate_x_sub(eo, eo2, x_var, morig);
								return true;
							}
							if(ct_comp == COMPARISON_EQUALS_GREATER) {
								set(1, 1, 0, true);
								return true;
							}
							CHILD(0).setToChild(1);
							MathStructure *mneg = new MathStructure(*this);
							if(ct_comp == COMPARISON_GREATER) mneg->setComparisonType(COMPARISON_LESS);
							mneg->isolate_x_sub(eo, eo2, x_var, morig);
							add_nocopy(mneg, OPERATION_LOGICAL_OR);
							calculatesub(eo2, eo, false);
						} else {
							if(!CHILD(0)[1].number().isInteger()) {
								if(ct_comp == COMPARISON_LESS) {
									clear(true);
									return true;
								}
								if(ct_comp == COMPARISON_EQUALS_LESS) {
									ct_comp = COMPARISON_EQUALS;
									CHILD(0).setToChild(1);
									CHILDREN_UPDATED
									isolate_x_sub(eo, eo2, x_var, morig);
									return true;
								}
							}
							CHILD(0).setToChild(1);
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
						}
						return true;
					} else if(b_neg && ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
						if(CHILD(0)[1].number().isMinusOne()) {
							CHILD(0).setToChild(1);
						} else {
							CHILD(0)[1].number().negate();

						}
						MathStructure *mtest = new MathStructure(CHILD(0));
						CHILD(1).set(1, 1, 0);
						CHILDREN_UPDATED
						MathStructure *malt = new MathStructure(*this);
						if(ct_comp == COMPARISON_EQUALS_GREATER) {
							ct_comp = COMPARISON_EQUALS_LESS;
						} else if(ct_comp == COMPARISON_GREATER) {
							ct_comp = COMPARISON_LESS;
						} else if(ct_comp == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_EQUALS_GREATER;
						} else if(ct_comp == COMPARISON_LESS) {
							ct_comp = COMPARISON_GREATER;
						}
						isolate_x_sub(eo, eo2, x_var, morig);
						mtest->add(m_zero, OPERATION_GREATER);
						MathStructure *mtest_alt = new MathStructure(*mtest);
						mtest_alt->setComparisonType(COMPARISON_LESS);
						mtest->isolate_x_sub(eo, eo2, x_var);
						add_nocopy(mtest, OPERATION_LOGICAL_AND);
						calculatesub(eo2, eo, false);
						malt->isolate_x_sub(eo, eo2, x_var, morig);
						mtest_alt->isolate_x_sub(eo, eo2, x_var);
						malt->add_nocopy(mtest_alt, OPERATION_LOGICAL_AND);
						malt->calculatesub(eo2, eo, false);
						add_nocopy(malt, OPERATION_LOGICAL_OR);
						calculatesub(eo2, eo, false);
						return true;
					}
					MathStructure mbak(*this);
					if(CHILD(0)[1].number().isMinusOne()) {
						CHILD(0).setToChild(1, true);
						CHILD(1).calculateRaise(m_minus_one, eo2);
						CHILDREN_UPDATED
						isolate_x_sub(eo, eo2, x_var, morig);
					} else if(CHILD(0)[1].number().isInteger()) {
						bool b_real = CHILD(0)[0].representsNonComplex(true);
						bool b_complex = !b_real && CHILD(0)[0].representsComplex(true);
						bool warn_complex = false;
						bool check_complex = false;
						if(CHILD(0)[1].number().isEven()) {
							if(!CHILD(1).representsNonNegative(true)) {
								if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
									if(CHILD(1).representsNegative(true)) {
										if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_LESS) clear(true);
										else set(1, 1, 0, true);
										return true;
									}
									return false;
								}
								if(b_real && (CHILD(1).representsNegative(true) || CHILD(1).representsComplex(true))) {
									if(ct_comp == COMPARISON_EQUALS) {
										clear(true);
									} else if(ct_comp == COMPARISON_NOT_EQUALS) {
										set(1, 1, 0, true);
									}
									return true;
								}
							}
						}
						bool b_set = false;
						if(b_neg) CHILD(0)[1].number().negate();
						if(CHILD(0)[1].number().isTwo()) {
							CHILD(1).raise(CHILD(0)[1].number());
							CHILD(1)[1].number().recip();
							if(b_neg) CHILD(1)[1].number().negate();
							CHILD(1).calculateRaiseExponent(eo2);
							CHILDREN_UPDATED
						} else if(!b_real && CHILD(0)[1].number() == 4 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
							CHILD(1).raise(CHILD(0)[1].number());
							CHILD(1)[1].number().recip();
							if(b_neg) CHILD(1)[1].number().negate();
							CHILD(1).calculateRaiseExponent(eo2);
							CHILD(0).setToChild(1);
							CHILDREN_UPDATED
							MathStructure *malt1 = new MathStructure(*this);
							MathStructure *malt2 = new MathStructure(*this);
							MathStructure *malt3 = new MathStructure(*this);
							(*malt1)[1].calculateNegate(eo2);
							(*malt2)[1].calculateMultiply(m_one_i, eo2);
							(*malt3)[1].calculateMultiply(nr_minus_i, eo2);
							malt1->childUpdated(2);
							malt2->childUpdated(2);
							malt3->childUpdated(2);
							malt1->isolate_x_sub(eo, eo2, x_var, morig);
							malt2->isolate_x_sub(eo, eo2, x_var, morig);
							malt3->isolate_x_sub(eo, eo2, x_var, morig);
							isolate_x_sub(eo, eo2, x_var, morig);
							add_nocopy(malt1, ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
							add_nocopy(malt2, ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
							add_nocopy(malt3, ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
							calculatesub(eo2, eo, false);
							b_set = true;
						} else if(!b_real && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
							if(CHILD(0)[1].number() > 20) return false;
							MathStructure mdeg(CHILD(0)[1]);
							Number marg_pi;
							MathStructure marg;
							if(CHILD(1).representsNegative(true)) {
								marg_pi.set(1, 1, 0);
							} else if(!CHILD(1).representsNegative(true)) {
								if(CHILD(1).isNumber() && !CHILD(1).number().hasRealPart()) {
									if(CHILD(1).number().imaginaryPartIsNegative()) marg_pi.set(-1, 2, 0);
									else marg_pi.set(1, 2, 0);
								} else {
									marg.set(CALCULATOR->getFunctionById(FUNCTION_ID_ARG), &CHILD(1), NULL);
									marg.calculateFunctions(eo);
									if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
										if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) marg /= CALCULATOR->getRadUnit();
									} else {
										marg.multiply(angle_units_in_turn(eo2, 2, 1, true));
										marg.multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
									}
									marg.calculatesub(eo2, eo, true);
								}
							}
							MathStructure minv(mdeg);
							minv.number().recip();
							MathStructure mmul(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), &CHILD(1), NULL);
							mmul.calculateFunctions(eo);
							mmul.calculateRaise(minv, eo2);
							Number nr_i;
							while(nr_i.isLessThan(mdeg.number())) {
								MathStructure mroot;
								if(CALCULATOR->aborted()) {set(mbak); return false;}
								MathStructure mexp;
								Number nexp;
								if(!nr_i.isZero()) {
									nexp.set(2, 1, 0);
									if(!nexp.multiply(nr_i)) {set(mbak); return false;}
								}
								b_set = false;
								if(!marg_pi.isZero()) {
									if(nexp.isZero()) nexp = marg_pi;
									else if(!nexp.add(marg_pi)) {set(mbak); return false;}
									if(!nexp.divide(mdeg.number())) {set(mbak); return false;}
									if(nexp.isInteger()) {
										mroot.set(mmul);
										if(nexp.isOdd()) {
											mroot.calculateNegate(eo2);
										}
										b_set = true;
									} else if(nexp.isRational() && nexp.denominatorIsTwo()) {
										if(!nexp.floor()) {set(mbak); return false;}
										mroot.set(mmul);
										if(nexp.isEven()) {
											mroot.calculateMultiply(nr_one_i, eo2);
										} else {
											mroot.calculateMultiply(nr_minus_i, eo2);
										}
										b_set = true;
									}
									if(!b_set) {
										mexp.set(nexp);
										mexp.multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
									}
								} else {
									if(nexp.isZero()) {
										if(!marg.isZero()) {
											mexp.set(marg);
											mexp.multiply(minv);
										}
									} else {
										mexp.set(nexp);
										mexp.multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI));
										if(!marg.isZero()) mexp.add(marg);
										mexp.multiply(minv);
									}
								}
								if(!b_set) {
									if(mexp.isZero()) {
										mroot.set(mmul);
									} else {
										mexp.multiply(m_one_i);
										mroot.set(CALCULATOR->getVariableById(VARIABLE_ID_E));
										mroot.raise(mexp);
										mroot.calculatesub(eo2, eo, true);
										mroot.calculateMultiply(mmul, eo2);
									}
								}
								if(b_neg) mroot.calculateRaise(m_minus_one, eo2);
								if(nr_i.isZero()) {
									CHILD(0).setToChild(1);
									CHILD(1) = mroot;
									isolate_x_sub(eo, eo2, x_var, morig);
								} else {
									MathStructure *malt = new MathStructure(mbak[0][0]);
									malt->add(mroot, mbak.comparisonType() == COMPARISON_NOT_EQUALS ? OPERATION_NOT_EQUALS : OPERATION_EQUALS);
									malt->isolate_x_sub(eo, eo2, x_var, morig);
									add_nocopy(malt, mbak.comparisonType() == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR, true);
								}
								nr_i++;
							}
							calculatesub(eo2, eo, false);
							b_set = true;
						} else {
							if(b_complex) {
								warn_complex = true;
							} else if(!b_real) {
								check_complex = true;
							}
							CHILD(1).transform(STRUCT_FUNCTION, CHILD(0)[1]);
							CHILD(1).setFunctionId(FUNCTION_ID_ROOT);
							if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
							if(b_neg) CHILD(1).calculateRaise(m_minus_one, eo2);
							childUpdated(2);
						}
						if(!b_set) {
							if(CHILD(0)[1].number().isEven()) {
								CHILD(0).setToChild(1);
								MathStructure *mneg = new MathStructure(*this);
								(*mneg)[1].calculateNegate(eo2);
								mneg->childUpdated(2);
								if(ct_comp == COMPARISON_LESS) mneg->setComparisonType(COMPARISON_GREATER);
								else if(ct_comp == COMPARISON_GREATER) mneg->setComparisonType(COMPARISON_LESS);
								else if(ct_comp == COMPARISON_EQUALS_LESS) mneg->setComparisonType(COMPARISON_EQUALS_GREATER);
								else if(ct_comp == COMPARISON_EQUALS_GREATER) mneg->setComparisonType(COMPARISON_EQUALS_LESS);
								mneg->isolate_x_sub(eo, eo2, x_var, morig);
								isolate_x_sub(eo, eo2, x_var, morig);
								add_nocopy(mneg, ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
								calculatesub(eo2, eo, false);
							} else {
								CHILD(0).setToChild(1);
								isolate_x_sub(eo, eo2, x_var, morig);
							}
						}
						if(check_complex) {
							if(!isComparison() || CHILD(0) != x_var) {
								warn_complex = true;
							}
							MathStructure mtest(mbak[0][0]);
							if(!warn_complex && check_complex) {
								mtest.replace(x_var, CHILD(1));
								if(mtest.representsNonComplex(true)) check_complex = false;
								if(mtest.representsComplex(true)) {
									warn_complex = true;
								}
							}
							if(!warn_complex && check_complex) {
								CALCULATOR->beginTemporaryStopMessages();
								EvaluationOptions eo3 = eo;
								eo3.approximation = APPROXIMATION_APPROXIMATE;
								mtest.eval(eo3);
								if(CALCULATOR->endTemporaryStopMessages() || !mtest.representsReal(true)) {
									warn_complex = true;
								}
							}
						}
						if(warn_complex) CALCULATOR->error(false, _("Only one or two of the roots were calculated for %s."), format_and_print(mbak).c_str(), NULL);
					} else {
						MathStructure *mposcheck = NULL;
						bool b_test = false;
						if(!CHILD(1).representsNonNegative(true)) {
							if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
								if(CHILD(1).representsNegative(true)) {
									if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_LESS) clear(true);
									else {CHILD(0).setToChild(1, true); CHILD(1).clear(true); CHILDREN_UPDATED}
									return true;
								}
								if(ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_GREATER) {
									mposcheck = new MathStructure(CHILD(0)[0]);
									mposcheck->add(m_zero, OPERATION_EQUALS_GREATER);
									mposcheck->isolate_x_sub(eo, eo2, x_var);
								}
							} else {
								if(CHILD(1).representsNegative(true)) {
									if(ct_comp == COMPARISON_EQUALS) {
										clear(true);
									} else if(ct_comp == COMPARISON_NOT_EQUALS) {
										set(1, 1, 0, true);
									}
									return true;
								}
								if(CHILD(1).representsNonComplex(true)) {
									mposcheck = new MathStructure(CHILD(1));
									mposcheck->add(m_zero, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LESS : OPERATION_EQUALS_GREATER);
									mposcheck->isolate_x_sub(eo, eo2, x_var);
								} else {
									b_test = true;
									mposcheck = new MathStructure(*this);
								}
							}
						}
						if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_LESS) {
							mposcheck = new MathStructure(CHILD(0)[0]);
							mposcheck->add(m_zero, OPERATION_EQUALS_GREATER);
							mposcheck->isolate_x_sub(eo, eo2, x_var);
						}
						CHILD(0)[1].number().recip();
						CHILD(1).calculateRaise(CHILD(0)[1], eo);
						CHILD(0).setToChild(1);
						CHILDREN_UPDATED
						isolate_x_sub(eo, eo2, x_var, morig);
						if(b_test) {
							if(test_comparisons(*mposcheck, *this, x_var, eo) < 0) {
								mposcheck->unref();
								return false;
							}
							mposcheck->unref();
						} else if(mposcheck) {
							add_nocopy(mposcheck, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							calculatesub(eo2, eo, false);
						}
					}
					if(b_neg && !b_nonzero) {
						MathStructure *mtest = new MathStructure(mbak[1]);
						mtest->add(m_zero, (mbak.comparisonType() == COMPARISON_NOT_EQUALS) ? OPERATION_EQUALS : OPERATION_NOT_EQUALS);
						mtest->isolate_x_sub(eo, eo2, x_var);
						add_nocopy(mtest, (mbak.comparisonType() == COMPARISON_NOT_EQUALS) ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						calculatesub(eo2, eo, false);
						if(mbak.comparisonType() != COMPARISON_NOT_EQUALS && mbak.comparisonType() != COMPARISON_EQUALS) {
							MathStructure *malt = new MathStructure(mbak[0]);
							if(mbak[0][1].representsInteger() && mbak[0][1].representsEven()) {
								malt->add(m_zero, OPERATION_NOT_EQUALS);
							} else {
								malt->add(m_zero, (ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_GREATER) ? OPERATION_LESS : OPERATION_GREATER);
							}
							malt->isolate_x_sub(eo, eo2, x_var, morig);
							MathStructure *mtest2 = new MathStructure(mbak[1]);
							mtest2->add(m_zero, OPERATION_EQUALS);
							mtest2->isolate_x_sub(eo, eo2, x_var);
							malt->add_nocopy(mtest2, (mbak.comparisonType() == COMPARISON_NOT_EQUALS) ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							malt->calculatesub(eo2, eo, false);
							add_nocopy(malt, OPERATION_LOGICAL_OR);
							calculatesub(eo2, eo, false);
						}
					}
					return true;
				}
			} else if(CHILD(0)[1].contains(x_var) && CHILD(0)[0].representsNumber() && (!CHILD(0)[1].representsNonComplex() || (CHILD(0)[0].representsNonNegative() || (CHILD(0)[0].isNumber() && CHILD(0)[0].number().isNegative() && CHILD(1).isNumber())))) {
				// a^x=b => x=log(b, a)
				MathStructure *mtest = NULL, *m0 = NULL, *m1 = NULL;
				if(CHILD(0)[0].isOne()) return false;
				if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && !CHILD(0)[0].representsNonZero()) {
					if(!CHILD(1).representsNonZero()) {
						MathStructure *mtest2 = NULL;
						if(!CHILD(1).isZero()) {
							mtest2 = new MathStructure(CHILD(1));
							mtest2->transform(ct_comp, m_zero);
							mtest2->isolate_x(eo2, eo);
						}
						m0 = new MathStructure(CHILD(0)[1]);
						if(!m0->representsNonComplex(true)) {
							m0->transformById(FUNCTION_ID_RE);
							if(m0->calculateFunctions(eo)) m0->calculatesub(eo2, eo, true);
						}
						m0->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, m_zero);
						m0->isolate_x_sub(eo, eo2, x_var, morig);
						if(mtest2) {
							m0->add_nocopy(mtest2, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
							m0->calculatesub(eo2, eo, false);
						}
						if(CHILD(0)[0].isZero()) {
							set_nocopy(*m0, true);
							m0->unref();
							return true;
						}
						MathStructure *mtest3 = new MathStructure(CHILD(0)[0]);
						mtest3->transform(ct_comp, m_zero);
						mtest3->isolate_x(eo2, eo);
						m0->add_nocopy(mtest3, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
						m0->calculatesub(eo2, eo, false);
					}
					mtest = new MathStructure(CHILD(0)[1]);
					mtest->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_NOT_EQUALS : COMPARISON_EQUALS, m_zero);
					mtest->isolate_x_sub(eo, eo2, x_var, morig);
					if(!CHILD(0)[0].isZero()) {
						MathStructure *mtest_b = new MathStructure(CHILD(0)[0]);
						mtest_b->transform(ct_comp == COMPARISON_EQUALS ? COMPARISON_NOT_EQUALS : COMPARISON_EQUALS, m_zero);
						mtest_b->isolate_x_sub(eo, eo2, x_var, morig);
						mtest->add_nocopy(mtest_b, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						mtest->calculatesub(eo2, eo, false);
					}
				}
				ComparisonResult cr1 = COMPARISON_RESULT_NOT_EQUAL;
				if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
					cr1 = CHILD(0)[0].compare(m_one);
					if(cr1 == COMPARISON_RESULT_EQUAL) {
						ComparisonResult cr2 = CHILD(1).compare(m_one);
						if(cr2 == COMPARISON_RESULT_EQUAL) {
							if(ct_comp == COMPARISON_EQUALS) set(1, 1, 0, true);
							else clear(true);
							return true;
						} else if(COMPARISON_MIGHT_BE_EQUAL(cr2)) {
							m1 = new MathStructure();
							m1->setType(STRUCT_COMPARISON);
							m1->setComparisonType(ct_comp);
							m1->addChild(CHILD(1));
							m1->addChild(m_one);
							m1->isolate_x(eo2, eo);
						}
					} else if(COMPARISON_MIGHT_BE_EQUAL(cr1)) {
						ComparisonResult cr2 = CHILD(1).compare(m_one);
						if(cr2 == COMPARISON_RESULT_EQUAL) {
							m1 = new MathStructure();
							m1->setType(STRUCT_COMPARISON);
							m1->setComparisonType(ct_comp);
							m1->addChild(CHILD(0)[0]);
							m1->addChild(m_one);
							m1->isolate_x(eo2, eo);
						} else if(COMPARISON_MIGHT_BE_EQUAL(cr2)) {
							m1 = new MathStructure();
							m1->setType(STRUCT_COMPARISON);
							m1->setComparisonType(ct_comp);
							m1->addChild(CHILD(1));
							m1->addChild(m_one);
							m1->isolate_x(eo2, eo);
							MathStructure *m1b = new MathStructure();
							m1b->setType(STRUCT_COMPARISON);
							m1b->setComparisonType(ct_comp);
							m1b->addChild(CHILD(0)[0]);
							m1b->addChild(m_one);
							m1b->isolate_x(eo2, eo);
							m1->add_nocopy(m1b, ct_comp == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							m1->calculatesub(eo2, eo, false);
						}
					}
				}
				ComparisonType ct = ct_comp;

				if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
					if(CHILD(0)[0].isMinusOne()) {
						if(CHILD(1).isOne() || CHILD(1).isMinusOne()) {
							bool b_m1 = CHILD(1).isMinusOne();
							CHILD(0).setToChild(2, true);
							CHILD(1).set(2, 1, 0, true);
							CHILD(1) *= CALCULATOR->getVariableById(VARIABLE_ID_N);
							if(b_m1) CHILD(1) += m_one;
							CHILD(1).evalSort(false);
							fix_n_multiple(*this, eo2, eo, x_var);
							CHILDREN_UPDATED;
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						}
					} else if(CHILD(1).isOne() && CHILD(0)[1].representsNonComplex()) {
						CHILD(0).setToChild(2, true);
						CHILD(1).clear(true);
						isolate_x_sub(eo, eo2, x_var, morig);
						if(m1) {
							add_nocopy(m1, ct == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							calculatesub(eo2, eo, false);
						}
						if(mtest) {
							add_nocopy(mtest, ct == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
							calculatesub(eo2, eo, false);
						}
						return true;
					}
				}
				if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
					if(CHILD(0)[0].isNumber() && CHILD(0)[0].number().isReal() && CHILD(0)[0].number().isPositive()) {
						if(CHILD(1).representsNegative()) {
							if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
								set(1, 1, 0, true);
							} else {
								clear(true);
							}
							return true;
						}
						if(CHILD(0)[0].number().isFraction()) {
							switch(ct_comp) {
								case COMPARISON_LESS: {ct_comp = COMPARISON_GREATER; break;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_LESS; break;}
								case COMPARISON_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
								case COMPARISON_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_LESS; break;}
								default: {}
							}
						}
					} else if(CHILD(0)[0].isZero()) {
						bool b_clear = false, b_gz = false;
						switch(ct_comp) {
							case COMPARISON_LESS: {b_gz = CHILD(1).representsPositive(); b_clear = !b_gz && CHILD(1).representsNonPositive(); break;}
							case COMPARISON_GREATER: {b_gz = CHILD(1).representsNegative(); b_clear = !b_gz && CHILD(1).representsNonNegative(); break;}
							case COMPARISON_EQUALS_LESS: {b_gz = CHILD(1).representsNonNegative(); b_clear = !b_gz && CHILD(1).representsNegative(); break;}
							case COMPARISON_EQUALS_GREATER: {b_gz = CHILD(1).representsNonPositive(); b_clear = !b_gz && CHILD(1).representsPositive(); break;}
							default: {}
						}
						if(b_clear) {
							clear(true);
							return true;
						} else if(b_gz) {
							ct_comp = COMPARISON_GREATER;
							CHILD(1).clear(true);
							CHILD(0).setToChild(2, true);
							if(!CHILD(0).representsNonComplex(true)) {
								CHILD(0).transformById(FUNCTION_ID_RE);
								if(CHILD(0).calculateFunctions(eo)) CHILD(0).calculatesub(eo2, eo, true);
							}
							CHILDREN_UPDATED
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						} else {
							return false;
						}
					} else {
						return false;
					}
				}
				MathStructure msave(CHILD(1));
				if(CHILD(0)[1].representsNonComplex()) {
					if(CHILD(0)[0].representsNegative()) {
						MathStructure mtest2(CALCULATOR->getFunctionById(FUNCTION_ID_LOGN), &msave, &CHILD(0)[0], NULL);
						mtest2[1].calculateNegate(eo2);
						mtest2.childUpdated(2);
						if(mtest2.calculateFunctions(eo)) mtest2.calculatesub(eo2, eo, true);
						if(mtest) mtest->unref();
						if(m0) m0->unref();
						mtest = NULL;
						if(!mtest2.isInteger()) return false;
						if(mtest2.number().isOdd()) {
							if(ct_comp == COMPARISON_NOT_EQUALS) set(1, 1, 0, true);
							else clear(true);
							return true;
						}
						CHILD(1).set(mtest2, true);
					} else {
						CHILD(1).set(CALCULATOR->getFunctionById(FUNCTION_ID_LOGN), &msave, &CHILD(0)[0], NULL);
						bool b = CHILD(1).calculateFunctions(eo);
						if(b) CHILD(1).calculatesub(eo2, eo, true);
					}
				} else {
					CHILD(1).set(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &msave, NULL);
					if(!msave.isZero()) {
						CHILD(1) += nr_one_i;
						CHILD(1)[1] *= nr_two;
						CHILD(1)[1].multiply(CALCULATOR->getVariableById(VARIABLE_ID_PI), true);
						CHILD(1)[1].multiply(CALCULATOR->getVariableById(VARIABLE_ID_N), true);
					}
					CHILD(1).divide_nocopy(new MathStructure(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), &CHILD(0)[0], NULL));
					CHILD(1).evalSort(true);
					bool b = CHILD(1).calculateFunctions(eo);
					if(b) CHILD(1).calculatesub(eo2, eo, true);
				}
				MathStructure *mn1 = NULL;
				if(COMPARISON_MIGHT_BE_EQUAL(cr1)) {
					mn1 = new MathStructure();
					mn1->setType(STRUCT_COMPARISON);
					mn1->setComparisonType(ct == COMPARISON_EQUALS ? COMPARISON_NOT_EQUALS : COMPARISON_EQUALS);
					mn1->addChild(CHILD(0)[0]);
					mn1->addChild(m_one);
					mn1->isolate_x(eo2, eo);
				}
				CHILD(0).setToChild(2, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				if(mn1) {
					add_nocopy(mn1, ct == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					calculatesub(eo2, eo, false);
				}
				if(m1) {
					add_nocopy(m1, ct == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					calculatesub(eo2, eo, false);
				}
				if(mtest) {
					add_nocopy(mtest, ct == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					calculatesub(eo2, eo, false);
					if(m0) {
						add_nocopy(m0, ct == COMPARISON_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						calculatesub(eo2, eo, false);
					}
				}
				fix_n_multiple(*this, eo2, eo, x_var);
				return true;
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				if(CHILD(1).isOne()) {
					CHILD(0).setToChild(1, true);
					CHILD(1).clear(true);
					return true;
				}
				if(CHILD(1).isZero()) {
					CHILD(0).setToChild(1, true);
					if(ct_comp == COMPARISON_NOT_EQUALS) ct_comp = COMPARISON_EQUALS;
					else ct_comp = COMPARISON_NOT_EQUALS;
					return true;
				}
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(CHILD(0).function()->id() == FUNCTION_ID_ROOT && VALID_ROOT(CHILD(0))) {
				if(CHILD(0)[0].contains(x_var)) {
					MathStructure *mposcheck = NULL;
					bool b_test = false;
					if(CHILD(0)[1].number().numeratorIsEven() && !CHILD(1).representsNonNegative(true)) {
						if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
							if(CHILD(1).representsNegative(true)) {
								if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_LESS) clear(true);
								else {CHILD(0).setToChild(1, true); CHILD(1).clear(true); CHILDREN_UPDATED}
								return true;
							}
							if(ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_GREATER) {
								mposcheck = new MathStructure(CHILD(0)[0]);
								mposcheck->add(m_zero, OPERATION_EQUALS_GREATER);
								mposcheck->isolate_x_sub(eo, eo2, x_var);
							}
						} else {
							if(CHILD(1).representsNegative(true)) {
								if(ct_comp == COMPARISON_EQUALS) {
									clear(true);
								} else if(ct_comp == COMPARISON_NOT_EQUALS) {
									set(1, 1, 0, true);
								}
								return true;
							}
							if(CHILD(1).representsNonComplex(true)) {
								mposcheck = new MathStructure(CHILD(1));
								mposcheck->add(m_zero, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LESS : OPERATION_EQUALS_GREATER);
								mposcheck->isolate_x_sub(eo, eo2, x_var);
							} else {
								b_test = true;
								mposcheck = new MathStructure(*this);
							}
						}
					}
					if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_LESS) && CHILD(0)[1].number().numeratorIsEven()) {
						mposcheck = new MathStructure(CHILD(0)[0]);
						mposcheck->add(m_zero, OPERATION_EQUALS_GREATER);
						mposcheck->isolate_x_sub(eo, eo2, x_var);
					}
					CHILD(1).calculateRaise(CHILD(0)[1], eo);
					CHILD(0).setToChild(1);
					CHILDREN_UPDATED
					isolate_x_sub(eo, eo2, x_var, morig);
					if(b_test) {
						if(test_comparisons(*mposcheck, *this, x_var, eo) < 0) {
							mposcheck->unref();
							return false;
						}
						mposcheck->unref();
					} else if(mposcheck) {
						add_nocopy(mposcheck, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						calculatesub(eo2, eo, false);
					}
					return true;
				}
			} else if(CHILD(0).function()->id() == FUNCTION_ID_LOG && CHILD(0).size() == 1) {
				if(CHILD(0)[0].contains(x_var)) {
					if(!CHILD(1).representsNonComplex()) {
						if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) return false;
						MathStructure mtest(CALCULATOR->getVariableById(VARIABLE_ID_E));
						mtest.raise(CHILD(1));
						mtest.transformById(FUNCTION_ID_LOG);
						ComparisonResult cr = mtest.compare(CHILD(1));
						if(!COMPARISON_MIGHT_BE_EQUAL(cr) || cr == COMPARISON_RESULT_UNKNOWN) {
							if(COMPARISON_IS_NOT_EQUAL(cr)) {
								if(ct_comp == COMPARISON_EQUALS) clear(true);
								else set(1, 1, 0, true);
								return true;
							}
							return false;
						}
					}
					MathStructure msave(CHILD(1));
					CHILD(1).set(CALCULATOR->getVariableById(VARIABLE_ID_E));
					CHILD(1).calculateRaise(msave, eo2);
					CHILD(0).setToChild(1, true);
					CHILDREN_UPDATED;
					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						MathStructure *mand = new MathStructure(CHILD(0));
						mand->add(m_zero, OPERATION_GREATER);
						mand->isolate_x_sub(eo, eo2, x_var);
						isolate_x_sub(eo, eo2, x_var, morig);
						add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						calculatesub(eo2, eo, false);
					} else {
						isolate_x_sub(eo, eo2, x_var, morig);
					}
					return true;
				}
			} else if(CHILD(0).function()->id() == FUNCTION_ID_LAMBERT_W && (CHILD(0).size() == 1 || (CHILD(0).size() == 2 && CHILD(0)[1].isInteger())) && ((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) || (CHILD(0).size() == 1 || CHILD(0)[1].isZero() || CHILD(0)[1].isMinusOne()))) {
				if(CHILD(0)[0].contains(x_var)) {
					CHILDREN_UPDATED;
					bool b_test = true;
					if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
						if(CHILD(0).size() == 2 && CHILD(0)[1].isMinusOne()) {
							ComparisonResult cr = CHILD(1).compare(nr_minus_one);
							if(cr == COMPARISON_RESULT_LESS) {
								if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {clear(true); return true;}
								if(ct_comp == COMPARISON_LESS) ct_comp = COMPARISON_EQUALS_LESS;
								CHILD(1) = nr_minus_one;
							} else if(cr == COMPARISON_RESULT_EQUAL_OR_LESS) {
								if(ct_comp == COMPARISON_GREATER) {clear(true); return true;}
								return false;
							} else if(cr == COMPARISON_RESULT_EQUAL) {
								if(ct_comp == COMPARISON_GREATER) {clear(true); return true;}
								if(ct_comp == COMPARISON_EQUALS_GREATER) ct_comp = COMPARISON_EQUALS;
							} else if(cr != COMPARISON_RESULT_GREATER && cr != COMPARISON_RESULT_EQUAL_OR_GREATER) {
								return false;
							}
						} else {
							ComparisonResult cr = CHILD(1).compare(nr_minus_one);
							if(cr == COMPARISON_RESULT_GREATER) {
								if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {clear(true); return true;}
								if(ct_comp == COMPARISON_GREATER) ct_comp = COMPARISON_EQUALS_GREATER;
								CHILD(1) = nr_minus_one;
							} else if(cr == COMPARISON_RESULT_EQUAL_OR_GREATER) {
								if(ct_comp == COMPARISON_LESS) {clear(true); return true;}
								return false;
							} else if(cr == COMPARISON_RESULT_EQUAL) {
								if(ct_comp == COMPARISON_LESS) {clear(true); return true;}
								if(ct_comp == COMPARISON_EQUALS_LESS) ct_comp = COMPARISON_EQUALS;
							} else if(cr != COMPARISON_RESULT_LESS && cr != COMPARISON_RESULT_EQUAL_OR_LESS) {
								return false;
							}
						}
					} else if(CHILD(0).size() == 1 || CHILD(0)[1].isZero()) {
						ComparisonResult cr = CHILD(1).compare(nr_minus_one);
						if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
							b_test = false;
						} else if(cr == COMPARISON_RESULT_GREATER) {
							if(ct_comp == COMPARISON_EQUALS) clear(true);
							else set(1, 1, 0, true);
							return true;
						}
					} else if(CHILD(0)[1].isMinusOne()) {
						ComparisonResult cr = CHILD(1).compare(nr_minus_one);
						if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
							b_test = false;
						} else if(cr == COMPARISON_RESULT_LESS) {
							if(ct_comp == COMPARISON_EQUALS) clear(true);
							else set(1, 1, 0, true);
							return true;
						}
					} else if(CHILD(1).representsReal()) {
						if(ct_comp == COMPARISON_EQUALS) clear(true);
						else set(1, 1, 0, true);
						return true;
					}
					MathStructure mtest(*this);
					mtest[1].set(CALCULATOR->getVariableById(VARIABLE_ID_E));
					mtest[1].calculateRaise(CHILD(1), eo2);
					mtest[1].calculateMultiply(CHILD(1), eo2);
					mtest[0].setToChild(1, true);
					if((ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) && (CHILD(0).size() == 1 || CHILD(0)[1].isZero())) {
						MathStructure *mand = new MathStructure(mtest[0]);
						mand->add(MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_E)), CHILD(0).size() == 2 && CHILD(0)[1].isMinusOne() ? OPERATION_EQUALS_LESS : OPERATION_EQUALS_GREATER);
						(*mand)[1].inverse();
						(*mand)[1].negate();
						(*mand)[1].calculatesub(eo2, eo, true);
						mand->childUpdated(2);
						mand->isolate_x_sub(eo, eo2, x_var);
						mtest.isolate_x_sub(eo, eo2, x_var, morig);
						mtest.add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						mtest.calculatesub(eo2, eo, false);
					} else if((ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) && CHILD(0).size() == 2 && CHILD(0)[1].isMinusOne()) {
						MathStructure *mand = new MathStructure(mtest[0]);
						mand->add(m_zero, OPERATION_EQUALS_LESS);
						mand->childUpdated(2);
						mand->isolate_x_sub(eo, eo2, x_var);
						mtest.isolate_x_sub(eo, eo2, x_var, morig);
						mtest.add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						mtest.calculatesub(eo2, eo, false);
					} else {
						mtest.isolate_x_sub(eo, eo2, x_var, morig);
					}
					if(!b_test || (!mtest.isLogicalAnd() && !mtest.isLogicalOr() && !mtest.isComparison()) || test_comparisons(*this, mtest, x_var, eo, false, eo2.expand ? 1 : 2) >= 0) {
						set(mtest);
						return true;
					}
				}
			} else if(CHILD(0).function()->id() == FUNCTION_ID_LOGN && CHILD(0).size() == 2) {
				if(CHILD(0)[0].contains(x_var)) {
					MathStructure msave(CHILD(1));
					CHILD(1) = CHILD(0)[1];
					CHILD(1).calculateRaise(msave, eo2);
					CHILD(0).setToChild(1, true);
					CHILDREN_UPDATED;
					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						MathStructure *mand = new MathStructure(CHILD(0));
						mand->add(m_zero, OPERATION_GREATER);
						mand->isolate_x_sub(eo, eo2, x_var);
						isolate_x_sub(eo, eo2, x_var, morig);
						add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						calculatesub(eo2, eo, false);
					} else {
						isolate_x_sub(eo, eo2, x_var, morig);
					}
					return true;
				}
			} else if((CHILD(0).function()->id() == FUNCTION_ID_ERF || CHILD(0).function()->id() == FUNCTION_ID_ERFC || CHILD(0).function()->id() == FUNCTION_ID_ERFI) && CHILD(0).size() == 1) {
				if(CHILD(0)[0].contains(x_var) && (ct_comp == COMPARISON_EQUALS || (CHILD(0)[0].representsReal() && CHILD(0).function()->id() != FUNCTION_ID_ERFI)) && CHILD(1).isNumber()) {
					Number nr(CHILD(1).number());
					if(CHILD(0).function()->id() == FUNCTION_ID_ERFC) {nr--; nr.negate();}
					else if(CHILD(0).function()->id() == FUNCTION_ID_ERFI) {nr /= nr_minus_i;}
					if(nr.isFraction()) {
						if(CHILD(0).function()->id() != FUNCTION_ID_ERF) CHILD(1).set(nr, true);
						if(CHILD(0).function()->id() == FUNCTION_ID_ERFI) {
							CHILD(0).setToChild(1, true);
							CHILD(0).calculateMultiply(nr_one_i, eo2);
						} else {
							CHILD(0).setToChild(1, true);
						}
						CHILD(1).transformById(FUNCTION_ID_ERFINV);
						if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
						isolate_x_sub(eo, eo2, x_var, morig);
						return true;
					} else if(ct_comp == COMPARISON_LESS) {
						if(nr > 1) {set(1, 1, 0, true); return true;}
						else if(nr <= -1) {clear(true); return true;}
					} else if(ct_comp == COMPARISON_GREATER) {
						if(nr < -1) {set(1, 1, 0, true); return true;}
						else if(nr >= 1) {clear(true); return true;}
					} else if(ct_comp == COMPARISON_EQUALS_LESS) {
						if(nr >= 1) {set(1, 1, 0, true); return true;}
						else if(nr < -1) {clear(true); return true;}
					} else if(ct_comp == COMPARISON_EQUALS_GREATER) {
						if(nr <= -1) {set(1, 1, 0, true); return true;}
						else if(nr > 1) {clear(true); return true;}
					}
				}
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ERFINV && CHILD(0).size() == 1) {
				if(CHILD(0)[0].contains(x_var) && CHILD(1).representsReal()) {
					CHILD(0).setToChild(1, true);
					CHILD(1).transformById(FUNCTION_ID_ERF);
					if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
					isolate_x_sub(eo, eo2, x_var, morig);
					return true;
				}
			} else if((CHILD(0).function()->id() == FUNCTION_ID_TAN || CHILD(0).function()->id() == FUNCTION_ID_SIN || CHILD(0).function()->id() == FUNCTION_ID_COS) && CHILD(0).size() == 1 && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathFunction *f = CHILD(0).function();
				CHILD(0).setToChild(1, true);
				bool b_one = false, b_zero = false;
				if(f->id() != FUNCTION_ID_TAN) {
					b_zero = CHILD(1).isZero();
					b_one = CHILD(1).isOne();
				}
				if(b_one) CHILD(1).clear(true);
				if(!b_zero && !b_one) {
					if(f->id() == FUNCTION_ID_SIN) CHILD(1).transformById(FUNCTION_ID_ASIN);
					else if(f->id() == FUNCTION_ID_COS) CHILD(1).transformById(FUNCTION_ID_ACOS);
					else CHILD(1).transformById(FUNCTION_ID_ATAN);
					CHILD(1).calculateFunctions(eo);
				}
				switch(eo.parse_options.angle_unit) {
					case ANGLE_UNIT_DEGREES: {
						EvaluationOptions eo3 = eo2;
						eo3.sync_units = true;
						CHILD(0) /= CALCULATOR->getDegUnit();
						CHILD(0).calculatesub(eo3, eo, true);
						CHILD(1) += Number(180, 1);
						break;
					}
					case ANGLE_UNIT_GRADIANS: {
						EvaluationOptions eo3 = eo2;
						eo3.sync_units = true;
						CHILD(0) /= CALCULATOR->getGraUnit();
						CHILD(0).calculatesub(eo3, eo, true);
						CHILD(1) += Number(200, 1);
						break;
					}
					case ANGLE_UNIT_CUSTOM: {
						if(CALCULATOR->customAngleUnit()) {
							EvaluationOptions eo3 = eo2;
							eo3.sync_units = true;
							CHILD(0) /= CALCULATOR->customAngleUnit();
							CHILD(0).calculatesub(eo3, eo, true);
							CHILD(1) += angle_units_in_turn(eo3, 1, 2);
						} else {
							CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_PI);
							CHILD(1)[1] *= CALCULATOR->getRadUnit();
						}
						break;
					}
					case ANGLE_UNIT_RADIANS: {
						CHILD(0).calculateDivide(CALCULATOR->getRadUnit(), eo2);
						CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_PI);
						break;
					}
					default: {
						CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_PI);
						CHILD(1)[1] *= CALCULATOR->getRadUnit();
					}
				}
				CHILD(1)[1] *= CALCULATOR->getVariableById(VARIABLE_ID_N);
				if(b_zero && f->id() == FUNCTION_ID_COS) {
					add_fraction_of_turn(CHILD(1), eo2, -1, 4, true);
				}
				if(b_one) {
					CHILD(1)[1] *= 2;
					if(f->id() == FUNCTION_ID_SIN) {
						add_fraction_of_turn(CHILD(1), eo2, 1, 4, true);
					}
				}
				if(!b_one && !b_zero && (f->id() == FUNCTION_ID_SIN || f->id() == FUNCTION_ID_COS)) {
					CHILD(1)[1] *= 2;
					MathStructure *malt = new MathStructure(*this);
					(*malt)[1][0].negate();
					if(f->id() == FUNCTION_ID_SIN) {
						add_fraction_of_turn((*malt)[1], eo2, 1, 2, true);
					}
					CHILD(1).calculatesub(eo2, eo, true);
					(*malt)[1].calculatesub(eo2, eo, true);
					CHILDREN_UPDATED;
					malt->childrenUpdated();
					isolate_x_sub(eo, eo2, x_var, morig);
					malt->isolate_x_sub(eo, eo2, x_var, morig);
					if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(malt, OPERATION_LOGICAL_AND);
					else add_nocopy(malt, OPERATION_LOGICAL_OR);
					calculatesub(eo2, eo, false);
				} else {
					CHILDREN_UPDATED;
					CHILD(1).calculatesub(eo2, eo, true);
					CHILDREN_UPDATED;
					isolate_x_sub(eo, eo2, x_var, morig);
				}
				fix_n_multiple(*this, eo2, eo, x_var);
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_SINH && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				CHILD(0).setToChild(1, true);
				CHILD(1).transformById(FUNCTION_ID_ASINH);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				if(CHILD(0).representsNonComplex()) {
					CHILDREN_UPDATED;
					isolate_x_sub(eo, eo2, x_var, morig);
				} else {
					MathStructure *malt = new MathStructure(*this);
					CHILD(1) *= nr_one_i;
					(*malt)[1] *= nr_minus_i;
					CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_PI);
					CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_N); CHILD(1).last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI); CHILD(1).last() *= nr_two;
					(*malt)[1] += CALCULATOR->getVariableById(VARIABLE_ID_N); (*malt)[1].last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI); (*malt)[1].last() *= nr_two;
					CHILD(1) *= nr_one_i;
					(*malt)[1] *= nr_one_i;
					CHILD(1).calculatesub(eo2, eo, true);
					(*malt)[1].calculatesub(eo2, eo, true);
					CHILDREN_UPDATED;
					malt->childrenUpdated();
					isolate_x_sub(eo, eo2, x_var, morig);
					malt->isolate_x_sub(eo, eo2, x_var, morig);
					if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(malt, OPERATION_LOGICAL_AND);
					else add_nocopy(malt, OPERATION_LOGICAL_OR);
					calculatesub(eo2, eo, false);
					fix_n_multiple(*this, eo2, eo, x_var);
				}
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_COSH && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				CHILD(0).setToChild(1, true);
				CHILD(1).transformById(FUNCTION_ID_ACOSH);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				if(CHILD(0).representsNonComplex()) {
					MathStructure *malt = new MathStructure(*this);
					(*malt)[1].calculateNegate(eo2);
					CHILDREN_UPDATED;
					malt->childrenUpdated();
					isolate_x_sub(eo, eo2, x_var, morig);
					malt->isolate_x_sub(eo, eo2, x_var, morig);
					if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(malt, OPERATION_LOGICAL_AND);
					else add_nocopy(malt, OPERATION_LOGICAL_OR);
					calculatesub(eo2, eo, false);
				} else {
					MathStructure *malt = new MathStructure(*this);
					CHILD(1) *= nr_one_i;
					(*malt)[1] *= nr_minus_i;
					CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_N); CHILD(1).last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI); CHILD(1).last() *= nr_two;
					(*malt)[1] += CALCULATOR->getVariableById(VARIABLE_ID_N); (*malt)[1].last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI); (*malt)[1].last() *= nr_two;
					CHILD(1) *= nr_one_i;
					(*malt)[1] *= nr_one_i;
					CHILD(1).calculatesub(eo2, eo, true);
					(*malt)[1].calculatesub(eo2, eo, true);
					CHILDREN_UPDATED;
					malt->childrenUpdated();
					isolate_x_sub(eo, eo2, x_var, morig);
					malt->isolate_x_sub(eo, eo2, x_var, morig);
					if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(malt, OPERATION_LOGICAL_AND);
					else add_nocopy(malt, OPERATION_LOGICAL_OR);
					calculatesub(eo2, eo, false);
					fix_n_multiple(*this, eo2, eo, x_var);
				}
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_TANH && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				CHILD(0).setToChild(1, true);
				CHILD(1).transformById(FUNCTION_ID_ATANH);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				if(CHILD(0).representsNonComplex()) {
					CHILDREN_UPDATED;
					isolate_x_sub(eo, eo2, x_var, morig);
				} else {
					CHILD(1) *= nr_minus_i;
					CHILD(1) += CALCULATOR->getVariableById(VARIABLE_ID_N); CHILD(1).last() *= CALCULATOR->getVariableById(VARIABLE_ID_PI);
					CHILD(1) *= nr_one_i;
					CHILD(1).calculatesub(eo2, eo, true);
					CHILDREN_UPDATED;
					isolate_x_sub(eo, eo2, x_var, morig);
					fix_n_multiple(*this, eo2, eo, x_var);
				}
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ASIN && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathStructure m1(CHILD(1));
				CHILD(0).setToChild(1, true);
				Unit *u = default_angle_unit(eo, false);
				if(u && !contains_angle_unit(CHILD(1), eo.parse_options)) {
					CHILD(1) *= u;
				} else {
					if(!u) u = CALCULATOR->getRadUnit();
					m1 /= u;
					m1.convert(u);
				}
				MathStructure m2(m1);
				CHILD(1).transformById(FUNCTION_ID_SIN);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				m1.transformById(FUNCTION_ID_RE);
				if(m1.calculateFunctions(eo)) m1.calculatesub(eo2, eo, true);
				m2.transformById(FUNCTION_ID_IM);
				if(m2.calculateFunctions(eo)) m2.calculatesub(eo2, eo, true);
				MathStructure *mreq1 = new MathStructure(m1);
				MathStructure *mreq2 = new MathStructure(m1);
				MathStructure *mreq3 = NULL, *mreq4 = NULL, *mreq5 = NULL, *mreq6 = NULL;
				if(!m2.isZero()) {
					if(!m1.representsNonNegative() && !m2.representsNonNegative()) {
						mreq3 = new MathStructure(m1);
						mreq4 = new MathStructure(m2);
					}
					if(!m1.representsNonPositive() && !m2.representsNonPositive()) {
						mreq5 = new MathStructure(m1);
						mreq6 = new MathStructure(m2);
					}
				}
				if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq1->last() *= nr_half;
					mreq2->last() *= nr_minus_half;
					if(mreq3) {
						mreq3->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq3->last() *= nr_minus_half;
					}
					if(mreq5) {
						mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq5->last() *= nr_half;
					}
				} else {
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, angle_units_in_turn(eo2, 1, 4));
					mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, angle_units_in_turn(eo2, -1, 4));
					if(mreq3) mreq3->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, angle_units_in_turn(eo2, -1, 4));
					if(mreq5) mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, angle_units_in_turn(eo2, 1, 4));
				}
				if(mreq4) mreq4->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
				if(mreq6) mreq6->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, m_zero);
				mreq1->isolate_x(eo2, eo);
				mreq2->isolate_x(eo2, eo);
				add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
				add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				if(mreq3) {
					mreq3->isolate_x(eo2, eo);
					mreq4->isolate_x(eo2, eo);
					mreq3->add_nocopy(mreq4, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq3->calculatesub(eo2, eo, false);
					add_nocopy(mreq3, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				if(mreq5) {
					mreq5->isolate_x(eo2, eo);
					mreq6->isolate_x(eo2, eo);
					mreq5->add_nocopy(mreq6, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq5->calculatesub(eo2, eo, false);
					add_nocopy(mreq5, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) convert(CALCULATOR->getRadUnit());
				calculatesub(eo2, eo, false);
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ACOS && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathStructure m1(CHILD(1));
				CHILD(0).setToChild(1, true);
				Unit *u = default_angle_unit(eo, false);
				if(u && !contains_angle_unit(CHILD(1), eo.parse_options)) {
					CHILD(1) *= u;
				} else {
					if(!u) u = CALCULATOR->getRadUnit();
					m1 /= u;
					m1.convert(u);
				}
				MathStructure m2(m1);
				CHILD(1).transformById(FUNCTION_ID_COS);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				m2.transformById(FUNCTION_ID_IM);
				m1.transformById(FUNCTION_ID_RE);
				if(m1.calculateFunctions(eo)) m1.calculatesub(eo2, eo, true);
				if(m2.calculateFunctions(eo)) m2.calculatesub(eo2, eo, true);
				MathStructure *mreq1 = new MathStructure(m1);
				MathStructure *mreq2 = new MathStructure(m1);
				MathStructure *mreq3 = NULL, *mreq4 = NULL, *mreq5 = NULL, *mreq6 = NULL;
				if(!m2.isZero()) {
					if(!m1.representsNonZero() && !m2.representsNonNegative()) {
						mreq3 = new MathStructure(m1);
						mreq4 = new MathStructure(m2);
					}
					if(!m2.representsNonPositive()) {
						mreq5 = new MathStructure(m1);
						mreq6 = new MathStructure(m2);
					}
				}
				if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					if(mreq5) mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
				} else {
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, angle_units_in_turn(eo2, 1, 2));
					if(mreq5) mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, angle_units_in_turn(eo2, 1, 2));
				}
				if(mreq3) mreq3->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, m_zero);
				if(mreq4) mreq4->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
				if(mreq6) mreq6->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, m_zero);
				mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
				mreq1->isolate_x(eo2, eo);
				mreq2->isolate_x(eo2, eo);
				add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
				add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				if(mreq3) {
					mreq3->isolate_x(eo2, eo);
					mreq4->isolate_x(eo2, eo);
					mreq3->add_nocopy(mreq4, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq3->calculatesub(eo2, eo, false);
					add_nocopy(mreq3, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				if(mreq5) {
					mreq5->isolate_x(eo2, eo);
					mreq6->isolate_x(eo2, eo);
					mreq5->add_nocopy(mreq6, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq5->calculatesub(eo2, eo, false);
					add_nocopy(mreq5, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) convert(CALCULATOR->getRadUnit());
				calculatesub(eo2, eo, false);
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ATAN && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathStructure m1(CHILD(1));
				CHILD(0).setToChild(1, true);
				Unit *u = default_angle_unit(eo, false);
				if(u && !contains_angle_unit(CHILD(1), eo.parse_options)) {
					CHILD(1) *= u;
				} else {
					if(!u) u = CALCULATOR->getRadUnit();
					m1 /= u;
					m1.convert(u);
				}
				MathStructure m2(m1);
				CHILD(1).transformById(FUNCTION_ID_TAN);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				m2.transformById(FUNCTION_ID_IM);
				m1.transformById(FUNCTION_ID_RE);
				if(m1.calculateFunctions(eo)) m1.calculatesub(eo2, eo, true);
				if(m2.calculateFunctions(eo)) m2.calculatesub(eo2, eo, true);
				MathStructure *mreq1 = new MathStructure(m1);
				MathStructure *mreq2 = new MathStructure(m1);
				MathStructure *mreq3 = NULL, *mreq4 = NULL, *mreq5 = NULL, *mreq6 = NULL;
				if(!m2.isZero()) {
					if(!m1.representsNonNegative() && !m2.representsPositive()) {
						mreq3 = new MathStructure(m1);
						mreq4 = new MathStructure(m2);
					}
					if(!m1.representsNonPositive() && !m2.representsNegative()) {
						mreq5 = new MathStructure(m1);
						mreq6 = new MathStructure(m2);
					}
				}
				if(DEFAULT_RADIANS(eo.parse_options.angle_unit)) {
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq1->last() *= nr_half;
					mreq2->last() *= nr_minus_half;
					if(mreq3) {
						mreq3->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq3->last() *= nr_minus_half;
					}
					if(mreq5) {
						mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq5->last() *= nr_half;
					}
				} else {
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, angle_units_in_turn(eo2, 1, 4));
					mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, angle_units_in_turn(eo2, -1, 4));
					if(mreq3) mreq3->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, angle_units_in_turn(eo2, -1, 4));
					if(mreq5) mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS : COMPARISON_NOT_EQUALS, angle_units_in_turn(eo2, 1, 4));
				}
				if(mreq4) mreq4->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_GREATER : COMPARISON_LESS, m_zero);
				if(mreq6) mreq6->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, m_zero);
				mreq1->isolate_x(eo2, eo);
				mreq2->isolate_x(eo2, eo);
				add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
				add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				if(mreq3) {
					mreq3->isolate_x(eo2, eo);
					mreq4->isolate_x(eo2, eo);
					mreq3->add_nocopy(mreq4, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq3->calculatesub(eo2, eo, false);
					add_nocopy(mreq3, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				if(mreq5) {
					mreq5->isolate_x(eo2, eo);
					mreq6->isolate_x(eo2, eo);
					mreq5->add_nocopy(mreq6, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq5->calculatesub(eo2, eo, false);
					add_nocopy(mreq5, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				if(NO_DEFAULT_ANGLE_UNIT(eo.parse_options.angle_unit)) convert(CALCULATOR->getRadUnit());
				calculatesub(eo2, eo, false);
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ASINH && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathStructure m1(CHILD(1)), m2(CHILD(1));
				CHILD(0).setToChild(1, true);
				CHILD(1).transformById(FUNCTION_ID_SINH);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				m2.transformById(FUNCTION_ID_IM);
				if(m2.calculateFunctions(eo)) m2.calculatesub(eo2, eo, true);
				if(!m2.isZero()) {
					MathStructure *mreq1 = NULL, *mreq2 = NULL, *mreq3 = NULL, *mreq4 = NULL, *mreq5 = NULL, *mreq6 = NULL;
					m1.transformById(FUNCTION_ID_RE);
					if(m1.calculateFunctions(eo)) m1.calculatesub(eo2, eo, true);
					mreq1 = new MathStructure(m2);
					mreq2 = new MathStructure(m2);
					if(m1.isZero()) {
						mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					} else {
						mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_GREATER : COMPARISON_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq3 = new MathStructure(m2);
						mreq4 = new MathStructure(m2);
						mreq5 = new MathStructure(m1);
						mreq6 = new MathStructure(m1);
						mreq3->transform(ct_comp, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq4->transform(ct_comp, CALCULATOR->getVariableById(VARIABLE_ID_PI));
						mreq3->last() *= nr_half;
						mreq4->last() *= nr_minus_half;
						mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
						mreq6->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, m_zero);
					}
					mreq1->last() *= nr_half;
					mreq2->last() *= nr_minus_half;
					mreq1->isolate_x(eo2, eo);
					mreq2->isolate_x(eo2, eo);
					if(mreq3) {
						mreq3->isolate_x(eo2, eo);
						mreq4->isolate_x(eo2, eo);
						mreq5->isolate_x(eo2, eo);
						mreq6->isolate_x(eo2, eo);
						mreq3->add_nocopy(mreq5, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						mreq3->calculatesub(eo2, eo, false);
						mreq4->add_nocopy(mreq6, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						mreq4->calculatesub(eo2, eo, false);
						mreq1->add_nocopy(mreq3, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
						mreq1->calculatesub(eo2, eo, false);
						mreq2->add_nocopy(mreq4, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
						mreq2->calculatesub(eo2, eo, false);
					}
					add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				calculatesub(eo2, eo, false);
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ACOSH && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathStructure m1(CHILD(1)), m2(CHILD(1));
				CHILD(0).setToChild(1, true);
				CHILD(1).transformById(FUNCTION_ID_COSH);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				MathStructure m1im(m1);
				m1.transformById(FUNCTION_ID_RE);
				m2.transformById(FUNCTION_ID_IM);
				if(m1.calculateFunctions(eo)) m1.calculatesub(eo2, eo, true);
				if(m2.calculateFunctions(eo)) m2.calculatesub(eo2, eo, true);
				if(m1.representsNegative() || (m1.isZero() && m2.representsNegative())) {
					if(ct_comp == COMPARISON_NOT_EQUALS) set(1, 1, 0, true);
					else clear(true);
					return true;
				}
				if(m2.isZero()) {
					if(!m1.representsNonNegative()) {
						MathStructure *mreq1 = new MathStructure(m1);
						mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
						mreq1->isolate_x(eo2, eo);
						add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
						calculatesub(eo2, eo, false);
					}
				} else {
					MathStructure *mreq1 = new MathStructure(m1);
					mreq1->transform(ct_comp, m_zero);
					mreq1->isolate_x(eo2, eo);
					MathStructure *mreq2 = new MathStructure(m1);
					mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, m_zero);
					mreq2->isolate_x(eo2, eo);
					MathStructure *mreq3 = new MathStructure(m2);
					mreq3->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, m_zero);
					mreq3->isolate_x(eo2, eo);
					MathStructure *mreq4 = new MathStructure(m2);
					mreq4->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq4->isolate_x(eo2, eo);
					MathStructure *mreq5 = new MathStructure(m2);
					mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_LESS : COMPARISON_EQUALS_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq5->last().negate();
					mreq5->isolate_x(eo2, eo);
					MathStructure *mreq6 = new MathStructure(m2);
					mreq6->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_GREATER : COMPARISON_EQUALS_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq6->isolate_x(eo2, eo);
					mreq1->add_nocopy(mreq3, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					mreq1->add_nocopy(mreq4, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					mreq1->calculatesub(eo2, eo, false);
					mreq2->add_nocopy(mreq5, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					mreq2->add_nocopy(mreq6, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
					mreq2->calculatesub(eo2, eo, false);
					mreq1->add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq1->calculatesub(eo2, eo, false);
					add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					calculatesub(eo2, eo, false);
				}
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ATANH && (ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_EQUALS)) {
				MathStructure m1(CHILD(1)), m2(CHILD(1));
				CHILD(0).setToChild(1, true);
				CHILD(1).transformById(FUNCTION_ID_TANH);
				if(CHILD(1).calculateFunctions(eo)) CHILD(1).calculatesub(eo2, eo, true);
				CHILDREN_UPDATED;
				isolate_x_sub(eo, eo2, x_var, morig);
				m2.transformById(FUNCTION_ID_IM);
				if(m2.calculateFunctions(eo)) m2.calculatesub(eo2, eo, true);
				if(!m2.isZero()) {
					MathStructure *mreq1 = NULL, *mreq2 = NULL, *mreq3 = NULL, *mreq4 = NULL, *mreq5 = NULL, *mreq6 = NULL;
					m1.transformById(FUNCTION_ID_RE);
					if(m1.calculateFunctions(eo)) m1.calculatesub(eo2, eo, true);
					mreq1 = new MathStructure(m2);
					mreq2 = new MathStructure(m2);
					mreq1->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_GREATER : COMPARISON_LESS, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq2->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq3 = new MathStructure(m2);
					mreq4 = new MathStructure(m2);
					mreq5 = new MathStructure(m1);
					mreq6 = new MathStructure(m1);
					mreq3->transform(ct_comp, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq4->transform(ct_comp, CALCULATOR->getVariableById(VARIABLE_ID_PI));
					mreq3->last() *= nr_half;
					mreq4->last() *= nr_minus_half;
					mreq5->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_GREATER : COMPARISON_LESS, m_zero);
					mreq6->transform(ct_comp == COMPARISON_NOT_EQUALS ? COMPARISON_EQUALS_LESS : COMPARISON_GREATER, m_zero);
					mreq1->last() *= nr_half;
					mreq2->last() *= nr_minus_half;
					mreq1->isolate_x(eo2, eo);
					mreq2->isolate_x(eo2, eo);
					mreq3->isolate_x(eo2, eo);
					mreq4->isolate_x(eo2, eo);
					mreq5->isolate_x(eo2, eo);
					mreq6->isolate_x(eo2, eo);
					mreq3->add_nocopy(mreq5, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					mreq3->calculatesub(eo2, eo, false);
					mreq4->add_nocopy(mreq6, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					mreq4->calculatesub(eo2, eo, false);
					mreq1->add_nocopy(mreq3, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq1->calculatesub(eo2, eo, false);
					mreq2->add_nocopy(mreq4, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_AND : OPERATION_LOGICAL_OR);
					mreq2->calculatesub(eo2, eo, false);
					add_nocopy(mreq1, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND);
					add_nocopy(mreq2, ct_comp == COMPARISON_NOT_EQUALS ? OPERATION_LOGICAL_OR : OPERATION_LOGICAL_AND, true);
				}
				return true;
			} else if(CHILD(0).function()->id() == FUNCTION_ID_ABS && CHILD(0).size() == 1) {
				if(CHILD(0)[0].contains(x_var)) {
					if(CHILD(1).representsComplex() && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
						if(ct_comp == COMPARISON_EQUALS) clear(true);
						else set(1, 1, 0, true);
						return true;
					} else if(CHILD(1).representsNegative()) {
						if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) clear(true);
						else if(ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) set(1, 1, 0, true);
						return true;
					} else if((ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_GREATER) && CHILD(1).representsNonPositive()) {
						if(ct_comp == COMPARISON_LESS) clear(true);
						else set(1, 1, 0, true);
						return true;
					} else if(CHILD(1).representsReal(true)) {
						if(CHILD(0)[0].representsReal(true)) {
							CHILD(0).setToChild(1);
							CHILD_UPDATED(0)
							CHILD(0) ^= nr_two;
							CHILD(0) ^= nr_half;
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						} else if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(1).representsNonNegative(true)) {
							CHILD(0).setToChild(1);
							CHILD_UPDATED(0)
							CHILD(1) *= CALCULATOR->getVariableById(VARIABLE_ID_E);
							CHILD(1).last() ^= CALCULATOR->getVariableById(VARIABLE_ID_N);
							CHILD(1).last().last() *= nr_one_i;
							CHILD(1).calculatesub(eo2, eo);
							isolate_x_sub(eo, eo2, x_var, morig);
							fix_n_multiple(*this, eo2, eo, x_var);
							return true;
						}
					}
				}
			} else if(CHILD(0).function()->id() == FUNCTION_ID_SIGNUM && CHILD(0).size() == 2) {
				if(CHILD(0)[0].contains(x_var) && CHILD(0)[0].representsNonComplex(true)) {
					if(CHILD(1).isZero() && CHILD(0)[1].isZero()) {
						CHILD(0).setToChild(1, true, this, 1);
						isolate_x_sub(eo, eo2, x_var, morig);
						return true;
					}
					if(CHILD(1).isNumber() && !CHILD(1).number().isInterval(false)) {
						if(CHILD(1).number().isOne()) {
							if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_EQUALS_GREATER) ct_comp = (CHILD(0)[1].isOne() ? COMPARISON_EQUALS_GREATER : COMPARISON_GREATER);
							else if(ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS) ct_comp = (CHILD(0)[1].isOne() ? COMPARISON_LESS : COMPARISON_EQUALS_LESS);
							else if(ct_comp == COMPARISON_GREATER) {clear(true); return 1;}
							else {set(1, 1, 0, true); return 1;}
							CHILD(0).setToChild(1, true, this, 1);
							CHILD(1).clear(true);
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						} else if(CHILD(1).number().isMinusOne()) {
							if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_EQUALS_LESS) ct_comp = (CHILD(0)[1].isMinusOne() ?COMPARISON_EQUALS_LESS : COMPARISON_LESS);
							else if(ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_GREATER) ct_comp = (CHILD(0)[1].isMinusOne() ? COMPARISON_GREATER : COMPARISON_EQUALS_GREATER);
							else if(ct_comp == COMPARISON_LESS) {clear(true); return 1;}
							else {set(1, 1, 0, true); return 1;}
							CHILD(0).setToChild(1, true, this, 1);
							CHILD(1).clear(true);
							isolate_x_sub(eo, eo2, x_var, morig);
							return true;
						}
						if(ct_comp == COMPARISON_EQUALS) {
							clear(true);
							return true;
						} else if(ct_comp == COMPARISON_NOT_EQUALS) {
							set(1, 1, 0, true);
							return true;
						}
						if(CHILD(0)[1].isZero() || CHILD(0)[1].isOne() || CHILD(0)[1].isMinusOne()) {
							if(CHILD(1).number().isPositive()) {
								if(CHILD(1).number().isGreaterThan(1)) {
									if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) clear(true);
									else set(1, 1, 0, true);
									return 1;
								}
								if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) ct_comp = (CHILD(0)[1].isOne() ? COMPARISON_EQUALS_GREATER : COMPARISON_GREATER);
								else ct_comp = (CHILD(0)[1].isOne() ? COMPARISON_LESS : COMPARISON_EQUALS_LESS);
								CHILD(0).setToChild(1, true, this, 1);
								CHILD(1).clear(true);
								isolate_x_sub(eo, eo2, x_var, morig);
								return true;
							} else if(CHILD(1).number().isNegative()) {
								if(CHILD(1).number().isLessThan(-1)) {
									if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) set(1, 1, 0, true);
									else clear(true);
									return 1;
								}
								if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) ct_comp = (CHILD(0)[1].isMinusOne() ? COMPARISON_GREATER : COMPARISON_EQUALS_GREATER);
								else ct_comp = (CHILD(0)[1].isMinusOne() ? COMPARISON_EQUALS_LESS : COMPARISON_LESS);
								CHILD(0).setToChild(1, true, this, 1);
								CHILD(1).clear(true);
								isolate_x_sub(eo, eo2, x_var, morig);
								return true;
							}
						}
					}
				}
			}
			break;
		}
		default: {}
	}
	return false;
}

bool contains_unsolved_equals(const MathStructure &mstruct, const MathStructure &x_var) {
	if(mstruct.isComparison()) {
		return mstruct.comparisonType() == COMPARISON_EQUALS && mstruct[0] != x_var && mstruct[1] != x_var && mstruct.contains(x_var);
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(contains_unsolved_equals(mstruct[i], x_var)) return true;
	}
	return false;
}

bool sync_sine(MathStructure &mstruct, const EvaluationOptions &eo, const MathStructure &x_var, bool use_cos, bool b_hyp = false, const MathStructure &mstruct_parent = m_undefined) {
	if(!mstruct_parent.isUndefined() && mstruct.isFunction() && mstruct.function()->id() == (b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN) && mstruct.size() == 1 && mstruct[0].contains(x_var)) {
		MathStructure m_half(mstruct);
		m_half[0].calculateDivide(nr_two, eo);
		bool b = mstruct_parent.contains(m_half);
		if(!b) {
			m_half.setFunctionId(b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS);
			b = mstruct_parent.contains(m_half);
			m_half.setFunctionId(b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN);
		}
		if(b) {
			mstruct = m_half;
			MathStructure *m_cos = new MathStructure(mstruct);
			(*m_cos).setFunctionId(b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS);
			mstruct.multiply_nocopy(m_cos);
			mstruct.multiply(nr_two);
			return true;
		}
	} else if(mstruct.isPower() && mstruct[0].isFunction() && mstruct[1].isNumber() && mstruct[1].number().isEven() && mstruct[0].size() == 1) {
		if(!mstruct_parent.isUndefined() && mstruct[0].function()->id() == (b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN) && mstruct[0][0].contains(x_var)) {
			MathStructure m_half(mstruct[0]);
			m_half[0].calculateDivide(nr_two, eo);
			bool b = mstruct_parent.contains(m_half);
			if(!b) {
				m_half.setFunctionId(b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS);
				b = mstruct_parent.contains(m_half);
				m_half.setFunctionId(b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN);
			}
			if(b) {
				MathStructure *mmul = new MathStructure(2, 1, 0);
				mmul->raise(mstruct[1]);
				mstruct[0] = m_half;
				MathStructure *m_cos = new MathStructure(mstruct);
				(*m_cos)[0].setFunctionId(b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS);
				mstruct.multiply_nocopy(m_cos);
				mstruct.multiply_nocopy(mmul);
				sync_sine(mstruct, eo, x_var, use_cos, b_hyp, mstruct_parent);
				return true;
			}
		}
		if(mstruct[0].function()->id() == (b_hyp ? FUNCTION_ID_TANH : FUNCTION_ID_TAN) && mstruct[0][0].contains(x_var)) {
			mstruct[0].setFunctionId(FUNCTION_ID_SIN);
			MathStructure *m_cos = new MathStructure(mstruct);
			(*m_cos)[0].setFunctionId(FUNCTION_ID_COS);
			(*m_cos)[1].number().negate();
			mstruct.multiply_nocopy(m_cos);
			sync_sine(mstruct, eo, x_var, use_cos, b_hyp, mstruct_parent);
			return true;
		}
		if(mstruct[0].function()->id() == (use_cos ? (b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN) : (b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS)) && mstruct[0][0].contains(x_var)) {
			mstruct[0].setFunctionId(use_cos ? (b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS) : (b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN));
			Number nr_pow = mstruct[1].number();
			nr_pow /= 2;
			mstruct[1].set(nr_two, true);
			if(b_hyp) {
				if(use_cos) mstruct += m_minus_one;
				else mstruct += m_one;
			} else {
				mstruct.negate();
				mstruct += m_one;
			}
			if(!nr_pow.isOne()) {
				mstruct ^= nr_pow;
			}
			return true;
		}
	}
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(CALCULATOR->aborted()) return false;
		if(sync_sine(mstruct[i], eo, x_var, use_cos, b_hyp, mstruct_parent.isUndefined() ? mstruct : mstruct_parent)) b = true;
	}
	return b;
}
void sync_find_cos_sin(const MathStructure &mstruct, const MathStructure &x_var, bool &b_sin, bool &b_cos, bool b_hyp = false) {
	if(mstruct.isFunction() && mstruct.size() == 1) {
		if(!b_sin && mstruct.function()->id() == (b_hyp ? FUNCTION_ID_SINH : FUNCTION_ID_SIN) && mstruct[0].contains(x_var)) {
			b_sin = true;
		} else if(!b_cos && mstruct.function()->id() == (b_hyp ? FUNCTION_ID_COSH : FUNCTION_ID_COS) && mstruct[0].contains(x_var)) {
			b_cos = true;
		}
		if(b_sin && b_cos) return;
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		sync_find_cos_sin(mstruct[i], x_var, b_sin, b_cos, b_hyp);
		if(b_sin && b_cos) return;
	}
}
bool sync_trigonometric_functions(MathStructure &mstruct, const EvaluationOptions &eo, const MathStructure &x_var, bool use_cos = false) {
	bool b_ret = false;
	if(sync_sine(mstruct, eo, x_var, use_cos)) b_ret = true;
	if(sync_sine(mstruct, eo, x_var, use_cos, true)) b_ret = true;
	return b_ret;
}

bool MathStructure::isolate_x(const EvaluationOptions &eo, const MathStructure &x_varp, bool check_result) {
	return isolate_x(eo, eo, x_varp, check_result);
}
bool MathStructure::isolate_x(const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_varp, bool check_result) {
	if(isProtected()) return false;
	if(!isComparison()) {
		if(m_type == STRUCT_FUNCTION && o_function->id() == FUNCTION_ID_D_SOLVE) return false;
		bool b = false;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isolate_x(eo, feo, x_varp, check_result)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}
	MathStructure x_var(x_varp);
	if(x_var.isUndefined()) {
		const MathStructure *x_var2;
		if(eo.isolate_var && contains(*eo.isolate_var)) x_var2 = eo.isolate_var;
		else x_var2 = &find_x_var();
		if(x_var2->isUndefined()) return false;
		x_var = *x_var2;
	}

	if(CHILD(0) == x_var && !CHILD(1).contains(x_var)) return true;
	if(!CHILD(1).isZero()) {
		CHILD(0).calculateSubtract(CHILD(1), eo);
		CHILD(1).clear(true);
		CHILDREN_UPDATED
	}

	if(eo.expand > 0) simplify_functions(*this, eo, feo, x_var);

	EvaluationOptions eo2 = eo;
	eo2.calculate_functions = false;
	eo2.test_comparisons = false;
	eo2.isolate_x = false;

	if(check_result && CHILD(1).isZero() && CHILD(0).isAddition()) {
		bool found_1x = false;
		for(size_t i = 0; i < CHILD(0).size(); i++) {
			if(CHILD(0)[i] == x_var) {
				found_1x = true;
			} else if(CHILD(0)[i].contains(x_var)) {
				found_1x = false;
				break;
			}
		}
		if(found_1x) check_result = false;
	}

	MathStructure msave(*this);

	bool b = isolate_x_sub(feo, eo2, x_var);

	if(CALCULATOR->aborted()) return !check_result && b;

	if(eo.expand > 0 && contains_unsolved_equals(*this, x_var)) {
		MathStructure mtest(msave);
		EvaluationOptions eo3 = eo;
		eo3.transform_trigonometric_functions = false;
		eo2.transform_trigonometric_functions = false;
		bool do_cos = true;
		if(sync_trigonometric_functions(mtest, eo3, x_var, false)) {
			mtest.calculatesub(eo3, feo);
			if(CALCULATOR->aborted()) return !check_result && b;
			if(eo.do_polynomial_division) do_simplification(mtest, eo3, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
			if(CALCULATOR->aborted()) return !check_result && b;
			if(mtest.isComparison() && mtest.isolate_x_sub(feo, eo2, x_var) && !contains_unsolved_equals(mtest, x_var)) {
				set(mtest);
				b = true;
				do_cos = false;
			} else if(CALCULATOR->aborted()) {
				return !check_result && b;
			}
		}
		if(do_cos) {
			mtest = msave;
			if(sync_trigonometric_functions(mtest, eo3, x_var, true)) {
				mtest.calculatesub(eo3, feo);
				if(CALCULATOR->aborted()) return !check_result && b;
				if(eo.do_polynomial_division) do_simplification(mtest, eo3, true, eo.structuring == STRUCTURING_NONE || eo.structuring == STRUCTURING_FACTORIZE, false, true, true);
				if(CALCULATOR->aborted()) return !check_result && b;
				if(mtest.isComparison() && mtest.isolate_x_sub(feo, eo2, x_var) && !contains_unsolved_equals(mtest, x_var)) {
					b = true;
					set(mtest);
				}
			}
		}
	}
	fix_n_multiple(*this, eo, feo, x_var);
	if(check_result && b) {
		b = test_comparisons(msave, *this, x_var, eo) >= 0;
	}

	return b;

}

