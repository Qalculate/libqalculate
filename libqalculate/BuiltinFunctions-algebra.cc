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
#include "Variable.h"
#include "Unit.h"

#include <sstream>
#include <time.h>
#include <limits>
#include <algorithm>

#include "MathStructure-support.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

SumFunction::SumFunction() : MathFunction("sum", 3, 4) {
	Argument *arg = new IntegerArgument();
	arg->setHandleVector(false);
	setArgumentDefinition(2, arg);
	arg = new IntegerArgument();
	arg->setHandleVector(false);
	setArgumentDefinition(3, arg);
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "undefined");
	setCondition("\\z >= \\y");
}
int SumFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	MathStructure m1(vargs[0]);
	EvaluationOptions eo2 = eo;
	eo2.calculate_functions = false;
	eo2.expand = false;
	Number i_nr(vargs[1].number());
	if(eo2.approximation == APPROXIMATION_TRY_EXACT) {
		Number nr(vargs[2].number());
		nr.subtract(i_nr);
		if(nr.isGreaterThan(100)) eo2.approximation = APPROXIMATION_APPROXIMATE;
	}
	MathStructure mbak(m1);
	vector<Variable*> vars;
	if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
		while(true) {
			Variable *v = NULL;
			Variable *uv = find_interval_replace_var_comp(m1, eo, &v);
			if(!uv) break;
			if(v) m1.replace(v, uv);
			vars.push_back(uv);
		}
	}
	CALCULATOR->beginTemporaryStopMessages();
	m1.eval(eo2);
	if(calculate_userfunctions(m1, vargs[3], eo)) {
		if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
			while(true) {
				Variable *v = NULL;
				Variable *uv = find_interval_replace_var_comp(m1, eo, &v);
				if(!uv) break;
				if(v) m1.replace(v, uv);
				vars.push_back(uv);
			}
		}
		m1.calculatesub(eo2, eo2, true);
	}
	int im = 0;
	if(CALCULATOR->endTemporaryStopMessages(NULL, &im) > 0 || im > 0) m1 = mbak;
	eo2.calculate_functions = eo.calculate_functions;
	eo2.expand = eo.expand;
	mstruct.clear();
	MathStructure mstruct_calc;
	bool started = false;
	while(i_nr.isLessThanOrEqualTo(vargs[2].number())) {
		if(CALCULATOR->aborted()) {
			if(!started) {
				for(size_t i = 0; i < vars.size(); i++) vars[i]->destroy();
				return 0;
			} else if(i_nr != vargs[2].number()) {
				MathStructure mmin(i_nr);
				mstruct.add(MathStructure(this, &vargs[0], &mmin, &vargs[2], &vargs[3], NULL), true);
				break;
			}
		}
		mstruct_calc.set(m1);
		mstruct_calc.replace(vargs[3], i_nr);
		mstruct_calc.eval(eo2);
		if(started) {
			mstruct.calculateAdd(mstruct_calc, eo2);
		} else {
			mstruct = mstruct_calc;
			mstruct.calculatesub(eo2, eo2);
			started = true;
		}
		i_nr += 1;
	}
	for(size_t i = 0; i < vars.size(); i++) {
		if(vars[i]->isKnown()) mstruct.replace(vars[i], ((KnownVariable*) vars[i])->get());
		else mstruct.replace(vars[i], ((UnknownVariable*) vars[i])->interval());
		vars[i]->destroy();
	}
	return 1;

}
ProductFunction::ProductFunction() : MathFunction("product", 3, 4) {
	Argument *arg = new IntegerArgument();
	arg->setHandleVector(false);
	setArgumentDefinition(2, arg);
	arg = new IntegerArgument();
	arg->setHandleVector(false);
	setArgumentDefinition(3, arg);
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "undefined");
	setCondition("\\z >= \\y");
}
int ProductFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	MathStructure m1(vargs[0]);
	EvaluationOptions eo2 = eo;
	eo2.calculate_functions = false;
	eo2.expand = false;
	Number i_nr(vargs[1].number());
	if(eo2.approximation == APPROXIMATION_TRY_EXACT) {
		Number nr(vargs[2].number());
		nr.subtract(i_nr);
		if(nr.isGreaterThan(100)) eo2.approximation = APPROXIMATION_APPROXIMATE;
	}
	MathStructure mbak(m1);
	vector<Variable*> vars;
	if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
		while(true) {
			Variable *v = NULL;
			Variable *uv = find_interval_replace_var_comp(m1, eo, &v);
			if(!uv) break;
			if(v) m1.replace(v, uv);
			vars.push_back(uv);
		}
	}
	CALCULATOR->beginTemporaryStopMessages();
	m1.eval(eo2);
	if(calculate_userfunctions(m1, vargs[3], eo)) {
		if(eo.interval_calculation == INTERVAL_CALCULATION_VARIANCE_FORMULA || eo.interval_calculation == INTERVAL_CALCULATION_INTERVAL_ARITHMETIC) {
			while(true) {
				Variable *v = NULL;
				Variable *uv = find_interval_replace_var_comp(m1, eo, &v);
				if(!uv) break;
				if(v) m1.replace(v, uv);
				vars.push_back(uv);
			}
		}
		m1.calculatesub(eo2, eo2, true);
	}
	int im = 0;
	if(CALCULATOR->endTemporaryStopMessages(NULL, &im) || im > 0) m1 = mbak;
	eo2.calculate_functions = eo.calculate_functions;
	eo2.expand = eo.expand;
	mstruct.clear();
	MathStructure mstruct_calc;
	bool started = false;
	while(i_nr.isLessThanOrEqualTo(vargs[2].number())) {
		if(CALCULATOR->aborted()) {
			if(!started) {
				for(size_t i = 0; i < vars.size(); i++) vars[i]->destroy();
				return 0;
			} else if(i_nr != vargs[2].number()) {
				MathStructure mmin(i_nr);
				mstruct.multiply(MathStructure(this, &vargs[0], &mmin, &vargs[2], &vargs[3], NULL), true);
				break;
			}
		}
		mstruct_calc.set(m1);
		mstruct_calc.replace(vargs[3], i_nr);
		mstruct_calc.eval(eo2);
		if(started) {
			mstruct.calculateMultiply(mstruct_calc, eo2);
		} else {
			mstruct = mstruct_calc;
			mstruct.calculatesub(eo2, eo2);
			started = true;
		}
		i_nr += 1;
	}
	for(size_t i = 0; i < vars.size(); i++) {
		if(vars[i]->isKnown()) mstruct.replace(vars[i], ((KnownVariable*) vars[i])->get());
		else mstruct.replace(vars[i], ((UnknownVariable*) vars[i])->interval());
		vars[i]->destroy();
	}
	return 1;

}

SolveFunction::SolveFunction() : MathFunction("solve", 1, 2) {
	setArgumentDefinition(2, new SymbolicArgument());
	setDefaultValue(2, "undefined");
}
bool is_comparison_structure(const MathStructure &mstruct, const MathStructure &xvar, bool *bce = NULL, bool do_bce_or = false);
bool is_comparison_structure(const MathStructure &mstruct, const MathStructure &xvar, bool *bce, bool do_bce_or) {
	if(mstruct.isComparison()) {
		if(bce) *bce = mstruct.comparisonType() == COMPARISON_EQUALS && mstruct[0] == xvar;
		return true;
	}
	if(bce && do_bce_or && mstruct.isLogicalOr()) {
		*bce = true;
		for(size_t i = 0; i < mstruct.size(); i++) {
			bool bcei = false;
			if(!is_comparison_structure(mstruct[i], xvar, &bcei, false)) return false;
			if(!bcei) *bce = false;
		}
		return true;
	}
	if(bce) *bce = false;
	if(mstruct.isLogicalAnd()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(is_comparison_structure(mstruct[i], xvar)) return true;
		}
		return true;
	} else if(mstruct.isLogicalOr()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(!is_comparison_structure(mstruct[i], xvar)) return false;
		}
		return true;
	}
	return false;
}

MathStructure *solve_handle_logical_and(MathStructure &mstruct, MathStructure **mtruefor, ComparisonType ct, bool &b_partial, const MathStructure &x_var) {
	MathStructure *mcondition = NULL;
	for(size_t i2 = 0; i2 < mstruct.size(); ) {
		if(ct == COMPARISON_EQUALS) {
			if(mstruct[i2].isComparison() && ct == mstruct[i2].comparisonType() && mstruct[i2][0].contains(x_var)) {
				if(mstruct[i2][0] == x_var) {
					if(mstruct.size() == 2) {
						if(i2 == 0) {
							mstruct[1].ref();
							mcondition = &mstruct[1];
						} else {
							mstruct[0].ref();
							mcondition = &mstruct[0];
						}
					} else {
						mcondition = new MathStructure();
						mcondition->set_nocopy(mstruct);
						mcondition->delChild(i2 + 1);
					}
					mstruct.setToChild(i2 + 1, true);
					break;
				} else {
					b_partial = true;
					i2++;
				}
			} else {
				i2++;
			}
		} else {
			if(mstruct[i2].isComparison() && mstruct[i2][0].contains(x_var)) {
				i2++;
			} else {
				mstruct[i2].ref();
				if(mcondition) {
					mcondition->add_nocopy(&mstruct[i2], OPERATION_LOGICAL_AND, true);
				} else {
					mcondition = &mstruct[i2];
				}
				mstruct.delChild(i2 + 1);
			}
		}
	}
	if(ct == COMPARISON_EQUALS) {
		if(mstruct.isLogicalAnd()) {
			MathStructure *mtmp = new MathStructure();
			mtmp->set_nocopy(mstruct);
			if(!(*mtruefor)) {
				*mtruefor = mtmp;
			} else {
				(*mtruefor)->add_nocopy(mtmp, OPERATION_LOGICAL_OR, true);
			}
			mstruct.clear(true);
		}
	} else {
		if(mstruct.size() == 1) {
			mstruct.setToChild(1, true);
			if(ct != COMPARISON_EQUALS) mstruct.setProtected();
		} else if(mstruct.size() == 0) {
			mstruct.clear(true);
			if(!(*mtruefor)) {
				*mtruefor = mcondition;
			} else {
				(*mtruefor)->add_nocopy(mcondition, OPERATION_LOGICAL_OR, true);
			}
			mcondition = NULL;
		} else if(ct != COMPARISON_EQUALS) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				mstruct[i].setProtected();
			}
		}
	}
	return mcondition;
}

void simplify_constant(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &y_var, const MathStructure &c_var, bool in_comparison = false, bool in_or = false, bool in_and = false);
void simplify_constant(MathStructure &mstruct, const MathStructure &x_var, const MathStructure &y_var, const MathStructure &c_var, bool in_comparison, bool in_or, bool in_and) {
	if(!in_comparison && mstruct.isComparison()) {
		if(mstruct[0] == c_var) {
			if(in_or) mstruct.clear(true);
			else mstruct.set(1, 1, 0);
		} else if(mstruct[0] == y_var) {
			if(mstruct[1].contains(y_var, true) <= 0) simplify_constant(mstruct[1], x_var, y_var, c_var, true);
		} else if(mstruct[0].contains(y_var, true) <= 0 && mstruct.contains(c_var, true) > 0) {
			if(in_or) mstruct.clear(true);
			else mstruct.set(1, 1, 0);
		}
	}
	if(in_comparison) {
		if(mstruct.contains(y_var, true) <= 0 && mstruct.contains(x_var, true) <= 0 && mstruct.contains(c_var, true) > 0) {
			mstruct = c_var;
			return;
		}
	}
	if(in_comparison) {
		int n_c = 0, b_cx = false;
		size_t i_c = 0;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].contains(c_var, true) > 0) {
				n_c++;
				i_c = i;
				if(!b_cx && mstruct[i].contains(x_var, true) > 0) {
					b_cx = true;
				}
			}
		}
		if(!b_cx && n_c >= 1 && (mstruct.isAddition() || mstruct.isMultiplication())) {
			bool b_c = false;
			for(size_t i = 0; i < mstruct.size();) {
				if(mstruct[i].contains(c_var, true) > 0) {
					if(b_c) {
						mstruct.delChild(i + 1);
					} else {
						b_c = true;
						mstruct[i] = c_var;
						i++;
					}
				} else if(mstruct[i].contains(x_var, true) <= 0) {
					mstruct.delChild(i + 1);
				} else {
					i++;
				}
			}
			if(mstruct.size() == 1) mstruct.setToChild(1, true);
		} else if(n_c == 1) {
			if(b_cx) simplify_constant(mstruct[i_c], x_var, y_var, c_var, true);
			else mstruct[i_c] = c_var;
		}
	} else {
		for(size_t i = 0; i < mstruct.size(); i++) {
			simplify_constant(mstruct[i], x_var, y_var, c_var, false, mstruct.isLogicalOr(), mstruct.isLogicalAnd());
		}
	}
}

int test_equation(MathStructure &mstruct, const EvaluationOptions &eo, const MathStructure &x_var, const MathStructure &y_var, const MathStructure &x_value, const MathStructure &y_value) {
	if(mstruct.isComparison() && mstruct.comparisonType() == COMPARISON_EQUALS && mstruct[0] == y_var) {
		MathStructure mtest(mstruct);
		mtest.replace(x_var, x_value);
		MathStructure mtest2(y_var);
		mtest2.transform(COMPARISON_EQUALS, y_value);
		CALCULATOR->beginTemporaryStopMessages();
		EvaluationOptions eo2 = eo;
		eo2.approximation = APPROXIMATION_APPROXIMATE;
		mtest.calculateFunctions(eo2);
		mtest2.calculateFunctions(eo2);
		int b = test_comparisons(mtest, mtest2, y_var, eo);
		CALCULATOR->endTemporaryStopMessages();
		if(!b) mstruct.clear(true);
		return b;
	}
	bool b_ret = 0;
	for(size_t i = 0; i < mstruct.size(); i++) {
		int b = test_equation(mstruct[i], eo, x_var, y_var, x_value, y_value);
		if(b < 0) return b;
		else if(b > 0) b_ret = 1;
	}
	return b_ret;
}

int solve_equation(MathStructure &mstruct, const MathStructure &m_eqn, const MathStructure &y_var, const EvaluationOptions &eo, bool dsolve = false, const MathStructure &x_var = m_undefined, const MathStructure &c_var = m_undefined, const MathStructure &x_value = m_undefined, const MathStructure &y_value = m_undefined) {

	int itry = 0;
	int ierror = 0;
	int first_error = 0;

	Assumptions *assumptions = NULL;
	bool assumptions_added = false;
	AssumptionSign as = ASSUMPTION_SIGN_UNKNOWN;
	AssumptionType at = ASSUMPTION_TYPE_NUMBER;
	MathStructure msave;
	string strueforall;

	while(true) {

		if(itry == 1) {
			if(ierror == 1) {
				if(!dsolve) CALCULATOR->error(true, _("No equality or inequality to solve. The entered expression to solve is not correct (e.g. \"x + 5 = 3\" is correct)"), NULL);
				return -1;
			} else {
				first_error = ierror;
				msave = mstruct;
			}
		}

		itry++;

		if(itry == 2) {
			if(y_var.isVariable() && y_var.variable()->subtype() == SUBTYPE_UNKNOWN_VARIABLE) {
				assumptions = ((UnknownVariable*) y_var.variable())->assumptions();
				if(!assumptions) {
					assumptions = new Assumptions();
					assumptions->setSign(CALCULATOR->defaultAssumptions()->sign());
					assumptions->setType(CALCULATOR->defaultAssumptions()->type());
					((UnknownVariable*) y_var.variable())->setAssumptions(assumptions);
					assumptions_added = true;
				}
			} else {
				assumptions = CALCULATOR->defaultAssumptions();
			}
			if(assumptions->sign() != ASSUMPTION_SIGN_UNKNOWN) {
				as = assumptions->sign();
				assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);
			} else {
				itry++;
			}
		}
		if(itry == 3) {
			if(assumptions->type() > ASSUMPTION_TYPE_NUMBER) {
				at = assumptions->type();
				assumptions->setType(ASSUMPTION_TYPE_NUMBER);
				as = assumptions->sign();
				assumptions->setSign(ASSUMPTION_SIGN_UNKNOWN);
			} else {
				itry++;
			}
		}

		if(itry > 3) {
			if(as != ASSUMPTION_SIGN_UNKNOWN) assumptions->setSign(as);
			if(at > ASSUMPTION_TYPE_NUMBER) assumptions->setType(at);
			if(assumptions_added) ((UnknownVariable*) y_var.variable())->setAssumptions(NULL);
			switch(first_error) {
				case 2: {
					CALCULATOR->error(true, _("The comparison is true for all %s (with current assumptions)."), format_and_print(y_var).c_str(), NULL);
					break;
				}
				case 3: {
					CALCULATOR->error(true, _("No possible solution was found (with current assumptions)."), NULL);
					break;
				}
				case 4: {
					CALCULATOR->error(true, _("Was unable to completely isolate %s."), format_and_print(y_var).c_str(), NULL);
					break;
				}
				case 7: {
					CALCULATOR->error(false, _("The comparison is true for all %s if %s."), format_and_print(y_var).c_str(), strueforall.c_str(), NULL);
					break;
				}
				default: {
					CALCULATOR->error(true, _("Was unable to isolate %s."), format_and_print(y_var).c_str(), NULL);
					break;
				}
			}
			mstruct = msave;
			return -1;
		}

		ComparisonType ct = COMPARISON_EQUALS;

		bool b = false;
		bool b_partial = false;

		if(m_eqn.isComparison()) {
			ct = m_eqn.comparisonType();
			mstruct = m_eqn;
			b = true;
		} else if(m_eqn.isLogicalAnd() && m_eqn.size() > 0 && m_eqn[0].isComparison()) {
			ct = m_eqn[0].comparisonType();
			for(size_t i = 0; i < m_eqn.size(); i++) {
				if(m_eqn[i].isComparison() && m_eqn[i].contains(y_var, true) > 0) {
					ct = m_eqn[i].comparisonType();
					break;
				}
			}
			mstruct = m_eqn;
			b = true;
		} else if(m_eqn.isLogicalOr() && m_eqn.size() > 0 && m_eqn[0].isComparison()) {
			ct = m_eqn[0].comparisonType();
			mstruct = m_eqn;
			b = true;
		} else if(m_eqn.isLogicalOr() && m_eqn.size() > 0 && m_eqn[0].isLogicalAnd() && m_eqn[0].size() > 0 && m_eqn[0][0].isComparison()) {
			ct = m_eqn[0][0].comparisonType();
			for(size_t i = 0; i < m_eqn[0].size(); i++) {
				if(m_eqn[0][i].isComparison() && m_eqn[0][i].contains(y_var, true) > 0) {
					ct = m_eqn[0][i].comparisonType();
					break;
				}
			}
			mstruct = m_eqn;
			b = true;
		} else if(m_eqn.isVariable() && m_eqn.variable()->isKnown() && (eo.approximation != APPROXIMATION_EXACT || !m_eqn.variable()->isApproximate()) && ((KnownVariable*) m_eqn.variable())->get().isComparison()) {
			mstruct = ((KnownVariable*) m_eqn.variable())->get();
			mstruct.unformat();
			ct = m_eqn.comparisonType();
			b = true;
		} else {
			EvaluationOptions eo2 = eo;
			eo2.test_comparisons = false;
			eo2.assume_denominators_nonzero = false;
			eo2.isolate_x = false;
			mstruct = m_eqn;
			mstruct.eval(eo2);
			if(mstruct.isComparison()) {
				ct = mstruct.comparisonType();
				b = true;
			} else if(mstruct.isLogicalAnd() && mstruct.size() > 0 && mstruct[0].isComparison()) {
				ct = mstruct[0].comparisonType();
				b = true;
			} else if(mstruct.isLogicalOr() && mstruct.size() > 0 && mstruct[0].isComparison()) {
				ct = mstruct[0].comparisonType();
				b = true;
			} else if(mstruct.isLogicalOr() && mstruct.size() > 0 && mstruct[0].isLogicalAnd() && mstruct[0].size() > 0 && mstruct[0][0].isComparison()) {
				ct = mstruct[0][0].comparisonType();
				b = true;
			}
		}

		if(!b) {
			ierror = 1;
			continue;
		}

		EvaluationOptions eo2 = eo;
		eo2.isolate_var = &y_var;
		eo2.isolate_x = true;
		eo2.test_comparisons = true;
		mstruct.eval(eo2);
		if(dsolve) {
			if(x_value.isUndefined() || y_value.isUndefined()) {
				simplify_constant(mstruct, x_var, y_var, c_var);
				mstruct.eval(eo2);
			} else {
				int test_r = test_equation(mstruct, eo2, x_var, y_var, x_value, y_value);
				if(test_r < 0) {
					ierror = 8;
					continue;
				} else if(test_r > 0) {
					mstruct.eval(eo2);
				}
			}
		}

		if(mstruct.isOne()) {
			ierror = 2;
			continue;
		} else if(mstruct.isZero()) {
			ierror = 3;
			continue;
		}

		if(mstruct.isComparison()) {
			if((ct == COMPARISON_EQUALS && mstruct.comparisonType() != COMPARISON_EQUALS) || !mstruct.contains(y_var)) {
				if(itry == 1) {
					strueforall = format_and_print(mstruct);
				}
				ierror = 7;
				continue;
			} else if(ct == COMPARISON_EQUALS && mstruct[0] != y_var) {
				ierror = 4;
				continue;
			}
			if(ct == COMPARISON_EQUALS) {
				mstruct.setToChild(2, true);
			} else {
				mstruct.setProtected();
			}
			if(itry > 1) {
				assumptions->setSign(as);
				if(itry == 2) {
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed sign was therefore temporarily set as unknown."), format_and_print(y_var).c_str(), NULL);
				} else if(itry == 3) {
					assumptions->setType(at);
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed type and sign was therefore temporarily set as unknown."), format_and_print(y_var).c_str(), NULL);
				}
				if(assumptions_added) ((UnknownVariable*) y_var.variable())->setAssumptions(NULL);
			}
			return 1;
		} else if(mstruct.isLogicalAnd()) {
			MathStructure *mtruefor = NULL;
			bool b_partial;
			MathStructure mcopy(mstruct);
			MathStructure *mcondition = solve_handle_logical_and(mstruct, &mtruefor, ct, b_partial, y_var);
			if((!mstruct.isComparison() && !mstruct.isLogicalAnd()) || (ct == COMPARISON_EQUALS && (!mstruct.isComparison() || mstruct.comparisonType() != COMPARISON_EQUALS || mstruct[0] != y_var)) || !mstruct.contains(y_var)) {
				if(mtruefor) delete mtruefor;
				if(mcondition) delete mcondition;
				if(b_partial) {
					ierror = 4;
				} else {
					ierror = 5;
				}
				mstruct = mcopy;
				continue;
			}
			if(itry > 1) {
				assumptions->setSign(as);
				if(itry == 2) {
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed sign was therefore temporarily set as unknown."), format_and_print(y_var).c_str(), NULL);
				} else if(itry == 3) {
					assumptions->setType(at);
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed type and sign was therefore temporarily set as unknown."), format_and_print(y_var).c_str(), NULL);
				}
				if(assumptions_added) ((UnknownVariable*) y_var.variable())->setAssumptions(NULL);
			}
			if(mcondition) {
				CALCULATOR->error(false, _("The solution requires that %s."), format_and_print(*mcondition).c_str(), NULL);
				delete mcondition;
			}
			if(mtruefor) {
				CALCULATOR->error(false, _("The comparison is true for all %s if %s."), format_and_print(y_var).c_str(), format_and_print(*mtruefor).c_str(), NULL);
				delete mtruefor;
			}
			if(ct == COMPARISON_EQUALS) mstruct.setToChild(2, true);
			return 1;
		} else if(mstruct.isLogicalOr()) {
			MathStructure mcopy(mstruct);
			MathStructure *mtruefor = NULL;
			vector<MathStructure*> mconditions;
			for(size_t i = 0; i < mstruct.size(); ) {
				MathStructure *mcondition = NULL;
				bool b_and = false;
				if(mstruct[i].isLogicalAnd()) {
					mcondition = solve_handle_logical_and(mstruct[i], &mtruefor, ct, b_partial, y_var);
					b_and = true;
				}
				if(!mstruct[i].isZero()) {
					for(size_t i2 = 0; i2 < i; i2++) {
						if(mstruct[i2] == mstruct[i]) {
							mstruct[i].clear();
							if(mcondition && mconditions[i2]) {
								mconditions[i2]->add_nocopy(mcondition, OPERATION_LOGICAL_OR, true);
							}
							break;
						}
					}
				}
				bool b_del = false;
				if((!mstruct[i].isComparison() && !mstruct[i].isLogicalAnd()) || (ct == COMPARISON_EQUALS && (!mstruct[i].isComparison() || mstruct[i].comparisonType() != COMPARISON_EQUALS)) || !mstruct[i].contains(y_var)) {
					b_del = true;
				} else if(ct == COMPARISON_EQUALS && mstruct[i][0] != y_var) {
					b_partial = true;
					b_del = true;
				}
				if(b_del) {
					if(!mstruct[i].isZero()) {
						mstruct[i].ref();
						if(!mtruefor) {
							mtruefor = &mstruct[i];
						} else {
							mtruefor->add_nocopy(&mstruct[i], OPERATION_LOGICAL_OR, true);
						}
					}
					mstruct.delChild(i + 1);
				} else {
					mconditions.push_back(mcondition);
					if(!b_and && ct != COMPARISON_EQUALS) mstruct[i].setProtected();
					i++;
				}
			}
			if(ct == COMPARISON_EQUALS) {
				for(size_t i = 0; i < mstruct.size(); i++) {
					if(mstruct[i].isComparison() && mstruct[i].comparisonType() == ct) mstruct[i].setToChild(2, true);
				}
			}
			if(mstruct.size() == 1) {
				mstruct.setToChild(1, true);
			} else if(mstruct.size() == 0) {
				if(mtruefor) delete mtruefor;
				if(b_partial) ierror = 4;
				else ierror = 5;
				mstruct = mcopy;
				continue;
			} else {
				mstruct.setType(STRUCT_VECTOR);
			}
			if(itry > 1) {
				assumptions->setSign(as);
				if(itry == 2) {
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed sign was therefore temporarily set as unknown."), format_and_print(y_var).c_str(), NULL);
				} else if(itry == 3) {
					assumptions->setType(at);
					CALCULATOR->error(false, _("Was unable to isolate %s with the current assumptions. The assumed type and sign was therefore temporarily set as unknown."), format_and_print(y_var).c_str(), NULL);
				}
				if(assumptions_added) ((UnknownVariable*) y_var.variable())->setAssumptions(NULL);
			}

			if(mconditions.size() == 1) {
				if(mconditions[0]) {
					CALCULATOR->error(false, _("The solution requires that %s."), format_and_print(*mconditions[0]).c_str(), NULL);
					delete mconditions[0];
				}
			} else {
				string sconditions;
				for(size_t i = 0; i < mconditions.size(); i++) {
					if(mconditions[i]) {
						CALCULATOR->error(false, _("Solution %s requires that %s."), i2s(i + 1).c_str(), format_and_print(*mconditions[i]).c_str(), NULL);
						delete mconditions[i];
					}
				}
			}
			if(mtruefor) {
				CALCULATOR->error(false, _("The comparison is true for all %s if %s."), format_and_print(y_var).c_str(), format_and_print(*mtruefor).c_str(), NULL);
				delete mtruefor;
			}
			return 1;
		} else {
			ierror = 6;
		}
	}
	return -1;

}

int SolveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	return solve_equation(mstruct, vargs[0], vargs[1], eo);
}

SolveMultipleFunction::SolveMultipleFunction() : MathFunction("multisolve", 2) {
	setArgumentDefinition(1, new VectorArgument());
	VectorArgument *arg = new VectorArgument();
	arg->addArgument(new SymbolicArgument());
	arg->setReoccuringArguments(true);
	setArgumentDefinition(2, arg);
	setCondition("dimension(\\x) = dimension(\\y)");
}
int SolveMultipleFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {

	mstruct.clearVector();

	if(vargs[1].size() < 1) return 1;

	vector<bool> eleft;
	eleft.resize(vargs[0].size(), true);
	vector<size_t> eorder;
	bool b = false;
	for(size_t i = 0; i < vargs[1].size(); i++) {
		b = false;
		for(size_t i2 = 0; i2 < vargs[0].size(); i2++) {
			if(eleft[i2] && vargs[0][i2].contains(vargs[1][i], true)) {
				eorder.push_back(i2);
				eleft[i2] = false;
				b = true;
				break;
			}
		}
		if(!b) {
			eorder.clear();
			for(size_t i2 = 0; i2 < vargs[0].size(); i2++) {
				eorder.push_back(i2);
			}
			break;
		}
	}

	MathStructure mv; mv.clearVector();
	MathStructure mand; mand.resizeVector(eorder.size(), mv);
	if(mand.size() < eorder.size()) return 0;

	for(size_t i = 0; i < eorder.size(); i++) {
		MathStructure msolve(vargs[0][eorder[i]]);
		EvaluationOptions eo2 = eo;
		eo2.isolate_var = &vargs[1][i];
		for(size_t i2 = 0; i2 < i; i2++) {
			if(mstruct[i2].isVector()) {
				msolve.transform(STRUCT_LOGICAL_OR);
				for(size_t i4 = 1; i4 < mstruct[i2].size(); i4++) msolve.addChild(msolve[0]);
				for(size_t i4 = 0; i4 < mstruct[i2].size(); i4++) {
					msolve[i4].replace(vargs[1][i2], mstruct[i2][i4]);
				}
			} else {
				msolve.replace(vargs[1][i2], mstruct[i2]);
			}
		}
		msolve.eval(eo2);
		if(msolve.isComparison()) {
			if(msolve[0] != vargs[1][i]) {
				if(!b) {
					CALCULATOR->error(true, _("Unable to isolate %s.\n\nYou might need to place the equations and variables in an appropriate order so that each equation at least contains the corresponding variable (if automatic reordering failed)."), format_and_print(vargs[1][i]).c_str(), NULL);
				} else {
					CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
				}
				return 0;
			} else {
				if(msolve.comparisonType() == COMPARISON_EQUALS) {
					mstruct.addChild(msolve[1]);
				} else {
					CALCULATOR->error(true, _("Inequalities are not allowed in %s()."), preferredName().name.c_str(), NULL);
					return 0;
				}
			}
		} else if(msolve.isLogicalOr()) {
			for(size_t i2 = 0; i2 < msolve.size(); i2++) {
				if(msolve[i2].isLogicalAnd()) {
					size_t i_solve = 0;
					b = false;
					for(size_t i4 = 0; i4 < msolve[i2].size(); i4++) {
						if(!b && msolve[i2][i4].isComparison() && msolve[i2][i4].comparisonType() == COMPARISON_EQUALS && msolve[i2][i4][0] == vargs[1][i]) {
							msolve[i2][i4].setToChild(2, true);
							i_solve = i4;
							b = true;
						} else if(msolve[i2][i4].isComparison()) {
							bool b2 = false;
							for(size_t i3 = 0; i3 < vargs[1].size(); i3++) {
								if(msolve[i2][i4][0] == vargs[1][i3]) {
									mand[i3].addChild(msolve[i2][i4]);
									b2 = true;
									break;
								}
							}
							if(!b2) {b = false; break;}
						} else {
							b = false;
							break;
						}
					}
					if(!b) {
						CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
						return 0;
					}
					msolve[i2].setToChild(i_solve + 1, true);
				} else if(msolve[i2].isComparison() && msolve[i2].comparisonType() == COMPARISON_EQUALS && msolve[i2][0] == vargs[1][i]) {
					msolve[i2].setToChild(2, true);
				} else {
					CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
					return 0;
				}
			}
			msolve.setType(STRUCT_VECTOR);
			mstruct.addChild(msolve);
		} else if(msolve.isLogicalAnd() && msolve[0].isComparison()) {
			b = false;
			for(size_t i2 = 0; i2 < msolve.size(); i2++) {
				if(!b && msolve[i2].isComparison() && msolve[i2].comparisonType() == COMPARISON_EQUALS && msolve[i2][0] == vargs[1][i]) {
					mstruct.addChild(msolve[i2][1]);
					b = true;
				} else if(msolve[i2].isComparison()) {
					bool b2 = false;
					for(size_t i3 = 0; i3 < vargs[1].size(); i3++) {
						if(msolve[i2][0] == vargs[1][i3]) {
							mand[i3].addChild(msolve[i2]);
							b2 = true;
							break;
						}
					}
					if(!b2) {b = false; break;}
				} else {
					b = false;
					break;
				}
			}
			if(!b) {
				CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
				return 0;
			}
		} else {
			CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
			return 0;
		}
		for(size_t i2 = 0; i2 < i; i2++) {
			for(size_t i3 = 0; i3 <= i; i3++) {
				if(i2 != i3) {
					size_t index1 = i2;
					size_t index2 = i3;
					if(mstruct[index2].isVector()) {
						MathStructure m;
						m.clearVector();
						m.resizeVector(mstruct[index2].size(), mstruct[index1]);
						if(m.size() < mstruct[index2].size()) return 0;
						for(size_t i4 = 0; i4 < mstruct[index2].size(); i4++) {
							m[i4].replace(vargs[1][index2], mstruct[index2][i4]);
						}
						m.flattenVector(mstruct[index1]);
					} else {
						mstruct[index1].replace(vargs[1][index2], mstruct[index2]);
					}
				}
			}
		}
	}

	for(size_t i = 0; i < mstruct.size(); i++) {
		if(mstruct[i].isVector()) {
			for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
				for(size_t i3 = 0; i3 < mand[i].size(); i3++) {
					mand[i][i3][0] = mstruct[i][i2];
					CALCULATOR->beginTemporaryStopMessages();
					mand[i][i3].eval(eo);
					CALCULATOR->endTemporaryStopMessages();
					if(!mand[i][i3].isOne()) {
						CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
						return 0;
					}
				}
				for(size_t i3 = i2 + 1; i3 < mstruct[i].size();) {
					ComparisonResult cr = mstruct[i][i2].compare(mstruct[i][i3]);
					if(cr == COMPARISON_RESULT_EQUAL || cr == COMPARISON_RESULT_EQUAL_LIMITS) {
						mstruct[i].delChild(i3 + 1);
					} else {
						i3++;
					}
				}
				if(mstruct[i].size() == 1) {
					mstruct[i].setToChild(1, true);
					break;
				}
			}
		} else {
			for(size_t i3 = 0; i3 < mand[i].size(); i3++) {
				mand[i][i3][0] = mstruct[i];
				CALCULATOR->beginTemporaryStopMessages();
				mand[i][i3].eval(eo);
				CALCULATOR->endTemporaryStopMessages();
				if(!mand[i][i3].isOne()) {
					CALCULATOR->error(true, _("Unable to isolate %s."), format_and_print(vargs[1][i]).c_str(), NULL);
					return 0;
				}
			}
		}
	}

	return 1;

}

MathStructure *find_deqn(MathStructure &mstruct);
MathStructure *find_deqn(MathStructure &mstruct) {
	if(mstruct.isFunction() && mstruct.function()->id() == FUNCTION_ID_DIFFERENTIATE) return &mstruct;
	for(size_t i = 0; i < mstruct.size(); i++) {
		MathStructure *m = find_deqn(mstruct[i]);
		if(m) return m;
	}
	return NULL;
}

bool contains_ignore_diff(const MathStructure &m, const MathStructure &mstruct, const MathStructure &mdiff);
bool contains_ignore_diff(const MathStructure &m, const MathStructure &mstruct, const MathStructure &mdiff) {
	if(m.equals(mstruct)) return true;
	if(m.equals(mdiff)) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_ignore_diff(m[i], mstruct, mdiff)) return true;
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		return contains_ignore_diff(((KnownVariable*) m.variable())->get(), mstruct, mdiff);
	} else if(m.isVariable()) {
		if(mstruct.isNumber() || !m.representsNumber()) return true;
	} else if(m.isAborted()) {
		return true;
	}
	return false;
}

void add_C(MathStructure &m_eqn, const MathStructure &m_x, const MathStructure &m_y, const MathStructure &x_value, const MathStructure &y_value) {
	if(!y_value.isUndefined() && !x_value.isUndefined()) {
		MathStructure m_c(m_eqn);
		m_c.replace(m_x, x_value);
		m_c.replace(m_y, y_value);
		m_c.setType(STRUCT_ADDITION);
		m_c[1].negate();
		m_c.childUpdated(2);
		m_eqn[1] += m_c;
	} else {
		m_eqn[1] += CALCULATOR->getVariableById(VARIABLE_ID_C);
	}
	m_eqn.childrenUpdated();
}

bool dsolve(MathStructure &m_eqn, const EvaluationOptions &eo, const MathStructure &m_diff, const MathStructure &y_value, const MathStructure &x_value) {
	MathStructure m_y(m_diff[0]), m_x(m_diff[1]);
	bool b = false;
	if(m_eqn[0] == m_diff) {
		if(m_eqn[1].containsRepresentativeOf(m_y, true, true) == 0) {
			// y'=f(x)
			MathStructure m_fx(m_eqn[1]);
			if(m_fx.integrate(m_x, eo, true, false) > 0) {
				m_eqn[0] = m_y;
				m_eqn[1] = m_fx;
				b = true;
			}
		} else if(m_eqn[1].containsRepresentativeOf(m_x, true, true) == 0) {
			MathStructure m_fy(m_eqn[1]);
			m_fy.inverse();
			if(m_fy.integrate(m_y, eo, true, false) > 0) {
				m_eqn[0] = m_fy;
				m_eqn[1] = m_x;
				b = true;
			}
		} else if(m_eqn[1].isMultiplication() && m_eqn[1].size() >= 2) {
			b = true;
			MathStructure m_fx(1, 1, 0), m_fy(1, 1, 0);
			for(size_t i = 0; i < m_eqn[1].size(); i++) {
				if(m_eqn[1][i].containsRepresentativeOf(m_y, true, true) != 0) {
					if(m_eqn[1][i].containsRepresentativeOf(m_x, true, true) != 0) {
						b = false;
						break;
					}
					if(m_fy.isOne()) m_fy = m_eqn[1][i];
					else m_fy.multiply(m_eqn[1][i], true);
				} else {
					if(m_fx.isOne()) m_fx = m_eqn[1][i];
					else m_fx.multiply(m_eqn[1][i], true);
				}
			}
			if(b) {
				// y'=f(x)*f(y)
				m_fy.inverse();
				if(m_fy.integrate(m_y, eo, true, false)  > 0 && m_fx.integrate(m_x, eo, true, false) > 0) {
					m_eqn[0] = m_fy;
					m_eqn[1] = m_fx;
				} else {
					b = false;
				}
			}
		} else {
			MathStructure mfactor(m_eqn);
			mfactor[1].factorize(eo, false, 0, 0, false, false, NULL, m_x);
			if(mfactor[1].isMultiplication() && mfactor[1].size() >= 2) {
				mfactor.childUpdated(2);
				if(dsolve(mfactor, eo, m_diff, y_value, x_value)) {
					m_eqn = mfactor;
					return 1;
				}
			}
			if(m_eqn[1].isAddition()) {
				MathStructure m_left;
				MathStructure m_muly;
				MathStructure m_mul_exp;
				MathStructure m_exp;
				b = true;
				for(size_t i = 0; i < m_eqn[1].size(); i++) {
					if(m_eqn[1][i] == m_y) {
						if(m_muly.isZero()) m_muly = m_one;
						else m_muly.add(m_one, true);
					} else if(m_eqn[1][i].containsRepresentativeOf(m_y, true, true) != 0) {
						if(m_left.isZero() && m_eqn[1][i].isPower() && m_eqn[1][i][0] == m_y && (m_mul_exp.isZero() || m_eqn[1][i][1] == m_exp)) {
							if(m_mul_exp.isZero()) {
								m_exp = m_eqn[1][i][1];
								m_mul_exp = m_one;
							} else {
								m_mul_exp.add(m_one, true);
							}
						} else if(m_eqn[1][i].isMultiplication()) {
							size_t i_my = 0;
							bool b2 = false, b2_exp = false;
							for(size_t i2 = 0; i2 < m_eqn[1][i].size(); i2++) {
									if(!b2 && m_eqn[1][i][i2] == m_y) {
									i_my = i2;
									b2 = true;
								} else if(!b2 && m_left.isZero() && m_eqn[1][i][i2].isPower() && m_eqn[1][i][i2][0] == m_y && (m_mul_exp.isZero() || m_eqn[1][i][i2][1] == m_exp)) {
									i_my = i2;
									b2 = true;
									b2_exp = true;
								} else if(m_eqn[1][i][i2].containsRepresentativeOf(m_y, true, true) != 0) {
									b2 = false;
									break;
								}
							}
							if(b2) {
								MathStructure m_a(m_eqn[1][i]);
								m_a.delChild(i_my + 1, true);
								if(b2_exp) {
									if(m_mul_exp.isZero()) {
										m_exp = m_eqn[1][i][i_my][1];
										m_mul_exp = m_a;
									} else {
										m_mul_exp.add(m_a, true);
									}
								} else {
									if(m_muly.isZero()) m_muly = m_a;
									else m_muly.add(m_a, true);
								}
							} else {
								b = false;
								break;
							}
						} else {
							b = false;
							break;
						}
					} else {
						if(!m_mul_exp.isZero()) {
							b = false;
							break;
						}
						if(m_left.isZero()) m_left = m_eqn[1][i];
						else m_left.add(m_eqn[1][i], true);
					}
				}
				if(b && !m_muly.isZero()) {
					if(!m_mul_exp.isZero()) {
						if(m_exp.isOne() || !m_left.isZero()) return false;
						// y' = f(x)*y+g(x)*y^c
						b = false;
						m_muly.calculateNegate(eo);
						MathStructure m_y1_integ(m_muly);
						if(m_y1_integ.integrate(m_x, eo, true, false) > 0) {
							m_exp.negate();
							m_exp += m_one;
							MathStructure m_y1_exp(m_exp);
							m_y1_exp *= m_y1_integ;
							m_y1_exp.transform(STRUCT_POWER, CALCULATOR->getVariableById(VARIABLE_ID_E));
							m_y1_exp.swapChildren(1, 2);
							MathStructure m_y1_exp_integ(m_y1_exp);
							m_y1_exp_integ *= m_mul_exp;
							if(m_y1_exp_integ.integrate(m_x, eo, true, false) > 0) {
								m_eqn[1] = m_exp;
								m_eqn[1] *= m_y1_exp_integ;
								m_eqn[0] = m_y;
								m_eqn[0] ^= m_exp;
								m_eqn[0] *= m_y1_exp;
								add_C(m_eqn, m_x, m_y, x_value, y_value);
								m_eqn[0].delChild(m_eqn[0].size());
								m_eqn[1] /= m_y1_exp;
								m_eqn.childrenUpdated();
								return 1;
							}
						}
					} else if(m_left.isZero()) {
						// y'=f(x)*y+g(x)*y
						MathStructure mtest(m_eqn);
						MathStructure m_fy(m_y);
						m_fy.inverse();
						MathStructure m_fx(m_muly);
						if(m_fy.integrate(m_y, eo, true, false) > 0 && m_fx.integrate(m_x, eo, true, false) > 0) {
							m_eqn[0] = m_fy;
							m_eqn[1] = m_fx;
							b = true;
						}
					} else {
						// y'=f(x)*y+g(x)
						MathStructure integ_fac(m_muly);
						integ_fac.negate();
						if(integ_fac.integrate(m_x, eo, true, false) > 0) {
							UnknownVariable *var = new UnknownVariable("", "u");
							Assumptions *ass = new Assumptions();
							if(false) {
								ass->setType(ASSUMPTION_TYPE_REAL);
							}
							var->setAssumptions(ass);
							MathStructure m_u(var);
							m_u.inverse();
							if(m_u.integrate(var, eo, false, false) > 0) {
								MathStructure m_eqn2(integ_fac);
								m_eqn2.transform(COMPARISON_EQUALS, m_u);
								m_eqn2.isolate_x(eo, var);
								if(m_eqn2.isComparison() && m_eqn2.comparisonType() == COMPARISON_EQUALS && m_eqn2[0] == var) {
									integ_fac = m_eqn2[1];
									MathStructure m_fx(m_left);
									m_fx *= integ_fac;
									if(m_fx.integrate(m_x, eo, true, false) > 0) {
										m_eqn[0] = m_y;
										m_eqn[0] *= integ_fac;
										m_eqn[1] = m_fx;
										add_C(m_eqn, m_x, m_y, x_value, y_value);
										m_eqn[0] = m_y;
										m_eqn[1] /= integ_fac;
										m_eqn.childrenUpdated();
										return 1;
									}
								} else if(m_eqn2.isLogicalOr() && m_eqn2.size() >= 2) {
									b = true;
									for(size_t i = 0; i < m_eqn2.size(); i++) {
										if(!m_eqn2[i].isComparison() || m_eqn2[i].comparisonType() != COMPARISON_EQUALS || m_eqn2[i][0] != var) {
											b = false;
											break;
										}
									}
									if(b) {
										MathStructure m_eqn_new;
										m_eqn_new.setType(STRUCT_LOGICAL_OR);
										for(size_t i = 0; i < m_eqn2.size(); i++) {
											integ_fac = m_eqn2[i][1];
											MathStructure m_fx(m_left);
											m_fx *= integ_fac;
											if(m_fx.integrate(m_x, eo, true, false) > 0) {
												MathStructure m_fy(m_y);
												m_fy *= integ_fac;
												m_eqn_new.addChild(m_fy);
												m_eqn_new.last().transform(COMPARISON_EQUALS, m_fx);
												add_C(m_eqn_new.last(), m_x, m_y, x_value, y_value);
												m_eqn_new.last()[0] = m_y;
												m_eqn_new.last()[1] /= integ_fac;
												m_eqn_new.last().childrenUpdated();
												m_eqn_new.childUpdated(m_eqn_new.size());
											} else {
												b = false;
												break;
											}
										}
										if(b) {
											m_eqn_new.childrenUpdated();
											m_eqn = m_eqn_new;
											return 1;
										}
									}
								}
							}
							var->destroy();
						}
					}
				} else {
					b = false;
				}
			}
		}
		if(b) {
			add_C(m_eqn, m_x, m_y, x_value, y_value);
			return 1;
		}
	}
	return false;
}
void protect_mdiff(MathStructure &m, const MathStructure &mdiff, const EvaluationOptions &eo, bool b_top = true) {
	if(m == mdiff) {
		m.setProtected(true);
	} else {
		for(size_t i = 0; i < m.size(); i++) {
			protect_mdiff(m[i], mdiff, eo, false);
		}
	}
	if(b_top && eo.isolate_x) {
		EvaluationOptions eo2 = eo;
		eo2.isolate_var = &mdiff;
		m.eval(eo2);
		m.setProtected(true);
	}
}

DSolveFunction::DSolveFunction() : MathFunction("dsolve", 1, 3) {
	setDefaultValue(2, "undefined");
	setDefaultValue(3, "0");
}
int DSolveFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	MathStructure m_eqn(vargs[0]);
	EvaluationOptions eo2 = eo;
	eo2.isolate_x = false;
	eo2.protected_function = CALCULATOR->getFunctionById(FUNCTION_ID_DIFFERENTIATE);
	m_eqn.eval(eo2);
	MathStructure *m_diff_p = NULL;
	if(m_eqn.isLogicalAnd()) {
		for(size_t i = 0; i < m_eqn.size(); i++) {
			if(m_eqn[i].isComparison() && m_eqn.comparisonType() == COMPARISON_EQUALS) {
				m_diff_p = find_deqn(m_eqn[i]);
				if(m_diff_p) break;
			}
		}
	} else if(m_eqn.isComparison() && m_eqn.comparisonType() == COMPARISON_EQUALS) {
		m_diff_p = find_deqn(m_eqn);
	}
	if(!m_diff_p) {
		CALCULATOR->error(true, _("No differential equation found."), NULL);
		mstruct = m_eqn; return -1;
	}
	MathStructure m_diff(*m_diff_p);
	if(m_diff.size() < 3 || (!m_diff[0].isSymbolic() && !m_diff[0].isVariable()) || (!m_diff[1].isSymbolic() && !m_diff[1].isVariable()) || !m_diff[2].isInteger() || !m_diff[2].number().isPositive() || !m_diff[2].number().isLessThanOrEqualTo(10)) {
		CALCULATOR->error(true, _("No differential equation found."), NULL);
		mstruct = m_eqn; return -1;
	}
	if(m_diff[2].number().intValue() != 1) {
		CALCULATOR->error(true, _("Unable to solve differential equation."), NULL);
		mstruct = m_eqn;
		protect_mdiff(mstruct, m_diff, eo);
		return -1;
	}
	m_eqn.isolate_x(eo2, m_diff);
	mstruct = m_eqn;
	if(eo.approximation == APPROXIMATION_TRY_EXACT) eo2.approximation = APPROXIMATION_EXACT;
	if(m_eqn.isLogicalAnd()) {
		for(size_t i = 0; i < m_eqn.size(); i++) {
			if(m_eqn[i].isComparison() && m_eqn[i].comparisonType() == COMPARISON_EQUALS && m_eqn[i][0] == m_diff) {
				dsolve(m_eqn[i], eo2, m_diff, vargs[1], vargs[2]);
			}
		}
	} else if(m_eqn.isLogicalOr()) {
		for(size_t i = 0; i < m_eqn.size(); i++) {
			if(m_eqn[i].isComparison() && m_eqn[i].comparisonType() == COMPARISON_EQUALS && m_eqn[i][0] == m_diff) {
				dsolve(m_eqn[i], eo2, m_diff, vargs[1], vargs[2]);
			} else if(m_eqn[i].isLogicalAnd()) {
				for(size_t i2 = 0; i2 < m_eqn[i].size(); i2++) {
					if(m_eqn[i][i2].isComparison() && m_eqn[i][i2].comparisonType() == COMPARISON_EQUALS && m_eqn[i][i2][0] == m_diff) {
						dsolve(m_eqn[i][i2], eo2, m_diff, vargs[1], vargs[2]);
					}
				}
			}
		}
	} else if(m_eqn.isComparison() && m_eqn.comparisonType() == COMPARISON_EQUALS && m_eqn[0] == m_diff) {
		dsolve(m_eqn, eo2, m_diff, vargs[1], vargs[2]);
	}
	if(m_eqn.contains(m_diff)) {
		CALCULATOR->error(true, _("Unable to solve differential equation."), NULL);
		protect_mdiff(mstruct, m_diff, eo);
		return -1;
	}
	m_eqn.calculatesub(eo2, eo2, true);
	MathStructure msolve(m_eqn);
	if(solve_equation(msolve, m_eqn, m_diff[0], eo, true, m_diff[1], MathStructure(CALCULATOR->getVariableById(VARIABLE_ID_C)), vargs[2], vargs[1]) <= 0) {
		CALCULATOR->error(true, _("Unable to solve differential equation."), NULL);
		protect_mdiff(mstruct, m_diff, eo);
		return -1;
	}
	mstruct = msolve;
	return 1;
}

NewtonRaphsonFunction::NewtonRaphsonFunction() : MathFunction("newtonsolve", 2, 5) {
	setArgumentDefinition(2, new NumberArgument());
	setArgumentDefinition(3, new SymbolicArgument());
	setDefaultValue(3, "undefined");
	setArgumentDefinition(4, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SINT));
	setDefaultValue(4, "-10");
	setArgumentDefinition(5, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setDefaultValue(5, "1000");
}
int NewtonRaphsonFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	int ret = 0;
	MathStructure mfunc(vargs[0]);
	if(mfunc.isComparison() && mfunc.comparisonType() == COMPARISON_EQUALS) {
		mfunc[1].ref();
		mfunc[0].subtract_nocopy(&mfunc[1]);
		mfunc.setToChild(1);
		mstruct = mfunc;
		ret = -1;
	}
	bool compare_with_1 = false;
	calculate_userfunctions(mfunc, vargs[2], eo);
	MathStructure mdiff(mfunc);
	mdiff.differentiate(vargs[2], eo);
	if(mdiff.containsFunction(CALCULATOR->getFunctionById(FUNCTION_ID_DIFFERENTIATE))) return ret;
	mfunc /= mdiff;
	Number nr_prec(1, 1, vargs[3].number() <= 0 ? -(PRECISION - vargs[3].number().intValue()) : -vargs[3].number().intValue());
	int precbak = PRECISION;
	nrf_begin:
	CALCULATOR->beginTemporaryStopMessages();
	Number x_i(vargs[1].number()), x_itest;
	x_i.setToFloatingPoint();
	MathStructure x_if;
	unsigned int iter = 0;
	unsigned int max_iter = vargs[4].number().uintValue();
	while(true) {
		if(CALCULATOR->aborted()) break;
		x_if = mfunc;
		x_if.replace(vargs[2], x_i);
		x_if.eval(eo);
		if(!x_if.isNumber()) break;
		if(x_if.isZero() && x_i.isZero()) {ret = 1; break;}
		if(iter > 0) x_itest = x_if.number();
		if((iter > 0 && !compare_with_1 && !x_itest.divide(x_i)) || !x_itest.abs() || !x_i.subtract(x_if.number())) break;
		if(iter > 0) {
			if(x_i.hasImaginaryPart()) {
				if((x_itest.realPart() < nr_prec && x_itest.imaginaryPart() < nr_prec) || (!x_i.isNonZero() && (x_i.realPart() < nr_prec && x_i.imaginaryPart() < nr_prec && x_if.number().realPart() < nr_prec && x_if.number().imaginaryPart() < nr_prec))) {
					x_i.setUncertainty(x_if.number(), !CALCULATOR->usesIntervalArithmetic());
					ret = 1;
					break;
				}
				if(!compare_with_1 && iter > 10 && x_i.realPart().isFraction() && x_i.imaginaryPart().isFraction() && (x_itest.realPart() > Number(9, 10) || x_itest.imaginaryPart() > Number(9, 10))) {
					compare_with_1 = true;
				}
				int prec = x_i.precision(true);
				if(!x_i.isNonZero() || (prec >= 0 && prec < precbak && PRECISION * 5 < 1000)) {
					if(!compare_with_1 && x_i.realPart().isFraction() && x_i.imaginaryPart().isFraction() && (x_itest.realPart() > Number(9, 10) || x_itest.imaginaryPart() > Number(9, 10))) {
						compare_with_1 = true;
					}
					CALCULATOR->setPrecision(PRECISION * 10 > 1000 ? 1000 : PRECISION * 10);
					CALCULATOR->endTemporaryStopMessages();
					goto nrf_begin;
				}
			} else {
				if(x_itest < nr_prec || (!x_i.isNonZero() && x_i < nr_prec && x_if.number() < nr_prec)) {
					x_i.setUncertainty(x_if.number(), !CALCULATOR->usesIntervalArithmetic());
					ret = 1;
					break;
				}
				if(!compare_with_1 && iter > 10 && x_i.isFraction() && x_itest > Number(9, 10)) {
					compare_with_1 = true;
				}
				int prec = x_i.precision(true);
				if(!x_i.isNonZero() || (prec >= 0 && prec < precbak && PRECISION * 5 < 1000)) {
					if(!compare_with_1 && x_i.isFraction() && x_itest > Number(9, 10)) {
						compare_with_1 = true;
					}
					CALCULATOR->setPrecision(PRECISION * 10 > 1000 ? 1000 : PRECISION * 10);
					CALCULATOR->endTemporaryStopMessages();
					goto nrf_begin;
				}
			}
		}
		iter++;
		if(iter > max_iter) {
			x_i.setUncertainty(x_if.number(), !CALCULATOR->usesIntervalArithmetic());
			int prec = x_i.precision(true);
			if(prec < 5) break;
			ret = 1;
			break;
		}
	}
	CALCULATOR->endTemporaryStopMessages(true);
	if(precbak != PRECISION) CALCULATOR->setPrecision(precbak);
	if(ret == 1) mstruct = x_i;
	return ret;
}
SecantMethodFunction::SecantMethodFunction() : MathFunction("secantsolve", 3, 6) {
	setArgumentDefinition(2, new NumberArgument());
	setArgumentDefinition(3, new NumberArgument());
	setArgumentDefinition(4, new SymbolicArgument());
	setDefaultValue(4, "undefined");
	setArgumentDefinition(5, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_SINT));
	setDefaultValue(5, "-10");
	setArgumentDefinition(6, new IntegerArgument("", ARGUMENT_MIN_MAX_NONE, true, true, INTEGER_TYPE_UINT));
	setDefaultValue(6, "1000");
}
int SecantMethodFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo) {
	int ret = 0;
	MathStructure mfunc(vargs[0]);
	if(mfunc.isComparison() && mfunc.comparisonType() == COMPARISON_EQUALS) {
		mfunc[1].ref();
		mfunc[0].subtract_nocopy(&mfunc[1]);
		mfunc.setToChild(1);
		mstruct = mfunc;
		ret = -1;
	}
	bool compare_with_1 = false;
	calculate_userfunctions(mfunc, vargs[3], eo);
	Number nr_prec(1, 1, vargs[4].number() <= 0 ? -(PRECISION - vargs[4].number().intValue()) : -vargs[4].number().intValue());
	int precbak = PRECISION;
	secm_begin:
	CALCULATOR->beginTemporaryStopMessages();
	Number x0(vargs[1].number());
	x0.setToFloatingPoint();
	MathStructure m_if(mfunc);
	m_if.replace(vargs[3], x0);
	m_if.eval(eo);
	Number x_i(vargs[2].number()), x_itest, x_fi;
	if(m_if.isNumber()) {
		Number f0(m_if.number());
		x_i.setToFloatingPoint();
		unsigned int iter = 0;
		unsigned int max_iter = vargs[5].number().uintValue();
		while(true) {
			if(CALCULATOR->aborted()) break;
			m_if = mfunc;
			m_if.replace(vargs[3], x_i);
			m_if.eval(eo);
			if(!m_if.isNumber() || !x0.negate() || !x0.add(x_i) || !f0.negate() || !f0.add(m_if.number()) || !x0.divide(f0) || !x0.multiply(m_if.number())) break;
			x_fi = x0;
			x0 = x_i;
			f0 = m_if.number();
			if(x_fi.isZero() && x_i.isZero()) {ret = 1; break;}
			if(iter > 0) x_itest = x_fi;
			if((iter > 0 && !compare_with_1 && !x_itest.divide(x_i)) || !x_itest.abs() || !x_i.subtract(x_fi)) break;
			if(iter > 0) {
				if(x_i.hasImaginaryPart()) {
					if((x_itest.realPart() < nr_prec && x_itest.imaginaryPart() < nr_prec) || (!x_i.isNonZero() && (x_i.realPart() < nr_prec && x_i.imaginaryPart() < nr_prec && x_fi.realPart() < nr_prec && x_fi.imaginaryPart() < nr_prec))) {
						x_i.setUncertainty(x_fi, !CALCULATOR->usesIntervalArithmetic());
						ret = 1;
						break;
					}
					if(!compare_with_1 && iter > 10 && x_i.realPart().isFraction() && x_i.imaginaryPart().isFraction() && (x_itest.realPart() > Number(9, 10) || x_itest.imaginaryPart() > Number(9, 10))) {
						compare_with_1 = true;
					}
					int prec = x_i.precision(true);
					if(!x_i.isNonZero() || (prec >= 0 && prec < precbak && PRECISION * 5 < 1000)) {
						if(!compare_with_1 && x_i.realPart().isFraction() && x_i.imaginaryPart().isFraction() && (x_itest.realPart() > Number(9, 10) || x_itest.imaginaryPart() > Number(9, 10))) {
							compare_with_1 = true;
						}
						CALCULATOR->setPrecision(PRECISION * 10 > 1000 ? 1000 : PRECISION * 10);
						CALCULATOR->endTemporaryStopMessages();
						goto secm_begin;
					}
				} else {
					if(x_itest < nr_prec || (!x_i.isNonZero() && x_i < nr_prec && x_fi < nr_prec)) {
						x_i.setUncertainty(x_fi, !CALCULATOR->usesIntervalArithmetic());
						ret = 1;
						break;
					}
					if(!compare_with_1 && iter > 10 && x_i.isFraction() && x_itest > Number(9, 10)) {
						compare_with_1 = true;
					}
					int prec = x_i.precision(true);
					if(!x_i.isNonZero() || (prec >= 0 && prec < precbak && PRECISION * 5 < 1000)) {
						if(!compare_with_1 && x_i.isFraction() && x_itest > Number(9, 10)) {
							compare_with_1 = true;
						}
						CALCULATOR->setPrecision(PRECISION * 10 > 1000 ? 1000 : PRECISION * 10);
						CALCULATOR->endTemporaryStopMessages();
						goto secm_begin;
					}
				}
			}
			iter++;
			if(iter > max_iter) {
				x_i.setUncertainty(x_fi, !CALCULATOR->usesIntervalArithmetic());
				int prec = x_i.precision(true);
				if(prec < 5) break;
				ret = 1;
				break;
			}
		}
	}
	CALCULATOR->endTemporaryStopMessages(true);
	if(precbak != PRECISION) CALCULATOR->setPrecision(precbak);
	if(ret == 1) mstruct = x_i;
	return ret;
}

